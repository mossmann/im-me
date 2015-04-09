// helper functions for cc111x
// -samy kamkar

#include <math.h>

#ifdef CC1110
#define MHZ 26
#elif CC1111
#define MHZ 48
#else
#error Please define CC1110 or CC1111 so clock can be determined.
#endif

// set FREQ registers in cc111x from a float
void setFreq(float freq, char *FREQ2, char *FREQ1, char *FREQ0)
{
	float freqMult = (0x10000 / 1000000.0) / MHZ;
	u32 num = freq * freqMult;

	*FREQ2 = num >> 16;
	*FREQ1 = (num >> 8) & 0xFF;
	*FREQ0 = num & 0xFF;
}

// set baudrate registers from a float
void setBaud(float drate, char *MDMCFG4, char *MDMCFG3)
{
	u8 drate_e = 0;
	u8 drate_m = 0;
	u8 e;
	float m = 0;

	for (e = 0; e < 16; e++)
	{

		m = (drate * powf(2,28) / (powf(2,e)* (MHZ*1000000.0))-256) + .5;
		if (m < 256)
		{
			drate_e = e;
			drate_m = m;
			break;
		}
	}

	drate = 1000000.0 * MHZ * (256+drate_m) * powf(2,drate_e) / powf(2,28);

	*MDMCFG3 = drate_m;
#ifndef MDMCFG4_DRATE_E
#define MDMCFG4_DRATE_E 0x0F
#endif
  *MDMCFG4 &= ~MDMCFG4_DRATE_E;
  *MDMCFG4 |= drate_e;
}


