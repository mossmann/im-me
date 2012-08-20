libs = display.rel keys.rel
CC=sdcc
CFLAGS=--no-pack-iram
LFLAGS = --xram-loc 0xF000

all: opensesame.hex

%.rel : %.c
	$(CC) $(CFLAGS) -c $<

opensesame.hex: opensesame.rel $(libs)
	sdcc $(LFLAGS) opensesame.rel $(libs)
	packihx <opensesame.ihx >opensesame.hex

install: opensesame.hex
	goodfet.cc erase
	goodfet.cc flash opensesame.hex
verify: opensesame.hex
	goodfet.cc verify opensesame.hex
clean:
	rm -f *.hex *.ihx *.rel *.map *.lnk *.lst *.mem *.sym *.rst *.asm
