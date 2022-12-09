#include <string.h>
#include <cstdlib>
#include <iostream>
#include "dymon.h"
#include "FreeSans15pt7b.h"
#include "FreeSans18pt7b.h"
#include "usbprint.h"


using namespace std;


static void usage(void)
{
   cout << "Usage:\n";
   cout << "./dymon_cli <ip>|<usb> <title> <line1> <line2> <barcode>" << endl;

   cout << "Examples:\n";
   cout << "./dymon_cli net#192.168.178.23 DYMO-Wireless Hello World 13\n";
   cout << "./dymon_cli usb#/dev/usb/lp0 DYMO-LW-550 on Linux 14\n";
   cout << "./dymon_cli usb#vid_0922 DYMO-LW-550 on Windows 15\n";
   cout << endl;
}


int main(int argc, char * argv[])
{
   enum {
      NET = 0,
      USB
   } interface;
   char * path;

   if (argc < 6)
   {
      usage();
      return -1;
   }


   //get interface and path
   if (strncmp(argv[1], "net#", 4) == 0)
   {
      interface = NET;
      path = &argv[1][4]; //get substring -> ip-address expected
   }
   else if (strncmp(argv[1], "usb#", 4) == 0)
   {
      interface = USB;
   #ifdef _WIN32
      //get the device name, to be used on windows
      //
      static char devname[256];
      int err = usbprint_get_devicename(devname, sizeof(devname), &argv[1][4]); //input something like "vid_0922" and get device name like "\\?\usb#vid_0922&pid_0028#04133046018600#{28d78fad-5a12-11d1-ae5b-0000f803a8c2}"
      if (err != 0)
      {
         cout << "Can't find any USB connected DYMO" << endl;
         return -2;
      }
      path = devname;
   #else
      path = &argv[1][4]; //get substring -> device-path expected (something like "/dev/usb/lp0")
   #endif
   }
   else
   {
      usage();
      return -3;
   }


   Dymon * dymon;
   if (interface == NET)
   {
      dymon = new DymonNet;
   }
   else //interface == USB
   {
      dymon = new DymonUsb;
   }


   Bitmap bitmap(272, 252 /*, Bitmap::Orientation::Vertically*/);
   const GFXfont * font;
   uint32_t y = 0; //Y-coordinate of the bitmap

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


   //print label
   int error = dymon->start(path); //connect to labelwriter
   if (error == 0)
   {
      dymon->print(&bitmap, 25.4, 0);
      dymon->end(); //finalize printing (form-feed) and close socket
   }
   delete dymon;
   return error;
}

