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

int 	mx35_fec_shutdown1 (int reg_hdl, void *func_hdl)

{
mx35_t	*ext = (mx35_t *) func_hdl;

	if (ext->cfg.verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mx35: shutdown1() starting");
		}

	InterruptDetach (ext->iid);
	ConnectDetach (ext->coid);
	ChannelDestroy (ext->chid);

	if (ext->mdi) {
		MDI_DisableMonitor (ext->mdi);
		MDI_DeRegister (&ext->mdi);
		}

	if (ext->cfg.verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mx35: shutdown1() done");
		}

	return (0);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int		mx35_fec_shutdown2 (int reg_hdl, void *func_hdl)

{
mx35_t		*ext = (mx35_t *) func_hdl;
npkt_t			*npkt;
uint32_t		status;
volatile	uint32_t	*base = ext->reg;

	if (ext->cfg.verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mx35: shutdown2() starting");
		}

	pthread_cancel (ext->tid);	
	pthread_join (ext->tid, NULL);

	mx35_fec_flush (ext->reg_hdl, ext);

	*(base + MX35_IMASK) = 0;
	*(base + MX35_X_CNTRL) = XCNTRL_GTS;
	status = *(base + MX35_IEVENT);
	while ((status & IEVENT_GRA) != IEVENT_GRA) {
		nanospin_ns (1000);
		status = *(base + MX35_IEVENT);
		}
	*(base + MX35_ECNTRL) = ECNTRL_RESET;
	nanospin_ns (1000);

	for( ; npkt = ext->rx_free_pkt_q; ) {
		ext->rx_free_pkt_q = npkt->next;
		ion_free (npkt->org_data);
		ion_free (npkt);
		}

	munmap ((void *) ext->rx_bd, sizeof (mx35_fec_bd_t) * ext->num_rx_descriptors);
	munmap ((void *) ext->tx_bd, sizeof (mx35_fec_bd_t) * ext->num_tx_descriptors);
	free (ext->rx_pktq);
	free (ext->tx_pktq);
		
	pthread_mutex_destroy (&ext->tx_mutex);
	pthread_mutex_destroy (&ext->rx_free_pkt_q_mutex);

	if (ext->cfg.verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mx35: shutdown2() ending");
		}

	free (ext);

	return (0);
}
