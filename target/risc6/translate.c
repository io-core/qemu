/*
 *  RISC6 translation
 *
 * Copyright (C) 2019 Charles Perkins <chuck@kuracali.com>
 *
 *  Adapted from SH4 translation
 *
 *  Copyright (c) 2005 Samuel Tardieu
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#define DEBUG_DISAS

#include "qemu/osdep.h"
#include "cpu.h"
#include "disas/disas.h"
#include "exec/exec-all.h"
#include "tcg-op.h"
#include "disas/disas.h"
#include "disas/risc6.h"
#include "exec/cpu_ldst.h"
#include "exec/helper-proto.h"
#include "exec/helper-gen.h"
#include "exec/translator.h"
#include "trace-tcg.h"
#include "exec/log.h"
#include "qemu/qemu-print.h"


typedef struct DisasContext {
    DisasContextBase base;

    uint32_t tbflags;  /* should stay unmodified during the TB translation */
    uint32_t envflags; /* should stay in sync with env->flags using TCG ops */
    int memidx;
//    int gbank;
//    int fbank;
//    uint32_t delayed_pc;
//    uint32_t features;

    uint32_t opcode;

//    bool has_movcal;
} DisasContext;

/* Target-specific values for ctx->base.is_jmp.  */
/* We want to exit back to the cpu loop for some reason.
   Usually this is to recognize interrupts immediately.  */
#define DISAS_STOP    DISAS_TARGET_0

/* global register indexes */
static TCGv cpu_R[NUM_CORE_REGS];

//static TCGv cpu_gregs[32];
//static TCGv cpu_sr, cpu_sr_m, cpu_sr_q, cpu_sr_t;

static TCGv cpu_pc, cpu_sr_t; //, cpu_ssr, cpu_spc, cpu_gbr;

//static TCGv cpu_vbr, cpu_sgr, cpu_dbr, cpu_mach, cpu_macl;
//static TCGv cpu_pr, cpu_fpscr, cpu_fpul;
//static TCGv cpu_lock_addr, cpu_lock_value;
//static TCGv cpu_fregs[32];

/* internal register indexes */
static TCGv cpu_flags; //, cpu_delayed_pc, cpu_delayed_cond;

#include "exec/gen-icount.h"

void risc6_tcg_init(void)
{
    int i;

    for (i = 0; i < NUM_CORE_REGS; i++) {
        cpu_R[i] = tcg_global_mem_new(cpu_env,
                                      offsetof(CPURISC6State, regs[i]),
                                      regnames[i]);
    }

    cpu_pc = tcg_global_mem_new_i32(cpu_env,
                                    offsetof(CPURISC6State, pc), "PC");

    cpu_sr_t = tcg_global_mem_new_i32(cpu_env,
                                      offsetof(CPURISC6State, sr_t), "SR_T");
    cpu_flags = tcg_global_mem_new_i32(cpu_env,
				       offsetof(CPURISC6State, flags), "_flags_");

}

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


static inline void gen_save_cpu_state(DisasContext *ctx, bool save_pc)
{
    if (save_pc) {
        tcg_gen_movi_i32(cpu_pc, ctx->base.pc_next);
    }
//    if (ctx->delayed_pc != (uint32_t) -1) {
//        tcg_gen_movi_i32(cpu_delayed_pc, ctx->delayed_pc);
//    }
//    if ((ctx->tbflags & TB_FLAG_ENVFLAGS_MASK) != ctx->envflags) {
//        tcg_gen_movi_i32(cpu_flags, ctx->envflags);
//    }
}

static inline bool use_exit_tb(DisasContext *ctx)
{
    return (ctx->tbflags) != 0;
}

static inline bool use_goto_tb(DisasContext *ctx, target_ulong dest)
{
    /* Use a direct jump if in same page and singlestep not enabled */
    if (unlikely(ctx->base.singlestep_enabled || use_exit_tb(ctx))) {
        return false;
    }
    return true;
}

