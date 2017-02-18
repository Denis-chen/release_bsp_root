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
 * MTD device I/O routines
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <gulliver.h>
#include <sys/slog.h>
#include <sys/neutrino.h>
#include <fs/etfs.h>

#include "devio.h"

/*
 * Initialize the part and stuff physical paramaters for it.
 */
int devio_init(struct etfs_devio *dev)
{
	uint8_t			id[2];
	uint8_t			status;
	CHIPIO			*cio = dev->cio;
	
	// Allow IO operations
	if(ThreadCtl(_NTO_TCTL_IO, 0) != EOK) {
		dev->log(_SLOG_CRITICAL, "You must be root!");
		// We will not return
	}

	// Do anything special to get the part ready to use
	if(nand_init(dev) != 0) {
		dev->log(_SLOG_CRITICAL, "nand_init failed : %s", strerror(errno));
		// We will not return
	}

	// Reset the part
	nand_write_cmd(cio, NANDCMD_RESET);
	
	// Read Status Command
	nand_write_cmd(cio, NANDCMD_STATUSREAD);
	nand_read_data(cio, (uint8_t *) &status, 1);

	// Check if write protect is on	
	if ((status & NAND_WRITE_PROTECT) == 0) {
		dev->log(_SLOG_WARNING, "devio_init: Write-Protect ON: Readonly device (Status = 0x%x)", (status & 0xFF));
		// We will not return
	}
	
	// Read id info from the part
	nand_write_cmd(cio, NANDCMD_IDREAD);
	nand_write_pageaddr(cio, 0, 1);
	nand_read_data(cio, id, 2);
	
	switch (id[1]) {
			
	/* 2048M / 16 Gigabit */
	case 0xa5: case 0xd5: case 0xb5: case 0xc5:
		/* Actual size 2GB */
		dev->numblks = 8192;
		cio->addrcycles = 5;
		break;

	default:
		dev->log(_SLOG_CRITICAL, "Unsupported NAND device (%2.2x %2.2x)", id[0], id[1]);
		// We will not return
	}

	dev->log(_SLOG_INFO, "Flash ID: Manufacturer ID=0x%x, Device ID=0x%x", id[0], id[1]);
	
	// These must be initialized here. All other numbers are built from them.
	sprintf(dev->id, "NAND%2.2x%2.2x", id[0], id[1]);
	dev->memtype       = ETFS_MEMTYPE_NAND;
	// We glue to physical pages at the driver and report back their combined size
	dev->clustersize   = DATASIZE * PAGES2CLUSTER;
	dev->sparesize     = SPARESIZE * PAGES2CLUSTER;
	dev->clusters2blk  = PAGES2BLK / PAGES2CLUSTER;
	dev->blksize       = (dev->clustersize + SPARESIZE) * dev->clusters2blk;

	cio->lastpage = ~0;
	return(EOK);
}


/*
 * Read a cluster of data.
 * 
 * Verify crc for both the spare area and the entire page (data + spare).
 * The passed buffer "buf" is larger than the cluster size. It can hold
 * PAGESIZE bytes. This is for convenience when reading data from the
 * device and calculating the crc. The work area after clustersize
 * bytes is ignored by the caller.
 */
