#ifndef EMULATOR_FUNCTION_H_
#define EMULATOR_FUNCTION_H_
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "emulator.h"

static uint32_t parity_table[8] =
{
	0x96696996,
	0x69969669,
	0x69969669,
	0x96696996,
	0x69969669,
	0x96696996,
	0x96696996,
	0x69969669,
};

#define PARITY(x)   (((parity_table[(x) / 32] >> ((x) % 32)) & 1) == 0)
#define XOR2(x) 	(((x) ^ ((x)>>1)) & 0x1)

#define CARRY_FLAG (1)
#define PARITY_FLAG (1 << 1)
#define AUX_FLAG (1 << 2)
#define ZERO_FLAG (1 << 3)
#define SIGN_FLAG (1 << 4)
#define TRAP_FLAG (1 << 5)
#define INTERRUPT_ENABLE_FLAG (1 << 6)
#define DIR_FLAG (1 << 7)
#define OVERFLOW_FLAG (1 << 8)

/* Get an unsigned 8-bit value from the program counter to the relative position */
uint8_t get_code8(Emulator* emu, int index);
/* Get a signed 8-bit value from the program counter to the relative position */
int8_t get_sign_code8(Emulator* emu, int index);

/* Get the unsigned 16-bit value at relative position of the program counter */
uint32_t get_code16(Emulator* emu, int index);
/* Get a signed 16-bit value from the program counter to the relative position */
int32_t get_sign_code16(Emulator* emu, int index);

/* Get the unsigned 32-bit value at relative position of the program counter */
uint32_t get_code32(Emulator* emu, int index);
/* Get a signed 32-bit value from the program counter to the relative position*/
int32_t get_sign_code32(Emulator* emu, int index);

/* To get the value of the index the 32-bit general-purpose register */
uint32_t get_register32(Emulator* emu, int index);
/* The value is set to index the 32-bit general-purpose register */
void set_register32(Emulator* emu, int index, uint32_t value);

/* To get the value of the index the 16-bit general-purpose register */
uint16_t get_register16(Emulator* emu, int index);
/* The value is set to index the 16-bit general-purpose register */
void set_register16(Emulator* emu, int index, uint16_t value);

/* To get the value of the index the 8-bit general-purpose register */
uint8_t get_register8(Emulator* emu, int index);
/* The value is set to index the 8-bit general-purpose register */
void set_register8(Emulator* emu, int index, uint8_t value);

/* To get the 8-bit value of the index address of the memory */
uint32_t get_memory8(Emulator* emu, uint32_t address);
/* Setting the 8-bit value to the index address of the memory */
void set_memory8(Emulator* emu, uint32_t address, uint32_t value);

/* To get the 16-bit value of the index address of the memory */
uint32_t get_memory16(Emulator* emu, uint32_t address);
/* To set a 16-bit value to the index address of the memory */
void set_memory16(Emulator* emu, uint32_t address, uint32_t value);

/* To get the 32-bit value of the index address of the memory */
uint32_t get_memory32(Emulator* emu, uint32_t address);
/* To set a 32-bit value to the index address of the memory */
void set_memory32(Emulator* emu, uint32_t address, uint32_t value);

/*Gain 32-bit value on the stack */
void push32(Emulator* emu, uint32_t value);

/* Taking out a 32-bit value from the stack */
uint32_t pop32(Emulator* emu);

/*Gain 16-bit value on the stack */
void push16(Emulator* emu, uint16_t value);

/* Taking out a 16-bit value from the stack */
uint16_t pop16(Emulator* emu);

/* Function for each setting of the flags of the EFLAGS */
void set_carry(Emulator* emu, int is_carry);
void set_parity(Emulator* emu, int is_parity);
void set_aux(Emulator* emu, int is_aux);
void set_zero(Emulator* emu, int is_zero);
void set_sign(Emulator* emu, int is_sign);
void set_trap(Emulator* emu, int is_trap);
void set_interrupt_enable(Emulator* emu, int is_interrupt_enable);
void set_direction(Emulator* emu, int is_direction);
void set_overflow(Emulator* emu, int is_overflow);

/* Each flag acquisition function of EFLAGS */
int32_t is_carry(Emulator* emu);
int32_t is_parity(Emulator* emu);
int32_t is_aux(Emulator* emu);
int32_t is_zero(Emulator* emu);
int32_t is_sign(Emulator* emu);
int32_t is_trap(Emulator* emu);
int32_t is_interrupt_enable(Emulator* emu);
int32_t is_direction(Emulator* emu);
int32_t is_overflow(Emulator* emu);

/* Update function of EFLAGS by subtraction */
void update_eflags_sub(Emulator* emu, uint32_t v1, uint32_t v2, uint64_t result);

#endif
