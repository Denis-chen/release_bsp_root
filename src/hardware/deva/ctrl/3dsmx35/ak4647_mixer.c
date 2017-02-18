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


struct ak4647_context;

#define	 MIXER_CONTEXT_T struct ak4647_context
#include <audio_driver.h>
#include <string.h>

#include "mx35_3ds.h"
#include "ak4647.h"

#define REC_SRC_MIC		0
#define REC_SRC_LINE1	1
#define REC_SRC_LINE2	2

typedef struct ak4647_context
{
	ado_mixer_t *mixer;
	ado_mixer_delement_t *mic_in;
	ado_mixer_delement_t *line1_in;
	ado_mixer_delement_t *line2_in;
	HW_CONTEXT_T *hwc;
	int recsrc;
	uint8_t mgain;
	int imute;
}
ak4647_context_t;

#define MX35_3DS_CTRL		(ak4647->hwc)

static int32_t pcm_devices[2] = {
	0, 1
};

static snd_mixer_voice_t stereo_voices[2] = {
	{SND_MIXER_VOICE_LEFT, 0},
	{SND_MIXER_VOICE_RIGHT, 0}
};

static struct snd_mixer_element_volume1_range output_range[2] = {
	{0, 255, -11500, 1200},
	{0, 255, -11500, 1200},
};

static struct snd_mixer_element_volume1_range input_range[2] = {
	{0, 241, -5400, 3600},
	{0, 241, -5400, 3600},
};

/* Digital output volume control */
static int32_t
ak4647_pcm_vol_control (MIXER_CONTEXT_T * ak4647,
	ado_mixer_delement_t * element, uint8_t set,
	uint32_t * vol, void *instance_data)
{
	int max = ado_mixer_element_vol_range_max (element);
	uint8_t data[2];
	int32_t altered = 0;
	
	if (mx35_3ds_i2c_read (MX35_3DS_CTRL, MODE_CONTROL_3) & 0x10)
	{
		data[0] = mx35_3ds_i2c_read (MX35_3DS_CTRL, LCH_DIG_VOL_CTRL);
		if (set)
		{
			altered = vol[0] != (data[0] & max);
			data[0] = (data[0] & ~(max)) | (max - vol[0]);
			mx35_3ds_i2c_write (MX35_3DS_CTRL, LCH_DIG_VOL_CTRL, data[0]);
			mx35_3ds_i2c_write (MX35_3DS_CTRL, RCH_DIG_VOL_CTRL, data[0]);
		}
		else
		{
			vol[0] = max - (data[0] & max);
			vol[1] = max - (data[0] & max);
		}
	}
	else
	{
		data[0] = mx35_3ds_i2c_read (MX35_3DS_CTRL, LCH_DIG_VOL_CTRL);
		data[1] = mx35_3ds_i2c_read (MX35_3DS_CTRL, RCH_DIG_VOL_CTRL);
		if (set)
		{
			altered = vol[0] != (data[0] & max) && vol[1] != (data[1] & max);
			data[0] = (data[0] & ~(max)) | (max - vol[0]);
			data[1] = (data[1] & ~(max)) | (max - vol[1]);
			mx35_3ds_i2c_write (MX35_3DS_CTRL, LCH_DIG_VOL_CTRL, data[0]);
			mx35_3ds_i2c_write (MX35_3DS_CTRL, RCH_DIG_VOL_CTRL, data[1]);
		}
		else
		{
			vol[0] = max - (data[0] & max);
			vol[1] = max - (data[1] & max);
		}
	}

	return (altered);
}

