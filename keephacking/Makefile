libs = display.rel keys.rel
CC=sdcc
CFLAGS=--no-pack-iram
LFLAGS = --xram-loc 0xF000

all: keephacking.hex

%.rel : %.c
	$(CC) $(CFLAGS) -c $<

keephacking.hex: keephacking.rel $(libs)
	sdcc $(LFLAGS) keephacking.rel $(libs)
	packihx <keephacking.ihx >keephacking.hex

install: keephacking.hex
	goodfet.cc erase
	goodfet.cc flash keephacking.hex
verify: keephacking.hex
	goodfet.cc verify keephacking.hex
clean:
	rm -f *.hex *.ihx *.rel *.map *.lnk *.lst *.mem *.sym *.rst *.asm *.lk
