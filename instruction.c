#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "instruction.h"
#include "emulator.h"
#include "emulator_function.h"
#include "io.h"

#include "modrm.h"

instruction_func_t* instructions[256];

/* 0x01 */
static void add_rm32_r32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t r32 = get_r32(emu, &modrm);
	uint32_t rm32 = get_rm32(emu, &modrm);
	
	register uint32_t lo = (rm32 & 0xFFFF) + (r32 & 0xFFFF);
	register uint32_t hi = (lo >> 16) + (rm32 >> 16) + (r32 >> 16);
	register uint32_t res = rm32 + r32;
	/* calculate the carry chain. */
	register uint32_t cc = (rm32 & r32) | ((~res) & (rm32 | r32));
	
	set_carry(emu, hi & 0x10000);
    set_zero(emu, (res & 0xffffffff) == 0);
    set_sign(emu, res & 0x80000000);
    set_parity(emu, PARITY(res & 0xff));
    set_overflow(emu, XOR2(cc >> 30));
    set_aux(emu, cc & 0x8);
	
	set_rm32(emu, &modrm, res);
}

/* 0x03 */
static void add_r32_rm32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t r32 = get_r32(emu, &modrm);
	uint32_t rm32 = get_rm32(emu, &modrm);
	
	register uint32_t lo = (r32 & 0xFFFF) + (rm32 & 0xFFFF);
	register uint32_t hi = (lo >> 16) + (r32 >> 16) + (rm32 >> 16);
	register uint32_t res = r32 + rm32;
	/* calculate the carry chain. */
	register uint32_t cc = (r32 & rm32) | ((~res) & (r32 | rm32));
	
	set_carry(emu, hi & 0x10000);
    set_zero(emu, (res & 0xffffffff) == 0);
    set_sign(emu, res & 0x80000000);
    set_parity(emu, PARITY(res & 0xff));
    set_overflow(emu, XOR2(cc >> 30));
    set_aux(emu, cc & 0x8);
	
	set_r32(emu, &modrm, res);
}

/* 0x04 */
static void add_al_imm8(Emulator* emu) {
	uint8_t al = get_register8(emu, AL);
	uint8_t value = get_code8(emu, 1);
	
	register uint32_t res = al + value;
	/* calculate the carry chain. */
	register uint32_t cc = (al & value) | ((~res) & (al | value));
	
	set_carry(emu, res & 0x100);
    set_zero(emu, (res & 0xff) == 0);
    set_sign(emu, res & 0x80);
    set_parity(emu, PARITY(res & 0xff));
    set_overflow(emu, XOR2(cc >> 6));
    set_aux(emu, cc & 0x8);
    
	set_register8(emu, AL, (uint8_t)res);
	emu->eip += 2;
}

/* 0x05 */
static void add_eax_imm32(Emulator* emu) {
	uint32_t value = get_code32(emu, 1);
	uint32_t eax = get_register32(emu, EAX);
	
	register uint32_t lo = (eax & 0xFFFF) + (value & 0xFFFF);
	register uint32_t hi = (lo >> 16) + (eax >> 16) + (value >> 16);
	register uint32_t res = eax + value;
	/* calculate the carry chain. */
	register uint32_t cc = (eax & value) | ((~res) & (eax | value));
	
	set_carry(emu, hi & 0x10000);
    set_zero(emu, (res & 0xffffffff) == 0);
    set_sign(emu, res & 0x80000000);
    set_parity(emu, PARITY(res & 0xff));
    set_overflow(emu, XOR2(cc >> 30));
    set_aux(emu, cc & 0x8);

	set_register32(emu, EAX, res);
	emu->eip += 5;
}

/* 0x09 */
static void or_rm32_r32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t rm32 = get_rm32(emu, &modrm);
	uint32_t r32 = get_r32(emu, &modrm);
	
	uint32_t res = rm32 | r32;
	set_rm32(emu, &modrm, res);
	
	/* set flags */
	set_overflow(emu, 0);
	set_carry(emu, 0);
	set_aux(emu, 0);
	set_sign(emu, res & 0x80000000);
	set_zero(emu, res == 0);
	set_parity(emu, PARITY(res & 0xff));
}

/* 0x0B /r */
static void or_r32_rm32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t r32 = get_r32(emu, &modrm);
	uint32_t rm32 = get_rm32(emu, &modrm);

	uint32_t res = r32 | rm32;
	set_r32(emu, &modrm, res);
	
	/* set flags */
	set_overflow(emu, 0);
	set_carry(emu, 0);
	set_aux(emu, 0);
	set_sign(emu, res & 0x80000000);
	set_zero(emu, res == 0);
	set_parity(emu, PARITY(res & 0xff));
}

/* 0x0D       id */
static void or_eax_imm32(Emulator* emu) {
	uint32_t value = get_code32(emu, 1);
	uint32_t eax = get_register32(emu, EAX);
	
	uint32_t res = eax | value;
	set_register32(emu, EAX, res);
	
	/* set flags */
	set_overflow(emu, 0);
	set_carry(emu, 0);
	set_aux(emu, 0);
	set_sign(emu, res & 0x80000000);
	set_zero(emu, res == 0);
	set_parity(emu, PARITY(res & 0xff));

	emu->eip += 5;
}

/* 0x0F AF /r */
static void imul_r32_rm32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t r32 = get_r32(emu, &modrm);
	uint32_t rm32 = get_rm32(emu, &modrm);
	
	uint64_t res = (uint64_t) (uint32_t)r32 * (uint32_t)rm32;
	uint32_t res_lo = (uint32_t)res;
	uint32_t res_hi = (uint32_t) (res >> 32);
	
	set_r32(emu, &modrm, (uint32_t)res_lo);
	
	if (res_hi != 0) {
		set_carry(emu, 1);
		set_overflow(emu, 1);
    } else {
    	set_carry(emu, 0);
    	set_overflow(emu, 0);
    }
}

static void code_0f(Emulator* emu) {
	emu->eip += 1;
	uint8_t second_code = get_code8(emu, 0);
	switch (second_code) {
		case 0xAF:
			imul_r32_rm32(emu);
			break;
		default:
			printf("Not implemented:0F%02X\n", second_code);
			system("pause");
	}
}

/* 0x21 */
static void and_rm32_r32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t rm32 = get_rm32(emu, &modrm);
	uint32_t r32 = get_r32(emu, &modrm);
	
	uint32_t res = rm32 & r32;
	set_rm32(emu, &modrm, res);
	
	/* set flags */
	set_overflow(emu, 0);
	set_carry(emu, 0);
	set_aux(emu, 0);
	set_sign(emu, res & 0x80000000);
	set_zero(emu, res == 0);
	set_parity(emu, PARITY(res & 0xff));
}

/* 0x23 */
static void and_r32_rm32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t r32 = get_r32(emu, &modrm);
	uint32_t rm32 = get_rm32(emu, &modrm);
		
	uint32_t res = r32 & rm32;
	set_r32(emu, &modrm, res);
	
	/* set flags */
	set_overflow(emu, 0);
	set_carry(emu, 0);
	set_aux(emu, 0);
	set_sign(emu, res & 0x80000000);
	set_zero(emu, res == 0);
	set_parity(emu, PARITY(res & 0xff));
}

/* 0x25       id */
static void and_eax_imm32(Emulator* emu) {
	uint32_t value = get_code32(emu, 1);
	uint32_t eax = get_register32(emu, EAX);
	
	uint32_t res = eax & value;
	set_register32(emu, EAX, res);
	
	/* set flags */
	set_overflow(emu, 0);
	set_carry(emu, 0);
	set_aux(emu, 0);
	set_sign(emu, res & 0x80000000);
	set_zero(emu, res == 0);
	set_parity(emu, PARITY(res & 0xff));
	
	emu->eip += 5;
}

/* 0x29 */
static void sub_rm32_r32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t rm32 = get_rm32(emu, &modrm);
	uint32_t r32 = get_r32(emu, &modrm);
	
	uint32_t res = rm32 - r32;
	set_rm32(emu, &modrm, res);
		
	set_sign(emu, res & 0x80000000);
	set_zero(emu, (res & 0xffffffff) == 0);
	set_parity(emu, PARITY(res & 0xff));

    /* calculate the borrow chain.  See note at top */
    register uint32_t bc = (res & (~rm32 | r32)) | (~rm32 & r32);
    set_carry(emu, bc & 0x80000000);
    set_overflow(emu, XOR2(bc >> 30));
    set_aux(emu, bc & 0x8);
}

/* 0x2B */
static void sub_r32_rm32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t rm32 = get_rm32(emu, &modrm);
	uint32_t r32 = get_r32(emu, &modrm);
	
	uint32_t res = r32 - rm32;
	set_r32(emu, &modrm, res);
		
	set_sign(emu, res & 0x80000000);
	set_zero(emu, (res & 0xffffffff) == 0);
	set_parity(emu, PARITY(res & 0xff));

    /* calculate the borrow chain.  See note at top */
    register uint32_t bc = (res & (~r32 | rm32)) | (~r32 & rm32);
    set_carry(emu, bc & 0x80000000);
    set_overflow(emu, XOR2(bc >> 30));
    set_aux(emu, bc & 0x8);
}

