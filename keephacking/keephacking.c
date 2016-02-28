#define CC1110

#include <cc1110.h>
#include "ioCCxx10_bitdef.h"
#include "display.h"
#include "keys.h"
#include "stdio.h"
#include "helpers.h"

int main(void)
{
	int i;
	xtalClock();
	setIOPorts();
	configureSPI();
	LCDReset();

	clear();
	SSN = LOW;
	for (i = 0; i < 8; i++) {
		setCursor(i, i*8);
		printf("KEEP HACKING");
	}
	SSN = HIGH;

	while (1);
}
