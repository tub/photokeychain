/*
  instructions.c
  Copyright 2000,2001,2003 by William Sheldon Simms III

  This file is a part of open apple -- a free Apple II emulator.
 
  Open apple is free software; you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software Foundation; either
  version 2, or (at your option) any later version.
 
  Open apple is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with open apple;
  see the file COPYING. If not, visit the Free Software Foundation website at http://www.fsf.org
*/

#include "instructions.h"
#include "tables.h"

#ifdef OPENAPPLE
#include "apple_cpu.h"
#include "iie_memory.h"
#else
#include "AddressPeripheral.h"
#endif

/* 65c02 registers */

unsigned char A;         /* Accumulator     */
unsigned char X, Y;      /* Index Registers */
unsigned char P = 0x34;  /* Status Byte     */
unsigned char S;         /* Stack Pointer   */
unsigned char C = 0, Z = 0, I = 1, D = 0, B = 1, V = 0, N = 0;
unsigned char interrupt_flags = 0;
unsigned short emPC;
unsigned char inside_interrupt = 0;

unsigned long long cycle_clock = 0;

/* Macros to push bytes to and pull bytes from the 65c02 stack */

#define PUSH(b) (WRITE_STACK(S,(b)),--S)
#define PULL()  (++S,READ_STACK(S))

/* Macro to set N and Z flags */

#define SET_FLAGS_NZ(a)     \
  N = ((a) & 0x80) ? 1 : 0; \
  Z = (a) ? 0 : 1;          \

/* 65c02 addressing modes */

static unsigned short imm (void)
{
  return emPC++;
}

static unsigned short zp_indirect (void)
{
  unsigned char  zp_addr;
  unsigned short op_addr;

  zp_addr  = READ(emPC); emPC++;
  op_addr  = READ_ZP(zp_addr); zp_addr++;
  op_addr += (256 * READ_ZP(zp_addr));

  return op_addr;
}

static unsigned short indirect_x (void)
{
  unsigned char  zp_addr;
  unsigned short op_addr;

  zp_addr  = READ(emPC) + X; emPC++;
  op_addr  = READ_ZP(zp_addr); zp_addr++;
  op_addr += (256 * READ_ZP(zp_addr));

  return op_addr;
}

static unsigned short indirect_y (void)
{
  unsigned char  zp_addr;
  unsigned short op_base;
  unsigned short op_addr;

  zp_addr  = READ(emPC); emPC++;
  op_base  = READ_ZP(zp_addr); zp_addr++;
  op_base += (256 * READ_ZP(zp_addr));
  op_addr  = op_base + (unsigned short)Y;

  if ((op_base & 0xFF00) != (op_addr & 0xFF00))
    ++cycle_clock;

  return op_addr;
}

static unsigned short absolute (void)
{
  unsigned short op_addr;

  op_addr  = READ(emPC); emPC++;
  op_addr += (256 * READ(emPC)); emPC++;

  return op_addr;
}

static unsigned short absolute_x (void)
{
  unsigned short op_base;
  unsigned short op_addr;

  op_base  = READ(emPC); emPC++;
  op_base += (256 * READ(emPC)); emPC++;
  op_addr  = op_base + (unsigned short)X;

  if ((op_base & 0xFF00) != (op_addr & 0xFF00))
    ++cycle_clock;

  return op_addr;
}

static unsigned short absolute_y (void)
{
  unsigned short op_base;
  unsigned short op_addr;

  op_base  = READ(emPC); emPC++;
  op_base += (256 * READ(emPC)); emPC++;
  op_addr  = op_base + (unsigned short)Y;

  if ((op_base & 0xFF00) != (op_addr & 0xFF00))
    ++cycle_clock;

  return op_addr;
}

static unsigned char zp (void)
{
  unsigned char zp_addr;
  zp_addr = READ(emPC); emPC++;
  return zp_addr;
}

static unsigned char zp_x (void)
{
  return zp() + X;
}

static unsigned char zp_y (void)
{
  return zp() + Y;
}

/* 65c02 relative branch */

static void branch (void)
{
  int branch_offset;
  unsigned short branch_dest;

  branch_offset = (signed char)READ(emPC); emPC++;
  branch_dest   = (unsigned short)(branch_offset + (int)emPC);

  ++cycle_clock;
  if ((emPC & 0xFF00) != (branch_dest & 0xFF00))
    ++cycle_clock;

  emPC = branch_dest;
}

/* 65c02 loads */

#define LOAD(reg,addr) \
do { \
  int taddr; \
  taddr = (addr); \
  (reg) = READ(taddr); \
  SET_FLAGS_NZ((reg)); \
} while (0)

#define LOAD_ZP(reg,addr) \
do { \
  int taddr; \
  taddr = (addr); \
  (reg) = READ_ZP(taddr); \
  SET_FLAGS_NZ((reg)); \
} while (0)