/* update Input Volume and ALC reference level */
static int32_t
ak4647_input_vol_control (MIXER_CONTEXT_T * ak4647,
	ado_mixer_delement_t * element, uint8_t set,
	uint32_t * vol, void *instance_data)
{
	int max = ado_mixer_element_vol_range_max (element);
	uint8_t data[3], reqvol;
	int32_t altered = 0;

	data[0] = mx35_3ds_i2c_read (MX35_3DS_CTRL, ALC_MOD_CTRL_2);
	if (set)
	{
		reqvol = (vol[0] > max) ? max : vol[0];
		altered = reqvol != data[0];
		if (altered)
		{
			data[1] = mx35_3ds_i2c_read (MX35_3DS_CTRL, POWER_MNGMT_1);
			data[2] = mx35_3ds_i2c_read (MX35_3DS_CTRL, POWER_MNGMT_3);

			/* power off ADC, remember settings */
			mx35_3ds_i2c_write (MX35_3DS_CTRL, POWER_MNGMT_1, data[1] & ~0x1);
			mx35_3ds_i2c_write (MX35_3DS_CTRL, POWER_MNGMT_3, data[2] & ~0x1);

			/* update input volume and ALC reference level */
			mx35_3ds_i2c_write (MX35_3DS_CTRL, LCH_IP_VOL_CTRL, reqvol);
			mx35_3ds_i2c_write (MX35_3DS_CTRL, ALC_MOD_CTRL_2, reqvol);

			/* power on ADC, restore settings */
 			mx35_3ds_i2c_write (MX35_3DS_CTRL, POWER_MNGMT_1, data[1]);
 			mx35_3ds_i2c_write (MX35_3DS_CTRL, POWER_MNGMT_3, data[2]);
 		}
 	}
 	else
 	{
		vol[0] = vol[1] = data[0];
 	}

	return (altered);
}

/* AK4647 soft mute (SMUTE) */
static int32_t
ak4647_pcm_mute_control (MIXER_CONTEXT_T * ak4647,
	ado_mixer_delement_t * element, uint8_t set,
	uint32_t * val, void *instance_data)
{
	uint8_t data;
	int32_t altered = 0;
	
	data = mx35_3ds_i2c_read (MX35_3DS_CTRL, MODE_CONTROL_3);
	if (set)
	{
		altered = val[0] != (data & (1 << 5));
		data = (data & ~(1 << 5)) | (val[0] ? (1 << 5) : 0);
		mx35_3ds_i2c_write (MX35_3DS_CTRL, MODE_CONTROL_3, data);
	}
	else
	{
		val[0] = (data & (1 << 5) ? 1 : 0);
	}
			
	return (altered);
}

/* fake mute by turning off ADC */
static int32_t
ak4647_input_mute_control (MIXER_CONTEXT_T * ak4647,
	ado_mixer_delement_t * element, uint8_t set,
	uint32_t * val, void *instance_data)
{
	uint8_t data[2];
	int32_t altered = 0;
	
	if (set)
	{
		data[0] = mx35_3ds_i2c_read (MX35_3DS_CTRL, POWER_MNGMT_1);
		data[1] = mx35_3ds_i2c_read (MX35_3DS_CTRL, POWER_MNGMT_3);
		altered = val[0] != ak4647->imute;
		if (val[0] && !ak4647->imute)
		{
			mx35_3ds_i2c_write (MX35_3DS_CTRL, POWER_MNGMT_1, data[0] & ~0x1);
			if (ak4647->recsrc != REC_SRC_MIC)
			{
				mx35_3ds_i2c_write (MX35_3DS_CTRL,
					POWER_MNGMT_3, data[1] & ~0x1);
			}
			ak4647->imute = 1;
		}
		else if (!val[0] && ak4647->imute)
		{
			mx35_3ds_i2c_write (MX35_3DS_CTRL, POWER_MNGMT_1, data[0] | 0x1);
			if (ak4647->recsrc != REC_SRC_MIC)
			{
				mx35_3ds_i2c_write (MX35_3DS_CTRL,
					POWER_MNGMT_3, data[1] | 0x1);
			}
			delay (25);
			ak4647->imute = 0;
		}
	}
	else
	{
		val[0] = ak4647->imute;
	}
	
	return (altered);
}

