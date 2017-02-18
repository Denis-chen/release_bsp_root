/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
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
 * TI L137 EVM with ARM 926 core
 */

/* PLL CONTROLLER */
#define OMAPL137_PLL_BASE               0x01C11000
#define OMAPL137_PLL_PLLDIV2            0x11C
#define OMAPL137_PLL_PLLDIV6            0x168
#define OMAPL137_PLL_PLLM               0x110
#define OMAPL137_PLL_PREDIV             0x114
#define OMAPL137_PLL_POSTDIV            0x128
#define OMAPL137_PLL_INPUT_CLK          24000000 // 24 MHz

/* GPIO */

#define OMAPL137_GPIO_BASE              0x01e26000

#define OMAPL137_GPIO_REVID             0x0
#define OMAPL137_GPIO_BINTEN            0x8
#define OMAPL137_GPIO_DIR01             0x10
#define OMAPL137_GPIO_OUT_DATA01        0x14
#define OMAPL137_GPIO_SET_DATA01        0x18
#define OMAPL137_GPIO_CLR_DATA01        0x1C
#define OMAPL137_GPIO_IN_DATA01         0x20
#define OMAPL137_GPIO_SET_RIS_TRIG01    0x24
#define OMAPL137_GPIO_CLR_RIS_TRIG01    0x28
#define OMAPL137_GPIO_SET_FAL_TRIG01    0x2C
#define OMAPL137_GPIO_CLR_FAL_TRIG01    0x30
#define OMAPL137_GPIO_INTSTAT01         0x34

/* RTC base */

#define OMAPL137_RTC_REGS               0x01c23000

// Extra RTC registers not on regular OMAP

#define OMAPL137_RTC_KICK0R             0x6C
#define OMAPL137_RTC_KICK1R             0x70

// Values to write to KICK regs to unlock RTC for writing
#define OMAPL137_RTC_KICK0R_UNLOCK      0x83E70B13
#define OMAPL137_RTC_KICK1R_UNLOCK      0x95A4F1E0

/* TIMERS */
#define OMAPL137_TIMER_SIZE             0x40
#define OMAPL137_TMR_1_REGS             0x01C20000
#define OMAPL137_TMR_2_REGS             0x01C21000
#define OMAPL137_TMR_PID12              0x00
#define OMAPL137_TMR_EMUMGT             0x04
#define OMAPL137_TMR_GPINTGPEN          0x08
#define OMAPL137_TMR_GPDATGPDIR         0x0C
#define OMAPL137_TMR_TIM12              0x10
#define OMAPL137_TMR_TIM34              0x14
#define OMAPL137_TMR_PRD12              0x18
#define OMAPL137_TMR_PRD34              0x1C
#define OMAPL137_TMR_TCR                0x20
#define OMAPL137_TMR_TGCR               0x24
#define OMAPL137_TMR_WDTCR              0x28
#define OMAPL137_TMR_REL12              0x34
#define OMAPL137_TMR_REL34              0x38
#define OMAPL137_TMR_CAP12              0x3C
#define OMAPL137_TMR_CAP34              0x40
#define OMAPL137_TMR_INTCTLSTAT         0x44

#define OMAPL137_TMR_CMP0               0x60
#define OMAPL137_TMR_CMP1               0x64
#define OMAPL137_TMR_CMP2               0x68
#define OMAPL137_TMR_CMP3               0x6C
#define OMAPL137_TMR_CMP4               0x70
#define OMAPL137_TMR_CMP5               0x74
#define OMAPL137_TMR_CMP6               0x78
#define OMAPL137_TMR_CMP7               0x7C

#define OMAPL137_TCR_ENA34              (0x2 << 22) // auto reload
#define OMAPL137_TCR_ENA12              (0x2 << 6) // auto reload
#define OMAPL137_TGCR_TIM34RS           0x2
#define OMAPL137_TGCR_TIM12RS           0x1
#define OMAPL137_TGCR_TIMMODE           (6)
#define OMAPL137_PRDINTSTAT34           0x00020000

