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
#include <VG/ovg_backend.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <atomic.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/neutrino.h>

#include <dlfcn.h>

static char g_ovg_libName[1024];

static void __attribute__ ((constructor))
imx35_Constructor(void)
{
	g_ovg_libHandle = NULL;
	memset(g_ovg_libName, 0, sizeof(g_ovg_libName));
}

static void __attribute__ ((destructor))
imx35_Destructor(void)
{
	imx35_close_ovg_ifs();
}

int devg_shmem_size = sizeof(imx35_context_t);

void
imx35_close_ovg_ifs()
{
	if (g_ovg_libHandle != NULL)
	{
		dlclose(g_ovg_libHandle);
		g_ovg_libHandle = NULL;
		memset(g_ovg_libName, 0, sizeof(g_ovg_libName));
		memset(&g_ovg_backend_funcs, 0, sizeof(g_ovg_backend_funcs));
	}
}

int
imx35_load_ovg_ifs(imx35_context_t *imx35)
{
	get_ovg_backend_funcs_t get_backend_funcs;

	if (g_ovg_libHandle != NULL && strcmp(g_ovg_libName, imx35->ovgLibName) != 0)
		imx35_close_ovg_ifs();

	if ((g_ovg_libHandle = dlopen(imx35->ovgLibName, 0)) == NULL)
		return -1;

	if ((get_backend_funcs = dlsym(g_ovg_libHandle, "ovg_get_backend_funcs")) == NULL)
	{
		dlclose(g_ovg_libHandle);
		g_ovg_libHandle = NULL;
		return -2;
	}

	if (get_backend_funcs(&g_ovg_backend_funcs, sizeof(g_ovg_backend_funcs)) != 0)
	{
		dlclose(g_ovg_libHandle);
		g_ovg_libHandle = NULL;
		return -3;
	}

	strcpy(g_ovg_libName, imx35->ovgLibName);

	return 0;
}

