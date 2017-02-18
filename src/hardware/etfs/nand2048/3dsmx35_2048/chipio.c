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
 * NAND flash memory chip interface routines
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/slog.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <fs/etfs.h>
#include <string.h>
#include <arm/mx35.h>

struct chipio;
#define CHIPIO	struct chipio
#include "devio.h"

#define SPAS_MASK		(0xFF00)
#define SPAS_SHIFT		(0)
#define NFC_SPAS_64		 32

//
// Device specific data structure for the Ringo i.MX35ADS, with 2112 byte NAND page.
//
struct chipio {
	struct _chipio	chip;
	unsigned		phys_base;
	uintptr_t		io_base;
	uintptr_t		reg_base;
	volatile unsigned 	data;
	volatile unsigned 	data_other;
	volatile unsigned 	spare0;
	volatile unsigned 	spare1;
	volatile unsigned	spare2;
	volatile unsigned	spare3;
} chipio;


int
main(int argc, char *argv[])
{
	return(etfs_main(argc, argv));
}


/*
 * Process device specific options (if any).
 * This is always called before any access to the part.
 * It is called by the -D option to the filesystem. If no -D option is given,
 * this function will still be called with "" for optstr.
 */
int devio_options(struct etfs_devio *dev, char *optstr)
{
	struct chipio	*cio;
	char			*value;
	static char		*opts[] = {
						"use",		// 0
						"addr",		// 1
						NULL
					} ;

	cio = dev->cio = &chipio;

	while (*optstr) {
		switch (getsubopt(&optstr, opts, &value)) {
		case 0:
			printf("Device specific options:\n");
			printf("  -D use,addr=xxxx\n");
			return(-1);

		case 1:
			cio->phys_base = strtoul(value, NULL, 16);
			break;

		default:
			dev->log(_SLOG_ERROR, "Invalid -D suboption.");
			return(EINVAL);
		}
	}

	return(EOK);
}


/*
 * Initialize the NAND flash memory device and controller.
 * Called once at startup.
 */
int
nand_init(struct etfs_devio *dev) 
{
	int				i;
	struct chipio	*cio = dev->cio;

	// Pick a default for the board.
	if (cio->phys_base == 0)
		cio->phys_base = MX35_NAND_BASE;

	// Map in the device registers
	cio->io_base = mmap_device_io(MX35_NAND_SIZE, cio->phys_base);
	if (cio->io_base == (uintptr_t) MAP_FAILED) {
		dev->log(_SLOG_CRITICAL, "Unable to map in device registers (%d).", errno);
		return(-1);
	}

	// Store mapped register addresses
	cio->data = cio->io_base + MX35_NAND_MAIN_BUF_0;
	cio->data_other = cio->io_base + MX35_NAND_MAIN_BUF_1;
	cio->spare0 = cio->io_base + MX35_NAND_SPARE_BUF_0;
	cio->spare1 = cio->io_base + MX35_NAND_SPARE_BUF_1;
	cio->spare2 = cio->io_base + MX35_NAND_SPARE_BUF_2;
	cio->spare3 = cio->io_base + MX35_NAND_SPARE_BUF_3;
	cio->reg_base = cio->io_base + MX35_NFC_REGISTER;

	// Reset the NAND state machine
	out16(cio->reg_base + MX35_NAND_FLASH_CONFIG1, NAND_CONFIG1_NFC_RST);
	for (i = 0; i < 1000; i++) {
		if (!(in16(cio->reg_base + MX35_NAND_FLASH_CONFIG1) & NAND_CONFIG1_NFC_RST)) {
			break;
		}
	}
	if ( i >= 1000 ) {
		dev->log(_SLOG_CRITICAL, "NAND Reset Timeout!\n");
	}
	
	// Unlock the internal RAM buffer
	out16(cio->reg_base + MX35_NFC_CONFIGURATION, 0x2);

	// Set up configuration bits
	out16(cio->reg_base + MX35_NAND_FLASH_CONFIG1, (NAND_CONFIG1_FP_INT | NAND_CONFIG1_PPB128 | NAND_CONFIG1_NF_CE));
	
	// Disable Write Protection
	out16(cio->reg_base + MX35_UNLOCK_START_BLK_ADD0, 0);		// Start block
	out16(cio->reg_base + MX35_UNLOCK_END_BLK_ADD0, 0x4000);	// End block
	out16(cio->reg_base + MX35_NF_WR_PROT, 0x4);			// Unlock blocks according to given address range
	out16(cio->reg_base + MX35_RAM_BUFFER_ADDRESS, 0);		// RAM main buffer 0
	// Set spare to 64
	out16(cio->reg_base + MX35_SPAS, ((in16(cio->reg_base + MX35_SPAS) & SPAS_MASK) | (NFC_SPAS_64 << SPAS_SHIFT)) );

	return(0);
}


