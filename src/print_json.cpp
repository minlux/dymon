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
#include "print_json.h"


/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */
using namespace std;

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */


/*
   Print a label on the printer user has connected to.
   This function can be called multiple times to print several labels (at once) on the same printer.

   JSON object data like this expexted:
   {
      "ip":"127.0.0.1", //don't care for USB printers
      "width":272,
      "height":252,
      "orientation":0,
      "text":"\\24cTitle\n\\3_\nHello\n\\rWorld\n\n\\100#214",
      "count":1
   }
*/
int PrintJson::print(cJSON * json)
{
   cJSON * j;

   //get label width
   j = cJSON_GetObjectItemCaseSensitive(json, "width");
   if (j == nullptr) return -1;
   const int width = j->valueint;

   //get label height
   j = cJSON_GetObjectItemCaseSensitive(json, "height");
   if (j == nullptr) return -1;
   const int height = j->valueint;

   //get label orientation
   j = cJSON_GetObjectItemCaseSensitive(json, "orientation");
   if (j == nullptr) return -1;
   const int orientation = j->valueint;


   //get text to be printed
   j = cJSON_GetObjectItemCaseSensitive(json, "text");
   const char * const text = cJSON_GetStringValue(j);
   if (text == nullptr) return -1;

   //create bitmap
   Bitmap bitmap = Bitmap::fromText(width, height, (Bitmap::Orientation)orientation, text);

   //print labels
   //get number of copies to be printed
   j = cJSON_GetObjectItemCaseSensitive(json, "count");
   if (j == nullptr) return -1;
   const int copies = j->valueint;
   for (int i = 0; i < copies; ++i) 
   {
      int err = dymon->print(&bitmap, 0, ((i+1) < copies));
      if (err != 0 ) return err;
   }

   return 0;
}
