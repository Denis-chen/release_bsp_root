/*
 * $QNXLicenseC: 
 * Copyright 2007 - 2009, QNX Software Systems.  
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


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "imx35.h"

static char *imx35_opts[] = {
#define I_WIDTH		0
			"xres",
#define I_HEIGHSS	1
			"yres",
#define I_REFRESH	2
			"refresh",
#define I_IRQ		3	
			"irq",
#define I_HSW		4
			"hsw",
#define I_HSS		5
			"hss",
#define I_HEW		6
			"hew",
#define I_VSW		7
			"vsw",
#define I_VSS		8
			"vss",
#define I_VEW		9
			"vew",
#define I_DMASK		10
			"dmask",
#define I_CLK_IDLE	11
			"cidle",
#define I_CLK_SEL	12
			"csel",
#define I_VPOL		13
			"vpol",
#define I_EPOL		14
			"epol",
#define I_DPOL		15
			"dpol",
#define I_CPOL		16
			"cpol",
#define I_HPOL		17
			"hpol",
#define I_SDC		18
			"sdc",
#define I_SHARP1	19
			"sharp1",
#define I_SHARP2	20
			"sharp2",
#define I_OUTFMT	21
			"ofmt",
#define I_BRIGHT	22
			"bright",
#define I_CLK_D		23
			"clkdwn",
#define I_CLK_U		24
			"clkup",
#define I_CLK_P		25
			"clkper",
#define I_VIDBASE	26
			"vidbase",
#define I_VIDSIZE	27
			"vidsize",
#define I_HW2DACCEL	28
			"hw2daccel",
#define I_OVGLIBNAME 29
			"ovgLibName",
#define I_CSI		30
			"csi",
#define I_CST_TEST	31
			"csitest",	
#define I_CCIR1		32
			"ccir1",
#define I_CCIR2		33
			"ccir2",
#define I_CCIR3		34
			"ccir3",
			NULL
};

/*
 * Get configuration information.  Info from the .conf file overrides
 * info retrieved from the resource database.
 */