/* 0x2D */
static void sub_eax_imm32(Emulator* emu) {
	uint32_t value = get_code32(emu, 1);
	uint32_t eax = get_register32(emu, EAX);
	
	uint32_t res = eax - value;
	set_register32(emu, EAX, res);
		
	set_sign(emu, res & 0x80000000);
	set_zero(emu, (res & 0xffffffff) == 0);
	set_parity(emu, PARITY(res & 0xff));

    /* calculate the borrow chain.  See note at top */
    register uint32_t bc = (res & (~eax | value)) | (~eax & value);
    set_carry(emu, bc & 0x80000000);
    set_overflow(emu, XOR2(bc >> 30));
    set_aux(emu, bc & 0x8);
	
	emu->eip += 5;
}

/* 0x31 */
static void xor_rm32_r32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t rm32 = get_rm32(emu, &modrm);
	uint32_t r32 = get_r32(emu, &modrm);
		
	uint32_t res = rm32 ^ r32;
	set_rm32(emu, &modrm, res);
	
	set_overflow(emu, 0);
    set_sign(emu, res & 0x80000000);
    set_zero(emu, res == 0);
    set_parity(emu, PARITY(res & 0xff));
    set_carry(emu, 0);
    set_aux(emu, 0);
}

/* 0x33 */
static void xor_r32_rm32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t r32 = get_r32(emu, &modrm);
	uint32_t rm32 = get_rm32(emu, &modrm);
	
	uint32_t res = rm32 ^ r32;
	set_r32(emu, &modrm, res);
	
	set_overflow(emu, 0);
    set_sign(emu, res & 0x80000000);
    set_zero(emu, res == 0);
    set_parity(emu, PARITY(res & 0xff));
    set_carry(emu, 0);
    set_aux(emu, 0);
}

/* 0x35 */
static void xor_eax_imm32(Emulator* emu) {
	uint32_t value = get_code32(emu, 1);
	uint32_t eax = get_register32(emu, EAX);
		
	uint32_t res = eax ^ value;
	set_register32(emu, EAX, res);
	
	set_overflow(emu, 0);
    set_sign(emu, res & 0x80000000);
    set_zero(emu, res == 0);
    set_parity(emu, PARITY(res & 0xff));
    set_carry(emu, 0);
    set_aux(emu, 0);
	
	emu->eip += 5;
}

/* 0x3B */
static void cmp_r32_rm32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t r32 = get_r32(emu, &modrm);
	uint32_t rm32 = get_rm32(emu, &modrm);
	
	uint32_t res = r32 - rm32;
	
	set_sign(emu, res & 0x80000000);
	set_zero(emu, (res & 0xffffffff) == 0);
	set_parity(emu, PARITY(res & 0xff));

    /* calculate the borrow chain.  See note at top */
    register uint32_t bc = (res & (~r32 | rm32)) | (~r32 & rm32);
    set_carry(emu, bc & 0x80000000);
    set_overflow(emu, XOR2(bc >> 30));
    set_aux(emu, bc & 0x8);
}

/* 0x3C */
static void cmp_al_imm8(Emulator* emu) {
	uint8_t value = get_code8(emu, 1);
	uint8_t al = get_register8(emu, AL);
	
	register uint32_t res = al - value;
	/* calculate the borrow chain. */
    register uint32_t bc = (res & (~al | value)) | (~al & value);

    set_sign(emu, res & 0x80);
    set_zero(emu, (res & 0xff) == 0);
    set_parity(emu, PARITY(res & 0xff));
    set_carry(emu, bc & 0x80);
    set_overflow(emu, XOR2(bc >> 6));
    set_aux(emu, bc & 0x8);

	emu->eip += 2;
}

/* 0x3D */
static void cmp_eax_imm32(Emulator* emu) {
	uint32_t value = get_code32(emu, 1);
	uint32_t eax = get_register32(emu, EAX);
	
	uint32_t res = eax - value;
	
	set_sign(emu, res & 0x80000000);
	set_zero(emu, (res & 0xffffffff) == 0);
	set_parity(emu, PARITY(res & 0xff));

    /* calculate the borrow chain.  See note at top */
    register uint32_t bc = (res & (~eax | value)) | (~eax & value);
    set_carry(emu, bc & 0x80000000);
    set_overflow(emu, XOR2(bc >> 30));
    set_aux(emu, bc & 0x8);

	emu->eip += 5;
}

/* 0x40 /r */
static void inc_r32(Emulator* emu) {
	uint8_t reg = get_code8(emu, 0) - 0x40;
	
	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t register_value = get_register32(emu, reg);
		
		register uint32_t res = register_value + 1;
		/* calculate the carry chain */
		register uint32_t cc = (1 & register_value) | ((~res) & (1 | register_value));
	    	
		set_register32(emu, reg, res);
	
		set_zero(emu, (res & 0xffffffff) == 0);
		set_sign(emu, res & 0x80000000);
		set_parity(emu, PARITY(res & 0xff));
		set_overflow(emu, XOR2(cc >> 30));
		set_aux(emu, cc & 0x8);
	} else {	//16-bit mode registers mode active
		uint16_t register_value = get_register16(emu, reg);
		
		register uint32_t res = register_value + 1;
		/* calculate the carry chain */
		register uint32_t cc = (1 & register_value) | ((~res) & (1 | register_value));
	    
		set_register16(emu, reg, (uint16_t)res);
		
		set_zero(emu, (res & 0xffff) == 0);
		set_sign(emu, res & 0x8000);
		set_parity(emu, PARITY(res & 0xff));
		set_overflow(emu, XOR2(cc >> 14));
		set_aux(emu, cc & 0x8);
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;

	emu->eip += 1;
}

/* 0x50 /r */
static void push_r32(Emulator* emu) {
	uint8_t reg = get_code8(emu, 0) - 0x50;
	push32(emu, get_register32(emu, reg));
	emu->eip += 1;
}

/* 0x58 /r */
static void pop_r32(Emulator* emu) {
	uint8_t reg = get_code8(emu, 0) - 0x58;
	set_register32(emu, reg, pop32(emu));
	emu->eip += 1;
}

/* 0x66 */
static void opsize_mode(Emulator* emu) {
	emu->prefix_mode ^= PREFIX_OPSIZE_MODE_32_BIT;
	emu->eip += 1;
}

/* 0x67 */
static void address_mode(Emulator* emu) {
	emu->prefix_mode ^= PREFIX_ADDRESS_MODE_32_BIT;
	emu->eip += 1;
}

/* 0x68 */
static void push_imm32(Emulator* emu) {
	uint32_t value = get_code32(emu, 1);
	push32(emu, value);
	emu->eip += 5;
}

/* 0x69 / 2 */
static void imul_r32_r32_imm32(Emulator* emu, ModRM* modrm) {

	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t rm32 = get_rm32(emu, modrm); //register
		uint32_t imm32 = get_code32(emu, 0); //hardcoded value

		int64_t res = (int64_t) (uint32_t) rm32 * (uint32_t) imm32;
		uint32_t res_lo = (uint32_t) res;
		uint32_t res_hi = (uint32_t) (res >> 32);
		emu->eip += 4;
		set_r32(emu, modrm, res_lo);
		
		if(res_hi != 0) {
			set_carry(emu, 1);
			set_overflow(emu, 1);
		} else {
			set_carry(emu, 0);
			set_overflow(emu, 0);
		}
	} else {	//16-bit mode registers mode active
		uint32_t rm16 = get_rm16(emu, modrm); //register
		uint32_t imm16 = get_code16(emu, 0); //hardcoded value

		int32_t res = (int32_t) (uint16_t) rm16 * (uint16_t) imm16;
		uint16_t res_lo = (uint16_t) res;
		uint16_t res_hi = (uint16_t) (res >> 16);
		set_r16(emu, modrm, res_lo);
		
		if(res > 0xFFFF) {
			set_carry(emu, 1);
			set_overflow(emu, 1);
		} else {
			set_carry(emu, 0);
			set_overflow(emu, 0);
		}
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0x69 */
static void imul_r32_rm32_imm32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);

	switch (modrm.mod) {
		case 0:
			//IMUL r32,imm32 ?
			printf("not implemented: 69/0 \n");
			system("pause");
			break;
		case 1:
			// IMUL r32,imm32 ?
			printf("not implemented: 69/1 \n");
			system("pause");
			break;
		case 2:
			//IMUL r32,imm32  ?
			printf("not implemented: 69/2\n");
			system("pause");
			break;
		case 3:
			//IMUL r16,rm16,imm16
			/* register to register */
			imul_r32_r32_imm32(emu, &modrm);
			break;
		default:
			printf("not implemented: 69/default\n");
			system("pause");
	}
}

/* 0x6A */
static void push_imm8(Emulator* emu) {
	uint8_t value = get_code8(emu, 1);
	push32(emu, value);
	emu->eip += 2;
}

#define DEFINE_JX(flag, is_flag) \
	static void j ## flag(Emulator* emu) \
	{ \
		int diff = is_flag(emu) ? get_sign_code8(emu, 1) : 0; \
		emu->eip += (diff + 2); \
	} \
	static void jn ## flag(Emulator* emu) \
	{ \
		int diff = is_flag(emu) ? 0 : get_sign_code8(emu, 1); \
		emu->eip += (diff + 2); \
	}

DEFINE_JX(c, is_carry)
DEFINE_JX(z, is_zero)
DEFINE_JX(s, is_sign)
DEFINE_JX(o, is_overflow)

