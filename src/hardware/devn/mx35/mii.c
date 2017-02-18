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

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

uint16_t	mx35_fec_mii_read (void *handle, uint8_t phy_add, uint8_t reg_add)

{
mx35_t	*ext = (mx35_t *) handle;
volatile	uint32_t	*base = ext->reg;
int			timeout = MX35_TIMEOUT;
uint32_t	phy_data;

	*(base + MX35_IEVENT) |= IEVENT_MII;
	phy_data = ((1 << 30) | (0x2 << 28) | (phy_add << 23) | (reg_add << 18) | (2 << 16));
	*(base + MX35_MII_DATA) = phy_data;
	while (timeout--) {
		if (*(base + MX35_IEVENT) & IEVENT_MII) {
			*(base + MX35_IEVENT) |= IEVENT_MII;
			break;
			}
		nanospin_ns (10000);
		}
	return ((timeout <= 0) ? 0 : (*(base + MX35_MII_DATA) & 0xffff));
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

void	mx35_fec_mii_write (void *handle, uint8_t phy_add, uint8_t reg_add, uint16_t data)

{
mx35_t	*ext = (mx35_t *) handle;
volatile	uint32_t	*base = ext->reg;
int			timeout = MX35_TIMEOUT;
uint32_t	phy_data;

	*(base + MX35_IEVENT) |= IEVENT_MII;
	phy_data = ((1 << 30) | (0x1 << 28) | (phy_add << 23) | (reg_add << 18) | (2 << 16) | data);
	*(base + MX35_MII_DATA) = phy_data;
	while (timeout--) {
		if (*(base + MX35_IEVENT) & IEVENT_MII) {
			*(base + MX35_IEVENT) |= IEVENT_MII;
			break;
			}
		nanospin_ns (10000);
		}
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

void	mx35_fec_mii_callback (void *handle, uchar_t phy, uchar_t newstate)

{
mx35_t		*ext = (mx35_t *) handle;
int				i, mode, oldstate;
char			*s, tmp [10];
nic_config_t	*cfg = &ext->cfg;
volatile		uint32_t	*base = ext->reg;

	switch (newstate) {

		case    MDI_LINK_UP:
			if ((i = MDI_GetActiveMedia (ext->mdi, ext->phy_addr, &mode)) != MDI_LINK_UP) {
				if (cfg->verbose)
					nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "callback GetActiveMedia returned %x", i);
				mode = MDI_10bT;
				}
			if (mode != ext->phy_mode) {
				ext->phy_mode = mode;
				}
			if (cfg->verbose) {
				switch (mode) {
                        
					case    MDI_10bT:
						s = "10bT";
						break;
					case    MDI_10bTFD:
						s = "10bT FD";
						break;
					case    MDI_100bT:
						s = "100bT";
						break;
					case    MDI_100bTFD:
						s = "100bT FD";
						break;
					case    MDI_100bT4:
						s = "100bT4";
						break;
					default:
						sprintf (tmp, "%d", mode);
						s = tmp;
						break;
					}
				nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "Link up %d (%s)", cfg->lan, s);
				}

			oldstate = cfg->duplex;
			cfg->duplex = 0;
			switch (mode) {

				case    MDI_10bTFD:
					cfg->duplex = 1;
				case    MDI_10bT:
					cfg->media_rate = 10000;
					break;
				case    MDI_100bTFD:
					cfg->duplex = 1;
				case    MDI_100bT:
				case    MDI_100bT4:
					cfg->media_rate = 100000;
					break;
				default:
					cfg->media_rate = 100000;
					break;
				}
                
			if (oldstate != (cfg->duplex)) {
				if (cfg->duplex) {
					*(base + MX35_X_CNTRL) |= XCNTRL_FDEN;
					*(base + MX35_R_CNTRL) &= ~RCNTRL_DRT;
					}
				else {
					*(base + MX35_X_CNTRL) &= ~XCNTRL_FDEN;
					*(base + MX35_R_CNTRL) |= RCNTRL_DRT;
					}
				}
			cfg->flags &= ~NIC_FLAG_LINK_DOWN;
			break;

		case    MDI_LINK_DOWN:
			if (cfg->verbose) {
				nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "Link down %d", cfg->lan);
				}
			cfg->media_rate = cfg->duplex = -1;
			cfg->flags |= NIC_FLAG_LINK_DOWN;
			MDI_AutoNegotiate (ext->mdi, ext->phy_addr, NoWait);
			break;

		default:
			break;
		}
}

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

