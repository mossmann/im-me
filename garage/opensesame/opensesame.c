#define BAUDRATE 5000
#define FREQ 390000000 // frequency in Hz (390MHz)
#define CC1110

#include <cc1110.h>
#include "ioCCxx10_bitdef.h"
#include "display.h"
#include "keys.h"
#include "stdio.h"
#include "helpers.h"

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

static volatile u8 txdone = 0;

__xdata DMA_DESC dmaConfig;

/* raw ook code captured from garage door opener */
#define LEN 29
__xdata u8 buf[] = {
	0xaa, 0xaa, 0xaa, 0x00, 0x4d, 0x34, 0xd3,
	0x49, 0x36, 0xd2, 0x49, 0xb6, 0xda, 0x6d,
	0x34, 0xdb, 0x69, 0xb4, 0x92, 0x69, 0x36,
	0xda, 0x49, 0x24, 0x92, 0x6d, 0xb6, 0xdb, 0x68
};

void setup_dma_tx()
{
	//forum guy used high priority and repeated single mode (TMODE = 2)
	dmaConfig.PRIORITY       = 2;  // high priority
	dmaConfig.M8             = 0;  // not applicable
	dmaConfig.IRQMASK        = 0;  // disable interrupts
	dmaConfig.TRIG           = 19; // radio
	//dmaConfig.TMODE          = 0;  // single byte mode
	dmaConfig.TMODE          = 2;  // single byte mode
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
	xtalClock();
	setIOPorts();
	configureSPI();
	LCDReset();

	/* IF setting */
    FSCTRL1   = 0x06;
    FSCTRL0   = 0x00;

	/* 390 MHz */
	setFreq(FREQ, &FREQ2, &FREQ1, &FREQ0);
    CHANNR    = 0x00;

	/* maximum channel bandwidth, 5000 baud */
		setBaud(BAUDRATE, &MDMCFG4, &MDMCFG3);

	/* DC blocking enabled, OOK/ASK */
	MDMCFG2   = 0x30; // no preamble/sync

	/* no FEC, 4 byte preamble, default channel spacing */
    MDMCFG1   = 0x22;
    MDMCFG0   = 0xF8;

    FREND1    = 0x56;   // Front end RX configuration.
    FREND0    = 0x11;   // Front end RX configuration.

	/* automatic frequency calibration */
    MCSM0     = 0x14;
	//MCSM2 ?
	MCSM1     = 0x30; // TXOFF_MODE = IDLE

    FSCAL3    = 0xE9;   // Frequency synthesizer calibration.
    FSCAL2    = 0x2A;   // Frequency synthesizer calibration.
    FSCAL1    = 0x00;   // Frequency synthesizer calibration.
    FSCAL0    = 0x1F;   // Frequency synthesizer calibration.
    TEST2     = 0x88;   // Various test settings.
    TEST1     = 0x31;   // Various test settings.
    TEST0     = 0x0B;   // low VCO (we're in the lower 400 band)

	/* no preamble quality check, no address check */
    PKTCTRL1  = 0x04;

	/* no whitening, no CRC, fixed packet length */
    PKTCTRL0  = 0x00;

	/* device address */
    ADDR      = 0x11;

	/* packet length in bytes */
    PKTLEN    = LEN;

	//PA_TABLE0 = 0x12;
	PA_TABLE0 = 0x00;
	PA_TABLE1 = 0xC0;

	setup_dma_tx();

	while (1) {

		clear();
		SSN = LOW;
		setCursor(0, 0);
		printf("open sesame");
		SSN = HIGH;

		while (getkey() != ' ')
			sleepMillis(200);

		SSN = LOW;
		setCursor(3, 0);
		printf("open!");
		SSN = HIGH;

		EA = 1; // enable interrupts globally
		IEN2 |= IEN2_RFIE; // enable RF interrupt
		RFIM = RFIM_IM_DONE; // mask IRQ_DONE only
		DMAARM |= DMAARM0;  // Arm DMA channel 0
    	RFST = RFST_STX;;

		while (!txdone);

    	RFST = RFST_SIDLE;

		sleepMillis(500);
		txdone = 0;
	}
}

/* IRQ_DONE interrupt service routine */
void rf_isr() __interrupt (RF_VECTOR) {
	/* clear the interrupt flags */
	RFIF &= ~RFIF_IRQ_DONE;
	S1CON &= ~0x03;           // Clear the general RFIF interrupt registers

	txdone = 1;
}
