#ifndef DYMON_H_INCLUDED
#define DYMON_H_INCLUDED


#include <stdint.h>
#include "bitmap.h"

class Dymon
{
public:
   int start(const char * host, uint16_t port = 9100); //create TCP socket and connect to LabelWriter
   int print(const Bitmap * bitmap, double labelLength1mm); //print Label (can be called several times to print multiple labels)
   int print_bitmap(const char * filepath, const uint32_t width, const uint32_t height, double labelLength1mm); //print bitmap from file to label
   void end(); //finalize printing (form-feed) and close socket

private:
   //TCP access functions. Must be implemented in derived class!
   virtual bool connect(const char * host, const uint16_t port) = 0;
   virtual int send(const uint8_t * data, const size_t dataLen, bool more = false) = 0;
   virtual int receive(uint8_t * buffer, const size_t bufferLen) = 0;
   virtual void close() = 0;

private:
   static const uint8_t _initial[];
   static const uint8_t _header[];
   static const uint8_t _footer[];
   static const uint8_t _final[];
};



//shall handle timeouts
class  DymonWin32 : public Dymon
{
public:
   DymonWin32() { sockfd = -1; };

private:
   //TCP access functions. Must be implemented in derived class!
   bool connect(const char * host, const uint16_t port);
   int send(const uint8_t * data, const size_t dataLen, bool more = false);
   int receive(uint8_t * buffer, const size_t bufferLen);
   void close();

private:
   int sockfd;
};



//shall handle timeouts
class  DymonLinux : public Dymon
{
public:
   DymonLinux() { sockfd = -1; };

private:
   //TCP access functions. Must be implemented in derived class!
   bool connect(const char * host, const uint16_t port);
   int send(const uint8_t * data, const size_t dataLen, bool more = false);
   int receive(uint8_t * buffer, const size_t bufferLen);
   void close();

private:
   int sockfd;
};



#endif
