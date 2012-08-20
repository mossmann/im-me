#!/usr/bin/python

# decodes 66 bit remote keyless entry signal
# pulse width modulation
# to be used with grc/usrp demodulator

import struct
	
data = open('/tmp/garage.out').read()
symbols = struct.unpack('B1'*len(data), data)

# one bit is encoded in a triple (three adjacent symbols)
def pwm_decode(triple):
	# strip any extra bits added by correlator
	stripped = (triple[0]&1, triple[1]&1, triple[2]&1)

	# short pulse
	if stripped == (1,0,0):
		return 0
	# long pulse
	if stripped == (1,1,0):
		return 1
	else:
		print "pwm decoding error"
		raise

# extract an integer value from bitstream
def extract(start, len, bits):
	val = 0
	for i in range(start, start + len):
		val <<= 1
		val += bits[i]
	return val

# decode 66 bit garage door opener code
def decode_frame(start):
	print
	print "decoding frame"
	bits = []
	for i in range(66):
		j = (i * 3) + start
		try:
			bit = pwm_decode(symbols[j:j+3])
		except:
			return
		bits.append(bit)

	print bits
	print "pwm hex: %017x" % extract(0, 66, bits)
	# preamble
	frame = 0xaaaaaa00
	frame <<= 1
	for sym in symbols[start:start+198]:
		frame <<= 1
		frame |= (sym & 1)
	frame <<= 1
	print "raw ook hex: %058x" % frame

# look for correlations flagged by gr_correlate_access_code_bb
for i in range(len(symbols) - 198):
	if symbols[i] & 2:
		decode_frame(i)
