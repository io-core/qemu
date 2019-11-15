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
#include "exec/helper-proto.h"
#include "exec/helper-gen.h"
#include "exec/log.h"
#include "exec/cpu_ldst.h"
#include "exec/translator.h"
#include "qemu/qemu-print.h"

/* is_jmp field values */
#define DISAS_JUMP    DISAS_TARGET_0 /* only pc was modified dynamically */
#define DISAS_UPDATE  DISAS_TARGET_1 /* cpu state was modified dynamically */
#define DISAS_TB_JUMP DISAS_TARGET_2 /* only pc was modified statically */

#define INSTRUCTION_FLG(func, flags) { (func), (flags) }
#define INSTRUCTION(func)             INSTRUCTION_FLG(func, 0)
#define INSTRUCTION_NOP()             INSTRUCTION_FLG(nop, 0)
#define INSTRUCTION_UNIMPLEMENTED()   INSTRUCTION_FLG(gen_excp, EXCP_UNIMPL)
#define INSTRUCTION_ILLEGAL()         INSTRUCTION_FLG(gen_excp, EXCP_ILLEGAL)


/* I-Type instruction parsing */
#define I_TYPE(instr, code)                \
    struct {                               \
        uint8_t op;                        \
        union {                            \
            uint16_t u;                    \
            int16_t s;                     \
        } imm16;                           \
        union {                            \
            uint32_t u;                    \
            int32_t s;                     \
        } imm20;                           \
        union {                            \
            uint32_t u;                    \
            int32_t s;                     \
        } imm24;                           \
        uint8_t opx;                       \
        uint8_t opv;                       \
        uint8_t opu;                       \
        uint8_t c;                         \
        uint8_t b;                         \
        uint8_t a;                         \
    } (instr) = {                          \
        .op    = extract32((code), 16, 4), \
        .imm16.u = extract32((code), 0, 16), \
        .imm20.s = sextract32((code), 0, 20), \
        .imm24.s = sextract32((code), 0, 24), \
        .opx   = extract32((code), 30, 2), \
        .opv   = extract32((code), 28, 1), \
        .opu   = extract32((code), 29, 1), \
        .c     = extract32((code), 0, 4),  \
        .b     = extract32((code), 20, 4), \
        .a     = extract32((code), 24, 4), \
    }

/* R-Type instruction parsing  */
#define R_TYPE(instr, code)                \
    struct {                               \
        uint8_t op;                        \
        uint8_t imm5;                      \
        uint8_t opx;                       \
        uint8_t c;                         \
        uint8_t b;                         \
        uint8_t a;                         \
    } (instr) = {                          \
        .op    = extract32((code), 16, 4), \
        .imm5  = extract32((code), 6, 5),  \
        .opx   = extract32((code), 28, 4), \
        .c     = extract32((code), 0, 4),  \
        .b     = extract32((code), 20, 4), \
        .a     = extract32((code), 24, 4), \
    }

/* J-Type instruction parsing */
#define J_TYPE(instr, code)                \
    struct {                               \
        uint8_t op;                        \
        uint8_t opx;                       \
        uint8_t opv;                       \
        uint8_t opu;                       \
        uint8_t c;                         \
        uint32_t imm24;                    \
    } (instr) = {                          \
        .op    = extract32((code), 24, 4), \
        .opx   = extract32((code), 30, 2), \
        .opv   = extract32((code), 28, 1), \
        .opu   = extract32((code), 29, 1), \
        .c     = extract32((code), 0, 4),  \
        .imm24 = extract32((code), 0, 24), \
    }

typedef struct DisasContext {
    TCGv_ptr          cpu_env;
    TCGv             *cpu_R;
    TCGv_i32          zero;
    int               is_jmp;
    target_ulong      pc;
    TranslationBlock *tb;
    int               mem_idx;
    bool              singlestep_enabled;
} DisasContext;

typedef struct RISC6Instruction {
    void     (*handler)(DisasContext *dc, uint32_t code, uint32_t flags);
    uint32_t  flags;
} RISC6Instruction;

static uint8_t get_opcode(uint32_t code)
{
    I_TYPE(instr, code);
    return instr.op;
}

static uint8_t get_ucode(uint32_t code)
{
    I_TYPE(instr, code);
    return instr.opu;
}

static uint8_t get_acode(uint32_t code)
{
    I_TYPE(instr, code);
    return instr.a;
}


static TCGv load_gpr(DisasContext *dc, uint8_t reg)
{
//    if (likely(reg != R_ZERO)) {
        return dc->cpu_R[reg];
//    } else {
//        return load_zero(dc);
//    }
}

