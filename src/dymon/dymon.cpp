#include <stdio.h>
#include <cstring>
#include <iostream>
#include "dymon.h"


//helper functions
static int getFileContent(const char * filename, uint8_t * buffer, size_t bufferSize); //read file
static uint32_t parsePortableBitmapP4(const char * file, uint32_t * h, uint32_t * w);


const uint8_t Dymon::_status[] = {
   0x1B, 0x41, 1           //status request
};

#define CONFIGURATION_OFFSET_SESSION         (2)
#define CONFIGURATION_OFFSET_LABEL_LENGTH    (11) //label length must be patched int bytes [11,12]
const uint8_t Dymon::_configuration[] = {
   0x1B, 0x73, 1, 0, 0, 0, //counter (session)
   0x1B, 0x43, 0x64,       //print density "normal"
   0x1B, 0x4C,             //label length...
   0, 0,                   //...given as multiple of 1/600 inch. (e.g. 600 ^= 600 * 1/600 inch = 1 inch) (16-bit little endian)
   0x1B, 0x68,             //print quality, 300x300dpi
   0x1B, 0x4D, 0, 0, 0, 0, 0, 0, 0, 0, //media type, default
   0x1B, 0x68
};

#define LABEL_INDEX_OFFSET    (2) //label index must be patched int bytes [2,3]
const uint8_t Dymon::_labelIndex[] = {
   0x1B, 0x6E, 1, 0,       //label index
};

#define LABEL_HEIGHT_OFFSET    (4) //label height must be patched int bytes [4..7]
#define LABEL_WIDTH_OFFSET     (8) //label height must be patched int bytes [8..11]
const uint8_t Dymon::_labelHeightWidth[] = { //must be prefixed to the labels bitmap blob
   0x1B, 0x44, 0x01, 0x02,
   0, 0, 0, 0,             //label height in pixel (32-bit little endian)
   0, 0, 0, 0              //label width in pixel (32-bit little endian). shall be a multiple of 8!
};

const uint8_t Dymon::_labelFeed[] = { //must be suffixed to the labels bitmap blob (because _labelHeightWidth+bitmap+_labelLineFeed must be sent in one unit)
   0x1B, 0x47,             //short line feed
};

const uint8_t Dymon::_labelStatus[] = {
   0x1B, 0x41, 0           //status request
};

const uint8_t Dymon::_final[] = {
   0x1B, 0x45,             //line feed
   0x1B, 0x51              //line tab
};


//Hexdump of the 32 Status bytes returned from printer
static void log_status(int i, const uint8_t * status, uint32_t count)
{
   static const char * const hex = "0123456789ABCDEF";
   std::cout << "Status" << i << " (" << count << "): ";
   for (uint32_t j = 0; j < count; ++j)
   {
      std::cout << hex[(status[j] >> 4)] << hex[(status[j] & 0xF)] << " ";
   }
   std::cout << std::endl;
}



int Dymon::start(const char * host, uint16_t port)
{
   uint8_t receiveBuffer[128];
   int status;

   //check host address
   if (host == nullptr)
   {
      return -1;
   }
   //create TCP socket and connect to LabelWriter
   status = this->connect(host, port);
   if (status == 0)
   {
      return -2;
   }
   //request LabelWriter status
   status = this->send(_status, sizeof(_status));
   if (status <= 0)
   {
      return -3;
   }
   status = this->receive(receiveBuffer, sizeof(receiveBuffer));
   // if (receiveBuffer[15] == 1) -> papier ist alle!!! TODO
   if (status <= 0)
   {
      return -4;
   }
#ifdef DYMON_DEBUG
   log_status(0, receiveBuffer, status);
#endif
   //success
   index = 0;
   return 0;
}



