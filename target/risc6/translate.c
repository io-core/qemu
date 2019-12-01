/*
 * Oberon RISC6 emulation for qemu: main translation routines.
 *
 * Copyright (C) 2019 Charles Perkins <chuck@kuracali.com>
 * Copyright (C) 2016 Marek Vasut <marex@denx.de>
 * Copyright (C) 2012 Chris Wulff <crwulff@gmail.com>
 * Copyright (C) 2010 Tobias Klauser <tklauser@distanz.ch>
 *  (Portions of this file that were originally from nios2sim-ng.)
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
#include "tcg-op.h"
#include "exec/exec-all.h"
#include "disas/disas.h"
#include "disas/risc6.h"
#include "exec/helper-proto.h"
#include "exec/helper-gen.h"
#include "exec/log.h"
#include "exec/cpu_ldst.h"
#include "exec/translator.h"
#include "qemu/qemu-print.h"

/* is_jmp field values
    DISAS_NEXT = 0
    DISAS_TOO_MANY = 1
    DISAS_NORETURN = 2
    DISAS_TARGET_0 = 3
    DISAS_TARGET_1 = 4, etc.  */

#define DISAS_JUMP    DISAS_TARGET_0 /* = 3  only pc was modified dynamically */
#define DISAS_UPDATE  DISAS_TARGET_1 /* = 4  cpu state was modified dynamically */
#define DISAS_TB_JUMP DISAS_TARGET_2 /* = 5  only pc was modified statically */
#define DISAS_STOP    DISAS_TARGET_4 /* = 6  Target-specific value for ctx->base.is_jmp.  */
                                     /*      We want to exit back to the cpu loop for some reason. */
                                     /*      Usually this is to recognize interrupts immediately.  */


#define INSTRUCTION_FLG(func, flags) { (func), (flags) }
#define INSTRUCTION(func)             INSTRUCTION_FLG(func, 0)
#define INSTRUCTION_NOP()             INSTRUCTION_FLG(nop, 0)
#define INSTRUCTION_UNIMPLEMENTED()   INSTRUCTION_FLG(gen_excp, EXCP_UNIMPL)
#define INSTRUCTION_ILLEGAL()         INSTRUCTION_FLG(gen_excp, EXCP_ILLEGAL)



typedef struct DisasContext {
    DisasContextBase  base;
    TCGv             *cpu_R;
    TCGv_i32          zero;
    int               mem_idx;
} DisasContext;

typedef struct RISC6Instruction {
    void     (*handler)(DisasContext *dc, uint32_t code, uint32_t flags);
    uint32_t  flags;
} RISC6Instruction;

//TCGv_i32 cpu_CF, cpu_NF, cpu_VF, cpu_ZF;
static TCGv cpu_R[NUM_CORE_REGS];
//static TCGv cpu_PSW_C;
//static TCGv cpu_PSW_V;



static bool use_goto_tb(DisasContext *dc, uint32_t dest)
{
    if (unlikely(dc->base.singlestep_enabled)) {
        return false;
    }

    return (dc->base.tb->pc & TARGET_PAGE_MASK) == (dest & TARGET_PAGE_MASK);
}

static void gen_goto_tb(DisasContext *dc, int n, uint32_t dest)
{
    TranslationBlock *tb = dc->base.tb;

    if (use_goto_tb(dc, dest)) {
        tcg_gen_goto_tb(n);
        tcg_gen_movi_tl(dc->cpu_R[R_PC], dest);
        tcg_gen_exit_tb(tb, n);
    } else {
        tcg_gen_movi_tl(dc->cpu_R[R_PC], dest);
        tcg_gen_exit_tb(NULL, 0);
    }
}



#include "exec/gen-icount.h"


void risc6_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    RISC6CPU *cpu = RISC6_CPU(cs);
    CPURISC6State *env = &cpu->env;
    int i;

    if (!env) {
        return;
    }
    for (i = 0; i < 16; i++) {
        qemu_fprintf(f, "%9s=%8.8x ", regnames[i], env->regs[i]);
        if ((i + 1) % 4 == 0) {
            qemu_fprintf(f, "\n");
        }
    }
    for (i = 20; i < NUM_CORE_REGS; i++) {
        qemu_fprintf(f, "%9s=%8.8x ", regnames[i], env->regs[i]);
    }
    qemu_fprintf(f, "%4s=%d %s=%d %s=%d %s=%d ", "C", env->regs[16], "V", env->regs[17], "N", env->regs[18], "Z", env->regs[19]);

    qemu_fprintf(f, "\n\n");
}


void risc6_tcg_init(void)
{
    int i;

    for (i = 0; i < NUM_CORE_REGS; i++) {
        cpu_R[i] = tcg_global_mem_new(cpu_env,
                                      offsetof(CPURISC6State, regs[i]),
                                      regnames[i]);
    }
}