#undef DEFINE_JX

static void jl(Emulator* emu) {
	int diff = (is_sign(emu) != is_overflow(emu)) ? get_sign_code8(emu, 1) : 0;
	emu->eip += (diff + 2);
}

static void jle(Emulator* emu) {
	int diff = (is_zero(emu) || (is_sign(emu) != is_overflow(emu))) ? get_sign_code8(emu, 1) : 0;
	emu->eip += (diff + 2);
}

static void jg(Emulator* emu) {
	int diff = (!is_zero(emu) && is_sign(emu) == is_overflow(emu)) ? get_sign_code8(emu, 1) : 0;
	emu->eip += (diff + 2);
}

/* 0x81 /0 */
static void add_rm32_imm32(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm32 = get_code32(emu, 0);
	
	
	register uint32_t lo = (rm32 & 0xFFFF) + (imm32 & 0xFFFF);
	register uint32_t hi = (lo >> 16) + (rm32 >> 16) + (imm32 >> 16);
	register uint32_t res = rm32 + imm32;
	/* calculate the carry chain. */
	register uint32_t cc = (imm32 & rm32) | ((~res) & (imm32 | rm32));
	
	set_rm32(emu, modrm, res);
	
	set_carry(emu, hi & 0x10000);
    set_zero(emu, (res & 0xffffffff) == 0);
    set_sign(emu, res & 0x80000000);
    set_parity(emu, PARITY(res & 0xff));
    set_overflow(emu, XOR2(cc >> 30));
    set_aux(emu, cc & 0x8);
	
	emu->eip += 4;
}

/* 0x81 /1 */
static void or_rm32_imm32(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm32 = get_code32(emu, 0);
	
	uint32_t res = rm32 | imm32;
	set_rm32(emu, modrm, res);
	
	/* set flags */
	set_overflow(emu, 0);
	set_carry(emu, 0);
	set_aux(emu, 0);
	set_sign(emu, res & 0x80000000);
	set_zero(emu, res == 0);
	set_parity(emu, PARITY(res & 0xff));
	
	emu->eip += 4;
}

/* 0x81 /2 */
static void adc_rm32_imm32(Emulator* emu, ModRM* modrm) {
	printf("not implemented: 81/2 /%d\n");
}

/* 0x81 /3 */
static void sbb_rm32_imm32(Emulator* emu, ModRM* modrm) {
	printf("not implemented: 81/3 /%d\n");
}

/* 0x81 /4 */
static void and_rm32_imm32(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm32 = get_code32(emu, 0);
	
	uint32_t res = rm32 & imm32;
	set_rm32(emu, modrm, res);
	
	/* set flags */
	set_overflow(emu, 0);
	set_carry(emu, 0);
	set_aux(emu, 0);
	set_sign(emu, res & 0x80000000);
	set_zero(emu, res == 0);
	set_parity(emu, PARITY(res & 0xff));
	
	emu->eip += 4;
}

/* 0x81 /5 */
static void sub_rm32_imm32(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm32 = get_code32(emu, 0);
	
	uint32_t res = rm32 - imm32;
	set_rm32(emu, modrm, res);
		
	set_sign(emu, res & 0x80000000);
	set_zero(emu, (res & 0xffffffff) == 0);
	set_parity(emu, PARITY(res & 0xff));

    /* calculate the borrow chain.  See note at top */
    register uint32_t bc = (res & (~rm32 | imm32)) | (~rm32 & imm32);
    set_carry(emu, bc & 0x80000000);
    set_overflow(emu, XOR2(bc >> 30));
    set_aux(emu, bc & 0x8);
	
	emu->eip += 4;
}

/* 0x81 /6 */
static void xor_rm32_imm32(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm32 = get_code32(emu, 0);

	uint32_t res = rm32 ^ imm32;
	set_rm32(emu, modrm, res);
	
	set_overflow(emu, 0);
    set_sign(emu, res & 0x80000000);
    set_zero(emu, res == 0);
    set_parity(emu, PARITY(res & 0xff));
    set_carry(emu, 0);
    set_aux(emu, 0);
	
	emu->eip += 4;
}

/* 0x81 /7 */
static void cmp_rm32_imm32(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm32 = get_code32(emu, 0);
	emu->eip += 1;
	uint32_t res = rm32 - imm32;
	
	
	set_sign(emu, res & 0x80000000);
	set_zero(emu, (res & 0xffffffff) == 0);
	set_parity(emu, PARITY(res & 0xff));

    /* calculate the borrow chain.  See note at top */
    register uint32_t bc = (res & (~rm32 | imm32)) | (~rm32 & imm32);
    set_carry(emu, bc & 0x80000000);
    set_overflow(emu, XOR2(bc >> 30));
    set_aux(emu, bc & 0x8);
}

/* 0x81 */
static void code_81(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);

	switch (modrm.opcode) {
		case 0:
			add_rm32_imm32(emu, &modrm); //ADD
			break;
		case 1:
			or_rm32_imm32(emu, &modrm); //OR
			break;
		case 2:
			adc_rm32_imm32(emu, &modrm); //ADC
			break;
		case 3:
			sbb_rm32_imm32(emu, &modrm); //SBB
			break;
		case 4:
			and_rm32_imm32(emu, &modrm); //AND
			break;
		case 5:
			sub_rm32_imm32(emu, &modrm); //SUB
			break;
		case 6:
			xor_rm32_imm32(emu, &modrm); //XOR
			break;
		case 7:
			cmp_rm32_imm32(emu, &modrm); //CMP
			break;
		default:
			printf("not implemented: 81 /%d\n", modrm.opcode);
			system("pause");
	}
}

/* 0x83 /0 */
static void add_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	int32_t imm8 = (int32_t)get_sign_code8(emu, 0);

	register uint32_t lo = (rm32 & 0xFFFF) + (imm8 & 0xFFFF);
	register uint32_t hi = (lo >> 16) + (rm32 >> 16) + (imm8 >> 16);
	register uint32_t res = rm32 + imm8;
	/* calculate the carry chain. */
	register uint32_t cc = (imm8 & rm32) | ((~res) & (imm8 | rm32));
	
	set_rm32(emu, modrm, res);
	
	set_carry(emu, hi & 0x10000);
    set_zero(emu, (res & 0xffffffff) == 0);
    set_sign(emu, res & 0x80000000);
    set_parity(emu, PARITY(res & 0xff));
    set_overflow(emu, XOR2(cc >> 30));
    set_aux(emu, cc & 0x8);
	
	emu->eip += 1;
}

/* 0x83 /1 */
static void or_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	int32_t imm8 = (int32_t)get_sign_code8(emu, 0);

	uint32_t res = rm32 | imm8;
	set_rm32(emu, modrm, res);
	
	/* set flags */
	set_overflow(emu, 0);
	set_carry(emu, 0);
	set_aux(emu, 0);
	set_sign(emu, res & 0x80000000);
	set_zero(emu, res == 0);
	set_parity(emu, PARITY(res & 0xff));
	
	emu->eip += 1;	
}

/* 0x83 /4 */
static void and_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	int32_t imm8 = (int32_t)get_sign_code8(emu, 0);
	
	uint32_t res = rm32 & imm8;
	set_rm32(emu, modrm, res);
	
	/* set flags */
	set_overflow(emu, 0);
	set_carry(emu, 0);
	set_aux(emu, 0);
	set_sign(emu, res & 0x80000000);
	set_zero(emu, res == 0);
	set_parity(emu, PARITY(res & 0xff));
	
	emu->eip += 1;
}

/* 0x83 /5 */
static void sub_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	int32_t imm8 = (int32_t)get_sign_code8(emu, 0);
	emu->eip += 1;
	uint32_t res = rm32 - imm8;
	set_rm32(emu, modrm, res);
		
	set_sign(emu, res & 0x80000000);
	set_zero(emu, (res & 0xffffffff) == 0);
	set_parity(emu, PARITY(res & 0xff));

    /* calculate the borrow chain.  See note at top */
    register uint32_t bc = (res & (~rm32 | imm8)) | (~rm32 & imm8);
    set_carry(emu, bc & 0x80000000);
    set_overflow(emu, XOR2(bc >> 30));
    set_aux(emu, bc & 0x8);	
	
}

/* 0x83 /6 */
static void xor_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	int32_t imm8 = (int32_t)get_sign_code8(emu, 0);
	
	uint32_t res = rm32 ^ imm8;
	set_rm32(emu, modrm, res);
	
	set_overflow(emu, 0);
    set_sign(emu, res & 0x80000000);
    set_zero(emu, res == 0);
    set_parity(emu, PARITY(res & 0xff));
    set_carry(emu, 0);
    set_aux(emu, 0);
	
	emu->eip += 1;
}

/* 0x83 /7 */
static void cmp_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	int32_t imm8 = (int32_t)get_sign_code8(emu, 0);
	emu->eip += 1;
	uint32_t res = rm32 - imm8;
	
	
	set_sign(emu, res & 0x80000000);
	set_zero(emu, (res & 0xffffffff) == 0);
	set_parity(emu, PARITY(res & 0xff));

    /* calculate the borrow chain.  See note at top */
    register uint32_t bc = (res & (~rm32 | imm8)) | (~rm32 & imm8);
    set_carry(emu, bc & 0x80000000);
    set_overflow(emu, XOR2(bc >> 30));
    set_aux(emu, bc & 0x8);	
}

