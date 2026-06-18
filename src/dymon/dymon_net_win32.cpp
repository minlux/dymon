#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "cJSON.h"
#include "dymon.h"

#define _CONNECT_TIMEOUT_1SEC    (5)      //seconds connect timeout
#define _SEND_RECV_TIMEOUT_1MS   (5000)   //milli seconds send/receive timeout


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




bool DymonNet::connect(void * arg)
{
   const char * printerIp = (const char *)arg;
   if (printerIp == nullptr)
   {
      return false;
   }
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

   //resolve hostname or IP address
   struct addrinfo hints = {};
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = IPPROTO_TCP;
   struct addrinfo * res = nullptr;
   if (getaddrinfo(printerIp, nullptr, &hints, &res) != 0 || res == nullptr)
   {
      closesocket(sock);
      return false;
   }
   struct sockaddr_in address = *reinterpret_cast<struct sockaddr_in *>(res->ai_addr);
   freeaddrinfo(res);
   address.sin_port = htons(port);
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


int DymonNet::send(const uint8_t * data, const size_t dataLen, bool more)
{
   //Send some data
   int status = ::send(sockfd, (const char *)data, dataLen, 0); //there is no MSG_MORE flag in windows, but it works anyway :-)
   if (status >= 0) //on success
   {
      return status;
   }
   return -1;
}


int DymonNet::receive(uint8_t * buffer, const size_t bufferLen)
{
   //Receive a reply from the server
   int status = ::recv(sockfd, (char *)buffer, bufferLen, 0);
   if (status >= 0) //on success
   {
      return status;
   }
   return -1;
}


void DymonNet::close()
{
   closesocket(sockfd);
   sockfd = -1;
}
