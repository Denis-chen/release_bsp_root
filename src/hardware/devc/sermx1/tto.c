/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, QNX Software Systems.  
 *  
 * Licensed under the Apache License, Version 2.0 (the "License"). You  
 * may not reproduce, modify or distribute this software except in  
 * compliance with the License. You may obtain a copy of the License  
 * at: http://www.apache.org/licenses/LICENSE-2.0  
 *  
 * Unless required by applicable law or agreed to in writing, software  
 * distributed under the License is distributed on an "AS IS" basis,  
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied. 
 * 
 * This file may contain contributions from others, either as  
 * contributors under the License or as licensors under other terms.   
 * Please review this entire file for other proprietary rights or license  
 * notices, as well as the QNX Development Suite License Guide at  
 * http://licensing.qnx.com/license-guide/ for other information. 
 * $ 
 */






#include "externs.h"

int
tto(TTYDEV *ttydev, int action, int arg1)
{
	TTYBUF 			*bup = &ttydev->obuf;
	DEV_MX1			*dev = (DEV_MX1 *)ttydev;
	uintptr_t		base = dev->base;
	unsigned char 	c;
	unsigned		cr1;

	switch (action) {
		case TTO_STTY:
			ser_stty(dev);
			return 0;
 
		case TTO_CTRL:
			if (arg1 & _SERCTL_BRK_CHG) {
				cr1 = in32(base + MX1_UART_CR1);

				if (arg1 &_SERCTL_BRK) 
					cr1 |= MX1_UCR1_SNDBRK;
				else 
					cr1 &= ~MX1_UCR1_SNDBRK;

				out32(base + MX1_UART_CR1, cr1);
			}

			/*
			 * Modem ctrl
			 */
			if (arg1 & _SERCTL_DTR_CHG) {
				cr1 = in32(base + MX1_UART_CR3);

				if (arg1 & _SERCTL_DTR)
					cr1 |= MX1_UCR3_DSR;
				else
					cr1 &= ~MX1_UCR3_DSR;

				out32(base + MX1_UART_CR3, cr1);
			}

			/*
			 * RTS Control
			 */
			if (arg1 & _SERCTL_RTS_CHG) {
				cr1 = in32(base + MX1_UART_CR2);

				if (arg1 & _SERCTL_RTS)
					cr1 |= MX1_UCR2_CTS;
				else
					cr1 &= ~MX1_UCR2_CTS;

				out32(base + MX1_UART_CR2, cr1);
			}
			return 0;

		case TTO_LINESTATUS:
			return ((in32(base + MX1_UART_SR1) & 0xFFFF) | (in32(base + MX1_UART_SR2)) << 16);

		case TTO_DATA:
		case TTO_EVENT:
			break;

		default:
			return 0;
	}

	while (bup->cnt > 0 && (in32(base + MX1_UART_SR1) & MX1_USR1_TRDY))
	{
		/*
		 * If the OSW_PAGED_OVERRIDE flag is set then allow
		 * transmit of character even if output is suspended via
		 * the OSW_PAGED flag. This flag implies that the next
	 	 * character in the obuf is a software flow control
		 * charater (STOP/START).
		 * Note: tx_inject sets it up so that the contol
		 *       character is at the start (tail) of the buffer.
		 */
		if (dev->tty.flags & (OHW_PAGED|OSW_PAGED) && !(dev->tty.xflags & OSW_PAGED_OVERRIDE))
			break;
		
		/*
		 * Get the next character to print from the output buffer
		 */
		dev_lock(&dev->tty);
		c=tto_getchar(&dev->tty);
		dev_unlock(&dev->tty);
	    	
		dev->tty.un.s.tx_tmr = 3;		/* Timeout 3 */
		out32(base + MX1_UART_TXDATA, c);
			
		/* Clear the OSW_PAGED_OVERRIDE flag as we only want
		 * one character to be transmitted in this case.
		 */
		if (dev->tty.xflags & OSW_PAGED_OVERRIDE)
		{
			atomic_clr(&dev->tty.xflags, OSW_PAGED_OVERRIDE);         
			break;
		}
	}
	
	if (!(dev->tty.flags & (OHW_PAGED|OSW_PAGED)) && bup->cnt) {
		cr1 = in32(base + MX1_UART_CR1);
		out32(base + MX1_UART_CR1, cr1 | MX1_UCR1_TRDYEN);		
	}
	
	return (tto_checkclients(&dev->tty));
}

