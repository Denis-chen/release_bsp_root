/*
 * $QNXLicenseC: 
 * Copyright 2008,2009, QNX Software Systems.  
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

 
#include "sdma.h"

/////////////
// Defines //
/////////////

#define MAX_DESCRIPTORS                     1024

#define SDMA_CHN0ADDR_SMSZ_MASK             0x4000

/////////////////
// global vars //
/////////////////
uintptr_t sdma_base;
volatile sdma_ccb_t * ccb_ptr;

////////////////
// local vars //
////////////////

// lib init parameters
static int irq = SDMA_IRQ;    
static int regphys = SDMA_BASE;
static char * sdma_opts[] = {
    "irq",
    "regbase",
    NULL
};

static char * channel_create_opts[] = {
    "eventnum",
    "watermark",
    "fifopaddr",
    "regen",
    "contloop",
    NULL
};

// mutex
static pthread_mutex_t processinit_mutex = PTHREAD_MUTEX_INITIALIZER;
static int n_users_in_process = 0;

// List of chan contextes so that ISR callbacks can access contextes
static sdma_chan_t * chan_ptr_list[SDMA_N_CH] = {NULL};

////////////////////////////////////////////////////////////////////////////////
//                            PRIVATE FUNCTIONS                               //
////////////////////////////////////////////////////////////////////////////////

// Create Channel Control Block for channel 0 - 31
// CCB must be created in physical memory that must be accessible from multiple
// intances of the library.
// Return 0 = OK, -1 = ERROR

int ccb_create() {
    int status;    
    off64_t paddr64;
    uint64_t physical;
    unsigned size = SDMA_N_CH * sizeof(sdma_ccb_t);
    

    
    if ( sdmasync_is_first_process() ) {
        // ccb buffer doesn't exist, so create it
        ccb_ptr = mmap(     0,
                            size,
                            PROT_READ|PROT_WRITE|PROT_NOCACHE,
                            MAP_PHYS|MAP_ANON|MAP_PRIVATE,
                            NOFD,
                            0   );
        if (ccb_ptr == MAP_FAILED) {
            goto fail1;
        }
        status = mem_offset64(   (void*) ccb_ptr,
                                 NOFD,
                                 size,
                                 &paddr64,
                                 0  );
        if (status != 0) {
            goto fail2;
        }

        memset((void*)ccb_ptr,0,size);
        // associate CCB with SDMA
        out32(  sdma_base + SDMA_MC0PTR, (uint32_t) paddr64);
    } else {
        // ccb buffer already exists... so associate with current CCB buffer
        physical = in32(sdma_base + SDMA_MC0PTR);
        
        ccb_ptr = mmap_device_memory(   NULL,
                                        size,
                                        PROT_READ|PROT_WRITE|PROT_NOCACHE,
                                        0,
                                        physical        );
        if (ccb_ptr == MAP_FAILED) {
            goto fail1;
        }
    }
    return 0;
fail2:
    munmap((void*)ccb_ptr,size);
fail1:
    return -1;
}

void ccb_destroy() {
    munmap((void*)ccb_ptr,SDMA_N_CH * sizeof(sdma_ccb_t));
};

// Create all the data structures required by the channel.
sdma_chan_t * chan_create(unsigned ch_num) {
    sdma_chan_t * chan_ptr;
    int status;
    off64_t paddr64;
    
    // create channel control structure
    chan_ptr = (sdma_chan_t *) calloc(1,sizeof(sdma_chan_t));
    if (chan_ptr == NULL) {
        goto fail1;
    }
    
    /* Create buffer descriptors for channel in physically contigous RAM */
    chan_ptr->bd_ptr = mmap(      0,
                                  sizeof(sdma_bd_t) * MAX_DESCRIPTORS,
                                  PROT_READ|PROT_WRITE|PROT_NOCACHE,
                                  MAP_PHYS|MAP_ANON|MAP_PRIVATE,
                                  NOFD,
                                  0          );
    if (chan_ptr->bd_ptr == MAP_FAILED) {
        goto fail2;
    }
    status = mem_offset64(   (void*) chan_ptr->bd_ptr,
                             NOFD,
                             sizeof(sdma_bd_t) * MAX_DESCRIPTORS,
                             &paddr64,
                             0  );
                             
     if (status != 0) {
         goto fail3;
     }
     // associate newly created descriptor with ccb
     ccb_ptr[ch_num].base_bd_paddr = (uint32_t) paddr64;
     ccb_ptr[ch_num].current_bd_paddr = (uint32_t) paddr64;
     
    /*
     * Create SDMA channel context image which get loaded into SDMA
     *  Context RAM 
     */
    
    chan_ptr->ctx_ptr = mmap(  0,
                               sizeof(sdma_ch_ctx_t),
                               PROT_READ|PROT_WRITE|PROT_NOCACHE,
                               MAP_PHYS|MAP_ANON|MAP_PRIVATE,
                               NOFD,
                               0          );
    if (chan_ptr->ctx_ptr == MAP_FAILED) {
        goto fail3;
    }
    status = mem_offset64(   (void *) chan_ptr->ctx_ptr,
                             NOFD,
                             sizeof(sdma_ch_ctx_t),
                             &paddr64,
                             0      );
    if (status != 0) {
        goto fail4;
    }
    chan_ptr->ctx_paddr = (uint32_t) paddr64;
    
    chan_ptr_list[ch_num] = chan_ptr;
    return chan_ptr;
