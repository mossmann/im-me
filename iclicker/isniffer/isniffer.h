/*
 * Copyright 2010 Michael Ossmann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/* power button debouncing for wake from sleep */
#define DEBOUNCE_COUNT  4
#define DEBOUNCE_PERIOD 50

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define HIBYTE(a)  (u8) ((u16)(a) >> 8 )
#define LOBYTE(a)  (u8)  (u16)(a)

#define SET_WORD(regH, regL, word) \
	do {                           \
		(regH) = HIBYTE( word );   \
		(regL) = LOBYTE( word );   \
	} while (0)

/* codes transmitted by iclicker */
#define BUTTON_A  0x1
#define BUTTON_B  0x5
#define BUTTON_C  0xD
#define BUTTON_D  0xE
#define BUTTON_E  0xA
#define JOIN_CHAN 0x2

typedef struct {
	u32 id;
	u8 button;
} clicker;

/* note sdcc wants reverse bit order from datasheet */
typedef struct {
	u8 SRCADDRH;
	u8 SRCADDRL;
	u8 DESTADDRH;
	u8 DESTADDRL;
	u8 LENH      : 5;
	u8 VLEN      : 3;

	u8 LENL      : 8;

	u8 TRIG      : 5;
	u8 TMODE     : 2;
	u8 WORDSIZE  : 1;

	u8 PRIORITY  : 2;
	u8 M8        : 1;
	u8 IRQMASK   : 1;
	u8 DESTINC   : 2;
	u8 SRCINC    : 2;
} DMA_DESC;

/*
 * Channels are designated by the user by entering a two letter button code.
 * This maps the code to a channel number (905.5 MHz + (250 kHz * n)).
 *
 * 'DA' 905.5 MHz, channel 0
 * 'CC' 907.0 MHz, channel 6
 * 'CD' 908.0 MHz, channel 10
 * 'DB' 909.0 MHz, channel 14
 * 'DD' 910.0 MHz, channel 18
 * 'DC' 911.0 MHz, channel 22
 * 'AB' 913.0 MHz, channel 30
 * 'AC' 914.0 MHz, channel 34
 * 'AD' 915.0 MHz, channel 38
 * 'BA' 916.0 MHz, channel 42
 * 'AA' 917.0 MHz, channel 46
 * 'BB' 919.0 MHz, channel 54
 * 'BC' 920.0 MHz, channel 58
 * 'BD' 921.0 MHz, channel 62
 * 'CA' 922.0 MHz, channel 66
 * 'CB' 923.0 MHz, channel 70
 */
static const u8 channel_table[4][4] = { {46, 30, 34, 38},
                                        {42, 54, 58, 62},
                                        {66, 70, 6,  10},
                                        {0,  14, 22, 18} };

void radio_setup();
//void tune();
void poll_keyboard();
void main(void);
