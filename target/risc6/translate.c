/*
 * Oberon RISC6 emulation for qemu: main translation routines.
 *
 * Copyright (C) 2019 Charles Perkins <chuck@kuracali.com>
 * Copyright (C) 2016 Marek Vasut <marex@denx.de>
 * Copyright (C) 2012 Chris Wulff <crwulff@gmail.com>
 * Copyright (C) 2010 Tobias Klauser <tklauser@distanz.ch>
 *  (Portions of this file that were originally from risc6sim-ng.)
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

TCGv_i32 cpu_CF, cpu_NF, cpu_VF, cpu_ZF;


static TCGv load_gpr(DisasContext *dc, uint8_t reg)
{
        return dc->cpu_R[reg];
}


static bool use_goto_tb(DisasContext *dc, uint32_t dest)
{
    if (unlikely(dc->base.singlestep_enabled)) {
        return false;
    }

    return (dc->base.tb->pc & TARGET_PAGE_MASK) == (dest & TARGET_PAGE_MASK);
}


// keep
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


// keep
static void nop(DisasContext *dc, uint32_t code, uint32_t flags)
{
    /* Nothing to do here */
}

static void mov(DisasContext *dc, uint32_t code, uint32_t flags)
{
    I_TYPE(instr, code);


    TCGv data;
        data = dc->cpu_R[instr.a];
    if (instr.opx == 0){                   /* register */
     if (instr.opu == 0){
      if (instr.opv == 0) {
         tcg_gen_mov_tl(dc->cpu_R[instr.a], dc->cpu_R[17]);
      }else{
         tcg_gen_mov_tl(dc->cpu_R[instr.a], dc->cpu_R[16]);
      }
     }else{
         tcg_gen_mov_tl(dc->cpu_R[instr.a], dc->cpu_R[instr.c]);
     }
    }else{                                /* immediate */
     if (instr.opu == 0){
      if (instr.opv == 0) {
        tcg_gen_andi_tl(data, load_gpr(dc, instr.a), 0x00000000);
      }else{
        tcg_gen_andi_tl(data, load_gpr(dc, instr.a), 0x00000000);
        tcg_gen_ori_tl(data, load_gpr(dc, instr.a), 0xFFFF0000);
      }
      tcg_gen_addi_tl(data, load_gpr(dc, instr.a), instr.imm16.u);
     }else{
      tcg_gen_andi_tl(data, load_gpr(dc, instr.a), 0x0000FFFF);
      tcg_gen_addi_tl(data, load_gpr(dc, instr.a), instr.imm16.u << 16);
     }
     // and also test for zero and negative
    }


}




/*
 * I-Type instructions
 */
/* Load instructions */
static void gen_ldx(DisasContext *dc, uint32_t code, uint32_t flags)
{
    I_TYPE(instr, code);

    TCGv addr = tcg_temp_new();
    TCGv data;
        data = dc->cpu_R[instr.b];
    tcg_gen_addi_tl(addr, load_gpr(dc, instr.a), instr.imm16.s);
    tcg_gen_qemu_ld_tl(data, addr, dc->mem_idx, flags);

    tcg_temp_free(addr);
}

/* Store instructions */
static void gen_stx(DisasContext *dc, uint32_t code, uint32_t flags)
{
    I_TYPE(instr, code);
    TCGv val = load_gpr(dc, instr.b);

    TCGv addr = tcg_temp_new();
    tcg_gen_addi_tl(addr, load_gpr(dc, instr.a), instr.imm16.s);
    tcg_gen_qemu_st_tl(val, addr, dc->mem_idx, flags);
    tcg_temp_free(addr);
}

/* Branch instructions */
static void br(DisasContext *dc, uint32_t code, uint32_t flags)
{
    I_TYPE(instr, code);

    if (instr.opv == 1){                   /* return address in r15 */
      tcg_gen_movi_tl(dc->cpu_R[R_RA], dc->base.tb->pc + 4);
    } 

    if (instr.opu == 0) {                     /* dest in register c */
      tcg_gen_mov_tl(dc->cpu_R[R_PC], dc->cpu_R[instr.c]);
    }else{                                      /* dest pc relative */
      gen_goto_tb(dc, 0, dc->base.tb->pc + 4 + (instr.imm24.s << 2));
    }
    dc->base.is_jmp = DISAS_TB_JUMP;
}


