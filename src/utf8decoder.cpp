#include "utf8decoder.h"

//must match the indices in the font files!!!
#define CHARACTER_CODE_EURO       (0x80)
#define CHARACTER_CODE_UPPER_AE   (0x81)
#define CHARACTER_CODE_UPPER_OE   (0x82)
#define CHARACTER_CODE_UPPER_UE   (0x83)
#define CHARACTER_CODE_SZ         (0x84)  //Sz (scharfes S)
#define CHARACTER_CODE_LOWER_ae   (0x85)
#define CHARACTER_CODE_LOWER_oe   (0x86)
#define CHARACTER_CODE_LOWER_ue   (0x87)





Utf8Decoder::Utf8Decoder()
{
   esc = 0;
}


void Utf8Decoder::reset()
{
   esc = 0;
}


int Utf8Decoder::decode(const int character)
{
   //ASCII char 0 .. 0x7E
   if (character < 0x80)
   {
      // esc = 0;
      return character;
   }

   //UTF8 byte sequence
   esc = (esc << 8) | character;
   const uint32_t esc16 = esc & 0xFFFFuL;    //16-bit
   const uint32_t esc24 = esc & 0xFFFFFFuL;  //24-bit

   //Ä
   if (esc16 == 0xC384)
   {
      // esc = 0;
      return CHARACTER_CODE_UPPER_AE;
   }
   //ä
   if (esc16 == 0xC3A4)
   {
      // esc = 0;
      return CHARACTER_CODE_LOWER_ae;
   }
   //Ö
   if (esc16 == 0xC396)
   {
      // esc = 0;
      return CHARACTER_CODE_UPPER_OE;
   }
   //ö
   if (esc16 == 0xC3B6)
   {
      // esc = 0;
      return CHARACTER_CODE_LOWER_oe;
   }
   //Ü
   if (esc16 == 0xC39C)
   {
      // esc = 0;
      return CHARACTER_CODE_UPPER_UE;
   }
   //ü
   if (esc16 == 0xC3BC)
   {
      // esc = 0;
      return CHARACTER_CODE_LOWER_ue;
   }
   //Sz (scharfes S)
   if (esc16 == 0xC39F)
   {
      // esc = 0;
      return CHARACTER_CODE_SZ;
   }
   //EURO Symbol
   if (esc24 == 0xE282AC)
   {
      // esc = 0;
      return CHARACTER_CODE_EURO;
   }

   //unsupported (unicode) char
   return -1;
}


