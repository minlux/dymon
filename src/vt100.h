#ifndef VT100_H_INCLUDED
#define VT100_H_INCLUDED


#ifndef _WIN32
   #define USE_VT100 //do not use VT100 commands on windows (as there is only little VT100 support on windows...)
#endif

#ifdef USE_VT100
   #define VT100_RESET             "\x1B[0m"
   #define VT100_BOLD              "\x1B[1m"
   #define VT100_UNDERLINED        "\x1B[4m"
   #define VT100_BOLD_UNDERLINED   "\x1B[1;4m"
   #define VT100_RED               "\x1B[31m"
   #define VT100_GREEN             "\x1B[32m"
   #define VT100_CLEAR_SCREEN      "\x1B[2J"
#else
   #define VT100_RESET
   #define VT100_BOLD
   #define VT100_UNDERLINED
   #define VT100_BOLD_UNDERLINED
   #define VT100_RED
   #define VT100_GREEN
   #define VT100_CLEAR_SCREEN
#endif


#endif
