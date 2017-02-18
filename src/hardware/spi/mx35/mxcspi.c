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


#include "mxcspi.h"


static char *mx35_opts[] = {
	"base",			/* Base address for this CSPI controller */
	"irq",			/* IRQ for this CSPI intereface */
	"clock",		/* CSPI clock */
    "loopback",     /*loopback interface for test purposes*/
    "waitstate",    /* waitstates between xfers */
	"burst",
	NULL
};

spi_funcs_t spi_drv_entry = {
	sizeof(spi_funcs_t),
	mx35_init,		/* init() */
	mx35_dinit,		/* fini() */
	mx35_drvinfo,	/* drvinfo() */
	mx35_devinfo,	/* devinfo() */
	mx35_setcfg,	/* setcfg() */
	mx35_xfer,		/* xfer() */
	NULL			/* dma_xfer() */
};

/*
 * Note:
 * The devices listed are just examples, users should change
 * this according to their own hardware spec.
 */
static spi_devinfo_t devlist[4] = {
	{
		0x00,				// Device ID, for SS0
		"CSPI-DEV0",		// Description
		{ 
			16,		        // data length 16bit, MSB
			5000000			// Clock rate 5M
		},
	},
	{
		0x01,				// Device ID, for SS1
		"CSPI-DEV1",		// Description
		{ 
			16,		        // data length 16bit, MSB
			5000000			// Clock rate 5M
		},
	},
	{
		0x02,				// Device ID, for SS2
		"CSPI-DEV2",		// Description
		{ 
			16,		        // data length 16bit, MSB
			5000000			// Clock rate 5M
		},
	},
	{
		0x03,				// Device ID, for SS3
		"CSPI-DEV3",		// Description
		{ 
			16,		        // data length 16bit, MSB
			5000000			// Clock rate 5M
		},
	}
};

static uint32_t devctrl[4];

static int mx35_options(mx35_cspi_t *dev, char *optstring)
{
	int		opt, rc = 0, err = EOK;
	char	*options, *freeptr, *c, *value;

	if (optstring == NULL)
		return 0;

	freeptr = options = strdup(optstring);

	while (options && *options != '\0') {
		c = options;
		if ((opt = getsubopt(&options, mx35_opts, &value)) == -1)
			goto error;

		switch (opt) {
			case 0:
				dev->pbase = strtoul(value, 0, 0); 
				continue;
			case 1:
				dev->irq   = strtoul(value, 0, 0);
				continue;
			case 2:
				dev->clock = strtoul(value, 0, 0);
				continue;
			case 3:
				dev->loopback = 1;
				continue;
			case 4:
				dev->waitstates = strtoul(value, 0, 0);
				continue;
			case 5:
				dev->burst = 1;
				continue;
		}
error:
		fprintf(stderr, "mx35: unknown option %s", c);
		err = EINVAL;
		rc = -1;
	}

	free(freeptr);

	return rc;
}

void *mx35_init(void *hdl, char *options)
{
	mx35_cspi_t	*dev;
	uintptr_t	base;
	uintptr_t	gpio_base;
	int			i;
    uint32_t periodreg=0;
    
	dev = calloc(1, sizeof(mx35_cspi_t));

	if (dev == NULL)
		return NULL;

    //Set defaults
	dev->pbase = MX35_CSPI1_BASE;
	dev->gpio_pbase = MX35_GPIO1_BASE;
	dev->irq   = MX35_CSPI1_IRQ;
	dev->clock = 66500000;			/* 66.5 MHz CSPI clock */
    dev->loopback = 0;
    dev->burst = 0;
    dev->waitstates=0;
    
	if (mx35_options(dev, options))
		goto fail0;
    
	/*
	 * Map in SPI registers
	 */
	if ((base = mmap_device_io(MX35_CSPI_SIZE, dev->pbase)) == (uintptr_t)MAP_FAILED)
		goto fail0;

	dev->vbase = base;
    
	/*
	 * Map in GPIO registers
	 */
	if ((gpio_base = mmap_device_io(0x20, dev->gpio_pbase)) == (uintptr_t)MAP_FAILED)
		goto fail1;	

	dev->gpio_vbase = gpio_base;

    /* enable interface */
    out32(  base + MX35_CSPI_CONREG, 
            CSPI_CONREG_MODE | CSPI_CONREG_ENABLE    );	/* Enable SPI */
    
   if (dev->loopback == 1) {
       out32(base + MX35_CSPI_TESTREG,CSPI_TESTREG_LOOPBACK);
   }
   
    // set wait states between xfers
    periodreg |= dev->waitstates & CSPI_PERIODREG_SP_MASK;
    out32(base + MX35_CSPI_PERIODREG, periodreg);
            
	/*
	 * Calculate all device configuration here
	 */
	for (i = 0; i < 4; i++)
		devctrl[i] = mx35_cfg(dev, &devlist[i].cfg);

	/*
	 * Attach SPI interrupt
	 */
	if (mx35_attach_intr(dev))
		goto fail2;
  
 
	dev->spi.hdl = hdl;

   	return dev;

fail2:
	munmap_device_io(dev->gpio_vbase, 0x20);
fail1:
	munmap_device_io(dev->vbase, MX35_CSPI_SIZE);
fail0:
	free(dev);
	return NULL;
}

