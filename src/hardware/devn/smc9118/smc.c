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

#define SMC_INTERRUPT_EVENT	0x1a
#define	SMC_TIMER_PULSE		0x55

extern uint32_t smc9118_mac_write( uintptr_t base, uint8_t reg_off, uint32_t data );
extern uint32_t smc9118_mac_read( uintptr_t base, uint8_t reg_off );

io_net_dll_entry_t io_net_dll_entry = {
	2, 
	smc9118_init, 
	NULL
};

io_net_registrant_funcs_t smc9118_funcs = {
	8, 
	NULL,
	smc9118_send_packets, 
	smc9118_receive_complete,
	smc9118_shutdown1,
	smc9118_shutdown2,
	smc9118_advertise,
	smc9118_devctl, 
	smc9118_flush, 
	NULL
};

io_net_registrant_t smc9118_entry = {
	_REG_PRODUCER_UP,
	"devn-smc9118",
	"en",
	NULL,
	NULL,
	&smc9118_funcs,
	0
};

int
smc9118_eeprom_send_cmd( uintptr_t base, int command, int offset )
{
	uint32_t	cmd_reg;
	int			timeout = 100;

	OUTLE32(base + SMC9118_E2P_CMD, 0);
	cmd_reg = (command & 0x7) << 28;
	cmd_reg |= offset;
	cmd_reg |= EPC_BUSY;
	OUTLE32(base + SMC9118_E2P_CMD, cmd_reg);

	do {
		timeout--;
		nanospin_ns(100);
	} while ((INLE32(base + SMC9118_E2P_CMD) & EPC_BUSY));

	if (timeout = 0 && (INLE32(base + SMC9118_E2P_CMD) & EPC_BUSY)) 
		return (-1);
	else
		return (0);
}


/*
 * smc9118_init()
 *
 * Description: driver's io-net entry point
 *
 * Returns: -1 on error
 */
int
smc9118_init(void *dll_hdl, dispatch_t *dpp, io_net_self_t *ion, char *options)
{
	int ret;

	dpp = dpp;
	
	if (ret = smc9118_detect(dll_hdl, ion, options)) {
		errno = ret;
		return -1;
	}

	return 0;
}

int
smc9118_flush(int reg_hdl, void *hdl)
{
	smc9118_t	*dev = hdl;
	npkt_t		*npkt, *tmp;

	pthread_mutex_lock(&dev->mutex);
	npkt = dev->nhead;
	dev->nhead = dev->ntail = NULL;

	for (; npkt; npkt = tmp) {
		tmp = npkt->next;
		dev->ion->tx_done(dev->reg_hdl, npkt);
	}

	pthread_mutex_unlock(&dev->mutex);

	return (0);
}

npkt_t *
smc9118_alloc_npkt(smc9118_t *dev, size_t size) 
{
	npkt_t		*npkt;
	net_buf_t	*nb;
	net_iov_t	*iov;
	char		*ptr;

	if ((npkt = dev->ion->alloc_up_npkt(sizeof(net_buf_t) + sizeof(net_iov_t), 
	    (void **) &nb)) == NULL) {
		return NULL;
	}
	
	if ((ptr = dev->ion->alloc(size, 0)) == NULL) {
		dev->ion->free(npkt);
		return NULL;
	}

	TAILQ_INSERT_HEAD(&npkt->buffers, nb, ptrs);

	iov = (net_iov_t *)(nb + 1);

	nb->niov		= 1;
	nb->net_iov		= iov;

	iov->iov_base   = (void *)ptr;
	iov->iov_len    = size;
	iov->iov_phys   = (paddr_t)(dev->ion->mphys(iov->iov_base));

	npkt->org_data  = ptr;
	npkt->next      = NULL;
	npkt->tot_iov	= 1;

	return npkt;
}

