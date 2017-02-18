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


/*
 * Routines to initialize the various hardware subsystems
 * on the i.MX35 3DS board. 
 */


#include "startup.h"
#include "mx35_iomux.h"
#include "i2c.h"
#include <arm/mx35.h>


/* Base Addr */
#define	MX35_USBOTG_BASE	0x53FF4000					/* OTG base */
#define USB_H2REGS_BASE		(MX35_USBOTG_BASE + 0x400)	/* Host2 base */
#define USB_OTHERREGS_BASE	(MX35_USBOTG_BASE + 0x600)	/* USBOH base */


/* OTG registers */
#define UOG_HWHOST			0x08			/* Host h/w params */
#define UOG_HWTXBUF			0x10			/* TX buffer h/w params */
#define UOG_HWRXBUF			0x14			/* RX buffer h/w params */
#define UOG_SBUSCFG         0x90	        /* System bus interface */
#define UOG_CAPLENGTH		0x100			/* Capability register length */
#define UOG_HCIVERSION		0x102			/* Host Interface version */
#define UOG_HCSPARAMS		0x104			/* Host control structural params */
#define UOG_USBCMD			0x140			/* USB command register */
#define UOG_PORTSC1		 	0x184			/* port status and control */

/* Host2 */
#define UH2_HCSPARAMS		0x504			/* Host control structural params */
#define UH2_USBCMD			0x540			/* USB command register */
#define UH2_ULPIVIEW		0x570			/* ULPI viewport */
#define UH2_PORTSC1			0x584			/* port status and control */

/* USBOH */
#define USBCTRL				0x600			/* USB Control register */
#define USB_OTG_MIRROR		0x604			/* USB OTG mirror register */
#define USB_PHY_CTR_FUNC    0x608	        /* OTG UTMI PHY Function Control register */


/* x_PORTSCx */
#define PORTSC_PTS_MASK		(3 << 30)		/* parallel xcvr select mask */
#define PORTSC_PTS_UTMI		(0 << 30)		/* UTMI/UTMI+ */
#define PORTSC_PTS_PHILIPS	(1 << 30)		/* Philips classic */
#define PORTSC_PTS_ULPI		(2 << 30)		/* ULPI */
#define PORTSC_PTS_SERIAL	(3 << 30)		/* serial */
#define PORTSC_STS			(1 << 29)		/* serial xcvr select */
#define PORTSC_PTW          (1 << 28)   	/* UTMI width */
#define PORTSC_PORT_POWER	(1 << 12)		/* port power */
#define PORTSC_LS_MASK		(3 << 10)		/* Line State mask */
#define PORTSC_LS_SE0		(0 << 10)		/* SE0     */

