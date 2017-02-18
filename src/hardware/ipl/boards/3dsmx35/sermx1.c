/*
 * $QNXLicenseC: 
 * Copyright 2008, QNX Software Systems.  
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


#include "ipl.h"
#include <hw/inout.h>
#include "3dsmx35.h"

#define MX1_UFCR_RFDIV  MX1_UFCR_RFDIV_2
static unsigned char	mx1uart_pollkey();
static unsigned char	mx1uart_getchar();
static void				mx1uart_putchar(unsigned char);

static const ser_dev mx35_dev = {
	mx1uart_getchar,
	mx1uart_putchar,
	mx1uart_pollkey
};

unsigned char mx1uart_pollkey(void)
{
	if (MX1_UART_SR2 & MX1_USR2_RDR)
		return 1;
	else
		return 0;
}

unsigned char mx1uart_getchar(void)
{
	while (!(mx1uart_pollkey()));
    return ((unsigned char)MX1_UART_RXDATA);
}

void mx1uart_putchar(unsigned char data1)
{
	while(!(MX1_UART_SR1 & MX1_USR1_TRDY));
    MX1_UART_TXDATA = (unsigned short)data1;
}

void
init_ser3dsmx35(unsigned address, unsigned baud, unsigned clk)
{
    /* Wait for UART to finish transmitting */
    while (!(MX1_UART_TS & MX1_UTS_TXEMPTY));
	/* Disable UART */
	MX1_UART_CR1   &= (unsigned short) (~MX1_UCR1_UARTEN);
	/* Set to default POR state */
    MX1_UART_CR1    = (unsigned short) 0x00000000;
    MX1_UART_CR2    = (unsigned short) 0x00000000;
    while (!(MX1_UART_CR2 & MX1_UCR2_SRST));
    MX1_UART_CR3    = (unsigned short) 0x0704;
	MX1_UART_CR4    = (unsigned short) 0x8000;
    MX1_UART_FCR    = (unsigned short) 0x0801;
    MX1_UART_ESC    = (unsigned short) 0x002B;
    MX1_UART_TIM    = (unsigned short) 0x0000;
    MX1_UART_BIR    = (unsigned short) 0x0000;
    MX1_UART_BMR    = (unsigned short) 0x0000;
    MX1_UART_ONEMS  = 0x0000;
    MX1_UART_TS     = (unsigned short) 0x0000;
     /* Configure FIFOs */
	MX1_UART_FCR    = (unsigned short) ((1 << MX1_UFCR_RXTL_SHIFT) | 
	                  MX1_UFCR_RFDIV | 
	                  (2 << MX1_UFCR_TXTL_SHIFT));
	MX1_UART_ONEMS  = 0xc350; //(UART ref. freq. / 1000) where UART ref. freq. = 50 MHZ
	/* Set to 8N1 */
    MX1_UART_CR2   &= (unsigned short) ~MX1_UCR2_PREN;
    MX1_UART_CR2   |= (unsigned short) MX1_UCR2_WS;
    MX1_UART_CR2   &= (unsigned short) ~MX1_UCR2_STPB;
    /* Ignore RTS */
    MX1_UART_CR2   |= (unsigned short) MX1_UCR2_IRTS;
     /* Enable UART */
	MX1_UART_CR1   |= (unsigned short) MX1_UCR1_UARTEN;
	/* Enable FIFOs */
    MX1_UART_CR2   |= (unsigned short) (MX1_UCR2_SRST | 
                      MX1_UCR2_RXEN | MX1_UCR2_TXEN);
    /* Clear status flags */
    MX1_UART_SR2   |= (unsigned short) (MX1_USR2_ADET  |
	                  MX1_USR2_IDLE  |
	                  MX1_USR2_IRINT |
	                  MX1_USR2_WAKE  |
	                  MX1_USR2_RTSF  |
	                  MX1_USR2_BRCD  |
	                  MX1_USR2_ORE   |
	                  MX1_USR2_RDR);

    /* Clear status flags */
   MX1_UART_SR1    |= (unsigned short) (MX1_USR1_PARITYERR |
	                  MX1_USR1_RTSD      |
	                  MX1_USR1_ESCF      |
	                  MX1_USR1_FRAMERR   |
	                  MX1_USR1_AIRINT    |
	                  MX1_USR1_AWAKE);
	MX1_UART_BIR    = (unsigned short) 0x047F; //(baudrate/100) - 1 where baudrate = 115200
	MX1_UART_BMR    = (unsigned short) 0x7A11; //(UART ref. freq / 1600) - 1 where UART ref. freq = 50 MHZ 
	/*
	 * Register our debug functions
	 */
	init_serdev((ser_dev *)&mx35_dev);
}
 
