/* RISC6 opcode library for QEMU.
   Copyright (C) 2019 Free Software Foundation, Inc.
   Contributed by Charles Perkins (charlesap@gmail.com).

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA  02110-1301, USA.  */


#ifndef _RISC6_H_
#define _RISC6_H_


#include "qemu/osdep.h"
#include "disas/dis-asm.h"
#include "disas/risc6.h"

//enum {
//  MOV, LSL, ASR, ROR,
//  AND, ANN, IOR, XOR,
//  ADD, SUB, MUL, DIV,
//  FAD, FSB, FML, FDV,
//};
//
//enum {
//  BMI, BEQ, BCS, BVS,
//  BLS, BLT, BLE, BR,
//  BPL, BNE, BCC, BVC,
//  BHI, BGE, BGT, NOP,
//};

const char * opMOV = "MOV";
const char * opLSL = "LSL";
const char * opASR = "ASR";
const char * opROR = "ROR";
const char * opAND = "AND";
const char * opANN = "ANN";
const char * opIOR = "IOR";
const char * opXOR = "XOR";
const char * opADD = "ADD";
const char * opSUB = "SUB";
const char * opMUL = "MUL";
const char * opDIV = "DIV";
const char * opFAD = "FAD";
const char * opFSB = "FSB";
const char * opFML = "FML";
const char * opFDV = "FDV";
const char * opLDR = "LDR";
const char * opLDB = "LDB";
const char * opSTR = "STR";
const char * opSTB = "STB";
const char * opBMI = "BMI";
const char * opBEQ = "BEQ";
const char * opBCS = "BCS";
const char * opBVS = "BVS";
const char * opBLS = "BLS";
const char * opBLT = "BLT";
const char * opBLE = "BLE";
const char * opBR  = "BR ";
const char * opBPL = "BPL";
const char * opBNE = "BNE";
const char * opBCC = "BCC";
const char * opBVC = "BVC";
const char * opBHI = "BHI";
const char * opBGE = "BGE";
const char * opBGT = "BGT";
const char * opNOP = "NOP";
const char * opRTI = "RTI";
const char * opSTI = "STI";
const char * opCLI = "CLI";


  const uint32_t pbit = 0x80000000;
  const uint32_t qbit = 0x40000000;
  const uint32_t ubit = 0x20000000;
  const uint32_t vbit = 0x10000000;



struct risc6_opcode
{
  const char *name;		/* The name of the instruction.  */
  const char *args;		/* A string describing the arguments for this 
				   instruction.  */
  unsigned long match;		/* The basic opcode for the instruction.  */
  unsigned long mask;		/* Mask for the opcode field of the 
				   instruction.  */
};

#define IW_OP_LSB 0 
#define IW_OP_SIZE 4

#define IW_OP_UNSHIFTED_MASK (0xffffffffu >> (32 - IW_OP_SIZE)) 
#define IW_OP_SHIFTED_MASK (IW_OP_UNSHIFTED_MASK << IW_OP_LSB) 

#define GET_IW_OP(W) (((W) >> IW_OP_LSB) & IW_OP_UNSHIFTED_MASK) 
#define SET_IW_OP(V) (((V) & IW_OP_UNSHIFTED_MASK) << IW_OP_LSB) 

 
 
extern const struct risc6_opcode risc6_r_opcodes[];
extern const int risc6_num_r_opcodes;
extern struct risc6_opcode *risc6_opcodes;
extern int risc6_num_opcodes;

extern const struct risc6_opcode *
risc6_find_opcode_hash (unsigned long, unsigned long);


#endif /* _RISC6_H */

#define INSNLEN 4
typedef struct _risc6_opcode_link
{
  const struct risc6_opcode *opcode;
  struct _risc6_opcode_link *next;
} risc6_opcode_link;

/* Hash table size.  */
#define OPCODE_HASH_SIZE (IW_OP_UNSHIFTED_MASK + 1)

/* Return a pointer to an risc6_opcode struct for a given instruction
   word OPCODE for bfd machine MACH, or NULL if there is an error.  */

