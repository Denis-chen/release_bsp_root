/*
 * $QNXLicenseC: 
 * Copyright 2007 - 2009, QNX Software Systems.  
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <atomic.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/neutrino.h>

#include "imx35.h"

const struct sigevent *
imx35_isr(void *arg, int id)
{
	imx35_context_t		*imx35 = arg;
	layer_info		*linfo;
	uint32_t		dma_addr, *params;

	if (*IPU_INT_STAT_3 & IPU_IRQ_SDC_DISP3_VSYNC) {
		linfo = &imx35->layer[0];

		switch (linfo->update_flags) {
		default:
			if (!linfo->enable) {
				*IDMAC_CHA_EN &= ~(1 << IMX35_DMA_CHAN_0);
			}
			break;
		case 2:
		case 3:
			params = linfo->params;
			dma_addr = 0x10000 | (IMX35_DMA_CHAN_0 << 4);

			*IDMAC_CHA_EN &= ~(1 << IMX35_DMA_CHAN_0);

			atomic_clr(&linfo->update_flags, 3);

			InterruptLock(&imx35->spinlock);

			*IPU_IMA_ADDR = dma_addr+3;
			*IPU_IMA_DATA = params[0];
			*IPU_IMA_ADDR = dma_addr+4;
			*IPU_IMA_DATA = params[1];
			*IPU_IMA_ADDR = dma_addr+8;
			*IPU_IMA_DATA = params[2];
			*IPU_IMA_ADDR = dma_addr+10;
			*IPU_IMA_DATA = params[3];
			*IPU_IMA_ADDR = dma_addr+11;
			*IPU_IMA_DATA = params[4];
			*IPU_IMA_ADDR = dma_addr+12;
			*IPU_IMA_DATA = params[5];

			InterruptUnlock(&imx35->spinlock);

			*IPU_CHA_BUF0_RDY = 1 << IMX35_DMA_CHAN_0;

			if (linfo->enable) {
				*IDMAC_CHA_EN |= 1 << IMX35_DMA_CHAN_0;
			}
			break;
		case 1:
			if (!linfo->enable) {
				*IDMAC_CHA_EN &= ~(1 << IMX35_DMA_CHAN_0);
			}

			atomic_clr(&linfo->update_flags, 1);

			/* Offset only */
			*IPU_IMA_ADDR = 0x10000 | (IMX35_DMA_CHAN_0<<4) + 8;
			*IPU_IMA_DATA = imx35->layer[0].commit_offset;
			*IPU_CHA_BUF0_RDY = 1 << IMX35_DMA_CHAN_0;
			break;
		}

		linfo = &imx35->layer[1];

		switch (linfo->update_flags) {
		default:
			if (!linfo->enable) {
				*IDMAC_CHA_EN &= ~(1 << IMX35_DMA_CHAN_1);
			}
			break;
		case 2:
		case 3:
			params = linfo->params;
			dma_addr = 0x10000 | (IMX35_DMA_CHAN_1 << 4);

			*IDMAC_CHA_EN &= ~(1 << IMX35_DMA_CHAN_1);

			atomic_clr(&linfo->update_flags, 3);

			InterruptLock(&imx35->spinlock);

			*IPU_IMA_ADDR = dma_addr+3;
			*IPU_IMA_DATA = params[0];
			*IPU_IMA_ADDR = dma_addr+4;
			*IPU_IMA_DATA = params[1];
			*IPU_IMA_ADDR = dma_addr+8;
			*IPU_IMA_DATA = params[2];
			*IPU_IMA_ADDR = dma_addr+10;
			*IPU_IMA_DATA = params[3];
			*IPU_IMA_ADDR = dma_addr+11;
			*IPU_IMA_DATA = params[4];
			*IPU_IMA_ADDR = dma_addr+12;
			*IPU_IMA_DATA = params[5];

			InterruptUnlock(&imx35->spinlock);

			*IPU_CHA_BUF0_RDY = 1 << IMX35_DMA_CHAN_1;

			if (linfo->enable) {
				*IDMAC_CHA_EN |= 1 << IMX35_DMA_CHAN_1;
			}
			break;
		case 1:
			if (!linfo->enable) {
				*IDMAC_CHA_EN &= ~(1 << IMX35_DMA_CHAN_1);
			}

			atomic_clr(&linfo->update_flags, 1);

			/* Offset only */
			*IPU_IMA_ADDR = 0x10000 | (IMX35_DMA_CHAN_1<<4) + 8;
			*IPU_IMA_DATA = imx35->layer[1].commit_offset;
			*IPU_CHA_BUF0_RDY = 1 << IMX35_DMA_CHAN_1;
			break;
		}

		atomic_add(&imx35->vsync_counter, 1);

		*IPU_INT_STAT_3 |= IPU_IRQ_SDC_DISP3_VSYNC;


		if (imx35->want_vsync_pulse) {
			imx35->want_vsync_pulse = 0;
			return &imx35->vsync_event;
		}
	}

	return NULL;
}

void 
imx35_irq_cleanup (imx35_context_t *imx35)
{
	*IPU_INT_CTRL_3 &= ~IPU_IRQ_SDC_DISP3_VSYNC;	/* Disable vsync interrupts */

        InterruptDetach(imx35->intrid);
        ConnectDetach(imx35->vsync_coid);
        ChannelDestroy(imx35->vsync_chan);
}

int
imx35_irq_setup (disp_adapter_t *adapter)
{
	imx35_context_t		*imx35 = adapter->ms_ctx;

        if ((imx35->vsync_chan = ChannelCreate(0)) == -1)
               	return -1; 

        if ((imx35->vsync_coid = ConnectAttach(0, 0,
                imx35->vsync_chan, _NTO_SIDE_CHANNEL, 0)) == -1) {
		goto fail1;
	}

        imx35->intrid = InterruptAttach(_NTO_INTR_CLASS_EXTERNAL|imx35->irq,
                imx35_isr, imx35, sizeof (*imx35),
                _NTO_INTR_FLAGS_TRK_MSK |
                _NTO_INTR_FLAGS_END | _NTO_INTR_FLAGS_PROCESS);

        if (imx35->intrid == -1) {
                perror("InterruptAttach");
		goto fail2;
        }

        SIGEV_PULSE_INIT(&imx35->vsync_event,
                imx35->vsync_coid, adapter->pulseprio, IPU_DISP3_VSYNC_PULSE, 0);

	return 0;

fail2:
       	ConnectDetach(imx35->vsync_coid);
fail1:
       	ChannelDestroy(imx35->vsync_chan);

	return -1;
}


