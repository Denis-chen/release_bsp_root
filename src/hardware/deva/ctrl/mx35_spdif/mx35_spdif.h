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


#ifndef __MX35_SPDIF_H
#define __MX35_SPDIF_H

/*
 *  Definitions for i.MX35 SPDIF audio setup
 */

struct	mx35_spdif;
#define  HW_CONTEXT_T struct mx35_spdif
#define  PCM_SUBCHN_CONTEXT_T   ado_pcm_subchn_t
#include <audio_driver.h>
#include <hw/dma.h>
#include <string.h>
#include "proto.h"

typedef volatile uint32_t vuint32_t;
typedef volatile uint16_t vuint16_t;
typedef volatile uint8_t vuint8_t;

#define SDMA_PLAYBACK_CHANNEL   1
#define SDMA_CAPTURE_CHANNEL    2

/* experimental: uses SDMA script in SDMA controller RAM */
#define SDMA_SPDIF_PLAYBACK_CHANNEL   3
#define SDMA_SPDIF_CAPTURE_CHANNEL    4

#define SPDIF_BASE		0x50028000
#define SPDIF_IRQ		47


typedef struct spdif
{
    vuint32_t   scr;		/* Offset: 0x00 */
    vuint32_t   srcd;
    vuint32_t   srpc;
    vuint32_t   sie;
    vuint32_t   sisc;		/* Offset: 0x10 */
    vuint32_t   srl;
    vuint32_t   srr;
    vuint32_t   srcsh;
    vuint32_t   srcsl;		/* Offset: 0x20 */
    vuint32_t   sru;
    vuint32_t   srq;
    vuint32_t   stl;
    vuint32_t   str;		/* Offset: 0x30 */
    vuint32_t   stcsch;
    vuint32_t   stcscl;
    vuint32_t   stcsph;
    vuint32_t   stcspl;		/* Offset: 0x40 */
    vuint32_t   srfm;
    vuint32_t	reserved[2];
    vuint32_t   stc;		/* Offset: 0x50 */
} spdif_t;


/* SPDIF Configuration register */
#define SCR_RXFIFO_CTL_ZERO		(1 << 23)
#define SCR_RXFIFO_OFF			(1 << 22)
#define SCR_RXFIFO_RST			(1 << 21)
#define SCR_RXFIFO_FSEL_BIT		(19)
#define SCR_RXFIFO_FSEL_MASK	(0x3 << SCR_RXFIFO_FSEL_BIT)
#define SCR_RXFIFO_AUTOSYNC		(1 << 18)
#define SCR_TXFIFO_AUTOSYNC		(1 << 17)
#define SCR_TXFIFO_ESEL_BIT		(15)
#define SCR_TXFIFO_ESEL_MASK	(0x3 << SCR_TXFIFO_FSEL_BIT)
#define SCR_LOW_POWER			(1 << 13)
#define SCR_SOFT_RESET			(1 << 12)
#define SCR_TXFIFO_ZERO			(0 << 10)
#define SCR_TXFIFO_NORMAL		(1 << 10)
#define SCR_TXFIFO_ONESAMPLE	(1 << 11)
#define SCR_DMA_RX_EN			(1 << 9)
#define SCR_DMA_TX_EN			(1 << 8)
#define SCR_VAL_CLEAR			(1 << 5)
#define SCR_TXSEL_OFF			(0 << 2)
#define SCR_TXSEL_RX			(1 << 2)
#define SCR_TXSEL_NORMAL		(0x5 << 2)
#define SCR_USRC_SEL_NONE		(0x0)
#define SCR_USRC_SEL_RECV		(0x1)
#define SCR_USRC_SEL_CHIP		(0x3)

/* SPDIF CDText control */
#define SRCD_CD_USER_OFFSET		1
#define SRCD_CD_USER			(1 << SRCD_CD_USER_OFFSET)

/* SPDIF Phase Configuration register */
#define SRPC_DPLL_LOCKED		(1 << 6)
#define SRPC_CLKSRC_SEL_OFFSET	7
#define SRPC_CLKSRC_SEL_LOCKED	5
#define SRPC_GAINSEL_OFFSET	3

enum spdif_gainsel {
	GAINSEL_MULTI_24 = 0,
	GAINSEL_MULTI_16,
	GAINSEL_MULTI_12,
	GAINSEL_MULTI_8,
	GAINSEL_MULTI_6,
	GAINSEL_MULTI_4,
	GAINSEL_MULTI_3,
	GAINSEL_MULTI_MAX,
};

#define SPDIF_DEFAULT_GAINSEL	GAINSEL_MULTI_8

/*static unsigned int gainsel_multi[GAINSEL_MULTI_MAX] = {
	24 * 1024, 16 * 1024, 12 * 1024,
	8 * 1024, 6 * 1024, 4 * 1024,
	3 * 1024,
};
*/