/* select one of Mic In and Line In */
static int32_t
ak4647_input_mux_control(MIXER_CONTEXT_T * ak4647,
	ado_mixer_delement_t * element, uint8_t set,
	ado_mixer_delement_t **inelements, void *instance_data)
{
	uint32_t data;
	uint32_t mdata[2];
	uint32_t mode = 0;
	int32_t altered = 0;

	data = mx35_3ds_i2c_read (MX35_3DS_CTRL, POWER_MNGMT_3);
	if (set)
	{
		if (inelements[0] == ak4647->line1_in &&
			ak4647->recsrc != REC_SRC_LINE1)
		{
			/* Line In 1: power up both ADL and ADR (stereo) */
			mode = 1;
			ak4647->recsrc = REC_SRC_LINE1;
			altered = 1;

			/*
			 * Clear Mic Gain
			 */
			mdata[0] = mx35_3ds_i2c_read (MX35_3DS_CTRL, SIGNAL_SLCT_1);
			mdata[0] = mdata[0] & ~0x01;
			mx35_3ds_i2c_write (MX35_3DS_CTRL, SIGNAL_SLCT_1, mdata[0]);
			
			mdata[1] = mx35_3ds_i2c_read (MX35_3DS_CTRL, SIGNAL_SLCT_2);
			mdata[1] = mdata[1] & ~0x20;
			mx35_3ds_i2c_write (MX35_3DS_CTRL, SIGNAL_SLCT_2, mdata[1]);
		}
		else if (inelements[0] == ak4647->line2_in &&
			ak4647->recsrc != REC_SRC_LINE2)
		{
			/* Line In 2: power up both ADL and ADR (stereo) */
			mode = 7;
			ak4647->recsrc = REC_SRC_LINE2;
			altered = 1;

			/*
			 * Clear Mic Gain
			 */
			mdata[0] = mx35_3ds_i2c_read (MX35_3DS_CTRL, SIGNAL_SLCT_1);
			mdata[0] = mdata[0] & ~0x01;
			mx35_3ds_i2c_write (MX35_3DS_CTRL, SIGNAL_SLCT_1, mdata[0]);
			
			mdata[1] = mx35_3ds_i2c_read (MX35_3DS_CTRL, SIGNAL_SLCT_2);
			mdata[1] = mdata[1] & ~0x20;
			mx35_3ds_i2c_write (MX35_3DS_CTRL, SIGNAL_SLCT_2, mdata[1]);
		}
		else if (inelements[0] == ak4647->mic_in &&
			ak4647->recsrc != REC_SRC_MIC)
		{
			/* Mono Mic-In: only power up ADL (mono) */
			mode = 0;
			ak4647->recsrc = REC_SRC_MIC;
			altered = 1;

			/*
			 * Set Mic Gain from mixer context
			 */
			mdata[0] = mx35_3ds_i2c_read (MX35_3DS_CTRL, SIGNAL_SLCT_1);
			mdata[0] = (mdata[0] & ~0x01) | (ak4647->mgain & 0x01);
			mx35_3ds_i2c_write (MX35_3DS_CTRL, SIGNAL_SLCT_1, mdata[0]);
			
			mdata[1] = mx35_3ds_i2c_read (MX35_3DS_CTRL, SIGNAL_SLCT_2);
			mdata[1] = (mdata[1] & ~0x20) | ((ak4647->mgain & 0x02) << 4);
			mx35_3ds_i2c_write (MX35_3DS_CTRL, SIGNAL_SLCT_2, mdata[1]);
		}
		data = (data & ~0x07) | mode;
		mx35_3ds_i2c_write (MX35_3DS_CTRL, POWER_MNGMT_3, data);
	}
	else
	{
		if (ak4647->recsrc == REC_SRC_MIC)
			inelements[0] = inelements[1] = ak4647->mic_in;
		else if (ak4647->recsrc == REC_SRC_LINE1)
			inelements[0] = inelements[1] = ak4647->line1_in;
		else if (ak4647->recsrc == REC_SRC_LINE2)
			inelements[0] = inelements[1] = ak4647->line2_in;
	}

	return (altered);
}

/*
 * configure routing: i.MX PCM -> AK4647
 */
