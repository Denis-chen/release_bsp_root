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

#include "startup.h"
#include <arm/mx35.h>

/*
 * Clock values
 */
uint64_t mx35_arm_clock = 0;
uint64_t mx35_ahb_clock = 0;

unsigned mx31_per_clock = 0;

/*
 * Work out settings for automotive path clock source
 */
static void mx35_auto_calc_clocks(unsigned auto_mux)
{
	switch (auto_mux) {
		case 0:
			mx35_arm_clock = 399000000;
			mx35_ahb_clock = 133000000;
			break;
		case 1:
			mx35_arm_clock = 266000000;
			mx35_ahb_clock = 133000000;
			break;
		case 2:
			mx35_arm_clock = 133000000;
			mx35_ahb_clock = 133000000;
			break;
		case 4:
			mx35_arm_clock = 399000000;
			mx35_ahb_clock = 66500000;
			break;
		case 5:
			mx35_arm_clock = 266000000;
			mx35_ahb_clock = 66500000;
			break;
		case 6:
			mx35_arm_clock = 133000000;
			mx35_ahb_clock = 66500000;
			break;
		default:
			kprintf("Unknown AUTO_MUX_DIV! Insane clock settings?\n");
	}
}

/*
 * Work out settings for consumer path clock source
 */
static void mx35_con_calc_clocks(unsigned con_mux)
{
	switch (con_mux) {
		case 0:
			mx35_arm_clock = 532000000;
			mx35_ahb_clock = 133000000;
			break;
		case 1:
			mx35_arm_clock = 399000000;
			mx35_ahb_clock = 133000000;
			break;
		case 2:
			mx35_arm_clock = 266000000;
			mx35_ahb_clock = 133000000;
			break;
		case 6:
			mx35_arm_clock = 133000000;
			mx35_ahb_clock = 133000000;
			break;
		case 7:
			mx35_arm_clock = 665000000;
			mx35_ahb_clock = 133000000;
			break;
		case 8:
			mx35_arm_clock = 532000000;
			mx35_ahb_clock = 66500000;
			break;
		case 9:
			mx35_arm_clock = 399000000;
			mx35_ahb_clock = 66500000;
			break;
		case 10:
			mx35_arm_clock = 266000000;
			mx35_ahb_clock = 66500000;
			break;
		case 14:
			mx35_arm_clock = 133000000;
			mx35_ahb_clock = 66500000;
			break;
		default:
			kprintf("Unknown CON_MUX_DIV! Insane clock settings?\n");
	}
}


void mx35_calc_clocks(unsigned ckih)
{
	uint32_t pdr0, pdr4;
	uint64_t ref_clk, mfi, mfn, mfn0, mfd, pd, mpll, upll;

	chip_access(MX35_CCM_BASE, 0, 0, 0x80);

	mfn = chip_read32(MX35_CCM_MPCTL);
	mfn0 = chip_read32(MX35_CCM_PPCTL);
	pdr0 = chip_read32(MX35_CCM_PDR0);
	pdr4 = chip_read32(MX35_CCM_PDR4);
	
	/* NFC_FMS set to 2 Kbyte page size */
	chip_write32(MX35_CCM_RCSR, (chip_read32(MX35_CCM_RCSR) | (1 << 8)));

	chip_done();

	/*
	 * work out the MPLL frequency
	 */
	pd  = (mfn >> 26) & 0x0F;
	mfd = (mfn >> 16) & 0x3FF;
	mfi = (mfn >> 10) & 0x0F;
	if (mfi < 5)
		mfi = 5;
	mfn = mfn & 0x3FF;
	ref_clk = ckih;
	mpll = (ref_clk * 2 * (mfi * (mfd + 1) + mfn)) / (mfd + 1) / (pd + 1);

	/*
	 * work out the UPLL frequency
	 */
	pd  = (mfn0 >> 26) & 0x0F;
	mfd = (mfn0 >> 16) & 0x3FF;
	mfi = (mfn0 >> 10) & 0x0F;
	if (mfi < 5)
		mfi = 5;
	mfn0 = mfn0 & 0x3FF;
	ref_clk = ckih;
	upll = (ref_clk * 2 * (mfi * (mfd + 1) + mfn0)) / (mfd + 1) / (pd + 1);

	/*
	 * work out ARM clock and AHB clock
	 */
	if ((pdr0 & 0x1) || (in32(0x40) & 0x0F) >= 2)
		mx35_con_calc_clocks((pdr0 >> 16) & 0xF);
	else
		mx35_auto_calc_clocks((pdr0 >> 9) & 0x7);
		
	/*
	 * work out Peripheral clock
	 */
	if (pdr0 & (1 << 26)) {
		/*
		 * Peripheral clock derived from ARM high-frequency clock
		 * and synched with AHB clock
		 */
		mx31_per_clock = (unsigned)(mx35_arm_clock /
		    ((((pdr4 >> 19) & 0x7) + 1) * (((pdr4 >> 16) & 0x7) + 1)));
	}
	else {
		/*
		 * Peripheral clock derived from AHB divided clock
		 */
		mx31_per_clock = (unsigned)(mx35_ahb_clock /
		    (((pdr0 >> 12) & 0x7) + 1));
	}
}