/* USBCTRL */
#define UCTRL_OWIR			(1 << 31)		/* OTG wakeup intr request received */
#define UCTRL_OSIC_MASK		(3 << 29)		/* OTG  Serial Interface Config: */
#define UCTRL_OSIC_DU6		(0 << 29)		/* Differential/unidirectional 6 wire */
#define UCTRL_OSIC_DB4		(1 << 29)		/* Differential/bidirectional  4 wire */
#define UCTRL_OSIC_SU6		(2 << 29)		/* single-ended/unidirectional 6 wire */
#define UCTRL_OSIC_SB3		(3 << 29)		/* single-ended/bidirectional  3 wire */
#define UCTRL_OUIE			(1 << 28)		/* OTG ULPI intr enable */
#define UCTRL_OWIE			(1 << 27)		/* OTG wakeup intr enable */
#define UCTRL_OBPVAL_RXDP	(1 << 26)		/* OTG RxDp status in bypass mode */
#define UCTRL_OBPVAL_RXDM	(1 << 25)		/* OTG RxDm status in bypass mode */
#define UCTRL_OPM			(1 << 24)		/* OTG power mask */
#define UCTRL_H2WIR			(1 << 23)		/* HOST2 wakeup intr request received */
#define UCTRL_H2SIC_MASK	(3 << 21)		/* HOST2 Serial Interface Config: */
#define UCTRL_H2SIC_DU6		(0 << 21)		/* Differential/unidirectional 6 wire */
#define UCTRL_H2SIC_DB4		(1 << 21)		/* Differential/bidirectional  4 wire */
#define UCTRL_H2SIC_SU6		(2 << 21)		/* single-ended/unidirectional 6 wire */
#define UCTRL_H2SIC_SB3		(3 << 21)		/* single-ended/bidirectional  3 wire */
#define UCTRL_H2UIE			(1 << 20)		/* HOST2 ULPI intr enable */
#define UCTRL_H2WIE			(1 << 19)		/* HOST2 wakeup intr enable */
#define UCTRL_H2PM			(1 << 16)		/* HOST2 power mask */
#define UCTRL_PP            (1 << 11)       /* The power polarity bit controls the polarity of the pwr output signal*/
#define UCTRL_XCSO          (1 << 10)       /* Xcvr Clock Select for OTG port */
#define UCTRL_XCSH2         (1 <<  9)       /* Xcvr Clock Select for Host port */
#define UCTRL_IP_PULIDP     (1 <<  8)       /* Ipp_Puimpel_Pullup_Dp */
#define UCTRL_IP_PUE_UP     (1 <<  7)       /* ipp_pue_pullup_dp */
#define UCTRL_IP_PUE_DOWN   (1 <<  6)       /* ipp_pue_pulldwn_dpdm */
#define UCTRL_H2DT			(1 <<  5)		/* HOST2 TLL disabled */
#define UCTRL_USBTE         (1 <<  4)       /* USBT Transceiver enable */
#define UCTRL_OCPOL         (1 <<  3)       /* OverCurrent Polarity */
#define UCTRL_OCE           (1 <<  2)       /* OverCurrent Enable */
#define UCTRL_H2OCS         (1 <<  1)       /* Host OverCurrent State */
#define UCTRL_OOCS          (1 <<  0)       /* OTG OverCurrent State */
 
/* USBCMD */
#define UCMD_RUN_STOP       (1 << 0)        /* controller run/stop */
#define UCMD_RESET			(1 << 1)		/* controller reset */

/* USB_PHY_CTRL_FUNC */
#define USB_UTMI_PHYCTRL_UTMI_ENABLE   0x01000000

/* ULPIVIEW register bits */
#define ULPIVW_WU			(1 << 31)		/* Wakeup */
#define ULPIVW_RUN			(1 << 30)		/* read/write run */
#define ULPIVW_WDATA_MASK	0xFF			/* write data field */
#define ULPIVW_WDATA_SHIFT	0
 
#define HCSPARAMS_PPC       (0x1<<4)        /* Port Power Control */

/* MC9S08DZ60 MPU register */
#define MC9S08DZ60_I2C_REG_MAX				0x2a
#define MC9S08DZ60_I2C_SLAVE_ADDR			0x69
#define MC9S08DZ60_I2C_GPIO_CTL_REG1		0x20
#define MC9S08DZ60_I2C_GPIO_CTL_REG2		0x21
#define MC9S08DZ60_I2C_GPIO_CTL_REG2_CAN    0x02


void mx35_3ds_init_i2c(void)
{
	/* I2C1 */
    pinmux_set_swmux(SWMUX_I2C1_CLK, MUX_CTL_SION);
    pinmux_set_swmux(SWMUX_I2C1_DAT, MUX_CTL_SION);

    pinmux_set_padcfg(SWPAD_I2C1_CLK, 0x1e8);
    pinmux_set_padcfg(SWPAD_I2C1_DAT, 0x1e8);

	/* AK4647 Audio port (SSI4) */
	pinmux_set_swmux(SWMUX_SCK4, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_SRXD4, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_STXD4, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_STXFS4, MUX_CTL_MUX_MODE_ALT0);

}

/*
 * Touch screen MC13892 
 */
