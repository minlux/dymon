#include <io.h>
#include <winsock2.h>
#include "cJSON.h"
#include "dymon.h"

#define _CONNECT_TIMEOUT_1SEC    (10)      //seconds connect timeout
#define _SEND_RECV_TIMEOUT_1MS   (10000)   //milli seconds send/receive timeout


//This is just a helper class.
//There is one global static instance of that class.
//The constructor of the global instance initializes WSA
//Its destructor performs cleanup!
class WsaHelper
{
public:
   WsaHelper()
   {
      WSADATA wsaData;
      WSAStartup(0x0002, &wsaData);
   }

   ~WsaHelper()
   {
      WSACleanup();
   }
};
static WsaHelper _wasHelper;




bool DymonWin32::connect(void * arg)
{
   if (arg == nullptr)
   {
      return false;
   }
   cJSON * ip = cJSON_GetObjectItemCaseSensitive((cJSON *)arg, "ip"); //get IP
   if (!cJSON_IsString(ip))
   {
      return false;
   }
   const char * printerIp = ip->valuestring;
   constexpr uint16_t port = 9100;


   //try to create socket
   SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (sock == INVALID_SOCKET) //INVALID_SOCKET == ~0 ^= -1
   {
      return false;
   }

   //on success
   //set the socket to non-blocking
   unsigned long iMode = 1;
   ioctlsocket(sock, FIONBIO, &iMode);

   //start connecting to host
   struct sockaddr_in address;  /* the libc network address data structure */
   uint32_t addr = inet_addr(printerIp); //convert string representation of IP address (decimals and dots) to binary;
   address.sin_addr.s_addr = addr; /* assign the address */
   address.sin_port = htons(port);            /* translate int2port num */
   address.sin_family = AF_INET;
   ::connect(sock, (struct sockaddr *)&address, sizeof(address));

   //go back to blocking mode
   iMode = 0;
   ioctlsocket(sock, FIONBIO, &iMode);
   //configure send and receive timeouts
   int sendRecvTimeout = _SEND_RECV_TIMEOUT_1MS;
   setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&sendRecvTimeout, sizeof(sendRecvTimeout));
   setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&sendRecvTimeout, sizeof(sendRecvTimeout));

   //wait for connection, with timeout of 2 seconds
   TIMEVAL connectTimeout;
   connectTimeout.tv_sec = _CONNECT_TIMEOUT_1SEC;
   connectTimeout.tv_usec = 0;
   fd_set writeSet;
   FD_ZERO(&writeSet);
   FD_SET(sock, &writeSet);

   // check if the socket is ready
   select(sock + 1, nullptr, &writeSet, nullptr, &connectTimeout);
   if (FD_ISSET(sock, &writeSet))
   {
      this->sockfd = (int)sock;
      return true;
   }

   //close socket in case of timeout
   closesocket(sock);
   return false;
}


int DymonWin32::send(const uint8_t * data, const size_t dataLen, bool more)
{
   //Send some data
   int status = ::send(sockfd, (const char *)data, dataLen, 0); //there is no MSG_MORE flag in windows, but it works anyway :-)
   if (status >= 0) //on success
   {
      return status;
   }
   return -1;
}


int DymonWin32::receive(uint8_t * buffer, const size_t bufferLen)
{
   //Receive a reply from the server
   int status = ::recv(sockfd, (char *)buffer, bufferLen, 0);
   if (status >= 0) //on success
   {
      return status;
   }
   return -1;
}


void DymonWin32::close()
{
   closesocket(sockfd);
   sockfd = -1;
}
