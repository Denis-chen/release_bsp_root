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
 * i.MX35 3DS Development Board with ARM1136J-FS core
 */

#include "startup.h"
#include <arm/mx35.h>

/*
 * GPIO/MUX stuff
 */
extern void mx35_3ds_init_ata(void);
extern void mx35_3ds_init_spi(void);
extern void mx35_3ds_init_i2c(void);
extern void mx35_3ds_init_usbh2(void);
extern void mx35_3ds_init_usbotg_utmi(void);
extern void mx35_3ds_init_nand(void);
extern void mx35_3ds_init_can(void);
extern void mx35_3ds_init_lcd(void);
extern void mx35_3ds_init_fec(void);
extern void mx35_3ds_init_lan(void);

/* touchscreen init */
extern void mx35_3ds_init_ts_mc13892(void);
extern void mx35_3ds_init_ts_tsc2007(void);

/* audio init */
extern void init_audio (void);
  
/*
 * Clocking and timekeeping stuff
 */
extern void mx35_calc_clocks(unsigned ckih);

extern uint64_t mx35_arm_clock;

extern void init_qtime_mx31(void);

/*
 * Callout related stuff
 */
extern struct callout_rtn reboot_mx31;

const struct callout_slot callouts[] = {
	{ CALLOUT_SLOT( reboot, _mx31) },
};

/*
 * The i.MX35 3DS UART baud clock source is derived from the peripheral PLL
 * (UPLL). The UPLL output frequency is 300 MHz and the UART baud clock
 * divider is set to 3, for a UART baud clock of 100 MHz.
 */
const struct debug_device debug_devices[] = {
	{ 	"mx1",
		{	"0x43F90000^0.115200.100000000.16",
		},
		init_mx1,
		put_mx1,
		{	&display_char_mx1,
			&poll_key_mx1,
			&break_detect_mx1,
		}
	},
};

#define iMX35_AIPS1_OPACR1				(*(volatile uint32_t*)0x43f00040)
#define iMX35_AIPS1_OPACR2				(*(volatile uint32_t*)0x43f00044)
#define iMX35_AIPS1_OPACR3				(*(volatile uint32_t*)0x43f00048)
#define iMX35_AIPS1_OPACR4				(*(volatile uint32_t*)0x43f0004c)
#define iMX35_AIPS1_OPACR5				(*(volatile uint32_t*)0x43f00050)
#define iMX35_AIPS2_OPACR1				(*(volatile uint32_t*)0x53f00040)
#define iMX35_AIPS2_OPACR2				(*(volatile uint32_t*)0x53f00044)
#define iMX35_AIPS2_OPACR3				(*(volatile uint32_t*)0x53f00048)
#define iMX35_AIPS2_OPACR4				(*(volatile uint32_t*)0x53f0004c)
#define iMX35_AIPS2_OPACR5				(*(volatile uint32_t*)0x53f00050)

static void mx35_SDMA_init()
{
	/*
	 * Clear the Off-platform Peripheral Modules
	 * Supervisor Protect bit for SDMA access.
	 */
	iMX35_AIPS1_OPACR1 = 0x0;
	iMX35_AIPS1_OPACR2 = 0x0;
	iMX35_AIPS1_OPACR3 = 0x0;
	iMX35_AIPS1_OPACR4 = 0x0;
	iMX35_AIPS1_OPACR5 &= 0x00FFFFFF;
	iMX35_AIPS2_OPACR1 = 0x0;
	iMX35_AIPS2_OPACR2 = 0x0;
	iMX35_AIPS2_OPACR3 = 0x0;
	iMX35_AIPS2_OPACR4 = 0x0;
	iMX35_AIPS2_OPACR5 &= 0x00FFFFFF;
}

#define	MX35ADS_CFG_ATA		(1 << 0)
#define	MX35ADS_CFG_SPI		(1 << 1)
#define	MX35ADS_CFG_HC1		(1 << 2)
#define	MX35ADS_CFG_HC2		(1 << 3)
#define	MX35ADS_CFG_I2C		(1 << 4)
#define	MX35ADS_CFG_LCD		(1 << 5)
#define	MX35ADS_CFG_NND		(1 << 6)
#define	MX35ADS_CFG_FEC		(1 << 7)
#define	MX35ADS_CFG_CAN		(1 << 8)
#define	MX35ADS_CFG_TSC2007	(1 << 9)

