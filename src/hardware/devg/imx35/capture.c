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
#include <unistd.h>
#include <atomic.h>
#include "imx35.h"

static int
_calc_resize_coeffs(uint32_t inSize, uint32_t outSize, uint32_t * resizeCoeff, uint32_t * downsizeCoeff)
{
	uint32_t tempSize;
	uint32_t tempDownsize;

	/* Cannot downsize more than 8:1 */
	if ((outSize << 3) < inSize) {
		outSize = inSize >> 3;
	}

	/* compute downsizing coefficient */
	tempDownsize = 0;
	tempSize = inSize;

	while ((tempSize >= outSize*2) && (tempDownsize < 2)) {
		tempSize >>= 1;
		tempDownsize++;
	}
	
	*downsizeCoeff = tempDownsize;

	*resizeCoeff = (8192L * (tempSize - 1)) / (outSize - 1);

	if (*resizeCoeff >= 16384L) {
		*resizeCoeff = 0x3FFF;
	}

	return 0;
}


static void
set_layer_surfaces(imx35_context_t *imx35)
{
	if (imx35->capture_bound != -1) {
		layer_info      *linfo = &imx35->layer[imx35->capture_bound];

		linfo->commit_offset = imx35->cap_offset;
		linfo->offset = imx35->cap_offset; 
		linfo->stride = imx35->cap_stride;
		linfo->layer_format = DISP_LAYER_FORMAT_RGB565; 
		linfo->bpp    = 2;

		atomic_set(&linfo->update_flags, 3);
	}
}


static int
realloc_capture_buffer(imx35_context_t *imx35,
    int engine, int width, int height)
{
	int		stride;
	uint32_t	dma_addr = 0x10000 | (1 << 4);


	if (imx35->capture_buf != NULL) {
		if (width == imx35->capture_buf_width &&
			height == imx35->capture_buf_height) {
			return 0;
		} 

		disp_freemem(imx35->capture_buf, imx35->capture_buf_size);
	}

	stride = ((width*2) + 0x1f) & ~0x1f;
	imx35->capture_buf_size = (height * stride) + 0x1F;
	if ((imx35->capture_buf = disp_getmem(imx35->capture_buf_size,
		PROT_READ|PROT_WRITE|PROT_NOCACHE, DISP_MAP_PHYS)) == NULL) {
		disp_printf(imx35->adapter, "Failed to allocate capture buffers");
		return -1;
	}

	imx35->capture_buf_width = width;
	imx35->capture_buf_height = height;
	imx35->cap_stride = stride;
	imx35->cap_offset = (disp_phys_addr(imx35->capture_buf) + 0x1f) & ~0x1f;

	*IPU_IMA_ADDR = dma_addr+0;
	*IPU_IMA_DATA = 0;
	*IPU_IMA_ADDR = dma_addr+1;
	*IPU_IMA_DATA = 0x4000;
	*IPU_IMA_ADDR = dma_addr+2;
	*IPU_IMA_DATA = 0;
	*IPU_IMA_ADDR = dma_addr+3;
	*IPU_IMA_DATA = ((width - 1) << 12) | ((height - 1) << 24);
	*IPU_IMA_ADDR = dma_addr+4;
	*IPU_IMA_DATA = (height - 1) >> 8;
	*IPU_IMA_ADDR = dma_addr+8;
	*IPU_IMA_DATA = imx35->cap_offset;
	*IPU_IMA_ADDR = dma_addr+9;
	*IPU_IMA_DATA = 0;
	*IPU_IMA_ADDR = dma_addr+10;
	*IPU_IMA_DATA = (stride - 1) << 3 | 2 | (4 << 17) | (0xf << 25);
	*IPU_IMA_ADDR = dma_addr+11;
	*IPU_IMA_DATA = 2 | (0 << 3) | (5 << 8) |
		(11 << 13) | (16 << 18) | (4 << 23) |
		(5 << 26) | (4 << 29);
	*IPU_IMA_ADDR = dma_addr+12;
	*IPU_IMA_DATA = 0;

	if (imx35->capture_enabled) {
		*IPU_CONF |= 0x3;
		*IPU_CHA_BUF0_RDY |=2;
	}

	set_layer_surfaces(imx35);

	return 0;
}

