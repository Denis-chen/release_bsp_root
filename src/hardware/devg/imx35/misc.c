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

int
imx35_misc_wait_idle(disp_adapter_t *adapter)
{
	imx35_draw_context_t *idraw = (imx35_draw_context_t *)adapter->gd_ctx;

	return g_ovg_backend_funcs.wait_idle(idraw->ovgContext);
}

int
imx35_attach_external(disp_adapter_t *adapter, disp_aperture_t aper[])
{
	imx35_context_t *imx35 = adapter->shmem;
	imx35_draw_context_t *idraw;

	if (imx35_load_ovg_ifs(imx35) != 0)
		return -1;

	if ((idraw = calloc(1, sizeof (*idraw))) == NULL)
		return -1;

	idraw->imx35 = imx35;
	idraw->adapter = adapter;
	idraw->external_client = 1;
	idraw->vidptr = aper[0].vaddr;

	if ((idraw->ovgContext = g_ovg_backend_funcs.create_context()) == NULL)
	{
		free(idraw);
		return -1;
	}

	adapter->caps = DISP_CAP_2D_ACCEL | DISP_CAP_VG_ACCEL;
	adapter->ms_ctx = imx35;
	adapter->gd_ctx = idraw;

	return 0;
}

int
imx35_detach_external(disp_adapter_t *adapter)
{
	imx35_draw_context_t *idraw = adapter->gd_ctx;
	g_ovg_backend_funcs.destroy_context(idraw->ovgContext);
	free(idraw);
	adapter->gd_ctx = NULL;
	return 0;
}

int
imx35_recover(disp_adapter_t *adapter)
{
        return 0;
}

int
imx35_gui_init(disp_adapter_t *adapter, char *optstring)
{
	return 0;
}

void
imx35_gui_fini (disp_adapter_t *adapter)
{
}

/*
 * Called when a client process terminates unexpectedly.
 * This function gets called regardless of whether the device was locked.
 */
int
imx35_terminate_notify(disp_adapter_t *adapter, pid_t pid)
{
	g_ovg_backend_funcs.terminate_notify(pid);

	return 0;
}

void
imx35_module_info(disp_adapter_t *adp, disp_module_info_t *info)
{
	info->description = "Freescale i.mx35 Synchronous Display Controller";
	info->ddk_version_major = DDK_VERSION_MAJOR;
	info->ddk_version_minor = DDK_VERSION_MINOR;
	info->ddk_rev = DDK_REVISION;
	info->driver_rev = 0;
}

int
devg_get_miscfuncs(disp_adapter_t *adp, disp_draw_miscfuncs_t *funcs, int tabsize)
{
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, init, imx35_gui_init, tabsize);
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, fini, imx35_gui_fini, tabsize);
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, module_info, imx35_module_info, tabsize);
        DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, get_corefuncs_sw, ffb_get_corefuncs, tabsize);
        DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, get_contextfuncs_sw, ffb_get_contextfuncs, tabsize);
        DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, attach_external, imx35_attach_external, tabsize);
        DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, detach_external, imx35_detach_external, tabsize);
        DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, recover, imx35_recover, tabsize);
        DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, wait_idle, imx35_misc_wait_idle, tabsize);
	DISP_ADD_FUNC(disp_draw_miscfuncs_t, funcs, terminate_notify, imx35_terminate_notify, tabsize);

	return 0;
}


