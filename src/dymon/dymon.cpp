#include <stdio.h>
#include <cstring>
// #include <iostream>
#include "dymon.h"

//helper function to get read file
static int getFileContent(const char * filename, uint8_t * buffer, size_t bufferSize);


const uint8_t Dymon::_initial[] = {
   0x1B, 0x41, 1           //status request
};

const uint8_t Dymon::_header[] = {
   0x1B, 0x73, 1, 0, 0, 0, //counter
   0x1B, 0x43, 0x64,       //print density "normal"
   0x1B, 0x4C,             //label length...
   0, 0,                   //...given as multiple of 1/600 inch. (e.g. 600 ^= 600 * 1/600 inch = 1 inch) (16-bit little endian)
   0x1B, 0x68,             //print quality, 300x300dpi
   0x1B, 0x4D, 0, 0, 0, 0, 0, 0, 0, 0, //media type, default
   0x1B, 0x68,
   0x1B, 0x6E, 1, 0,       //label index
   0x1B, 0x44, 0x01, 0x02,
   0, 0, 0, 0,             //label height in pixel (32-bit little endian)
   0, 0, 0, 0              //label width in pixel (32-bit little endian). shall be a multiple of 8!
};

const uint8_t Dymon::_footer[] = {
   0x1B, 0x47,             //short line feed
   0x1B, 0x41, 0           //status request
};

const uint8_t Dymon::_final[] = {
   0x1B, 0x45,             //line feed
   0x1B, 0x51              //line tab
};




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
   status = this->send(_initial, sizeof(_initial));
   if (status <= 0)
   {
      return -3;
   }
   status = this->receive(receiveBuffer, sizeof(receiveBuffer));
   if (status <= 0)
   {
      return -4;
   }
   //success
   return 0;
}



//bitmap resolution is assumed to be 300dpi
int Dymon::print(const Bitmap * bitmap, double labelLength1mm)
{
   uint8_t receiveBuffer[128];
   int status;
   int error = 0;


   //parameter check
   if ((bitmap == nullptr) || (labelLength1mm <= 0))
   {
      return -1;
   }

   do
   {
      //send label information in header
      uint8_t labelHeader[sizeof(_header)];
      memcpy(labelHeader, _header, sizeof(_header)); //make a modifiable copy of the header
      //set label length
      uint32_t labelLength = (uint32_t)(((600.0 * labelLength1mm) / 25.4) + 0.5); //label length is based on 600 dots per inch
      labelHeader[11] = (uint8_t)labelLength;
      labelHeader[12] = (uint8_t)(labelLength >> 8);
      //set bitmap height
      labelHeader[35] = (uint8_t)bitmap->height;
      labelHeader[36] = (uint8_t)(bitmap->height >> 8);
      labelHeader[37] = (uint8_t)(bitmap->height >> 16);
      labelHeader[38] = (uint8_t)(bitmap->height >> 24);
      //set bitmap width
      labelHeader[39] = (uint8_t)bitmap->width;
      labelHeader[40] = (uint8_t)(bitmap->width >> 8);
      labelHeader[41] = (uint8_t)(bitmap->width >> 16);
      labelHeader[42] = (uint8_t)(bitmap->width >> 24);
      status = this->send(labelHeader, sizeof(labelHeader), true);
      if (status <= 0) { error = -3; break; }

      //send bitmap data
      status = this->send(bitmap->data, bitmap->lengthByte, true);
      if (status <= 0) { error = -3; break; }

      //send label footer + status request
      status = this->send(_footer, sizeof(_footer));
      if (status <= 0) { error = -3; break; }
      status = this->receive(receiveBuffer, sizeof(receiveBuffer));
      if (status <= 0) { error = -4; break; }
   } while (false);
   return error;
}



int Dymon::print_bitmap(const char * file)
{
   uint8_t bitmapBuffer[32*1024]; //32kB Buffer
   uint8_t receiveBuffer[128];
   int status;
   int error;

   //get (1st bunch of) bitmap from file
   FILE * f = fopen(file, "r"); //caller has already ensured, that the file exists!
   size_t count = fread(bitmapBuffer, sizeof(uint8_t), sizeof(bitmapBuffer), f);

   error = -1; //preset error code to "input file invalid"
   //parse the data for the expected image format (based on the magic number) and the image width and height
   uint32_t dataOffset = 0;
   uint32_t width = 0;
   uint32_t height = 0;
   while (count > 7) //at least 7 bytes ("P4" + \n + width + space + height + \n)
   {
      uint32_t i = 0;
      //magic number + newline. skip if the are not as expected!
      if (bitmapBuffer[i++] != 'P') break;
      if (bitmapBuffer[i++] != '4') break;
      if (bitmapBuffer[i++] != '\n') break;
      //optional comment (until end of line (given by newline char))
      if (bitmapBuffer[i] == '#')
      {
         do
         {
            if (i >= count)
            {
               goto _printBitmapException;
            }
         } while (bitmapBuffer[i++] != '\n'); //skip all chars until the end of the comment line was reached
      }
      //get image width
      uint8_t digit;
      while (1)
      {
         if (i >= count)
         {
            goto _printBitmapException;
         }
         digit = bitmapBuffer[i++];
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
            goto _printBitmapException;
         }
         digit = bitmapBuffer[i++];
         if ((digit >= '0') && (digit <= '9')) //decimal number
         {
            height = 10 * height + (digit - '0');
            continue;
         }
         break; //not a char
      }

      //the final expected char ia a newline
      if (digit == '\n')
      {
         dataOffset = i; //image blob starts at this offset
      }
      break; //unconditional break, as this is not realy a while loop!
   }

   //header parsed successfully
   if ((dataOffset > 0) && (dataOffset < count))
   {
      error = -2; //preset error code to communication error
      do
      {
         //send label information in header
         uint8_t labelHeader[sizeof(_header)];
         memcpy(labelHeader, _header, sizeof(_header)); //make a modifiable copy of the header
         //set label length
         uint32_t labelLength = 2uL * height; //label length is based on 600 dots per inch (whereas width and height are 300dpi)
         labelHeader[11] = (uint8_t)labelLength;
         labelHeader[12] = (uint8_t)(labelLength >> 8);
         //set bitmap height
         labelHeader[35] = (uint8_t)height;
         labelHeader[36] = (uint8_t)(height >> 8);
         labelHeader[37] = (uint8_t)(height >> 16);
         labelHeader[38] = (uint8_t)(height >> 24);
         //set bitmap width
         labelHeader[39] = (uint8_t)width;
         labelHeader[40] = (uint8_t)(width >> 8);
         labelHeader[41] = (uint8_t)(width >> 16);
         labelHeader[42] = (uint8_t)(width >> 24);
         status = this->send(labelHeader, sizeof(labelHeader), true);
         if (status <= 0) { goto _printBitmapException; }

         //send bitmap data
         do
         {
            status = this->send(&bitmapBuffer[dataOffset], (uint32_t)(count - dataOffset), true);
            if (status <= 0) { goto _printBitmapException; }
            //get next data blob from file
            dataOffset = 0; //now data offset is 0!
            count = fread(bitmapBuffer, sizeof(uint8_t), sizeof(bitmapBuffer), f);
         } while (count > 0);

         //send label footer + status request
         status = this->send(_footer, sizeof(_footer));
         if (status <= 0) { goto _printBitmapException; }
         status = this->receive(receiveBuffer, sizeof(receiveBuffer));
         if (status <= 0) { goto _printBitmapException; }
      } while (false);
      error = 0; //everything OK!
   }


_printBitmapException:
   fclose(f);
   return error;
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