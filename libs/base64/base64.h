//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Base64 stuff
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef BASE64_DOT_H
#define BASE64_DOT_H

/* -- Includes ------------------------------------------------------------ */
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/* -- Types --------------------------------------------------------------- */


/* -- Global Variables ---------------------------------------------------- */
extern const char g_Base64Alphabet[64];


/* -- Prototypes ---------------------------------------------------------- */
uint32_t base64_encode(char * destination, const uint8_t * source, uint32_t len); //destination will be zero terminated. return length of output (without zero termination)
uint32_t base64_decode(uint8_t * destination, const char * source, uint32_t len); //decode maximal *len* source chars. stop at zero termination of source. return length of decoded output.


/* -- Implementation ------------------------------------------------------ */




#ifdef __cplusplus
} /* end of extern "C" */
#endif


#endif

