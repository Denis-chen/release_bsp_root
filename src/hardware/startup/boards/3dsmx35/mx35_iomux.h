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


#ifndef	__ARM_MX35_IOMUX_H_INCLUDED
#define	__ARM_MX35_IOMUX_H_INCLUDED

/*
 * offsets of IOMUXC registers from MX35_IOMUX_SWMUX
 * where MX35_IOMUX_SWMUX = MX35_IOMUXC_BASE + 0x4
 */
#define	SWMUX_CAPTURE			0
#define	SWMUX_COMPARE			1
#define SWMUX_WDOG_RESET		2
#define SWMUX_GPIO1_0			3
#define SWMUX_GPIO1_1			4
#define SWMUX_GPIO2_0			5
#define SWMUX_GPIO3_0			6
#define SWMUX_CLKO				7
#define SWMUX_VSTBY				8
#define SWMUX_A0				9
#define SWMUX_A1				10
#define SWMUX_A2				11
#define SWMUX_A3				12
#define SWMUX_A4				13
#define SWMUX_A5				14
#define SWMUX_A6				15
#define SWMUX_A7				16
#define SWMUX_A8				17
#define SWMUX_A9				18
#define SWMUX_A10				10
#define SWMUX_MA10				20
#define SWMUX_A11				21
#define SWMUX_A12				22
#define SWMUX_A13				23
#define SWMUX_A14				24
#define SWMUX_A15				25
#define SWMUX_A16				26
#define SWMUX_A17				27
#define SWMUX_A18				28
#define SWMUX_A19				29
#define SWMUX_A20				30
#define SWMUX_A21				31
#define SWMUX_A22				32
#define SWMUX_A23				33
#define SWMUX_A24				34
#define SWMUX_A25				35
#define SWMUX_EB0				36
#define SWMUX_EB1				37
#define SWMUX_OE				38
#define SWMUX_CS0				39
#define SWMUX_CS1				40
#define SWMUX_CS2				41
#define SWMUX_CS3				42
#define SWMUX_CS4				43
#define SWMUX_CS5				44
#define SWMUX_NF_CE0			45
#define SWMUX_LBA				46
#define SWMUX_BCLK				47
#define SWMUX_RW				48
#define SWMUX_NFWE_B			49
#define SWMUX_NFRE_B			50
#define SWMUX_NFALE				51
#define SWMUX_NFCLE				52
#define SWMUX_NFWP_B			53
#define SWMUX_NFRB				54
#define SWMUX_CSI_D8			55
#define SWMUX_CSI_D9			56
#define SWMUX_CSI_D10			57
#define SWMUX_CSI_D11			58
#define SWMUX_CSI_D12			59
#define SWMUX_CSI_D13			60
#define SWMUX_CSI_D14			61
#define SWMUX_CSI_D15			62
#define SWMUX_CSI_MCLK			63
#define SWMUX_CSI_VSYNC			64
#define SWMUX_CSI_HSYNC			65
#define SWMUX_CSI_PIXCLK		66
#define SWMUX_I2C1_CLK			67
#define SWMUX_I2C1_DAT			68
#define SWMUX_I2C2_CLK			69
#define SWMUX_I2C2_DAT			70
#define SWMUX_STXD4				71
#define SWMUX_SRXD4				72
#define SWMUX_SCK4				73
#define SWMUX_STXFS4			74
#define SWMUX_STXD5				75
#define SWMUX_SRXD5				76
#define SWMUX_SCK5				77
#define SWMUX_STXFS5			78
#define SWMUX_SCKR				79
#define SWMUX_FSR				80
#define SWMUX_HCKR				81
#define SWMUX_SCKT				82
#define SWMUX_FST				83
#define SWMUX_HCKT				84
#define SWMUX_TX5_RX0			85
#define SWMUX_TX4_RX1			86
#define SWMUX_TX3_RX2			87
#define SWMUX_TX2_RX3			88
#define SWMUX_TX1				89
#define SWMUX_TX0				90
#define SWMUX_CSPI1_MOSI		91
#define SWMUX_CSPI1_MISO		92
#define SWMUX_CSPI1_SS0			93
#define SWMUX_CSPI1_SS1			94
#define SWMUX_CSPI1_SCLK		95
#define SWMUX_CSPI1_SPI_RDY		96
#define SWMUX_RXD1				97
#define SWMUX_TXD1				98
#define SWMUX_RTS1				99
#define SWMUX_CTS1				100
#define SWMUX_RXD2				101
#define SWMUX_TXD2				102
#define SWMUX_RTS2				103
#define SWMUX_CTS2				104
#define SWMUX_USBOTG_PWR		105
#define SWMUX_USBOTG_OC			106
#define SWMUX_LD0				107
#define SWMUX_LD1				108
#define SWMUX_LD2				109
#define SWMUX_LD3				110
#define SWMUX_LD4				111
#define SWMUX_LD5				112
#define SWMUX_LD6				113
#define SWMUX_LD7				114
#define SWMUX_LD8				115
#define SWMUX_LD9				116
#define SWMUX_LD10				117
#define SWMUX_LD11				118
#define SWMUX_LD12				119
#define SWMUX_LD13				120
#define SWMUX_LD14				121
#define SWMUX_LD15				122
#define SWMUX_LD16				123
#define SWMUX_LD17				124
#define SWMUX_LD18				125
#define SWMUX_LD19				126
#define SWMUX_LD20				127
#define SWMUX_LD21				128
#define SWMUX_LD22				129
#define SWMUX_LD23				130
#define SWMUX_D3_HSYNC			131
#define SWMUX_D3_FPSHIFT		132
#define SWMUX_D3_DRDY			133
#define SWMUX_CONTRAST			134
#define SWMUX_D3_VSYNC			135
#define SWMUX_D3_REV			136
#define SWMUX_D3_CLS			137
#define SWMUX_D3_SPL			138
#define SWMUX_SD1_CMD			139
#define SWMUX_SD1_CLK			140
#define SWMUX_SD1_DATA0			141
#define SWMUX_SD1_DATA1			142
#define SWMUX_SD1_DATA2			143
#define SWMUX_SD1_DATA3			144
#define SWMUX_SD2_CMD			145
#define SWMUX_SD2_CLK			146
#define SWMUX_SD2_DATA0			147
#define SWMUX_SD2_DATA1			148
#define SWMUX_SD2_DATA2			149
#define SWMUX_SD2_DATA3			150
#define SWMUX_ATA_CS0			151
#define SWMUX_ATA_CS1			152
#define SWMUX_ATA_DIOR			153
#define SWMUX_ATA_DIOW			154
#define SWMUX_ATA_DMACK			155
#define SWMUX_ATA_RESET_B		156
#define SWMUX_ATA_IORDY			157
#define SWMUX_ATA_DATA0			158
#define SWMUX_ATA_DATA1			159
#define SWMUX_ATA_DATA2			160
#define SWMUX_ATA_DATA3			161
#define SWMUX_ATA_DATA4			162
#define SWMUX_ATA_DATA5			163
#define SWMUX_ATA_DATA6			164
#define SWMUX_ATA_DATA7			165
#define SWMUX_ATA_DATA8			166
#define SWMUX_ATA_DATA9			167
#define SWMUX_ATA_DATA10		168
#define SWMUX_ATA_DATA11		169
#define SWMUX_ATA_DATA12		170
#define SWMUX_ATA_DATA13		171
#define SWMUX_ATA_DATA14		172
#define SWMUX_ATA_DATA15		173
#define SWMUX_ATA_INTRQ			174
#define SWMUX_ATA_BUFF_EN		175
#define SWMUX_ATA_DMARQ			176
#define SWMUX_ATA_DA0			177
#define SWMUX_ATA_DA1			178
#define SWMUX_ATA_DA2			179
#define SWMUX_MLB_CLK			180
#define SWMUX_MLB_DAT			181
#define SWMUX_MLB_SIG			182
#define SWMUX_FEC_TX_CLK		183
#define SWMUX_FEC_RX_CLK		184
#define SWMUX_FEC_RX_DV			185
#define SWMUX_FEC_COL			186
#define SWMUX_FEC_RDATA0		187
#define SWMUX_FEC_TDATA0		188
#define SWMUX_FEC_TX_EN			189
#define SWMUX_FEC_MDC			190
#define SWMUX_FEC_MDIO			191
#define SWMUX_FEC_TX_ERR		192
#define SWMUX_FEC_RX_ERR		193
#define SWMUX_FEC_CRS			194
#define SWMUX_FEC_RDATA1		195
#define SWMUX_FEC_TDATA1		196
#define SWMUX_FEC_RDATA2		197
#define SWMUX_FEC_TDATA2		198
#define SWMUX_FEC_RDATA3		199
#define SWMUX_FEC_TDATA3		200

