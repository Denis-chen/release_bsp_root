/*
 * $QNXLicenseC:
 * Copyright 2007, 2008, QNX Software Systems. 
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


#ifndef	_ESDHC_INCLUDED
#define	_ESDHC_INCLUDED


#define	__REG8(x)		(*((volatile uint8_t  *)(x)))
#define	__REG16(x)		(*((volatile uint16_t *)(x)))
#define	__REG32(x)		(*((volatile uint32_t *)(x)))


#define __REG32_W(x, v, s, m)			\
		{				\
		uint32_t t =	__REG32(x); 	\
		t = t & ~(m);		\
		t = t | ((v) << (s));	\
		__REG32(x) = t;		\
		}								

/*
 * SDIO/MMC Memory-mapped registers
 */
#define	ESDHC_DMAADR(base)		__REG32(base + 0x00)

#define	ESDHC_BLKCNT_R(base)		__REG16(base + 0x04)
#define	ESDHC_BLKSZ_R(base)		__REG16(base + 0x06)

#define	ESDHC_BLKCNT_W(base, value)	__REG32_W(base + 0x04, value, 16, 0xFFFF0000)
#define	ESDHC_BLKSZ_W(base,value)	__REG32_W(base + 0x04, value, 0, 0x0000FFFF)

#define	ESDHC_CMDARG(base)		__REG32(base + 0x08)

#define	ESDHC_SDCMD_R(base)		__REG16(base + 0x0C)
#define	ESDHC_XFRMODE_R(base)		__REG16(base + 0x0E)

#define	ESDHC_SDCMD_W(base,value)	 __REG32_W(base + 0x0C, value, 16, 0xFFFF0000)
#define	ESDHC_XFRMODE_W(base,value)	__REG32_W(base + 0x0C, value, 0, 0x0000FFFF)
#define ESDHC_SDCMD_W32(base)		__REG32(base + 0x0C)

#define	ESDHC_RESP0(base)		__REG32(base + 0x10)

#define	ESDHC_RESP1(base)		__REG32(base + 0x14)

#define	ESDHC_RESP2(base)		__REG32(base + 0x18)

#define	ESDHC_RESP3(base)		__REG32(base + 0x1C)

#define	ESDHC_BUFDATA(base)		__REG32(base + 0x20)

#define	ESDHC_PSTATE(base)		__REG32(base + 0x24)

#define	ESDHC_WAKECTL_R(base)		__REG8 (base + 0x28)
#define	ESDHC_BLKGAPCTL_R(base)		__REG8 (base + 0x29)
#define	ESDHC_PWRCTL_R(base)		__REG8 (base + 0x2A)
#define	ESDHC_HOSTCTL_R(base)		__REG8 (base + 0x2B)

#define	ESDHC_WAKECTL_W(base,value)	 __REG32_W(base + 0x28, value, 24, 0xFF000000)
#define	ESDHC_BLKGAPCTL_W(base,value)	 __REG32_W(base + 0x28, value, 16, 0x00FF0000)
#define	ESDHC_PWRCTL_W(base,value)	 __REG32_W(base + 0x28, value, 8, 0x0000FF00)
#define	ESDHC_HOSTCTL_W(base,value)	 __REG32_W(base + 0x28, value, 0, 0x000000FF)


#define	ESDHC_SWRST_R(base)		__REG8 (base + 0x2C)
#define	ESDHC_TOCTL_R(base)		__REG8 (base + 0x2D)
#define	ESDHC_CLKCTL_R(base)		__REG16(base + 0x2E)

#define	ESDHC_SWRST_W(base,value)	__REG32_W(base + 0x2C, value, 24, 0xFF000000)
#define	ESDHC_TOCTL_W(base,value)	__REG32_W(base + 0x2C, value, 16, 0x00FF0000)
#define	ESDHC_CLKCTL_W(base,value)	__REG32_W(base + 0x2C, value, 0, 0x0000FFFF)


#define	ESDHC_ERINTSTS_R(base)		__REG16(base + 0x30)
#define	ESDHC_NINTSTS_R(base)		__REG16(base + 0x32)