/* SPDIF interrupt mask define */
#define INT_DPLL_LOCKED		(1 << 20)
#define INT_TXFIFO_UNOV		(1 << 19)
#define INT_TXFIFO_RESYNC	(1 << 18)
#define INT_CNEW		(1 << 17)
#define INT_VAL_NOGOOD		(1 << 16)
#define INT_SYM_ERR		(1 << 15)
#define INT_BIT_ERR		(1 << 14)
#define INT_URX_FUL		(1 << 10)
#define INT_URX_OV		(1 << 9)
#define INT_QRX_FUL		(1 << 8)
#define INT_QRX_OV		(1 << 7)
#define INT_UQ_SYNC		(1 << 6)
#define INT_UQ_ERR		(1 << 5)
#define INT_RX_UNOV		(1 << 4)
#define INT_RX_RESYNC		(1 << 3)
#define INT_LOSS_LOCK		(1 << 2)
#define INT_TX_EMPTY		(1 << 1)
#define INT_RXFIFO_FUL		(1 << 0)

/* SPDIF Clock register */
#define STC_SYSCLK_DIV_OFFSET	11
#define STC_TXCLK_SRC_OFFSET	8
#define STC_TXCLK_DIV_OFFSET	0

#define SPDIF_CSTATUS_BYTE	6
#define SPDIF_UBITS_SIZE	96
#define SPDIF_QSUB_SIZE		(SPDIF_UBITS_SIZE/8)

enum spdif_clk_accuracy {
	SPDIF_CLK_ACCURACY_LEV2 = 0,
	SPDIF_CLK_ACCURACY_LEV1 = 2,
	SPDIF_CLK_ACCURACY_LEV3 = 1,
	SPDIF_CLK_ACCURACY_RESV = 3
};

/* SPDIF clock source */
enum spdif_clk_src {
	SPDIF_CLK_SRC1 = 0,
	SPDIF_CLK_SRC2,
	SPDIF_CLK_SRC3,
	SPDIF_CLK_SRC4,
	SPDIF_CLK_SRC5,
};

enum spdif_max_wdl {
	SPDIF_MAX_WDL_20,
	SPDIF_MAX_WDL_24
};

enum spdif_wdl {
	SPDIF_WDL_DEFAULT = 0,
	SPDIF_WDL_FIFTH = 4,
	SPDIF_WDL_FOURTH = 3,
	SPDIF_WDL_THIRD = 2,
	SPDIF_WDL_SECOND = 1,
	SPDIF_WDL_MAX = 5
};

#define CCM_BASE    0x53F80000

typedef struct ccm
{
    vuint32_t   ccmr;
    vuint32_t   pdr0;   
    vuint32_t   pdr1;
    vuint32_t   pdr2;
    vuint32_t   pdr3;
    vuint32_t   pdr4;
    vuint32_t   rcsr;   
    vuint32_t   mpctl;
    vuint32_t   ppctl;
    vuint32_t   acmr;
    vuint32_t   cosr;  
    vuint32_t   cgr0;
    vuint32_t   cgr1;   
    vuint32_t   cgr2;
    vuint32_t   cgr3;
    vuint32_t   reserved0;
    vuint32_t   dcvr0; 
    vuint32_t   dcvr1;
    vuint32_t   dcvr2;  
    vuint32_t   dcvr3;
    vuint32_t   ltr0;   
    vuint32_t   ltr1;
    vuint32_t   ltr2;   
    vuint32_t   ltr3;
    vuint32_t   ltbr0;  
    vuint32_t   ltbr1;
    vuint32_t   pmcr0;  
    vuint32_t   pmcr1;
    vuint32_t   pmcr2;
    vuint32_t   reserved1;
    vuint32_t   reserved2;
} ccm_t;


typedef struct mx35_spdif_stream {
	ado_pcm_subchn_t	*pcm_subchn;
	ado_pcm_cap_t		pcm_caps;
	ado_pcm_hw_t		pcm_funcs;
	ado_pcm_config_t	*pcm_config;
	uint32_t			cur_frag;
	uint32_t			cur_pos;
	int					frag_complete;
	dma_addr_t          dma_buf;
	struct sigevent     sdma_event;
	void                *pulse;
	void                *dma_chn;
	int					intr_id;
} mx35_spdif_strm_t;

typedef struct mx35_spdif {
	ado_card_t			*card;
	ado_mutex_t			hw_lock;
	ado_pcm_t 			*pcm1;
	ado_mixer_t			*mixer;
	spdif_t				*spdif;
	mx35_spdif_strm_t	play_strm;
	mx35_spdif_strm_t	cap_strm;
    intrspin_t          spinlock;
	unsigned int		ch_status[4];
	vuint32_t			dpll_locked;
	dma_functions_t     sdmafuncs;
} mx35_spdif_t;


/*
 * Function prototypes
 */
int mx35_spdif_mixer (ado_card_t * card, ado_mixer_t ** mixer, HW_CONTEXT_T * hwc);


#endif	/* __MX35_SPDIF_H */
