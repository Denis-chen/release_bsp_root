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

#ifdef MMCSD_VENDOR_ESDHC

 
#include	<sim_esdhc.h>

#define	ESDHC_CARD_STABLE	(ESDHC_PSTATE_CD | ESDHC_PSTATE_CI)

static int _esdhc_detect(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	esdhc_ext_t		*esdhc;
	uintptr_t		base;
	uint32_t		i;

	ext   = (SIM_MMC_EXT *)hba->ext;
	esdhc = (esdhc_ext_t *)ext->handle;
	base  = esdhc->base;
	if (!(ESDHC_PSTATE(base) & ESDHC_PSTATE_CI))
		return (MMC_FAILURE);

	i = 0;
	while ((ESDHC_PSTATE(base) & ESDHC_CARD_STABLE) != ESDHC_CARD_STABLE) {
		if (++i > 1000)
			return (MMC_FAILURE);
		delay(1);
	}

	return (MMC_SUCCESS);
}

static int _esdhc_interrupt(SIM_HBA *hba, int irq, int resp_type, uint32_t *resp)
{
	SIM_MMC_EXT		*ext;
	esdhc_ext_t		*esdhc;
	uintptr_t		base;
	uint16_t		nsts, ersts, nmask, ermask;
	int				intr = 0;
	uint16_t 		temp;

	ext   = (SIM_MMC_EXT *)hba->ext;
	esdhc = (esdhc_ext_t *)ext->handle;
	base  = esdhc->base;

	nsts   = ESDHC_NINTSTS_R(base);
	ersts  = ESDHC_ERINTSTS_R(base);
	nmask  = ESDHC_NINTEN_R(base);
	ermask = ESDHC_ERINTEN_R(base);

	if (nsts & ESDHC_NINT_DMA) {		// DMA interrupt
		if (!(nsts & ESDHC_NINT_TC)) 	// Data not completed ?
			ESDHC_DMAADR(base) = ESDHC_DMAADR(base);
	}

	if (nsts & ESDHC_NINT_EI)
		slogf(_SLOGC_SIM_MMC, _SLOG_ERROR, "esdhc_init: error 0x%04x", ersts);

	if (nmask & ESDHC_NINT_CC) {		// Command complete interrupt enabled ?
		if (ersts & ESDHC_ERINT_CTE) {	// Command Timeout ?
			intr |= MMC_ERR_CMD_TO | MMC_INTR_ERROR | MMC_INTR_COMMAND;

			ESDHC_SWRST_W(base, ESDHC_SWRST_RC | ESDHC_SWRST_RD);
			while (ESDHC_SWRST_R(base) & (ESDHC_SWRST_RC | ESDHC_SWRST_RD))
				delay(1);
		} else if (nsts & ESDHC_NINT_CC) {
			intr |= MMC_INTR_COMMAND;
			if (ersts & (ESDHC_ERINT_CCE | ESDHC_ERINT_CEBE | ESDHC_ERINT_CIE))
				intr |= MMC_INTR_ERROR;	// Other errors
			if (resp_type & MMC_RSP_136) {
				resp[0] = ESDHC_RESP0(base);
				resp[1] = ESDHC_RESP1(base);
				resp[2] = ESDHC_RESP2(base);
				resp[3] = ESDHC_RESP3(base);

				/*
				 * CRC is not included in the response register,
				 * we have to left shift 8 bit to match the 128 CID/CSD structure
				 */
				resp[3] = (resp[3] << 8) | (resp[2] >> 24);
				resp[2] = (resp[2] << 8) | (resp[1] >> 24);
				resp[1] = (resp[1] << 8) | (resp[0] >> 24);
				resp[0] = (resp[0] << 8);
			} else if (resp_type & MMC_RSP_PRESENT) {
				resp[0] = ESDHC_RESP0(base);
				resp[1] = ESDHC_RESP1(base);
				resp[2] = ESDHC_RESP2(base);
				resp[3] = ESDHC_RESP3(base);
			}
		}
		/*
		 * We only clear command related error status here
		 */
		ESDHC_ERINTSTS_W(base, ersts & 0x0F);
	}

	/*
	 * Check for data related interrupts
	 * FIXME!!! ACMD12 error ?
	 */
	if (nmask & (ESDHC_NINT_TC | ESDHC_NINT_BRR | ESDHC_NINT_BWR)) {
		if (nsts & ESDHC_NINT_BRR)
			intr |= MMC_INTR_RBRDY;
		else if (nsts & ESDHC_NINT_BWR)
			intr |= MMC_INTR_WBRDY;
		if (nsts & ESDHC_NINT_TC) {		// data done
			intr |= MMC_INTR_DATA;
		} else if (ersts & 0x1FF) {		// errors
			intr |= MMC_ERR_DATA_TO | MMC_INTR_ERROR;
			ESDHC_SWRST_W(base, ESDHC_SWRST_RC | ESDHC_SWRST_RD);
			while (ESDHC_SWRST_R(base) & (ESDHC_SWRST_RC | ESDHC_SWRST_RD))
				delay(1);
		}
		if (ersts & (ESDHC_ERINT_DTE | ESDHC_ERINT_DCE | ESDHC_ERINT_DEBE))
			ESDHC_ERINTSTS_W(base, ersts & (ESDHC_ERINT_DTE | ESDHC_ERINT_DCE | ESDHC_ERINT_DEBE));
	}

	if (nsts & ESDHC_NINT_CIN) { 
		intr |= MMC_INTR_CARDINS;
		temp = ESDHC_NINTEN_R(base);		
		ESDHC_NINTEN_W(base, temp & ~ESDHC_NINT_CIN);
		temp = ESDHC_NINTSIGEN_R(base);
		ESDHC_NINTSIGEN_W(base, temp & ~ESDHC_NINT_CIN);
	}
	if (nsts & ESDHC_NINT_CRM) {
		intr |= MMC_INTR_CARDRMV;
		temp = ESDHC_NINTEN_R(base);
		ESDHC_NINTEN_W(base, temp & ~ESDHC_NINT_CRM);
		temp = ESDHC_NINTSIGEN_R(base);
		ESDHC_NINTSIGEN_W(base, temp & ~ESDHC_NINT_CRM);
	}

	ESDHC_NINTSTS_W(base, nsts);		// clear interrupt status

	return intr;
}