// stores use memory.h WRITE() macro directly

/* 65c02 ALU operations */

static void BIT_common (unsigned char op_byte)
{
  V = (op_byte & 0x40) ? 1 : 0;
  N = (op_byte & 0x80) ? 1 : 0;
  Z = (op_byte & A)    ? 0 : 1;
}

static void BIT (unsigned short op_addr)
{
  BIT_common(READ(op_addr));
}

static void BIT_zp (unsigned char zp_addr)
{
  BIT_common(READ_ZP(zp_addr));
}

static void ORA_common (unsigned char op_byte)
{
  A |= op_byte;
  SET_FLAGS_NZ(A);
}

static void ORA (unsigned short op_addr)
{
  ORA_common(READ(op_addr));
}

static void ORA_zp (unsigned char zp_addr)
{
  ORA_common(READ_ZP(zp_addr));
}

static void EOR_common (unsigned char op_byte)
{
  A ^= op_byte;
  SET_FLAGS_NZ(A);
}

static void EOR (unsigned short op_addr)
{
  EOR_common(READ(op_addr));
}

static void EOR_zp (unsigned char zp_addr)
{
  EOR_common(READ_ZP(zp_addr));
}

static void AND_common (unsigned char op_byte)
{
  A &= op_byte;
  SET_FLAGS_NZ(A);
}

static void AND (unsigned short op_addr)
{
  AND_common(READ(op_addr));
}

static void AND_zp (unsigned char zp_addr)
{
  AND_common(READ_ZP(zp_addr));
}

static void ASL (unsigned short op_addr)
{
  unsigned char op_byte;

  op_byte   = READ(op_addr);
  C         = (op_byte & 0x80) ? 1 : 0;
  op_byte <<= 1;

  SET_FLAGS_NZ(op_byte);
  WRITE(op_addr, op_byte);
}

static void ASL_zp (unsigned char zp_addr)
{
  unsigned char zp_byte;

  zp_byte   = READ_ZP(zp_addr);
  C         = (zp_byte & 0x80) ? 1 : 0;
  zp_byte <<= 1;

  SET_FLAGS_NZ(zp_byte);
  WRITE_ZP(zp_addr, zp_byte);
}

static void LSR (unsigned short op_addr)
{
  unsigned char op_byte;

  op_byte   = READ(op_addr);
  C         = op_byte & 0x01;
  op_byte >>= 1;
  N         = 0;
  Z         = op_byte ? 0 : 1;

  WRITE(op_addr, op_byte);
}

static void LSR_zp (unsigned char zp_addr)
{
  unsigned char zp_byte;

  zp_byte   = READ_ZP(zp_addr);
  C         = zp_byte & 0x01;
  zp_byte >>= 1;
  N         = 0;
  Z         = zp_byte ? 0 : 1;

  WRITE_ZP(zp_addr, zp_byte);
}

static void ROL (unsigned short op_addr)
{
  unsigned char op_byte;
  unsigned char result;

  op_byte = READ(op_addr);
  result  = (op_byte << 1) | C;
  C       = (op_byte & 0x80) ? 1 : 0;

  SET_FLAGS_NZ(result);
  WRITE(op_addr, result);
}

static void ROL_zp (unsigned char zp_addr)
{
  unsigned char zp_byte;
  unsigned char result;

  zp_byte = READ_ZP(zp_addr);
  result  = (zp_byte << 1) | C;
  C       = (zp_byte & 0x80) ? 1 : 0;

  SET_FLAGS_NZ(result);
  WRITE_ZP(zp_addr, result);
}

static void ROR (unsigned short op_addr)
{
  unsigned char op_byte;
  unsigned char result;

  op_byte = READ(op_addr);
  result  = op_byte >> 1;
  result |= (C ? 0x80 : 0x00);
  C       = op_byte & 0x01;

  SET_FLAGS_NZ(result);
  WRITE(op_addr, result);
}

static void ROR_zp (unsigned char zp_addr)
{
  unsigned char zp_byte;
  unsigned char result;

  zp_byte = READ_ZP(zp_addr);
  result  = zp_byte >> 1;
  result |= (C ? 0x80 : 0x00);
  C       = zp_byte & 0x01;

  SET_FLAGS_NZ(result);
  WRITE_ZP(zp_addr, result);
}

static void CMP (unsigned char op_register, unsigned short op_addr)
{
  unsigned char op_byte;
  unsigned char result;

  op_byte = READ(op_addr);
  C = (op_register >= op_byte) ? 1 : 0;
  result = op_register - op_byte;
  SET_FLAGS_NZ(result);
}

