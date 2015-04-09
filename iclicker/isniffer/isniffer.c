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
#include "stdio.h"
#include "isniffer.h"
#include "pm.h"

/* globals */
__bit sleepy;
static volatile u8 rxdone = 0;
__xdata DMA_DESC dmaConfig;
u16 pktcount = 0;

//#define NUM_CLICKERS 100
//xdata clicker clicker_table[NUM_CLICKERS];

#define LEN 9
__xdata u8 rxbuf[LEN];

void setup_dma_rx()
{
	dmaConfig.PRIORITY       = 2;  // high priority
	dmaConfig.M8             = 0;  // not applicable
	dmaConfig.IRQMASK        = 0;  // disable interrupts
	dmaConfig.TRIG           = 19; // radio
	dmaConfig.TMODE          = 0;  // single byte mode
	dmaConfig.WORDSIZE       = 0;  // one byte words;
	dmaConfig.VLEN           = 0;  // use LEN
	SET_WORD(dmaConfig.LENH, dmaConfig.LENL, LEN);

	SET_WORD(dmaConfig.SRCADDRH, dmaConfig.SRCADDRL, &X_RFD);
	SET_WORD(dmaConfig.DESTADDRH, dmaConfig.DESTADDRL, rxbuf);
	dmaConfig.SRCINC         = 0;  // do not increment
	dmaConfig.DESTINC        = 1;  // increment by one

	SET_WORD(DMA0CFGH, DMA0CFGL, &dmaConfig);

	return;
}

void radio_setup() {
	/* IF setting */
    FSCTRL1   = 0x06;
    FSCTRL0   = 0x00;

	/* 905.5 MHz */
	FREQ2     = 0x22;
	FREQ1     = 0xD3;
	FREQ0     = 0xAC;
    CHANNR    = 0x00;

	/* maximum channel bandwidth (812.5 kHz), 152.34 kbaud */
    MDMCFG4   = 0x1C;
    MDMCFG3   = 0x80;

	/* DC blocking enabled, 2-FSK */
	//MDMCFG2   = 0x0l; // 15/16 bit sync
	MDMCFG2   = 0x02; // 16/16 bit sync

	/* no FEC, 2 byte preamble, 250 kHz channel spacing */
    MDMCFG1   = 0x03;
    MDMCFG0   = 0x3B;

	/* 228.5 kHz frequency deviation */
    //DEVIATN   = 0x71;
	/* 253.9 kHz frequency deviation */
    DEVIATN   = 0x72;

    FREND1    = 0x56;   // Front end RX configuration.
    FREND0    = 0x10;   // Front end RX configuration.

	/* automatic frequency calibration */
    MCSM0     = 0x14;
	MCSM1     = 0x30; // TXOFF_MODE = IDLE

    FSCAL3    = 0xE9;   // Frequency synthesizer calibration.
    FSCAL2    = 0x2A;   // Frequency synthesizer calibration.
    FSCAL1    = 0x00;   // Frequency synthesizer calibration.
    FSCAL0    = 0x1F;   // Frequency synthesizer calibration.
    TEST2     = 0x88;   // Various test settings.
    TEST1     = 0x31;   // Various test settings.
    TEST0     = 0x09;   // high VCO (we're in the upper 800/900 band)
    PA_TABLE0 = 0xC0;   // PA output power setting.

	/* no preamble quality check, no address check, no append status */
    //PKTCTRL1  = 0x00;
    //PKTCTRL1  = 0x84;
	/* preamble quality check 2*4=6, address check, append status */
    PKTCTRL1  = 0x45;

	/* no whitening, no CRC, fixed packet length */
    PKTCTRL0  = 0x00;

	/* packet length in bytes */
    PKTLEN    = LEN;

    SYNC1     = 0xB0;
    SYNC0     = 0xB0;
    ADDR      = 0xB0;
}

/* tune the radio to a particular channel */
void tune(char *channame) {
	//FIXME bounds checking
	CHANNR = channel_table[channame[0] - 'A'][channame[1] - 'A'];
}

