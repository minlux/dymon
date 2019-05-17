#ifndef BITMAP_H_INCLUDED
#define BITMAP_H_INCLUDED


#include <stdint.h>
#include "gfxfont.h"
#include "glyphIterator.h"
#include "utf8decoder.h"



class Bitmap
{
   enum Orientation
   {
      Horizontally = 0 /*,
      Vertically
      */ //vertical orientation not implemented yet!
   };

public:
   Bitmap(const uint32_t width, const uint32_t height, const GFXfont * const font,
          enum Orientation orientation = Orientation::Horizontally);

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
   int32_t getOrigin(const uint32_t x, const uint32_t y); //return -1 in case of overflow
   int32_t getPixel(const uint32_t origin, const uint32_t xoff, const uint32_t yoff); //return -1 in case of overflow
   void duplicateLineDown(const uint32_t y, const uint32_t times = 1); //duplicate the line of y-coordinate n-times downdards

   enum Orientation orientation;
   uint32_t widthByte;
   uint32_t length; //length in bits

   const GFXfont * font;
   GlyphIterator * glyphIterator;
   Utf8Decoder utf8;
};



#endif