/*
 * setup DMA transfer
 */
static int _esdhc_setup_dma(SIM_HBA *hba, paddr_t paddr, int len, int dir)
{
	SIM_MMC_EXT		*ext;
	esdhc_ext_t		*esdhc;
	uintptr_t		base;
	uint32_t		xlen;
	uint16_t		blkcnt;
	uint16_t		xmode;

	ext   = (SIM_MMC_EXT *)hba->ext;
	esdhc = (esdhc_ext_t *)ext->handle;
	base  = esdhc->base;

	xlen = len;
	blkcnt = xlen / esdhc->blksz;
	xlen   = esdhc->blksz * blkcnt;

	if (blkcnt == 0)
		return 0;

	xmode = ESDHC_XFRMODE_DMAEN;

	if (blkcnt > 1)
		xmode |= ESDHC_XFRMODE_BCE | ESDHC_XFRMODE_AC12EN | ESDHC_XFRMODE_MBS;

	if (dir == MMC_DIR_IN)
		xmode |= ESDHC_XFRMODE_DATDIR;

	// only valid for multi-block transfer
	ESDHC_BLKCNT_W(base, blkcnt);
	esdhc->xmode = xmode;
	ESDHC_DMAADR(base)  = paddr;

	return (xlen);
}

/*
 * setup PIO transfer
 */
static int _esdhc_setup_pio(SIM_HBA *hba, int len, int dir)
{
	SIM_MMC_EXT		*ext;
	esdhc_ext_t		*esdhc;
	uintptr_t		base;
	uint16_t		blkcnt, xmode = 0;

	ext   = (SIM_MMC_EXT *)hba->ext;
	esdhc = (esdhc_ext_t *)ext->handle;
	base  = esdhc->base;

	blkcnt = len / esdhc->blksz;

	if (blkcnt == 0)
		return 0;

	if (blkcnt > 1)
		xmode |= ESDHC_XFRMODE_BCE | ESDHC_XFRMODE_AC12EN | ESDHC_XFRMODE_MBS;

	if (dir == MMC_DIR_IN)
		xmode |= ESDHC_XFRMODE_DATDIR;

	// only valid for multi-block transfer
	ESDHC_BLKCNT_W(base, blkcnt);
	esdhc->xmode = xmode;

	return (blkcnt * esdhc->blksz);
}

