//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Base64 stuff
*/
//---------------------------------------------------------------------------------------------------------------------


/* -- Includes ------------------------------------------------------------ */
#include <stdint.h>
#include "base64.h"



/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */

/* -- Global Variables ---------------------------------------------------- */
const char g_Base64Alphabet[64] =
{
   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
   'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};


//map the ascii chars 0x20 .. 0x7F
//translate the respective base64 letter to its binary value
static const uint8_t decoderArray[96] =
{
   //0x20 .. 0x3F:
   //' '  '!'  '"'  '#'  '$'  '%'  '&'  '''  '('  ')'  '*'  '+'  ','  '-'  '.'  '/'  '0'  '1'  '2'  '3'  '4'  '5'  '6'  '7'  '8'  '9'  ':'  ';'  '<'  '='  '>'  '?'
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   62,  0,   0,   0,   63,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  0,   0,   0,   0,   0,   0,
   //0x40 .. 0x5F
   //'@'  'A'  'B'  'C'  'D'  'E'  'F'  'G'  'H'  'I'  'J'  'K'  'L'  'M'  'N'  'O'  'P'  'Q'  'R'  'S'  'T'  'U'  'V'  'W'  'X'  'Y'  'Z'  '['  '\'  ']'  '^'  '_'
      0,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  0,   0,   0,   0,   0,
   //0x60 .. 0x7F
   //'`'  'a'  'b'  'c'  'd'  'e'  'f'  'g'  'h'  'i'  'j'  'k'  'l'  'm'  'n'  'o'  'p'  'q'  'r'  's'  't'  'u'  'v'  'w'  'x'  'y'  'z'  '{'  '|'  '}'  '~'  DEL
      0,   26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  0,   0,   0,   0,   0
};


/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */


uint32_t base64_encode(char * destination, const uint8_t * source, uint32_t len)
{
   uint32_t count = 0;
   uint32_t tmp;

   while (len >= 3)
   {
      tmp = *source++;
      tmp = (tmp << 8) | *source++;
      tmp = (tmp << 8) | *source++;
      destination[3] = g_Base64Alphabet[tmp & 0x3F];
      tmp = tmp >> 6;
      destination[2] = g_Base64Alphabet[tmp & 0x3F];
      tmp = tmp >> 6;
      destination[1] = g_Base64Alphabet[tmp & 0x3F];
      tmp = tmp >> 6;
      destination[0] = g_Base64Alphabet[tmp & 0x3F];
      destination += 4;
      len -= 3;
      count += 4;
   }
   if (len == 1) //one byte remaining
   {
      tmp = *source;
      destination[0] = g_Base64Alphabet[tmp >> 2];
      destination[1] = g_Base64Alphabet[(tmp << 4) & 0x3F];
      destination[2] = '=';
      destination[3] = '=';
      destination += 4;
      count += 4;
   }
   if (len == 2) //two bytes remaining
   {
      tmp = *source++;
      tmp = (tmp << 8) | *source;
      destination[0] = g_Base64Alphabet[(tmp >> 10) & 0x3F];
      destination[1] = g_Base64Alphabet[(tmp >> 4) & 0x3F];
      destination[2] = g_Base64Alphabet[(tmp << 2) & 0x3F];
      destination[3] = '=';
      destination += 4;
      count += 4;
   }
   destination[0] = 0; //add zero termination
   return count;
}




uint32_t base64_decode(uint8_t * destination, const char * source, uint32_t len)
{
   uint32_t count = 0;
   uint32_t tmp;
   uint32_t c;

   while ((len > 0) && (((c = *source++) >= 0x20) && (c < 0x80))) //within range of decoderArray
   {
      --len;
      tmp = (uint32_t)decoderArray[c - 0x20] << 18;
      if ((len > 0) && (((c = *source++) >= 0x20) && (c < 0x80)))
      {
         --len;
         tmp |= (uint32_t)decoderArray[c - 0x20] << 12;
         ++count;
         //the last but one char is optional (because it could be a '=' and i tolerate it when this is missing)
         if ((len > 0) && (((c = *source) >= 0x20) && (c < 0x80)))
         {
            --len;
            ++source;
            if (c != '=')
            {
               tmp |= (uint32_t)decoderArray[c - 0x20] << 6;
               ++count;
               //the last char is optional (because it could be a '=' and i tolerate it when this is missing)
               if ((len > 0) && (((c = *source) >= 0x20) && (c < 0x80)))
               {
                  --len;
                  ++source;
                  if (c != '=')
                  {
                     tmp |= (uint32_t)decoderArray[c - 0x20];
                     ++count;
                  }
               }
            }
         }
         //store 3 bytes
         *destination++ = (uint8_t)(tmp >> 16);
         *destination++ = (uint8_t)(tmp >> 8);
         *destination++ = (uint8_t)tmp;
         continue;
      }
      break;
   }
   return count;
}
