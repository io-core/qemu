/*
 * Oberon RISC6 virtual CPU header
 *
 * Copyright (c) 2019 Charles Perkins <cperkins@kuracali.com>
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

#ifndef RISC6_CPU_H
#define RISC6_CPU_H

#include "exec/cpu-defs.h"
#include "hw/core/cpu.h"

typedef struct CPURISC6State CPURISC6State;



#include "mmu.h"

//#define TARGET_LONG_BITS 32
#define TYPE_RISC6_CPU "risc6-cpu"

#define RISC6_CPU_CLASS(klass) \
    OBJECT_CLASS_CHECK(RISC6CPUClass, (klass), TYPE_RISC6_CPU)
#define RISC6_CPU(obj) \
    OBJECT_CHECK(RISC6CPU, (obj), TYPE_RISC6_CPU)
#define RISC6_CPU_GET_CLASS(obj) \
    OBJECT_GET_CLASS(RISC6CPUClass, (obj), TYPE_RISC6_CPU)

/**
 * RISC6CPUClass:
 * @parent_reset: The parent class' reset handler.
 *
 * A RISC6 CPU model.
 */
typedef struct RISC6CPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/

    DeviceRealize parent_realize;
    void (*parent_reset)(CPUState *cpu);
} RISC6CPUClass;

#define TARGET_HAS_ICE 1

/* Configuration options for Oberon RISC6 */
#define RESET_ADDRESS         0xffffe000
#define EXCEPTION_ADDRESS     0xffffe000
#define FAST_TLB_MISS_ADDRESS 0xffffe000




/* r0-r9, ra, rb, mt, sb, sp ,lr , rc, rv, rn, rz, rh, xc, pc */
#define NUM_CORE_REGS 23 

/* General purpose register aliases */

#define R_0      0
#define R_1      1
#define R_2      2
#define R_3      3
#define R_4      4
#define R_5      5
#define R_6      6
#define R_7      7
#define R_8      8 
#define R_9      9 
#define R_A      10
#define R_B      11
#define R_MT     12
#define R_SB     13
#define R_SP     14
#define R_LR     15
#define R_C          16
#define R_V          17
#define R_N          18
#define R_Z          19
#define R_H          20
#define R_XC         21
#define R_PC         22



/* Control register aliases */

#define CR_BASE  16
#define CR_STATUS    (CR_BASE + 0)
#define   CR_STATUS_PIE  (1 << 0)
#define   CR_STATUS_U    (1 << 1)
#define   CR_STATUS_EH   (1 << 2)
#define   CR_STATUS_IH   (1 << 3)
#define   CR_STATUS_IL   (63 << 4)
#define   CR_STATUS_CRS  (63 << 10)
#define   CR_STATUS_PRS  (63 << 16)
#define   CR_STATUS_NMI  (1 << 22)
#define   CR_STATUS_RSIE (1 << 23)

#define CR_IENABLE   (CR_BASE + 0)
#define CR_IPENDING  (CR_BASE + 0)




/* Exceptions */

#define EXCP_BREAK    0x1000
#define EXCP_RESET    0
#define EXCP_PRESET   1
#define EXCP_IRQ      2
#define EXCP_TRAP     3
#define EXCP_UNIMPL   4
#define EXCP_ILLEGAL  5
#define EXCP_UNALIGN  6
#define EXCP_UNALIGND 7
#define EXCP_DIV      8
#define EXCP_SUPERA   9
#define EXCP_SUPERI   10
#define EXCP_SUPERD   11
#define EXCP_TLBD     12
#define EXCP_TLBX     13
#define EXCP_TLBR     14
#define EXCP_TLBW     15
#define EXCP_MPUI     16
#define EXCP_MPUD     17

#define CPU_INTERRUPT_NMI       CPU_INTERRUPT_TGT_EXT_3

#define NB_MMU_MODES 2

struct CPURISC6State {
    uint32_t regs[NUM_CORE_REGS];
    uint32_t pc;                /* program counter */
    uint32_t sr_t;              /* T bit of status register */
    uint32_t flags;             /* general execution flags */

    RISC6MMU mmu;

    uint32_t irq_pending;
};

/**
 * RISC6CPU:
 * @env: #CPURISC6State
 *
 * A RISC6 CPU.
 */
typedef struct RISC6CPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUNegativeOffsetState neg;
    CPURISC6State env;

    bool mmu_present;
    uint32_t pid_num_bits;
    uint32_t tlb_num_ways;
    uint32_t tlb_num_entries;

    /* Addresses that are hard-coded in the FPGA build settings */
    uint32_t reset_addr;
    uint32_t exception_addr;
    uint32_t fast_tlb_miss_addr;
} RISC6CPU;

//static inline RISC6CPU *risc6_env_get_cpu(CPURISC6State *env)
//{
//    return RISC6_CPU(container_of(env, RISC6CPU, env));
//}
//#define ENV_GET_CPU(e) CPU(risc6_env_get_cpu(e))
//#define ENV_OFFSET offsetof(RISC6CPU, env)

void risc6_tcg_init(void);
void risc6_cpu_do_interrupt(CPUState *cs);
int cpu_risc6_signal_handler(int host_signum, void *pinfo, void *puc);
void dump_mmu(CPURISC6State *env);
void risc6_cpu_dump_state(CPUState *cpu, FILE *f, int flags);
hwaddr risc6_cpu_get_phys_page_debug(CPUState *cpu, vaddr addr);
void risc6_cpu_do_unaligned_access(CPUState *cpu, vaddr addr,
                                   MMUAccessType access_type,
                                   int mmu_idx, uintptr_t retaddr);

qemu_irq *risc6_cpu_pic_init(RISC6CPU *cpu);
void risc6_check_interrupts(CPURISC6State *env);

void do_risc6_semihosting(CPURISC6State *env);

#define TARGET_PHYS_ADDR_SPACE_BITS 32
#define TARGET_VIRT_ADDR_SPACE_BITS 32

#define CPU_RESOLVING_TYPE TYPE_RISC6_CPU

#define cpu_gen_code cpu_risc6_gen_code
#define cpu_signal_handler cpu_risc6_signal_handler

#define CPU_SAVE_VERSION 1

#define TARGET_PAGE_BITS 12

/* MMU modes definitions */
#define MMU_MODE0_SUFFIX _kernel
#define MMU_MODE1_SUFFIX _user
#define MMU_SUPERVISOR_IDX  0
#define MMU_USER_IDX        1

static inline int cpu_mmu_index(CPURISC6State *env, bool ifetch)
{
    return (env->regs[CR_STATUS] & CR_STATUS_U) ? MMU_USER_IDX :
                                                  MMU_SUPERVISOR_IDX;
}

bool risc6_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                        MMUAccessType access_type, int mmu_idx,
                        bool probe, uintptr_t retaddr);

static inline int cpu_interrupts_enabled(CPURISC6State *env)
{
    return env->regs[CR_STATUS] & CR_STATUS_PIE;
}

typedef CPURISC6State CPUArchState;
typedef RISC6CPU ArchCPU;

#include "exec/cpu-all.h"

static inline void cpu_get_tb_cpu_state(CPURISC6State *env, target_ulong *pc,
                                        target_ulong *cs_base, uint32_t *flags)
{
    *pc = env->regs[R_PC];
    *cs_base = 0;
    *flags = (env->regs[CR_STATUS] & (CR_STATUS_EH | CR_STATUS_U));
}

#endif /* RISC6_CPU_H */
