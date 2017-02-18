/*
 * $QNXLicenseC: 
 * Copyright 2008, QNX Software Systems.  
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

/*
 * IPL for 3DSMX35 Board
 */

#include "ipl.h"
#include <arm/mx1.h>
#include <arm/mx35.h>
#include <hw/inout.h>

#define	MAX_SCAN	4096
#define	FLASH_IMAGE_ADDR	0xA0004000 // QNX image can be found here in NOR flash
#define RAM_DOWNLOAD_ADDR 	0x82000000 // QNX image got serially is downloaded here in RAM

extern void init_ser3dsmx35(unsigned port, unsigned baud, unsigned clk);

void
setup_image(unsigned image)
{
	ser_putstr("Found Image        @ 0x");
	ser_puthex(image);
	ser_putstr("\n");

	image_setup(image);

	ser_putstr("Jumping to Startup @ 0x");
	ser_puthex(startup_hdr.startup_vaddr);
	ser_putstr("\n");
	image_start(image);
}


void
download_ram(unsigned addr)
{
	unsigned	image;

	ser_putstr("\nDownloading Image to  0x");
	ser_puthex(addr);
	ser_putstr("\n");

	if (image_download_ser(addr)) {
		ser_putstr("Download Failed\n");
		return;
	}

	ser_putstr("\nScanning Image     @ 0x");
	ser_puthex(addr);
	ser_putstr("\n");

	image = image_scan(addr, addr + MAX_SCAN);

	if (image != -1) 
		setup_image(image);
	ser_putstr("\n IPL: image_scan() failed: Not a valid QNX image\n");
}

unsigned image_copy(unsigned dst, unsigned src, unsigned nbytes)
{
	unsigned	ret = dst;
	unsigned	n;

	/* Both addresses must be aligned to stuff in int size chunks */
	if (nbytes >= sizeof(unsigned) &&
		(src & (sizeof(unsigned) - 1)) == 0 &&
		(dst & (sizeof(unsigned) - 1)) == 0) {
		unsigned	*d = (unsigned *)dst;
		unsigned	*s = (unsigned *)src;


		n = ((nbytes >> 2) + 15) / 16;
                 
		switch ((nbytes >> 2) % 16){
		case 0:        do {  *d++ = *s++;
		case 15:             *d++ = *s++;
		case 14:             *d++ = *s++;
		case 13:             *d++ = *s++;
		case 12:             *d++ = *s++;
		case 11:             *d++ = *s++;
		case 10:             *d++ = *s++;
		case 9:              *d++ = *s++;
		case 8:              *d++ = *s++;
		case 7:              *d++ = *s++;
		case 6:              *d++ = *s++;
		case 5:              *d++ = *s++;
		case 4:              *d++ = *s++;
		case 3:              *d++ = *s++;
		case 2:              *d++ = *s++;
		case 1:              *d++ = *s++;
   	                   } while (--n > 0);
		}
              
		/* remainder */
		nbytes = nbytes & 0x3;


		if (nbytes) {
			dst = (unsigned)d;
			src = (unsigned)s;
		}
	}

	/* Get the unaligned bytes, or the remaining bytes */
	while (nbytes) {
		*(unsigned char *)dst = *(const unsigned char *)src;
		dst = (unsigned)dst + 1;
		src = (unsigned)src + 1;
		--nbytes;
	}

	return ret;
}

void scan_flash(unsigned addr)
{
	struct startup_header	*hdr;
	unsigned				flash_addr, ram_addr;

	ser_putstr("\nScanning flash at     0x");
	ser_puthex(addr);
	ser_putstr("\n");
	flash_addr = image_scan(addr, addr + MAX_SCAN);

	if (flash_addr != -1) {
		ser_putstr("Found image at        0x");
		ser_puthex(flash_addr);
		ser_putstr("\n");

		hdr = (struct startup_header *)flash_addr;
		ram_addr = hdr->ram_paddr + hdr->paddr_bias;	

		image_copy(ram_addr, flash_addr, hdr->startup_size);

		hdr = (struct startup_header *)ram_addr;

		if (hdr->flags1 & STARTUP_HDR_FLAGS1_COMPRESS_MASK) {
			image_copy(ram_addr + hdr->stored_size + hdr->imagefs_size, 
						flash_addr + hdr->startup_size, hdr->stored_size - hdr->startup_size);
			hdr->imagefs_paddr = ram_addr + hdr->stored_size + hdr->imagefs_size;
		} else {
			image_copy(ram_addr + hdr->startup_size, flash_addr + hdr->startup_size, hdr->stored_size - hdr->startup_size);
			hdr->imagefs_paddr = ram_addr + hdr->startup_size - hdr->paddr_bias;
		}
		ser_putstr("Jumping to startup at 0x");
		ser_puthex(hdr->startup_vaddr);
		ser_putstr("\n");
		jump(hdr->startup_vaddr);
	}
	ser_putstr("\n IPL: image_scan() failed: Not a valid QNX image\n");
}

int main()
{
	/* Clear Watchdog PDN bit to avoid it enters power-down mode */
	*(volatile unsigned short *) (MX35_WDOG_BASE + MX35_WDOG_MCR) = 0;

	/* Init serial interface */
	init_ser3dsmx35(0x43F90000, 115200, 100000000);
   
    ser_putstr("\nQNX Neutrino IPL for 3DSMX35 board\n");
    /* Show Menu */
	while (1) {
		ser_putstr("Commands:\n");
		ser_putstr("  d: download image to RAM\n");
		ser_putstr("  f: scan flash for image\n");
		ser_putstr("ipl> ");

		switch (ser_getchar()) {
			case 'd':
				download_ram(RAM_DOWNLOAD_ADDR);
				break;

			case 'f':
				scan_flash(FLASH_IMAGE_ADDR);
				break;
		}
				
	}
	return 0;
}
