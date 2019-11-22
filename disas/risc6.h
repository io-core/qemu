static const char * const regnames[] = {
    "r0 ",         "r1 ",         "r2 ",        "r3 ",
    "r4 ",         "r5 ",         "r6 ",        "r7 ",
    "r8 ",         "r9 ",         "r10",        "r11",
    "mt ",         "sb ",         "sp ",        "lnk",
    "flg",         "H  ",         "spc",        "pc "
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

static const char * REGOPS = "movlslasrrorandanniorxoraddsubmuldivfadfsbfmlfdv"; //MOVLSLASRRORANDANNIORXORADDSUBMULDIVFADFSBVMLFDV";
static const char * MOVOPS = "ld ldbst sdb"; //LDRLDBSTRSTB";
static const char * BRAOPS = "bmibeqbcsbvsblsbltblebr bplbnebccbvcbhibgebgtnop"; //BMIBEQBCSBVSBLSBLTBLEBR BPLBNEBCCBVCBHIBGEBGTNOP";