static void gen_goto_tb(DisasContext *ctx, int n, target_ulong dest)
{
    if (use_goto_tb(ctx, dest)) {
        tcg_gen_goto_tb(n);
        tcg_gen_movi_i32(cpu_pc, dest);
        tcg_gen_exit_tb(ctx->base.tb, n);
    } else {
        tcg_gen_movi_i32(cpu_pc, dest);
        if (ctx->base.singlestep_enabled) {
//            gen_helper_debug(cpu_env);
        } else if (use_exit_tb(ctx)) {
            tcg_gen_exit_tb(NULL, 0);
        } else {
            tcg_gen_lookup_and_goto_ptr();
        }
    }
    ctx->base.is_jmp = DISAS_NORETURN;
}

/*
static void gen_jump(DisasContext * ctx)
{
    if (ctx->delayed_pc == -1) {
	// Target is not statically known, it comes necessarily from a
	// delayed jump as immediate jump are conditinal jumps 
	tcg_gen_mov_i32(cpu_pc, cpu_delayed_pc);
        tcg_gen_discard_i32(cpu_delayed_pc);
        if (ctx->base.singlestep_enabled) {
//            gen_helper_debug(cpu_env);
        } else if (use_exit_tb(ctx)) {
            tcg_gen_exit_tb(NULL, 0);
        } else {
            tcg_gen_lookup_and_goto_ptr();
        }
        ctx->base.is_jmp = DISAS_NORETURN;
    } else {
	gen_goto_tb(ctx, 0, ctx->delayed_pc);
    }
}
*/

/* Immediate conditional jump (bt or bf) */
/*
static void gen_conditional_jump(DisasContext *ctx, target_ulong dest,
                                 bool jump_if_true)
{
    TCGLabel *l1 = gen_new_label();
    TCGCond cond_not_taken = jump_if_true ? TCG_COND_EQ : TCG_COND_NE;

    gen_save_cpu_state(ctx, false);
    tcg_gen_brcondi_i32(cond_not_taken, cpu_sr_t, 0, l1);
    gen_goto_tb(ctx, 0, dest);
    gen_set_label(l1);
    gen_goto_tb(ctx, 1, ctx->base.pc_next + 4);
    ctx->base.is_jmp = DISAS_NORETURN;
}
*/
/* Delayed conditional jump (bt or bf) */
/*
static void gen_delayed_conditional_jump(DisasContext * ctx)
{
    TCGLabel *l1 = gen_new_label();
    TCGv ds = tcg_temp_new();

    tcg_gen_mov_i32(ds, cpu_delayed_cond);
    tcg_gen_discard_i32(cpu_delayed_cond);


    tcg_gen_brcondi_i32(TCG_COND_NE, ds, 0, l1);
    gen_goto_tb(ctx, 1, ctx->base.pc_next + 2);
    gen_set_label(l1);
    gen_jump(ctx);
}
*/

#define IMM16 (ctx->opcode & 0xffff)
#define IMM20s (ctx->opcode & 0x80000 ? 0xfff00000 | (ctx->opcode & 0xfffff) : (ctx->opcode & 0xfffff))
#define IMM24 (ctx->opcode & 0xffffff)
#define IMM24s (ctx->opcode & 0x800000 ? 0xff000000 | (ctx->opcode & 0xffffff) :  (ctx->opcode & 0xffffff))
#define OPCODE ((ctx->opcode >>16) & 0xf)
#define REGA ((ctx->opcode >>24) & 0xf)
#define REGB ((ctx->opcode >>20) & 0xf)
#define REGC (ctx->opcode & 0xf)
#define OPU ((ctx->opcode >>29) & 1)

#define B3_0 (ctx->opcode & 0xf)
#define B6_4 ((ctx->opcode >> 4) & 0x7)
#define B7_4 ((ctx->opcode >> 4) & 0xf)
#define B7_0 (ctx->opcode & 0xff)
#define B7_0s ((int32_t) (int8_t) (ctx->opcode & 0xff))
#define B11_0s (ctx->opcode & 0x800 ? 0xfffff000 | (ctx->opcode & 0xfff) :  (ctx->opcode & 0xfff))
#define B11_8 ((ctx->opcode >> 8) & 0xf)
#define B15_12 ((ctx->opcode >> 12) & 0xf)