int
smc9118_advertise(int reg_hdl, void *func_hdl)
{
	npkt_t			*npkt;
	smc9118_t		*dev = func_hdl;
	io_net_msg_dl_advert_t	*ap;

	if ((npkt = smc9118_alloc_npkt(dev, SMC9118_MTU_SIZE)) == NULL)
		return -1;

	ap = npkt->org_data;
	npkt->buffers.tqh_first->net_iov->iov_base = npkt->org_data;

	memset(ap, 0x00, sizeof *ap);
	ap->type   = _IO_NET_MSG_DL_ADVERT;
	ap->iflags = (IFF_SIMPLEX | IFF_BROADCAST | IFF_RUNNING);
	if (dev->cfg.flags & NIC_FLAG_MULTICAST)
		ap->iflags |= IFF_MULTICAST;

	ap->mtu_min = 0;
	ap->mtu_max = dev->cfg.mtu;
	ap->mtu_preferred = dev->cfg.mtu;
	strcpy(ap->up_type, (char *)dev->cfg.uptype);
	itoa(dev->cfg.lan, ap->up_type + strlen(ap->up_type), 10);

	strcpy(ap->dl.sdl_data, ap->up_type);

	ap->dl.sdl_len = sizeof(struct sockaddr_dl);
	ap->dl.sdl_family = AF_LINK;
	ap->dl.sdl_index  = dev->cfg.lan;
	ap->dl.sdl_type = dev->cfg.iftype;
	ap->dl.sdl_nlen = strlen(ap->dl.sdl_data); /* not null terminated */
	ap->dl.sdl_alen = 6;
	memcpy(ap->dl.sdl_data + ap->dl.sdl_nlen, dev->cfg.current_address, 6);

	npkt->flags |= _NPKT_MSG;
	npkt->iface = 0;
	npkt->framelen = npkt->buffers.tqh_first->net_iov->iov_len = sizeof *ap;

	if (dev->ion->reg_tx_done(dev->reg_hdl, npkt, dev) == -1) {
		dev->ion->free(ap);
		dev->ion->free(npkt);
		return 0;
	}

	if (dev->ion->tx_up(dev->reg_hdl,
	    npkt, 0, 0, dev->cell, dev->cfg.lan, 0) == 0) {
		dev->ion->tx_done(dev->reg_hdl, npkt);
	}

	return 0;
}

void
smc9118_initialize(smc9118_t *dev)
{
	uint32_t	addrl, addrh, maccr;
	uintptr_t	iobase = dev->iobase;

	/* Set the Ethernet address */
	addrl = dev->cfg.current_address[0] | (dev->cfg.current_address[1] << 8) | (dev->cfg.current_address[2] << 16) |
			(dev->cfg.current_address[3] << 24);
	addrh = dev->cfg.current_address[4] | (dev->cfg.current_address[5] << 8);

	smc9118_mac_write(iobase, SMC9118_ADDRL, addrl);
	smc9118_mac_write(iobase, SMC9118_ADDRH, addrh);

	/* Setup / Disable all interrupts */
	OUTLE32(iobase + SMC9118_INT_EN, 0);
	OUTLE32(iobase + SMC9118_INT_STS, SW_INT | TXSTOP_INT | RXSTOP_INT | RXDFH_INT | TX_IOC |
			RXD_INT | GPT_INT | PHY_INT | PME_INT | TXSO | RWT | RXE | TXE | TDFU | TDFO | TDFA |
			TSFF | TSFL | RXDF_INT | RDFL | RSFF | RSFL);
	OUTLE32(iobase + SMC9118_IRQ_CFG, INT_DEAS | IRQ_EN);

	/* Setup Rx / Tx */
	OUTLE32(iobase + SMC9118_FIFO_INT, (TX_DATA_AVAIL << 24));
	/* Promiscuous bit is on by default in CR, so turn it off */
	maccr = 0;	//smc9118_mac_read(iobase, SMC9118_MAC_CR);
	maccr = TXEN | RXEN | (dev->n_prom ? PRMS:0) | (dev->n_allmulti ? MCPAS: 0);
	smc9118_mac_write(iobase, SMC9118_MAC_CR, maccr);	
	
	OUTLE32(iobase + SMC9118_TX_CFG, TX_ON | TXSAO);
}

