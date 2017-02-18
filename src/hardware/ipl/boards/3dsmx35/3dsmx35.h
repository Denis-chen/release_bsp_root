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

#ifndef	__3DSMX35_H_INCLUDED
#define	__3DSMX35_H_INCLUDED

/*
 * -------------------------------------------------------------------------
 * Serial ports
 * -------------------------------------------------------------------------
 */

/* UART1 base address */
#define	MX1_UART1_BASE			0x43F90000
/* UART2 base address */
#define	MX1_UART2_BASE			0x43F94000
#define	MX1_UART_SIZE			0xB8
#define	MX1_UART_BASE			MX1_UART1_BASE
/* UART registers, offset from base address */
#define	MX1_UART_RXDATA			*(volatile unsigned short *) (MX1_UART_BASE + 0x00)		/* Receiver Register */
#define	MX1_UART_TXDATA			*(volatile unsigned short *) (MX1_UART_BASE + 0x40)		/* Transmitter Register */
#define	MX1_UART_CR1			*(volatile unsigned short *) (MX1_UART_BASE + 0x80)		/* Control Register 1 */
#define	MX1_UART_CR2			*(volatile unsigned short *) (MX1_UART_BASE + 0x84)		/* Control Register 2 */
#define	MX1_UART_CR3			*(volatile unsigned short *) (MX1_UART_BASE + 0x88)		/* Control Register 3 */
#define	MX1_UART_CR4			*(volatile unsigned short *) (MX1_UART_BASE + 0x8C)		/* Control Register 4 */
#define	MX1_UART_FCR			*(volatile unsigned short *) (MX1_UART_BASE + 0x90)		/* FIFO Control Register */
#define	MX1_UART_SR1			*(volatile unsigned short *) (MX1_UART_BASE + 0x94)		/* Status Register 1 */
#define	MX1_UART_SR2			*(volatile unsigned short *) (MX1_UART_BASE + 0x98)		/* Status Register 2 */
#define	MX1_UART_ESC			*(volatile unsigned short *) (MX1_UART_BASE + 0x9C)		/* Escape Character Register */
#define	MX1_UART_TIM			*(volatile unsigned short *) (MX1_UART_BASE + 0xA0)		/* Escape Timer Register */
#define	MX1_UART_BIR			*(volatile unsigned short *) (MX1_UART_BASE + 0xA4)		/* BRM Incremental Register */
#define	MX1_UART_BMR			*(volatile unsigned short *) (MX1_UART_BASE + 0xA8)		/* BRM Modulator Register */
#define	MX1_UART_BRC			*(volatile unsigned short *) (MX1_UART_BASE + 0xAC)		/* Baud Rate Count Register */
#define	MX1_UART_ONEMS			*(volatile unsigned *) (MX1_UART_BASE + 0xB0)           /* contain the value of the UART internal frequency divided by 1000*/
#define	MX1_UART_TS				*(volatile unsigned short *) (MX1_UART_BASE + 0xB4)		/* Test Register */

/* 
 * Receiver Register bits
 */
#define	MX1_URXD_CHARRDY		(1<<15)		/* Character Ready */
#define	MX1_URXD_ERR			(1<<14)		/* Error Detect */
#define	MX1_URXD_OVERRUN		(1<<13)		/* Receiver Overrun */
#define	MX1_URXD_FRMERR			(1<<12)		/* Frame Error */
#define	MX1_URXD_BRK			(1<<11)		/* BREAK detect */
#define	MX1_URXD_PRERR			(1<<10)		/* Parity Error */

/* 
 * Control Register 1 bits
 */
#define	MX1_UCR1_ADEN			(1<<15)		/* Automatic Baud Rate Detection Interrupt Enable */
#define	MX1_UCR1_ADBR			(1<<14)		/* Automatic Detection of Baud Rate */
#define	MX1_UCR1_TRDYEN			(1<<13)		/* Transmitter Ready Interrupt Enable */
#define	MX1_UCR1_IDEN			(1<<12)		/* Idle Condition Detected Interrupt */
#define	MX1_UCR1_ICD_MASK		(3<<10)		/* Idle Condition Detect Mask */
#define	MX1_UCR1_RRDYEN			(1<<9)		/* Receiver Ready Interrupt Enable */
#define	MX1_UCR1_RDMAEN			(1<<8)		/* Receive Ready DMA Enable */
#define	MX1_UCR1_IREN			(1<<7)		/* Infrared Interface Enable */
#define	MX1_UCR1_TXMPTYEN		(1<<6)		/* Transmitter Empty Interrupt Enable */
#define	MX1_UCR1_RTSDEN			(1<<5)		/* RTS Delta Interrupt Enable */
#define	MX1_UCR1_SNDBRK			(1<<4)		/* Send BREAK */
#define	MX1_UCR1_TDMAEN			(1<<3)		/* Transmitter Ready DMA Enable */
#define	MX1_UCR1_UARTCLKEN		(1<<2)		/* UART Clock Enable */
#define	MX1_UCR1_DOZE			(1<<1)		/* UART DOZE State Control */
#define	MX1_UCR1_UARTEN			(1<<0)		/* UART Enable */