/*
 * This function polls the NANDFC to wait for the basic operation to complete by
 * checking the INT bit of config2 register.
 */
int
nand_wait_busy(struct chipio *cio, uint32_t usec)
{
	uint16_t	stat;

	for (usec = MAX_ERASE_USEC ; usec ; --usec)
	{
		stat = in16(cio->reg_base + MX35_NAND_FLASH_CONFIG2);
		if (stat & NAND_CONFIG2_INT)
		{
			out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, (stat & ~NAND_CONFIG2_INT));
			return(0);
		}
		else 
			nanospin_ns(1000);
	}

	return(-1);		// We will exit from the log and never reach here. 
}


/*
 * Write a command to flash memory device.
 * This function is used to write command to NAND Flash for
 * different operations to be carried out on NAND Flash.
 * Accessing the command register automatically sets ALE=0, CLE=1.
 */
void
nand_write_cmd(struct chipio *cio, int command)
{	
	out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, 0);

	out16(cio->reg_base + MX35_NAND_FLASH_CMD, (uint16_t)command);
	out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, NAND_CONFIG2_FCMD);

	if(nand_wait_busy(cio, MAX_ERASE_USEC) != 0)
		fprintf(stderr, "Timedout on write_cmd");
	
	cio->chip.lastcmd = command;
}


/*
 * Write the page address to flash memory device.
 * Page read and page program need the same 5 address cycles following the
 * command input.
 * Accessing the address register automatically sets ALE=1, CLE=0.
 */
void
nand_write_pageaddr(struct chipio *cio, unsigned page, int addr_cycles)
{
	int			i;
	uint64_t	addr = ((uint64_t) page) << 16;

	for (i = 0; i < addr_cycles; i++)
	{
		out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, 0);

		out16(cio->reg_base + MX35_NAND_FLASH_ADD, (addr & 0xFF));
		addr >>= 8;
		out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, NAND_CONFIG2_FADD);

		if(nand_wait_busy(cio, MAX_READ_USEC) != 0)
			fprintf(stderr, "Timedout on write_pageaddr");
	}
}


/*
 * Write the block address to flash memory device.
 * In block erase operation, only the 3 row address cycles (page address)
 * are used.
 * Accessing the address register automatically sets ALE=1, CLE=0.
 */
void
nand_write_blkaddr(struct chipio *cio, unsigned blk, int addr_cycles)
{
	int			i;
	unsigned	addr = (blk * PAGES2BLK);

	for (i = 0; i < (addr_cycles - 2); i++)
	{
		out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, 0);

		out16(cio->reg_base + MX35_NAND_FLASH_ADD, (addr & 0xFF));
		addr >>= 8;
		out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, NAND_CONFIG2_FADD);

		if(nand_wait_busy(cio, MAX_READ_USEC) != 0)
			fprintf(stderr, "Timedout on write_blkaddr");
	}
}


/*
 * Write data to flash memory device.
 * The data to be written on NAND Flash is first copied to RAM buffer.
 * After the Data Input Operation by the NFC, the data is written to NAND Flash.
 * Accessing the data reg automatically sets ALE=0, CLE=0.
 */