static void code_83(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);

	switch (modrm.opcode) {
		case 0:
			add_rm32_imm8(emu, &modrm);
			break;
		case 1:
			or_rm32_imm8(emu, &modrm);
			break;
		case 4:
			and_rm32_imm8(emu, &modrm);
			break;
		case 5:
			sub_rm32_imm8(emu, &modrm);
			break;
		case 6:
			xor_rm32_imm8(emu, &modrm);
			break;
		case 7:
			cmp_rm32_imm8(emu, &modrm);
			break;
		default:
			printf("not implemented: 83 /%d\n", modrm.opcode);
			system("pause");
	}
}

/* 0x85 */
static void test_rm32_r32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t r32 = get_r32(emu, &modrm);
	uint32_t rm32 = get_rm32(emu, &modrm);
	
	uint32_t res;
	res = rm32 & r32;
	set_overflow(emu, 0); //clear
	set_sign(emu, res & 0x80000000);
	set_zero(emu, res == 0);
	set_parity(emu, PARITY(res & 0xff));
	set_carry(emu, 0); //clear
}

/* 0x88 */
static void mov_rm8_r8(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t r8 = get_r8(emu, &modrm);
	set_rm8(emu, &modrm, r8);
}

/* 0x89 */
static void mov_rm32_r32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t r32 = get_r32(emu, &modrm);
	set_rm32(emu, &modrm, r32);
}

/* 0x8A */
static void mov_r8_rm8(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t rm8 = get_rm8(emu, &modrm);
	set_r8(emu, &modrm, rm8);
}

/* 0x8B */
static void mov_r32_rm32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, !(emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT));
	
	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t rm32 = get_rm32(emu, &modrm);
		set_r32(emu, &modrm, rm32);
	} else {	//16-bit mode registers mode active
		uint32_t rm16 = get_rm16(emu, &modrm);
		set_r16(emu, &modrm, rm16);
	}
		
	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0x8D /r */
static void lea_r16_r32_m(Emulator* emu) {
	emu->eip += 1;
	uint32_t code_value = get_code8(emu, 0);
	ModRM modrm;
	parse_modrm(emu, &modrm, false);

	uint32_t address;
	address = calc_memory_address32(emu, &modrm);
	set_r32(emu, &modrm, address);
}

/* 0x90 */
static void nop(Emulator* emu) {
	emu->eip += 1;
}

/* 0xAB */
static void stos(Emulator* emu) {
	emu->eip += 1;

	ModRM modrm;

	int32_t incrementer;
	uint32_t count = 1;
	uint32_t value;

	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		//stosd (DWORD)
		incrementer = is_direction(emu) ? -4 : 4;

		if( emu->prefix_mode & PREFIX_REP ) {
			count = get_register32(emu, ECX);
			set_register32(emu, ECX, 0);
			emu->prefix_mode &= ~(PREFIX_REP | PREFIX_REPE);
		}

		value = get_register32(emu, EAX);
		uint32_t address = get_register32(emu, EDI);

		while(count--) {
			set_memory32(emu, address, value);
			address += incrementer;
			set_register32(emu, EDI, address);
		}
	} else { //16-bit mode registers mode active
		//stosw (WORD)
		incrementer = is_direction(emu) ? -2 : 2;

		if( emu->prefix_mode & PREFIX_REP ) {
			count = get_register16(emu, CX);
			set_register16(emu, CX, 0);
			emu->prefix_mode &= ~(PREFIX_REP | PREFIX_REPE);
		}

		value = get_register16(emu, AX);
		uint32_t address = get_register32(emu, EDI); //i dont know.. 16 bit this not good ^.^

		while(count--) {
			set_memory16(emu, address, value);
			address += incrementer;
			set_register32(emu, EDI, address);
		}
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0xB0 /r */
static void mov_r8_imm8(Emulator* emu) {
	uint8_t reg = get_code8(emu, 0) - 0xB0;
	set_register8(emu, reg, get_code8(emu, 1));
	emu->eip += 2;
}

/* 0xB8 /r */
static void mov_r32_imm32(Emulator* emu) {
	uint8_t reg = get_code8(emu, 0) - 0xB8;
	uint32_t value = get_code32(emu, 1);
	set_register32(emu, reg, value);
	emu->eip += 5;
}

/* 0xC1 /0 */
static void rol_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm8 = (int32_t)get_sign_code8(emu, 0);

	imm8 %= 32;
	rm32 = rm32 << imm8 | rm32 >> (32 - imm8);

	//overflow flag is set if imm8 == 1
	if(imm8 == 1)
		set_overflow(emu, (rm32 + (rm32 >> 31)) & 1);

	/* set new CF; note that it is the LSB of the result */
	set_carry(emu, rm32 & 0x1);

	//TODO: FIX for all bit types later
	set_rm32(emu, modrm, rm32);
	emu->eip += 1;
}

/* 0xC1 /1 */
static void ror_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm8 = (int32_t)get_sign_code8(emu, 0);

	imm8 %= 32;
	rm32 = rm32 << (32 - imm8) | rm32 >> (imm8);

	if(imm8 == 1)
		set_overflow(emu, (((rm32 >> 30) ^ ((rm32 >> 30)>>1)) & 0x1));

	/* set new CF; note that it is the MSB of the result */
	set_carry(emu, rm32 & (1 << 31));

	//TODO: FIX for all bit types later
	set_rm32(emu, modrm, rm32);
	emu->eip += 1;
}

/* 0xC1 /2 */
static void rcl_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm8 = (int32_t)get_sign_code8(emu, 0);

	register uint32_t res, cnt, mask, cf;

	if ((imm8 = imm8 % 33) != 0) {
		cf = (rm32 >> (32 - imm8)) & 0x1;
		rm32 = (rm32 << imm8) & 0xffffffff;
		mask = (1 << (imm8 - 1)) - 1;
		rm32 |= (rm32 >> (33 - imm8)) & mask;
		if (is_carry(emu)) {     /* carry flag is set */
			rm32 |= 1 << (imm8 - 1);
		}
		set_carry(emu, cf);
		set_overflow(emu, (imm8 == 1 && (((cf + ((rm32 >> 30) & 0x2)) ^ ((cf + ((rm32 >> 30) & 0x2))>>1)) & 0x1)));
	}

	//TODO: FIX for all bit types later
	set_rm32(emu, modrm, rm32);
	emu->eip += 1;
}

/* 0xC1 /3 */
static void rcr_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm8 = (int32_t)get_sign_code8(emu, 0);

	uint32_t mask, cf, ocf = 0;

	/* rotate right through carry */
	if ((imm8 = imm8 % 33) != 0) {
		if (imm8 == 1) {
			cf = rm32 & 0x1;
			ocf = is_carry(emu) != 0;
		} else
			cf = (rm32 >> (imm8 - 1)) & 0x1;
		mask = (1 << (32 - imm8)) - 1;
		rm32 = (rm32 >> imm8) & mask;
		if (imm8 != 1)
			rm32 |= (rm32 << (33 - imm8));
		if (is_carry(emu)) {     /* carry flag is set */
			rm32 |= 1 << (32 - imm8);
		}
		set_carry(emu, cf);
		if (imm8 == 1) {
			set_overflow(emu, (((ocf + ((rm32 >> 30) & 0x2)) ^ ((ocf + ((rm32 >> 30) & 0x2))>>1)) & 0x1));
		}
	}

	//TODO: FIX for all bit types later
	set_rm32(emu, modrm, rm32);
	emu->eip += 1;
}

/* 0xC1 /4 */
static void shl_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value
	uint32_t imm8 = (int32_t)get_sign_code8(emu, 0); //0x10 [16] shift amount
	uint32_t original_rm32 = rm32;

	imm8 %= 32;
	if(imm8 > 0) {
		rm32 = rm32 << imm8;
		set_carry(emu, (rm32 & (1 << (32 - imm8))));
		set_zero(emu, (rm32 & 0xffffffff) == 0);
		set_sign(emu, rm32 & 0x80000000);
		set_parity(emu, PARITY(rm32 & 0xff));
	} else {
		rm32 = 0;
	}
	if(imm8 == 1)
		set_overflow(emu, (((rm32 & 0x80000000) == 0x80000000) ^
                                  (is_carry(emu) != 0)));
	emu->eip += 1;
	set_rm32(emu, modrm, rm32);
}

/* 0xC1 /5 */
static void shr_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value
	uint32_t imm8 = (int32_t)get_sign_code8(emu, 0); //0x10 [16] shift amount
	uint32_t original_rm32 = rm32;

	imm8 %= 32;
	if(imm8 > 0) {
		rm32 = rm32 >> imm8;
		set_carry(emu, (rm32 & (1 << (imm8 - 1))));
		set_zero(emu, (rm32 & 0xffffffff) == 0);
		set_sign(emu, rm32 & 0x80000000);
		set_parity(emu, PARITY(rm32 & 0xff));
	}
	
	if(imm8 == 1)
		set_overflow(emu, XOR2(rm32 >> 30));
	else
		set_overflow(emu, 0);

	emu->eip += 1;
	set_rm32(emu, modrm, rm32);
}