//bitmap resolution is assumed to be 300dpi
int Dymon::print(const Bitmap * bitmap, double labelLength1mm)
{
   uint8_t buffer[1460]; //this it the (typical) maximal payload size of a tcp packet.
   int status;


   //parameter check
   if (bitmap == nullptr)
   {
      return -10;
   }
   const uint16_t length = (uint16_t)(((600.0 * labelLength1mm) / 25.4) + 0.5); //calculate label length in 600dpi unit
   if (length == 0)
   {
      return -11;
   }


   //for the first label (after call to start) we have to send the configuration (like the label length, print density, print quality, media type...)
   //for all further labels we assume that these values doesn't change!!!
   if (this->index == 0)
   {
      //send label configuration (includes the label length)
      memcpy(buffer, _configuration, sizeof(_configuration)); //make a modifiable copy
      buffer[CONFIGURATION_OFFSET_SESSION] = (uint8_t)this->session;
      buffer[CONFIGURATION_OFFSET_SESSION + 1] = (uint8_t)(this->session >> 8);
      buffer[CONFIGURATION_OFFSET_SESSION + 2] = (uint8_t)(this->session >> 16);
      buffer[CONFIGURATION_OFFSET_SESSION + 3] = (uint8_t)(this->session >> 24);
      buffer[CONFIGURATION_OFFSET_LABEL_LENGTH] = (uint8_t)length;
      buffer[CONFIGURATION_OFFSET_LABEL_LENGTH + 1] = (uint8_t)(length >> 8);
      status = this->send(buffer, sizeof(_configuration));
      if (status <= 0) return -12;
      this->index = 1;
      sleep1ms(10); //sleep to prevent the os to concatenate the following data to this TCP package
   }


   //send label index
   memcpy(buffer, _labelIndex, sizeof(_labelIndex)); //make a modifiable copy
   buffer[LABEL_INDEX_OFFSET] = (uint8_t)this->index;
   buffer[LABEL_INDEX_OFFSET + 1] = (uint8_t)(this->index >> 8);
   status = this->send(buffer, sizeof(_labelIndex));
   if (status <= 0) return -13;
   this->index++; //preset for the next
   sleep1ms(10); //sleep to prevent the os to concatenate the following data to this TCP package


   //send the label bitmap
   //the bitmap has to be sent as one blob with an header and an footer
   //for a performant implementation, i am sending in chunks of 1460 bytes, which is exactly the max. payload size of a TCP package
   //setup the header (contains bitmap height, width)
   memcpy(buffer, _labelHeightWidth, sizeof(_labelHeightWidth));
   //set bitmap height
   buffer[LABEL_HEIGHT_OFFSET] = (uint8_t)bitmap->height;
   buffer[LABEL_HEIGHT_OFFSET + 1] = (uint8_t)(bitmap->height >> 8);
   buffer[LABEL_HEIGHT_OFFSET + 2] = (uint8_t)(bitmap->height >> 16);
   buffer[LABEL_HEIGHT_OFFSET + 3] = (uint8_t)(bitmap->height >> 24);
   //set bitmap width
   buffer[LABEL_WIDTH_OFFSET] = (uint8_t)bitmap->width;
   buffer[LABEL_WIDTH_OFFSET + 1] = (uint8_t)(bitmap->width >> 8);
   buffer[LABEL_WIDTH_OFFSET + 2] = (uint8_t)(bitmap->width >> 16);
   buffer[LABEL_WIDTH_OFFSET + 3] = (uint8_t)(bitmap->width >> 24);
   //concat the bitmap data
   uint32_t offset = sizeof(_labelHeightWidth);
   for (uint32_t count = 0; count < bitmap->lengthByte; ++count)
   {
      if (offset >= sizeof(buffer)) //buffer full
      {
         //send chunk
         status = this->send(buffer, sizeof(buffer));
         if (status <= 0) return -14;
         offset = 0;
      }
      //copy bitmap byte into buffer
      buffer[offset++] = bitmap->data[count];
   }
   //send the remaining data (+ footer)
   if (offset >= sizeof(buffer)) //if buffer is full, i have to send first
   {
      //send chunk
      status = this->send(buffer, sizeof(buffer));
      if (status <= 0) return -15;
      offset = 0;
   }
   buffer[offset++] = _labelFeed[0];
   if (offset >= sizeof(buffer)) //if buffer is full, i have to send first
   {
      //send chunk
      status = this->send(buffer, sizeof(buffer));
      if (status <= 0) return -16;
      offset = 0;
   }
   buffer[offset++] = _labelFeed[1];
   status = this->send(buffer, offset);
   if (status <= 0) return -17;
   sleep1ms(10); //sleep to prevent the os to concatenate the following data to this TCP package


   //request LabelWriter status
   status = this->send(_labelStatus, sizeof(_labelStatus));
   if (status <= 0) return -18;
   status = this->receive(buffer, sizeof(buffer));
   // if (buffer[15] == 1) -> papier ist alle!!! TODO
   if (status <= 0) return -19;
#ifdef DYMON_DEBUG
   log_status(1, buffer, status);
#endif


   //success
   return 0;
}