// Set bit == use as GPIO
#define OMAPL137_GPINTGPEN_GPENO34      0x02000000
#define OMAPL137_GPINTGPEN_GPENI34      0x01000000
#define OMAPL137_GPINTGPEN_GPENO12      0x00020000
#define OMAPL137_GPINTGPEN_GPENI12      0x00010000
#define OMAPL137_GPINTGPEN_GPINT34INVO  0x00000200
#define OMAPL137_GPINTGPEN_GPINT34INVI  0x00000100
#define OMAPL137_GPINTGPEN_GPINT12INVO  0x00000002
#define OMAPL137_GPINTGPEN_GPINT12INVI  0x00000001

/* INTERRUPT CONTROLLER */
#define OMAPL137_INTR_BASE              0xfffee000

#define OMAPL137_INTR_REVID             0x000
#define OMAPL137_INTR_CR                0x004
#define OMAPL137_INTR_GER               0x010
#define OMAPL137_INTR_GNLR              0x01C
#define OMAPL137_INTR_SISR              0x020
#define OMAPL137_INTR_SICR              0x024
#define OMAPL137_INTR_EISR              0x028
#define OMAPL137_INTR_EICR              0x02C
#define OMAPL137_INTR_HIEISR            0x034
#define OMAPL137_INTR_HIEICR            0x038
#define OMAPL137_INTR_VBR               0x050
#define OMAPL137_INTR_VSR               0x054
#define OMAPL137_INTR_VNR               0x058
#define OMAPL137_INTR_GPIR              0x080
#define OMAPL137_INTR_GPVR              0x084
#define OMAPL137_INTR_SRSR1             0x200
#define OMAPL137_INTR_SRSR2             0x204
#define OMAPL137_INTR_SRSR3             0x208
#define OMAPL137_INTR_SECR1             0x280
#define OMAPL137_INTR_SECR2             0x284
#define OMAPL137_INTR_SECR3             0x288
#define OMAPL137_INTR_ESR1              0x300
#define OMAPL137_INTR_ESR2              0x304
#define OMAPL137_INTR_ESR3              0x308
#define OMAPL137_INTR_ECR1              0x380
#define OMAPL137_INTR_ECR2              0x384
#define OMAPL137_INTR_ECR3              0x388

#define OMAPL137_INTR_CMR0              0x400 // 23 registers for mappings

#define OMAPL137_INTR_HIPIR1            0x900
#define OMAPL137_INTR_HIPIR2            0x904
#define OMAPL137_INTR_HINLR1            0x1100
#define OMAPL137_INTR_HINLR2            0x1104
#define OMAPL137_INTR_HIER              0x1500
#define OMAPL137_INTR_HIPVR1            0x1600
#define OMAPL137_INTR_HIPVR2            0x1604

/* I2C */
#define OMAPL137_I2C0_BASE              0x01c22000
#define OMAPL137_I2C0_SIZE              0x1000
#define OMAPL137_I2C1_BASE              0x01e28000
#define OMAPL137_I2C1_SIZE              0x1000

