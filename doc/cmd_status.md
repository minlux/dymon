# Dymo Labelwriter Wireless

## Printer Commands

| CMD | HEX | Description |
|---|---|---|
| A | 1B 41 s0 | Request device status. s0=01: At begin of print job. s0=00: At end of print job. |
| C | 1B 43 d0 | Set print density. d0=4B, d0=58, d0=64, d0=71. Default 64. |
| D | 1B 44 01 02 h0 h1 h2 h3 w0 w1 w2 w3 | Bitmap pixel dimensions. 32-bit width and height value in little endian format. |
| E | 1B 45 | Form Feed. |
| G | 1B 47 | Short Form Feed. |
| L | 1B 4C l0 l1 | Label Length. 16-bit length value in little endian format. |
| M | 1B 4D m0 00 00 00 00 00 00 00 | MediaType. m0=00 standard. m0=01 endurable. |
| Q | 1B 51 | ??? comes at the end of a printing job |
| h | 1B 68 | Print in 300 x 300 dpi Text Quality mode. This is the default, high speed printing mode. |
| i | 1B 69 | Print in 300 x 600 dpi Barcode and Graphics mode. This results in lower speed but greater positional and sizing accuracy of the print elements. |


### E - Form Feed
This command advances the most recently printed label to a position where it can be torn off. This positioning places the next label beyond the starting print position. Therefore, a reverse-feed will be automatically invoked when printing on the next label. To optimize print speed and eliminate this reverse feeding when printing multiple labels, use the Short Form Feed command (see below) between labels, and the Form Feed command after the last label.


### G - Short Form Feed
Feeds the next label into print position. The most recently printed label will still be partially inside the printer and cannot be torn off. This command is meant to be used only between labels on a multiple label print job.


### L - Label Length
This command indicates the maximum distance the printer should travel while searching for the top-of-form hole or mark. Print lines and lines fed both count towards this total so that a value related to the length of the label to be printed can be used. For normal labels with top-of-form marks, the actual distance fed is adjusted once the top–of-form mark is detected. As a result this command is usually set to a value slightly longer than the true label length to ensure that the top-of-form mark is reached before feeding is terminated.

The length value to be given here must be calculated according the following formula:
```
l = labelHeight / 2 + 300
```
Whereas `labelHeight` is given in 600dpi unit.

Examples:
- Geometry of a 25mm x 25mm label is 300x600 (width = 300 dots, height = 600 dots): l = 600 / 2 + 300 = 600 (^= 0x258) -> l0=58, l1=02
- Geometry of a 36mm x 89mm LargeAddress99012 is 422x2092 (width = 422 dots, height = 2092 dots): l = 2092 / 2 + 300 = 1346 (^= 0x542) -> l0=42, l1=05

This command can also be used to put the printer into continuous feed mode. Any negative value (0x8000 - 0xFFFF) will place the printer in continuous feed mode.

In continuous feed mode the printer will respond to Form Feed (E) and Short Form Feed (G) commands by feeding a few lines out from the current print position. An ESC E command causes the print position to feed to the tear bar and an ESC G causes it to feed far enough so that a reverse feed will not cause lines to overlap.



## Printer Status
32-Byte Printer Status

| Byte | Description |
|---|---|
| 0 | todo |
| .. | todo |
| 31 | todo |

