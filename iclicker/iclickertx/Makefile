#libs = rnd_rssi.rel
CC=sdcc
CFLAGS=--no-pack-iram
LFLAGS = --xram-loc 0xF000

all: iclickertx.hex

%.rel : %.c
	$(CC) $(CFLAGS) -c $<

iclickertx.hex: iclickertx.rel $(libs)
	sdcc $(LFLAGS) iclickertx.rel $(libs)
	packihx <iclickertx.ihx >iclickertx.hex

install: iclickertx.hex
	goodfet.cc erase
	goodfet.cc flash iclickertx.hex
verify: iclickertx.hex
	goodfet.cc verify iclickertx.hex
clean:
	rm -f *.hex *.ihx *.rel *.map *.lnk *.lst *.mem *.sym *.rst *.asm
