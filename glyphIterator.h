#ifndef GLYPHITERATOR_H_INCLUDED
#define GLYPHITERATOR_H_INCLUDED


#include <stdint.h>
#include "gfxfont.h"



class GlyphIterator
{
public:
   GlyphIterator();

   bool init(const GFXfont * font, int character);
   bool next();

   bool value; //value of current pixel
   int32_t xOffset; //relative position of current pixel
   int32_t yOffset; //relative position of current pixel

   int32_t width; //width of glyph
   int32_t xAdvance; //cursor increment by this glyph


private:
   bool getPixelValue(uint32_t pixel);

   const GFXfont * font;
   const GFXglyph * glyph;

   uint32_t pixel;
   uint32_t length;
};



#endif
