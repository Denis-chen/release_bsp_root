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

// perform sanity check on multicast address range length
static int
mcast_range2big(void *start_p, void *end_p) {
    unsigned char      *start = start_p;
    unsigned char      *end   = end_p;
    unsigned long long  sval;
    unsigned long long  eval;

    sval = start[5] + (start[4] << 8) + (start[3] << 16) + (start[2] << 24) + 
      ((unsigned long long)start[1] << 32) + ((unsigned long long)start[0] << 40);

    eval = end[5] + (end[4] << 8) + (end[3] << 16) + (end[2] << 24) + 
      ((unsigned long long)end[1] << 32) + ((unsigned long long)end[0] << 40);

    if (eval-sval > 2000000) {
        return 1;
    }
    return 0;
}


/* Adds one to a MAC address */
static void
bump_macaddr(uint8_t *mcaddr)
{
	int	i;

	for (i = 5; i >= 0; i--) {
		if (mcaddr[i] + 1 < 0x100) {
			mcaddr[i]++;
			while (i++ < 6)
				mcaddr[i] = 0;
			return;
		}
	}
}

static void
mcast_add(smc9118_t *dev, uint8_t *mcaddr)
{
uint8_t		high_bits;
uint32_t	mcast;
int			reg = 1;
uintptr_t	iobase = dev->iobase;

	high_bits = (*mcaddr >> 2);
	if (high_bits & (1 << 6))
		reg = 0;
	high_bits &= ~(1 << 6);
	mcast = smc9118_mac_read (iobase, SMC9118_HASHH + reg);
	mcast |= (1 << (high_bits + 1));
	smc9118_mac_write (iobase, SMC9118_HASHH + reg, mcast);
	mcast = smc9118_mac_read (iobase, SMC9118_MAC_CR);
	if (~(mcast & HPFILT))
		smc9118_mac_write (iobase, SMC9118_MAC_CR, (mcast | HPFILT));
}

static void
mcast_remove(smc9118_t *dev, uint8_t *mcaddr)
{
uint8_t		high_bits;
uint32_t	mcast, mcast1;
int			reg = 1;
uintptr_t	iobase = dev->iobase;

	high_bits = (*mcaddr >> 2);
	if (high_bits & (1 << 6))
		reg = 0;
	high_bits &= ~(1 << 6);
	mcast = smc9118_mac_read (iobase, SMC9118_HASHH + reg);
	mcast &= ~(1 << (high_bits + 1));
	smc9118_mac_write (iobase, SMC9118_HASHH + reg, mcast);
	mcast = smc9118_mac_read (iobase, SMC9118_HASHH);
	mcast1 = smc9118_mac_read (iobase, SMC9118_HASHL);
	if (mcast == 0 && mcast1 == 0) {
		mcast = smc9118_mac_read (iobase, SMC9118_MAC_CR);
		smc9118_mac_write (iobase, SMC9118_MAC_CR, (mcast & ~HPFILT));
		}
}

static void
enable_prom_mcast(smc9118_t *dev)
{
	uintptr_t	iobase = dev->iobase;
	uint32_t	mac;

	mac = smc9118_mac_read (iobase, SMC9118_MAC_CR);
	if (dev->n_allmulti++ == 0) {
		/* Enable promiscuous multicast */
		smc9118_mac_write (iobase, SMC9118_MAC_CR, (mac | MCPAS));
	}
}

static void
disable_prom_mcast(smc9118_t *dev)
{
	uintptr_t	iobase = dev->iobase;
	uint32_t	mac;

	mac = smc9118_mac_read (iobase, SMC9118_MAC_CR);
	switch (dev->n_allmulti) {
		case 0:
			break;
		case 1:
			smc9118_mac_write (iobase, SMC9118_MAC_CR, (mac & ~MCPAS));
			/* Fallthrough */
		default:
			dev->n_allmulti--;
			break;
	}
}