fail4:
    munmap((void *) chan_ptr->ctx_ptr, sizeof(sdma_ch_ctx_t));
fail3:
    munmap((void*)chan_ptr->bd_ptr, sizeof(sdma_bd_t) * MAX_DESCRIPTORS);
fail2:
    free(chan_ptr);
fail1:
    return NULL;
}

void chan_destroy(sdma_chan_t * chan_ptr) {
    chan_ptr_list[chan_ptr->ch_num] = NULL;
    munmap((void*) chan_ptr->ctx_ptr, sizeof(sdma_ch_ctx_t));
    munmap((void*)chan_ptr->bd_ptr, sizeof(sdma_bd_t) * MAX_DESCRIPTORS);
    free(chan_ptr);
}


void register_init() {
    int i;
    
    if ( sdmasync_is_first_process()  ) {
        out32(sdma_base + SDMA_RESET , 1); 
        nanospin_ns(10000);
        out32(sdma_base + SDMA_INTR,0xffffffff); // clr irqs

        // by default, all dmas are software triggered.
        out32(sdma_base + SDMA_EVTOVR , 0xffffffff);
        
        // set context size to 32
        out32(  sdma_base + SDMA_CHN0ADDR ,
                in32(sdma_base + SDMA_CHN0ADDR) | SDMA_CHN0ADDR_SMSZ_MASK);

        for(i=0; i < SDMA_N_CH; i++) {
            out32(sdma_base + SDMA_CHNENBL(i),0x0);
        }
    }
}

