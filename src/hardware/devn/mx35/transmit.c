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


#include "mx35.h"

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

static npkt_t  *mx35_fec_defrag (mx35_t *ext, npkt_t *npkt)

{
npkt_t			*dpkt;
char			*dst;
net_iov_t		*iov;
net_buf_t		*dov, *buf;
int				totalLen;
int				i, len;
	
	/* Setup our default return value */
	dpkt = NULL;
	
	/* Do some checks */
	buf = TAILQ_FIRST (&npkt->buffers);
	totalLen = 0;
	while (buf != NULL) {
		for (i = 0, iov = buf->net_iov; i < buf->niov; i++, iov++) {
			totalLen += iov->iov_len;
			}
		buf = TAILQ_NEXT (buf, ptrs);
		}
	
	if(totalLen <= MX35_MTU_SIZE) {
		if((dpkt = mx35_fec_alloc_npkt (ext, npkt->framelen)) != NULL) {
			dpkt->framelen  = npkt->framelen;
			dpkt->csum_flags = npkt->csum_flags;
			dpkt->flags = MX35_DEFRAG_PACKET;
			dpkt->next = NULL;

			dov = TAILQ_FIRST (&dpkt->buffers);
			dst = dov->net_iov->iov_base;

			len = 0;
			buf = TAILQ_FIRST (&npkt->buffers);
			while (buf != NULL) {
				for (i = 0, iov = buf->net_iov; i < buf->niov; i++, iov++) {
					memcpy (dst, iov->iov_base, iov->iov_len);
					dst += iov->iov_len;
					len += iov->iov_len;
					}

				buf = TAILQ_NEXT (buf, ptrs);
				}

			dov->net_iov->iov_len = len;
			}
		}

	npkt->next = NULL;
	ion_tx_complete (ext->reg_hdl, npkt);
	return (dpkt);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int	mx35_fec_reap_pkts (mx35_t *ext, int max_reap)

{
int						idx;
npkt_t					*npkt;
mx35_fec_bd_t				*bd;
int						reaped = 0;
nic_ethernet_stats_t    *estats;
	
	if ((ext->tx_pkts_queued == 0) && (ext->tx_pidx == ext->tx_cidx)) {
		return (0);
		}
	estats = &ext->stats.un.estats;

	do {
		uint32_t status;
		
		idx = ext->tx_cidx;
		bd = &ext->tx_bd [idx];
		status = ext->tx_status;
		if (bd->status & TXBD_R)
			break;
		if (idx == ext->num_tx_descriptors - 1)
			bd->status = TXBD_W;
		else
			bd->status = 0;
		if (! (status & (IEVENT_LATE_COL | IEVENT_COL_RET_LIM | IEVENT_XFIFO_UN))) {
			if (bd->status & TXBD_L) {
				ext->stats.txed_ok++;
				}
			}
		else {
			if (status & IEVENT_LATE_COL)
				estats->single_collisions++;
			if (status & IEVENT_COL_RET_LIM)
				estats->xcoll_aborted++;
			if (status & IEVENT_XFIFO_UN)
				estats->internal_tx_errors++;
			}
		
		npkt = ext->tx_pktq [idx];
		if (npkt) {
			npkt->next = NULL;
			if (npkt->flags & MX35_DEFRAG_PACKET) {
				ion_free (npkt->org_data);
				ion_free (npkt);
				}
			else {
				ion_tx_complete (ext->reg_hdl, npkt);
				}
			ext->tx_pkts_queued--;
			}

		ext->tx_pktq [idx] = NULL;
		ext->tx_q_len--;
		ext->tx_cidx = NEXT_TX (ext->tx_cidx);
		if (max_reap && ++reaped == max_reap) {
			break;
			}
		} while (ext->tx_cidx != ext->tx_pidx);

	return (0);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

/* 
 * mx35_fec_send
 *
 * Initiates transmission of a packet to the NIC.  
 *
 */

int  mx35_fec_send (mx35_t *ext, npkt_t *npkt)

{
int				idx;
npkt_t			*npkt2;
net_buf_t		*buf;
net_iov_t		*iov;
mx35_fec_bd_t		*tx_bd, *tx_bd_start;
int				i;
uint16_t 		status;
volatile	uint32_t	*base = ext->reg;
	
	if (npkt->tot_iov > DEFRAG_LIMIT) {
		if ((npkt2 = mx35_fec_defrag (ext, npkt)) == NULL) {
			/* If that fails, probably not worth sticking around */
			return (-1);
			}
		npkt = npkt2;
		}
	else {
		npkt->flags &= ~MX35_DEFRAG_PACKET;
		}

	_mutex_lock (&ext->tx_mutex);
	if (ext->tx_q_len >= DEFRAG_LIMIT) {
		mx35_fec_reap_pkts (ext, DEFRAG_LIMIT);
		}
	
	if (npkt->tot_iov + 1 > ext->num_tx_descriptors - ext->tx_q_len) {
		npkt->flags |= _NPKT_NOT_TXED;
		errno = ENOBUFS;
		if (npkt->flags & MX35_DEFRAG_PACKET) {
			ion_free (npkt->org_data);
			ion_free (npkt);
			}
		else {
			ion_tx_complete (ext->reg_hdl, npkt);
			}
		_mutex_unlock(&ext->tx_mutex);
		return (TX_DOWN_FAILED);
		}


	idx = ext->tx_pidx;
	tx_bd_start = tx_bd = NULL;
	status = (TXBD_R | TXBD_TC);
	for (buf = TAILQ_FIRST (&npkt->buffers); buf; buf = TAILQ_NEXT (buf, ptrs)) {
		for (i = 0, iov = buf->net_iov; i < buf->niov; i++, iov++) {
			tx_bd = &ext->tx_bd [idx];
			CACHE_FLUSH(&ext->cachectl, iov->iov_base, iov->iov_phys, iov->iov_len);
			tx_bd->buffer = iov->iov_phys;
			tx_bd->length = iov->iov_len;
			if (tx_bd->status & TXBD_R) {
				npkt->flags |= _NPKT_NOT_TXED;
				errno = ENOBUFS;
				if (npkt->flags & MX35_DEFRAG_PACKET) {
					ion_free (npkt->org_data);
					ion_free(npkt);
					}
				else {
					ion_tx_complete (ext->reg_hdl, npkt);
					}
				_mutex_unlock(&ext->tx_mutex);
				return (TX_DOWN_FAILED);
				}
			if (!tx_bd_start) {
				tx_bd_start = tx_bd;
				}
			else {
				tx_bd->status |= status;
				}
			
			idx = NEXT_TX (idx);
			ext->tx_q_len++;
			ext->stats.octets_txed_ok += iov->iov_len;
			}
		}
	
	ext->tx_pktq [ext->tx_pidx] = npkt;
	ext->tx_pkts_queued++;
	ext->tx_pidx = idx;

	/*
	 * Start the transmitter.
	 */
	if (tx_bd != tx_bd_start) {
		tx_bd->status |= TXBD_L;
		}
	else {
		status |= TXBD_L;
		}
	tx_bd_start->status |= status;
	*(base + MX35_X_DES_ACTIVE) = X_DES_ACTIVE;

	_mutex_unlock (&ext->tx_mutex);
	return (EOK);
}


/***************************************************************************/
/*                                                                         */
/***************************************************************************/

/*
 * mx35_fec_enet_send_packets
 *
 * Entry point from io-net layer by protocol to send
 * packets out the NIC
 *
 */

int mx35_fec_send_packets (npkt_t *npkt, void *hdl)

{
mx35_t		*ext = (mx35_t *) hdl;
net_buf_t		*buf;
int				ret = EOK;

	if (npkt->flags & _NPKT_MSG) {
		buf = TAILQ_FIRST(&npkt->buffers);
		if(buf != NULL) {
			/* If selective multicasting worked on this card 
			 this is where it would happen */
			if (ext->cfg.flags & NIC_FLAG_MULTICAST) {
				do_multicast (ext, (io_net_msg_join_mcast_t *)buf->net_iov->iov_base);
				}
			else {
				ret = ENOTSUP;
				}
			}
		
		ion_tx_complete (ext->reg_hdl, npkt);
		}
	else {
		mx35_fec_send (ext, npkt);
		}

	return (ret);
}


/***************************************************************************/
/*                                                                         */
/***************************************************************************/

/* 
 * mx35_fec_download_complete
 *
 * Check to see if any pkts can be reaped (released back to protocol).
 *
 */

int	 mx35_fec_download_complete (mx35_t *ext)

{
	_mutex_lock (&ext->tx_mutex);
	mx35_fec_reap_pkts (ext, 0);
	_mutex_unlock (&ext->tx_mutex);
	return (EOK);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int 	mx35_fec_flush (int reg_hdl, void *func_hdl)

{
	return (EOK);
}

