//-----------------------------------------------------------------------------
/*!
   \file
   \brief Generate bitmap of label from JSON data.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include "text_label.h"
#include "bitmap.h"

/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */

Dymon::Bitmap TextLabel::fromJson(cJSON *json)
{
   // get label width
   cJSON *j = cJSON_GetObjectItemCaseSensitive(json, "width");
   if (j == nullptr) return Dymon::Bitmap();
   const int width = j->valueint;

   // get label height
   j = cJSON_GetObjectItemCaseSensitive(json, "height");
   if (j == nullptr) return Dymon::Bitmap();
   const int height = j->valueint;

   // get label orientation
   j = cJSON_GetObjectItemCaseSensitive(json, "orientation");
   if (j == nullptr) return Dymon::Bitmap();
   const int orientation = j->valueint;

   // get text to be printed
   j = cJSON_GetObjectItemCaseSensitive(json, "text");
   const char *const text = cJSON_GetStringValue(j);
   if (text == nullptr) return Dymon::Bitmap();

   // create bitmap
   Bitmap bitmap = Bitmap::fromText(width, height, (Bitmap::Orientation)orientation, text);
   return Dymon::Bitmap(bitmap.width, bitmap.height, bitmap.data, bitmap.lengthByte);
}
