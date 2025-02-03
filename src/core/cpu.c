/* Copyright (C) 2024, 2025  Angus McLean
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/cpu.h"
#include "core/errors.h"
#include "core/memory.h"

/* Sets current instruction and address mode strings in cpu_state struct
 * by "stringifying" them e.g. opstr = ADC, mode = ABS_X -> "ADC", "ABS_X",
 * and set cpu state struct current opcode.
 */
#define SET_INSTRUCTION(opstr, opcode, mode)                                   \
  do {                                                                         \
    cpu_state.curr_instruction = #opstr;                                       \
    cpu_state.curr_addr_mode = #mode;                                          \
    cpu_state.opc = opcode;                                                    \
  } while (0)

/* Same as above but asterix is prepended to opstr e.g. opstr = NOP -> "*NOP" */
#define SET_ILLEGAL_INSTRUCTION(opstr, opcode, mode)                           \
  do {                                                                         \
    cpu_state.curr_instruction = "*" #opstr;                                   \
    cpu_state.curr_addr_mode = #mode;                                          \
    cpu_state.opc = opcode;                                                    \
  } while (0)

/*
 * These macros expand out all the cases for instructions with different
 * addressing modes, using that all of the different versions of an instruction
 * appear on the same row (or in one case column) of the opcode table.
 */

