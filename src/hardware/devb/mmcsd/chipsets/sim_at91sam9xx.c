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

#include	<sim_mmc.h>

#ifdef MMCSD_VENDOR_MCI
 
#include	<sim_at91sam9xx.h>


#define MCI_ERRORS (RINDE | RDIRE | RCRCE       \
                | RENDE | RTOE | DCRCE               \
                | DTOE | OVRE | UNRE)

uint8_t Txflag = 0;

	/* Used for debugging purpose */
#if 0   
static void _mci_dumpregs(void * base)
{       
        fprintf(stderr, "\n-------------- MCI Register Dump --------------\n"); 
        fprintf(stderr, "MCI_CR %08x, MCI_MR %08x, MCI_DTOR %08x,\nMCI_SDCR %08x MCI_ARGR %08x, MCI_CMDR %08x,\nMCI_BLKR %08x, MCI_RSPR0 %08x MCI_RSPR1 %08x,\nMCI_RSPR2 %08x, MCI_RSPR3 %08x, MCI_RDR %08x\nMCI_TDR %08x, MCI_SR %08x, MCI_IER %08x,\nMCI_IDR %08x, MCI_IMR %08x",READ_MCI(MCI_CR), READ_MCI(MCI_MR), READ_MCI(MCI_DTOR), READ_MCI(MCI_SDCR),READ_MCI(MCI_ARGR), READ_MCI(MCI_CMDR), READ_MCI(MCI_BLKR), READ_MCI(MCI_RSPR0),READ_MCI(MCI_RSPR1), READ_MCI(MCI_RSPR2), READ_MCI(MCI_RSPR3), READ_MCI(MCI_RDR),READ_MCI(MCI_TDR), READ_MCI(MCI_SR), READ_MCI(MCI_IER), READ_MCI(MCI_IDR), READ_MCI(MCI_IMR) );
        fprintf(stderr, "\n-----------------------------------------------\n");
}
#endif

static int _mci_detect(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	mci_ext_t		*mci;
	uintptr_t		base;

	ext   = (SIM_MMC_EXT *)hba->ext;
	mci = (mci_ext_t *)ext->handle;
	base  = mci->base;

	/* card detection pin is not available on all boards */
#if 0
	uint32_t                i;
        uint32_t        io_gpio_base;
	io_gpio_base = mmap_device_io(AT91_PIO_SIZE, AT91_PIOA_BASE);

	if (io_gpio_base == (uintptr_t) MAP_FAILED) {
		fprintf(stderr, "Unable to map in io_base (%d).", errno);
		return (MMC_FAILURE);
	}

	i = 0;
	while (in32(io_gpio_base + AT91_PIO_PDSR) & (1 << 15)) {
		if (++i > 1000)
			return (MMC_FAILURE);
		delay(1);
		}	
#endif
	return (MMC_SUCCESS);
}

static int _mci_interrupt(SIM_HBA *hba, int irq, int resp_type, uint32_t *resp)
{
	SIM_MMC_EXT		*ext;
	mci_ext_t		*mci;
	uintptr_t		base;
	uint32_t		status = 0;
	int			intr = 0;
	int 			completed = 0;

	ext   = (SIM_MMC_EXT *)hba->ext;
	mci = (mci_ext_t *)ext->handle;
	base  = mci->base;

	READ_MCI(MCI_SR);
	status =  READ_MCI(MCI_SR) & READ_MCI(MCI_IMR);

	if(status & MCI_ERRORS) {
		completed = 1;
	}
	else {
		if (status & CMDRDY) {
			if (Txflag){
                                WRITE_MCI(MCI_IER, TXBUFE);
                                WRITE_MCI(MCI_PTCR, MCI_TXTEN);
                        }else
				completed = 1;
		}
		if (status & ENDRX) {
			WRITE_MCI(MCI_IDR, ENDRX);
			intr |= MMC_INTR_DATA;
                }
		if (status & RXBUFF) {
                        WRITE_MCI(MCI_PTCR, MCI_RXTDIS | MCI_TXTDIS);
                        WRITE_MCI(MCI_IDR, RXBUFF | ENDRX);
			intr |= MMC_INTR_DATA;
                }
		if (status & TXBUFE) {
			Txflag = 0;
			/* Disable the transfer */
		        WRITE_MCI(MCI_PTCR, MCI_RXTDIS | MCI_TXTDIS);
		        /* Now wait for cmd ready */
		        WRITE_MCI(MCI_IDR, TXBUFE);
			WRITE_MCI(MCI_IER, NOTBUSY);
                }
		if (status & NOTBUSY) {
			WRITE_MCI(MCI_IER, CMDRDY);
			intr |= MMC_INTR_DATA;
                }
	}
	if (completed) {
		intr |= MMC_INTR_COMMAND;
		WRITE_MCI(MCI_IDR, 0xffffffff);	
		
		/* Handle a command that has been completed */
		if (resp_type & MMC_RSP_136) {
			resp[0] = READ_MCI(MCI_RSPR0);
			resp[1] = READ_MCI(MCI_RSPR1);
			resp[2] = READ_MCI(MCI_RSPR2);
			resp[3] = READ_MCI(MCI_RSPR3);
		} else if (resp_type & MMC_RSP_PRESENT) {
			resp[0] = READ_MCI(MCI_RSPR0);
		}
		status = READ_MCI(MCI_SR);

		if (status & MCI_ERRORS) {
			if(status & RTOE) 
				intr |= MMC_ERR_CMD_TO;
		}

	} else  
		WRITE_MCI(MCI_IDR, status);	
	delay(2);
	return intr;
}