/* Comparison instructions */
#define gen_i_cmpxx(fname, op3)                                              \
static void (fname)(DisasContext *dc, uint32_t code, uint32_t flags)         \
{                                                                            \
    I_TYPE(instr, (code));                                                   \
    tcg_gen_setcondi_tl(flags, (dc)->cpu_R[instr.b], (dc)->cpu_R[instr.a],   \
                        (op3));                                              \
}


/* Math/logic instructions */
#define gen_i_math_logic(fname, insn, resimm, op3)                          \
static void (fname)(DisasContext *dc, uint32_t code, uint32_t flags)        \
{                                                                           \
    I_TYPE(instr, (code));                                                  \
        tcg_gen_##insn##_tl((dc)->cpu_R[instr.b], (dc)->cpu_R[instr.a],     \
                            (op3));                                         \
}

gen_i_math_logic(addi,  addi, 1, instr.imm16.s)
gen_i_math_logic(muli,  muli, 0, instr.imm16.s)

gen_i_math_logic(andi,  andi, 0, instr.imm16.u)
gen_i_math_logic(ori,   ori,  1, instr.imm16.u)
gen_i_math_logic(xori,  xori, 1, instr.imm16.u)



/* Math/logic instructions */
#define gen_r_math_logic(fname, insn, op3)                                 \
static void (fname)(DisasContext *dc, uint32_t code, uint32_t flags)       \
{                                                                          \
    I_TYPE(instr, (code));                                                 \
        tcg_gen_##insn((dc)->cpu_R[instr.c], load_gpr((dc), instr.a),      \
                       (op3));                                             \
}

//gen_r_math_logic(add,  add_tl,   load_gpr(dc, instr.b))
gen_r_math_logic(sub,  sub_tl,   load_gpr(dc, instr.b))
//gen_r_math_logic(mul,  mul_tl,   load_gpr(dc, instr.b))

//gen_r_math_logic(and,  and_tl,   load_gpr(dc, instr.b))
//gen_r_math_logic(or,   or_tl,    load_gpr(dc, instr.b))
//gen_r_math_logic(xor,  xor_tl,   load_gpr(dc, instr.b))
//gen_r_math_logic(nor,  nor_tl,   load_gpr(dc, instr.b))




#define gen_r_mul(fname, insn)                                         \
static void (fname)(DisasContext *dc, uint32_t code, uint32_t flags)   \
{                                                                      \
    I_TYPE(instr, (code));                                             \
        TCGv t0 = tcg_temp_new();                                      \
        tcg_gen_##insn(t0, dc->cpu_R[instr.c],                         \
                       load_gpr(dc, instr.a), load_gpr(dc, instr.b)); \
        tcg_temp_free(t0);                                             \
}



#define gen_r_shift_s(fname, insn)                                         \
static void (fname)(DisasContext *dc, uint32_t code, uint32_t flags)       \
{                                                                          \
    I_TYPE(instr, (code));                                                 \
        TCGv t0 = tcg_temp_new();                                          \
        tcg_gen_andi_tl(t0, load_gpr((dc), instr.b), 31);                  \
        tcg_gen_##insn((dc)->cpu_R[instr.c], load_gpr((dc), instr.a), t0); \
        tcg_temp_free(t0);                                                 \
}


gen_r_shift_s(srl, shr_tl)
gen_r_shift_s(sll, shl_tl)

gen_r_shift_s(ror, rotr_tl)