/* 
 * Control Register 2 bits
 */
#define	MX1_UCR2_ESCI			(1<<15)		/* Escape Sequence Interrupt Enable */
#define	MX1_UCR2_IRTS			(1<<14)		/* Ignore UART RTS pin */
#define	MX1_UCR2_CTSC			(1<<13)		/* UART CTS pin Control */
#define	MX1_UCR2_CTS			(1<<12)		/* Clear To Send */
#define	MX1_UCR2_ESCEN			(1<<11)		/* Escape Enable */
#define	MX1_UCR2_RTEC_MASK		(3<<9)		/* Request to Send Edge Control Mask */
#define	MX1_UCR2_PREN			(1<<8)		/* Parity Enable */
#define	MX1_UCR2_PROE			(1<<7)		/* Parity Odd/Even */
#define	MX1_UCR2_STPB			(1<<6)		/* Stop Bit */
#define	MX1_UCR2_WS				(1<<5)		/* Word Size */
#define	MX1_UCR2_RTSEN			(1<<4)		/* Request to Send Interrupt Enable */
#define	MX1_UCR2_TXEN			(1<<2)		/* Transmitter Enable */
#define	MX1_UCR2_RXEN			(1<<1)		/* Receiver Enable */
#define	MX1_UCR2_SRST			(1<<0)		/* Software Reset */

/* 
 * Control Register 3 bits
 */
#define	MX1_UCR3_DPEC_MASK		(3<<14)		/* DTR Interrupt Edge Control */
#define	MX1_UCR3_DTREN			(1<<13)		/* Data Terminal Ready Interrupt Enable */
#define	MX1_UCR3_PARERREN		(1<<12)		/* Parity Error Interrupt Enable */
#define	MX1_UCR3_FRAERREN		(1<<11)		/* Frame Error Interrupt Enable */
#define	MX1_UCR3_DSR			(1<<10)		/* Data Set Ready */
#define	MX1_UCR3_DCD			(1<<9)		/* Data Carrier Detect */
#define	MX1_UCR3_RI				(1<<8)		/* Ring Indicator */
#define	MX1_UCR3_RXDSEN			(1<<6)		/* Receive Status Interrupt Enable */
#define	MX1_UCR3_AIRINTEN		(1<<5)		/* Asynchronous IR WAKE Interrupt Enable */
#define	MX1_UCR3_AWAKEN			(1<<4)		/* Asynchronous WAKE Interrupt Enable */
#define	MX1_UCR3_REF25			(1<<3)		/* Reference Frequency 25 MHz */
#define	MX1_UCR3_REF20			(1<<2)		/* Reference Frequency 30 MHz */
#define	MX1_UCR3_INVT			(1<<1)		/* Inverted Infrared Transmission */
#define	MX1_UCR3_BPEN			(1<<0)		/* Preset Registers Enable */

/* 
 * Control Register 4 bits
 */
#define	MX1_UCR4_CTSTL_MASK		(0x3F<<10)	/* CTS Trigger Level (0-32)*/
#define	MX1_UCR4_INVR			(1<<9)		/* Inverted Infrared Reception */
#define	MX1_UCR4_ENIRI			(1<<8)		/* Serial Infrared Interrupt Enable */
#define	MX1_UCR4_WKEN			(1<<7)		/* WAKE Interrupt Enable */
#define	MX1_UCR4_REF16			(1<<6)		/* Reference Frequency 16 MHz */
#define	MX1_UCR4_IRSC			(1<<5)		/* IR Special Case */
#define	MX1_UCR4_TCEN			(1<<3)		/* Transmit Complete Interrupt Enable */
#define	MX1_UCR4_BKEN			(1<<2)		/* BREAK Condition Detected Interrupt Enable */
#define	MX1_UCR4_OREN			(1<<1)		/* Receive Overrun Interrupt Enable */
#define	MX1_UCR4_DREN			(1<<0)		/* Receive Data Ready Interrupt Enable */

