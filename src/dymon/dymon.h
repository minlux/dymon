/*
   This module implements the TCP protocol used to print labels on a DYMO wireless.
   The protocol was reverse engineered. The data are sent in several TCP packages.
   The data "partitioning" into packages is exactly as i have seen in wireshark...
*/

#ifndef DYMON_H_INCLUDED
#define DYMON_H_INCLUDED


#include <stdint.h>
#include "bitmap.h"


extern "C" {
   extern unsigned int dymonDebug;
}


class Dymon
{
public:
   Dymon(uint32_t session, bool lw450) : connected(false), lw450flavor(lw450), session(session), index(0) { }
   int start(void * arg); //start calls connect. For DymonNet, arg ist expected to be a cJSON object, with a string attribute "ip"; For DymonUsb, arg is expected to be the path to the device to be opened
   int read_status(uint8_t mode); //request a status update (mode: 0 ^= passive, 1 ^= active)
   int print(const Bitmap * bitmap, double labelLength1mm, int more); //print Label (can be called several times to print multiple labels)
   int print_bitmap(const char * file); //print bitmap from file to label (bitmap file must be in raw-pmb format (P4))
   void end(); //finalize printing (form-feed) and close socket
   // void _debugEnd(); //close without form feed

   //status information (are read/updated in 'start', 'read_status' and at the end of 'print/print_bitmap')
   inline bool paperOut() { return (status[15] != 0); } //no label in printer
   //??? inline bool topOfForm() { return (status[7] != 0); } //the hole in the label is at the top, so that the label could be precisly teared down
   //??? inline bool printerBusy() { return (status[0] != 0); } //printer is busy - may be allocated by me or by someone other


private:
   //TCP access functions. Must be implemented in derived class!
   virtual bool connect(void * arg) = 0;
   virtual int send(const uint8_t * data, const size_t dataLen, bool more = false) = 0;
   virtual int receive(uint8_t * buffer, const size_t bufferLen) = 0;
   virtual void close() = 0;

private:
   static const uint8_t _configuration[];
   static const uint8_t _labelIndexHeightWidth[];
   static const uint8_t _labelFeedStatus[];
   static const uint8_t _final[];
   bool connected;
   bool lw450flavor;
   uint32_t session;
   uint16_t index;
   uint8_t status[32];
};



class DymonNet : public Dymon
{
public:
   DymonNet(uint32_t session = 1) : Dymon(session, false) { sockfd = -1; };

private:
   //TCP access functions. Must be implemented in derived class!
   bool connect(void * arg); //arg: cJSON *
   int send(const uint8_t * data, const size_t dataLen, bool more = false);
   int receive(uint8_t * buffer, const size_t bufferLen);
   void close();


private:
   int sockfd;
};



class DymonUsb : public Dymon
{
public:
   DymonUsb(uint32_t session = 1, bool lw450 = false) : Dymon(session, lw450) { sockfd = -1; };

private:
   //TCP access functions. Must be implemented in derived class!
   bool connect(void * arg); //arg: const char * devicePath
   int send(const uint8_t * data, const size_t dataLen, bool more = false);
   int receive(uint8_t * buffer, const size_t bufferLen);
   void close();


private:
   int sockfd;
};




#endif