static void CMP_zp (unsigned char op_register, unsigned char zp_addr)
{
  unsigned char zp_byte;
  unsigned char result;

  zp_byte = READ_ZP(zp_addr);
  C = (op_register >= zp_byte) ? 1 : 0;
  result = op_register - zp_byte;
  SET_FLAGS_NZ(result);
}

static void INC (unsigned short op_addr)
{
  unsigned char op_byte;

  op_byte = READ(op_addr) + 1;
  SET_FLAGS_NZ(op_byte);
  WRITE(op_addr, op_byte);
}

static void INC_zp (unsigned char zp_addr)
{
  unsigned char zp_byte;

  zp_byte = READ_ZP(zp_addr) + 1;
  SET_FLAGS_NZ(zp_byte);
  WRITE_ZP(zp_addr, zp_byte);
}

static void DEC (unsigned short op_addr)
{
  unsigned char op_byte;

  op_byte = READ(op_addr) - 1;
  SET_FLAGS_NZ(op_byte);
  WRITE(op_addr, op_byte);
}

static void DEC_zp (unsigned char zp_addr)
{
  unsigned char zp_byte;

  zp_byte = READ_ZP(zp_addr) - 1;
  SET_FLAGS_NZ(zp_byte);
  WRITE_ZP(zp_addr, zp_byte);
}

static void ADC_decimal_common (unsigned char op_byte)
{
  unsigned char temp, result;

  /* add 'ones' digits */
  result = C + (A & 0xF) + (op_byte & 0xF);

  /* carry if needed */
  temp = 0;
  if (result > 9)
    {
      temp = 1;
      result -= 10;
    }

  /* add 'tens' digits */
  temp += ((A >> 4) + (op_byte >> 4));

  C = 0;
  if (temp > 9)
    {
      C = 1;
      temp -= 10;
    }

  V = C;
  A = result + (0x10 * temp);

  /* set flags */
  SET_FLAGS_NZ(A);

  /* takes one cycle more than binary */
  ++cycle_clock;
}

static void SBC_decimal_common (unsigned char op_byte)
{
  unsigned char old_A = A;

  /* subtract 'ones' digits */
  A = A - (op_byte & 0xF) + C - 1;

  /* if borrow was needed */
  if ((A & 0x0F) > (old_A & 0xF))
    {
      /* change 0x0F -> 0x09, etc. */
      A -= 6;
    }

  /* subtract 'tens' digits */
  A -= (op_byte & 0xF0);

  /* borrow if needed */
  if ((A & 0xF0) > (old_A & 0xF0))
    A -= 0x60;

  /* set flags */
  V = C = (A > old_A) ? 0 : 1;
  SET_FLAGS_NZ(A);

  /* takes one cycle more than binary */
  ++cycle_clock;
}

static void ADC_binary_common (unsigned char op_byte)
{
  unsigned char high_bit;

  high_bit = A >> 7;
  A = A + op_byte + C;

  if (A & 0x80) /* A < 0 */
    {
      Z = 0;
      N = 1;

      if (op_byte & 0x80) /* op_byte < 0 */
	{
	  C = high_bit;
	  V = 0;
	}
      else /* op_byte >= 0 */
	{
	  C = 0;
	  V = 1 ^ high_bit;
	}
    }
  else /* A >= 0 */
    {
      N = 0;
      Z = A ? 0 : 1;

      if (op_byte & 0x80) /* op_byte < 0 */
	{
	  C = 1;
	  V = high_bit;
	}
      else /* op_byte >= 0 */
	{
	  C = high_bit;
	  V = 0;
	}
    }
}

static void SBC_binary_common (unsigned char op_byte)
{
  unsigned char high_bit;

  high_bit = A >> 7;
  A = A - op_byte + C - 1;

  if (A & 0x80) /* A < 0 */
    {
      Z = 0;
      N = 1;

      if (op_byte & 0x80) /* op_byte < 0 */
	{
	  C = 0;
	  V = 1 ^ high_bit;
	}
      else /* op_byte >= 0 */
	{
	  C = high_bit;
	  V = 0;
	}
    }
  else /* A >= 0 */
    {
      N = 0;
      Z = A ? 0 : 1;

      if (op_byte & 0x80) /* op_byte < 0 */
	{
	  C = high_bit;
	  V = 0;
	}
      else /* op_byte >= 0 */
	{
	  C = 1;
	  V = high_bit;
	}
    }
}

static void ADC_binary (unsigned short op_addr)
{
  ADC_binary_common(READ(op_addr));
}

static void ADC_binary_zp (unsigned char zp_addr)
{
  ADC_binary_common(READ_ZP(zp_addr));
}

static void ADC_decimal (unsigned short op_addr)
{
  ADC_decimal_common(READ(op_addr));
}

static void ADC_decimal_zp (unsigned char zp_addr)
{
  ADC_decimal_common(READ_ZP(zp_addr));
}

