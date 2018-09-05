#ifndef BITMAP_H_INCLUDED
#define BITMAP_H_INCLUDED


#include <stdint.h>
#include "gfxfont.h"
#include "glyphIterator.h"



class Bitmap
{
public:
   Bitmap(const uint32_t width, const uint32_t height, const GFXfont * const font);

   void setFont(const GFXfont * const font);

   uint32_t getTextWidth(const char * text);
   int drawText(const uint32_t x, const uint32_t y, const char * text);
   void drawBarcode(const uint32_t y, const uint32_t height, const uint32_t value); //will be printed centered

   uint32_t width;
   uint32_t height;

   uint8_t * data;
   uint32_t lengthByte; //length in bytes

private:
   void setPixelValue(const uint32_t pixel, const bool value);
   static int getEscapedCharacter(int c);


   uint32_t widthByte;
   uint32_t length; //length in bits

   const GFXfont * font;
   GlyphIterator * glyphIterator;
};



#endif