#define REG(x)     cpu_gregs[(x) ^ ctx->gbank]
#define ALTREG(x)  cpu_gregs[(x) ^ ctx->gbank ^ 0x10]
#define FREG(x)    cpu_fregs[(x) ^ ctx->fbank]

#define XHACK(x) ((((x) & 1 ) << 4) | ((x) & 0xe))





static void regop_decode_opc(DisasContext * ctx){
    TCGv t0, t_31;

//    printf("regop %08x %08x \n",ctx->base.pc_next,ctx->opcode);

    tcg_gen_addi_tl(cpu_R[R_PC], cpu_R[R_PC], 4);

    TCGv cval = tcg_temp_new_i32();

    switch (ctx->opcode >> 30) {
    case 0:
      tcg_gen_mov_tl(cval, cpu_R[REGC]);
      break;
    case 1:
      tcg_gen_movi_tl(cval, IMM16 );
      if (((ctx->opcode >> 28) & 1) == 1){
        tcg_gen_ori_tl(cval, cval, 0xFFFF0000 );
      }
    }

    switch (OPCODE){
    case MOV:
      switch (ctx->opcode >> 28) {
      case 0:
        tcg_gen_mov_tl(cpu_R[REGA], cval);
        break;
      case 1: // not supposed to happen but...
        tcg_gen_mov_tl(cpu_R[REGA], cval);
        break;
      case 2:
        tcg_gen_mov_tl(cpu_R[REGA], cpu_R[R_H]);
        break;
      case 3:
        tcg_gen_mov_tl(cpu_R[REGA], cpu_R[R_N]);
        tcg_gen_shli_tl(cpu_R[REGA], cpu_R[REGA], 1);
        tcg_gen_or_tl(cpu_R[REGA], cpu_R[REGA], cpu_R[R_Z]);
        tcg_gen_shli_tl(cpu_R[REGA], cpu_R[REGA], 1);
        tcg_gen_or_tl(cpu_R[REGA], cpu_R[REGA], cpu_R[R_C]);
        tcg_gen_shli_tl(cpu_R[REGA], cpu_R[REGA], 1);
        tcg_gen_or_tl(cpu_R[REGA], cpu_R[REGA], cpu_R[R_V]);
        break;
      case 4: // fall through
      case 5:
       tcg_gen_mov_tl(cpu_R[REGA], cval);
       break;
      case 6: // sign extend bits disappear in shift below
      case 7:
        tcg_gen_movi_tl(cpu_R[REGA], IMM16 << 16 );
      }
      break;
    case LSL:
      t_31 = tcg_const_tl(31);
      tcg_gen_and_tl(cval, cval, t_31);
      tcg_gen_shl_tl(cpu_R[REGA], cpu_R[REGB], cval);
      tcg_temp_free(t_31);
      break;
    case ASR:
      t_31 = tcg_const_tl(31);
      tcg_gen_and_tl(cval, cval, t_31);
      tcg_gen_sar_tl(cpu_R[REGA], cpu_R[REGB], cval);
      tcg_temp_free(t_31);
      break;
    case ROR:
      tcg_gen_rotr_i32(cpu_R[REGA], cpu_R[REGB], cval);
      break;
    case AND:
      tcg_gen_and_tl(cpu_R[REGA], cpu_R[REGB], cval);
      break;
    case ANN:
      tcg_gen_not_tl(cval, cval);
      tcg_gen_and_tl(cpu_R[REGA], cpu_R[REGB], cval);
      break;
    case IOR:
      tcg_gen_or_tl(cpu_R[REGA], cpu_R[REGB], cval);
      break;
    case XOR:
      tcg_gen_xor_tl(cpu_R[REGA], cpu_R[REGB], cval);
      break;
    case ADD:
//        risc->C = a_val < b_val;
//        risc->V = ((a_val ^ c_val) & (a_val ^ b_val)) >> 31;
      t0 = tcg_temp_new_i32();
      t_31 = tcg_temp_new_i32();
      tcg_gen_mov_tl(t_31, cpu_R[REGB]);
      if ((ctx->opcode & 0x20000000) == 0){
        tcg_gen_add_tl(cpu_R[REGA], cpu_R[REGB], cval);
      }else{
        tcg_gen_add_tl(t0, cpu_R[REGA], cval);
        tcg_gen_add_tl(cpu_R[REGA], t0, cpu_R[R_C]);
      }
      tcg_gen_setcond_i32(TCG_COND_LTU, cpu_R[R_C], cpu_R[REGA], t_31);
      tcg_temp_free_i32(t0);
      tcg_temp_free_i32(t_31);
      break;
    case SUB:
//        risc->C = a_val > b_val;
//        risc->V = ((b_val ^ c_val) & (a_val ^ b_val)) >> 31;
      t0 = tcg_temp_new_i32();
      t_31 = tcg_temp_new_i32();
      tcg_gen_mov_i32(t_31, cpu_R[REGB]);
      if ((ctx->opcode & 0x20000000) == 0){
        tcg_gen_sub_tl(cpu_R[REGA], cpu_R[REGB],cval);
      }else{
        tcg_gen_sub_tl(t0, cpu_R[REGB],cval);
        tcg_gen_sub_tl(cpu_R[REGA], t0, cpu_R[R_C]);
      }
      tcg_gen_setcond_i32(TCG_COND_GTU, cpu_R[R_C], cpu_R[REGA], t_31);
      tcg_temp_free_i32(t0);
      tcg_temp_free_i32(t_31);
      break;
    case MUL:
      tcg_gen_mulu2_tl(cpu_R[REGA], cpu_R[R_H], cpu_R[REGB], cval);

      break;
    case DIV:
      tcg_gen_divu_tl(cpu_R[REGA], cpu_R[REGB], cval);

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
    tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_R[R_Z], cpu_R[REGA], 0x00);
    tcg_gen_setcondi_tl(TCG_COND_LT, cpu_R[R_N], cpu_R[REGA], 0x00);
    tcg_temp_free(cval);
}

