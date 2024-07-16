#include <stdio.h>
#include <cstring>
#include <iostream>
#include "dymon.h"


extern "C" {
   unsigned int dymonDebug;
}



//helper functions
static int getFileContent(const char * filename, uint8_t * buffer, size_t bufferSize); //read file
static uint32_t parsePortableBitmapP4(const char * file, uint32_t * h, uint32_t * w);



#define CONFIGURATION_OFFSET_SESSION         (2)
// #define CONFIGURATION_OFFSET_BYTES_PER_LINE  (13)
// #define CONFIGURATION_OFFSET_LABEL_LENGTH    (16) //label length must be patched int bytes [16,17]
const uint8_t Dymon::_configuration[] = {
   0x1B, 0x73, 1, 0, 0, 0, //(s) counter (session, preset = 1)
   0x1B, 0x43, 0x64,       //(C) print density "normal"
   // 0x1B, 0x65,             //(e) print density "normal" (LW450)
   // 0x1B, 0x44, 0,          //(D) bytes per line (LW450)
   // 0x1B, 0x4C, 0, 0,       //(L) label length given as multiple of 1/600 inch. (e.g. 600 ^= 600 * 1/600 inch = 1 inch) (16-bit little endian)
   0x1B, 0x68,             //(h) print quality, 300x300dpi (text mode)
   0x1B, 0x4D, 0, 0, 0, 0, 0, 0, 0, 0 //(M) media type, standard
};

#define LABEL_INDEX_OFFSET     (2) //label index must be patched int bytes [2,3]
#define LABEL_HEIGHT_OFFSET    (8) //label height must be patched int bytes [8..11]
#define LABEL_WIDTH_OFFSET     (12) //label height must be patched int bytes [12..15]
const uint8_t Dymon::_labelIndexHeightWidth[] = {
   0x1B, 0x6E, 1, 0,       //(n) label index (preset = 1)
   0x1B, 0x44, 0x01, 0x02, //(D)
   0, 0, 0, 0,             //label height in pixel (32-bit little endian)
   0, 0, 0, 0              //label width in pixel (32-bit little endian). shall be a multiple of 8!
};

#define LABEL_STATUS_OFFSET   (4)
const uint8_t Dymon::_labelFeedStatus [] = {
   0x1B, 0x47,             //(G) short form feed
   0x1B, 0x41, 0           //(A) status request (passive)
};

const uint8_t Dymon::_final[] = {
   0x1B, 0x45,             //(E) form feed
   0x1B, 0x51              //(Q) ???
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



int Dymon::start(void * arg)
{
   static const uint8_t _setupRequest[] = {
      0x1B, 0x40,      //(@) Reset printer
      0x1B, 0x56,      //(V) Get printer model and firmware version
   };
   static const uint8_t _statusRequest[] = {
      0x1B, 0x41, 1    //(A) Get printer status
   };
   uint8_t buffer[12] = { 0 };
   int status;

   //create TCP socket and connect to LabelWriter
   connected = this->connect(arg);
   if (connected == false)
   {
      return -2;
   }

#if 0 //LW550 doesn't answer to that request. so receive blocks until timeout which slows down usage
   //do soft reset and request LabelWriter model number and firmware version
   status = this->send(_setupRequest, sizeof(_setupRequest));
   if (status <= 0)
   {
      return -3;
   }
   this->receive(buffer, sizeof(buffer) - 1); //The information is returned as a 10- character ASCII string in the following format:
                                                         //Bytes[0..6]: 7 digit model number (e.g. "1750111");
                                                         //byte[7]: ascii char 'v'
                                                         //Bytes[8..9]: the two digit firmware version (e.g. "0N")
                                                       //See https://www.dymo-label-printers.co.uk/news/list-of-dymo-model-numbers.html for list of model numbers
   if (memcmp(buffer, "175", 3) == 0) //some kind of Labelwriter 450
   {
      this->lw450flavor = true;
   }
#endif

   //request LabelWriter status (active)
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
#if 1
   if (dymonDebug)
   {
      log_status(1, this->status, status); //active status
   }
#endif
   //success
   index = 0;
   return 0;
}


//mode: 0 ^= release lock, 1 ^= grant lock, 2 ^= keep lock
int Dymon::read_status(uint8_t mode) //request a status update
{
   uint8_t _statusRequest[] = {
      0x1B, 0x41, 0           //(A) status request
   };
   _statusRequest[2] = mode;

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
#if 1
   if (dymonDebug)
   {
      log_status(mode, this->status, status);
   }
#endif
   //success
   return 0;
}



//bitmap resolution is assumed to be 300dpi
int Dymon::print(const Bitmap * bitmap, double _labelLength1mm, int more)
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

   if (lw450flavor)
   {
      //set bytes per line
      const uint32_t bytesPerLine = bitmap->width / 8;
      buffer[0] = 0x1B;
      buffer[1] = 0x44; //bytes per line
      buffer[2] = (uint8_t)bytesPerLine;
      status = this->send(buffer, 3);
      if (status <= 0) return -13;

      //send the bitmap data, line by line
      for (uint32_t idx = 0; idx < bitmap->lengthByte; idx += bytesPerLine)
      {
         buffer[0] = 0x16; //SYN -> uncompressed bitmap data
         memcpy(&buffer[1], &bitmap->data[idx], bytesPerLine);
         status = this->send(buffer, bytesPerLine + 1, true);
         if (status <= 0) return -14;
      }

      //send final short form feed
      buffer[0] = 0x1B;
      buffer[1] = 0x47; //(G) short form feed
      status = this->send(buffer, 2);
      if (status <= 0) return -15;
   }
   else
   {
      //send label index + geometry
      memcpy(buffer, _labelIndexHeightWidth, sizeof(_labelIndexHeightWidth)); //make a modifiable copy
      //index
      buffer[LABEL_INDEX_OFFSET] = (uint8_t)this->index;
      buffer[LABEL_INDEX_OFFSET + 1] = (uint8_t)(this->index >> 8);
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

      //patch short form feed command to end of bitmap (therefore i have reserved some extra bytes, see Bitmap::init)
      bitmap->data[bitmap->lengthByte + 0] = 0x1B;
      bitmap->data[bitmap->lengthByte + 1] = 0x47; //(G) short form feed
      //send the bitmap data + short form feed command
      status = this->send(bitmap->data, bitmap->lengthByte + 2, true);
      if (status <= 0) return -14;
   }

   //receive status response for the *previous print*
   this->index++;
   if (this->index > 2)
   {
      //wait for status response
      status = this->receive(this->status, sizeof(this->status));
      if (status <= 0) return -19;
   #if 1
      if (dymonDebug)
      {
         log_status(0, this->status, status);
      }
   #endif
   }


   //send get status command for *this print*
   buffer[0] = 0x1B;
   buffer[1] = 0x41; //A
   buffer[2] = (more ? 2 : 0); //if more labels will follow, keep the lock. otherwise release the lock
   status = this->send(buffer, 3);
   if (status <= 0) return -18;


   //if more labels will follow, then i don't wait for the status response here
   //because - to speed up printing - i directly start the printing the next label...
   //However, if it is the last label, then we wait for the final status response
   if (more == false)
   {
      //wait for status response
      status = this->receive(this->status, sizeof(this->status));
      if (status <= 0) return -19;
   #if 1
      if (dymonDebug)
      {
         log_status(0, this->status, status);
      }
   #endif
   }

   //success
   return 0;
}



