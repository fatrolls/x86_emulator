#include "emulator_function.h"

uint8_t get_code8(Emulator* emu, int index)
{
	return emu->memory[emu->eip + index];
}

int8_t get_sign_code8(Emulator* emu, int index)
{
	return (int8_t)get_code8(emu, index);
}


uint32_t get_code16(Emulator* emu, int index)
{
  int i;
  uint32_t ret = 0;

  /* Get the value of the memory in little-endian */
  for (i = 0; i < 2; i++) {
    ret |= get_code8(emu, index + i) << (i * 8);
  }

  return ret;
}

int32_t get_sign_code16(Emulator* emu, int index)
{
  return (int32_t)get_code16(emu, index);
}

uint32_t get_code32(Emulator* emu, int index)
{
  int i;
  uint32_t ret = 0;

  /* Get the value of the memory in little-endian */
  for (i = 0; i < 4; i++) {
    ret |= get_code8(emu, index + i) << (i * 8);
  }

  return ret;
}

int32_t get_sign_code32(Emulator* emu, int index)
{
  return (int32_t)get_code32(emu, index);
}

uint32_t get_register32(Emulator* emu, int index)
{
  return emu->registers[index];
}

void set_register32(Emulator* emu, int index, uint32_t value)
{
  emu->registers[index] = value;
}

uint16_t get_register16(Emulator* emu, int index)
{
    return emu->registers[index] & 0xffff;
}

void set_register16(Emulator* emu, int index, uint16_t value)
{
  uint32_t r = emu->registers[index] & 0xffff0000;
  emu->registers[index] = r | (uint32_t)value;
}

uint8_t get_register8(Emulator* emu, int index)
{
  if (index < 4) {
    return emu->registers[index] & 0xff;
  } else {
    return (emu->registers[index - 4] >> 8) & 0xff;
  }
}

void set_register8(Emulator* emu, int index, uint8_t value)
{
  if (index < 4) {
    uint32_t r = emu->registers[index] & 0xffffff00;
    emu->registers[index] = r | (uint32_t)value;
  } else {
    uint32_t r = emu->registers[index - 4] & 0xffff00ff;
    emu->registers[index - 4] = r | ((uint32_t)value << 8) & 0xffff00ff;
  }
}

uint32_t get_memory8(Emulator* emu, uint32_t address)
{
	if(address > MEMORY_SIZE) {
		printf("error cant access this memory: %X\n", address);
		return;
	}
	 return emu->memory[address];
}

void set_memory8(Emulator* emu, uint32_t address, uint32_t value)
{
	if(address > MEMORY_SIZE) {
		printf("error cant set this memory: %X = %X\n", address, value);
		return;
	}
	emu->memory[address] = value & 0xFF;
}

uint32_t get_memory16(Emulator* emu, uint32_t address)
{
  int i;
  uint32_t ret = 0;

  /* To get the value of the memory in little-endian */
  for (i = 0; i < 2; i++) {
    ret |= get_memory8(emu, address + i) << (8 * i);
  }

  return ret;
}

void set_memory16(Emulator* emu, uint32_t address, uint32_t value)
{
  int i;

  /* To set the value of the memory in little-endian */
  for (i = 0; i < 2; i++) {
    set_memory8(emu, address + i, value >> (i * 8));
  }
}

uint32_t get_memory32(Emulator* emu, uint32_t address)
{
  int i;
  uint32_t ret = 0;

  /* To get the value of the memory in little-endian */
  for (i = 0; i < 4; i++) {
    ret |= get_memory8(emu, address + i) << (8 * i);
  }

  return ret;
}

void set_memory32(Emulator* emu, uint32_t address, uint32_t value)
{
  int i;

  /* To set the value of the memory in little-endian */
  for (i = 0; i < 4; i++) {
    set_memory8(emu, address + i, value >> (i * 8));
  }
}

void push32(Emulator* emu, uint32_t value)
{
  uint32_t address = get_register32(emu, ESP) - 4;
  set_register32(emu, ESP, address);
  set_memory32(emu, address, value);
}

uint32_t pop32(Emulator* emu)
{
  uint32_t address = get_register32(emu, ESP);
  uint32_t ret = get_memory32(emu, address);
  set_register32(emu, ESP, address + 4);

  return ret;
}

