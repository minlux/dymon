//-----------------------------------------------------------------------------
/*!
   \file
   \brief Print labes on a DYMO LabelWriter wireless.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "gfxfont.h"
#include "FreeSans15pt7b.h"
#include "FreeSans18pt7b.h"
// #include "FreeSans21pt7b.h"
#include "FreeSans24pt7b.h"
#include "print_json.h"


/* -- Defines ------------------------------------------------------------- */
#define NUM_LABEL_FORMATS     (2)      //currently i support only 2 different label formats


using namespace std;


/* -- Types --------------------------------------------------------------- */
typedef struct
{
   uint16_t bitmapOrientation;
   uint16_t bitmapWidth;   //pixel
   uint16_t bitmapHeight;  //pixel
   float labelLength;   //mm
   float barcodeScale; //0 .. 1
   const GFXfont * titleFont;
   const GFXfont * bodyFont;
} LabelFormat_t;

/* -- (Module) Global Variables ------------------------------------------- */
static const LabelFormat_t m_LableFormat[NUM_LABEL_FORMATS] =
{
   //Bitmap-                             Bitmap-       Label-  Barcode-  Title-            Body-
   //Orientation                         Width/Height  Length  Scale     Font              Font
   { Bitmap::Orientation::Horizontally,  272,  252,    25.4,   0.9,      &FreeSans18pt7b,  &FreeSans15pt7b   }, //0: 25mm x 25mm Label
   { Bitmap::Orientation::Vertically,    400,  960,    88.9,   0.8,      &FreeSans24pt7b,  &FreeSans18pt7b   }, //1: 36mm x 89mm Label
}; //Note: Width/Height are given as a pixels with 300dpi.


/* -- Module Global Function Prototypes ----------------------------------- */


/* -- Implementation ------------------------------------------------------ */

//return
//On success, it returns the (positive) format number of the label (index into m_LableFormat array)
//-1 on error, if no valid JSON object
//-3 on error, if there is no format specified
//-4 on error, if the given format is undefined
static int _getBitmap(cJSON * json, Bitmap& bitmap)
{
   int error = -1;
   if (cJSON_IsObject(json))
   {
      error = -3;
      cJSON * format = cJSON_GetObjectItemCaseSensitive(json, "format");
      if (cJSON_IsNumber(format))
      {
         error = -4;
         //Lable-Format
         const int labelFormat = format->valueint; //currently not supported
         if ((uint32_t)labelFormat < NUM_LABEL_FORMATS)
         {
            const LabelFormat_t * lf = &m_LableFormat[labelFormat];
            bitmap.init((uint32_t)lf->bitmapWidth, (uint32_t)lf->bitmapHeight, (enum Bitmap::Orientation)lf->bitmapOrientation);
            const GFXfont * font;
            uint32_t y = 0; //Y-coordinate of the bitmap

            //Title (Headline)
            font = lf->titleFont;
            bitmap.setFont(font);
            cJSON * title = cJSON_GetObjectItemCaseSensitive(json, "title");
            if (cJSON_IsString(title))
            {
               y += font->yAdvance; //text is bottom based. so i have to pre-increment Y by the line height
               bitmap.drawText(0, y, title->valuestring);
            }

            //Body-Lines
            font = lf->bodyFont;
            bitmap.setFont(font);
            cJSON * lines = cJSON_GetObjectItemCaseSensitive(json, "body");
            cJSON * line;
            cJSON_ArrayForEach(line, lines)
            {
               if (cJSON_IsString(line))
               {
                  y += font->yAdvance; //text is bottom based. so i have to pre-increment Y by the line height
                  bitmap.drawText(0, y, line->valuestring);
               }
            }
            //Barcode
            cJSON * barcode = cJSON_GetObjectItemCaseSensitive(json, "barcode");
            if (cJSON_IsNumber(barcode))
            {
               //ean8 barcode
               if (labelFormat == 1) //special handling for that format
               {
                  //Rechtwinkling zum Text, ans Ende des Labels (ab 88%) mit Barcode-Höhe von 10% des Labels
                  bitmap.setOrientation(Bitmap::Orientation::Horizontally);
                  bitmap.drawBarcode(((88 * lf->bitmapHeight) / 100), ((10 * lf->bitmapHeight) / 100),
                                       barcode->valueint, lf->barcodeScale); //zentriert auf die ganze Breite
               }
               else
               {
                  y += font->yAdvance / 2; //add "margin" of 0.5 lines to previous text/barcode
                  bitmap.drawBarcode(y, 2*font->yAdvance, barcode->valueint, lf->barcodeScale); //zentriert auf die ganze Breite
                  //y += 2 * font->yAdvance; //add height of barcode to y-coordinate
               }
            }
            //bitmap successfully printed
            return labelFormat;
         }
      }
   }
   return error;
}




/*
   Print a label on the printer user has connected to.
   This function can be called multiple times to print several labels (at once) on the same printer.

   JSON object data like this expexted:
   {
      "format":2,
      "title":"todo",
      "body":[
         "first line text",
         "second line",
         "3rd",
         "4th line of text"
      ],
      "barcode":1234567
   }
*/
int PrintJson::print(cJSON * json)
{
   Bitmap bitmap;
   int status = _getBitmap(json, bitmap);
   if (status >= 0)
   {
      //print label
      const LabelFormat_t * lf = &m_LableFormat[status];
      int error = dymon->print(&bitmap, lf->labelLength, (json->next != NULL));
      return error;
   }
   return -status;
}



/*
   Print a label into a PMB file (P4 formate).
   JSON object data like this expexted:
   {
      "format":2,
      "title":"todo",
      "body":[
         "first line text",
         "second line",
         "3rd",
         "4th line of text"
      ],
      "barcode":1234567
   }
*/
int PrintJson::write_to_pbm(cJSON * json, const char * filename)
{
   Bitmap bitmap;
   int error = _getBitmap(json, bitmap);
   if (error >= 0)
   {
      error = -1; //preset
      FILE * f = fopen(filename, "w");
      if (f != nullptr)
      {
         char header[32];
         int len = sprintf(header, "P4\n%u %u\n", bitmap.width, bitmap.height); //generate header
         fwrite(header, 1, len, f); //write header
         fwrite(bitmap.data, 1, bitmap.lengthByte, f); //write payload
         fclose(f); //we are done
         error = 0;
      }
   }
   return error;
}
