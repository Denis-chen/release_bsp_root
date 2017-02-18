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
 
/*
 * mx35_spdif_dll.c
 *   The primary interface into the mx35_spdif DLL.
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <sys/asoundlib.h>
#include <devctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mx35_spdif.h"


void mx35_spdif_interrupt_handler (HW_CONTEXT_T *mx35_spdif, int32_t irq)
{
	unsigned long int_stat;

	ado_mutex_lock (&mx35_spdif->hw_lock);

	/* get spdif interrupt status and clear the interrupt */
	int_stat = mx35_spdif->spdif->sisc & 0xffffff;
	int_stat &= mx35_spdif->spdif->sie;
	mx35_spdif->spdif->sisc = int_stat;

	if (int_stat & (INT_DPLL_LOCKED | INT_LOSS_LOCK)) {
		if (mx35_spdif->spdif->srpc & SRPC_DPLL_LOCKED) {
			/* INT_DPLL_LOCKED */
			atomic_set(&mx35_spdif->dpll_locked, 1);
		} else {
			/* INT_LOSS_LOCK */
			atomic_clr(&mx35_spdif->dpll_locked, 1);
		}
	}

	/*
	 * Handle other interrupts here if required. We don't have to do
	 * anything other than clear the interrupt status bit for most
	 * interrupts, and we don't have a clean way to handle the errors.
	 */

	ado_mutex_unlock (&mx35_spdif->hw_lock);
}

static int
mx35_spdif_init (HW_CONTEXT_T * mx35_spdif)
{
	/* soft reset */
	mx35_spdif->spdif->scr = SCR_SOFT_RESET;
	usleep (10);

	/* enable interrupt */
	mx35_spdif->spdif->sie = 0;
	if (ado_attach_interrupt (mx35_spdif->card, SPDIF_IRQ,
	    mx35_spdif_interrupt_handler, mx35_spdif) != 0)
		return -1;

	return 0;
}

static int
mx35_spdif_fini (HW_CONTEXT_T * mx35_spdif)
{
	mx35_spdif->spdif->scr &= ~0x1c;
	return 0;
}

static void
mx35_init_spdif_device (HW_CONTEXT_T * mx35_spdif)
{
#define IEC958_AES0_CON_NOT_COPYRIGHT	(1 << 2)
#define IEC958_AES0_CON_EMPHASIS_5015	(1 << 3)
#define IEC958_AES1_CON_DIGDIGCONV_ID	0x02
#define IEC958_AES3_CON_FS_44100		(0 << 0)
#define IEC958_AES3_CON_CLOCK_1000PPM	(0 << 4)

	mx35_spdif->ch_status[0] = IEC958_AES0_CON_NOT_COPYRIGHT |
	    IEC958_AES0_CON_EMPHASIS_5015;
	mx35_spdif->ch_status[1] = IEC958_AES1_CON_DIGDIGCONV_ID;
	mx35_spdif->ch_status[2] = 0x00;
	mx35_spdif->ch_status[3] = IEC958_AES3_CON_FS_44100 |
	    IEC958_AES3_CON_CLOCK_1000PPM;
}

static void mx35_spdif_tx_init (HW_CONTEXT_T * mx35_spdif)
{
	mx35_spdif->spdif->scr &= 0xfc32e3;
	mx35_spdif->spdif->scr |= (SCR_TXFIFO_AUTOSYNC | SCR_TXFIFO_NORMAL |
	    SCR_TXSEL_NORMAL | SCR_USRC_SEL_CHIP | (2 << SCR_TXFIFO_ESEL_BIT));

	/* Default clock source from EXTAL, divide by 8, generate 44.1 kHz */
	mx35_spdif->spdif->stc = 0x07;
}

static void mx35_spdif_tx_fini (HW_CONTEXT_T * mx35_spdif)
{
	mx35_spdif->spdif->scr &= 0xffffe3;
	mx35_spdif->spdif->scr |= SCR_TXSEL_OFF;
}