#define OMAPL137_I2C_ICOAR              0x00
#define OMAPL137_I2C_ICIMR              0x04
#define OMAPL137_I2C_ICIMR_AL           (1 << 0)
#define OMAPL137_I2C_ICIMR_NACK         (1 << 1)
#define OMAPL137_I2C_ICIMR_ARDY         (1 << 2)
#define OMAPL137_I2C_ICIMR_ICRRDY       (1 << 3)
#define OMAPL137_I2C_ICIMR_ICXRDY       (1 << 4)
#define OMAPL137_I2C_ICIMR_SCD          (1 << 5)
#define OMAPL137_I2C_ICIMR_AAS          (1 << 6)
#define OMAPL137_I2C_ICSTR              0x08
#define OMAPL137_I2C_ICSTR_SDIR         (1 << 14)
#define OMAPL137_I2C_ICSTR_NACKSNT      (1 << 13)
#define OMAPL137_I2C_ICSTR_BB           (1 << 12)
#define OMAPL137_I2C_ICSTR_RSFULL       (1 << 11)
#define OMAPL137_I2C_ICSTR_XSMT         (1 << 10)
#define OMAPL137_I2C_ICSTR_AAS          (1 << 9)
#define OMAPL137_I2C_ICSTR_AD0          (1 << 8)
#define OMAPL137_I2C_ICSTR_SCD          (1 << 5)
#define OMAPL137_I2C_ICSTR_ICXRDY       (1 << 4)
#define OMAPL137_I2C_ICSTR_ICRRDY       (1 << 3)
#define OMAPL137_I2C_ICSTR_ARDY         (1 << 2)
#define OMAPL137_I2C_ICSTR_NACK         (1 << 1)
#define OMAPL137_I2C_ICSTR_AL           (1 << 0)
#define OMAPL137_I2C_ICCLKL             0x0C
#define OMAPL137_I2C_ICCLKH             0x10
#define OMAPL137_I2C_ICCNT              0x14
#define OMAPL137_I2C_ICDRR              0x18
#define OMAPL137_I2C_ICSAR              0x1C
#define OMAPL137_I2C_ICDXR              0x20
#define OMAPL137_I2C_ICMDR              0x24
#define OMAPL137_I2C_ICMDR_FREE         (1 << 14)
#define OMAPL137_I2C_ICMDR_STT          (1 << 13)
#define OMAPL137_I2C_ICMDR_STP          (1 << 11)
#define OMAPL137_I2C_ICMDR_MST          (1 << 10)
#define OMAPL137_I2C_ICMDR_TRX          (1 << 9)
#define OMAPL137_I2C_ICMDR_XA           (1 << 8)
#define OMAPL137_I2C_ICMDR_IRS          (1 << 5)
#define OMAPL137_I2C_ICMDR_STB          (1 << 4)
#define OMAPL137_I2C_ICIVR              0x28
#define OMAPL137_I2C_ICIVR_AL           0x1
#define OMAPL137_I2C_ICIVR_NACK         0x2
#define OMAPL137_I2C_ICIVR_ARDY         0x3
#define OMAPL137_I2C_ICIVR_ICRRDY       0x4
#define OMAPL137_I2C_ICIVR_ICXRDY       0x5
#define OMAPL137_I2C_ICIVR_SCD          0x6
#define OMAPL137_I2C_ICIVR_AAS          0x7
#define OMAPL137_I2C_ICEMDR             0x2C
#define OMAPL137_I2C_ICPSC              0x30
#define OMAPL137_I2C_ICPID1             0x34
#define OMAPL137_I2C_ICPID2             0x38
#define OMAPL1x_I2C_REVID1              0x4415
#define OMAPL1x_I2C_REVID2              0x6
#define OMAPL137_I2C_ICPFUNC            0x48
#define OMAPL137_I2C_ICPDIR             0x4C	
#define OMAPL137_I2C_ICPDIN             0x50
#define OMAPL137_I2C_ICPDOUT            0x54
#define OMAPL137_I2C_ICPDSET            0x58
#define OMAPL137_I2C_ICPDCLR            0x5C

/*
 * PSC
 */
#define OMAPL137_PSC0_BASE              0x01C10000
#define OMAPL137_PSC1_BASE              0x01E27000

