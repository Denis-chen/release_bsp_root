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




#include "smc.h"

#define SMC9118_IS_BROADCAST(p) \
			((p)[0] == 0xff && (p)[1] == 0xff && \
			 (p)[2] == 0xff && (p)[3] == 0xff && \
			 (p)[4] == 0xff && (p)[5] == 0xff)

/* Take packets of the software queue and queue them to the tx hardware */
void
smc9118_send(smc9118_t *dev)
{
	int			numwrites, i;
	int			iov_cnt;
	uintptr_t	iobase = dev->iobase;
	npkt_t		*npkt;
	uint16_t	fifo_free;
	net_buf_t	*buf;
	net_iov_t	*iov;
	uint32_t	tx_cmda, b_offset;
	uint32_t	*pbuf;
	uint8_t		*p;

	while (dev->nhead) {
		npkt = dev->nhead;

		fifo_free = INLE32(iobase + SMC9118_TX_FIFO_INF) & TDFREE;
		if (fifo_free < (npkt->framelen + (npkt->tot_iov << 4)))
			break;

		dev->nhead = dev->nhead->next;
		npkt->next = NULL;

		if (npkt == dev->ntail)
			dev->ntail = NULL;

		dev->stats.octets_txed_ok += npkt->framelen;

		for (iov_cnt = 0, buf = TAILQ_FIRST(&npkt->buffers); buf; buf = TAILQ_NEXT(buf, ptrs)) {
			for (i = 0, iov = buf->net_iov; i < buf->niov; i++, iov++) {
				p = (uint8_t *)iov->iov_base;

				tx_cmda = iov->iov_len;
				if (iov_cnt == 0) {
					if (p[0] & 1) {
						if (SMC9118_IS_BROADCAST(p))
							dev->stats.txed_broadcast++;
						else
							dev->stats.txed_multicast++;
					}
					tx_cmda |= TX_CMDA_FIRST_SEG;
				}

				if (iov_cnt == npkt->tot_iov - 1)
					tx_cmda |= TX_CMDA_IOC | TX_CMDA_LAST_SEG;

				numwrites = iov->iov_len;

				/* Handle un-aligned buffer */
				b_offset = ((uint32_t)p) & 3;

				/* Send TX Commands to FIFO */
				OUTLE32(iobase + TX_FIFO_DATA_PORT, tx_cmda | (b_offset << 16));
				OUTLE32(iobase + TX_FIFO_DATA_PORT, npkt->framelen);

				/* Write unaligned data to FIFO */
				if (b_offset) {
					uint32_t	b_cnt = 4 - b_offset;

					numwrites -= b_cnt;

					while (b_cnt--)
						b_offset = (b_offset >> 8) | (*p++ << 24);

					OUTLE32(iobase + TX_FIFO_DATA_PORT, b_offset);
				}

				numwrites = (numwrites + 3) >> 2;
				pbuf = (uint32_t *)p;

				/* Copy TX data to FIFO */
				while (numwrites-- > 0)
					OUTLE32(iobase + TX_FIFO_DATA_PORT, *pbuf++);

				iov_cnt++;
			}
		}

		pthread_mutex_unlock(&dev->mutex);
		dev->ion->tx_done(dev->reg_hdl, npkt);
		pthread_mutex_lock(&dev->mutex);
	}
}

/*
 * smc9118_send_packets
 *
 * Description: Entry point from io-net layer by protocol to send
 *              packets out the NIC
 */
int
smc9118_send_packets(npkt_t *npkt, void *hdl)
{
	smc9118_t	*dev = hdl;

	if (npkt->flags & _NPKT_MSG) {	// control packet
		// just pass the packet back up
		dev->ion->tx_done(dev->reg_hdl, npkt);
	} else {  // data packet
		pthread_mutex_lock(&dev->mutex);

		if (dev->ntail)
			dev->ntail->next = npkt;
		else
			dev->nhead = npkt;

		while (npkt->next)
			npkt = npkt->next;
		dev->ntail = npkt;

		smc9118_send(dev);
		pthread_mutex_unlock(&dev->mutex);
	}

	return TX_DOWN_OK;
}

__SRCVERSION("transmit.c $Rev: 262087 $");