int
imx35_mode_init(disp_adapter_t *adapter, char *optstring)
{
	imx35_context_t		*imx35;
	uint32_t		dma_addr;
	imx35_draw_context_t *idraw;

	if (adapter->shmem == NULL)
		goto fail;

	if ((idraw = calloc(1, sizeof(imx35_draw_context_t))) == NULL)
		goto fail;

	imx35 = adapter->shmem;
	memset(imx35, 0, sizeof(*imx35));

	adapter->ms_ctx = imx35;
	adapter->gd_ctx = idraw;

	adapter->vsync_counter = &imx35->vsync_counter;
	imx35->adapter = adapter;
	idraw->adapter = adapter;
	idraw->imx35 = imx35;

	if (get_config_data(imx35, optstring) == -1) {
		goto fail1;
	}

	if (imx35_load_ovg_ifs(imx35) != 0)
		goto fail1;

	if (g_ovg_backend_funcs.init(adapter->pulseprio) == -1)
		goto fail1;

	/* Register with the display utility lib */
	if (disp_register_adapter(adapter) == -1) {
		goto fail2;
	}

	/* Map SDC registers */
	imx35->regptr = mmap_device_memory(NULL, IMX35_IPU_REGSIZE,
	    PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, IMX35_IPU_REGBASE);

	if (imx35->regptr == MAP_FAILED) {
		disp_perror(adapter, "mmap control registers: MAP_FAILED");
		goto fail3;
	}

	/* Map CCM registers */
	imx35->ccm_regptr = mmap_device_memory(NULL, IMX35_CCM_REGSIZE,
	    PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, IMX35_CCM_REGBASE);

	if (imx35->ccm_regptr == MAP_FAILED) {
		disp_perror(adapter, "mmap control registers: MAP_FAILED");
		goto fail4;
	}

	if (imx35_irq_setup(adapter) == -1) {
		disp_perror(adapter, "irq setup failed");
		goto fail5;
	}

        /* create surface to fake out layer 0, if only layer 1 is used */
	imx35->zero_stride = imx35->xres * 2;
	imx35->zero_stride = (imx35->zero_stride + 31) & ~31;
        imx35->zero_size = imx35->yres * imx35->zero_stride;
        imx35->zero_ptr = disp_getmem(imx35->zero_size, PROT_READ|PROT_WRITE, DISP_MAP_PHYS);
        if (imx35->zero_ptr == NULL) {
                goto fail6;
        }
        imx35->zero_base = disp_phys_addr(imx35->zero_ptr);
	imx35->capture_bound = -1;

#ifdef VARIANT_wakuwaku
        {
                char tbuff[255];
                sprintf(tbuff, "WakuWaku variant %s %s\n", __DATE__,__TIME__);
                disp_printf(adapter,tbuff);
        }
        /* Map board-specific GPIO reg */
        if((imx35->gpio_regptr = mmap_device_memory(NULL, WAKUWAKU3_GPIO_REGSIZE,
                        PROT_READ|PROT_WRITE|PROT_NOCACHE, 0, WAKUWAKU3_GPIO_REGBASE)) == MAP_FAILED) {
                disp_perror(adapter, "mmap gpio control registers: MAP_FAILED");
                goto fail7;
        }

	/* board-specific: take the ADV7125 out of powersave */
	*GPIO_PSAVE |= (1 << 6);
#else
	/* board specific to IMX35 refernce board */
	if (mcu_setup (adapter, 1) != 0)   {
		disp_perror(adapter, "i2c setup failed");
		goto fail7;
	}
#endif

	/* set the data mask so that the screen is black */
	*DI_DISP_IF_CONF |= (1 << 24);

	*IDMAC_CONF = 0;	/* set to double master and set service request counter 0 at FS suggestion */
	*IDMAC_CHA_PR = 0xC000;	/* set SDC 0 (BG) SDC 1(FG) dma channels to hight priorty */ 	

	*CCM_CGR1 |= 3 << 18;	/* IPU clock enable */

	*DI_HSP_CLK_PER = 0x00100010; 

	/* Initialize display DMA control registers */
	dma_addr = 0x10000 | (IMX35_DMA_CHAN_0 << 4);
	*IPU_IMA_ADDR = dma_addr+0;
	*IPU_IMA_DATA = 0;
	*IPU_IMA_ADDR = dma_addr+1;
	*IPU_IMA_DATA = 0x4000;
	*IPU_IMA_ADDR = dma_addr+2;
	*IPU_IMA_DATA = 0;
	*IPU_IMA_ADDR = dma_addr+3;
	*IPU_IMA_DATA = ((imx35->xres - 1) << 12) | ((imx35->yres - 1) << 24);
	*IPU_IMA_ADDR = dma_addr+4;
	*IPU_IMA_DATA = (imx35->yres - 1) >> 8;
	*IPU_IMA_ADDR = dma_addr+8;
	*IPU_IMA_DATA = imx35->zero_base;
	*IPU_IMA_ADDR = dma_addr+9;
	*IPU_IMA_DATA = 0;
	*IPU_IMA_ADDR = dma_addr+10;
	*IPU_IMA_DATA = (imx35->zero_stride - 1) << 3 | 2 | (4 << 17) | (0xf << 25); 
	*IPU_IMA_ADDR = dma_addr+11;
	*IPU_IMA_DATA =  2 | (0 << 3) | (5 << 8) | (11 << 13) | (16 << 18) | (4 << 23) |
        			(5 << 26) | (4 << 29);
	*IPU_IMA_ADDR = dma_addr+12;
	*IPU_IMA_DATA = 5;
	*IPU_CHA_DB_MODE_SEL &= ~(1 << IMX35_DMA_CHAN_0); /* no double buffering */
	*IPU_CHA_CUR_BUF &= ~(1 << IMX35_DMA_CHAN_0);

	/* Now channel 1 */
	dma_addr = 0x10000 | (IMX35_DMA_CHAN_1 << 4);
	*IPU_IMA_ADDR = dma_addr+0;
	*IPU_IMA_DATA = 0;
	*IPU_IMA_ADDR = dma_addr+1;
	*IPU_IMA_DATA = 0x4000;
	*IPU_IMA_ADDR = dma_addr+2;
	*IPU_IMA_DATA = 0;
	*IPU_IMA_ADDR = dma_addr+3;
	*IPU_IMA_DATA = ((imx35->xres - 1) << 12) | ((imx35->yres - 1) << 24);
	*IPU_IMA_ADDR = dma_addr+4;
	*IPU_IMA_DATA = (imx35->yres - 1) >> 8;
	*IPU_IMA_ADDR = dma_addr+8;
	*IPU_IMA_DATA = imx35->zero_base;
	*IPU_IMA_ADDR = dma_addr+9;
	*IPU_IMA_DATA = 0;
	*IPU_IMA_ADDR = dma_addr+10;
	*IPU_IMA_DATA = (imx35->zero_stride - 1) << 3 | 2 | (4 << 17) | (0xf << 25); 
	*IPU_IMA_ADDR = dma_addr+11;
	*IPU_IMA_DATA =  2 | (0 << 3) | (5 << 8) | (11 << 13) | (16 << 18) | (4 << 23) |
        			(5 << 26) | (4 << 29);
	*IPU_IMA_ADDR = dma_addr+12;
	*IPU_IMA_DATA = 5;

	*IPU_CHA_DB_MODE_SEL &= ~(1 << IMX35_DMA_CHAN_1); /* no double buffering */
	*IPU_CHA_CUR_BUF &= ~(1 << IMX35_DMA_CHAN_1);

	adapter->caps |= DISP_CAP_2D_ACCEL | DISP_CAP_VG_ACCEL;


	return 1;

fail7:
	disp_freemem(imx35->zero_ptr, imx35->zero_size);
fail6:
	imx35_irq_cleanup(imx35);
fail5:
	disp_munmap_device_memory(imx35->ccm_regptr, IMX35_CCM_REGSIZE);
fail4:
	disp_munmap_device_memory(imx35->regptr, IMX35_IPU_REGSIZE);
fail3:
	disp_unregister_adapter(adapter);
fail2:
	g_ovg_backend_funcs.fini();
fail1:
	free(idraw);
fail:
	return -1;
}