static int _esdhc_dma_done(SIM_HBA *hba, int dir)
{
	return MMC_SUCCESS;
}

static int _esdhc_pio_done(SIM_HBA *hba, char *buf, int len, int dir)
{
	SIM_MMC_EXT		*ext;
	esdhc_ext_t		*esdhc;
	uintptr_t		base;
	uint32_t		cnt, *pbuf = (uint32_t *)buf;
	uint32_t                i;

	ext   = (SIM_MMC_EXT *)hba->ext;
	esdhc = (esdhc_ext_t *)ext->handle;
	base  = esdhc->base;

	if (len < esdhc->blksz)
		return 0;
	if (dir == MMC_DIR_IN) {

		for (i = 0; !(ESDHC_PSTATE(base) & ESDHC_PSTATE_BRE); i++) {
			if (i > 200000) {
				fprintf(stderr, "[%s %d] ESDHC_PSTATE_BRE not set.\n",__FUNCTION__, __LINE__);
			return 0;
			}
			nanospin_ns(100);
		}  		


		for (cnt = 0; cnt < esdhc->blksz; cnt += 4) {
			*pbuf = ESDHC_BUFDATA(base);
			*pbuf = ENDIAN_LE32(*pbuf);
			pbuf++;

			/* Added sleep temporarily */
			nanospin_ns(1000);
		}

		

	} else {
		
		for (i = 0; !(ESDHC_PSTATE(base) & ESDHC_PSTATE_BUFWREN); i++) {
			if (i > 200000) {                                
				fprintf(stderr, "[%s %d] ESDHC_PSTATE_BUFWREN not set.\n",__FUNCTION__, __LINE__);
			return 0;
			}
			nanospin_ns(100);
		} 
		
		for (cnt = 0; cnt < esdhc->blksz; cnt += 4) {
			*pbuf = ENDIAN_LE32(*pbuf);
			ESDHC_BUFDATA(base) = *pbuf;
			pbuf++;

			/* Added sleep temporarily */
			nanospin_ns(10000000);
		}
	}

	return esdhc->blksz;
}

static int _esdhc_command(SIM_HBA *hba, mmc_cmd_t *cmd)
{
	SIM_MMC_EXT		*ext;
	esdhc_ext_t		*esdhc;
	uintptr_t		base;
	uint16_t		command;
	uint32_t		i;
	uint16_t		mask = ESDHC_PSTATE_CINH;

	ext   = (SIM_MMC_EXT *)hba->ext;
	esdhc = (esdhc_ext_t *)ext->handle;
	base  = esdhc->base;
	command = ESDHC_CMD_CMDIDX(cmd->opcode);

	if (cmd->resp & MMC_RSP_PRESENT) {
		if (cmd->resp & MMC_RSP_OPCODE)
			command |= ESDHC_CMD_CICE;

		if (cmd->resp & MMC_RSP_CRC)
			command |= ESDHC_CMD_CCCE;

		if (cmd->resp & MMC_RSP_BUSY) {			// must be R1b
			command |= ESDHC_CMD_RSPLEN48b;
			mask |= ESDHC_PSTATE_DCI;
		} else if (cmd->resp & MMC_RSP_136) 	// must be R2
			command |= ESDHC_CMD_RSPLEN136;
		else
			command |= ESDHC_CMD_RSPLEN48;
	}

	if (cmd->eflags & MMC_CMD_DATA) {
		mask |= ESDHC_PSTATE_DCI;
		command |= ESDHC_CMD_DPS;
	}

	for (i = 0; ESDHC_PSTATE(base) & mask; i++) {
		if (i > 1000000) { 
                        fprintf(stderr, "[%s %d] Controller never released inhibit bit(s).\n",__FUNCTION__, __LINE__);
			return (MMC_FAILURE);
		}
		nanospin_ns(100);
	}

	/*
	 * Only enable the interrupts we want
	 */
	mask = 0;
	if (cmd->eflags & MMC_CMD_DATA) {
		ESDHC_ERINTEN_W(base, 0x017F);
		if (cmd->eflags & MMC_CMD_DATA_DMA) {
			mask |= ESDHC_NINT_DMA;
			mask |= ESDHC_NINT_TC;
		}
		else if (cmd->eflags & MMC_CMD_DATA_IN)
			mask |= ESDHC_NINT_BRR;
		else
			mask |= ESDHC_NINT_BWR | ESDHC_NINT_TC;
	} else {
//		ESDHC_ERINTEN(base) = 0x000F;
		ESDHC_ERINTEN_W(base, 0x017F);
		mask |= ESDHC_NINT_CC;
	}

	ESDHC_NINTEN_W(base, mask);
	ESDHC_CMDARG(base) = cmd->argument;
	if (cmd->eflags & MMC_CMD_DATA)
		ESDHC_SDCMD_W32(base) = ((command << 16) | esdhc->xmode);	
	else
		ESDHC_SDCMD_W32(base) = (command << 16);
	return (MMC_SUCCESS);
}

