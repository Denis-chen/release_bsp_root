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


#include <net/if_ether.h>


io_net_dll_entry_t io_net_dll_entry = {
					2, 
					mx35_fec_init, 
					NULL 
					};

io_net_registrant_funcs_t mx35_fec_funcs = {
							8, 
							NULL, 
							mx35_fec_send_packets,
							mx35_fec_receive_complete, 
							mx35_fec_shutdown1,
							mx35_fec_shutdown2,
							mx35_fec_advertise,
							mx35_fec_devctl, 
							mx35_fec_flush, 
							NULL
							};

io_net_registrant_t mx35_fec_entry = {
					_REG_PRODUCER_UP | _REG_TRACK_MCAST,
					"devn-mx35",
					"en",
					NULL,
					NULL,
					&mx35_fec_funcs,
					0
					};

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

/* 
 * mx35_fec_init
 *
 * Initial entry point for io-net into driver.  Initiates a search
 * for nic.
 *
 * return  0 on success
 *        -1 and set errno on error
 */

int		mx35_fec_init (void *dll_hdl, dispatch_t *dpp, io_net_self_t *ion, char *opts)

{
int		rval;

	dpp = dpp;             
        
	if (rval = mx35_fec_detect (dll_hdl, ion, opts)) {
		errno = rval;
		return -1;
		}
	return (0);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

npkt_t *mx35_fec_alloc_npkt (mx35_t *ext, size_t size) 

{
npkt_t			*npkt;
net_buf_t		*nb;
net_iov_t		*iov;
char			*ptr;
int				linesize = MX35_CACHE_LINE_SIZE;

	if (size < MX35_MTU_SIZE)
		size = MX35_MTU_SIZE;

	if ((npkt = ion_alloc_npkt (sizeof (net_buf_t) + sizeof (net_iov_t), 
		(void **) &nb)) == NULL) {
		return (NULL);
		}

	if ((ptr = ion_alloc (size + linesize * 2, 0)) == NULL) {
		ion_free (npkt);
		return (NULL);
		}

	npkt->org_data = ptr;

	/* Pad buffer to begin on a cache line boundary */
	if (linesize != 0)
		ptr = (char *)((unsigned)(ptr + (linesize-1)) & ~(linesize-1));

	TAILQ_INSERT_HEAD (&npkt->buffers, nb, ptrs);

	iov = (net_iov_t *)(nb + 1);

	nb->niov = 1;
	nb->net_iov = iov;

	iov->iov_base = (void *) ptr;
	iov->iov_len = size;
	iov->iov_phys = (paddr_t)(ion_mphys (iov->iov_base));

	npkt->next = NULL;
	npkt->tot_iov = 1;

	return (npkt);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static int	mx35_fec_process_interrupt (mx35_t *ext)

{
volatile uint32_t	*base = ext->reg;
uint32_t	ievent;

	for (;;) {
		ievent = *(base + MX35_IEVENT);

		*(base + MX35_IEVENT) = ievent;

		if (!ievent) {
			break;
			}

		if ((ievent & (IEVENT_RFINT | IEVENT_RXB)) != 0) {
			mx35_fec_receive (ext);
			}
		
		if ((ievent & (IEVENT_TFINT | IEVENT_TXB | IEVENT_XFIFO_UN)) != 0) {
			ext->tx_status = ievent;
			mx35_fec_download_complete (ext);
			}

		}
	InterruptUnmask (ext->cfg.irq [0], ext->iid);
	return (0);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static void mx35_fec_event_handler (mx35_t *ext)

{
struct _pulse	pulse;
iov_t			iov;
int				rcvid;

	SETIOV (&iov, &pulse, sizeof(pulse));

	while (1) {

		rcvid = MsgReceivev (ext->chid, &iov, 1, NULL);

		if (rcvid < 0) {
			if (errno == ESRCH) {
				pthread_exit (NULL);
				}
			continue;
			}
		
		switch (pulse.code) {

			case	MDI_TIMER:
				MDI_MonitorPhy (ext->mdi);
//				mx35_fec_download_complete (ext);
				break;

			case	NIC_INTR_EVENT:
				mx35_fec_process_interrupt (ext);
				break;

			default:
				if (rcvid) {
					MsgReplyv (rcvid, ENOTSUP, &iov, 1);
					}
				break;
			}
		}
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static void *mx35_fec_driver_thread (void *data)

{
	mx35_fec_event_handler ((mx35_t *) data);
	return (NULL);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static paddr_t vtophys (void *addr)

{
off64_t                 offset;

	if (mem_offset64 (addr, NOFD, 1, &offset, 0) == -1) {
		return (-1);
		}
	return (offset);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static int	mx35_fec_init_memory (mx35_t *ext)

{
	/* set up the descriptors and packet pools/lists */
	
	if ((ext->rx_bd = mmap (NULL, sizeof (mx35_fec_bd_t) * ext->num_rx_descriptors, 
	    PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_ANON | MAP_PHYS /*| MAP_PRIVATE*/, NOFD, 0)) == MAP_FAILED) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "mmap at \"%s\":%d: %s\n",
			__FILE__, __LINE__, strerror(errno));
		return (errno);
		}

	if ((ext->rx_pktq = calloc (ext->num_rx_descriptors, sizeof(npkt_t *))) == NULL) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "calloc at \"%s\":%d: %s\n",
			__FILE__, __LINE__, strerror(errno));
		return (errno);
		}

	if ((ext->tx_bd = mmap (NULL, sizeof (mx35_fec_bd_t) * ext->num_tx_descriptors, 
	    PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_ANON | MAP_PHYS /*| MAP_PRIVATE*/, NOFD, 0)) == MAP_FAILED) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "mmap at \"%s\":%d: %s\n",
			__FILE__, __LINE__, strerror(errno));
		return (errno);
		}

	if ((ext->tx_pktq = calloc (ext->num_tx_descriptors, sizeof(npkt_t *))) == NULL) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "calloc at \"%s\":%d: %s\n",
			__FILE__, __LINE__, strerror(errno));
		return (errno);
		}

	return (EOK);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static int	mx35_fec_init_list (mx35_t *ext)

