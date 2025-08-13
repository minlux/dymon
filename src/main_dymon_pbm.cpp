#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include "dymon.h"
#include "usbprint.h"
#include "argtable3.h"
#include "vt100.h"


/* -- Defines ------------------------------------------------------------- */
#define APP_NAME           "dymon_pbm"
#define APP_VERSION        "2.0.1"


/* -- Types --------------------------------------------------------------- */
using namespace std;


/* -- (Module-)Global Variables ------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



static void print_usage(void ** argtable)
{
   puts(VT100_BOLD_UNDERLINED "Usage:" VT100_RESET " " VT100_BOLD APP_NAME VT100_RESET);
   arg_print_syntax(stdout, argtable, "\n\n");
}

static void print_help(void ** argtable)
{
   //Description
   puts("Print P4 portable bitmap on DYMO LabelWriter.\n");
   //Usage
   print_usage(argtable);
   //Options
   puts(VT100_BOLD_UNDERLINED "Options:" VT100_RESET);
   arg_print_glossary(stdout, argtable,"  %-25s %s\n");
}



static inline bool file_exist(const char * name)
{
   ifstream f(name);
   return f.good();
}


int main(int argc, char * argv[])
{
   struct arg_lit * argHelp;
   struct arg_lit * argVersion;
   struct arg_str * argUsb;
   struct arg_str * argNet;
   struct arg_int * argModel;
   struct arg_int * argCopies;
   struct arg_file * argInput;
   struct arg_lit * argDebug;
   struct arg_end * argEnd;
   void * argtable[] =
   {
      argHelp = arg_lit0(NULL, "help", "Print help and exit"),
      argVersion = arg_lit0(NULL, "version", "Print version and exit"),
   #ifdef _WIN32
      argUsb = arg_str0(NULL, "usb", "<ID>", "Use USB printer with vid/pid ID (e.g. 'vid_0922')"),
   #else
      argUsb = arg_str0(NULL, "usb", "<DEVICE>", "Use USB printer device (e.g. '/dev/usb/lp1')"),
   #endif
      argNet = arg_str0(NULL, "net", "<IP>", "Use network printer with IP (e.g. '192.168.178.23')"),
      argModel = arg_int0(NULL, "model", "<NUMBER>", "Model number of DYMO LabelWriter (e.g. '450')"),
      argCopies = arg_int0(NULL, "copies", "<NUMBER>", "Number of copies to be printed [default: 1]"),
      argDebug = arg_lit0(NULL, "debug", "Enable debug output"),
      argInput = arg_file1(NULL, NULL, "<INPUT>", "PBM file to be printed"),
      argEnd = arg_end(3),
   };

   // Parse command line arguments.
   arg_parse(argc, argv, argtable);

   // Help
   if (argHelp->count)
   {
      print_help(argtable);
      return 0;
   }

   // Version
   if (argVersion->count)
   {
      puts(APP_VERSION);
      return 0;
   }

   // Check existance of one (and only one) of the supported interfaces
   if (argUsb->count && argNet->count)
   {
      puts(VT100_RED "Error:" VT100_RESET " Only one interface allowed (one of '--usb' or '--net')\n");
      print_usage(argtable);
      return -1;
   }
   if ((argUsb->count ^ argNet->count) == 0)
   {
      puts(VT100_RED "Error:" VT100_RESET " Interface specifier required (one of '--usb' or '--net')\n");
      print_usage(argtable);
      return -1;
   }

   // Check for input file / pbm
   if (!argInput->count)
   {
      puts(VT100_RED "Error:" VT100_RESET " Input file required\n");
      print_usage(argtable);
      return -1;
   }

   // Check for debug switch
   if (argDebug->count)
   {
      dymonDebug = 1;
   }

   //check if inpuzt file exists
   const char * const bitmapFile = argInput->filename[0];
   if (!file_exist(bitmapFile))
   {
      cout << VT100_RED "Error:" VT100_RESET " File '" << bitmapFile << "' does not exist" << endl;
      return -4;
   }


   enum {
      NET = 0,
      USB
   } interfaze;
   const char * path;
   bool lw450 = false;

   //get interface and path
   if (argNet->count)
   {
      interfaze = NET;
      path = (char *)argNet->sval[0]; //pass this to DymonNet::start, which expects a ip address string
   }
   else //if (argUsb->count)
   {
      interfaze = USB;
      const char * const dev = argUsb->sval[0];
      const int model = (argModel->count ? argModel->ival[0] : 0);
      lw450 = (model == 450);
   #ifdef _WIN32
      //get the device name, to be used on windows
      //
      static char devname[256];
      int err = usbprint_get_devicename(devname, sizeof(devname), dev); //input something like "vid_0922" and get device name like "\\?\usb#vid_0922&pid_0028#04133046018600#{28d78fad-5a12-11d1-ae5b-0000f803a8c2}"
      if (err != 0)
      {
         cout << "Can't find any USB connected DYMO" << endl;
         return -2;
      }
      path = devname;
   #else
      path = dev;
   #endif
   }


   // Create instance
   Dymon * dymon;
   if (interfaze == NET)
   {
      dymon = new DymonNet;
   }
   else //interfaze == USB
   {
      dymon = new DymonUsb(1, lw450);
   }

   // Print label
   int error = dymon->start((void *)path); //connect to labelwriter
   if (error == 0)
   {
      auto bitmap = Dymon::Bitmap::fromFile(bitmapFile);
      const int copies = (argCopies->count ? argCopies->ival[0] : 1);
      for (int i = 0; i < copies; ++i) 
      {
         dymon->print(&bitmap, 0, ((i+1) < copies));
      }
      dymon->end(); //finalize printing (form-feed) and close socket
   }

   // Clean up
   delete dymon;
   return error;
}