#define OMAPL137_PSC_REVID              0x4
#define OMAPL137_PSC_INTEVAL            0x18
#define OMAPL137_PSC_MERRPR0            0x40
#define OMAPL137_PSC_MERRCR0            0x50
#define OMAPL137_PSC_PERRPR             0x60
#define OMAPL137_PSC_PERRCR             0x68
#define OMAPL137_PSC_PTCMD              0x120
#define OMAPL137_PSC_PTSTAT             0x128
#define OMAPL137_PSC_PDSTAT0            0x200
#define OMAPL137_PSC_PDSTAT1            0x204
#define OMAPL137_PSC_PDCTL0             0x300
#define OMAPL137_PSC_PDCTL1             0x304
#define OMAPL137_PSC_PDCFG0             0x400
#define OMAPL137_PSC_PDCFG1             0x404
#define OMAPL137_PSC_MDSTAT0            0x800
#define OMAPL137_PSC_MDCTL0             0xA00

#define PSC0                            0
#define PSC1                            16

#define PSC_MODULE_EDMA                 (PSC0 + 0)
#define PSC_MODULE_EDMA_TC0             (PSC0 + 1)
#define PSC_MODULE_EDMA_TC1             (PSC0 + 2)
#define PSC_MODULE_EMIFA                (PSC0 + 3)
#define PSC_MODULE_SPI0                 (PSC0 + 4)
#define PSC_MODULE_MMCSD                (PSC0 + 5)
#define PSC_MODULE_AINTC                (PSC0 + 6)
#define PSC_MODULE_UART0                (PSC0 + 9)

#define PSC_MODULE_USB0                 (PSC1 + 1)
#define PSC_MODULE_USB1                 (PSC1 + 2)
#define PSC_MODULE_GPIO                 (PSC1 + 3)
#define PSC_MODULE_EMAC                 (PSC1 + 5)
#define PSC_MODULE_MCASP0               (PSC1 + 7)
#define PSC_MODULE_MCASP1               (PSC1 + 8)
#define PSC_MODULE_MCASP2               (PSC1 + 9)
#define PSC_MODULE_SPI1                 (PSC1 + 10)
#define PSC_MODULE_I2C1                 (PSC1 + 11)
#define PSC_MODULE_UART1                (PSC1 + 12)
#define PSC_MODULE_UART2                (PSC1 + 13)
#define PSC_MODULE_LCDC                 (PSC1 + 16)
#define PSC_MODULE_SHRAM                (PSC1 + 31)


/*
 * SYSCFG
 */
#define OMAPL137_SYSCFG_BASE            0x01c14000
#define OMAPL137_SYSCFG_SIZE            0x1000

/*
 * SYSCFG register offsets
 */

#define OMAPL137_SYSCFG_REVID           0x000

#define OMAPL137_SYSCFG_DIEIDR0         0x008
#define OMAPL137_SYSCFG_DIEIDR1         0x00C
#define OMAPL137_SYSCFG_DIEIDR2         0x010
#define OMAPL137_SYSCFG_DIEIDR3         0x014

#define OMAPL137_SYSCFG_BOOTCFG         0x020
#define OMAPL137_SYSCFG_KICK0R          0x038
#define OMAPL137_SYSCFG_KICK1R          0x03C
#define OMAPL137_SYSCFG_HOST0CFG        0x040
#define OMAPL137_SYSCFG_HOST1CFG        0x044

// Values to unlock syscfg regs
#define OMAPL137_SYSCFG_KICK0R_UNLOCK   0x83E70B13
#define OMAPL137_SYSCFG_KICK1R_UNLOCK   0x95A4F1E0

#define OMAPL137_SYSCFG_IRAWSTAT        0x0E0
#define OMAPL137_SYSCFG_IENSTAT         0x0E4
#define OMAPL137_SYSCFG_IENSET          0x0E8
#define OMAPL137_SYSCFG_IENCLR          0x0EC
#define OMAPL137_SYSCFG_EOI             0x0F0
#define OMAPL137_SYSCFG_FLTADDRR        0x0F4
#define OMAPL137_SYSCFG_FLTSTAT         0x0F8

#define OMAPL137_SYSCFG_MSTPRI0         0x110
#define OMAPL137_SYSCFG_MSTPRI1         0x114
#define OMAPL137_SYSCFG_MSTPRI2         0x118

