/* -- Includes ------------------------------------------------------------ */
#include <sstream>
#include <algorithm> 
#include "bitmap.h"
#include "FreeSans15pt7b.h"
#include "FreeSans18pt7b.h"
#include "FreeSans21pt7b.h"
#include "FreeSans24pt7b.h"


/* -- Defines ------------------------------------------------------------- */
#define MAX(a,b)           (((a) > (b)) ? (a) : (b))

/* -- Types --------------------------------------------------------------- */
using namespace std;

typedef enum
{
   ALIGN_LEFT = 0,
   ALIGN_CENTER = 1,
   ALIGN_RIGHT = 2,
   // ALIGN_JUSTIFIED = 3,
} Alignment_t;

/* -- (Module-)Global Variables ------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */


//trim trailing whitespaces
void trim_right(std::string& s) 
{
   s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) 
   {
      return !std::isspace(ch);
   }).base(), s.end());
}



Bitmap Bitmap::fromText(const uint32_t width, const uint32_t height, enum Bitmap::Orientation orientation,
                        const char * const text)
{
   Bitmap bitmap(width, height, orientation, &FreeSans15pt7b);
   uint32_t yCursor = 0;

   // Read line by line
   istringstream stream(text);
   string line;
   while (getline(stream, line))
   {
      unsigned int barcode = 0;
      Alignment_t align = ALIGN_LEFT;
      unsigned int size = 0;
      bool hline = false;
      unsigned int i = 0;
      trim_right(line);
      if (line[i] == '\\')
      {
         ++i;

         // Size
         while (isdigit(line[i]))
         {
            size = 10 * size + (line[i++] - '0');
         }

         // Alignment
         switch (line[i])
         {
         case 'l':
         // case 'L':
            align = ALIGN_LEFT;
            ++i;
            break;

         case 'c':
         // case 'C':
            align = ALIGN_CENTER;
            ++i;
            break;

         case 'r':
         // case 'R':
            align = ALIGN_RIGHT;
            ++i;
            break;

         case '_':
            hline = true;
            ++i;
            break;

         default:
            break;
         }

         // Barcode
         if (line[i] == '#')
         {
            ++i;
            while (isdigit(line[i]))
            {
               barcode = 10 * barcode + (line[i++] - '0');
            }
         }
      }

      // Set font size
      if (size <= 15)
      {
         bitmap.setFont(&FreeSans15pt7b);
      }
      else if (size <= 18)
      {
         bitmap.setFont(&FreeSans18pt7b);
      }
      else if (size <= 21)
      {
         bitmap.setFont(&FreeSans21pt7b);
      }
      else
      {
         bitmap.setFont(&FreeSans24pt7b);
      }

      // Print Barcode
      if (barcode)
      {
         // const uint32_t lineHeight = bitmap.getLineHeight();
         // const uint32_t marginTop = lineHeight - bitmap.getGlyphHeight();
         const uint32_t marginTop = 20;

         const uint32_t barcodeWeight = MAX(size/32, 1);
         const uint32_t barcodeWidth = bitmap.getBarcodeWidth(barcodeWeight);
         int32_t xCursor = 3*barcodeWeight; //consider a margin left, of 3*weight pixel
         if (align == ALIGN_RIGHT)
         {
            xCursor = ((int32_t)bitmap.getWidth() - (int32_t)barcodeWidth) - 3*barcodeWeight; //consider a margin right, of 3*weight pixel
         }
         else if (align == ALIGN_CENTER)
         {
            xCursor = ((int32_t)bitmap.getWidth() - (int32_t)barcodeWidth) / 2;
         }

         // const uint32_t barcodeHeight = bitmap.getGlyphHeight();
         const uint32_t barcodeHeight = MAX(size, 32);
         bitmap.drawBarcode(xCursor, yCursor + marginTop, barcodeWeight, barcodeHeight, barcode);
         yCursor += marginTop + barcodeHeight;
      }
      else if (hline) //Horizontal line
      {
         // const uint32_t lineHeight = bitmap.getLineHeight();
         // const uint32_t marginTop = lineHeight - bitmap.getGlyphHeight();
         const uint32_t marginTop = 20;
         yCursor += marginTop;
         bitmap.drawLine(yCursor, MAX(size, 1));
      }
      else // Print Text
      {
         const uint32_t lineHeight = bitmap.getLineHeight();
         // const uint32_t lineHeight = (8 * bitmap.getLineHeight()) / 10;
         yCursor += lineHeight;

         const char * const text = &line[i];
         const uint32_t textWidth = bitmap.getTextWidth(text);
         int32_t xCursor = 0;
         if (align == ALIGN_RIGHT)
         {
            xCursor = (int32_t)bitmap.getWidth() - (int32_t)textWidth;
         }
         else if (align == ALIGN_CENTER)
         {
            xCursor = ((int32_t)bitmap.getWidth() - (int32_t)textWidth) / 2;
         }

         bitmap.drawText(xCursor, yCursor, &line[i]);
      }
   }

   return bitmap;
}

