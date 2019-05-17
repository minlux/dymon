#include "bitmap.h"
#include "barcodeEan8.h"



Bitmap::Bitmap(const uint32_t width, const uint32_t height, const GFXfont * const font,
               enum Orientation orientation)
{
//   assert((width % 8) == 0); //width shall be a multiple of 8


   //set bitmap properties
   this->width = width; //width shall be a multiple of 8
   this->widthByte = width / 8; //round up to next integer
   this->height = height;
   this->length = width * height;
   this->lengthByte = this->widthByte * height;
   this->orientation = orientation;

   //set default text font
   this->font = font;

   //allocate buffer for bitmap
   this->data = new uint8_t[this->lengthByte];

   //create glyph iterator
   this->glyphIterator = new GlyphIterator();
}



void Bitmap::setFont(const GFXfont * const font)
{
   this->font = font;
}


uint32_t Bitmap::getTextWidth(const char * text)
{
   char character;
   uint32_t width;

   width = 0;
   while ((character = *text++) != 0)
   {
      //draw character
      glyphIterator->init(this->font, character);
      width += glyphIterator->xAdvance;
   }
   return width;
}



int Bitmap::drawText(const uint32_t x, const uint32_t y, const char * text)
{
   uint32_t cursor;
   int32_t pixel;
   uint32_t width;
   int character;
   bool status;

   width = 0;
   cursor = x;
   while ((character = (uint8_t)(*text++)) != 0)
   {
      //handle utf8 characters
      character = utf8.decode(character);
      //only for ASCII chars in range 1 ..127 and the suported UTF8 onces. Others will be skipped!
      if (character > 0)
      {
         const int32_t origin = getOrigin(cursor, y);
         if (origin < 0) break;  //not enough space left to draw further characters into this line

         //draw character
         status = glyphIterator->init(this->font, character);
         if (getOrigin(cursor + glyphIterator->width, y) < 0) break; //not enough space left to draw further characters into this line


         //draw pixel for pixel of character
         while (status)
         {
            //calculate pixel index to be set to the respective value
            pixel = getPixel((uint32_t)origin, glyphIterator->xOffset, glyphIterator->yOffset);
            if (pixel >= 0)
            {
               //set pixel to its value
               setPixelValue((uint32_t)pixel, glyphIterator->value);
            }
            //select next pixel
            status = glyphIterator->next();
         }

         //increment cursor
         cursor += glyphIterator->xAdvance;
         width += glyphIterator->xAdvance;
      }
   }

   return width;
}



void Bitmap::setPixelValue(const uint32_t pixel, const bool value)
{
   const uint32_t index = pixel / 8;
   const uint8_t mask = (0x80 >> (pixel & 7));
   uint8_t data;

   //ignore all pixel that does not fit onto the lable
   if (pixel < this->length)
   {
      //set pixel value by read-modify-write
      //read respective byte
      data = this->data[index];
      //modify respective bit
      if (value == false)
      {
         data = data & ~mask; //clear
      }
      else
      {
         data = data | mask; //clear
      }
      //write back respective byte
      this->data[index] = data;
   }
}



void Bitmap::drawBarcode(const uint32_t y, const uint32_t height, const uint32_t value)
{
   if (height > 0)
   {
      uint8_t barcode[9];
      uint32_t barcodeLength;
      uint32_t scaleFactor;
      uint32_t cursor;
      uint32_t pixel;

      //get encode barcode
      barcodeLength = BarcodeEan8::intToBarcode(value, barcode);

      //calculate a scale factor
      scaleFactor = this->width / barcodeLength;
      //calculate initial cursor position
      cursor = (this->width - (scaleFactor * barcodeLength)) / 2;

      //draw one barcode line
      for (uint32_t i = 0; i < barcodeLength; ++i)
      {
         bool value = ((barcode[i/8] & (0x80 >> (i & 7))) != 0); //get respective bit
         for (uint32_t j = 0; j < scaleFactor; ++j)
         {
            pixel = (uint32_t)getOrigin(cursor++, y);
            setPixelValue(pixel, value);
         }
      }

      //duplicate that line (height - 1)-times
      duplicateLineDown(y, height - 1);
   }
}



int32_t Bitmap::getOrigin(const uint32_t x, const uint32_t y)
{
   //check for "label-overflow"
   if (x >= this->width) return -1;
   if (y >= this->height) return -1;
   //otherwise
   return (y * this->width) + x;
}


int32_t Bitmap::getPixel(const uint32_t origin, const uint32_t xoff, const uint32_t yoff)
{
   uint32_t pixel = origin;
   pixel += xoff;
   if ((pixel / this->width) == (origin / this->width)) //stay in the same line (as origin was)?
   {
      pixel += yoff * this->width;
      if (pixel < this->length)
      {
         return pixel;
      }
   }
   //otherwise: "label-overflow"
   return -1;
}


//duplicate the line of y-coordinate n-times downdards
void Bitmap::duplicateLineDown(const uint32_t y, const uint32_t times)
{
   uint32_t origin = y * this->widthByte;
   for (uint32_t i = 1; i <= times; ++i)
   {
      uint32_t line = i*this->widthByte + origin;
      for (uint32_t j = 0; j < this->widthByte; ++j)
      {
         this->data[line + j] = this->data[origin + j];
      }
   }
}
