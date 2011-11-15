Pocket Spectrum Analyzer

This is alternative firmware for the Girl Tech IM-Me
(http://www.girltech.com/electronics-imMe.aspx) that implements a basic
spectrum analyzer.

WARNING!  You could destroy your device if you try to use this software.  You
certainly would have a hard time getting your device to work as originally
intended ever again.  There is no known way to recover the original firmware.


installation:

For more complete instructions, see:

http://travisgoodspeed.blogspot.com/2010/03/im-me-goodfet-wiring-tutorial.html

Abbreviated instructions follow:

1. Acquire or build a GoodFET (http://goodfet.sourceforge.net/).  There are
other ways to program the IM-Me, but you're on your own.

2. Wire your IM-Me to the GoodFET according to
http://www.flickr.com/photos/travisgoodspeed/4322361457/

3. goodfet.cc flash specan.hex

Alternatively you may compile from source with sdcc and install with "make
install".


usage:

The LCD shows the relative power levels received across a range of frequencies
indicated in MHz along the bottom.

bandwidth settings:

There are three bandwidth modes: wide (default), narrow, and ultrawide.  Wide
mode displays 26.4 MHz of bandwidth in 200 kHz increments.  Narrow mode
displays 6.6 MHz of bandwidth in 50 kHz increments.  Ultrawide mode displays 88
MHz of bandwidth in 667 kHz increments.  You can cycle through the modes with
the "Menu" button or select a mode with the "W", "N", or "U" buttons.

frequency selection:

Set the frequency by scrolling with the left or right arrow buttons.  The
supported ranges are 281 - 361, 378 - 481, and 749 - 962 MHz.

power level resolution:

There are two power level resolution modes: short (default) and tall.  Short
mode displays six bits of power level information.  Tall mode displays eight
bits of power level information.  You can toggle between the two modes with the
"Bye!" button or select a mode with the "S" or "T" buttons.

power level scrolling:

Scroll up and down with the smile wheel or the "Q" and "A" buttons.  Scrolling
is often necessary when using tall mode.  Tip: You can scroll faster in short
mode and then switch back to tall mode.

max hold:

Press the "M" button to active or deactivate max hold.  The maximum power level
received during the current activation of max hold is displayed as a dot above
the normal bar graph display.  Max hold is automatically deactivated whenever
the frequency or bandwidth settings change.


thanks:

Thanks to Dave for an incredible job of reverse engineering the IM-Me and
sharing his code:

http://daveshacks.blogspot.com/

Thanks to Travis Goodspeed for developing the GoodFET, introducing me to the
IM-Me, and sharing his code:

http://travisgoodspeed.blogspot.com/


home:

You can find the most recent version of this software at:

http://www.ossmann.com/sa/


author:

Michael Ossmann <mike@ossmann.com>
