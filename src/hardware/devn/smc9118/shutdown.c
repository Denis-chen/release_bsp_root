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
smc9118_shutdown1(int reg_hdl, void *hdl)
{
	smc9118_t		*dev = hdl;

	pthread_mutex_lock(&dev->mutex);

	if (dev->cfg.verbose)
		nic_slogf(_SLOGC_NETWORK, _SLOG_INFO, "devn-smc9118: Shutdown1()");

	if (dev->mdi) {
		MDI_DisableMonitor(dev->mdi);
		MDI_DeRegister(&dev->mdi);
		dev->mdi = 0;
	}

	pthread_mutex_unlock(&dev->mutex);

	return 0;
}

int
smc9118_shutdown2(int reg_hdl, void *hdl)
{
	smc9118_t		*dev = hdl;
	uintptr_t		iobase = dev->iobase;
	npkt_t			*npkt;

	if (dev->cfg.verbose)
		nic_slogf(_SLOGC_NETWORK, _SLOG_INFO, "devn-smc9118: Shutdown2()");

	pthread_cancel(dev->tid);
	pthread_join(dev->tid, NULL);

	/* Disable all interrupts. */
	OUTLE32(iobase + SMC9118_INT_EN, 0);

	InterruptDetach(dev->iid);
	ConnectDetach(dev->coid);
	ChannelDestroy(dev->chid);

	/* Tell io-net to finish off any packets sitting in the  tx queue */
	smc9118_flush(dev->reg_hdl, dev);

	/* Free the rx free list */
	for (; npkt = dev->rx_free;) {
		dev->rx_free = npkt->next;
		dev->ion->free(npkt->org_data);
		dev->ion->free(npkt);
	}

	/* Destroy the mutexes */
	pthread_mutex_destroy(&dev->mutex);
	pthread_mutex_destroy(&dev->rx_freeq_mutex);
	munmap_device_io(dev->iobase, SMC9118_MMAP_SIZE);

	free(dev);

	return 0;
}

__SRCVERSION("shutdown.c $Rev: 262087 $");