{
mx35_fec_bd_t		*bd;
int 			i;
net_iov_t		*iov;
npkt_t          *npkt;

	/* initialize the receive ring */
	
	for (i = 0, bd = (mx35_fec_bd_t *) ext->rx_bd; i < ext->num_rx_descriptors; i++, bd++) {
		if ((npkt = mx35_fec_alloc_npkt (ext, MX35_MTU_SIZE)) == NULL) {
			errno = ENOBUFS;
			return -1;
			}
		if (i == ext->num_rx_descriptors - 1) {
			bd->status = (RXBD_W | RXBD_E);	/* Wrap indicator */
			}
		else {
			bd->status = (RXBD_E);
			}

		ext->rx_pktq[i] = npkt;
		
		iov = TAILQ_FIRST(&(ext->rx_pktq[i]->buffers))->net_iov;
		CACHE_INVAL(&ext->cachectl, iov->iov_base, iov->iov_phys, iov->iov_len);
		bd->buffer = iov->iov_phys;
		bd->length = 0;
		}
	ext->rx_cidx = 0;
                	
	/* setup the tx_addr_table */
	for (i = 0; i < ext->num_tx_descriptors; i++) {
		ext->tx_bd[i].status = 0;
		}
	ext->tx_bd[ext->num_tx_descriptors - 1].status = TXBD_W;
	
	ext->tx_pidx = ext->tx_cidx = 0;

	return (EOK);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static	void	read_phys_addr (mx35_t *ext)

{
volatile uint32_t	*base = ext->reg;
uint32_t	addr [2];

	addr [0] = *(base + MX35_PADDR1);
	addr [1] = *(base + MX35_PADDR2);
	if (ext->cfg.verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, "MAC addr1 %08x - addr2 %08x", addr [0], addr [1]);
		}
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static	void	set_phys_addr (mx35_t *ext)

{
volatile uint32_t	*base = ext->reg;

//	*(base + MX35_PADDR1) = *(uint32_t *) ext->cfg.current_address;
	*(base + MX35_PADDR1) = (ext->cfg.current_address [0] << 24 |
		ext->cfg.current_address [1] << 16 | ext->cfg.current_address [2] << 8 |
		ext->cfg.current_address [3]);
	*(base + MX35_PADDR2) = (ext->cfg.current_address [4] << 24 |
		ext->cfg.current_address [5] << 16 | 0x8808);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

static int	mx35_fec_config (mx35_t *ext)

{
nic_config_t			*cfg = &ext->cfg;
nic_ethernet_stats_t	*estats = &ext->stats.un.estats;
nic_stats_t				*gstats = &ext->stats;
uint32_t				addr;
volatile	uint32_t	*base = ext->reg;

	/* nic ethernet default params */
	cfg->media = NIC_MEDIA_802_3;
	cfg->mac_length = ETH_MAC_LEN;
	if (cfg->mtu == 0 || cfg->mtu > ETH_MAX_PKT_LEN ) {
		cfg->mtu = ETH_MAX_PKT_LEN;
		}
	cfg->mru = cfg->mtu;

	gstats->media = cfg->media;
	estats->valid_stats =
		NIC_ETHER_STAT_INTERNAL_TX_ERRORS |
		NIC_ETHER_STAT_INTERNAL_RX_ERRORS |
		NIC_ETHER_STAT_TX_DEFERRED |
		NIC_ETHER_STAT_XCOLL_ABORTED |
		NIC_ETHER_STAT_LATE_COLLISIONS |
		NIC_ETHER_STAT_SINGLE_COLLISIONS |
		NIC_ETHER_STAT_MULTI_COLLISIONS |
		NIC_ETHER_STAT_TOTAL_COLLISION_FRAMES |
		NIC_ETHER_STAT_ALIGN_ERRORS |
		NIC_ETHER_STAT_FCS_ERRORS |
		NIC_ETHER_STAT_JABBER_DETECTED |
		NIC_ETHER_STAT_OVERSIZED_PACKETS |
		NIC_ETHER_STAT_SHORT_PACKETS |
		NIC_ETHER_STAT_LENGTH_FIELD_OUTRANGE |
		NIC_ETHER_STAT_LENGTH_FIELD_MISMATCH |
		NIC_ETHER_STAT_EXCESSIVE_DEFERRALS;


	if (ext->num_rx_descriptors <= 0) {
		ext->num_rx_descriptors = DEFAULT_NUM_RX_DESCRIPTORS;
		}
	
	if (ext->num_tx_descriptors <= 0) {
		ext->num_tx_descriptors = DEFAULT_NUM_TX_DESCRIPTORS;
		}

	if (mx35_fec_init_memory (ext) != EOK) {
		return (-1);
		}

	if (mx35_fec_init_list (ext) != EOK) {
		return (-1);
		}

	*(base + MX35_ECNTRL) = ECNTRL_RESET;
	nanospin_ns (10000);
	*(base + MX35_ECNTRL) = 0;		/* Seems like the reset bit doesn't auto clear */

	if (*(base + MX35_ECNTRL) & ECNTRL_RESET) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "Reset bit didn't clear");
		return (-1);
		}

	*(base + MX35_IEVENT) = 0xffffffff;

	/* get our physical address */
	if (nic_get_syspage_mac ((char *)cfg->permanent_address) == -1) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "devn-mx35: nic_get_syspage_mac failed");
		}

	/* set up our address */

	/* check for command line override */
	if (memcmp (cfg->current_address, "\0\0\0\0\0\0", 6) == 0) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "You must specify a MAC address");
		return (-1);
		}

	set_phys_addr (ext);
	read_phys_addr (ext);

	*(base + MX35_X_DES_START)  = vtophys ((void *) ext->tx_bd);
	*(base + MX35_R_DES_START)  = vtophys ((void *) ext->rx_bd);
	*(base + MX35_X_WMRK) = 0x03;
