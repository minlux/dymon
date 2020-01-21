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

   if (argc < 3)
   {
      cout << "Usage:" <<endl;
      cout << "./dymon_bmp <ip-of-labelwriter> <bitmap-file>" << endl;
      return -1;
   }


   //check if file exists
   bitmapFile = argv[2];
   if (!file_exist(bitmapFile))
   {
      cout << "File " << bitmapFile << " does not exist!" << endl;
      return -2;
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
      dymon.print_bitmap(bitmapFile);
      dymon.end(); //finalize printing (form-feed) and close socket
   }
   return error;
}



