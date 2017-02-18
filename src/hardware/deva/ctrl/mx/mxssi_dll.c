/*
 * $QNXLicenseC: 
 * Copyright 2009, QNX Software Systems.  
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
 *
 *    mxssi_dll.c
 *      The primary interface into the mx DLL.
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

#include "mxssi.h"

int codec_mixer (ado_card_t * card, HW_CONTEXT_T * hwc);

int32_t
mx_capabilities (HW_CONTEXT_T * mx, snd_pcm_channel_info_t * info)
{
	int   chn_avail = 1;

	if (info->channel == SND_PCM_CHANNEL_PLAYBACK &&
		mx->play_strm.pcm_subchn)
		chn_avail = 0;
	else if (info->channel == SND_PCM_CHANNEL_CAPTURE &&
		mx->cap_strm.pcm_subchn)
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
mx_frag_dmabuf_sizer (HW_CONTEXT_T * mx, ado_pcm_config_t * config,
	ado_pcm_subchn_t * subchn)
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
mx_playback_aquire (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T ** pc,
	ado_pcm_config_t * config, ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
	dma_transfer_t tinfo;
	int     frag_idx;
	
	if (mx->play_strm.pcm_subchn)
	{
		*why_failed = SND_PCM_PARAMS_NO_CHANNEL;
		return (EAGAIN);
	}

	ado_mutex_lock (&mx->hw_lock);

	mx_frag_dmabuf_sizer (mx, config, subchn);

	if ((config->dmabuf.addr = ado_shm_alloc (config->dmabuf.size, config->dmabuf.name,
				ADO_SHM_DMA_SAFE, &config->dmabuf.phys_addr)) == NULL)
	{
		ado_mutex_unlock (&mx->hw_lock);
		return (errno);
	}

	memset (&tinfo, 0, sizeof (tinfo));
	tinfo.src_addrs = malloc (config->mode.block.frags_total * sizeof (dma_addr_t));
	for (frag_idx = 0; frag_idx < config->mode.block.frags_total; frag_idx++)
	{
		tinfo.src_addrs[frag_idx].paddr =
			config->dmabuf.phys_addr + (frag_idx * config->mode.block.frag_size);
		tinfo.src_addrs[frag_idx].len = config->mode.block.frag_size;
	}
	tinfo.src_fragments = config->mode.block.frags_total;
	tinfo.xfer_unit_size = 16;                             /* 16-bit samples */
	tinfo.xfer_bytes = config->dmabuf.size;

	mx->sdmafuncs.setup_xfer (mx->play_strm.dma_chn, &tinfo);
	free (tinfo.src_addrs);

	mx->play_strm.pcm_subchn = *pc = subchn;
	mx->play_strm.pcm_config = config;
	ado_mutex_unlock (&mx->hw_lock);
	return (EOK);
}


int32_t
mx_playback_release (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc,
	ado_pcm_config_t * config)
{
	ado_mutex_lock (&mx->hw_lock);
	mx->play_strm.pcm_subchn = NULL;
	ado_shm_free (config->dmabuf.addr, config->dmabuf.size, config->dmabuf.name);
	ado_mutex_unlock (&mx->hw_lock);
	return (0);
}