static int _esdhc_cfg_bus(SIM_HBA *hba, int width, int mmc)
{
	SIM_MMC_EXT		*ext;
	esdhc_ext_t		*esdhc;
	uintptr_t		base;
	uint8_t			hostctl;

	ext   = (SIM_MMC_EXT *)hba->ext;
	esdhc = (esdhc_ext_t *)ext->handle;
	base  = esdhc->base;

	hostctl = ESDHC_HOSTCTL_R(base) & ~ESDHC_HOSTCTL_DTW4BIT;

	if (width == 4)
		hostctl |= ESDHC_HOSTCTL_DTW4BIT;

	ESDHC_HOSTCTL_W(base, hostctl);

	return (MMC_SUCCESS);
}

#define ESDHC_PREDIV_SHIFT      8 
#define ESDHC_DIVIDER_SHIFT     4

static int _esdhc_clock(SIM_HBA *hba, int *clock, int high_speed)
{
	SIM_MMC_EXT		*ext;
	esdhc_ext_t		*esdhc;
	uintptr_t		base;
	uint16_t		clkctl;
	uint32_t		max_clk;
        int div, pre_div;
        unsigned long timeout;

	ext   = (SIM_MMC_EXT *)hba->ext;
	esdhc = (esdhc_ext_t *)ext->handle;
	base  = esdhc->base;
	max_clk = esdhc->clock;
	clkctl = ESDHC_CLKCTL_R(base); 
	ESDHC_CLKCTL_W(base, clkctl & 0x000F);
	
	if (*clock == 0 || *clock > max_clk)
		return (MMC_FAILURE);

        if (max_clk / 16 > *clock) {
                for (pre_div = 1; pre_div < 256; pre_div *= 2) {
                        if ((max_clk / pre_div) < (*clock*16))
                                break;
                }
        } else
                pre_div = 1;

        for (div = 1; div <= 16; div++) {
                if ((max_clk / (div*pre_div)) <= *clock)
                        break;
        }

        pre_div >>= 1;
        div -= 1;
	clkctl 	= ESDHC_CLKCTL_R(base);
	
	clkctl = clkctl | (div << ESDHC_DIVIDER_SHIFT) | (pre_div << ESDHC_PREDIV_SHIFT);
	ESDHC_CLKCTL_W(base, clkctl);

	/* Wait max 10 ms */
        timeout = 10;
        while (timeout) {
                timeout--;
                delay(1);
        }  

	clkctl  = ESDHC_CLKCTL_R(base);
	ESDHC_CLKCTL_W(base, clkctl | ESDHC_CLKCTL_CLKEN);

	return (MMC_SUCCESS);
}

