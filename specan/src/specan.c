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

#include <cc1110.h>
#include "ioCCxx10_bitdef.h"
#include "display.h"
#include "keys.h"
#include "5x7.h"
#include "stdio.h"
#include "specan.h"
#include "pm.h"

/* globals */
xdata channel_info chan_table[NUM_CHANNELS];
u16 center_freq;
u16 user_freq;
u8 band;
u8 width;
bit max_hold;
bit height;
bit sleepy;
u8 vscroll;
u8 min_chan;
u8 max_chan;

/* plot one value of bar chart */
void plot(u8 col) {
	u8 section;
	u8 row;
	u8 pixels;
	u8 s, m;

	SSN = LOW;
	setDisplayStart(0);

	if (height == TALL) {
		m = MAX((chan_table[col].max - vscroll), 0);
		s = MAX((chan_table[col].ss - vscroll), 0);
	} else {
		s = MAX((chan_table[col].ss - vscroll) >> 2, 0);
		m = MAX((chan_table[col].max - vscroll) >> 2, 0);
	}

	for (row = 0; row < 6; row++) {
		setCursor(5 - row, col);
		section = s - (8 * row);
		if (s >= (8 * (row + 1)))
			section = 8;
		pixels = 0xff << (8 - section);
		if (m <= (8 * (row + 1)))
			pixels |= (0x01 << ((8 * (row + 1)) - m));
		txData(pixels);
	}

	SSN = HIGH;
}

void draw_ruler() {
	u8 col;
	u8 offset = 0;

	SSN = LOW;

	switch (width) {
	case NARROW:
		setCursor(6, 0);
		for (col = 0; col < NUM_CHANNELS; col++)
			txData(narrow_ruler[col]);
		break;
	case ULTRAWIDE:
		setCursor(6, 0);
		for (col = 0; col < NUM_CHANNELS; col++)
			txData(ultrawide_ruler[col]);
		break;
	default:
		if (center_freq % 10)
			offset = 25;
			
		setCursor(6, 0);
		for (col = 0; col < NUM_CHANNELS; col++)
			txData(wide_ruler[col + offset]);
		break;
	}

	SSN = HIGH;
}

void draw_freq() {
	SSN = LOW;

	switch (width) {
	case NARROW:
		setCursor(7, 18);
		printf("%d", center_freq - 2);

		setCursor(7, 58);
		printf("%d", center_freq);

		setCursor(7, 98);
		printf("%d", center_freq + 2);
		break;
	case ULTRAWIDE:
		setCursor(7, 13);
		printf("%d  %d  %d  %d",
				center_freq - 30,
				center_freq - 10,
				center_freq + 10,
				center_freq + 30);
		break;
	default:
		setCursor(7, 8);
		printf("%d", center_freq - 10);

		setCursor(7, 58);
		printf("%d", center_freq);

		setCursor(7, 108);
		printf("%d", center_freq + 10);
		break;
	}

	SSN = HIGH;
}

void radio_setup() {
	/* IF of 457.031 kHz */
	FSCTRL1 = 0x12;
	FSCTRL0 = 0x00;

	/* disable 3 highest DVGA settings */
	AGCCTRL2 |= AGCCTRL2_MAX_DVGA_GAIN;

	/* frequency synthesizer calibration */
	FSCAL3 = 0xEA;
	FSCAL2 = 0x2A;
	FSCAL1 = 0x00;
	FSCAL0 = 0x1F;

	/* "various test settings" */
	TEST2 = 0x88;
	TEST1 = 0x31;
	TEST0 = 0x09;

	/* no automatic frequency calibration */
	MCSM0 = 0;
}

/* set the channel bandwidth */
void set_filter() {
	/* channel spacing should fit within 80% of channel filter bandwidth */
	switch (width) {
	case NARROW:
		MDMCFG4 = 0xEC; /* 67.708333 kHz */
		break;
	case ULTRAWIDE:
		MDMCFG4 = 0x0C; /* 812.5 kHz */
		break;
	default:
		MDMCFG4 = 0x6C; /* 270.833333 kHz */
		break;
	}
}