static void mx35_spdif_rx_init (HW_CONTEXT_T * mx35_spdif)
{
	/* RxFIFO off, RxFIFO sel to 8 sample, Autosync, Valid bit set */
	mx35_spdif->spdif->stc &= ~(SCR_RXFIFO_OFF |
		SCR_RXFIFO_CTL_ZERO | SCR_LOW_POWER);
	mx35_spdif->spdif->stc |= (2 << SCR_RXFIFO_FSEL_BIT) | SCR_RXFIFO_AUTOSYNC;
}

static void mx35_spdif_rx_fini (HW_CONTEXT_T * mx35_spdif)
{
	/* turn off RX fifo, disable dma and autosync */
	mx35_spdif->spdif->stc |= SCR_RXFIFO_OFF | SCR_RXFIFO_CTL_ZERO;
	mx35_spdif->spdif->stc &= ~(SCR_DMA_RX_EN | SCR_RXFIFO_AUTOSYNC);
}


int32_t
mx35_spdif_capabilities (HW_CONTEXT_T * mx35_spdif,
	snd_pcm_channel_info_t * info)
{
	int chn_avail = 1;

	if (info->channel == SND_PCM_CHANNEL_PLAYBACK &&
		mx35_spdif->play_strm.pcm_subchn)
		chn_avail = 0;
	else if (info->channel == SND_PCM_CHANNEL_CAPTURE &&
		mx35_spdif->cap_strm.pcm_subchn)
		chn_avail = 0;

	if (chn_avail == 0)
	{
		info->formats = 0;
		info->rates = 0;
		info->min_rate = 0;
		info->max_rate = 0;
		info->min_voices = 0;
		info->max_voices = 0;
		info->min_fragment_size = 0;
		info->max_fragment_size = 0;
	}

	return (0);
}

static void
mx35_spdif_frag_dmabuf_sizer (HW_CONTEXT_T * mx35_spdif,
	ado_pcm_config_t * config, ado_pcm_subchn_t * subchn)
{
	int32_t result, frag_size;

	frag_size = ado_pcm_dma_int_size (config);
	result = (frag_size / 16) * 16;
	if (config->trans_mode == SND_PCM_MODE_BLOCK)
	{
		config->mode.block.frag_size = result;
		config->dmabuf.size = config->mode.block.frags_total *
			config->mode.block.frag_size;
	}
	else
	{
		config->mode.stream.queue_size = result;
		config->dmabuf.size = config->mode.stream.queue_size;
	}
}

int32_t
mx35_spdif_playback_aquire (HW_CONTEXT_T * mx35_spdif,
	PCM_SUBCHN_CONTEXT_T ** pc, ado_pcm_config_t * config,
	ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
	dma_transfer_t tinfo;
	int     frag_idx;
	
	if (mx35_spdif->play_strm.pcm_subchn)
	{
		*why_failed = SND_PCM_PARAMS_NO_CHANNEL;
		return (EAGAIN);
	}

	ado_mutex_lock (&mx35_spdif->hw_lock);

	mx35_spdif_frag_dmabuf_sizer (mx35_spdif, config, subchn);

	if ((config->dmabuf.addr = ado_shm_alloc (config->dmabuf.size,
		config->dmabuf.name, ADO_SHM_DMA_SAFE,
		&config->dmabuf.phys_addr)) == NULL)
	{
		ado_mutex_unlock (&mx35_spdif->hw_lock);
		return (errno);
	}

	memset (&tinfo, 0, sizeof (tinfo));
	tinfo.src_addrs = malloc (config->mode.block.frags_total *
		sizeof (dma_addr_t));
	for (frag_idx = 0; frag_idx < config->mode.block.frags_total; frag_idx++)
	{
    	tinfo.src_addrs[frag_idx].paddr = config->dmabuf.phys_addr +
			(frag_idx * config->mode.block.frag_size);
	    tinfo.src_addrs[frag_idx].len = config->mode.block.frag_size;
	}
	tinfo.src_fragments = config->mode.block.frags_total;
	tinfo.xfer_unit_size = 16;		/* 16-bit samples */
	tinfo.xfer_bytes = config->dmabuf.size;

	mx35_spdif->sdmafuncs.setup_xfer (mx35_spdif->play_strm.dma_chn, &tinfo);
	free (tinfo.src_addrs);

	mx35_spdif->play_strm.pcm_subchn = *pc = subchn;
	mx35_spdif->play_strm.pcm_config = config;
	ado_mutex_unlock (&mx35_spdif->hw_lock);
	return (EOK);
}

