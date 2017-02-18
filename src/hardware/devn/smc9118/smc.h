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







#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/slogcodes.h>
#include <sys/io-net.h>
#include <sys/mman.h>
#include <sys/dcmd_io-net.h>
#include <drvr/mdi.h>
#include <drvr/nicsupport.h>
#include <drvr/eth.h>
#include <hw/inout.h>
#include <net/if_types.h>
#include <net/if.h>

#define SMC9118_NUM_RX_BUFFERS	32
#define SMC9118_MTU_SIZE		1518

/*
 * chipset definitions
 */
#define SMSC_ID_9118			0x0118
#define SMSC_ID_9218I			0x118A

#define SMC9118_DIR_IN			0
#define SMC9118_DIR_OUT			1

#define SMC9118_MMAP_SIZE		0x100

#define SMC9118_ID_REV			0x50
	#define CHIP_ID				0xFFFF
	#define CHIP_REV			0xFFFF

#define SMC9118_IRQ_CFG			0x54
	#define INT_DEAS			(0x22 << 24)
	#define INT_DEAS_CLR		(1 << 14)
	#define INT_DEAS_STS		(1 << 13)
	#define IRQ_INT				(1 << 12)
	#define IRQ_EN				(1 << 8)
	#define IRQ_POL				(1 << 4)
	#define IRQ_TYPE			(1 << 0)

#define SMC9118_INT_STS			0x58
	#define SW_INT				(1 << 31)
	#define TXSTOP_INT			(1 << 25)
	#define RXSTOP_INT			(1 << 24)
	#define RXDFH_INT			(1 << 23)
	#define TX_IOC				(1 << 21)
	#define RXD_INT				(1 << 20)
	#define GPT_INT				(1 << 19)
	#define PHY_INT				(1 << 18)
	#define PME_INT				(1 << 17)
	#define TXSO				(1 << 16)
	#define RWT					(1 << 15)
	#define RXE					(1 << 14)
	#define TXE					(1 << 13)
	#define TDFU				(1 << 11)
	#define TDFO				(1 << 10)
	#define TDFA				(1 << 9)
	#define TSFF				(1 << 8)
	#define TSFL				(1 << 7)
	#define RXDF_INT			(1 << 6)
	#define RDFL				(1 << 5)
	#define RSFF				(1 << 4)
	#define RSFL				(1 << 3)

#define SMC9118_INT_EN			0x5C
	#define SW_INT_EN			(1 << 31)
	#define TXSTOP_INT_EN		(1 << 25)
	#define RXSTOP_INT_EN		(1 << 24)
	#define RXDFH_INT_EN		(1 << 23)
	#define TX_IOC_EN			(1 << 21)
	#define RXD_INT_EN			(1 << 20)
	#define GPT_INT_EN			(1 << 19)
	#define PHY_INT_EN			(1 << 18)
	#define PME_INT_EN			(1 << 17)
	#define TXSO_EN				(1 << 16)
	#define RWT_EN				(1 << 15)
	#define RXE_EN				(1 << 14)
	#define TXE_EN				(1 << 13)
	#define TDFU_EN				(1 << 11)
	#define TDFO_EN				(1 << 10)
	#define TDFA_EN				(1 << 9)
	#define TSFF_EN				(1 << 8)
	#define TSFL_EN				(1 << 7)
	#define RXDF_INT_EN			(1 << 6)
	#define RDFL_EN				(1 << 5)
	#define RSFF_EN				(1 << 4)
	#define RSFL_EN				(1 << 3)

#define SMC9118_BYTE_TEST		0x64

#define SMC9118_FIFO_INT		0x68
	#define TX_DATA_AVAIL		0xFF
	#define RX_DATA_FREE		8

#define SMC9118_RX_CFG			0x6C
	#define RXDOFF				(0x2 << 8)
	#define RX_DUMP				(1 << 15)

#define SMC9118_TX_CFG			0x70
	#define TXS_DUMP			(1 << 15)
	#define TXD_DUMP			(1 << 14)
	#define TXSAO				(1 << 2)
	#define TX_ON				(1 << 1)
	#define STOP_TX				(1 << 0)
	
#define SMC9118_HW_CFG			0x74
	#define TTM					(1 << 21)
	#define SF					(1 << 20)
	#define BIT32_16			(1 << 2)
	#define SRST_TO				(1 << 1)
	#define SRST				(1 << 0)
	#define TX_FIF_SZ_1K		0x2
	#define TX_FIF_SZ_2K		0x3
	#define TX_FIF_SZ_3K		0x4
	#define TX_FIF_SZ_4K		0x5
	#define TX_FIF_SZ_5K		0x6
	#define TX_FIF_SZ_6K		0x7
	#define TX_FIF_SZ_7K		0x8
	#define TX_FIF_SZ_8K		0x9
	#define TX_FIF_SZ_9K		0xA
	#define TX_FIF_SZ_10K		0xB
	#define TX_FIF_SZ_11K		0xC
	#define TX_FIF_SZ_12K		0xD
	#define TX_FIF_SZ_13K		0xE

