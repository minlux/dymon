//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Read from .
*/
//---------------------------------------------------------------------------------------------------------------------


/* -- Includes ------------------------------------------------------------ */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include "argtable3.h"
#include "bitmap.h"
#include "vt100.h"


/* -- Defines ------------------------------------------------------------- */
#define APP_NAME           "txt2pbm"
#define APP_VERSION        "1.0.0"

#define MAX_LINE_LENGTH    (120)


/* -- Types --------------------------------------------------------------- */
using namespace std;


/* -- (Module-)Global Variables ------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */
static void print_usage(void ** argtable, FILE * out)
{
   fprintf(out, VT100_BOLD_UNDERLINED "Usage:" VT100_RESET " " VT100_BOLD APP_NAME VT100_RESET "\n");
   arg_print_syntax(out, argtable, "\n\n");
}

static void print_help(void ** argtable, FILE * out)
{
   //Description
   fprintf(out, "Convert text to P4 portable bitmap.\n\n");
   //Usage
   print_usage(argtable, out);
   //Options
   fprintf(out, VT100_BOLD_UNDERLINED "Options:" VT100_RESET "\n");
   arg_print_glossary(out, argtable,"  %-25s %s\n");
}


int main(int argc, char * argv[])
{
   struct arg_lit * argHelp;
   struct arg_lit * argVersion;
   struct arg_int * argWidth;
   struct arg_int * argHeight;
   struct arg_str * argComment;
   struct arg_lit * argRotate;
   struct arg_file * argOutfile;
   struct arg_file * argInfile;
   struct arg_end * argEnd;
   void * argtable[] =
   {
      argHelp = arg_lit0(NULL, "help", "Print help and exit"),
      argVersion = arg_lit0("v", "version", "Print version and exit"),
      argWidth = arg_int1("w", "width", "<PIXEL>", "Width of PBM"),
      argHeight = arg_int1("h", "height", "<HEIGHT>", "Height of PBM"),
      argComment = arg_str0("c", "comment", "<TEXT>", "Comment, embedded into PBM"),
      argRotate = arg_lit0("r", "rotate", "Rotate final PBM by 90 degrees"),
      argOutfile = arg_file1("o", "output", "<OUTPUT>", "Output PBM file (use '-' for stdout)"),
      argInfile = arg_file0(NULL, NULL, "<INPUT>", "Input text file [default: stdin]"),
      argEnd = arg_end(3),
   };

   // Parse command line arguments.
   arg_parse(argc, argv, argtable);

   // Help
   if (argHelp->count)
   {
      print_help(argtable, stdout);
      return 0;
   }

   // Version
   if (argVersion->count)
   {
      puts(APP_VERSION);
      return 0;
   }

   // Dimension arguments
   if (!argWidth->count || !argHeight->count)
   {
      fprintf(stderr, VT100_RED "Error:" VT100_RESET " Width/Height required\n\n");
      print_usage(argtable, stderr);
      return 1;
   }
   // Validate dimension parameter
   const int width = argWidth->ival[0];
   const int height = argHeight->ival[0];
   if ((width <= 0) || (height <= 0))
   {
      fprintf(stderr, VT100_RED "Error:" VT100_RESET " Width/Heigt invalid\n\n");
      print_usage(argtable, stderr);
      return 1;
   }

   // Output
   if (!argOutfile->count)
   {
      fprintf(stderr, VT100_RED "Error:" VT100_RESET " Output file required\n\n");
      print_usage(argtable, stderr);
      return 1;
   }
   FILE * const fout = (strcmp(argOutfile->filename[0], "-") == 0) ? stdout : fopen(argOutfile->filename[0], "wb");
   if (fout == NULL)
   {
      fprintf(stderr, VT100_RED "Error:" VT100_RESET " Failed to open file '%s'\n", argOutfile->filename[0]);
      return 1;
   }

   // Input
   FILE *fin = stdin; // Set default to read from stdin
   // Optional, read from file
   if (argInfile->count)
   {
      fin = fopen(argInfile->filename[0], "r");
      if (fin == NULL)
      {
         fprintf(stderr, VT100_RED "Error:" VT100_RESET " Failed to open file '%s'\n", argInfile->filename[0]);
         if (fout != stdout) fclose(fout);
         return 1;
      }
   }

   // Read all lines from file/stdin into a string
   string text;
   char line[MAX_LINE_LENGTH];
   while (fgets(line, sizeof(line), fin))
   {
      text += line;
   }

   // Create bitmap
   Bitmap bitmap = Bitmap::fromText(width, height, argRotate->count ? Bitmap::Vertically : Bitmap::Horizontally, text.c_str());

   // Write bitmap into file
   // PBM Header
   fwrite("P4\n", 1, 3, fout);
   if (argComment->count)
   {
      const char * const comment = argComment->sval[0];
      fprintf(fout, "#%s\n", comment);
   }
   fprintf(fout, "%u %u\n", bitmap.width, bitmap.height);
   // PBM Bitmap
   fwrite(bitmap.data, 1, bitmap.lengthByte, fout);

   //Close file
   if (fin != stdin) fclose(fin);
   if (fout != stdout) fclose(fout);
   return 0;
}