static int _esdhc_block_size(SIM_HBA *hba, int blksz)
{
	SIM_MMC_EXT		*ext;
	esdhc_ext_t		*esdhc;
	uintptr_t		base;

	ext   = (SIM_MMC_EXT *)hba->ext;
	esdhc = (esdhc_ext_t *)ext->handle;
	base  = esdhc->base;

	if (blksz > 4095)
		return (MMC_FAILURE);

	esdhc->blksz = blksz;

	ESDHC_BLKSZ_W(base, blksz);
	return (MMC_SUCCESS);
}

/*
 * Reset host controller and card
 * The clock should be enabled and set to minimum (<400KHz)
 */
static int _esdhc_powerup(SIM_HBA *hba)
{
	SIM_MMC_EXT		*ext;
	esdhc_ext_t		*esdhc;
	uintptr_t		base;
	uint32_t		clk;
	uint16_t		clkctl;
	uint16_t		temp;

	ext   = (SIM_MMC_EXT *)hba->ext;
	esdhc = (esdhc_ext_t *)ext->handle;
	base  = esdhc->base;

	/*
	 * Card is in
	 */
	if (ESDHC_PSTATE(base) & ESDHC_PSTATE_WP)
		ext->eflags &= ~MMC_EFLAG_WP;
	else
		ext->eflags |= MMC_EFLAG_WP;

	/*
	 * Apply a software reset
	 */
	ESDHC_SWRST_W(base, ESDHC_SWRST_ALL);
	delay(10);
	temp = ESDHC_SWRST_R(base);
	ESDHC_SWRST_W(base, temp | (1<<3)); /* INITA */

	/*
	 * Enable internal clock
	 * Set clock to 400KHz for ident
	 */
	clk = esdhc->clock;
	for (clkctl = 0; clkctl <= 8; clkctl++) {
		if ((clk / (1 << clkctl)) <= 400 * 1000) {
			clkctl = (1 << (clkctl - 1)) << 8;
			break;
		}
	}
	clkctl = 0x20C0;
	ESDHC_CLKCTL_W(base, clkctl | ESDHC_CLKCTL_ICE);

	temp = ESDHC_CLKCTL_R(base); 
	ESDHC_CLKCTL_W(base , temp | (ESDHC_CLKCTL_CLKEN | ESDHC_CLKCTL_ICS));


	/*
	 * Currently set the timeout to maximum
	 */
	ESDHC_TOCTL_W(base, 0x0E);

	/*
	 * Enable interrupt signals
	 */
	ESDHC_NINTSIGEN_W(base, 0x00BF);
	ESDHC_ERINTSIGEN_W(base, 0x01FF);

	/*
	 * disable all interrupts except card insertion and removal
	 */
	ESDHC_ERINTEN_W(base, 0x0000);
	ESDHC_NINTEN_W(base, ESDHC_NINT_CRM);

	return (MMC_SUCCESS);
}

static int _esdhc_powerdown(SIM_HBA *hba)
{
	CONFIG_INFO			*cfg;
	SIM_MMC_EXT			*ext;
	esdhc_ext_t			*esdhc;

	ext = (SIM_MMC_EXT *)hba->ext;
	cfg = (CONFIG_INFO *)&hba->cfg;
	esdhc = (esdhc_ext_t *)ext->handle;

	/* Disable clocks */
	ESDHC_CLKCTL_W(esdhc->base, 0x00);

	return (MMC_SUCCESS);
}