#define OMAPL137_SYSCFG_PINMUX0         0x120
#define OMAPL137_SYSCFG_PINMUX1         0x124
#define OMAPL137_SYSCFG_PINMUX2         0x128
#define OMAPL137_SYSCFG_PINMUX3         0x12C
#define OMAPL137_SYSCFG_PINMUX4         0x130
#define OMAPL137_SYSCFG_PINMUX5         0x134
#define OMAPL137_SYSCFG_PINMUX6         0x138
#define OMAPL137_SYSCFG_PINMUX7         0x13C
#define OMAPL137_SYSCFG_PINMUX8         0x140
#define OMAPL137_SYSCFG_PINMUX9         0x144
#define OMAPL137_SYSCFG_PINMUX10        0x148
#define OMAPL137_SYSCFG_PINMUX11        0x14C
#define OMAPL137_SYSCFG_PINMUX12        0x150
#define OMAPL137_SYSCFG_PINMUX13        0x154
#define OMAPL137_SYSCFG_PINMUX14        0x158
#define OMAPL137_SYSCFG_PINMUX15        0x15C
#define OMAPL137_SYSCFG_PINMUX16        0x160
#define OMAPL137_SYSCFG_PINMUX17        0x164
#define OMAPL137_SYSCFG_PINMUX18        0x168
#define OMAPL137_SYSCFG_PINMUX19        0x16C

/*
 * PINMUX defines - specific to OMAPL137 board
 */

#define OMAPL137_SYSCFG_PINMUX8_19_16_MASK      0x000F0000
#define OMAPL137_SYSCFG_PINMUX8_15_12_MASK      0x0000F000

#define OMAPL137_SYSCFG_PINMUX8_TM64P0_OUT12    0x00040000
#define OMAPL137_SYSCFG_PINMUX8_TM64P0_IN12     0x00004000


#define OMAPL137_SYSCFG_SUSPSRC         0x170
#define OMAPL137_SYSCFG_CHIPSIG         0x174
#define OMAPL137_SYSCFG_CHIPSIG_CLR     0x178

#define OMAPL137_SYSCFG_CFGCHIP0        0x17C
#define OMAPL137_SYSCFG_CFGCHIP1        0x180
#define OMAPL137_SYSCFG_CFGCHIP2        0x184
#define OMAPL137_SYSCFG_CFGCHIP3        0x188
#define OMAPL137_SYSCFG_CFGCHIP4        0x18C

/*
 * SPI
 */
#define OMAPL137_SPI0_BASE              0x01C41000
#define OMAPL137_SPI0_SIZE              0x1000

#define OMAPL137_SPI1_BASE              0x01E12000
#define OMAPL137_SPI1_SIZE              0x1000

/*
 * SPI registers, Offset from SPI base
 */
