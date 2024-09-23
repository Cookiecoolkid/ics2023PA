/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#ifdef CONFIG_FTRACE
#include <elf.h>
#include <common.h>
#endif

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S,
  TYPE_N, // none
  TYPE_J, TYPE_B, TYPE_R,
};

extern CPU_state cpu;  // get cpu system register

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immJ() do { *imm = ((((SEXT(BITS(i, 31, 31), 1) << 8) | BITS(i, 19, 12)) << 1 | BITS(i, 20, 20)) << 10 | BITS(i, 30, 21)) << 1; } while(0)
#define immB() do { *imm = (((SEXT(BITS(i, 31,31), 1) << 1 | BITS(i, 7, 7)) << 6 | BITS(i, 30, 25)) << 4 | BITS(i, 11, 8)) << 1; } while(0)
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
	case TYPE_R: src1R(); src2R();         break;
	case TYPE_J:                   immJ(); break;
	case TYPE_B: src1R(); src2R(); immB(); break;
  }
}

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;
 // uint32_t csr_imm = BITS(s->isa.inst.val, 19, 15);

  int csr_rs1 = BITS(s->isa.inst.val, 19, 15);

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}
#ifdef CONFIG_FTRACE
  bool is_jal = false;
  bool is_jalr = false;
  static int stackDepth = 0;
#endif

#ifdef CONFIG_ETRACE
  bool getException = false;
#endif
  INSTPAT_START();
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = src1 * src2);
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, R(rd) = (int32_t)(SEXT(src1, 32) * SEXT(src2, 32) >> 32));
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , R, R(rd) = (uint32_t)((((uint64_t)src1 * (uint64_t)src2)) >> 32));
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = (int32_t)src1 % (int32_t)src2);
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, R(rd) = (unsigned)src1 % (unsigned)src2);
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, R(rd) = (int32_t)src1 / (int32_t)src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, R(rd) = (unsigned)src1 / (unsigned)src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1 ^ src2);
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1 | src2);
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << BITS(src2, 4, 0));
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = (unsigned)src1 >> BITS(src2, 4, 0));
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = (int32_t)src1 >> BITS(src2, 4, 0));
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = (int32_t)src1 < (int32_t)src2);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = (unsigned)src1 < (unsigned)src2);

  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, s->dnpc = (src1 == src2 ? s->pc + imm : s->snpc));
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, s->dnpc = src1 != src2 ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, s->dnpc = ((int32_t)src1 < (int32_t)src2 ? s->pc + imm : s->snpc));
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, s->dnpc = (int32_t)src1 >= (int32_t)src2 ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, s->dnpc = (unsigned)src1 < (unsigned)src2 ? s->pc + imm : s->snpc);
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, s->dnpc = (unsigned)src1 >= (unsigned)src2 ? s->pc + imm : s->snpc);

  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);
