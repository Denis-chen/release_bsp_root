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
print_typed_strings(void) {
	struct typed_strings_entry	*string = lsp.typed_strings.p;
	unsigned	type;
	unsigned	i;

	i = 0;
	for( ;; ) {
		type = *(uint32_t *)&string->data[i];
		if(type == _CS_NONE) break;
		i += sizeof(uint32_t);
		kprintf("  off:%d type:%d string:'%s'\n", i-sizeof(uint32_t), type, &string->data[i]);
		i += strlen(&string->data[i]) + 1;
		i = ROUND(i, sizeof(uint32_t));
	}
}


void
print_strings(void) {
	char		*p = lsp.strings.p->data;
	char		*start = p;
	unsigned	off;
	unsigned	len;
	char		buff[80];

	kprintf(" ");
	off = 1;
	while(*p != '\0') {
		ksprintf(buff, " [%d]'%s'", p - start, p);
		len = strlen(buff);
		if((off + len) >= 80) {
			kprintf("\n ");
			off = 1;
		}
		kprintf("%s", buff);
		off += len;
		p += strlen(p) + 1;
	}
	kprintf("\n");
}


void
print_system_private(void) {
	struct system_private_entry	*private = lsp.system_private.p;
	unsigned				i;

	kprintf("  syspage ptr user:%l kernel:%l\n", private->user_syspageptr, private->kern_syspageptr);
	kprintf("  cpupage ptr user:%l kernel:%l spacing:%d\n", private->user_cpupageptr, private->kern_cpupageptr, private->cpupage_spacing);
	kprintf("  kdebug info:%l callback:%l\n", private->kdebug_info, private->kdebug_call);
	kprintf("  boot pgms: idx=%d\n", private->boot_idx);
	i = 0;
	for( ;; ) {
		if(i >= NUM_ELTS(private->boot_pgm)) break;
		if(private->boot_pgm[i].entry == 0) break;
		kprintf("    %d) base paddr:%l start addr:%l\n",
				i, private->boot_pgm[i].base, private->boot_pgm[i].entry);
		++i;
	}
	kprintf("  ramsize:%l pagesize:%l\n", private->ramsize, private->pagesize);
}


void
print_meminfo(void) {
	struct meminfo_entry *ram = lsp.meminfo.p;
	int i = 0;

	kprintf(" ");
	while(ram->type != MEMTYPE_NONE) {
		if(++i%4 == 0)
			kprintf("\n ");
		kprintf(" t:%d a:%l s:%l", ram->type, ram->addr, ram->size);
		++ram;
	}
	kprintf("\n");
}


void
print_asinfo(void) {
	struct asinfo_entry 	*as = lsp.asinfo.p;
	int						num = lsp.asinfo.size / sizeof(*as);
	int						i;

	for(i = 0; i < num; ++i) {
		kprintf("  %w) %L-%L o:%w a:%w p:%d c:%l n:%d\n",
				i*sizeof(*as),
				as->start,
				as->end,
				as->owner,
				as->attr,
				as->priority,
				as->alloc_checker,
				as->name);
		++as;
	}
}


void
print_hwinfo(void) {
	hwi_tag				*tag = (hwi_tag *)lsp.hwinfo.p->data;
	void				*base;
	void				*next;
	char				*name;

	while(tag->prefix.size != 0) {
		next = (hwi_tag *)((uint32_t *)tag + tag->prefix.size);
		base = (void *)(&tag->prefix + 1);
		kprintf("  %d) size:%d tag:%d", hwi_tag2off(tag), tag->prefix.size, tag->prefix.name);
		name = __hwi_find_string(tag->prefix.name);
		if(*name >= 'A' && *name <= 'Z') {
			base = (void *) (&tag->item + 1);
			kprintf(" isize:%d, iname:%d, owner:%d, kids:%d",
					tag->item.itemsize, tag->item.itemname,
					tag->item.owner, tag->item.kids);
		}
		if(base != next) {
			kprintf("\n    ");
			while(base < next) {
				uint8_t		*p = base;
	
				kprintf(" %b", *p);
				base = p + 1;
			}
		}
		kprintf("\n");
		tag = next;
	}
}


