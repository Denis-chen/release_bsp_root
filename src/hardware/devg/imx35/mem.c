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
#include <string.h>

int
imx35_mem_init(disp_adapter_t *adapter, char *optstring)
{
	imx35_context_t *imx35 = adapter->ms_ctx;
	imx35_draw_context_t *idraw = adapter->gd_ctx;
	disp_surface_t surf;

	// memory wasn't reserved, allocation is done when surfaces are allocated
	if (imx35->vidsize <= 0 || imx35->vidbase <= 0)
		return 0;

	if ((idraw->vidptr = disp_mmap_device_memory(imx35->vidbase, imx35->vidsize,
													PROT_READ | PROT_WRITE | PROT_NOCACHE, 0)) == NULL)
	{
		goto fail1;
	}

	surf.offset = 0;
	surf.stride = surf.width = imx35->vidsize - surf.offset;
	surf.height = 1;
	surf.vidptr = idraw->vidptr + surf.offset;
	surf.paddr = imx35->vidbase + surf.offset;
	surf.flags = DISP_SURFACE_DISPLAYABLE | DISP_SURFACE_PHYS_CONTIG |
					DISP_SURFACE_DMA_SAFE | DISP_SURFACE_NON_CACHEABLE |
					DISP_SURFACE_CPU_LINEAR_READABLE | DISP_SURFACE_CPU_LINEAR_WRITEABLE |
					DISP_SURFACE_2D_TARGETABLE | DISP_SURFACE_2D_READABLE |
					DISP_SURFACE_VG_TARGETABLE | DISP_SURFACE_VG_READABLE;
	adapter->mm_ctx = disp_vm_create_pool(adapter, &surf, 128);

	// initialize the trace info to zero
	memset(idraw->vidptr, 0, surf.offset);

	return 0;
fail1:

	return -1;
}

void
imx35_mem_fini(disp_adapter_t *adapter)
{
	imx35_context_t *imx35 = adapter->ms_ctx;
	imx35_draw_context_t *idraw = adapter->gd_ctx;

	if (adapter->mm_ctx != NULL)
	{
		disp_vm_destroy_pool(adapter, adapter->mm_ctx);
		adapter->mm_ctx = NULL;
	}

	if (imx35 != NULL && idraw != NULL && idraw->vidptr != NULL)
		disp_munmap_device_memory(idraw->vidptr, imx35->vidsize);
}

int
imx35_mem_reset(disp_adapter_t *adapter, disp_surface_t *surf)
{
	imx35_mem_fini(adapter);
	return imx35_mem_init(adapter, NULL);
}

unsigned
layer_format_to_surface_format(unsigned layer_format)
{
	switch (layer_format)
	{
		case DISP_LAYER_FORMAT_RGB565:
			return DISP_SURFACE_FORMAT_RGB565;
		case DISP_LAYER_FORMAT_RGB888:
			return DISP_SURFACE_FORMAT_RGB888;
		case DISP_LAYER_FORMAT_ARGB8888:
			return DISP_SURFACE_FORMAT_ARGB8888;
		case DISP_LAYER_FORMAT_PAL8:
			return DISP_SURFACE_FORMAT_PAL8;
		case DISP_LAYER_FORMAT_ARGB1555:
			return DISP_SURFACE_FORMAT_ARGB1555;
		case DISP_LAYER_FORMAT_RGB556:
			return DISP_SURFACE_FORMAT_RGB556;
		case DISP_LAYER_FORMAT_BYTES:
			return DISP_SURFACE_FORMAT_BYTES;
		case DISP_LAYER_FORMAT_UYVY:
			return DISP_SURFACE_FORMAT_PACKEDYUV_UYVY;
		case DISP_LAYER_FORMAT_YUY2:
			return DISP_SURFACE_FORMAT_PACKEDYUV_YUY2;
		case DISP_LAYER_FORMAT_YVYU:
			return DISP_SURFACE_FORMAT_PACKEDYUV_YVYU;
		case DISP_LAYER_FORMAT_V422:
			return DISP_SURFACE_FORMAT_PACKEDYUV_V422;
		case DISP_LAYER_FORMAT_AYUV:
			return DISP_SURFACE_FORMAT_PACKEDYUV_AYUV;
		default:
			return ~0;
	}
}