static void t_gen_helper_raise_exception(DisasContext *dc,
                                         uint32_t index)
{
    TCGv_i32 tmp = tcg_const_i32(index);

    tcg_gen_movi_tl(dc->cpu_R[R_PC], dc->pc);
    gen_helper_raise_exception(dc->cpu_env, tmp);
    tcg_temp_free_i32(tmp);
    dc->is_jmp = DISAS_UPDATE;
}

static bool use_goto_tb(DisasContext *dc, uint32_t dest)
{
    if (unlikely(dc->singlestep_enabled)) {
        return false;
    }

#ifndef CONFIG_USER_ONLY
    return (dc->tb->pc & TARGET_PAGE_MASK) == (dest & TARGET_PAGE_MASK);
#else
    return true;
#endif
}

static void gen_goto_tb(DisasContext *dc, int n, uint32_t dest)
{
    TranslationBlock *tb = dc->tb;

    if (use_goto_tb(dc, dest)) {
        tcg_gen_goto_tb(n);
        tcg_gen_movi_tl(dc->cpu_R[R_PC], dest);
        tcg_gen_exit_tb(tb, n);
    } else {
        tcg_gen_movi_tl(dc->cpu_R[R_PC], dest);
        tcg_gen_exit_tb(NULL, 0);
    }
}

static void gen_excp(DisasContext *dc, uint32_t code, uint32_t flags)
{
    t_gen_helper_raise_exception(dc, flags);
}


/*
 * Used as a placeholder for all instructions which do not have
 * an effect on the simulator (e.g. flush, sync)
 */
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
 * J-Type instructions
 */
static void jmpi(DisasContext *dc, uint32_t code, uint32_t flags)
{
    J_TYPE(instr, code);
    gen_goto_tb(dc, 0, (dc->pc & 0xF0000000) | (instr.imm24 << 2));
    dc->is_jmp = DISAS_TB_JUMP;
}

static void call(DisasContext *dc, uint32_t code, uint32_t flags)
{
    tcg_gen_movi_tl(dc->cpu_R[R_RA], dc->pc + 4);
    jmpi(dc, code, flags);
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
      tcg_gen_movi_tl(dc->cpu_R[R_RA], dc->pc + 4);
    } 

    if (instr.opu == 0) {                     /* dest in register c */
      tcg_gen_mov_tl(dc->cpu_R[R_PC], dc->cpu_R[instr.c]);
    }else{                                      /* dest pc relative */
      gen_goto_tb(dc, 0, dc->pc + 4 + (instr.imm24.s << 2));
    }
    dc->is_jmp = DISAS_TB_JUMP;
}

static void gen_bxx(DisasContext *dc, uint32_t code, uint32_t flags)
{
    I_TYPE(instr, code);

    TCGLabel *l1 = gen_new_label();
    tcg_gen_brcond_tl(flags, dc->cpu_R[instr.a], dc->cpu_R[instr.b], l1);
    gen_goto_tb(dc, 0, dc->pc + 4);
    gen_set_label(l1);
    gen_goto_tb(dc, 1, dc->pc + 4 + (instr.imm16.s & -4));
    dc->is_jmp = DISAS_TB_JUMP;
}

/* Comparison instructions */
#define gen_i_cmpxx(fname, op3)                                              \
static void (fname)(DisasContext *dc, uint32_t code, uint32_t flags)         \
{                                                                            \
    I_TYPE(instr, (code));                                                   \
    tcg_gen_setcondi_tl(flags, (dc)->cpu_R[instr.b], (dc)->cpu_R[instr.a],   \
                        (op3));                                              \
}

gen_i_cmpxx(gen_cmpxxsi, instr.imm16.s)
gen_i_cmpxx(gen_cmpxxui, instr.imm16.u)

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

gen_i_math_logic(andhi, andi, 0, instr.imm16.u << 16)
gen_i_math_logic(orhi , ori,  1, instr.imm16.u << 16)
gen_i_math_logic(xorhi, xori, 1, instr.imm16.u << 16)


/*
 * R-Type instructions
 */
/*
 * status <- estatus
 * PC <- ea
 */
static void eret(DisasContext *dc, uint32_t code, uint32_t flags)
{
//    tcg_gen_mov_tl(dc->cpu_R[CR_STATUS], dc->cpu_R[CR_ESTATUS]);
//    tcg_gen_mov_tl(dc->cpu_R[R_PC], dc->cpu_R[R_EA]);

//    dc->is_jmp = DISAS_JUMP;
}

/* PC <- ra */
static void ret(DisasContext *dc, uint32_t code, uint32_t flags)
{
    tcg_gen_mov_tl(dc->cpu_R[R_PC], dc->cpu_R[R_RA]);

    dc->is_jmp = DISAS_JUMP;
}


/* PC <- rA */
static void jmp(DisasContext *dc, uint32_t code, uint32_t flags)
{
    R_TYPE(instr, code);

    tcg_gen_mov_tl(dc->cpu_R[R_PC], load_gpr(dc, instr.a));

    dc->is_jmp = DISAS_JUMP;
}

