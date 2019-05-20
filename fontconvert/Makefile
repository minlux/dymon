all: fontconvert

CC     = gcc
CFLAGS = -Wall -I../libfreetype2/freetype2/include -L../libfreetype2/freetype2/objs/.libs
LIBS   = -lfreetype

fontconvert: fontconvert.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@
	strip $@

clean:
	rm -f fontconvert
