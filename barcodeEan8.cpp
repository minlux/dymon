#include "barcodeEan8.h"

const uint8_t BarcodeEan8::leftOddDigit[10] =
{
   0x0D, //0
   0x19, //1
   0x13, //2
   0x3D, //3
   0x23, //4
   0x31, //5
   0x2F, //6
   0x3B, //7
   0x37, //8
   0x0B  //9
};
const uint8_t BarcodeEan8::rightDigit[10] =
{
   0x72, //0
   0x66, //1
   0x6C, //2
   0x42, //3
   0x5C, //4
   0x4E, //5
   0x50, //6
   0x44, //7
   0x48, //8
   0x74  //9
};



//returns a 67-bit barcode from a given integer value
//left aligned, msb first: barcode[0]{7..0}, barcode[1]{7..0}, .. , barcode[8]{7..5}
//barcode[8]{4..0} is always 0!
//returns 67
uint32_t BarcodeEan8::intToBarcode(const uint32_t value, uint8_t barcode[9])
{
   uint8_t digit[8];
   uint32_t oddDigits;
   uint32_t evenDigits;
   uint32_t remainder;
   uint64_t helper;

   //get 7 decimal digits
   digit[0] = (value / (1000000uL)) % 10;
   digit[1] = (value / (100000uL)) % 10;
   digit[2] = (value / (10000uL)) % 10;
   digit[3] = (value / (1000uL)) % 10;
   digit[4] = (value / (100uL)) % 10;
   digit[5] = (value / (10uL)) % 10;
   digit[6] = value % 10;

   //checksum digit
   oddDigits = digit[6] + digit[4] + digit[2] + digit[0];
   evenDigits = digit[5] + digit[3] + digit[1];
   remainder = ((3 * oddDigits + evenDigits) % 10);
   digit[7] = (uint8_t)((10 - remainder) % 10);

   //encode into barcode

   //use 64-bit helper variable
   //start marker
   helper = 0x05; //3bit
   //left digits
   helper = (helper << 7) | leftOddDigit[digit[0]];
   helper = (helper << 7) | leftOddDigit[digit[1]];
   helper = (helper << 7) | leftOddDigit[digit[2]];
   helper = (helper << 7) | leftOddDigit[digit[3]];
   //center marker
   helper = (helper << 5) | 0x0A; //5bit
   //right digits
   helper = (helper << 7) | rightDigit[digit[4]];
   helper = (helper << 7) | rightDigit[digit[5]];
   helper = (helper << 7) | rightDigit[digit[6]];
   helper = (helper << 7) | rightDigit[digit[7]];

   //copy into byte array
   barcode[0] = (uint8_t)(helper >> 56);
   barcode[1] = (uint8_t)(helper >> 48);
   barcode[2] = (uint8_t)(helper >> 40);
   barcode[3] = (uint8_t)(helper >> 32);
   barcode[4] = (uint8_t)(helper >> 24);
   barcode[5] = (uint8_t)(helper >> 16);
   barcode[6] = (uint8_t)(helper >> 8);
   barcode[7] = (uint8_t)helper;
   barcode[8] = 0x05 << 5; //add end marker


   //return length of barcode
   return (3 + 4*7 + 5 + 4*7 + 3); //3bit start, four 7-bit digits, 5 bit middle, 4 7-bit digits, 3bit end
}