int32_t
mx35_spdif_playback_release (HW_CONTEXT_T * mx35_spdif,
	PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	ado_mutex_lock (&mx35_spdif->hw_lock);

	/* spdif Tx uninit */
	mx35_spdif->spdif->sie &= ~INT_TXFIFO_RESYNC;
	mx35_spdif_tx_fini (mx35_spdif);

	mx35_spdif->play_strm.pcm_subchn = NULL;
	ado_shm_free (config->dmabuf.addr, config->dmabuf.size,
		config->dmabuf.name);

	ado_mutex_unlock (&mx35_spdif->hw_lock);

	return (0);
}

int32_t
mx35_spdif_playback_prepare (HW_CONTEXT_T * mx35_spdif,
	PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	unsigned int ch_status;

	/* init SPDIF for Tx */
	mx35_spdif_tx_init (mx35_spdif);

	/* set channel status bits */
	ch_status = (mx35_spdif->ch_status[2] << 16) |
	    (mx35_spdif->ch_status[1] << 8) | mx35_spdif->ch_status[0];
	mx35_spdif->spdif->stcsch = ch_status;
	ch_status = mx35_spdif->ch_status[3];
	mx35_spdif->spdif->stcscl = ch_status;

	/* enable interrupts */
	mx35_spdif->spdif->sie |= INT_TXFIFO_RESYNC;

#if 0
	/* set sample rate (fixed 44.1 kHz) */
	mx35_spdif->spdif->stc |= (0x3 << 8) | 0x07;
#else
	/* audio osc source for 48 kHz clock */
	mx35_spdif->spdif->stc |= (0x0 << 8) | 0x07;
	/* set sample rate to 48 kHz */
	mx35_spdif->spdif->stcscl |= 0x04;
#endif

	/* set clock accuracy */
	mx35_spdif->spdif->stcscl &= 0xffffcf;

	return (0);
}

int32_t
mx35_spdif_playback_trigger (HW_CONTEXT_T * mx35_spdif, PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
	int rtn;

	ado_mutex_lock (&mx35_spdif->hw_lock);
	
	if (cmd == ADO_PCM_TRIGGER_GO)
	{
		if (pc == mx35_spdif->play_strm.pcm_subchn)
		{ 
			if (mx35_spdif->sdmafuncs.xfer_start
				(mx35_spdif->play_strm.dma_chn) == -1)
			{
			    rtn = errno;
				ado_error ("MX35 SPDIF Audio DMA Start failed (%s)",
					strerror(rtn));
			}
			/* start DMA transmit request */
			mx35_spdif->spdif->scr |= SCR_DMA_TX_EN;
		}
	}
	else
	{
		if (pc == mx35_spdif->play_strm.pcm_subchn)
		{
			if (mx35_spdif->sdmafuncs.xfer_abort
				(mx35_spdif->play_strm.dma_chn) == -1)
			{
			    rtn = errno;
			    ado_error ("MX35 SPDIF Audio DMA Stop failed (%s)",
					strerror(rtn));
			}
			/* stop DMA transmit request */
			mx35_spdif->spdif->scr &= ~SCR_DMA_TX_EN;
		}
	}
	ado_mutex_unlock (&mx35_spdif->hw_lock);
	return (0);
}

