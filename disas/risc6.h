#include "qemu/bitops.h"

static const char * const regnames[] = {
    "r0",         "r1",         "r2",        "r3",
    "r4",         "r5",         "r6",        "r7",
    "r8",         "r9",         "ra",        "rb",
    "mt",         "sb",         "sp",        "lr",
    "rc",         "rv",         "rn",        "rz",
    "rh",         "xc",        "pc"
};

enum {
  MOV, LSL, ASR, ROR,
  AND, ANN, IOR, XOR,
  ADD, SUB, MUL, DIV,
  FAD, FSB, FML, FDV,
};

enum {
  LDR, LDB, STR, STB,
};

enum {
  BMI, BEQ, BCS, BVS,
  BLS, BLT, BLE, BR,
  BPL, BNE, BCC, BVC,
  BHI, BGE, BGT, NOP,
};
/*
const char * REGOPS = "movlslasrrorandanniorxoraddsubmuldivfadfsbfmlfdv";
const char * MOVOPS = "ld ldbst sdb";
const char * BRAOPS = "bmibeqbcsbvsblsbltblebr bplbnebccbvcbhibgebgtnop";
*/
/* instruction parsing */
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
        uint8_t imm5;                      \
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
        .imm5  = extract32((code), 6, 5),  \
        .opx   = extract32((code), 30, 2), \
        .opv   = extract32((code), 28, 1), \
        .opu   = extract32((code), 29, 1), \
        .c     = extract32((code), 0, 4),  \
        .b     = extract32((code), 20, 4), \
        .a     = extract32((code), 24, 4), \
    }

void ins2str( unsigned long addr, unsigned long insn, char * dstr);