int
get_config_data(imx35_context_t *imx35, const char *filename)
{
	FILE		*fin = NULL;
	char		buf[384], *c, *opt, *value;

	if (filename == NULL) {
		disp_printf(imx35->adapter,
		    "devg-imx35: No config file specified, "
		    "using default parameters");
	} else if ((fin = fopen(filename, "r")) == NULL) {
		disp_printf(imx35->adapter,
		    "Could not open config file \"%s\": %s",
		    filename, strerror(errno));
	}

	/* default - CLAA070VC01 800x480 */
	imx35->xres = 800;
	imx35->yres = 480;
	imx35->refresh = 60;
	imx35->vsw = 2;
	imx35->vss = 10;
	imx35->vew = 10;
	imx35->hsw = 5;
	imx35->hss = 50;
	imx35->hew = 50;
	imx35->data_mask = 0;
	imx35->clk_idle = 0;
	imx35->clk_sel = 0;
	imx35->vsync_pol = 0;
	imx35->enable_pol = 1;
	imx35->data_pol = 0;
	imx35->clock_pol = 0;
	imx35->hsync_pol = 0;
	imx35->sdc_mode = 1;
	imx35->sharp_conf1 = 0;
	imx35->sharp_conf2 = 0;
	imx35->output_fmt = 18;
	imx35->irq = 42;
	imx35->brightness = 0xff;
	imx35->vidbase = 0;
	imx35->vidsize = 0;
	imx35->hw2daccel = 1;
	imx35->csi_sens_conf = 1 << 7 | 2 << 8 | 1 << 10 | 4 << 16 | 3 << 4;
	imx35->ccir3 = 0xff0000;
	imx35->ccir1 = 6 | 2 << 3 | 6 << 6 | 2 << 9 | 4 << 16;
	imx35->ccir2 = 7 | 3 << 3 | 7 << 6 | 3 << 9 | 5 << 16 | 1 << 19;
	strcpy(imx35->ovgLibName, "libOpenVG-G12.so.1");

	if (fin == NULL) {
		/* No config file, use what we have */
		return 0;
	}

	while (fgets(buf, sizeof (buf), fin) != NULL) {
		c = buf;
		while (*c == ' ' || *c == '\t')
			c++;
		if (*c == '\015' || *c== '\032' || *c == '\0' || *c == '\n' || *c == '#')
			continue;
		opt = c;
		while (*c != '\015' && *c!= '\032' && *c != '\0' && *c != '\n' && *c != '#')
			c++;
		*c = '\0';
		break;
	}

	while (*opt != '\0') {
		c = opt;

		switch (getsubopt(&opt, imx35_opts, &value)) {
		case I_WIDTH:
			imx35->xres = strtoul(value, 0, 0);
			break;
		case I_HEIGHSS:
			imx35->yres = strtoul(value, 0, 0);
			break;
		case I_REFRESH:
			imx35->refresh = strtoul(value, 0, 0);
			break;
		case I_IRQ:
			imx35->irq = strtoul(value, 0, 0);
			break;
		case I_HSW:
			imx35->hsw = strtoul(value, 0, 0);
			break;
		case I_HSS:
			imx35->hss = strtoul(value, 0, 0);
			break;
		case I_HEW:
			imx35->hew = strtoul(value, 0, 0);
			break;
		case I_VSW:
			imx35->vsw = strtoul(value, 0, 0);
			break;
		case I_VSS:
			imx35->vss = strtoul(value, 0, 0);
			break;
		case I_VEW:
			imx35->vew = strtoul(value, 0, 0);
			break;
		case I_DMASK:
			imx35->data_mask = strtoul(value, 0, 0);
			break;
		case I_CLK_IDLE:
			imx35->clk_idle = strtoul(value, 0, 0);
			break;
		case I_CLK_SEL:
			imx35->clk_sel = strtoul(value, 0, 0);
			break;
		case I_VPOL:
			imx35->vsync_pol = strtoul(value, 0, 0);
			break;
		case I_EPOL:
			imx35->enable_pol = strtoul(value, 0, 0);
			break;
		case I_DPOL:
			imx35->data_pol = strtoul(value, 0, 0);
			break;
		case I_CPOL:
			imx35->clock_pol = strtoul(value, 0, 0);
			break;
		case I_HPOL:
			imx35->hsync_pol = strtoul(value, 0, 0);
			break;
		case I_SDC:
			imx35->sdc_mode = strtoul(value, 0, 0);
			break;
		case I_SHARP1:
			imx35->sharp_conf1 = strtoul(value, 0, 0);
			break;
		case I_SHARP2:
			imx35->sharp_conf2 = strtoul(value, 0, 0);
			break;
		case I_OUTFMT:
			imx35->output_fmt = strtoul(value, 0, 0);
			break;
		case I_BRIGHT:
			imx35->brightness = strtoul(value, 0, 0);
			break;
		case I_CLK_D:
			imx35->clk_down_wr = strtoul(value, 0, 0);
			break;
		case I_CLK_U:
			imx35->clk_up_wr = strtoul(value, 0, 0);
			break;
		case I_CLK_P:
			imx35->clk_per_wr = strtoul(value, 0, 0);
			break;
		case I_VIDBASE:
			imx35->vidbase = strtoul(value, 0, 0);
			break;
		case I_VIDSIZE:
			imx35->vidsize = strtoul(value, 0, 0);
			break;
		case I_HW2DACCEL:
			imx35->hw2daccel = strtol(value, 0, 0);
			disp_printf(imx35->adapter, "devg-imx35: 2D HW accel: %d", imx35->hw2daccel);
			break;
		case I_OVGLIBNAME:
			if (strlen(value) >= sizeof(imx35->ovgLibName))
				disp_printf(imx35->adapter, "devg-imx35: ovg lib name (%s) too long, using default (%s)", value, imx35->ovgLibName);
			else
			{
				strcpy(imx35->ovgLibName, value);
				disp_printf(imx35->adapter, "devg-imx35: ovg lib: using %s", imx35->ovgLibName);
			}
			break;
		case I_CSI:
			imx35->csi_sens_conf = strtoul(value, 0, 0);
			break;
		case I_CST_TEST:
			imx35->cap_test = 1;
			break;
		case I_CCIR1:
			imx35->ccir1 = strtoul(value, 0, 0);
			break;
		case I_CCIR2:
			imx35->ccir1 = strtoul(value, 0, 0);
			break;
		case I_CCIR3:
			imx35->ccir1 = strtoul(value, 0, 0);
			break;
		default:
			disp_printf(imx35->adapter, "Unknown option %s", c);
			break;
		}
	}

	fclose(fin);

	return 0;
}


