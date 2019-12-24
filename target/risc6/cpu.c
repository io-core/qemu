/*
 * QEMU Oberon RISC6 CPU
 *
 * Copyright (c) 2019 Charles Perkins <charlesap@gmail.com>
 * Copyright (c) 2012 Chris Wulff <crwulff@gmail.com>
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
#include "qemu-common.h"
#include "qapi/error.h"
#include "cpu.h"
#include "exec/log.h"
#include "exec/gdbstub.h"
#include "hw/qdev-properties.h"

static void risc6_cpu_set_pc(CPUState *cs, vaddr value)
{
    RISC6CPU *cpu = RISC6_CPU(cs);
    CPURISC6State *env = &cpu->env;

    env->regs[R_PC] = value;
}

static bool risc6_cpu_has_work(CPUState *cs)
{
    return cs->interrupt_request & (CPU_INTERRUPT_HARD | CPU_INTERRUPT_NMI);
}

/* CPUClass::reset() */
static void risc6_cpu_reset(CPUState *cs)
{
    RISC6CPU *cpu = RISC6_CPU(cs);
    RISC6CPUClass *ncc = RISC6_CPU_GET_CLASS(cpu);
    CPURISC6State *env = &cpu->env;

    if (qemu_loglevel_mask(CPU_LOG_RESET)) {
        qemu_log("CPU Reset (CPU %d)\n", cs->cpu_index);
        log_cpu_state(cs, 0);
    }

    ncc->parent_reset(cs);

    memset(env->regs, 0, sizeof(uint32_t) * NUM_CORE_REGS);
    env->regs[R_PC] = 0xFFFFF800;  // cpu->reset_addr;

    env->regs[CR_STATUS] = 0;

}

static void risc6_cpu_initfn(Object *obj)
{
    RISC6CPU *cpu = RISC6_CPU(obj);

    cpu_set_cpustate_pointers(cpu);

    mmu_init(&cpu->env);

}

static ObjectClass *risc6_cpu_class_by_name(const char *cpu_model)
{
    return object_class_by_name(TYPE_RISC6_CPU);
}

static void risc6_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    RISC6CPUClass *ncc = RISC6_CPU_GET_CLASS(dev);
    Error *local_err = NULL;

    cpu_exec_realizefn(cs, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }

    qemu_init_vcpu(cs);
    cpu_reset(cs);

    ncc->parent_realize(dev, errp);
}

static bool risc6_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    RISC6CPU *cpu = RISC6_CPU(cs);
    CPURISC6State *env = &cpu->env;

    if ((interrupt_request & CPU_INTERRUPT_HARD) &&
        (env->regs[CR_STATUS] & CR_STATUS_PIE)) {
        cs->exception_index = EXCP_IRQ;
        risc6_cpu_do_interrupt(cs);
        return true;
    }
    return false;
}


static void risc6_cpu_disas_set_info(CPUState *cpu, disassemble_info *info)
{
    /* NOTE: NiosII R2 is not supported yet. */
    info->mach = bfd_arch_risc6;
#ifdef TARGET_WORDS_BIGENDIAN
    info->print_insn = print_insn_big_risc6;
#else
    info->print_insn = print_insn_little_risc6;
#endif
}

static int risc6_cpu_gdb_read_register(CPUState *cs, uint8_t *mem_buf, int n)
{
//    RISC6CPU *cpu = RISC6_CPU(cs);
//    CPUClass *cc = CPU_GET_CLASS(cs);
//    CPURISC6State *env = &cpu->env;
//
//    if (n > cc->gdb_num_core_regs) {
//        return 0;
//    }
//
//    if (n < 32) {          /* GP regs */
//        return gdb_get_reg32(mem_buf, env->regs[n]);
//    } else if (n == 32) {    /* PC */
//        return gdb_get_reg32(mem_buf, env->regs[R_PC]);
//    } else if (n < 49) {     /* Status regs */
//        return gdb_get_reg32(mem_buf, env->regs[n - 1]);
//    }
//
    /* Invalid regs */
    return 0;
}

static int risc6_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
//    RISC6CPU *cpu = RISC6_CPU(cs);
//    CPUClass *cc = CPU_GET_CLASS(cs);
//    CPURISC6State *env = &cpu->env;
//
//    if (n > cc->gdb_num_core_regs) {
//        return 0;
//    }
//
//    if (n < 32) {            /* GP regs */
//        env->regs[n] = ldl_p(mem_buf);
//    } else if (n == 32) {    /* PC */
//        env->regs[R_PC] = ldl_p(mem_buf);
//    } else if (n < 49) {     /* Status regs */
//        env->regs[n - 1] = ldl_p(mem_buf);
//    }

    return 4;
}

static Property risc6_properties[] = {
    DEFINE_PROP_BOOL("mmu_present", RISC6CPU, mmu_present, true),
    /* ALTR,pid-num-bits */
    DEFINE_PROP_UINT32("mmu_pid_num_bits", RISC6CPU, pid_num_bits, 8),
    /* ALTR,tlb-num-ways */
    DEFINE_PROP_UINT32("mmu_tlb_num_ways", RISC6CPU, tlb_num_ways, 16),
    /* ALTR,tlb-num-entries */
    DEFINE_PROP_UINT32("mmu_pid_num_entries", RISC6CPU, tlb_num_entries, 256),
    DEFINE_PROP_END_OF_LIST(),
};


static void risc6_cpu_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    RISC6CPUClass *ncc = RISC6_CPU_CLASS(oc);

    device_class_set_parent_realize(dc, risc6_cpu_realizefn,
                                    &ncc->parent_realize);
    dc->props = risc6_properties;
    ncc->parent_reset = cc->reset;
    cc->reset = risc6_cpu_reset;

    cc->class_by_name = risc6_cpu_class_by_name;
    cc->has_work = risc6_cpu_has_work;
    cc->do_interrupt = risc6_cpu_do_interrupt;
    cc->cpu_exec_interrupt = risc6_cpu_exec_interrupt;
    cc->dump_state = risc6_cpu_dump_state;
    cc->set_pc = risc6_cpu_set_pc;
    cc->disas_set_info = risc6_cpu_disas_set_info;
    cc->tlb_fill = risc6_cpu_tlb_fill;
    cc->gdb_read_register = risc6_cpu_gdb_read_register;
    cc->gdb_write_register = risc6_cpu_gdb_write_register;
    cc->gdb_num_core_regs = 49;
    cc->tcg_initialize = risc6_tcg_init;
}

static const TypeInfo risc6_cpu_type_info = {
    .name = TYPE_RISC6_CPU,
    .parent = TYPE_CPU,
    .instance_size = sizeof(RISC6CPU),
    .instance_init = risc6_cpu_initfn,
    .class_size = sizeof(RISC6CPUClass),
    .class_init = risc6_cpu_class_init,
};

static void risc6_cpu_register_types(void)
{
    type_register_static(&risc6_cpu_type_info);
}

type_init(risc6_cpu_register_types)
