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



#ifndef __IMX35_INCLUDED
#define __IMX35_INCLUDED

#include <pthread.h>
#include <graphics/display.h>
#include <graphics/disputil.h>
#include <graphics/ffb.h>

#include <VG/ovg_backend.h>

#define IPU_IRQ_SDC_DISP3_VSYNC	0x10000	
#define IPU_IRQ_SDC_0_EOF	0x4000
#define IPU_IRQ_SDC_1_EOF	0x8000

#define IPU_DISP3_VSYNC_PULSE	0x5A
#define IPU_SDC_EOF_0_PULSE	0x5B
#define IPU_SDC_EOF_1_PULSE	0x5C

#define IMX35_DMA_CHAN_0	14
#define IMX35_DMA_CHAN_1	15

#define CSI_FORMAT_MASK		0x300

#define ipu_ptr32(offset) (uint32_t volatile *)  \
	(((unsigned char volatile *)imx35->regptr) + offset)

#define ccm_ptr32(offset) (uint32_t volatile *)  \
	(((unsigned char volatile *)imx35->ccm_regptr) + offset)

#define gpio_ptr32(offset) (uint32_t volatile *)  \
       (((unsigned char volatile *)imx35->gpio_regptr) + offset)

#define CCM_PDR0_HSP_PODF_MASK		(0x3 << 20)
#define CCM_PDR0_HSP_PODF_OFFSET	20

/* Clock Control Module Registers */
#define IMX35_CCM_REGBASE	0x53F80000
#define IMX35_CCM_REGSIZE	0x70

#define CCM_CCMR                ccm_ptr32(0x00)
#define CCM_PDR0                ccm_ptr32(0x04)
#define CCM_PDR1                ccm_ptr32(0x08)
#define CCM_RCSR                ccm_ptr32(0x0C)
#define CCM_MPCTL               ccm_ptr32(0x10)
#define CCM_UPCTL               ccm_ptr32(0x14)
#define CCM_SPCTL               ccm_ptr32(0x18)
#define CCM_COSR                ccm_ptr32(0x1C)
#define CCM_CGR0                ccm_ptr32(0x20)
#define CCM_CGR1                ccm_ptr32(0x24)
#define CCM_CGR2                ccm_ptr32(0x28)
#define CCM_WIMR0               ccm_ptr32(0x2C)
#define CCM_LDC                 ccm_ptr32(0x30)
#define CCM_DCVR0               ccm_ptr32(0x34)
#define CCM_DCVR1               ccm_ptr32(0x38)
#define CCM_DCVR2               ccm_ptr32(0x3C)
#define CCM_DCVR3               ccm_ptr32(0x40)
#define CCM_LTR0                ccm_ptr32(0x44)
#define CCM_LTR1                ccm_ptr32(0x48)
#define CCM_LTR2                ccm_ptr32(0x4C)
#define CCM_LTR3                ccm_ptr32(0x50)
#define CCM_LTBR0               ccm_ptr32(0x54)
#define CCM_LTBR1               ccm_ptr32(0x58)
#define CCM_PMCR0               ccm_ptr32(0x5C)
#define CCM_PMCR1               ccm_ptr32(0x60)
#define CCM_PDR2                ccm_ptr32(0x64)

#define IMX35_IPU_REGBASE 	0x53FC0000
#define IMX35_IPU_REGSIZE 	0x200

#define IPU_CONF		ipu_ptr32(0x00)
#define IPU_CHA_BUF0_RDY	ipu_ptr32(0x04)
#define IPU_CHA_BUF1_RDY	ipu_ptr32(0x08)
#define IPU_CHA_DB_MODE_SEL	ipu_ptr32(0x0C)
#define IPU_CHA_CUR_BUF		ipu_ptr32(0x10)
#define IPU_FS_PROC_FLOW	ipu_ptr32(0x14)
#define IPU_FS_DISP_FLOW	ipu_ptr32(0x18)
#define IPU_TASKS_STAT		ipu_ptr32(0x1C)
#define IPU_IMA_ADDR		ipu_ptr32(0x20)
#define IPU_IMA_DATA		ipu_ptr32(0x24)
#define IPU_INT_CTRL_1		ipu_ptr32(0x28)
#define IPU_INT_CTRL_2		ipu_ptr32(0x2C)
#define IPU_INT_CTRL_3		ipu_ptr32(0x30)
#define IPU_INT_CTRL_4		ipu_ptr32(0x34)
#define IPU_INT_CTRL_5		ipu_ptr32(0x38)
#define IPU_INT_STAT_1		ipu_ptr32(0x3C)
#define IPU_INT_STAT_2		ipu_ptr32(0x40)
#define IPU_INT_STAT_3		ipu_ptr32(0x44)
#define IPU_INT_STAT_4		ipu_ptr32(0x48)
#define IPU_INT_STAT_5		ipu_ptr32(0x4C)

