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
#include <sys/sockio.h>

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int mx35_fec_devctl (void *hdl, int dcmd, void *data, size_t size, union _io_net_dcmd_ret_cred *ret_cred)

{
mx35_t			*ext = (mx35_t *) hdl;
int 				status, promoff = 0;
nic_config_t		*cfg = &ext->cfg;
volatile	uint32_t	*base = ext->reg;

	status = EOK;

	switch (dcmd) {

		case DCMD_IO_NET_GET_STATS:
			_mutex_lock (&ext->tx_mutex);
			mx35_fec_update_stats (ext);
			_mutex_unlock (&ext->tx_mutex);
			memcpy (data, &ext->stats, sizeof (nic_stats_t));
			break;
			   
		case DCMD_IO_NET_GET_CONFIG:
			memcpy (data, &ext->cfg, sizeof (ext->cfg));
			status = EOK;
			break;

		case DCMD_IO_NET_PROMISCUOUS:
			if (ext->version != 0 && ret_cred->ret_cred.cred->euid) {
				status = EPERM;
				break;
				}
			
			if (*(int *) data) {
				cfg->flags |= NIC_FLAG_PROMISCUOUS;
				*(base + MX35_R_CNTRL) |= RCNTRL_PROM;
				}
			else {
				cfg->flags &= ~NIC_FLAG_PROMISCUOUS;
				*(base + MX35_R_CNTRL) &= ~RCNTRL_PROM;
				promoff = 1;
				}				
			break;

		case	DCMD_IO_NET_CHANGE_MCAST:
			if (cfg->flags & NIC_FLAG_MULTICAST) {
				status = do_multicast (ext, (io_net_msg_join_mcast_t *) data);
				}
			else {
				status = ENOTSUP;
				}
			break;

		case DCMD_IO_NET_TX_FLUSH:
			_mutex_lock (&ext->tx_mutex);
			mx35_fec_reap_pkts (ext, 0);
			_mutex_unlock (&ext->tx_mutex);
			break;

		default:
			status = ENOTSUP;
			break;
		}
		
	return (status);
}