void audmux_init(audmux_t *audmux, int other)
{
	/*
	 * Configure Port 4 (external port to which AK4647 is connected)
	 *  Reset, Sync mode, Frame Sync Input, Clock Input
	 *  Port 4 receives clocks from AK4647
	 */
	audmux->ptcr4 = 0x0;
	audmux->pdcr4 = 0x0;
	audmux->ptcr4 |= (AUDMUX_SYNC);
	audmux->ptcr4 &= ~(AUDMUX_TFS_DIROUT);
	audmux->ptcr4 &= ~(AUDMUX_TCLK_DIROUT);

	if (other)
	{
		/* Configure SSI2 (internal Port 2) for AK4647 on Port 4
		 *  Reset, Sync mode, Frame Sync Output, Clock Output
		 *  SSI2 = Slave, AK4647 = Master
		 */
		audmux->ptcr2 = 0x0;
		audmux->pdcr2 = 0x0;

		audmux->pdcr2 |= (AUDMUX_RXDSEL_PORT4);
		audmux->pdcr4 &= ~AUDMUX_RXDSEL_MSK;
		audmux->pdcr4 |= AUDMUX_RXDSEL_PORT2;

		audmux->ptcr2 |= (AUDMUX_SYNC |
			AUDMUX_TFS_DIROUT | AUDMUX_TFS_PORT4 |
			AUDMUX_TCLK_DIROUT | AUDMUX_TCLK_PORT4 |
			AUDMUX_RFS_DIROUT | AUDMUX_RFS_PORT4 |
			AUDMUX_RCLK_DIROUT | AUDMUX_RCLK_PORT4);
	}
	else
	{
		/* Configure SSI1 (internal Port 1) for AK4647 on POrt 4
		 *  Reset, Sync mode, Frame Sync Output, Clock Output
		 *  SSI1 = Slave, AK4647 = Master
		 */
		audmux->ptcr1 = 0x0;
		audmux->pdcr1 = 0x0;

		audmux->pdcr1 |= (AUDMUX_RXDSEL_PORT4);
		audmux->pdcr4 &= ~AUDMUX_RXDSEL_MSK;
		audmux->pdcr4 |= AUDMUX_RXDSEL_PORT1;

		audmux->ptcr1 |= (AUDMUX_SYNC |
			AUDMUX_TFS_DIROUT | AUDMUX_TFS_PORT4 |
			AUDMUX_TCLK_DIROUT | AUDMUX_TCLK_PORT4 |
			AUDMUX_RFS_DIROUT | AUDMUX_RFS_PORT4 |
			AUDMUX_RCLK_DIROUT | AUDMUX_RCLK_PORT4);
	}
}

int32_t
ak4647_mgain_get (MIXER_CONTEXT_T * ak4647,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	cswitch->type = SND_SW_TYPE_BYTE;
	cswitch->value.byte.low = 0;
	cswitch->value.byte.high = 3;
	cswitch->value.byte.data = ak4647->mgain;

	return (0);	
}

int32_t
ak4647_mgain_set (MIXER_CONTEXT_T * ak4647,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data[2];
	int32_t altered = 0;

	data[0] = mx35_3ds_i2c_read (MX35_3DS_CTRL, SIGNAL_SLCT_1);
	data[1] = mx35_3ds_i2c_read (MX35_3DS_CTRL, SIGNAL_SLCT_2);

	if (cswitch->value.byte.low <= cswitch->value.byte.data &&
	    cswitch->value.byte.data <= cswitch->value.byte.high &&
	    cswitch->value.byte.data != ak4647->mgain)
	{
		ak4647->mgain = cswitch->value.byte.data;
		/*
		 * Set Mic Gain only if Mic is selected as the record source
		 */
		if (ak4647->recsrc == REC_SRC_MIC)
		{
			data[0] = (data[0] & ~0x01) | (cswitch->value.byte.data & 0x01);
			data[1] = (data[1] & ~0x20) |
				((cswitch->value.byte.data & 0x02) << 4);
			mx35_3ds_i2c_write (MX35_3DS_CTRL, SIGNAL_SLCT_1, data[0]);
			mx35_3ds_i2c_write (MX35_3DS_CTRL, SIGNAL_SLCT_2, data[1]);
		}
		altered = 1;
	}

	return (altered);
}

