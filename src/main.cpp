#include <cstdlib>
#include <iostream>
#include "dymon.h"
#include "FreeSans15pt7b.h"
#include "FreeSans18pt7b.h"


using namespace std;

int main(int argc, char * argv[])
{
   Bitmap bitmap(272, 252 /*, Bitmap::Orientation::Vertically*/);
   const GFXfont * font;
   uint32_t y = 0; //Y-coordinate of the bitmap

   if (argc < 6)
   {
      cout << "Usage:" <<endl;
      cout << "./dymon <ip-of-labelwriter> <title> <line1> <line2> <barcode>" << endl;
      return -1;
   }

   //Title (Headline)
   font = &FreeSans18pt7b;
   bitmap.setFont(font);
   y += font->yAdvance; //text is bottom based. so i have to pre-increment Y by the line height
   bitmap.drawText(0, y, argv[2]);

   //Body-Lines
   font = &FreeSans15pt7b;
   bitmap.setFont(font);
   //Line 1
   y += font->yAdvance; //text is bottom based. so i have to pre-increment Y by the line height
   bitmap.drawText(0, y, argv[3]);
   //line 2
   y += font->yAdvance; //text is bottom based. so i have to pre-increment Y by the line height
   bitmap.drawText(0, y, argv[4]);

   //EAN8 Barcode
   y += font->yAdvance / 2; //add "margin" of 0.5 lines to previous text/barcode
   bitmap.drawBarcode(y, 2*font->yAdvance, atoi(argv[5]), 1.0); //zentriert auf die ganze Breite



#ifdef _WIN32
   DymonWin32 dymon;
#else //Linux assumed
   DymonLinux dymon;
#endif

   //print label
   int error = dymon.print(&bitmap, 25.4, argv[1]);
   return error;
}

