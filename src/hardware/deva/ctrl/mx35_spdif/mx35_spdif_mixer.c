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


struct mx35_spdif_context;

#define	 MIXER_CONTEXT_T struct mx35_spdif_context
#include <audio_driver.h>
#include <string.h>
#include "mx35_spdif.h"


typedef struct mx35_spdif_context {
	ado_mixer_t *mixer;
	HW_CONTEXT_T *hwc;
} mx35_spdif_context_t;


static int32_t
build_mx35_spdif_mixer (MIXER_CONTEXT_T * mx35_spdif, ado_mixer_t * mixer)
{
	return (0);		
}

static ado_mixer_reset_t mx35_spdif_reset;
static int
mx35_spdif_reset (MIXER_CONTEXT_T * mx35_spdif)
{
	return (0);
}

static ado_mixer_destroy_t mx35_spdif_destroy;
static int
mx35_spdif_destroy (MIXER_CONTEXT_T * mx35_spdif)
{
	ado_debug (DB_LVL_MIXER, "Destroying SPDIF dummy mixer");
	ado_free (mx35_spdif);
	return (0);
}


int
mx35_spdif_mixer (ado_card_t * card, ado_mixer_t ** mixer, HW_CONTEXT_T * hwc)
{
	mx35_spdif_context_t *mx35_spdif;
	int32_t status;

	ado_debug (DB_LVL_MIXER, "Initializing SPDIF dummy mixer");

	if ((mx35_spdif = (mx35_spdif_context_t *)
		ado_calloc (1, sizeof (mx35_spdif_context_t))) == NULL)
	{
		ado_error ("mx35_spdif: no memory (%s)", strerror (errno));
		return (-1);
	}

	if ((status = ado_mixer_create (card,
		"mx35_spdif", mixer, mx35_spdif)) != EOK)
	{
		ado_free (mx35_spdif);
		return (status);
	}
	mx35_spdif->mixer = *mixer;
	mx35_spdif->hwc = hwc;
	
	if (build_mx35_spdif_mixer (mx35_spdif, mx35_spdif->mixer))
	{
		ado_free (mx35_spdif);
		return (-1);
	}
	
	if (mx35_spdif_reset (mx35_spdif))
	{
		ado_free (mx35_spdif);
		return (-1);
	}

	ado_mixer_set_reset_func (mx35_spdif->mixer, mx35_spdif_reset);
	ado_mixer_set_destroy_func (mx35_spdif->mixer, mx35_spdif_destroy);

	return (0);
}