/* rC <- PC + 4 */
static void nextpc(DisasContext *dc, uint32_t code, uint32_t flags)
{
    R_TYPE(instr, code);


        tcg_gen_movi_tl(dc->cpu_R[instr.c], dc->pc + 4);

}

/*
 * ra <- PC + 4
 * PC <- rA
 */
static void callr(DisasContext *dc, uint32_t code, uint32_t flags)
{
    R_TYPE(instr, code);

    tcg_gen_mov_tl(dc->cpu_R[R_PC], load_gpr(dc, instr.a));
    tcg_gen_movi_tl(dc->cpu_R[R_RA], dc->pc + 4);

    dc->is_jmp = DISAS_JUMP;
}


/* Comparison instructions */
static void gen_cmpxx(DisasContext *dc, uint32_t code, uint32_t flags)
{
    R_TYPE(instr, code);

        tcg_gen_setcond_tl(flags, dc->cpu_R[instr.c], dc->cpu_R[instr.a],
                           dc->cpu_R[instr.b]);

}

/* Math/logic instructions */
#define gen_r_math_logic(fname, insn, op3)                                 \
static void (fname)(DisasContext *dc, uint32_t code, uint32_t flags)       \
{                                                                          \
    R_TYPE(instr, (code));                                                 \
        tcg_gen_##insn((dc)->cpu_R[instr.c], load_gpr((dc), instr.a),      \
                       (op3));                                             \
}

gen_r_math_logic(add,  add_tl,   load_gpr(dc, instr.b))
gen_r_math_logic(sub,  sub_tl,   load_gpr(dc, instr.b))
gen_r_math_logic(mul,  mul_tl,   load_gpr(dc, instr.b))

gen_r_math_logic(and,  and_tl,   load_gpr(dc, instr.b))
gen_r_math_logic(or,   or_tl,    load_gpr(dc, instr.b))
gen_r_math_logic(xor,  xor_tl,   load_gpr(dc, instr.b))
gen_r_math_logic(nor,  nor_tl,   load_gpr(dc, instr.b))

gen_r_math_logic(srai, sari_tl,  instr.imm5)
gen_r_math_logic(srli, shri_tl,  instr.imm5)
gen_r_math_logic(slli, shli_tl,  instr.imm5)
gen_r_math_logic(roli, rotli_tl, instr.imm5)

#define gen_r_mul(fname, insn)                                         \
static void (fname)(DisasContext *dc, uint32_t code, uint32_t flags)   \
{                                                                      \
    R_TYPE(instr, (code));                                             \
        TCGv t0 = tcg_temp_new();                                      \
        tcg_gen_##insn(t0, dc->cpu_R[instr.c],                         \
                       load_gpr(dc, instr.a), load_gpr(dc, instr.b)); \
        tcg_temp_free(t0);                                             \
}

gen_r_mul(mulxss, muls2_tl)
gen_r_mul(mulxuu, mulu2_tl)
gen_r_mul(mulxsu, mulsu2_tl)

#define gen_r_shift_s(fname, insn)                                         \
static void (fname)(DisasContext *dc, uint32_t code, uint32_t flags)       \
{                                                                          \
    R_TYPE(instr, (code));                                                 \
        TCGv t0 = tcg_temp_new();                                          \
        tcg_gen_andi_tl(t0, load_gpr((dc), instr.b), 31);                  \
        tcg_gen_##insn((dc)->cpu_R[instr.c], load_gpr((dc), instr.a), t0); \
        tcg_temp_free(t0);                                                 \
}

gen_r_shift_s(sra, sar_tl)
gen_r_shift_s(srl, shr_tl)
gen_r_shift_s(sll, shl_tl)
gen_r_shift_s(rol, rotl_tl)
gen_r_shift_s(ror, rotr_tl)

static void divs(DisasContext *dc, uint32_t code, uint32_t flags)
{
    R_TYPE(instr, (code));


    TCGv t0 = tcg_temp_new();
    TCGv t1 = tcg_temp_new();
    TCGv t2 = tcg_temp_new();
    TCGv t3 = tcg_temp_new();

    tcg_gen_ext32s_tl(t0, load_gpr(dc, instr.a));
    tcg_gen_ext32s_tl(t1, load_gpr(dc, instr.b));
    tcg_gen_setcondi_tl(TCG_COND_EQ, t2, t0, INT_MIN);
    tcg_gen_setcondi_tl(TCG_COND_EQ, t3, t1, -1);
    tcg_gen_and_tl(t2, t2, t3);
    tcg_gen_setcondi_tl(TCG_COND_EQ, t3, t1, 0);
    tcg_gen_or_tl(t2, t2, t3);
    tcg_gen_movi_tl(t3, 0);
    tcg_gen_movcond_tl(TCG_COND_NE, t1, t2, t3, t2, t1);
    tcg_gen_div_tl(dc->cpu_R[instr.c], t0, t1);
    tcg_gen_ext32s_tl(dc->cpu_R[instr.c], dc->cpu_R[instr.c]);

    tcg_temp_free(t3);
    tcg_temp_free(t2);
    tcg_temp_free(t1);
    tcg_temp_free(t0);
}

