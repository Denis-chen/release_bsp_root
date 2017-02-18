/*
 * $QNXLicenseC: 
 * Copyright 2007, QNX Software Systems.  
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

#include "startup.h"


void mx35_add_esdhc(void)
{
	hwi_add_device(HWI_ITEM_BUS_UNKNOWN, HWI_ITEM_DEVCLASS_DISK, "eSDHC", 0);
	hwi_add_location(0x53FB4000, 0x4000, 0, hwi_find_as(0x53FB4000, 0));
	hwi_add_irq(7);
	hwi_add_inputclk(100000000, 1);

	hwi_add_device(HWI_ITEM_BUS_UNKNOWN, HWI_ITEM_DEVCLASS_DISK, "eSDHC", 0);
	hwi_add_location(0x53FB8000, 0x4000, 0, hwi_find_as(0x53FB8000, 0));
	hwi_add_irq(8);
	hwi_add_inputclk(100000000, 1);

	hwi_add_device(HWI_ITEM_BUS_UNKNOWN, HWI_ITEM_DEVCLASS_DISK, "eSDHC", 0);
	hwi_add_location(0x53FBC000, 0x4000, 0, hwi_find_as(0x53FBC000, 0));
	hwi_add_irq(9);
	hwi_add_inputclk(100000000, 1);
}

void
init_hwinfo()
{
	if ((in32(0x40) & 0x0F) < 2)
		return;

	/*
	 * Add eSDHC
	 */
	mx35_add_esdhc();
}