/*
 * Bit definitions for SW_MUX_CTL registers
 */
#define MUX_CTL_SION				(0x1 << 4)
#define	MUX_CTL_MUX_MODE_ALT0		0
#define	MUX_CTL_MUX_MODE_ALT1		1
#define MUX_CTL_MUX_MODE_ALT2		2
#define	MUX_CTL_MUX_MODE_ALT3		3
#define	MUX_CTL_MUX_MODE_ALT4		4
#define	MUX_CTL_MUX_MODE_ALT5		5
#define	MUX_CTL_MUX_MODE_ALT6		6
#define	MUX_CTL_MUX_MODE_ALT7		7

/*
 * offsets of IOMUXC registers from MX35_IOMUX_SWPAD
 * where MX35_IOMUX_SWPAD = MX35_IOMUXC_BASE + 0x328
 */
#define	SWPAD_CAPTURE			0
#define	SWPAD_COMPARE			1
#define SWPAD_WDOG_RESET		2
#define SWPAD_GPIO1_0			3
#define SWPAD_GPIO1_1			4
#define SWPAD_GPIO2_0			5
#define SWPAD_GPIO3_0			6
#define SWPAD_RESET_IN_B		7
#define SWPAD_POR_B				8
#define SWPAD_CLKO				9
#define SWPAD_BOOT_MODE0		10
#define SWPAD_BOOT_MODE1		11
#define SWPAD_CLK_MODE0			12
#define SWPAD_CLK_MODE1			13
#define SWPAD_POWER_FAIL		14
#define SWPAD_VSTBY				15
#define SWPAD_A0				16
#define SWPAD_A1				17
#define SWPAD_A2				18
#define SWPAD_A3				19
#define SWPAD_A4				20
#define SWPAD_A5				21
#define SWPAD_A6				22
#define SWPAD_A7				23
#define SWPAD_A8				24
#define SWPAD_A9				25
#define SWPAD_A10				26
#define SWPAD_MA10				27
#define SWPAD_A11				28
#define SWPAD_A12				29
#define SWPAD_A13				30
#define SWPAD_A14				31
#define SWPAD_A15				32
#define SWPAD_A16				33
#define SWPAD_A17				34
#define SWPAD_A18				35
#define SWPAD_A19				36
#define SWPAD_A20				37
#define SWPAD_A21				38
#define SWPAD_A22				39
#define SWPAD_A23				40
#define SWPAD_A24				41
#define SWPAD_A25				42
#define SWPAD_SDBA1				43
#define SWPAD_SDBA0				44
#define SWPAD_SD0				45
#define SWPAD_SD1				46
#define SWPAD_SD2				47
#define SWPAD_SD3				48
#define SWPAD_SD4				49
#define SWPAD_SD5				50
#define SWPAD_SD6				51
#define SWPAD_SD7				52
#define SWPAD_SD8				53
#define SWPAD_SD9				54
#define SWPAD_SD10				55
#define SWPAD_SD11				56
#define SWPAD_SD12				57
#define SWPAD_SD13				58
#define SWPAD_SD14				59
#define SWPAD_SD15				60
#define SWPAD_SD16				61
#define SWPAD_SD17				62
#define SWPAD_SD18				63
#define SWPAD_SD19				64
#define SWPAD_SD20				65
#define SWPAD_SD21				66
#define SWPAD_SD22				67
#define SWPAD_SD23				68
#define SWPAD_SD24				69
#define SWPAD_SD25				70
#define SWPAD_SD26				71
#define SWPAD_SD27				72
#define SWPAD_SD28				73
#define SWPAD_SD29				74
#define SWPAD_SD30				75
#define SWPAD_SD31				76
#define SWPAD_DQM0				77
#define SWPAD_DQM1				78
#define SWPAD_DQM2				79
#define SWPAD_DQM3				80
#define SWPAD_EB0				81
#define SWPAD_EB1				82
#define SWPAD_OE				83
#define SWPAD_CS0				84
#define SWPAD_CS1				85
#define SWPAD_CS2				86
#define SWPAD_CS3				87
#define SWPAD_CS4				88
#define SWPAD_CS5				89
#define SWPAD_NF_CE0			90
#define SWPAD_ECB				91
#define SWPAD_LBA				92
#define SWPAD_BCLK				93
#define SWPAD_RW				94
#define SWPAD_RAS				95
#define SWPAD_CAS				96
#define SWPAD_SDWE				97
#define SWPAD_SDCKE0			98
#define SWPAD_SDCKE1			99
#define SWPAD_SDCLK				100
#define SWPAD_SDQS0				101
#define SWPAD_SDQS1				102
#define SWPAD_SDQS2				103
#define SWPAD_SDQS3				104
#define SWPAD_NFWE_B			105
#define SWPAD_NFRE_B			106
#define SWPAD_NFALE				107
#define SWPAD_NFCLE				108
#define SWPAD_NFWP_B			109
#define SWPAD_NFRB				110
#define SWPAD_D15				111
#define SWPAD_D14				112
#define SWPAD_D13				113
#define SWPAD_D12				114
#define SWPAD_D11				115
#define SWPAD_D10				116
#define SWPAD_D9				117
#define SWPAD_D8				118
#define SWPAD_D7				119
#define SWPAD_D6				120
#define SWPAD_D5				121
#define SWPAD_D4				122
#define SWPAD_D3				123
#define SWPAD_D2				124
#define SWPAD_D1				125
#define SWPAD_D0				126
#define SWPAD_CSI_D8			127
#define SWPAD_CSI_D9			128
#define SWPAD_CSI_D10			129
#define SWPAD_CSI_D11			130
#define SWPAD_CSI_D12			131
#define SWPAD_CSI_D13			132
#define SWPAD_CSI_D14			133
#define SWPAD_CSI_D15			134
#define SWPAD_CSI_MCLK			135
#define SWPAD_CSI_VSYNC			136
#define SWPAD_CSI_HSYNC			137
#define SWPAD_CSI_PIXCLK		138
#define SWPAD_I2C1_CLK			139
#define SWPAD_I2C1_DAT			140
#define SWPAD_I2C2_CLK			141
#define SWPAD_I2C2_DAT			142
#define SWPAD_STXD4				143
#define SWPAD_SRXD4				144
#define SWPAD_SCK4				145
#define SWPAD_STXFS4			146
#define SWPAD_STXD5				147
#define SWPAD_SRXD5				148
#define SWPAD_SCK5				149
#define SWPAD_STXFS5			150
#define SWPAD_SCKR				151
#define SWPAD_FSR				152
#define SWPAD_HCKR				153
#define SWPAD_SCKT				154
#define SWPAD_FST				155
#define SWPAD_HCKT				156
#define SWPAD_TX5_RX0			157
#define SWPAD_TX4_RX1			158
#define SWPAD_TX3_RX2			159
#define SWPAD_TX2_RX3			160
#define SWPAD_TX1				161
#define SWPAD_TX0				162
#define SWPAD_CSPI1_MOSI		163
#define SWPAD_CSPI1_MISO		164
#define SWPAD_CSPI1_SS0			165
#define SWPAD_CSPI1_SS1			166
#define SWPAD_CSPI1_SCLK		167
#define SWPAD_CSPI1_SPI_RDY		168
#define SWPAD_RXD1				169
#define SWPAD_TXD1				170
#define SWPAD_RTS1				171
#define SWPAD_CTS1				172
#define SWPAD_RXD2				173
#define SWPAD_TXD2				174
#define SWPAD_RTS2				175
#define SWPAD_CTS2				176
#define SWPAD_RTCK				177
#define SWPAD_TCK				178
#define SWPAD_TMS				179
#define SWPAD_TDI				180
#define SWPAD_TDO				181
#define SWPAD_TRSTB				182
#define SWPAD_DE_B				183
#define SWPAD_SJC_MOD			184
#define SWPAD_USBOTG_PWR		185
#define SWPAD_USBOTG_OC			186
#define SWPAD_LD0				187
#define SWPAD_LD1				188
#define SWPAD_LD2				189
#define SWPAD_LD3				190
#define SWPAD_LD4				191
#define SWPAD_LD5				192
#define SWPAD_LD6				193
#define SWPAD_LD7				194
#define SWPAD_LD8				195
#define SWPAD_LD9				196
#define SWPAD_LD10				197
#define SWPAD_LD11				198
#define SWPAD_LD12				199
#define SWPAD_LD13				200
#define SWPAD_LD14				201
#define SWPAD_LD15				202
#define SWPAD_LD16				203
#define SWPAD_LD17				204
#define SWPAD_LD18				205
#define SWPAD_LD19				206
#define SWPAD_LD20				207
#define SWPAD_LD21				208
#define SWPAD_LD22				209
#define SWPAD_LD23				210
#define SWPAD_D3_HSYNC			211
#define SWPAD_D3_FPSHIFT		212
#define SWPAD_D3_DRDY			213
#define SWPAD_CONTRAST			214
#define SWPAD_D3_VSYNC			215
#define SWPAD_D3_REV			216
#define SWPAD_D3_CLS			217
#define SWPAD_D3_SPL			218
#define SWPAD_SD1_CMD			219
#define SWPAD_SD1_CLK			220
#define SWPAD_SD1_DATA0			221
#define SWPAD_SD1_DATA1			222
#define SWPAD_SD1_DATA2			223
#define SWPAD_SD1_DATA3			224
#define SWPAD_SD2_CMD			225
#define SWPAD_SD2_CLK			226
#define SWPAD_SD2_DATA0			227
#define SWPAD_SD2_DATA1			228
#define SWPAD_SD2_DATA2			229
#define SWPAD_SD2_DATA3			230
#define SWPAD_ATA_CS0			231
#define SWPAD_ATA_CS1			232
#define SWPAD_ATA_DIOR			233
#define SWPAD_ATA_DIOW			234
#define SWPAD_ATA_DMACK			235
#define SWPAD_ATA_RESET_B		236
#define SWPAD_ATA_IORDY			237
#define SWPAD_ATA_DATA0			238
#define SWPAD_ATA_DATA1			239
#define SWPAD_ATA_DATA2			240
#define SWPAD_ATA_DATA3			241
#define SWPAD_ATA_DATA4			242
#define SWPAD_ATA_DATA5			243
#define SWPAD_ATA_DATA6			244
#define SWPAD_ATA_DATA7			245
#define SWPAD_ATA_DATA8			246
#define SWPAD_ATA_DATA9			247
#define SWPAD_ATA_DATA10		248
#define SWPAD_ATA_DATA11		249
#define SWPAD_ATA_DATA12		250
#define SWPAD_ATA_DATA13		251
#define SWPAD_ATA_DATA14		252
#define SWPAD_ATA_DATA15		253
#define SWPAD_ATA_INTRQ			254
#define SWPAD_ATA_BUFF_EN		255
#define SWPAD_ATA_DMARQ			256
#define SWPAD_ATA_DA0			257
#define SWPAD_ATA_DA1			258
#define SWPAD_ATA_DA2			259
#define SWPAD_MLB_CLK			260
#define SWPAD_MLB_DAT			261
#define SWPAD_MLB_SIG			262
#define SWPAD_FEC_TX_CLK		263
#define SWPAD_FEC_RX_CLK		264
#define SWPAD_FEC_RX_DV			265
#define SWPAD_FEC_COL			266
#define SWPAD_FEC_RDATA0		267
#define SWPAD_FEC_TDATA0		268
#define SWPAD_FEC_TX_EN			269
#define SWPAD_FEC_MDC			270
#define SWPAD_FEC_MDIO			271
#define SWPAD_FEC_TX_ERR		272
#define SWPAD_FEC_RX_ERR		273
#define SWPAD_FEC_CRS			274
#define SWPAD_FEC_RDATA1		275
#define SWPAD_FEC_TDATA1		276
#define SWPAD_FEC_RDATA2		277
#define SWPAD_FEC_TDATA2		278
#define SWPAD_FEC_RDATA3		279
#define SWPAD_FEC_TDATA3		280
#define SWPAD_EXT_ARMCLK		281
#define SWPAD_TEST_MODE			282

