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


#ifndef __3DSMX35_H
#define __3DSMX35_H

/*
 *  Definitions for i.MX35 3DS audio setup
 */

struct	mx35_3ds_card;
#define  HW_CONTEXT_T struct mx35_3ds_card
#define  PCM_SUBCHN_CONTEXT_T   ado_pcm_subchn_t
#include <audio_driver.h>
#include <hw/dma.h>
#include <string.h>
#include <proto.h>

typedef volatile uint32_t vuint32_t;
typedef volatile uint16_t vuint16_t;
typedef volatile uint8_t vuint8_t;

#define SDMA_PLAYBACK_CHANNEL   1
#define SDMA_CAPTURE_CHANNEL    2

#define SSI1_BASE   0x43FA0000
#define SSI1_IRQ	11
#define SSI2_BASE   0x50014000
#define SSI2_IRQ	12

typedef struct  ssi
{
    vuint32_t   stx0;      
    vuint32_t   stx1;
    vuint32_t   srx0;       
    vuint32_t   srx1;
 
    vuint32_t   scr;        
#define	SCR_SSI_EN		(1<<0)
#define	SCR_TX_EN		(1<<1)
#define	SCR_RX_EN		(1<<2)
#define	SCR_NET_MODE	(1<<3)
#define	SCR_SYNC_MODE	(1<<4)
#define	SCR_I2SMODE_MSK	(3<<5)
#define	SCR_I2S_SLAVE	(1<<6)
#define	SCR_SYS_CLK_EN	(1<<7)
#define	SCR_TCH_EN		(1<<8)
#define	SCR_CLK_IST		(1<<9)

    vuint32_t   sisr;		
#define	SISR_TFE0		(1<<0)
#define	SISR_RFF0		(1<<2)
 
    vuint32_t   sier;       
#define SIER_TDMAE			(1<<20)
#define	SIER_TIE			(1<<19)
#define	SIER_TFE0			(1<<0)
#define SIER_RDMAE			(1<<22)
#define	SIER_RIE			(1<<21)
#define	SIER_RFF0			(1<<2)
 
    vuint32_t   stcr;		
#define	STCR_TXBIT0			(1<<9)	
#define	STCR_TXFIFO1_EN		(1<<8)
#define	STCR_TXFIFO0_EN		(1<<7)
#define	STCR_TFDIR_INTERNAL	(1<<6)
#define	STCR_TXDIR_INTERNAL	(1<<5)
#define STCR_TSHFD_LSB		(1<<4)
#define	STCR_TSCKP_FE		(1<<3)
#define STCR_TFSI_AL		(1<<2)
#define STCR_TFSL_BIT		(1<<1)
#define STCR_TEFS_EARLY		(1<<0)
  
    vuint32_t   srcr;      
#define	SRCR_RXBIT0			(1<<9)	
#define	SRCR_RXFIFO0_EN		(1<<7)
#define	SRCR_RXFIFO1_EN		(1<<8)
#define	SRCR_RFDIR_INTERNAL	(1<<6)
#define	SRCR_RXDIR_INTERNAL	(1<<5)
#define SRCR_RSHFD_LSB		(1<<4)
#define	SRCR_RSCKP_FE		(1<<3)
#define SRCR_RFSI_AL		(1<<2)
#define SRCR_RFSL_BIT		(1<<1)
#define SRCR_REFS_EARLY		(1<<0)

    vuint32_t   stccr;		
#define STCCR_DIV2			(1<<18)
#define	STCCR_PSR			(1<<17)
#define	STCCR_WL_MSK		(0xf<<13)
#define	STCCR_WL_16BIT		(7<<13)
#define STCCR_DC_MSK		(0x1f<<8)
#define	STCCR_DC_2W			(1<<8)
#define STCCR_PM_MSK		(0xff<<0)

    vuint32_t   srccr;     
#define SRCCR_DIV2			(1<<18)
#define	SRCCR_PSR			(1<<17)
#define	SRCCR_WL_MSK		(0xf<<13)
#define	SRCCR_WL_16BIT		(7<<13)
#define SRCCR_DC_MSK		(0x1f<<8)
#define	SRCCR_DC_2W			(1<<8)
#define SRCCR_PM_MSK		(0xff<<0)

    vuint32_t   sfcsr;		
#define SFCSR_TFWM0_MSK			(0xf<<0)
#define SFCSR_TXFIFO0_CNT(x)	(((x) >> 8) & 0xf)
#define SFCSR_RFWM0_MSK			(0xf<<4)
#define SFCSR_RXFIFO0_CNT(x)	(((x) >> 12) & 0xf)

	vuint32_t	str;
	vuint32_t	sor;
#define SOR_RX_CLR		(1<<5)
#define SOR_TX_CLR		(1<<4)

    vuint32_t   sacnt;
    vuint32_t   sacadd;
    vuint32_t   sacdat;
    vuint32_t   satag;
 
    vuint32_t   stmsk;		 
#define	STMSK_ALL	0xffffffff
#define	STMSK_SLOT0	(1<<0)
#define	STMSK_SLOT1	(1<<1)
 
    vuint32_t   srmsk;      
#define	SRMSK_ALL	0xffffffff
#define	SRMSK_SLOT0	(1<<0)
#define	SRMSK_SLOT1	(1<<1)

	vuint32_t	saccst;
	vuint32_t	saccen;
	vuint32_t	saccdis;
}ssi_t;


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
#define	COSR_CLKOEN			(1 << 5)
#define	COSR_CLKO_DIV1		(1 << 6)
#define COSR_CLKOUTDIV_MSK	(0x3f << 10)
#define	COSR_CLKOUTDIV_2	(1 << 10)
#define	COSR_CLKOSEL_MSK	(0x1f << 0)
#define COSR_CLKOSEL_PLLI	(1 << 0)
    vuint32_t   cgr0;
    vuint32_t   cgr1;   
    vuint32_t   cgr2;