void mx35_dinit(void *hdl)
{
	mx35_cspi_t	*dev = hdl;
    
	/*
	 * unmap the register, detach the interrupt
	 */
	InterruptDetach(dev->iid);
	ConnectDetach(dev->coid);
	ChannelDestroy(dev->chid);

	/*
	 * Disable SPI
	 */
    out32(     dev->vbase + MX35_CSPI_CONREG,
               in32( dev->vbase + MX35_CSPI_CONREG) & ~CSPI_CONREG_ENABLE  );
	munmap_device_io(dev->gpio_vbase, 0x20);
	munmap_device_io(dev->vbase, MX35_CSPI_SIZE);

	free(hdl);
}

int mx35_drvinfo(void *hdl, spi_drvinfo_t *info)
{
	info->version = (SPI_VERSION_MAJOR << SPI_VERMAJOR_SHIFT) | (SPI_VERSION_MINOR << SPI_VERMINOR_SHIFT) | (SPI_REVISION << SPI_VERREV_SHIFT);
	strcpy(info->name, "MX35 CSPI");
	info->feature = 0;
	return (EOK);
}

int mx35_setcfg(void *hdl, uint16_t device, spi_cfg_t *cfg)
{
	uint32_t	control;

	if (device > 3)
		return (EINVAL);

	memcpy(&devlist[device].cfg, cfg, sizeof(spi_cfg_t));

	control = mx35_cfg(hdl, &devlist[device].cfg);
	if (control == 0)
		return (EINVAL);

	devctrl[device] = control;

	return (EOK);
}

int mx35_devinfo(void *hdl, uint32_t device, spi_devinfo_t *info)
{
	int		dev = device & SPI_DEV_ID_MASK;

	if (device & SPI_DEV_DEFAULT) {
		/*
		 * Info of this device
		 */
		if (dev >= 0 && dev <= 3)
			memcpy(info, &devlist[dev], sizeof(spi_devinfo_t));
		else
			return (EINVAL);
	}
	else {
		/*
		 * Info of next device 
		 */
		if (dev == SPI_DEV_ID_NONE)
			dev = -1;
		dev++;		// get next device number		
		if (dev < 4)
			memcpy(info, &devlist[dev], sizeof(spi_devinfo_t));
		else
			return (EINVAL);
	}

	return (EOK);
}