static void memop_decode_opc(DisasContext * ctx){
    TCGv addr = tcg_temp_new_i32();

    tcg_gen_addi_tl(cpu_R[R_PC], cpu_R[R_PC], 4);

    tcg_gen_addi_tl(addr, cpu_R[REGB], IMM20s); 
  

    switch((ctx->opcode >> 28) & 3){
    case LDR:
      tcg_gen_qemu_ld32u(cpu_R[REGA], addr, ctx->memidx);
      tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_R[R_Z], cpu_R[REGA], 0x00);
      tcg_gen_setcondi_tl(TCG_COND_LT, cpu_R[R_N], cpu_R[REGA], 0x00);
      break;
    case LDB:
      tcg_gen_qemu_ld8u(cpu_R[REGA], addr, ctx->memidx);
      tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_R[R_Z], cpu_R[REGA], 0x00);
      tcg_gen_setcondi_tl(TCG_COND_LT, cpu_R[R_N], cpu_R[REGA], 0x00);
      break;
    case STR:
      tcg_gen_qemu_st32(cpu_R[REGA], addr, ctx->memidx);

      break;
    case STB:
      tcg_gen_qemu_st8(cpu_R[REGA], addr, ctx->memidx);
      break;
    }

    tcg_temp_free(addr);
}