/* 0xC1 /7 */
static void sar_rm32_imm8(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value
	uint32_t imm8 = (int32_t)get_sign_code8(emu, 0); //0x10 [16] shift amount
	uint32_t original_rm32 = rm32;
	imm8 %= 32;
	uint32_t sign_flag = rm32 & 0x80000000;
	if(imm8 > 0 && imm8 < 32) {
		uint32_t mask = (1 << (32 - imm8)) - 1;
		uint32_t carry_flag = rm32 & (1 << (imm8 - 1));
	
		rm32 = (rm32 >> imm8) & mask;
		set_carry(emu, carry_flag);
	
		if(sign_flag)
			rm32 |= ~mask;
	
		set_zero(emu, (rm32 & 0xffffffff) == 0);
		set_sign(emu, (rm32 & 0x80000000));
		set_parity(emu, PARITY(rm32 & 0xff));
	
		emu->eip += 1;
		set_rm32(emu, modrm, rm32);
	} else if(imm8 >= 32) {
		/* seems impossible but idk maybe possible??? */
		if(sign_flag) {
			set_rm32(emu, modrm, 0xffffffff);
			set_carry(emu, 1);
			set_zero(emu, 0);
			set_sign(emu, 1);
			set_parity(emu, 1);
		} else {
			set_rm32(emu, modrm, 0);
			set_carry(emu, 0);
			set_zero(emu, 1);
			set_sign(emu, 0);
			set_parity(emu, 0);	
		}
	}
}

/* 0xC1 */
static void code_c1(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);

	switch (modrm.opcode) {
		case 0:
			rol_rm32_imm8(emu, &modrm); //ROL
			break;
		case 1:
			ror_rm32_imm8(emu, &modrm); //ROR
			break;
		case 2:
			rcl_rm32_imm8(emu, &modrm); //RCL
			break;
		case 3:
			rcr_rm32_imm8(emu, &modrm); //RCR
			break;
		case 4:
			shl_rm32_imm8(emu, &modrm); //SAL / SHL
			break;
		case 5:
			shr_rm32_imm8(emu, &modrm); //SHR
			break;
		case 7:
			sar_rm32_imm8(emu, &modrm); //SAR
			break;
		default:
			printf("not implemented: c1 /%d\n", modrm.opcode);
			system("pause");
	}
}

/* 0xC3 */
static void ret(Emulator* emu) {
	emu->eip = pop32(emu);
}

/* 0xC7 */
static void mov_rm32_imm32(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);
	uint32_t value = get_code32(emu, 0);
	emu->eip += 4;
	set_rm32(emu, &modrm, value);
}

/* 0xC9 */
static void leave(Emulator* emu) {
	uint32_t ebp = get_register32(emu, EBP);
	set_register32(emu, ESP, ebp);
	set_register32(emu, EBP, pop32(emu));
	emu->eip += 1;
}

/* 0xCD */
static void swi(Emulator* emu) {
	uint8_t int_index = get_code8(emu, 1);
	emu->eip += 2;

	switch (int_index) {
		case 0x10:
			//bios_video(emu);
			break;
		default:
			printf("unknown interrupt: 0x%02x\n", int_index);
	}
}

/* 0xD1 / 0 */
static void rol_rm32_1(Emulator* emu, ModRM* modrm) {
	uint8_t distance = 1;
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value

	register unsigned int res, cnt;

	/*
	  rotate left
	   value is the rotate distance.  It varies from 0 - 8.
	   EDX is the byte object rotated.

	   have 7 to 0
	   The rotate is done mod 8.
	 */
	res = rm32;
	if ((cnt = distance % 8) != 0) {
       rm32 = (rm32 << cnt);
       rm32 |= (rm32 >> (8-cnt)) & (1 << cnt) - 1;
       
		/* Carry flag set if there is a carry on the calculation results */
		set_carry(emu, rm32 & 0x1);
		set_overflow(emu, (rm32 + (rm32 >> 7)) & 1);
	} else if (distance != 0) {
		/* Carry flag set if there is a carry on the calculation results */
		set_carry(emu, res & 0x80);
	}

	set_rm32(emu, modrm, res);
}

/* 0xD1 / 1 */
static void ror_rm32_1(Emulator* emu, ModRM* modrm) {
	uint8_t distance = 1;
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value

	register unsigned int res, cnt;

	/*
	  rotate right
	   value is the rotate distance.  It varies from 0 - 8.
	   EDX is the byte object rotated.

	   have 7 to 0
	   The rotate is done mod 8.
	 */
	res = rm32;
	if ((cnt = distance % 8) != 0) {
		res = (rm32 << (8 - cnt));
		res |= (rm32 >> (cnt)) & ((1 << (8 - cnt)) - 1);

		/* Carry flag set if there is a carry on the calculation results */
		set_carry(emu, res & 0x80);

		/* OVERFLOW is set *IFF* s==1, then it is the xor of the two most significant bits.  Blecck. */
		set_overflow(emu, distance == 1 && ((res >> 6) ^ (res >> 6) & 0x1));
	} else if (distance != 0) {
		/* Carry flag set if there is a carry on the calculation results */
		set_carry(emu, res & 0x80);
	}

	set_rm32(emu, modrm, res);
}

/* 0xd1 /2 */
static void rcl_rm32_1(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm8 = 1;

	register uint32_t res, cnt, mask, cf;

	if ((imm8 = imm8 % 33) != 0) {
		cf = (rm32 >> (32 - imm8)) & 0x1;
		rm32 = (rm32 << imm8) & 0xffffffff;
		mask = (1 << (imm8 - 1)) - 1;
		rm32 |= (rm32 >> (33 - imm8)) & mask;
		if (is_carry(emu)) {     /* carry flag is set */
			rm32 |= 1 << (imm8 - 1);
		}
		set_carry(emu, cf);
		set_overflow(emu, (imm8 == 1 && (((cf + ((rm32 >> 30) & 0x2)) ^ ((cf + ((rm32 >> 30) & 0x2))>>1)) & 0x1)));
	}

	//TODO: FIX for all bit types later
	set_rm32(emu, modrm, rm32);
	emu->eip += 1;
}

/* 0xd1 /3 */
static void rcr_rm32_1(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm8 = 1;

	uint32_t mask, cf, ocf = 0;

	/* rotate right through carry */
	if ((imm8 = imm8 % 33) != 0) {
		if (imm8 == 1) {
			cf = rm32 & 0x1;
			ocf = is_carry(emu) != 0;
		} else
			cf = (rm32 >> (imm8 - 1)) & 0x1;
		mask = (1 << (32 - imm8)) - 1;
		rm32 = (rm32 >> imm8) & mask;
		if (imm8 != 1)
			rm32 |= (rm32 << (33 - imm8));
		if (is_carry(emu)) {     /* carry flag is set */
			rm32 |= 1 << (32 - imm8);
		}
		set_carry(emu, cf);
		if (imm8 == 1) {
			set_overflow(emu, (((ocf + ((rm32 >> 30) & 0x2)) ^ ((ocf + ((rm32 >> 30) & 0x2))>>1)) & 0x1));
		}
	}

	//TODO: FIX for all bit types later
	set_rm32(emu, modrm, rm32);
	emu->eip += 1;
}

/* 0xd1 /4 */
static void shl_rm32_1(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value
	uint32_t imm8 = 1; //0x10 [16] shift amount
	uint32_t original_rm32 = rm32;

	imm8 %= 32;
	if(imm8 > 0) {
		rm32 = rm32 << imm8;
		set_carry(emu, (rm32 & (1 << (32 - imm8))));
		set_zero(emu, (rm32 & 0xffffffff) == 0);
		set_sign(emu, rm32 & 0x80000000);
		set_parity(emu, PARITY(rm32 & 0xff));
	} else {
		rm32 = 0;
	}
	if(imm8 == 1)
		set_overflow(emu, (((rm32 & 0x80000000) == 0x80000000) ^
                                  (is_carry(emu) != 0)));
	emu->eip += 1;
	set_rm32(emu, modrm, rm32);
}

/* 0xd1 /5 */
static void shr_rm32_1(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value
	uint32_t imm8 = 1; //0x10 [16] shift amount
	uint32_t original_rm32 = rm32;

	imm8 %= 32;
	if(imm8 > 0) {
		rm32 = rm32 >> imm8;
		set_carry(emu, (rm32 & (1 << (imm8 - 1))));
		set_zero(emu, (rm32 & 0xffffffff) == 0);
		set_sign(emu, rm32 & 0x80000000);
		set_parity(emu, PARITY(rm32 & 0xff));
	}
	
	if(imm8 == 1)
		set_overflow(emu, XOR2(rm32 >> 30));
	else
		set_overflow(emu, 0);

	emu->eip += 1;
	set_rm32(emu, modrm, rm32);
}

/* 0xd1 /7 */
static void sar_rm32_1(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value
	uint32_t imm8 = 1; //0x10 [16] shift amount
	uint32_t original_rm32 = rm32;
	imm8 %= 32;
	uint32_t sign_flag = rm32 & 0x80000000;
	uint32_t mask = (1 << (32 - imm8)) - 1;
	uint32_t carry_flag = rm32 & (1 << (imm8 - 1));

	rm32 = (rm32 >> imm8) & mask;
	set_carry(emu, carry_flag);

	if(sign_flag)
		rm32 |= ~mask;

	set_zero(emu, (rm32 & 0xffffffff) == 0);
	set_sign(emu, (rm32 & 0x80000000));
	set_parity(emu, PARITY(rm32 & 0xff));

	emu->eip += 1;
	set_rm32(emu, modrm, rm32);
}

