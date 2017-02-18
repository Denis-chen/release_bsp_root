/*
 * $QNXLicenseC: 
 * Copyright 2009, QNX Software Systems.  
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
#include "microcode.h"

// ROM Script locations 
#define AP_2_AP_ADDR                642
#define AP_2_MCU_ADDR               683
#define MCU_2_AP_ADDR               747
#define MCU_2_SHP_ADDR              961
#define SHP_2_MCU_ADDR              892

// RAM Script locations 
#define MCU_2_SPDIF_ADDR            6462
#define SPDIF_2_MCU_ADDR            6595

// ROMv2 Script locations 
#define AP_2_AP_ADDR_V2              729
#define AP_2_MCU_ADDR_V2             770
#define MCU_2_AP_ADDR_V2             834
#define MCU_2_SHP_ADDR_V2            1048
#define SHP_2_MCU_ADDR_V2            979

// RAMv2 Script locations 
#define MCU_2_SPDIF_ADDR_V2          6382
#define SPDIF_2_MCU_ADDR_V2          6746


///////////////////////////////////////////////////////////////////////////////
//                           PRIVATE FUNCTIONS                               //
///////////////////////////////////////////////////////////////////////////////


#define ROM_BASE            0x0
#define ROM_SIZE            16 * 1024

#define REG_CHIPID          0x40
#define CHIPID_MASK         0xf

int get_chip_rev( void ) {
    uintptr_t	mx35_rom_vbase;
    int         mx35_chip_rev;
    
	if ((mx35_rom_vbase = mmap_device_io(ROM_SIZE, ROM_BASE)) == (uintptr_t)MAP_FAILED) {
		return -1;
    }
    
	mx35_chip_rev = in32(mx35_rom_vbase + REG_CHIPID) & CHIPID_MASK;
	munmap_device_io(mx35_rom_vbase, ROM_SIZE);
    
    return mx35_chip_rev;
}



///////////////////////////////////////////////////////////////////////////////
//                            PUBLIC FUNCTIONS                               //
///////////////////////////////////////////////////////////////////////////////


int sdmascript_lookup( sdma_scriptinfo_t * scriptinfo ) {
     
    
    if ( get_chip_rev() >= 2 ) {
    
        // get ram microcode data...      
        scriptinfo->ram_microcode_info.p    = sdma_code_v2;
        scriptinfo->ram_microcode_info.addr = SDMA_RAM_CODE_START_ADDR_V2;
        scriptinfo->ram_microcode_info.size = SDMA_RAM_CODE_SIZE_V2;
        
        // populate the scriptinfo struct with script addresses
        scriptinfo->script_addr_arr[SDMA_CHTYPE_AP_2_AP]     =  AP_2_AP_ADDR_V2;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_MCU_2_AP]    =  MCU_2_AP_ADDR_V2;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_AP_2_MCU]    =  AP_2_MCU_ADDR_V2;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_MCU_2_SHP]   =  MCU_2_SHP_ADDR_V2;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_SHP_2_MCU]   =  SHP_2_MCU_ADDR_V2;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_MCU_2_SPDIF] =  MCU_2_SPDIF_ADDR_V2;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_SPDIF_2_MCU] =  SPDIF_2_MCU_ADDR_V2;
     
    } else {
        
        // get ram microcode data...      
        scriptinfo->ram_microcode_info.p    = sdma_code;
        scriptinfo->ram_microcode_info.addr = SDMA_RAM_CODE_START_ADDR;
        scriptinfo->ram_microcode_info.size = SDMA_RAM_CODE_SIZE;
        
        // populate the scriptinfo struct with script addresses
        scriptinfo->script_addr_arr[SDMA_CHTYPE_AP_2_AP]     = AP_2_AP_ADDR;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_MCU_2_AP]    = MCU_2_AP_ADDR;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_AP_2_MCU]    = AP_2_MCU_ADDR;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_MCU_2_SHP]   = MCU_2_SHP_ADDR;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_SHP_2_MCU]   = SHP_2_MCU_ADDR;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_MCU_2_SPDIF] = MCU_2_SPDIF_ADDR;
        scriptinfo->script_addr_arr[SDMA_CHTYPE_SPDIF_2_MCU] = SPDIF_2_MCU_ADDR;
    }
     
     return EOK;
}