static void risc6_translate_regop(DisasContextBase *dcbase, CPUState *cs, uint32_t insn){
    TCGv cval;
    TCGv t0, t_31;
    uint8_t opx;
    opx = insn >> 30;

    I_TYPE(instr, insn);

      cval = tcg_temp_new_i32();
      switch (opx) {
      case 0:
        tcg_gen_mov_tl(cval, cpu_R[instr.c]);
        break;
      case 1:
        tcg_gen_movi_tl(cval, instr.imm16.u);
      }

      switch (instr.op){
      case MOV:  
        
        if (instr.opx == 0){                   // register 
         if (instr.opu == 0){
          if (instr.opv == 0) {
             tcg_gen_mov_tl(cpu_R[instr.a], cpu_R[R_H]);
          }else{
//             tcg_gen_mov_tl(cpu_R[instr.a], cpu_R[R_FLG]);
          }
         }else{
             tcg_gen_mov_tl(cpu_R[instr.a], cpu_R[instr.c]);
         }
        }else{                                // immediate 
          if (instr.opu == 0){
           if (instr.opv == 0) {
             tcg_gen_andi_tl(cpu_R[instr.a], cpu_R[instr.a], 0x00000000);
           }else{
             tcg_gen_andi_tl(cpu_R[instr.a], cpu_R[instr.a], 0x00000000);
             tcg_gen_ori_tl(cpu_R[instr.a], cpu_R[instr.a], 0xFFFF0000);
           }
           tcg_gen_addi_tl(cpu_R[instr.a], cpu_R[instr.a], instr.imm16.u);
          }else{
            tcg_gen_andi_tl(cpu_R[instr.a], cpu_R[instr.a], 0x0000FFFF);
            tcg_gen_addi_tl(cpu_R[instr.a], cpu_R[instr.a], instr.imm16.u << 16);
          }
        }
        break;
      case LSL:  
        t0 = tcg_temp_new();
        t_31 = tcg_const_tl(31);
        tcg_gen_shl_tl(cpu_R[instr.a], cpu_R[instr.b], cval); 
        tcg_gen_sub_tl(t0, t_31, cval);
        tcg_gen_sar_tl(t0, t0, t_31);
        tcg_gen_and_tl(t0, t0, cpu_R[instr.a]);
        tcg_gen_xor_tl(cpu_R[instr.a], cpu_R[instr.a], t0);
        tcg_temp_free(t0);
        tcg_temp_free(t_31);
        break;
      case ASR:  
        t0 = tcg_temp_new();
        t_31 = tcg_temp_new();
        tcg_gen_sar_tl(cpu_R[instr.a], cpu_R[instr.b], cval);
        tcg_gen_movi_tl(t_31, 31);
        tcg_gen_sub_tl(t0, t_31, cval);
        tcg_gen_sar_tl(t0, t0, t_31);
        tcg_gen_or_tl(cpu_R[instr.a], cpu_R[instr.a], t0);
        tcg_temp_free(t0);
        tcg_temp_free(t_31);
        break;
      case ROR:  
        tcg_gen_rotr_tl(cpu_R[instr.a], cpu_R[instr.b], cval);
        break;
      case AND:  
        tcg_gen_and_tl(cpu_R[instr.a], cpu_R[instr.b], cval);
        break;
      case ANN:  
        tcg_gen_and_tl(cpu_R[instr.a], cpu_R[instr.b], cval);
        tcg_gen_not_tl(cpu_R[instr.a], cpu_R[instr.a]);
        break;
      case IOR:  
        tcg_gen_or_tl(cpu_R[instr.a], cpu_R[instr.b], cval);
        break;
      case XOR:  
        tcg_gen_xor_tl(cpu_R[instr.a], cpu_R[instr.b], cval);
        break;
      case ADD:  
        tcg_gen_add_tl(cpu_R[instr.a], cpu_R[instr.b], cval);
        tcg_gen_setcond_tl(TCG_COND_GT, cpu_R[R_C], cpu_R[instr.a], cpu_R[instr.b]);
        break;
      case SUB:
        tcg_gen_sub_tl(cpu_R[instr.a], cpu_R[instr.b],cval);
        tcg_gen_setcond_tl(TCG_COND_LT, cpu_R[R_C], cpu_R[instr.a], cpu_R[instr.b]);
        break;
      case MUL:  

        break;
      case DIV:  

        break;
      case FAD:  

        break;
      case FSB:  

        break;
      case FML:  

        break;
      case FDV:  

        break;
      }


     

      tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_R[R_Z], cpu_R[instr.a], 0x00);
      tcg_gen_setcondi_tl(TCG_COND_LT, cpu_R[R_N], cpu_R[instr.a], 0x00);

      tcg_temp_free(cval);


}