/* set the radio frequency in Hz */
void set_radio_freq(u32 freq) {
	/* the frequency setting is in units of 396.728515625 Hz */
	u32 setting = (u32) (freq * .0025206154);
	FREQ2 = (setting >> 16) & 0xff;
	FREQ1 = (setting >> 8) & 0xff;
	FREQ0 = setting & 0xff;

	if ((band == BAND_300 && freq < MID_300) ||
			(band == BAND_400 && freq < MID_400) ||
			(band == BAND_900 && freq < MID_900))
		/* select low VCO */
		FSCAL2 = 0x0A;
	else
		/* select high VCO */
		FSCAL2 = 0x2A;
}

/* freq in Hz */
void calibrate_freq(u32 freq, u8 ch) {
		set_radio_freq(freq);

		RFST = RFST_SCAL;
		RFST = RFST_SRX;

		/* wait for calibration */
		sleepMillis(2);

		/* store frequency/calibration settings */
		chan_table[ch].freq2 = FREQ2;
		chan_table[ch].freq1 = FREQ1;
		chan_table[ch].freq0 = FREQ0;
		chan_table[ch].fscal3 = FSCAL3;
		chan_table[ch].fscal2 = FSCAL2;
		chan_table[ch].fscal1 = FSCAL1;

		/* get initial RSSI measurement */
		chan_table[ch].ss = (RSSI ^ 0x80);
		chan_table[ch].max = 0;

		RFST = RFST_SIDLE;
}

#define UPPER(a, b, c)  ((((a) - (b) + ((c) / 2)) / (c)) * (c))
#define LOWER(a, b, c)  ((((a) + (b)) / (c)) * (c))

/* set the center frequency in MHz */
u16 set_center_freq(u16 freq) {
	u8 new_band;
	u32 spacing;
	u32 hz;
	u32 min_hz;
	u32 max_hz;
	u8 margin;
	u8 step;
	u16 upper_limit;
	u16 lower_limit;
	u16 next_up;
	u16 next_down;
	u8 next_band_up;
	u8 next_band_down;

	switch (width) {
	case NARROW:
		margin = NARROW_MARGIN;
		step = NARROW_STEP;
		spacing = NARROW_SPACING;
		break;
	case ULTRAWIDE:
		margin = ULTRAWIDE_MARGIN;
		step = ULTRAWIDE_STEP;
		spacing = ULTRAWIDE_SPACING;

		/* nearest 20 MHz step */
		freq = ((freq + 10) / 20) * 20;
		break;
	default:
		margin = WIDE_MARGIN;
		step = WIDE_STEP;
		spacing = WIDE_SPACING;

		/* nearest 5 MHz step */
		freq = ((freq + 2) / 5) * 5;
		break;
	}

	/* handle cases near edges of bands */
	if (freq > EDGE_900) {
		new_band = BAND_900;
		upper_limit = UPPER(MAX_900, margin, step);
		lower_limit = LOWER(MIN_900, margin, step);
		next_up = LOWER(MIN_300, margin, step);
		next_down = UPPER(MAX_400, margin, step);
		next_band_up = BAND_300;
		next_band_down = BAND_400;
	} else if (freq > EDGE_400) {
		new_band = BAND_400;
		upper_limit = UPPER(MAX_400, margin, step);
		lower_limit = LOWER(MIN_400, margin, step);
		next_up = LOWER(MIN_900, margin, step);
		next_down = UPPER(MAX_300, margin, step);
		next_band_up = BAND_900;
		next_band_down = BAND_300;
	} else {
		new_band = BAND_300;
		upper_limit = UPPER(MAX_300, margin, step);
		lower_limit = LOWER(MIN_300, margin, step);
		next_up = LOWER(MIN_400, margin, step);
		next_down = UPPER(MAX_900, margin, step);
		next_band_up = BAND_400;
		next_band_down = BAND_900;
	}

	if (freq > upper_limit) {
		freq = upper_limit;
		if (new_band == band) {
			new_band = next_band_up;
			freq = next_up;
		}
	} else if (freq < lower_limit) {
		freq = lower_limit;
		if (new_band == band) {
			new_band = next_band_down;
			freq = next_down;
		}
	}

	band = new_band;

	/* doing everything in Hz from here on */
	switch (band) {
	case BAND_400:
		min_hz = MIN_400 * 1000000;
		max_hz = MAX_400 * 1000000;
		break;
	case BAND_300:
		min_hz = MIN_300 * 1000000;
		max_hz = MAX_300 * 1000000;
		break;
	default:
		min_hz = MIN_900 * 1000000;
		max_hz = MAX_900 * 1000000;
		break;
	}

	/* calibrate upper channels */
	hz = freq * 1000000;
	max_chan = NUM_CHANNELS / 2;
	while (hz <= max_hz && max_chan < NUM_CHANNELS) {
		calibrate_freq(hz, max_chan);
		hz += spacing;
		max_chan++;
	}

	/* calibrate lower channels */
	hz = freq * 1000000 - spacing;
	min_chan = NUM_CHANNELS / 2;
	while (hz >= min_hz && min_chan > 0) {
		min_chan--;
		calibrate_freq(hz, min_chan);
		hz -= spacing;
	}

	center_freq = freq;
	clear();
	draw_ruler();
	draw_freq();
	max_hold = 0;

	return freq;
}

