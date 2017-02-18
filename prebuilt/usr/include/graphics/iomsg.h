/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, QNX Software Systems.  
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

#ifndef _GRAPHICS_IOMSG_H_INCLUDED
#define _GRAPHICS_IOMSG_H_INCLUDED

#ifndef _WIN32
#include <sys/iomsg.h>
#include <_pack64.h>
#else
	#ifdef _MSC_VER
	typedef int pid_t;
	#endif
#endif

#include <graphics/display.h>

#ifndef _IOMGR_DISPLAY
#define _IOMGR_DISPLAY	0x15
#endif

/*
 * This version number is used to detect version mismatches between the
 * GF lib and io-display.  At every release, the number should be incremented
 */
#define IOD_VERSION			0x00000003

#define DISP_SURFACE_ID_INVALID		(~0)

#define DISP_IOMSG_SIZEOF(u_type)	(sizeof (io_msg_t) + sizeof (u_type))

/*
 * We want to make the surface shareable with other clients, which means
 * it needs to be allocated inside of a shared memory object.
 */
#define IOD_ALLOC_FLAG_SHAREABLE	0x00000001
/*
 * Do not attempt to allocate the memory internally.  The client will
 * allocate the memory.
 */
#define IOD_ALLOC_FLAG_NO_ALLOC		0x00000002

#define IOD_ALLOC_METHOD_APERTURE	0
#define IOD_ALLOC_METHOD_EXTERNAL	1
#define IOD_ALLOC_METHOD_SHMEM		2

#define IOD_LAYER_UPDATE_ENABLED	0x00000001
#define IOD_LAYER_UPDATE_SURFACES	0x00000004
#define IOD_LAYER_UPDATE_SRC_VIEWPORT	0x00000008
#define IOD_LAYER_UPDATE_DST_VIEWPORT	0x00000010
#define IOD_LAYER_UPDATE_BLENDING	0x00000020
#define IOD_LAYER_UPDATE_CHROMA		0x00000040
#define IOD_LAYER_UPDATE_BRIGHTNESS	0x00000080
#define IOD_LAYER_UPDATE_CONTRAST	0x00000100
#define IOD_LAYER_UPDATE_SATURATION	0x00000200
#define IOD_LAYER_UPDATE_FLAGS		0x00000400	/* Filtering, wrapping etc. */
#define IOD_LAYER_UPDATE_OUTPUTS	0x00000800
#define IOD_LAYER_UPDATE_HUE		0x00001000

#define IOD_VCAP_UPDATE_ENABLED		0x00000001
#define IOD_VCAP_UPDATE_SRC_DIM		0x00000002
#define IOD_VCAP_UPDATE_SRC_CROP	0x00000004
#define IOD_VCAP_UPDATE_DST_DIM		0x00000008
#define IOD_VCAP_UPDATE_BRIGHTNESS	0x00000010
#define IOD_VCAP_UPDATE_CONTRAST	0x00000020
#define IOD_VCAP_UPDATE_SATURATION	0x00000040
#define IOD_VCAP_UPDATE_FLAGS		0x00000080	/* Bob, Weave etc. */
#define IOD_VCAP_UPDATE_INPUT_SOURCE	0x00000100
#define IOD_VCAP_UPDATE_BINDING		0x00000200
#define IOD_VCAP_UPDATE_SYNC_VALUES	0x00000400
#define IOD_VCAP_UPDATE_HUE		0x00000800

#define IOD_DEVCTL_DUMP_MEMORY		0x200

typedef struct {
	unsigned	update_flags;	/* See IOD_LAYER_UPDATE_* */
	int		enabled;
	unsigned	layer_format;
	unsigned	sid[4], nsids;
	struct {
			int	x1;
			int	y1;
			int	x2;
			int	y2;
	} src_vp;
	struct {
			int	x1;
			int	y1;
			int	x2;
			int	y2;
	} dst_vp;
	struct {
			unsigned	mode;
			int		m1;
			int		m2;
			unsigned	map_sid;
	} blending;
	struct {
			unsigned	mode;
			disp_color_t	color0;
			disp_color_t	color1;
			disp_color_t	mask;
	} chroma;
	int		brightness;
	int		contrast;
	int		saturation;
	int		hue;
	unsigned	flag_mask;
	unsigned	flag_values;
	uint32_t	output_mask;
} disp_layer_state_t;

#ifdef IOD_SOCKET_API
typedef struct {
	int rc;
	int size;
} disp_iomsg_hdr_t;
#endif

#ifdef _WIN32
/*
 * Message of _IO_MSG
 */
struct _io_msg {
	_Uint16t					type;
	_Uint16t					combine_len;
	_Uint16t					mgrid;		/* manager id (sys/iomgr.h) */
	_Uint16t					subtype;	/* manager specific subtype */
};