static void risc6_translate_memop(DisasContextBase *dcbase, CPUState *cs, uint32_t insn, int mem_idx){

    I_TYPE(instr, insn);

    TCGv addr = tcg_temp_new();
    TCGv val = cpu_R[instr.a];
    tcg_gen_addi_tl(addr, cpu_R[instr.b], instr.imm16.s);

    switch((insn >> 28) & 3){
    case LDR:
      tcg_gen_qemu_ld32u(val, addr, mem_idx);
      break;
    case LDB:
      tcg_gen_qemu_ld8u(val, addr, mem_idx);
      break;
    case STR:
      tcg_gen_qemu_st32(val, addr, mem_idx);
      break;
    case STB:
      tcg_gen_qemu_st8(val, addr, mem_idx);
      break;
    }

    tcg_temp_free(addr);
}

static void risc6_translate_braop(DisasContextBase *dcbase, CPUState *cs, uint32_t insn){

    TCGLabel *l1;

    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    I_TYPE(instr, insn);


      if (instr.opv == 1){                   /* return address in r15 */
        tcg_gen_movi_tl(cpu_R[R_LR], dcbase->tb->pc + 4);
      }

      l1 = gen_new_label();
//        TCG_COND_NEVER,
//        TCG_COND_EQ,      eq:  Z 
//        TCG_COND_LE,      le:  Z | (N ^ V) -> Z | N 
//        TCG_COND_LT,      lt:  N ^ V -> N 
//        TCG_COND_EQ,      leu: C | Z -> Z 
//        TCG_COND_NEVER,   ltu: C -> 0 
//        TCG_COND_LT,      neg: N 
//        TCG_COND_NEVER,   vs:  V -> 0 
//        TCG_COND_ALWAYS,
//        TCG_COND_NE,      ne:  !Z 
//        TCG_COND_GT,      gt:  !(Z | (N ^ V)) -> !(Z | N) 
//        TCG_COND_GE,      ge:  !(N ^ V) -> !N 
//        TCG_COND_NE,      gtu: !(C | Z) -> !Z 
//        TCG_COND_ALWAYS,  geu: !C -> 1 
//        TCG_COND_GE,      pos: !N 
//        TCG_COND_ALWAYS,  vc:  !V -> 1 

// Risc6 comparisons:
// *0000 MI negative (minus)  N       *1000 PL positive (plus)  ~N
// *0001 EQ equal (zero)      Z       *1001 NE positive (plus)  ~Z
// *0010 CS carry set (lower) C       *1010 CC carry clear      ~C
// *0011 VS overflow set      V       *1011 VC overflow clear   ~V
// *0100 LS lower or same     ~C|Z     1100 HI higher           ~(~C}Z)      
// *0101 LT less than        N!=V     *1101 GE greater or equal ~(N!=V)
// *0110 LE less or equal    (N!=V)|Z  1110 GT greater than     ~((N!=V)|Z)
// *0111    always            true    *1111    never            false

      switch (instr.a){
      case BMI: // ok
        tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_R[R_N], 1, l1);
        break;
      case BEQ: // ok
        tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_R[R_Z], 1, l1);
        break;
      case BCS: // ok
        tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_R[R_C], 1, l1);
        break;
      case BVS: // ok
        tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_R[R_V], 1, l1);
        break;
      case BLS: // ok
        tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_R[R_C], 0, l1);
        tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_R[R_Z], 1, l1);
        break;
      case BLT: // ok 
        tcg_gen_brcond_tl(TCG_COND_NE, cpu_R[R_N], cpu_R[R_V], l1);
        break;
      case BLE: // ok 
        tcg_gen_brcond_tl(TCG_COND_NE, cpu_R[R_N], cpu_R[R_V], l1);
        tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_R[R_Z], 1, l1);
        break;
      case BR:  // ok
        tcg_gen_brcond_tl(TCG_COND_ALWAYS, cpu_R[instr.a], cpu_R[instr.b], l1);
        break;
      case BPL: // ok
        tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_R[R_N], 0, l1);
        break;
      case BNE: // ok
        tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_R[R_Z], 0, l1);
        break;
      case BCC: // ok
        tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_R[R_C], 0, l1);
        break;
      case BVC: // ok
        tcg_gen_brcondi_tl(TCG_COND_EQ, cpu_R[R_V], 0, l1);
        break;
      case BHI: 
        
        break;
      case BGE: // ok
        tcg_gen_brcond_tl(TCG_COND_EQ, cpu_R[R_N], cpu_R[R_V], l1);
        break;
      case BGT: 
        
        break;
      case NOP: // ok
        tcg_gen_brcond_tl(TCG_COND_NEVER, cpu_R[instr.a], cpu_R[instr.b], l1);
      }

      gen_goto_tb(ctx, 0, dcbase->tb->pc + 4);
      gen_set_label(l1);
      if (instr.opu == 0) {                     /* dest in register c */
        tcg_gen_mov_tl(cpu_R[R_PC], cpu_R[instr.c]); 
        dcbase->is_jmp = DISAS_JUMP;
      }else{                                      /* dest pc relative */
        gen_goto_tb(ctx, 1, dcbase->tb->pc + 4 + (instr.imm24.s <<2 ));
        tcg_gen_movi_tl(cpu_R[R_PC], dcbase->tb->pc + 4 + (instr.imm24.s <<2 ));
        dcbase->is_jmp = DISAS_TB_JUMP;
      }

      dcbase->pc_next = dcbase->tb->pc + 4;


}