#define	ESDHC_ERINTSTS_W(base,value)	__REG32_W(base + 0x30, value, 16, 0xFFFF0000)	
#define	ESDHC_NINTSTS_W(base,value)	__REG32_W(base + 0x30, value, 0, 0x0000FFFF)

#define	ESDHC_ERINTEN_R(base)		__REG16(base + 0x34)
#define	ESDHC_NINTEN_R(base)		__REG16(base + 0x36)

#define	ESDHC_ERINTEN_W(base,value)	__REG32_W(base + 0x34, value, 16, 0xFFFF0000)	
#define	ESDHC_NINTEN_W(base, value)	__REG32_W(base + 0x34, value, 0, 0x0000FFFF)

#define	ESDHC_ERINTSIGEN_R(base)	__REG16(base + 0x38)
#define	ESDHC_NINTSIGEN_R(base)		__REG16(base + 0x3A)

#define	ESDHC_ERINTSIGEN_W(base, value)	__REG32_W(base + 0x38, value, 16, 0xFFFF0000)
#define	ESDHC_NINTSIGEN_W(base,value)	__REG32_W(base + 0x38, value, 0, 0x0000FFFF)

#define	ESDHC_AC12ERRSTS(base)		__REG16(base + 0x3C)

#define	ESDHC_CAP(base)			__REG32(base + 0x40)

#define	ESDHC_MCCAP(base)		__REG32(base + 0x48)

#define	ESDHC_SLTINTSTS(base)		__REG16(base + 0xFC)
#define	ESDHC_CTRLRVER(base)		__REG16(base + 0xFE)

#define	ESDHC_WML(base)			__REG32(base + 0x44)

#define	ESDHC_FEVT(base)		__REG32(base + 0x50)

#define	ESDHC_SCR(base)			__REG32(base + 0x40C)



/*
 * Transfer Mode Register (XFRMODE) bit defination
 */
#define	ESDHC_XFRMODE_DMAEN		(1 << 0)	// DMA enable
#define	ESDHC_XFRMODE_BCE		(1 << 1)	// Block Count Enable
#define	ESDHC_XFRMODE_AC12EN	(1 << 2)	// Auto CMD12 Enable
#define	ESDHC_XFRMODE_DATDIR	(1 << 4)	// Data Direction
#define	ESDHC_XFRMODE_MBS		(1 << 5)	// Multiple Block Select

/*
 * Command Register (CMD) bit defination
 */
#define	ESDHC_CMD_RSPLEN0		(0 << 0)	// No response
#define	ESDHC_CMD_RSPLEN136		(1 << 0)	// 136 bit response
#define	ESDHC_CMD_RSPLEN48		(2 << 0)	// 48 bit response
#define	ESDHC_CMD_RSPLEN48b		(3 << 0)	// 48 bit response with busy bit check
#define	ESDHC_CMD_CCCE			(1 << 3)	// Command CRC Check enable
#define	ESDHC_CMD_CICE			(1 << 4)	// Command Index Check enable
#define	ESDHC_CMD_DPS			(1 << 5)	// Data Present
#define	ESDHC_CMD_NORMAL		(0 << 6)	// Normal Command
#define	ESDHC_CMD_CMDIDX(x)		(((x) & 0x3F) << 8)


/*
 * Present State Register (PSTATE) bit defination
 */
#define	ESDHC_PSTATE_WP			(1 << 19)
#define	ESDHC_PSTATE_CD			(1 << 18)
#define	ESDHC_PSTATE_CSS		(1 << 17)
#define	ESDHC_PSTATE_CI			(1 << 16)
#define	ESDHC_PSTATE_BRE		(1 << 11)
#define	ESDHC_PSTATE_BUFWREN	(1 << 10)
#define	ESDHC_PSTATE_RTA		(1 <<  9)
#define	ESDHC_PSTATE_WTA		(1 <<  8)
#define	ESDHC_PSTATE_DLA		(1 <<  2)
#define	ESDHC_PSTATE_DCI		(1 <<  1)
#define	ESDHC_PSTATE_CINH		(1 <<  0)

/*
 * Normal Interrupt status/enable/signal enable bit defination
 */
