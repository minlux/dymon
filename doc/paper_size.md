# Dymo Labelwriter Wireless

## Paper Size Specification

| Label | rcName | OptionId | Format | PageDimensions¹ | PrintableArea¹ | PrintableOrigin¹ | Cmd (ESC L) | Cmd-Arg (lable length²) |
|---|---|---|---|---|---|---|---|---|---|
| Address30252 | 259 | 259 | 28mm x 89mm | 329x2100 | 299x1926 | 18x138 | 1B 4C 46 05 | 0x0546 = 1350 |
| Address30320 | 260 | 260 | 28mm x 89mm | 329x2100 | 299x1926 | 18x138 | 1B 4C 46 05 | 0x0546 = 1350 |
| HandingFileInsert30376 | 261 | 261 | 28mm x 51mm | 330x1200 | 282x976 | 36x188 | 1B 4C 84 03 | 0x0384 = 900 |
| StandardAddress99010 | 262 | 262 | 28mm x 89mm | 329x2100 | 299x1926 | 18x138 | 1B 4C 46 05 | 0x0546 = 1350 |
| Shipping30256 | 263 | 263 | 59mm x 102mm | 694x2400 | 665x2224 | 17x140 | 1B 4C DC 05 | 0x05DC = 1500 |
| Shipping99014 | 264 | 264 | 54mm x 101mm | 638x2382 | 608x2218 | 18x128 | 1B 4C D3 05 | 0x05D3 = 1491 |
| NameBadgeLabel99014 | 265 | 265 | 54mm x 101mm | 638x2382 | 608x2218 | 18x128 | 1B 4C D3 05 | 0x05D3 = 1491 |
| Shipping30323 | 266 | 266 | 54mm x 101mm | 638x2382 | 608x2218 | 18x128 | 1B 4C D3 05 | 0x05D3 = 1491 |
| PCPostage3Part30383 | 267 | 267 | 57mm x 178mm | 675x4200 | 646x4032 | 17x132 | 1B 4C 60 09 1B 65 | 0x0960 = 2400 |
| PCPostage2Part30384 | 268 | 268 | 59mm x 191mm | 694x4500 | 665x4332 | 17x132 | 1B 4C F6 09 1B 65 | 0x09F6 = 2550 |
| Diskette20258 | 270 | 270 | 54mm x 70mm | 638x1650 | 609x1482 | 17x132 | 1B 4C 65 04 | 0x0465 = 1125 |
| Diskette99015 | 271 | 271 | 54mm x 70mm | 638x1650 | 614x1482 | 12x132 | 1B 4C 65 04 | 0x0465 = 1125 |
| Diskette30324 | 272 | 272 | 54mm x 70mm | 638x1650 | 614x1482 | 12x132 | 1B 4C 65 04 | 0x0465 = 1125 |
| ReturnAddress30330 | 273 | 273 | 19mm x 51mm | 225x1200 | 196x1028 | 17x136 | 1B 4C 84 03 | 0x0384 = 900 |
| Address2Up30253 | 274 | 274 | 59mm x 89mm (2*) | 693x2100 | 664x1928 | 17x136 | 1B 4C 46 05 | 0x0546 = 1350 |
| fileFolder2Up30277 | 275 | 275 | 29mm x 87mm (2*) | 338x2062 | 308x1906 | 18x120 | 1B 4C 96 03 | 0x0396 = 918 |
| FileFolder30327 | 276 | 276 | 20mm x 87mm | 235x2062 | 223x1894 | 0x132 | 1B 4C 34 05 | 0x0534 = 1332 |
| Zipdisk30370 | 277 | 277 | 51mm x 60mm | 600x1406 | 570x1238 | 18x132 | 1B 4C EB 03 | 0x03EB = 1003 |
| LargeAddress30321 | 278 | 278 | 36mm x 89mm | 422x2092 | 392x1922 | 18x134 | 1B 4C 46 05 | 0x0546 = 1350 |
| LargeAddress99012 | 279 | 279 | 36mm x 89mm | 422x2092 | 392x1922 | 18x134 | 1B 4C 42 05 | 0x0542 = 1346 |
| NameBadgeLabel30364 | 280 | 280 | 59mm x 102mm | 694x2400 | 665x2224 | 17x140 | 1B 4C DC 05 | 0x05DC = 1500 |
| NameBadgeCard30365 | 281 | 281 | 59mm x 89mm | 696x2100 | 672x1840 | 0x224 | 1B 4C 46 05 | 0x0546 = 1350 |
| AppointmantCard30374 | 282 | 282 | 51mm x 89mm | 600x2100 | 552x1840 | 36x224 | 1B 4C 46 05 | 0x0546 = 1350 |
| VideoSpine30325 | 283 | 283 | 19mm x 149mm | 225x3526 | 201x3358 | 12x132 | 1B 4C 0F 08 | 0x080F = 2063 |
| VideoSpine99016 | 284 | 284 | 22mm x 148mm | 260x3488 | 248x3316 | 0x136 | 1B 4C FC 07 | 0x07FC = 2044 |
| VideoTop30326 | 285 | 285 | 46mm x 78mm | 544x1838 | 515x1670 | 17x132 | 1B 4C C3 04 | 0x04C3 = 1219 |
| VideoTop99016 | 286 | 286 | 49mm x 78mm | 579x1838 | 567x1668 | 0x134 | 1B 4C C3 04 | 0x04C3 = 1219 |
| SuspensuinFile99017 | 287 | 287 | 13mm x 51mm | 150x1200 | 126x1030 | 12x134 | 1B 4C 84 03 | 0x0384 = 900 |
| SmallLeverArch99018 | 288 | 288 | 38mm x 190mm | 449x4488 | 420x4320 | 17x132 | 1B 4C F0 09 | 0x09F0 = 2544 |
| LargeLeverArch99019 | 289 | 289 | 59mm x 190mm | 694x4488 | 665x4320 | 17x132 | 1B 4C F0 09 | 0x09F0 = 2544 |
| AudioCassette30337 | 290 | 290 | 41mm x 89mm | 488x2100 | 459x1926 | 17x138 | 1B 4C 46 05 | 0x0546 = 1350 |
| Video8mm2Up30339 | 291 | 291 | 19mm x 71mm (2*) | 225x1688 | 196x1520 | 17x132 | 1B 4C 78 04 | 0x0478 = 1144 |
| N30333 | 292 | 292 | 25mm x 25mm (2*) | 300x600 | 271x508 | 17x56 | 1B 4C 58 02 | 0x0258 = 600 |
| N30332 | 293 | 293 | 25mm x 25mm | 300x600 | 271x504 | 17x60 | 1B 4C 58 02 | 0x0258 = 600 |
| N30334 | 294 | 294 | 57mm x 32mm | 675x750 | 651x678 | 12x36 | 1B 4C A3 02 | 0x02A3 = 675 |
| N30336 | 295 | 295 | 25mm x 54mm | 300x1276 | 271x1186 | 17x54 | 1B 4C AA 03 | 0x03AA = 938 |
| JewelryLabel2Up30299 | 296 | 296 | 54mm x 22mm (2*) | 640x526 | 628x454 | 0x36 | 1B 4C 33 02 | 0x0233 = 563 |
| PriceTagLabel30373 | 297 | 297 | 25mm x 51mm | 293x1200 | 281x452 | 0x36 | 1B 4C 84 03 | 0x0384 = 900 |
| N30335 | 298 | 298 | 26mm x 30mm (4*) | 304x712 | 292x640 | 0x36 | 1B 4C 90 02 | 0x0290 = 656 |
| N30345 | 299 | 299 | 19mm x 64mm | 225x1500 | 196x1328 | 17x136 | 1B 4C 1A 04 | 0x041A = 1050 |
| N30346 | 300 | 300 | 13mm x 48mm | 150x1126 | 121x954 | 17x136 | 1B 4C 5F 03 | 0x035F = 863 |
| N30347 | 301 | 301 | 25mm x 38mm | 300x900 | 271x728 | 17x136 | 1B 4C EE 02 | 0x02EE = 750 |
| N30348 | 302 | 302 | 23mm x 32mm | 270x750 | 237x578 | 21x136 | 1B 4C A3 02 | 0x02A3 = 675 |
| JewelryLabel11351 | 310 | 310 | 54mm x 22mm | 640x526 | 628x454 | 0x36 | 1B 4C 33 02 | 0x0233 = 563 |
| ReturnAddressInt11352 | 311 | 311 | 25mm x 54mm | 300x1276 | 271x1186 | 17x54 | 1B 4C AA 03 | 0x03AA = 938 |
| MultiPurpose11353 | 312 | 312 | 25mm x 25mm | 300x600 | 271x508 | 17x56 | 1B 4C 58 02 | 0x0258 = 600 |
| MultiPurpose11354 | 313 | 313 | 57mm x 32mm | 675x750 | 651x678 | 12x36 | 1B 4C A3 02 | 0x02A3 = 675 |
| MultiPurpose11355 | 314 | 314 | 19mm x 51mm | 225x1200 | 196x1028 | 17x136 | 1B 4C 84 03 | 0x0384 = 900 |
| WhiteNameBadge11356 | 315 | 315 | 41mm x 89mm | 488x2100 | 459x1926 | 17x138 | 1B 4C 46 05 | 0x0546 = 1350 |
| PCPostageEPS30387 | 316 | 316 | 59mm x 267mm | 694x6300 | 665x6132 | 17x132 | 1B 4C 7A 0D | 0x0D7A = 3450 |
| ContinuousWide | 318 | 318 | 54mm x 279mm | 638x6600 | 608x6428 | 18x136 | 1B 4C FF FF | 0xFFFF = 65535 |
| Banner | 402 | 402 | 54mm x 2709mm | 638x64000 | 600x63724 | 17x134 | 1B 4C FF FF | 0xFFFF = 65535 |
| CDLabel30854 | 319 | 319 | 59mm x 66mm | 693x1560 | 655x1278 | 17x140 | 1B 4C 38 04 | 0x0438 = 1080 |
| BadgeCardLabel30856 | 400 | 400 | 62mm x 103mm | 731x2430 | 672x2170 | 0x224 | 1B 4C EB 05 | 0x05EB = 1515 |
| BadgeLabel30857 | 401 | 401 | 59mm x 102mm | 694x2400 | 665x2224 | 17x140 | 1B 4C DC 05 | 0x05DC = 1500 |
| CDDVDLabel14681 | 403 | 403 | 59mm x 66mm | 693x1560 | 655x1278 | 17x140 | 1B 4C 38 04 | 0x0438 = 1080 |
| CDLabel30886 | 404 | 404 | 39mm x 44mm | 464x1050 | 408x768 | 18x132 | 1B 4C 39 03 | 0x0339 = 825 |

¹) Geometry is given in `width x height`, whereas `width` is based on 300dpi resolution, and `height` is based on 600dpi resolution!

²) Label *length* can be calculated form label *heigth*, using the following formula: `length = height/2 + 300`. \
   Label *length* is encoded into to command sequence as a unsigned 16-bit value in little endian format.