void
imx35_mode_fini(disp_adapter_t *adapter)
{
	imx35_context_t		*imx35 = adapter->ms_ctx;
	imx35_draw_context_t	*idraw = adapter->gd_ctx;
	int			i = 100000;

	/* set the data mask so that the screen is black */
	*DI_DISP_IF_CONF |= (1 << 24);

	*IPU_CHA_BUF0_RDY = 0;

	/* wait for channels to become idle before disabling */
	while (*IDMAC_CHA_BUSY & 0xC000) {
		nanospin_ns(500);
		if (i-- == 0) {
			break;
		}
	}

	if (*IDMAC_CHA_EN & (1 << IMX35_DMA_CHAN_0)) {
		*IDMAC_CHA_EN &= ~(1 << IMX35_DMA_CHAN_0);
	}
	if (*IDMAC_CHA_EN & (1 << IMX35_DMA_CHAN_1)) {
		*IDMAC_CHA_EN &= ~(1 << IMX35_DMA_CHAN_1);
	}

	/* uncommenting the line below results in colorful effects the driver shuts down */
//	*IPU_CONF &= ~0x50;		/* disable DI & SDC */
	*CCM_CGR1 &= ~(3 << 18);	/* IPU clock disable */
	*SDC_CUR_BLINK_PWM_CTRL = 0x03000000;

#ifndef VARIANT_wakuwaku
	mcu_setup (adapter, 0);
#endif

	imx35_irq_cleanup(imx35);

	if (imx35->zero_ptr) {
		disp_freemem(imx35->zero_ptr, imx35->zero_size);
	}

	g_ovg_backend_funcs.fini();

#ifdef VARIANT_wakuwaku
        disp_munmap_device_memory(imx35->gpio_regptr, WAKUWAKU3_GPIO_REGSIZE);
#endif
	disp_munmap_device_memory(imx35->regptr, IMX35_IPU_REGSIZE);
	disp_munmap_device_memory(imx35->ccm_regptr, IMX35_CCM_REGSIZE);

	free(idraw);

	disp_unregister_adapter(adapter);
}