/*
 * setup DMA transfer
 */
static int _mci_setup_dma(SIM_HBA *hba, paddr_t paddr, int len, int dir)
{
	SIM_MMC_EXT		*ext;
	mci_ext_t		*mci;
	uintptr_t		base;
	uint32_t		xlen, mr;
	uint16_t		blkcnt;

	ext   = (SIM_MMC_EXT *)hba->ext;
	mci = (mci_ext_t *)ext->handle;
	base  = mci->base;

	xlen = len;
	blkcnt = xlen / mci->blksz;
	xlen   = mci->blksz * blkcnt;

	if (blkcnt == 0)
		return 0;

                /* Check to see if this needs filling */
        if (READ_MCI(MCI_RCR) != 0) 
                fprintf(stderr, "Transfer active in current\n");

	WRITE_MCI(MCI_PTCR, MCI_RXTDIS | MCI_TXTDIS);
	
	if (len > 512) 
		len = 512 ;
	else {
		mr = READ_MCI(MCI_MR) & 0xffff;
	        WRITE_MCI(MCI_MR, mr | (len << 16));
	}
	
	if (dir == 1){
		WRITE_MCI(MCI_RPR, paddr);
	       	WRITE_MCI(MCI_RCR, len / 4);
	} else if (dir == 2){
		WRITE_MCI(MCI_TPR, paddr);
                WRITE_MCI(MCI_TCR, len / 4);
	}
	
	return (len);
}

/*
 * setup PIO transfer
 */
static int _mci_setup_pio(SIM_HBA *hba, int len, int dir)
{
	SIM_MMC_EXT		*ext;
	mci_ext_t		*mci;
	uintptr_t		base;
	uint16_t		blkcnt;

	ext   = (SIM_MMC_EXT *)hba->ext;
	mci = (mci_ext_t *)ext->handle;
	base  = mci->base;

	blkcnt = len / mci->blksz;
	
	if (blkcnt == 0)
		return 0;
	
	return (blkcnt * mci->blksz);
}

static int _mci_dma_done(SIM_HBA *hba, int dir)
{
	return MMC_SUCCESS;
}

static int _mci_pio_done(SIM_HBA *hba, char *buf, int len, int dir)
{
	SIM_MMC_EXT		*ext;
	mci_ext_t		*mci;
	uintptr_t		base;

	ext   = (SIM_MMC_EXT *)hba->ext;
	mci = (mci_ext_t *)ext->handle;
	base  = mci->base;

	if (len < mci->blksz)
		return 0;
	
	return mci->blksz;
}