const struct risc6_opcode *
risc6_find_opcode_hash (unsigned long opcode, unsigned long mach)
{

  return NULL;
}


/* risc6_disassemble does all the work of disassembling a RISC6
   instruction opcode.  */

static int
risc6_disassemble (bfd_vma address, unsigned long opcode,
		   disassemble_info *info)
{

  bool found;
  const char * thisop;
  int form;

  form = 1;
  found = true;

  info->bytes_per_line = INSNLEN;
  info->bytes_per_chunk = INSNLEN;
  info->display_endian = info->endian;
  info->insn_info_valid = 1;
  info->branch_delay_insns = 0;
  info->data_size = 0;
  info->insn_type = dis_nonbranch;
  info->target = 0;
  info->target2 = 0;


  if ((opcode & pbit) == 0) {
    // Register instructions
    uint32_t rega  = (opcode & 0x0F000000) >> 24;
    uint32_t regb  = (opcode & 0x00F00000) >> 20;
    uint32_t op = (opcode & 0x000F0000) >> 16;
    uint32_t im =  opcode & 0x0000FFFF;
//    uint32_t c  =  opcode & 0x0000000F;

    thisop = "???";

    switch (op) {
      case MOV: {
	thisop = opMOV;
        if ((opcode & ubit) == 0) { 
        }else if ((opcode & qbit) != 0){
          form = 2;
        }else if ((opcode & vbit) != 0){
          form = 3;
        }else {
          form = 4;
	}
        break;
      }
      case LSL: {       /*ok*/
	thisop = opLSL;
        break;
      }
      case ASR: {       /*ok*/
        thisop = opASR;
        break;
      }
      case ROR: {       /*ok*/
        thisop = opROR;
        break;
      }
      case AND: {       /*ok*/
        thisop = opAND;
        break;
      }
      case ANN: {       /*ok*/
        thisop = opANN;
        break;
      }
      case IOR: {       /*ok*/
        thisop = opIOR;
        break;
      }
      case XOR: {       /*ok*/
        thisop = opXOR;
        break;
      }
      case ADD: {
        thisop = opADD;
        form = 3;
        break;
      }
      case SUB: {
        thisop = opSUB;
        form = 3;
        break;
      }
      case MUL: {
        thisop = opMUL;
        form = 3;
        break;
      }
      case DIV: {
        thisop = opDIV;
        form = 3;
        break;
      }
      case FAD: {
        thisop = opFAD;
        form = 3;
        break;
      }
      case FSB: {
        thisop = opFSB;
        form = 3;
        break;
      }
      case FML: {
        thisop = opFML;
        form = 3;
        break;
      }
      case FDV: {
        thisop = opFDV;
        form = 3;
        break;
      }
    }
   
    (*info->fprintf_func) (info->stream, "0x%08lx ", opcode);
    (*info->fprintf_func) (info->stream, "%s ", thisop);
    (*info->fprintf_func) (info->stream, "r%i ", rega);
    (*info->fprintf_func) (info->stream, "r%i ", regb);
    if (form==1){
      (*info->fprintf_func) (info->stream, "%lxH ", (unsigned long)im);
    }else if (form==2){
      (*info->fprintf_func) (info->stream, "%lx0000H ", (unsigned long)im);
    }else{
      (*info->fprintf_func) (info->stream, "??? ");

    }
      return INSNLEN;

  } else if ((opcode & qbit) == 0) {
    // Memory instructions

    uint32_t rega = (opcode & 0x0F000000) >> 24;
    uint32_t regb = (opcode & 0x00F00000) >> 20;
    int32_t offs = opcode & 0x000FFFFF;
    offs = (offs ^ 0x00080000) - 0x00080000;  // sign-extend

    if ((opcode & ubit) == 0) {
      if ((opcode & vbit) == 0) {
        thisop = opLDR;
      }else{
        thisop = opLDB;
      }
    }else{
      if ((opcode & vbit) == 0) {
        thisop = opSTR;
      }else{
        thisop = opSTB;
      }
    }

    (*info->fprintf_func) (info->stream, "0x%08lx ", opcode);
    (*info->fprintf_func) (info->stream, "%s ", thisop);
    (*info->fprintf_func) (info->stream, "r%i ", rega);
    (*info->fprintf_func) (info->stream, "r%i ", regb);
    (*info->fprintf_func) (info->stream, "%i ", offs);

     return INSNLEN;
  } else {
    // Branch instructions
    uint32_t ccode = (opcode & 0x0F000000) >> 24;
    (*info->fprintf_func) (info->stream, "0x%lx ", opcode);
    switch (ccode) {
      case BMI: {
        thisop = opBMI;
        break;
      }
      case BEQ: {
        thisop = opBEQ;
        break;
      }
      case BCS: {
        thisop = opBCS;
        break;
      }
      case BVS: {
        thisop = opBVS;
        break;
      }
      case BLS: {
        thisop = opBLS;
        break;
      }
      case BLT: {
        thisop = opBLT;
        break;
      }
      case BLE: {
        thisop = opBLE;
        break;
      }
      case BR: {
        thisop = opBR;
        break;
      }
      case BPL: {
        thisop = opBPL;
        break;
      }
      case BNE: {
        thisop = opBNE;
        break;
      }
      case BCC: {
        thisop = opBCC;
        break;
      }
      case BVC: {
        thisop = opBVC;
        break;
      }
      case BHI: {
        thisop = opBHI;
        break;
      }
      case BGE: {
        thisop = opBGE;
        break;
      }
      case BGT: {
        thisop = opBGT;
        break;
      }
      case NOP: {
        thisop = opNOP;
        break;
      }
    }

    (*info->fprintf_func) (info->stream, "%s ", thisop);
    (*info->fprintf_func) (info->stream, "from 0x%08lu ", address);
    if ((opcode & ubit) == 0) {
      (*info->fprintf_func) (info->stream, "r%ld ", opcode & 0x0000000F);
    }else{
      int32_t destv = (opcode & 0x00FFFFFF);
      if ((destv >> 23)==1){
        destv = (destv | 0xFF000000) << 2;
      }else{
        destv = destv << 2;
      }
      (*info->fprintf_func) (info->stream, "0x%08lu ", address + 4 + destv);
    }
    return INSNLEN;


  }


  if (found == true)
    {
     return INSNLEN;
    }
  else
    {
     info->insn_type = dis_noninsn;
     (*info->fprintf_func) (info->stream, "0x%lx", opcode);
     return INSNLEN;
    }
}


