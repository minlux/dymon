#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <winsock.h> //winsock2.h ?
#include "dymon.h"



static BOOL ReadWithTimeout(HANDLE h, void *buf, DWORD len, DWORD timeoutMs, DWORD *bytesRead) {
    OVERLAPPED ov = {0};
    ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!ov.hEvent) return FALSE;

    BOOL ok = ReadFile(h, buf, len, NULL, &ov);
    if (!ok && GetLastError() == ERROR_IO_PENDING) {
        DWORD waitRes = WaitForSingleObject(ov.hEvent, timeoutMs);
        if (waitRes == WAIT_OBJECT_0) {
            ok = GetOverlappedResult(h, &ov, bytesRead, FALSE);
        } else if (waitRes == WAIT_TIMEOUT) {
            CancelIo(h);  // abort the pending read
            SetLastError(ERROR_TIMEOUT);
            ok = FALSE;
        } else {
            ok = FALSE;
        }
    } else if (ok) {
        GetOverlappedResult(h, &ov, bytesRead, FALSE);
    }

    CloseHandle(ov.hEvent);
    return ok;
}


static BOOL BlockingWrite(HANDLE h, const void *buf, DWORD len, DWORD *bytesWritten) {
    OVERLAPPED ov = {0};
    ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!ov.hEvent) return FALSE;

    BOOL ok = WriteFile(h, buf, len, NULL, &ov);
    if (!ok && GetLastError() == ERROR_IO_PENDING) {
        // Wait indefinitely for write to finish
        if (WaitForSingleObject(ov.hEvent, INFINITE) == WAIT_OBJECT_0) {
            ok = GetOverlappedResult(h, &ov, bytesWritten, FALSE);
        }
    } else if (ok) {
        GetOverlappedResult(h, &ov, bytesWritten, FALSE);
    }

    CloseHandle(ov.hEvent);
    return ok;
}



bool DymonUsb::connect(void * arg)
{
   const char * const device = (const char *)arg;
    HANDLE hdl = CreateFile(device,
      GENERIC_WRITE | GENERIC_READ,
      0, // no sharing
      NULL,
      OPEN_EXISTING,         // device file must exist
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // | FILE_FLAG_WRITE_THROUGH, FILE_FLAG_SEQUENTIAL_SCAN, FILE_FLAG_NO_BUFFERING (dangerous!)
      NULL
   );
   if (hdl != INVALID_HANDLE_VALUE)
   {
      sockfd = (intptr_t)hdl;
      return true;
   }
   printf("CreateFile, errno=%d, lastError=%lu\n", errno, GetLastError());
   return false;
}


int DymonUsb::send(const uint8_t * data, const size_t dataLen, bool more)
{
   //Send some data
   DWORD cnt;
   int status = BlockingWrite((void *)sockfd, data, dataLen, &cnt);
   if (status != 0) //on success
   {
      return (int)cnt;
   }
   printf("WriteFile, errno=%d, lastError=%lu\n", errno, GetLastError());
   return -1;
}


int DymonUsb::receive(uint8_t * buffer, const size_t bufferLen)
{
   DWORD cnt;
   int status = ReadWithTimeout((void *)sockfd, buffer, bufferLen, 5000, &cnt);
   if (status != 0)
   {
      return (int)cnt;
   }
   printf("ReadFile, errno=%d, lastError=%lu\n", errno, GetLastError());
   return -1;
}



void DymonUsb::close()
{
   CloseHandle((void *)sockfd);
   sockfd = -1;
}