#ifdef CONFIG_FTRACE
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, R(rd) = s->snpc, s->dnpc = s->pc + imm, is_jal = true);
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, R(rd) = s->snpc, s->dnpc = (src1 + imm) & ~1, is_jalr = true);
#else
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, R(rd) = s->snpc, s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, R(rd) = s->snpc, s->dnpc = (src1 + imm) & ~1);
#endif
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, R(rd) = SEXT(Mr(src1 + imm, 1), 8));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(Mr(src1 + imm, 2), 16));
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = Mr(src1 + imm, 4));

  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2));
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, R(rd) = src1 < imm? 1 : 0);
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = (uint32_t)src1 < (uint32_t)imm); 
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & imm);
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli   , I, R(rd) = src1 << BITS(imm, 4, 0));
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli   , I, R(rd) = (uint32_t)src1 >> BITS(imm, 4, 0));
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai   , I, R(rd) = (int32_t)src1 >> BITS(imm, 4, 0));

  INSTPAT("0011000 00000 ????? 001 ????? 11100 11", csrrw-mstatus, R, if(rd != 0) R(rd) = cpu.mstatus; cpu.mstatus = src1);
  INSTPAT("0011000 00101 ????? 001 ????? 11100 11", csrrw-mtvec  , R, if(rd != 0) R(rd) = cpu.mtvec; cpu.mtvec = src1);
  INSTPAT("0011010 00001 ????? 001 ????? 11100 11", csrrw-mepc   , R, if(rd != 0) R(rd) = cpu.mepc; cpu.mepc = src1);
  INSTPAT("0011010 00010 ????? 001 ????? 11100 11", csrrw-mcause , R, if(rd != 0) R(rd) = cpu.mcause; cpu.mcause = src1);

  INSTPAT("0011000 00000 ????? 010 ????? 11100 11", csrrs-mstatus, R, if(csr_rs1 != 0 && src1 != 0) cpu.mstatus = (cpu.mstatus | src1); R(rd) = cpu.mstatus);
  INSTPAT("0011000 00101 ????? 010 ????? 11100 11", csrrs-mtvec  , R, if(csr_rs1 != 0 && src1 != 0) cpu.mtvec = (cpu.mtvec | src1); R(rd) = cpu.mtvec);
  INSTPAT("0011010 00001 ????? 010 ????? 11100 11", csrrs-mepc   , R, if(csr_rs1 != 0 && src1 != 0) cpu.mepc = (cpu.mepc | src1); R(rd) = cpu.mepc);
  INSTPAT("0011010 00010 ????? 010 ????? 11100 11", csrrs-mcause , R, if(csr_rs1 != 0 && src1 != 0) cpu.mcause = (cpu.mcause | src1); R(rd) = cpu.mcause);
 /*
  INSTPAT("0011000 00000 ????? 101 ????? 11100 11", csrrwi-mstatus, I, if(rd != 0) R(rd) = (uint32_t)cpu.mstatus; cpu.mstatus = csr_imm);
  INSTPAT("0011000 00101 ????? 101 ????? 11100 11", csrrwi-mtvec  , I, if(rd != 0) R(rd) = (uint32_t)cpu.mtvec; cpu.mtvec = csr_imm);
  INSTPAT("0011010 00001 ????? 101 ????? 11100 11", csrrwi-mepc   , I, if(rd != 0) R(rd) = (uint32_t)cpu.mepc; cpu.mepc = csr_imm);
  INSTPAT("0011010 00010 ????? 101 ????? 11100 11", csrrwi-mcause , I, if(rd != 0) R(rd) = (uint32_t)cpu.mcause; cpu.mcause = csr_imm);
*/

  INSTPAT("0011000 00010 00000 000 00000 11100 11", mret   , N, s->dnpc = cpu.mepc + 4);
  INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall  , N, bool success; s->dnpc = isa_raise_intr(isa_reg_str2val("a7", &success), s->pc); IFDEF(CONFIG_ETRACE, getException = true));
  
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

//  printf("pc: %x\n", cpu.pc);
  R(0) = 0; // reset $zero to 0
  
#ifdef CONFIG_FTRACE

  uint32_t inst_val = s->isa.inst.val; 
  uint32_t Value;
  char funcName[64];
  int rs1 = BITS(inst_val, 19, 15);
  extern func_table funcTable[256];
  if(is_jalr || is_jal)
  {
	 int i;
	 bool is_ret = false;
	 bool is_call = false;
	 for(i = 0; i < sizeof(funcTable) / sizeof(func_table); i++)
	 {
		if(rd == 1 && s->dnpc >= funcTable[i].start &&
		   s->dnpc < funcTable[i].start + funcTable[i].size)
		{
			is_call = true;
			stackDepth++;
		    Value = funcTable[i].start;
		    strncpy(funcName, funcTable[i].func_name, 64);
			break;
		}
		else if (is_jalr && rd == 0 && rs1 == 1 && s->pc >= funcTable[i].start 
			     && s->pc < funcTable[i].start + funcTable[i].size)
		{
			is_ret = true;
			stackDepth--;
			Value = funcTable[i].start;
			strncpy(funcName, funcTable[i].func_name, 64);
			break;
		}
	 }
	 // assert(i != sizeof(funcTable) / sizeof(func_table));
	 assert(stackDepth >= 0);
	 if(is_call || is_ret)
	 {
		 int i = 0;
		 while(i++ < stackDepth) printf(" ");
		 if(is_ret) printf(" ");
	 }
	 if(is_call)
		printf("0x%x: call [%s @ 0x%x]\n", s->pc, funcName, Value);
	 else if(is_ret)
	    printf("0x%x: ret  [%s]\n", s->pc, funcName);
	 is_jalr = false;
	 is_jal = false;
  }

#endif
#ifdef CONFIG_ETRACE
  if(getException)
  {
	  getException = false;
	  printf("Catch Exception: [mepc]:%x, [mcause]:%d, [mtvec]:%x, [mstatus]:%x\n", cpu.mepc, cpu.mcause, cpu.mtvec, cpu.mstatus);
  }
#endif
  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