int Dymon::print_bitmap(const char * file)
{
   int status;
   //parse the bitmap file
   uint32_t height;
   uint32_t width;
   uint32_t dataOffset = parsePortableBitmapP4(file, &height, &width);
   if (dataOffset == 0)
   {
      return -20; //input file invalid
   }
   //header parsed successfully
   const uint16_t length = (uint16_t)(2uL * height); //label length is based on 600 dots per inch (whereas width and height are 300dpi)
   if (length == 0)
   {
      return -21;
   }

   uint8_t buffer[1460]; //this it the (typical) maximal payload size of a tcp packet.

   //for the first label (after call to start) we have to send the configuration (like the label length, print density, print quality, media type...)
   //for all further labels we assume that these values doesn't change!!!
   if (this->index == 0)
   {
      //send label configuration (includes the label length)
      memcpy(buffer, _configuration, sizeof(_configuration)); //make a modifiable copy
      buffer[CONFIGURATION_OFFSET_SESSION] = (uint8_t)this->session;
      buffer[CONFIGURATION_OFFSET_SESSION + 1] = (uint8_t)(this->session >> 8);
      buffer[CONFIGURATION_OFFSET_SESSION + 2] = (uint8_t)(this->session >> 16);
      buffer[CONFIGURATION_OFFSET_SESSION + 3] = (uint8_t)(this->session >> 24);
      buffer[CONFIGURATION_OFFSET_LABEL_LENGTH] = (uint8_t)length;
      buffer[CONFIGURATION_OFFSET_LABEL_LENGTH + 1] = (uint8_t)(length >> 8);
      status = this->send(buffer, sizeof(_configuration));
      if (status <= 0) return -22;
      this->index = 1;
      sleep1ms(10); //sleep to prevent the os to concatenate the following data to this TCP package
   }

   //send label index
   memcpy(buffer, _labelIndex, sizeof(_labelIndex)); //make a modifiable copy
   buffer[LABEL_INDEX_OFFSET] = (uint8_t)this->index;
   buffer[LABEL_INDEX_OFFSET + 1] = (uint8_t)(this->index >> 8);
   status = this->send(buffer, sizeof(_labelIndex));
   if (status <= 0) return -23;
   this->index++; //preset for the next
   sleep1ms(10); //sleep to prevent the os to concatenate the following data to this TCP package

   //the bitmap has to be sent as one blob with an header and an footer
   //for a performant implementation, i am sending in chunks of 1460 bytes, which is exactly the max. payload size of a TCP package
   //setup the header (contains bitmap height, width)
   memcpy(buffer, _labelHeightWidth, sizeof(_labelHeightWidth));
   //set bitmap height
   buffer[LABEL_HEIGHT_OFFSET] = (uint8_t)height;
   buffer[LABEL_HEIGHT_OFFSET + 1] = (uint8_t)(height >> 8);
   buffer[LABEL_HEIGHT_OFFSET + 2] = (uint8_t)(height >> 16);
   buffer[LABEL_HEIGHT_OFFSET + 3] = (uint8_t)(height >> 24);
   //set bitmap width
   buffer[LABEL_WIDTH_OFFSET] = (uint8_t)width;
   buffer[LABEL_WIDTH_OFFSET + 1] = (uint8_t)(width >> 8);
   buffer[LABEL_WIDTH_OFFSET + 2] = (uint8_t)(width >> 16);
   buffer[LABEL_WIDTH_OFFSET + 3] = (uint8_t)(width >> 24);


   //send the bitmap
   //read first bunch of data into
   FILE * f = fopen(file, "r"); //caller has already ensured, that the file exists!
   fseek(f , dataOffset, SEEK_SET); //skip the bitmap header and move forward to the begin of the bitmap data
   uint32_t offset = sizeof(_labelHeightWidth);
   while (uint32_t count = fread(&buffer[offset], sizeof(uint8_t), sizeof(buffer) - offset, f)) //read data into buffer
   {
      offset += count;
      if (offset >= sizeof(buffer)) //buffer full
      {
         //send chunk
         status = this->send(buffer, sizeof(buffer));
         if (status <= 0)
         {
            fclose(f);
            return -24;
         }
         offset = 0;
      }
   }
   fclose(f);
   //send the remaining data (+ footer)
   buffer[offset++] = _labelFeed[0];
   if (offset >= sizeof(buffer)) //if buffer is full, i have to send first
   {
      //send chunk
      status = this->send(buffer, sizeof(buffer));
      if (status <= 0) return -25;
      offset = 0;
   }
   buffer[offset++] = _labelFeed[1];
   status = this->send(buffer, offset);
   if (status <= 0) return -26;
   sleep1ms(10); //sleep to prevent the os to concatenate the following data to this TCP package


   //request LabelWriter status
   status = this->send(_labelStatus, sizeof(_labelStatus));
   if (status <= 0) return -27;
   status = this->receive(buffer, sizeof(buffer));
   // if (buffer[15] == 1) -> papier ist alle!!! TODO
   if (status <= 0) return -28;

   //success
   return 0;
}


