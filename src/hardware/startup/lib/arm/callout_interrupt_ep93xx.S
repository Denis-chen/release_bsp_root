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
 * EP93xx specific interrupt callouts
 *
 * interrupt_id_* and interrupt_eoi_* are copied and intermixed with other
 * kernel code during initialisation.
 *
 * They do not follow normal calling conventions, and must fall through
 * to the end, rather than attempting to perform a return instruction.
 *
 * The INTR_GENFLAG_* bits in the intrinfo_entry defines which of the
 * following values can be loaded on entry to these code fragments:
 *
 *	r5 - holds the syspageptr				(INTR_GENFLAG_SYSPAGE  set)
 *	r6 - holds the intrinfo_entry pointer	(INTR_GENFLAG_INTRINFO set)
 *	r7 - holds the interrupt mask count		(INTR_GENFLAG_INTRMASK set)
 *
 * The interrupt_id_* routine returns the (controller-relative) level in r4
 */

#include "callout.ah"

/*
 * -----------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *	r0 - physical address of syspage
 *	r1 - virtual  address of syspage
 *	r2 - offset from start of syspage to start of the callout routine
 *	r3 - offset from start of syspage to read/write data used by callout
 *  r4 - patch data pointer
 * -----------------------------------------------------------------------
 */
interrupt_patch:
	stmdb	sp!,{r4,lr}
	add		r4, r0, r2					// address of callout routine

	/* Map interrupt controller registers */
	mov		r0, #EP93xx_INTR_SIZE
	ldr		r1, Lpaddr
	mov		r2, #0xB00				// PROT_READ|PROT_WRITE|PROT_NOCACHE
	bl		callout_memory_map

	/* Patch the callout routines with the mapped address (in r0) */
	CALLOUT_PATCH	r4, r0, r1, r2, ip
	ldmia	sp!,{r4,pc}

Lpaddr:	.word	EP93xx_INTR_BASE


/*
 * -----------------------------------------------------------------------
 * Identify interrupt source.
 *
 * Returns interrupt number in r4
 *
 * Interrupt priority will be VIC1, 31 to 0 followed by VIC2, 31 to 0 making
 * interrupt source 31 the highest and interrupt source 32 the lowest
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_id_ep93xx, 0, interrupt_patch)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/* Read pending IRQ interrupts from VIC1 (patched with EP93xx_INTR_BASE) */
	mov		r3, #0
	ldr		r2, [ip, #EP93xx_VIC_IRQ_STATUS]
	cmp		r2, #0
	bne		id_scan

	/* Read pending IRQ interrupts from VIC2 (patched with EP93xx_INTR_BASE) */
	add		ip, #0x10000
	mov		r3, #32
	ldr		r2, [ip, #EP93xx_VIC_IRQ_STATUS]
	mov		r4, #-1		/* spurious if VIC2 status is also 0 */
	cmp		r2, #0
	beq		0f

id_scan:
	/* Scan for first set bit (starting with MSb) */
	mov		r4, #32
	mov		r1, #1
1:	
	subs	r4, r4, #1
	blt		0f			/* r4 has rolled to -1 */
	tst		r2, r1, lsl r4
	beq		1b

	/* clear (and disable/mask) the interrupt */
	mov		r1, r1, lsl r4
	str		r1, [ip, #EP93xx_VIC_INT_CLEAR]
	add		r4, r4, r3
0:

CALLOUT_END(interrupt_id_ep93xx)

/*
 * -----------------------------------------------------------------------
 * Acknowledge specified interrupt
 *
 * On entry:
 *	r4 contains the interrupt number
 *	r7 contains the interrupt mask count
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_eoi_ep93xx, 0, interrupt_patch)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/* Only unmask interrupt if mask count is zero */
	teq		r7, #0
	bne		0f

	/* normalize the vector to the appropriate controller */
	mov		r1, r4		// don't modify r4
	cmp		r1, #32
	blt		1f
	sub		r1, #32
	add		ip, #0x10000
1:
	mov		r2, #1
	mov		r2, r2, lsl r1
	
	/* re-enable (unmask) the interrupt */
	ldr		r1, [ip, #EP93xx_VIC_INT_ENABLE]
	orr		r1, r1, r2
	str		r1, [ip, #EP93xx_VIC_INT_ENABLE]
0:
CALLOUT_END(interrupt_eoi_ep93xx)


/*
 * -----------------------------------------------------------------------
 * Mask specified interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_mask_ep93xx, 0, interrupt_patch)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/* normalize the vector to the appropriate controller */
	cmp		r1, #32
	blt		1f
	sub		r1, #32
	add		ip, #0x10000

1:
	/* Clear enable for bit (1 << interrupt_number) */
	mov		r2, #1
	mov		r2, r2, lsl r1
	str		r2, [ip, #EP93xx_VIC_INT_CLEAR]
	mov		r0, #0
	mov		pc, lr

CALLOUT_END(interrupt_mask_ep93xx)


/*
 * -----------------------------------------------------------------------
 * Unmask specified interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_unmask_ep93xx, 0, interrupt_patch)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/* normalize the vector to the appropriate controller */
	cmp		r1, #32
	blt		1f
	sub		r1, #32
	add		ip, #0x10000

1:
	/* Set enable for bit (1 << interrupt_number) */
	mov		r2, #1
	mov		r2, r2, lsl r1
	ldr		r1, [ip, #EP93xx_VIC_INT_ENABLE]
	orr		r1, r1, r2
	str		r1, [ip, #EP93xx_VIC_INT_ENABLE]
	mov		r0, #0
	mov		pc, lr

CALLOUT_END(interrupt_unmask_ep93xx)

/*
 * -----------------------------------------------------------------------
 * interrupt config
 *
 * We don't want anyone to be able to attach to a cascade vector. Normally
 * the config table would give us this table but hack it in for now since we
 * aren't using the vector/config tables for this device yet.
 * Vectors not attachable are for the GPIO's 59, 20, 21, 22
 
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - config flag value
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_config_ep93xx, 0, 0)

	mov		r0, #INTR_CONFIG_FLAG_DISALLOWED

	cmp		r0, #59
	beq		0f
	cmp		r1, #20
	beq		0f
	cmp		r1, #21
	beq		0f
	cmp		r1, #22
	beq		0f
	
	/* no flags to return */
	mov		r0, #0
0:
	mov		pc, lr

CALLOUT_END(interrupt_config_ep93xx)

