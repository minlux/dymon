#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dymon.h"



bool DymonUsb::connect(void * arg)
{
   //try to create socket
   const char * const device = (const char *)arg;
   int fd = ::open(device, O_RDWR);
   if (fd < 0)
   {
      return false;
   }
   sockfd = fd;
   return true;
}


int DymonUsb::send(const uint8_t * data, const size_t dataLen, bool more)
{
   //Send some data
   int status = ::write(sockfd, data, dataLen);
   if (status >= 0) //on success
   {
      return status;
   }
   return -1;

}


int DymonUsb::receive(uint8_t * buffer, const size_t bufferLen)
{
   struct timeval timeout;
   fd_set set;

   //read timeout
   timeout.tv_sec = 5;
   timeout.tv_usec = 0;

   //use "select" for read with timeout
   FD_ZERO(&set); //clear the set
   FD_SET(sockfd, &set); //add our file descriptor to the set
   int status  = select(sockfd + 1, &set, NULL, NULL, &timeout); //returns -1 on error; 0 on timeout; 1 on read
   if (status > 0)
   {
      status = ::read(sockfd, buffer, bufferLen);
      if (status >= 0) //on success
      {
         return status;
      }
   }
   return -1;
}



void DymonUsb::close()
{
   ::close(sockfd);
   sockfd = -1;
}
