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


#include <atomic.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <sys/mman.h>
#include <sys/mbuf.h>
#include <sys/types.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <sys/io-net.h>
#include <sys/dcmd_io-net.h>
#include <sys/slogcodes.h>
#include <sys/cache.h>
#include <sys/syspage.h>
#include <hw/inout.h>
#include <hw/sysinfo.h>

#include <drvr/mdi.h>
#include <drvr/eth.h>
#include <drvr/nicsupport.h>

#ifndef	MX35_H_INCLUDED
#define	MX35_H_INCLUDED

#define NIC_PRIORITY 			21

#define MX35_KEEP_PACKET			0x100000
#define MX35_DEFRAG_PACKET		0x1000000
#define	DEFRAG_LIMIT			8

#define MX35_MTU_SIZE			1536

#define DEFAULT_NUM_RX_DESCRIPTORS  64
#define DEFAULT_NUM_TX_DESCRIPTORS  256

#define MIN_NUM_RX_DESCRIPTORS  16
#define MIN_NUM_TX_DESCRIPTORS  16
#define MAX_NUM_RX_DESCRIPTORS  2048
#define MAX_NUM_TX_DESCRIPTORS  2048

#define	MX35_TIMEOUT				1000
#define	NIC_INTR_EVENT			1

#define MX35_IOBASE				0x50038000
#define	MX35_INTR				57

#define MX35_CACHE_LINE_SIZE	32

/* Control/Status Registers */

#define	MX35_FEC_ID				(0x0000)
#define	MX35_IEVENT				(0x0004 >> 2)
	#define	IEVENT_HBERR		(1 << 31)
	#define	IEVENT_BABR			(1 << 30)
	#define	IEVENT_BABT			(1 << 29)
	#define	IEVENT_GRA			(1 << 28)
	#define	IEVENT_TFINT		(1 << 27)
	#define	IEVENT_TXB			(1 << 26)
	#define	IEVENT_RFINT		(1 << 25)
	#define	IEVENT_RXB			(1 << 24)
	#define	IEVENT_MII			(1 << 23)
	#define	IEVENT_EBERR		(1 << 22)
	#define	IEVENT_LATE_COL		(1 << 21)
	#define	IEVENT_COL_RET_LIM	(1 << 20)
	#define	IEVENT_XFIFO_UN		(1 << 19)
	#define	IEVENT_XFIFO_ERR	(1 << 18)
	#define	IEVENT_RFIFO_ERR	(1 << 17)

#define	MX35_IMASK				(0x0008 >> 2)	/* Interrupt Mask Register */
	#define	IMASK_HBEEN			(1 << 31)
	#define	IMASK_BREN			(1 << 30)
	#define	IMASK_BTEN			(1 << 29)
	#define	IMASK_GRAEN			(1 << 28)
	#define	IMASK_TFIEN			(1 << 27)
	#define	IMASK_TBIEN			(1 << 26)
	#define	IMASK_RFIEN			(1 << 25)
	#define	IMASK_RBIEN			(1 << 24)
	#define	IMASK_MIIEN			(1 << 23)
	#define	IMASK_EBERREN		(1 << 22)
	#define	IMASK_LCEN			(1 << 21)
	#define	IMASK_CRLEN			(1 << 20)
	#define	IMASK_XFUNEN		(1 << 19)
	#define	IMASK_XFERREN		(1 << 18)
	#define	IMASK_RFERREN		(1 << 17)

#define	MX35_R_DES_ACTIVE		(0x0010 >> 2)
	#define	R_DES_ACTIVE		(1 << 24)

#define	MX35_X_DES_ACTIVE		(0x0014 >> 2)
	#define	X_DES_ACTIVE		(1 << 24)

#define	MX35_ECNTRL				(0x0024 >> 2)
	#define	ECNTRL_FEC_OE		(1 << 2)
	#define	ECNTRL_ETHER_EN		(1 << 1)
	#define	ECNTRL_RESET		(1 << 0)

#define	MX35_MII_DATA			(0x0040 >> 2)
#define	MX35_MII_SPEED			(0x0044 >> 2)
	#define	DIS_PREAMBLE		(1 << 7)