void
mx35_spdif_play_pulse_hdlr (HW_CONTEXT_T * mx35_spdif, struct sigevent *event)
{
	if (mx35_spdif->play_strm.pcm_subchn)
		dma_interrupt (mx35_spdif->play_strm.pcm_subchn);
}

int32_t
mx35_spdif_capture_aquire (HW_CONTEXT_T * mx35_spdif,
	PCM_SUBCHN_CONTEXT_T ** pc, ado_pcm_config_t * config,
	ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
	dma_transfer_t tinfo;
	int frag_idx;
	
	if (mx35_spdif->cap_strm.pcm_subchn)
	{
		*why_failed = SND_PCM_PARAMS_NO_CHANNEL;
		return (EAGAIN);
	}

	ado_mutex_lock (&mx35_spdif->hw_lock);

	mx35_spdif_frag_dmabuf_sizer (mx35_spdif, config, subchn);

	if ((config->dmabuf.addr = ado_shm_alloc (config->dmabuf.size,
		config->dmabuf.name, ADO_SHM_DMA_SAFE,
		&config->dmabuf.phys_addr)) == NULL)
	{
		ado_mutex_unlock (&mx35_spdif->hw_lock);
		return (errno);
	}

	memset (&tinfo, 0, sizeof (tinfo));
	tinfo.dst_addrs = malloc (config->mode.block.frags_total *
		sizeof (dma_addr_t));
	for (frag_idx = 0; frag_idx < config->mode.block.frags_total; frag_idx++)
	{
    	tinfo.dst_addrs[frag_idx].paddr = config->dmabuf.phys_addr +
			(frag_idx * config->mode.block.frag_size);
	    tinfo.dst_addrs[frag_idx].len = config->mode.block.frag_size;
	}
	tinfo.dst_fragments = config->mode.block.frags_total;
	tinfo.xfer_unit_size = 16;		/* 16-bit samples */
	tinfo.xfer_bytes = config->dmabuf.size;

	mx35_spdif->sdmafuncs.setup_xfer (mx35_spdif->cap_strm.dma_chn, &tinfo);
	free (tinfo.dst_addrs);

	mx35_spdif->cap_strm.pcm_subchn = *pc = subchn;
	mx35_spdif->cap_strm.pcm_config = config;
	ado_mutex_unlock (&mx35_spdif->hw_lock);
	return (EOK);
}

int32_t
mx35_spdif_capture_release (HW_CONTEXT_T * mx35_spdif,
	PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	ado_mutex_lock (&mx35_spdif->hw_lock);

	/* spdif Rx uninit */
	mx35_spdif->spdif->sie &= ~(INT_DPLL_LOCKED | INT_SYM_ERR | INT_BIT_ERR |
			  INT_URX_FUL | INT_URX_OV | INT_QRX_FUL | INT_QRX_OV |
			  INT_UQ_SYNC | INT_UQ_ERR | INT_RX_RESYNC | INT_LOSS_LOCK);
	mx35_spdif_rx_fini (mx35_spdif);

	mx35_spdif->cap_strm.pcm_subchn = NULL;
	ado_shm_free (config->dmabuf.addr, config->dmabuf.size,
		config->dmabuf.name);

	ado_mutex_unlock (&mx35_spdif->hw_lock);

	return (0);
}

int32_t
mx35_spdif_capture_prepare (HW_CONTEXT_T * mx35_spdif,
	PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	/* enable spdif dpll lock interrupt */
	mx35_spdif->spdif->sie |= INT_DPLL_LOCKED;

	/* init SPDIF for Rx */
	mx35_spdif_rx_init (mx35_spdif);

	/* enable interrupts, include DPLL lock */
	mx35_spdif->spdif->sie |= (INT_SYM_ERR | INT_BIT_ERR | INT_URX_FUL |
			  INT_URX_OV | INT_QRX_FUL | INT_QRX_OV |
			  INT_UQ_SYNC | INT_UQ_ERR | INT_RX_RESYNC |
			  INT_LOSS_LOCK);

	/* setup rx clock source */
	mx35_spdif->spdif->srpc = (0 << SRPC_CLKSRC_SEL_OFFSET) | 
			  (SPDIF_DEFAULT_GAINSEL << SRPC_GAINSEL_OFFSET);

	return (0);
}

