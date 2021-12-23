/*
  Wrapper to interface with Labelwriter and print one or more labels.
  First user has to open up a connection to the labelwriter using "print_json_start".
  The function "print_json_do" can be called repeatedly to print one or more labels.
  Finally the connection has to be closed using print_json_end".

  The functions expects an array of json objects, containing the data to be printer.
  It is using cJSON (https://github.com/DaveGamble/cJSON) to handle the json data.
  The json data shall look like this:

  [
    {
        "ip":"127.0.0.1",
        "format":2,
        "title":"todo",
        "body":[
          "first line text",
          "second line",
          "3rd",
          "4th line of text"
        ],
        "barcode":1234567
    },
    ....
  ]
*/

#ifndef PRINT_H_INCLUDED
#define PRINT_H_INCLUDED

#include "cJSON.h"


void * print_json_start(cJSON * json, uint32_t session = 1); //open connection to labelprinter
int print_json_do(cJSON * json, void * prt); //send label to labelprinter
void print_json_end(void * prt); //close connection to labelprinter

int print_json_p4(cJSON * json, const char * filename); //print label into a PMB (P4) file.

#endif
