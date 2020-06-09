# Dymo Labelwriter Wireless

## Printer Commands

| CMD | HEX | Description |
|---|---|---|
| A | 1B 41 s0 | Request device status. s0=01: At begin of print job. s0=00: At end of print job. |
| C | 1B 43 d0 | Set print density. d0=4B (light), d0=58 (medium), d0=64 (normal), d0=71 (dark). Default 64 (normal). |
| D | 1B 44 01 02 h0 h1 h2 h3 w0 w1 w2 w3 | Bitmap pixel dimensions. 32-bit width and height value in little endian format. |
| E | 1B 45 | Form Feed. |
| G | 1B 47 | Short Form Feed. |
| L | 1B 4C l0 l1 | Label Length. 16-bit length value in little endian format. |
| M | 1B 4D m0 00 00 00 00 00 00 00 | MediaType. m0=00 standard. m0=01 endurable. |
| Q | 1B 51 | ??? comes at the end of a printing job |
| c | 1B 63 | Print density light |
| d | 1B 64 | Print density medium |
| e | 1B 65 | Print density normal |
| g | 1B 67 | Print density dark |
| h | 1B 68 | Print in 300 x 300 dpi Text Quality mode. This is the default, high speed printing mode. |
| i | 1B 69 | Print in 300 x 600 dpi Barcode and Graphics mode. This results in lower speed but greater positional and sizing accuracy of the print elements. |
| n | 1B 6E i0 i1 | Label index. 16-bit upcounting index (relevant when printing multiple lables, starting with 1), in little endian format. |
| s | 1B 73 s0 s1 s2 s3 | Session id. 32-bit session identifieer. ??? can be set to a random value -> 1. |


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
The meaning of the 32 status bytes read back from the printer are not realy decoded yet. However there are some findings:

First, there seems to be two modes how the get the printer status. I call this *active* and *passive* mode. I think this is also someting like a semaphore acquisition.
- A active status request looks like that: `0x1B, 0x41, 1`. May be something like "aquire_semaphore_and_return_status()"
- A passive status request looks like that: `0x1B, 0x41, 0`. May be something like "just_get_status()"

| Byte | Meaning |Description |
|---|---|---|
| 0 | BUSY | Indicates if the printer is free (BUSY==0) or aquired by someone else (BUSY!=0) |
| 7 | Top Of Form (TOF) | The hole in the label is at the top, so that the label could be precisly teared down (TOF!=0) |
| 15 | Paper Out | There is no more label in the printer (PaperOut!=0) |
| others | todo |




### TODO

In the bytes[1..4] there seems to be the session-counter from the request (cf 3.1.1). Kommt aber nur zum "Vorschein" wenn man mehrere Labels druckt. Und zwar erst ab dem 2ten Status Request mit `0x1B, 0x41, 0` (also nach dem Drucken des 2ten Label). Anfangs kommt immer 4x 0. Nach dem Ende des Druckens kommt auch wieder 4x 0, wenn man z.B. noch ein paar  `0x1B, 0x41, 1` oder `0x1B, 0x41, 0` Request hinterher schickt.

In the bytes[5..6] there seems to be the label-index-counter from the request (cf. 3.1.7). Hier ist es ählich wie beim Session-Counter. Hier kommt der Wert zurück, den man im Vorherigen Request gesetzt hat. Das gilt aber nicht, für das erste `0x1B, 0x41, 1` und das erst `0x1B, 0x41, 0`. Da kommt dann noch der Wert den man Vorletzen Request gemacht hat. Nach dem Ende des Drucks kann man zuletzt gesetzen Counter Wert auch beliebig oft abrufen (indem man `0x1B, 0x41, 1` oder `0x1B, 0x41, 0` Request schickt).

Dieses "komische" Verhalten, hat bestimmt mit Byte[7] zu tun. Dieses Byte ist beim ersten `0x1B, 0x41, 1` und beim ersten `0x1B, 0x41, 0` immer 0. Wenn man mehrere Labels druckt, ist dieses Byte ab den 2ten `0x1B, 0x41, 0` dann 1. Schickt man nach dem Ende `0x1B, 0x41, 1` oder `0x1B, 0x41, 0` Request, bleibe dies Byte auf 1. Das könnte also so eine Art "letzter Job completed" flag sein. Oder (und das erscheint mir jetzt, während ich drüber nachdenkte fast plausibler) ist es ein Idle-Flag das anzeigt ob der Drucker druckt oder nicht. Beim ersten `0x1B, 0x41, 1` ist der Drucker ja noch *Idle*. Nach dem senden des ersten Labels und dem anschließenden `0x1B, 0x41, 0` ist der Drucker auch noch Idle, weil das ganze "sooooo schnell" ging und der Drucker noch garnicht angefangen hat...
Die anderen Status Bytes:
- Byte[10]: Normal 0x08, wenn kein Papier drin ist und mehrere Label gedruckt werden kommt hier ab dem 2ten `0x1B, 0x41, 0` Request eine 2.
- Byte[20]: Immer 0x01
- Byte[21]: Immer 0x01
- Byte[27]: Immer 0x02
- Byte[28]: Immer 0x02
- alle nicht genannten: Immer 0x00


In byte[15] there seems to be the "paper out" information. If paper is present, this byte is 0. If paper is out, this byte is 1.

When requesting the active) status with `0x1B, 0x41, 1` then in byte[0] stands a 0. When requesting the status with `0x1B, 0x41, 0` then in byte[0] stands a 1.
Wenn man z.B. 3 Label druckt aber kein Papier drin ist, dann geht steht hier ab dem 2ten `0x1B, 0x41, 0` request eine 2. Das ist wahrscheinlich ein Fehlerflag