int32_t
mx35_spdif_capture_trigger (HW_CONTEXT_T * mx35_spdif,
	PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
	int rtn;

	ado_mutex_lock (&mx35_spdif->hw_lock);
	
	if (cmd == ADO_PCM_TRIGGER_GO)
	{
		if (pc == mx35_spdif->cap_strm.pcm_subchn)
		{ 
			if (mx35_spdif->sdmafuncs.xfer_start
				(mx35_spdif->cap_strm.dma_chn) == -1)
			{
			    rtn = errno;
				ado_error ("MX35 SPDIF Audio DMA Start failed (%s)",
					strerror(rtn));
			}
			/* start DMA transmit request */
			mx35_spdif->spdif->scr |= SCR_DMA_RX_EN;
		}
	}
	else
	{
		if (pc == mx35_spdif->cap_strm.pcm_subchn)
		{
			if (mx35_spdif->sdmafuncs.xfer_abort
				(mx35_spdif->cap_strm.dma_chn) == -1)
			{
			    rtn = errno;
				ado_error ("MX35 SPDIF Audio DMA Stop failed (%s)",
					strerror(rtn));
			}
			/* stop DMA transmit request */
			mx35_spdif->spdif->scr &= ~SCR_DMA_RX_EN;
		}
	}

	ado_mutex_unlock (&mx35_spdif->hw_lock);

	return (0);
}

void
mx35_spdif_cap_pulse_hdlr (HW_CONTEXT_T * mx35_spdif, struct sigevent *event)
{
	if (mx35_spdif->cap_strm.pcm_subchn)
		dma_interrupt (mx35_spdif->cap_strm.pcm_subchn);
}


ado_dll_version_t ctrl_version;
void
ctrl_version (int *major, int *minor, char *date)
{
	*major = ADO_MAJOR_VERSION;
	*minor = 1;
	date = __DATE__;
}

