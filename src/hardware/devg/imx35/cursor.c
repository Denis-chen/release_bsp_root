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



#include <string.h>

#include "imx35.h"

int
imx35_set_hw_cursor(disp_adapter_t *adapter, int dispno, 
	uint8_t * bmp0, uint8_t * bmp1, unsigned color0, unsigned color1,
	int hotspot_x, int hotspot_y, int size_x, int size_y, int bmp_stride)
{
	imx35_context_t 	*imx35 = adapter->ms_ctx;

	/* use sw cursor */
	if (size_x > 32 || size_y > 32)
		return -1;

	*SDC_CUR_MAP = color0;

	imx35->cur_w = size_x;
	imx35->cur_h = size_y;
	imx35->hs_x = hotspot_x;
	imx35->hs_y = hotspot_y;

	return 0;
}


void
imx35_enable_hw_cursor(disp_adapter_t *adapter, int dispno)
{
	imx35_context_t 	*imx35 = adapter->ms_ctx;

	*SDC_COM_CONF |= 1 << 16;
	imx35->sdc_com |= 1 << 16;
}


void
imx35_disable_hw_cursor(disp_adapter_t *adapter, int dispno)
{
	imx35_context_t *imx35 = adapter->ms_ctx;

	*SDC_COM_CONF &= ~0x70000;
	imx35->sdc_com &= ~(0x70000);
}


void
imx35_set_hw_cursor_pos(disp_adapter_t *adapter, int dispno, int x, int y)
{
	imx35_context_t 	*imx35 = adapter->ms_ctx;

	x += imx35->hss;
	y += imx35->vss;

	x -= imx35->hs_x;
	y -= imx35->hs_y;

	if (x < 0) {
		x = 0;
	}
	if (y < 0) {
		y = 0;
	}

	*SDC_CUR_POS = imx35->cur_w-1<< 26 | x << 16 | imx35->cur_h-1 << 10 | y;
}