void Dymon::end()
{
   //send final form-feed command data
   this->send(_final, sizeof(_final));
   //close socket
   this->close();
}




//Datei-Inhalt in Buffer auslesen
static int getFileContent(const char * filename, uint8_t * buffer, size_t bufferSize)
{
   //open file for read
   FILE* f = fopen(filename, "r");
   if (f == NULL)
   {
      buffer[0] = 0;
      return -1;
   }

   //determine file size
   fseek(f, 0, SEEK_END); //goto end of file
   size_t fileSize = ftell(f); //get position -> thats the file size
   rewind(f); //go back to start of file

   //read file into buffer
   if (fileSize >= bufferSize)
   {
      fileSize = bufferSize - 1; //do limitations (keep one byte reserved for the zero termination)
   }
   size_t count = fread(buffer, sizeof(char), fileSize, f);
   //add zero termination
   buffer[count] = 0;

   //close file
   fclose(f);
   //return number of bytes read from file
   return (int)count;
}



static uint32_t parsePortableBitmapP4(const char * file, uint32_t * h, uint32_t * w)
{
   uint8_t buffer[512]; //this should be enoug to read the first line in the bitmap file

   //get (1st bunch of) bitmap from file
   FILE * f = fopen(file, "r"); //caller has already ensured, that the file exists!
   size_t count = fread(buffer, sizeof(uint8_t), sizeof(buffer), f);

   //parse the data for the expected image format (based on the magic number) and the image width and height
   uint32_t dataOffset = 0;
   uint32_t width = 0;
   uint32_t height = 0;
   while (count > 7) //at least 7 bytes ("P4" + \n + width + space + height + \n)
   {
      uint32_t i = 0;
      //magic number + newline. skip if the are not as expected!
      if (buffer[i++] != 'P') break;
      if (buffer[i++] != '4') break;
      if (buffer[i++] != '\n') break;
      //optional comment (until end of line (given by newline char))
      while (buffer[i] == '#') //may be multiple lines
      {
         do
         {
            if (i >= count)
            {
               fclose(f);
               return 0;
            }
         } while (buffer[i++] != '\n'); //skip all chars until the end of the comment line was reached
      }
      //get image width
      uint8_t digit;
      while (1)
      {
         if (i >= count)
         {
            fclose(f);
            return 0;
         }
         digit = buffer[i++];
         if ((digit >= '0') && (digit <= '9')) //decimal number
         {
            width = 10 * width + (digit - '0');
            continue;
         }
         break; //not a char
      }
      //width and height are separated by a space char
      if (digit != ' ') break;
      //get image height
      while (1)
      {
         if (i >= count)
         {
            fclose(f);
            return 0;
         }
         digit = buffer[i++];
         if ((digit >= '0') && (digit <= '9')) //decimal number
         {
            height = 10 * height + (digit - '0');
            continue;
         }
         break; //not a char
      }

      //the final expected char is a newline
      if (digit == '\n')
      {
         fclose(f);
         if (i < count) //not at the end of the file?
         {
            *h = height;
            *w = width;
            return i; //image blob starts at this offset
         }
      }
      break; //unconditional break, as this is not realy a while loop!
   }

   fclose(f);
   return 0;
}


