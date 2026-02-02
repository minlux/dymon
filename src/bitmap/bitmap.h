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
   Bitmap();
   Bitmap(const uint32_t width, const uint32_t height, enum Orientation orientation = Orientation::Horizontally,
          const GFXfont * const font = nullptr);
   ~Bitmap();

   void init(const uint32_t width, const uint32_t height, enum Orientation orientation = Orientation::Horizontally,
             const GFXfont * const font = nullptr);

   void setFont(const GFXfont * const font);
   void setOrientation(enum Orientation orientation);

   uint32_t getTextWidth(const char * text) const; //font must be set first!!!
   int drawText(const int32_t x, const int32_t y, const char * text); //font must be set first!!!
   void drawBarcode(const uint32_t y, const uint32_t height, const uint32_t value,
                    double scale = 1.0); //will be printed centered (set scale < 1 for smaller barcode width)
   static inline uint32_t getBarcodeWidth(uint32_t weight) { return 67u * weight; }
   uint32_t drawBarcode(uint32_t x, uint32_t y, uint32_t weight, uint32_t height, uint32_t value);
   inline uint32_t getGlyphHeight() const { return font->glyph[3].height; } //3 -> use height of '#'
   inline uint32_t getLineHeight() const { return font->yAdvance; }

   void drawLine(uint32_t y, uint32_t height);

   inline uint32_t getWidth() const  { return (orientation == Orientation::Horizontally) ? width : height; }
   inline uint32_t getHeight() const { return (orientation == Orientation::Horizontally) ? height : width; }


//factory function
   static Bitmap fromText(const uint32_t width, const uint32_t height, enum Orientation orientation,
                           const char * const text, const uint32_t defaultFontSize = 0);


/*readonly*/
   uint32_t width;
   uint32_t height;

   uint8_t * data;
   uint32_t lengthByte; //length in bytes

private:
   int32_t getPixelIndex(const int32_t x, const int32_t y); //return -1 in case of overflow
   bool getPixelValue(const int32_t pixel);
   void setPixelValue(const int32_t pixel, const bool value);
   void duplicateLineDown(const uint32_t y, const uint32_t times = 1); //duplicate the line of y-coordinate n-times downdards
   void duplicateLineSegmentDown(const uint32_t y, const uint32_t times, uint32_t x, uint32_t len);

   enum Orientation orientation;
   uint32_t widthByte;
   int32_t length; //length in bits

   const GFXfont * font;
   GlyphIterator * glyphIterator;
   Utf8Decoder utf8;
};



#endif