int devio_readcluster(struct etfs_devio *dev, unsigned cluster, uint8_t *buf, struct etfs_trans *trp)
{
	struct spare		*sp;
	int					err;
	unsigned			page = cluster * PAGES2CLUSTER;
	CHIPIO				*cio = dev->cio;
	
	// Read Page
	nand_write_cmd(cio, NANDCMD_READ);
	cio->inspare = 0;
	nand_write_pageaddr(cio, page, cio->addrcycles);
	nand_write_cmd(cio, NANDCMD_READCONFIRM);
	nand_read_data(cio, buf, PAGESIZE);

	// Determine transaction codes
	sp = (struct spare *)(buf + DATASIZE);
	// Refer Spare Area Buffer 8-bit organization
	if (sp->align[3] != 0xff) {
		dev->log(_SLOG_ERROR, "devio_readcluster: readcluster BADBLK on cluster %d", cluster);
		trp->tacode = ETFS_TRANS_BADBLK;
	}
	else if (((uint64_t *)sp)[2] == ~0ll  &&  ((uint64_t *)sp)[3] == ~0ll
 	       && ((uint64_t *)sp)[4] == ~0ll  &&  ((uint64_t *)sp)[5] == ~0ll
 	       && ((uint64_t *)sp)[6] == ~0ll  &&  ((uint64_t *)sp)[7] == ~0ll)
 	{
		if (sp->erasesig[0] == ERASESIG1 && sp->erasesig[1] == ERASESIG2)
			trp->tacode = ETFS_TRANS_ERASED;
		else
			trp->tacode = ETFS_TRANS_FOXES;
	}
	else if(dev->crc32((uint8_t *) sp, sizeof(*sp) - sizeof(sp->crctrans)) != sp->crctrans) 
	{
		dev->log(_SLOG_ERROR, "devio_readcluster: readcluster trans DATAERR on cluster %d", cluster);
		trp->tacode = ETFS_TRANS_DATAERR;
	}
	else
		trp->tacode = ETFS_TRANS_OK;

	// Build transaction data from data in the spare area.
	trp->sequence   = ENDIAN_LE32(sp->sequence);
	trp->fid        = ENDIAN_LE16(sp->fid);
	trp->cluster    = ENDIAN_LE16(sp->clusterlo) + (sp->clusterhi << 16);
	trp->nclusters  = sp->nclusters;

	trp->dacode = ETFS_TRANS_OK;
	if(trp->tacode == ETFS_TRANS_OK) {
		if(dev->crc32(buf, DATASIZE) != sp->crcdata) {
 			err = dev->ecc(buf, &sp->eccv[0], 8, 1); 			
			if(err >= ETFS_ECC_ERROR || dev->crc32(buf, DATASIZE) != sp->crcdata) {
				dev->log(_SLOG_ERROR, "devio_readcluster: readcluster DATAERR on cluster %d", cluster);
				return(trp->dacode = ETFS_TRANS_DATAERR);
			}
			dev->log(_SLOG_ERROR, "devio_readcluster: readcluster ECCERR on cluster %d", cluster);
			return(trp->dacode = ETFS_TRANS_ECC);
		}
	}

	return(trp->tacode);
}


/*
 * Read the spare area of a page (not the data) to return transaction information.
 * 
 * This called is used heavily on startup to process the transactions. It is
 * a cheaper call than readcluster() since it reads less data and has a smaller
 * checksum to calculate.
 */
int devio_readtrans(struct etfs_devio *dev, unsigned cluster, struct etfs_trans *trp)
{
	struct spare		spare;
 	volatile  unsigned		page = cluster * PAGES2CLUSTER;
	CHIPIO				*cio = dev->cio;
	
	// Read spare
	nand_write_cmd(cio, NANDCMD_READ);
	cio->inspare = 1;
	nand_write_pageaddr(cio, page, cio->addrcycles);
	nand_write_cmd(cio, NANDCMD_READCONFIRM);
	nand_read_data(cio, (uint8_t *)&spare, sizeof(spare));
	cio->lastpage = page;

	// Check for bad block (Refer Spare Area Buffer 8-bit organization)
	if (spare.align[3] != 0xff) {
		dev->log(_SLOG_ERROR, "devio_readtrans: readtrans BADBLK on cluster %d", cluster);
		return(ETFS_TRANS_BADBLK);
	}

	// Check for erased block
	if (((uint64_t *)&spare)[2] == ~0ll  &&  ((uint64_t *)&spare)[3] == ~0ll
		&& ((uint64_t *)&spare)[4] == ~0ll  &&  ((uint64_t *)&spare)[5] == ~0ll
		&& ((uint64_t *)&spare)[6] == ~0ll  &&  ((uint64_t *)&spare)[7] == ~0ll)
	{
		if(spare.erasesig[0] == ERASESIG1 && spare.erasesig[1] == ERASESIG2)
			return(ETFS_TRANS_ERASED);
		else
			return(ETFS_TRANS_FOXES);
	}
	
	// Validate checksum
	if(dev->crc32((uint8_t *) &spare, sizeof(spare) - sizeof(spare.crctrans)) != spare.crctrans) {
		dev->log(_SLOG_ERROR, "devio_readtrans: readtrans DATAERR on cluster %d", cluster);
		return(ETFS_TRANS_DATAERR);
	}

	// Build transaction data
	trp->sequence    = ENDIAN_LE32(spare.sequence);
	trp->fid         = ENDIAN_LE16(spare.fid);
	trp->cluster     = ENDIAN_LE16(spare.clusterlo) + (spare.clusterhi << 16);
	trp->nclusters   = spare.nclusters;

	return(ETFS_TRANS_OK);
}