//	*(base + MX35_X_WMRK) = 0x00;
	*(base + MX35_R_BUFF_SIZE) = MX35_MTU_SIZE;

	*(base + MX35_X_CNTRL) = (cfg->duplex ? XCNTRL_FDEN : 0);
	
	*(base + MX35_IADDR1) = 0x0;
	*(base + MX35_IADDR2) = 0x0;
	*(base + MX35_GADDR1) = 0x0;
	*(base + MX35_GADDR2) = 0x0;
	addr = (1518 << 16) | RCNTRL_MII_MODE;
	if (cfg->flags & NIC_FLAG_PROMISCUOUS) {
		addr |= RCNTRL_PROM;
		}
	*(base + MX35_R_CNTRL) = addr;
//	*(base + MX35_DMA_CONTROL) = (DMA_DATA_BO | DMA_DESC_BO);
	*(base + MX35_IMASK) = (IMASK_TBIEN | IMASK_TFIEN | IMASK_RBIEN | IMASK_RFIEN | IMASK_BREN | IMASK_EBERREN |
						IMASK_XFUNEN | IMASK_XFERREN | IMASK_RFERREN);

	mx35_fec_init_phy (ext);

	/* Clear all MIB registers */
	for (addr = MX35_RMON_T_DROP; addr <= MX35_IEEE_OCTETS_OK; addr++)
		*(base + addr) = 0;

	*(base + MX35_MIB_CONTROL) &= ~MIB_DISABLE;
	*(base + MX35_ECNTRL) = ECNTRL_ETHER_EN | ECNTRL_FEC_OE;
	*(base + MX35_R_DES_ACTIVE) = R_DES_ACTIVE;

	return (EOK);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