int Dymon::print_bitmap(const char * file, int more)
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


   if (lw450flavor)
   {
      //set bytes per line
      const uint32_t bytesPerLine = width / 8;
      buffer[0] = 0x1B;
      buffer[1] = 0x44; //bytes per line
      buffer[2] = (uint8_t)bytesPerLine;
      status = this->send(buffer, 3);
      if (status <= 0) return -23;
   }
   else
   {
      //send label index + geometry
      memcpy(buffer, _labelIndexHeightWidth, sizeof(_labelIndexHeightWidth)); //make a modifiable copy
      //index
      buffer[LABEL_INDEX_OFFSET] = (uint8_t)this->index;
      buffer[LABEL_INDEX_OFFSET + 1] = (uint8_t)(this->index >> 8);
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
   }

   //send the bitmap
   //read first bunch of data into
   FILE * f = fopen(file, "r"); //caller has already ensured, that the file exists!
   fseek(f , dataOffset, SEEK_SET); //skip the bitmap header and move forward to the begin of the bitmap data
   uint8_t * buf = buffer;
   unsigned int bufsize = sizeof(buffer);
   if (lw450flavor)
   {
      *buf++ = 0x16; //prepend a 0x16 to indicate that the following bytes are uncrompressed bitmap data
      bufsize = width / 8; //limit buffer size to the number of bytes per "print line"
   }
   while (count = fread(buf, sizeof(uint8_t), bufsize, f)) //read data into buffer
   {
      status = this->send(buffer, count + (uint32_t)lw450flavor, true); //here we send "buffer" that is correct. thus we consider whether or not we have lw450 flavor (then we also need to send 1 byte more - because of the prepended 0x16)
      if (status <= 0)
      {
         fclose(f);
         return -24;
      }
   }
   fclose(f);

   //send short form feed for *this print*
   buffer[0] = 0x1B;
   buffer[1] = 0x47; //G
   status = this->send(buffer, 2);
   if (status <= 0) return -28;

   //receive status response for the *previous print*
   this->index++;
   if (this->index > 2)
   {
      //wait for status response
      status = this->receive(this->status, sizeof(this->status));
      if (status <= 0) return -29;
   #if 1
      if (dymonDebug)
      {
         log_status(0, this->status, status);
      }
   #endif
   }

   //send get status command for *this print*
   buffer[0] = 0x1B;
   buffer[1] = 0x41; //A
   buffer[2] = (more ? 2 : 0); //if more labels will follow, keep the lock. otherwise release the lock
   status = this->send(buffer, 3);
   if (status <= 0) return -28;


   //if more labels will follow, then i don't wait for the status response here
   //because - to speed up printing - i directly start the printing the next label...
   //However, if it is the last label, then we wait for the final status response
   if (more == false)
   {
      //wait for status response
      status = this->receive(this->status, sizeof(this->status));
      if (status <= 0) return -29;
   #if 1
      if (dymonDebug)
      {
         log_status(0, this->status, status);
      }
   #endif
   }

   //success
   return 0;
}


void Dymon::end()
{
   if (connected)
   {
      //send final form-feed command data
      this->send(_final, sizeof(_final));
      //close socket
      this->close();
      connected = false;
   }
}


#if 0
void Dymon::_debugEnd()
{
   if (connected)
   {
      //send final form-feed command data
      this->send(&_final[2], sizeof(_final) - 2);
      //close socket
      this->close();
      connected = false;
   }
}
#endif




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