/*
 * Post a cluster of data.
 * 
 * Set crc for both the spare area and the entire page (data + spare).
 * The passed buffer "buf" is larger than the cluster size. It can hold
 * PAGESIZE bytes. This is for convenience writing data to the device and
 * calculating the crc. The work area after clustersize bytes is ignored
 * by the caller.
 */
int devio_postcluster(struct etfs_devio *dev, unsigned cluster, uint8_t *buf, struct etfs_trans *trp)
{
	struct spare		*sp;
	uint8_t				status;
	unsigned			page = cluster * PAGES2CLUSTER;
	CHIPIO				*cio = dev->cio;
	
	// Build spare area
	sp = (struct spare *) (buf + DATASIZE);
	memset((void *)sp, 0xff, sizeof(*sp));
	sp->erasesig[0] = ERASESIG1;
	sp->erasesig[1] = ERASESIG2;
	
	if(trp) {
		sp->sequence   = ENDIAN_LE32(trp->sequence);
		sp->fid        = ENDIAN_LE16((uint16_t) trp->fid);
		sp->clusterlo  = ENDIAN_LE16((uint16_t) trp->cluster);
		sp->clusterhi  = trp->cluster >> 16;
		sp->nclusters  = trp->nclusters;
		sp->status     = 0xff;
		sp->status2    = 0xff;
		sp->crcdata    = dev->crc32(buf, DATASIZE);
		dev->ecc(buf, &sp->eccv[0], 8, 0);
		sp->crctrans   = dev->crc32((uint8_t *) sp, sizeof(*sp) - sizeof(sp->crctrans));
		if (cluster % dev->clusters2blk == 0) {
			// Can only punch bits down once and we did it on the erase
			sp->erasesig[0] = ~0;
			sp->erasesig[1] = ~0;
		}
	}

	// Write Page (Data + Spare)
	nand_write_cmd(cio, NANDCMD_PROGRAM);
	nand_write_pageaddr(cio, page, cio->addrcycles);
	nand_write_data(cio, buf, PAGESIZE);
	nand_write_cmd(cio, NANDCMD_PROGRAMCONFIRM);

	// Read Status after Program
	nand_write_cmd(cio, NANDCMD_STATUSREAD);
	nand_read_data(cio, (uint8_t *) &status, 1);

	// Check for program error -- NAND_PROGRAM_ERASE_ERROR (SR0)
	if(status & NAND_PROGRAM_ERASE_ERROR) {
		dev->log(_SLOG_ERROR, "devio_postcluster: Post SPARE error on page %d (0x%x)", page, status);
		return(ETFS_TRANS_DEVERR);
	}

	return(ETFS_TRANS_OK);
}


/*
 * Erase a block.
 */
int devio_eraseblk(struct etfs_devio *dev, unsigned blk)
{
	uint8_t			status = 0;
	CHIPIO			*cio = dev->cio;
	uint8_t			*buf = alloca(PAGESIZE * PAGES2CLUSTER);
	
	// Erase Setup
	nand_write_cmd(cio, NANDCMD_ERASE);
	nand_write_blkaddr(cio, blk, cio->addrcycles);
	nand_write_cmd(cio, NANDCMD_ERASECONFIRM);

	// Read Status after Erase
	nand_write_cmd(cio, NANDCMD_STATUSREAD);
	nand_read_data(cio, &status, 1);
	
	// Check for erase error
	if(status & NAND_PROGRAM_ERASE_ERROR) {
		dev->log(_SLOG_ERROR, "devio_eraseblk: erase error on blk %d (0x%x)", blk, status);
		return(ETFS_TRANS_DEVERR);
	}
		
	/*
	 * Write the erase signature. We only write non FFs in the first 16 bytes
	 * of the spare area and put FFs everywhere else. This is required for
	 * multilevel NAND devices.
	 */
	memset(buf, 0xff, PAGESIZE * PAGES2CLUSTER);
	status = devio_postcluster(dev, blk * dev->clusters2blk, buf, NULL);
	return(status);
}


/*
 * Called to allow the driver to flush any cached data that
 * has not been written to the device. The NAND class driver does
 * not need it.
 */
int devio_sync(struct etfs_devio *dev)
{
	return(-1);
}
