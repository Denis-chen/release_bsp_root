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

// Module Description:  board specific interface

#include <sim_mmc.h>
#include <sim_mx35.h>
#include <fcntl.h>
#include <hw/i2c.h>

static int mx35_3ds_card_sts(mx35_ext_t *mx35)
{
  //TODO: Do I2C access with MCU on MX353DS board. Return 0 if card not present.
  //If card is present, return 1 if card is write protected by switch else return 2.
  return 2;
}

static int mx35_3ds_detect(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	mx35_ext_t	    *mx35;
	int				sts;

	ext    = (SIM_MMC_EXT *)hba->ext;
	mx35 = (mx35_ext_t *)ext->handle;

	if ((sts = mx35_3ds_card_sts(mx35)) != -1) {
		if (sts == 0 )		// No card
			return MMC_FAILURE;

		if (sts == 1 )		// Write protected
			ext->eflags |= MMC_EFLAG_WP;
		else
			ext->eflags &= ~MMC_EFLAG_WP;

		return MMC_SUCCESS;
	} 

	return MMC_FAILURE;
}

static void mx35_3ds_config(SIM_HBA *hba)
{
	CONFIG_INFO		*cfg;

	cfg = (CONFIG_INFO *)&hba->cfg;
	if (!cfg->NumIOPorts) {
		cfg->IOPort_Base[0]   = MX35_SDC_BASE;
		cfg->IOPort_Length[0] = MX35_SDC_SIZE;
		cfg->NumIOPorts = 1;
	} 
	if (!cfg->NumIRQs) 
	{
		cfg->IRQRegisters[0] = MX35_SDC_IRQ;
		cfg->NumIRQs = 1; 
	}
}

int bs_init(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	mx35_ext_t	    *mx35;
   
    mx35_3ds_config(hba);
    if (mx35_attach(hba) != MMC_SUCCESS)
		return MMC_FAILURE;
	ext  = (SIM_MMC_EXT *)hba->ext;	
	mx35 = (mx35_ext_t *)ext->handle;
    mx35->bshdl = open(MC9S08DZ60MLH_I2C_DEVNAME, O_RDWR);
	if (mx35->bshdl < 0)
		return MMC_SUCCESS;
		
	if (mx35_3ds_card_sts(mx35) != -1)
		ext->detect = mx35_3ds_detect;
	
	return MMC_SUCCESS;
}

int bs_dinit(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	mx35_ext_t	    *mx35;

	ext    = (SIM_MMC_EXT *)hba->ext;
	mx35   = (mx35_ext_t *)ext->handle;

    if (mx35->bshdl >= 0)
		close(mx35->bshdl);

	return (CAM_SUCCESS);
}

__SRCVERSION("sim_bs.c $Rev: 140019 $");

