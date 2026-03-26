# Protocol

> **Applicability:** The protocol described here applies to the *LabelWriter Wireless* and *LabelWriter 550*, regardless of whether they are connected via TCP (network) or USB. The *LabelWriter 450* uses a slightly different protocol — the following description does not fully apply to it.

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
* The bitmap blob seems to match the *pbm P4* format (see https://en.wikipedia.org/wiki/Netpbm_format)

> I just testes this with a label width that is a multipel of 8. So the wrap around is at a byte boundary, and there are no unused trailing bits in the blob. Don't know what happens if this isn't considered!?


## Protocol

Example for printing a 272x252 bitmap to a 25mm X 25mm label with 300x300dpi.

1. Open TCP connection

2. Get status of printer
   1. `0x1B, 0x41, 1` : **A** Send a TCP packet containing 3 bytes (2 byte command, 1 unknown byte)
   2. Receive reply from printer (32 bytes - see doc/cmd_status.md for status details)

3. Send label data

   1. Configure label settings
      1. `0x1B, 0x73, 1, 0, 0, 0` : **s** Session-Counter, 2 byte command, 32-bit value (meaning of counter unknown)
      2. `0x1B, 0x43, 0x64` : **C** Print-Density, 2 byte command, 1 byte value (0x64 ^= normal)
      3. `0x1B, 0x68` : **h** Printer in 300x300 dpi text mode
      4. `0x1B, 0x4D, 0, 0, 0, 0, 0, 0, 0, 0` : **M** Media/Paper-Type, 2 byte command, 8 byte value (8x0 ^= normal)

   2. Setup label
      1. `0x1B, 0x6E, 1, 0` : **n** Label index, 2 byte command, 16-bit value (meaning of index unknown)
      2. `0x1B, 0x44, 0x01, 0x02, 0xFC, 0, 0, 0, 0x10, 0x01, 0, 0` : **D** Bitmap spec, 2 byte command, 2 unknown bytes, 32-bit bitmap height, 32-bit bitmap width (height 252px = 0xFC ^= [0xFC, 0, 0, 0]; width 272px = 0x110 ^= [0x10, 0x01, 0, 0])

   3. Send bitmap "blob"

   3. Send "footer data" of label (last packet of blob - MSG_MORE = false)
      1. `0x1B, 0x47` : **G** Short form feed, 2 byte command
      2. `0x1B, 0x41, 0` : **A** Get status, 2 byte command, 1 unknown byte

4. Receive reply from printer (32 bytes - see doc/cmd_status.md for status details)

5. Continue with 3.2 to print the next label (of the same type as configured in 3.1 - but with diffrent content). Goto step 6 when done.

6. Send a TCP packet containing following data to output label
   1. `0x1B, 0x45` : **E** Line feed, 2 byte command
   2. `0x1B, 0x51` : **Q** Line tab, 2 byte command

7. Close TCP connection
