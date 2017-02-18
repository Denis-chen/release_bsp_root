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

#define	MX1_RXERR	(MX1_URXD_ERR | MX1_URXD_OVERRUN | MX1_URXD_FRMERR | MX1_URXD_BRK | MX1_URXD_PRERR)

static inline int ms_interrupt(DEV_MX1 *dev)
{
    int status = 0;
    uintptr_t       base = dev->base;
    int sr1;

    sr1 = in32(base + MX1_UART_SR1);
    out32(base + MX1_UART_SR1, MX1_USR1_RTSD);
    status |= tti(&dev->tty,
                (sr1 & MX1_USR1_RTSS) ? TTI_OHW_CONT : TTI_OHW_STOP);

    return (status);
}

static inline int tx_interrupt(DEV_MX1 *dev)
{
	int	status = 0;
	uintptr_t		base = dev->base;
	int cr1;

	cr1 = in32(base + MX1_UART_CR1);
	out32(base + MX1_UART_CR1, cr1 & ~MX1_UCR1_TRDYEN);		
	
	dev->tty.un.s.tx_tmr = 0;
	/* Send event to io-char, tto() will be processed at thread time */
	atomic_set(&dev->tty.flags, EVENT_TTO);
	status |= 1;
	
	return (status);
}

static inline int rx_interrupt(DEV_MX1 *dev)
{
	int			status = 0;
	unsigned	key, rxdata;
	uintptr_t	base = dev->base;

	while (in32(base + MX1_UART_SR2) & MX1_USR2_RDR) {

		/*
		 * Read next character from FIFO
		 */
		rxdata = in32(base + MX1_UART_RXDATA);
		key = rxdata & 0xFF;
		if (rxdata & MX1_RXERR) {
			/*
			 * Save error as out-of-band data which can be read via devctl()
			 */
			dev->tty.oband_data |= rxdata;
			atomic_set(&dev->tty.flags, OBAND_DATA);

			if (rxdata & MX1_URXD_BRK)
				key |= TTI_BREAK;
			else if (rxdata & MX1_URXD_FRMERR)
				key |= TTI_FRAME;
			else if (rxdata & MX1_URXD_PRERR)
				key |= TTI_PARITY;
			else if (rxdata & MX1_URXD_OVERRUN)
				key |= TTI_OVERRUN;
		}

		status |= tti(&dev->tty, key);
	}

	return status;
}

static inline int do_interrupt(DEV_MX1 *dev, int id)
{
	int sts;

	sts = rx_interrupt(dev);
	if (in32(dev->base + MX1_UART_SR1) & MX1_USR1_TRDY)
		sts |= tx_interrupt(dev);
    if ((in32(dev->base + MX1_UART_SR1) & MX1_USR1_RTSD) &&
            (dev->tty.c_cflag & OHFLOW))
        sts |= ms_interrupt(dev);

	return sts;
}

/*
 * Serial interrupt handler
 */
static const struct sigevent * ser_intr(void *area, int id)
{
	DEV_MX1	*dev = area;

	if (do_interrupt(dev,id) && (dev->tty.flags & EVENT_QUEUED) == 0) {
		dev_lock(&ttyctrl);
		ttyctrl.event_queue[ttyctrl.num_events++] = &dev->tty;
		atomic_set(&dev->tty.flags, EVENT_QUEUED);
		dev_unlock(&ttyctrl);
		return &ttyctrl.event;
	}

	return 0;
}

void
ser_attach_intr(DEV_MX1 *dev)
{
	dev->iid[0] = InterruptAttach(dev->intr[0], ser_intr, dev, 0, _NTO_INTR_FLAGS_TRK_MSK);
	if (dev->intr[1] != -1)
		dev->iid[1] = InterruptAttach(dev->intr[1], ser_intr, dev, 0, _NTO_INTR_FLAGS_TRK_MSK);
}


#if 0
void
ser_detach_intr(DEV_MX1 *dev)
{
	uintptr_t	base = dev->base;

	/* Disable UART */
	out32(base + MX1_UART_CR1, 0x04);
	out32(base + MX1_UART_CR2, 0x00);

	InterruptDetach(dev->iid[0]);
	dev->intr[0] = -1;
	if (dev->intr[1] != -1) {
		InterruptDetach(dev->iid[1]);
		dev->intr[1] = -1;
	}

}
#endif

