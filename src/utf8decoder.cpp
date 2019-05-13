#include "utf8decoder.h"

//so muss das im Font hinterlegt sein (vgl. z.B. FreeSans15pt7b.h)
#define CHARACTER_CODE_EURO       (0x80)
#define CHARACTER_CODE_UPPER_AE   (0x81)
#define CHARACTER_CODE_LOWER_ae   (0x82)
#define CHARACTER_CODE_UPPER_OE   (0x83)
#define CHARACTER_CODE_LOWER_oe   (0x84)
#define CHARACTER_CODE_UPPER_UE   (0x85)
#define CHARACTER_CODE_LOWER_ue   (0x86)
#define CHARACTER_CODE_SZ         (0x87)  //scharfes S





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
      esc = 0;
      return character;
   }

   //UTF8 byte sequence
   esc = (esc << 8) | character;
   switch (esc)
   {
   case 0xE282AC: //EURO Symbol
      esc = 0;
      return CHARACTER_CODE_EURO;

   case 0xC384: //Ae
      esc = 0;
      return CHARACTER_CODE_UPPER_AE;
   case 0xC3A4: //ae
      esc = 0;
      return CHARACTER_CODE_LOWER_ae;

   case 0xC396: //Oe
      esc = 0;
      return CHARACTER_CODE_UPPER_OE;
   case 0xC3B6: //oe
      esc = 0;
      return CHARACTER_CODE_LOWER_oe;

   case 0xC39C: //Ue
      esc = 0;
      return CHARACTER_CODE_UPPER_UE;
   case 0xC3BC: //ue
      esc = 0;
      return CHARACTER_CODE_LOWER_ue;

   case 0xC39F: //Sz (scharfes S)
      esc = 0;
      return CHARACTER_CODE_SZ;

   default:
      break;
   }
   return -1;
}


