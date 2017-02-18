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



#ifndef _MX35_CSPI_H_INCLUDED
#define _MX35_CSPI_H_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <hw/spi-master.h>
#include <arm/mx31.h>
#include <arm/mx35.h>

// GPIO register configuration for slaves
#define MX35_GPIO1_BASE			0x53FCC000
#define	MX35_GPIO_DR			0x00

#define	MX35_CSPI_PRIORITY		    21
#define	MX35_CSPI_EVENT			     1
#define	MX35_DMA_PULSE_PRIORITY     21
#define	MX35_DMA_PULSE_CODE			 1

#define MX35_CSPI1_BASE         0x43fa4000
#define MX35_CSPI2_BASE         0x50010000
#define MX35_CSPI_SIZE          0x20
#define MX35_CSPI_BURST_MAX		0x20	/* we only support 32 bytes MAX burst bytes for now */

#define MX35_CSPI1_IRQ         14
#define MX35_CSPI2_IRQ         13

#define MX35_CSPI_RXDATA		0x00	/* Receive data register */
#define MX35_CSPI_TXDATA		0x04	/* Transmit data register */
#define MX35_CSPI_CONREG		0x08	/* Control register */
#define MX35_CSPI_INTREG		0x0C	/* Interrupt control register */
#define MX35_CSPI_DMAREG		0x10	/* DMA control register */
#define MX35_CSPI_STATREG       0x14    /* Status register */
#define MX35_CSPI_PERIODREG		0x18	/* Sample period control register */
#define	MX35_CSPI_TESTREG		0x1c	/* Test register */

// CONREG BIT Definitions
#define CSPI_CONREG_ENABLE         	0x1
#define CSPI_CONREG_MODE           	0x2
#define CSPI_CONREG_XCH            	0x4
#define CSPI_CONREG_SMC            	0x8
#define CSPI_CONREG_POL            	0x10
#define CSPI_CONREG_PHA            	0x20
#define CSPI_CONREG_SSCTL          	0x40
#define CSPI_CONREG_SSPOL          	0x80
#define CSPI_CONREG_BCNT_MASK		0xFFF00000
#define CSPI_CONREG_BCNT_POS		20
#define CSPI_CONREG_DRATE_MASK		0x70000
#define CSPI_CONREG_DRATE_POS		16
#define CSPI_CONREG_DRCTL_MASK		0x300
#define CSPI_CONREG_DRCTL_POS		8
#define CSPI_CONREG_DRCTL_EDGE      1
#define CSPI_CONREG_DRCTL_LEVEL     2
#define CSPI_CONREG_CSEL_MASK	    0x3000
#define CSPI_CONREG_CSEL_POS		12


// INTREG BIT Definitions
#define CSPI_INTREG_TEEN            0x1
#define CSPI_INTREG_THEN            0x2
#define CSPI_INTREG_TFEN            0x4
#define CSPI_INTREG_RREN            0x8
#define CSPI_INTREG_RHEN            0x10
#define CSPI_INTREG_RFEN            0x20
#define CSPI_INTREG_ROEN            0x40
#define CSPI_INTREG_TCEN 			0x80


// STATREG BIT Definitions
#define CSPI_STATREG_TE             0x1
#define CSPI_STATREG_TH             0x2
#define CSPI_STATREG_TF             0x4
#define CSPI_STATREG_RR             0x8
#define CSPI_STATREG_RH             0x10
#define CSPI_STATREG_RF             0x20
#define CSPI_STATREG_RO             0x40
#define CSPI_STATREG_TC				0x80

// PERIODREG Definitions
#define CSPI_PERIODREG_SP_MASK      0x7fff;
#define CSPI_PERIODREG_32K_CLK      0x8000

// TESTREG BIT Definitions
#define CSPI_TESTREG_LOOPBACK       0x4000
#define CSPI_TESTREG_RXCNT			4
#define CSPI_TESTREG_RXCNT_MASK     0x00F0
#define CSPI_TESTREG_TXCNT_MASK     0x000F

typedef struct {
	SPIDEV		        spi;	/* has to be the first element */

	unsigned	        pbase;
	unsigned 		gpio_pbase;
	uintptr_t	        vbase;
	uintptr_t		gpio_vbase;
	int			        irq;
	int			        iid;
	int			        chid, coid;

	uint32_t	        bitrate;

	uint32_t	        clock;
    uint32_t            waitstates;

	uint8_t		        *pbuf;
	int			        xlen, tlen, rlen;
	int			        dlen;
	int			        dtime;		/* usec per data, for time out use */

    int                 loopback;
    int                 burst;
    
	struct sigevent	spievent;
} mx35_cspi_t;

extern void *mx35_init(void *hdl, char *options);
extern void mx35_dinit(void *hdl);
extern void *mx35_xfer(void *hdl, uint32_t device, uint8_t *buf, int *len);
extern int mx35_setcfg(void *hdl, uint16_t device, spi_cfg_t *cfg);

extern int mx35_devinfo(void *hdl, uint32_t device, spi_devinfo_t *info);
extern int mx35_drvinfo(void *hdl, spi_drvinfo_t *info);

extern int mx35_xchange(mx35_cspi_t *mx35, uint8_t *buf, int len);
extern int mx35_read(mx35_cspi_t *mx35, uint8_t *buf, int len);
extern int mx35_write(mx35_cspi_t *mx35, uint8_t *buf, int len);

extern int mx35_attach_intr(mx35_cspi_t *mx35);
extern int mx35_wait(mx35_cspi_t *dev, int len);

extern int mx35_cfg(void *hdl, spi_cfg_t *cfg);

extern int mx35_dma_init(mx35_cspi_t *mx35);
extern void mx35_dma_config_xfer(mx35_cspi_t *mx35,uint8_t *buf,int len,int xfer_width);
extern int mx35_dma_wait(mx35_cspi_t *mx35);


#endif
