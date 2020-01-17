#include <cstdlib>
#include <iostream>
#include <fstream>
#include "dymon.h"
#include "FreeSans15pt7b.h"
#include "FreeSans18pt7b.h"


using namespace std;

static inline bool file_exist(const char * name)
{
   ifstream f(name);
   return f.good();
}


int main(int argc, char * argv[])
{
   const char * ipAddress;
   const char * bitmapFile;
   int32_t bitmapWidth;
   int32_t bitmapHeight;
   double labelLength;

   if (argc < 6)
   {
      cout << "Usage:" <<endl;
      cout << "./dymon_bmp <ip-of-labelwriter> <bitmap-width> <bitmap-height> <label-length> <bitmap-file>" << endl;
      cout << " Bitmap width and height has to be given in pixel with a resolution of 300dpi" << endl;
      cout << " Label length has to be given in mm" << endl;
      return -1;
   }


   //check if file exists
   bitmapFile = argv[5];
   if (!file_exist(bitmapFile))
   {
      cout << "File " << bitmapFile << " does not exist!" << endl;
      return -2;
   }

   //get geometry parameters
   bitmapWidth = atoi(argv[2]);
   bitmapHeight = atoi(argv[3]);
   labelLength = atof(argv[4]);
   if ((bitmapWidth <= 0) || (bitmapHeight <= 0) || (labelLength <= 0))
   {
      cout << "Invalid geometry parameter of the label. Width, height and length must be positive numbers!" << endl;
      return -3;
   }


#ifdef _WIN32
   DymonWin32 dymon;
#else //Linux assumed
   DymonLinux dymon;
#endif

   //print label
   ipAddress = argv[1];
   int error = dymon.start(ipAddress); //connect to labelwriter
   if (error == 0)
   {
      dymon.print_bitmap(bitmapFile, bitmapWidth, bitmapHeight, labelLength);
      dymon.end(); //finalize printing (form-feed) and close socket
   }
   return error;
}



