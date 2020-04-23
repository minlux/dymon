#include <stdio.h>
#include <cstring>
#include <iostream>
#include "dymon.h"


//helper functions
static int getFileContent(const char * filename, uint8_t * buffer, size_t bufferSize); //read file
static uint32_t parsePortableBitmapP4(const char * file, uint32_t * h, uint32_t * w);



#define CONFIGURATION_OFFSET_SESSION         (2)
// #define CONFIGURATION_OFFSET_LABEL_LENGTH    (11) //label length must be patched int bytes [11,12]
const uint8_t Dymon::_configuration[] = {
   0x1B, 0x73, 1, 0, 0, 0, //counter (session, preset = 1)
   0x1B, 0x43, 0x64,       //print density "normal"
   // 0x1B, 0x4C, 0, 0,       //label length given as multiple of 1/600 inch. (e.g. 600 ^= 600 * 1/600 inch = 1 inch) (16-bit little endian)
   0x1B, 0x68,             //print quality, 300x300dpi (text mode)
   0x1B, 0x4D, 0, 0, 0, 0, 0, 0, 0, 0 //media type, standard
};

#define LABEL_INDEX_OFFSET     (2) //label index must be patched int bytes [2,3]
#define LABEL_HEIGHT_OFFSET    (8) //label height must be patched int bytes [8..11]
#define LABEL_WIDTH_OFFSET     (12) //label height must be patched int bytes [12..15]
const uint8_t Dymon::_labelIndexHeightWidth[] = {
   0x1B, 0x6E, 1, 0,       //label index (preset = 1)
   0x1B, 0x44, 0x01, 0x02,
   0, 0, 0, 0,             //label height in pixel (32-bit little endian)
   0, 0, 0, 0              //label width in pixel (32-bit little endian). shall be a multiple of 8!
};

const uint8_t Dymon::_labelFeedStatus [] = {
   0x1B, 0x47,             //short form feed
   0x1B, 0x41, 0           //status request
};

const uint8_t Dymon::_final[] = {
   0x1B, 0x45,             //form feed
   0x1B, 0x51              //???
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
   static const uint8_t _statusRequest[] = {
      0x1B, 0x41, 1           //status request
   };

   //check host address
   if (host == nullptr)
   {
      return -1;
   }
   //create TCP socket and connect to LabelWriter
   int status = this->connect(host, port);
   if (status == 0)
   {
      return -2;
   }
   //request LabelWriter status
   status = this->send(_statusRequest, sizeof(_statusRequest));
   if (status <= 0)
   {
      return -3;
   }
   status = this->receive(this->status, sizeof(this->status));
   if (status <= 0)
   {
      return -4;
   }
#ifdef DYMON_DEBUG
   log_status(0, this->status, status);
#endif
   //success
   index = 0;
   return 0;
}


int Dymon::read_status() //request a status update
{
   static const uint8_t _statusRequest[] = {
      0x1B, 0x41, 0           //status request
   };

   int status = this->send(_statusRequest, sizeof(_statusRequest));
   if (status <= 0)
   {
      return -3;
   }
   status = this->receive(this->status, sizeof(this->status));
   if (status <= 0)
   {
      return -4;
   }
#ifdef DYMON_DEBUG
   log_status(0, this->status, status);
#endif
   //success
   return 0;
}



//bitmap resolution is assumed to be 300dpi
int Dymon::print(const Bitmap * bitmap, double labelLength1mm)
{
   uint8_t buffer[4096];
   int status;

   //parameter check
   if (bitmap == nullptr)
   {
      return -10;
   }

   //for the first label (after call to start) we have to send the configuration (like the label-length???, print-density, print-quality, media-type...)
   //for all further labels we assume that these values doesn't change!!!
   if (this->index == 0)
   {
      status = this->send(_configuration, sizeof(_configuration), true);
      if (status <= 0) return -12;
      this->index = 1;
   }

   //send label index + geometry
   memcpy(buffer, _labelIndexHeightWidth, sizeof(_labelIndexHeightWidth)); //make a modifiable copy
   //index
   buffer[LABEL_INDEX_OFFSET] = (uint8_t)this->index;
   buffer[LABEL_INDEX_OFFSET + 1] = (uint8_t)(this->index >> 8);
   this->index++;
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
   status = this->send(buffer, sizeof(_labelIndexHeightWidth), true);
   if (status <= 0) return -13;

   //send the bitmap data
   status = this->send(bitmap->data, bitmap->lengthByte, true);
   if (status <= 0) return -14;

   //send the form feed + status request
   status = this->send(_labelFeedStatus, sizeof(_labelFeedStatus));
   if (status <= 0) return -18;

   //wait for status response
   status = this->receive(this->status, sizeof(this->status));
   if (status <= 0) return -19;
#ifdef DYMON_DEBUG
   log_status(0, this->status, status);
#endif

   //success
   return 0;
}



int Dymon::print_bitmap(const char * file)
{
   uint8_t buffer[4096];
   uint32_t height;
   uint32_t width;
   uint32_t dataOffset;
   uint32_t count;
   int status;

   //parameter check
   if (file == nullptr)
   {
      return -20;
   }

   //parse the bitmap file
   dataOffset = parsePortableBitmapP4(file, &height, &width);
   if (dataOffset == 0)
   {
      return -21; //input file invalid
   }

   //for the first label (after call to start) we have to send the configuration (like the label-length???, print-density, print-quality, media-type...)
   //for all further labels we assume that these values doesn't change!!!
   if (this->index == 0)
   {
      status = this->send(_configuration, sizeof(_configuration), true);
      if (status <= 0) return -22;
      this->index = 1;
   }

   //send label index + geometry
   memcpy(buffer, _labelIndexHeightWidth, sizeof(_labelIndexHeightWidth)); //make a modifiable copy
   //index
   buffer[LABEL_INDEX_OFFSET] = (uint8_t)this->index;
   buffer[LABEL_INDEX_OFFSET + 1] = (uint8_t)(this->index >> 8);
   this->index++;
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
   status = this->send(buffer, sizeof(_labelIndexHeightWidth), true);
   if (status <= 0) return -23;

   //send the bitmap
   //read first bunch of data into
   FILE * f = fopen(file, "r"); //caller has already ensured, that the file exists!
   fseek(f , dataOffset, SEEK_SET); //skip the bitmap header and move forward to the begin of the bitmap data
   while (count = fread(buffer, sizeof(uint8_t), sizeof(buffer), f)) //read data into buffer
   {
      status = this->send(buffer, count, true);
      if (status <= 0)
      {
         fclose(f);
         return -24;
      }
   }
   fclose(f);

   //send the form feed + status request
   status = this->send(_labelFeedStatus, sizeof(_labelFeedStatus));
   if (status <= 0) return -28;

   //wait for status response
   status = this->receive(this->status, sizeof(this->status));
   if (status <= 0) return -29;
#ifdef DYMON_DEBUG
   log_status(0, this->status, status);
#endif

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


