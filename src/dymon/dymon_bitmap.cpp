#include <stdio.h>
#include <cstring>
#include <iostream>
#include "dymon.h"



#if 0
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
#endif


Dymon::Bitmap Dymon::Bitmap::fromBytes(const uint8_t * buffer, uint32_t count)
{
   Dymon::Bitmap bitmap;

   //parse the data for the expected image format (based on the magic number) and the image width and height
   uint32_t dataOffset = 0;
   uint32_t width = 0;
   uint32_t height = 0;
   while (count > 7) //at least 7 bytes ("P4" + \n + width + space + height + \n)
   {
      size_t i = 0;
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
               return bitmap;
            }
         } while (buffer[i++] != '\n'); //skip all chars until the end of the comment line was reached
      }
      //get image width
      uint8_t digit;
      while (1)
      {
         if (i >= count)
         {
            return bitmap;
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
            return bitmap;
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
         const uint32_t length = ((width + 7) / 8) * height; //calculate expected data length
         if ((length + i) == count) //expected == actual data length
         {
            //copy the already buffered data
            std::vector<uint8_t> data(length); //allocate data buffer
            memcpy(data.data(), &buffer[i], length);
            bitmap.height = height;
            bitmap.width = width;
            bitmap.data = data;
            return bitmap;
         }
      }
      break; //unconditional break, as this is not realy a while loop!
   }

   return bitmap;
}


Dymon::Bitmap Dymon::Bitmap::fromBytes(const std::vector<uint8_t> pbm)
{
   return fromBytes(pbm.data(), pbm.size());
}


Dymon::Bitmap Dymon::Bitmap::fromFile(const char * file)
{
   Dymon::Bitmap bitmap;
   uint8_t buffer[4096]; //this should be enoug to read the first line in the bitmap file

   //get (1st bunch of) bitmap from file
   FILE * f = fopen(file, "r"); //caller has already ensured, that the file exists!
   size_t count = fread(buffer, sizeof(uint8_t), sizeof(buffer), f);

   //parse the data for the expected image format (based on the magic number) and the image width and height
   uint32_t dataOffset = 0;
   uint32_t width = 0;
   uint32_t height = 0;
   while (count > 7) //at least 7 bytes ("P4" + \n + width + space + height + \n)
   {
      size_t i = 0;
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
               return bitmap;
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
            return bitmap;
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
            return bitmap;
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
         const uint32_t length = ((width + 7) / 8) * height; //calculate expected data length
         std::vector<uint8_t> data(length); //allocate data buffer

         //read the data from file into the buffer
         //copy the first chunk of already buffered data
         size_t num = count - i;
         memcpy(data.data(), &buffer[i], num);
         while ((count = fread(buffer, sizeof(uint8_t), sizeof(buffer), f)) > 0)
         {
            //copy the rest of the data
            if ((num + count) > data.size()) data.resize(num + count); //prevent overflow
            memcpy(data.data() + num, buffer, count);
            num += count;
         }
         data.resize(num);
         bitmap.height = height;
         bitmap.width = width;
         bitmap.data = data;

         fclose(f);
         return bitmap;
      }
      break; //unconditional break, as this is not realy a while loop!
   }

   fclose(f);
   return bitmap;
}