/* 0xD1 */
static void code_d1(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);

	switch (modrm.opcode) {
		case 0:
			rol_rm32_1(emu, &modrm); //ROL
			break;
		case 1:
			ror_rm32_1(emu, &modrm); //ROR
			break;
		case 2:
			rcl_rm32_1(emu, &modrm); //ROR
			break;
		case 3:
			rcr_rm32_1(emu, &modrm); //ROR
			break;
		case 4:
			shl_rm32_1(emu, &modrm); //ROR
			break;
		case 5:
			shr_rm32_1(emu, &modrm); //ROR
			break;
		case 7:
			sar_rm32_1(emu, &modrm); //ROR
			break;			
		default:
			printf("not implemented: d1 /%d\n", modrm.opcode);
			system("pause");
	}
}

/* 0xD3 / 0 */
static void rol_rm32_cl(Emulator* emu, ModRM* modrm) {
	uint8_t distance = get_register8(emu, CL);
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value

	register unsigned int res, cnt;

	/*
	  rotate left
	   value is the rotate distance.  It varies from 0 - 8.
	   EDX is the byte object rotated.

	   have 7 to 0
	   The rotate is done mod 8.
	 */
	res = rm32;
	if ((cnt = distance % 8) != 0) {
       rm32 = (rm32 << cnt);
       rm32 |= (rm32 >> (8-cnt)) & (1 << cnt) - 1;
       
		/* Carry flag set if there is a carry on the calculation results */
		set_carry(emu, rm32 & 0x1);
		set_overflow(emu, (rm32 + (rm32 >> 7)) & 1);
	} else if (distance != 0) {
		/* Carry flag set if there is a carry on the calculation results */
		set_carry(emu, res & 0x80);
	}

	set_rm32(emu, modrm, res);
}

/* 0xD3 / 1 */
static void ror_rm32_cl(Emulator* emu, ModRM* modrm) {
	uint8_t distance = get_register8(emu, CL);
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value

	register unsigned int res, cnt;

	/*
	  rotate right
	   value is the rotate distance.  It varies from 0 - 8.
	   EDX is the byte object rotated.

	   have 7 to 0
	   The rotate is done mod 8.
	 */
	res = rm32;
	if ((cnt = distance % 8) != 0) {
		res = (rm32 << (8 - cnt));
		res |= (rm32 >> (cnt)) & ((1 << (8 - cnt)) - 1);

		/* Carry flag set if there is a carry on the calculation results */
		set_carry(emu, res & 0x80);

		/* OVERFLOW is set *IFF* s==1, then it is the xor of the two most significant bits.  Blecck. */
		set_overflow(emu, distance == 1 && ((res >> 6) ^ (res >> 6) & 0x1));
	} else if (distance != 0) {
		/* Carry flag set if there is a carry on the calculation results */
		set_carry(emu, res & 0x80);
	}

	set_rm32(emu, modrm, res);
}

/* 0xd3 /2 */
static void rcl_rm32_cl(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm8 = get_register8(emu, CL);

	register uint32_t res, cnt, mask, cf;

	if ((imm8 = imm8 % 33) != 0) {
		cf = (rm32 >> (32 - imm8)) & 0x1;
		rm32 = (rm32 << imm8) & 0xffffffff;
		mask = (1 << (imm8 - 1)) - 1;
		rm32 |= (rm32 >> (33 - imm8)) & mask;
		if (is_carry(emu)) {     /* carry flag is set */
			rm32 |= 1 << (imm8 - 1);
		}
		set_carry(emu, cf);
		set_overflow(emu, (imm8 == 1 && (((cf + ((rm32 >> 30) & 0x2)) ^ ((cf + ((rm32 >> 30) & 0x2))>>1)) & 0x1)));
	}

	//TODO: FIX for all bit types later
	set_rm32(emu, modrm, rm32);
	emu->eip += 1;
}

/* 0xd3 /3 */
static void rcr_rm32_cl(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	uint32_t imm8 = get_register8(emu, CL);

	uint32_t mask, cf, ocf = 0;

	/* rotate right through carry */
	if ((imm8 = imm8 % 33) != 0) {
		if (imm8 == 1) {
			cf = rm32 & 0x1;
			ocf = is_carry(emu) != 0;
		} else
			cf = (rm32 >> (imm8 - 1)) & 0x1;
		mask = (1 << (32 - imm8)) - 1;
		rm32 = (rm32 >> imm8) & mask;
		if (imm8 != 1)
			rm32 |= (rm32 << (33 - imm8));
		if (is_carry(emu)) {     /* carry flag is set */
			rm32 |= 1 << (32 - imm8);
		}
		set_carry(emu, cf);
		if (imm8 == 1) {
			set_overflow(emu, (((ocf + ((rm32 >> 30) & 0x2)) ^ ((ocf + ((rm32 >> 30) & 0x2))>>1)) & 0x1));
		}
	}

	//TODO: FIX for all bit types later
	set_rm32(emu, modrm, rm32);
	emu->eip += 1;
}

/* 0xd3 /4 */
static void shl_rm32_cl(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value
	uint32_t imm8 = get_register8(emu, CL); //0x10 [16] shift amount
	uint32_t original_rm32 = rm32;

	imm8 %= 32;
	if(imm8 > 0) {
		rm32 = rm32 << imm8;
		set_carry(emu, (rm32 & (1 << (32 - imm8))));
		set_zero(emu, (rm32 & 0xffffffff) == 0);
		set_sign(emu, rm32 & 0x80000000);
		set_parity(emu, PARITY(rm32 & 0xff));
	} else {
		rm32 = 0;
	}
	if(imm8 == 1)
		set_overflow(emu, (((rm32 & 0x80000000) == 0x80000000) ^
                                  (is_carry(emu) != 0)));
	emu->eip += 1;
	set_rm32(emu, modrm, rm32);
}

/* 0xd3 /5 */
static void shr_rm32_cl(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value
	uint32_t imm8 = get_register8(emu, CL); //0x10 [16] shift amount
	uint32_t original_rm32 = rm32;

	imm8 %= 32;
	if(imm8 > 0) {
		rm32 = rm32 >> imm8;
		
		set_carry(emu, (rm32 & (1 << (imm8 - 1))));
		set_zero(emu, (rm32 & 0xffffffff) == 0);
		set_sign(emu, rm32 & 0x80000000);
		set_parity(emu, PARITY(rm32 & 0xff));
	}
	
	if(imm8 == 1)
		set_overflow(emu, XOR2(rm32 >> 30));
	else
		set_overflow(emu, 0);
	
	emu->eip += 1;
	set_rm32(emu, modrm, rm32);
}

/* 0xd3 /7 */
static void sar_rm32_cl(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm); //eax register value
	uint32_t imm8 = get_register8(emu, CL); //0x10 [16] shift amount
	uint32_t original_rm32 = rm32;
	imm8 %= 32;
	uint32_t sign_flag = rm32 & 0x80000000;
	
	if(imm8 > 0 && imm8 < 32) {	
		
		uint32_t mask = (1 << (32 - imm8)) - 1;
		uint32_t carry_flag = rm32 & (1 << (imm8 - 1));
	
		rm32 = (rm32 >> imm8) & mask;
		set_carry(emu, carry_flag);
	
		if(sign_flag)
			rm32 |= ~mask;
	
		set_zero(emu, (rm32 & 0xffffffff) == 0);
		set_sign(emu, (rm32 & 0x80000000));
		set_parity(emu, PARITY(rm32 & 0xff));
		
		set_rm32(emu, modrm, rm32);
	} else if(imm8 >= 32) {
		/* seems impossible but idk maybe possible??? */
		if(sign_flag) {
			set_rm32(emu, modrm, 0xffffffff);
			set_carry(emu, 1);
			set_zero(emu, 0);
			set_sign(emu, 1);
			set_parity(emu, 1);
		} else {
			set_rm32(emu, modrm, 0);
			set_carry(emu, 0);
			set_zero(emu, 1);
			set_sign(emu, 0);
			set_parity(emu, 0);	
		}
	}
	emu->eip += 1;
}

/* 0xD3 */
static void code_d3(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);

	switch (modrm.opcode) {
		case 0:
			rol_rm32_cl(emu, &modrm); //ROL
			break;
		case 1:
			ror_rm32_cl(emu, &modrm); //ROR
			break;
		case 2:
			rcl_rm32_cl(emu, &modrm); //ROR
			break;
		case 3:
			rcr_rm32_cl(emu, &modrm); //ROR
			break;
		case 4:
			shl_rm32_cl(emu, &modrm); //ROR
			break;
		case 5:
			shr_rm32_cl(emu, &modrm); //ROR
			break;
		case 7:
			sar_rm32_cl(emu, &modrm); //ROR
			break;
		default:
			printf("not implemented: d3 /%d\n", modrm.opcode);
			system("pause");
	}
}

/* 0xE4 */
static void in_al_imm8(Emulator* emu) {
	uint16_t address = (uint16_t)get_code8(emu, 1);
	uint8_t value = io_in8(address);
	set_register8(emu, AL, value);
	emu->eip += 2;
}

