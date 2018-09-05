#include "bitmap.h"
#include "barcodeEan8.h"



Bitmap::Bitmap(const uint32_t width, const uint32_t height, const GFXfont * const font)
{
//   assert((width % 8) == 0); //width shall be a multiple of 8


   //set bitmap properties
   this->width = width; //width shall be a multiple of 8
   this->widthByte = width / 8; //round up to next integer
   this->height = height;
   this->length = width * height;
   this->lengthByte = this->widthByte * height;

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
   uint32_t pixel;
   uint32_t width;
   int character;
   bool status;
   bool esc;

   esc = false;
   width = 0;
   cursor = x;
   while ((character = *text++) != 0)
   {
      //handle escaped sequence
      if (esc) //previous char was escape char ...
      {
         esc = false;
         character = getEscapedCharacter(character);
      }
      else
      {
         if (character == '^') //Escape character?
         {
            esc = true;
            continue; //skip escape char
         }
      }

      //only for ASCII chars in range 1 ..127 and the escaped onces. Others will be skipped!
      if (character > 0)
      {
         const uint32_t origion = y * this->width + cursor;

         //draw character
         status = glyphIterator->init(this->font, character);
         if ((cursor + glyphIterator->width) >= this->width) break; //not enough space left to draw further characters into this line


         //draw pixel for pixel of character
         while (status)
         {
            //calculate pixel index to be set to the respective value
            pixel = origion;
            pixel += glyphIterator->yOffset * this->width;
            pixel += glyphIterator->xOffset;

            //set pixel to its value
            setPixelValue(pixel, glyphIterator->value);

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
   uint8_t barcode[9];
   uint32_t barcodeLength;
   uint32_t scaleFactor;
   uint32_t cursor;
   uint32_t origion;

   //get encode barcode
   barcodeLength = BarcodeEan8::intToBarcode(value, barcode);

   //calculate a scale factor
   scaleFactor = this->width / barcodeLength;
   //calculate initial cursor position
   cursor = (this->width - (scaleFactor * barcodeLength)) / 2;

   //draw one barcode line
   origion = y * this->width + cursor;
   for (uint32_t i = 0; i < barcodeLength; ++i)
   {
      bool value = ((barcode[i/8] & (0x80 >> (i & 7))) != 0); //get respective bit
      for (uint32_t j = 0; j < scaleFactor; ++j)
      {
         setPixelValue(origion++, value);
      }
   }

   //duplicate that line
   origion = y * this->widthByte;
   for (uint32_t i = 1; i < height; ++i)
   {
      uint32_t line = i*this->widthByte + origion;
      for (uint32_t j = 0; j < this->widthByte; ++j)
      {
         this->data[line + j] = this->data[origion + j];
      }
   }
}


int Bitmap::getEscapedCharacter(int c)
{
   //Deutsche Umlaute und Euro-Zeichen ummappen:
   //E (0x45) -> EURO (0x80)
   //A (0x41) -> Ae (0x81)
   //a (0x61) -> ae (0x82)
   //O (0x4F) -> Oe (0x83)
   //o (0x6F) -> oe (0x84)
   //U (0x55) -> Ue (0x85)
   //u (0x75) -> ue (0x86)
   //s (0x73) -> sz (0x87)
   static const uint8_t escCharacterMap[128] =
   {
   //        x0    x1    x2    x3    x4    x5    x6    x7    x8    x9    xA    xB    xC    xD    xE    xF
   /* 0x */   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   /* 1x */   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   /* 2x */   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   /* 3x */   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   /* 4x */   0, 0x81,    0,    0,    0, 0x80,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x83,
   /* 5x */   0,    0,    0,    0,    0, 0x85,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   /* 6x */   0, 0x82,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 0x84,
   /* 7x */   0,    0,    0, 0x87,    0, 0x86,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
   };


   if ((c >= 0) && (c <= 128))
   {
      return escCharacterMap[c];
   }
   return 0;
}

