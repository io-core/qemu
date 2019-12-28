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

#include "qemu/osdep.h"
#include "qemu/qemu-print.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "mmu.h"



/* Define this to enable MMU debug messages */
/* #define DEBUG_MMU */

#ifdef DEBUG_MMU
#define MMU_LOG(x) x
#else
#define MMU_LOG(x)
#endif

void mmu_read_debug(CPURISC6State *env, uint32_t rn)
{
}

/* rw - 0 = read, 1 = write, 2 = fetch.  */
unsigned int mmu_translate(CPURISC6State *env,
                           RISC6MMULookup *lu,
                           target_ulong vaddr, int rw, int mmu_idx)
{
    return 0;
}


void mmu_write(CPURISC6State *env, uint32_t rn, uint32_t v)
{
}

void mmu_init(CPURISC6State *env)
{
}

void dump_mmu(CPURISC6State *env)
{
}


