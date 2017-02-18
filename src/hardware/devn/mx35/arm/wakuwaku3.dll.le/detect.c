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


#include "mx35.h"
#include <hw/i2c.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arm/mx35.h>


static int mx35_fec_wakuwaku3_init(void)
{
	uintptr_t	gpio_base;

	gpio_base = mmap_device_io(0x04, MX35_GPIO3_BASE);
	if (gpio_base == MAP_DEVICE_FAILED)
		return -1;

	out32(gpio_base, in32(gpio_base) | (1 << 0));
	munmap_device_io(gpio_base, 0x04);
	return 0;
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

static	char *mx35_fec_opts [] = {
	  "receive", 
	  "transmit",
	  NULL
};

static int	mx35_fec_parse_options (mx35_t *ext, const char *optstring,
			nic_config_t *cfg)
{
char	*value;
int		opt;
char	*options, *freeptr;
char	*c;
int		rc = 0;
int		err = EOK;

	if (optstring == NULL) {
		return 0;
		}

	/* getsubopt() is destructive */
	freeptr = options = strdup(optstring);

	while (options && *options != '\0') {
		c = options;
		if ((opt = getsubopt (&options, mx35_fec_opts, &value)) == -1) {
			if (nic_parse_options (cfg, value) == EOK) {
				continue;
				}
			goto error;
			}

		switch (opt) {
			case	0:
				if (ext != NULL) {
					ext->num_rx_descriptors = strtoul (value, 0, 0);
					}
				continue;
			
			case	1:
				if (ext != NULL) {
					ext->num_tx_descriptors = strtoul (value, 0, 0);
					}
				continue;
			
			default:
			  	continue;
		
			}
		
error:
		nic_slogf(_SLOGC_NETWORK, _SLOG_WARNING, "devn-mx35: unknown option %s", c);
		err = EINVAL;
		rc = -1;
		}

	free (freeptr);

	errno = err;

	return (rc);
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

static int	mx35_fec_create_instance (void *dll_hdl, io_net_self_t *ion,
    char *options, int idx)

{
nic_config_t		*cfg;
mx35_t			*ext;
	
	if ((ext = calloc (1, sizeof (*ext) + 256 * sizeof (int))) == NULL) {
		return (-1);
		}

	cfg = &ext->cfg;

	/* set some defaults for the command line options */
	cfg->flags = NIC_FLAG_MULTICAST;
	cfg->device_index = idx;
	cfg->media_rate = cfg->duplex = -1;
	cfg->priority = NIC_PRIORITY;
	cfg->iftype = IFT_ETHER;
	cfg->lan = -1;
	strcpy ((char *)cfg->uptype, "en");
	ext->num_tx_descriptors = DEFAULT_NUM_TX_DESCRIPTORS;
	ext->num_rx_descriptors = DEFAULT_NUM_RX_DESCRIPTORS;

	if (mx35_fec_parse_options (ext, options, cfg) == -1) {
		return (-1);
		}

	if (cfg->mtu == 0 || cfg->mtu > ETH_MAX_PKT_LEN) {
		cfg->mtu = ETH_MAX_PKT_LEN;
		}

	if (cfg->mru == 0 || cfg->mru > ETH_MAX_PKT_LEN) {
		cfg->mru = ETH_MAX_PKT_LEN;
		}

	ext->force_advertise = -1;

	if (cfg->media_rate != -1 || cfg->duplex != -1) {
		if (cfg->media_rate == 100*1000) {
			if (cfg->duplex) {
				ext->force_advertise = MDI_100bTFD | MDI_100bT;
				}
			else {
				ext->force_advertise = MDI_100bT;
				}
			}
		else
			{
			cfg->media_rate = 10 * 1000;
			if (cfg->duplex) {
				ext->force_advertise = MDI_10bTFD | MDI_10bT;
				}
			else {
				ext->force_advertise = MDI_10bT;
				}
			}
		}

	if (cfg->num_irqs == 0) {
		cfg->irq[0] = MX35_INTR;
		cfg->num_irqs = 1;
		}

	if (cfg->num_mem_windows == 0) {
		cfg->mem_window_base [0] = ext->iobase = MX35_IOBASE;
		cfg->mem_window_size [0] = 0x1000;
		cfg->num_mem_windows = 1;
		}

	strcpy ((char *)cfg->device_description, "i.MX35 FEC");

	if ((ext->reg = mmap_device_memory (NULL, 0x1000, PROT_READ | PROT_WRITE | PROT_NOCACHE,
		MAP_SHARED, ext->iobase)) == MAP_FAILED) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "devn-mx35: mmap memory failed");
		return (-1);
		}
	
	ext->num_rx_descriptors &= ~3;
	if (ext->num_rx_descriptors < MIN_NUM_RX_DESCRIPTORS) {
		ext->num_rx_descriptors = MIN_NUM_RX_DESCRIPTORS;
		}
	if (ext->num_rx_descriptors > MAX_NUM_RX_DESCRIPTORS) {
		ext->num_rx_descriptors = MAX_NUM_RX_DESCRIPTORS;
		}

	ext->num_tx_descriptors &= ~3;
	if (ext->num_tx_descriptors < MIN_NUM_TX_DESCRIPTORS) {
		ext->num_tx_descriptors = MIN_NUM_TX_DESCRIPTORS;
		}
	if (ext->num_tx_descriptors > MAX_NUM_TX_DESCRIPTORS) {
		ext->num_tx_descriptors = MAX_NUM_TX_DESCRIPTORS;
		}

	cfg->revision = NIC_CONFIG_REVISION;
	ext->stats.revision = NIC_STATS_REVISION;

	if (!mx35_fec_register_device (ext, ion, dll_hdl)) {
		mx35_fec_advertise (ext->reg_hdl, ext);
		return (EOK);
		}

	free (ext);

	errno = ENODEV;
	return (-1);
}

/****************************************************************************
 * mx35_fec_detect
 ****************************************************************************/

int	mx35_fec_detect (void *dll_hdl, io_net_self_t *ion, char *options)

{
nic_config_t	cfg;
int				rc = EOK;

	if (mx35_fec_wakuwaku3_init() != 0)
		return (errno);

	memset (&cfg, 0, sizeof (cfg));

	cfg.device_index = 0xffffffff;
	if (options != NULL) {
		if (mx35_fec_parse_options (NULL, options, &cfg) != 0)
			return (errno);
		}

	errno = ENODEV;

	if (mx35_fec_create_instance (dll_hdl, ion, options, 0) != EOK) {
		return (ENODEV);
		}

	return (rc);
}