static void
disable_unit(imx35_context_t *imx35)
{
	int i=100000;

	if (*IDMAC_CHA_EN & 2) {
		while (*IDMAC_CHA_BUSY & (1 << 1)) {
			nanospin_ns(500);
			if (i-- == 0) {
				disp_printf(imx35->adapter,"Capture DMA lockup (disable_unit)");
				break;
			}
		}

		*IPU_CONF &= ~0x3;
		*IDMAC_CHA_EN &= ~2;
		*IPU_CHA_BUF0_RDY &= ~2;
	}
}

static void
set_enabled(imx35_context_t *imx35, int engine, int enable)
{
	if (enable == imx35->capture_enabled) {
		return;
	}

	if (enable) {
		if (realloc_capture_buffer(imx35, engine,
			imx35->capture_buf_width, imx35->capture_buf_height) != -1) {
			*IPU_CONF |= 0x3;
			*IDMAC_CHA_EN |= 2;
			*IPU_CHA_BUF0_RDY |=2;
		}
	} else {
		disable_unit(imx35);

		if (imx35->capture_buf != NULL) {
			disp_freemem(imx35->capture_buf,
			imx35->capture_buf_size);
			imx35->capture_buf = NULL;
		}
	}

	imx35->capture_enabled = enable;
}


int 
imx35_vcap_set_props(disp_adapter_t *adapter,
    int unit, disp_vcap_props_t *props)
{
	imx35_context_t	*imx35 = adapter->ms_ctx;
	int		x1, y1, x2, y2;
	int		cap_width, cap_height;
	int		dst_width, dst_height;
	uint32_t	frm_ctrl = 0;
	uint32_t	resize=0, down_coeff, resize_coeff;
	int		hskip;
	int		i=1000000;

	if (unit != 0) {
		return -1;
	}

	if (props->input_source != 0) {
		return -1;
	}

	x1 = props->src_x1;
	y1 = props->src_y1;
	x2 = props->src_x2;
	y2 = props->src_y2;
	
	cap_width = x2-x1+1;
	cap_height = y2-y1+1;

	if (cap_width < 1 || cap_height < 1) {
		disp_printf(adapter,"Invalid capture size");
		return -1;
	}
	
	dst_width = props->dst_width;
	dst_height = props->dst_height;
	hskip = ((x1 + 1) & ~1) & 0xff;
	
	if (dst_width == 0 || dst_height == 0 ||
		props->src_width == 0 || props->src_height == 0 ||
		cap_width == 0 || cap_height == 0) {
		set_enabled(imx35, unit, 0);
		return 0;
	}

	/* wait for EOF and disable capture; updating CSI with it enable has shown to cause lockups */
	if (imx35->capture_enabled == 1) {
		while (*IDMAC_CHA_BUSY & (1 << 1)) {
			nanospin_ns(500);
			if (i-- == 0) {
				disp_printf(adapter,"Capture DMA Lockup (vcap_set_props)");
				return -1;
			}
		}
		*IPU_CONF &= ~0x3;
		*IPU_CHA_BUF0_RDY &= ~2;
	}

	*CSI_SENS_CONF = imx35->csi_sens_conf;

	if (imx35->cap_test) {
		*CSI_SENS_CONF &= ~0x330;
		*CSI_SENS_CONF |= 0x10;
		*CSI_TST_CTRL = 0x0100FF00;
	} else {
		*CSI_TST_CTRL = 0;
	}

	*CSI_SENS_FRM_SIZE = (props->src_height - 1) << 16 | (props->src_width-1);
	*CSI_ACT_FRM_SIZE = (cap_height-1) << 16 | (cap_width-1);

	*CSI_CCIR_CODE_3 = imx35->ccir3;
	*CSI_CCIR_CODE_1 = imx35->ccir1;
	*CSI_CCIR_CODE_2 = imx35->ccir2;

	frm_ctrl = (hskip << 8) | (y1 & 0xff);
	if ((cap_width / dst_width) >= 2) {
		frm_ctrl |=  1 << 29;
		cap_width /= 2;
	}
	if ((cap_height / dst_height) >= 2) {
		frm_ctrl |=  1 << 28;
		cap_height /= 2;
	}

	*CSI_OUT_FRM_CTRL = frm_ctrl;   

	if (((imx35->csi_sens_conf & CSI_FORMAT_MASK)>>8) == 2) {
		*IC_CONF = 1 << 8 | 1 << 9 | 1 << 28 | 1 << 31;
	} else {
		*IC_CONF = 1 << 8 | 0 << 9 | 1 << 28 | 1 << 31;
	}
	
	_calc_resize_coeffs(cap_width, dst_width, &resize_coeff, &down_coeff);
	resize = (down_coeff << 14) | resize_coeff;
	_calc_resize_coeffs(cap_height, dst_height, &resize_coeff, &down_coeff);
	resize |= (down_coeff << 30) | (resize_coeff << 16);
	*IC_PRP_VF_RSC = resize;

	if (realloc_capture_buffer(imx35, unit, dst_width, dst_height) == -1) {
		return -1;
	}

	if (props->flags & DISP_VCAP_FLAG_EVEN_FRAMES_ONLY) {
		/* turn on double buffering but only use 1 buffer; application must pass in 1/2 the height */
		*IPU_CHA_DB_MODE_SEL |= 1 << 1;
	} else {
		*IPU_CHA_DB_MODE_SEL &= ~(1 << 1);
	}

	if (imx35->capture_enableme) {
		set_enabled(imx35, unit, 1);
		imx35->capture_enableme = 0;
	}

	imx35->capture_configured = 1;

	return 0;
}