int
imx35_get_modelist(disp_adapter_t *adapter, int dispno, unsigned short *list,
    int index, int size)
{
	static unsigned modes[] = { 16, 24, 32 };
	int i = index;
	int j = 0;

	if (dispno > 0 || size-- < 2) // keep a spot for DISP_MODE_LISTEND by decrementing size
		return -1;

	while (j < size && i < sizeof(modes) / sizeof(modes[0]))
		list[j++] = modes[i++];

	list[j] = DISP_MODE_LISTEND;

	return 0;
}

int
imx35_get_modeinfo(disp_adapter_t *adapter,
    int dispno, disp_mode_t mode, disp_mode_info_t *info)
{
	imx35_context_t	*imx35 = adapter->ms_ctx;

	memset(info, 0, sizeof (*info));

	info->size = sizeof (*info);
	info->mode = mode; 
	info->xres = imx35->xres;
	info->yres = imx35->yres;

	switch (mode) {
		case 16:
			info->pixel_format = DISP_SURFACE_FORMAT_RGB565;
			break;
		case 24:
			info->pixel_format = DISP_SURFACE_FORMAT_RGB888;
			break;
		case 32:
			info->pixel_format = DISP_SURFACE_FORMAT_ARGB8888;
			break;
		default:
			return -1;
	}	

	info->u.fixed.refresh[0] = 60;
	info->u.fixed.refresh[1] = 0;

	return 0;
}

static const uint32_t DI_mappings[] =
{
#ifdef VARIANT_wakuwaku
	0x00070000, 0x000F0000, 0x00170000, 1,  /* RGB888 */
#else
	0x1600AAAA, 0x00E05555, 0x00070000, 3,  /* RGB888 */
#endif
	0x0005000F, 0x000B000F, 0x0011000F, 1,	/* RGB666 */
	0x0004003F, 0x000A000F, 0x000F003F, 1 	/* RGB565 */
};