void mx35_3ds_init_ts_mc13892(void) 
{
	/* MC13892 PENIRQ */
	pinmux_set_swmux(SWMUX_GPIO2_0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_GPIO2_0, PAD_CTL_GPIO_PKE_100K_PU);
	/* MX35_GPIO_ICR1 Interrupt 0-low-level, 1-high-level, 2-rising-edge, 3-falling-edge. */
	out32(MX35_GPIO2_BASE + MX35_GPIO_ICR1, in32(MX35_GPIO2_BASE + MX35_GPIO_ICR1) | (0x1 << 0));
	out32(MX35_GPIO2_BASE + MX35_GPIO_IMR, in32(MX35_GPIO2_BASE + MX35_GPIO_IMR) | (1 << 0));
}

/*
 * Touch screen TSC2007 
 */
void mx35_3ds_init_ts_tsc2007(void) 
{
	/* TSC2007 PENIRQ */
	pinmux_set_swmux(SWMUX_CAPTURE, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_CAPTURE, PAD_CTL_GPIO_PKE_100K_PU);
	out32(MX35_GPIO1_BASE + MX35_GPIO_GDIR,	    in32(MX35_GPIO1_BASE + MX35_GPIO_GDIR) & ~(1 << 4));
	out32(MX35_GPIO2_BASE + MX35_GPIO_IMR, in32(MX35_GPIO2_BASE + MX35_GPIO_IMR) | (1 << 0));
}

/*
 *  USB Host2
 */
void mx35_3ds_init_usbh2(void)
{
	uint32_t	tmp = 0;
	
	/* Setting as GPIO pin */
	pinmux_set_swmux(SWMUX_I2C2_CLK, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_I2C2_CLK, 0x0048);
	
	/* Set CAN_TX1 (GPIO2_26) pin direction as Output */
	out32(MX35_GPIO2_BASE + MX35_GPIO_GDIR, in32(MX35_GPIO2_BASE + MX35_GPIO_GDIR) | (1 << 26));
    
	pinmux_set_swmux(SWMUX_I2C2_DAT, MUX_CTL_MUX_MODE_ALT2);
	pinmux_set_input(SWINPUT_USB_UH2_USB_OC, 0x1);
	pinmux_set_padcfg(SWPAD_I2C2_DAT, 0x01c0);
        
    /* Stop then Reset */
    out32(MX35_USBOTG_BASE + UH2_USBCMD, (in32(MX35_USBOTG_BASE + UH2_USBCMD) & ~UCMD_RUN_STOP) );
 	while (in32(MX35_USBOTG_BASE + UH2_USBCMD) & UCMD_RUN_STOP);
    out32(MX35_USBOTG_BASE + UH2_USBCMD, (in32(MX35_USBOTG_BASE + UH2_USBCMD) | UCMD_RESET) );
 	while (in32(MX35_USBOTG_BASE + UH2_USBCMD) & UCMD_RESET);

	/* Disable bypass mode */
	out32(MX35_USBOTG_BASE + USBCTRL, (in32(MX35_USBOTG_BASE + USBCTRL) & ~UCTRL_H2SIC_MASK) );
	/* Power Mask */
	out32(MX35_USBOTG_BASE + USBCTRL, (in32(MX35_USBOTG_BASE + USBCTRL) & ~UCTRL_H2PM) );
	tmp |=  UCTRL_H2WIE |        	/* Wakeup intr enable */
			UCTRL_IP_PUE_DOWN |		/* ipp_pue_pulldwn_dpdm */
			UCTRL_USBTE |			/* USBT is enabled */
			UCTRL_H2DT;				/* Disable H2 TLL */
	out32(MX35_USBOTG_BASE + USBCTRL, tmp);
	out32(MX35_USBOTG_BASE + USBCTRL, (in32(MX35_USBOTG_BASE + USBCTRL) & ~UCTRL_PP) );
   
	out32(MX35_USBOTG_BASE + UH2_PORTSC1, ((in32(MX35_USBOTG_BASE + UH2_PORTSC1) & ~PORTSC_PTS_MASK) | PORTSC_PTS_SERIAL) );
}

/*
 *  USB OTG UTMI
 */