static void divu(DisasContext *dc, uint32_t code, uint32_t flags)
{
    I_TYPE(instr, (code));


    TCGv t0 = tcg_temp_new();
    TCGv t1 = tcg_temp_new();
    TCGv t2 = tcg_const_tl(0);
    TCGv t3 = tcg_const_tl(1);

    tcg_gen_ext32u_tl(t0, load_gpr(dc, instr.a));
    tcg_gen_ext32u_tl(t1, load_gpr(dc, instr.b));
    tcg_gen_movcond_tl(TCG_COND_EQ, t1, t1, t2, t3, t1);
    tcg_gen_divu_tl(dc->cpu_R[instr.c], t0, t1);
    tcg_gen_ext32s_tl(dc->cpu_R[instr.c], dc->cpu_R[instr.c]);

    tcg_temp_free(t3);
    tcg_temp_free(t2);
    tcg_temp_free(t1);
    tcg_temp_free(t0);
}


static const RISC6Instruction i_type_instructions[] = {

    INSTRUCTION(mov),                                 /* mov */
    INSTRUCTION(sll),                                 /* lsl */
    INSTRUCTION(srl),                                 /* asr */
    INSTRUCTION(ror),                                 /* ror */
    INSTRUCTION(andi),                                /* and */
    INSTRUCTION(nop),                                 /* ann */
    INSTRUCTION(ori),                                 /* ior */
    INSTRUCTION(xori),                                /* xor */
    INSTRUCTION(addi),                                /* add */
    INSTRUCTION(sub),                                 /* sub */
    INSTRUCTION(muli),                                /* mul */
    INSTRUCTION(divu),                                /* div */
    INSTRUCTION(nop),                                 /* fad */
    INSTRUCTION(nop),                                 /* fsb */
    INSTRUCTION(nop),                                 /* fml */
    INSTRUCTION(nop),                                 /* fdv */

    INSTRUCTION_FLG(gen_ldx, MO_SW),                  /* ldr */
    INSTRUCTION_FLG(gen_ldx, MO_SB),                  /* ldb */
    INSTRUCTION_FLG(gen_stx, MO_UW),                  /* str */
    INSTRUCTION_FLG(gen_stx, MO_UB),                  /* stb */

    INSTRUCTION(br),                                 /* bmi */
    INSTRUCTION(br),                                 /* beq */
    INSTRUCTION(br),                                 /* bcs */
    INSTRUCTION(br),                                 /* bvs */
    INSTRUCTION(br),                                 /* bls*/
    INSTRUCTION(br),                                 /* blt */
    INSTRUCTION(br),                                 /* ble */
    INSTRUCTION(br),                                  /* b   */
    INSTRUCTION(br),                                 /* bpl */
    INSTRUCTION(br),                                 /* bne */
    INSTRUCTION(br),                                 /* bcc */
    INSTRUCTION(br),                                 /* bvc */
    INSTRUCTION(br),                                 /* bhi */
    INSTRUCTION(br),                                 /* bge */
    INSTRUCTION(br),                                 /* bgt */
    INSTRUCTION(nop),                                 /* nop */


};

/*
static const char * const regnames[] = {
    "r0",         "r1",         "r2",         "r3",
    "r4",         "r5",         "r6",         "r7",
    "r8",         "r9",         "r10",        "r11",
    "r12",        "r13",        "r14",        "r15",
    "flg",        "H",          "spc",        "pc"
};
*/

static TCGv cpu_R[NUM_CORE_REGS];

#include "exec/gen-icount.h"

/*
static void gen_exception(DisasContext *dc, uint32_t excp)
{
    TCGv_i32 tmp = tcg_const_i32(excp);

    tcg_gen_movi_tl(cpu_R[R_PC], dc->pc);
    gen_helper_raise_exception(cpu_env, tmp);
    tcg_temp_free_i32(tmp);
    dc->base.is_jmp = DISAS_UPDATE;

}
*/