static void SBC_binary (unsigned short op_addr)
{
  SBC_binary_common(READ(op_addr));
}

static void SBC_binary_zp (unsigned char zp_addr)
{
  SBC_binary_common(READ_ZP(zp_addr));
}

static void SBC_decimal (unsigned short op_addr)
{
  SBC_decimal_common(READ(op_addr));
}

static void SBC_decimal_zp (unsigned char zp_addr)
{
  SBC_decimal_common(READ_ZP(zp_addr));
}

/* Convert individual bits to P and reverse */

unsigned char build_P (void)
{
  P = 0x20;

  if (N) P |= 0x80;
  if (V) P |= 0x40;
  if (B) P |= 0x10;
  if (D) P |= 0x08;
  if (I) P |= 0x04;
  if (Z) P |= 0x02;
  if (C) P |= 0x01;

  return P;
}

void unbuild_P (unsigned char status)
{
  P = status;

  N = (P & 0x80) ? 1 : 0;
  V = (P & 0x40) ? 1 : 0;

  if (B == 0)
    B = (P & 0x10) ? 1 : 0;

  D = (P & 0x08) ? 1 : 0;
  I = (P & 0x04) ? 1 : 0;
  Z = (P & 0x02) ? 1 : 0;
  C = (P & 0x01) ? 1 : 0;

  if (D)
    instruction_table = decimal_instruction_table;
  else
    instruction_table = binary_instruction_table;
}

/* Generate an interrupt */

void interrupt (unsigned short vector, unsigned char flag)
{
  unsigned char s_byte;

  interrupt_flags &= (~flag);

  if (flag == F_RESET)
    {
      B = 1;
    }
  else
    {
      s_byte = emPC / 256;
      PUSH(s_byte);
      s_byte = emPC & 0xFF;
      PUSH(s_byte);

      B = 0;
      s_byte = build_P();
      PUSH(s_byte);

      inside_interrupt = true;
    }

  I = 1;
  D = 0;
  instruction_table = binary_instruction_table;

  build_P();

  emPC = READ(vector); vector++;
  emPC += (256 * READ(vector));

  cycle_clock += 7;
}

/* 65c02 instructions */

void i00_BRK (void)
{
  ++emPC;

  PUSH(emPC / 256);
  PUSH(emPC & 255);
    
  B = 1;
  PUSH(build_P());

  I = 1;
  D = 0;
  instruction_table = binary_instruction_table;

  emPC = READ(IRQ_VECTOR) + (256 * READ(IRQ_VECTOR+1));
}

void i01_ORA (void)
{
  ORA(indirect_x());
}

void i04_TSB (void)
{
  unsigned char zp_byte;
  unsigned char zp_addr;

  zp_addr  = zp();
  zp_byte  = READ_ZP(zp_addr);
  Z        = (A & zp_byte) ? 0 : 1;
  zp_byte |= A;

  WRITE_ZP(zp_addr, zp_byte);
}

void i05_ORA (void)
{
  ORA_zp(zp());
}

void i06_ASL (void)
{
  ASL_zp(zp());
}

void i08_PHP (void)
{
  PUSH(build_P());
}

void i09_ORA (void)
{
  ORA(imm());
}

void i0A_ASL (void)
{
  C = (A & 0x80) ? 1 : 0;
  A <<= 1;
  SET_FLAGS_NZ(A);
}

void i0C_TSB (void)
{
  unsigned char  op_byte;
  unsigned short op_addr;

  op_addr  = absolute();
  op_byte  = READ(op_addr);
  Z        = (A & op_byte) ? 0 : 1;
  op_byte |= A;

  WRITE(op_addr, op_byte);
}

void i0D_ORA (void)
{
  ORA(absolute());
}

void i0E_ASL (void)
{
  ASL(absolute());
}

void i10_BPL (void)
{
  if (N)
    ++emPC;
  else
    branch();
}

void i11_ORA (void)
{
  ORA(indirect_y());
}

void i12_ORA (void)
{
  ORA(zp_indirect());
}

void i14_TRB (void)
{
  unsigned char zp_addr;
  unsigned char zp_byte;

  zp_addr  = zp();
  zp_byte  = READ_ZP(zp_addr);
  Z        = (A & zp_byte) ? 0 : 1;
  zp_byte &= (~A);

  WRITE_ZP(zp_addr, zp_byte);
}

void i15_ORA (void)
{
  ORA_zp(zp_x());
}

void i16_ASL (void)
{
  ASL_zp(zp_x());
}

void i18_CLC (void)
{
  C = 0;
}

void i19_ORA (void)
{
  ORA(absolute_y());
}

void i1A_INC (void)
{
  ++A;
  SET_FLAGS_NZ(A);
}

