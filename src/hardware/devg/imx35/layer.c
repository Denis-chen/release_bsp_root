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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <atomic.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/neutrino.h>
#include "imx35.h"

unsigned  imx35_formats[] = {
	DISP_LAYER_FORMAT_RGB565,
	DISP_LAYER_FORMAT_RGB888,
	DISP_LAYER_FORMAT_ARGB8888,
};

int
imx35_layer_query(disp_adapter_t * adapter, int dispno,
	int layer_idx, int fmt_idx, disp_layer_query_t *info_t)
{
	imx35_context_t		*imx35 = adapter->ms_ctx;
	disp_layer_query_t	info;

	if (layer_idx > 1 || fmt_idx > 2)
		return -1;
	
	memset(&info, 0, sizeof (info));

	info.size = sizeof(info);

	info.max_scaledown_x = info.max_scaleup_x = 1;
	info.max_scaledown_y = info.max_scaleup_y = 1;

	info.dst_max_width = imx35->xres;
	info.dst_max_height = imx35->yres;
	info.src_max_width = info.src_max_height = 4096;
	info.src_max_viewport_width = info.src_max_viewport_height = 4096;
	info.alpha_combinations = 0;

	info.pixel_format = imx35_formats[fmt_idx];

	if (layer_idx == 0) {
		info.caps = DISP_LAYER_CAP_PAN_SOURCE | DISP_LAYER_CAP_MAIN_DISPLAY;  
		info.alpha_valid_flags = 0; 
		info.chromakey_caps = 0;
		info.dst_min_height = imx35->yres; 
		info.dst_min_width = imx35->xres;
		info.order_mask = 1;
	} else {
		info.caps |=  DISP_LAYER_CAP_PAN_SOURCE | DISP_LAYER_CAP_SIZE_DEST | 
			DISP_LAYER_CAP_PAN_DEST | DISP_LAYER_CAP_DISABLE | 
			DISP_LAYER_CAP_ALPHA_WITH_CHROMA; 
		info.alpha_valid_flags = DISP_ALPHA_M1_GLOBAL | DISP_BLEND_SRC_M1_ALPHA | 
				DISP_BLEND_DST_ONE_MINUS_M1_ALPHA;
		info.chromakey_caps = DISP_LAYER_CHROMAKEY_CAP_SRC_SINGLE | 
			DISP_LAYER_CHROMAKEY_CAP_SHOWTHROUGH;
		info.dst_min_height = 1;
		info.dst_min_width = 16;
		info.order_mask = 2;
	}

	memcpy(info_t, &info, info.size);

	return 0;
}

int
imx35_layer_enable(disp_adapter_t * adapter, int dispno, int layer_idx)
{
	imx35_context_t *imx35 = adapter->ms_ctx;

	switch (layer_idx) {
		case 0:
			imx35->sdc_com |= 1 << 9;
			break;
		case 1:	
			imx35->sdc_com |= 1 << 4;
			break;
		default:	
			return -1;
	}

	imx35->layer[layer_idx].enable = 1;
	imx35->layer[layer_idx].change = 1;

	return 0;
}

int
imx35_layer_disable(disp_adapter_t * adapter, int dispno, int layer_idx)
{
	imx35_context_t *imx35 = adapter->ms_ctx;

	switch (layer_idx) {
		case 0:
			imx35->sdc_com &= ~(1 << 9);
			break;
		case 1:	
			imx35->sdc_com &= ~(1 << 4);
			break;
		default:	
			return -1;
	}
		
	imx35->layer[layer_idx].enable = 0;
	imx35->layer[layer_idx].change = 1;

	return 0;
}

int
imx35_layer_set_surface(disp_adapter_t * adapter, int dispno,
    int layer_idx, unsigned layer_format, disp_surface_t *surfaces[])
{
	imx35_context_t	*imx35 = adapter->ms_ctx;
        disp_surface_t	*surf = surfaces[0];
	int		oldbpp = imx35->layer[layer_idx].bpp;
	int		oldstride = imx35->layer[layer_idx].stride;

	if (layer_idx > 1) {
		return -1;
	}

	imx35->layer[layer_idx].next_surf = surf;
	imx35->layer[layer_idx].offset = surf->paddr;
	imx35->layer[layer_idx].stride = surf->stride;
	imx35->layer[layer_idx].layer_format = layer_format;
	switch(layer_format) {
		case DISP_LAYER_FORMAT_RGB888:
			imx35->layer[layer_idx].bpp = 3;
			break;
		case DISP_LAYER_FORMAT_ARGB8888:
			imx35->layer[layer_idx].bpp = 4;
			break;
		default:
			imx35->layer[layer_idx].bpp = 2;
			break;
	}

	if (imx35->layer[layer_idx].bpp != oldbpp ||
		imx35->layer[layer_idx].stride != oldstride) {
		imx35->layer[layer_idx].change = 1;
	}

	return 0;
}