/* generate intermediate code for basic block 'tb'.  
void old_gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int max_insns)
{
    CPURISC6State *env = cs->env_ptr;
    DisasContext dc1, *dc = &dc1;
    int num_insns;

    // Initialize DC 
    dc->cpu_env = cpu_env;
    dc->cpu_R   = cpu_R;
    dc->base.is_jmp  = DISAS_NEXT;
    dc->pc      = tb->pc;
    dc->base.tb = tb;
    dc->mem_idx = cpu_mmu_index(env, false);
    dc->base.singlestep_enabled = cs->singlestep_enabled;

    // Set up instruction counts 
    num_insns = 0;
    if (max_insns > 1) {
        int page_insns = (TARGET_PAGE_SIZE - (tb->pc & TARGET_PAGE_MASK)) / 4;
        if (max_insns > page_insns) {
            max_insns = page_insns;
        }
    }

    gen_tb_start(tb);
    do {
        tcg_gen_insn_start(dc->pc);
        num_insns++;

        if (unlikely(cpu_breakpoint_test(cs, dc->pc, BP_ANY))) {
            gen_exception(dc, EXCP_DEBUG);
            // The address covered by the breakpoint must be included in
            // [tb->pc, tb->pc + tb->size) in order to for it to be
            // properly cleared -- thus we increment the PC here so that
            // the logic setting tb->size below does the right thing.  
            dc->pc += 4;
            break;
        }

        if (num_insns == max_insns && (tb_cflags(tb) & CF_LAST_IO)) {
            gen_io_start();
        }

        // Decode an instruction 
        handle_instruction(dc, env);

        dc->pc += 4;

        // Translation stops when a conditional branch is encountered.
        // Otherwise the subsequent code could get translated several times.
        // Also stop translation when a page boundary is reached.  This
        // ensures prefetch aborts occur at the right place.  
    } while (!dc->base.is_jmp &&
             !tcg_op_buf_full() &&
             num_insns < max_insns);

    if (tb_cflags(tb) & CF_LAST_IO) {
        gen_io_end();
    }

    // Indicate where the next block should start 
    switch (dc->base.is_jmp) {
    case DISAS_NEXT:
        // Save the current PC back into the CPU register 
        tcg_gen_movi_tl(cpu_R[R_PC], dc->pc);
        tcg_gen_exit_tb(NULL, 0);
        break;

    default:
    case DISAS_JUMP:
    case DISAS_UPDATE:
        // The jump will already have updated the PC register 
        tcg_gen_exit_tb(NULL, 0);
        break;

    case DISAS_TB_JUMP:
        // nothing more to generate 
        break;
    }

    // End off the block 
    gen_tb_end(tb, num_insns);

    // Mark instruction starts for the final generated instruction 
    tb->size = dc->pc - tb->pc;
    tb->icount = num_insns;

}

*/

void risc6_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    RISC6CPU *cpu = RISC6_CPU(cs);
    CPURISC6State *env = &cpu->env;
    int i;

    if (!env) {
        return;
    }
    for (i = 0; i < NUM_CORE_REGS; i++) {
        qemu_fprintf(f, "%9s=%8.8x ", regnames[i], env->regs[i]);
        if ((i + 1) % 4 == 0) {
            qemu_fprintf(f, "\n");
        }
    }
    qemu_fprintf(f, "\n\n");
}

///------------------------------



/* dest = T0 - T1. Compute C, N, V and Z flags */
/*
static void gen_sub_CC(TCGv_i32 dest, TCGv_i32 t0, TCGv_i32 t1)
{
    TCGv_i32 tmp;
//    tcg_gen_sub_i32(cpu_NF, t0, t1);
//    tcg_gen_mov_i32(cpu_ZF, cpu_NF);
//    tcg_gen_setcond_i32(TCG_COND_GEU, cpu_CF, t0, t1);
//    tcg_gen_xor_i32(cpu_VF, cpu_NF, t0);
    tmp = tcg_temp_new_i32();
    tcg_gen_xor_i32(tmp, t0, t1);
    tcg_gen_and_i32(cpu_VF, cpu_VF, tmp);
    tcg_temp_free_i32(tmp);
//    tcg_gen_mov_i32(dest, cpu_NF);
}
*/

void risc6_tcg_init(void)
{
    int i;

    for (i = 0; i < NUM_CORE_REGS; i++) {
        cpu_R[i] = tcg_global_mem_new(cpu_env,
                                      offsetof(CPURISC6State, regs[i]),
                                      regnames[i]);
    }
}


