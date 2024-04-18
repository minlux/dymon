#include <string.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include "dymon.h"
#include "FreeSans15pt7b.h"
#include "FreeSans18pt7b.h"
#include "usbprint.h"



using namespace std;

static inline bool file_exist(const char * name)
{
   ifstream f(name);
   return f.good();
}


static void usage(void)
{
   cout << "Usage:\n";
   cout << " dymon_pbm <net>|<usb> <bitmap-file>\n\n";

   cout << "Examples:\n";
   cout << " dymon_pbm net:192.168.178.23 <bitmap-file>\n";
   cout << " dymon_pbm usb:/dev/usb/lp0 <bitmap-file>\n";
   cout << " dymon_pbm usb:vid_0922 <bitmap-file>\n";
   cout << endl;
}


int main(int argc, char * argv[])
{
   enum {
      USB
   } interfaze;
   char * path;
   const char * bitmapFile;


   if (argc < 3)
   {
      usage();
      return -1;
   }

   //get interface and path

   if (strncmp(argv[1], "usb:", 4) == 0)
   {
      interfaze = USB;
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

   //check if file exists
   bitmapFile = argv[2];
   if (!file_exist(bitmapFile))
   {
      cout << "File " << bitmapFile << " does not exist!" << endl;
      return -4;
   }

   Dymon * dymon;
   if (interfaze == USB)
   {
      dymon = new DymonUsb;
   }




   //print label
   int error = dymon->start(path); //connect to labelwriter
   if (error == 0)
   {
      dymon->print_bitmap(bitmapFile);
      dymon->end(); //finalize printing (form-feed) and close socket
   }
   delete dymon;
   return error;
}