/* 0xE8 */
static void call_rel32(Emulator* emu) {
	int32_t diff = get_sign_code32(emu, 1);
	push32(emu, emu->eip + 5);
	emu->eip += (diff + 5);
}

/* 0xE9 */
static void near_jump_rel32(Emulator* emu) {
	int32_t diff = get_sign_code32(emu, 1);
	emu->eip += (diff + 5);
}

/* 0xEB */
static void short_jump_rel8(Emulator* emu) {
	int8_t diff = get_sign_code8(emu, 1);
	/* eip added to eip is jmp statement*/
	emu->eip += (diff + 2);
}

/* 0xEC */
static void in_al_dx(Emulator* emu) {
	uint16_t address = get_register32(emu, EDX) & 0xffff;
	uint8_t value = io_in8(address);
	set_register8(emu, AL, value);
	emu->eip += 1;
}

/* 0xEE */
static void out_dx_al(Emulator* emu) {
	uint16_t address = get_register32(emu, EDX) & 0xffff;
	uint8_t value = get_register8(emu, AL);
	io_out8(address, value);
	emu->eip += 1;
}

/* 0xF2 */
static void repne_mode(Emulator* emu) {
	emu->prefix_mode |= PREFIX_REPNE;
	emu->eip += 1;
}

/* 0xF3 */
static void repe_mode(Emulator* emu) {
	emu->prefix_mode |= PREFIX_REPE;
	emu->eip += 1;
}