void i1C_TRB (void)
{
  unsigned char  op_byte;
  unsigned short op_addr;

  op_addr  = absolute();
  op_byte  = READ(op_addr);
  Z        = (A & op_byte) ? 0 : 1;
  op_byte &= (~A);

  WRITE(op_addr, op_byte);
}

void i1D_ORA (void)
{
  ORA(absolute_x());
}

void i1E_ASL (void)
{
  ASL(absolute_x());
}

void i20_JSR (void)
{
  unsigned short addr;
  addr = emPC + 1;
  PUSH(addr / 256);
  PUSH(addr & 255);
  emPC = absolute();
}

void i21_AND (void)
{
  AND(indirect_x());
}

void i24_BIT (void)
{
  BIT_zp(zp());
}

void i25_AND (void)
{
  AND_zp(zp());
}

void i26_ROL (void)
{
  ROL_zp(zp());
}

void i28_PLP (void)
{
  unbuild_P(PULL());
}

void i29_AND (void)
{
  AND(imm());
}

void i2A_ROL (void)
{
  unsigned char result;

  result = (A << 1) | C;
  C      = (A & 0x80) ? 1 : 0;
  A      = result;

  SET_FLAGS_NZ(A);
}

void i2C_BIT (void)
{
  BIT(absolute());
}

void i2D_AND (void)
{
  AND(absolute());
}

void i2E_ROL (void)
{
  ROL(absolute());
}

void i30_BMI (void)
{
  if (N)
    branch();
  else
    ++emPC;
}

void i31_AND (void)
{
  AND(indirect_y());
}

void i32_AND (void)
{
  AND(zp_indirect());
}

void i34_BIT (void)
{
  BIT_zp(zp_x());
}

void i35_AND (void)
{
  AND_zp(zp_x());
}

void i36_ROL (void)
{
  ROL_zp(zp_x());
}

void i38_SEC (void)
{
  C = 1;
}

void i39_AND (void)
{
  AND(absolute_y());
}

void i3A_DEC (void)
{
  --A;
  SET_FLAGS_NZ(A);
}

void i3C_BIT (void)
{
  BIT(absolute_x());
}

void i3D_AND (void)
{
  AND(absolute_x());
}

void i3E_ROL (void)
{
  ROL(absolute_x());
}

void i40_RTI (void)
{
  inside_interrupt = false;
  unbuild_P(PULL());
  emPC = PULL();
  emPC += (256 * PULL());
}

void i41_EOR (void)
{
  EOR(indirect_x());
}

void i45_EOR (void)
{
  EOR_zp(zp());
}

void i46_LSR (void)
{
  LSR_zp(zp());
}

void i48_PHA (void)
{
  PUSH(A);
}

void i49_EOR (void)
{
  EOR(imm());
}

void i4A_LSR (void)
{
  C = A & 0x01;
  A >>= 1;
  N = 0;
  Z = A ? 0 : 1;
}

void i4C_JMP (void)
{
  emPC = absolute();
}

void i4D_EOR (void)
{
  EOR(absolute());
}

void i4E_LSR (void)
{
  LSR(absolute());
}

void i50_BVC (void)
{
  if (V)
    ++emPC;
  else
    branch();
}

void i51_EOR (void)
{
  EOR(indirect_y());
}

void i52_EOR (void)
{
  EOR(zp_indirect());
}

void i55_EOR (void)
{
  EOR_zp(zp_x());
}

void i56_LSR (void)
{
  LSR_zp(zp_x());
}

void i58_CLI (void)
{
  I = 0;
}

void i59_EOR (void)
{
  EOR(absolute_y());
}

void i5A_PHY (void)
{
  PUSH(Y);
}

void i5D_EOR (void)
{
  EOR(absolute_x());
}

void i5E_LSR (void)
{
  LSR(absolute_x());
}

void i60_RTS (void)
{
  emPC  = PULL();
  emPC += (256 * PULL());
  emPC += 1;
}

void i61_ADC_binary (void)
{
  ADC_binary(indirect_x());
}

void i61_ADC_decimal (void)
{
  ADC_decimal(indirect_x());
}

void i64_STZ (void)
{
  unsigned char zp_addr;
  zp_addr = zp();
  WRITE_ZP(zp_addr, 0);
}

void i65_ADC_binary (void)
{
  ADC_binary_zp(zp());
}

void i65_ADC_decimal (void)
{
  ADC_decimal_zp(zp());
}

void i66_ROR (void)
{
  ROR_zp(zp());
}

void i68_PLA (void)
{
  A = PULL();
  SET_FLAGS_NZ(A);
}

void i69_ADC_binary (void)
{
  ADC_binary(imm());
}

void i69_ADC_decimal (void)
{
  ADC_decimal(imm());
}

void i6A_ROR (void)
{
  unsigned char result;

  result  = A >> 1;
  result |= (C ? 0x80 : 0x00);
  C       = A & 0x01;
  A       = result;

  SET_FLAGS_NZ(A);
}