#define OMAPL137_SPI_GCR0               0x00		/* SPI Global Control Register 0 */
#define OMAPL137_SPIGCR0_RESET          (1 << 0)
#define OMAPL137_SPI_GCR1               0x04		/* SPI Global Control Register 1 */
#define OMAPL137_SPIGCR1_MASTER         (1 << 0)
#define OMAPL137_SPIGCR1_CLKMOD         (1 << 1)
#define OMAPL137_SPIGCR1_LOOPBACK       (1 << 16)
#define OMAPL137_SPIGCR1_SPIENA         (1 << 24)
#define OMAPL137_SPI_INT                0x08		/* SPI Interrupt Register */
#define OMAPL137_SPIINT_BITERRENA       (1 << 4)
#define OMAPL137_SPIINT_OVRNINTEN       (1 << 6)
#define OMAPL137_SPIINT_RXINTEN         (1 << 8)
#define OMAPL137_SPIINT_DMAREQEN        (1 << 16)
#define OMAPL137_SPI_LVL                0x0C		/* SPI Interrupt Level Register */
#define OMAPL137_SPI_FLG                0x10		/* SPI Flag Register */
#define OMAPL137_SPI_PC0                0x14		/* SPI Pin Control Register 0 */
#define OMAPL137_SPI_PC2                0x1C		/* SPI Pin Control Register 2 */
#define OMAPL137_SPIPC_EN0              (1 << 0)
#define OMAPL137_SPIPC_EN1              (1 << 1)
#define OMAPL137_SPIPC_CLK              (1 << 9)
#define OMAPL137_SPIPC_DO               (1 << 10)
#define OMAPL137_SPIPC_DI               (1 << 11)
#define OMAPL137_SPIPC_ENA              (1 << 8)
#define OMAPL137_SPI_DAT1               0x3C		/* SPI Shift Register */
#define	OMAPL137_SPIDAT1_CSHOLD         (1 << 28)
#define	OMAPL137_SPIDAT1_CSNR(c)        ((c) << 16)
#define	OMAPL137_SPIDAT1_DFSEL(d)       ((d) << 24)
#define	OMAPL137_SPIDAT1_WDEL           (1 << 26)
#define OMAPL137_SPI_BUF                0x40		/* SPI Buffer Register */
#define	OMAPL137_SPIBUF_RXEMPTY         (1 << 31)
#define	OMAPL137_SPIBUF_RXOVR           (1 << 30)
#define	OMAPL137_SPIBUF_TXFULL          (1 << 29)
#define	OMAPL137_SPIBUF_BITERR          (1 << 28)
#define OMAPL137_SPI_EMU                0x44		/* SPI Emulation Register */
#define OMAPL137_SPI_DELAY              0x48		/* SPI Delay Register */
#define	OMAPL137_SPIDELAY_C2TDELAY(d)   ((d) << 24)
#define	OMAPL137_SPIDELAY_T2CDELAY(d)   ((d) << 16)
#define OMAPL137_SPI_DEF                0x4C		/* SPI Default Chip Select Register */
#define	OMAPL137_SPIDEF_EN1DEF          (1 << 1)
#define	OMAPL137_SPIDEF_EN0DEF          (1 << 0)
#define OMAPL137_SPI_FMT0               0x50		/* SPI Data Format Register 0 */
#define OMAPL137_SPI_FMT1               0x54		/* SPI Data Format Register 1 */
#define OMAPL137_SPI_FMT2               0x58		/* SPI Data Format Register 2 */
#define OMAPL137_SPI_FMT3               0x5C		/* SPI Data Format Register 3 */
#define	OMAPL137_SPIFMT_CLEN(l)         ((l) & 0x1F)
#define	OMAPL137_SPIFMT_PRESCALE(p)     (((p) & 0xFF) << 8)
#define	OMAPL137_SPIFMT_PHASE1          (1 << 16)
#define	OMAPL137_SPIFMT_POLARITY1       (1 << 17)
#define	OMAPL137_SPIFMT_SHIFTLSB        (1 << 20)
#define	OMAPL137_SPIFMT_WAITENA         (1 << 21)


/*
 * EDMA
 */
#define OMAPL137_EDMA_BASE              0x01C00000
#define OMAPL137_EDMA_SIZE              0x8800

/*
 * Registers, Offset from EDMA base
 */
#define	OMAPL137_EDMA_PID               0x00
#define	OMAPL137_EDMA_CCCFG             0x04

