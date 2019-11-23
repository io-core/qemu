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

#endif /* _RISC6_H */

#define INSNLEN 4

static const char * REGOPS = "movlslasrrorandanniorxoraddsubmuldivfadfsbfmlfdv";
static const char * MOVOPS = "ld ldbst sdb";
static const char * BRAOPS = "bmibeqbcsbvsblsbltblebr bplbnebccbvcbhibgebgtnop";

void ins2str( unsigned long addr, unsigned long insn, char * dstr){   
    uint8_t opx;
    uint32_t ldst;
    char * thisop;
    char opbuff[5];
    int creg;
    const char * nprfx;

   
    nprfx = "";
    thisop = opbuff;
    opx = insn >> 30;

    I_TYPE(instr, insn);

    switch (opx ){
    case 0:
    case 1:
      memcpy( thisop, &REGOPS[3*instr.op], 3 );
      if ((insn & 0x20000000)!=0) {
        thisop[3] = '\'';
      }else{
        thisop[3] = ' ';
      }
      thisop[4] = '\0';
      
      if (instr.opx == 0){
         if ((insn & 0x20000000)==0){
           creg = instr.c;

         }else{
           if ((insn & 0x10000000)!=0){
             creg = 17; //R_H
             printf("H\n");
           }else{
             creg = 16; //R_FLG
           }
         }
         if (instr.op==0){
            sprintf(dstr,"%08lx %s %s %s",insn,thisop,regnames[instr.a],regnames[creg]);
         }else{
            sprintf(dstr,"%08lx %s %s %s %s",insn,thisop,regnames[instr.a],regnames[instr.b],regnames[creg]);
         }
      }else{
         if ((insn & 0x10000000)!=0){
           nprfx = "ffff";
         }
         if (instr.op==0){
           sprintf(dstr,"%08lx %s %s %s%xH",insn,thisop,regnames[instr.a],nprfx,instr.imm16.u);
         }else{
           sprintf(dstr,"%08lx %s %s %s %s%xH",insn,thisop,regnames[instr.a],regnames[instr.b],nprfx,instr.imm16.u);
         }
      }
      break;
    case 2:
      ldst = (instr.opu << 1) + ((insn >> 28) & 1);
      memcpy( thisop, &MOVOPS[3*ldst], 3 );
      thisop[3] = ' ';
      thisop[4] = '\0';
      sprintf(dstr,"%08lx %s %s %s %d",insn,thisop,regnames[instr.a],regnames[instr.b],instr.imm16.s);
      break;
    default:
      memcpy( thisop, &BRAOPS[3*instr.a], 3 );
      if ((insn & 10000000)!=0){
        thisop[3] = '.';
      }else{
        thisop[3] = ' ';
      }
      thisop[4] = '\0';
     
      if (instr.opu == 0){
        sprintf(dstr,"%08lx %s %s",insn,thisop,regnames[instr.c]);
      }else{
        sprintf(dstr,"%08lx %s 0x%08lx",insn,thisop,addr + 4 + (instr.imm24.s << 2));
      }
    }
}

/* risc6_disassemble does all the work of disassembling a RISC6
   instruction opcode.  */

static int
risc6_disassemble (bfd_vma address, unsigned long opcode,
		   disassemble_info *info)
{
  char dstrbuff[128];
  ins2str( address, opcode, dstrbuff);
  (*info->fprintf_func) (info->stream, "%s",dstrbuff);
  return INSNLEN;
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