#define MX1_UFCR_TXTL_SHIFT     10 
#define MX1_UFCR_RFDIV_1        (5<<7)      /* Reference freq divider (div 1) */
#define MX1_UFCR_RFDIV_2        (4<<7)      /* Reference freq divider (div 2) */
#define MX1_UFCR_RFDIV_3        (3<<7)      /* Reference freq divider (div 3) */
#define MX1_UFCR_RFDIV_4        (2<<7)      /* Reference freq divider (div 4) */
#define MX1_UFCR_RFDIV_5        (1<<7)      /* Reference freq divider (div 5) */
#define MX1_UFCR_RFDIV_6        (0<<7)      /* Reference freq divider (div 6) */
#define MX1_UFCR_RFDIV_7        (6<<7)      /* Reference freq divider (div 7) */
#define MX1_UFCR_DCEDTE         (1<<6) 
#define MX1_UFCR_RXTL_SHIFT     0 
/* 
 * Status Register 1 bits
 */
#define	MX1_USR1_PARITYERR		(1<<15)		/* Parity Error Interrupt Flag */
#define	MX1_USR1_RTSS			(1<<14)		/* RTS Pin Status */
#define	MX1_USR1_TRDY			(1<<13)		/* Transmitter Ready Interrupt/DMA Flag */
#define	MX1_USR1_RTSD			(1<<12)		/* RTS Delta */
#define	MX1_USR1_ESCF			(1<<11)		/* Escape Sequence Interrupt Flag */
#define	MX1_USR1_FRAMERR		(1<<10)		/* Frame Error Interrupt Flag */
#define	MX1_USR1_RRDY			(1<<9)		/* Receiver Ready Interrupt/DMA Flag */
#define	MX1_USR1_RXDS			(1<<6)		/* Receiver IDLE Interrupt Flag */
#define	MX1_USR1_AIRINT			(1<<5)		/* Asynchronous IR WAKE Interrupt Flag */
#define	MX1_USR1_AWAKE			(1<<4)		/* Asynchronous WAKE Interrupt Flag */

/* 
 * Status Register 2 bits
 */
#define	MX1_USR2_ADET			(1<<15)		/* Automatic Baud Rate Detect Complete */
#define	MX1_USR2_TXFE			(1<<14)		/* Transmit Buffer FIFO Empty */
#define	MX1_USR2_DTRF			(1<<13)		/* DTR Edge Triggered Interrupt Flag */
#define	MX1_USR2_IDLE			(1<<12)		/* IDLE Condition */
#define	MX1_USR2_IRINT			(1<<8)		/* Serial Infrared Interrupt Flag */
#define	MX1_USR2_WAKE			(1<<7)		/* WAKE */
#define	MX1_USR2_RTSF			(1<<4)		/* RTS Edge Triggered Interrupt Flag */
#define	MX1_USR2_TXDC			(1<<3)		/* Transmitter Complete */
#define	MX1_USR2_BRCD			(1<<2)		/* BREAK Condition Detected */
#define	MX1_USR2_ORE			(1<<1)		/* Overrun Error */
#define	MX1_USR2_RDR			(1<<0)		/* Receive Data Ready */

/*
 * Test register bits
 */