void i6C_JMP (void)
{
  unsigned short addr;

  addr  = absolute();
  emPC  = READ(addr); addr++;
  emPC += (256 * READ(addr));
}

void i6D_ADC_binary (void)
{
  ADC_binary(absolute());
}

void i6D_ADC_decimal (void)
{
  ADC_decimal(absolute());
}

void i6E_ROR (void)
{
  ROR(absolute());
}

void i70_BVS (void)
{
  if (V)
    branch();
  else
    ++emPC;
}

void i71_ADC_binary (void)
{
  ADC_binary(indirect_y());
}

void i71_ADC_decimal (void)
{
  ADC_decimal(indirect_y());
}

void i72_ADC_binary (void)
{
  ADC_binary(zp_indirect());
}

void i72_ADC_decimal (void)
{
  ADC_decimal(zp_indirect());
}

void i74_STZ (void)
{
  unsigned char zp_addr;
  zp_addr = zp_x();
  WRITE_ZP(zp_addr, 0);
}

void i75_ADC_binary (void)
{
  ADC_binary_zp(zp_x());
}

void i75_ADC_decimal (void)
{
  ADC_decimal_zp(zp_x());
}

void i76_ROR (void)
{
  ROR_zp(zp_x());
}

void i78_SEI (void)
{
  I = 1;
}

void i79_ADC_binary (void)
{
  ADC_binary(absolute_y());
}

void i79_ADC_decimal (void)
{
  ADC_decimal(absolute_y());
}

void i7A_PLY (void)
{
  Y = PULL();
  SET_FLAGS_NZ(Y);
}

void i7C_JMP (void)
{
  unsigned short addr;

  addr  = absolute() + (unsigned short)X;
  emPC  = READ(addr); addr++;
  emPC += (256 * READ(addr));
}

void i7D_ADC_binary (void)
{
  ADC_binary(absolute_x());
}

void i7D_ADC_decimal (void)
{
  ADC_decimal(absolute_x());
}

void i7E_ROR (void)
{
  ROR(absolute_x());
}

void i80_BRA (void)
{
  branch();
}

void i81_STA (void)
{
  unsigned short addr;
  addr = indirect_x();
  WRITE(addr, A);
}

void i84_STY (void)
{
  unsigned char zp_addr;
  zp_addr = zp();
  WRITE_ZP(zp_addr, Y);
}

void i85_STA (void)
{
  unsigned char zp_addr;
  zp_addr = zp();
  WRITE_ZP(zp_addr, A);
}

void i86_STX (void)
{
  unsigned char zp_addr;
  zp_addr = zp();
  WRITE_ZP(zp_addr, X);
}

void i88_DEY (void)
{
  --Y;
  SET_FLAGS_NZ(Y);
}

void i89_BIT (void)
{
  /* doesn't use BIT() because in immediate addressing function is different */
  int addr = imm();
  Z = (READ(addr) & A) ? 0 : 1;
}

void i8A_TXA (void)
{
  A = X;
  SET_FLAGS_NZ(A);
}

void i8C_STY (void)
{
  unsigned short addr;
  addr = absolute();
  WRITE(addr, Y);
}

void i8D_STA (void)
{
  unsigned short addr;
  addr = absolute();
  WRITE(addr, A);
}

void i8E_STX (void)
{
  unsigned short addr;
  addr = absolute();
  WRITE(addr, X);
}

void i90_BCC (void)
{
  if (C)
    ++emPC;
  else
    branch();
}

void i91_STA (void)
{
  unsigned short addr;
  addr = indirect_y();
  WRITE(addr, A);
}

void i92_STA (void)
{
  unsigned short addr;
  addr = zp_indirect();
  WRITE(addr, A);
}

void i94_STY (void)
{
  unsigned char zp_addr;
  zp_addr = zp_x();
  WRITE_ZP(zp_addr, Y);
}

void i95_STA (void)
{
  unsigned char zp_addr;
  zp_addr = zp_x();
  WRITE_ZP(zp_addr, A);
}

void i96_STX (void)
{
  unsigned char zp_addr;
  zp_addr = zp_y();
  WRITE_ZP(zp_addr, X);
}

void i98_TYA (void)
{
  A = Y;
  SET_FLAGS_NZ(A);
}

void i99_STA (void)
{
  unsigned short addr;
  addr = absolute_y();
  WRITE(addr, A);
}

void i9A_TXS (void)
{
  S = X;
}

void i9C_STZ (void)
{
  unsigned short addr;
  addr = absolute();
  WRITE(addr, 0);
}

void i9D_STA (void)
{
  unsigned short addr;
  addr = absolute_x();
  WRITE(addr, A);
}

void i9E_STZ (void)
{
  unsigned short addr;
  addr = absolute_x();
  WRITE(addr, 0);
}