/* tune the radio using stored calibration */
void tune(u8 ch) {
	FREQ2 = chan_table[ch].freq2;
	FREQ1 = chan_table[ch].freq1;
	FREQ0 = chan_table[ch].freq0;

	FSCAL3 = chan_table[ch].fscal3;
	FSCAL2 = chan_table[ch].fscal2;
	FSCAL1 = chan_table[ch].fscal1;
}

void set_width(u8 w) {
	width = w;
	set_filter();
	set_center_freq(center_freq);
}

void poll_keyboard() {
	u8 vstep;
	u8 hstep;

	vstep = (height == TALL) ? TALL_STEP : SHORT_STEP;

	switch (width) {
	case NARROW:
		hstep = NARROW_STEP;
		break;
	case ULTRAWIDE:
		hstep = ULTRAWIDE_STEP;
		break;
	default:
		hstep = WIDE_STEP;
		break;
	}

	switch (getkey()) {
	case 'W':
		set_width(WIDE);
		break;
	case 'N':
		set_width(NARROW);
		break;
	case 'U':
		set_width(ULTRAWIDE);
		break;
	case KMNU:
		switch (width) {
		case WIDE:
			set_width(NARROW);
			break;
		case NARROW:
			set_width(ULTRAWIDE);
			break;
		default:
			set_width(WIDE);
			break;
		}
		break;
	case 'T':
		height = TALL;
		break;
	case 'S':
		height = SHORT;
		break;
	case KBYE:
		height = !height;
		break;
	case '>':
		user_freq += hstep;
		break;
	case '<':
		user_freq -= hstep;
		break;
	case '^':
	case 'Q':
		vscroll = MIN(vscroll + vstep, MAX_VSCROLL);
		break;
	case KDWN:
	case 'A':
		vscroll = MAX(vscroll - vstep, MIN_VSCROLL);
		break;
	case 'M':
		max_hold = !max_hold;
		break;
	case KPWR:
		sleepy = 1;
		break;
	default:
		break;
	}
}

void main(void) {
	u8 ch;
	u16 i;

reset:
	center_freq = DEFAULT_FREQ;
	user_freq = DEFAULT_FREQ;
	band = BAND_900;
	width = WIDE;
	max_hold = 0;
	height = 0;
	sleepy = 0;
	vscroll = 0;
	min_chan = 0;
	max_chan = NUM_CHANNELS - 1;

	xtalClock();
	setIOPorts();
	configureSPI();
	LCDReset();
	radio_setup();
	set_width(WIDE);

	while (1) {
		for (ch = min_chan; ch < max_chan; ch++) {
			/* tune radio and start RX */
			tune(ch);
			RFST = RFST_SRX;

			/* plot previous measurement while waiting for RSSI measurement */
			plot(ch);

			/* measurement needs a bit more time in narrow mode */
			if (width == NARROW)
				for (i = 350; i-- ;);

			/* read RSSI */
			chan_table[ch].ss = (RSSI ^ 0x80);
			if (max_hold)
				chan_table[ch].max = MAX(chan_table[ch].ss,
						chan_table[ch].max);
			else
				chan_table[ch].max = 0;

			/* end RX */
			RFST = RFST_SIDLE;
		}

		poll_keyboard();

		/* go to sleep (more or less a shutdown) if power button pressed */
		if (sleepy) {
			clear();
			sleepMillis(1000);
			SSN = LOW;
			LCDPowerSave();
			SSN = HIGH;
			sleep();
			/* reset on wake */
			goto reset;
		}

		if (user_freq != center_freq)
			user_freq = set_center_freq(user_freq);
	}
}