#define	MX35_MIB_CONTROL			(0x0064 >> 2)
	#define	MIB_DISABLE			(1 << 31)
	#define	MIB_IDLE			(1 << 30)

#define	MX35_R_CNTRL				(0x0084 >> 2)
	#define	RCNTRL_FCE			(1 << 5)
	#define	RCNTRL_BC_REJ		(1 << 4)
	#define	RCNTRL_PROM			(1 << 3)
	#define	RCNTRL_MII_MODE		(1 << 2)
	#define	RCNTRL_DRT			(1 << 1)
	#define	RCNTRL_LOOP			(1 << 0)

#define	MX35_R_HASH				(0x0088 >> 2)
	#define	RHASH_MULTCAST		(1 << 30)

#define	MX35_X_CNTRL				(0x00c4 >> 2)
	#define	XCNTRL_TFC_PAUSE	(1 << 3)
	#define	XCNTRL_FDEN			(1 << 2)
	#define	XCNTRL_HBC			(1 << 1)
	#define	XCNTRL_GTS			(1 << 0)

#define	MX35_PADDR1				(0x00e4 >> 2)
#define	MX35_PADDR2				(0x00e8 >> 2)
#define	MX35_OP_PAUSE			(0x00ec >> 2)
#define	MX35_IADDR1				(0x0118 >> 2)
#define	MX35_IADDR2				(0x011c >> 2)
#define	MX35_GADDR1				(0x0120 >> 2)
#define	MX35_GADDR2				(0x0124 >> 2)
#define	MX35_FIFO_ID				(0x0140 >> 2)
#define	MX35_X_WMRK				(0x0144 >> 2)
#define	MX35_R_BOUND				(0x014c >> 2)
#define	MX35_R_FSTART			(0x0150 >> 2)
#define	MX35_R_DES_START			(0x0180 >> 2)
#define	MX35_X_DES_START			(0x0184 >> 2)
#define	MX35_R_BUFF_SIZE			(0x0188 >> 2)
#define	MX35_DMA_CONTROL			(0x01f4 >> 2)
	#define	DMA_DATA_BO			(1 << 31)
	#define	DMA_DESC_BO			(1 << 30)

/* MIB Block Counters */