void mx35_3ds_init_usbotg_utmi(void)
{
	/* Setting as GPIO pin */
	pinmux_set_swmux(SWMUX_USBOTG_PWR, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_USBOTG_PWR, 0x0048);
    
    /* Set USBOTG_PWR (GPIO3_14) pin direction as Output */
	out32(MX35_GPIO3_BASE + MX35_GPIO_GDIR, in32(MX35_GPIO3_BASE + MX35_GPIO_GDIR) | (1 << 14));
	
	pinmux_set_swmux(SWMUX_USBOTG_OC, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_USBOTG_OC, 0x01c0);
    	
	/* Stop then Reset */
	out32(MX35_USBOTG_BASE + UOG_USBCMD, (in32(MX35_USBOTG_BASE + UOG_USBCMD) & ~UCMD_RUN_STOP) );
 	while (in32(MX35_USBOTG_BASE + UOG_USBCMD) & UCMD_RUN_STOP);
    out32(MX35_USBOTG_BASE + UOG_USBCMD, (in32(MX35_USBOTG_BASE + UOG_USBCMD) | UCMD_RESET) );
 	while (in32(MX35_USBOTG_BASE + UOG_USBCMD) & UCMD_RESET);
	
	/* Disable OverCurrent signal */
	out32(MX35_USBOTG_BASE + USBCTRL, (in32(MX35_USBOTG_BASE + USBCTRL) & ~UCTRL_OCE) );
	/* USBOTG_PWR low active */
	out32(MX35_USBOTG_BASE + USBCTRL, (in32(MX35_USBOTG_BASE + USBCTRL) & ~UCTRL_PP) );
	/* OverCurrent Polarity is Low Active */
	out32(MX35_USBOTG_BASE + USBCTRL, (in32(MX35_USBOTG_BASE + USBCTRL) & ~UCTRL_OCPOL) );
	/* OTG Power Mask */
	out32(MX35_USBOTG_BASE + USBCTRL, (in32(MX35_USBOTG_BASE + USBCTRL) & ~UCTRL_OPM) );
	/* ULPI intr enable */
	out32(MX35_USBOTG_BASE + USBCTRL, (in32(MX35_USBOTG_BASE + USBCTRL) | UCTRL_OWIE) );
	
	/* set UTMI xcvr */
	out32(MX35_USBOTG_BASE + UOG_PORTSC1, ((in32(MX35_USBOTG_BASE + UOG_PORTSC1) & ~PORTSC_PTS_MASK) | PORTSC_PTS_UTMI) );

	/* Enable UTMI interface in PHY control Reg */
	out32(MX35_USBOTG_BASE + USB_PHY_CTR_FUNC, (in32(MX35_USBOTG_BASE + USB_PHY_CTR_FUNC) | USB_UTMI_PHYCTRL_UTMI_ENABLE) );

	if (in32(MX35_USBOTG_BASE + UOG_HCSPARAMS) & HCSPARAMS_PPC)
		out32(MX35_USBOTG_BASE + UOG_PORTSC1, (in32(MX35_USBOTG_BASE + UOG_PORTSC1) | PORTSC_PORT_POWER) );
	
	/* need to reset the controller here so that the ID pin
 	 * is correctly detected.
 	 */
	out32(MX35_USBOTG_BASE + UOG_USBCMD, (in32(MX35_USBOTG_BASE + UOG_USBCMD) | UCMD_RESET) );
 	while (in32(MX35_USBOTG_BASE + UOG_USBCMD) & UCMD_RESET);
}

/*
 * Setup GPIO for NAND to be active
 */
void mx35_3ds_init_nand(void)
{
	pinmux_set_swmux(SWMUX_NFWE_B, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_NFRE_B, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_NFALE, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_NFCLE, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_NFWP_B, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_NF_CE0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_NFRB, MUX_CTL_MUX_MODE_ALT0);
}

/*
 * Setup GPIO for CAN to be active
 */