/* 0xF7 /0 */
static void test_rm32_imm32(Emulator* emu, ModRM* modrm) {
	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t rm32 = get_rm32(emu, modrm); //register
		uint32_t imm32 = get_code32(emu, 0); //hardcoded value

		register uint32_t res = rm32 & imm32;
		set_overflow(emu, 0);
		set_sign(emu, res & 0x80000000);
		set_zero(emu, res == 0);
		set_parity(emu, PARITY(res & 0xff));
		set_carry(emu, 0);
	} else {	//16-bit mode registers mode active
		uint32_t rm16 = get_rm16(emu, modrm); //register
		uint32_t imm16 = get_code16(emu, 0); //hardcoded value

		register uint32_t res = rm16 & imm16;
		set_overflow(emu, 0);
		set_sign(emu, res & 0x8000);
		set_zero(emu, res == 0);
		set_parity(emu, PARITY(res & 0xff));
		set_carry(emu, 0);
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0xF7 /2 */
static void not_rm32(Emulator* emu, ModRM* modrm) {
	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t rm32 = get_rm32(emu, modrm);
		set_rm32(emu, modrm, ~rm32);
	} else {	//16-bit mode registers mode active
		uint16_t rm16 = get_rm16(emu, modrm);
		set_rm16(emu, modrm, ~rm16);
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0xF7 /3 */
static void neg_rm32(Emulator* emu, ModRM* modrm) {
	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t rm32 = get_rm32(emu, modrm);
	
		register uint32_t res = (uint32_t) - rm32;
		register uint32_t bc = res | rm32;
		
		/* calculate the borrow chain --- modified such that d=0.
	       substitutiing d=0 into     bc= res&(~d|s)|(~d&s);
	       (the one used for sub) and simplifying, since ~d=0xff...,
	       ~d|s == 0xffff..., and res&0xfff... == res.  Similarly
	       ~d&s == s.  So the simplified result is: */
		
		set_carry(emu, rm32 != 0);
		set_zero(emu, (res & 0xffffffff) == 0);
		set_sign(emu, res & 0x80000000);
		set_parity(emu, PARITY(res & 0xff));
		set_overflow(emu, XOR2(bc >> 30));
		set_aux(emu, bc & 0x8);
	    
		set_rm32(emu, modrm, res);
	} else {	//16-bit mode registers mode active
		uint16_t rm16 = get_rm16(emu, modrm);
	
		register uint16_t res = (uint16_t) - rm16;
		register uint16_t bc = res | rm16;
		
		/* calculate the borrow chain --- modified such that d=0.
	       substitutiing d=0 into     bc= res&(~d|s)|(~d&s);
	       (the one used for sub) and simplifying, since ~d=0xff...,
	       ~d|s == 0xffff..., and res&0xfff... == res.  Similarly
	       ~d&s == s.  So the simplified result is: */
	       
	    set_carry(emu, rm16 != 0);
		set_zero(emu, (res & 0xffff) == 0);
		set_sign(emu, res & 0x8000);
		set_parity(emu, PARITY(res & 0xff));
		set_overflow(emu, XOR2(bc >> 14));
		set_aux(emu, bc & 0x8);
	    
		set_rm16(emu, modrm, res);
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0xF7 /4 */
static void mul_rm32(Emulator* emu, ModRM* modrm) {
	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t eax = get_register32(emu, EAX);
		uint32_t rm32 = get_rm32(emu, modrm);
		
		uint64_t res = (uint64_t)eax * rm32;
		
		set_register32(emu, EAX, (uint32_t)res);
		set_register32(emu, EDX, (uint32_t)(res >> 32));
		
		if (get_register32(emu, EDX) == 0) {
	    	set_carry(emu, 0);
	    	set_overflow(emu, 0);
	    }
	    else {
	    	set_carry(emu, 1);
	    	set_overflow(emu, 1);
	    }
    } else {	//16-bit mode registers mode active	
		uint16_t ax = get_register16(emu, AX);
		uint16_t rm16 = get_rm16(emu, modrm);
		
		uint32_t res = ax * rm16;
		
		set_register16(emu, AX, (uint16_t)res);
		set_register16(emu, DX, (uint16_t)(res >> 16));
		
		if (get_register16(emu, DX) == 0) {
	    	set_carry(emu, 0);
	    	set_overflow(emu, 0);
	    }
	    else {
	    	set_carry(emu, 1);
	    	set_overflow(emu, 1);
	    }
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0xF7 /5 */
static void imul_rm32(Emulator* emu, ModRM* modrm) {
	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t eax = get_register32(emu, EAX);
		uint32_t rm32 = get_rm32(emu, modrm);
		
		uint64_t res = (uint64_t) (uint32_t)eax * (uint32_t)rm32;
		eax = (uint32_t)res;
		uint32_t edx = (uint32_t) (res >> 32);
	
		set_register32(emu, EAX, eax);
		set_register32(emu, EDX, edx);		
	
		if (((get_register32(emu, EAX) & 0x80000000) == 0 && get_register32(emu, EDX) == 0x00) ||
	        ((get_register32(emu, EAX) & 0x80000000) != 0 && get_register32(emu, EDX) == 0xFF)) {
	        set_carry(emu, 0);
	    	set_overflow(emu, 0);
	    } else {
	        set_carry(emu, 1);
			set_overflow(emu, 1);
	    }
    } else {	//16-bit mode registers mode active
		uint16_t ax = get_register16(emu, AX);
		uint16_t rm16 = get_rm16(emu, modrm);
		
		uint32_t res = (uint16_t)ax * (uint16_t)rm16;

		set_register16(emu, AX, (uint16_t)res);
		set_register16(emu, DX, (uint16_t)(res >> 16));		
	
		if (((get_register16(emu, AX) & 0x8000) == 0 && get_register16(emu, DX) == 0x00) ||
	        ((get_register16(emu, AX) & 0x8000) != 0 && get_register16(emu, DX) == 0xFF)) {
	        set_carry(emu, 0);
	    	set_overflow(emu, 0);
	    } else {
	        set_carry(emu, 1);
			set_overflow(emu, 1);
	    }
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0xF7 /6 */
static void div_rm16_32(Emulator* emu, ModRM* modrm) {
	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t rm32 = get_rm32(emu, modrm); //register
		uint32_t eax = get_register32(emu, EAX);
		uint32_t edx = get_register32(emu, EDX);
		
		uint64_t dvd = (((uint64_t) edx) << 32) | eax;
		uint64_t div = dvd / (uint32_t)rm32;
		uint64_t mod = dvd % (uint32_t)rm32;
		
		set_carry(emu, 0);
		set_aux(emu, 0);
		set_sign(emu, 0);
		set_zero(emu, 1);
		set_parity(emu, PARITY(mod & 0xff));
	
		set_register32(emu, EAX, div);
		set_register32(emu, EDX, mod);
	} else {	//16-bit mode registers mode active
		uint16_t rm16 = get_rm16(emu, modrm); //register
		uint16_t ax = get_register16(emu, AX);
		uint16_t dx = get_register16(emu, DX);
		
		uint32_t dvd = (((uint32_t) dx) << 16) | ax;
		uint32_t div = dvd / (uint16_t)rm16;
		uint32_t mod = dvd % (uint16_t)rm16;
		
		
		set_carry(emu, 0);
		set_sign(emu, 0);
		set_zero(emu, (div == 0));
		set_parity(emu, PARITY(mod & 0xff));
		
		set_register16(emu, AX, div);
		set_register16(emu, DX, mod);
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0xF7 /7 */
static void idiv_rm32(Emulator* emu, ModRM* modrm) {
	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t rm32 = get_rm32(emu, modrm); //register
		uint32_t eax = get_register32(emu, EAX);
		uint32_t edx = get_register32(emu, EDX);
		
		uint64_t dvd = (((uint64_t) edx) << 32) | eax;
		uint64_t div = dvd / (uint32_t)rm32;
		uint64_t mod = dvd % (uint32_t)rm32;
		
		set_carry(emu, 0);
		set_aux(emu, 0);
		set_sign(emu, 0);
		set_zero(emu, 1);
		set_parity(emu, PARITY(mod & 0xff));
	
		set_register32(emu, EAX, div);
		set_register32(emu, EDX, mod);
	} else {	//16-bit mode registers mode active
		uint16_t rm16 = get_rm16(emu, modrm); //register
		uint16_t ax = get_register16(emu, AX);
		uint16_t dx = get_register16(emu, DX);
		
		uint32_t dvd = (((uint32_t) dx) << 16) | ax;
		uint32_t div = dvd / (uint16_t)rm16;
		uint32_t mod = dvd % (uint16_t)rm16;
		
		
		set_carry(emu, 0);
		set_sign(emu, 0);
		set_zero(emu, (div == 0));
		set_parity(emu, PARITY(mod & 0xff));
		
		set_register16(emu, AX, div);
		set_register16(emu, DX, mod);
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0xF7 */
static void code_f7(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);

	switch (modrm.opcode) {
		case 0:
			test_rm32_imm32(emu, &modrm); //TEST rm32, imm32
			break;
		case 2:
			not_rm32(emu, &modrm); //NOT rm32
			break;
		case 3:
			neg_rm32(emu, &modrm); //NEG rm32
			break;
		case 4:
			mul_rm32(emu, &modrm); //MUL rm32
			break;
		case 5:
			imul_rm32(emu, &modrm); //IMUL rm32
			break;
		case 6:
			div_rm16_32(emu, &modrm); //DIV rm32 / rm16
			break;
		case 7:
			idiv_rm32(emu, &modrm); //IDIV
			break;
		default:
			printf("not implemented: F7 /%d\n", modrm.opcode);
			system("pause");
	}
}

/* 0xFF /0 */
static void inc_rm32(Emulator* emu, ModRM* modrm) {
	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t rm32 = get_rm32(emu, modrm);
		
		register uint32_t res = rm32 + 1;
		/* calculate the carry chain */
		register uint32_t cc = (1 & rm32) | ((~res) & (1 | rm32));
	    
		set_rm32(emu, modrm, res);
		
		set_zero(emu, (res & 0xffffffff) == 0);
		set_sign(emu, res & 0x80000000);
		set_parity(emu, PARITY(res & 0xff));
		set_overflow(emu, XOR2(cc >> 30));
		set_aux(emu, cc & 0x8);
	} else {	//16-bit mode registers mode active
		uint16_t rm16 = get_rm16(emu, modrm);
		
		register uint32_t res = rm16 + 1;
		/* calculate the carry chain */
		register uint32_t cc = (1 & rm16) | ((~res) & (1 | rm16));
	    
		set_rm16(emu, modrm, (uint16_t)res);
		
		set_zero(emu, (res & 0xffff) == 0);
		set_sign(emu, res & 0x8000);
		set_parity(emu, PARITY(res & 0xff));
		set_overflow(emu, XOR2(cc >> 14));
		set_aux(emu, cc & 0x8);
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0xFF /1*/
static void dec_rm32(Emulator* emu, ModRM* modrm) {
	if( emu->prefix_mode & PREFIX_OPSIZE_MODE_32_BIT ) { //32-bit mode registers mode active
		uint32_t rm32 = get_rm32(emu, modrm);
		
		register uint32_t res = rm32 - 1;
		/* calculate the borrow chain. */
		register uint32_t bc = (res & (~rm32 | 1)) | (~rm32 & 1);;
		
		set_rm32(emu, modrm, res);
		
		set_zero(emu, (res & 0xffffffff) == 0);
		set_sign(emu, res & 0x80000000);
		set_parity(emu, PARITY(res & 0xff));
		set_overflow(emu, XOR2(bc >> 30));
		set_aux(emu, bc & 0x8);
	} else {	//16-bit mode registers mode active
		uint16_t rm16 = get_rm16(emu, modrm);
		
		register uint32_t res = rm16 - 1;
		/* calculate the borrow chain. */
		register uint32_t bc = (res & (~rm16 | 1)) | (~rm16 & 1);
	    
		set_rm16(emu, modrm, (uint16_t)res);
		
		set_zero(emu, (res & 0xffff) == 0);
		set_sign(emu, res & 0x8000);
		set_parity(emu, PARITY(res & 0xff));
		set_overflow(emu, XOR2(bc >> 14));
		set_aux(emu, bc & 0x8);
	}

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;
}

/* 0xFF /2*/
static void call_rm32(Emulator* emu, ModRM* modrm) {
	uint32_t value = get_rm32(emu, modrm);
	push32(emu, emu->eip);
	emu->eip = value;
}

/* 0xFF /6 (Ver3.7 time self-implementation)*/
static void push_rm32(Emulator* emu, ModRM* modrm) {
	uint32_t rm32 = get_rm32(emu, modrm);
	push32(emu, rm32);
}

static void code_ff(Emulator* emu) {
	emu->eip += 1;
	ModRM modrm;
	parse_modrm(emu, &modrm, false);

	switch (modrm.opcode) {
		case 0:
			inc_rm32(emu, &modrm); //INC
			break;
		case 1:
			dec_rm32(emu, &modrm); //DEC
			break;
		case 2:
			//CALL
			call_rm32(emu, &modrm);
			break;
		case 3:
			//CALL
			call_rel32(emu);  //TODO: CHECK COULD BE BAD
			break;
		case 4:
			//JMP
			printf("not implemented: FF /%d\n", modrm.opcode);
			break;
		case 5:
			//JMP
			printf("not implemented: FF /%d\n", modrm.opcode);
			break;
		case 6:
			push_rm32(emu, &modrm);
			break;
		default:
			printf("not implemented: FF /%d\n", modrm.opcode);
			system("pause");
	}
}

/* Function pointer table */
void init_instructions(void) {
	int i;
	memset(instructions, 0, sizeof(instructions));
	instructions[0x01] = add_rm32_r32;
	instructions[0x03] = add_r32_rm32;
	instructions[0x04] = add_al_imm8;
	instructions[0x05] = add_eax_imm32;
	instructions[0x09] = or_rm32_r32;
	instructions[0x0b] = or_r32_rm32;
	instructions[0x0d] = or_eax_imm32;
	instructions[0x0F] = code_0f;
	instructions[0x21] = and_rm32_r32;
	instructions[0x23] = and_r32_rm32;
	instructions[0x25] = and_eax_imm32;
	instructions[0x29] = sub_rm32_r32;
	instructions[0x2B] = sub_r32_rm32;
	instructions[0x2D] = sub_eax_imm32;
	instructions[0x31] = xor_rm32_r32;	
	instructions[0x33] = xor_r32_rm32;
	instructions[0x35] = xor_eax_imm32;
	instructions[0x3B] = cmp_r32_rm32;
	instructions[0x3C] = cmp_al_imm8;
	instructions[0x3D] = cmp_eax_imm32;

	for (i = 0; i < 8; i++) {
		instructions[0x40 + i] = inc_r32;
	}

	for (i = 0; i < 8; i++) {
		instructions[0x50 + i] = push_r32;
	}

	for (i = 0; i < 8; i++) {
		instructions[0x58 + i] = pop_r32;
	}

	instructions[0x66] = opsize_mode;
	instructions[0x67] = address_mode;
	instructions[0x68] = push_imm32;
	instructions[0x69] = imul_r32_rm32_imm32;
	instructions[0x6A] = push_imm8;

	instructions[0x70] = jo;
	instructions[0x71] = jno;
	instructions[0x72] = jc;
	instructions[0x73] = jnc;
	instructions[0x74] = jz;
	instructions[0x75] = jnz;
	instructions[0x78] = js;
	instructions[0x79] = jns;
	instructions[0x7C] = jl;
	instructions[0x7E] = jle;
	instructions[0x7F] = jg;

	instructions[0x81] = code_81;
	instructions[0x83] = code_83;
	instructions[0x85] = test_rm32_r32;
	instructions[0x88] = mov_rm8_r8;
	instructions[0x89] = mov_rm32_r32;
	instructions[0x8A] = mov_r8_rm8;
	instructions[0x8B] = mov_r32_rm32;
	instructions[0x8D] = lea_r16_r32_m;

	instructions[0x90] = nop;

	instructions[0xAB] = stos;

	for (i = 0; i < 8; i++) {
		instructions[0xB0 + i] = mov_r8_imm8;
	}

	for (i = 0; i < 8; i++) {
		instructions[0xB8 + i] = mov_r32_imm32;
	}

	instructions[0xC1] = code_c1;
	instructions[0xC3] = ret;
	instructions[0xC7] = mov_rm32_imm32;
	instructions[0xC9] = leave;
	instructions[0xCD] = swi;
	instructions[0xD1] = code_d1;
	instructions[0xD3] = code_d3;

	instructions[0xE4] = in_al_imm8;
	instructions[0xE8] = call_rel32;
	instructions[0xE9] = near_jump_rel32;
	instructions[0xEB] = short_jump_rel8;
	instructions[0xEC] = in_al_dx;
	instructions[0xEE] = out_dx_al;

	instructions[0xF2] = repne_mode;
	instructions[0xF3] = repe_mode;
	instructions[0xF7] = code_f7;
	instructions[0xFF] = code_ff;
}
