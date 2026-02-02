#include <cstring>
#include "bitmap.h"
#include "barcodeEan8.h"



Bitmap::Bitmap()
{
   glyphIterator = nullptr;
   data = nullptr;
   // memset(this, 0, sizeof(Bitmap));
}



Bitmap::Bitmap(const uint32_t width, const uint32_t height, enum Orientation orientation,
               const GFXfont * const font)
{
   glyphIterator = nullptr;
   data = nullptr;
   init(width, height, orientation, font);
}


void Bitmap::init(const uint32_t width, const uint32_t height, enum Orientation orientation,
               const GFXfont * const font)
{
   //cleanup old instance
   if (glyphIterator) delete glyphIterator;
   if (data) delete[] data;
   utf8.reset();

   //assert((width % 8) == 0); //width shall be a multiple of 8
   //set bitmap properties
   this->width = width;
   this->widthByte = width / 8;
   this->height = height;
   uint32_t length = width * height;
   this->length = (int32_t)length;
   this->lengthByte = length / 8;
   this->orientation = orientation;

   //set default text font
   this->font = font;

   //allocate buffer for bitmap (and clear it to zero)
   this->data = new uint8_t[this->lengthByte + 4]; //i allocate 4 extra bytes for utilized by dymon as scratchpad
   memset(this->data, 0, this->lengthByte);

   //create glyph iterator
   this->glyphIterator = new GlyphIterator();
}


Bitmap::~Bitmap()
{
   if (glyphIterator) delete glyphIterator;
   if (data) delete[] data;
}


void Bitmap::setFont(const GFXfont * const font)
{
   this->font = font;
}


void Bitmap::setOrientation(enum Orientation orientation)
{
   this->orientation = orientation;
}


uint32_t Bitmap::getTextWidth(const char * text) const
{
   char character;
   uint32_t width;

   if (this->font == nullptr)
   {
      return 0;
   }

   width = 0;
   while ((character = *text++) != 0)
   {
      //draw character
      glyphIterator->init(this->font, character);
      width += glyphIterator->xAdvance;
   }
   return width;
}