void iA0_LDY (void)
{
  LOAD(Y,imm());
}

void iA1_LDA (void)
{
  LOAD(A,indirect_x());
}

void iA2_LDX (void)
{
  LOAD(X,imm());
}

void iA4_LDY (void)
{
  LOAD_ZP(Y,zp());
}

void iA5_LDA (void)
{
  LOAD_ZP(A,zp());
}

void iA6_LDX (void)
{
  LOAD_ZP(X,zp());
}

void iA8_TAY (void)
{
  Y = A;
  SET_FLAGS_NZ(Y);
}

void iA9_LDA (void)
{
  LOAD(A,imm());
}

void iAA_TAX (void)
{
  X = A;
  SET_FLAGS_NZ(X);
}

void iAC_LDY (void)
{
  LOAD(Y,absolute());
}

void iAD_LDA (void)
{
  LOAD(A,absolute());
}

void iAE_LDX (void)
{
  LOAD(X,absolute());
}

void iB0_BCS (void)
{
  if (C)
    branch();
  else
    ++emPC;
}

void iB1_LDA (void)
{
  LOAD(A,indirect_y());
}

void iB2_LDA (void)
{
  LOAD(A,zp_indirect());
}

void iB4_LDY (void)
{
  LOAD_ZP(Y,zp_x());
}

void iB5_LDA (void)
{
  LOAD_ZP(A,zp_x());
}

void iB6_LDX (void)
{
  LOAD_ZP(X,zp_x());
}

void iB8_CLV (void)
{
  V = 0;
}

void iB9_LDA (void)
{
  LOAD(A,absolute_y());
}

void iBA_TSX (void)
{
  X = S;
  SET_FLAGS_NZ(X);
}

void iBC_LDY (void)
{
  LOAD(Y,absolute_x());
}

void iBD_LDA (void)
{
  LOAD(A,absolute_x());
}

void iBE_LDX (void)
{
  LOAD(X,absolute_y());
}

void iC0_CPY (void)
{
  CMP(Y, imm());
}

void iC1_CMP (void)
{
  CMP(A, indirect_x());
}

void iC4_CPY (void)
{
  CMP_zp(Y, zp());
}

void iC5_CMP (void)
{
  CMP_zp(A, zp());
}

void iC6_DEC (void)
{
  DEC_zp(zp());
}

void iC8_INY (void)
{
  Y++;
  SET_FLAGS_NZ(Y);
}

void iC9_CMP (void)
{
  CMP(A, imm());
}

void iCA_DEX (void)
{
  X--;
  SET_FLAGS_NZ(X);
}

void iCC_CPY (void)
{
  CMP(Y, absolute());
}

void iCD_CMP (void)
{
  CMP(A, absolute());
}

void iCE_DEC (void)
{
  DEC(absolute());
}

void iD0_BNE (void)
{
  if (Z)
    ++emPC;
  else
    branch();
}

void iD1_CMP (void)
{
  CMP(A, indirect_y());
}

void iD2_CMP (void)
{
  CMP(A, zp_indirect());
}

void iD5_CMP (void)
{
  CMP_zp(A, zp_x());
}

void iD6_DEC (void)
{
  DEC_zp(zp_x());
}

void iD8_CLD (void)
{
  D = 0;
  instruction_table = binary_instruction_table;
}

void iD9_CMP (void)
{
  CMP(A, absolute_y());
}

void iDA_PHX (void)
{
  PUSH(X);
}

void iDD_CMP (void)
{
  CMP(A, absolute_x());
}

void iDE_DEC (void)
{
  DEC(absolute_x());
}

void iE0_CPX (void)
{
  CMP(X, imm());
}

void iE1_SBC_binary (void)
{
  SBC_binary(indirect_x());
}

void iE1_SBC_decimal (void)
{
  SBC_decimal(indirect_x());
}

void iE4_CPX (void)
{
  CMP_zp(X, zp());
}

void iE5_SBC_binary (void)
{
  SBC_binary_zp(zp());
}

void iE5_SBC_decimal (void)
{
  SBC_decimal_zp(zp());
}

void iE6_INC (void)
{
  INC_zp(zp());
}

void iE8_INX (void)
{
  X++;
  SET_FLAGS_NZ(X);
}

void iE9_SBC_binary (void)
{
  SBC_binary(imm());
}

void iE9_SBC_decimal (void)
{
  SBC_decimal(imm());
}

void iEA_NOP (void)
{
  /* Do nothing! */
}

void iEC_CPX (void)
{
  CMP(X, absolute());
}

void iED_SBC_binary (void)
{
  SBC_binary(absolute());
}

void iED_SBC_decimal (void)
{
  SBC_decimal(absolute());
}

void iEE_INC (void)
{
  INC(absolute());
}