int32_t
mx_capture_aquire (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T ** pc,
	ado_pcm_config_t * config, ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
	dma_transfer_t tinfo;
	int     frag_idx;

	if (mx->cap_strm.pcm_subchn)
	{
		*why_failed = SND_PCM_PARAMS_NO_CHANNEL;
		return (EAGAIN);
	}

	ado_mutex_lock (&mx->hw_lock);

	mx_frag_dmabuf_sizer (mx, config, subchn);

	if ((config->dmabuf.addr = ado_shm_alloc (config->dmabuf.size, config->dmabuf.name,
				ADO_SHM_DMA_SAFE, &config->dmabuf.phys_addr)) == NULL)
	{
		ado_mutex_unlock (&mx->hw_lock);
		return (errno);
	}

	memset (&tinfo, 0, sizeof (tinfo));
	tinfo.dst_addrs = malloc (config->mode.block.frags_total * sizeof (dma_addr_t));
	for (frag_idx = 0; frag_idx < config->mode.block.frags_total; frag_idx++)
	{
	    tinfo.dst_addrs[frag_idx].paddr =
	        config->dmabuf.phys_addr + (frag_idx * config->mode.block.frag_size);
	    tinfo.dst_addrs[frag_idx].len = config->mode.block.frag_size;
	}
	tinfo.dst_fragments = config->mode.block.frags_total;
	tinfo.xfer_unit_size = 16;                             /* 16-bit samples */
	tinfo.xfer_bytes = config->dmabuf.size;

	mx->sdmafuncs.setup_xfer (mx->cap_strm.dma_chn, &tinfo);
	free (tinfo.dst_addrs);
	
	mx->cap_strm.pcm_subchn = *pc = subchn;
	mx->cap_strm.pcm_config = config;
	ado_mutex_unlock (&mx->hw_lock);
	return (EOK);
}

int32_t
mx_capture_release (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc,
	ado_pcm_config_t * config)
{
	ado_mutex_lock (&mx->hw_lock);
	mx->cap_strm.pcm_subchn = NULL;
	ado_shm_free (config->dmabuf.addr, config->dmabuf.size, config->dmabuf.name);
	ado_mutex_unlock (&mx->hw_lock);
	return (0);
}

int32_t
mx_prepare (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	return (0);
}

int32_t
mx_playback_trigger (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
	int rtn = EOK, i = 0;

	ado_mutex_lock (&mx->hw_lock);
	
	if (cmd == ADO_PCM_TRIGGER_GO)
	{
		if (pc == mx->play_strm.pcm_subchn)
		{ 
			mx->ssi->sor |= SOR_TX_CLR;	/* Flush TX FIFO */

			if (mx->sdmafuncs.xfer_start (mx->play_strm.dma_chn) == -1)
			{
				rtn = errno;
				ado_error ("MX SSI: Audio DMA Start failed (%s)", strerror(rtn));
			}

			/* Give DMA time to fill the FIFO */
			while ((SFCSR_TXFIFO0_CNT(mx->ssi->sfcsr) == 0) && (i<100))
			{
				i++;
				nanospin_ns(1000);
			}
			
			/* Report an error if DMA didn't fill the FIFO on time, but still keep running */
			if (i>=100) 
			{
				rtn = ETIME;
				ado_error ("MX SSI: Audio TX FIFO underrun");
			}

			/* Enable TX */
			mx->ssi->scr |= SCR_TX_EN;
		}
	}
	else
	{
		if (pc == mx->play_strm.pcm_subchn)
		{
			mx->ssi->scr &= ~(SCR_TX_EN);

			if (mx->sdmafuncs.xfer_abort (mx->play_strm.dma_chn) == -1)
			{
			    rtn = errno;
			    ado_error ("MX SSI: Audio DMA Stop failed (%s)", strerror(rtn));
			}
		}
	}
	ado_mutex_unlock (&mx->hw_lock);
	return (rtn);
}

int32_t
mx_capture_trigger (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
	int rtn = EOK;
	
	ado_mutex_lock (&mx->hw_lock);

	if (cmd == ADO_PCM_TRIGGER_GO)
	{
		if (pc == mx->cap_strm.pcm_subchn)
		{
			mx->ssi->sor |= SOR_RX_CLR;	/* Flush RX FIFO */

			if (mx->sdmafuncs.xfer_start (mx->cap_strm.dma_chn) == -1)
			{
				rtn = errno;
				ado_error ("MX SSI: Audio DMA Start failed (%s)", strerror(rtn));
			}

			mx->ssi->scr |= SCR_RX_EN;	/* Enable RX */
		}
	}
	else
	{
		if (pc == mx->cap_strm.pcm_subchn)
		{
			mx->ssi->scr &= ~(SCR_RX_EN);

			if (mx->sdmafuncs.xfer_abort (mx->cap_strm.dma_chn) == -1)
			{
				rtn = errno;
				ado_error ("MX SSI: Audio DMA Stop failed (%s)", strerror(rtn));
			}
		}
	}

	ado_mutex_unlock (&mx->hw_lock);
	
	return (rtn);
}