int32_t
ak4647_bassboost_get (MIXER_CONTEXT_T * ak4647,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data;

	data = mx35_3ds_i2c_read (MX35_3DS_CTRL, MODE_CONTROL_3);
	cswitch->type = SND_SW_TYPE_BYTE;
	cswitch->value.byte.low = 0;
	cswitch->value.byte.high = 3;
	cswitch->value.byte.data = (data >> 2) & 0x3;

	return (0);
}

int32_t
ak4647_bassboost_set (MIXER_CONTEXT_T * ak4647,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data, val;
	int32_t altered = 0;

	data = mx35_3ds_i2c_read (MX35_3DS_CTRL, MODE_CONTROL_3);
	val = (data >> 2) & 0x3;
	if (cswitch->value.byte.low <= cswitch->value.byte.data &&
	    cswitch->value.byte.data <= cswitch->value.byte.high &&
	    cswitch->value.byte.data != val)
	{
		data = (data & ~0xc) | (cswitch->value.byte.data << 2);
		mx35_3ds_i2c_write (MX35_3DS_CTRL, MODE_CONTROL_3, data);
		altered = 1;
	}

	return (altered);
}

int32_t
ak4647_loopback_get (MIXER_CONTEXT_T * ak4647,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data;

	data = mx35_3ds_i2c_read (MX35_3DS_CTRL, MODE_CONTROL_3);
	cswitch->type = SND_SW_TYPE_BOOLEAN;
	cswitch->value.enable = (data & (1 << 6)) ? 1 : 0;

	return (0);
}

int32_t
ak4647_loopback_set (MIXER_CONTEXT_T * ak4647,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data;
	int32_t altered = 0;

	data = mx35_3ds_i2c_read (MX35_3DS_CTRL, MODE_CONTROL_3);
	altered = cswitch->value.enable != ((data & (1 << 6)) ? 1 : 0);
	data = (data & ~(1 << 6)) | (cswitch->value.enable ? (1 << 6) : 0);
	mx35_3ds_i2c_write (MX35_3DS_CTRL, MODE_CONTROL_3, data);

	return (altered);
}

int32_t
ak4647_alc_get (MIXER_CONTEXT_T * ak4647,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data;

	data = mx35_3ds_i2c_read (MX35_3DS_CTRL, ALC_MOD_CTRL_1);
	cswitch->type = SND_SW_TYPE_BOOLEAN;
	cswitch->value.enable = (data & (1 << 5)) ? 1 : 0;

	return (0);
}

int32_t
ak4647_alc_set (MIXER_CONTEXT_T * ak4647,
	ado_dswitch_t * dswitch, snd_switch_t * cswitch,
	void *instance_data)
{
	uint32_t data;
	int32_t altered = 0;

	data = mx35_3ds_i2c_read (MX35_3DS_CTRL, ALC_MOD_CTRL_1);
	altered = cswitch->value.enable != ((data & (1 << 5)) ? 1 : 0);
	data = (data & ~(1 << 5)) | (cswitch->value.enable ? (1 << 5) : 0);
	mx35_3ds_i2c_write (MX35_3DS_CTRL, ALC_MOD_CTRL_1, data);

	return (altered);
}