static int _mci_command(SIM_HBA *hba, mmc_cmd_t *cmd)
{
        SIM_MMC_EXT             *ext;
        mci_ext_t               *mci;
        uintptr_t               base;
        uint32_t                command;
        uint32_t                mask = 0;

        ext   = (SIM_MMC_EXT *)hba->ext;
        mci = (mci_ext_t *)ext->handle;
        base  = mci->base;

        if ((READ_MCI(MCI_SR) & RTOE)) {
                WRITE_MCI(MCI_ARGR, 0);
                WRITE_MCI(MCI_CMDR, OPDCMD);
                while (!(READ_MCI(MCI_SR) & CMDRDY)) {
                        /* spin */
                }
        }

        command = cmd->opcode;
	
	if (cmd->resp & MMC_RSP_PRESENT) {
                command |= MAXLAT;
                if (cmd->resp & MMC_RSP_136)     
                        command |= RSPTYPE_136;
                else    
                        command |= RSPTYPE_48;
        } else 
                command |= RSPTYPE_NONE;

	if (cmd->eflags & MMC_CMD_DATA) {
                if (cmd->eflags & MMC_CMD_DATA_IN) 
                        command |= (TRDIR | TRCMD_START);       
                else
                        command |= TRCMD_START;

                /* TODO : MMC Stream not supported */
                if(cmd->eflags & MMC_CMD_DATA_MULTI)
                        command |= TRTYP_MB; 
                else
                        command |= TRTYP_SB;
        } 

	 /* Open Drain Command */
        if (((cmd->eflags & MMC_CMD_PPL) && (cmd->opcode == 0)))
		       command |= OPDCMD;	

	/* Set the arguments and send the command */
        if (!(cmd->eflags & MMC_CMD_DATA)) {
                WRITE_MCI(MCI_PTCR, MCI_TXTDIS | MCI_RXTDIS);
                WRITE_MCI(MCI_RPR, 0);
                WRITE_MCI(MCI_RCR, 0);
                WRITE_MCI(MCI_RNPR, 0);
                WRITE_MCI(MCI_RNCR, 0);
                WRITE_MCI(MCI_TPR, 0);
                WRITE_MCI(MCI_TCR, 0);
                WRITE_MCI(MCI_TNPR, 0);
                WRITE_MCI(MCI_TNCR, 0);
                mask |= CMDRDY; 
        } else {
                if (command & TRCMD_START) {
                        if (command & TRDIR) {
                                 /*
                                 * Handle a read
                                 */
        		if (cmd->opcode == 51) {
                                command = (51 | 1 << 6 | 1<<12);
                                mask |= CMDRDY;
                        } else
	                	mask |= ENDRX;
                        }
                        else {
				Txflag = 1;
                                mask |= CMDRDY;
                        }
        	}
	}
         /* Send the command and then enable the PDC */
        WRITE_MCI(MCI_ARGR, cmd->argument);
        WRITE_MCI(MCI_CMDR, command);
	
	if (command & TRCMD_START) {
        	if (command & TRDIR) {
                	WRITE_MCI(MCI_PTCR, MCI_RXTEN);
            	}
        }
       
         /* Enable selected interrupts */
        WRITE_MCI(MCI_IER, MCI_ERRORS | mask);

        return (MMC_SUCCESS);
}

static int _mci_cfg_bus(SIM_HBA *hba, int width, int mmc)
{
	SIM_MMC_EXT		*ext;
	mci_ext_t		*mci;
	uintptr_t		base;

	ext   = (SIM_MMC_EXT *)hba->ext;
	mci = (mci_ext_t *)ext->handle;
	base  = mci->base;
	
	return (MMC_SUCCESS);
}

static int _mci_clock(SIM_HBA *hba, int *clock, int high_speed)
{
	SIM_MMC_EXT		*ext;
	mci_ext_t		*mci;
	uintptr_t		base;
	uint32_t		max_clk;

	ext   = (SIM_MMC_EXT *)hba->ext;
	mci = (mci_ext_t *)ext->handle;
	base  = mci->base;
	max_clk = mci->clock;

	return (MMC_SUCCESS);
}

static int _mci_block_size(SIM_HBA *hba, int blksz)
{
	SIM_MMC_EXT		*ext;
	mci_ext_t		*mci;
	uintptr_t		base;

	ext   = (SIM_MMC_EXT *)hba->ext;
	mci = (mci_ext_t *)ext->handle;
	base  = mci->base;

	if (blksz > 4095)
		return (MMC_FAILURE);

	mci->blksz = blksz;
	
	return (MMC_SUCCESS);
}

/*
 * Reset host controller and card
 * The clock should be enabled and set to minimum (<400KHz)
 */
static int _mci_powerup(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	mci_ext_t		*mci;
	uintptr_t		base;

	ext   = (SIM_MMC_EXT *)hba->ext;
	mci = (mci_ext_t *)ext->handle;
	base  = mci->base;
	
	return (MMC_SUCCESS);
}

static int _mci_powerdown(SIM_HBA *hba)
{
	CONFIG_INFO			*cfg;
	SIM_MMC_EXT			*ext;
	mci_ext_t			*mci;

	ext = (SIM_MMC_EXT *)hba->ext;
	cfg = (CONFIG_INFO *)&hba->cfg;
	mci = (mci_ext_t *)ext->handle;

	return (MMC_SUCCESS);
}

static int _mci_shutdown(SIM_HBA *hba)
{
	CONFIG_INFO			*cfg;
	SIM_MMC_EXT			*ext;
	mci_ext_t			*mci;

	ext = (SIM_MMC_EXT *)hba->ext;
	cfg = (CONFIG_INFO *)&hba->cfg;
	mci = (mci_ext_t *)ext->handle;
	_mci_powerdown(hba);

	munmap_device_memory((void *)mci->base, cfg->MemLength[0]);

	free(mci);

	return (MMC_SUCCESS);
}