/* Global registers, offset from EDMA base */
#define	OMAPL137_EDMA_QCHMAP(c)         (0x200 + (c) * 4)
#define	OMAPL137_EDMA_DMAQNUM(c)        (0x240 + (c) * 4)
#define	OMAPL137_EDMA_QDMAQNUM          0x260
#define	OMAPL137_EDMA_QUEPRI            0x284
#define	OMAPL137_EDMA_EMR               0x300
#define	OMAPL137_EDMA_EMRH              0x304
#define	OMAPL137_EDMA_EMCR              0x308
#define	OMAPL137_EDMA_EMCRH             0x30C
#define	OMAPL137_EDMA_QEMR              0x310
#define	OMAPL137_EDMA_QEMCR             0x314
#define	OMAPL137_EDMA_CCERR             0x318
#define	OMAPL137_EDMA_CCERRCLR          0x31C
#define	OMAPL137_EDMA_EEVAL             0x320
#define	OMAPL137_EDMA_DRAE(c)           (0x340 + (c) * 8)
#define	OMAPL137_EDMA_DRAEH(c)          (0x344 + (c) * 8)
#define	OMAPL137_EDMA_QRAE(c)           (0x380 + (c) * 4)
#define	OMAPL137_EDMA_QRAE(c)           (0x380 + (c) * 4)
#define	OMAPL137_EDMA_Q0E(c)            (0x400 + (c) * 4)
#define	OMAPL137_EDMA_Q1E(c)            (0x440 + (c) * 4)
#define	OMAPL137_EDMA_QSTAT0            0x600
#define	OMAPL137_EDMA_QSTAT1            0x604
#define	OMAPL137_EDMA_QWMTHRA           0x620
#define	OMAPL137_EDMA_CCSTAT            0x640

/* Channel registers, offset from EDMA base */
#define	OMAPL137_EDMA_GLOBAL            0x1000
#define	OMAPL137_EDMA_REGION0           0x2000
#define	OMAPL137_EDMA_REGION1           0x2200
#define	OMAPL137_EDMA_REGION2           0x2400
#define	OMAPL137_EDMA_REGION3           0x2600
#define	OMAPL137_EDMA_ER                0x00
#define	OMAPL137_EDMA_ERH               0x04
#define	OMAPL137_EDMA_ECR               0x08
#define	OMAPL137_EDMA_ECRH              0x0C
#define	OMAPL137_EDMA_ESR               0x10
#define	OMAPL137_EDMA_ESRH              0x14
#define	OMAPL137_EDMA_CER               0x18
#define	OMAPL137_EDMA_CERH              0x1C
#define	OMAPL137_EDMA_EER               0x20
#define	OMAPL137_EDMA_EERH              0x24
#define	OMAPL137_EDMA_EECR              0x28
#define	OMAPL137_EDMA_EECRH             0x2C
#define	OMAPL137_EDMA_EESR              0x30
#define	OMAPL137_EDMA_EESRH             0x34
#define	OMAPL137_EDMA_SER               0x38
#define	OMAPL137_EDMA_SERH              0x3C
#define	OMAPL137_EDMA_SECR              0x40
#define	OMAPL137_EDMA_SECRH             0x44
#define	OMAPL137_EDMA_IER               0x50
#define	OMAPL137_EDMA_IERH              0x54
#define	OMAPL137_EDMA_IECR              0x58
#define	OMAPL137_EDMA_IECRH             0x5C
#define	OMAPL137_EDMA_IESR              0x60
#define	OMAPL137_EDMA_IESRH             0x64
#define	OMAPL137_EDMA_IPR               0x68
#define	OMAPL137_EDMA_IPRH              0x6C
#define	OMAPL137_EDMA_ICR               0x70
#define	OMAPL137_EDMA_ICRH              0x74
#define	OMAPL137_EDMA_IEVAL             0x78
#define	OMAPL137_EDMA_QER               0x80
#define	OMAPL137_EDMA_QEER              0x84
#define	OMAPL137_EDMA_QEECR             0x88
#define	OMAPL137_EDMA_QEESR             0x8C
#define	OMAPL137_EDMA_QSER              0x90
#define	OMAPL137_EDMA_QSECR             0x94
/* Parameter RAM base, offser from EDMA base */
#define	OMAPL137_EDMA_PARAM_BASE        0x4000

#define OMAPL137_RAM_BASE               0xc0000000
