/*
 * RISC6 kernel loader
 *
 * Copyright (c) 2019 Charles Perkins <charlesap@gmail.com>
 *
 * Adapted from the nios2 kernel loader
 *
 * Copyright (c) 2016 Marek Vasut <marek.vasut@gmail.com>
 *
 * Based on microblaze kernel loader
 *
 * Copyright (c) 2012 Peter Crosthwaite <peter.crosthwaite@petalogix.com>
 * Copyright (c) 2012 PetaLogix
 * Copyright (c) 2009 Edgar E. Iglesias.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu-common.h"
#include "cpu.h"
#include "qemu/option.h"
#include "qemu/config-file.h"
#include "qemu/error-report.h"
#include "sysemu/device_tree.h"
#include "sysemu/reset.h"
#include "sysemu/sysemu.h"
#include "hw/loader.h"
#include "elf.h"

#include "boot.h"

#define RISC6_MAGIC    0xf0e1d2c3;

static struct risc6_boot_info {
    void (*machine_cpu_reset)(RISC6CPU *);
    uint32_t bootstrap_pc;
    uint32_t cmdline;
    uint32_t initrd_start;
    uint32_t initrd_end;
    uint32_t fdt;
} boot_info;

static void main_cpu_reset(void *opaque)
{
    RISC6CPU *cpu = opaque;
//    CPUState *cs = CPU(cpu);
    CPURISC6State *env = &cpu->env;

    cpu_reset(CPU(cpu));

    printf("Reset, pc=0x%08x\n",env->regs[R_PC]);

    env->regs[0] =  RISC6_MAGIC; 
    env->regs[R_PC] =  0xffffe000;

    env->regs[1] = 0x12345678;      //boot_info.initrd_start;
    env->regs[2] = 0x23456789;      //boot_info.fdt;
    env->regs[3] = 0x3456789a;      //boot_info.cmdline;

//    cpu_set_pc(cs, boot_info.bootstrap_pc);
//    if (boot_info.machine_cpu_reset) {
//        boot_info.machine_cpu_reset(cpu);
//    }
}


void risc6_board_reset(RISC6CPU *cpu, hwaddr ddr_base, hwaddr rom_base,
                            uint32_t ramsize,
                            const char *initrd_filename,
                            const char *dtb_filename,
                            void (*machine_cpu_reset)(RISC6CPU *))
{
    
    boot_info.machine_cpu_reset = machine_cpu_reset;
    qemu_register_reset(main_cpu_reset, cpu);

}