disp_surface_t *
imx35_alloc_surface(disp_adapter_t *adapter, int width, int height, unsigned format, unsigned flags, unsigned user_flags)
{
	disp_alloc_info_t info;

	if (adapter->mm_ctx == NULL)
		return NULL;

	if ((DISP_VMEM_HINT_USAGE(user_flags) == DISP_VMEM_HINT_USAGE_CPU) &&
				!(flags & DISP_SURFACE_DISPLAYABLE || flags & DISP_SURFACE_2D_TARGETABLE ||
					flags & DISP_SURFACE_2D_READABLE || flags & DISP_SURFACE_VG_TARGETABLE ||
					flags & DISP_SURFACE_VG_READABLE))
	{
		return NULL;
	}

	if (imx35_get_alloc_info(adapter, width, height, format, flags, user_flags, &info) != 0)
		return NULL;

	return disp_vm_alloc_surface(adapter->mm_ctx, width, height, info.min_stride, format,
									info.surface_flags & ~DISP_SURFACE_DRIVER_NOT_OWNER);
}

disp_surface_t *
imx35_alloc_layer_surface(disp_adapter_t *adapter, int dispno[], int layer_idx[], int nlayers, unsigned format,
							int surface_index, int width, int height, unsigned sflags, unsigned hint_flags)
{
	int i;

	if (surface_index > 0)
		return NULL;

	/* Compatibility checks */
	for (i = 0; i < nlayers; ++i)
	{
		if (layer_idx[i] > 1 || layer_idx[i] < 0 || dispno[i] > 0)
			return NULL;
	}

	return imx35_alloc_surface(adapter, width, height, layer_format_to_surface_format(format),
								sflags | DISP_SURFACE_DISPLAYABLE, hint_flags);
}

int
imx35_free_surface(disp_adapter_t *adapter, disp_surface_t *surf)
{
	if (adapter->mm_ctx == NULL)
		return 0;

	disp_vm_free_surface(adapter, surf);

	return 0;
}

unsigned long
imx35_mem_avail(disp_adapter_t *adp, unsigned flags)
{
	return 0;
}

int
imx35_query_apertures(disp_adapter_t *adapter, disp_aperture_t *ap)
{
	imx35_context_t *imx35 = adapter->ms_ctx;

	if (imx35->vidsize <= 0 || imx35->vidbase <= 0)
		return 0;

	ap->base = imx35->vidbase;
	ap->size = imx35->vidsize;
	ap->flags = DISP_APER_NOCACHE;
	ap++;

	return 1;
}

/*
 * return the aperture within which the memory surface resides, and
 * the physical offset of the memory within that aperture
 */

int
imx35_query_surface(disp_adapter_t *adp,
    disp_surface_t *surf, disp_surface_info_t *info)
{
	info->aperture_index = 0;
	info->offset = surf->offset;

	return 0;
}

/*
 * If a client of the driver wants to allocate memory itself,
 * it must allocate it in accordance with the parameters returned by
 * this function.  Since this memory will not be coming from
 * video memory, we must check the flags accordingly.
 */
int
imx35_get_alloc_info(disp_adapter_t *adapter, int width, int height, unsigned format,
					 unsigned flags, unsigned user_flags, disp_alloc_info_t *info)
{
	return g_ovg_backend_funcs.get_alloc_info(width, height, format, flags, user_flags, info);
}

int
imx35_get_alloc_layer_info(disp_adapter_t *adp, int dispno[], int layer_idx[], int nlayers,
						   unsigned format, int surface_index, int width, int height,
						   unsigned sflags, unsigned hint_flags, disp_alloc_info_t *info)
{
	int i;

	if (surface_index > 0)
		return -1;

	/* Compatibility checks */
	for (i = 0; i < nlayers; i++)
	{
		if (layer_idx[i] > 1 || layer_idx[i] < 0 || dispno[i] > 0)
			return -1;
	}

	return imx35_get_alloc_info(adp, width, height, layer_format_to_surface_format(format),
								sflags | DISP_SURFACE_DISPLAYABLE, hint_flags, info);
}

int
devg_get_memfuncs(disp_adapter_t *adp, disp_memfuncs_t *funcs, int tabsize)
{
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, init, imx35_mem_init, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, fini, imx35_mem_fini, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, module_info, imx35_module_info, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, reset, imx35_mem_reset, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, alloc_surface, imx35_alloc_surface, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, alloc_layer_surface, imx35_alloc_layer_surface, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, free_surface, imx35_free_surface, tabsize);
	DISP_ADD_FUNC(disp_memfuncs_t, funcs, mem_avail, imx35_mem_avail, tabsize);
        DISP_ADD_FUNC(disp_memfuncs_t, funcs, query_apertures, imx35_query_apertures, tabsize);
        DISP_ADD_FUNC(disp_memfuncs_t, funcs, query_surface, imx35_query_surface, tabsize);
        DISP_ADD_FUNC(disp_memfuncs_t, funcs, get_alloc_info, imx35_get_alloc_info, tabsize);
        DISP_ADD_FUNC(disp_memfuncs_t, funcs, get_alloc_layer_info, imx35_get_alloc_layer_info, tabsize);

	return 0;
}