static int32_t
build_ak4647_mixer (MIXER_CONTEXT_T * ak4647, ado_mixer_t * mixer)
{
	/* Output elements */
	ado_mixer_delement_t *output_accu = NULL;
	ado_mixer_delement_t *dac = NULL, *pcm_vol = NULL, *pcm_mute = NULL;
	ado_mixer_dgroup_t *pcm_grp = NULL;

	/* Input elements */
	ado_mixer_delement_t *mic_in = NULL, *line1_in = NULL, *line2_in = NULL;
	ado_mixer_delement_t *input_mux = NULL, *input_vol = NULL;
	ado_mixer_delement_t *input_mute = NULL, *adc = NULL;
	ado_mixer_dgroup_t *cap_line1in = NULL, *cap_line2in = NULL;
	ado_mixer_dgroup_t *cap_igain = NULL, *cap_micin = NULL;

	int error = 0;

	/* ################# */
	/*  PLAYBACK GROUPS  */
	/* ################# */

	/*
	 * Output Accumulator
	 */
	if ((output_accu = ado_mixer_element_accu1 (mixer,
				SND_MIXER_ELEMENT_OUTPUT_ACCU, 0)) == NULL)
		error++;

	/*
	 * AK4647 DAC: PCM OUT
	 */
	if (!error && (dac = ado_mixer_element_pcm1 (mixer, "PCM Out",
				SND_MIXER_ETYPE_PLAYBACK1, 1, &pcm_devices[0])) == NULL)
		error++;

	if (!error && (pcm_vol = ado_mixer_element_volume1 (mixer, "PCM Volume",
				2, output_range, ak4647_pcm_vol_control, NULL, NULL)) == NULL)
		error++;

	if (!error && ado_mixer_element_route_add (mixer, dac, pcm_vol) != 0)
		error++;

	if (!error && (pcm_mute = ado_mixer_element_sw2 (mixer, "PCM Mute",
				ak4647_pcm_mute_control, NULL, NULL)) == NULL)
		error++;

	if (!error && ado_mixer_element_route_add (mixer, pcm_vol, pcm_mute) != 0)
		error++;
	
	if (!error && ado_mixer_element_route_add (mixer,
				pcm_mute, output_accu) != 0)
		error++;

	if (!error && (pcm_grp = ado_mixer_playback_group_create (mixer, "PCM",
				SND_MIXER_CHN_MASK_STEREO, pcm_vol, pcm_mute)) == NULL)
		error++;

	/* ################ */
	/*  CAPTURE GROUPS  */
	/* ################ */

	/*
	 * Input Multiplexer
	 */
	if (!error && (input_mux = ado_mixer_element_mux1 (mixer,
				SND_MIXER_ELEMENT_INPUT_MUX, 0, 2,
				ak4647_input_mux_control, NULL, NULL)) == NULL)
		error++;

	if (!error && (input_vol = ado_mixer_element_volume1 (mixer,
				"Input Volume", 2, input_range,
				ak4647_input_vol_control, NULL, NULL)) == NULL)
		error++;
	
	if (!error && ado_mixer_element_route_add (mixer,
				input_mux, input_vol) != 0)
		error++;
	
	if (!error && (input_mute = ado_mixer_element_sw2 (mixer, "Input Mute", 
				ak4647_input_mute_control, NULL, NULL)) == NULL)
		error++;
	
	if (!error && ado_mixer_element_route_add (mixer,
				input_vol, input_mute) != 0)
		error++;
	
	/*
	 * AK4647 ADC: PCM IN
	 */	
	if (!error && (adc = ado_mixer_element_pcm1 (mixer, "PCM In",
				SND_MIXER_ETYPE_CAPTURE1, 1, &pcm_devices[0])) == NULL)
		error++;
	
	if (!error && ado_mixer_element_route_add (mixer, input_mute, adc) != 0)
		error++;	
	
	if (!error && (cap_igain = ado_mixer_capture_group_create (mixer,
				"Input Gain", SND_MIXER_CHN_MASK_STEREO,
				input_vol, input_mute, NULL, NULL)) == NULL)
		error++;		

	/*
	 * AK4647 MIN: Mic
	 *
	 * NOTE: AK4647 MIN used for connecting microphone in MX35 3DS HARDWARE,
	 *       so microphone record may not work as expected
	 */
	if (!error && (mic_in = ado_mixer_element_io (mixer, "Mic",
				SND_MIXER_ETYPE_INPUT , 0, 2, stereo_voices)) == NULL)
		error++;
	ak4647->mic_in = mic_in;
	
	/*
	 * AK4647 LIN1/RIN1: Line 1
	 */
	if (!error && (line1_in = ado_mixer_element_io (mixer, "Line 1",
				SND_MIXER_ETYPE_INPUT , 0, 2, stereo_voices)) == NULL)
		error++;
	ak4647->line1_in = line1_in;
	
	/*
	 * AK4647 LIN2/RIN2: Line 2
	 */
	if (!error && (line2_in = ado_mixer_element_io (mixer, "Line 2",
				SND_MIXER_ETYPE_INPUT , 0, 2, stereo_voices)) == NULL)
		error++;
	ak4647->line2_in = line2_in;

	/*
	 * Setup connections to MUX
	 */
	if (!error &&
			(ado_mixer_element_route_add (mixer, mic_in, input_mux) != 0 ||
			ado_mixer_element_route_add (mixer, line1_in, input_mux) != 0 ||
			ado_mixer_element_route_add (mixer, line2_in, input_mux) != 0 ))
		error++;

	/* Mic Capture Group */	
	if (!error && (cap_micin = ado_mixer_capture_group_create (mixer, "Mic In",
			SND_MIXER_CHN_MASK_STEREO, NULL, NULL, input_mux, mic_in)) == NULL)
		error++;
	
	/* Line 1 Capture Group */	
	if (!error && (cap_line1in = ado_mixer_capture_group_create (mixer,
			"Line1 In", SND_MIXER_CHN_MASK_STEREO, NULL, NULL,
			input_mux, line1_in)) == NULL)
		error++;
	
	/* Line 2 Capture Group */	
	if (!error && (cap_line2in = ado_mixer_capture_group_create (mixer,
			"Line2 In", SND_MIXER_CHN_MASK_STEREO, NULL, NULL,
			input_mux, line2_in)) == NULL)
	 error++;

	/* ####################### */
	/* SWITCHES                */
	/* ####################### */

	if (!error && ado_mixer_switch_new (mixer, "Mic Gain", SND_SW_TYPE_BYTE,
			0 , ak4647_mgain_get, ak4647_mgain_set, NULL, NULL) == NULL)
		error++;
		
	if (!error && ado_mixer_switch_new (mixer, "Bass Boost", SND_SW_TYPE_BYTE,
			0 , ak4647_bassboost_get, ak4647_bassboost_set, NULL, NULL) == NULL)
		error++;
		
	if (!error && ado_mixer_switch_new (mixer, "ADC/DAC Loopback",
			SND_SW_TYPE_BOOLEAN, 0,
			ak4647_loopback_get, ak4647_loopback_set, NULL, NULL) == NULL)
		error++;

	if (!error && ado_mixer_switch_new (mixer, "ALC", SND_SW_TYPE_BOOLEAN,
			0 , ak4647_alc_get, ak4647_alc_set, NULL, NULL) == NULL)
		error++;

	return (!error ? 0 : -1);		
}