int		mx35_fec_init_phy (mx35_t *ext)

{
int	 				i, phy_idx;
struct sigevent		mdi_event;
nic_config_t		*cfg = &ext->cfg;
int					negotiate = (cfg->media_rate == -1 && cfg->duplex == -1);
int					an_capable;
uint16_t			reg;
uint32_t			tmp;
volatile	uint32_t	*base = ext->reg;

	if (cfg->verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "mx35_fec_init_phy: speed: %d, duplex: %d", cfg->media_rate, cfg->duplex);
		}

	tmp = *(base + MX35_R_CNTRL);
	tmp |= RCNTRL_MII_MODE;
	*(base + MX35_R_CNTRL) = tmp;
	*(base + MX35_MII_SPEED) = (0xd << 1);
	memset (&mdi_event, 0, sizeof (struct sigevent));
	mdi_event.sigev_coid = ext->coid;
	MDI_Register_Extended (ext, mx35_fec_mii_write, mx35_fec_mii_read, mx35_fec_mii_callback, 
		(mdi_t **) &ext->mdi, &mdi_event, 10, 3);

	for (ext->phy_addr = phy_idx = 0; ext->phy_addr < 32; ext->phy_addr++) {

		if (MDI_FindPhy (ext->mdi, ext->phy_addr) == MDI_SUCCESS &&
			MDI_InitPhy (ext->mdi, ext->phy_addr) == MDI_SUCCESS) {
			if (cfg->verbose) {
				nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "devn-mx35: PHY found at address %d.", ext->phy_addr);
				}
			MDI_ResetPhy (ext->mdi, ext->phy_addr, WaitBusy);

			phy_idx++;
			break;
			}
		}

	if (phy_idx) {
		cfg->phy_addr = ext->phy_addr;
		if (negotiate) {
			MDI_AutoNegotiate (ext->mdi, ext->phy_addr, NoWait);
			if ((i = MDI_EnableMonitor (ext->mdi, 1)) != MDI_SUCCESS) {
				if (cfg->verbose) {
					nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "MDI_EnableMonitor returned %x", i);
					}
				}
			}
		else {
			an_capable = mx35_fec_mii_read (ext, cfg->phy_addr, MDI_BMSR) & 8;

			if (ext->force_advertise != -1 || !an_capable) {
				reg = mx35_fec_mii_read (ext, cfg->phy_addr, MDI_BMCR);

				reg &= ~(BMCR_RESTART_AN | BMCR_SPEED_100 | BMCR_FULL_DUPLEX);

				if (an_capable && ext->force_advertise != 0) {
					/*
					 * If we force the speed, but the link partner
					 * is autonegotiating, there is a greater chance
					 * that everything will work if we advertise with
					 * the speed that we are forcing to.
					 */
					MDI_SetAdvert (ext->mdi, cfg->phy_addr, ext->force_advertise);

					reg |= BMCR_RESTART_AN | BMCR_AN_ENABLE;

					if (cfg->verbose)
						nic_slogf(_SLOGC_NETWORK, _SLOG_INFO, "devn-mx35: "
						    "restricted autonegotiate (%dMbps only)", cfg->media_rate / 1000);
					}
				else {
					reg &= ~BMCR_AN_ENABLE;

					if (cfg->verbose)
						nic_slogf(_SLOGC_NETWORK, _SLOG_INFO, "devn-mx35: forcing the link");
					}

				if (cfg->duplex > 0)
					reg |= BMCR_FULL_DUPLEX;
				if (cfg->media_rate == 100 * 1000)
					reg |= BMCR_SPEED_100;

				mx35_fec_mii_write (ext, cfg->phy_addr, MDI_BMCR, reg);

				if (reg & BMCR_AN_ENABLE)
					MDI_EnableMonitor (ext->mdi, 1);
				}
			}
		cfg->connector = NIC_CONNECTOR_MII;
		}

	return (phy_idx);
}