#define CSI_SENS_CONF		ipu_ptr32(0x60)
#define CSI_SENS_FRM_SIZE	ipu_ptr32(0x64)
#define CSI_ACT_FRM_SIZE	ipu_ptr32(0x68)
#define CSI_OUT_FRM_CTRL	ipu_ptr32(0x6C)
#define CSI_TST_CTRL		ipu_ptr32(0x70)
#define CSI_CCIR_CODE_1		ipu_ptr32(0x74)
#define CSI_CCIR_CODE_2		ipu_ptr32(0x78)
#define CSI_CCIR_CODE_3		ipu_ptr32(0x7C)
#define CSI_FLASH_STROBE_1	ipu_ptr32(0x80)
#define CSI_FLASH_STROBE_2	ipu_ptr32(0x84)

#define IC_CONF			ipu_ptr32(0x88)
#define IC_PRP_ENC_RSC		ipu_ptr32(0x8C)
#define IC_PRP_VF_RSC		ipu_ptr32(0x90)
#define IC_PP_RSC		ipu_ptr32(0x94)
#define IC_CMBP_1		ipu_ptr32(0x98)
#define IC_CMBP_2		ipu_ptr32(0x9C)
#define PF_CONF			ipu_ptr32(0xA0)
#define IDMAC_CONF		ipu_ptr32(0xA4)
#define IDMAC_CHA_EN		ipu_ptr32(0xA8)
#define IDMAC_CHA_PR		ipu_ptr32(0xAC)
#define	IDMAC_CHA_BUSY		ipu_ptr32(0xB0)


/* Synchronous Display Controller Regs */
#define	SDC_COM_CONF		ipu_ptr32(0xB4)
#define SDC_GRAPH_WIND_CTRL	ipu_ptr32(0xB8)
#define SDC_FG_POS		ipu_ptr32(0xBC)
#define SDC_BG_POS		ipu_ptr32(0xC0)
#define SDC_CUR_POS		ipu_ptr32(0xC4)
#define SDC_CUR_BLINK_PWM_CTRL	ipu_ptr32(0xC8)
#define SDC_CUR_MAP		ipu_ptr32(0xCC)
#define SDC_HOR_CONF		ipu_ptr32(0xD0)
#define SDC_VER_CONF		ipu_ptr32(0xD4)
#define SDC_SHARP_CONF_1	ipu_ptr32(0xD8)
#define SDC_SHARP_CONF_2	ipu_ptr32(0xDC)


/* Display Interface Regs */
#define DI_DISP_IF_CONF		ipu_ptr32(0x124)
#define DI_DISP_SIG_POL		ipu_ptr32(0x128)
#define DI_SER_DISP1_CONF	ipu_ptr32(0x12C)
#define DI_SER_DISP2_CONF	ipu_ptr32(0x130)
#define DI_HSP_CLK_PER		ipu_ptr32(0x134)
#define DI_DISP0_TIME_CONF_1	ipu_ptr32(0x138)
#define DI_DISP0_TIME_CONF_2	ipu_ptr32(0x13C)
#define DI_DISP0_TIME_CONF_3	ipu_ptr32(0x140)
#define DI_DISP1_TIME_CONF_1	ipu_ptr32(0x144)
#define DI_DISP1_TIME_CONF_2	ipu_ptr32(0x148)
#define DI_DISP1_TIME_CONF_3	ipu_ptr32(0x14C)
#define DI_DISP2_TIME_CONF_1	ipu_ptr32(0x150)
#define DI_DISP2_TIME_CONF_2	ipu_ptr32(0x154)
#define DI_DISP2_TIME_CONF_3	ipu_ptr32(0x158)
#define DI_DISP3_TIME_CONF	ipu_ptr32(0x15C)
#define DI_DISP0_DB0_MAP	ipu_ptr32(0x160)
#define DI_DISP0_DB1_MAP	ipu_ptr32(0x164)
#define DI_DISP0_DB2_MAP	ipu_ptr32(0x168)
#define DI_DISP0_CB0_MAP	ipu_ptr32(0x16C)
#define DI_DISP0_CB1_MAP	ipu_ptr32(0x170)
#define DI_DISP0_CB2_MAP	ipu_ptr32(0x174)
#define DI_DISP1_DB0_MAP	ipu_ptr32(0x178)
#define DI_DISP1_DB1_MAP	ipu_ptr32(0x17C)
#define DI_DISP1_DB2_MAP	ipu_ptr32(0x180)
#define DI_DISP1_CB0_MAP	ipu_ptr32(0x184)
#define DI_DISP1_CB1_MAP	ipu_ptr32(0x188)
#define DI_DISP1_CB2_MAP	ipu_ptr32(0x18C)
#define DI_DISP2_DB0_MAP	ipu_ptr32(0x190)
#define DI_DISP2_DB1_MAP	ipu_ptr32(0x194)
#define DI_DISP2_DB2_MAP	ipu_ptr32(0x198)
#define DI_DISP2_CB0_MAP	ipu_ptr32(0x19C)
#define DI_DISP2_CB1_MAP	ipu_ptr32(0x1A0)
#define DI_DISP2_CB2_MAP	ipu_ptr32(0x1A4)
#define DI_DISP3_B0_MAP		ipu_ptr32(0x1A8)
#define DI_DISP3_B1_MAP		ipu_ptr32(0x1AC)
#define DI_DISP3_B2_MAP		ipu_ptr32(0x1B0)
#define DI_DISP_ACC_CC		ipu_ptr32(0x1B4)
#define DI_DISP_LLA_CONF	ipu_ptr32(0x1B8)
#define DI_DISP_LLA_DATA	ipu_ptr32(0x1BC)

