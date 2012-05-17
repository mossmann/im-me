#include <cc1110.h>
#include "cc1110-ext.h"
#include "types.h"

#define HIBYTE(a)     (u8) ((u16)(a) >> 8 )
#define LOBYTE(a)     (u8)  (u16)(a)

#define SET_WORD(regH, regL, word) \
	do {                           \
		(regH) = HIBYTE( word );   \
		(regL) = LOBYTE( word );   \
	} while (0)

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

static volatile u8 txdone = 0;

xdata DMA_DESC dmaConfig;

/* code captured from iclicker */
#define LEN 7
xdata u8 buf[] = {
	0xb0, // really a third byte of sync, but cc1110 only allows 2 or 4
	0xaf, 0xa2, 0xbf, 0x5a, 0x2b, 0xa0 //last 5 bits are extraneous
};

void setup_dma_tx()
{
	dmaConfig.PRIORITY       = 2;  // high priority
	dmaConfig.M8             = 0;  // not applicable
	dmaConfig.IRQMASK        = 0;  // disable interrupts
	dmaConfig.TRIG           = 19; // radio
	dmaConfig.TMODE          = 0;  // single byte mode
	dmaConfig.WORDSIZE       = 0;  // one byte words;
	dmaConfig.VLEN           = 0;  // use LEN
	SET_WORD(dmaConfig.LENH, dmaConfig.LENL, LEN);

	SET_WORD(dmaConfig.SRCADDRH, dmaConfig.SRCADDRL, buf);
	SET_WORD(dmaConfig.DESTADDRH, dmaConfig.DESTADDRL, &X_RFD);
	dmaConfig.SRCINC         = 1;  // increment by one
	dmaConfig.DESTINC        = 0;  // do not increment

	SET_WORD(DMA0CFGH, DMA0CFGL, &dmaConfig);

	return;
}

int main(void)
{
    SLEEP &= ~SLEEP_OSC_PD;
    while( !(SLEEP & SLEEP_XOSC_S) );
    CLKCON = (CLKCON & ~(CLKCON_CLKSPD | CLKCON_OSC)) | CLKSPD_DIV_1;
    while (CLKCON & CLKCON_OSC);
    SLEEP |= SLEEP_OSC_PD;

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
	MDMCFG2   = 0x02; // 16/16 bit sync

	/* no FEC, 2 byte preamble, 250 kHz channel spacing */
    MDMCFG1   = 0x03;
    MDMCFG0   = 0x3B;

	/* 203 kHz frequency deviation */
    DEVIATN   = 0x70;

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

	/* no preamble quality check, no address check */
    PKTCTRL1  = 0x04;

	/* no whitening, no CRC, fixed packet length */
    PKTCTRL0  = 0x00;

	/* packet length in bytes */
    PKTLEN    = LEN;

    SYNC1     = 0xB0;
    SYNC0     = 0xB0;

	setup_dma_tx();

	EA = 1; // enable interrupts globally
	IEN2 |= IEN2_RFIE; // enable RF interrupt
	RFIM = RFIM_IM_DONE; // mask IRQ_DONE only
	DMAARM |= DMAARM0;  // Arm DMA channel 0
    RFST = RFST_STX;;

	while (!txdone);

    RFST = RFST_SIDLE;
    while (1);
}

/* IRQ_DONE interrupt service routine */
void rf_isr() __interrupt (RF_VECTOR) {
	/* clear the interrupt flags */
	RFIF &= ~RFIF_IRQ_DONE;
	S1CON &= ~0x03;           // Clear the general RFIF interrupt registers

	txdone = 1;
}