void
smc9118_halt(smc9118_t *dev)
{
	uintptr_t	iobase = dev->iobase;
	uint32_t	maccr;

	smc9118_flush(dev->reg_hdl, dev);

	/* allow TX completion interrupts only */
	OUTLE32(iobase + SMC9118_INT_EN, TX_IOC);

	/* disable transmit/receive */
	maccr = smc9118_mac_read(iobase, SMC9118_MAC_CR);
	maccr &= ~(TXEN | RXEN);
	smc9118_mac_write(iobase, SMC9118_MAC_CR, maccr);	

	/* disable interrupts */
	OUTLE32(iobase + SMC9118_INT_EN, 0);
}

void
smc9118_reset(smc9118_t *dev)
{
	smc9118_halt(dev);
	smc9118_initialize(dev);
}

static int
smc9118_read_mac(smc9118_t *dev)
{
	uintptr_t	iobase = dev->iobase;
	uint32_t	addrl = 0, addrh = 0;

	/*
	 * Trigger an EEPROM reload, so we get the real MAC address, not
	 * the one that was programmed the last time the driver ran.
	 */
	smc9118_eeprom_send_cmd(iobase, EPC_RELOAD, 0);

	/* Check for MAC loaded */
	if (INLE32(iobase + SMC9118_E2P_CMD) & MAC_ADDR_LOADED) {
		addrl = smc9118_mac_read(iobase, SMC9118_ADDRL);
		((short *)dev->cfg.permanent_address)[0] = addrl & 0xffff;
		((short *)dev->cfg.permanent_address)[1] = addrl >> 16;
		addrh = smc9118_mac_read(iobase, SMC9118_ADDRH);
		((short *)dev->cfg.permanent_address)[2] = addrh & 0xffff;
	}

	/* check for command line override */
	if (memcmp(dev->cfg.current_address, "\0\0\0\0\0\0", 6) == 0) {
		if (addrl == 0 && addrh == 0) {
			nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devn-smc9118: MAC address not specified on cmdline.");
			return -1;
		}
		memcpy(dev->cfg.current_address, dev->cfg.permanent_address, ETH_MAC_LEN);
	}

	return 0;
}

static int
smc9118_config(smc9118_t *dev)
{
	npkt_t	*npkt;
	int		i;

	if (smc9118_read_mac(dev))
		return -1;

	smc9118_reset(dev);

	for (i = 0; i < dev->max_rx_buffers; i++) {
		if ((npkt = smc9118_alloc_npkt(dev, SMC9118_MTU_SIZE)) == NULL) { 
			for(; npkt = dev->rx_free;) {
				dev->rx_free = npkt->next;
				dev->ion->free(npkt->org_data);
				dev->ion->free(npkt);
			}
			nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devn-smc9118: Unable to alloc receive packet\n");
			return -1;
		}
		npkt->flags = _NPKT_UP;
		npkt->next = dev->rx_free;
		dev->rx_free = npkt;
		dev->num_rx_free++;
	}

	if (smc9118_init_phy(dev) == -1) {
		dev->cfg.media_rate = 10 * 1000;
		dev->phy_addr = -1;
	}

	if (dev->cfg.verbose)
		nic_dump_config(&dev->cfg);

	dev->stats.un.estats.valid_stats =
	    NIC_ETHER_STAT_INTERNAL_TX_ERRORS |
		NIC_ETHER_STAT_DRIBBLE_BITS |
	    NIC_ETHER_STAT_INTERNAL_RX_ERRORS |
	    NIC_ETHER_STAT_NO_CARRIER |
	    NIC_ETHER_STAT_TX_DEFERRED |
		NIC_ETHER_STAT_LENGTH_FIELD_MISMATCH |
	    NIC_ETHER_STAT_EXCESSIVE_DEFERRALS |
	    NIC_ETHER_STAT_XCOLL_ABORTED |
	    NIC_ETHER_STAT_LATE_COLLISIONS |
	    NIC_ETHER_STAT_SINGLE_COLLISIONS |
	    NIC_ETHER_STAT_MULTI_COLLISIONS |
	    NIC_ETHER_STAT_ALIGN_ERRORS |
	    NIC_ETHER_STAT_FCS_ERRORS |
	    NIC_ETHER_STAT_SHORT_PACKETS |
	    NIC_ETHER_STAT_OVERSIZED_PACKETS;

	dev->stats.valid_stats = NIC_STAT_RX_FAILED_ALLOCS |
	    NIC_STAT_TXED_MULTICAST | NIC_STAT_TXED_BROADCAST |
	    NIC_STAT_RXED_MULTICAST | NIC_STAT_RXED_BROADCAST;

	return 0;
}

