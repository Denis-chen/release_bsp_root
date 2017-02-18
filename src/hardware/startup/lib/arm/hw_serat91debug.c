/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems. All Rights Reserved.
 * 
 * You must obtain a written license from and pay applicable license fees to QNX 
 * Software Systems before you may reproduce, modify or distribute this software, 
 * or any work that includes all or part of this software.   Free development 
 * licenses are available for evaluation and non-commercial purposes.  For more 
 * information visit http://licensing.qnx.com or email licensing@qnx.com.
 *  
 * This file may contain contributions from others.  Please review this entire 
 * file for other proprietary rights or license notices, as well as the QNX 
 * Development Suite License Guide at http://licensing.qnx.com/license-guide/ 
 * for other information.
 * $
 */



#include "startup.h"
#include <arm/at91sam9xx.h>

static void
parse_line(unsigned channel, const char *line, unsigned *baud, unsigned *clk)
{
	/*
	 * Get device base address and register stride
	 */
	if (*line != '.' && *line != '\0') {
		dbg_device[channel].base = strtoul(line, (char **)&line, 16);
		if (*line == '^')
			dbg_device[channel].shift = strtoul(line+1, (char **)&line, 0);
	}

	/*
	 * Get baud rate
	 */
	if (*line == '.')
		++line;
	if (*line != '.' && *line != '\0')
		*baud = strtoul(line, (char **)&line, 0);

	/*
	 * Get clock rate
	 */
	if (*line == '.')
		++line;
	if (*line != '.' && *line != '\0')
		*clk = strtoul(line, (char **)&line, 0);
}

/*
 * Initialise one of the serial ports
 */
void
init_at91sam9xx(unsigned channel, const char *init, const char *defaults)
{
	unsigned	baud, clk, base;
	
	parse_line(channel, defaults, &baud, &clk);
	parse_line(channel, init, &baud, &clk);
	base = dbg_device[channel].base;

	/* Disable all interrupts */
	out32(base + AT91_DBGU_IDR, 0x000F3FFF);

	if (baud != 0) {
		/* Reset Rx, Tx and Status bits */
		out32(base + AT91_DBGU_CR, AT91_DBGU_CR_RSTRX | AT91_DBGU_CR_RSTTX | AT91_DBGU_CR_RSTSTA);

		/* 8-bit, no-parity, 1 stop bit */
		out32(base + AT91_DBGU_MR, (3 << 6) | (4 << 9));

		/* Program BRG Register */
		out32(base + AT91_DBGU_BRGR, (clk >> 4) / baud);
	}

	/* Enable Tx/Rx */
	out32(base + AT91_DBGU_CR, AT91_DBGU_CR_RXEN | AT91_DBGU_CR_TXEN);
}

/*
 * Send a character
 */
void
put_at91sam9xx(int data)
{
	unsigned base = dbg_device[0].base;

	while (!(in32(base + AT91_DBGU_SR) & AT91_DBGU_IE_SR_TXRDY)) {
	};

	out32(base + AT91_DBGU_THR, (unsigned)data);
}
