#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include "cJSON.h"
#include "dymon.h"


#define _CONNECT_TIMEOUT_1SEC    (5)      //seconds connect timeout
#define _SEND_RECV_TIMEOUT_1S    (5)   //milli seconds send/receive timeout

// MSG_MORE (Linux) coalesces successive sends like TCP_CORK. macOS has no
// equivalent send() flag; falling back to 0 sends immediately, which is fine
// since the receiver reassembles the TCP stream regardless.
#ifndef MSG_MORE
#define MSG_MORE 0
#endif



bool DymonNet::connect(void * arg)
{
   const char * printerIp = (const char *)arg;
   if (printerIp == nullptr)
   {
      return false;
   }
   constexpr uint16_t port = 9100;

   //try to create socket
   int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (sock < 0)
   {
      return false;
   }

   //on success
   //set the socket to non-blocking
   int flags = fcntl(sock, F_GETFL, 0);
   fcntl(sock, F_SETFL, flags | O_NONBLOCK);

   //resolve hostname or IP address
   struct addrinfo hints = {};
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = IPPROTO_TCP;
   struct addrinfo * res = nullptr;
   if (getaddrinfo(printerIp, nullptr, &hints, &res) != 0 || res == nullptr)
   {
      ::close(sock);
      return false;
   }
   struct sockaddr_in address = *reinterpret_cast<struct sockaddr_in *>(res->ai_addr);
   freeaddrinfo(res);
   address.sin_port = htons(port);
   ::connect(sock, (struct sockaddr *)&address, sizeof(address));

   //go back to blocking mode
   fcntl(sock, F_SETFL, flags);
   //configure send and receive timeouts
   struct timeval sendTimeout = { 0 };
   struct timeval recvTimeout = { 0 };
   sendTimeout.tv_sec = _SEND_RECV_TIMEOUT_1S;
   recvTimeout.tv_sec = _SEND_RECV_TIMEOUT_1S;
   setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&sendTimeout, sizeof(struct timeval));
   setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&recvTimeout, sizeof(struct timeval));

   //wait for connection, with timeout of 2 seconds
   struct timeval connectTimeout = { 0 };
   connectTimeout.tv_sec = _CONNECT_TIMEOUT_1SEC;
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
   ::close(sock);
   return false;

}


int DymonNet::send(const uint8_t * data, const size_t dataLen, bool more)
{
   //Send some data
   int status = ::send(sockfd, data, dataLen, more ? MSG_MORE : 0);
   if (status >= 0) //on success
   {
      return status;
   }
   return -1;

}


int DymonNet::receive(uint8_t * buffer, const size_t bufferLen)
{
   //Receive a reply from the server
   int status = ::recv(sockfd, buffer, bufferLen, 0);
   if (status >= 0) //on success
   {
      return status;
   }
   return -1;
}


void DymonNet::close()
{
   ::close(sockfd);
   sockfd = -1;
}