static DisasJumpType translate_one(DisasContext *ctx, uint32_t insn, CPUState *cs){
    DisasJumpType ret;
    uint8_t opx;
    uint32_t ldst;
    TCGv addr;
    TCGv data;
    TCGv val;
    TCGLabel *l1;

//    char * thisop;
//    char opbuff[5];

//    thisop = opbuff;

    ret = DISAS_NEXT;
    opx = insn >> 30;

    ctx->zero = NULL;

    I_TYPE(instr, insn);

    
    switch (opx ){
    case 0:
    case 1: 
      switch (instr.op){
      case MOV:  
        data = ctx->cpu_R[instr.a];
        if (instr.opx == 0){                   /* register */
         if (instr.opu == 0){
          if (instr.opv == 0) {
             tcg_gen_mov_tl(ctx->cpu_R[instr.a], ctx->cpu_R[R_H]);
          }else{
             tcg_gen_mov_tl(ctx->cpu_R[instr.a], ctx->cpu_R[R_FLG]);
          }
         }else{
             tcg_gen_mov_tl(ctx->cpu_R[instr.a], ctx->cpu_R[instr.c]);
         }
        }else{                                /* immediate */
         if (instr.opu == 0){
          if (instr.opv == 0) {
            tcg_gen_andi_tl(data, load_gpr(ctx, instr.a), 0x00000000);
          }else{
            tcg_gen_andi_tl(data, load_gpr(ctx, instr.a), 0x00000000);
            tcg_gen_ori_tl(data, load_gpr(ctx, instr.a), 0xFFFF0000);
          }
          tcg_gen_addi_tl(data, load_gpr(ctx, instr.a), instr.imm16.u);
         }else{
          tcg_gen_andi_tl(data, load_gpr(ctx, instr.a), 0x0000FFFF);
          tcg_gen_addi_tl(data, load_gpr(ctx, instr.a), instr.imm16.u << 16);
         }
         // and also test for zero and negative
        }
        break;
      case LSL:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case ASR:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case ROR:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case AND:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case ANN:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case IOR:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case XOR:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case ADD:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case SUB: 
        tcg_gen_sub_tl((ctx)->cpu_R[instr.c], load_gpr((ctx), instr.a),load_gpr(ctx, instr.b));
        tcg_gen_setcond_tl(i_type_instructions[instr.op].flags, ctx->cpu_R[instr.c], ctx->cpu_R[instr.a], ctx->cpu_R[instr.b]);
        tcg_gen_setcondi_tl(i_type_instructions[instr.op].flags, (ctx)->cpu_R[instr.b], (ctx)->cpu_R[instr.a], instr.imm16.u);                   
 
   //     gen_sub_CC(ctx->cpu_R[instr.a],ctx->cpu_R[instr.b],ctx->cpu_R[instr.c]);
        break;
      case MUL:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case DIV:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case FAD:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case FSB:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case FML:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case FDV:  
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      }
//      memcpy( thisop, &REGOPS[3*instr.op], 3 );
//      if ((insn & 0x20000000)!=0) {
//        thisop[3] = '\'';
//      }else{
//        thisop[3] = ' ';
//      }
//      thisop[4] = '\0';
//      if (instr.op==0){
//        printf("first: 0x%08x instr: 0x%08x %s %s     ",ctx->base.pc_first,insn,thisop,regnames[instr.a]);
//      }else{
//        printf("first: 0x%08x instr: 0x%08x %s %s %s ",ctx->base.pc_first,insn,thisop,regnames[instr.a],regnames[instr.b]);
//      }
//      if (instr.opx == 0){
//         if ((insn & 0x20000000)==0){
//           printf("%s\n",regnames[instr.c]);
//         }else{
//         if ((insn & 0x10000000)!=0){
//           printf("H\n");
//         }else{
//           printf("flg\n");
//         }
//         }
//      }else{
//         if ((insn & 0x10000000)!=0){
//           printf("ffff");
//         }
//         printf("%xH\n",instr.imm16.u);
//      }
      break;
    case 2:
      addr = tcg_temp_new();
      ldst = (instr.opu << 1) + ((insn >> 28) & 1);
      switch(ldst){
      case LDR: 
        data = ctx->cpu_R[instr.b];
        tcg_gen_addi_tl(addr, load_gpr(ctx, instr.a), instr.imm16.s);
        tcg_gen_qemu_ld_tl(data, addr, ctx->mem_idx, i_type_instructions[instr.op].flags);
        break;    
      case LDB:
        nop(ctx, insn, i_type_instructions[instr.op].flags);
        break;
      case STR:
        val = load_gpr(ctx, instr.b);
        tcg_gen_addi_tl(addr, load_gpr(ctx, instr.a), instr.imm16.s);
        tcg_gen_qemu_st_tl(val, addr, ctx->mem_idx, i_type_instructions[instr.op].flags);
        break;
      case STB:
        nop(ctx, insn, i_type_instructions[instr.op].flags);   
        break;
      }

      tcg_temp_free(addr);
      
//      memcpy( thisop, &MOVOPS[3*ldst], 3 );
//      thisop[3] = ' ';
//      thisop[4] = '\0';
//      printf("first: 0x%08x instr: 0x%08x %s %s %s %d\n",ctx->base.pc_first,insn,thisop,regnames[instr.a],regnames[instr.b],instr.imm16.s);
      break;
    default:
      if (instr.opv == 1){                   /* return address in r15 */
        tcg_gen_movi_tl(ctx->cpu_R[R_RA], ctx->base.tb->pc + 4);
      }

      l1 = gen_new_label();

      switch (instr.a){
      case BMI:
        tcg_gen_brcond_tl(TCG_COND_LE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BEQ:
        tcg_gen_brcond_tl(TCG_COND_EQ, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BCS:
        tcg_gen_brcond_tl(TCG_COND_LE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BVS:
        tcg_gen_brcond_tl(TCG_COND_LE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BLS:
        tcg_gen_brcond_tl(TCG_COND_LE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BLT:
        tcg_gen_brcond_tl(TCG_COND_LE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BLE:
        tcg_gen_brcond_tl(TCG_COND_LE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        break;
      case BR: 
        tcg_gen_brcond_tl(TCG_COND_ALWAYS, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
//        if (instr.opu == 0) {                     /* dest in register c */
//          tcg_gen_mov_tl(ctx->cpu_R[R_PC], ctx->cpu_R[instr.c]);
//        }else{                                      /* dest pc relative */
//          gen_goto_tb(ctx, 0, ctx->base.tb->pc + 4 + (instr.imm24.s << 2));
//        }
//        ret = DISAS_TB_JUMP;
        break;
      case BPL:
        tcg_gen_brcond_tl(TCG_COND_LE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BNE:
        tcg_gen_brcond_tl(TCG_COND_NE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BCC:
        tcg_gen_brcond_tl(TCG_COND_LE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BVC:
        tcg_gen_brcond_tl(TCG_COND_LE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BHI:
        tcg_gen_brcond_tl(TCG_COND_LE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BGE:
        tcg_gen_brcond_tl(TCG_COND_GE, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case BGT:
        tcg_gen_brcond_tl(TCG_COND_GT, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      case NOP:
        tcg_gen_brcond_tl(TCG_COND_NEVER, ctx->cpu_R[instr.a], ctx->cpu_R[instr.b], l1);
        ret = DISAS_TB_JUMP;
        break;
      }

      gen_goto_tb(ctx, 0, ctx->base.tb->pc + 4);
      gen_set_label(l1);
      if (instr.opu == 0) {                     /* dest in register c */
        tcg_gen_mov_tl(ctx->cpu_R[R_PC], ctx->cpu_R[instr.c]);
      }else{                                      /* dest pc relative */
        gen_goto_tb(ctx, 1, ctx->base.tb->pc + 4 + (instr.imm24.s <<2 ));
      }
      ret = DISAS_TB_JUMP;


//      memcpy( thisop, &BRAOPS[3*instr.a], 3 );
//      if ((insn & 10000000)!=0){
//        thisop[3] = '.';
//      }else{
//        thisop[3] = ' ';
//      }
//      thisop[4] = '\0';

//      printf("first: 0x%08x instr: 0x%08x %s ",ctx->base.pc_first,insn,thisop);

//      if (instr.opu == 0){
//        printf("%s\n",regnames[instr.c]);
//      }else{
//        printf("0x%08x\n",ctx->base.tb->pc + 4 + (instr.imm24.s << 2));
//      }
    }
    char dstrbuff[128];
    ins2str( ctx->base.tb->pc, insn, dstrbuff);
    printf("first: 0x%08x instr: %s\n",ctx->base.pc_first,dstrbuff);


    ctx->base.num_insns=ctx->base.num_insns+1;
    if (ctx->base.num_insns >= ctx->base.max_insns ){
        ret = DISAS_TOO_MANY;
    }


    if (ctx->zero) {
        tcg_temp_free(ctx->zero);
    }

    return ret;
}

static void risc6_tr_init_disas_context(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    CPURISC6State *env = cs->env_ptr;
//    RISC6CPU *cpu = RISC6_CPU(cs);

// DisasContextBase:
//    TranslationBlock *tb;
//    target_ulong pc_first;
//    target_ulong pc_next;
//    DisasJumpType is_jmp;
//    int num_insns;
//    int max_insns;
//    bool singlestep_enabled;
// DisasContext:
//    DisasContextBase  base
//    TCGv             *cpu_R;
//    TCGv_i32          zero;
//    target_ulong      pc;
//    int               mem_idx;

//    int num_insns;
    

    /* Initialize DC */
    ctx->cpu_R   = cpu_R;
//    ctx->pc      = ctx->base.pc_first;
    ctx->mem_idx = cpu_mmu_index(env, false);
    ctx->base.singlestep_enabled = cs->singlestep_enabled;

    /* Set up instruction counts */
    ctx->base.num_insns = 0;
//    if (ctx->base.max_insns > 1) {
        int page_insns = (TARGET_PAGE_SIZE - (ctx->base.tb->pc & TARGET_PAGE_MASK)) / 4;
        if (ctx->base.max_insns > page_insns) {
            ctx->base.max_insns = page_insns;
        }
//    }

   

    ctx->base.max_insns = 1; //page_insns;
}

static void risc6_tr_tb_start(DisasContextBase *dcbase, CPUState *cs)
{
}

static void risc6_tr_insn_start(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    tcg_gen_insn_start(ctx->base.pc_next);  //, ctx->envflags);
}


static bool risc6_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cs,
                                    const CPUBreakpoint *bp)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    /* We have hit a breakpoint - make sure PC is up-to-date */
//    gen_save_cpu_state(ctx, true);
//    gen_helper_debug(cpu_env);
    ctx->base.is_jmp = DISAS_NORETURN;
    /* The address covered by the breakpoint must be included in
       [tb->pc, tb->pc + tb->size) in order to for it to be
       properly cleared -- thus we increment the PC here so that
       the logic setting tb->size below does the right thing.  */
    ctx->base.pc_next += 4;
    return true;
}

static void risc6_tr_translate_insn(DisasContextBase *dcbase, CPUState *cs)
{
    CPURISC6State *env = cs->env_ptr;
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    uint32_t insn = translator_ldl(env, ctx->base.pc_next);

    ctx->base.pc_next += 4;
    ctx->base.is_jmp = translate_one(ctx, insn, cs);

    translator_loop_temp_check(&ctx->base);

}


static void risc6_tr_tb_stop(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    switch (ctx->base.is_jmp) {
    case DISAS_TOO_MANY:
        gen_goto_tb(ctx, 0, ctx->base.pc_next);
        break;
    case DISAS_NORETURN:
        break;
    case DISAS_TB_JUMP:
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