typedef union {
	struct _io_msg				i;
} io_msg_t;
#endif

typedef struct {
	unsigned	update_flags;	/* See IOD_VCAP_UPDATE_* */
	int		enabled;
	int		input_source;
	int		src_width;
	int		src_height;
	struct {
			int	x1;
			int	y1;
			int	x2;
			int	y2;
	} src_vp;
	int		dst_width;
	int		dst_height;
	int		h_offset;
	int		v_offset;
	int		h_total;
	int		v_total;
	int		brightness;
	int		contrast;
	int		saturation;
	unsigned	flags;
	int		bound_display;
	int		bound_layer;
	int		hue;
} disp_vcap_state_t;

typedef struct {
	io_msg_t	iomsg;
	union {
#define IOD_QUERY_DEVICE		0x0001
		struct {
			unsigned unused;
		} query_dev;
#define IOD_QUERY_DISPLAY		0x0002
		struct {
			int		dispno;
		} query_disp;
#define IOD_REGISTER			0x0003
		struct {
			unsigned	iod_version;
		} reg;
#define IOD_UNREGISTER			0x0004
		struct {
			unsigned unused;
		} unreg;
#define IOD_GET_STRINGS			0x0006
		struct {
			unsigned unused;
		} get_strings;
#define IOD_ALLOC_SURFACE		0x0007
		struct {
			unsigned	flags;		/* See IOD_ALLOC_FLAG_* */
			unsigned	surface_flags;	/* See DISP_SURFACE_* */
			unsigned	hint_flags;	/* See DISP_VMEM_HINT_* */
			unsigned	format;		/* See DISP_SURFACE_FORMAT_* */
			int		width;
			int		height;
			int		hw_locked;
		} alloc_surface;
#define IOD_ALLOC_LAYER_SURFACE		0x0008
		struct {
			unsigned	flags;		/* See IOD_ALLOC_FLAG_* */
			unsigned	surface_flags;	/* See DISP_SURFACE_* */
			unsigned	hint_flags;	/* See DISP_VMEM_HINT_* */
			int		width;
			int		height;
			int		dispno[16];
			int		layer_idx[16];
			int		nlayers;
			unsigned	layer_format;
			int		surface_idx;
			int		hw_locked;
		} alloc_layer_surface;
#define IOD_FREE_SURFACE		0x0009
		struct {
			unsigned	surface_id;
			int		hw_locked;
		} free_surface;
#define IOD_LAYER_UPDATE		0x000a
		struct {
			int		dispno;
			int		nlayers;
			int		wait_vsync;
			int		wait_idle;
		} layer_update;
#define IOD_LAYER_SET_ORDER		0x000b
		struct {
			int		dispno;
			int		wait_vsync;
			int		wait_idle;
			/* array of unsigned follows the msg */
		} layer_set_order;
#define IOD_SWAP_BUFFERS		0x000c
		struct {
			int		dispno;
			int		layer;
			int		wait_vsync;
			unsigned	surface_id[4];
		} swap_buffers;
#define IOD_GET_SURFACE_BY_SID		0x000d
		struct {
			unsigned	sid;
		} get_surface_by_sid;
#define IOD_QUERY_LAYER			0x000e
		struct {
			int		dispno;
			int		layer_index;
			int		format_index;
		} query_layer;
#define IOD_WAIT_IDLE			0x000f
		struct {
			unsigned unused;
		} wait_idle;
#define IOD_CURSOR_SET_BITMAP	0x0010
		struct {
			int dispno;
			int cursor_index;
			unsigned fg_color, bg_color;
			int hotspot_x, hotspot_y;
			int size_x, size_y;
			/* bitmap data follows the msg */
		} cursor_bitmap;
#define IOD_CURSOR_SET_POS		0x0011
		struct {
			int dispno;
			int cursor_index;
			int x, y;
		} cursor_pos;
#define IOD_CURSOR_SET_ENABLED	0x0012
		struct {
			int dispno;
			int cursor_index;
			int enabled;
		} cursor_enable;
#define IOD_LAYER_ATTACH	0x0013
		struct {
			int	dispno;
			int	layer_index;
			int	noautodisable;
			int	do_reset;
		} layer_attach;
#define IOD_LAYER_DETACH	0x0014
		struct {
			int	dispno;
			int	layer_index;
		} layer_detach;
#define IOD_I2C_WRITE		0x0015
		struct {
			int	busno;
			int	slaveaddr;
			uint8_t	data[512];
			int	nbytes;
		} i2c_write;
#define IOD_I2C_READ		0x0016
		struct {
			int	busno;
			int	slaveaddr;
			int	nbytes;
		} i2c_read;
#define IOD_I2C_WRITEREAD	0x0017
		struct {
			int	busno;
			int	slaveaddr;
			uint8_t	odata[512];
			int	obytes;
			int	ibytes;
		} i2c_writeread;
#define IOD_VCAP_UPDATE		0x0018
		struct {
			int			unit;
			int			wait_vsync;
			disp_vcap_state_t	state;
		} vcap_update;
#define IOD_DPMS_MODE		0x0019
		struct {
			int			dispno;
			unsigned		mode;
		} dpms_mode;
#define IOD_SNAPSHOT		0x001a
		struct {
			int			dispno;
			int			output;
			unsigned		sid;
			int			x1, y1, x2, y2;
		} snapshot;
#define IOD_QUERY_MODE		0x001b
		struct {
			int			dispno;
			int			index;
		} query_mode;
#define IOD_SET_MODE		0x001c
		struct {
			int			dispno;
			int			xres;
			int			yres;
			int			refresh;
			unsigned		format;
			unsigned		flags;
		} set_mode;
#define IOD_QUERY_SIDLIST	0x001d
		struct {
			int			idx_offset;
		} query_sidlist;
#define IOD_MEM_INFO		0x001e
		struct {
			int		unused;
		} query_mem;
#define IOD_EXTERNAL_CHROMA	0x001f
		struct {
			int		dispno;
			unsigned	mode;
			unsigned	color0;
			unsigned	color1;
			unsigned	mask;
		} external_chroma;
#define IOD_DEVCTL		0x0020
		struct {
			int		dispno;
			int		cmd;
			int		obytes;
			int		ibytes;
			uint8_t		data[512];
		} devctl;
#define IOD_WAIT_VSYNC		0x0021
		struct {
			int		dispno;
		} wait_vsync;
#define IOD_COLOR_LUT16                0x0022
		struct {
			int		dispno;
			uint16_t	redLUT[256];
			uint16_t	greenLUT[256];
			uint16_t	blueLUT[256];
		} color_lut16;
#define IOD_LAYER_GET_ORDER		0x0024
		struct {
			int		dispno;
		} layer_get_order;
#define IOD_LAYER_GET_STATE		0x0025
		struct {
			int		dispno;
			int		layer_index;
		} layer_get_state;
#define IOD_LAYER_FLUSHRECT		0x0026
		struct {
			int		dispno;
			int		layer_index;
			int		x1, y1, x2, y2;
		} layer_flushrect;
	} u;
} disp_iomsg_t;