void push16(Emulator* emu, uint16_t value)
{
  uint32_t address = get_register32(emu, ESP) - 2;
  set_register32(emu, ESP, address);
  set_memory16(emu, address, value);
}

uint16_t pop16(Emulator* emu)
{
  uint32_t address = get_register32(emu, ESP);
  uint32_t ret = get_memory16(emu, address);
  set_register32(emu, ESP, address + 2);

  return (uint16_t)ret;
}

void set_carry(Emulator* emu, int is_carry)
{
  if (is_carry) {
    emu->eflags |= CARRY_FLAG;
  } else {
    emu->eflags &= ~CARRY_FLAG;
  }
}

void set_parity(Emulator* emu, int is_parity)
{
  if (is_parity) {
    emu->eflags |= PARITY_FLAG;
  } else {
    emu->eflags &= ~PARITY_FLAG;
  }
}

void set_aux(Emulator* emu, int is_aux)
{
  if (is_aux) {
    emu->eflags |= AUX_FLAG;
  } else {
    emu->eflags &= ~AUX_FLAG;
  }
}

void set_zero(Emulator* emu, int is_zero)
{
  if (is_zero) {
    emu->eflags |= ZERO_FLAG;
  } else {
    emu->eflags &= ~ZERO_FLAG;
  }
}

void set_sign(Emulator* emu, int is_sign)
{
  if (is_sign) {
    emu->eflags |= SIGN_FLAG;
  } else {
    emu->eflags &= ~SIGN_FLAG;
  }
}

void set_trap(Emulator* emu, int is_trap)
{
  if (is_trap) {
    emu->eflags |= TRAP_FLAG;
  } else {
    emu->eflags &= ~TRAP_FLAG;
  }
}

void set_interrupt_enable(Emulator* emu, int is_interrupt_enable)
{
  if (is_interrupt_enable) {
    emu->eflags |= INTERRUPT_ENABLE_FLAG;
  } else {
    emu->eflags &= ~INTERRUPT_ENABLE_FLAG;
  }
}

void set_direction(Emulator* emu, int is_direction)
{
  if (is_direction) {
    emu->eflags |= DIR_FLAG;
  } else {
    emu->eflags &= ~DIR_FLAG;
  }
}

void set_overflow(Emulator* emu, int is_overflow)
{
  if (is_overflow) {
    emu->eflags |= OVERFLOW_FLAG;
  } else {
    emu->eflags &= ~OVERFLOW_FLAG;
  }
}

int is_carry(Emulator* emu)
{
  return (emu->eflags & CARRY_FLAG) != 0;
}

int is_parity(Emulator* emu)
{
  return (emu->eflags & PARITY_FLAG) != 0;
}

int is_aux(Emulator* emu)
{
  return (emu->eflags & AUX_FLAG) != 0;
}

int is_zero(Emulator* emu)
{
  return (emu->eflags & ZERO_FLAG) != 0;
}

int is_sign(Emulator* emu)
{
  return (emu->eflags & SIGN_FLAG) != 0;
}

int is_trap(Emulator* emu)
{
  return (emu->eflags & TRAP_FLAG) != 0;
}

int is_interrupt_enable(Emulator* emu)
{
  return (emu->eflags & INTERRUPT_ENABLE_FLAG) != 0;
}

int is_direction(Emulator* emu)
{
  return (emu->eflags & DIR_FLAG) != 0;
}

int is_overflow(Emulator* emu)
{
  return (emu->eflags & OVERFLOW_FLAG) != 0;
}

void update_eflags_sub(Emulator* emu, uint32_t v1, uint32_t v2, uint64_t result)
{
  /* Get the code of each value */
  int sign1 = v1 >> 31;
  int sign2 = v2 >> 31;
  int signr = (result >> 31) & 1;

  /* Carry flag set if there is a carry on the calculation results */
  set_carry(emu, result >> 32);

  /* The result is 0, the zero flag set */
  set_zero(emu, result == 0);

  /* Sign flag set if there is a sign on the calculation results */
  set_sign(emu, signr);

  /* Overflow flag setting result of the operation when I was overflow */
  set_overflow(emu, sign1 != sign2 && sign1 != signr);
}
