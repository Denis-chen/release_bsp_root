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
 * i.MX35 interrupt controller support.
 */

#include "startup.h"
#include <arm/mx1.h>
#include <arm/mx31.h>
#include <arm/mx35.h>
#include "mx35_3ds_cpld.h"

extern struct callout_rtn	interrupt_id_mx31_gpio;
extern struct callout_rtn	interrupt_eoi_mx31_gpio;
extern struct callout_rtn	interrupt_mask_mx31_gpio;
extern struct callout_rtn	interrupt_unmask_mx31_gpio;

extern struct callout_rtn	interrupt_id_3dsmx35_cpld;
extern struct callout_rtn	interrupt_eoi_3dsmx35_cpld;
extern struct callout_rtn	interrupt_mask_3dsmx35_cpld;
extern struct callout_rtn	interrupt_unmask_3dsmx35_cpld;

static paddr_t mx35_avic_base     = MX31_AVIC_BASE;
static paddr_t mx35_gpio1_base    = MX31_GPIO1_BASE;
static paddr_t mx35_gpio2_base    = MX31_GPIO2_BASE;
static paddr_t mx35_gpio3_base    = MX31_GPIO3_BASE;
static paddr_t mx35_sdma_base     = MX31_SDMA_BASE;
static paddr_t mx35_3ds_cpld_base = MX35_3DS_CPLD_BASE;


const static struct startup_intrinfo	intrs[] = {
	{	_NTO_INTR_CLASS_EXTERNAL, 	// vector base
		64,				// number of vectors
		_NTO_INTR_SPARE,		// cascade vector
		0,				// CPU vector base
		0,				// CPU vector stride
		0,				// flags

		{ INTR_GENFLAG_LOAD_SYSPAGE,	0, &interrupt_id_aitc },
		{ INTR_GENFLAG_LOAD_SYSPAGE | INTR_GENFLAG_LOAD_INTRMASK, 0, &interrupt_eoi_aitc },
		&interrupt_mask_aitc,		// mask   callout
		&interrupt_unmask_aitc,		// unmask callout
		0,				// config callout
		&mx35_avic_base,
	},
	// GPIO 1 interrupt (64-95)
	{	64,				// vector base
		32,				// number of vectors
		52,                         // cascade vector
		0,				// CPU vector base
		0,				// CPU vector stride
		0,				// flags

		{ 0, 0, &interrupt_id_mx31_gpio },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx31_gpio },
		&interrupt_mask_mx31_gpio,	// mask   callout
		&interrupt_unmask_mx31_gpio,	// unmask callout
		0,				// config callout
		&mx35_gpio1_base,
	},
	// GPIO 2 interrupt (96-127)
	{	96,				// vector base
		32,				// number of vectors
		51,                         // cascade vector
		0,				// CPU vector base
		0,				// CPU vector stride
		0,				// flags

		{ 0, 0, &interrupt_id_mx31_gpio },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx31_gpio },
		&interrupt_mask_mx31_gpio,	// mask   callout
		&interrupt_unmask_mx31_gpio,	// unmask callout
		0,				// config callout
		&mx35_gpio2_base,
	},
	// GPIO 3 interrupt (128-159)
	{	128,				// vector base
		32,				// number of vectors
		56,                         // cascade vector
		0,				// CPU vector base
		0,				// CPU vector stride
		0,				// flags

		{ 0, 0, &interrupt_id_mx31_gpio },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx31_gpio },
		&interrupt_mask_mx31_gpio,	// mask   callout
		&interrupt_unmask_mx31_gpio,	// unmask callout
		0,				// config callout
		&mx35_gpio3_base,
	},
	// CPLD Interrupt (160 - 175) 
	{	160,						// vector base
		16,							// number of vectors
		65,                         // cascade vector - GPIO1_1
		0,							// CPU vector base
		0,							// CPU vector stride
		0,							// flags

		{ 0, 0, &interrupt_id_3dsmx35_cpld },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_3dsmx35_cpld },
		&interrupt_mask_3dsmx35_cpld,	// mask   callout
		&interrupt_unmask_3dsmx35_cpld,	// unmask callout
		0,							// config callout
		&mx35_3ds_cpld_base,
	},
};

void init_intrinfo()
{
	int	i;

	/*
	 * Disable all interrupts
	 */
	out32(mx35_avic_base + MX1_AITC_INTENABLEH, 0);
	out32(mx35_avic_base + MX1_AITC_INTENABLEL, 0);

	/*
	 * Disable all GPIO interrupts	
	 */
	out32(mx35_gpio1_base + MX31_GPIO_IMR, 0);
	out32(mx35_gpio2_base + MX31_GPIO_IMR, 0);
	out32(mx35_gpio3_base + MX31_GPIO_IMR, 0);

	/*
	 * Disable all SDMA interrupts	
	 */
	out32(mx35_sdma_base + MX31_SDMA_INTRMASK, 0);
	out32(mx35_sdma_base + MX31_SDMA_INTR, ~0);

	/*
	 * Configure GPIO1_4 (TSC2007 PENIRQ) as an active-low,
	 * level-triggered input
	 */
	out32(mx35_gpio1_base + MX31_GPIO_ICR1, in32(mx35_gpio1_base + MX31_GPIO_ICR1) & ~(3 << 8));
	out32(mx35_gpio1_base + MX35_GPIO_EDR, in32(mx35_gpio1_base + MX35_GPIO_EDR) & ~(1 << 4));

	/*
	 * Configure GPIO1_1 (DEBUG_INT_B) as an active-low,
	 * level-triggered input; this comes from the CPLD on
	 * the debug board
	 */
	out32(mx35_gpio1_base + MX31_GPIO_ICR1, in32(mx35_gpio1_base + MX31_GPIO_ICR1) & ~(3 << 2));
	out32(mx35_gpio1_base + MX35_GPIO_EDR, in32(mx35_gpio1_base + MX35_GPIO_EDR) & ~(1 << 1));

	/*
	 * All interrupt sources generate normal interrupt
	 */
	out32(mx35_avic_base + MX1_AITC_INTTYPEH, 0);
	out32(mx35_avic_base + MX1_AITC_INTTYPEL, 0);

	/*
	 * Only disable level 0 normal interrupt
	 */
	out32(mx35_avic_base + MX1_AITC_NIMASK, 0);

	/*
	 * Set all interrupt priority as highest normal interrupt
	 */
	for (i = MX1_AITC_NIPRIORITY7; i <= MX1_AITC_NIPRIORITY0; i += 4)
		out32(mx35_avic_base + i, ~0);

	/* Disable fast interrupt, clear ABFLAG */
	out32(mx35_avic_base + MX1_AITC_INTCNTL, (1 << 21) | (1 << 25));

	add_interrupt_array(intrs, sizeof(intrs));
}