typedef struct {
	union {
		struct {
			int		ndisplays;
			int		flags;			/* See IOD_QUERY_DEV_FLAG_* */
		} query_dev;
		struct {
			int		cur_xres;
			int		cur_yres;
			int		cur_refresh;
			unsigned	cur_main_layer_format;
			int		nlayers;
			int		main_layer;
			int		display_index;
		} query_disp;
		struct {
			int		n_apertures;
			int		string_sizes;
			unsigned	vid;
			unsigned	did;
			unsigned	deviceindex;
			struct {
				char		name[32];
				unsigned	flags;		/* See DISP_APER_* */
			} aperture[DISP_MAX_APERTURES];
			int		need_io_privity;
		} reg;
		struct {
			unsigned		method;		/* See IOD_ALLOC_METHOD_* */
			unsigned		surface_id;
			union {
				struct {
					disp_surface_t		surf;
					disp_surface_info_t	info;
				} aperture;
				disp_alloc_info_t	external_alloc_info;
				struct {
					char		shmem_name[32];
					unsigned	start_align;
					unsigned	stride;
					unsigned	surface_flags;
					unsigned	prot_flags;
					unsigned	map_flags;
					unsigned	pixel_format;
					unsigned	width;
					unsigned	height;
				} shmem;
			} u;
			unsigned		servergen;
		} alloc_surface;
		struct {
			disp_layer_query_t	info;
			int			enabled;
		} query_layer;
		struct {
			uint8_t	data[512];
		} i2c_read;
		struct {
			unsigned	flags;
			unsigned	primary_format;
			unsigned	xres;
			unsigned	yres;
			unsigned	refresh[DISP_MODE_NUM_REFRESH];
		} query_mode;
		struct {
			unsigned	list[10];
			unsigned	num;
			unsigned	idx_num;
		} query_sidlist;
		struct {
			unsigned	total_mem;
			unsigned	avail_mem;
		} query_mem;
		struct {
			int		bytes_returned;
			uint8_t		data[512];
		} devctl;
		struct {
			unsigned	layer_idx[128];
		} layer_get_order;
		struct {
			disp_layer_state_t	state;
		} layer_get_state;
	} u;
} disp_iomsg_reply_t;

typedef struct {
	gfpr_mutex_t	mutex;
	pid_t		last_pid;
	void		*last_context;
	unsigned	server_generation;
	unsigned	reserved[59];
} disp_shmem_t;

#ifndef _WIN32
#include <_packpop.h>
#endif

#endif /* _GRAPHICS_IOMSG_H_INCLUDED */
