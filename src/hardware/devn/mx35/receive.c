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

static int mx35_fec_add_pkt (mx35_t *ext, npkt_t *npkt, int idx)

{
mx35_fec_bd_t		*bd = &ext->rx_bd [idx];
net_buf_t		*buf = TAILQ_FIRST (&npkt->buffers);
net_iov_t		*iov = buf->net_iov;
uint16_t        status;

	ext->rx_pktq [idx] = npkt;
	CACHE_INVAL(&ext->cachectl, iov->iov_base, iov->iov_phys, iov->iov_len);
	bd->buffer = iov->iov_phys;
	status = RXBD_E;
	if (idx == ext->num_rx_descriptors -1) {
		status |= RXBD_W;
		}
	bd->length = 0;
	bd->status = status;

	return (EOK);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int		mx35_fec_receive_complete (npkt_t *npkt, void *done_hdl, void *func_hdl)

{
mx35_t		*ext = (mx35_t *) done_hdl;
net_buf_t		*buf;

	_mutex_lock (&ext->rx_free_pkt_q_mutex);

	if (ext->num_rx_free < ext->num_rx_descriptors) {
		npkt->next = ext->rx_free_pkt_q;
		ext->rx_free_pkt_q = npkt;
		ext->num_rx_free++;

		npkt->ref_cnt = 1;
		npkt->tot_iov = 1;
		npkt->req_complete = 0;
		npkt->flags = _NPKT_UP;
		npkt->iface = 0;

		buf = TAILQ_FIRST (&npkt->buffers);
		buf->net_iov->iov_len = MX35_MTU_SIZE;
		buf->niov = 1;
		_mutex_unlock (&ext->rx_free_pkt_q_mutex);
		}
	else {
		_mutex_unlock (&ext->rx_free_pkt_q_mutex);
		ion_free (npkt->org_data);
		ion_free (npkt);
		}
	return (0);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

void	mx35_fec_receive (mx35_t *ext)

{
npkt_t					*npkt, *new;
net_buf_t				*buf;
int						cidx;
uint16_t				status;
mx35_fec_bd_t				*rx_bd;
nic_ethernet_stats_t	*estats = &ext->stats.un.estats;

	/* mask off the rx_interrupt */
	while (1) {
		cidx = ext->rx_cidx;
		rx_bd = &ext->rx_bd [cidx];
		status = rx_bd->status;
		if (status & RXBD_E) {
			break;
			}
		
		/* take a packet off the free queue */
		_mutex_lock (&ext->rx_free_pkt_q_mutex);
		if ((new = ext->rx_free_pkt_q)) {
			ext->rx_free_pkt_q = new->next;
			new->next = NULL;
			ext->num_rx_free--;
			_mutex_unlock (&ext->rx_free_pkt_q_mutex);
			}
		else {
			_mutex_unlock (&ext->rx_free_pkt_q_mutex);
			if (!(new = mx35_fec_alloc_npkt (ext, MX35_MTU_SIZE))) {
				errno = ENOBUFS;
				nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "devn-mx35_fec: Packet Allocation failure.");
				break;
				}
			}

		npkt = ext->rx_pktq [cidx];                
		ext->rx_pktq [cidx] = NULL;
		ext->rx_cidx = NEXT_RX (ext->rx_cidx);

		if (status & RXBD_ERR) {
			if (status & (RXBD_TR | RXBD_SH | RXBD_LG))
				estats->length_field_outrange++;
                        
			if (status & RXBD_OV)
				estats->internal_rx_errors++;

			if (status & RXBD_NO)
				estats->align_errors++;

			if (status & RXBD_CR)
				estats->fcs_errors++;
			nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "devn-mx35_fec: status = %08x\n", status);			
			mx35_fec_add_pkt (ext, npkt, cidx);
			continue;                        
			}

		npkt->framelen = (rx_bd->length - 4);
		ext->stats.octets_rxed_ok += npkt->framelen;
		ext->stats.rxed_ok++;
		buf = TAILQ_FIRST (&npkt->buffers);
		buf->net_iov->iov_len = npkt->framelen;


		/* & give the packet to io-net */
		npkt->next = NULL;

		if (ext->cfg.flags & NIC_FLAG_PROMISCUOUS) {
			npkt->flags |= _NPKT_PROMISC;
			}
		
		if (ion_add_done (ext->reg_hdl, npkt, ext) == -1) {
			mx35_fec_receive_complete (npkt, ext, ext);
			mx35_fec_add_pkt (ext, new, cidx);
			continue;
			}
		if (ext->ion->tx_up (ext->reg_hdl, npkt, 0, 0, ext->cell, ext->cfg.lan, 0) <= 0)	{
			mx35_fec_receive_complete (npkt, ext, ext);
			}
		mx35_fec_add_pkt (ext, new, cidx);
		}
	return;
}