/* GPIO registers */
#define WAKUWAKU3_GPIO_REGBASE	0x53FD0000
#define WAKUWAKU3_GPIO_REGSIZE	0x04
#define GPIO_PSAVE		gpio_ptr32(0x00)

typedef struct {
	disp_surface_t	*bound_surf;
	disp_surface_t	*next_surf;
        uint32_t        offset;
        uint32_t        commit_offset;
        int             stride;
        unsigned        layer_format;
        uint32_t        chroma_color;
        disp_point_t    src1;
        disp_point_t    src2;
        disp_point_t    dst1;
        disp_point_t    dst2;
        int             bpp;
        int             alpha;
	int		enable;
	int		change;
	unsigned	update_flags;
	uint32_t	params[6];
} layer_info;

typedef struct imx35_context {
	disp_adapter_t	*adapter;
	unsigned	vidbase;
	unsigned	vidsize;
	int hw2daccel;
	uint16_t        *gpio_regptr;	/* WAKUWAKU */
	uint32_t	*regptr;
	uint32_t	*ccm_regptr;
	unsigned	panel_on;
	unsigned	vsync_counter;
	int		want_vsync_pulse;

	/* Layer stuff */
	layer_info	layer[2];
	uint32_t	sdc_com;
	unsigned	zero_size;
	uint8_t		*zero_ptr;
	unsigned	zero_base;
	unsigned	zero_stride;
	intrspin_t	spinlock;
	
        /* Interrupt/pulse stuff */
        int             intrid;
        int             vsync_chan;
        int             vsync_coid;
        struct sigevent vsync_event;

	/* Video input control */
	int		capture_enabled;
	int		capture_bound;
	void		*capture_buf;
	int		capture_buf_size;
	int		capture_buf_width;
	int		capture_buf_height;
	int		capture_enableme;
	int		capture_configured;
	unsigned	cap_offset;
	int		cap_stride;
	int		cap_update;

	/* hw cursor */
	int             cur_w;
	int             cur_h;
	int             hs_x;
	int             hs_y;

	/* Configurable options from .conf file */
	unsigned	xres;
	unsigned	yres;
	unsigned	refresh;
	unsigned	vsw;
	unsigned	vss;
	unsigned	vew;
	unsigned	hsw;
	unsigned	hss;
	unsigned	hew;
	unsigned	data_mask;
	unsigned	clk_idle;
	unsigned	clk_sel;
	unsigned	vsync_pol;
	unsigned	enable_pol;
	unsigned	data_pol;
	unsigned	clock_pol;
	unsigned	hsync_pol;	
	unsigned	sdc_mode;
	unsigned	sharp_conf1;
	unsigned	sharp_conf2;
	unsigned	irq;
	unsigned	output_fmt;
	unsigned	brightness;
	unsigned	cpu_freq;	/* in Mhz */
	unsigned	clk_down_wr;
	unsigned	clk_up_wr;
	unsigned	clk_per_wr;
	uint32_t	csi_sens_conf;	
	int		cap_test;
	uint32_t	ccir1;
	uint32_t	ccir2;
	uint32_t	ccir3;

	char ovgLibName[1024];
} imx35_context_t;

typedef struct imx35_draw_context
{
	disp_adapter_t *adapter;
	imx35_context_t *imx35;
	void *vidptr;
	int external_client;
	struct ovg_context* ovgContext;
} imx35_draw_context_t;

/* Interrupt functions */
const struct sigevent * imx35_isr(void *arg, int id);
void imx35_irq_cleanup (imx35_context_t *imx35);
int imx35_irq_setup (disp_adapter_t *adapter);

/* Miscellaneous draw-related functions */
int imx35_gui_init(disp_adapter_t *adapter, char *optstring);
void imx35_gui_fini(disp_adapter_t *adapter);
void imx35_module_info(disp_adapter_t *adapter, disp_module_info_t *info);

