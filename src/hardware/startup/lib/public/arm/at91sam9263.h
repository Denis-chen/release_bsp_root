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


#ifndef _AT91SAM9263_H__
#define _AT91SAM9263_H__
/*
 * Atmel AT91SAM9263 SOC with ARM 926ES-J core.
 */

/*
 * System On Chip (SOC) Memory Mapping.
 */

/*
 * Peripheral ID Declaration and Interrupt Number.
 */
#define AT91SAM9263_ID_INUM_FIQ                 0
#define AT91SAM9263_ID_INUM_SYSC                1
#define AT91SAM9263_ID_INUM_PIOA                2
#define AT91SAM9263_ID_INUM_PIOB                3
#define AT91SAM9263_ID_INUM_PIOC                4
#define AT91SAM9263_ID_INUM_PIOD                AT91SAM9263_ID_INUM_PIOC
#define AT91SAM9263_ID_INUM_PIOE                AT91SAM9263_ID_INUM_PIOC
#define AT91SAM9263_ID_INUM_USART0              7
#define AT91SAM9263_ID_INUM_USART1              8
#define AT91SAM9263_ID_INUM_USART2              9
#define AT91SAM9263_ID_INUM_MCI0                10
#define AT91SAM9263_ID_INUM_MCI1                11
#define AT91SAM9263_ID_INUM_CAN                 12
#define AT91SAM9263_ID_INUM_TWI                 13
#define AT91SAM9263_ID_INUM_SPI0                14
#define AT91SAM9263_ID_INUM_SPI1                15
#define AT91SAM9263_ID_INUM_SSC0                16
#define AT91SAM9263_ID_INUM_SSC1                17
#define AT91SAM9263_ID_INUM_AC97C               18
#define AT91SAM9263_ID_INUM_TC0                 19
#define AT91SAM9263_ID_INUM_TC1                 AT91SAM9263_ID_INUM_TC0
#define AT91SAM9263_ID_INUM_TC2                 AT91SAM9263_ID_INUM_TC0
#define AT91SAM9263_ID_INUM_PWMC                20
#define AT91SAM9263_ID_INUM_EMAC                21
#define AT91SAM9263_ID_INUM_2DGE                23
#define AT91SAM9263_ID_INUM_UDP                 24
#define AT91SAM9263_ID_INUM_ISI                 25
#define AT91SAM9263_ID_INUM_LCDC                26
#define AT91SAM9263_ID_INUM_DMA                 27
#define AT91SAM9263_ID_INUM_UHP                 29
#define AT91SAM9263_ID_INUM_IRQ0                30
#define AT91SAM9263_ID_INUM_IRQ1                31
    
  /* Internal Memory Mapping */
#define AT91SAM9263_BOOT_BASE                   0x00000000
#define AT91SAM9263_ITCM_BASE                   0x00100000
#define AT91SAM9263_DTCM_BASE                   0x00200000
#define AT91SAM9263_SRAM_BASE                   0x00300000
#define AT91SAM9263_ROM_BASE                    0x00400000
#define AT91SAM9263_SRAM0_BASE                  0x00500000
#define AT91SAM9263_LCD_CONTROLLER_BASE         0x00700000
#define AT91SAM9263_DMAC_BASE                   0x00800000
#define AT91SAM9263_USB_HOST_BASE               0x00a00000

#define AT91SAM9263_BOOT_SIZE                   0x00100000
#define AT91SAM9263_ITCM_SIZE                   0x00100000
#define AT91SAM9263_DTCM_SIZE                   0x00100000
#define AT91SAM9263_SRAM_SIZE                   0x00100000
#define AT91SAM9263_ROM_SIZE                    0x00100000
#define AT91SAM9263_SRAM0_SIZE                  0x00100000
#define AT91SAM9263_LCD_CONTROLLER_SIZE         0x00100000
#define AT91SAM9263_DMAC_SIZE                   0x00100000
#define AT91SAM9263_USB_HOST_SIZE               0x00100000

  /* External Bus Interface 0 & 1 Mapping*/
