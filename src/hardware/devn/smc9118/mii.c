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

uint32_t smc9118_mac_read(uintptr_t base, uint8_t reg_off)
{
	uint32_t	status, cmd;

	status = INLE32(base + SMC9118_MAC_CSR_CMD);
	if (!(status & CSR_BUSY)) {
		OUTLE32(base + SMC9118_MAC_CSR_CMD, 0);
		cmd = reg_off | (CSR_READ | CSR_BUSY);
		OUTLE32(base + SMC9118_MAC_CSR_CMD, cmd);
	
		while ((INLE32(base + SMC9118_MAC_CSR_CMD) & CSR_BUSY));

		return (INLE32(base + SMC9118_MAC_CSR_DATA));
	}

	return (-1);
}

uint32_t smc9118_mac_write(uintptr_t base, uint8_t reg_off, uint32_t data)
{
	uint32_t	status, cmd;

	status = INLE32(base + SMC9118_MAC_CSR_CMD);
	if (!(status & CSR_BUSY)) {
		OUTLE32(base + SMC9118_MAC_CSR_DATA, data);
		OUTLE32(base + SMC9118_MAC_CSR_CMD, 0);
		cmd = reg_off | CSR_BUSY;
		OUTLE32(base + SMC9118_MAC_CSR_CMD, cmd);
	
		while ((INLE32(base + SMC9118_MAC_CSR_CMD) & CSR_BUSY));

		return (0);
	}

	return (-1);
}

uint16_t
smc9118_mii_read(void *handle, uint8_t phy_id, uint8_t location)
{
	smc9118_t	*dev = handle;
	uintptr_t	iobase = dev->iobase;
	uint32_t	status, phycmd, data;

	status = smc9118_mac_read(iobase, SMC9118_MII_ACC);

	if (!(status & MIIBZY)) {
		phycmd = (MIIBZY << 11) | MIIBZY | (location << 6);
		smc9118_mac_write(iobase, SMC9118_MII_ACC, phycmd);
		do {
			status = smc9118_mac_read(iobase, SMC9118_MII_ACC);
		} while (status & MIIBZY);
	
		data = smc9118_mac_read(iobase, SMC9118_MII_DATA);
		return (data & 0xFFFF);
	}

	return (-1);
} 

void
smc9118_mii_write(void *handle, uint8_t phy_id, uint8_t location, uint16_t value)
{
	smc9118_t	*dev = handle;
	uintptr_t	iobase = dev->iobase;
	uint32_t	status, phycmd;

	status = smc9118_mac_read(iobase, SMC9118_MII_ACC);

	if (!(status & MIIBZY)) {
		smc9118_mac_write(iobase, SMC9118_MII_DATA, value);
		phycmd = (MIIBZY << 11) | (MIIWNR | MIIBZY) | (location << 6);
		smc9118_mac_write(iobase, SMC9118_MII_ACC, phycmd);
		do {
			status = smc9118_mac_read(iobase, SMC9118_MII_ACC);
		} while (status & MIIBZY);
	}
}		

void
smc9118_linkup(smc9118_t *dev, unsigned mode, int force)
{
	uintptr_t	iobase = dev->iobase;
	char		*s;
	uint32_t	cr;

	if (!force)  {
		dev->cfg.flags &= ~NIC_FLAG_LINK_DOWN;

		dev->cfg.media_rate = -1;	/* Unknown */
		dev->cfg.duplex = -1;		/* Unknown */

		switch (mode) {
			case MDI_10bT:
				s = "10BTHD";
				dev->cfg.media_rate = 10*1000;
				dev->cfg.duplex = 0;
				break;
			case MDI_10bTFD:
				s = "10BTFD";
				dev->cfg.media_rate = 10*1000;
				dev->cfg.duplex = 1;
				break;
			case MDI_100bT:
				s = "100BTHD";
				dev->cfg.media_rate = 10*10000;
				dev->cfg.duplex = 0;
				break;
			case MDI_100bTFD:
				s = "100BTFD";
				dev->cfg.media_rate = 10*10000;
				dev->cfg.duplex = 1;
				break;
			case MDI_100bT4:
				s = "100BT4";
				dev->cfg.media_rate = 10*10000;
				dev->cfg.duplex = 0;
				break;
			default:
				s = "Unknown";
		}

		if (dev->cfg.verbose)
			nic_slogf(_SLOGC_NETWORK, _SLOG_INFO, "Link up (%s)", s);
	}

	cr = smc9118_mac_read(iobase, SMC9118_MAC_CR);
	if (dev->cfg.duplex == 1)
		cr |= FDPX;
	else
		cr &= ~FDPX;
	smc9118_mac_write(iobase, SMC9118_MAC_CR, cr);
}

