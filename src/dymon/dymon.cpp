#include <stdio.h>
#include <cstring>
#include <iostream>
#include "dymon.h"


extern "C" {
   unsigned int dymonDebug;
}


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
   index = 0;
   //create TCP socket and connect to LabelWriter
   connected = this->connect(arg);
   if (connected == false)
   {
      return -2;
   }

   //request LabelWriter status (active)
   return read_status(1);
}


//mode: 0 ^= release lock, 1 ^= grant lock, 2 ^= keep lock
int Dymon::read_status(uint8_t mode) //request a status update
{
   uint8_t _statusRequest[] = {
      0x1B, 0x41, 0           //(A) status request
   };
   _statusRequest[2] = mode;

   int status = this->send(_statusRequest, lw450flavor ? 2 : sizeof(_statusRequest));
   if (status <= 0)
   {
      return -3;
   }
   status = this->receive(this->status, lw450flavor ? 1 : sizeof(this->status));
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

   if (lw450flavor)
   {
      //for the first label (after call to start) we have to send the configuration (like the label-length???, print-density, print-quality, media-type...)
      //for all further labels we assume that these values doesn't change!!!
      if (this->index == 0)
      {
         static const uint8_t configuration[] = {
            0x1B, 0x65,            //(e) print density "normal" (LW450)
            0x1B, 0x68             //(h) print quality, 300x300dpi (text mode)
         };
         status = this->send(configuration, sizeof(configuration), true);
         if (status <= 0) return -12;
         this->index = 1;
      }

      //set bytes per line
      const uint32_t bytesPerLine = bitmap->width / 8;
      buffer[0] = 0x1B;
      buffer[1] = 0x44; //bytes per line
      buffer[2] = (uint8_t)bytesPerLine;
      status = this->send(buffer, 3);
      if (status <= 0) return -13;

      //send the bitmap data, line by line
      for (uint32_t idx = 0; idx < bitmap->data.size(); idx += bytesPerLine)
      {
         buffer[0] = 0x16; //SYN -> uncompressed bitmap data
         memcpy(&buffer[1], &bitmap->data[idx], bytesPerLine);
         status = this->send(buffer, bytesPerLine + 1, true);
         if (status <= 0) return -14;
      }
   }
   else
   {
      //for the first label (after call to start) we have to send the configuration (like the label-length???, print-density, print-quality, media-type...)
      //for all further labels we assume that these values doesn't change!!!
      if (this->index == 0)
      {
         static const uint8_t configuration[] = {
            0x1B, 0x73, 1, 0, 0, 0, //(s) counter (session, preset = 1)
            0x1B, 0x43, 0x64,       //(C) print density "normal"
            // 0x1B, 0x4C, 0, 0,       //(L) label length given as multiple of 1/600 inch. (e.g. 600 ^= 600 * 1/600 inch = 1 inch) (16-bit little endian)
            0x1B, 0x68,             //(h) print quality, 300x300dpi (text mode)
            0x1B, 0x4D, 0, 0, 0, 0, 0, 0, 0, 0 //(M) media type, standard
         };
         status = this->send(configuration, sizeof(configuration), true);
         if (status <= 0) return -12;
         this->index = 1;
      }

      //send label index + geometry
      #define LABEL_INDEX_OFFSET     (2) //label index must be patched int bytes [2,3]
      #define LABEL_HEIGHT_OFFSET    (8) //label height must be patched int bytes [8..11]
      #define LABEL_WIDTH_OFFSET     (12) //label height must be patched int bytes [12..15]
      static const uint8_t labelIndexHeightWidth[] = {
         0x1B, 0x6E, 1, 0,       //(n) label index (preset = 1)
         0x1B, 0x44, 0x01, 0x02, //(D)
         0, 0, 0, 0,             //label height in pixel (32-bit little endian)
         0, 0, 0, 0              //label width in pixel (32-bit little endian). shall be a multiple of 8!
      };
      memcpy(buffer, labelIndexHeightWidth, sizeof(labelIndexHeightWidth)); //make a modifiable copy
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
      status = this->send(buffer, sizeof(labelIndexHeightWidth), true);
      if (status <= 0) return -13;

      //send the bitmap data
      status = this->send(bitmap->data.data(), bitmap->data.size(), true);
      if (status <= 0) return -14;
   }

   //short form feed command
   buffer[0] = 0x1B;
   buffer[1] = 0x47; //(G) short form feed
   status = this->send(buffer, 2, true);
   if (status <= 0) return -15;

   //receive status response for the *previous print*
   this->index++;
   if (this->index > 2)
   {
      //wait for status response
      status = this->receive(this->status, lw450flavor ? 1 : sizeof(this->status));
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
   status = this->send(buffer, lw450flavor ? 2 : 3);
   if (status <= 0) return -18;


   //if more labels will follow, then i don't wait for the status response here
   //because - to speed up printing - i directly start the printing the next label...
   //However, if it is the last label, then we wait for the final status response
   if (more == false)
   {
      //wait for status response
      status = this->receive(this->status, lw450flavor ? 1 : sizeof(this->status));
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


void Dymon::end()
{
   if (connected)
   {
      static const uint8_t final[] = {
         0x1B, 0x45,             //(E) form feed
         0x1B, 0x51              //(Q) ???
      };
      //send final form-feed command data
      this->send(final, lw450flavor ? 2 : sizeof(final));
      //close socket
      this->close();
      connected = false;
   }
}






