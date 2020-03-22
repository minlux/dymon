//-----------------------------------------------------------------------------
/*!
   \file
   \brief Print labes on a DYMO LabelWriter wireless.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <stdint.h>
#include "gfxfont.h"
#include "FreeSans15pt7b.h"
#include "FreeSans18pt7b.h"
// #include "FreeSans21pt7b.h"
#include "FreeSans24pt7b.h"
#include "print.h"
#include "dymon.h"


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


/*
   Open up a connection to the DYMO given by the IP.

   JSON object data like this expexted:
   {
      "ip":"127.0.0.1"
   }
*/
void * print_json_start(cJSON * json)
{
   if (cJSON_IsObject(json))
   {
      cJSON * ip = cJSON_GetObjectItemCaseSensitive(json, "ip"); //get IP
      if (cJSON_IsString(ip))
      {
      #ifdef _WIN32
         DymonWin32 * dymon = new DymonWin32;
      #else //Linux assumed
         DymonLinux * dymon = new DymonLinux;
      #endif
         const char * printerIp = ip->valuestring;
         int status = dymon->start(printerIp); //connect to a DYMO at the given IP
         if (status == 0)
         {
            return dymon;
         }
         delete dymon;
      }
   }
   return nullptr;
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
int print_json_do(cJSON * json, void * prt)
{
   int error = 1;
   if (cJSON_IsObject(json))
   {
      error = 3;
      cJSON * format = cJSON_GetObjectItemCaseSensitive(json, "format");
      if (cJSON_IsNumber(format))
      {
         error = 4;
         //Lable-Format
         const int labelFormat = format->valueint; //currently not supported
         if ((uint32_t)labelFormat < NUM_LABEL_FORMATS)
         {
            const LabelFormat_t * lf = &m_LableFormat[labelFormat];
            Bitmap bitmap((uint32_t)lf->bitmapWidth, (uint32_t)lf->bitmapHeight,
                           (enum Bitmap::Orientation)lf->bitmapOrientation);
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

            //print label
            error = ((Dymon *)prt)->print(&bitmap, lf->labelLength);
         }
      }
   }
   return error;
}


//finalize printing
void print_json_end(void * prt)
{
   Dymon * dymon = (Dymon *)prt;
   dymon->end(); //this executes the final form feed
#ifdef _WIN32
   delete (DymonWin32 *)prt;
#else //Linux assumed
   delete (DymonLinux *)prt;
#endif
}