#define SMC9118_RX_DP_CTL		0x78
	#define RX_FFWD				(1 << 31)

#define SMC9118_RX_FIFO_INF		0x7C
	#define RXUSED				0xFFFF
	
#define SMC9118_TX_FIFO_INF		0x80
	#define TDFREE				0xFFFF

#define SMC9118_PMT_CTRL		0x84
	#define PHY_RST				(1 << 10)
	#define WOL_EN				(1 << 9)
	#define ED_EN				(1 << 8)
	#define PME_TYPE			(1 << 6)
	#define PME_IND				(1 << 3)
	#define PME_POL				(1 << 2)
	#define PME_EN				(1 << 1)
	#define READY				(1 << 0)

#define SMC9118_GPIO_CFG		0x88
	#define GPIO_CFG_LED_OUT	0x70070000

#define SMC9118_GPT_CFG			0x8c
	#define TIMER_EN			(1 << 29)

#define SMC9118_GPT_CNT			0x90

#define SMC9118_ENDIAN			0x98

#define SMC9118_FREE_RUN		0x9C

#define SMC9118_RX_DROP			0xA0

#define SMC9118_MAC_CSR_CMD		0xA4
	#define CSR_BUSY			(1 << 31)
	#define CSR_READ			(1 << 30)

#define SMC9118_MAC_CSR_DATA	0xA8

#define SMC9118_AFC_CFG			0xAC
	#define AFC_HI				(0x6E << 16)
	#define AFC_LO				(0x37 << 8)
	#define BACK_DUR			(0x4 << 4)
	#define FCMULT				(1 << 3)
	#define FCBRD				(1 << 2)
	#define FCADD				(1 << 1)
	#define FC_ANY				(1 << 0)

#define SMC9118_E2P_CMD			0xB0
	#define EPC_BUSY			(1 << 31)
	#define EPC_READ			0x0
	#define EPC_WRITE			0x3
	#define EPC_RELOAD			0x7
	#define EPC_TIMEOUT			(1 << 9)
	#define MAC_ADDR_LOADED		(1 << 8)

#define SMC9118_E2P_DATA		0xB4

/*==============================================================================
 *
 * MAC Control Register Definitions
 *
 *============================================================================*/
#define SMC9118_MAC_CR			0x1
	#define RXALL				(1 << 31)
	#define RCVOWN				(1 << 23)
	#define LOOPBK				(1 << 21)
	#define FDPX				(1 << 20)
	#define MCPAS				(1 << 19)
	#define PRMS				(1 << 18)
	#define INVFILT				(1 << 17)
	#define PASSBAD				(1 << 16)
	#define HO					(1 << 15)
	#define HPFILT				(1 << 13)
	#define LCOLL				(1 << 12)
	#define BCAST				(1 << 11)
	#define DISRTY				(1 << 10)
	#define PADSTR				(1 << 8)
	#define DFCHK				(1 << 5)
	#define TXEN				(1 << 3)
	#define RXEN				(1 << 2)

#define SMC9118_ADDRH			0x2

#define SMC9118_ADDRL			0x3

#define SMC9118_HASHH			0x4

#define SMC9118_HASHL			0x5

#define SMC9118_MII_ACC			0x6
	#define MIIWNR				(1 << 1)
	#define MIIBZY				(1 << 0)

#define SMC9118_MII_DATA		0x7

#define SMC9118_FLOW			0x8
	#define FCPASS				(1 << 2)
	#define FCEN				(1 << 1)
	#define FCBSY				(1 << 0)

#define SMC9118_VLAN1			0x9

#define SMC9118_VLAN2			0xA

#define SMC9118_WUFF			0xB

#define SMC9118_WUCSR			0xC
	#define GUE					(1 << 9)
	#define WUFR				(1 << 6)
	#define MPR					(1 << 5)
	#define WUEN				(1 << 2)
	#define MPEN				(1 << 1)

/*==============================================================================
 *
 * PHY Control Register Definitions
 *
 *============================================================================*/