static void braop_decode_opc(DisasContext * ctx){
    TCGv dval = tcg_temp_new_i32();
    TCGLabel *l1 = gen_new_label();

//    printf("braop %08x %08x ",ctx->base.pc_next,ctx->opcode);

    tcg_gen_movi_tl(cpu_sr_t, (ctx->opcode >> 27) & 1);
    switch ((ctx->opcode >> 24) & 7) {
      case BMI: tcg_gen_xor_tl(cpu_sr_t, cpu_sr_t, cpu_R[R_N]); break; // BPL
      case BEQ: tcg_gen_xor_tl(cpu_sr_t, cpu_sr_t, cpu_R[R_Z]); break; // BNE
      case BCS: tcg_gen_xor_tl(cpu_sr_t, cpu_sr_t, cpu_R[R_C]); break; // BCC
      case BVS: tcg_gen_xor_tl(cpu_sr_t, cpu_sr_t, cpu_R[R_V]); break; // BVC
      case BLS: tcg_gen_or_tl(dval, cpu_R[R_C], cpu_R[R_Z]);           // BHI
                tcg_gen_xor_tl(cpu_sr_t, cpu_sr_t, dval); break;       
      case BLT: tcg_gen_xor_tl(dval, cpu_R[R_N], cpu_R[R_V]);          // BGE
                tcg_gen_xor_tl(cpu_sr_t, cpu_sr_t, dval); break;
      case BLE: tcg_gen_xor_tl(dval, cpu_R[R_N], cpu_R[R_V]);          // BGT 
                tcg_gen_or_tl(dval, dval, cpu_R[R_Z]);
                tcg_gen_xor_tl(cpu_sr_t, cpu_sr_t, dval); break;       
      case BR:  tcg_gen_xori_tl(cpu_sr_t, cpu_sr_t, 1);                // NOP
    }

    gen_save_cpu_state(ctx, false);
    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_sr_t, 0, l1);

    if (((ctx->opcode >> 28) & 1) == 1){        /* return address in r15 */
      tcg_gen_movi_tl(cpu_R[R_LR], ctx->base.pc_next + 4 );
      tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_R[R_Z], cpu_R[R_LR], 0x00);
      tcg_gen_setcondi_tl(TCG_COND_LT, cpu_R[R_N], cpu_R[R_LR], 0x00);
    }


    if ( OPU  == 0) {                          /* dest in register c */
//      printf(" register branch\n");
      tcg_gen_mov_tl(cpu_R[R_PC], cpu_R[REGC]);
            tcg_gen_exit_tb(NULL, 0);
//            tcg_gen_lookup_and_goto_ptr();
    }else{                                      /* dest pc relative */
      tcg_gen_movi_tl(cpu_R[R_PC],ctx->base.pc_next + 4 + (IMM24s << 2 ));
      gen_goto_tb(ctx, 0, ctx->base.pc_next + 4 + (IMM24s << 2 ));
//      printf(" relative branch to %08x \n",ctx->base.pc_next + 4 + (IMM24s << 2 ));
    }

    gen_set_label(l1);

    tcg_gen_movi_tl(cpu_R[R_PC],ctx->base.pc_next + 4);
    gen_goto_tb(ctx, 1, ctx->base.pc_next + 4);

    ctx->base.is_jmp = DISAS_NORETURN;

    tcg_temp_free(dval);
}

static void decode_opc(DisasContext * ctx)
{
    switch (ctx->opcode >> 30){
    case 0:
    case 1:
      regop_decode_opc(ctx);
      break;
    case 2:
      memop_decode_opc(ctx);
      break;
    default:
      braop_decode_opc(ctx);
    }
}


static void risc6_tr_init_disas_context(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
//    CPURISC6State *env = cs->env_ptr;
    uint32_t tbflags;
    int bound;

    ctx->tbflags = tbflags = ctx->base.tb->flags;
//    ctx->envflags = tbflags & TB_FLAG_ENVFLAGS_MASK;
    ctx->memidx = 0; //(tbflags & (1u << SR_MD)) == 0 ? 1 : 0;
//    /* We don't know if the delayed pc came from a dynamic or static branch,
//       so assume it is a dynamic branch.  */
//    ctx->delayed_pc = -1; /* use delayed pc from env pointer */
//    ctx->features = env->features;
//    ctx->has_movcal = (tbflags & TB_FLAG_PENDING_MOVCA);
//    ctx->gbank = ((tbflags & (1 << SR_MD)) &&
//                  (tbflags & (1 << SR_RB))) * 0x10;
//    ctx->fbank = tbflags & FPSCR_FR ? 0x10 : 0;


    /* Since the ISA is fixed-width, we can bound by the number
       of instructions remaining on the page.  */
    bound = -(ctx->base.pc_next | TARGET_PAGE_MASK) / 4;
    ctx->base.max_insns = MIN(ctx->base.max_insns, bound);
}

