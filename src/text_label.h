#ifndef TEXT_LABEL_H_INCLUDE
#define TEXT_LABEL_H_INCLUDE

#include "cJSON.h"
#include "dymon.h"

class TextLabel
{
public:
   /*
       Generate bitmap of label from JSON data.

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
   static Dymon::Bitmap fromJson(cJSON *json);
};

#endif
