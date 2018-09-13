# dymon

## Intro
2018 I bought DYMO's new wireless label printer (*LabelWriter Wireless*). I had the idea to print labels out of an (web)application
containing data like numbers and barcodes dynamically retrieved from a database.

My starting point was the DYMO's LabelWriter SDK. Unfortunately there was little information for LabelWriter Wireless. If found
*LabelWriter 450 Series Printers Technical Reference Manual* via Google but none for *LabelWriter Wireless*. I get in contact with
DYMO's support and asked for a *Reference Manual* for *LabelWriter Wireless* but I got only the one for *LabelWriter 450*.

The *Reference Manual* for *LabelWriter Wireless* hat some useful information but not exactly that what I wanted to know. Searching
around the internet ended up in two interessting projects whose sourcecode contains additional informaiton:
- https://sbronner.com/dymoprint.html
- https://github.com/computerlyrik/dymoprint

Nevertheless I still didn't have all required information to "operatate" the *LabelWriter Wireless* out of my own application.
So it was time for *Wireshark* to capture the communication between the DYMO's label creater software *DYML Label* and *LabelWriter Wireless*...  
To make it short: Based on the information provided in the resouces noted above I managed it to figure out the majority of the TCP communication protocol.


## Findings
For communiction with the *LabelWriter Wireless* TCP port 9100 is used. This makes sense, because port 9100 is reserved for PDL (page description language) 
data streams, used for printing to certain network printers. Communicatin is **NOT** encrypted. 

The printer implements a kind of *ESC* protocol. The commands are similar as specified in *LabelWriter 450 Series Printers Technical Reference Manual* but not in all cases. Also there are commands, that
aren't described in this document. 

Each command starts with `0x1B` followed by a "command selector byte" and optional parameter data. These parameter are little endian encoded.

The label itself is given by a bitmap "blob". In a preceeding header some meta information are defined. Among other things the width (number of columns) and height (number of rows) of the bitmap, the label length, print density, print quality...

The bits of the bitmap blob are mapped to label pixels row by row, column by column (Z)
* It starts in upper left corner with bit 7 of the first bitmap-byte. 
* The next pixel (1st row, 2nd column) is followed by bit 6 ...
* There is a wrap around at the end of each row ...  
* It ends in the lower right corner with bit 0 of the last byte.

> I just testes this with a label width that is a multipel of 8. So the wrap around is at a byte boundary, and there are no unused trailing bits in the blob. Don't know what happens if this isn't considered!?


## Protocol
Example for printing a 252x272 bitmap to a 25mm X 25mm label with 300x300dpi.

1. Open TCP connection

2. Get status of printer
   1. `0x1B, 0x41, 1` : Send a TCP packet containing 3 bytes (2 byte command, 1 unknown byte)
   2. Receive reply from printer (32 bytes, whose meaning is not known, yet!)

3. Send label data as **ONE** blob 
   Either one big packet or multible packets with (all packets but the last) MSG_MOREflag set. 

   1. Set "header data" of label to be print (may be one packet with MSG_MORE)
      1. `0x1B, 0x41, 1, 0, 0, 0` : Counter, 2 byte command, 32-bit value (meaning of counter unknown)
      2. `0x1B, 0x43, 0x64` : Print-Density, 2 byte command, 1 byte value (0x64 ^= normal)
      3. `0x1B, 0x4C, 0x58, 0x02` Label-Length, 2 byte command, 16-bit value (600 ^= 0x258 ^= [0x58, 0x02])
      4. `0x1B, 0x43` : Printer quality (300x300 dpi)
      5. `0x1B, 0x4D, 0, 0, 0, 0, 0, 0, 0, 0` : Media/Paper-Type, 2 byte command, 8 byte value (8x0 ^= normal)
      6. `0x1B, 0x68` : Unknown 2 byte command
      7. `0x1B, 0x6E, 1, 0` : Label index, 2 byte command, 16-bit value (meaning of index unknown)
      8. `0x1B, 0x44, 0x01, 0x02, 0xFC, 0, 0, 0, 0x10, 0x01, 0, 0` : Bitmap spec, 2 byte command, 1 unknown byte, 32-bit bitmap width, 1 unknown byte, 32-bit bitmap height (width 252px = 0xFC ^= [0xFC, 0, 0, 0]; height 272px = 0x110 ^= [0x10, 0x01, 0, 0])

   2. Send bitmap "blob" (may be next packet with MSG_MORE)

   3. Send "footer data" of label (last packet of blob - MSG_MORE clear!)
      1. `0x1B, 0x47` : Short form feed, 2 byte command
      2. `0x1B, 0x41, 0` : Get status, 2 byte command, 1 unknown byte

4. Receive reply from printer (32 bytes, whose meaning is not known, yet!)

5. Send a TCP packet containing following data to output label
   1. `0x1B, 0x45` : Line feed, 2 byte command
   2. `0x1B, 0x51` : Line tab, 2 byte command

6. Close TCP connection


> Label length is user for form feed. Its scale is 600dpi. In this example: 600 dots * 600 dpi = 1 inch = 25 mm.


## Implementation
This command line tool for linux implements the the described application.

However most of the application code relates to the creation of the bitmap (text and barcode) but isn't realy in scope of this project. All code related to the communictation with the label writer is done in *main.cpp*.

## How to build
Build process is based on CMake.
 
1. Create a build directory (e.g. `mkdir build`)
2. Within the build directory execute `cmake ..`
3. Within the build directory execute `make`





