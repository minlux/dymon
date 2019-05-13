#ifndef BARCODEEAN8_H_INCLUDED
#define BARCODEEAN8_H_INCLUDED


#include <stdint.h>
#include "gfxfont.h"
#include "glyphIterator.h"



class BarcodeEan8
{
public:
   static uint32_t intToBarcode(const uint32_t value, uint8_t barcode[7]);

private:
   BarcodeEan8(); //shall not be instantiated

   static const uint8_t leftOddDigit[10];
   static const uint8_t rightDigit[10];
};



#endif
