/*
 * Oberon RISC6 MMU emulation for qemu.
 *
 * Copyright (C) 2019 Charles Perkins <cperkins@kuracali.com>
 * Copyright (C) 2012 Chris Wulff <crwulff@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

#ifndef RISC6_MMU_H
#define RISC6_MMU_H

typedef struct RISC6TLBEntry {
    target_ulong tag;
    target_ulong data;
} RISC6TLBEntry;

typedef struct RISC6MMU {
    int tlb_entry_mask;
    uint32_t pteaddr_wr;
    uint32_t tlbacc_wr;
    uint32_t tlbmisc_wr;
    RISC6TLBEntry *tlb;
} RISC6MMU;

typedef struct RISC6MMULookup {
    target_ulong vaddr;
    target_ulong paddr;
    int prot;
} RISC6MMULookup;

void mmu_flip_um(CPURISC6State *env, unsigned int um);
unsigned int mmu_translate(CPURISC6State *env,
                           RISC6MMULookup *lu,
                           target_ulong vaddr, int rw, int mmu_idx);
void mmu_read_debug(CPURISC6State *env, uint32_t rn);
void mmu_write(CPURISC6State *env, uint32_t rn, uint32_t v);
void mmu_init(CPURISC6State *env);

#endif /* RISC6_MMU_H */
