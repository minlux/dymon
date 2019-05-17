/*
   Command line based print tool for DYMO LabelWriter Wireless.
   Contains protocol description to interface with DYMO LabelWriter Wireless.

   Compile and build the programm using *cmake*.
   Invoke from command line, like this:

   ./dymon '{"lp":"192.168.178.49","format":2,"lines":["Hallo äÄöÖüÜß€","Zeile 23456789abcdefghijklm","","Z4"],"barcodes":[7531234]}'
*/
#include <stdint.h>
#include <iostream>
#include <fstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netdb.h>


#include "gfxfont.h"
#include "FreeSans15pt7b.h"

#include "bitmap.h"
#include "glyphIterator.h"

#include "cJSON.h"



const uint8_t dymoInitial[] = {
   0x1B, 0x41, 1           //status request
};
const uint8_t dymoHeader[] = {
   0x1B, 0x73, 1, 0, 0, 0, //counter
   0x1B, 0x43, 0x64,       //print density "normal"
   0x1B, 0x4C, 0x58, 0x02, //label length, 600dpi
   0x1B, 0x68,             //print quality, 300x300dpi
   0x1B, 0x4D, 0, 0, 0, 0, 0, 0, 0, 0, //media type, default
   0x1B, 0x68,
   0x1B, 0x6E, 1, 0,       //label index
   0x1B, 0x44, 0x01, 0x02, 0xFC, 0, 0, 0, 0x10, 0x01, 0, 0 //print data header (252 rows x 272 pixel)
};
const uint8_t dymoFooter[] = {
   0x1B, 0x47,             //short line feed
   0x1B, 0x41, 0           //status request
};
const uint8_t dymoFinal[] = {
   0x1B, 0x45,             //line feed
   0x1B, 0x51              //line tab
};


using namespace std;



/*
//test function. argument shall looks like that!!!
static const char * getUtf8Request(void)
{
   static const char * jsonUtf8 = "{" \
      "\"lp\":\"192.168.178.49\"," \
      "\"format\":2," \
      "\"lines\":[" \
         "\"4194 Manuel Hei\xC3\x9F\"," \
         "\"BR\xE2\x82\xAC BR\xC3\xBC_M1\"," \
         "\"EIN PKT MST\"," \
         "\"#97531 (13.05.19)\"" \
      "]," \
      "\"barcodes\":[" \
         "24689" \
      "]" \
   "}";
   return jsonUtf8;
}
*/




int createTcpConnection(void)
{
   //create socket
   int sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock < 0)
   {
      cout << "Failed to create socket!" << endl;
      return -1;
   }
   return sock;
}

int openTcpConnection(int sockfd, const char * ipAddrString, const uint16_t ou16_Port)
{
   struct sockaddr_in server;
   int status;

   //set server address
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = inet_addr(ipAddrString);
   server.sin_port = htons(ou16_Port);
   //try to connect to server
   status = connect(sockfd , (struct sockaddr *)&server , sizeof(server));
   if (status < 0)
   {
      cout << "Failed to connect to server!" << endl;
      return -1;
   }
   return status;
}

int sendViaTcpConnection(int sockfd, const uint8_t * data, const size_t dataLen, int flags=0)
{
   int status;

   //Send some data
   status = send(sockfd, data, dataLen, flags);
   if (status < 0)
   {
      cout << "Failed to send data to server!" << endl;
      return -1;
   }
   return status;
}


int receiveViaTcpConnection(int sockfd, uint8_t * buffer, const size_t bufferLen)
{
   int status;

   //Receive a reply from the server
   status = recv(sockfd, buffer, bufferLen, 0);
   if (status < 0)
   {
      cout << "Failed to receive data from server!" << endl;
      return -1;
   }
   return status;
}


void closeTcpConnection(int sockfd)
{
   close(sockfd);
}





int main(int argc, char * argv[])
{
   if (argc < 2)
   {
      cout << "Error: No argument given. Expect json object as (first) argument!" << endl;
      return -1;
   }


   const char * const request = argv[1]; //getUtf8Request();
   cJSON * const json = cJSON_Parse(request);
   if (cJSON_IsObject(json))
   {
      cJSON * lp = cJSON_GetObjectItemCaseSensitive(json, "lp");
      if (cJSON_IsString(lp))
      {
         //Printer-IP
         const char * printerIp = lp->valuestring;
         cJSON * format = cJSON_GetObjectItemCaseSensitive(json, "format");
         if (cJSON_IsNumber(format))
         {
            //Lable-Format
//todo            const int labelFormat = format->valueint; //currently not supported
            const GFXfont * const font = &FreeSans15pt7b;
            Bitmap * const bitmap = new Bitmap(272, 252, font /*, Bitmap::Orientation::Vertically*/);
            uint32_t y = 0; //Y-coordinate of the bitmap

            //Text-Lines
            cJSON * lines = cJSON_GetObjectItemCaseSensitive(json, "lines");
            cJSON * line;
            cJSON_ArrayForEach(line, lines)
            {
               if (cJSON_IsString(line))
               {
                  y += font->yAdvance; //text is bottom based. so i have to pre-increment Y by the line height
                  bitmap->drawText(0, y, line->valuestring);
               }
            }
            //Barcodes
            cJSON * barcodes = cJSON_GetObjectItemCaseSensitive(json, "barcodes");
            cJSON * barcode;
            cJSON_ArrayForEach(barcode, barcodes)
            {
               if (cJSON_IsNumber(barcode))
               {
                  //ean8 barcode
                  y += font->yAdvance / 2; //add "margin" of 0.5 lines to previous text/barcode
                  bitmap->drawBarcode(y, 2*font->yAdvance, barcode->valueint);
                  y += 2 * font->yAdvance; //add height of barcode to y-coordinate
               }
            }


            //print
            int sockfd = createTcpConnection();
            uint8_t receiveBuffer[128];
            int status = openTcpConnection(sockfd, printerIp, 9100);
            if (status >= 0)
            {
               cout << "Connection established!" << endl;

               status = sendViaTcpConnection(sockfd, dymoInitial, sizeof(dymoInitial));
               cout << "Initial status request sent: " << status << endl;

               status = receiveViaTcpConnection(sockfd, receiveBuffer, sizeof(receiveBuffer));
               cout << "Response received: " << status << endl;


               status = sendViaTcpConnection(sockfd, dymoHeader, sizeof(dymoHeader), MSG_MORE);
               cout << "Header sent: " << status << endl;

               status = sendViaTcpConnection(sockfd, bitmap->data, bitmap->lengthByte, MSG_MORE);
               cout << "Bitmap sent: " << status << endl;

               status = sendViaTcpConnection(sockfd, dymoFooter, sizeof(dymoFooter));
               cout << "Footer status request sent: " << status << endl;


               status = receiveViaTcpConnection(sockfd, receiveBuffer, sizeof(receiveBuffer));
               cout << "Response received: " << status << endl;

               status = sendViaTcpConnection(sockfd, dymoFinal, sizeof(dymoFinal));
               cout << "Final status request sent: " << status << endl;
            }
            closeTcpConnection(sockfd);
            cout << "Connection closed!" << endl;
         }
      }

      cJSON_Delete(json);
      return 0;
   }
   cout << "Error: Given argument is not a valid json object fo the expected format!" << endl;
   return -1;
}