void *mx35_xfer(void *hdl, uint32_t device, uint8_t *buf, int *len)
{
	mx35_cspi_t	*dev = hdl;
	uintptr_t	base = dev->vbase;
	uint32_t	ctrl, id;
	uint32_t	data, burst, tmp;
	uint32_t	i = 0;

	id = device & SPI_DEV_ID_MASK;
	if (id > 3) {
		*len = -1;
		return buf;
	}

	dev->xlen = *len;
	dev->rlen = dev->tlen = 0;
	dev->pbuf = buf;
	dev->dlen = ((devlist[id].cfg.mode & SPI_MODE_CHAR_LEN_MASK) + 7) >> 3;

    // Estimate transfer time in us... The calculated dtime is only used for
    // the timeout, so it doesn't have to be that accurate.  At higher clock
    // rates, a calcuated dtime of 0 would mess-up the timeout calculation, so
    // round up to 1 us
	dev->dtime = dev->dlen * (8 + dev->waitstates) * 1000 * 1000 / devlist[id].cfg.clock_rate;
    if (dev->dtime == 0)
        dev->dtime = 1;

	ctrl =  devctrl[id]                     | 
            (id << CSPI_CONREG_CSEL_POS)    |
            CSPI_CONREG_SSCTL               | 
            CSPI_CONREG_MODE                | 
            CSPI_CONREG_ENABLE;

	/* clean up the RXFIFO */
	while (in32(base + MX35_CSPI_TESTREG) & CSPI_TESTREG_RXCNT_MASK)
		in32(base + MX35_CSPI_RXDATA);

	if (dev->burst) {		// for SPI burst transmit mode
		if (dev->xlen >= MX35_CSPI_BURST_MAX) {
		
			burst = MX35_CSPI_BURST_MAX * 8 - 1;
		
		} else {

			burst = dev->xlen * 8 - 1;
			
			/* check if the xlen not aligne on word size */
			switch (tmp = dev->xlen % 4) {
				case 1:   // only write 8 bits in TXFIFO
					if (1 == dev->dlen) {
						out32(base + MX35_CSPI_TXDATA, buf[dev->tlen]);
						dev->tlen++;
					} else
						fprintf(stderr, "mx35_xfer: Unexpected tranfer length.\n");
					break;
				case 2:   // only write 16 bits in TXFIFO
					if (1 == dev->dlen) {
						tmp = (buf[dev->tlen] << 8) | (buf[dev->tlen + 1]);
						out32(base + MX35_CSPI_TXDATA, tmp);
						dev->tlen += 2;
					} else if (2 == dev->dlen) {
						tmp = *(uint16_t *)(&buf[dev->tlen]);
						out32(base + MX35_CSPI_TXDATA, tmp);
						dev->tlen += 2;
					}else
						fprintf(stderr, "mx35_xfer: Unexpected tranfer length.\n");
					break;
				case 3:   // only write 24 bits in TXFIFO
					if (1 == dev->dlen) {
						tmp = (buf[dev->tlen] << 16) | (buf[dev->tlen + 1] << 8) | (buf[dev->tlen + 2]); 
						out32(base + MX35_CSPI_TXDATA, tmp);
						dev->tlen += 3;
					} else
						fprintf(stderr, "mx35_xfer: Unexpected tranfer length.\n");
					break;
				default:
					break;
			}
		}

		while ((dev->xlen > dev->tlen) && (i < 8)) {	// write rest of data to TXFIFO
			switch (dev->dlen) {
				case 1:
					data = (buf[dev->tlen] << 24) | (buf[dev->tlen + 1] << 16)
							| (buf[dev->tlen + 2] << 8) | (buf[dev->tlen + 3]); 
					break;
				case 2:
					data = (*(uint16_t *)(&buf[dev->tlen]) << 16) 
							| (*(uint16_t *)(&buf[dev->tlen + 2]));
					break;
				case 4:
					data = *(uint32_t *)(&buf[dev->tlen]);
					break;
				default:
					fprintf(stderr, "mx35_xfer: Unsupport word length.\n");
					data = 0;
					break;
			}
			out32(base + MX35_CSPI_TXDATA, data);
			dev->tlen += 4;
			i++;
		}

		ctrl &= ~CSPI_CONREG_SSCTL;
		ctrl = (ctrl & ~CSPI_CONREG_BCNT_MASK) | (burst << CSPI_CONREG_BCNT_POS);

	} else {

		for (i = 0; i < 8; i++) {

			if (dev->tlen >= dev->xlen)
				break;

			switch (dev->dlen) {
				case 1:
					out32(base + MX35_CSPI_TXDATA, buf[dev->tlen]);
					dev->tlen++;
					break;
				case 2:
					out32(base + MX35_CSPI_TXDATA, *(uint16_t *)(&buf[dev->tlen]));
					dev->tlen += 2;
					break;
				case 3:
				case 4:
					out32(base + MX35_CSPI_TXDATA, *(uint32_t *)(&buf[dev->tlen]));
					dev->tlen += 4;
					break;
			}
		}
  	}

	/* Enable SPI */
    out32(base + MX35_CSPI_CONREG, ctrl);	

    /* enable tx complete interrupt and receive ready FIFO interrupt */
    out32(base + MX35_CSPI_INTREG, CSPI_INTREG_TCEN );
    
    /* Start exchange */
    out32(  base + MX35_CSPI_CONREG, ctrl | CSPI_CONREG_XCH  );	
	
    /*
     * Wait for exchange to finish
     */
          
     if (mx35_wait(dev, dev->xlen)) {
        fprintf(stderr, "spi-mx35: XFER Timeout!!!");
     
        dev->rlen = -1;
     }
 
	if (dev->burst) {		// for SPI burst transmit mode

		burst = (dev->xlen / MX35_CSPI_BURST_MAX ) * MX35_CSPI_BURST_MAX;
		switch (tmp = dev->xlen % 4) {
			case 1:   // ignore first 3 bytes in RXFIFO
				memcpy(&buf[burst], &buf[burst + 3], dev->xlen - burst);
				dev->rlen = dev->rlen - 3;
				break;

			case 2:   // ignore first 2 bytes in RXFIFO
				memcpy(&buf[burst], &buf[burst + 2], dev->xlen - burst);
				dev->rlen = dev->rlen - 2;
				break;

			case 3:   // ignore first 1 bytes in RXFIFO
				memcpy(&buf[burst], &buf[burst + 1], dev->xlen - burst);
				dev->rlen = dev->rlen - 1;
				break;

			default:
				break;
		}
	}
	
	*len = dev->rlen;

	return buf;
}