/*
 * Bit definitions for SW_PAD_CTL registers
 */

/* GPIO type pads */
#define PAD_CTL_GPIO_DRV_3_3_V		(0x0 << 13)
#define PAD_CTL_GPIO_DRV_1_8_V		(0x1 << 13)
#define PAD_CTL_GPIO_HYS_DISABLE	(0x0 << 8)
#define PAD_CTL_GPIO_HYS_ENABLE		(0x1 << 8)
#define PAD_CTL_GPIO_PKE_NONE		(0x0 << 4)
#define PAD_CTL_GPIO_PKE_KEEPER		(0x8 << 4)
#define PAD_CTL_GPIO_PKE_100K_PD	(0xc << 4)
#define PAD_CTL_GPIO_PKE_47K_PU		(0xd << 4)
#define PAD_CTL_GPIO_PKE_100K_PU	(0xe << 4)
#define PAD_CTL_GPIO_PKE_22K_PU		(0xf << 4)
#define PAD_CTL_GPIO_ODE_DISABLE	(0x0 << 3)
#define PAD_CTL_GPIO_ODE_ENABLE		(0x1 << 3)
#define PAD_CTL_GPIO_DSE_STD		(0x0 << 1)
#define PAD_CTL_GPIO_DSE_HIGH		(0x1 << 1)
#define PAD_CTL_GPIO_DSE_MAX		(0x2 << 1)
#define	PAD_CTL_GPIO_SRE_SLOW		(0x0 << 0)
#define	PAD_CTL_GPIO_SRE_FAST		(0x1 << 0)