int
imx35_set_mode(disp_adapter_t *adapter, int dispno, disp_mode_t mode,
    disp_crtc_settings_t *settings, disp_surface_t *surf, unsigned flags)
{
	imx35_context_t	*imx35 = adapter->ms_ctx;
	int		if_clk_cnt_d = 0;
	int		screen_width = (imx35->xres + imx35->hss + imx35->hew);
	int		screen_height = (imx35->yres + imx35->vss + imx35->vew);

	*SDC_HOR_CONF = ((imx35->hsw - 1) << 26) | ((screen_width - 1) << 16);
	*SDC_VER_CONF = ((imx35->vsw - 1) << 26) | ((screen_height - 1) << 16) | 1;

	*SDC_SHARP_CONF_1 = imx35->sharp_conf1;
	*SDC_SHARP_CONF_2 = imx35->sharp_conf2;

	/* for proper layer blending/chroma make fg plane the 'graphics plane' */
	imx35->sdc_com = imx35->sdc_mode | 0x20;

	if (imx35->sharp_conf1 != 0)
		imx35->sdc_com |= 1 << 12;

	switch(imx35->output_fmt) {
		case 24:	/* RGB888 */
			*DI_DISP3_B0_MAP = DI_mappings[0];
			*DI_DISP3_B1_MAP = DI_mappings[1];
			*DI_DISP3_B2_MAP = DI_mappings[2];
			*DI_DISP_ACC_CC = *DI_DISP_ACC_CC | ((DI_mappings[3] - 1) << 12);
			if_clk_cnt_d = DI_mappings[3];
			break;
		case 18:	/* RGB666 */
			*DI_DISP3_B0_MAP = DI_mappings[4];
			*DI_DISP3_B1_MAP = DI_mappings[5];
			*DI_DISP3_B2_MAP = DI_mappings[6];
			*DI_DISP_ACC_CC = *DI_DISP_ACC_CC | ((DI_mappings[7] - 1) << 12);
			if_clk_cnt_d = DI_mappings[7];
			break;
		case 16:	/* RGB565 */
			*DI_DISP3_B0_MAP = DI_mappings[8];
			*DI_DISP3_B1_MAP = DI_mappings[9];
			*DI_DISP3_B2_MAP = DI_mappings[10];
			*DI_DISP_ACC_CC = *DI_DISP_ACC_CC | ((DI_mappings[11] - 1) << 12);
			if_clk_cnt_d = DI_mappings[11];
			break;
		default:
			return -1;
	}

	/* if no clock parameters are given */
	if (imx35->clk_down_wr == 0 && imx35->clk_per_wr == 0) {
		uint32_t        hsp_clk = 0;
		int             hsp_pdf; 
		int		hsp_per = *DI_HSP_CLK_PER & 0x7f;

		hsp_pdf = (*CCM_PDR0 & CCM_PDR0_HSP_PODF_MASK) >> CCM_PDR0_HSP_PODF_OFFSET;

		switch (hsp_pdf) {
			case 1:
				hsp_clk = 66500000;
				break;
			case 2:
				hsp_clk = 178000000;
				break;
			case 0:
			default:
				hsp_clk = 133000000;
				break;
			break;
		}

		imx35->clk_down_wr = ((hsp_clk / imx35->refresh)*hsp_per) /
			(screen_width * screen_height * if_clk_cnt_d);

		*DI_DISP3_TIME_CONF = ((imx35->clk_down_wr / 8) - 1) << 22 | imx35->clk_down_wr;
	} else {
		*DI_DISP3_TIME_CONF = imx35->clk_down_wr << 22 | imx35->clk_up_wr << 12 | imx35->clk_per_wr;	
	}
	
	/* DI settings */
	*DI_DISP_IF_CONF &= 0x78FFFFFF;
	*DI_DISP_IF_CONF |= imx35->data_mask << 24 |
			imx35->clk_sel << 25|
			imx35->clk_idle << 26;


	*DI_DISP_SIG_POL &= 0x30FFFFFF;
	*DI_DISP_SIG_POL |= imx35->data_pol << 24 |
				imx35->clock_pol << 25 |
				imx35->enable_pol << 26 |
				imx35->hsync_pol << 27 |
				imx35->vsync_pol << 28;	


	imx35_layer_reset(adapter, dispno, 0);
	imx35_layer_reset(adapter, dispno, 1);

	*IPU_INT_CTRL_3 |= IPU_IRQ_SDC_DISP3_VSYNC;	/* Enable vsync interrupts */

	return 0;
}

int
imx35_wait_vsync(disp_adapter_t *adapter, int dispno)
{
        imx35_context_t         *imx35 = adapter->ms_ctx;
        struct _pulse           pulse;
        iov_t                   iov;
        uint64_t                halfsecond = 500*1000*1000;

	imx35->want_vsync_pulse = 1;

	adapter->callback(adapter->callback_handle, DISP_CALLBACK_UNLOCK, NULL);

        SETIOV(&iov, &pulse, sizeof (pulse));

        while (1) {
                TimerTimeout(CLOCK_REALTIME,
                    _NTO_TIMEOUT_RECEIVE, NULL, &halfsecond, NULL);

                if (MsgReceivev(imx35->vsync_chan, &iov, 1, NULL) == -1) {
                        disp_perror(adapter, "MsgReceive");
                        break;
                }
                if (pulse.code == IPU_DISP3_VSYNC_PULSE) {
                        break;
		}
        }

	adapter->callback(adapter->callback_handle, DISP_CALLBACK_LOCK, NULL);

        return 0;
}


