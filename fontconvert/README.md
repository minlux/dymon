From
https://github.com/adafruit/Adafruit-GFX-Library/tree/master/fontconvert

Adapted in that way, that it will generate all printable ASCII chars from 0x20 .. 0x7E
+ German "Umlaute" + Euro sign.
See the generated header files to identify the "character-codes" of the additional chars... (can be found at the end of the table)

To generate font-headers you need a "font.tff".
E.g. Generate font-header of font "FreeSans.tff" with font-size 18, like this:
./fontconvert ../doc/FreeSans.tff 18 > FreeSans18pt7b.h









To build the executable from source, checkout "libfreetype2" from github:
$ git clone https://github.com/servo/libfreetype2/tree/master/freetype2

Build "libfreetype2":
$ ./autogen.sh 		(if that fails, you neeed to install Automake, Autoconf and Libtool)
$ ./configure
$ make



Then adapt Makefile of "fontconvert". Set compiler include and linker include path.
E.g. 
libfreetype2/freetype2/include 
libfreetype2/freetype2/objs/.libs

Then "fontconvert" can be build:
$ make


