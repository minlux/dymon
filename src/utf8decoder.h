#ifndef UTF8_DECODER_H_INCLUDED
#define UTF8_DECODER_H_INCLUDED


#include <stdint.h>





class Utf8Decoder
{
public:
   Utf8Decoder();
   void reset();
   int decode(const int character);
private:
   uint32_t esc;
};



#endif