int
smc9118_register_device(smc9118_t *dev, io_net_self_t *ion, void *dll_hdl)
{
	pthread_attr_t		pattr;
	struct sched_param	param;
	struct sigevent		event;
	uint16_t			lan;
	char				*err = NULL;

	dev->ion = ion;
	dev->dll_hdl = dll_hdl;
	smc9118_entry.func_hdl = dev;
	smc9118_entry.top_type = (char *)dev->cfg.uptype;
	smc9118_entry.reg_pmmparent = dev->pmmparent;

 	if (dev->tx_max_buffer == 0)
		dev->tx_max_buffer = 100;

	pthread_attr_init(&pattr);
	pthread_attr_setschedpolicy(&pattr, SCHED_RR);
	param.sched_priority = dev->cfg.priority ? dev->cfg.priority : 21;
	pthread_attr_setschedparam(&pattr, &param);
	pthread_attr_setinheritsched(&pattr, PTHREAD_EXPLICIT_SCHED);

	if ((dev->chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1) {
		err = "ChannelCreate";
		goto fail;
	}

	if ((dev->coid = ConnectAttach(0, 0,
	    dev->chid, _NTO_SIDE_CHANNEL, 0)) == -1) {
		err = "ConnectAttach";
		goto fail1;
	}

	if (pthread_mutex_init(&dev->mutex, NULL) == -1) {
		err = "pthread_mutex_init";
		goto fail2;
	}

	if (pthread_mutex_init(&dev->rx_freeq_mutex, NULL) == -1) {
		err = "pthread_mutex_init";
		goto fail3;
	}

	/* create the interface/interrupt/event thread */
	if (pthread_create(&dev->tid,
	    &pattr, (void *)smc9118_event_handler, dev) != 0) {
		err = "pthread_create";
		goto fail4;
	}

	if (smc9118_config(dev) == -1) {
		err = "smc9118_config";
		goto fail6;
	}

	smc9118_entry.flags = _REG_PRODUCER_UP | _REG_TRACK_MCAST;

	if (dev->cfg.lan != -1 ) {
		smc9118_entry.flags |= _REG_ENDPOINT_ARG;
		lan = dev->cfg.lan;
	}

	if (dev->ion->reg(dll_hdl,
	    &smc9118_entry, &dev->reg_hdl, &dev->cell, &lan) == -1) {
		err = "ion_register";
		goto fail6;
	}

	event.sigev_notify = SIGEV_PULSE;
	event.sigev_coid = dev->coid;
	event.sigev_code = SMC_INTERRUPT_EVENT;
	event.sigev_priority = dev->cfg.priority ? dev->cfg.priority : 21;

	if ((dev->iid = InterruptAttachEvent(dev->cfg.irq[0],
	    &event, _NTO_INTR_FLAGS_TRK_MSK)) == -1) {
		err = "InterruptAttachEvent";
		goto fail7;
	}

	/* enable interrupts */
	OUTLE32(dev->iobase + SMC9118_INT_EN, TX_IOC | RSFL_EN);

	OUTLE32(dev->iobase + SMC9118_INT_STS, SW_INT | TXSTOP_INT | RXSTOP_INT | RXDFH_INT | TX_IOC |
			RXD_INT | GPT_INT | PHY_INT | PME_INT | TXSO | RWT | RXE | TXE | TDFU | TDFO | TDFA |
			TSFF | TSFL | RXDF_INT | RDFL | RSFF | RSFL);

	dev->cfg.lan = lan;
	dev->ion->devctl(dev->reg_hdl, DCMD_IO_NET_MAX_QUEUE,
		&dev->tx_max_buffer, sizeof(dev->tx_max_buffer), NULL);

	return 0;

fail7:
	dev->ion->dereg(dev->reg_hdl);

fail6:

//fail5:
	pthread_kill(dev->tid, SIGKILL);

fail4:
	pthread_mutex_destroy(&dev->rx_freeq_mutex);

fail3:
	pthread_mutex_destroy(&dev->mutex);

fail2:
	ConnectDetach(dev->coid);

fail1:
	ChannelDestroy(dev->chid);

fail:
	nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR, "devn-smc9118: %s", err);

	return -1;
}

