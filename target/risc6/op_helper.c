/*
 * Oberon RISC6 helper routines.
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
#include "cpu.h"
#include "exec/helper-proto.h"
#include "exec/cpu_ldst.h"
#include "exec/exec-all.h"
#include "qemu/main-loop.h"


void helper_mmu_read_debug(CPURISC6State *env, uint32_t rn)
{
    mmu_read_debug(env, rn);
}

void helper_mmu_write(CPURISC6State *env, uint32_t rn, uint32_t v)
{
    mmu_write(env, rn, v);
}

void helper_check_interrupts(CPURISC6State *env)
{
    qemu_mutex_lock_iothread();
    risc6_check_interrupts(env);
    qemu_mutex_unlock_iothread();
}


void helper_raise_exception(CPURISC6State *env, uint32_t index)
{
    CPUState *cs = env_cpu(env);
    cs->exception_index = index;
    cpu_loop_exit(cs);
}