#define	ESDHC_NINT_CC			(1 << 0)	// Command Complete
#define	ESDHC_NINT_TC			(1 << 1)	// Transfer Complete
#define	ESDHC_NINT_BGE			(1 << 2)	// Block Gap Event
#define	ESDHC_NINT_DMA			(1 << 3)	// DMA Interrupt
#define	ESDHC_NINT_BWR			(1 << 4)	// Buffer Write Ready
#define	ESDHC_NINT_BRR			(1 << 5)	// Buffer Read Ready
#define	ESDHC_NINT_CIN			(1 << 6)	// Card Insertion
#define	ESDHC_NINT_CRM			(1 << 7)	// Card Removal
#define	ESDHC_NINT_CI			(1 << 8)	// Card Interrupt
#define	ESDHC_NINT_EI			(1 << 15)	// Error Interrupt, for status register only

/*
 * Error Interrupt status/enable/signal enable bit defination
 */
#define	ESDHC_ERINT_CTE			(1 << 0)	// Command Timeout Error
#define	ESDHC_ERINT_CCE			(1 << 1)	// Command CRC Error
#define	ESDHC_ERINT_CEBE		(1 << 2)	// Command End Bit Error
#define	ESDHC_ERINT_CIE			(1 << 3)	// Command Index Error
#define	ESDHC_ERINT_DTE			(1 << 4)	// Data Timeout Error
#define	ESDHC_ERINT_DCE			(1 << 5)	// Data CRC Error
#define	ESDHC_ERINT_DEBE		(1 << 6)	// Data End Bit Error
#define	ESDHC_ERINT_CL			(1 << 7)	// Current Limit Error
#define	ESDHC_ERINT_AC12		(1 << 8)	// Auto CMD12 Error

/*
 * Clock Control Register bit defination
 */
#define	ESDHC_CLKCTL_ICE		(1 << 0)	// Internal Clock Enable
#define	ESDHC_CLKCTL_ICS		(1 << 1)	// Internal Clock Stable
#define	ESDHC_CLKCTL_CLKEN		(1 << 2)	// Clock Enable

/*
 * Host Control Register bit defination
 */
#define	ESDHC_HOSTCTL_LEDCTL	(1 << 0)	// LED Control
#define	ESDHC_HOSTCTL_DTW1BIT	(0 << 1)	// Data Bus Width 1 bit
#define	ESDHC_HOSTCTL_DTW4BIT	(1 << 1)	// Data Bus Width 4 bit
#define	ESDHC_HOSTCTL_HSEN		(1 << 2)	// High Speed Enable

/*
 * Power Control Register bit defination
 */
#define	ESDHC_PWRCTL_PWREN		(1 << 0)	// SD Bus Power Enable
#define	ESDHC_PWRCTL_V33		(7 << 1)	// 3.3V

/*
 * Software Reset Register bit defination
 */
#define	ESDHC_SWRST_ALL			(1 << 0)	// Reset All
#define	ESDHC_SWRST_RC			(1 << 1)	// Reset CMD
#define	ESDHC_SWRST_RD			(1 << 2)	// Reset Data

/*
 * Capability Register bit defination
 */
#define	ESDHC_CAP_S18			(1 << 26)	// 1.8V support
#define	ESDHC_CAP_S30			(1 << 25)	// 3.0V support
#define	ESDHC_CAP_S33			(1 << 24)	// 3.3V support
#define	ESDHC_CAP_SRS			(1 << 23)	// Suspend/Resume support
#define	ESDHC_CAP_DMA			(1 << 22)	// DMA support
#define	ESDHC_CAP_HS			(1 << 21)	// High-Speed support
#define	ESDHC_CAP_MBL512		(0 << 16)	// Max block length 512
#define	ESDHC_CAP_MBL2048		(2 << 16)	// Max block length 2048


typedef	struct _esdhc_ext {
	int			pci_hdl;
	void		*pci_dev_hdl;
	uintptr_t	base;
	int			blksz;
	uint32_t		xmode;
	uint32_t	clock;
	void		*hba;
} esdhc_ext_t;

extern int esdhc_init(SIM_HBA *hba);

#endif