static int _esdhc_shutdown(SIM_HBA *hba)
{
	CONFIG_INFO			*cfg;
	SIM_MMC_EXT			*ext;
	esdhc_ext_t			*esdhc;

	ext = (SIM_MMC_EXT *)hba->ext;
	cfg = (CONFIG_INFO *)&hba->cfg;
	esdhc = (esdhc_ext_t *)ext->handle;

	/* soft reset all */
	ESDHC_SWRST_W(esdhc->base, ESDHC_SWRST_ALL);
	delay(10);

	_esdhc_powerdown(hba);

	munmap_device_memory((void *)esdhc->base, cfg->MemLength[0]);

	free(esdhc);

	return (MMC_SUCCESS);
}
void esdhc_dump_regs(SIM_HBA *hba) 
{ 

        SIM_MMC_EXT             *ext;
        esdhc_ext_t             *esdhc;
        uintptr_t               base;

        ext   = (SIM_MMC_EXT *)hba->ext;
        esdhc = (esdhc_ext_t *)ext->handle;
        base  = esdhc->base;
 
	fprintf(stderr, "-------------ESDHC Regs-------------\n");
	fprintf(stderr, "ESDHC_DMAADR %08x\n", ESDHC_DMAADR(base));

	fprintf(stderr, "ESDHC_BLKCNT %04x - ", ESDHC_BLKCNT_R(base));
	fprintf(stderr, "ESDHC_BLKSZ %04x\n", ESDHC_BLKSZ_R(base));

	fprintf(stderr, "ESDHC_SDCMD %04x - ", ESDHC_SDCMD_R(base));
	fprintf(stderr, "ESDHC_XFRMODE %04x\n", ESDHC_XFRMODE_R(base));

	fprintf(stderr, "ESDHC_RESP0 %08x\n", ESDHC_RESP0(base));

	fprintf(stderr, "ESDHC_RESP1 %08x\n", ESDHC_RESP1(base));

	fprintf(stderr, "ESDHC_RESP2 %08x\n", ESDHC_RESP2(base));

	fprintf(stderr, "ESDHC_RESP3 %08x\n", ESDHC_RESP3(base));

	fprintf(stderr, "ESDHC_BUFDATA %08x\n", ESDHC_BUFDATA(base));

	fprintf(stderr, "ESDHC_PSTATE %08x\n", ESDHC_PSTATE(base));

	fprintf(stderr, "ESDHC_WAKECTL %02x - ", ESDHC_WAKECTL_R(base));
	fprintf(stderr, "ESDHC_BLKGAPCTL %02x - ", ESDHC_BLKGAPCTL_R(base));
	fprintf(stderr, "ESDHC_PWRCTL %02x - ", ESDHC_PWRCTL_R(base));
	fprintf(stderr, "ESDHC_HOSTCTL %02x\n", ESDHC_HOSTCTL_R(base));

	fprintf(stderr, "ESDHC_SWRST %02x - ", ESDHC_SWRST_R(base));
	fprintf(stderr, "ESDHC_TOCTL %02x - ", ESDHC_TOCTL_R(base));
	fprintf(stderr, "ESDHC_CLKCTL %04x\n", ESDHC_CLKCTL_R(base));

	fprintf(stderr, "ESDHC_ERINTSTS %04x - ", ESDHC_ERINTSTS_R(base));
	fprintf(stderr, "ESDHC_NINTSTS %04x\n", ESDHC_NINTSTS_R(base));

	fprintf(stderr, "ESDHC_ERINTEN %04x - ", ESDHC_ERINTEN_R(base));
	fprintf(stderr, "ESDHC_NINTEN %04x\n", ESDHC_NINTEN_R(base));

	fprintf(stderr, "ESDHC_ERINTSIGEN %04x - ", ESDHC_ERINTSIGEN_R(base));
	fprintf(stderr, "ESDHC_NINTSIGEN %04x\n", ESDHC_NINTSIGEN_R(base));

	fprintf(stderr, "ESDHC_AC12ERRSTS %08x\n", ESDHC_AC12ERRSTS(base));

	fprintf(stderr, "ESDHC_CAP %08x\n", ESDHC_CAP(base));
	fprintf(stderr, "ESDHC_MCCAP %08x\n", ESDHC_MCCAP(base));
	fprintf(stderr, "ESDHC_HOSTVER %08x\n", ESDHC_SLTINTSTS(base));
	fprintf(stderr, "ESDHC_CTRLRVER %08x\n", ESDHC_CTRLRVER(base));
	fprintf(stderr, "ESDHC_WML %08x\n", ESDHC_WML(base));
	fprintf(stderr, "ESDHC_FEVT %08x\n", ESDHC_FEVT(base));
	fprintf(stderr, "ESDHC_SCR %08x\n", ESDHC_SCR(base));
	fprintf(stderr, "------------------------------------\n");
}