/* print_insn_risc6 is the main disassemble function for RISC6.
   The function diassembler(abfd) (source in disassemble.c) returns a
   pointer to this either print_insn_big_risc6 or
   print_insn_little_risc6, which in turn call this function when the
   bfd machine type is risc6. print_insn_risc6 reads the
   instruction word at the address given, and prints the disassembled
   instruction on the stream info->stream using info->fprintf_func. */

static int
print_insn_risc6 (bfd_vma address, disassemble_info *info,
		  enum bfd_endian endianness)
{
  bfd_byte buffer[INSNLEN];
  int status;

  status = (*info->read_memory_func) (address, buffer, INSNLEN, info);
  if (status == 0)
    {
	unsigned long insn;
	insn = (unsigned long) bfd_getl32 (buffer);
        return risc6_disassemble (address, insn, info);
    }

  /* If we got here, we couldn't read anything.  */
  (*info->memory_error_func) (status, address, info);
  return -1;
}

/* These two functions are the main entry points, accessed from
   disassemble.c.  */
int
print_insn_big_risc6 (bfd_vma address, disassemble_info *info)
{
  return print_insn_risc6 (address, info, BFD_ENDIAN_BIG);
}

int
print_insn_little_risc6 (bfd_vma address, disassemble_info *info)
{
  return print_insn_risc6 (address, info, BFD_ENDIAN_LITTLE);
}