void
smc9118_mii_callback(void *handle, uchar_t phy, uchar_t newstate)
{
	smc9118_t	*dev = handle;
	int			i, mode;

	switch (newstate) {
		case MDI_LINK_UP:
			dev->cfg.flags &= ~NIC_FLAG_LINK_DOWN;
			if ((i = MDI_GetActiveMedia(dev->mdi,
			    dev->phy_addr, &mode)) != MDI_LINK_UP) {
				if (dev->cfg.verbose)
					nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
					    "GetActiveMedia returned %x", i);
				mode = MDI_10bT;
			}

			smc9118_linkup(dev, mode, 0);

			break;

		case MDI_LINK_DOWN:
			dev->cfg.flags |= NIC_FLAG_LINK_DOWN;
			dev->cfg.media_rate = dev->cfg.duplex = -1;
			if (dev->cfg.verbose) {
				nic_slogf(_SLOGC_NETWORK, _SLOG_INFO,
				    "Link down %d", dev->cfg.lan);
			}
			break;
		default:
			break;
	}
}


static void
smc9118_mii_activate(smc9118_t *dev)
{
	int		advert = 0, rc;
	int		iobase = dev->iobase;

	/*
	 * Some PHYs need to be explicitly RESET in
	 * order for them to work properly.
	 */
	MDI_ResetPhy(dev->mdi, dev->phy_addr, WaitBusy);
	MDI_SyncPhy(dev->mdi, dev->phy_addr);

	/* Setup GPIOs to outputs for LEDs */
	OUTLE32(iobase + SMC9118_GPIO_CFG, GPIO_CFG_LED_OUT);

	if (dev->forced_media == 0) {
		/* Advertise everything we support */
		advert = smc9118_mii_read(dev, dev->phy_addr, 1) >> 11;
		dev->cfg.flags |= NIC_FLAG_LINK_DOWN;
	} else {
		/*
		 * Force the speed - only advertise a single speed,
		 * and let the duplex be auto-negotiated.
		 */
		advert = dev->forced_media;
		dev->cfg.flags |= NIC_FLAG_LINK_DOWN;

		smc9118_linkup(dev, advert, !dev->negotiate);
	}

	if (advert) {
		if ((rc = MDI_SetAdvert(dev->mdi,
		    dev->phy_addr, advert)) != MDI_SUCCESS) {
			if (dev->cfg.verbose)
				nic_slogf(_SLOGC_NETWORK, _SLOG_INFO,
				    "SetAdvert returned %x", rc);
		} else {
			if (dev->forced_media) {
				/*
				 * Reneg. with newly advertised value.  Don't
				 * use MDI_AutoNegotiate, since it will go and
				 * advertise *everything*.
				 */
				smc9118_mii_write(dev, dev->phy_addr, MDI_BMCR, 0x1200);
			} else {
				/* Reneg. with newly advertised value */
				MDI_AutoNegotiate(dev->mdi, dev->phy_addr, NoWait);
			}
		}
	}
}

int
smc9118_init_phy(smc9118_t *dev)
{
	struct sigevent	mdi_event;
	int				rc;

	mdi_event.sigev_coid = dev->coid;
	MDI_Register_Extended(dev, smc9118_mii_write, smc9118_mii_read,
					smc9118_mii_callback, (mdi_t **)&dev->mdi, &mdi_event, 10, 3);

	for (dev->phy_addr = 1; dev->phy_addr < 32; dev->phy_addr++) {
		if (MDI_FindPhy(dev->mdi, dev->phy_addr) == MDI_SUCCESS &&
			MDI_InitPhy(dev->mdi, dev->phy_addr) == MDI_SUCCESS) {
			if (dev->cfg.verbose & 4)
				nic_slogf(_SLOGC_NETWORK, _SLOG_INFO,
				    "MII transceiver found at address %d", dev->phy_addr);
			break;
		}
	}

	if (dev->phy_addr < 32) {
		smc9118_mii_activate(dev);

		if ((rc = MDI_EnableMonitor(dev->mdi, 1)) != MDI_SUCCESS) {
			nic_slogf(_SLOGC_NETWORK, _SLOG_WARNING,
			    "MDI_EnableMonitor returned %x", rc);
			}
	
		return (0);
	}

	nic_slogf(_SLOGC_NETWORK, _SLOG_INFO, "Unable to find MII transceiver");
	dev->phy_addr = -1;

	return (-1);
}