/* Internal function prototypes */
int get_config_data(imx35_context_t *q_ctx, const char *filename);

/* Memory function prototypes */
int devg_get_memfuncs(disp_adapter_t *adp, disp_memfuncs_t *funcs, int tabsize);
int imx35_mem_init(disp_adapter_t *adapter, char *optstring);
void imx35_mem_fini(disp_adapter_t *adapter);
int imx35_mem_reset(disp_adapter_t *adapter, disp_surface_t *surf);
disp_surface_t *imx35_alloc_surface(disp_adapter_t *adapter,
    int width, int height, unsigned format, unsigned flags, unsigned user_flags);
int imx35_free_surface(disp_adapter_t *adapter, disp_surface_t *surf);
unsigned long imx35_mem_avail(disp_adapter_t *adapter, unsigned flags);
void imx35_module_info(disp_adapter_t *adapter, disp_module_info_t *info);

int imx35_query_apertures(disp_adapter_t *adapter, disp_aperture_t *ap);
disp_surface_t *imx35_alloc_layer_surface(disp_adapter_t *adapter, int dispno[], int layer_idx[], int nlayers, unsigned format,
											int surface_index, int width, int height, unsigned sflags, unsigned hint_flags);
int imx35_query_surface(disp_adapter_t *adp, disp_surface_t *surf, disp_surface_info_t *info);
int imx35_get_alloc_info(disp_adapter_t *adapter, int width, int height, unsigned format,
							unsigned flags, unsigned user_flags, disp_alloc_info_t *info);
int imx35_get_alloc_layer_info(disp_adapter_t *adp, int dispno[], int layer_idx[], int nlayers,
								unsigned format, int surface_index, int width, int height,
								unsigned sflags, unsigned hint_flags, disp_alloc_info_t *info);

/* Miscellaneous functions */
void imx35_wait_idle(disp_draw_context_t * ctx);

/* Mode switcher (layer control) */
int imx35_layer_query(disp_adapter_t *adapter, int dispno, int layer_idx,
        int fmt_index, disp_layer_query_t * info);
int imx35_layer_enable(disp_adapter_t *adapter, int dispno, int layer_idx);
int imx35_layer_disable(disp_adapter_t *adapter, int dispno, int layer_idx);
int imx35_layer_set_surface(disp_adapter_t * adapter, int dispno,
    int layer_idx, unsigned layer_format, disp_surface_t *surfaces[]);
int imx35_layer_set_source_viewport(disp_adapter_t *adapter, int dispno, int layer_idx,
        int x1, int y1, int x2, int y2);
int imx35_layer_set_dest_viewport(disp_adapter_t *adapter, int dispno, int layer_idx,
        int x1, int y1, int x2, int y2);
int imx35_layer_set_blending(disp_adapter_t *adapter, int dispno, int layer_idx,
        unsigned alpha_mode, int m1, int m2);
int imx35_layer_set_chromakey(disp_adapter_t *adapter, int dispno, int layer_idx,
        unsigned chroma_mode, disp_color_t color0, disp_color_t color1, disp_color_t mask);
int imx35_layer_set_flags(disp_adapter_t *adapter, int dispno, int layer_idx,
        unsigned flag_mask, unsigned flag_values);
void imx35_layer_update_begin(disp_adapter_t *adapter, int dispno, uint64_t layer_mask);
void imx35_layer_update_end(disp_adapter_t *adapter, int dispno, uint64_t layer_mask,
        int wait_vsync);
void imx35_layer_reset(disp_adapter_t *adapter, int dispno, int layer_idx);
int imx35_wait_vsync(disp_adapter_t *adapter, int dispno);
void imx35_layer_program(imx35_context_t *imx35, int layer_idx, int wait_vsync);

void imx35_blit2(disp_draw_context_t *ctx, disp_surface_t *src, disp_surface_t *dst,
								 int sx, int sy, int dx, int dy, int width, int height);

/* hw cursor */
int imx35_set_hw_cursor(disp_adapter_t * ctx, int dispno, uint8_t * fg_bmp, uint8_t * bg_bmp, unsigned color0, 
	unsigned color1, int hotspot_x, int hotspot_y, int size_x, int size_y, int bmp_stride);
void imx35_enable_hw_cursor(disp_adapter_t * ctx, int dispno);
void imx35_disable_hw_cursor(disp_adapter_t * ctx, int dispno);
void imx35_set_hw_cursor_pos(disp_adapter_t * ctx, int dispno, int x, int y);

int mcu_setup (disp_adapter_t *adapter, int enable);

int imx35_load_ovg_ifs(imx35_context_t *imx35);
void imx35_close_ovg_ifs();

ovg_backend_interface_t g_ovg_backend_funcs;
void *g_ovg_libHandle;

#endif /* __IMX35_INCLUDED */