int 	mx35_fec_register_device (mx35_t *ext, io_net_self_t *ion, void *dll_hdl)

{
nic_config_t			*cfg = &ext->cfg;
pthread_attr_t			pattr;
pthread_mutexattr_t		mattr;
struct sched_param		param;
struct sigevent			event;
uint16_t				lan;
int						ret, err = 0;

	ext->ion = ion;
	ext->dll_hdl = dll_hdl;

	if ((ext->chid = ChannelCreate (_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) < 0) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "ChannelCreate at \"%s\":%d: %s\n",
			__FILE__, __LINE__, strerror(errno));
		goto error;
		}
	err++;

	if ((ext->coid = ConnectAttach (0, 0, ext->chid, _NTO_SIDE_CHANNEL, 0)) < 0) {
		nic_slogf( _SLOGC_NETWORK, _SLOG_ERROR, "devn-mx35:  Unable to ConnectAtttach" );
		goto error;
		}
	err++;

	ext->cachectl.fd = NOFD;
	if (cache_init(0, &ext->cachectl, NULL) == -1) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "devn-mx35: cache_init() failed");
		goto error;
	}
	err++;

	/* we need a valid connection id before calling mx35_fec_config */
	if (mx35_fec_config (ext) != EOK) {
		nic_slogf ( _SLOGC_NETWORK, _SLOG_ERROR, "devn-mx35:  mx35_fec_config error" );
		goto error;
		}

	pthread_mutexattr_init (&mattr);

	if (pthread_mutex_init (&ext->tx_mutex, &mattr) != EOK) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "devn-mx35:  Unable to initialize TX mutex");
		goto error;
		}
	err++;

	if (pthread_mutex_init (&ext->rx_free_pkt_q_mutex, &mattr) != EOK) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "devn-mx35:  Unable to initialize RX Free Queue mutex");
		goto error;
		}
	err++;

	mx35_fec_entry.func_hdl = (void *) ext;
	mx35_fec_entry.top_type = (char *)cfg->uptype;

	if (cfg->lan != -1) {
		mx35_fec_entry.flags |= _REG_ENDPOINT_ARG;
		lan = cfg->lan;
		}

	if (ion_register (dll_hdl, &mx35_fec_entry, &ext->reg_hdl, &ext->cell, &lan) < 0) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "ion_register at \"%s\":%d: %s\n",
			__FILE__, __LINE__, strerror(errno));
		goto error;
		}
	cfg->lan = lan;

	if (ext->ion->devctl (ext->reg_hdl, DCMD_IO_NET_VERSION, &ext->version, sizeof (ext->version), NULL)) {
		ext->version = 0;
		}

	pthread_attr_init (&pattr);
	pthread_attr_setschedpolicy (&pattr, SCHED_RR);
	param.sched_priority = cfg->priority;
	pthread_attr_setschedparam (&pattr, &param);
	pthread_attr_setinheritsched (&pattr, PTHREAD_EXPLICIT_SCHED);

	if ((ret = pthread_create (&ext->tid, &pattr,(void *) mx35_fec_driver_thread, ext)) != EOK) {
		errno = ret;
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "pthread_create at \"%s\":%d: %s\n",
			__FILE__, __LINE__, strerror(errno));
		goto error;
		}
	err++;

	memset (&event, 0, sizeof(event));
	SIGEV_PULSE_INIT (&event, ext->coid, cfg->priority, NIC_INTR_EVENT, 0);

	if ((ext->iid = InterruptAttachEvent (cfg->irq [0], &event, _NTO_INTR_FLAGS_TRK_MSK)) < 0) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_ERROR, "InterruptAttachEvent at \"%s\":%d: %s\n",
			__FILE__, __LINE__, strerror(errno));
		goto error;
		}
	err++;

	memset (&event, 0, sizeof(event));
	SIGEV_PULSE_INIT (&event, ext->coid, cfg->priority, NIC_INTR_EVENT, 0);

	return (EOK);