void ser_stty(DEV_MX1 *dev)
{
	uintptr_t	base = dev->base;
	unsigned	tmp, cr2, clk, rfdiv, fcr, bir;

	/*
	 * Check hardware flow control setting
	 */
	cr2 = tmp = in32(base + MX1_UART_CR2);
	if (dev->tty.c_cflag & (IHFLOW|OHFLOW)) {
		tmp &= ~(MX1_UCR2_IRTS | MX1_UCR2_CTSC);
	}
	else {
		tmp |= (MX1_UCR2_IRTS | MX1_UCR2_CTS);
		tmp &= ~MX1_UCR2_CTSC;
	}
	if (cr2 != tmp)
		out32(base + MX1_UART_CR2, tmp | MX1_UCR2_SRST);
	dev->cr2 = cr2 = tmp;

	/*
	 * Calculate baud rate divisor, data size, stop bits and parity
	 */
	clk = dev->clk;
	rfdiv = clk / 16000000;
	if (rfdiv > 6) {
		rfdiv = 6;
		clk /= 7;
	} else if (rfdiv > 0) {
		clk /= rfdiv;
		rfdiv = 6 - rfdiv;
	} else
		rfdiv = 5;

	fcr = (dev->fifo & 0xFC3F) | (rfdiv << 7);
	bir = dev->tty.baud * 16 / (clk / 10000) - 1;

	switch (dev->tty.c_cflag & CSIZE) {
		case CS8:
			cr2 |= MX1_UCR2_WS;
			break;

		case CS7:
			cr2 &= ~MX1_UCR2_WS;
			break;
	}

	if (dev->tty.c_cflag & CSTOPB)
		cr2 |= MX1_UCR2_STPB;

	cr2 &= ~(MX1_UCR2_PREN | MX1_UCR2_PROE);
	if (dev->tty.c_cflag & PARENB) {
		cr2 |= MX1_UCR2_PREN;
		if (dev->tty.c_cflag & PARODD)
			cr2 |= MX1_UCR2_PROE;
	}

	if ((dev->fcr == fcr) && (dev->cr2 == cr2) && (dev->bir == bir))
		return;
	
	dev->fcr = fcr;
	dev->cr2 = cr2;
	dev->bir = bir;

	/*
	 * Wait for Tx FIFO and shift register empty if the UART is enabled
	 */
	if ((in32(base + MX1_UART_CR1) & (MX1_UCR1_UARTCLKEN|MX1_UCR1_UARTEN)) == (MX1_UCR1_UARTCLKEN|MX1_UCR1_UARTEN)) {
		if (in32(base + MX1_UART_CR2) & MX1_UCR2_TXEN) {
			while (!(in32(base + MX1_UART_SR2) & MX1_USR2_TXDC))
				;
		}
	}

	/* Disable UART */
	out32(base + MX1_UART_CR1, MX1_UCR1_UARTCLKEN);

	/* Reset UART */
	out32(base + MX1_UART_CR2, 0);

	for (tmp = 0; tmp < 10; tmp++)
		;

	out32(base + MX1_UART_CR2, cr2 | MX1_UCR2_SRST);

	if (dev->mx1) {
		out32(base + MX1_UART_CR3, 0);
		/* Reference clock is 16MHz */
		out32(base + MX1_UART_CR4, 0x8000 | MX1_UCR4_REF16 | MX1_UCR4_DREN);
	}
	else {
		/* Program RXD muxed input */
		out32(base + MX1_UART_CR3, 4);
		out32(base + MX1_UART_CR4, 0x8000 | MX1_UCR4_DREN);
	}

	out32(base + MX1_UART_FCR, fcr);

	out32(base + MX1_UART_BIR, bir);
	out32(base + MX1_UART_BMR, 9999);

    /* program ONEMS register for MX21/MX31/MX35 */
    if (!dev->mx1)
        out32(base + MX1_UART_BIPR1, clk / 1000);

	/* Enable UART and Receiver Ready Interrupt */
	out32(base + MX1_UART_CR1, MX1_UCR1_UARTCLKEN | MX1_UCR1_UARTEN | MX1_UCR1_RRDYEN);

    /* If flow control is enabled then enable the RTS Delta interrupt */
    if (dev->tty.c_cflag & (IHFLOW|OHFLOW))
        out32(base + MX1_UART_CR1, in32(base + MX1_UART_CR1) | MX1_UCR1_RTSDEN);

	/* Enable Tx/Rx */
	out32(base + MX1_UART_CR2, cr2 | MX1_UCR2_TXEN | MX1_UCR2_RXEN | MX1_UCR2_SRST);
}