#define	MX1_UTS_FRCPERR         (1<<13)
#define	MX1_UTS_LOOP            (1<<12)
#define	MX1_UTS_DBGEN           (1<<11)
#define	MX1_UTS_LOOPIR          (1<<10)
#define	MX1_UTS_RXDBG           (1<<9)
#define	MX1_UTS_TXEMPTY         (1<<6)
#define	MX1_UTS_RXEMPTY         (1<<5)
#define	MX1_UTS_TXFULL          (1<<4)
#define	MX1_UTS_RXFULL          (1<<3)
#define	MX1_UTS_SOFTRST         (1<<0)
 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ Various system control registers
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#define AIPS1_CTRL_BASE_ADDR_W    0x43F00000
#define AIPS2_CTRL_BASE_ADDR_W    0x53F00000
#define AIPS1_PARAM_W             0x77777777
#define MAX_BASE_ADDR_W           0x43F04000
#define MAX_PARAM1                0x00302154
#define CLKCTL_BASE_ADDR_W        0x43F0C000
#define ESDCTL_BASE_W             0xB8001000
#define ESDCTL_DELAY5             0x00F49F00
#define ESDCTL_0x92220000         0x92220000
#define ESDCTL_0xA2220000         0xA2220000
#define ESDCTL_0xB2220000         0xB2220000
#define ESDCTL_0x82226080         0x82226080
#define ESDCTL_CONFIG_MDDR     	  0x007FFC3F	
#define ESDCTL_CONFIG_DDR2        0x00295729
#define M3IF_BASE_W               0xB8003000
#define RAM_BANK0_BASE            0x80000000
#define RAM_BANK1_BASE            0x90000000
#define RAM_PARAM1_MDDR           0x00000400
#define RAM_PARAM2_MDDR           0x00000333
#define RAM_PARAM3_MDDR           0x02000400
#define RAM_PARAM3_DDR2			  0x02000000
#define RAM_PARAM4_MDDR           0x04000000
#define RAM_PARAM5_MDDR           0x06000000
#define RAM_PARAM6_MDDR           0x00000233
#define RAM_PARAM6_DDR2           0x00000033
#define RAM_PARAM7_MDDR           0x02000780
#define IOMUXC_BASE_ADDR_W        0x43FAC000
#define CCM_CCMR_W                0x003F4208
#define CCM_PDR0_W                0x00821000
/* Assuming 24MHz input clock */
/*                                     PD             MFD              MFI          MFN */
#define MPCTL_PARAM_399_W         (((1-1) << 26) + ((16-1) << 16) + (8  << 10) + (5 << 0))
#define MPCTL_PARAM_532_W         ((1 << 31) + ((1-1) << 26) + ((12-1) << 16) + (11  << 10) + (1 << 0))
/* UPCTL                               PD             MFD              MFI          MFN */
#define PPCTL_PARAM_W    	      (((1-1) << 26) + ((4-1) << 16) + (6  << 10) + (1  << 0))
#define CCM_BASE_ADDR_W           0x53F80000
#define WEIM_CTRL_CS0_W           0xB8002000
//#define WEIM_CTRL_CS5_W           0x53F80050
#define WEIM_CTRL_CS5_W           0xB8002050
#define CS0_CSCRU_0x0000CC03      0x0000DCF6
#define CS0_CSCRL_0xA0330D01      0x444A4541
#define CS0_CSCRA_0x00220800      0x44443302
#define CS5_CSCRU_0x0000D843      0x0000D843
#define CS5_CSCRL_0x22252521      0x22252521
#define CS5_CSCRA_0x22220A00      0x22220A00
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ L2CC 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#define L2CC_BASE_ADDR		0x30000000
#define L2_CACHE_LINE_SIZE  32
#define L2CACHE_PARAM       0x00030024
#define L2_CACHE_CTL_REG                0x100
#define L2_CACHE_AUX_CTL_REG            0x104
#define L2_CACHE_SYNC_REG               0x730
#define L2_CACHE_INV_LINE_REG           0x770
#define L2_CACHE_INV_WAY_REG            0x77C
#define L2_CACHE_CLEAN_LINE_REG         0x7B0
#define L2_CACHE_CLEAN_INV_LINE_REG     0x7F0
#define L2_CACHE_DBG_CTL_REG     	    0xF40

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ WEIM - offset
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#define CSCRU			0x00
#define CSCRL			0x04
#define CSCRA			0x08

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ UART - offset
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#define URXD			0x00
#define UTXD			0x40
#define UCR1			0x80
#define UCR2			0x84
#define UCR3			0x88
#define UCR4			0x8c
#define UFCR			0x90
#define USR1			0x94
#define USR2			0x98
#define UESC			0x9c
#define UTIM			0xa0
#define UBIR			0xa4
#define UBMR			0xa8
#define UBRC			0xac
#define ONEMS			0xb0
#define UTS			    0xb4

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ CCM - offset
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#define CLKCTL_CCMR                     0x00
#define CLKCTL_PDR0                     0x04
#define CLKCTL_PDR1                     0x08
#define CLKCTL_PDR2                     0x0C
#define CLKCTL_PDR3                     0x10
#define CLKCTL_PDR4                     0x14
#define CLKCTL_RCSR                     0x18
#define CLKCTL_MPCTL                    0x1C
#define CLKCTL_PPCTL                    0x20
#define CLKCTL_ACMR                     0x24
#define CLKCTL_COSR                     0x28
#define CLKCTL_CGR0                     0x2C
#define CLKCTL_CGR1                     0x30
#define CLKCTL_CGR2                     0x34
#define CLKCTL_CGR3                     0x38

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ GPIO - offset
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#define DR			0x00
#define GDIR		0x04
#define PSR			0x08
#define ICR1		0x0c
#define ICR2		0x10
#define IMR			0x14
#define ISR			0x18

#endif	/* __3DSMX35_H_INCLUDED */
