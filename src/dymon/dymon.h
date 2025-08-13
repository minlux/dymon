/*
   This module implements the TCP protocol used to print labels on a DYMO wireless.
   The protocol was reverse engineered. The data are sent in several TCP packages.
   The data "partitioning" into packages is exactly as i have seen in wireshark...
*/

#ifndef DYMON_H_INCLUDED
#define DYMON_H_INCLUDED


#include <stdint.h>
#include <fstream>
#include <string>
#include <vector>


extern "C" {
   extern unsigned int dymonDebug;
}


class Dymon
{
public:
   class Bitmap
   {
   public:
      Bitmap() : width(0), height(0) {}
      Bitmap(uint32_t width, uint32_t height, std::vector<uint8_t> data)
         : width(width), height(height), data(data) {}
      Bitmap(uint32_t width, uint32_t height, const uint8_t * data, uint32_t length)
         : width(width), height(height), data(std::vector(data, data + length)) {}
      static Bitmap fromBytes(const uint8_t * pbm, uint32_t length);
      static Bitmap fromBytes(const std::vector<uint8_t> pbm);
      static Bitmap fromFile(const char * file);

      uint32_t width; //in pixel
      uint32_t height; //in pixel
      std::vector<uint8_t> data;
   };

public:
   Dymon(uint32_t session, bool lw450) : connected(false), lw450flavor(lw450), session(session), index(0) { }
   int start(void * arg); //start calls connect. For DymonNet, arg ist expected to be a ip address (e.g. "192.168.178.21"); For DymonUsb, arg is expected to be the path to the device to be opened
   int read_status(uint8_t mode); //request a status update (mode: 0 ^= passive, 1 ^= active)
   int print(const Bitmap * bitmap, double labelLength1mm, int more); //print Label (can be called several times to print multiple labels)
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
   bool connect(void * arg); //arg: Dymon::Bitmap *
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


// Print the labels as PBM files into the given output directory
class DymonFile : public Dymon
{
public:
   DymonFile(const char * const outDirectory);

private:
   bool connect(void * arg);
   int send(const uint8_t * data, const size_t dataLen, bool more = false);
   int receive(uint8_t * buffer, const size_t bufferLen);
   void close(); 

private:
   std::string outDirectory;
   std::ofstream outFile;
   uint32_t counter;
};


#endif