/* DDR type pads */
#define PAD_CTL_DDR_DRV_3_3_V		(0x0 << 13)
#define PAD_CTL_DDR_DRV_1_8_V		(0x1 << 13)
#define PAD_CTL_DDR_TYPE_MOBILE		(0x0 << 11)
#define PAD_CTL_DDR_TYPE_SDRAM		(0x1 << 11)
#define PAD_CTL_DDR_TYPE_DDR2		(0x2 << 11)
#define PAD_CTL_DDR_PKE_DISABLE		(0x0 << 7)
#define PAD_CTL_DDR_PKE_ENABLE		(0x1 << 7)
#define PAD_CTL_DDR_ODE_DISABLE		(0x0 << 3)
#define PAD_CTL_DDR_ODE_ENABLE		(0x1 << 3)
#define PAD_CTL_DDR_DSE_STD			(0x0 << 1)
#define PAD_CTL_DDR_DSE_HIGH		(0x1 << 1)
#define PAD_CTL_DDR_DSE_MAX			(0x2 << 1)

/*
 * offsets of IOMUXC registers from MX35_IOMUX_INPUT
 * where MX35_IOMUX_INPUT = MX35_IOMUXC_BASE + 0x7a8
 */
#define SWINPUT_AUDMUX_P5_RXCLK			0
#define SWINPUT_AUDMUX_P5_RXFS			1
#define SWINPUT_AUDMUX_P6_DA			2
#define SWINPUT_AUDMUX_P6_DB			3
#define SWINPUT_AUDMUX_P6_RXCLK			4
#define SWINPUT_AUDMUX_P6_RXFS			5
#define SWINPUT_AUDMUX_P6_TXCLK			6
#define SWINPUT_AUDMUX_P6_TXFS			7
#define SWINPUT_CAN1_CANRX				8
#define SWINPUT_CAN2_CANRX				9
#define SWINPUT_CCM_32K_MUXED			10
#define SWINPUT_CCM_PMIC_RDY			11
#define SWINPUT_CSPI1_SS2_B				12
#define SWINPUT_CSPI1_SS3_B				13
#define SWINPUT_CSPI2_CLK_IN			14
#define SWINPUT_CSPI2_DATAREADY_B		15
#define SWINPUT_CSPI2_MISO				16
#define SWINPUT_CSPI2_MOSI				17
#define SWINPUT_CSPI2_SS0_B				18
#define SWINPUT_CSPI2_SS1_B				19
#define SWINPUT_CSPI2_SS2_B				20
#define SWINPUT_CSPI2_SS3_B				21
#define SWINPUT_EMI_WEIM_DTACK_B		22
#define SWINPUT_ESDHC1_DAT4_IN			23
#define SWINPUT_ESDHC1_DAT5_IN			24
#define SWINPUT_ESDHC1_DAT6_IN			25
#define SWINPUT_ESDHC1_DAT7_IN			26
#define SWINPUT_ESDHC3_CARD_CLK_IN		27
#define SWINPUT_ESDHC3_CMD_IN			28
#define SWINPUT_ESDHC3_DAT0				29
#define SWINPUT_ESDHC3_DAT1				30
#define SWINPUT_ESDHC3_DAT2				31
#define SWINPUT_ESDHC3_DAT3				32
#define SWINPUT_GPIO1_IN_0				33
#define SWINPUT_GPIO1_IN_10				34
#define SWINPUT_GPIO1_IN_11				35
#define SWINPUT_GPIO1_IN_1				36
#define SWINPUT_GPIO1_IN_20				37
#define SWINPUT_GPIO1_IN_21				38
#define SWINPUT_GPIO1_IN_22				39
#define SWINPUT_GPIO1_IN_2				40
#define SWINPUT_GPIO1_IN_3				41
#define SWINPUT_GPIO1_IN_4				42
#define SWINPUT_GPIO1_IN_5				43
#define SWINPUT_GPIO1_IN_6				44
#define SWINPUT_GPIO1_IN_7				45
#define SWINPUT_GPIO1_IN_8				46
#define SWINPUT_GPIO1_IN_9				47
#define SWINPUT_GPIO2_IN_0				48
#define SWINPUT_GPIO2_IN_10				49
#define SWINPUT_GPIO2_IN_11				50
#define SWINPUT_GPIO2_IN_12				51
#define SWINPUT_GPIO2_IN_13				52
#define SWINPUT_GPIO2_IN_14				53
#define SWINPUT_GPIO2_IN_15				54
#define SWINPUT_GPIO2_IN_16				55
#define SWINPUT_GPIO2_IN_17				56
#define SWINPUT_GPIO2_IN_18				57
#define SWINPUT_GPIO2_IN_19				58
#define SWINPUT_GPIO2_IN_1				59
#define SWINPUT_GPIO2_IN_20				60
#define SWINPUT_GPIO2_IN_21				61
#define SWINPUT_GPIO2_IN_22				62
#define SWINPUT_GPIO2_IN_23				63
#define SWINPUT_GPIO2_IN_24				64
#define SWINPUT_GPIO2_IN_25				65
#define SWINPUT_GPIO2_IN_26				66
#define SWINPUT_GPIO2_IN_27				67
#define SWINPUT_GPIO2_IN_28				68
#define SWINPUT_GPIO2_IN_29				69
#define SWINPUT_GPIO2_IN_2				70
#define SWINPUT_GPIO2_IN_30				71
#define SWINPUT_GPIO2_IN_31				72
#define SWINPUT_GPIO2_IN_3				73
#define SWINPUT_GPIO2_IN_4				74
#define SWINPUT_GPIO2_IN_5				75
#define SWINPUT_GPIO2_IN_6				76
#define SWINPUT_GPIO2_IN_7				77
#define SWINPUT_GPIO2_IN_8				78
#define SWINPUT_GPIO2_IN_9				79
#define SWINPUT_GPIO3_IN_0				80
#define SWINPUT_GPIO3_IN_10				81
#define SWINPUT_GPIO3_IN_11				82
#define SWINPUT_GPIO3_IN_12				83
#define SWINPUT_GPIO3_IN_13				84
#define SWINPUT_GPIO3_IN_14				85
#define SWINPUT_GPIO3_IN_15				86
#define SWINPUT_GPIO3_IN_4				87
#define SWINPUT_GPIO3_IN_5				88
#define SWINPUT_GPIO3_IN_6				89
#define SWINPUT_GPIO3_IN_7				90
#define SWINPUT_GPIO3_IN_8				91
#define SWINPUT_GPIO3_IN_9				92
#define SWINPUT_I2C3_SCL_IN				93
#define SWINPUT_I2C3_SDA_IN				94
#define SWINPUT_IPU_DISPB_D0_VSYNC		95
#define SWINPUT_IPU_DISPB_D12_VSYNC		96
#define SWINPUT_IPU_DISPB_SD_D			97
#define SWINPUT_IPU_SENSB_DATA_0		98
#define SWINPUT_IPU_SENSB_DATA_1		99
#define SWINPUT_IPU_SENSB_DATA_2		100
#define SWINPUT_IPU_SENSB_DATA_3		101
#define SWINPUT_IPU_SENSB_DATA_4		102
#define SWINPUT_IPU_SENSB_DATA_5		103
#define SWINPUT_IPU_SENSB_DATA_6		104
#define SWINPUT_IPU_SENSB_DATA_7		105
#define SWINPUT_KPP_COL_0				106
#define SWINPUT_KPP_COL_1				107
#define SWINPUT_KPP_COL_2				108
#define SWINPUT_KPP_COL_3				109
#define SWINPUT_KPP_COL_4				110
#define SWINPUT_KPP_COL_5				111
#define SWINPUT_KPP_COL_6				112
#define SWINPUT_KPP_COL_7				113
#define SWINPUT_KPP_ROW_0				114
#define SWINPUT_KPP_ROW_1				115
#define SWINPUT_KPP_ROW_2				116
#define SWINPUT_KPP_ROW_3				117
#define SWINPUT_KPP_ROW_4				118
#define SWINPUT_KPP_ROW_5				119
#define SWINPUT_KPP_ROW_6				120
#define SWINPUT_KPP_ROW_7				121
#define SWINPUT_OWIRE_BATTERY_LINE		122
#define SWINPUT_SPDIF_HCKT_CLK2			123
#define SWINPUT_SPDIF_SPDIF_IN1			124
#define SWINPUT_UART3_UART_RTS_B		125
#define SWINPUT_UART3_UART_RXD_MUX		126
#define SWINPUT_USB_OTG_DATA_0			127
#define SWINPUT_USB_OTG_DATA_1			128
#define SWINPUT_USB_OTG_DATA_2			129
#define SWINPUT_USB_OTG_DATA_3			130
#define SWINPUT_USB_OTG_DATA_4			131
#define SWINPUT_USB_OTG_DATA_5			132
#define SWINPUT_USB_OTG_DATA_6			133
#define SWINPUT_USB_OTG_DATA_7			134
#define SWINPUT_USB_OTG_DIR				135
#define SWINPUT_USB_OTG_NXT				136
#define SWINPUT_USB_UH2_DATA_0			137
#define SWINPUT_USB_UH2_DATA_1			138
#define SWINPUT_USB_UH2_DATA_2			139
#define SWINPUT_USB_UH2_DATA_3			140
#define SWINPUT_USB_UH2_DATA_4			141
#define SWINPUT_USB_UH2_DATA_5			142
#define SWINPUT_USB_UH2_DATA_6			143
#define SWINPUT_USB_UH2_DATA_7			144
#define SWINPUT_USB_UH2_DIR				145
#define SWINPUT_USB_UH2_NXT				146
#define SWINPUT_USB_UH2_USB_OC			147


/*
 * Function prototypes
 */
void pinmux_set_swmux(int pin, int mux_config);
void pinmux_set_padcfg(int pin, int pad_config);
void pinmux_set_input(int pin, int input_config);


#endif	/* __ARM_MX35_IOMUX_H_INCLUDED */
