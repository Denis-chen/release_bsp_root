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






#include "smc.h"


static char *smc9118_opts[] = {
	"txfifo",
	NULL
};

static int
smc9118_parse_options(smc9118_t *smc9118,
    const char *optstring, nic_config_t *cfg)
{
	char		*value;
	int			opt;
	char		*options, *freeptr;
	char		*c;
	int			rc = 0;
	int			err = EOK;

	if (optstring == NULL)
		return 0;

	/* getsubopt() is destructive */
	freeptr = options = strdup(optstring);

	while (options && *options != '\0') {
		c = options;
		if ((opt = getsubopt(&options, smc9118_opts, &value)) == -1) {
			if (nic_parse_options(cfg, value) == EOK)
				continue;
			goto error;
		}

		switch (opt) {
			case 0:
				if (smc9118 != NULL)
					smc9118->tx_fifo_size = strtoul(value, 0, 0);
				continue;
		}
error:
		nic_slogf(_SLOGC_NETWORK, _SLOG_WARNING, 
		    "devn-smc9118: unknown option %s", c);
		err = EINVAL;
		rc = -1;
	}

	free(freeptr);

	errno = err;

	return rc;
}

int
smc9118_detect(void *dll_hdl, io_net_self_t *ion, char *options)
{
	uintptr_t		iobase;
	smc9118_t		*dev;
	nic_config_t	*cfg;
	int				reset, timeout;
	uint32_t		hw_cfg;
	char			*s;

	if ((dev = calloc(1, sizeof (*dev))) == NULL)
		return -1;

	cfg = &dev->cfg;
	
	/* set some defaults for the command line options */
	cfg->flags = NIC_FLAG_MULTICAST;
	cfg->media_rate = cfg->duplex = -1;
	cfg->priority = 21;
	cfg->iftype = IFT_ETHER;
	strcpy((char *)cfg->uptype, "en");
	cfg->lan = -1;
	cfg->media = NIC_MEDIA_802_3;
	cfg->connector = NIC_CONNECTOR_UTP;
	cfg->vendor_id = 0x10b8;
	dev->max_rx_buffers = 32;
	dev->cfg.media = NIC_MEDIA_802_3;
	dev->cfg.mac_length = ETH_MAC_LEN;

	if (smc9118_parse_options(dev, options, cfg) != 0) {
		free(dev);
		return -1;
	}

	if (cfg->num_io_windows == 0 || cfg->num_irqs == 0) {
		nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
		    "devn-smc9118: must specify an I/O port and IRQ");
		free(dev);
		return ENODEV;
	}

	if (cfg->duplex == -1)
		dev->negotiate = 1;

	if (cfg->media_rate != -1) {
		if (cfg->media_rate == 100 * 1000) {
			dev->forced_media = (cfg->duplex == 1) ? MDI_100bTFD : MDI_100bT;
			}
		else {
			dev->forced_media = (cfg->duplex == 1) ? MDI_10bTFD : MDI_10bT;
			}
		}
	if (cfg->duplex != -1) {
		if (cfg->media_rate == -1) {
			dev->forced_media = (cfg->duplex == 1) ? (MDI_100bTFD | MDI_10bTFD) : (MDI_100bT | MDI_10bT);
			}
		else {
			if (cfg->media_rate == 100 * 1000) {
				dev->forced_media = (cfg->duplex == 1) ? MDI_100bTFD : MDI_100bT;
				}
			else {
				dev->forced_media = (cfg->duplex == 1) ? MDI_10bTFD : MDI_10bT;
				}
			}
		}

	/* TODO See if we can support 1514+4 byte frames for VLAN support */
	if (cfg->mtu == 0 || cfg->mtu > ETH_MAX_PKT_LEN)
		cfg->mtu = ETH_MAX_PKT_LEN;
	if (cfg->mru == 0 || cfg->mru > ETH_MAX_PKT_LEN)
		cfg->mru = ETH_MAX_PKT_LEN;

	if (cfg->io_window_size[0] == 0)
		cfg->io_window_size[0] = SMC9118_MMAP_SIZE;

	iobase = mmap_device_io(cfg->io_window_size[0], cfg->io_window_base[0]);
	if (iobase == (uintptr_t)MAP_FAILED) {
		free(dev);
		return -1;
	}

	dev->iobase = iobase;

	cfg->device_id = (INLE32(dev->iobase + SMC9118_ID_REV) >> 16) & CHIP_ID;

	switch (cfg->device_id) {
		case SMSC_ID_9218I: s = "SMC9218I"; break;
		default:            s = "SMC9118";  break;
	}
	strcpy((char *)cfg->device_description, s);

	if (in32(iobase + SMC9118_HW_CFG) & BIT32_16)
		dev->width = 32;
	else 
		dev->width = 16;

	cfg->device_revision = INLE32(dev->iobase + SMC9118_ID_REV) & CHIP_REV;

	/* RESET MAC */
	for (reset = 0; reset < 3; reset++) {
		hw_cfg = INLE32(dev->iobase + SMC9118_HW_CFG);
		OUTLE32(dev->iobase + SMC9118_HW_CFG, (hw_cfg | SRST));
		timeout = 10;
		do {
			timeout--;
			nanospin_ns(100);
		} while ((timeout > 0) && (INLE32(dev->iobase + SMC9118_HW_CFG) & SRST));
	}

	/* Setup TX FIFO size */	
	hw_cfg = INLE32(dev->iobase + SMC9118_HW_CFG) & ~(0x0F << 16);
	if (dev->tx_fifo_size > TX_FIF_SZ_13K || dev->tx_fifo_size < TX_FIF_SZ_1K)
		dev->tx_fifo_size = TX_FIF_SZ_3K;

	hw_cfg |= (dev->tx_fifo_size << 16);

	OUTLE32(dev->iobase + SMC9118_HW_CFG, hw_cfg | SF);

	/*  Setup AUTO Flow Control */
	OUTLE32(dev->iobase + SMC9118_AFC_CFG, (AFC_HI | AFC_LO | BACK_DUR));

	if (dev->cfg.verbose) {
		nic_slogf(_SLOGC_NETWORK, _SLOG_INFO,
		    "devn-smc9118: %d bit bus detected", dev->width);
		nic_slogf(_SLOGC_NETWORK, _SLOG_INFO,
		    "devn-smc9118: Allocate %d bytes for TX FIFO", dev->tx_fifo_size * 1024);
	}

	if (!smc9118_register_device(dev, ion, dll_hdl)) {
		smc9118_advertise(dev->reg_hdl, dev);

		return 0;
	}

	munmap_device_io(iobase, cfg->io_window_size[0]);
	free(dev);

	return ENODEV;
}
