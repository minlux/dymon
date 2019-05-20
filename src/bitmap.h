#ifndef BITMAP_H_INCLUDED
#define BITMAP_H_INCLUDED


#include <stdint.h>
#include "gfxfont.h"
#include "glyphIterator.h"
#include "utf8decoder.h"



class Bitmap
{
public:
   enum Orientation
   {
      Horizontally = 0,
      Vertically
   };

public:
   Bitmap(const uint32_t width, const uint32_t height, enum Orientation orientation = Orientation::Horizontally,
          const GFXfont * const font = nullptr);
   ~Bitmap();

   void setFont(const GFXfont * const font);
   void setOrientation(enum Orientation orientation);

   uint32_t getTextWidth(const char * text); //font must be set first!!!
   int drawText(const uint32_t x, const uint32_t y, const char * text); //font must be set first!!!
   void drawBarcode(const uint32_t y, const uint32_t height, const uint32_t value,
                    double scale = 1.0); //will be printed centered (set scale < 1 for smaller barcode width)

/*readonly*/
   uint32_t width;
   uint32_t height;

   uint8_t * data;
   uint32_t lengthByte; //length in bytes

private:
   int32_t getPixelIndex(const uint32_t x, const uint32_t y); //return -1 in case of overflow
   int32_t getPixelIndex(const uint32_t pixel, const int32_t xoff, const int32_t yoff); //return -1 in case of overflow
   bool getPixelValue(const uint32_t pixel);
   void setPixelValue(const uint32_t pixel, const bool value);
   void duplicateLineDown(const uint32_t y, const uint32_t times = 1); //duplicate the line of y-coordinate n-times downdards

   enum Orientation orientation;
   uint32_t widthByte;
   int32_t length; //length in bits

   const GFXfont * font;
   GlyphIterator * glyphIterator;
   Utf8Decoder utf8;
};



#endif