#define	MX35_RMON_T_DROP			(0x0200 >> 2)
#define	MX35_RMON_T_PACKETS		(0x0204 >> 2)
#define	MX35_RMON_T_BC_PKT		(0x0208 >> 2)
#define	MX35_RMON_T_MC_PKT		(0x020c >> 2)
#define	MX35_RMON_T_CRC_ALIGN	(0x0210 >> 2)
#define	MX35_RMON_T_UNDERSIZE	(0x0214 >> 2)
#define	MX35_RMON_T_OVERSIZE		(0x0218 >> 2)
#define	MX35_RMON_T_FRAG			(0x021c >> 2)
#define	MX35_RMON_T_JAB			(0x0220 >> 2)
#define	MX35_RMON_T_COL			(0x0224 >> 2)
#define	MX35_RMON_T_P64			(0x0228 >> 2)
#define	MX35_RMON_T_P65TO127		(0x022c >> 2)
#define	MX35_RMON_T_P128TO255	(0x0230 >> 2)
#define	MX35_RMON_T_P256TO511	(0x0234 >> 2)
#define	MX35_RMON_T_P512TO1023	(0x0238 >> 2)
#define	MX35_RMON_T_P1024TO2047	(0x023c >> 2)
#define	MX35_RMON_T_P_GTE2048	(0x0240 >> 2)
#define	MX35_RMON_T_OCTETS		(0x0244 >> 2)
#define	MX35_IEEE_T_DROP			(0x0248 >> 2)
#define	MX35_IEEE_T_FRAME_OK		(0x024c >> 2)
#define	MX35_IEEE_T_1COL			(0x0250 >> 2)
#define	MX35_IEEE_T_MCOL			(0x0254 >> 2)
#define	MX35_IEEE_T_DEF			(0x0258 >> 2)
#define	MX35_IEEE_T_LCOL			(0x025c >> 2)
#define	MX35_IEEE_T_EXCOL		(0x0260 >> 2)
#define	MX35_IEEE_T_MACERR		(0x0264 >> 2)
#define	MX35_IEEE_T_CSERR		(0x0268 >> 2)
#define	MX35_IEEE_T_SQE			(0x026c >> 2)
#define	MX35_T_FDXFC				(0x0270 >> 2)
#define	MX35_IEEE_T_OCTETS_OK	(0x0274 >> 2)
#define	MX35_RMON_R_DROP			(0x0280 >> 2)
#define	MX35_RMON_R_PACKETS		(0x0284 >> 2)
#define	MX35_RMON_R_BC_PKT		(0x0288 >> 2)
#define	MX35_RMON_R_MC_PKT		(0x028c >> 2)
#define	MX35_RMON_R_CRC_ALIGN	(0x0290 >> 2)
#define	MX35_RMON_R_UNDERSIZE	(0x0294 >> 2)
#define	MX35_RMON_R_OVERSIZE		(0x0298 >> 2)
#define	MX35_RMON_R_FRAG			(0x029c >> 2)
#define	MX35_RMON_R_JAB			(0x02a0 >> 2)
#define	MX35_RMON_R_P64			(0x02a8 >> 2)
#define	MX35_RMON_R_P65TO127		(0x02ac >> 2)
#define	MX35_RMON_R_P128TO255	(0x02b0 >> 2)
#define	MX35_RMON_R_P256TO511	(0x02b4 >> 2)
#define	MX35_RMON_R_P512TO1023	(0x02b8 >> 2)
#define	MX35_RMON_R_P1024TO2047	(0x02bc >> 2)
#define	MX35_RMON_R_P_GTE2048	(0x02c0 >> 2)
#define	MX35_RMON_R_OCTETS		(0x02c4 >> 2)
#define	MX35_IEEE_R_DROP			(0x02c8 >> 2)
#define	MX35_IEEE_R_FRAME_OK		(0x02cc >> 2)
#define	MX35_IEEE_R_CRC			(0x02d0 >> 2)
#define	MX35_IEEE_R_ALIGN		(0x02d4 >> 2)
#define	MX35_IEEE_R_MACERR		(0x02d8 >> 2)
#define	MX35_R_FDXFC				(0x02dc >> 2)
#define	MX35_IEEE_OCTETS_OK		(0x02e0 >> 2)

/* FIFO RAM */

#define	MX35_TX_FIFO				(0x0400 >> 2)

/* Transmit/receive buffer descriptor */

typedef	struct {
	uint16_t	length;
	uint16_t	status;
	uint32_t	buffer;
} mx35_fec_bd_t;

#define	TXBD_R				(1 << 15)		/* Ready */
#define	TXBD_TO1			(1 << 14)		/* Transmit Ownership */
#define	TXBD_W				(1 << 13)		/* Wrap */
#define	TXBD_TO2			(1 << 12)		/* Transmit Ownership */
#define	TXBD_L				(1 << 11)		/* Last */
#define	TXBD_TC				(1 << 10)		/* Tx CRC */
#define	TXBD_ABC			(1 << 9)		/* Append bad CRC */

#define	RXBD_E				(1 << 15)		/* Empty */
#define	RXBD_RO1			(1 << 14)		/* Receive software ownership bit */
#define	RXBD_W				(1 << 13)		/* Wrap */
#define	RXBD_RO2			(1 << 12)		/* Receive Ownership */
#define	RXBD_L				(1 << 11)		/* Last in frame */
#define	RXBD_M				(1 << 8)		/* Miss */
#define	RXBD_BC				(1 << 7)		/* Broadcast */
#define	RXBD_MC				(1 << 6)		/* Multicast */
#define	RXBD_LG				(1 << 5)		/* Rx frame length violation */
#define	RXBD_NO				(1 << 4)		/* Rx non-octet aligned frame */
#define	RXBD_SH				(1 << 3)		/* Short frame */
#define	RXBD_CR				(1 << 2)		/* Rx CRC error */
#define	RXBD_OV				(1 << 1)		/* Overrun */
#define	RXBD_TR				(1 << 0)		/* Truncation */
#define	RXBD_ERR			(RXBD_TR | RXBD_OV | RXBD_CR | RXBD_SH | RXBD_NO | RXBD_LG)