static void risc6_tr_translate_insn (DisasContextBase *dcbase, CPUState *cs){
    uint32_t insn=translator_ldl(cs->env_ptr,dcbase->tb->pc);

    dcbase->pc_next += 4;
    dcbase->is_jmp = DISAS_NEXT;

    switch (insn >> 30){
    case 0:
    case 1: 
      risc6_translate_regop (dcbase, cs, insn);
      break;
    case 2:
      risc6_translate_memop (dcbase, cs, insn, container_of(dcbase, DisasContext, base)->mem_idx);
      break;
    default:
      risc6_translate_braop (dcbase, cs, insn);
    }

//    char dstrbuff[128];
//    ins2str( dcbase->tb->pc, insn, dstrbuff);
//    printf("first: 0x%08x addr: 0x%08x instr: %s\n",dcbase->pc_first,dcbase->tb->pc,dstrbuff);

    dcbase->tb->pc = dcbase->tb->pc + 4;
    dcbase->num_insns=dcbase->num_insns+1;
    if (dcbase->num_insns >= 40 ){ //dcbase->max_insns ){
        dcbase->is_jmp = DISAS_TOO_MANY;
    }
}

static void risc6_tr_init_disas_context(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    
    ctx->cpu_R   = cpu_R;
    ctx->mem_idx = cpu_mmu_index(cs->env_ptr, false);
    dcbase->singlestep_enabled = cs->singlestep_enabled;

    dcbase->num_insns = 0;
    int page_insns = (TARGET_PAGE_SIZE - (dcbase->tb->pc & TARGET_PAGE_MASK)) / 4;
    if (dcbase->max_insns > page_insns) {
        dcbase->max_insns = page_insns;
    }
}

static void risc6_tr_tb_start(DisasContextBase *dcbase, CPUState *cs)
{
}

static void risc6_tr_insn_start(DisasContextBase *dcbase, CPUState *cs)
{
    tcg_gen_insn_start(container_of(dcbase, DisasContext, base)->base.pc_next);
}

static bool risc6_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cs, const CPUBreakpoint *bp)
{
    return true;
}

static void risc6_tr_tb_stop(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    switch (ctx->base.is_jmp) {
    case DISAS_TOO_MANY:
//        printf("DISAS_TOO_MANY\n");
        gen_goto_tb(ctx, 0, ctx->base.pc_next);
        break;
    case DISAS_JUMP:
//        printf("DISAS_JUMP\n");
        tcg_gen_lookup_and_goto_ptr();
        tcg_gen_exit_tb(NULL, 0);
        break;
    case DISAS_TB_JUMP:
//        printf("DISAS_TB_JUMP\n");
        break;
    default:
        printf("stop code: %d\n",ctx->base.is_jmp);
        g_assert_not_reached();
    }

}

static void risc6_tr_disas_log(const DisasContextBase *dcbase, CPUState *cs)
{
    qemu_log("IN:\n");  /* , lookup_symbol(dcbase->pc_first)); */
    log_target_disas(cs, dcbase->pc_first, dcbase->tb->size);
}

static const TranslatorOps risc6_tr_ops = {
    .init_disas_context = risc6_tr_init_disas_context,
    .tb_start           = risc6_tr_tb_start,
    .insn_start         = risc6_tr_insn_start,
    .breakpoint_check   = risc6_tr_breakpoint_check,
    .translate_insn     = risc6_tr_translate_insn,
    .tb_stop            = risc6_tr_tb_stop,
    .disas_log          = risc6_tr_disas_log,
};

void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int max_insns)
{
    DisasContext ctx;
    translator_loop(&risc6_tr_ops, &ctx.base, cs, tb, max_insns);
}

void restore_state_to_opc(CPURISC6State *env, TranslationBlock *tb, target_ulong *data)
{
    env->regs[R_PC] = data[0];
}

