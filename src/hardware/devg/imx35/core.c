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


#include "imx35.h"

#ifdef IMX35_KERNEL_TRACE
	#include <sys/neutrino.h>
	#include <sys/trace.h>
#endif // IMX35_KERNEL_TRACE

int
devg_get_corefuncs(disp_adapter_t *adapter, unsigned pixel_format, disp_draw_corefuncs_t *funcs, int tabsize)
{
	imx35_context_t *imx35 = adapter->ms_ctx;

	if (ffb_get_corefuncs(adapter, pixel_format, funcs, tabsize) == -1)
		return -1;

	if (imx35->hw2daccel)
	{
		switch (pixel_format)
		{
			case DISP_SURFACE_FORMAT_RGB565:
			case DISP_SURFACE_FORMAT_ARGB8888:
				break;
			default:
				return 0;
		}

		DISP_ADD_FUNC(disp_draw_corefuncs_t, funcs, blit2, imx35_blit2, tabsize);
	}

	return 0;
}

void
imx35_blit2(disp_draw_context_t *ctx, disp_surface_t *src, disp_surface_t *dst,
						int sx, int sy, int dx, int dy, int width, int height)
{
	imx35_draw_context_t *idraw = (imx35_draw_context_t *)ctx->gd_ctx;

#ifdef IMX35_KERNEL_TRACE
	TraceEvent(_NTO_TRACE_INSERTUSRSTREVENT, _NTO_TRACE_USERLAST - 1, __FUNCTION__);
#endif // IMX35_KERNEL_TRACE

	if (idraw->imx35->hw2daccel < 0)
		goto done;

	if (g_ovg_backend_funcs.blit2(idraw->ovgContext, src, dst, sx, sy, dx, dy, width, height, 0, 1) >= 0)
		goto done;

	ffb_core_blit2(ctx, src, dst, sx, sy, dx, dy, width, height);

done:
	{
#ifdef IMX35_KERNEL_TRACE
		TraceEvent(_NTO_TRACE_INSERTUSRSTREVENT, _NTO_TRACE_USERLAST, __FUNCTION__);
#endif // IMX35_KERNEL_TRACE
	}
}