int
imx35_vcap_set_enabled(disp_adapter_t *adapter, int unit, int enabled)
{
	imx35_context_t *imx35 = adapter->ms_ctx;

	if (unit != 0) {
		return -1;
	}

	if (enabled && !imx35->capture_enabled) {
		if (imx35->capture_configured) {
			set_enabled(imx35, unit, 1);
		} else {
			imx35->capture_enableme = 1;
		}
	} else if (!enabled && imx35->capture_enabled) {
		set_enabled(imx35, unit, 0);
	}

	return 0;
}

int
imx35_vcap_init(disp_adapter_t *adapter, char *optstring)
{
	imx35_context_t *imx35 = adapter->ms_ctx;
	uint32_t        params[2];
	uint32_t        address =0;

	imx35->capture_enabled = 0;
	imx35->capture_configured = 0;
	imx35->capture_bound = -1;

	if (*IDMAC_CHA_EN & 2){
		disable_unit(imx35);
	}

	*IPU_CHA_CUR_BUF &= ~(1 << 1);

	*IC_CMBP_1 = 0xff| 0xff << 8;  

	if (((imx35->csi_sens_conf & CSI_FORMAT_MASK)>>8) == 2) {
		/* YUV422 to RGB color conversion */
		address = 0x645 << 3;
		params[0] = 0 | ((512-23) << 9) | 128 << 18 | ((8192-410)<< 27);
		params[1] = ((8192 - 410) >> 5) | 2 << 8;
		*IPU_IMA_ADDR = address;
		*IPU_IMA_DATA = params[0];
		*IPU_IMA_ADDR = address+1;
		*IPU_IMA_DATA = params[1];
		address = 0x646 << 3;
		params[0] = 128 | 128 << 9 | 0 << 18 | 169 << 27;
		params[1] = 169 >> 5;
		*IPU_IMA_ADDR = address;
		*IPU_IMA_DATA = params[0];
		*IPU_IMA_ADDR = address+1;
		*IPU_IMA_DATA = params[1];
		address = 0x647 << 3;
		params[0] = 239 | (512 - 59) << 9 | 205 << 18 | (8192-474)<< 27;
		params[1] = (8192-474) >> 5;
		*IPU_IMA_ADDR = address;
		*IPU_IMA_DATA = params[0];
		*IPU_IMA_ADDR = address+1;
		*IPU_IMA_DATA = params[1];
	}

	return 1;
}