error:

	switch (err) {
		case	6:
			pthread_kill (ext->tid, SIGKILL);
		case	5:
			pthread_mutex_destroy (&ext->rx_free_pkt_q_mutex);
		case	4:
			pthread_mutex_destroy (&ext->tx_mutex);
		case	3:
			cache_fini (&ext->cachectl);
		case	2:
			ConnectDetach (ext->coid);
		case	1:
			ChannelDestroy (ext->chid);
		}
	return (errno);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

int		mx35_fec_advertise (int reg_hdl, void *func_hdl)

{
npkt_t					*npkt;
net_iov_t				*iov;
mx35_t    		    *ext = (mx35_t *) func_hdl;
io_net_msg_dl_advert_t	*ap;
nic_config_t			*cfg = &ext->cfg;


	_mutex_lock (&ext->rx_free_pkt_q_mutex);
	if ((npkt = ext->rx_free_pkt_q)) {
		ext->rx_free_pkt_q = npkt->next;
		npkt->next = NULL;
		ext->num_rx_free--;
		_mutex_unlock (&ext->rx_free_pkt_q_mutex);
		}
	else {
		_mutex_unlock (&ext->rx_free_pkt_q_mutex);
		if ((npkt = mx35_fec_alloc_npkt(ext, MX35_MTU_SIZE)) == NULL) {
			return (0);
			}
		}

	iov = TAILQ_FIRST(&npkt->buffers)->net_iov;
	ap = iov->iov_base;

	memset (ap, 0x00, sizeof *ap);
	ap->type = _IO_NET_MSG_DL_ADVERT;
	ap->iflags = (IFF_SIMPLEX | IFF_BROADCAST | IFF_RUNNING);
	if (cfg->flags & NIC_FLAG_MULTICAST) {
		ap->iflags |= IFF_MULTICAST;
		} 

	ap->mtu_min = 0;
	ap->mtu_max = cfg->mtu;
	ap->mtu_preferred = cfg->mtu;
	strcpy (ap->up_type, "en");
	itoa (cfg->lan, ap->up_type + 2, 10);

	strcpy (ap->dl.sdl_data, ap->up_type);

	ap->dl.sdl_len = sizeof(struct sockaddr_dl);
	ap->dl.sdl_family = AF_LINK;
	ap->dl.sdl_index  = cfg->lan;
	ap->dl.sdl_type = IFT_ETHER;
	ap->dl.sdl_nlen = strlen(ap->dl.sdl_data); /* not null terminated */
	ap->dl.sdl_alen = 6;
	memcpy (ap->dl.sdl_data + ap->dl.sdl_nlen, cfg->current_address, 6);

	npkt->flags |= _NPKT_MSG;
	npkt->iface = 0;
	npkt->framelen = iov->iov_len = sizeof *ap;
	npkt->tot_iov = 1;
	
	if (ion_add_done (ext->reg_hdl, npkt, ext) == -1) {
		mx35_fec_receive_complete(npkt, ext, ext);
		return (0);
		}

	if(ion_rx_packets (ext->reg_hdl, npkt, 0, 0, ext->cell, cfg->lan, 0) == 0) {
		ion_tx_complete (ext->reg_hdl, npkt);
		}

	return (0);
}