#define	CGR2_SSI1_MSK	(3 << 12)
#define	CGR2_SSI1_EN	(3 << 12)
#define	CGR2_SSI2_MSK	(3 << 14)
#define	CGR2_SSI2_EN	(3 << 14)
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
}ccm_t;


#define AUDMUX_BASE 0x53FC4000

typedef struct audmux
{
    vuint32_t   ptcr1; 
    vuint32_t   pdcr1;
    vuint32_t   ptcr2;  
    vuint32_t   pdcr2;
    vuint32_t   ptcr3;  
    vuint32_t   pdcr3;
    vuint32_t   ptcr4;  
    vuint32_t   pdcr4;
    vuint32_t   ptcr5;  
    vuint32_t   pdcr5;
    vuint32_t   ptcr6;  
    vuint32_t   pdcr6;
    vuint32_t   ptcr7;  
    vuint32_t   pdcr7;
    vuint32_t   cnmcr; 
}audmux_t;

/* DIGITAL AUDIO MULTIPLEXER (AUDMUX) */
#define AUDMUX_SYNC				(1<<11)
#define AUDMUX_TFS_DIROUT		(1<<31)
#define	AUDMUX_TFS_PORT4		(3<<27)
#define	AUDMUX_TFS_PORT5		(4<<27)
#define	AUDMUX_TCLK_DIROUT		(1<<26)
#define	AUDMUX_TCLK_PORT4		(3<<22)
#define	AUDMUX_TCLK_PORT5		(4<<22)
#define AUDMUX_RFS_DIROUT		(1<<21)
#define AUDMUX_RFS_PORT4		(3<<17)
#define AUDMUX_RFS_PORT5		(4<<17)
#define	AUDMUX_RCLK_DIROUT		(1<<16)
#define	AUDMUX_RCLK_PORT4		(3<<12)
#define	AUDMUX_RCLK_PORT5		(4<<12)
#define AUDMUX_RXDSEL_PORT1		(0<<13)
#define AUDMUX_RXDSEL_PORT2		(1<<13)
#define AUDMUX_RXDSEL_PORT4		(3<<13)
#define AUDMUX_RXDSEL_PORT5		(4<<13)
#define AUDMUX_RXDSEL_MSK		(7<<13)


#define POWER_DOWN      0
#define POWER_UP      	1
#define PLAY			2
#define RECORD			3

struct	mx35_3ds_stream {
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
};

typedef struct mx35_3ds_stream mx35_3ds_strm_t;

struct mx35_3ds_card {
	ado_mutex_t			hw_lock;
	ado_pcm_t 			*pcm1;
	ado_mixer_t			*mixer;
	audmux_t			*audmux;
	uint32_t			ssi_base;
	ssi_t				*ssi;
	int					i2c_fd;
	mx35_3ds_strm_t		play_strm;
	mx35_3ds_strm_t		cap_strm;
	int 				power_state;
    intrspin_t          spinlock;
	dma_functions_t     sdmafuncs;
 };
typedef struct mx35_3ds_card mx35_3ds_t;

int ak4647_mixer (ado_card_t * card, ado_mixer_t ** mixer, HW_CONTEXT_T * hwc);
int setCodecPowerMode (HW_CONTEXT_T * mx35_3ds, int mode);
int AudmuxInit(HW_CONTEXT_T * mx35_3ds);

int mx35_3ds_i2c_init (HW_CONTEXT_T * mx35_3ds);
uint8_t mx35_3ds_i2c_read (HW_CONTEXT_T * mx35_3ds, uint8_t reg);
void mx35_3ds_i2c_write (HW_CONTEXT_T * mx35_3ds, uint8_t reg, uint8_t val);


#endif	