int	esdhc_init(SIM_HBA *hba)
{
	CONFIG_INFO			*cfg;
	SIM_MMC_EXT			*ext;
	esdhc_ext_t			*esdhc;
	uintptr_t			base;
	uint32_t			reg;
	uint16_t			temp;

	ext = (SIM_MMC_EXT *)hba->ext;
	cfg = (CONFIG_INFO *)&hba->cfg;
	hba->verbosity = 4;
	if ((esdhc = calloc(1, sizeof(esdhc_ext_t))) == NULL)
		return (MMC_FAILURE);

        cfg->IRQRegisters[0] = 56; /* eSDHC */
        cfg->NumIRQs =1;

        cfg->MemBase[0] = 0xFFE00000 + 0x2E000;
        cfg->MemLength[0] = 0x1000;
        cfg->NumMemWindows = 1;

	base = (uintptr_t)mmap_device_memory(NULL, cfg->MemLength[0],
				PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_SHARED, cfg->MemBase[0]);
	if (base == (uintptr_t)MAP_FAILED) {
		printf("mmap_device_memory failed\n");
		return (MMC_FAILURE);
	}
	/* ESDHC_HOSTCTL[EMODE] = 10 */
	temp = ESDHC_HOSTCTL_R(base);
	ESDHC_HOSTCTL_W(base, temp | (1<<5)); 

	/* SDCHI_SCR[SNOOP] = 1 */
	ESDHC_SCR(base) = 0x00000040;

        uint32_t               *guts_base;
        guts_base = mmap_device_memory(NULL, 0x4,
                                PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_SHARED, 0xffee0060);
        if ((uintptr_t)guts_base == (uintptr_t)MAP_FAILED) {
                printf("mmap_device_memory failed\n");
                return (MMC_FAILURE);
        }

        *(guts_base) |= (1<<30); /* PMUXCR[SDHC_CD] */
        *(guts_base) |= (1<<29); /* PMUXCR[SDHC_WP] */

	ext->hccap |= MMC_HCCAP_ACMD12 | MMC_HCCAP_BW1 | MMC_HCCAP_BW4 | MMC_HCCAP_CD_INTR;

	reg = ESDHC_CAP(base);

	if (reg & ESDHC_CAP_S18)
		ext->hccap |= MMC_HCCAP_18V;

	if (reg & ESDHC_CAP_S30)
		ext->hccap |= MMC_HCCAP_30V;

	if (reg & ESDHC_CAP_S33)
		ext->hccap |= MMC_HCCAP_33V;

	if (reg & ESDHC_CAP_DMA)
		ext->hccap |= MMC_HCCAP_DMA;

	if (reg & ESDHC_CAP_HS)
		ext->hccap |= MMC_HCCAP_HS;

	esdhc->clock = 396000000;
	esdhc->base  = base;
	esdhc->hba   = hba;

	ext->handle    = esdhc;
	ext->clock     = esdhc->clock;
	ext->detect    = _esdhc_detect;
	ext->powerup   = _esdhc_powerup;
	ext->powerdown = _esdhc_powerdown;
	ext->cfg_bus   = _esdhc_cfg_bus;
	ext->set_clock = _esdhc_clock;
	ext->set_blksz = _esdhc_block_size;
	ext->interrupt = _esdhc_interrupt;
	ext->command   = _esdhc_command;
	ext->setup_dma = _esdhc_setup_dma;
	ext->dma_done  = _esdhc_dma_done;
	ext->setup_pio = _esdhc_setup_pio;
	ext->pio_done  = _esdhc_pio_done;
	ext->shutdown  = _esdhc_shutdown;

	/*
	 * Enable interrupt signals
	 */
	ESDHC_NINTSIGEN_W(base, 0x007F);
	ESDHC_ERINTSIGEN_W(base, 0x01FF);

	/*
	 * disable all interrupts except card insertion / removal
	 */
	ESDHC_ERINTEN_W(base, 0);
	ESDHC_NINTEN_W(base, ESDHC_NINT_CIN);

	if (!cfg->Description[0])
		strncpy(cfg->Description, "Generic ESDHC", sizeof(cfg->Description));

	return (MMC_SUCCESS);
}

#endif

__SRCVERSION("sim_esdhc.c $Rev: 140019 $");