/****************************************************************************/
/* bump_macaddr: increment multicast address by one                         */
/****************************************************************************/

void	bump_macaddr(uint8_t *mcaddr)

{
int i;

	for (i = 5; i >= 0; i--) {
		if (mcaddr[i] + 1 < 0x100) {
			mcaddr[i]++;
			while (i++ < 6)
				mcaddr[i] = 0;
			return;
			}
		}
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

void	setup_mcast_range (mx35_t *ext, uint8_t *da, uint64_t len, int on)

{
unsigned 	crc;
unsigned	bitIndex;
unsigned	regIndex;
volatile	uint32_t	*gaddr_reg = ext->reg;

	while (len--) {

		crc = nic_calc_crc_be (da, ETHER_ADDR_LEN);
		bitIndex = (crc & 0x7e000000) >> 26;    // MSB 5 bits for bit index
		regIndex = (crc & 0x80000000);      // bit 31 for register index
		gaddr_reg += (regIndex) ? MX35_GADDR1 : MX35_GADDR2;

		if (ext->cfg.verbose) {
			fprintf(stderr,"setup_mcast_range(): mac addr: %X %X %X %X %X %X\n",
				da[0], da[1], da[2], da[3], da[4], da[5]);
			fprintf(stderr,"setup_mcast_range(): crc:      %X\n",crc);
			fprintf(stderr,"setup_mcast_range(): regIndex: %d\n",regIndex);
			fprintf(stderr,"setup_mcast_range(): bitIndex: %d\n",bitIndex);
			fprintf(stderr,"setup_mcast_range(): on:       %d\n",on);
		}

		if (on) {  // set bit in indicated register
			ext->gaddr [regIndex] |= (0x80000000 >> bitIndex);
			}
		else {   // drop bit in indicated register
			ext->gaddr [regIndex] &= ~(0x80000000 >> bitIndex);
			}

		// if we are not in multicast promiscusous mode, write
		// changed hash mask to ENET.  When we come out of
		// multicast promiscuous mode, our in-memory copy of
		// the hash mask will be rewritten to ENET by mcast_prom()

		if (!ext->mcast_nprom) {
			*(gaddr_reg) = ext->gaddr [regIndex];
			}

		bump_macaddr(da);
		}

}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

void	mcast_prom (mx35_t *ext, int enable)

{
volatile	uint32_t	*gaddr_reg = ext->reg + MX35_GADDR1;

	if (enable) {
		if (++ext->mcast_nprom != 1) {
			return;  // stacked request - already done
			}
        // first enable request - enable all in ENET
		*(gaddr_reg++) = 0xffffffff;
		*(gaddr_reg) = 0xffffffff;
		}
	else {  // disable (previous) request
        if (ext->mcast_nprom <= 0) {
			return;  // no underflow allowed during regression testing
    	    }
        // decrement positive valued counter
		if (--ext->mcast_nprom) {
			return;  // still outstanding stacked enable all requests
			}

		// coming out of multicast promiscuous mode - refresh ENET
		*(gaddr_reg++) = ext->gaddr [1];
		*(gaddr_reg) = ext->gaddr [2];
		}
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

int		do_multicast (mx35_t *ext, struct _io_net_msg_mcast *mcast)

{
uint64_t			start_val;
uint64_t		   	end_val;
uint64_t		   	range_length;
uint64_t		   	start, end;

	if (ext->cfg.verbose) {
		nic_slogf (_SLOGC_NETWORK, _SLOG_INFO, 
		"multicast msg %p - type %x - flags %x", mcast, mcast->type, mcast->flags);
		}

	if (mcast->type == _IO_NET_REMOVE_MCAST) {
		if ((mcast->flags & _IO_NET_MCAST_ALL) == 0) {
			memcpy ((char *) &start + 2, LLADDR(&mcast->mc_min.addr_dl), ETHER_ADDR_LEN);
			memcpy ((char *) &end + 2, LLADDR(&mcast->mc_max.addr_dl), ETHER_ADDR_LEN);

            start_val = ENDIAN_BE64 (start);
            end_val   = ENDIAN_BE64 (end);

            if (start_val > end_val) {
				if (ext->cfg.verbose) {
				    nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
			        "devn-mx35: _IO_NET_REMOVE_MCAST: start_val > end_val",
			        mcast->type);
					}
                return EINVAL;
	            }

            range_length = end_val - start_val + 1;

            if (range_length > 256) {
				mcast_prom (ext, 0);  // was never added to filter
				}
			else {
				setup_mcast_range (ext, (uint8_t *) &start_val, range_length, 0);
				}
			}
		else {
			mcast_prom (ext, 0);  // remove all multicast
			}
		}
	else if (mcast->type ==  _IO_NET_JOIN_MCAST) {
			if ((mcast->flags & _IO_NET_MCAST_ALL) == 0) {
				if (nic_ether_mcast_valid (mcast) == -1) {
					return EINVAL;
					}

				memcpy ((char *) &start + 2, LLADDR(&mcast->mc_min.addr_dl), ETHER_ADDR_LEN);
				memcpy ((char *) &end + 2, LLADDR(&mcast->mc_max.addr_dl), ETHER_ADDR_LEN);

	            start_val = ENDIAN_BE64 (start);
	            end_val   = ENDIAN_BE64 (end);

	            if (start_val > end_val) {
					if (ext->cfg.verbose) {
					    nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
				        "devn-mx35: _IO_NET_JOIN_MCAST: start_val > end_val",
				        mcast->type);
						}
	                return EINVAL;
		            }

	            range_length = end_val - start_val + 1;

	            if (range_length > 256) {
					mcast_prom (ext, 1);  // dont add to filter
					}
				else {
					setup_mcast_range (ext, (uint8_t *) &start_val, range_length, 1);
					}
				}
			else {
				mcast_prom (ext, 1);  // join all multicast
				}
			}
		else {
			if (ext->cfg.verbose) {
			    nic_slogf(_SLOGC_NETWORK, _SLOG_ERROR,
			        "devn-mx35: message 0x%x not supported",
			        mcast->type);
				}
			return EINVAL;
			}

	return (EOK);
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/

void	mx35_fec_update_stats (mx35_t *ext)

{
volatile	uint32_t				*base = ext->reg;
nic_ethernet_stats_t	*estats = &ext->stats.un.estats;

	ext->stats.octets_txed_ok = *(base + MX35_RMON_T_OCTETS);
	ext->stats.txed_ok = *(base + MX35_RMON_T_PACKETS);
	ext->stats.txed_multicast = *(base + MX35_RMON_T_MC_PKT);
	ext->stats.txed_broadcast = *(base + MX35_RMON_T_BC_PKT);
	estats->excessive_deferrals = *(base + MX35_IEEE_T_DEF);
	estats->single_collisions = *(base + MX35_IEEE_T_1COL);
	estats->multi_collisions = *(base + MX35_IEEE_T_MCOL);
	estats->late_collisions = *(base + MX35_IEEE_T_LCOL);

	ext->stats.octets_rxed_ok = *(base + MX35_RMON_R_OCTETS);
	ext->stats.rxed_ok = *(base + MX35_RMON_R_PACKETS);
	estats->fcs_errors = *(base + MX35_RMON_R_CRC_ALIGN);
	ext->stats.rxed_multicast = *(base + MX35_RMON_R_MC_PKT);
	ext->stats.rxed_broadcast = *(base + MX35_RMON_R_BC_PKT);
	estats->internal_rx_errors = *(base + MX35_RMON_R_DROP);
}