void mci_dump_regs(SIM_HBA *hba) 
{ 

        SIM_MMC_EXT             *ext;
        mci_ext_t             *mci;
        uintptr_t               base;

        ext   = (SIM_MMC_EXT *)hba->ext;
        mci = (mci_ext_t *)ext->handle;
        base  = mci->base;
}

int	mci_init(SIM_HBA *hba)
{
	CONFIG_INFO			*cfg;
	SIM_MMC_EXT			*ext;
	mci_ext_t			*mci;
	uintptr_t			base;
	uint32_t 			*dbgu_base;
	uint32_t cpu_id;
	
	ext = (SIM_MMC_EXT *)hba->ext;
	cfg = (CONFIG_INFO *)&hba->cfg;
	hba->verbosity = 4;

	if ((mci = calloc(1, sizeof(mci_ext_t))) == NULL)
		return (MMC_FAILURE);

	cfg->MemLength[0] = 0x1000;
        cfg->NumMemWindows = 1;

	dbgu_base = mmap_device_memory(NULL, DBGU_SIZE,
                                PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_SHARED, DBGU_BASE);

	cpu_id = *(dbgu_base + 0x10);

	if ((cpu_id != ID_RL64) && (cpu_id != ID_9260) && (cpu_id != ID_9261)) {
		dbgu_base = mmap_device_memory(NULL, DBGU_SIZE,
                                PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_SHARED, DBGU_BASE1);
        	cpu_id = *(dbgu_base + 0x10);
	}
        
	switch (cpu_id) {

        case ID_RL64:
                cfg->MemBase[0] = cfg->IOPort_Base[0]; /* MCI */
		break;
	
	case ID_9260:
                cfg->MemBase[0] = cfg->IOPort_Base[0]; /* MCI */
		break;

	case ID_9261:
                cfg->MemBase[0] = cfg->IOPort_Base[0]; /* MCI */
		break;

	case ID_9263:
                cfg->MemBase[0] = cfg->IOPort_Base[0]; /* MCI */
		break;

        default:
                cfg->MemBase[0] = cfg->IOPort_Base[0]; /* MCI */
		break;

         }
	 
	base = (uintptr_t)mmap_device_memory(NULL, cfg->MemLength[0],
                                PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_SHARED, cfg->MemBase[0]);
        if (base == (uintptr_t)MAP_FAILED) {
                printf("mmap_device_memory failed\n");
                return (MMC_FAILURE);
        }
	
	/* Disable the controller */
	WRITE_MCI(MCI_CR, SWRST);
	delay (100);
	WRITE_MCI(MCI_CR, MCIDIS | PWSDIS); 

	/* Enable the controller */
	WRITE_MCI(MCI_CR, MCIEN | PWSEN);
	WRITE_MCI(MCI_IDR, 0xffffffff);
	WRITE_MCI(MCI_DTOR, 0x0f | (7 << 4));
	WRITE_MCI(MCI_MR, 0x59 | (3 << 8) | PDCMODE | (512 << 16));
	if (cpu_id == ID_9260) 
		WRITE_MCI(MCI_SDCR, SDCBUS | SDCSEL1);
	else
		WRITE_MCI(MCI_SDCR, SDCBUS);

	/* Dump registers */
	/* mci_dumpregs(base);			used for debugging */

	ext->hccap |= MMC_HCCAP_ACMD12 | MMC_HCCAP_BW1 | MMC_HCCAP_BW4 | MMC_HCCAP_DMA ;	// | MMC_HCCAP_HS;

	mci->clock = 100000000;
	mci->base  = base;
	mci->hba   = hba;

	ext->handle    = mci;
	ext->clock     = mci->clock;
	ext->detect    = _mci_detect;
	ext->powerup   = _mci_powerup;
	ext->powerdown = _mci_powerdown;
	ext->cfg_bus   = _mci_cfg_bus;
	ext->set_clock = _mci_clock;
	ext->set_blksz = _mci_block_size;
	ext->interrupt = _mci_interrupt;
	ext->command   = _mci_command;
	ext->setup_dma = _mci_setup_dma;
	ext->dma_done  = _mci_dma_done;
	ext->setup_pio = _mci_setup_pio;
	ext->pio_done  = _mci_pio_done;
	ext->shutdown  = _mci_shutdown;


	if (!cfg->Description[0])
		strncpy(cfg->Description, "Generic MCI", sizeof(cfg->Description));

	return (MMC_SUCCESS);
}

#endif

__SRCVERSION("sim_mci.c $Rev: 140019 $");