#define PHY_BASCR				0x0
	#define RESET				(1 << 15)
	#define PHY_LOOPBK			(1 << 14)
	#define SPEED_SEL			(1 << 13)
	#define AN_EN				(1 << 12)
	#define POWER_DN			(1 << 11)
	#define AN_RESTART			(1 << 9)
	#define DUPLEX_MD			(1 << 8)
	#define COLL_TEST			(1 << 7)

#define PHY_BASSR				0x1
	#define BASET4100			(1 << 15)
	#define BASETX100FD			(1 << 14)
	#define BASETX100HD			(1 << 13)
	#define BASET10FD			(1 << 12)
	#define BASET10HD			(1 << 11)
	#define AN_COMPLETE			(1 << 5)
	#define REMOTE_FLT			(1 << 4)
	#define AN_ABILITY			(1 << 3)
	#define LINK_STS			(1 << 2)
	#define JABBER_DET			(1 << 1)
	#define EXT_CAP				(1 << 0)

#define PHY_ID1					0x2

#define PHY_ID2					0x3

#define PHY_ANAR				0x4
	#define AN_NEXT_PAGE		(1 << 15)
	#define AN_REMOTE_FLT		(1 << 13)
	#define AN_BASET4100		(1 << 9)
	#define AN_BASETX100FD		(1 << 8)
	#define AN_BASETX100		(1 << 7)
	#define AN_BASET10FD		(1 << 6)
	#define AN_BASET10			(1 << 5)

#define PHY_ANLPAR				0x5
	#define	LP_NEXT_PAGE		(1 << 15)
	#define LP_ACK				(1 << 14)
	#define LP_REMOTE_FLT		(1 << 13)
	#define LP_BASET4100		(1 << 9)
	#define LP_BASETX100FD		(1 << 8)
	#define LP_BASETX100		(1 << 7)
	#define LP_BASET10FD		(1 << 6)
	#define LP_BASET10			(1 << 5)

#define PHY_ANER				0x6
	#define PARA_DET_FLT		(1 << 4)
	#define LP_NP_ABLE			(1 << 3)
	#define NP_ABLE				(1 << 2)
	#define PAGE_RECV			(1 << 1)
	#define LP_AN_ABLE			(1 << 0)

#define PHY_MCSR				0x11
	#define EDPWRDOWN			(1 << 13)
	#define ENERGYON			(1 << 1)

#define PHY_SMR					0x12

#define PHY_SCSI				0x1B
	#define VCOOFF_LP			(1 << 10)
	#define XPOL				(1 << 4)

#define PHY_ISR					0x1D
	#define PHY_INT7			(1 << 7)
	#define PHY_INT6			(1 << 6)
	#define PHY_INT5			(1 << 5)
	#define PHY_INT4			(1 << 4)
	#define PHY_INT3			(1 << 3)
	#define PHY_INT2			(1 << 2)
	#define PHY_INT1			(1 << 1)

#define PHY_IMR					0x1E

#define PHY_SPCSR				0x1F
	#define AUTODONE			(1 << 12)

#define TX_FIFO_STS_PORT		0x48
#define TX_FIFO_DATA_PORT		0x20

/*==============================================================================
 *
 * TX Command / Status bit definitions
 *
 *============================================================================*/
#define TX_CMDA_IOC				(1 << 31)
#define TX_CMDA_BEA				24
#define TX_CMDA_FIRST_SEG		(1 << 13)
#define TX_CMDA_DATAST_OFF		16
#define TX_CMDA_LAST_SEG		(1 << 12)
#define TX_CMDA_BUFF_SIZE		0

#define TX_CMDB_PKT_TAG			16
#define TX_CMDB_CRC_DISABLE		(1 << 13)
#define TX_CMDB_FRM_PAD_DISABLE	(1 << 12)
#define TX_CMDB_PKT_LENGTH		0

#define TX_STS_PKT_TAG			16
#define TX_STS_ERR_STS			(1 << 15)
#define TX_STS_LOC				(1 << 11)
#define TX_STS_NOC				(1 << 10)
#define TX_STS_LATE_COLL		(1 << 9)
#define TX_STS_EXC_COLL			(1 << 8)
#define TX_STS_COLL_CNT			0x78
#define TX_STS_EXC_DEFERRAL		(1 << 2)
#define TX_STS_UNDERRUN			(1 << 1)
#define TX_STS_DEFERRED			(1 << 0)

#define RX_FIFO_STS_PORT		0x40
#define RX_FIFO_DATA_PORT		0x00

/*==============================================================================
 *
 * RX status bit definitions
 *
 *============================================================================*/