/*
 * main()
 *	Startup program executing out of RAM
 *
 * 1. It gathers information about the system and places it in a structure
 *    called the system page. The kernel references this structure to
 *    determine everything it needs to know about the system. This structure
 *    is also available to user programs (read only if protection is on)
 *    via _syspage->.
 *
 * 2. It (optionally) turns on the MMU and starts the next program
 *    in the image file system.
 */
int
main(int argc, char **argv, char **envv)
{
	int			opt;
	unsigned	cfg = 0;
	int			l2_enable = 1;

	add_callout_array(callouts, sizeof(callouts));

	while ((opt = getopt(argc, argv,
	    COMMON_OPTIONS_STRING "ascLU:nECl:")) != -1) {
		switch (opt) {
			case 'a':
				cfg |= MX35ADS_CFG_ATA;
				break;
			case 's':
				cfg |= MX35ADS_CFG_SPI;
				break;
			case 'c':
				cfg |= MX35ADS_CFG_I2C;
				break;
			case 'L':
				cfg |= MX35ADS_CFG_LCD;
				break;
			case 'U':
				if (getsize(optarg, NULL) == 2)
					cfg |= MX35ADS_CFG_HC2;
				else
					cfg |= MX35ADS_CFG_HC1;
				break;
			case 'n':
				cfg |= MX35ADS_CFG_NND;
				break;
			case 'E':
				cfg |= MX35ADS_CFG_FEC;
				break;
			case 'C':
				cfg |= MX35ADS_CFG_CAN;
				break;
			case 'l':
				switch (optarg[0]) {
					case 'e':
						l2_enable = 1;
						break;
					case 'd':
						l2_enable = 0;
						break;
					default:
						break;
				}
				break;
			case 't':
				cfg |= MX35ADS_CFG_TSC2007;
				break;
			default:
				handle_common_option(opt);
				break;
		}
	}

	/*
	 * Initialise debugging output
	 */
	select_debug(debug_devices, sizeof(debug_devices));

	/*
	 * Collect information on all free RAM in the system
	 */
	init_raminfo();

	/*
	 * Set CPU frequency
	 */
	if (cpu_freq == 0) {
		mx35_calc_clocks(24000000);
		cpu_freq = mx35_arm_clock;
	}

	/*
	 * Remove RAM used by modules in the image
	 */
	alloc_ram(shdr->ram_paddr, shdr->ram_size, 1);

	if (shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL)
		init_mmu();

	init_intrinfo();
	init_qtime_mx31();
	init_cacheattr_mx3x(l2_enable);
	mx35_SDMA_init();
	init_cpuinfo();
	init_hwinfo();
	init_audio();

	add_typed_string(_CS_MACHINE, "i.MX35 3DS");

	if (cfg & MX35ADS_CFG_HC1)
		mx35_3ds_init_usbotg_utmi();
	else if (cfg & MX35ADS_CFG_HC2)
		mx35_3ds_init_usbh2();

	if (cfg & MX35ADS_CFG_ATA)
		mx35_3ds_init_ata();

	if (cfg & MX35ADS_CFG_SPI)
		mx35_3ds_init_spi();

	if (cfg & MX35ADS_CFG_I2C)
		mx35_3ds_init_i2c();

	if (cfg & MX35ADS_CFG_NND)
		mx35_3ds_init_nand();

	if (cfg & MX35ADS_CFG_LCD)
		mx35_3ds_init_lcd();
	
	if (cfg & MX35ADS_CFG_FEC)
		mx35_3ds_init_fec();
	
	if (cfg & MX35ADS_CFG_CAN)
		mx35_3ds_init_can();


	/* Early mx35_3ds used the TSC2007 touchscreen later pdk versions have a power management 
	   chip MC13892 for the touchscreen.
	   Look for MC13892 chip on the CPU board
	*/
	if (cfg & MX35ADS_CFG_TSC2007) {
		mx35_3ds_init_ts_tsc2007();
	} else {
		mx35_3ds_init_ts_mc13892();
	}

	/*
	 * Load bootstrap executables in the image file system and Initialise
	 * various syspage pointers. This must be the _last_ initialisation done
	 * before transferring control to the next program.
	 */
	init_system_private();

	/*
	 * This is handy for debugging a new version of the startup program.
	 * Commenting this line out will save a great deal of code.
	 */
	print_syspage();

	return 0;
}