#define	NEXT_TX(x)		((x + 1) % ext->num_tx_descriptors)
#define	NEXT_RX(x)		((x + 1) % ext->num_rx_descriptors)
#define	PREV_TX(x)		((x == 0) ? ext->num_tx_descriptors - 1 : x - 1)

typedef	struct {
	nic_config_t	cfg;
	nic_stats_t		stats;
	int				chid;
	int 			coid;
	int 			tid;
	int 			iid;
	int				version;
	uintptr_t		iobase;
	volatile uint32_t		*reg;
	mdi_t			*mdi;
	int				force_advertise;
	int				phy_addr;
	int				phy_mode;

	io_net_self_t	*ion;
	struct cache_ctrl	cachectl;
	void			*dll_hdl;
	int				reg_hdl;
	uint16_t		cell;
	npkt_t			**tx_pktq;
	mx35_fec_bd_t		*tx_bd;
	uint32_t		tx_status;
	int				num_tx_descriptors;
	int				tx_pidx;
	int 			tx_cidx;
	pthread_mutex_t	tx_mutex;
	int				tx_pkts_queued;
	int				tx_q_len;
	int			 	num_rx_descriptors;
	int				num_rx_free;
	int				rx_cidx;
	npkt_t			**rx_pktq;
	npkt_t			*rx_free_pkt_q;
	mx35_fec_bd_t		*rx_bd;
	pthread_mutex_t	rx_free_pkt_q_mutex;
	uint32_t		gaddr [2];
	int				mcast_nprom;
} mx35_t;

/*
 * standard io-net macros
 */
#define ion_rx_packets          ext->ion->tx_up
#define ion_register            ext->ion->reg
#define ion_deregister          ext->ion->dereg
#define ion_alloc               ext->ion->alloc
#define ion_free                ext->ion->free
#define ion_alloc_npkt          ext->ion->alloc_up_npkt
#define ion_mphys               ext->ion->mphys
#define ion_tx_complete         ext->ion->tx_done
#define ion_add_done            ext->ion->reg_tx_done

#ifndef TAILQ_EMPTY
#define TAILQ_EMPTY(head) ((head)->tqh_first == NULL)
#endif
#ifndef TAILQ_FIRST
#define TAILQ_FIRST(head) ((head)->tqh_first)
#endif
#ifndef TAILQ_LAST
#define TAILQ_LAST(head) ((head)->tqh_last)
#endif
#ifndef TAILQ_NEXT
#define TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)
#endif
#ifndef TAILQ_PREV
#define TAILQ_PREV(elm, field) ((elm)->field.tqe_prev)
#endif

/*==============================================================================
 *
 * function prototypes by file 
 */

/* shutdown.c */
int mx35_fec_shutdown1 (int, void *);
int mx35_fec_shutdown2 (int, void *);

/* devctl.c */
int mx35_fec_devctl (void *hdl, int dcmd, void *data, size_t size, union _io_net_dcmd_ret_cred *ret);

/* receive.c */
int mx35_fec_receive_complete (npkt_t *npkt, void *, void *);
void mx35_fec_receive (mx35_t *ext);

/* transmit.c */
int mx35_fec_send_packets (npkt_t *npkt, void *);
int mx35_fec_download_complete (mx35_t *ext);
int mx35_fec_send (mx35_t *ext, npkt_t *npkt);
int mx35_fec_reap_pkts (mx35_t *ext, int max_reap);
int mx35_fec_flush (int, void *);


/* mx35.c */
int mx35_fec_init (void *, dispatch_t *, io_net_self_t *, char *);
int mx35_fec_advertise (int reg_hdl, void *func_hdl);
npkt_t *mx35_fec_alloc_npkt (mx35_t *ext, size_t size);
void mx35_fec_reset (mx35_t *ext);
int mx35_fec_register_device (mx35_t *ext, io_net_self_t *, void *);
int mx35_fec_detect (void *, io_net_self_t *, char *options);
int	mx35_fec_init_phy (mx35_t *ext);
int mx35_fec_set_duplex (mx35_t *ext, int on);
int	do_multicast (mx35_t *ext, io_net_msg_join_mcast_t *msg);
void mx35_fec_update_stats (mx35_t *ext);

#endif
