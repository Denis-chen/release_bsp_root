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

#include "VG/ovg_driver_backend.h"

int devg_get_egl14vgfuncs(disp_adapter_t *adapter, egl_interface_t *funcs, int tabsize)
{
	get_ovg_vg_egl14backend_funcs_t get_ovg_egl14_funcs;

	if (imx35_load_ovg_ifs(adapter->shmem) != 0)
		return -1;

	if ((get_ovg_egl14_funcs = dlsym(g_ovg_libHandle, "ovg_get_vg_egl14backend_funcs")) == NULL)
		return -1;

	return get_ovg_egl14_funcs(funcs, tabsize);
}

int devg_get_vg11funcs(disp_adapter_t *adapter, ovg11_interface_t *funcs, int tabsize)
{
	get_ovg_vg11_funcs_t get_ovg_vg11_funcs;

	if (imx35_load_ovg_ifs(adapter->shmem) != 0)
		return -1;

	if ((get_ovg_vg11_funcs = dlsym(g_ovg_libHandle, "ovg_get_vg11_funcs")) == NULL)
		return -1;

	return get_ovg_vg11_funcs(funcs, tabsize);
}