int
parse_init_options(const char *options)
{
    char    *value;
    int     opt;
    char    *temp_ptr = NULL;
    
    if (options) {
        temp_ptr = strdup(options);
    } 

    while (temp_ptr && *temp_ptr != '\0') {
        if ((opt = getsubopt(&temp_ptr, sdma_opts, &value)) == -1)
            return -1;

        switch (opt) {
            case 0:
                irq = strtoul(value, 0, 0);
                break;
            case 1:
                regphys = strtoul(value, 0, 0);
                break;
            default:
                return -1;
        }
    }

    return 0;
}
int parse_channel_options(sdma_chan_t * chan_ptr,const char *options) {
    char    *value;
    int     opt;
    char    *temp_ptr = NULL;
    
    if (options) {
        temp_ptr = strdup(options);
    } 

    while (temp_ptr && *temp_ptr != '\0') {
        
        if ((opt = getsubopt(&temp_ptr, channel_create_opts, &value)) == -1) {
            return -1;
        }

        switch (opt) {
            case 0:
                chan_ptr->eventnum = strtoul(value, 0, 0);
                chan_ptr->is_event_driven = 1;
                break;
            case 1:
                chan_ptr->watermark = strtoul(value, 0, 0);
                break;
            case 2:
                chan_ptr->fifo_paddr = strtoul(value, 0, 0);
                break;
            case 3:
                chan_ptr->regen_descr = 1;
                break;
            case 4:
                chan_ptr->cont_descr_loop = 1;
                break;
            default:
                return -1;
        }
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
//                               ISR CALLBACKS                                //
////////////////////////////////////////////////////////////////////////////////

void callback_reenable_descr(unsigned ch_num) {
    int i;
    sdma_chan_t * chan_ptr = chan_ptr_list[ch_num];
    
    for(i=0; i < chan_ptr->n_frags; i++) {
        chan_ptr->bd_ptr[i].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;
    }
}

////////////////////////////////////////////////////////////////////////////////
//                                   API                                      //
////////////////////////////////////////////////////////////////////////////////

int sdma_init(const char *options) {

    pthread_mutex_lock(&processinit_mutex);
    n_users_in_process++;
        
    // Only need to initialize once per process
    if (n_users_in_process == 1) {

        if ( parse_init_options(options) !=0 ) {
            goto fail1;
        }
        // init multi-process saftey
        if ( sdmasync_init() != 0) {   
            goto fail1;
        }
        
        sdma_base = mmap_device_io(SDMA_SIZE,regphys); 
        if (sdma_base == (uintptr_t)MAP_FAILED) {
            goto fail2;
        }

        if ( sdmairq_init(irq) != 0 ) {
            goto fail3;
        }
        
        // allow only 1 process to init the sdma shared components at a time
        pthread_mutex_lock( sdmasync_libinit_mutex_get() );
        sdmasync_process_cnt_incr();

        register_init();
        if ( ccb_create() != 0 ) {
            goto fail4;
        }
        if ( sdmacmd_cmdch_create() != 0 ) {
            goto fail5;
        }
        pthread_mutex_unlock( sdmasync_libinit_mutex_get() ); // done share init
    }
    pthread_mutex_unlock(&processinit_mutex);    

    return 0;

// Cleanup on error        
fail5:
    ccb_destroy();    
fail4:
    sdmasync_process_cnt_decr();
    pthread_mutex_unlock( sdmasync_libinit_mutex_get() );
    sdmairq_fini();
fail3:
    munmap_device_io(sdma_base,SDMA_SIZE);
fail2:
    sdmasync_fini();
fail1:
    n_users_in_process--;
    pthread_mutex_unlock(&processinit_mutex);
    return -1;
}

void sdma_fini() {
    
    pthread_mutex_lock(&processinit_mutex);
    n_users_in_process--;

    // if we are the last lib user in this process, then cleanup
    if (n_users_in_process == 0) {
 
        pthread_mutex_lock( sdmasync_libinit_mutex_get() );
        sdmasync_process_cnt_decr();
        
        sdmacmd_cmdch_destroy();
        ccb_destroy();    
        pthread_mutex_unlock( sdmasync_libinit_mutex_get() );
        sdmairq_fini();
        munmap_device_io(sdma_base,SDMA_SIZE);
        sdmasync_fini();
    }
    pthread_mutex_unlock(&processinit_mutex);
}


void
sdma_query_channel(void *handle, dma_channel_query_t *chinfo)
{
    sdma_chan_t * chan_ptr = handle;

    chinfo->chan_idx = chan_ptr->ch_num;
    chinfo->irq = irq;
}

int
sdma_driver_info(dma_driver_info_t *info)
{
    info->dma_version_major = DMALIB_VERSION_MAJOR;
    info->dma_version_minor = DMALIB_VERSION_MINOR;
    info->dma_rev = DMALIB_REVISION;
    info->lib_rev = 0;
    info->description = SDMA_DESCRIPTION_STR;
    info->num_channels = SDMA_N_CH - 1;

    return 0;
}

int
sdma_channel_info(unsigned channel, dma_channel_info_t *info)
{
    info->max_xfer_size = 0xfffc;
    info->max_src_fragments = MAX_DESCRIPTORS;
    info->max_dst_fragments = MAX_DESCRIPTORS;
    info->max_src_segments = MAX_DESCRIPTORS;
    info->max_dst_segments = MAX_DESCRIPTORS;
    info->xfer_unit_sizes = 0x4;    
    info->caps =    DMA_CAP_EVENT_ON_COMPLETE |
                    DMA_CAP_EVENT_PER_SEGMENT |
                    DMA_CAP_DEVICE_TO_MEMORY | 
                    DMA_CAP_MEMORY_TO_DEVICE |
                    DMA_CAP_MEMORY_TO_MEMORY;
    info->mem_lower_limit = 0;
    info->mem_upper_limit = 0xffffffff;
    info->mem_nocross_boundary = 0;
    return 0;
}


void *
sdma_channel_attach(const char *optstring,
    const struct sigevent *event, unsigned *channel, int prio, unsigned flags)
{
    sdma_chan_t * chan_ptr;
    unsigned ch_type = *channel;
    rsrc_request_t    req;
    unsigned ch_num;
    uint32_t chnenbl_reg;
    
    // get free channel from resource database manager
    memset(&req,0,sizeof(rsrc_request_t));
    req.start = SDMA_CH_LO;
    req.end = SDMA_CH_HI;
    req.length = 1;
    req.flags = RSRCDBMGR_DMA_CHANNEL | RSRCDBMGR_FLAG_RANGE;
    if (rsrcdbmgr_attach(&req, 1) != EOK) {
        goto fail1;
    }
    ch_num = req.start;
    
    // clear interrupts on this channel just in case it may still be set
    // from a previous driver crash or something.
    out32(sdma_base + SDMA_INTR,(1 << ch_num));
    
    // create channel control struct, and populate with channel data structures
    chan_ptr = chan_create(ch_num);
    if (chan_ptr == NULL) {
        goto fail2;
    }
    // init channel struct
    chan_ptr->ch_num = ch_num;
    chan_ptr->attach_flags = flags;
    chan_ptr->ch_type = ch_type;

    // parse the opstring for additional configurations
    if ( parse_channel_options(chan_ptr,optstring) != 0 ) {
        goto fail3;
    }

    // configure and load the SDMA conrtext into private SDMA memspace
    sdmacmd_ctx_config(chan_ptr);
    if ( sdmacmd_ctx_load(chan_ptr) != 0) {
        goto fail3;
    }
    //set channel priority
    if (flags == DMA_ATTACH_PRIORITY_HIGHEST || prio > SDMA_CH_PRIO_HI ) {
        prio = SDMA_CH_PRIO_HI;
    } else if (prio < SDMA_CH_PRIO_LO) {
        prio = SDMA_CH_PRIO_LO;
    }
    out32(sdma_base + SDMA_CHNPRI(ch_num) , prio);
    
    if (chan_ptr->is_event_driven) {
        out32(  sdma_base + SDMA_EVTOVR, 
                in32(sdma_base + SDMA_EVTOVR) & ~(1 << chan_ptr->ch_num));
                
        chnenbl_reg = in32(sdma_base + SDMA_CHNENBL(chan_ptr->eventnum));
        chnenbl_reg |= 1 << ch_num;
        out32(sdma_base + SDMA_CHNENBL(chan_ptr->eventnum), chnenbl_reg);
    }
    
    // attach OS event to the channel interrupt
    sdmairq_event_add(ch_num,event);
    
    // Attach a callback to the isr to automatically regenerate the descriptors
    // for a given channel on an interrupt.  This enables the user to only call
    // sdma_setup_xfer() once per channel if desired
    if (chan_ptr->regen_descr) {
        sdmairq_callback_add(ch_num, callback_reenable_descr );
    } 
    
    return chan_ptr;
fail3:
    chan_destroy(chan_ptr);    
fail2:
    req.length = 1;
    req.start = req.end = chan_ptr->ch_num;
    req.flags = RSRCDBMGR_DMA_CHANNEL;
    rsrcdbmgr_detach(&req, 1);
fail1:
    return NULL;
}


void
sdma_channel_release(void * handle) {
    sdma_chan_t * chan_ptr = handle;
    rsrc_request_t    req = { 0 };
    unsigned ch_num = chan_ptr->ch_num;
    uint32_t chnenbl_reg;

    // clear interrupts on this channel
    out32(sdma_base + SDMA_INTR,(1 << ch_num));
    
    // deactivate dma-events, default to software triggered
    
    pthread_mutex_lock( sdmasync_regmutex_get() );
    out32(  sdma_base + SDMA_EVTOVR, 
            in32(sdma_base + SDMA_EVTOVR) | (1 << ch_num));
    out32(  sdma_base + SDMA_HOSTOVR, 
            in32(sdma_base + SDMA_HOSTOVR) & ~(1 << ch_num));
            
    chnenbl_reg = in32(sdma_base + SDMA_CHNENBL(chan_ptr->eventnum));
    chnenbl_reg &= ~(1 << ch_num);
    out32(sdma_base + SDMA_CHNENBL(chan_ptr->eventnum), chnenbl_reg);
    
    
    pthread_mutex_unlock( sdmasync_regmutex_get() );
    
     if (chan_ptr->regen_descr) {
         sdmairq_callback_remove(chan_ptr->ch_num);
     } 
     
    sdmairq_event_remove(chan_ptr->ch_num);
    chan_destroy(chan_ptr);    
    
    req.length = 1;
    req.start = req.end = chan_ptr->ch_num;
    req.flags = RSRCDBMGR_DMA_CHANNEL;
    rsrcdbmgr_detach(&req, 1);
}

int
sdma_setup_xfer(void *handle, const dma_transfer_t *tinfo) {
    sdma_chan_t * chan_ptr = handle;
    volatile sdma_bd_t * bd_ptr = chan_ptr->bd_ptr;
    unsigned cmd_and_status;
    unsigned n_frags;
    int i;

    //////////////////////////////////////////////
    // FILL-IN BUFFER DESCRIPTOR TO GOVERN XFER //
    //////////////////////////////////////////////
    // calculate descriptor parameters
    cmd_and_status =    SDMA_CMDSTAT_EXT_MASK | 
                        SDMA_CMDSTAT_CONT_MASK |
                        SDMA_CMD_XFER_SIZE(tinfo->xfer_unit_size);
    
    if (chan_ptr->attach_flags & DMA_ATTACH_EVENT_PER_SEGMENT) {
        cmd_and_status |= SDMA_CMDSTAT_INT_MASK;
    } 
    
    switch(chan_ptr->ch_type) {
    
    case SDMA_CHTYPE_AP_2_MCU:
    case SDMA_CHTYPE_SHP_2_MCU: 
    case SDMA_CHTYPE_SPDIF_2_MCU:
        n_frags = tinfo->dst_fragments;
        for(i=0; i < n_frags-1; i++) {
            bd_ptr[i].cmd_and_status = cmd_and_status;
            bd_ptr[i].cmd_and_status |= tinfo->dst_addrs[i].len;
            bd_ptr[i].buf_paddr = (uint32_t) (tinfo->dst_addrs[i].paddr);
            // enable the descriptor last
            bd_ptr[i].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;
        }
        
        // Fill in last descriptor
        bd_ptr[n_frags-1].cmd_and_status =  SDMA_CMDSTAT_EXT_MASK |
                                            SDMA_CMD_XFER_SIZE(tinfo->xfer_unit_size) |
                                            SDMA_CMDSTAT_WRAP_MASK | 
                                            tinfo->dst_addrs[n_frags-1].len;

        if (chan_ptr->cont_descr_loop) { 
            bd_ptr[n_frags-1].cmd_and_status |= SDMA_CMDSTAT_CONT_MASK;
        }
        if  (   (chan_ptr->attach_flags & DMA_ATTACH_EVENT_ON_COMPLETE) || 
                (chan_ptr->attach_flags & DMA_ATTACH_EVENT_PER_SEGMENT) ) {
            bd_ptr[n_frags-1].cmd_and_status |= SDMA_CMDSTAT_INT_MASK;
        }
        bd_ptr[n_frags-1].buf_paddr = 
            (uint32_t) (tinfo->dst_addrs[n_frags-1].paddr);
        
        // enable the descr last
        bd_ptr[n_frags-1].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;
        break;
    
    
    case SDMA_CHTYPE_MCU_2_AP:
    case SDMA_CHTYPE_MCU_2_SHP:
    case SDMA_CHTYPE_MCU_2_SPDIF:
        n_frags = tinfo->src_fragments;
        for(i=0; i < n_frags-1; i++) {
            bd_ptr[i].cmd_and_status = cmd_and_status;
            bd_ptr[i].cmd_and_status |= tinfo->src_addrs[i].len;
            bd_ptr[i].buf_paddr = (uint32_t) (tinfo->src_addrs[i].paddr);
            bd_ptr[i].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;
        }

        // Fill in last descriptor
        bd_ptr[n_frags-1].cmd_and_status =  SDMA_CMDSTAT_EXT_MASK |
                                            SDMA_CMD_XFER_SIZE(tinfo->xfer_unit_size) |
                                            SDMA_CMDSTAT_WRAP_MASK | 
                                            tinfo->src_addrs[n_frags-1].len;

        if (chan_ptr->cont_descr_loop) { 
            bd_ptr[n_frags-1].cmd_and_status |= SDMA_CMDSTAT_CONT_MASK;
        }
        if  (   (chan_ptr->attach_flags & DMA_ATTACH_EVENT_ON_COMPLETE) || 
                (chan_ptr->attach_flags & DMA_ATTACH_EVENT_PER_SEGMENT) ) {
            bd_ptr[n_frags-1].cmd_and_status |= SDMA_CMDSTAT_INT_MASK;
        }

        bd_ptr[n_frags-1].buf_paddr = 
            (uint32_t) (tinfo->src_addrs[n_frags-1].paddr);

        // enable the descr last
        bd_ptr[n_frags-1].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;
        break;
    
    
    
    case SDMA_CHTYPE_AP_2_AP:
    default:
        n_frags = tinfo->src_fragments;
        for(i=0; i < n_frags-1; i++) {
            bd_ptr[i].cmd_and_status = cmd_and_status;
            bd_ptr[i].cmd_and_status |= tinfo->src_addrs[i].len;
            bd_ptr[i].buf_paddr = (uint32_t) (tinfo->src_addrs[i].paddr);
            bd_ptr[i].ext_buf_paddr = (uint32_t) (tinfo->dst_addrs[i].paddr);
            bd_ptr[i].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;
        }

        // Fill in last descriptor
        bd_ptr[n_frags-1].cmd_and_status =  SDMA_CMDSTAT_EXT_MASK |
                                            SDMA_CMD_XFER_SIZE(tinfo->xfer_unit_size) |
                                            SDMA_CMDSTAT_WRAP_MASK | 
                                            tinfo->src_addrs[n_frags-1].len;

        if (chan_ptr->cont_descr_loop) { 
            bd_ptr[n_frags-1].cmd_and_status |= SDMA_CMDSTAT_CONT_MASK;
        }
        if  (   (chan_ptr->attach_flags & DMA_ATTACH_EVENT_ON_COMPLETE) || 
                (chan_ptr->attach_flags & DMA_ATTACH_EVENT_PER_SEGMENT) ) {
            bd_ptr[n_frags-1].cmd_and_status |= SDMA_CMDSTAT_INT_MASK;
        }

        bd_ptr[n_frags-1].buf_paddr = 
            (uint32_t) (tinfo->src_addrs[n_frags-1].paddr);
        bd_ptr[n_frags-1].ext_buf_paddr = 
            (uint32_t) (tinfo->dst_addrs[n_frags-1].paddr);
        
        // enable the descr last
        bd_ptr[n_frags-1].cmd_and_status |= SDMA_CMDSTAT_DONE_MASK;
        break;
    }


    // need to remember how many descriptors last used on this channel, so that
    // we can automatically renable the 'done' bit without having to call
    // sdma_setup_xfer() again.
    chan_ptr->n_frags = n_frags;
    
    return 0;
}

int
sdma_xfer_start(void * handle) {
    sdma_chan_t * chan_ptr = handle;

    if(chan_ptr->is_event_driven) {
        // Turn on events
        pthread_mutex_lock( sdmasync_regmutex_get() );
        out32(  sdma_base + SDMA_HOSTOVR,
                in32(sdma_base + SDMA_HOSTOVR) | (1 << chan_ptr->ch_num));
        pthread_mutex_unlock( sdmasync_regmutex_get() );
    } else {
        // trigger the dma using software
        out32(sdma_base + SDMA_HSTART , 1 << chan_ptr->ch_num);
    }
    
    
    return 0;
}


int 
sdma_xfer_abort(void * handle) {
    sdma_chan_t * chan_ptr = handle;
    
    if(chan_ptr->is_event_driven) {
        // Turn off events
        pthread_mutex_lock( sdmasync_regmutex_get() );
        out32(  sdma_base + SDMA_HOSTOVR, 
                in32(sdma_base + SDMA_HOSTOVR) & ~(1 << chan_ptr->ch_num));
        pthread_mutex_unlock( sdmasync_regmutex_get() );

        if (chan_ptr->cont_descr_loop) {
            // Can't be sure where the microcode stopped because of
            // continous dma, so load  the channel context which acts as
            // reset on that channel.
            if ( sdmacmd_ctx_load(chan_ptr) != 0) {
                return -1;
            }
        }

        // reset buffer pointer to the head
        ccb_ptr[chan_ptr->ch_num].current_bd_paddr = 
            ccb_ptr[chan_ptr->ch_num].base_bd_paddr;
        
    }    
    
    return 0;
}


unsigned
sdma_bytes_left(void * handle) {
    sdma_chan_t * chan_ptr = handle;
    
    if (chan_ptr->bd_ptr[chan_ptr->n_frags -1].cmd_and_status & SDMA_CMDSTAT_DONE_MASK) {
        return 1;
    } else {
        return 0;
    }
}

int
sdma_xfer_complete(void * handle) {
    sdma_chan_t * chan_ptr = handle;
    
    if(chan_ptr->is_event_driven) {
        // Turn off events
        pthread_mutex_lock( sdmasync_regmutex_get() );
        out32(  sdma_base + SDMA_HOSTOVR, 
                in32(sdma_base + SDMA_HOSTOVR) & ~(1 << chan_ptr->ch_num));
        pthread_mutex_unlock( sdmasync_regmutex_get() );
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
//                                NO SUPPORT                                  //
////////////////////////////////////////////////////////////////////////////////

#if 0

int
sdma_alloc_buffer ( void *handle, dma_addr_t *addr, unsigned size, unsigned flags) {
    return NULL;
}

void
sdma_free_buffer(void *handle, dma_addr_t *addr){
}

#endif
////////////////////////////////////////////////////////////////////////////////

int
get_dmafuncs(dma_functions_t *functable, int tabsize)
{
    DMA_ADD_FUNC(functable, init, sdma_init, tabsize);
    DMA_ADD_FUNC(functable, fini, sdma_fini, tabsize);
    DMA_ADD_FUNC(functable, driver_info, sdma_driver_info, tabsize);
    DMA_ADD_FUNC(functable, channel_info, sdma_channel_info, tabsize);
    DMA_ADD_FUNC(functable, channel_attach, sdma_channel_attach, tabsize);
    DMA_ADD_FUNC(functable, channel_release, sdma_channel_release, tabsize);
    //DMA_ADD_FUNC(functable, alloc_buffer, sdma_alloc_buffer, tabsize);
    //DMA_ADD_FUNC(functable, free_buffer, sdma_free_buffer, tabsize);
    DMA_ADD_FUNC(functable, setup_xfer, sdma_setup_xfer, tabsize);
    DMA_ADD_FUNC(functable, xfer_start, sdma_xfer_start, tabsize);
    DMA_ADD_FUNC(functable, xfer_abort, sdma_xfer_abort, tabsize);
    DMA_ADD_FUNC(functable, xfer_complete, sdma_xfer_complete, tabsize);
    DMA_ADD_FUNC(functable, bytes_left, sdma_bytes_left, tabsize);
    DMA_ADD_FUNC(functable, query_channel, sdma_query_channel, tabsize);
    return 0;
}