static int
handle_mcast(smc9118_t *dev, io_net_msg_join_mcast_t *mcast)
{
	uint8_t		tmp_mcaddr[6];

	while (mcast) {
		switch (mcast->type) {
		case _IO_NET_JOIN_MCAST:
			if (mcast->flags & _IO_NET_MCAST_ALL) {
                enable_prom_mcast(dev);
			} else {
				if (nic_ether_mcast_valid(mcast) == -1)
					return EINVAL;

                if (mcast_range2big(&mcast->mc_min.addr_dl, &mcast->mc_max.addr_dl)) {
					// map a big range addition request
                    // into a JOIN MCAST_ALL request
 	                enable_prom_mcast(dev);  
                    break;
                }

				memcpy(tmp_mcaddr, LLADDR(&mcast->mc_min.addr_dl), 6);
				while (1) {
					mcast_add(dev, tmp_mcaddr);
					if (!memcmp(tmp_mcaddr,
					    LLADDR(&mcast->mc_max.addr_dl), 6))
						break;
					bump_macaddr(tmp_mcaddr);
				}
			}
			break;

		case _IO_NET_REMOVE_MCAST:
			if (mcast->flags & _IO_NET_MCAST_ALL) {
                disable_prom_mcast(dev);
			} else {
                if (mcast_range2big(&mcast->mc_min.addr_dl, &mcast->mc_max.addr_dl)) {
					// map a big range removal request
                    // into a REMOVE MCAST_ALL request
					disable_prom_mcast(dev);
                    break;                    
                }

				memcpy(tmp_mcaddr, LLADDR(&mcast->mc_min.addr_dl), 6);
				while (1) {
					mcast_remove(dev, tmp_mcaddr);
					if (!memcmp(tmp_mcaddr,
					    LLADDR(&mcast->mc_max.addr_dl), 6))
						break;
					bump_macaddr(tmp_mcaddr);
				}
			}
			break;

		default:
			if (dev->cfg.verbose)
				nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
				    "devn-smc9118: message 0x%x not supported",
				    mcast->type);
			return EINVAL;
		}

		if (mcast->type == _IO_NET_JOIN_MCAST)
			        break;

		mcast = mcast->next;
	}
	return EOK;
}

int
smc9118_devctl(void *hdl, int dcmd, void *data, size_t size, union _io_net_dcmd_ret_cred *ret)
{
	smc9118_t		*dev = hdl;
	int				status = EOK;
	unsigned		mac;

	pthread_mutex_lock(&dev->mutex);

	switch (dcmd) {
		case DCMD_IO_NET_GET_STATS:
			memcpy(data, &dev->stats, sizeof (dev->stats));
			break;

		case DCMD_IO_NET_GET_CONFIG:
			memcpy(data, &dev->cfg, sizeof (dev->cfg));
			break;

		case DCMD_IO_NET_PROMISCUOUS:
			if (ret->ret_cred.cred->euid) {
				break;
			}

			mac = smc9118_mac_read (dev->iobase, SMC9118_MAC_CR);
			if (*(int *)data) {
				if (dev->n_prom++ == 0) {
					smc9118_mac_write (dev->iobase, SMC9118_MAC_CR, (mac | RXALL));
					dev->cfg.flags |= NIC_FLAG_PROMISCUOUS;
				}
			} else {
				switch (dev->n_prom) {
					case 0:
						break;
					case 1:
						smc9118_mac_write (dev->iobase, SMC9118_MAC_CR, (mac & ~RXALL));
						dev->cfg.flags &= ~NIC_FLAG_PROMISCUOUS;
						/* Fallthrough */
					default:
						dev->n_prom--;
						break;
				}
			}
			break;

		case DCMD_IO_NET_CHANGE_MCAST:
			if (ret->ret_cred.cred->euid)
				break;

			if (dev->cfg.flags & NIC_FLAG_MULTICAST) {
				status = handle_mcast(dev,
				    (io_net_msg_join_mcast_t *)data);
			} else
				status = ENOTSUP;
			break;

		default:
			status = ENOTSUP;
			break;
	}

	pthread_mutex_unlock(&dev->mutex);

	return status;
}