/*
 * No position function as we are unable to get the transfer count of the
 * current DMA operation from the SDMA microcode. The resolution of the
 * positional information returned to the client will be limited to the
 * fragment size.
 *
 * If we get new SDMA microcode that supports this and the dma library's
 * bytes_left() function is updated to use this info to return the actual
 * bytes left, uncomment the below function and the function pointer
 * assignments in ctrl_init().
 */
 
#if 0
uint32_t
mx_position (HW_CONTEXT_T * mx, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	uint32_t pos;

	ado_mutex_lock (&mx->hw_lock);

	if (pc == mx->play_strm.pcm_subchn)
	{
		pos =
			ado_pcm_dma_int_size (config) -
			mx->sdmafuncs.bytes_left (mx->play_strm.dma_chn);
	}
	else
	{
		pos =
			ado_pcm_dma_int_size (config) -
			mx->sdmafuncs.bytes_left (mx->cap_strm.dma_chn);
	}

	ado_mutex_unlock (&mx->hw_lock);

	return (pos);
}
#endif

void
mx_play_pulse_hdlr (HW_CONTEXT_T * mx, struct sigevent *event)
{
	if (mx->play_strm.pcm_subchn)
		dma_interrupt (mx->play_strm.pcm_subchn);
}

void
mx_cap_pulse_hdlr (HW_CONTEXT_T * mx, struct sigevent *event)
{
	if (mx->cap_strm.pcm_subchn)
		dma_interrupt (mx->cap_strm.pcm_subchn);
}