void
imx35_vcap_fini(disp_adapter_t *adapter)
{
	imx35_context_t *imx35 = adapter->ms_ctx;

	disable_unit(imx35);

	imx35->capture_configured = 0;

	if (imx35->capture_buf != NULL) {
		disp_freemem(imx35->capture_buf, imx35->capture_buf_size);
		imx35->capture_buf = NULL;
	}
}

int
imx35_vcap_bind_layer(disp_adapter_t *adapter,
    int unit, int dispno, int layer_idx)
{
	imx35_context_t *imx35 = adapter->ms_ctx;

	if (unit != 0) {
		return -1;
	}

	if (layer_idx < 0 || layer_idx > 1) {
		imx35->capture_bound = -1;
		disable_unit(imx35);
	
		return -1;
	}

	if (imx35->capture_bound != -1 && imx35->capture_bound != layer_idx) {
		layer_info      *linfo;

		if (imx35->capture_bound == 1) {
			*IPU_FS_PROC_FLOW &= ~(5 << 16);
			*IPU_FS_DISP_FLOW &= ~0x30;
		} else {
			*IPU_FS_PROC_FLOW &= ~(4 << 16);
			*IPU_FS_DISP_FLOW &= ~3;
		}
		linfo = &imx35->layer[imx35->capture_bound];
	}

	imx35->capture_bound = layer_idx;

	if (imx35->capture_bound == 1) {
		*IPU_FS_PROC_FLOW |= 5 << 16;
		*IPU_FS_DISP_FLOW |= 0x30;
	} else {
		*IPU_FS_PROC_FLOW |= 4 << 16;
		*IPU_FS_DISP_FLOW |= 0x3;
	}

	set_layer_surfaces(imx35);

	return 0;
}

void
imx35_vcap_wait_vsync(disp_adapter_t *adapter, int unit)
{
	imx35_context_t *imx35 = adapter->ms_ctx;
	int     dispno = 0;

	if (imx35->panel_on) {
		adapter->callback(adapter->callback_handle,
			DISP_CALLBACK_WAIT_VSYNC, &dispno);
	}
}

int
devg_get_vcapfuncs(disp_adapter_t *adapter, disp_vcapfuncs_t *funcs, int tabsize)
{
	DISP_ADD_FUNC(disp_vcapfuncs_t, funcs,
		init, imx35_vcap_init, tabsize);
	DISP_ADD_FUNC(disp_vcapfuncs_t, funcs,
		fini, imx35_vcap_fini, tabsize);
	DISP_ADD_FUNC(disp_vcapfuncs_t, funcs,
		module_info, imx35_module_info, tabsize);
	DISP_ADD_FUNC(disp_vcapfuncs_t, funcs,
		set_props, imx35_vcap_set_props, tabsize);
	DISP_ADD_FUNC(disp_vcapfuncs_t, funcs,
		set_enabled, imx35_vcap_set_enabled, tabsize);
	DISP_ADD_FUNC(disp_vcapfuncs_t, funcs,
		bind_layer, imx35_vcap_bind_layer, tabsize);
	DISP_ADD_FUNC(disp_vcapfuncs_t, funcs,
		wait_vsync, imx35_vcap_wait_vsync, tabsize);
	
	return 0;
}
