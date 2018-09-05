#include "glyphIterator.h"



GlyphIterator::GlyphIterator()
{
}



bool GlyphIterator::init(const GFXfont * font, int character)
{
   //set font
   this->font = font;
   this->glyph = &font->glyph[0]; //use first char as default

   //character supported or not?
   if ((character < font->first) || (character > font->last))
   {
      return 0;
   }

   //set glyph by character
   this->glyph = &font->glyph[character - font->first];
   //get glyph properties
   this->width = this->glyph->width;
   this->length = this->glyph->width * this->glyph->height; //last+1 pixel of the glyph
   this->xAdvance = this->glyph->xAdvance;

   //start with first pixel of glyph
   this->pixel = 0;
   this->xOffset = this->glyph->xOffset;
   this->yOffset = this->glyph->yOffset;
   this->value = getPixelValue(this->pixel);

   //return status
   return (this->pixel < this->length); //return 0 for "blank chars" (length == 0); 1 otherwise
}



bool GlyphIterator::next()
{
   this->pixel++;
   if (this->pixel >= this->length)
   {
      return 0; //end of glyph
   }

   //increment offsets
   this->xOffset++;
   if ((this->pixel % this->width) == 0) //wrap around?
   {
      this->xOffset = this->glyph->xOffset;
      this->yOffset++;
   }

   //get new pixel value
   this->value = getPixelValue(this->pixel);
   return 1;
}






bool GlyphIterator::getPixelValue(uint32_t pixel)
{
   const uint8_t * bitmap = this->font->bitmap;
   uint32_t index = this->glyph->bitmapOffset;
   uint8_t value;

   value = bitmap[index + pixel/8]; //get respective byte
   value = value & (0x80 >> (pixel & 7)); //isolate respective bit
   return (value != 0); //return boolean value of pixel
}


