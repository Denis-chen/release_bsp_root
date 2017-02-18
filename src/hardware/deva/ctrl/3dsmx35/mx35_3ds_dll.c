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
 *
 *    mx35_3ds_dll.c
 *      The primary interface into the mx35_3ds DLL.
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

#include "mx35_3ds.h"
#include "ak4647.h"

int32_t
mx35_3ds_capabilities (HW_CONTEXT_T * mx35_3ds, snd_pcm_channel_info_t * info)
{
	int   chn_avail = 1;

	if (info->channel == SND_PCM_CHANNEL_PLAYBACK &&
		mx35_3ds->play_strm.pcm_subchn)
		chn_avail = 0;
	else if (info->channel == SND_PCM_CHANNEL_CAPTURE &&
		mx35_3ds->cap_strm.pcm_subchn)
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
mx35_3ds_frag_dmabuf_sizer (HW_CONTEXT_T * mx35_3ds, ado_pcm_config_t * config,
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
mx35_3ds_playback_aquire (HW_CONTEXT_T * mx35_3ds, PCM_SUBCHN_CONTEXT_T ** pc,
	ado_pcm_config_t * config, ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
	dma_transfer_t tinfo;
	int     frag_idx;
	
	if (mx35_3ds->play_strm.pcm_subchn)
	{
		*why_failed = SND_PCM_PARAMS_NO_CHANNEL;
		return (EAGAIN);
	}

	ado_mutex_lock (&mx35_3ds->hw_lock);

	mx35_3ds_frag_dmabuf_sizer (mx35_3ds, config, subchn);

	if ((config->dmabuf.addr = ado_shm_alloc (config->dmabuf.size, config->dmabuf.name,
				ADO_SHM_DMA_SAFE, &config->dmabuf.phys_addr)) == NULL)
	{
		ado_mutex_unlock (&mx35_3ds->hw_lock);
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

	mx35_3ds->sdmafuncs.setup_xfer (mx35_3ds->play_strm.dma_chn, &tinfo);
	free (tinfo.src_addrs);

	mx35_3ds->play_strm.pcm_subchn = *pc = subchn;
	mx35_3ds->play_strm.pcm_config = config;
	ado_mutex_unlock (&mx35_3ds->hw_lock);
	return (EOK);
}


int32_t
mx35_3ds_playback_release (HW_CONTEXT_T * mx35_3ds, PCM_SUBCHN_CONTEXT_T * pc,
	ado_pcm_config_t * config)
{
	ado_mutex_lock (&mx35_3ds->hw_lock);
	mx35_3ds->play_strm.pcm_subchn = NULL;
	ado_shm_free (config->dmabuf.addr, config->dmabuf.size, config->dmabuf.name);
	ado_mutex_unlock (&mx35_3ds->hw_lock);
	return (0);
}

int32_t
mx35_3ds_capture_aquire (HW_CONTEXT_T * mx35_3ds, PCM_SUBCHN_CONTEXT_T ** pc,
	ado_pcm_config_t * config, ado_pcm_subchn_t * subchn, uint32_t * why_failed)
{
	dma_transfer_t tinfo;
	int     frag_idx;

	if (mx35_3ds->cap_strm.pcm_subchn)
	{
		*why_failed = SND_PCM_PARAMS_NO_CHANNEL;
		return (EAGAIN);
	}

	ado_mutex_lock (&mx35_3ds->hw_lock);

	mx35_3ds_frag_dmabuf_sizer (mx35_3ds, config, subchn);

	if ((config->dmabuf.addr = ado_shm_alloc (config->dmabuf.size, config->dmabuf.name,
				ADO_SHM_DMA_SAFE, &config->dmabuf.phys_addr)) == NULL)
	{
		ado_mutex_unlock (&mx35_3ds->hw_lock);
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

	mx35_3ds->sdmafuncs.setup_xfer (mx35_3ds->cap_strm.dma_chn, &tinfo);
	free (tinfo.dst_addrs);
	
	mx35_3ds->cap_strm.pcm_subchn = *pc = subchn;
	mx35_3ds->cap_strm.pcm_config = config;
	ado_mutex_unlock (&mx35_3ds->hw_lock);
	return (EOK);
}

int32_t
mx35_3ds_capture_release (HW_CONTEXT_T * mx35_3ds, PCM_SUBCHN_CONTEXT_T * pc,
	ado_pcm_config_t * config)
{
	ado_mutex_lock (&mx35_3ds->hw_lock);
	mx35_3ds->cap_strm.pcm_subchn = NULL;
	ado_shm_free (config->dmabuf.addr, config->dmabuf.size, config->dmabuf.name);
	ado_mutex_unlock (&mx35_3ds->hw_lock);
	return (0);
}

int32_t
mx35_3ds_prepare (HW_CONTEXT_T * mx35_3ds, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	return (0);
}

int32_t
mx35_3ds_playback_trigger (HW_CONTEXT_T * mx35_3ds, PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
	int rtn = EOK, i = 0;

	ado_mutex_lock (&mx35_3ds->hw_lock);
	
	if (cmd == ADO_PCM_TRIGGER_GO)
	{
		if (pc == mx35_3ds->play_strm.pcm_subchn)
		{ 

			if (mx35_3ds->sdmafuncs.xfer_start (mx35_3ds->play_strm.dma_chn) == -1)
			{
			    rtn = errno;
				ado_error ("3DS MX35 Audio DMA Start failed (%s)", strerror(rtn));
			}

			mx35_3ds->ssi->stmsk &= ~(STMSK_SLOT0 | STMSK_SLOT1); /* Enable/unmask first two transmit time slots (stereo) */
			mx35_3ds->ssi->stcr |= STCR_TXFIFO0_EN;
			mx35_3ds->ssi->sor |= SOR_TX_CLR;	/* Flush TX FIFO */
			mx35_3ds->ssi->sier |= SIER_TDMAE;   /* Enable DMA Event */

			/* Give DMA time to fill the FIFO */
			while ((SFCSR_TXFIFO0_CNT(mx35_3ds->ssi->sfcsr) ==0) && (i<100))
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
			mx35_3ds->ssi->scr |= SCR_TX_EN;
		}
	}
	else
	{
		if (pc == mx35_3ds->play_strm.pcm_subchn)
		{
			mx35_3ds->ssi->scr &= ~(SCR_TX_EN);
			mx35_3ds->ssi->stcr &= ~(STCR_TXFIFO0_EN);
			mx35_3ds->ssi->stmsk |= STMSK_SLOT0 | STMSK_SLOT1;	
			mx35_3ds->ssi->sier &= ~SIER_TDMAE;  /* Disable DMA Event */

			if (mx35_3ds->sdmafuncs.xfer_abort (mx35_3ds->play_strm.dma_chn) == -1)
			{
			    rtn = errno;
			    ado_error ("3DS MX35 Audio DMA Stop failed (%s)", strerror(rtn));
			}
		}
	}

	ado_mutex_unlock (&mx35_3ds->hw_lock);

	return (rtn);
}

int32_t
mx35_3ds_capture_trigger (HW_CONTEXT_T * mx35_3ds, PCM_SUBCHN_CONTEXT_T * pc, uint32_t cmd)
{
	int rtn = EOK;
	
	ado_mutex_lock (&mx35_3ds->hw_lock);

	if (cmd == ADO_PCM_TRIGGER_GO)
	{
		if (pc == mx35_3ds->cap_strm.pcm_subchn)
		{
			if (mx35_3ds->sdmafuncs.xfer_start (mx35_3ds->cap_strm.dma_chn) == -1)
			{
			    rtn = errno;
				ado_error ("3DS MX35 Audio DMA Start failed (%s)", strerror(rtn));
			}

			mx35_3ds->ssi->srmsk &= ~(SRMSK_SLOT0 | SRMSK_SLOT1);	
			mx35_3ds->ssi->srcr |= SRCR_RXFIFO0_EN;
			mx35_3ds->ssi->sor |= SOR_RX_CLR;	/* Flush RX FIFO */

			mx35_3ds->ssi->scr |= SCR_RX_EN;

			mx35_3ds->ssi->sier |= SIER_RDMAE;   /* Enable DMA Event */
		}
	}
	else
	{
		if (pc == mx35_3ds->cap_strm.pcm_subchn)
		{
			mx35_3ds->ssi->scr &= ~(SCR_RX_EN);
			mx35_3ds->ssi->srcr &= ~(SRCR_RXFIFO0_EN);
			mx35_3ds->ssi->srmsk |= SRMSK_SLOT0 | SRMSK_SLOT1;	/* Mask receive time slots */

			mx35_3ds->ssi->sier &= ~SIER_RDMAE;  /* Disable DMA Event */
			if (mx35_3ds->sdmafuncs.xfer_abort (mx35_3ds->cap_strm.dma_chn) == -1)
			{
			    rtn = errno;
			    ado_error ("3DS MX35 Audio DMA Stop failed (%s)", strerror(rtn));
			}
		}
	}

	ado_mutex_unlock (&mx35_3ds->hw_lock);
	
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
mx35_3ds_position (HW_CONTEXT_T * mx35_3ds, PCM_SUBCHN_CONTEXT_T * pc, ado_pcm_config_t * config)
{
	uint32_t pos;

	ado_mutex_lock (&mx35_3ds->hw_lock);

	if (pc == mx35_3ds->play_strm.pcm_subchn)
	{
		pos =
			ado_pcm_dma_int_size (config) -
			mx35_3ds->sdmafuncs.bytes_left (mx35_3ds->play_strm.dma_chn);
	}
	else
	{
		pos =
			ado_pcm_dma_int_size (config) -
			mx35_3ds->sdmafuncs.bytes_left (mx35_3ds->cap_strm.dma_chn);
	}

	ado_mutex_unlock (&mx35_3ds->hw_lock);

	return (pos);
}
#endif

void
mx35_3ds_play_pulse_hdlr (HW_CONTEXT_T * mx35_3ds, struct sigevent *event)
{
	if (mx35_3ds->play_strm.pcm_subchn)
		dma_interrupt (mx35_3ds->play_strm.pcm_subchn);
}

void
mx35_3ds_cap_pulse_hdlr (HW_CONTEXT_T * mx35_3ds, struct sigevent *event)
{
	if (mx35_3ds->cap_strm.pcm_subchn)
		dma_interrupt (mx35_3ds->cap_strm.pcm_subchn);
}

int
setCodecPowerMode (HW_CONTEXT_T * mx35_3ds, int mode)
{
	uint8_t reg_val = 0;
	
	switch (mode)
	{
	case POWER_UP:
	    /* Master mode */
		mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_2, 0x08);
		/* MCKI = 12 Mhz, BCKO = 32fs, I2S aligned */
		mx35_3ds_i2c_write (mx35_3ds, MODE_CONTROL_1, 0x63);
		/* Sampling Frequency as 44.1 khz */
		mx35_3ds_i2c_write (mx35_3ds, MODE_CONTROL_2, 0x27);
		/* VCOM power management */
		mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_1, 0x40);
		/* PLL and power up */
		mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_2, 0x09);

		delay (50);

		/* Mic Gain off */
		reg_val = mx35_3ds_i2c_read (mx35_3ds, SIGNAL_SLCT_1);
		reg_val &= ~0x01;
		mx35_3ds_i2c_write (mx35_3ds, SIGNAL_SLCT_1, reg_val);

		/* program ALC registers */
		mx35_3ds_i2c_write (mx35_3ds, TIMER_SELECT, 0x3C);
		mx35_3ds_i2c_write (mx35_3ds, ALC_MOD_CTRL_2, 0xE1);
		mx35_3ds_i2c_write (mx35_3ds, ALC_MOD_CTRL_3, 0x00);
		mx35_3ds_i2c_write (mx35_3ds, ALC_MOD_CTRL_1, 0x21);

		/* power up ADL and ADR */
    	reg_val = mx35_3ds_i2c_read (mx35_3ds, POWER_MNGMT_1); 
    	reg_val |= 0x01;
    	mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_1, reg_val); 
    	reg_val = mx35_3ds_i2c_read (mx35_3ds, POWER_MNGMT_3); 
    	reg_val |= 0x01;
    	mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_3, reg_val); 
		delay (250);

		/* route DAC -> Headphone-Amp */
		reg_val = mx35_3ds_i2c_read (mx35_3ds, MODE_CONTROL_4);
		reg_val |= 0x01;
		mx35_3ds_i2c_write (mx35_3ds, MODE_CONTROL_4, reg_val);

		/* setup input digital volume (0dB) */
		mx35_3ds_i2c_write (mx35_3ds, LCH_IP_VOL_CTRL, 0x91);
		mx35_3ds_i2c_write (mx35_3ds, RCH_IP_VOL_CTRL, 0x91);

		/* independent volume controls for Headphone Out */
    	reg_val = mx35_3ds_i2c_read (mx35_3ds, MODE_CONTROL_3);
    	reg_val &= ~0x10;
		mx35_3ds_i2c_write (mx35_3ds, MODE_CONTROL_3, reg_val);

		/* power up DAC and MIN-Amp */
    	reg_val = mx35_3ds_i2c_read (mx35_3ds, POWER_MNGMT_1); 
    	reg_val |= 0x24;
    	mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_1, reg_val); 
		delay (30);

		/* power up Headphone-Amp */
    	reg_val = mx35_3ds_i2c_read (mx35_3ds, POWER_MNGMT_2); 
    	reg_val |= 0x30;
    	mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_2, reg_val); 

		/* rise up common voltage of Headphone-Amp */
    	reg_val = mx35_3ds_i2c_read (mx35_3ds, POWER_MNGMT_2); 
    	reg_val |= 0x40;
    	mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_2, reg_val);
    	delay (250);

		mx35_3ds->power_state = POWER_UP;
		break;

	case POWER_DOWN:
		/* fall down common voltage of Headphone-Amp */
    	reg_val = mx35_3ds_i2c_read (mx35_3ds, POWER_MNGMT_2); 
    	reg_val &= ~0x40;
    	mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_2, reg_val);
    	delay (250);

		/* power down Headphone-Amp */
    	reg_val = mx35_3ds_i2c_read (mx35_3ds, POWER_MNGMT_2); 
    	reg_val &= ~0x30;
    	mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_2, reg_val); 

		/* power down DAC and MIN-Amp */
    	reg_val = mx35_3ds_i2c_read (mx35_3ds, POWER_MNGMT_1); 
    	reg_val &= ~0x24;
    	mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_1, reg_val); 

		/* disable DAC -> Headphone-Amp */
		reg_val = mx35_3ds_i2c_read (mx35_3ds, MODE_CONTROL_4);
		reg_val &= ~0x01;
		mx35_3ds_i2c_write (mx35_3ds, MODE_CONTROL_4, reg_val);

		/* power down ADC */
    	reg_val = mx35_3ds_i2c_read (mx35_3ds, POWER_MNGMT_3); 
    	reg_val &= ~0x01;
    	mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_3, reg_val); 
    	reg_val = mx35_3ds_i2c_read (mx35_3ds, POWER_MNGMT_1); 
    	reg_val &= ~0x01;
    	mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_1, reg_val); 

	    /* disable MCKO and PLL */
		mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_2, 0x08);
		/* power down VCOM */
		mx35_3ds_i2c_write (mx35_3ds, POWER_MNGMT_1, 0x00);

		mx35_3ds->power_state = POWER_DOWN;
		break;

	default:
		ado_error ("Invalid mode");
		return -1;
		break;
	}

	return 0;
}