static void
smc9118_process_interrupt(smc9118_t *dev)
{
	uint32_t				istat, status, ienable;
	int						kick_xmit;
	uintptr_t				iobase = dev->iobase;
	nic_ethernet_stats_t    *estats = &dev->stats.un.estats;

recheck:
	kick_xmit = 0;
	istat = INLE32(iobase + SMC9118_INT_STS);
	ienable = INLE32(iobase + SMC9118_INT_EN);

	istat = istat & ienable;

	do {
		OUTLE32(iobase + SMC9118_INT_STS, istat);
		if (istat & RSFL)
			smc9118_receive(dev);

		if (istat & TX_IOC) {

			status = INLE32(iobase + TX_FIFO_STS_PORT);

			if (status & TX_STS_ERR_STS) {
				if (status & TX_STS_LOC || status & TX_STS_NOC)
					estats->no_carrier++;
				if (status & TX_STS_EXC_COLL)
					estats->xcoll_aborted++;
				if (status & TX_STS_LATE_COLL)
					estats->late_collisions++;
				if (status & TX_STS_COLL_CNT && !(status & TX_STS_EXC_COLL))
					estats->single_collisions += status & TX_STS_COLL_CNT;
				if (status & TX_STS_EXC_DEFERRAL)
					estats->tx_deferred++;
				if (status & TX_STS_UNDERRUN)
					estats->internal_tx_errors++;
				if (status & TX_STS_DEFERRED)
					estats->tx_deferred++;
			}
			else
				dev->stats.txed_ok++;
			kick_xmit = 1;
		}
		
		istat = INLE32(iobase + SMC9118_INT_STS) & ienable;
	} while (istat);

	if (kick_xmit) {
		if (dev->nhead) 
			smc9118_send(dev);
		goto recheck;
	}
}

void
smc9118_event_handler(smc9118_t *dev)
{
	struct _pulse	pulse;
	iov_t			iov;
	int				rcvid;

	SETIOV(&iov, &pulse, sizeof(pulse));

	while (1) {
		if ((rcvid = MsgReceivev(dev->chid, &iov, 1, NULL)) == -1) {
			if (errno == ESRCH)
				pthread_exit(NULL);

			continue;
		}

		pthread_mutex_lock(&dev->mutex);
		switch(pulse.code) {
			case SMC_INTERRUPT_EVENT:
				smc9118_process_interrupt(dev);
				InterruptUnmask(dev->cfg.irq[0], dev->iid);
				break;

			case MDI_TIMER:
				if (!dev->did_rx || (dev->cfg.flags & NIC_FLAG_LINK_DOWN))
					MDI_MonitorPhy(dev->mdi);
				dev->did_rx = 0;
				break;

			default:
				if (rcvid) {
					MsgReplyv(rcvid, ENOTSUP, &iov, 1);
				}
				break;
		}
		pthread_mutex_unlock(&dev->mutex);
	}
}

__SRCVERSION("smc.c $Rev: 262087 $");