#define RX_STS_FILTER_FAIL		(1 << 30)
#define RX_STS_PKT_LEN			16
#define RX_STS_ERR_STS			(1 << 15)
#define RX_STS_BCAST_FRAME		(1 << 13)
#define RX_STS_LEN_ERR			(1 << 12)
#define RX_STS_RUNT_FRAME		(1 << 11)
#define RX_STS_MCAST_FRAME		(1 << 10)
#define RX_STS_FRAME_TOO_LONG	(1 << 7)
#define RX_STS_COLL_SEEN		(1 << 6)
#define RX_STS_FRAME_TYPE		(1 << 5)
#define RX_STS_WDG_TIMEOUT		(1 << 4)
#define RX_STS_MII_ERR			(1 << 3)
#define RX_STS_DRIBB_BIT		(1 << 2)
#define RX_STS_CRC_ERR			(1 << 1)	


typedef struct _smc9118 {
	nic_config_t		cfg;
	nic_stats_t			stats;
	int					chid;			/* channel id */
	int					coid;			/* connection id */
	int					tid;			/* thread id */
	int					iid;			/* interrupt id */
	io_net_self_t		*ion;
	void				*dll_hdl;
	int					reg_hdl;
	uint16_t			cell;
	pthread_mutex_t		mutex;
	mdi_t				*mdi;
	int					phy_addr;
	uintptr_t			iobase;
	unsigned			forced_media;
	int					negotiate;

	npkt_t				*nhead;
	npkt_t				*ntail;
	unsigned			tx_max_buffer;
	unsigned			tx_fifo_size;

	int					width;			/* Bus width */
	char				*pmmparent;
	pmd_attr_t          pmd;

	/* RX free queue management  */
	int					did_rx;
	npkt_t				*rx_free;
	int					num_rx_free;
	int					max_rx_buffers;
	pthread_mutex_t		rx_freeq_mutex;

	/* Address filtering */
	int					n_prom;
	int					n_allmulti;
} smc9118_t;

/*
 * function prototypes by file 
 */
/* shutdown.c */
int		smc9118_shutdown1(int, void *hdl);
int		smc9118_shutdown2(int, void *hdl);

/* transmit.c */
int		smc9118_send_packets(npkt_t *npkt, void *);
void	smc9118_send(smc9118_t *dev);

/* receive.c */
int		smc9118_receive(smc9118_t *dev);
int		smc9118_receive_complete(npkt_t *npkt, void *, void *);

/* devctl.c */
int		smc9118_devctl(void *hdl, int dcmd, void *data, size_t size, union _io_net_dcmd_ret_cred *ret);

/* detect.c */
int		smc9118_detect(void *, io_net_self_t *, char *options);

/* mii.c */
int		smc9118_init_phy(smc9118_t *dev);
uint32_t smc9118_mac_read( uintptr_t base, uint8_t reg_off );
uint32_t smc9118_mac_write( uintptr_t base, uint8_t reg_off, uint32_t data );

/* smc.c */
void	smc9118_halt(smc9118_t *dev);
void	smc9118_initialize(smc9118_t *dev);
void	smc9118_event_handler(smc9118_t *dev);

int		smc9118_init(void *, dispatch_t *dpp, io_net_self_t *, char *options);
int		smc9118_register_device(smc9118_t *dev, io_net_self_t *, void *);
int		smc9118_flush(int, void *hdl);
void	smc9118_load(smc9118_t *dev);
npkt_t*	smc9118_alloc_npkt(smc9118_t *dev, size_t size);
int		smc9118_advertise(int reg_hdl, void *func_hdl);



#if 0
#define IN8(port)       		in8(port)
#define OUT8(port, value)   	out8((port), (value))
#define INLE16(port)        	inle16(port)
#define OUTLE16(port, value)    outle16((port), (value))
#endif
#define OUTLE32(port, value)    outle32((port), (value))
#define INLE32(port)        	inle32(port)

#if 0
#define IN8S(addr, len, port)   in8s((addr), (len), (port))
#define OUT8S(addr, len, port)  out8s((addr), (len), (port))
#define IN16S(addr, len, port)  in16s((addr), (len), (port))
#define OUT16S(addr, len, port) out16s((addr), (len), (port))
#define IN32S(addr, len, port)  in32s((addr), (len), (port))
#define OUT32S(addr, len, port) out32s((addr), (len), (port))
#endif

#if defined(TAILQ_FIRST)
#undef TAILQ_FIRST
#endif
#define TAILQ_FIRST(head)((head)->tqh_first)

#if defined(TAILQ_NEXT)
#undef TAILQ_NEXT
#endif
#define TAILQ_NEXT(elm, field)((elm)->field.tqe_next)


__SRCVERSION("smc.h $Rev: 262087 $");