void
nand_write_data(struct chipio *cio, uint8_t *databuffer, int data_cycles)
{			
	/*
	 * Use RAM main buffer 0
	 */
	out16(cio->reg_base + MX35_RAM_BUFFER_ADDRESS, 0);
	
	/*
	 * Fill up main buffer
	 */
	memcpy((void *)cio->data, (void *)databuffer, DATASIZE);
	databuffer += DATASIZE;
	/*
	 * Fill up spare buffer
	 */
	memcpy((void *)cio->spare0, (void *)databuffer, 16);
	databuffer += 16;
	memcpy((void *)cio->spare1, (void *)databuffer, 16);
	databuffer += 16;
	memcpy((void *)cio->spare2, (void *)databuffer, 16);
	databuffer += 16;
	memcpy((void *)cio->spare3, (void *)databuffer, 16);

	out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, 0);

	out16(cio->reg_base + MX35_RAM_BUFFER_ADDRESS, 0);
	out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, NAND_CONFIG2_FDI);

	if(nand_wait_busy(cio, MAX_POST_USEC) != 0)
		fprintf(stderr, "Timedout on POST");
}


/*
 * Read data from flash memory device.
 * To read the data from NAND Flash, first the data output cycle is initiated by
 * the NFC, which copies the data to RAM buffer. This data is then copied to databuffer.
 * Accessing the data reg automatically sets ALE=0, CLE=0.
 */
void
nand_read_data(struct chipio *cio, uint8_t *databuffer, int data_cycles)
{
	uint8_t	*pbuf;
	uint16_t	store = 0;
	
	// Determine what is being read; page data, chip ID, or chip status, and set
	// the config2 register accordingly
	switch(cio->chip.lastcmd)
	{
		case NANDCMD_IDREAD:
			out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, 0);
			
			out16(cio->reg_base + MX35_RAM_BUFFER_ADDRESS, 0);	// RAM main buffer 0
			
			out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, NAND_CONFIG2_FDO_ID_OUT);
			if(nand_wait_busy(cio, MAX_READ_USEC) != 0)
				fprintf(stderr, "Timeout on ID_READ");
			
			*(uint16_t *)databuffer = in16(cio->data);
			break;

		case NANDCMD_STATUSREAD:
			out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, 0);
			
			store = in16(cio->data_other);
			
			out16(cio->reg_base + MX35_RAM_BUFFER_ADDRESS, 1);	// RAM main buffer 1
			
			out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, NAND_CONFIG2_FDO_STAT_OUT);
			if(nand_wait_busy(cio, MAX_READ_USEC) != 0)
				fprintf(stderr, "Timeout on STATUS_READ");
			
			databuffer[0] = in16(cio->data_other);
			out16(cio->data_other, store);
			break;

		case NANDCMD_READCONFIRM:			
			out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, 0);
			out16(cio->reg_base + MX35_RAM_BUFFER_ADDRESS, 0);	// RAM buffer
			out16(cio->reg_base + MX35_NAND_FLASH_CONFIG2, NAND_CONFIG2_FDO_PDO);
			if(nand_wait_busy(cio, MAX_READ_USEC) != 0)
				fprintf(stderr, "Timeout on READ");

			pbuf = (uint8_t *)databuffer;

			/*
			 * Read main buffer
			 */
			if (data_cycles > DATASIZE) {
				memcpy((void *)pbuf, (void *)cio->data, DATASIZE);
				pbuf += DATASIZE;
			}

			/*
			 * Read spare buffer
			 */
			memcpy((void *)pbuf, (void *)cio->spare0, 16);
			pbuf += 16;
			memcpy((void *)pbuf, (void *)cio->spare1, 16);
			pbuf += 16;
			memcpy((void *)pbuf, (void *)cio->spare2, 16);
			pbuf += 16;
			memcpy((void *)pbuf, (void *)cio->spare3, 16);
			break;
	}
}