void iF0_BEQ (void)
{
  if (Z)
    branch();
  else
    ++emPC;
}

void iF1_SBC_binary (void)
{
  SBC_binary(indirect_y());
}

void iF1_SBC_decimal (void)
{
  SBC_decimal(indirect_y());
}

void iF2_SBC_binary (void)
{
  SBC_binary(zp_indirect());
}

void iF2_SBC_decimal (void)
{
  SBC_decimal(zp_indirect());
}

void iF5_SBC_binary (void)
{
  SBC_binary_zp(zp_x());
}

void iF5_SBC_decimal (void)
{
  SBC_decimal_zp(zp_x());
}

void iF6_INC (void)
{
  INC_zp(zp_x());
}

void iF8_SED (void)
{
  D = 1;
  instruction_table = decimal_instruction_table;
}

void iF9_SBC_binary (void)
{
  SBC_binary(absolute_y());
}

void iF9_SBC_decimal (void)
{
  SBC_decimal(absolute_y());
}

void iFA_PLX (void)
{
  X = PULL();
  SET_FLAGS_NZ(X);
}

void iFD_SBC_binary (void)
{
  SBC_binary(absolute_x());
}

void iFD_SBC_decimal (void)
{
  SBC_decimal(absolute_x());
}

void iFE_INC (void)
{
  INC(absolute_x());
}

// extended instructions

void ixF_BBRx( unsigned char x )
{
  unsigned char testaddr = zp();
  unsigned char testvalue = READ_ZP(testaddr);
  unsigned char bitset = (1<<x) & testvalue;
  if (!bitset)
    branch();
  else
    ++emPC;
}

void ixF_BBSx( unsigned char x )
{
  unsigned char testaddr = zp();
  unsigned char testvalue = READ_ZP(testaddr);
  unsigned char bitset = (1<<x) & testvalue;
  if (bitset)
    branch();
  else
    ++emPC;
}


void i0F_BBR0( void )  { ixF_BBRx(0); };
void i1F_BBR1( void )  { ixF_BBRx(1); };
void i2F_BBR2( void )  { ixF_BBRx(2); };
void i3F_BBR3( void )  { ixF_BBRx(3); };
void i4F_BBR4( void )  { ixF_BBRx(4); };
void i5F_BBR5( void )  { ixF_BBRx(5); };
void i6F_BBR6( void )  { ixF_BBRx(6); };
void i7F_BBR7( void )  { ixF_BBRx(7); };

void i8F_BBS0( void )  { ixF_BBSx(0); };
void i9F_BBS1( void )  { ixF_BBSx(1); };
void iAF_BBS2( void )  { ixF_BBSx(2); };
void iBF_BBS3( void )  { ixF_BBSx(3); };
void iCF_BBS4( void )  { ixF_BBSx(4); };
void iDF_BBS5( void )  { ixF_BBSx(5); };
void iEF_BBS6( void )  { ixF_BBSx(6); };
void iFF_BBS7( void )  { ixF_BBSx(7); };

void ix7_RMBx( unsigned char x )
{
  unsigned char zp_addr;
  zp_addr = zp();
  unsigned char v = READ_ZP( zp_addr );
  WRITE_ZP(zp_addr, (v & ~(1<<x)) );
}

void ix7_SMBx( unsigned char x )
{
  unsigned char zp_addr;
  zp_addr = zp();
  unsigned char v = READ_ZP( zp_addr );
  WRITE_ZP(zp_addr, (v | (1<<x)) );
}


void i07_RMB0( void ) { ix7_RMBx(0); };
void i17_RMB1( void ) { ix7_RMBx(1); };
void i27_RMB2( void ) { ix7_RMBx(2); };
void i37_RMB3( void ) { ix7_RMBx(3); };
void i47_RMB4( void ) { ix7_RMBx(4); };
void i57_RMB5( void ) { ix7_RMBx(5); };
void i67_RMB6( void ) { ix7_RMBx(6); };
void i77_RMB7( void ) { ix7_RMBx(7); };

void i87_SMB0( void ) { ix7_SMBx(0); };
void i97_SMB1( void ) { ix7_SMBx(1); };
void iA7_SMB2( void ) { ix7_SMBx(2); };
void iB7_SMB3( void ) { ix7_SMBx(3); };
void iC7_SMB4( void ) { ix7_SMBx(4); };
void iD7_SMB5( void ) { ix7_SMBx(5); };
void iE7_SMB6( void ) { ix7_SMBx(6); };
void iF7_SMB7( void ) { ix7_SMBx(7); };


void iCB_WAI( void )
{
  // wait for interrupt
  printf("encountered WAI instruction, not implemented yet\n");
};

void iDB_STP( void )
{
  // stop processor, killed until reset.
  printf("got STP instruction, processor should be stopped, not implemented yet\n");
};
