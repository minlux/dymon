#include <cstring>
// #include <iostream>
#include "dymon.h"



const uint8_t Dymon::_initial[] = {
   0x1B, 0x41, 1           //status request
};

const uint8_t Dymon::_header[] = {
   0x1B, 0x73, 1, 0, 0, 0, //counter
   0x1B, 0x43, 0x64,       //print density "normal"
   0x1B, 0x4C,             //label length...
   0, 0,                   //...given as multiple of 1/600 inch. (e.g. 600 ^= 600 * 1/600 inch = 1 inch) (16-bit little endian)
   0x1B, 0x68,             //print quality, 300x300dpi
   0x1B, 0x4D, 0, 0, 0, 0, 0, 0, 0, 0, //media type, default
   0x1B, 0x68,
   0x1B, 0x6E, 1, 0,       //label index
   0x1B, 0x44, 0x01, 0x02,
   0, 0, 0, 0,             //label height in pixel (32-bit little endian)
   0, 0, 0, 0              //label width in pixel (32-bit little endian). shall be a multiple of 8!
};

const uint8_t Dymon::_footer[] = {
   0x1B, 0x47,             //short line feed
   0x1B, 0x41, 0           //status request
};

const uint8_t Dymon::_final[] = {
   0x1B, 0x45,             //line feed
   0x1B, 0x51              //line tab
};




int Dymon::start(const char * host, uint16_t port)
{
   uint8_t receiveBuffer[128];
   int status;

   //check host address
   if (host == nullptr)
   {
      return -1;
   }
   //create TCP socket and connect to LabelWriter
   status = this->connect(host, port);
   if (status == 0)
   {
      return -2;
   }
   //request LabelWriter status
   status = this->send(_initial, sizeof(_initial));
   if (status <= 0)
   {
      return -3;
   }
   status = this->receive(receiveBuffer, sizeof(receiveBuffer));
   if (status <= 0)
   {
      return -4;
   }
   //success
   return 0;
}



//bitmap resolution is assumed to be 300dpi
int Dymon::print(const Bitmap * bitmap, double labelLength1mm)
{
   uint8_t receiveBuffer[128];
   int status;
   int error = 0;


   //parameter check
   if ((bitmap == nullptr) || (labelLength1mm <= 0))
   {
      return -1;
   }

   do
   {
      //send label information in header
      uint8_t labelHeader[sizeof(_header)];
      memcpy(labelHeader, _header, sizeof(_header)); //make a modifiable copy of the header
      //set label length
      uint32_t labelLength = (uint32_t)(((600.0 * labelLength1mm) / 25.4) + 0.5); //label length is based on 600 dots per inch
      labelHeader[11] = (uint8_t)labelLength;
      labelHeader[12] = (uint8_t)(labelLength >> 8);
      //set bitmap height
      labelHeader[35] = (uint8_t)bitmap->height;
      labelHeader[36] = (uint8_t)(bitmap->height >> 8);
      labelHeader[37] = (uint8_t)(bitmap->height >> 16);
      labelHeader[38] = (uint8_t)(bitmap->height >> 24);
      //set bitmap width
      labelHeader[39] = (uint8_t)bitmap->width;
      labelHeader[40] = (uint8_t)(bitmap->width >> 8);
      labelHeader[41] = (uint8_t)(bitmap->width >> 16);
      labelHeader[42] = (uint8_t)(bitmap->width >> 24);
      status = this->send(labelHeader, sizeof(labelHeader), true);
      if (status <= 0) { error = -3; break; }

      //send bitmap data
      status = this->send(bitmap->data, bitmap->lengthByte, true);
      if (status <= 0) { error = -3; break; }

      //send label footer + status request
      status = this->send(_footer, sizeof(_footer));
      if (status <= 0) { error = -3; break; }
      status = this->receive(receiveBuffer, sizeof(receiveBuffer));
      if (status <= 0) { error = -4; break; }
   } while (false);
   return error;
}



void Dymon::end()
{
   //send final form-feed command data
   this->send(_final, sizeof(_final));
   //close socket
   this->close();
}