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

#include "3dsmx35.h"
	.text
	.align 2

.globl _start

_start:

# Platform Initialize
	mov	r0, #0			/* MMU mandatory */
	mcr	p15, 0, r0, c1, c0, 0	/* MMU init */
	mcr	p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
	mcr	p15, 0, r0, c8, c7, 0	/* flush v4 TLB */
	mcr	p15, 0, r0, c7, c10, 4	/* Drain the write buffer */

	/* Also setup the Peripheral Port Rename register inside the core */
	ldr	r0, =0x40000015
	mcr	p15, 0, r0, c15, c2, 4

	 /*** L2 Cache setup/invalidation/disable ***/
    /* Disable L2 cache first */
    mov r0, #L2CC_BASE_ADDR
    ldr r2, [r0, #L2_CACHE_CTL_REG]
    bic r2, r2, #0x1
    str r2, [r0, #L2_CACHE_CTL_REG]
    /*
     * Configure L2 Cache:
     * - 128k size(16k way)
     * - 8-way associativity
     * - 0 ws TAG/VALID/DIRTY
     * - 4 ws DATA R/W
     */
    ldr r1, [r0, #L2_CACHE_AUX_CTL_REG]
    and r1, r1, #0xFE000000
    ldr r2, =L2CACHE_PARAM
    orr r1, r1, r2
    str r1, [r0, #L2_CACHE_AUX_CTL_REG]
/* Check silicon revision */
	mov r1, #0x40
	ldr r1, [r1]
	cmp r1, #1
    /* Workaournd for DDR issue:WT*/
    ldreq r1, [r0, #L2_CACHE_DBG_CTL_REG]
    orreq r1, r1, #2
    streq r1, [r0, #L2_CACHE_DBG_CTL_REG]

    /* Invalidate L2 */
    mov r1, #0x000000FF
    str r1, [r0, #L2_CACHE_INV_WAY_REG]
L2_loop:
    /* Poll Invalidate By Way register */
    ldr r2, [r0, #L2_CACHE_INV_WAY_REG]
    cmp r2, #0
    bne L2_loop
    /*** End of L2 operations ***/
##############################################################################
    # AIPS setup - Only setup MPROTx. The PACR default values are good.
	/*
     * Set all MPROTx to be non-bufferable, trusted for R/W,
     * not forced to user-mode.
     */
    ldr r0, =AIPS1_CTRL_BASE_ADDR_W
    ldr r1, =AIPS1_PARAM_W
    str r1, [r0, #0x00]
    str r1, [r0, #0x04]
    ldr r0, =AIPS2_CTRL_BASE_ADDR_W
    str r1, [r0, #0x00]
    str r1, [r0, #0x04]

    /*
     * Clear the on and off peripheral modules Supervisor Protect bit
     * for SDMA to access them. Did not change the AIPS control registers
     * (offset 0x20) access type
     */
    ldr r0, =AIPS1_CTRL_BASE_ADDR_W
    ldr r1, =0x0
    str r1, [r0, #0x40]
    str r1, [r0, #0x44]
    str r1, [r0, #0x48]
    str r1, [r0, #0x4C]
    ldr r1, [r0, #0x50]
    and r1, r1, #0x00FFFFFF
    str r1, [r0, #0x50]

    ldr r0, =AIPS2_CTRL_BASE_ADDR_W
    ldr r1, =0x0
    str r1, [r0, #0x40]
    str r1, [r0, #0x44]
    str r1, [r0, #0x48]
    str r1, [r0, #0x4C]
    ldr r1, [r0, #0x50]
    and r1, r1, #0x00FFFFFF
    str r1, [r0, #0x50]

##############################################################################
# MAX (Multi-Layer AHB Crossbar Switch) setup

    ldr r0, =MAX_BASE_ADDR_W
    /* MPR - priority is M4 > M2 > M3 > M5 > M0 > M1 */
    ldr r1, =MAX_PARAM1
    str r1, [r0, #0x000]        /* for S0 */
    str r1, [r0, #0x100]        /* for S1 */
    str r1, [r0, #0x200]        /* for S2 */
    str r1, [r0, #0x300]        /* for S3 */
    str r1, [r0, #0x400]        /* for S4 */
    /* SGPCR - always park on last master */
    ldr r1, =0x10
    str r1, [r0, #0x010]        /* for S0 */
    str r1, [r0, #0x110]        /* for S1 */
    str r1, [r0, #0x210]        /* for S2 */
    str r1, [r0, #0x310]        /* for S3 */
    str r1, [r0, #0x410]        /* for S4 */
    /* MGPCR - restore default values */
    ldr r1, =0x0
    str r1, [r0, #0x800]        /* for M0 */
    str r1, [r0, #0x900]        /* for M1 */
    str r1, [r0, #0xA00]        /* for M2 */
    str r1, [r0, #0xB00]        /* for M3 */
    str r1, [r0, #0xC00]        /* for M4 */
    str r1, [r0, #0xD00]        /* for M5 */
##############################################################################
# Configure M3IF
	/* Configure M3IF registers */
    ldr r1, =M3IF_BASE_W
    /*
     * M3IF Control Register (M3IFCTL)
     * MRRP[0] = L2CC0 not on priority list (0 << 0)        = 0x00000000
     * MRRP[1] = L2CC1 not on priority list (0 << 0)        = 0x00000000
     * MRRP[2] = MBX not on priority list (0 << 0)        = 0x00000000
     * MRRP[3] = MAX1 not on priority list (0 << 0)        = 0x00000000
     * MRRP[4] = SDMA not on priority list (0 << 0)        = 0x00000000
     * MRRP[5] = MPEG4 not on priority list (0 << 0)       = 0x00000000
     * MRRP[6] = IPU1 on priority list (1 << 6)             = 0x00000040
     * MRRP[7] = IPU2 not on priority list (0 << 0)   = 0x00000000
     *                                                       ------------
     *                                                       0x00000040
     */
    ldr r0, =0x00000040
    str r0, [r1]  /* M3IF control reg */
##############################################################################
# MUX Control setup (To Support 133MHz DDR)
	
	mov r0, #0x2
	ldr r1, =IOMUXC_BASE_ADDR_W
	add r1, r1, #0x368
	add r2, r1, #0x4C8 -0x368
1:      str r0, [r1]
	adds r1, r1, #4
	ble 1b

##############################################################################
# CS0 async mode setup
	ldr r0, =WEIM_CTRL_CS0_W
	ldr r1, =CS0_CSCRU_0x0000CC03
	str r1, [r0, #CSCRU]
	ldr r1, =CS0_CSCRL_0xA0330D01
	str r1, [r0, #CSCRL]
	ldr r1, =CS0_CSCRA_0x00220800 
	str r1, [r0, #CSCRA] 

##############################################################################
# Clock Setup
	ldr r0, =CCM_BASE_ADDR_W

    /* default CLKO to 1/32 of the ARM core*/
    ldr r1, [r0, #CLKCTL_COSR]
    bic r1, r1, #0x00000FF00
    bic r1, r1, #0x0000000FF
    mov r2, #0x00006C00
    add r2, r2, #0x67
    orr r1, r1, r2
    str r1, [r0, #CLKCTL_COSR]
	
    ldr r2, =CCM_CCMR_W
    str r2, [r0, #CLKCTL_CCMR]

    /*check clock path*/
/* Check silicon revision*/
	mov		r1, #0x40
	ldr		r1, [r1]
	cmp		r1, #1
	ldreq	r2, [r0, #CLKCTL_PDR0]
	movne	r2, #1
    tst r2, #0x1           
    ldrne r3, =MPCTL_PARAM_532_W  /* consumer path*/
    ldreq r3, =MPCTL_PARAM_399_W  /* auto path*/

	/*Set MPLL , arm clock and ahb clock*/
    str r3, [r0, #CLKCTL_MPCTL]

    ldr r1, =PPCTL_PARAM_W
    str r1, [r0, #CLKCTL_PPCTL]

	mov		r1, #0x40
	ldr		r1, [r1]
	cmp		r1, #2
	bne		1f
    ldr		r1, =0x00801000
	bic		r1, r1, #0x800000
    str		r1, [r0, #CLKCTL_PDR0]
	b		2f

1:
    ldr r1, [r0, #CLKCTL_PDR0]
    orr r1, r1, #0x800000
    str r1, [r0, #CLKCTL_PDR0]

    ldr r1, =CCM_PDR0_W
    str r1, [r0, #CLKCTL_PDR0]
        
    ldr r1, [r0, #CLKCTL_CGR0]
    orr r1, r1, #0x00300000
    str r1, [r0, #CLKCTL_CGR0]

	ldr r1, [r0, #CLKCTL_CGR1]
	orr r1, r1, #0x00000C00
	orr r1, r1, #0x00000003
	str r1, [r0, #CLKCTL_CGR1]

2:

##############################################################################
# CPLD (CS 5) init
    ldr r0, =WEIM_CTRL_CS5_W
    ldr r1, =CS5_CSCRU_0x0000D843 
    str r1, [r0, #CSCRU]
    ldr r1, =CS5_CSCRL_0x22252521 
    str r1, [r0, #CSCRL]
    ldr r1, =CS5_CSCRA_0x22220A00 
    str r1, [r0, #CSCRA]
##############################################################################
# 32bit DDR Init

	ldr r0, =ESDCTL_BASE_W
    mov r3, #0x2000
    str r3, [r0, #0x0]
    strne r3, [r0, #0x8]

	mov r12, #0x00
	mov r2, #0x00
	mov r1, #RAM_BANK0_BASE
	bl setup_sdram_bank
	cmp r3, #0x0
	orreq r12, r12, #1
	eorne r2, r2, #0x1
	blne setup_sdram_bank
    
    cmp r12, #0
	movne r3, #L2CC_BASE_ADDR
	ldrne r4, [r3, #L2_CACHE_AUX_CTL_REG]
	orrne r4, r4, #0x1000
	strne r4, [r3, #L2_CACHE_AUX_CTL_REG]

	ldr r3, =ESDCTL_DELAY5 
	str r3, [r0, #0x30]
	
	/*------------------------------------------------------*
	 *       Set up stack pointer and jump to C main()      *
 	 *------------------------------------------------------*/

	mov		sp, #0x00040000
	orr		sp, sp, #0x87000000
	bl		main
	b		_start
#############################################################################	
    /*
     * r0: control base, r1: ram bank base
     * r2: ddr type(0:DDR2, 1:MDDR) r3, r4: working
     */
setup_sdram_bank:
	#0xA + 0x4
	mov r3, #0xE 
	tst r2, #0x1
	orreq r3, r3, #0x300 
	str r3, [r0, #0x10]
	bic r3, r3, #0x00A
	str r3, [r0, #0x10]
	beq 2f

	mov r3, #0x20000
1:	subs r3, r3, #1
	bne 1b

2:	tst r2, #0x1
    ldreq r3, =ESDCTL_CONFIG_MDDR
    ldrne r3, =ESDCTL_CONFIG_DDR2
	cmp r1, #RAM_BANK1_BASE
    strlo r3, [r0, #0x4]
    strhs r3, [r0, #0xC]

    ldr r3, =ESDCTL_0x92220000
    strlo r3, [r0, #0x0]
    strhs r3, [r0, #0x8]
	mov r3, #0xDA
    ldr r4, =RAM_PARAM1_MDDR
    strb r3, [r1, r4]

	tst r2, #0x1
	bne skip_set_mode

	cmp r1, #RAM_BANK1_BASE
	ldr r3, =ESDCTL_0xB2220000
    strlo r3, [r0, #0x0]
    strhs r3, [r0, #0x8]
	mov r3, #0xDA
	ldr r4, =RAM_PARAM4_MDDR
    strb r3, [r1, r4]
    ldr r4, =RAM_PARAM5_MDDR
    strb r3, [r1, r4]
    ldr r4, =RAM_PARAM3_MDDR
    strb r3, [r1, r4]
    ldr r4, =RAM_PARAM2_MDDR
    strb r3, [r1, r4]

    ldr r3, =ESDCTL_0x92220000
    strlo r3, [r0, #0x0]
    strhs r3, [r0, #0x8]
	mov r3, #0xDA
    ldr r4, =RAM_PARAM1_MDDR
    strb r3, [r1, r4]

skip_set_mode:
	cmp r1, #RAM_BANK1_BASE
    ldr r3, =ESDCTL_0xA2220000
    strlo r3, [r0, #0x0]
    strhs r3, [r0, #0x8]
    mov r3, #0xDA
    strb r3, [r1]
    strb r3, [r1]

    ldr r3, =ESDCTL_0xB2220000
    strlo r3, [r0, #0x0]
    strhs r3, [r0, #0x8]
	tst r2, #0x1
    ldreq r4, =RAM_PARAM6_MDDR
    ldrne r4, =RAM_PARAM6_DDR2
    mov r3, #0xDA
    strb r3, [r1, r4]
    ldreq r4, =RAM_PARAM7_MDDR
    streqb r3, [r1, r4]
    ldreq r4, =RAM_PARAM3_MDDR
    ldrne r4, =RAM_PARAM3_DDR2
    strb r3, [r1, r4]

	cmp r1, #RAM_BANK1_BASE
    ldr r3, =ESDCTL_0x82226080
    strlo r3, [r0, #0x0]
    strhs r3, [r0, #0x8]

	tst r2, #0x1
	moveq r4, #0x20000
	movne r4, #0x200
1:	subs r4, r4, #1
	bne 1b

	str r3, [r1, #0x100]
	ldr r4, [r1, #0x100]
	cmp r3, r4
	movne r3, #1
	moveq r3, #0

	mov pc, lr	