ado_ctrl_dll_init_t ctrl_init;
int
ctrl_init (HW_CONTEXT_T ** hw_context, ado_card_t * card, char *args)
{
	mx35_spdif_t *mx35_spdif;
	unsigned int channel;
	
	ado_debug (DB_LVL_DRIVER, "CTRL_DLL_INIT: MX35 SPDIF");

	if ((mx35_spdif = (mx35_spdif_t *) ado_calloc (1,
		sizeof (mx35_spdif_t))) == NULL)
	{
		ado_error ("Unable to allocate memory for mx35_spdif (%s)",
			strerror (errno));
		return -1;
	}

	mx35_spdif->card = card;
	*hw_context = mx35_spdif;
	ado_card_set_shortname (card, "MX35_SPDIF");
	ado_card_set_longname (card, "i.MX35 S/PDIF", 0);
	
	mx35_spdif->spdif = mmap_device_memory (0, sizeof (spdif_t),
		PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, SPDIF_BASE);
	if (mx35_spdif->spdif == MAP_FAILED)
	{
		ado_error ("Unable to mmap SPDIF registers (%s)", strerror (errno));
		ado_free (mx35_spdif);
		return -1;
	}

	ado_mutex_init (&mx35_spdif->hw_lock);

	if (get_dmafuncs (&mx35_spdif->sdmafuncs, sizeof (dma_functions_t)) == -1)
	{
	    ado_error ("Failed to get DMA lib functions");
	    ado_mutex_destroy (&mx35_spdif->hw_lock);
	    munmap_device_memory (mx35_spdif->spdif, sizeof (spdif_t));
	    ado_free (mx35_spdif);
	    return -1;
	}

	my_attach_pulse (&mx35_spdif->play_strm.pulse,
		&mx35_spdif->play_strm.sdma_event, mx35_spdif_play_pulse_hdlr,
		mx35_spdif);

	my_attach_pulse (&mx35_spdif->cap_strm.pulse,
		&mx35_spdif->cap_strm.sdma_event, mx35_spdif_cap_pulse_hdlr,
		mx35_spdif);

	if (mx35_spdif->sdmafuncs.init (NULL) == -1)
	{
	    ado_error ("DMA init failed");
	    my_detach_pulse (&mx35_spdif->cap_strm.pulse);
	    my_detach_pulse (&mx35_spdif->play_strm.pulse);
	    ado_mutex_destroy (&mx35_spdif->hw_lock);
	    munmap_device_memory (mx35_spdif->spdif, sizeof (spdif_t));
	    ado_free (mx35_spdif);
	    return -1;
	}

	/*
	 * DMA channel setup for Playback
	 * 1) watermark = 8 (must match the TxFifoEmpty_Sel value in SCR)
	 * 2) eventnum = 13 (SPDIF TX DMA event)
	 * 3) fifopaddr = 0x5002802C (Physical address of SPDIF TX channel)
	 * 4) regen = DMA in repeat mode so we only need to configure the
	 *    DMA transfer on channel acquire and not on every interrupt.
	 */
#if 1
	/* experimental: using MCU-to-SPDIF script in SDMA controller RAM */
	channel = SDMA_SPDIF_PLAYBACK_CHANNEL;
#else
	channel = SDMA_PLAYBACK_CHANNEL;
#endif
    if ((mx35_spdif->play_strm.dma_chn = mx35_spdif->sdmafuncs.channel_attach 
		("eventnum=13,watermark=8,fifopaddr=0x5002802C,regen",
		&mx35_spdif->play_strm.sdma_event, &channel,
		DMA_ATTACH_PRIORITY_HIGHEST, DMA_ATTACH_EVENT_PER_SEGMENT)) == NULL)
    {
        ado_error ("SDMA playback channel attach failed");
	    my_detach_pulse (&mx35_spdif->cap_strm.pulse);
        my_detach_pulse (&mx35_spdif->play_strm.pulse);
        mx35_spdif->sdmafuncs.fini ();
        ado_mutex_destroy (&mx35_spdif->hw_lock);
        munmap_device_memory (mx35_spdif->spdif, sizeof (spdif_t));
        ado_free (mx35_spdif);
        return -1;
    }

	/*
	 * DMA channel setup for Capture
	 * 1) watermark = 8 (must match the RxFifoFull_Sel value in SCR)
	 * 2) eventnum = 12 (SPDIF RX DMA event)
	 * 3) fifopaddr = 0x50028014 (Physical address of SPDIF RX channel)
	 * 4) regen = DMA in repeat mode so we only need to configure the
	 *    DMA transfer on channel acquire and not on every interrupt.
	 */
#if 1
	/* experimental: using MCU-to-SPDIF script in SDMA controller RAM */
	channel = SDMA_SPDIF_CAPTURE_CHANNEL;
#else
	channel = SDMA_CAPTURE_CHANNEL;
#endif
    if ((mx35_spdif->cap_strm.dma_chn = mx35_spdif->sdmafuncs.channel_attach 
		("eventnum=12,watermark=8,fifopaddr=0x50028014,regen",
		&mx35_spdif->cap_strm.sdma_event, &channel,
		DMA_ATTACH_PRIORITY_HIGHEST, DMA_ATTACH_EVENT_PER_SEGMENT)) == NULL)
    {
        ado_error ("SDMA playback channel attach failed");
	    my_detach_pulse (&mx35_spdif->cap_strm.pulse);
        my_detach_pulse (&mx35_spdif->play_strm.pulse);
        mx35_spdif->sdmafuncs.fini ();
        ado_mutex_destroy (&mx35_spdif->hw_lock);
        munmap_device_memory (mx35_spdif->spdif, sizeof (spdif_t));
        ado_free (mx35_spdif);
        return -1;
    }

	/* reset and init SPDIF */
	mx35_init_spdif_device (mx35_spdif);
	mx35_spdif_init (mx35_spdif);

	/* Setup a basic mixer */
	if (mx35_spdif_mixer (card, &mx35_spdif->mixer, mx35_spdif))
	{
		ado_error ("Unable to create MX35 SPDIF mixer");
		mx35_spdif_fini (mx35_spdif);
		mx35_spdif->sdmafuncs.channel_release (mx35_spdif->play_strm.dma_chn);
	    my_detach_pulse (&mx35_spdif->cap_strm.pulse);
		my_detach_pulse (&mx35_spdif->play_strm.pulse);
		mx35_spdif->sdmafuncs.fini ();
		ado_mutex_destroy (&mx35_spdif->hw_lock);
		munmap_device_memory (mx35_spdif->spdif, sizeof (spdif_t));
		ado_free (mx35_spdif);
		return -1;
	}

	mx35_spdif->play_strm.pcm_caps.chn_flags =
		SND_PCM_CHNINFO_BLOCK | SND_PCM_CHNINFO_STREAM |
		SND_PCM_CHNINFO_INTERLEAVE | SND_PCM_CHNINFO_BLOCK_TRANSFER |
		SND_PCM_CHNINFO_MMAP | SND_PCM_CHNINFO_MMAP_VALID;
	mx35_spdif->play_strm.pcm_caps.formats = SND_PCM_FMT_S16_LE;
#if 0
	mx35_spdif->play_strm.pcm_caps.rates = SND_PCM_RATE_44100;
	mx35_spdif->play_strm.pcm_caps.min_rate = 44100;
	mx35_spdif->play_strm.pcm_caps.max_rate = 44100;
#else
	mx35_spdif->play_strm.pcm_caps.rates = SND_PCM_RATE_48000;
	mx35_spdif->play_strm.pcm_caps.min_rate = 48000;
	mx35_spdif->play_strm.pcm_caps.max_rate = 48000;
#endif
	mx35_spdif->play_strm.pcm_caps.min_voices = 2;
	mx35_spdif->play_strm.pcm_caps.max_voices = 2;
	mx35_spdif->play_strm.pcm_caps.min_fragsize = 64;
	mx35_spdif->play_strm.pcm_caps.max_fragsize = 32 * 1024;
	
	memcpy (&mx35_spdif->cap_strm.pcm_caps, &mx35_spdif->play_strm.pcm_caps,
			sizeof (mx35_spdif->cap_strm.pcm_caps));

	mx35_spdif->play_strm.pcm_funcs.capabilities = mx35_spdif_capabilities;
	mx35_spdif->play_strm.pcm_funcs.aquire = mx35_spdif_playback_aquire;
	mx35_spdif->play_strm.pcm_funcs.release = mx35_spdif_playback_release;
	mx35_spdif->play_strm.pcm_funcs.prepare = mx35_spdif_playback_prepare;
	mx35_spdif->play_strm.pcm_funcs.trigger = mx35_spdif_playback_trigger;
#if 0
	mx35_spdif->play_strm.pcm_funcs.position = mx35_spdif_position;
#endif

	mx35_spdif->cap_strm.pcm_funcs.capabilities = mx35_spdif_capabilities;
	mx35_spdif->cap_strm.pcm_funcs.aquire = mx35_spdif_capture_aquire;
	mx35_spdif->cap_strm.pcm_funcs.release = mx35_spdif_capture_release;
	mx35_spdif->cap_strm.pcm_funcs.prepare = mx35_spdif_capture_prepare;
	mx35_spdif->cap_strm.pcm_funcs.trigger = mx35_spdif_capture_trigger;
#if 0
	mx35_spdif->cap_strm.pcm_funcs.position = mx35_spdif_position;
#endif

	if (ado_pcm_create (card, "mx35_spdif PCM 0", 0, "mx35_spdif-0",
		1, &mx35_spdif->play_strm.pcm_caps, &mx35_spdif->play_strm.pcm_funcs,
		1, &mx35_spdif->cap_strm.pcm_caps, &mx35_spdif->cap_strm.pcm_funcs,
		&mx35_spdif->pcm1))
	{
		ado_error ("Unable to create pcm devices (%s)", strerror (errno));
		mx35_spdif_fini (mx35_spdif);
		mx35_spdif->sdmafuncs.channel_release (mx35_spdif->play_strm.dma_chn);
	    my_detach_pulse (&mx35_spdif->cap_strm.pulse);
		my_detach_pulse (&mx35_spdif->play_strm.pulse);
		mx35_spdif->sdmafuncs.fini ();
		ado_mutex_destroy (&mx35_spdif->hw_lock);
		munmap_device_memory (mx35_spdif->spdif, sizeof (spdif_t));
		ado_free (mx35_spdif);
		return -1;
	}

#if 0
	/* setup mixer controls for playback and capture */
	ado_pcm_chn_mixer (mx35_spdif->pcm1,
		ADO_PCM_CHANNEL_PLAYBACK, mx35_spdif->mixer,
		ado_mixer_find_element (mx35_spdif->mixer,
			SND_MIXER_ETYPE_PLAYBACK1, SND_MIXER_PCM_OUT, 0),
		ado_mixer_find_group (mx35_spdif->mixer, SND_MIXER_PCM_OUT, 0));

	ado_pcm_chn_mixer (mx35_spdif->pcm1,
		ADO_PCM_CHANNEL_CAPTURE, mx35_spdif->mixer,
		ado_mixer_find_element (mx35_spdif->mixer,
			SND_MIXER_ETYPE_CAPTURE1, SND_MIXER_ELEMENT_CAPTURE, 0), 
		ado_mixer_find_group (mx35_spdif->mixer,SND_MIXER_GRP_IGAIN, 0));
#endif

	/* allow audio chip to support multiple simultaneous streams */
	if (ado_pcm_sw_mix (card, mx35_spdif->pcm1, mx35_spdif->mixer))
	{
		ado_error ("Unable to create a pcm sw mixer");
		mx35_spdif_fini (mx35_spdif);
		mx35_spdif->sdmafuncs.channel_release (mx35_spdif->play_strm.dma_chn);
	    my_detach_pulse (&mx35_spdif->cap_strm.pulse);
		my_detach_pulse (&mx35_spdif->play_strm.pulse);
		mx35_spdif->sdmafuncs.fini ();
		ado_mutex_destroy (&mx35_spdif->hw_lock);
		munmap_device_memory (mx35_spdif->spdif, sizeof (spdif_t));
		ado_free (mx35_spdif);
		return -1;
	}
	
    return 0;
}

ado_ctrl_dll_destroy_t ctrl_destroy;
int
ctrl_destroy (HW_CONTEXT_T * mx35_spdif)
{
	ado_debug (DB_LVL_DRIVER, "CTRL_DLL_DESTROY: MX35 SPDIF");

	mx35_spdif_fini (mx35_spdif);
	mx35_spdif->sdmafuncs.channel_release (mx35_spdif->play_strm.dma_chn);
    my_detach_pulse (&mx35_spdif->cap_strm.pulse);
	my_detach_pulse (&mx35_spdif->play_strm.pulse);
	mx35_spdif->sdmafuncs.fini ();
	ado_mutex_destroy (&mx35_spdif->hw_lock);
	munmap_device_memory (mx35_spdif->spdif, sizeof (spdif_t));
	ado_free (mx35_spdif);

	return 0;
}