int
imx35_set_dpms_mode(disp_adapter_t *adp, int dispno, int mode)
{
        imx35_context_t      *imx35 = adp->ms_ctx;

        /* Determine the state we want to be in */
        switch (mode) {
                case DISP_DPMS_MODE_ON:
			*SDC_CUR_BLINK_PWM_CTRL = 0x03000000 | imx35->brightness << 16;
                        break;
                default:
			*SDC_CUR_BLINK_PWM_CTRL = 0x03000000;
        }

        return 0;
}

int
devg_get_modefuncs(disp_adapter_t *adp, disp_modefuncs_t *funcs, int tabsize)
{
	DISP_ADD_FUNC(disp_modefuncs_t, funcs, init, imx35_mode_init, tabsize);
	DISP_ADD_FUNC(disp_modefuncs_t, funcs, fini, imx35_mode_fini, tabsize);
	DISP_ADD_FUNC(disp_modefuncs_t, funcs, module_info, imx35_module_info, tabsize);
	DISP_ADD_FUNC(disp_modefuncs_t, funcs, get_modeinfo, imx35_get_modeinfo, tabsize);
	DISP_ADD_FUNC(disp_modefuncs_t, funcs, get_modelist, imx35_get_modelist, tabsize);
	DISP_ADD_FUNC(disp_modefuncs_t, funcs, set_mode, imx35_set_mode, tabsize);
	DISP_ADD_FUNC(disp_modefuncs_t, funcs, wait_vsync, imx35_wait_vsync, tabsize);
	DISP_ADD_FUNC(disp_modefuncs_t, funcs, set_dpms_mode, imx35_set_dpms_mode, tabsize);

        DISP_ADD_FUNC(disp_modefuncs_t, funcs,
                layer_query, imx35_layer_query, tabsize);
        DISP_ADD_FUNC(disp_modefuncs_t, funcs,
                layer_enable, imx35_layer_enable, tabsize);
        DISP_ADD_FUNC(disp_modefuncs_t, funcs,
                layer_disable, imx35_layer_disable, tabsize);
        DISP_ADD_FUNC(disp_modefuncs_t, funcs,
                layer_set_surface, imx35_layer_set_surface, tabsize);
        DISP_ADD_FUNC(disp_modefuncs_t, funcs,
                layer_set_source_viewport, imx35_layer_set_source_viewport, tabsize);
        DISP_ADD_FUNC(disp_modefuncs_t, funcs,
                layer_set_dest_viewport, imx35_layer_set_dest_viewport, tabsize);
        DISP_ADD_FUNC(disp_modefuncs_t, funcs,
                layer_set_blending, imx35_layer_set_blending, tabsize);
        DISP_ADD_FUNC(disp_modefuncs_t, funcs,
                layer_set_chromakey, imx35_layer_set_chromakey, tabsize);
        DISP_ADD_FUNC(disp_modefuncs_t, funcs,
                layer_update_begin, imx35_layer_update_begin, tabsize);
        DISP_ADD_FUNC(disp_modefuncs_t, funcs,
                layer_update_end, imx35_layer_update_end, tabsize);
        DISP_ADD_FUNC(disp_modefuncs_t, funcs,
                layer_reset, imx35_layer_reset, tabsize);

	DISP_ADD_FUNC(disp_modefuncs_t, funcs,
		set_hw_cursor, imx35_set_hw_cursor, tabsize);
	DISP_ADD_FUNC(disp_modefuncs_t, funcs,
		enable_hw_cursor, imx35_enable_hw_cursor, tabsize);
	DISP_ADD_FUNC(disp_modefuncs_t, funcs,
		disable_hw_cursor, imx35_disable_hw_cursor, tabsize);
	DISP_ADD_FUNC(disp_modefuncs_t, funcs,
		set_hw_cursor_pos, imx35_set_hw_cursor_pos, tabsize);

	return 0;
}


