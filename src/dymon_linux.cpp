#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netdb.h>
#include <fcntl.h>
#include "dymon.h"


#define _CONNECT_TIMEOUT_1SEC    (10)      //seconds connect timeout
#define _SEND_RECV_TIMEOUT_1MS   (3000)   //milli seconds send/receive timeout



bool DymonLinux::connect(const char * host, const uint16_t port)
{
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

   //start connecting to host
   struct sockaddr_in address;  /* the libc network address data structure */
   address.sin_addr.s_addr = inet_addr(host); /* assign the address */
   address.sin_port = htons(port);            /* translate int2port num */
   address.sin_family = AF_INET;
   ::connect(sock, (struct sockaddr *)&address, sizeof(address));

   //go back to blocking mode
   fcntl(sock, F_SETFL, flags);
   //configure send and receive timeouts
   int sendRecvTimeout = _SEND_RECV_TIMEOUT_1MS;
   setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&sendRecvTimeout, sizeof(sendRecvTimeout));
   setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&sendRecvTimeout, sizeof(sendRecvTimeout));

   //wait for connection, with timeout of 2 seconds
   struct timeval connectTimeout;
   connectTimeout.tv_sec = _CONNECT_TIMEOUT_1SEC;
   connectTimeout.tv_usec = 0;
   fd_set writeSet;
   FD_ZERO(&writeSet);
   FD_SET(sock, &writeSet);

   // check if the socket is ready
   select(sock + 1, nullptr, &writeSet, nullptr, &connectTimeout);
   if (FD_ISSET(sock, &writeSet))
   {
      sockfd = (int)sock;
      return true;
   }

   //close socket in case of timeout
   ::close(sock);
   return false;

}


int DymonLinux::send(const uint8_t * data, const size_t dataLen, bool more)
{
   //Send some data
   int status = ::send(sockfd, data, dataLen, more ? MSG_MORE : 0);
   if (status >= 0) //on success
   {
      return status;
   }
   return -1;

}


int DymonLinux::receive(uint8_t * buffer, const size_t bufferLen)
{
   //Receive a reply from the server
   int status = ::recv(sockfd, buffer, bufferLen, 0);
   if (status >= 0) //on success
   {
      return status;
   }
   return -1;
}


void DymonLinux::close()
{
   ::close(sockfd);
   sockfd = -1;
}
