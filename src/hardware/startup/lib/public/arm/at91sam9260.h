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
 
 
#ifndef _AT91SAM9260_H__
#define _AT91SAM9260_H__


#include <arm/at91sam9xx.h>

/* Internal Memory Mapping */
#define AT91SAM9260_BOOT_BASE           0x00000000
#define AT91SAM9260_ITCM_BASE           0x00100000
#define AT91SAM9260_DTCM_BASE           0x00200000
#define AT91SAM9260_SRAM_BASE           0x00300000
#define AT91SAM9260_ROM_BASE            0x00400000
#define AT91SAM9260_LCD_BASE            0x00500000
#define AT91SAM9260_UDP_RAM_BASE        0x00600000

/* External Bus Interface Mapping */
#define AT91SAM9260_EBI0_CS0_BASE       0x10000000
#define AT91SAM9260_EBI0_CS1_BASE       0x20000000  
#define AT91SAM9260_EBI0_CS2_BASE       0x30000000
#define AT91SAM9260_EBI0_CS3_BASE       0x40000000  
#define AT91SAM9260_EBI0_CS4_BASE       0x50000000  
#define AT91SAM9260_EBI0_CS5_BASE       0x60000000  

/* Peripheral Mapping */
#define AT91SAM9260_TC_BASE             0xfffa0000
#define AT91SAM9260_UDP_BASE            0xfffa4000
#define AT91SAM9260_MCI_BASE            0xfffa8000
#define AT91SAM9260_TWI0_BASE           0xfffac000
#define AT91SAM9260_USART0_BASE         0xfffb0000
#define AT91SAM9260_USART1_BASE         0xfffb4000
#define AT91SAM9260_USART2_BASE         0xfffb8000
#define AT91SAM9260_SSC_BASE            0xfffbc000
#define AT91SAM9260_ISI_BASE            0xfffc0000
#define AT91SAM9260_EMAC_BASE           0xfffc4000
#define AT91SAM9260_SPI0_BASE           0xfffc8000
#define AT91SAM9260_SPI1_BASE           0xfffcc000
#define AT91SAM9260_USART3_BASE         0xfffd0000
#define AT91SAM9260_USART4_BASE         0xfffd4000
#define AT91SAM9260_USART5_BASE         0xfffd8000
#define AT91SAM9260_TCx_BASE            0xfffdc000
#define AT91SAM9260_ADC_BASE            0xfffe0000
#define AT91SAM9260_SYSC_BASE           0xffffc000

/* System Controller Mapping Mapping */
#define AT91SAM9260_DMAC_BASE           0xffffe600
#define AT91SAM9260_ECC_BASE            0xffffe800
#define AT91SAM9260_SDRAMC_BASE         0xffffea00
#define AT91SAM9260_SMC_BASE            0xffffec00
#define AT91SAM9260_MATRIX_BASE         0xffffee00
#define AT91SAM9260_AIC_BASE            0xfffff000
#define AT91SAM9260_DBGU_BASE           0xfffff200
#define AT91SAM9260_PIOA_BASE           0xfffff400
#define AT91SAM9260_PIOB_BASE           0xfffff600
#define AT91SAM9260_PIOC_BASE           0xfffff800
#define AT91SAM9260_PIOD_BASE           0xfffffa00
#define AT91SAM9260_PMC_BASE            0xfffffc00
#define AT91SAM9260_RSTC_BASE           0xfffffd00
#define AT91SAM9260_SHDWC_BASE          0xfffffd10
#define AT91SAM9260_RTCC0_BASE          0xfffffd20
#define AT91SAM9260_PIT_BASE            0xfffffd30
#define AT91SAM9260_WDTC_BASE           0xfffffd40
#define AT91SAM9260_SCKCR_BASE          0xfffffd50
#define AT91SAM9260_GPBR_BASE           0xfffffd60
#define AT91SAM9260_RTCC1_BASE          0xfffffe00

#define AT91SAM9260_INTR_EMAC           21

#endif /* #ifndef _AT91SAM9260_H__ */
