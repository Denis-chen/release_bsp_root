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


#include "startup.h"
#include <arm/mx35.h>

#define AUDIO_CLKO_DIV1 	0
#define AUDIO_SAMPLE_FREQ	48000
#define AUDIO_SCALER		256

void init_audio (void)
{
        uint32_t ptcr1, pdcr1, ptcr4, pdcr4;
	uint32_t cosr;
	uint32_t clko_div;

	/* 
	 * calculate clko divider against cpu_freq to set clko to 
	 * be 12288000Hz for sgtl5000 codec. The clko can be other value if only 
	 * it matches with mclk (SYS_MCLK) option in sgtl5000 options.
	 */
	clko_div = MX35_AUDIOCLOCK/((1<<AUDIO_CLKO_DIV1)*AUDIO_SAMPLE_FREQ*AUDIO_SCALER) - 1;
	
	chip_access(MX35_CCM_BASE, 0, 0, 0x80);

	cosr = chip_read32(MX35_CCM_COSR);

	cosr &= 0xFFFF0000;
	cosr |= MX35_CCM_CLKOSEL(MX35_CLKOSEL_AUDIOCLOCK) | MX35_CCM_CLKOEN | MX35_CCM_CLKO_DIV1(AUDIO_CLKO_DIV1) | MX35_CCM_CLKO_DIV(clko_div);

	chip_write32(MX35_CCM_COSR, cosr);

	chip_done();

	/* configure audio multiplexer, connect port 1 (SSI1) to port 4 (codec) */
        ptcr1 = (AUDMUX_SYNC |
                AUDMUX_TFS_DIROUT | AUDMUX_TFS_PORT4 |
                AUDMUX_TCLK_DIROUT | AUDMUX_TCLK_PORT4 |
                AUDMUX_RFS_DIROUT | AUDMUX_RFS_PORT4 |
                AUDMUX_RCLK_DIROUT | AUDMUX_RCLK_PORT4);
        pdcr1 = AUDMUX_RXDSEL_PORT4;
        ptcr4 = AUDMUX_SYNC;
        pdcr4 = AUDMUX_RXDSEL_PORT1;
        out32 (MX35_AUDMUX_BASE + MX35_AUDMUX_PTCR1, ptcr1);
        out32 (MX35_AUDMUX_BASE + MX35_AUDMUX_PDCR1, pdcr1);
        out32 (MX35_AUDMUX_BASE + MX35_AUDMUX_PTCR4, ptcr4);
        out32 (MX35_AUDMUX_BASE + MX35_AUDMUX_PDCR4, pdcr4);
}