#define AT91SAM9263_EBI0_CS0_BASE               0x10000000
#define AT91SAM9263_EBI0_CS1_BASE               0x20000000  /* SDRAMC    */
#define AT91SAM9263_EBI0_CS2_BASE               0x30000000
#define AT91SAM9263_EBI0_CS3_BASE               0x40000000  /* NANDFlash */
#define AT91SAM9263_EBI0_CS4_BASE               0x50000000  /* Compact Flash Slot 0 */
#define AT91SAM9263_EBI0_CS5_BASE               0x60000000  /* Compact Flash Slot 1 */
#define AT91SAM9263_EBI1_CS0_BASE               0x70000000
#define AT91SAM9263_EBI1_CS1_BASE               0x80000000  /* SDRAMC    */
#define AT91SAM9263_EBI1_CS2_BASE               0x90000000  /* NANDFlash */

#define AT91SAM9263_EBIX_CSX_MAX_SIZE           0x10000000  /* 256 MB           */

  /* Peripheral Mapping */
#define AT91SAM9263_UDP_BASE                    0xfff78000
#define AT91SAM9263_TC_BASE                     0xfff7c000
#define AT91SAM9263_MCI0_BASE                   0xfff80000
#define AT91SAM9263_MCI1_BASE                   0xfff84000
#define AT91SAM9263_TWI_BASE                    0xfff88000
#define AT91SAM9263_USART0_BASE                 0xfff8c000
#define AT91SAM9263_USART1_BASE                 0xfff90000
#define AT91SAM9263_USART2_BASE                 0xfff94000
#define AT91SAM9263_SSC0_BASE                   0xfff98000
#define AT91SAM9263_SSC1_BASE                   0xfff9c000
#define AT91SAM9263_AC97C_BASE                  0xfffa0000
#define AT91SAM9263_SPI0_BASE                   0xfffa4000
#define AT91SAM9263_SPI1_BASE                   0xfffa8000
#define AT91SAM9263_CAN0_BASE                   0xfffac000
#define AT91SAM9263_PWMC_BASE                   0xfffb8000
#define AT91SAM9263_EMAC_BASE                   0xfffbc000
#define AT91SAM9263_ISI_BASE                    0xfffc4000
#define AT91SAM9263_2DGC_BASE                   0xfffc8000

/* System Controller Mapping Mapping */
#define AT91SAM9263_ECC0_BASE                   0xffffe000
#define AT91SAM9263_SDRAMC0_BASE                0xffffe200
#define AT91SAM9263_SMC0_BASE                   0xffffe400
#define AT91SAM9263_ECC1_BASE                   0xffffe600
#define AT91SAM9263_SDRAMC1_BASE                0xffffe800
#define AT91SAM9263_SMC1_BASE                   0xffffea00
#define AT91SAM9263_MATRIX_BASE                 0xffffec00
#define AT91SAM9263_CCFG_BASE                   0xffffed10
#define AT91SAM9263_DBGU_BASE                   0xffffee00
#define AT91SAM9263_AIC_BASE                    0xfffff000
#define AT91SAM9263_PIOA_BASE                   0xfffff200
#define AT91SAM9263_PIOB_BASE                   0xfffff400
#define AT91SAM9263_PIOC_BASE                   0xfffff600
#define AT91SAM9263_PIOD_BASE                   0xfffff800
#define AT91SAM9263_PIOE_BASE                   0xfffffa00
#define AT91SAM9263_PMC_BASE                    0xfffffc00
#define AT91SAM9263_RSTC_BASE                   0xfffffd00
#define AT91SAM9263_SHDWC_BASE                  0xfffffd10
#define AT91SAM9263_RTT0_BASE                   0xfffffd20
#define AT91SAM9263_PIT_BASE                    0xfffffd30
#define AT91SAM9263_WDT_BASE                    0xfffffd40
#define AT91SAM9263_RTT1_BASE                   0xfffffd50
#define AT91SAM9263_GPBR_BASE                   0xfffffd60

/* DMAC */
#define AT91SAM9263_INTR_DMAC                   27

#define AT91SAM9263_INTR_EMAC                   21

#include <arm/at91sam9xx.h>

#endif /* #ifndef _AT91SAM9263_H__ */
