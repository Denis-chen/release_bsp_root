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





#include "startup.h"

void
print_arm_boxinfo()
{
	struct arm_boxinfo_entry *box = lsp.cpu.arm_boxinfo.p;

	kprintf("  hw_flags:%l\n", box->hw_flags);
}

void
print_arm_cpu()
{
	struct arm_cpu_entry *cpu = lsp.cpu.arm_cpu.p;

	kprintf("  page_flush:%l page_flush_deferred:%l\n", cpu->page_flush, cpu->page_flush_deferred);
	kprintf("  upte_ro:%l upte_rw:%l\n", cpu->upte_ro, cpu->upte_rw);
	kprintf("  kpte_ro:%l kpte_rw:%l\n", cpu->kpte_ro, cpu->kpte_rw);
	kprintf("  mask_nc:%l\n", cpu->mask_nc);
	kprintf("  mmu_cr1:%l set:%l clr:%x -> %l\n", arm_mmu_getcr(), mmu_cr_set, mmu_cr_clr, (arm_mmu_getcr() & ~mmu_cr_clr) | mmu_cr_set);
}

__SRCVERSION("cpu_debug.c $Rev: 262077 $");