#define OPC_CASE(op, offset, mode)                                             \
  case (op##_BASE + offset):                                                   \
    SET_INSTRUCTION(op, op##_BASE + offset, mode);                             \
    op(cpu, mode);                                                             \
    break;

#define IS_ALU_OPC(op)                                                         \
  OPC_CASE(op, 0x1, IND_X)                                                     \
  OPC_CASE(op, 0x5, ZP)                                                        \
  OPC_CASE(op, 0x9, IMM)                                                       \
  OPC_CASE(op, 0xD, ABS)                                                       \
  OPC_CASE(op, 0x11, IND_Y)                                                    \
  OPC_CASE(op, 0x15, ZP_X)                                                     \
  OPC_CASE(op, 0x19, ABS_Y)                                                    \
  OPC_CASE(op, 0x1D, ABS_X)

#define IS_RMW_OPC(op)                                                         \
  OPC_CASE(op, 0x6, ZP)                                                        \
  OPC_CASE(op, 0xA, IMP)                                                       \
  OPC_CASE(op, 0xE, ABS)                                                       \
  OPC_CASE(op, 0x16, ZP_X)                                                     \
  OPC_CASE(op, 0x1E, ABS_X_EC)

#define IS_INC_DEC(op)                                                         \
  OPC_CASE(op, 0x6, ZP)                                                        \
  OPC_CASE(op, 0xE, ABS)                                                       \
  OPC_CASE(op, 0x16, ZP_X)                                                     \
  OPC_CASE(op, 0x1E, ABS_X_EC)

#define IS_CPX_CPY(op)                                                         \
  OPC_CASE(op, 0x0, IMM)                                                       \
  OPC_CASE(op, 0x4, ZP)                                                        \
  OPC_CASE(op, 0xC, ABS)

#define IS_STA()                                                               \
  OPC_CASE(STA, 0x1, IND_X)                                                    \
  OPC_CASE(STA, 0x5, ZP)                                                       \
  OPC_CASE(STA, 0xD, ABS)                                                      \
  OPC_CASE(STA, 0x11, IND_Y_EC)                                                \
  OPC_CASE(STA, 0x15, ZP_X)                                                    \
  OPC_CASE(STA, 0x19, ABS_Y_EC)                                                \
  OPC_CASE(STA, 0x1D, ABS_X_EC)

#define IS_STX()                                                               \
  OPC_CASE(STX, 0x6, ZP)                                                       \
  OPC_CASE(STX, 0xE, ABS)                                                      \
  OPC_CASE(STX, 0x16, ZP_Y)

#define IS_STY()                                                               \
  OPC_CASE(STY, 0x4, ZP)                                                       \
  OPC_CASE(STY, 0xC, ABS)                                                      \
  OPC_CASE(STY, 0x14, ZP_X)

#define IS_LDX()                                                               \
  OPC_CASE(LDX, 0x2, IMM)                                                      \
  OPC_CASE(LDX, 0x6, ZP)                                                       \
  OPC_CASE(LDX, 0xE, ABS)                                                      \
  OPC_CASE(LDX, 0x16, ZP_Y)                                                    \
  OPC_CASE(LDX, 0x1E, ABS_Y)

#define IS_LDY()                                                               \
  OPC_CASE(LDY, 0x0, IMM)                                                      \
  OPC_CASE(LDY, 0x4, ZP)                                                       \
  OPC_CASE(LDY, 0xC, ABS)                                                      \
  OPC_CASE(LDY, 0x14, ZP_X)                                                    \
  OPC_CASE(LDY, 0x1C, ABS_X)

#define IS_BIT()                                                               \
  OPC_CASE(BIT, 0x4, ZP)                                                       \
  OPC_CASE(BIT, 0xC, ABS)

#define IS_JSR() OPC_CASE(JSR, 0x0, ABS)

#define IS_BRANCH_OPC()                                                        \
  OPC_CASE(BPL, 0x10, REL)                                                     \
  OPC_CASE(BMI, 0x10, REL)                                                     \
  OPC_CASE(BVC, 0x10, REL)                                                     \
  OPC_CASE(BVS, 0x10, REL)                                                     \
  OPC_CASE(BCC, 0x10, REL)                                                     \
  OPC_CASE(BCS, 0x10, REL)                                                     \
  OPC_CASE(BNE, 0x10, REL)                                                     \
  OPC_CASE(BEQ, 0x10, REL)

#define IS_OPC(op)                                                             \
  case op##_OPC:                                                               \
    SET_INSTRUCTION(op, op##_OPC, IMP);                                        \
    op(cpu, IMP);                                                              \
    break;

#define _IS_JMP(base, mode)                                                    \
  case (JMP_OFFSET + base):                                                    \
    SET_INSTRUCTION(JMP, JMP_OFFSET + base, mode);                             \
    JMP(cpu, mode);                                                            \
    break;

#define IS_JMP()                                                               \
  _IS_JMP(0x40, ABS)                                                           \
  _IS_JMP(0x60, ABS_IND)

/* illegal opcodes */

#define ILLEGAL_OPC_CASE(op, offset, mode)                                     \
  case (op##_BASE + offset):                                                   \
    SET_ILLEGAL_INSTRUCTION(op, op##_BASE + offset, mode);                     \
    op(cpu, mode);                                                             \
    break;

/* For individual illegal opcodes */
#define IS_ILLEGAL_OPC(op, opc, mode)                                          \
  case (opc):                                                                  \
    SET_ILLEGAL_INSTRUCTION(op, opc, mode);                                    \
    op(cpu, mode);                                                             \
    break;

#define _IS_ILLEGAL_NOP(base, mode)                                            \
  case (NOP_##mode##_OFFSET + base):                                           \
    SET_ILLEGAL_INSTRUCTION(NOP, NOP_##mode##_OFFSET + base, mode);            \
    NOP(cpu, mode);                                                            \
    break;

#define IS_ILLEGAL_NOP()                                                       \
  _IS_ILLEGAL_NOP(0X0, ZP)                                                     \
  _IS_ILLEGAL_NOP(0X40, ZP)                                                    \
  _IS_ILLEGAL_NOP(0X60, ZP)                                                    \
  _IS_ILLEGAL_NOP(0X0, ABS)                                                    \
  _IS_ILLEGAL_NOP(0X0, ZP_X)                                                   \
  _IS_ILLEGAL_NOP(0X20, ZP_X)                                                  \
  _IS_ILLEGAL_NOP(0X40, ZP_X)                                                  \
  _IS_ILLEGAL_NOP(0X60, ZP_X)                                                  \
  _IS_ILLEGAL_NOP(0XC0, ZP_X)                                                  \
  _IS_ILLEGAL_NOP(0XE0, ZP_X)                                                  \
  _IS_ILLEGAL_NOP(0X0, ABS_X)                                                  \
  _IS_ILLEGAL_NOP(0X20, ABS_X)                                                 \
  _IS_ILLEGAL_NOP(0X40, ABS_X)                                                 \
  _IS_ILLEGAL_NOP(0X60, ABS_X)                                                 \
  _IS_ILLEGAL_NOP(0XC0, ABS_X)                                                 \
  _IS_ILLEGAL_NOP(0XE0, ABS_X)                                                 \
  _IS_ILLEGAL_NOP(0X0, IMP)                                                    \
  _IS_ILLEGAL_NOP(0X20, IMP)                                                   \
  _IS_ILLEGAL_NOP(0X40, IMP)                                                   \
  _IS_ILLEGAL_NOP(0X60, IMP)                                                   \
  _IS_ILLEGAL_NOP(0XC0, IMP)                                                   \
  _IS_ILLEGAL_NOP(0XE0, IMP)                                                   \
  IS_ILLEGAL_OPC(NOP, 0x80, IMM)                                               \
  IS_ILLEGAL_OPC(NOP, 0X89, IMM)                                               \
  IS_ILLEGAL_OPC(NOP, 0X82, IMM)                                               \
  IS_ILLEGAL_OPC(NOP, 0XC2, IMM)                                               \
  IS_ILLEGAL_OPC(NOP, 0XE2, IMM)

#define IS_LAX()                                                               \
  ILLEGAL_OPC_CASE(LAX, 0x03, IND_X)                                           \
  ILLEGAL_OPC_CASE(LAX, 0X07, ZP)                                              \
  ILLEGAL_OPC_CASE(LAX, 0X0F, ABS)                                             \
  ILLEGAL_OPC_CASE(LAX, 0X13, IND_Y)                                           \
  ILLEGAL_OPC_CASE(LAX, 0X17, ZP_Y)                                            \
  ILLEGAL_OPC_CASE(LAX, 0X1F, ABS_Y)

#define IS_SAX()                                                               \
  ILLEGAL_OPC_CASE(SAX, 0X03, IND_X)                                           \
  ILLEGAL_OPC_CASE(SAX, 0X07, ZP)                                              \
  ILLEGAL_OPC_CASE(SAX, 0X0F, ABS)                                             \
  ILLEGAL_OPC_CASE(SAX, 0X17, ZP_Y)

#define IS_ILLEGAL_SBC() IS_ILLEGAL_OPC(SBC, 0XEB, IMM)

#define IS_ILLEGAL_RMW_OPC(op)                                                 \
  ILLEGAL_OPC_CASE(op, 0x03, IND_X)                                            \
  ILLEGAL_OPC_CASE(op, 0x07, ZP)                                               \
  ILLEGAL_OPC_CASE(op, 0x0f, ABS)                                              \
  ILLEGAL_OPC_CASE(op, 0x13, IND_Y_EC)                                         \
  ILLEGAL_OPC_CASE(op, 0x17, ZP_X)                                             \
  ILLEGAL_OPC_CASE(op, 0x1b, ABS_Y_EC)                                         \
  ILLEGAL_OPC_CASE(op, 0x1f, ABS_X_EC)

/* apparently this one doesn't work properly: */
/*ILLEGAL_OPC_CASE(LAX, 0X0B, IMM)		\*/

/* Masks for CPU flags: */

/* 0x30: 00110000 */
#define MASK_NVDIZC 0x30

/* 0x34: 00110100 */
#define MASK_NVDZC 0x34

/* 0x3C: 00111100 */
#define MASK_NVZC 0x3C

/* 0x3D: 00111101 */
#define MASK_NVZ 0x3D

/* 0x7C: 01111100 */
#define MASK_NZC 0x7C

/* 0x7D: 01111101 */
#define MASK_NZ 0x7D

/* 0xFB: 11111011 */
#define MASK_I 0xFB

enum {
  BPL_BASE = 0x0,
  ORA_BASE = 0x0,
  ASL_BASE = 0x0,
  JSR_BASE = 0x20,
  BMI_BASE = 0x20,
  BIT_BASE = 0X20,
  ROL_BASE = 0x20,
  AND_BASE = 0x20,
  BVC_BASE = 0x40,
  LSR_BASE = 0x40,
  EOR_BASE = 0x40,
  BVS_BASE = 0x60,
  ROR_BASE = 0x60,
  ADC_BASE = 0x60,
  BCC_BASE = 0x80,
  STX_BASE = 0x80,
  STY_BASE = 0x80,
  STA_BASE = 0x80,
  BCS_BASE = 0xA0,
  LDX_BASE = 0xA0,
  LDY_BASE = 0xA0,
  LDA_BASE = 0xA0,
  BNE_BASE = 0xC0,
  DEC_BASE = 0xC0,
  CMP_BASE = 0xC0,
  CPY_BASE = 0xC0,
  BEQ_BASE = 0xE0,
  CPX_BASE = 0xE0,
  INC_BASE = 0xE0,
  SBC_BASE = 0xE0
};

enum {
  BRK_OPC = 0x0,
  PHP_OPC = 0x8,
  CLC_OPC = 0x18,
  PLP_OPC = 0x28,
  SEC_OPC = 0x38,
  RTI_OPC = 0x40,
  PHA_OPC = 0x48,
  CLI_OPC = 0x58,
  RTS_OPC = 0x60,
  PLA_OPC = 0x68,
  SEI_OPC = 0x78,
  DEY_OPC = 0x88,
  TXA_OPC = 0x8A,
  TYA_OPC = 0x98,
  TXS_OPC = 0X9A,
  TAY_OPC = 0xA8,
  TAX_OPC = 0xAA,
  CLV_OPC = 0xB8,
  TSX_OPC = 0xBA,
  INY_OPC = 0xC8,
  DEX_OPC = 0xCA,
  CLD_OPC = 0xD8,
  INX_OPC = 0xE8,
  NOP_OPC = 0xEA,
  SED_OPC = 0xF8,
};

#define JMP_OFFSET 0xC

/* illegal opcode stuff */
enum {
  NOP_ZP_OFFSET = 0X4,
  NOP_ABS_OFFSET = 0XC,
  NOP_ZP_X_OFFSET = 0X14,
  NOP_ABS_X_OFFSET = 0X1C,
  NOP_IMP_OFFSET = 0X1A
};

enum {
  SLO_BASE = 0X0,
  RLA_BASE = 0X20,
  SRE_BASE = 0X40,
  RRA_BASE = 0X60,
  SAX_BASE = 0X80,
  LAX_BASE = 0xA0,
  DCP_BASE = 0XC0,
  ISB_BASE = 0xE0,
};

enum {
  FLAG_CARRY = 1 << 0,
  FLAG_ZERO = 1 << 1,
  FLAG_INT_DISABLE = 1 << 2,
  FLAG_DECIMAL = 1 << 3,
  FLAG_BREAK = 1 << 4,
  FLAG_UNUSED = 1 << 5,
  FLAG_OVERFLOW = 1 << 6,
  FLAG_NEGATIVE = 1 << 7
};

enum {
  CARRY_SHIFT = 0,
  ZERO_SHIFT = 1,
  INT_DISABLE_SHIFT = 2,
  DECIMAL_SHIFT = 3,
  BREAK_SHIFT = 4,
  UNUSED_SHIFT = 5,
  OVERFLOW_SHIFT = 6,
  NEGATIVE_SHIFT = 7
};

typedef struct cpu_s {
  uint8_t a; // Accumulator register
  uint8_t x; // X, Y registers, used for indexing
  uint8_t y;
  uint16_t pc; // Programme counter
  uint8_t sp;  // Stack pointer
  uint8_t flags;
  uint16_t to_update_flags; /* 0: No, 1: Yes, 2: After next instruction */
  uint8_t new_int_disable_flag;
  uint8_t to_oamdma;
  uint16_t cycles;
  uint8_t to_nmi;
  uint8_t in_nmi;
  uint8_t to_irq;
} cpu_s;

static inline uint16_t zero_page(cpu_s *cpu);
static inline uint16_t zero_page_x(cpu_s *cpu);
static inline uint16_t zero_page_y(cpu_s *cpu);
static inline uint16_t relative(cpu_s *cpu); // used only by branch instructions
static inline uint16_t absolute(cpu_s *cpu);
static inline uint16_t absolute_x(cpu_s *cpu);
static inline uint16_t absolute_y(cpu_s *cpu);
static inline uint16_t absolute_indirect(cpu_s *cpu); // used only by JMP
static inline uint16_t indirect_indexed(cpu_s *cpu);
static inline uint16_t indexed_indirect(cpu_s *cpu);
static inline uint16_t immediate(cpu_s *cpu);
static inline uint16_t absolute_x_extra_cycle(cpu_s *cpu);
static inline uint16_t absolute_y_extra_cycle(cpu_s *cpu);
static inline uint16_t indirect_indexed_extra_cycle(cpu_s *cpu);
static inline uint16_t implied(cpu_s *cpu);

/* This lets me index the addressing mode handlers by the addressing mode
 * enum without having to worry about ordering
 */
#define ADDRESS_MODE_LIST                                                      \
  X(IMP, implied), X(REL, relative), X(IMM, immediate), X(ABS, absolute),      \
      X(ABS_X, absolute_x), X(ABS_Y, absolute_y),                              \
      X(ABS_IND, absolute_indirect), X(IND_X, indexed_indirect),               \
      X(IND_Y, indirect_indexed), X(ZP, zero_page), X(ZP_X, zero_page_x),      \
      X(ZP_Y, zero_page_y), X(ABS_X_EC, absolute_x_extra_cycle),               \
      X(ABS_Y_EC, absolute_y_extra_cycle),                                     \
      X(IND_Y_EC, indirect_indexed_extra_cycle)

#define X(id, x) id
typedef enum { ADDRESS_MODE_LIST } addr_mode_e;
#undef X

#define X(x, handler) handler
static uint16_t (*addr_mode_handlers[])(cpu_s *) = {ADDRESS_MODE_LIST};
#undef X

#undef ADDRESS_MODE_LIST

/* Helper functions */
static inline void stack_push(cpu_s *cpu, uint8_t val);
static inline uint8_t stack_pop(cpu_s *cpu);
static inline uint8_t fetch8(cpu_s *cpu, uint16_t addr);
static inline uint16_t fetch16(cpu_s *cpu, uint16_t addr);
static inline void write8(cpu_s *cpu, uint16_t addr, uint8_t val);

/* Instructions */
static inline void ORA(cpu_s *cpu, addr_mode_e mode);
static inline void AND(cpu_s *cpu, addr_mode_e mode);
static inline void EOR(cpu_s *cpu, addr_mode_e mode);
static inline void ADC(cpu_s *cpu, addr_mode_e mode);
static inline void STA(cpu_s *cpu, addr_mode_e mode);
static inline void LDA(cpu_s *cpu, addr_mode_e mode);
static inline void CMP(cpu_s *cpu, addr_mode_e mode);
static inline void SBC(cpu_s *cpu, addr_mode_e mode);
static inline void ASL(cpu_s *cpu, addr_mode_e mode);
static inline void ROL(cpu_s *cpu, addr_mode_e mode);
static inline void LSR(cpu_s *cpu, addr_mode_e mode);
static inline void ROR(cpu_s *cpu, addr_mode_e mode);
static inline void DEC(cpu_s *cpu, addr_mode_e mode);
static inline void INC(cpu_s *cpu, addr_mode_e mode);
static inline void STX(cpu_s *cpu, addr_mode_e mode);
static inline void LDX(cpu_s *cpu, addr_mode_e mode);
static inline void TXA(cpu_s *cpu, addr_mode_e mode);
static inline void TXS(cpu_s *cpu, addr_mode_e mode);
static inline void TAX(cpu_s *cpu, addr_mode_e mode);
static inline void TSX(cpu_s *cpu, addr_mode_e mode);
static inline void DEX(cpu_s *cpu, addr_mode_e mode);
static inline void NOP(cpu_s *cpu, addr_mode_e mode);
static inline void BRK(cpu_s *cpu, addr_mode_e mode);
static inline void PHP(cpu_s *cpu, addr_mode_e mode);
static inline void BPL(cpu_s *cpu, addr_mode_e mode);
static inline void CLC(cpu_s *cpu, addr_mode_e mode);
static inline void JSR(cpu_s *cpu, addr_mode_e mode);
static inline void BIT(cpu_s *cpu, addr_mode_e mode);
static inline void BMI(cpu_s *cpu, addr_mode_e mode);
static inline void SEC(cpu_s *cpu, addr_mode_e mode);
static inline void RTI(cpu_s *cpu, addr_mode_e mode);
static inline void PHA(cpu_s *cpu, addr_mode_e mode);
static inline void JMP(cpu_s *cpu, addr_mode_e mode);
static inline void BVC(cpu_s *cpu, addr_mode_e mode);
static inline void CLI(cpu_s *cpu, addr_mode_e mode);
static inline void RTS(cpu_s *cpu, addr_mode_e mode);
static inline void PLA(cpu_s *cpu, addr_mode_e mode);
static inline void BVS(cpu_s *cpu, addr_mode_e mode);
static inline void SEI(cpu_s *cpu, addr_mode_e mode);
static inline void STY(cpu_s *cpu, addr_mode_e mode);
static inline void DEY(cpu_s *cpu, addr_mode_e mode);
static inline void BCC(cpu_s *cpu, addr_mode_e mode);
static inline void TYA(cpu_s *cpu, addr_mode_e mode);
static inline void LDY(cpu_s *cpu, addr_mode_e mode);
static inline void TAY(cpu_s *cpu, addr_mode_e mode);
static inline void BCS(cpu_s *cpu, addr_mode_e mode);
static inline void CLV(cpu_s *cpu, addr_mode_e mode);
static inline void CPY(cpu_s *cpu, addr_mode_e mode);
static inline void INY(cpu_s *cpu, addr_mode_e mode);
static inline void BNE(cpu_s *cpu, addr_mode_e mode);
static inline void CLD(cpu_s *cpu, addr_mode_e mode);
static inline void CPX(cpu_s *cpu, addr_mode_e mode);
static inline void INX(cpu_s *cpu, addr_mode_e mode);
static inline void BEQ(cpu_s *cpu, addr_mode_e mode);
static inline void SED(cpu_s *cpu, addr_mode_e mode);
static inline void PLP(cpu_s *cpu, addr_mode_e mode);

/* illegal instructions */
static inline void LAX(cpu_s *cpu, addr_mode_e mode);
static inline void SAX(cpu_s *cpu, addr_mode_e mode);
static inline void DCP(cpu_s *cpu, addr_mode_e mode);
static inline void ISB(cpu_s *cpu, addr_mode_e mode);
static inline void SLO(cpu_s *cpu, addr_mode_e mode);
static inline void RLA(cpu_s *cpu, addr_mode_e mode);
static inline void SRE(cpu_s *cpu, addr_mode_e mode);
static inline void RRA(cpu_s *cpu, addr_mode_e mode);

static inline void update_flags(cpu_s *cpu);
static void (*on_cpu_state_update)(const cpu_state_s *, void *) = NULL;
static void *on_cpu_state_update_data = NULL;

static void (*log_error)(const char *, ...) = NULL;
static cpu_state_s cpu_state;

// uint16_t (*addr_mode_handlers[])(void)


/* This is BRK but no pc increment, B flag not pushed, and goes to NMI handler
 * 0xFFFA */
static void NMI(cpu_s *cpu) {
  cpu->to_nmi = 0;
  cpu->in_nmi = 1;

  fetch8(cpu, cpu->pc); /* fetch next opcode, throw away and suppress pc increment */
  SET_INSTRUCTION(NMI, 0, IMP);
  stack_push(cpu, (cpu->pc & 0xFF00) >> 8);
  stack_push(cpu, cpu->pc & 0xFF);
  stack_push(cpu, (cpu->flags & ~MASK_NVDIZC) | FLAG_UNUSED);
  cpu->flags = (cpu->flags & MASK_I) | FLAG_INT_DISABLE;
  cpu->pc = fetch16(cpu, 0xFFFA);
  
  cpu->in_nmi = 0;
}
//static void IRQ(cpu_s *cpu) {}

static void update_cpu_state(cpu_s *cpu);

static void update_cpu_state(cpu_s *cpu) {
  cpu_state.a = cpu->a;
  cpu_state.x = cpu->x;
  cpu_state.y = cpu->y;
  cpu_state.p = cpu->flags;
  cpu_state.sp = cpu->sp;
  cpu_state.cycles = cpu->cycles;
  cpu_state.pc = cpu->pc;
}

/* =============================================================================
 *                              Extern Fucntions
 * =============================================================================
 */

void cpu_register_state_callback(void (*cpu_state_cb)(const cpu_state_s *, void *),
                                 void *cpu_cb_data) {
  on_cpu_state_update = cpu_state_cb;
  on_cpu_state_update_data = cpu_cb_data;
}

void cpu_unregister_state_callback(void) {
  on_cpu_state_update = NULL;
  on_cpu_state_update_data = NULL;
}

void cpu_register_error_callback(void (*log_error_cb)(const char *, ...)) {
  log_error = log_error_cb;
}

void cpu_unregister_error_callback(void) { log_error = NULL; }

void cpu_init_harte_test_case(cpu_s *cpu, cpu_state_s *test_case) {
  memset(cpu, 0, sizeof(cpu_s));
  cpu->pc = test_case->pc;
  cpu->a = test_case->a;
  cpu->x = test_case->x;
  cpu->y = test_case->y;
  cpu->sp = test_case->sp;
  cpu->flags = test_case->p;
  update_cpu_state(cpu);
}

int cpu_init(cpu_s **p_cpu, uint8_t nestest) {
  if (on_cpu_state_update == NULL || log_error == NULL) {
    return -E_NO_CALLBACK;
  }

  if (((*p_cpu) = malloc(sizeof(cpu_s))) == NULL) {
    return -E_MALLOC;
  }

  cpu_s *cpu = *p_cpu;
  cpu->a = 0;
  cpu->x = 0;
  cpu->y = 0;
  cpu->sp = 0xFD;
  cpu->flags = FLAG_UNUSED | FLAG_INT_DISABLE;
  cpu->to_oamdma = 0;
  cpu->to_update_flags = 0;
  cpu->to_irq = 0;
  cpu->to_nmi = 0;
  cpu->in_nmi = 0;

  if (!nestest) {
    cpu->cycles = 0;
    cpu->pc = fetch16(cpu, 0xFFFC);

    // The two lines that broke everything:
    /*=================================================*/
    /* cpu->pc = memory_init_cpu_pc();                 */
    /* JMP(cpu, ABS);                                  */
    /*=================================================*/
  } else {
    cpu->pc = 0xC000;
    cpu->cycles = 7;
  }
  SET_INSTRUCTION(JMP, 0x40 + JMP_OFFSET, ABS);
  update_cpu_state(cpu);
  return E_NO_ERROR;
}

void cpu_destroy(cpu_s *cpu) { free(cpu); }

/* Fetches next opcode and executes next instruction.
 * TODO: Proper interrupt handling
 */
int cpu_exec(cpu_s *cpu, char *e_context) {
  if (on_cpu_state_update == NULL || log_error == NULL) {
    return -E_NO_CALLBACK;
  }
#ifndef DOING_HARTE_TESTS
  update_cpu_state(cpu);
  update_flags(cpu);
#endif
  if (cpu->to_nmi) {
    NMI(cpu);
  }
  /*
  else if (cpu->to_irq) {
    IRQ(cpu);
  }
  */
  else {
    uint8_t opc = fetch8(cpu, cpu->pc++); /* 1 cycle */
    switch (opc) {

      /* Grouped by "sections" in opcode table: */

      /* Mostly ALU instructions */
      IS_ALU_OPC(ORA)
      IS_ALU_OPC(AND)
      IS_ALU_OPC(EOR)
      IS_ALU_OPC(ADC)
      IS_ALU_OPC(LDA)
      IS_ALU_OPC(CMP)
      IS_ALU_OPC(SBC)
      IS_STA()

      /* Mostly RMW (read-modify-write) instructions */
      IS_RMW_OPC(ASL)
      IS_RMW_OPC(ROL)
      IS_RMW_OPC(LSR)
      IS_RMW_OPC(ROR)
      IS_INC_DEC(DEC)
      IS_INC_DEC(INC)
      IS_STX()
      IS_LDX()
      IS_OPC(TXA)
      IS_OPC(TXS)
      IS_OPC(TAX)
      IS_OPC(TSX)
      IS_OPC(DEX)
      IS_OPC(NOP)

      /* Control instructions, and unique instructions */
      IS_BRANCH_OPC()
      IS_OPC(BRK)
      IS_OPC(PHP)
      IS_OPC(CLC)
      IS_OPC(PLP)
      IS_OPC(SEC)
      IS_OPC(RTI)
      IS_OPC(PHA)
      IS_OPC(CLI)
      IS_OPC(RTS)
      IS_OPC(PLA)
      IS_OPC(SEI)
      IS_OPC(DEY)
      IS_OPC(TYA)
      IS_OPC(TAY)
      IS_OPC(CLV)
      IS_OPC(INY)
      IS_OPC(CLD)
      IS_OPC(INX)
      IS_OPC(SED)
      IS_JMP()
      IS_JSR()
      IS_BIT()
      IS_LDY()
      IS_STY()
      IS_CPX_CPY(CPX)
      IS_CPX_CPY(CPY)

      /* illegal opcodes */
      IS_ILLEGAL_NOP()
      IS_LAX()
      IS_SAX()
      IS_ILLEGAL_SBC()
      IS_ILLEGAL_RMW_OPC(DCP)
      IS_ILLEGAL_RMW_OPC(ISB)
      IS_ILLEGAL_RMW_OPC(SLO)
      IS_ILLEGAL_RMW_OPC(RLA)
      IS_ILLEGAL_RMW_OPC(SRE)
      IS_ILLEGAL_RMW_OPC(RRA)
	
    default:
      sprintf(e_context, "%02x", opc);
      return -E_ILLEGAL_OPC;
    }
  }
#ifdef DOING_HARTE_TESTS
  update_flags(cpu);
  update_cpu_state(cpu);
#endif
  on_cpu_state_update(&cpu_state, on_cpu_state_update_data);
  return 1;
}

/*==============================================================================
 *                                HELPER FUNCTIONS
 *==============================================================================
 */

/* update interupt disable flag which has a delay */
static inline void update_flags(cpu_s *cpu) {

#ifdef DOING_HARTE_TESTS
  /* harte tests seem to update flags immediately, even though
   * nes wiki says that there is a one instruction delay to
   * update the interrupt disable flag.
   */
  if (cpu->to_update_flags > 0) {
    cpu->flags = (cpu->flags & MASK_I) | cpu->new_int_disable_flag;
    cpu->to_update_flags = 0;
  }
#else
  if (cpu->to_update_flags == 2) {
    cpu->to_update_flags--;
  } else if (cpu->to_update_flags == 1) {
    cpu->flags = (cpu->flags & MASK_I) | cpu->new_int_disable_flag;
    cpu->to_update_flags = 0;
  }
#endif
}

static inline uint8_t fetch8(cpu_s *cpu, uint16_t addr) {
  cpu->cycles++;
  return memory_fetch(addr, &(cpu->to_nmi));
}

static inline uint16_t fetch16(cpu_s *cpu, uint16_t addr) {
  uint8_t val_low = fetch8(cpu, addr);
  uint8_t val_high = fetch8(cpu, addr + 1);
  return (val_low | val_high << 8);
}

static inline void write8(cpu_s *cpu, uint16_t addr, uint8_t val) {
  memory_write(addr, val, &(cpu->to_oamdma), &(cpu->to_nmi));
  cpu->cycles++;
  /*
  if (cpu->to_oamdma) {
    memory_do_oamdma(val, &(cpu->cycles), &(cpu->to_nmi));
    cpu->to_oamdma = 0;
  }
  */
}

static inline void stack_push(cpu_s *cpu, uint8_t val) {
  write8(cpu, cpu->sp | (1 << 8), val);
  cpu->sp--;
}

static inline uint8_t stack_pop(cpu_s *cpu) {
  cpu->sp++;
  return fetch8(cpu, cpu->sp | (1 << 8));
}

/* =============================================================================
 *                              ADDRESSING MODE HANDLERS
 * =============================================================================
 */

/* For addressing modes absolute_x, absolute_y, and indirect_indexed which
 * add another cycle if a page is crossed, there is a variant which always adds
 * the extra cycle. These are for instructions which always use the extra cycle
 * whether there is a page cross or not.
 *
 * It feels better to do this than having to check whether
 * the extra cycle was already added within the instruction.
 */

/* NB: Cycle and PC counts below exclude reading opcode */

static inline uint16_t implied(cpu_s *cpu) { return cpu->pc; }

/* Cycles = 1
 * PC += 1
 */
static inline uint16_t zero_page(cpu_s *cpu) {
  return (uint16_t)fetch8(cpu, cpu->pc++);
}

/* Cycles = 2
 * PC += 1
 */
static inline uint16_t zero_page_x(cpu_s *cpu) {
  uint8_t addr = fetch8(cpu, cpu->pc++);
  fetch8(cpu, addr);
  addr += cpu->x;
  return (uint16_t)addr;
}

/* Cycles = 2
 * PC += 1
 */
static inline uint16_t zero_page_y(cpu_s *cpu) {
  uint8_t addr = fetch8(cpu, cpu->pc++);
  fetch8(cpu, addr);
  addr += cpu->y;
  return (uint16_t)addr;
}

/* Cycles = 2
 * PC += 2
 */
static inline uint16_t absolute(cpu_s *cpu) {
  uint16_t addr = fetch16(cpu, cpu->pc);
  cpu->pc += 2;
  return addr;
}

/* Cycles = 2 (+1)
 * PC += 2
 */
static inline uint16_t absolute_x(cpu_s *cpu) {
  uint8_t a_low = fetch8(cpu, cpu->pc++);
  uint8_t a_high = fetch8(cpu, cpu->pc++);
  if (((a_low + cpu->x) & 0xFF) < cpu->x) { /* i.e. we lost the high bit */
    fetch8(cpu, (a_high << 8) | ((a_low + cpu->x) & 0xFF));
  }
  return (uint16_t)(a_low | (a_high << 8)) + cpu->x;
}

/* Cycles = 3
 * PC += 2
 */
static inline uint16_t absolute_x_extra_cycle(cpu_s *cpu) {
  uint16_t addr = absolute(cpu);
  fetch8(cpu, (addr & 0xFF00) | ((addr + cpu->x) & 0xFF));
  return addr + cpu->x;
}

/* Cycles = 2 (+1)
 * PC += 2
 */
static inline uint16_t absolute_y(cpu_s *cpu) {
  uint8_t a_low = fetch8(cpu, cpu->pc++);
  uint8_t a_high = fetch8(cpu, cpu->pc++);
  if (((a_low + cpu->y) & 0xFF) < cpu->y) { /* i.e. we lost the high bit */
    fetch8(cpu, (a_high << 8) | ((a_low + cpu->y) & 0xFF));
  }
  return (uint16_t)(a_low | (a_high << 8)) + cpu->y;
}

/* Cycles = 3
 * PC += 2
 */
static inline uint16_t absolute_y_extra_cycle(cpu_s *cpu) {
  uint16_t addr = absolute(cpu);
  fetch8(cpu, (addr & 0xFF00) | ((addr + cpu->y) & 0xFF));
  return addr + cpu->y;
}

/* AKA IND_X
 *
 * Cycles: 4
 * PC += 1
 */
static inline uint16_t indexed_indirect(cpu_s *cpu) {
  uint16_t zp_addr = zero_page(cpu);
  /* this conflicts with another document that says that the dummy read
   * is at PC + 1, but the Harte tests say otherwise. Plus said document is
   * about 65x02 which is maybe slightly different even though I read that
   * cycle stuff is supposed to be the same
   */
  fetch8(cpu, zp_addr);
  uint16_t idx_low = (zp_addr + cpu->x) & 0xFF;
  uint16_t idx_high = (idx_low + 1) & 0xFF;
  return fetch8(cpu, idx_low) | (fetch8(cpu, idx_high) << 8);
}

/* AKA IND_Y
 *
 * Cycles: 3 (+1)
 * PC += 1
 */
static inline uint16_t indirect_indexed(cpu_s *cpu) {
  uint8_t idx = fetch8(cpu, cpu->pc++);
  uint8_t a_low = fetch8(cpu, idx++);
  uint8_t a_high = fetch8(cpu, idx);
  uint16_t addr = (a_low | (a_high << 8));
  if (((a_low + cpu->y) & 0xFF) < cpu->y) {
    fetch8(cpu, (a_high << 8) | ((a_low + cpu->y) & 0xFF));
  }
  return addr + cpu->y;
}

/* AKA IND_Y_EC
 *
 * Cycles: 4
 * PC += 1
 */
static inline uint16_t indirect_indexed_extra_cycle(cpu_s *cpu) {
  uint16_t idx_low = zero_page(cpu);
  uint16_t idx_high = (idx_low + 1) & 0xFF;
  uint16_t addr = (fetch8(cpu, idx_low) | fetch8(cpu, idx_high) << 8);
  fetch8(cpu, (addr & 0xFF00) | ((addr + cpu->y) & 0xFF));
  return addr + cpu->y;
}

static inline uint16_t absolute_indirect(cpu_s *cpu) {
  uint8_t idx_low = fetch8(cpu, cpu->pc++);
  uint8_t idx_high = fetch8(cpu, cpu->pc++);
  uint8_t addr_low = fetch8(cpu, idx_low | (idx_high << 8));
  idx_low++;
  uint8_t addr_high = fetch8(cpu, idx_low | (idx_high << 8));
  return (addr_low | (addr_high << 8));
}

static inline uint16_t relative(cpu_s *cpu) { return cpu->pc++; }

/* Cycles: 0
 * PC: += 1
 */
static inline uint16_t immediate(cpu_s *cpu) { return cpu->pc++; }

/* =============================================================================
 *                                INSTRUCTIONS
 * =============================================================================
 */

#define OVERFLOW(n, m, res) (!((n ^ m) & 0x80) && ((m ^ res) & 0x80))
#define NEGATIVE(res) ((res & 0x80) != 0)

/* NOP (No Operation)
 *
 * Addressing modes (bytes | cycles):
 * IMP: 1 | 2
 */
static void NOP(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */
}

/* -----------------------------------------------------------------------------*/
/* Arithmetic: ADC, SBC, INC, DEC, INX, DEX, INY, DEY */

/* ADC (Add with Carry)
 *
 * Flags: N+ V+ 1 B D I Z+ C+
 *
 * Addressing modes:
 * IMM: 2 | 2
 * ZP: 2 | 3
 * ZP_X: 2 | 4
 * ABS: 3 | 4
 * ABS_X: 3 | 4 (+1)
 * ABS_Y: 3 | 4 (+1)
 * IND_X: 2 | 6
 * IND_Y: 2 | 5 (+1)
 */
static void ADC(cpu_s *cpu, addr_mode_e mode) {
  uint8_t m = fetch8(cpu, addr_mode_handlers[mode](cpu));
  uint16_t oper = m + (cpu->flags & FLAG_CARRY);
  uint16_t res = cpu->a + oper;
  uint8_t trunc_res = res & 0xFF;
  cpu->flags = (cpu->flags & MASK_NVZC) | ((res != trunc_res) << CARRY_SHIFT) |
               (!trunc_res << ZERO_SHIFT) |
               (OVERFLOW(cpu->a, m, res) << OVERFLOW_SHIFT) |
               (NEGATIVE(trunc_res) << NEGATIVE_SHIFT);
  cpu->a = trunc_res;
}

/* SBC (Subtract with Carry)
 *
 * See ADC
 */
static void SBC(cpu_s *cpu, addr_mode_e mode) {
  uint8_t m = fetch8(cpu, addr_mode_handlers[mode](cpu));
  uint16_t res = cpu->a + (uint8_t)~m + (cpu->flags & FLAG_CARRY);
  uint8_t trunc_res = res & 0xFF;
  cpu->flags = (cpu->flags & MASK_NVZC) | ((res != trunc_res) << CARRY_SHIFT) |
               (!trunc_res << ZERO_SHIFT) |
               (OVERFLOW(cpu->a, ~m, res) << OVERFLOW_SHIFT) |
               (NEGATIVE(trunc_res) << NEGATIVE_SHIFT);
  cpu->a = trunc_res;
}

/* DEC (Decrement Memory)
 *
 * Flags: N+ V 1 B D I Z+ C
 *
 * Addressing modes:
 * ZP: 2 | 5
 * ZP_X: 2 | 6
 * ABS: 3 | 6
 * ABS_X: 3 | 7
 */
static void DEC(cpu_s *cpu, addr_mode_e mode) {
  uint16_t addr = addr_mode_handlers[mode](cpu);
  uint8_t val = fetch8(cpu, addr);
  write8(cpu, addr, val); /* dummy write */
  uint8_t res = val - 1;
  write8(cpu, addr, res);
  cpu->flags = (cpu->flags & MASK_NZ) | (!res << ZERO_SHIFT) |
               (NEGATIVE(res) << NEGATIVE_SHIFT);
}

/* DEX (Decrement X)
 *
 * Flags: See DEC
 *
 * Addressing modes:
 * IMP: 1 | 2
 */
static void DEX(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */
  cpu->x--;
  cpu->flags = (cpu->flags & MASK_NZ) | (!cpu->x << ZERO_SHIFT) |
               (NEGATIVE(cpu->x) << NEGATIVE_SHIFT);
}

/* DEY (Decrement Y)
 *
 * See DEX
 */
static void DEY(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */
  cpu->y--;
  cpu->flags = (cpu->flags & MASK_NZ) | (!cpu->y << ZERO_SHIFT) |
               (NEGATIVE(cpu->y) << NEGATIVE_SHIFT);
}

/* INC (Increment Memory)
 *
 * See DEC
 */
static void INC(cpu_s *cpu, addr_mode_e mode) {
  uint16_t addr = addr_mode_handlers[mode](cpu);
  uint8_t val = fetch8(cpu, addr);
  write8(cpu, addr, val); /* dummy write */
  uint8_t res = val + 1;
  write8(cpu, addr, res);
  cpu->flags = (cpu->flags & MASK_NZ) | (!res << ZERO_SHIFT) |
               (NEGATIVE(res) << NEGATIVE_SHIFT);
}

/* INX (Increment X)
 *
 * See DEX
 */
static void INX(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */
  cpu->x++;
  cpu->flags = (cpu->flags & MASK_NZ) | (!cpu->x << ZERO_SHIFT) |
               (NEGATIVE(cpu->x) << NEGATIVE_SHIFT);
}

/* INY (Increment Y)
 *
 * See DEX
 */
static void INY(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */
  cpu->y++;
  cpu->flags = (cpu->flags & MASK_NZ) | (!cpu->y << ZERO_SHIFT) |
               (NEGATIVE(cpu->y) << NEGATIVE_SHIFT);
}

/*-----------------------------------------------------------------------------*/
/*                               BITWISE INSTRUCTIONS */
// AND, ORA, EOR, BIT

/* AND (Bitwise AND)
 *
 * Flags: N+ V 1 B D I Z+ C
 *
 * Addressing modes:
 * IMM: 2 | 2
 * ZP: 2 | 3
 * ZP_X: 2 | 4
 * ABS: 3 | 4
 * ABS_X: 3 | 4 (+1)
 * ABS_Y: 3 | 4 (+1)
 * IND_X: 2 | 6
 * IND_Y: 2 | 5 (+1)
 */
static void AND(cpu_s *cpu, addr_mode_e mode) {
  cpu->a &= fetch8(cpu, addr_mode_handlers[mode](cpu));
  cpu->flags = (cpu->flags & MASK_NZ) | (NEGATIVE(cpu->a) << NEGATIVE_SHIFT) |
               (!cpu->a << ZERO_SHIFT);
}

/* ORA (Bitwise OR)
 *
 * See AND
 */
static void ORA(cpu_s *cpu, addr_mode_e mode) {
  cpu->a |= fetch8(cpu, addr_mode_handlers[mode](cpu));
  cpu->flags = (cpu->flags & MASK_NZ) | (NEGATIVE(cpu->a) << NEGATIVE_SHIFT) |
               (!cpu->a << ZERO_SHIFT);
}

/* EOR (Bitwise Exclusive OR)
 *
 * See AND
 */
static void EOR(cpu_s *cpu, addr_mode_e mode) {
  cpu->a ^= fetch8(cpu, addr_mode_handlers[mode](cpu));
  cpu->flags = (cpu->flags & MASK_NZ) | (NEGATIVE(cpu->a) << NEGATIVE_SHIFT) |
               (!cpu->a << ZERO_SHIFT);
}

/* BIT: Bit Test
 *
 * Flags: N+ V+ 1 B D I Z+ C
 *
 * Addressing modes:
 * ZP: 2 | 3
 * ABS: 3 | 4
 */
static void BIT(cpu_s *cpu, addr_mode_e mode) {
  uint8_t oper = fetch8(cpu, addr_mode_handlers[mode](cpu));
  cpu->flags = (cpu->flags & MASK_NVZ) | (oper & FLAG_NEGATIVE) |
               (oper & FLAG_OVERFLOW) | (!(oper & cpu->a) << ZERO_SHIFT);
}

/*-----------------------------------------------------------------------------*/
/*                              SHIFT INSTRUCTIONS */
// ASL, LSR, ROL, ROR

/* ASL (Arithmetic Shift Left)
 *
 * Flags: N+ V 1 B D I Z+ C+
 *
 * Addressing modes:
 * IMP: 1 | 2
 * ZP: 2 | 5
 * ZP_X | 2 | 6
 * ABS: 3 | 6
 * ABS_X 3 | 7
 */
static void ASL(cpu_s *cpu, addr_mode_e mode) {
  uint8_t oper, res;
  uint16_t addr = addr_mode_handlers[mode](cpu);
  // dummy_fetch(cpu);
  if (mode == IMP) {
    fetch8(cpu, addr); /* dummy fetch pc + 1 */
    oper = cpu->a;
    res = cpu->a << 1;
    cpu->a = res;
  } else {
    oper = fetch8(cpu, addr);
    write8(cpu, addr, oper); /* dummy write */
    res = oper << 1;
    write8(cpu, addr, res);
  }
  cpu->flags = (cpu->flags & MASK_NZC) | (((oper & 0x80) >> 7) << CARRY_SHIFT) |
               (!res << ZERO_SHIFT) | (NEGATIVE(res) << NEGATIVE_SHIFT);
}

/* LSR (Logical Shift Right)
 *
 * See ASL
 */
static void LSR(cpu_s *cpu, addr_mode_e mode) {
  uint8_t oper, res;
  uint16_t addr = addr_mode_handlers[mode](cpu);
  if (mode == IMP) {
    fetch8(cpu, addr); /* dummy fetch pc + 1 */
    oper = cpu->a;
    res = cpu->a >> 1;
    cpu->a = res;
  } else {
    oper = fetch8(cpu, addr);
    write8(cpu, addr, oper); /* dummy write */
    res = oper >> 1;
    write8(cpu, addr, res);
  }
  cpu->flags = (cpu->flags & MASK_NZC) | ((oper & 0x1) << CARRY_SHIFT) |
               (!res << ZERO_SHIFT) | (NEGATIVE(res) << NEGATIVE_SHIFT);
}

/* ROR (Rotate Right)
 *
 * See ASL
 */
static void ROR(cpu_s *cpu, addr_mode_e mode) {
  uint8_t oper, res;
  uint16_t addr = addr_mode_handlers[mode](cpu);
  if (mode == IMP) {   // cycles?
    fetch8(cpu, addr); /* dummy fetch pc + 1 */
    oper = cpu->a;
    res = (cpu->a >> 1) | ((cpu->flags & FLAG_CARRY) << (7 - CARRY_SHIFT));
    cpu->a = res;
  } else {
    oper = fetch8(cpu, addr);
    write8(cpu, addr, oper); /* dummy write */
    res = (oper >> 1) | ((cpu->flags & FLAG_CARRY) << (7 - CARRY_SHIFT));
    write8(cpu, addr, res);
  }
  cpu->flags = (cpu->flags & MASK_NZC) | ((oper & 0x1) << CARRY_SHIFT) |
               (!res << ZERO_SHIFT) | (NEGATIVE(res) << NEGATIVE_SHIFT);
}

/* ROL (Rotate Left)
 *
 * See ASL
 */
static void ROL(cpu_s *cpu, addr_mode_e mode) {
  uint8_t oper, res;
  uint16_t addr = addr_mode_handlers[mode](cpu);
  if (mode == IMP) {   // cycles?
    fetch8(cpu, addr); /* dummy fetch pc + 1 */
    oper = cpu->a;
    res = (cpu->a << 1) | ((cpu->flags & FLAG_CARRY) >> CARRY_SHIFT);
    cpu->a = res;
  } else {
    oper = fetch8(cpu, addr);
    write8(cpu, addr, oper); /* dummy write */
    res = (oper << 1) | ((cpu->flags & FLAG_CARRY) >> CARRY_SHIFT);
    write8(cpu, addr, res);
  }
  cpu->flags = (cpu->flags & MASK_NZC) | (((oper & 0x80) >> 7) << CARRY_SHIFT) |
               (!res << ZERO_SHIFT) | (NEGATIVE(res) << NEGATIVE_SHIFT);
}

/*-----------------------------------------------------------------------------*/
/* Branch instructinos: BCC, BCS, BEQ, BNE, BPL, BMI, BVC, BVS */

/* Addressing modes:
 * REL: 2 | 2 (+1 (+1))
 */

static inline void branch_flag_clear(cpu_s *cpu, addr_mode_e mode,
                                     uint8_t flag) {
  uint8_t offset = fetch8(cpu, addr_mode_handlers[mode](cpu));
  if (!(cpu->flags & flag)) {
    fetch8(cpu, cpu->pc); /* dummy fetch pc + 2 */
    uint16_t page = cpu->pc & 0xFF00;
    cpu->pc += (int8_t)offset;

    if (page != (cpu->pc & 0xFF00)) {
      fetch8(cpu,
             page | (cpu->pc &
                     0xFF)); /* dummy fetch pc + 2 + offset (before cross) */
    }
  }
}

static inline void branch_flag_set(cpu_s *cpu, addr_mode_e mode, uint8_t flag) {
  uint8_t offset = fetch8(cpu, addr_mode_handlers[mode](cpu));
  if (cpu->flags & flag) {
    fetch8(cpu, cpu->pc); /* dummy fetch pc + 2 */
    uint16_t page = cpu->pc & 0xFF00;
    cpu->pc += (int8_t)offset;
    if (page != (cpu->pc & 0xFF00)) {
      fetch8(cpu,
             page | (cpu->pc &
                     0xFF)); /* dummy fetch pc + 2 + offset (before cross) */
    }
  }
}

static void BCC(cpu_s *cpu, addr_mode_e mode) {
  branch_flag_clear(cpu, mode, FLAG_CARRY);
}

static void BNE(cpu_s *cpu, addr_mode_e mode) {
  branch_flag_clear(cpu, mode, FLAG_ZERO);
}

static void BPL(cpu_s *cpu, addr_mode_e mode) {
  branch_flag_clear(cpu, mode, FLAG_NEGATIVE);
}

static void BVS(cpu_s *cpu, addr_mode_e mode) {
  branch_flag_set(cpu, mode, FLAG_OVERFLOW);
}

static void BCS(cpu_s *cpu, addr_mode_e mode) {
  branch_flag_set(cpu, mode, FLAG_CARRY);
}

static void BEQ(cpu_s *cpu, addr_mode_e mode) {
  branch_flag_set(cpu, mode, FLAG_ZERO);
}

static void BMI(cpu_s *cpu, addr_mode_e mode) {
  branch_flag_set(cpu, mode, FLAG_NEGATIVE);
}

static void BVC(cpu_s *cpu, addr_mode_e mode) {
  branch_flag_clear(cpu, mode, FLAG_OVERFLOW);
}

/* -----------------------------------------------------------------------------*/
/* flag instructions: CLC, SEC, CLI, SEI, CLD, SED, CLV */

/* Addressing modes:
 * IMP: 1 | 2
 */
#define CLEAR_FLAG_INSTRUCTION(flag)                                           \
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */         \
  cpu->flags &= ~(flag);

static void CLC(cpu_s *cpu, addr_mode_e mode) {
  CLEAR_FLAG_INSTRUCTION(FLAG_CARRY)
}

static void CLD(cpu_s *cpu, addr_mode_e mode) {
  CLEAR_FLAG_INSTRUCTION(FLAG_DECIMAL)
}

static void CLV(cpu_s *cpu, addr_mode_e mode) {
  CLEAR_FLAG_INSTRUCTION(FLAG_OVERFLOW)
}

static void CLI(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */
  cpu->to_update_flags = 2;
  cpu->new_int_disable_flag = 0;
}

#undef CLEAR_FLAG_INSTRUCTION

#define SET_FLAG_INSTRUCTION(flag)                                             \
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */         \
  cpu->flags |= (flag);

static void SEC(cpu_s *cpu, addr_mode_e mode) {
  SET_FLAG_INSTRUCTION(FLAG_CARRY)
}

static void SED(cpu_s *cpu, addr_mode_e mode) {
  SET_FLAG_INSTRUCTION(FLAG_DECIMAL)
}

static void SEI(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */
  cpu->to_update_flags = 2;
  cpu->new_int_disable_flag = FLAG_INT_DISABLE;
}

#undef SET_FLAG_INSTRUCTION

/*-----------------------------------------------------------------------------*/
/* comparison isntructions: CMP, CPX, CPY */

/* Flags: N+ V 1 B D I Z+ C+
 *
 * Addressing modes:
 * IMM: 2 | 2
 * ZP: 2 | 3
 * ZP_X: 2 | 4
 * ABS: 3 | 4
 * ABS_X: 3 | 4 (+1)
 * ABS_Y: 3 | 4 (+1)
 * IND_X: 2 | 6
 * IND_Y: 2 | 5 (+1)
 */

static inline void comparison_instruction(cpu_s *cpu, addr_mode_e mode,
                                          uint8_t reg) {
  uint8_t oper = fetch8(cpu, addr_mode_handlers[mode](cpu));
  uint8_t res = reg - oper;
  cpu->flags = (cpu->flags & MASK_NZC) | ((reg >= oper) << CARRY_SHIFT) |
               ((reg == oper) << ZERO_SHIFT) |
               (NEGATIVE(res) << NEGATIVE_SHIFT);
}

static void CMP(cpu_s *cpu, addr_mode_e mode) {
  comparison_instruction(cpu, mode, cpu->a);
}

static void CPX(cpu_s *cpu, addr_mode_e mode) {
  comparison_instruction(cpu, mode, cpu->x);
}

static void CPY(cpu_s *cpu, addr_mode_e mode) {
  comparison_instruction(cpu, mode, cpu->y);
}

/* ----------------------------------------------------------------------------*/
/* access instrucitons: LDA, STA, LDX, STX, LDY, STY */
/* Flags: N+ V 1 B D I Z+ C
 *
 * Addressing modes:
 * IMM: 2 | 2
 * ZP: 2 | 3
 * ZP_X: 2 | 4
 * ABS: 3 | 4
 * ABS_X: 3 | 4 (+1)
 * ABS_Y: 3 | 4 (+1)
 * IND_X: 2 | 6
 * IND_Y: 2 | 5 (+1)
 */

static inline void load_instruction(cpu_s *cpu, addr_mode_e mode,
                                    uint8_t *reg) {
  *reg = fetch8(cpu, addr_mode_handlers[mode](cpu));
  cpu->flags = (cpu->flags & MASK_NZ) | (!(*reg) << ZERO_SHIFT) |
               (NEGATIVE((*reg)) << NEGATIVE_SHIFT);
}

static void LDA(cpu_s *cpu, addr_mode_e mode) {
  load_instruction(cpu, mode, &cpu->a);
}

static void LDX(cpu_s *cpu, addr_mode_e mode) {
  load_instruction(cpu, mode, &cpu->x);
}

static void LDY(cpu_s *cpu, addr_mode_e mode) {
  load_instruction(cpu, mode, &cpu->y);
}

/* Addressing modes:
 * ZP: 2 | 3
 * ZP_X: 2 | 4
 * ZP_Y: 2 | 4
 * ABS: 3 | 4
 * ABS_X: 3 | 5
 * ABS_Y: 3 | 5
 * IND_X: 2 | 6
 * IND_Y: 2 | 6
 */
#define STORE_INSTRUCTION(reg)                                                 \
  write8(cpu, addr_mode_handlers[mode](cpu), (reg));

static void STA(cpu_s *cpu, addr_mode_e mode) { STORE_INSTRUCTION(cpu->a) }

static void STX(cpu_s *cpu, addr_mode_e mode) { STORE_INSTRUCTION(cpu->x) }

static void STY(cpu_s *cpu, addr_mode_e mode) { STORE_INSTRUCTION(cpu->y) }

/* ----------------------------------------------------------------------------*/
/* TRANSFER INSTRUCITONS: TAX, TXA, TAY, TYA, TSX, TXS */
/* Flags: N+ V 1 B D I Z+ C */
/* Addressing modes:
 * IMP: 1 | 2
 */

static inline void transfer_instruction(cpu_s *cpu, addr_mode_e mode,
                                        uint8_t *src, uint8_t *dest) {
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */
  *dest = *src;
  cpu->flags = (cpu->flags & MASK_NZ) | (NEGATIVE(*dest) << NEGATIVE_SHIFT) |
               (!(*dest) << ZERO_SHIFT);
}

static void TAX(cpu_s *cpu, addr_mode_e mode) {
  transfer_instruction(cpu, mode, &cpu->a, &cpu->x);
}

static void TXA(cpu_s *cpu, addr_mode_e mode) {
  transfer_instruction(cpu, mode, &cpu->x, &cpu->a);
}

static void TAY(cpu_s *cpu, addr_mode_e mode) {
  transfer_instruction(cpu, mode, &cpu->a, &cpu->y);
}

static void TYA(cpu_s *cpu, addr_mode_e mode) {
  transfer_instruction(cpu, mode, &cpu->y, &cpu->a);
}

static void TSX(cpu_s *cpu, addr_mode_e mode) {
  transfer_instruction(cpu, mode, &cpu->sp, &cpu->x);
}

static void TXS(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */
  cpu->sp = cpu->x;
}

/* ----------------------------------------------------------------------------*/
/* JUMP INSTRUCTIONS: JMP, JSR, RTS, BRK, RTI */
static void JMP(cpu_s *cpu, addr_mode_e mode) {
  cpu->pc = addr_mode_handlers[mode](cpu);
}

static void JSR(cpu_s *cpu, addr_mode_e mode) {
  /* Note that the addressing mode is hardcoded into this instruction */
  uint8_t pc_low = fetch8(cpu, cpu->pc);
  fetch8(cpu, cpu->sp | (1 << 8)); /* dummy fetch stack */
  stack_push(cpu, ((cpu->pc + 1) & 0xFF00) >> 8);
  stack_push(cpu, (cpu->pc + 1) & 0xFF);
  uint8_t pc_high = fetch8(cpu, ++cpu->pc);
  cpu->pc++;
  cpu->pc = (pc_low | (pc_high << 8));
}

static void RTS(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, cpu->pc); /* 2x dummy fetch pc + 1 */
  fetch8(cpu, (1 << 8) | cpu->sp);
  uint8_t pc_low = stack_pop(cpu);
  uint8_t pc_high = stack_pop(cpu);
  fetch8(cpu, pc_low | (pc_high << 8));
  cpu->pc = (pc_low | pc_high << 8) + 1;
}


static void BRK(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, cpu->pc); /* dummy fetch pc + 1 (pc again if NMI)*/
  stack_push(cpu, ((cpu->pc + 1) & 0xFF00) >> 8);
  stack_push(cpu, (cpu->pc + 1) & 0xFF);
  stack_push(cpu, (cpu->flags & ~MASK_NVDIZC) | FLAG_BREAK | FLAG_UNUSED);
  cpu->flags = (cpu->flags & MASK_I) | FLAG_INT_DISABLE;
  cpu->pc = fetch16(cpu, 0xFFFE);
}

static void RTI(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, cpu->pc);            /* 2x dummy fetch pc + 1 */
  fetch8(cpu, (1 << 8) | cpu->sp); /* dummy fetch stack ? */
  cpu->flags = (cpu->flags & MASK_NVDIZC) | stack_pop(cpu);
/* harte tests require break "flag" not set */
//#ifdef DOING_HARTE_TESTS
  cpu->flags &= ~FLAG_BREAK;
  //#endif
  uint8_t pc_low = stack_pop(cpu);
  uint8_t pc_high = stack_pop(cpu);
  cpu->pc = (pc_low | pc_high << 8);
}

/* -----------------------------------------------------------------------------*/
/* STACK INSTRUCTIONS: PHA, PLA, PHP, PLP */

static void PHA(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */
  stack_push(cpu, cpu->a);
}

static void PHP(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, addr_mode_handlers[mode](cpu)); /* dummy fetch pc + 1 */
  stack_push(cpu, cpu->flags | FLAG_BREAK | FLAG_UNUSED);
}

/* Flags: N+ V 1 B D I Z+ C */
static void PLA(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, cpu->pc); /* dummy fetch pc + 1 */
  fetch8(cpu, (1 << 8) | cpu->sp);
  cpu->a = stack_pop(cpu);
  cpu->flags = (cpu->flags & MASK_NZ) | (!cpu->a << ZERO_SHIFT) |
               (NEGATIVE(cpu->a) << NEGATIVE_SHIFT);
}

/* Flags: N+ V+ 1 1 D+ I (+1) Z+ C+ */
static void PLP(cpu_s *cpu, addr_mode_e mode) {
  fetch8(cpu, cpu->pc);            /* dummy fetch pc + 1 */
  fetch8(cpu, (1 << 8) | cpu->sp); /* dummy fetch stack */
  uint8_t flags = stack_pop(cpu);
  cpu->to_update_flags = 2;
  cpu->new_int_disable_flag = flags & FLAG_INT_DISABLE;
  cpu->flags = (cpu->flags & MASK_NVDZC) | (flags & ~MASK_NVDZC);
}

/*=================================ILLEGAL
 * OPCODES===============================*/

/* TODO: fix all the repetition in this copy paste job... */

static void LAX(cpu_s *cpu, addr_mode_e mode) {
  LDX(cpu, mode);
  cpu->a = cpu->x;
  cpu->flags = (cpu->flags & MASK_NZ) | (NEGATIVE(cpu->a) << NEGATIVE_SHIFT) |
               (!(cpu->a) << ZERO_SHIFT);
}

static void SAX(cpu_s *cpu, addr_mode_e mode) {
  STORE_INSTRUCTION(cpu->a & cpu->x);
}

static void DCP(cpu_s *cpu, addr_mode_e mode) {
  /* dec */
  uint16_t addr = addr_mode_handlers[mode](cpu);
  uint8_t val = fetch8(cpu, addr);
  write8(cpu, addr, val); /* dummy write */
  uint8_t res = val - 1;
  write8(cpu, addr, res);
  cpu->flags = (cpu->flags & MASK_NZ) | (!res << ZERO_SHIFT) |
               (NEGATIVE(res) << NEGATIVE_SHIFT);
  /* cmp */
  uint8_t reg = cpu->a;
  uint8_t oper = res;
  res = reg - oper;
  cpu->flags = (cpu->flags & MASK_NZC) | ((reg >= oper) << CARRY_SHIFT) |
               ((reg == oper) << ZERO_SHIFT) |
               (NEGATIVE(res) << NEGATIVE_SHIFT);
}

static void ISB(cpu_s *cpu, addr_mode_e mode) {
  /* inc */
  uint16_t addr = addr_mode_handlers[mode](cpu);
  uint8_t val = fetch8(cpu, addr);
  write8(cpu, addr, val); /* dummy write */
  uint8_t inc_res = val + 1;
  write8(cpu, addr, inc_res);
  cpu->flags = (cpu->flags & MASK_NZ) | (!inc_res << ZERO_SHIFT) |
               (NEGATIVE(inc_res) << NEGATIVE_SHIFT);
  /* sbc */
  uint8_t m = inc_res;
  uint16_t res = cpu->a + (uint8_t)~m + (cpu->flags & FLAG_CARRY);
  uint8_t trunc_res = res & 0xFF;
  cpu->flags = (cpu->flags & MASK_NVZC) | (((res >> 8) & 1) << CARRY_SHIFT) |
               (!trunc_res << ZERO_SHIFT) |
               (OVERFLOW(cpu->a, ~m, res) << OVERFLOW_SHIFT) |
               (NEGATIVE(trunc_res) << NEGATIVE_SHIFT);
  cpu->a = trunc_res;
}

static void SLO(cpu_s *cpu, addr_mode_e mode) {
  /* asl */
  uint8_t oper, res;
  uint16_t addr = addr_mode_handlers[mode](cpu);
  oper = fetch8(cpu, addr);
  write8(cpu, addr, oper);
  res = oper << 1;
  write8(cpu, addr, res);
  cpu->flags = (cpu->flags & MASK_NZC) | (((oper & 0x80) >> 7) << CARRY_SHIFT) |
               (!res << ZERO_SHIFT) | (NEGATIVE(res) << NEGATIVE_SHIFT);
  /* ora */
  cpu->a |= res;
  cpu->flags = (cpu->flags & MASK_NZ) | (NEGATIVE(cpu->a) << NEGATIVE_SHIFT) |
               (!cpu->a << ZERO_SHIFT);
}

static void RLA(cpu_s *cpu, addr_mode_e mode) {
  /* ROL */
  uint8_t oper, res;
  uint16_t addr = addr_mode_handlers[mode](cpu);
  oper = fetch8(cpu, addr);
  write8(cpu, addr, oper);
  res = (oper << 1) | ((cpu->flags & FLAG_CARRY) >> CARRY_SHIFT);
  write8(cpu, addr, res);
  cpu->flags = (cpu->flags & MASK_NZC) | (((oper & 0x80) >> 7) << CARRY_SHIFT) |
               (!res << ZERO_SHIFT) | (NEGATIVE(res) << NEGATIVE_SHIFT);
  /* AND */
  cpu->a &= res;
  cpu->flags = (cpu->flags & MASK_NZ) | (NEGATIVE(cpu->a) << NEGATIVE_SHIFT) |
               (!cpu->a << ZERO_SHIFT);
}

static void SRE(cpu_s *cpu, addr_mode_e mode) {
  /* lsr */
  uint8_t oper, res;
  uint16_t addr = addr_mode_handlers[mode](cpu);
  oper = fetch8(cpu, addr);
  write8(cpu, addr, oper); /* dummy write */
  res = oper >> 1;
  write8(cpu, addr, res);
  cpu->flags = (cpu->flags & MASK_NZC) | ((oper & 0x1) << CARRY_SHIFT) |
               (!res << ZERO_SHIFT) | (NEGATIVE(res) << NEGATIVE_SHIFT);

  /* eor */
  cpu->a ^= res;
  cpu->flags = (cpu->flags & MASK_NZ) | (NEGATIVE(cpu->a) << NEGATIVE_SHIFT) |
               (!cpu->a << ZERO_SHIFT);
}

static void RRA(cpu_s *cpu, addr_mode_e mode) {
  /* ror */
  uint8_t oper, ror_res;
  uint16_t addr = addr_mode_handlers[mode](cpu);
  oper = fetch8(cpu, addr);
  write8(cpu, addr, oper); /* dummy write */
  ror_res = (oper >> 1) | ((cpu->flags & FLAG_CARRY) << (7 - CARRY_SHIFT));
  write8(cpu, addr, ror_res);
  cpu->flags = (cpu->flags & MASK_NZC) | ((oper & 0x1) << CARRY_SHIFT) |
               (!ror_res << ZERO_SHIFT) | (NEGATIVE(ror_res) << NEGATIVE_SHIFT);

  /* adc */
  uint8_t m = ror_res;
  uint16_t res = cpu->a + (uint8_t)m + (cpu->flags & FLAG_CARRY);
  uint8_t trunc_res = res & 0xFF;
  cpu->flags = (cpu->flags & MASK_NVZC) | ((res != trunc_res) << CARRY_SHIFT) |
               (!trunc_res << ZERO_SHIFT) |
               (OVERFLOW(cpu->a, m, res) << OVERFLOW_SHIFT) |
               (NEGATIVE(trunc_res) << NEGATIVE_SHIFT);
  cpu->a = trunc_res;
}