int
imx35_layer_set_source_viewport(disp_adapter_t * adapter,
	int dispno, int layer_idx, int x1, int y1, int x2, int y2)
{
	imx35_context_t *imx35 = adapter->ms_ctx;

	if (layer_idx > 1)
		return -1;

	imx35->layer[layer_idx].src1.x = x1;
	imx35->layer[layer_idx].src1.y = y1;
	imx35->layer[layer_idx].src2.x = x2;
	imx35->layer[layer_idx].src2.y = y2;

	imx35->layer[layer_idx].change = 1;

	return 0;
}

int
imx35_layer_set_dest_viewport(disp_adapter_t * adapter,
	int dispno, int layer_idx, int x1, int y1, int x2, int y2)
{
	imx35_context_t	*imx35 = adapter->ms_ctx;


	if (layer_idx > 1)
		return -1;

        if ((x1 >= imx35->xres) || (y1 >= imx35->yres))
                return -1;

	if (x1 < 0) {
		x1 = 0;
	}
	if (y1 < 0) {
		y1 = 0;
	}

	if (x2 >= imx35->xres)
		x2 = imx35->xres - 1;

	if (y2 >= imx35->yres)
		y2 = imx35->yres - 1;

	imx35->layer[layer_idx].dst1.x = x1 & 0x7ff;
	imx35->layer[layer_idx].dst1.y = y1 & 0x7ff;
	imx35->layer[layer_idx].dst2.x = x2 & 0x7ff;
	imx35->layer[layer_idx].dst2.y = y2 & 0x7ff;

	if (x2 == x1)
		return -1;

	if (y2 == y1)
		return -1;

	imx35->layer[layer_idx].change = 1;

	return 0;
}

int
imx35_layer_set_blending(disp_adapter_t * adapter, int dispno,
	int layer_idx, unsigned mode, int m1, int m2)
{
	imx35_context_t	*imx35 = adapter->ms_ctx;

	if (layer_idx != 1)
		return -1;

        if (mode == 0) {
                /* We need to turn on global blending, but set alpha value to full */
		imx35->sdc_com |= 0x40;
		imx35->layer[layer_idx].alpha = 0xff;
        } else if (DISP_ALPHA_M1_ORIGIN(mode) == DISP_ALPHA_M1_GLOBAL) {
		imx35->sdc_com |= 0x40;
		imx35->layer[layer_idx].alpha = m1;
        } else if (DISP_ALPHA_M1_ORIGIN(mode) == DISP_ALPHA_M1_SRC_PIXEL_ALPHA) {
		imx35->sdc_com &= ~0x40;
        } else 
		return -1;
	
	return 0;
}

int
imx35_layer_set_chromakey(disp_adapter_t * adapter, int dispno,
	int layer_idx, unsigned chroma_mode,
	disp_color_t color0, disp_color_t color1, disp_color_t mask)
{
	imx35_context_t	*imx35 = adapter->ms_ctx;

	if (layer_idx != 1)
		return -1;

        if (chroma_mode == 0) {
                /* Disable chroma */
		imx35->sdc_com &= ~0x80;
                return 0;
        }

        if (!(chroma_mode & DISP_CHROMA_OP_SRC_MATCH))
                return -1;

        if (chroma_mode & DISP_CHROMA_OP_DRAW)
                return -1;

        imx35->layer[layer_idx].chroma_color = color0;
	imx35->sdc_com |= 0x80;

	return 0;
}

void
imx35_layer_update_begin(disp_adapter_t *adapter,
   int dispno, uint64_t layer_mask)
{
}