int Bitmap::drawText(const int32_t x, const int32_t y, const char * text)
{
   int32_t cursor;
   int32_t pixel;
   int32_t width;
   int character;
   bool status;

   if (this->font == nullptr)
   {
      return -1;
   }

   width = 0;
   cursor = x;
   while ((character = (uint8_t)(*text++)) != 0)
   {
      //handle utf8 characters
      character = utf8.decode(character);
      //only for ASCII chars in range 1 ..127 and the suported UTF8 onces. Others will be skipped!
      if (character > 0)
      {
         //draw pixel for pixel of character
         status = glyphIterator->init(this->font, character);
         while (status)
         {
            //calculate pixel index to be set to the respective value
            pixel = getPixelIndex(cursor + glyphIterator->xOffset, y + glyphIterator->yOffset);
            if (pixel >= 0)
            {
               //set pixel to its value
               setPixelValue(pixel, glyphIterator->value);
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



void Bitmap::drawBarcode(const uint32_t y, const uint32_t height, const uint32_t value, double scale)
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
      uint32_t width = ((orientation == Orientation::Horizontally) ? this->width : this->height);
      scaleFactor = (uint32_t)((scale * width) / barcodeLength);
      //calculate initial cursor position
      cursor = (width - (scaleFactor * barcodeLength)) / 2;

      //draw one barcode line
      for (uint32_t i = 0; i < barcodeLength; ++i)
      {
         bool value = ((barcode[i/8] & (0x80 >> (i & 7))) != 0); //get respective bit
         for (uint32_t j = 0; j < scaleFactor; ++j)
         {
            pixel = (uint32_t)getPixelIndex(cursor++, y);
            setPixelValue(pixel, value);
         }
      }

      //duplicate that line (height - 1)-times
      duplicateLineDown(y, height - 1);
   }
}


uint32_t Bitmap::drawBarcode(uint32_t x, uint32_t y, uint32_t weight, uint32_t height, uint32_t value)
{
   if (height > 0)
   {
      uint8_t barcode[9];
      uint32_t barcodeLength;
      uint32_t cursor;
      uint32_t pixel;

      //get encode barcode
      barcodeLength = BarcodeEan8::intToBarcode(value, barcode);
      //draw one barcode line
      cursor = x;
      for (uint32_t i = 0; i < barcodeLength; ++i)
      {
         bool value = ((barcode[i/8] & (0x80 >> (i & 7))) != 0); //get respective bit
         for (uint32_t j = 0; j < weight; ++j)
         {
            pixel = getPixelIndex(cursor++, y);
            setPixelValue(pixel, value);
         }
      }

      //duplicate that line (height - 1)-times
      duplicateLineSegmentDown(y, height - 1, x, cursor - x);
      return weight * barcodeLength;
   }
   return 0;
}


void Bitmap::drawLine(uint32_t y, uint32_t height)
{
   if (height > 0)
   {
      //draw horizontal line
      for (uint32_t i = 0; i < width; ++i)
      {
         uint32_t pixel = getPixelIndex(i, y);
         setPixelValue(pixel, 1);
      }
      //duplicate that line (height - 1)-times
      duplicateLineDown(y, height - 1);
   }
}








int32_t Bitmap::getPixelIndex(const int32_t x, const int32_t y)
{
   if (orientation == Orientation::Horizontally)
   {
      //check for label under- and overflow
      if ((uint32_t)x >= this->width) return -1;
      if ((uint32_t)y >= this->height) return -1;
      //otherwise
      return (y * this->width) + x;
   }
   else //Vertically
   {
      //check for label under- and overflow
      if ((uint32_t)y >= this->width) return -1;
      if ((uint32_t)x >= this->height) return -1;
      //otherwise
      return ((this->width - 1) - y) + (x * this->width);
   }
}



bool Bitmap::getPixelValue(const int32_t pixel)
{
   const int32_t index = pixel / 8;
   const uint8_t mask = (0x80 >> (pixel & 7));
   uint8_t data = 0;

   //ignore all pixel that does not fit onto the lable
   if ((uint32_t)pixel < this->length)
   {
      //read respective byte
      data = this->data[index];
   }
   return ((data & mask) != 0);
}



void Bitmap::setPixelValue(const int32_t pixel, const bool value)
{
   const int32_t index = pixel / 8;
   const uint8_t mask = (0x80 >> (pixel & 7));
   uint8_t data;

   //ignore all pixel that does not fit onto the lable
   if ((uint32_t)pixel < this->length)
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



//duplicate the line of y-coordinate n-times downdards
void Bitmap::duplicateLineDown(const uint32_t y, const uint32_t times)
{
#if 1
   if (orientation == Orientation::Horizontally)
   {
      //for horizontal orientation, this operation can be done very efficient just coping bytes
      uint32_t origin = y * this->widthByte;
      for (uint32_t i = 1; i <= times; ++i)
      {
         uint32_t line = i*this->widthByte + origin;
         if (line + this->widthByte <= this->lengthByte) //preven buffer overflow
         {
            for (uint32_t j = 0; j < this->widthByte; ++j)
            {
               this->data[line + j] = this->data[origin + j];
            }
            continue;
         }
         break;
      }
   }
   else //Vertically
#endif
   {
      //for vertical orientation, this must be done plain... (pixel by pixel)
      uint32_t xend = (orientation == Orientation::Horizontally) ? this->width : this->height; //but can also be done for horizontal orientation
      for (uint32_t x = 0; x < xend; ++x)
      {
         uint32_t sourcePixel = getPixelIndex(x, y);
         bool value = getPixelValue(sourcePixel);
         for (uint32_t i = 1; i <= times; ++i)
         {
            uint32_t destinationPixel = getPixelIndex(x, y+i);
            setPixelValue(destinationPixel, value);
         }
      }
   }
}


//duplicate the line of y-coordinate n-times downdards
void Bitmap::duplicateLineSegmentDown(const uint32_t y, const uint32_t times, uint32_t x, uint32_t len)
{
   const uint32_t max = (orientation == Orientation::Horizontally) ? this->width : this->height; //but can also be done for horizontal orientation
   uint32_t xend = x + len;
   if (xend > max) xend = max;

   //copy pixel by pixel
   for (; x < xend; ++x)
   {
      uint32_t sourcePixel = getPixelIndex(x, y);
      bool value = getPixelValue(sourcePixel);
      for (uint32_t i = 1; i <= times; ++i)
      {
         uint32_t destinationPixel = getPixelIndex(x, y+i);
         setPixelValue(destinationPixel, value);
      }
   }
}