void
print_qtime(void) {
	struct qtime_entry *qtime = lsp.qtime.p;

	kprintf("  boot:%l CPS:%l%l rate/scale:%d/-%d intr:%d\n",
		qtime->boot_time,
		(unsigned long)(qtime->cycles_per_sec >> 32),
		(unsigned long)qtime->cycles_per_sec,
		qtime->timer_rate,
		-(int)qtime->timer_scale,
		(int)qtime->intr
		);
}

void
print_cpuinfo(void) {
 	struct cpuinfo_entry *cpu = lsp.cpuinfo.p;
	unsigned i;

	for( i = 0; i < lsp.syspage.p->num_cpu; ++i ) {
		kprintf("  %d) cpu:%l flags:%l speed:%l cache i/d:%d/%d name:%d\n",
			i,
			cpu[i].cpu,
			cpu[i].flags,
			cpu[i].speed,
			cpu[i].ins_cache,
			cpu[i].data_cache,
			cpu[i].name);
	}
}

void
print_cacheattr(void) {
 	struct cacheattr_entry *cache = lsp.cacheattr.p;
	int						num = lsp.cacheattr.size / sizeof(*cache);
	int						i;

	for( i = 0; i < num; ++i ) {
		kprintf("  %d) flags:%b size:%w #lines:%w control:%l next:%d\n",
			i,
			cache[i].flags,
			cache[i].line_size,
			cache[i].num_lines,
			cache[i].control,
			cache[i].next);
	}
}


void
print_callout(void) {
	struct callout_entry	*call = lsp.callout.p;
	unsigned				i;

	kprintf("  reboot:%l power:%l\n", call->reboot, call->power);
	kprintf("  timer_load:%l reload:%l value:%l\n",
			call->timer_load, call->timer_reload, call->timer_value);
	for(i = 0; i < NUM_ELTS(call->debug); ++i) {
		struct debug_callout	*dbg = &call->debug[i];

		kprintf("  %d) display:%l poll:%l break:%l\n", i,
			dbg->display_char, dbg->poll_key, dbg->break_detect);
	}
}

static void
print_intrgen(char *name, struct __intrgen_data *gen) {
	kprintf("     %s => flags:%w, size:%w, rtn:%l\n",
		name, gen->genflags, gen->size, gen->rtn);
}

void
print_intrinfo(void) {
 	struct intrinfo_entry *intr = lsp.intrinfo.p;
	int						num = lsp.intrinfo.size / sizeof(*intr);
	int						i;

	for( i = 0; i < num; ++i ) {
		kprintf("  %d) vector_base:%l, #vectors:%d, cascade_vector:%l\n",
				i, intr[i].vector_base, intr[i].num_vectors, intr[i].cascade_vector);
		kprintf("     cpu_intr_base:%l, cpu_intr_stride:%d, flags:%w\n",
				intr[i].cpu_intr_base, intr[i].cpu_intr_stride, intr[i].flags);
		print_intrgen(" id", &intr[i].id);
		print_intrgen("eoi", &intr[i].eoi);
		kprintf("     mask:%l, unmask:%l, config:%l\n",
			intr[i].mask, intr[i].unmask, intr[i].config);
	}
}

void
print_smp(void) {
	struct smp_entry *smp = lsp.smp.p;

	kprintf("  send_ipi:%l cpu:%l\n", smp->send_ipi, smp->cpu);
}

void
print_pminfo(void) {
	struct pminfo_entry *pm = lsp.pminfo.p;

	kprintf("  wakeup_condition:%l\n", pm->wakeup_condition);
}

