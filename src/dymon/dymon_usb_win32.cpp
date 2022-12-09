#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <winsock.h> //winsock2.h ?
#include "dymon.h"



bool DymonUsb::connect(void * arg)
{
   const char * const device = (const char *)arg;
   HANDLE hdl = CreateFile(device,
      GENERIC_WRITE | GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_ALWAYS,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
      NULL
   );
   if (hdl != INVALID_HANDLE_VALUE)
   {
      sockfd = (intptr_t)hdl;
      return true;
   }
   printf("CreateFile, errno = %d\n", errno);
   return false;
}


int DymonUsb::send(const uint8_t * data, const size_t dataLen, bool more)
{
   //Send some data
   DWORD cnt;
   int status = WriteFile((void *)sockfd, data, dataLen, &cnt, NULL);
   if (status != 0) //on success
   {
      return (int)cnt;
   }
   printf("WriteFile, errno = %d\n", errno);
   return -1;
}


int DymonUsb::receive(uint8_t * buffer, const size_t bufferLen)
{
   DWORD cnt;
   int status = ReadFile((void *)sockfd, buffer, bufferLen, &cnt, NULL);
   if (status != 0)
   {
      return (int)cnt;
   }
   printf("ReadFile, errno = %d\n", errno);
   return -1;
}



void DymonUsb::close()
{
   CloseHandle((void *)sockfd);
   sockfd = -1;
}
