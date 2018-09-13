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
   0x1B, 0x44, 0x01, 0x02, 0xFC, 0, 0, 0, 0x10, 0x01, 0, 0 //print data header (252 Zeilen x 272 pixel)
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
//   printf("%d\n", (uint8_t)argv[1][0]);
//   return -1;

   const GFXfont * const font = &FreeSans15pt7b;
   Bitmap * const bitmap = new Bitmap(272, 252, font);

//   const char * line1 = "71404194, 01.06.83";
//   bitmap->drawText((bitmap->width - textWidth)/2, font->yAdvance, line1); //centered

   //text
   bitmap->drawText(0, font->yAdvance, "7  2404394, 02.06.18");
   //bitmap->drawText(0, 2*font->yAdvance, "Dimpfelmooser Karl-Heinz");
   bitmap->drawText(0, 2*font->yAdvance, "^A ^a ^O ^o ^U ^u Hei^s ^E"); //Umlaute, Scharfes-S, Euro
   bitmap->drawText(0, 3*font->yAdvance, "JUN, LG, Manns. 28");
   bitmap->drawText(0, 4*font->yAdvance, "Fest, Punkt #123.4");
   //ean8 barcode
   bitmap->drawBarcode(4*font->yAdvance + font->yAdvance/2, 2*font->yAdvance, 8263214);




//   ofstream myFile ("data.bin", ios::out | ios::binary);
//   myFile.write ((const char *)bitmap->data, bitmap->lengthByte);
//   myFile.close();



   int sockfd = createTcpConnection();
   uint8_t receiveBuffer[128];
   int status;

   status = openTcpConnection(sockfd, "192.168.1.146", 9100);
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
   return 0;
}