void poll_keyboard() {
	switch (getkey()) {
	case ' ':
		/* pause */
		while (getkey() == ' ');
		while (getkey() != ' ')
			sleepMillis(200);
		break;
	case KPWR:
		sleepy = 1;
		break;
	default:
		break;
	}
}

void main(void) {
	u16 i;
	u8 button;
	u32 id;
	// just counting all packets for now
	u16 count_a = 0;
	u16 count_b = 0;
	u16 count_c = 0;
	u16 count_d = 0;
	u16 count_e = 0;

reset:
	sleepy = 0;

	xtalClock();
	setIOPorts();
	configureSPI();
	LCDReset();
	radio_setup();
	tune("AA");
	setup_dma_rx();
	clear();

	SSN = LOW;
	setCursor(0, 0);
	printf("isniffer");
	SSN = HIGH;

	while (1) {
		EA = 1; // enable interrupts globally
		IEN2 |= IEN2_RFIE; // enable RF interrupt
		RFIM = RFIM_IM_DONE; // mask IRQ_DONE only
		DMAARM |= DMAARM0;  // Arm DMA channel 0
    	RFST = RFST_SRX;;

		while (!rxdone);
		rxdone = 0;
		pktcount++;
    	RFST = RFST_SIDLE;

		button = ((rxbuf[4] & 1) << 3) | (rxbuf[5] >> 5);
		/*
		if ((button != BUTTON_A)
				&& (button != BUTTON_B)
				&& (button != BUTTON_C)
				&& (button != BUTTON_D)
				&& (button != BUTTON_E))
			continue;
			*/

		/*
		 * What I'm calling an "id" is the entire portion of the packet that is
		 * common to every packet transmitted by a particular iclicker.  It is
		 * probably the hexadecimal ID printed on the back of the unit encoded
		 * somehow.
		 *
		 * hmmm, one off from byte boundaries - probably wrong
		 */
		//id = ((u32)rxbuf[1] << 23) | ((u32)rxbuf[2] << 15)
				//| ((u32)rxbuf[3] << 7) | ((u32)rxbuf[4] >> 1);

		/*
		 * we should be only counting one answer for each id, but for now we
		 * just count every packet
		 */

		switch (button) {
		case BUTTON_A:
			count_a++;
			break;
		case BUTTON_B:
			count_b++;
			break;
		case BUTTON_C:
			count_c++;
			break;
		case BUTTON_D:
			count_d++;
			break;
		case BUTTON_E:
			count_e++;
			break;
		default:
			break;
		}

		SSN = LOW;
		setCursor(2, 0);
		printf("A: %d", count_a);
		setCursor(3, 0);
		printf("B: %d", count_b);
		setCursor(4, 0);
		printf("C: %d", count_c);
		setCursor(5, 0);
		printf("D: %d", count_d);
		setCursor(6, 0);
		printf("E: %d", count_e);
		setCursor(7, 0);
		printf("total packets: %d", pktcount);
		SSN = HIGH;
	}

	while (1) {
		poll_keyboard();

		/* go to sleep (more or less a shutdown) if power button pressed */
		if (sleepy) {
			clear();
			sleepMillis(1000);
			SSN = LOW;
			LCDPowerSave();
			SSN = HIGH;

			while (1) {
				sleep();

				/* power button depressed long enough to wake? */
				sleepy = 0;
				for (i = 0; i < DEBOUNCE_COUNT; i++) {
					sleepMillis(DEBOUNCE_PERIOD);
					if (keyscan() != KPWR) sleepy = 1;
				}
				if (!sleepy) break;
			}

			/* reset on wake */
			goto reset;
		}
	}
}

/* IRQ_DONE interrupt service routine */
void rf_isr() __interrupt (RF_VECTOR) {
	/* clear the interrupt flags */
	RFIF &= ~RFIF_IRQ_DONE;
	S1CON &= ~0x03;           // Clear the general RFIF interrupt registers

	rxdone = 1;
}