void
imx35_channel_setup(imx35_context_t *imx35, int layer_idx)
{
	uint32_t	*params;
	uint32_t	dma_chan;
	uint32_t	dma_addr; 
	layer_info	*linfo = &imx35->layer[layer_idx];
	int 		width = linfo->dst2.x - linfo->dst1.x + 1;
	int 		height = linfo->dst2.y - linfo->dst1.y + 1;

	dma_chan = layer_idx + 14;
	dma_addr = 0x10000 | (dma_chan << 4);
	params = linfo->params;

	/*
	 * Use spin lock to avoid partial updates (some parameters updated
	 * by ISR in one vsync cycle, and the rest updated in the next).
	 */
	InterruptLock(&imx35->spinlock);

	params[0] = ((width - 1) << 12) | ((height - 1) << 24);
	params[1] = (height - 1) >> 8;
	params[2] = (linfo->offset + linfo->src1.y * linfo->stride +
                linfo->bpp * linfo->src1.x) & ~0x1f;
	params[3] = (linfo->stride - 1) << 3;

	switch (imx35->layer[layer_idx].layer_format) {
		case DISP_LAYER_FORMAT_RGB565:
			params[3] |= 2 | (4 << 17) | (0xf << 25); 
			params[4] = 2 | (0 << 3) | (5 << 8) | 
				(11 << 13) | (16 << 18) | (4 << 23) | 
				(5 << 26) | (4 << 29); 
			params[5] = 5;
			break;
		case DISP_LAYER_FORMAT_RGB888:
			params[3] |= 1 | (4 << 17) | (7 << 25); 
			params[4] = 2 | (0 << 3) | (8 << 8) |
				(16 << 13) | (24 << 18) | (7 << 23) |
				(7 << 26) | (7 << 29);
			params[5] = 7;
			break;
		case DISP_LAYER_FORMAT_ARGB8888:
			params[3] |= 0 | (4 << 17) | (7 << 25); 
			params[4] = 2 | (8 << 3) | (16 << 8) |
				(24 << 13) | (0 << 18) | (7 << 23) |
				(7 << 26) | (7 << 29);
			params[5] = 7;
			break;
	}

	InterruptUnlock(&imx35->spinlock);
		
	linfo->change = 0;

}

void
imx35_layer_program(imx35_context_t *imx35, int layer_idx, int wait_vsync)
{
	layer_info	*linfo = &imx35->layer[layer_idx];

	if (layer_idx == 0) { 
		*SDC_BG_POS = (imx35->layer[0].dst1.x + imx35->hss) << 16 | imx35->layer[0].dst1.y + imx35->vss;
	} else { 
		*SDC_FG_POS = (imx35->layer[1].dst1.x + imx35->hss) << 16 | imx35->layer[1].dst1.y + imx35->vss;
		*SDC_GRAPH_WIND_CTRL = imx35->layer[1].alpha << 24 | imx35->layer[1].chroma_color; 
	}

	if (linfo->change) {
		/* Full update */
		imx35_channel_setup(imx35, layer_idx);
		atomic_set(&linfo->update_flags, 3);
	} else {
		/* Offset change only */
		linfo->commit_offset = (linfo->offset + linfo->src1.y * linfo->stride +
                	linfo->bpp * linfo->src1.x) & ~0x1f;
		atomic_set(&linfo->update_flags, 1);
	}
}

void
imx35_layer_update_end(disp_adapter_t *adapter,
    int dispno, uint64_t layer_mask, int wait_vsync)
{
	imx35_context_t		*imx35 = adapter->ms_ctx;
	int			i;

	if (layer_mask & (1 << 1)) {
		if (imx35->layer[1].enable && !imx35->layer[0].enable) {
			imx35_layer_reset(adapter, dispno, 0);
			imx35_layer_enable(adapter, dispno, 0);
			imx35_layer_program(imx35, 0, wait_vsync);
		} 
	} 

	for (i = 0; i < 2; i++) {
		if (layer_mask & (1<<i)) {
			imx35_layer_program(imx35,i, wait_vsync);
		}
	}

	if (*SDC_COM_CONF != imx35->sdc_com) {
		*SDC_COM_CONF = imx35->sdc_com;
	}

	if (!imx35->panel_on) {
		*IPU_CONF |= 0x50;       /* enable DI & SDC */
		delay(30);		/* to prevent flash at initial power on */
		*SDC_CUR_BLINK_PWM_CTRL = 0x03000000 | imx35->brightness << 16;   
		imx35->panel_on |= 1;
        }

	if (wait_vsync) {
		adapter->callback(adapter->callback_handle,
		    DISP_CALLBACK_WAIT_VSYNC, &dispno);
	}	
}

void
imx35_layer_reset(disp_adapter_t *adapter, int dispno, int layer_idx)
{
	imx35_context_t	*imx35 = adapter->ms_ctx;
	
	imx35->layer[layer_idx].change = 1;

	if (layer_idx == 1) {
		imx35->sdc_com &= ~0x80;	/* disable chroma */

		/*
		 * Since alphablending is always enabled, either per-pixel
	         * or global, for no blending we turn on global and set the
		 * pixels to opaque.
		 */
		imx35->sdc_com |= 0x40;
		imx35->layer[layer_idx].alpha = 0xff;
	} else {
		imx35_layer_set_source_viewport(adapter, 0,
	 		0, 0, 0, imx35->xres-1, imx35->yres-1);
		imx35_layer_set_dest_viewport(adapter, 0,
	 		0, 0, 0, imx35->xres-1, imx35->yres-1);
		imx35->layer[layer_idx].offset = imx35->zero_base;
		imx35->layer[layer_idx].stride = imx35->xres * 2;
		imx35->layer[layer_idx].layer_format = DISP_LAYER_FORMAT_RGB565;
		imx35->layer[layer_idx].bpp = 2;
	}
}


