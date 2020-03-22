/*
   This module implements the TCP protocol used to print labels on a DYMO wireless.
   The protocol was reverse engineered. The data are sent in several TCP packages.
   The data "partitioning" into packages is exactly as i have seen in wireshark...
*/

#ifndef DYMON_H_INCLUDED
#define DYMON_H_INCLUDED


#include <stdint.h>
#include "bitmap.h"

class Dymon
{
public:
   Dymon(uint32_t session) { this->session = session, index = 0; ipv4 = 0; }
   int start(const char * host, uint16_t port = 9100); //create TCP socket and connect to LabelWriter
   int print(const Bitmap * bitmap, double labelLength1mm); //print Label (can be called several times to print multiple labels)
   int print_bitmap(const char * file); //print bitmap from file to label (bitmap file must be in raw-pmb format (P4))
   void end(); //finalize printing (form-feed) and close socket
   inline bool isConnectedTo(const char * host) { (inetAddr(host) == ipv4); }; //checks if this Dymo-object is connected to a specific host

   //helper function!!!
   virtual uint32_t inetAddr(const char * host) = 0; //returns the ipV4 address of the given host (0 in case of error)

private:
   //TCP access functions. Must be implemented in derived class!
   virtual bool connect(const char * host, const uint16_t port) = 0;
   virtual int send(const uint8_t * data, const size_t dataLen, bool more = false) = 0;
   virtual int receive(uint8_t * buffer, const size_t bufferLen) = 0;
   virtual void close() = 0;
   virtual void sleep1ms(uint32_t millis) = 0;

private:
   static const uint8_t _status[];
   static const uint8_t _configuration[];
   static const uint8_t _labelIndex[];
   static const uint8_t _labelHeightWidth[];
   static const uint8_t _labelFeed[];
   static const uint8_t _labelStatus[];
   static const uint8_t _final[];
   uint32_t session;
   uint16_t index;
protected:
   uint32_t ipv4; //ipV4 internet address we are connected to (0 if not connected!)
};



//shall handle timeouts
class  DymonWin32 : public Dymon
{
public:
   DymonWin32(uint32_t session = 1) : Dymon(session) { sockfd = -1; };

private:
   //TCP access functions. Must be implemented in derived class!
   bool connect(const char * host, const uint16_t port);
   int send(const uint8_t * data, const size_t dataLen, bool more = false);
   int receive(uint8_t * buffer, const size_t bufferLen);
   void close();
   void sleep1ms(uint32_t millis);
   uint32_t inetAddr(const char * host); //returns the ipV4 address of the given host (0 in case of error)


private:
   int sockfd;
};



//shall handle timeouts
class  DymonLinux : public Dymon
{
public:
   DymonLinux(uint32_t session = 1) : Dymon(session) { sockfd = -1; };

private:
   //TCP access functions. Must be implemented in derived class!
   bool connect(const char * host, const uint16_t port);
   int send(const uint8_t * data, const size_t dataLen, bool more = false);
   int receive(uint8_t * buffer, const size_t bufferLen);
   void close();
   void sleep1ms(uint32_t millis);
   uint32_t inetAddr(const char * host); //returns the ipV4 address of the given host (0 in case of error)

private:
   int sockfd;
};



#endif