static void divu(DisasContext *dc, uint32_t code, uint32_t flags)
{
    R_TYPE(instr, (code));


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



static void handle_instruction(DisasContext *dc, CPURISC6State *env)
{
    uint32_t code;
    uint8_t op;
    uint8_t opx;
    
    const RISC6Instruction *instr;

    code = cpu_ldl_code(env, dc->pc);
    opx = code >> 30;

    if (opx < 2){
      op = get_opcode(code);
    }else if (opx == 2){
      op = 16 + (get_ucode(code) << 1) + ((code >> 28) & 1) ; 
    }else{
      op = 20 + get_acode(code);
    }

    dc->zero = NULL;

    instr = &i_type_instructions[op];
    instr->handler(dc, code, instr->flags);

    if (dc->zero) {
        tcg_temp_free(dc->zero);
    }

    return;

}

static const char * const regnames[] = {
    "r0",         "r1",         "r2",         "r3",
    "r4",         "r5",         "r6",         "r7",
    "r8",         "r9",         "r10",        "r11",
    "r12",        "r13",        "r14",        "r15",
    "N",          "Z",          "C",          "V",
    "H",          "spc",        "pc"
};
static TCGv cpu_R[NUM_CORE_REGS];

#include "exec/gen-icount.h"

static void gen_exception(DisasContext *dc, uint32_t excp)
{
    TCGv_i32 tmp = tcg_const_i32(excp);

    tcg_gen_movi_tl(cpu_R[R_PC], dc->pc);
    gen_helper_raise_exception(cpu_env, tmp);
    tcg_temp_free_i32(tmp);
    dc->is_jmp = DISAS_UPDATE;
}

/* generate intermediate code for basic block 'tb'.  */
void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int max_insns)
{
    CPURISC6State *env = cs->env_ptr;
    DisasContext dc1, *dc = &dc1;
    int num_insns;

    /* Initialize DC */
    dc->cpu_env = cpu_env;
    dc->cpu_R   = cpu_R;
    dc->is_jmp  = DISAS_NEXT;
    dc->pc      = tb->pc;
    dc->tb      = tb;
    dc->mem_idx = cpu_mmu_index(env, false);
    dc->singlestep_enabled = cs->singlestep_enabled;

    /* Set up instruction counts */
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
            /* The address covered by the breakpoint must be included in
               [tb->pc, tb->pc + tb->size) in order to for it to be
               properly cleared -- thus we increment the PC here so that
               the logic setting tb->size below does the right thing.  */
            dc->pc += 4;
            break;
        }

        if (num_insns == max_insns && (tb_cflags(tb) & CF_LAST_IO)) {
            gen_io_start();
        }

        /* Decode an instruction */
        handle_instruction(dc, env);

        dc->pc += 4;

        /* Translation stops when a conditional branch is encountered.
         * Otherwise the subsequent code could get translated several times.
         * Also stop translation when a page boundary is reached.  This
         * ensures prefetch aborts occur at the right place.  */
    } while (!dc->is_jmp &&
             !tcg_op_buf_full() &&
             num_insns < max_insns);

    if (tb_cflags(tb) & CF_LAST_IO) {
        gen_io_end();
    }

    /* Indicate where the next block should start */
    switch (dc->is_jmp) {
    case DISAS_NEXT:
        /* Save the current PC back into the CPU register */
        tcg_gen_movi_tl(cpu_R[R_PC], dc->pc);
        tcg_gen_exit_tb(NULL, 0);
        break;

    default:
    case DISAS_JUMP:
    case DISAS_UPDATE:
        /* The jump will already have updated the PC register */
        tcg_gen_exit_tb(NULL, 0);
        break;

    case DISAS_TB_JUMP:
        /* nothing more to generate */
        break;
    }

    /* End off the block */
    gen_tb_end(tb, num_insns);

    /* Mark instruction starts for the final generated instruction */
    tb->size = dc->pc - tb->pc;
    tb->icount = num_insns;

}

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

void risc6_tcg_init(void)
{
    int i;

    for (i = 0; i < NUM_CORE_REGS; i++) {
        cpu_R[i] = tcg_global_mem_new(cpu_env,
                                      offsetof(CPURISC6State, regs[i]),
                                      regnames[i]);
    }
}

void restore_state_to_opc(CPURISC6State *env, TranslationBlock *tb,
                          target_ulong *data)
{
    env->regs[R_PC] = data[0];
}