void
print_mdriver(void) {
	struct mdriver_entry *md = lsp.mdriver.p;
	int						num = lsp.mdriver.size / sizeof(*md);
	int						i;

	for(i = 0; i < num; ++i, ++md) {
		kprintf("  %d) name=%d, intr=%x, rtn=%l, paddr=%l, size=%d\n", i, 
				md->name, md->intr, md->handler, md->data_paddr, md->data_size);
	}
}

#define INFO_SECTION		0x0001
#define EXPLICIT_ENABLE		0x8000
#define EXPLICIT_DISABLE	0x4000

struct debug_syspage_section {
	const char 		*name;
	unsigned short	loc;
	unsigned short	flags;
	void			(*print)(void);
};

static struct debug_syspage_section sp_section[] = {
	PRT_SYSPAGE_RTN(1, system_private),
	PRT_SYSPAGE_RTN(1, qtime),
	PRT_SYSPAGE_RTN(1, callout),
//	PRT_SYSPAGE_RTN(1, callin),
	PRT_SYSPAGE_RTN(1, cpuinfo),
	PRT_SYSPAGE_RTN(1, cacheattr),
	PRT_SYSPAGE_RTN(1, meminfo),
	PRT_SYSPAGE_RTN(1, asinfo),
	PRT_SYSPAGE_RTN(1, hwinfo),
	PRT_SYSPAGE_RTN(1, typed_strings),
	PRT_SYSPAGE_RTN(1, strings),
	PRT_SYSPAGE_RTN(1, intrinfo),
	PRT_SYSPAGE_RTN(1, smp),
	PRT_SYSPAGE_RTN(1, pminfo),
	PRT_SYSPAGE_RTN(1, mdriver),
	CPU_PRT_SYSPAGE_RTNS
};

static int
enable_print_syspage(const char *name) {
	unsigned	i;
	unsigned	on_bit;
	unsigned	off_mask;
	
	if( *name == '~') {
		++name;
		on_bit = EXPLICIT_DISABLE;
		off_mask = ~EXPLICIT_ENABLE;
	} else {
		on_bit = EXPLICIT_ENABLE;
		off_mask = ~EXPLICIT_DISABLE;
	}
	for(i = 0; i < NUM_ELTS(sp_section); ++i) {
		if(strcmp(sp_section[i].name, name) == 0) {
			sp_section[i].flags &= off_mask;
			sp_section[i].flags |= on_bit;
		}
	}
	return(on_bit & EXPLICIT_ENABLE);
}

void
print_syspage(void) {
	unsigned	i;
	unsigned	flags;
	int			opt;
	int			have_enables = 0;

	if(debug_flag > 1) {
		kprintf("Header size=0x%x, Total Size=0x%x, #Cpu=%d, Type=%d\n",
			lsp.syspage.p->size, lsp.syspage.p->total_size,
			lsp.syspage.p->num_cpu, lsp.syspage.p->type);

		//We enable things here so that this code doesn't get
		//hauled in unless the user calls print_syspage().
		optind = 0;
		while((opt = getopt(_argc, _argv, "S:")) != -1) {
			if(opt == 'S') {
				have_enables |= enable_print_syspage(optarg);
			}
		}
		for(i = 0; i < NUM_ELTS(sp_section); ++i) {
			flags = sp_section[i].flags;
			if(!(flags & EXPLICIT_DISABLE) && !have_enables) {
				flags |= EXPLICIT_ENABLE;
			}
			if(flags & EXPLICIT_ENABLE) {
				kprintf("Section:%s ", sp_section[i].name);
				if(sp_section[i].flags & INFO_SECTION) {
					syspage_entry_info	*info;

					info = (void *)((uint8_t *)lsp.syspage.p + sp_section[i].loc);
					kprintf("offset:0x%x size:0x%x\n", info->entry_off, info->entry_size);
					if(info->entry_size > 0 && debug_flag > 2) {
						sp_section[i].print();
					}
				} else {
					kprintf("offset:0x%x\n", sp_section[i].loc);
					if(debug_flag > 2) {
						sp_section[i].print();
					}
				}
			}
		}
	}
}

__SRCVERSION("debug.c $Rev: 262077 $");