int
mx_ssi_init (HW_CONTEXT_T * mx)
{
	/* Disable SSI */
	mx->ssi->scr &= ~(SCR_SSI_EN | SCR_TX_EN | SCR_RX_EN);
	    
	/* Mask all the tx/rx time slots */
	mx->ssi->stmsk = STMSK_ALL;
	mx->ssi->srmsk = SRMSK_ALL;

	/* Disable all interrupts */
	mx->ssi->sier = 0x0;
    
	/* Disable Tx/Rx FIFOs */
	mx->ssi->stcr &= ~(STCR_TXFIFO0_EN | STCR_TXFIFO1_EN);
	mx->ssi->srcr &= ~(SRCR_RXFIFO0_EN | SRCR_RXFIFO1_EN);

	/* 
	 * Clock idle state is high, two channel mode disabled,
	 * Disable system clock (not needed as codec only needs
	 * frame sync and bit clock), Synchronous, I2S Slave mode,
	 * Clear network mode
	 */	
	mx->ssi->scr |= SCR_CLK_IST;
	mx->ssi->scr &= ~SCR_TCH_EN;
	mx->ssi->scr &= ~SCR_SYS_CLK_EN;
	mx->ssi->scr |= SCR_SYNC_MODE;
	mx->ssi->scr = (mx->ssi->scr & ~SCR_I2SMODE_MSK)
		| SCR_I2S_SLAVE;
	mx->ssi->scr &= ~SCR_NET_MODE;

	/*
	 * MSB Aligned, Clocks generated externally (slave mode),
	 * MSB first, Transmit data clocked at falling edge,
	 * Transmit frame sync active low, 1-word long frame sync,
	 * Early transmit frame sync
	 */ 
	mx->ssi->stcr &= ~STCR_TXBIT0;
	mx->ssi->stcr &= ~(STCR_TXFIFO0_EN | STCR_TXFIFO1_EN);
	mx->ssi->stcr &= ~(STCR_TFDIR_INTERNAL | STCR_TXDIR_INTERNAL);
	mx->ssi->stcr &= ~STCR_TSHFD_LSB;
	mx->ssi->stcr |= STCR_TSCKP_FE;
	mx->ssi->stcr |= STCR_TFSI_AL;
	mx->ssi->stcr &= ~STCR_TFSL_BIT;
	mx->ssi->stcr |= STCR_TEFS_EARLY;

	/* SSI is in sync mode so we don't need to configure clocks for receive */
	mx->ssi->srcr &= ~SRCR_RXBIT0;
	mx->ssi->srcr &= ~(SRCR_RXFIFO0_EN | SRCR_RXFIFO1_EN);
	mx->ssi->srcr &= ~SRCR_RSHFD_LSB;

	/* 
	 * Bypass precaler and divide by 2, 16-bit word length,
	 * 2 words per frame, Prescaler modulus = 0
	 */
	mx->ssi->stccr &= ~(STCCR_DIV2 | STCCR_PSR);
	mx->ssi->stccr = (mx->ssi->stccr & ~STCCR_WL_MSK) |
		STCCR_WL_16BIT;
	mx->ssi->stccr = (mx->ssi->stccr & ~STCCR_DC_MSK) | STCCR_DC_2W;
	mx->ssi->stccr &= ~STCCR_PM_MSK;

	/* Set fifo water mark */
	mx->ssi->sfcsr = (mx->ssi->sfcsr & ~SFCSR_TFWM0_MSK) | FIFO_WATERMARK;
	mx->ssi->sfcsr = (mx->ssi->sfcsr & ~SFCSR_RFWM0_MSK) | (FIFO_WATERMARK << 4);

	/* Enable DMA TX Event */
	mx->ssi->sier |= SIER_TDMAE;

	/* Enable DMA Event */
	mx->ssi->sier |= SIER_RDMAE;

	/* Enable TX FIFO */
	mx->ssi->stcr |= STCR_TXFIFO0_EN;

	/* Enable RX FIFO */
	mx->ssi->srcr |= SRCR_RXFIFO0_EN;

	/* Enable/unmask first transmit time slot */
	mx->ssi->stmsk &= ~(STMSK_SLOT0);
	mx->ssi->srmsk &= ~(SRMSK_SLOT0);

	/* Enable SSI */
	mx->ssi->scr |= SCR_SSI_EN;

	return 0;
}


ado_dll_version_t ctrl_version;
void
ctrl_version (int *major, int *minor, char *date)
{
	*major = ADO_MAJOR_VERSION;
	*minor = 1;
	date = __DATE__;
}

/*
ssibase = [#]; base address of ssi controller
tevt    = [#]; ssi TX DMA event number
tchn    = [#]; ssi TX channel number
revt    = [#]; ssi RX DMA event number
rchn    = [#]; ssi RX channel number
rate    = [#]; sample rate of audio
mixer = [info:[mixer option1]:[mixer options2][:[other options]]]
   mixer=info to dump the details of mixer options
*/

static char *
mx_opts[] = {
#define OPT0 0
#define OPT1 1
#define OPT2 2
#define OPT3 3
#define OPT4 4
#define OPT5 5
#define OPT6 6
	"ssibase",	/* OPT0 */
	"tevt",		/* OPT1 */
	"tchn",		/* OPT2 */
	"revt",		/* OPT3 */
	"rchn",		/* OPT4 */
	"rate",		/* OPT5 */
	"mixer",	/* OPT6 */
	NULL
};

