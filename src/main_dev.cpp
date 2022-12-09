//-----------------------------------------------------------------------------
/*!
   \file
   \brief Debug and development playground.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <unistd.h> //usleep
#include <iostream>
#include <cstdlib>
#include "dymon.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;

/* -- Types --------------------------------------------------------------- */


/* -- Module Global Function Prototypes ----------------------------------- */
/* -- (Module) Global Variables ------------------------------------------- */


/* -- Implementation ------------------------------------------------------ */

int main(int argc, char * argv[])
{
   if (argc < 2)
   {
      cout << "Usage:" << endl;
      cout << argv[0] << "<ip-of-labelwriter>" << endl;
      return -1;
   }

#ifdef _WIN32
   DymonNetWin32 dymon;
#else
   DymonNet dymon;
#endif

   for (int retry = 0; retry < 10; ++retry)
   {
      int error = dymon.start(argv[1]); //connect to labelwriter
      if (error == 0)
      {
         // if (!dymon.paperOut())
         {
            //subsequent status reading
            for (int i = 0; i < 200; ++i)
            {
               usleep(300000); //300ms
               dymon.read_status(1);
            }
            dymon._debugEnd();
            break;
         }
         //otherwise
         usleep(500000);
         dymon._debugEnd();
         usleep(500000);
      }
   }
   return 0;
}