void mx35_3ds_init_can(void)
{
    unsigned char val;

    /* CAN1 TXCAN */
    pinmux_set_swmux(SWMUX_I2C2_CLK, MUX_CTL_MUX_MODE_ALT1);
    /* CAN1 RXCAN */
    pinmux_set_swmux(SWMUX_I2C2_DAT, MUX_CTL_MUX_MODE_ALT1);
    /* CAN1 RXCAN Input: I2C2_DAT */
    pinmux_set_input(SWINPUT_CAN1_CANRX, 0);
    /* Set CAN_PWDN to low to enable CAN-1 interface */
    init_i2c_clock(); 
    i2c_xfer(MC9S08DZ60_I2C_SLAVE_ADDR, MC9S08DZ60_I2C_GPIO_CTL_REG2, &val, I2C_READ);
    val &= ~MC9S08DZ60_I2C_GPIO_CTL_REG2_CAN;
    i2c_xfer(MC9S08DZ60_I2C_SLAVE_ADDR, MC9S08DZ60_I2C_GPIO_CTL_REG2, &val, I2C_WRITE);
    
    /* CAN2 TXCAN */
    pinmux_set_swmux(SWMUX_FEC_MDC, MUX_CTL_MUX_MODE_ALT1);
    /* CAN2 RXCAN */
    pinmux_set_swmux(SWMUX_FEC_MDIO, MUX_CTL_MUX_MODE_ALT1);
    /* CAN2 RXCAN Input: FEC_MDIO */
    pinmux_set_input(SWINPUT_CAN2_CANRX, 0x2);
}

void mx35_3ds_init_ata(void)
{
    pinmux_set_swmux(SWMUX_ATA_CS0, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_CS1, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DIOR, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DIOW, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DMACK, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_RESET_B, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_IORDY, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA0, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA1, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA2, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA3, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA4, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA5, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA6, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA7, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA8, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA9, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA10, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA11, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA12, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA13, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA14, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DATA15, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_INTRQ, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_BUFF_EN, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DMARQ, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DA0, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DA1, MUX_CTL_MUX_MODE_ALT0);
    pinmux_set_swmux(SWMUX_ATA_DA2, MUX_CTL_MUX_MODE_ALT0);
     
    /* Need fast slew rate for UDMA mode */
    pinmux_set_padcfg(SWPAD_ATA_DATA0, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA1, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA2, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA3, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA4, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA5, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA6, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA7, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA8, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA9, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA10, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA11, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA12, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA13, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA14, PAD_CTL_GPIO_SRE_FAST);
    pinmux_set_padcfg(SWPAD_ATA_DATA15, PAD_CTL_GPIO_SRE_FAST);

	/*
	 * Set ATA timing to PIO mode 0
	 */
	out8(MX35_ATACTL_BASE + 0x00, 0x03);
	out8(MX35_ATACTL_BASE + 0x01, 0x03);
	out8(MX35_ATACTL_BASE + 0x02, 0x05);
	out8(MX35_ATACTL_BASE + 0x03, 0x14);
	out8(MX35_ATACTL_BASE + 0x04, 0x14);
	out8(MX35_ATACTL_BASE + 0x05, 0x06);
	out8(MX35_ATACTL_BASE + 0x06, 0x01);
	out8(MX35_ATACTL_BASE + 0x07, 0x03);
	out8(MX35_ATACTL_BASE + 0x08, 0x02);
	
	/*
	 * Enable ATA interrupt
	 */
	out8(MX35_ATACTL_BASE + 0x2C, 0x08);
	
	/*
	 * take the ATA controller out of reset  
	 */
	out8(MX35_ATACTL_BASE + 0x24, 0x40);
}

void mx35_3ds_init_spi(void)
{
	/* SPI1 */
	pinmux_set_swmux(SWMUX_CSPI1_MISO, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSPI1_MOSI, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSPI1_SCLK, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSPI1_SPI_RDY, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSPI1_SS0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSPI1_SS1, MUX_CTL_MUX_MODE_ALT0);
}

void mx35_3ds_init_lcd(void)
{
	pinmux_set_swmux(SWMUX_LD0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD1, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD2, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD3, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD4, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD5, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD6, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD7, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD8, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD9, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD10, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD11, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD12, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD13, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD14, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD15, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD16, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_LD17, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_D3_VSYNC, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_D3_HSYNC, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_D3_FPSHIFT, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_D3_DRDY, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_D3_REV, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CONTRAST, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_D3_SPL, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_D3_CLS, MUX_CTL_MUX_MODE_ALT0);
}