static void 
build_dma_string (char * dmastring, uint32_t fifopaddr, 
		int dmaevent, int watermark)
{
	char str[50];
	
	strcpy (dmastring, "eventnum=");
	strcat (dmastring, itoa (dmaevent, str, 10));
	strcat (dmastring, ",watermark=");
	strcat (dmastring, itoa (watermark, str, 10));
	strcat (dmastring, ",fifopaddr=0x");
	strcat (dmastring, ultoa (fifopaddr, str, 16));
	strcat (dmastring, ",regen,contloop");
}

static int
get_sndpcmrate (int samplerate)
{
	switch (samplerate)
	{
		case 44100: 
			return (SND_PCM_RATE_44100);
		case 48000:
			return (SND_PCM_RATE_48000);
		default:
			ado_error ("MX SSI: Unsupport sample rate %d\n", samplerate);
			ado_error ("MX SSI: Only support 48000 or 44100\n");
			return (-1);
	}
}

ado_ctrl_dll_init_t ctrl_init;
int
ctrl_init (HW_CONTEXT_T ** hw_context, ado_card_t * card, char *args)
{
	mx_t    * mx;
	char    * value;
	char    str[100];

	ado_debug (DB_LVL_DRIVER, "CTRL_DLL_INIT: MX");

	if ((mx = (mx_t *) ado_calloc (1, sizeof (mx_t))) == NULL)
	{
		ado_error ("MX SSI: Unable to allocate memory for mx (%s)",
			strerror (errno));
		return -1;
	}

	mx->samplerate = 0;
	mx->pcmrate = 0;
	strcpy (mx->mixeropts, "");

	while (*args != '\0')
	{
		switch (getsubopt (&args, mx_opts, &value)) {
			case OPT0:
				mx->ssibase = strtoul (value, NULL, 0);
				break;
			case OPT1:
				mx->tevt = strtol (value, NULL, 0);
				break;
			case OPT2:
				mx->tchn = strtol (value, NULL, 0);
				break;
			case OPT3:
				mx->revt = strtol (value, NULL, 0);
				break;
			case OPT4:
				mx->rchn = strtol (value, NULL, 0);
				break;
			case OPT5:
				mx->samplerate = strtol (value, NULL, 0);
				break;
			case OPT6:
				if (strlen (value) > MAX_MIXEROPT)
				{
					ado_error ("MX SSI: Board specific options pass maximum len %d", 
							MAX_MIXEROPT);
					ado_free (mx);
					return (-1);
				}
				strncat (mx->mixeropts, value, MAX_MIXEROPT);
				break;
			default:
				ado_error ("MX SSI: Unrecognized option \"%s\"", value);
				break;
		}
	}

	if ((mx->pcmrate=get_sndpcmrate(mx->samplerate))== (-1))
	{
		ado_free (mx);
		return (-1);
	}

	if (strcmp (mx->mixeropts, "") != 0)
		strncat (mx->mixeropts, ":", MAX_MIXEROPT);
	strncat (mx->mixeropts, "samplerate=", MAX_MIXEROPT);
	strncat (mx->mixeropts, itoa (mx->samplerate, &str[0], 10), MAX_MIXEROPT);

	*hw_context = mx;
	ado_card_set_shortname (card, "MX");
	ado_card_set_longname (card, "Freescale i.MX", 0);

	mx->ssi = mmap_device_memory (0, sizeof (ssi_t),
		PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, mx->ssibase);
	if (mx->ssi == MAP_FAILED)
	{
		ado_error ("MX SSI: Unable to mmap SSI (%s)", strerror (errno));
		ado_free (mx);
		return -1;
	}
	
	ado_mutex_init (&mx->hw_lock);

	if (get_dmafuncs (&mx->sdmafuncs, sizeof (dma_functions_t)) == -1)
	{
		ado_error ("MX SSI: Failed to get DMA lib functions");
		ado_mutex_destroy (&mx->hw_lock);
		munmap_device_memory (mx->ssi, sizeof (ssi_t));
		ado_free (mx);
		return -1;
	}

	my_attach_pulse (&mx->play_strm.pulse,
		&mx->play_strm.sdma_event, mx_play_pulse_hdlr, mx);
	my_attach_pulse (&mx->cap_strm.pulse,
		&mx->cap_strm.sdma_event, mx_cap_pulse_hdlr, mx);

	if (mx->sdmafuncs.init (NULL) == -1)
	{
		ado_error ("MX SSI: DMA init failed");
		my_detach_pulse (&mx->cap_strm.pulse);
		my_detach_pulse (&mx->play_strm.pulse);
		ado_mutex_destroy (&mx->hw_lock);
		munmap_device_memory (mx->ssi, sizeof (ssi_t));
		ado_free (mx);
		return -1;
	}

	/*
	 * DMA channel setup for Playback
	 * 1) watermark = must match the TX FIFO watermark in SSI
	 * 2) eventnum = SSI TX0 DMA event
	 * 3) fifopaddr = Physical address of SSI TX0 FIFO
	 * 4) regen,contloop = DMA in repeat/loop mode so we only need to configure
	 *    the DMA transfer on channel acquire and not on every interrupt.
	 */
	build_dma_string (str, mphys ((void *)&(mx->ssi->stx0)), mx->tevt, FIFO_WATERMARK);

	mx->play_strm.dma_chn =
		mx->sdmafuncs.channel_attach(str, &mx->play_strm.sdma_event, &mx->tchn,
			DMA_ATTACH_PRIORITY_HIGHEST, DMA_ATTACH_EVENT_PER_SEGMENT);

	if (mx->play_strm.dma_chn == NULL)
	{
		ado_error ("MX SSI: SDMA playback channel attach failed");
		my_detach_pulse (&mx->cap_strm.pulse);
		my_detach_pulse (&mx->play_strm.pulse);
		mx->sdmafuncs.fini ();
		ado_mutex_destroy (&mx->hw_lock);
		munmap_device_memory (mx->ssi, sizeof (ssi_t));
		ado_free (mx);
		return -1;
	}
	
	/*
	 * DMA channel setup for Capture
	 * 1) watermark = must match the RX FIFO watermark in SSI
	 * 2) eventnum = SSI RX0 DMA event
	 * 3) fifopaddr = Physical address of SSI RX0 FIFO
	 * 4) regen,contloop = DMA in repeat/loop mode so we only need to configure
	 *    the DMA transfer on channel acquire and not on every interrupt.
	 */
	build_dma_string (str, mphys ((void *)&(mx->ssi->srx0)), mx->revt, FIFO_WATERMARK);

	mx->cap_strm.dma_chn =
			mx->sdmafuncs.channel_attach(str, &mx->cap_strm.sdma_event, &mx->rchn,
			DMA_ATTACH_PRIORITY_HIGHEST, DMA_ATTACH_EVENT_PER_SEGMENT);

	if (mx->cap_strm.dma_chn == NULL)
	{
		ado_error ("MX SSI: SDMA capture channel attach failed");
		mx->sdmafuncs.channel_release (mx->play_strm.dma_chn);
		my_detach_pulse (&mx->cap_strm.pulse);
		my_detach_pulse (&mx->play_strm.pulse);
		mx->sdmafuncs.fini ();
		ado_mutex_destroy (&mx->hw_lock);
		munmap_device_memory (mx->ssi, sizeof (ssi_t));
		ado_free (mx);
		return -1;
	}

	mx_ssi_init (mx);

	mx->play_strm.pcm_caps.chn_flags =
		SND_PCM_CHNINFO_BLOCK | SND_PCM_CHNINFO_STREAM |
		SND_PCM_CHNINFO_INTERLEAVE | SND_PCM_CHNINFO_BLOCK_TRANSFER |
		SND_PCM_CHNINFO_MMAP | SND_PCM_CHNINFO_MMAP_VALID;
	mx->play_strm.pcm_caps.formats = SND_PCM_FMT_S16_LE;
	mx->play_strm.pcm_caps.rates = mx->pcmrate;
	mx->play_strm.pcm_caps.min_rate = mx->samplerate;
	mx->play_strm.pcm_caps.max_rate = mx->samplerate;
	mx->play_strm.pcm_caps.min_voices = 2;
	mx->play_strm.pcm_caps.max_voices = 2;
	mx->play_strm.pcm_caps.min_fragsize = 64;
	mx->play_strm.pcm_caps.max_fragsize = 32 * 1024;
	
	memcpy (&mx->cap_strm.pcm_caps, &mx->play_strm.pcm_caps,
		sizeof (mx->cap_strm.pcm_caps));

	mx->play_strm.pcm_funcs.capabilities = mx_capabilities;
	mx->play_strm.pcm_funcs.aquire = mx_playback_aquire;
	mx->play_strm.pcm_funcs.release = mx_playback_release;
	mx->play_strm.pcm_funcs.prepare = mx_prepare;
	mx->play_strm.pcm_funcs.trigger = mx_playback_trigger;
#if 0
	mx->play_strm.pcm_funcs.position = mx_position;
#endif

	mx->cap_strm.pcm_funcs.capabilities = mx_capabilities;
	mx->cap_strm.pcm_funcs.aquire = mx_capture_aquire;
	mx->cap_strm.pcm_funcs.release = mx_capture_release;
	mx->cap_strm.pcm_funcs.prepare = mx_prepare;
	mx->cap_strm.pcm_funcs.trigger = mx_capture_trigger;
#if 0
	mx->cap_strm.pcm_funcs.position = mx_position;
#endif

	if (ado_pcm_create (card, "mx PCM 0", 0, "mx-0",
			1, &mx->play_strm.pcm_caps, &mx->play_strm.pcm_funcs,
			1, &mx->cap_strm.pcm_caps, &mx->cap_strm.pcm_funcs,
			&mx->pcm1))
	{
		ado_error ("MX SSI: Unable to create pcm devices (%s)", strerror (errno));
		mx->sdmafuncs.channel_release (mx->cap_strm.dma_chn);
		mx->sdmafuncs.channel_release (mx->play_strm.dma_chn);
		my_detach_pulse (&mx->cap_strm.pulse);
		my_detach_pulse (&mx->play_strm.pulse);
		mx->sdmafuncs.fini ();
		ado_mutex_destroy (&mx->hw_lock);
		munmap_device_memory (mx->ssi, sizeof (ssi_t));
		ado_free (mx);
		return -1;
	}

	if (codec_mixer (card, mx))
	{
		ado_error ("MX SSI: Unable to create codec mixer");
		mx->sdmafuncs.channel_release (mx->cap_strm.dma_chn);
		mx->sdmafuncs.channel_release (mx->play_strm.dma_chn);
		my_detach_pulse (&mx->cap_strm.pulse);
		my_detach_pulse (&mx->play_strm.pulse);
		mx->sdmafuncs.fini ();
		ado_mutex_destroy (&mx->hw_lock);
		munmap_device_memory (mx->ssi, sizeof (ssi_t));
		ado_free (mx);
		return -1;
	}

    return 0;
}

ado_ctrl_dll_destroy_t ctrl_destroy;
int
ctrl_destroy (HW_CONTEXT_T * mx)
{
	ado_debug (DB_LVL_DRIVER, "CTRL_DLL_DESTROY: MX");
	mx->sdmafuncs.channel_release (mx->cap_strm.dma_chn);
	mx->sdmafuncs.channel_release (mx->play_strm.dma_chn);
	my_detach_pulse (&mx->cap_strm.pulse);
	my_detach_pulse (&mx->play_strm.pulse);
	mx->sdmafuncs.fini ();
	ado_mutex_destroy (&mx->hw_lock);
	munmap_device_memory (mx->ssi, sizeof (ssi_t));
	ado_free (mx);

	return 0;
}