int
ssi_init (HW_CONTEXT_T * mx35_3ds, ccm_t * ccm)
{
    /* Disable SSI */
	mx35_3ds->ssi->scr &= ~(SCR_SSI_EN | SCR_TX_EN | SCR_RX_EN);
	    
    /* Mask all the tx/rx time slots */
	mx35_3ds->ssi->stmsk = STMSK_ALL;
	mx35_3ds->ssi->srmsk = SRMSK_ALL;
	
	/* Enable clock to SSI1 and SSI2 */
	ccm->cgr2 = (ccm->cgr2 & ~CGR2_SSI1_MSK) | CGR2_SSI1_EN;
	ccm->cgr2 = (ccm->cgr2 & ~CGR2_SSI2_MSK) | CGR2_SSI2_EN;
	
	/* Disable all interrupts */
	mx35_3ds->ssi->sier = 0x0;
    
    /* Disable Tx/Rx FIFOs */
	mx35_3ds->ssi->stcr &= ~(STCR_TXFIFO0_EN | STCR_TXFIFO1_EN);
	mx35_3ds->ssi->srcr &= ~(SRCR_RXFIFO0_EN | SRCR_RXFIFO1_EN);

	/* Clock idle state is high, two channel mode disabled,
	 * Disable system clock (not needed as codec only needs
	 * frame sync and bit clock), Synchronous, I2S Slave mode,
	 * Clear network mode
	 */	
	mx35_3ds->ssi->scr |= SCR_CLK_IST;
	mx35_3ds->ssi->scr &= ~SCR_TCH_EN;
	mx35_3ds->ssi->scr &= ~SCR_SYS_CLK_EN;
    mx35_3ds->ssi->scr |= SCR_SYNC_MODE;
	mx35_3ds->ssi->scr = (mx35_3ds->ssi->scr & ~SCR_I2SMODE_MSK)
		| SCR_I2S_SLAVE;
	mx35_3ds->ssi->scr &= ~SCR_NET_MODE;

	/*
	 * MSB Aligned, Clocks generated externally (slave mode),
	 * MSB first, Transmit data clocked at falling edge,
	 * Transmit frame sync active low, 1-word long frame sync,
	 * Early transmit frame sync
	 */ 
	mx35_3ds->ssi->stcr &= ~STCR_TXBIT0;
	mx35_3ds->ssi->stcr &= ~(STCR_TXFIFO0_EN | STCR_TXFIFO1_EN);
	mx35_3ds->ssi->stcr &= ~(STCR_TFDIR_INTERNAL | STCR_TXDIR_INTERNAL);
	mx35_3ds->ssi->stcr &= ~STCR_TSHFD_LSB;
	mx35_3ds->ssi->stcr |= STCR_TSCKP_FE;
	mx35_3ds->ssi->stcr |= STCR_TFSI_AL;
	mx35_3ds->ssi->stcr &= ~STCR_TFSL_BIT;
	mx35_3ds->ssi->stcr |= STCR_TEFS_EARLY;

	/* SSI is in sync mode so we don't need to configure clocks for receive */
	mx35_3ds->ssi->srcr &= ~SRCR_RXBIT0;
	mx35_3ds->ssi->srcr &= ~(SRCR_RXFIFO0_EN | SRCR_RXFIFO1_EN);
	mx35_3ds->ssi->srcr &= ~SRCR_RSHFD_LSB;

	/* Bypass precaler and divide by 2, 16-bit word length,
	 * 2 words per frame, Prescaler modulus = 0
	 */
	mx35_3ds->ssi->stccr &= ~(STCCR_DIV2 | STCCR_PSR);
	mx35_3ds->ssi->stccr = (mx35_3ds->ssi->stccr & ~STCCR_WL_MSK) |
		STCCR_WL_16BIT;
	mx35_3ds->ssi->stccr = (mx35_3ds->ssi->stccr & ~STCCR_DC_MSK) | STCCR_DC_2W;
	mx35_3ds->ssi->stccr &= ~STCCR_PM_MSK;

	/* set fifo water mark */
	mx35_3ds->ssi->sfcsr = (mx35_3ds->ssi->sfcsr & ~SFCSR_TFWM0_MSK) | 0x4;
	mx35_3ds->ssi->sfcsr = (mx35_3ds->ssi->sfcsr & ~SFCSR_RFWM0_MSK)
		| (0x4 << 4);

	/* enable SSI */
	mx35_3ds->ssi->scr |= SCR_SSI_EN;

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


static char *mx35_3ds_opts[] = {
#define SSI_IF	0
	"ssi",
	NULL
};

ado_ctrl_dll_init_t ctrl_init;
int
ctrl_init (HW_CONTEXT_T ** hw_context, ado_card_t * card, char *args)
{
	mx35_3ds_t *mx35_3ds;
	char *value;
	int mx35_ssi = 1;	/* default to SSI1 */
	unsigned int channel;
	ccm_t  *ccm;

	ado_debug (DB_LVL_DRIVER, "CTRL_DLL_INIT: 3DS MX35");

	while (*args != '\0')
	{
		switch (getsubopt (&args, mx35_3ds_opts, &value)) {
			case SSI_IF:
				if (value == NULL)
				{
					ado_error ("Driver option \"%s\" requires a value",
						mx35_3ds_opts[SSI_IF]);
					return -1;
				}
				mx35_ssi = strtoul (value, NULL, 0);
				if (mx35_ssi != 1 && mx35_ssi != 2)
				{
					ado_error ("Incorrect value for option \"%s\"",
						mx35_3ds_opts[SSI_IF]);
					return -1;
				}
				break;
			default:
				ado_error ("Unrecognized option \"%s\"", value);
				break;
		}
	}

	if ((mx35_3ds = (mx35_3ds_t *) ado_calloc (1, sizeof (mx35_3ds_t))) == NULL)
	{
		ado_error ("Unable to allocate memory for mx35_3ds (%s)",
			strerror (errno));
		return -1;
	}

	*hw_context = mx35_3ds;
	ado_card_set_shortname (card, "AK4647");
	ado_card_set_longname (card, "ASAHI KASEI AKM4647", 0);

	switch (mx35_ssi)
	{
		case 1:
			mx35_3ds->ssi_base = SSI1_BASE;
			break;
		case 2:
			mx35_3ds->ssi_base = SSI2_BASE;
			break;
	}

	mx35_3ds->ssi = mmap_device_memory (0, sizeof (ssi_t),
		PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, mx35_3ds->ssi_base);
	if (mx35_3ds->ssi == MAP_FAILED)
	{
		ado_error ("Unable to mmap SSI (%s)", strerror (errno));
		munmap_device_memory (mx35_3ds->audmux, sizeof (audmux_t));
		ado_free (mx35_3ds);
		return -1;
	}
	
	mx35_3ds->audmux = mmap_device_memory (0, sizeof (audmux_t),
		PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, AUDMUX_BASE);
	if (mx35_3ds->audmux == MAP_FAILED)
	{
		ado_error ("Unable to mmap AUDMUX (%s)", strerror (errno));
		ado_free (mx35_3ds);
		return -1;
	}

	ccm = mmap_device_memory (0, sizeof (ccm_t),
		PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, CCM_BASE);
	if (ccm == MAP_FAILED)
	{
		ado_error ("Unable to mmap CCM (%s)", strerror (errno));
		munmap_device_memory (mx35_3ds->ssi, sizeof (ssi_t));
		munmap_device_memory (mx35_3ds->audmux, sizeof (audmux_t));
		ado_free (mx35_3ds);
		return -1;
	}

	ado_mutex_init (&mx35_3ds->hw_lock);

	/*
	 * AK4647 on i.MX35 3DS derives its clock from a 12MHz external
	 * oscillator, so disable Clock Out
	 */
	ccm->cosr &= ~COSR_CLKOEN;

	if (get_dmafuncs (&mx35_3ds->sdmafuncs, sizeof (dma_functions_t)) == -1)
	{
	    ado_error ("Failed to get DMA lib functions");
	    ado_mutex_destroy (&mx35_3ds->hw_lock);
	    munmap_device_memory (mx35_3ds->ssi, sizeof (ssi_t));
	    munmap_device_memory (mx35_3ds->audmux, sizeof (audmux_t));
	    ado_free (mx35_3ds);
	    return -1;
	}

	my_attach_pulse (&mx35_3ds->play_strm.pulse,
		&mx35_3ds->play_strm.sdma_event, mx35_3ds_play_pulse_hdlr, mx35_3ds);
	my_attach_pulse (&mx35_3ds->cap_strm.pulse,
		&mx35_3ds->cap_strm.sdma_event, mx35_3ds_cap_pulse_hdlr, mx35_3ds);

	if (mx35_3ds->sdmafuncs.init (NULL) == -1)
	{
	    ado_error ("DMA init failed");
        my_detach_pulse (&mx35_3ds->cap_strm.pulse);
	    my_detach_pulse (&mx35_3ds->play_strm.pulse);
	    ado_mutex_destroy (&mx35_3ds->hw_lock);
	    munmap_device_memory (mx35_3ds->ssi, sizeof (ssi_t));
	    munmap_device_memory (mx35_3ds->audmux, sizeof (audmux_t));
	    ado_free (mx35_3ds);
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
	channel = SDMA_PLAYBACK_CHANNEL;
	if (mx35_ssi == 1)
	{
		mx35_3ds->play_strm.dma_chn =
			mx35_3ds->sdmafuncs.channel_attach(
			"eventnum=29,watermark=4,fifopaddr=0x43FA0000,regen,contloop",
			&mx35_3ds->play_strm.sdma_event, &channel,
			DMA_ATTACH_PRIORITY_HIGHEST, DMA_ATTACH_EVENT_PER_SEGMENT);
	}
	else
	{
		mx35_3ds->play_strm.dma_chn =
			mx35_3ds->sdmafuncs.channel_attach(
			"eventnum=25,watermark=4,fifopaddr=0x50014000,regen,contloop",
			&mx35_3ds->play_strm.sdma_event, &channel,
			DMA_ATTACH_PRIORITY_HIGHEST, DMA_ATTACH_EVENT_PER_SEGMENT);
	}

	if (mx35_3ds->play_strm.dma_chn == NULL)
    {
        ado_error ("SDMA playback channel attach failed");
        my_detach_pulse (&mx35_3ds->cap_strm.pulse);
        my_detach_pulse (&mx35_3ds->play_strm.pulse);
        mx35_3ds->sdmafuncs.fini ();
        ado_mutex_destroy (&mx35_3ds->hw_lock);
        munmap_device_memory (mx35_3ds->ssi, sizeof (ssi_t));
        munmap_device_memory (mx35_3ds->audmux, sizeof (audmux_t));
        ado_free (mx35_3ds);
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
	channel = SDMA_CAPTURE_CHANNEL;
	if (mx35_ssi == 1)
	{
		mx35_3ds->cap_strm.dma_chn =
			mx35_3ds->sdmafuncs.channel_attach(
			"eventnum=28,watermark=4,fifopaddr=0x43FA0008,regen,contloop",
			&mx35_3ds->cap_strm.sdma_event, &channel,
			DMA_ATTACH_PRIORITY_HIGHEST, DMA_ATTACH_EVENT_PER_SEGMENT);
	}
	else
	{
		mx35_3ds->cap_strm.dma_chn =
			mx35_3ds->sdmafuncs.channel_attach(
			"eventnum=24,watermark=4,fifopaddr=0x50014008,regen,contloop",
			&mx35_3ds->cap_strm.sdma_event, &channel,
			DMA_ATTACH_PRIORITY_HIGHEST, DMA_ATTACH_EVENT_PER_SEGMENT);
	}

    if (mx35_3ds->cap_strm.dma_chn == NULL)
    {
        ado_error ("SDMA capture channel attach failed");
		mx35_3ds->sdmafuncs.channel_release (mx35_3ds->play_strm.dma_chn);
        my_detach_pulse (&mx35_3ds->cap_strm.pulse);
        my_detach_pulse (&mx35_3ds->play_strm.pulse);
        mx35_3ds->sdmafuncs.fini ();
        ado_mutex_destroy (&mx35_3ds->hw_lock);
        munmap_device_memory (mx35_3ds->ssi, sizeof (ssi_t));
        munmap_device_memory (mx35_3ds->audmux, sizeof (audmux_t));
        ado_free (mx35_3ds);
        return -1;
    }

	ssi_init (mx35_3ds, ccm);
	munmap_device_memory (ccm, sizeof (ccm_t));

	if (mx35_3ds_i2c_init (mx35_3ds))
	{
		mx35_3ds->sdmafuncs.channel_release (mx35_3ds->cap_strm.dma_chn);
		mx35_3ds->sdmafuncs.channel_release (mx35_3ds->play_strm.dma_chn);
        my_detach_pulse (&mx35_3ds->cap_strm.pulse);
		my_detach_pulse (&mx35_3ds->play_strm.pulse);
		mx35_3ds->sdmafuncs.fini ();
		ado_mutex_destroy (&mx35_3ds->hw_lock);
		munmap_device_memory (mx35_3ds->ssi, sizeof (ssi_t));
		ado_free (mx35_3ds);
	}

	/* reset and power up the codec */
	setCodecPowerMode (mx35_3ds, POWER_UP);

	if (ak4647_mixer (card, &mx35_3ds->mixer, mx35_3ds))
	{
		ado_error ("Unable to create AK4647 mixer");
		mx35_3ds->sdmafuncs.channel_release (mx35_3ds->cap_strm.dma_chn);
		mx35_3ds->sdmafuncs.channel_release (mx35_3ds->play_strm.dma_chn);
        my_detach_pulse (&mx35_3ds->cap_strm.pulse);
		my_detach_pulse (&mx35_3ds->play_strm.pulse);
		mx35_3ds->sdmafuncs.fini ();
		ado_mutex_destroy (&mx35_3ds->hw_lock);
		munmap_device_memory (mx35_3ds->ssi, sizeof (ssi_t));
		close (mx35_3ds->i2c_fd);
		ado_free (mx35_3ds);
		return -1;
	}

	mx35_3ds->play_strm.pcm_caps.chn_flags =
		SND_PCM_CHNINFO_BLOCK | SND_PCM_CHNINFO_STREAM |
		SND_PCM_CHNINFO_INTERLEAVE | SND_PCM_CHNINFO_BLOCK_TRANSFER |
		SND_PCM_CHNINFO_MMAP | SND_PCM_CHNINFO_MMAP_VALID;
	mx35_3ds->play_strm.pcm_caps.formats = SND_PCM_FMT_S16_LE;
	mx35_3ds->play_strm.pcm_caps.rates = SND_PCM_RATE_44100;
	mx35_3ds->play_strm.pcm_caps.min_rate = 44100;
	mx35_3ds->play_strm.pcm_caps.max_rate = 44100;
	mx35_3ds->play_strm.pcm_caps.min_voices = 2;
	mx35_3ds->play_strm.pcm_caps.max_voices = 2;
	mx35_3ds->play_strm.pcm_caps.min_fragsize = 64;
	mx35_3ds->play_strm.pcm_caps.max_fragsize = 32 * 1024;
	
	memcpy (&mx35_3ds->cap_strm.pcm_caps, &mx35_3ds->play_strm.pcm_caps,
		sizeof (mx35_3ds->cap_strm.pcm_caps));

	mx35_3ds->play_strm.pcm_funcs.capabilities = mx35_3ds_capabilities;
	mx35_3ds->play_strm.pcm_funcs.aquire = mx35_3ds_playback_aquire;
	mx35_3ds->play_strm.pcm_funcs.release = mx35_3ds_playback_release;
	mx35_3ds->play_strm.pcm_funcs.prepare = mx35_3ds_prepare;
	mx35_3ds->play_strm.pcm_funcs.trigger = mx35_3ds_playback_trigger;
#if 0
	mx35_3ds->play_strm.pcm_funcs.position = mx35_3ds_position;
#endif

	mx35_3ds->cap_strm.pcm_funcs.capabilities = mx35_3ds_capabilities;
	mx35_3ds->cap_strm.pcm_funcs.aquire = mx35_3ds_capture_aquire;
	mx35_3ds->cap_strm.pcm_funcs.release = mx35_3ds_capture_release;
	mx35_3ds->cap_strm.pcm_funcs.prepare = mx35_3ds_prepare;
	mx35_3ds->cap_strm.pcm_funcs.trigger = mx35_3ds_capture_trigger;
#if 0
	mx35_3ds->cap_strm.pcm_funcs.position = mx35_3ds_position;
#endif

	if (ado_pcm_create (card, "mx35_3ds PCM 0", 0, "mx35_3ds-0",
			1, &mx35_3ds->play_strm.pcm_caps, &mx35_3ds->play_strm.pcm_funcs,
			1, &mx35_3ds->cap_strm.pcm_caps, &mx35_3ds->cap_strm.pcm_funcs,
			&mx35_3ds->pcm1))
	{
		ado_error ("Unable to create pcm devices (%s)", strerror (errno));
		mx35_3ds->sdmafuncs.channel_release (mx35_3ds->cap_strm.dma_chn);
		mx35_3ds->sdmafuncs.channel_release (mx35_3ds->play_strm.dma_chn);
        my_detach_pulse (&mx35_3ds->cap_strm.pulse);
		my_detach_pulse (&mx35_3ds->play_strm.pulse);
		mx35_3ds->sdmafuncs.fini ();
		ado_mutex_destroy (&mx35_3ds->hw_lock);
		close (mx35_3ds->i2c_fd);
		munmap_device_memory (mx35_3ds->ssi, sizeof (ssi_t));
		ado_free (mx35_3ds);
		return -1;
	}

	/* setup mixer controls for playback and capture */
	ado_pcm_chn_mixer (mx35_3ds->pcm1,
		ADO_PCM_CHANNEL_PLAYBACK, mx35_3ds->mixer,
		ado_mixer_find_element (mx35_3ds->mixer, SND_MIXER_ETYPE_PLAYBACK1,
			SND_MIXER_PCM_OUT, 0),
		ado_mixer_find_group (mx35_3ds->mixer, SND_MIXER_PCM_OUT, 0));

	ado_pcm_chn_mixer (mx35_3ds->pcm1,
		ADO_PCM_CHANNEL_CAPTURE, mx35_3ds->mixer,
		ado_mixer_find_element (mx35_3ds->mixer, SND_MIXER_ETYPE_CAPTURE1,
			SND_MIXER_ELEMENT_CAPTURE, 0), 
		ado_mixer_find_group (mx35_3ds->mixer, SND_MIXER_GRP_IGAIN, 0));

	/* allow audio chip to support multiple simultaneous streams */
	if (ado_pcm_sw_mix (card, mx35_3ds->pcm1, mx35_3ds->mixer))
	{
		ado_error ("Unable to create a pcm sw mixer");
		mx35_3ds->sdmafuncs.channel_release (mx35_3ds->cap_strm.dma_chn);
		mx35_3ds->sdmafuncs.channel_release (mx35_3ds->play_strm.dma_chn);
        my_detach_pulse (&mx35_3ds->cap_strm.pulse);
		my_detach_pulse (&mx35_3ds->play_strm.pulse);
		mx35_3ds->sdmafuncs.fini ();
		ado_mutex_destroy (&mx35_3ds->hw_lock);
		close(mx35_3ds->i2c_fd);
		munmap_device_memory (mx35_3ds->ssi, sizeof (ssi_t));
		ado_free (mx35_3ds);
		return -1;
	}
	
    return 0;
}

ado_ctrl_dll_destroy_t ctrl_destroy;
int
ctrl_destroy (HW_CONTEXT_T * mx35_3ds)
{
	ado_debug (DB_LVL_DRIVER, "CTRL_DLL_DESTROY: 3DS MX35");

	/* power down the codec */
	setCodecPowerMode (mx35_3ds, POWER_DOWN);

	mx35_3ds->sdmafuncs.channel_release (mx35_3ds->cap_strm.dma_chn);
	mx35_3ds->sdmafuncs.channel_release (mx35_3ds->play_strm.dma_chn);
	my_detach_pulse (&mx35_3ds->cap_strm.pulse);
	my_detach_pulse (&mx35_3ds->play_strm.pulse);
	mx35_3ds->sdmafuncs.fini ();
	close(mx35_3ds->i2c_fd);
	ado_mutex_destroy (&mx35_3ds->hw_lock);
	munmap_device_memory (mx35_3ds->ssi, sizeof (ssi_t));
	ado_free (mx35_3ds);

	return 0;
}