void mx35_3ds_init_fec(void)
{
	pinmux_set_swmux(SWMUX_FEC_TX_CLK, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_RX_CLK, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_RX_DV, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_COL, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_RDATA0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_TDATA0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_TX_EN, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_MDC, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_MDIO, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_TX_ERR, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_RX_ERR, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_CRS, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_RDATA1, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_TDATA1, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_RDATA2, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_TDATA2, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_RDATA3, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_FEC_TDATA3, MUX_CTL_MUX_MODE_ALT0);

#define FEC_PAD_CTL_COMMON (PAD_CTL_GPIO_DRV_3_3_V | PAD_CTL_GPIO_ODE_DISABLE | PAD_CTL_GPIO_DSE_STD | PAD_CTL_GPIO_SRE_SLOW)

	pinmux_set_padcfg(SWPAD_FEC_TX_CLK, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_ENABLE | PAD_CTL_GPIO_PKE_KEEPER | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_RX_CLK, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_ENABLE | PAD_CTL_GPIO_PKE_KEEPER | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_RX_DV, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_ENABLE | PAD_CTL_GPIO_PKE_KEEPER | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_COL, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_ENABLE | PAD_CTL_GPIO_PKE_KEEPER | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_RDATA0, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_ENABLE | PAD_CTL_GPIO_PKE_KEEPER | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_TDATA0, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_DISABLE | PAD_CTL_GPIO_PKE_NONE | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_TX_EN, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_DISABLE | PAD_CTL_GPIO_PKE_NONE | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_MDC, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_DISABLE | PAD_CTL_GPIO_PKE_NONE | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_MDIO, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_ENABLE | PAD_CTL_GPIO_PKE_KEEPER | PAD_CTL_GPIO_PKE_22K_PU);
	pinmux_set_padcfg(SWPAD_FEC_TX_ERR, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_DISABLE | PAD_CTL_GPIO_PKE_NONE | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_RX_ERR, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_ENABLE | PAD_CTL_GPIO_PKE_KEEPER | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_CRS, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_ENABLE | PAD_CTL_GPIO_PKE_KEEPER | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_RDATA1, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_ENABLE | PAD_CTL_GPIO_PKE_KEEPER | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_TDATA1, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_DISABLE | PAD_CTL_GPIO_PKE_NONE | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_RDATA2, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_ENABLE | PAD_CTL_GPIO_PKE_KEEPER | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_TDATA2, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_DISABLE | PAD_CTL_GPIO_PKE_NONE | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_RDATA3, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_ENABLE | PAD_CTL_GPIO_PKE_KEEPER | PAD_CTL_GPIO_PKE_100K_PD);
	pinmux_set_padcfg(SWPAD_FEC_TDATA3, FEC_PAD_CTL_COMMON |
		PAD_CTL_GPIO_HYS_DISABLE | PAD_CTL_GPIO_PKE_NONE | PAD_CTL_GPIO_PKE_100K_PD);

    /*FEC/UART3 MUX, set SWMUX_COMPARE as GPIO1_5 pin */
	pinmux_set_swmux(SWMUX_COMPARE, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_COMPARE, PAD_CTL_GPIO_HYS_DISABLE | 
		PAD_CTL_GPIO_PKE_NONE | PAD_CTL_GPIO_ODE_DISABLE | PAD_CTL_GPIO_DSE_STD | PAD_CTL_GPIO_SRE_SLOW);
    
	/* set MX35 GPIO1_5 as output high-level for FEC function */
	out32(MX35_GPIO1_BASE + MX35_GPIO_GDIR, in32(MX35_GPIO1_BASE + MX35_GPIO_GDIR) | (1 << 5));
	out32(MX35_GPIO1_BASE + MX35_GPIO_DR, in32(MX35_GPIO2_BASE + MX35_GPIO_DR) | (1 << 5));
}