static void risc6_tr_tb_start(DisasContextBase *dcbase, CPUState *cs)
{
}

static void risc6_tr_insn_start(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    tcg_gen_insn_start(ctx->base.pc_next); //, ctx->envflags);
}

static bool risc6_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cs,
                                    const CPUBreakpoint *bp)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    /* We have hit a breakpoint - make sure PC is up-to-date */
    gen_save_cpu_state(ctx, true);
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

    ctx->opcode = translator_ldl(env, ctx->base.pc_next);



/*
    TCGv cval;
    TCGv t_31;

    if (ctx->base.pc_next < 0x100000){
    cval = tcg_temp_new_i32();
    t_31 = tcg_const_tl( 0xFFFFFFE0 );
    tcg_gen_movi_i32(cval, ctx->base.pc_next );
    tcg_gen_qemu_st32(cval, t_31, ctx->memidx);
    t_31 = tcg_const_tl( 0xFFFFFFE4 );
    tcg_gen_mov_tl(cval, cpu_R[R_N] );
    tcg_gen_qemu_st32(cval, t_31, ctx->memidx);
    tcg_gen_mov_tl(cval, cpu_R[R_Z] );
    tcg_gen_qemu_st32(cval, t_31, ctx->memidx);
    tcg_gen_mov_tl(cval, cpu_R[R_C] );
    tcg_gen_qemu_st32(cval, t_31, ctx->memidx);
    tcg_gen_mov_tl(cval, cpu_R[R_V] );
    tcg_gen_qemu_st32(cval, t_31, ctx->memidx);
    t_31 = tcg_const_tl( 0xFFFFFFE8 );
    tcg_gen_mov_tl(cval, cpu_R[R_PC] );
    tcg_gen_qemu_st32(cval, t_31, ctx->memidx);
    tcg_gen_mov_tl(cval, cpu_R[12] );
    tcg_gen_qemu_st32(cval, t_31, ctx->memidx);
    tcg_gen_mov_tl(cval, cpu_R[14] );
    tcg_gen_qemu_st32(cval, t_31, ctx->memidx);
    tcg_gen_mov_tl(cval, cpu_R[15] );
    tcg_gen_qemu_st32(cval, t_31, ctx->memidx);
    tcg_gen_mov_tl(cval, cpu_R[0] );
    tcg_gen_qemu_st32(cval, t_31, ctx->memidx);
    tcg_gen_mov_tl(cval, cpu_R[1] );
    tcg_gen_qemu_st32(cval, t_31, ctx->memidx);
     tcg_temp_free(t_31);
    tcg_temp_free(cval);
    }
*/







    decode_opc(ctx);
    ctx->base.pc_next += 4;
}

static void risc6_tr_tb_stop(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);


    switch (ctx->base.is_jmp) {
    case DISAS_STOP:
        gen_save_cpu_state(ctx, true);
        if (ctx->base.singlestep_enabled) {
//            gen_helper_debug(cpu_env);
        } else {
            tcg_gen_exit_tb(NULL, 0);
        }
        break;
    case DISAS_NEXT:
    case DISAS_TOO_MANY:
        gen_save_cpu_state(ctx, false);
        gen_goto_tb(ctx, 0, ctx->base.pc_next);
        break;
    case DISAS_NORETURN:
        break;
    default:
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

void restore_state_to_opc(CPURISC6State *env, TranslationBlock *tb,
                          target_ulong *data)
{
    env->pc = data[0];
    env->flags = data[1];
}