static ado_mixer_reset_t ak4647_reset;
static int
ak4647_reset (MIXER_CONTEXT_T * ak4647)
{
	audmux_init (ak4647->hwc->audmux,
		(ak4647->hwc->ssi_base == SSI1_BASE) ? 0 : 1);
	ak4647->recsrc = REC_SRC_LINE1;
	ak4647->mgain = 0;
	ak4647->imute = 0;

	return (0);
}


static ado_mixer_destroy_t ak4647_destroy;
static int
ak4647_destroy (MIXER_CONTEXT_T * ak4647)
{
	ado_debug (DB_LVL_MIXER, "Destroying AK4647 mixer");
	ado_free (ak4647);
	return (0);
}

int
ak4647_mixer (ado_card_t * card, ado_mixer_t ** mixer, HW_CONTEXT_T * hwc)
{
	ak4647_context_t *ak4647;
	int32_t status;

	ado_debug (DB_LVL_MIXER, "Initializing AK4647 mixer");

	if ((ak4647 = (ak4647_context_t *)
		ado_calloc (1, sizeof (ak4647_context_t))) == NULL)
	{
		ado_error ("ak4647: no memory (%s)", strerror (errno));
		return (-1);
	}
	if ((status = ado_mixer_create (card, "ak4647", mixer, ak4647)) != EOK)
	{
		ado_free (ak4647);
		return (status);
	}
	ak4647->mixer = *mixer;
	ak4647->hwc = hwc;
	
	if (build_ak4647_mixer (ak4647, ak4647->mixer))
	{
		ado_free (ak4647);
		return (-1);
	}
	
	if (ak4647_reset (ak4647))
	{
		ado_free (ak4647);
		return (-1);
	}

	ado_mixer_set_reset_func (ak4647->mixer, ak4647_reset);
	ado_mixer_set_destroy_func (ak4647->mixer, ak4647_destroy);

	return (0);
}
