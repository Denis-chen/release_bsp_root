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

int
smc9118_receive(smc9118_t *dev)
{
	npkt_t			*npkt;
	uintptr_t		iobase = dev->iobase;
	uint32_t		status;
	int				pktlen;
	net_iov_t		*iov;
	net_buf_t		*buf;
	npkt_t 			*rx_Q_head, *rx_Q_tail;
	int				buffered = 0, numreads;
	uint32_t		*ptr;

again:
	rx_Q_head = rx_Q_tail = NULL;

more:
	while (INLE32(iobase + SMC9118_INT_STS) & RSFL) {
		OUTLE32(iobase + SMC9118_INT_STS, RSFL);

		if ((INLE32(iobase + SMC9118_RX_FIFO_INF) & 0x00FF0000) == 0)
			break;
		
		INLE32(iobase + SMC9118_BYTE_TEST);
		INLE32(iobase + SMC9118_BYTE_TEST);
		INLE32(iobase + SMC9118_BYTE_TEST);

		status = INLE32(iobase + RX_FIFO_STS_PORT);

		/* grab a packet from the queue */
		if (dev->num_rx_free > 0) {
			pthread_mutex_lock(&dev->rx_freeq_mutex);
			npkt = dev->rx_free;
			dev->rx_free = npkt->next;
			npkt->next = NULL;
			dev->num_rx_free--;
			pthread_mutex_unlock(&dev->rx_freeq_mutex);
		} else {
			if ((npkt = smc9118_alloc_npkt(dev, SMC9118_MTU_SIZE)) == NULL) {
				/* no memory, drop the packet */
				dev->stats.rx_failed_allocs++;
				continue;
			}
		}

		pktlen = (status >> RX_STS_PKT_LEN) & 0x3FFF;

		buf = TAILQ_FIRST(&npkt->buffers);
		iov = buf->net_iov;

		ptr = iov->iov_base;
		numreads = (pktlen + 3) >> 2;

		npkt->framelen = iov->iov_len = pktlen - 4;
		while (numreads-- > 0)
			*ptr++ = INLE32(iobase + RX_FIFO_DATA_PORT);

		dev->stats.octets_rxed_ok += npkt->framelen;

		npkt->next = NULL;
		if (rx_Q_tail)
			rx_Q_tail->next = npkt;
		else
			rx_Q_head = npkt;
		rx_Q_tail = npkt;

		dev->stats.rxed_ok++;
		buffered++;
		dev->did_rx = 1;

		if (status & RX_STS_LEN_ERR)
			dev->stats.un.estats.length_field_mismatch++;
		if (status & RX_STS_CRC_ERR)
			dev->stats.un.estats.fcs_errors++;
		if (status & RX_STS_FRAME_TOO_LONG)
			dev->stats.un.estats.oversized_packets++;
		if (status & RX_STS_RUNT_FRAME)
			dev->stats.un.estats.short_packets++;
		if (status & RX_STS_DRIBB_BIT)
			dev->stats.un.estats.dribble_bits++;
		if (status & RX_STS_COLL_SEEN)
			dev->stats.un.estats.single_collisions++;
		if (status & (RX_STS_WDG_TIMEOUT | RX_STS_MII_ERR))
			dev->stats.un.estats.internal_rx_errors++;

		if (status & RX_STS_MCAST_FRAME)
			dev->stats.rxed_multicast++;
		if (status & RX_STS_BCAST_FRAME)
			dev->stats.rxed_broadcast++;

		if (buffered == 32) {
			/*
			 * Send some upstream, we can't buffer
			 * indefinitely or we'll use up all system RAM
			 */
			break;
		}
	}

	while (rx_Q_head) {
		npkt = rx_Q_head;
		rx_Q_head = npkt->next;
		npkt->next = NULL;
		if (dev->cfg.flags & NIC_FLAG_PROMISCUOUS)
			npkt->flags |= _NPKT_PROMISC;

		buffered--;

		pthread_mutex_unlock(&dev->mutex);
		if (npkt = dev->ion->tx_up_start(dev->reg_hdl,
		    npkt, 0, 0, dev->cell, dev->cfg.lan, 0, dev)) {
			smc9118_receive_complete(npkt, dev, dev);
		}
		pthread_mutex_lock(&dev->mutex);

		if (INLE32(iobase + SMC9118_INT_STS) & RSFL) {
			if (rx_Q_head == NULL)
				rx_Q_tail = NULL;
			goto more;
		}
	}

	if (INLE32(iobase + SMC9118_INT_STS) & RSFL)
		goto again;

	return 0;
}

int
smc9118_receive_complete(npkt_t *npkt, void *hdl, void *func_hdl)
{
	smc9118_t	*dev = hdl;

	if (dev->num_rx_free < dev->max_rx_buffers) {
		net_buf_t	*buf = TAILQ_FIRST(&npkt->buffers);
		net_iov_t	*iov = buf->net_iov;

		npkt->ref_cnt = 1;
		npkt->req_complete = 0;
		npkt->flags = _NPKT_UP;
		npkt->tot_iov = 1;

		buf->niov     = 1;

		iov->iov_base = (void *)npkt->org_data;
		iov->iov_phys = dev->ion->mphys(iov->iov_base);
		iov->iov_len  = SMC9118_MTU_SIZE;

		pthread_mutex_lock(&dev->rx_freeq_mutex);
		npkt->next = dev->rx_free;
		dev->rx_free = npkt;
		dev->num_rx_free++;
		pthread_mutex_unlock(&dev->rx_freeq_mutex);
	} else {
		dev->ion->free(npkt->org_data);
		dev->ion->free(npkt);
	}

	return 0;
}
