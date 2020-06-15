#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "emulator.h"
#include "emulator_function.h"
#include "instruction.h"

char* registers_name[] = {
	"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"
};

/* Emulator To 512 bytes copy the contents of the binary file to the memory */
static void read_binary(Emulator* emu, const char* filename) {
	FILE* binary;

	binary = fopen(filename, "rb");

	if (binary == NULL) {
		printf("%s file can not be opened\n", filename);
		system("pause");
	}

	fseek(binary,0,SEEK_END);
	long code_size = ftell(binary);
	long current_offset = PROGRAM_ORIGIN;
	code_size += PROGRAM_ORIGIN;
	rewind(binary); //start from beginning

	long temp_current_offset;
	int i = 0;
	unsigned char buffer[0x200];

	while (current_offset < code_size) {
		/* Read the machine language file (up to 512 Bytes) at a time */
		current_offset += fread(emu->memory + current_offset, 1, 0x200, binary);
	}

	fclose(binary);
}

/* It outputs the value of the general-purpose registers and a program counter to the standard output */
static void dump_registers(Emulator* emu) {
	int i;

	printf("[REGISTERS]\n");
	for (i = 0; i < REGISTERS_COUNT; i++)
		printf("%s = %08x\n", registers_name[i], emu->registers[i]);
	printf("EIP = %08x\n", emu->eip);
	printf("\n[FLAGS]\n");
	printf("C: %d P %d A: %d Z: %d S: %d T: %d D: %d O: %d\n",
	       is_carry(emu),
	       is_parity(emu),
	       is_aux(emu),
	       is_zero(emu),
	       is_sign(emu),
	       is_trap(emu),
	       is_direction(emu),
	       is_overflow(emu));

}

static void dump_stack(Emulator* emu) {
	uint32_t sp = emu->registers[ESP];

	printf("[STACK]\n");
	int i,j;
	for (; sp < STACK_BASE; sp = sp + 0x04)
		printf("%x: %08x\n", sp, get_memory32(emu, sp));
}

/* To create an emulator */
Emulator* create_emu(size_t size) {
	Emulator* emu = malloc(sizeof(Emulator));
	emu->memory = malloc(size);

	emu->eflags = 0;
	emu->prefix_mode = 0;

	//if current segment is CS (CODE) default modes are 32 bit.
	emu->prefix_mode |= PREFIX_OPSIZE_MODE_32_BIT | PREFIX_ADDRESS_MODE_32_BIT;

	/* All the initial value of the general-purpose register to 0 */
	memset(emu->registers, 0, sizeof(emu->registers));

	return emu;
}

/* Discard the emulator */
void destroy_emu(Emulator* emu) {
	free(emu->memory);
	free(emu);
}

/* To ensure the emulator */
int opt_remove_at(int argc, char* argv[], int index) {
	if (index < 0 || argc <= index) {
		return argc;
	} else {
		int i = index;
		for (; i < argc - 1; i++) {
			argv[i] = argv[i + 1];
		}
		argv[i] = NULL;
		return argc - 1;
	}
}

int main(int argc, char* argv[]) {
	unsigned int debug = 1;
	Emulator* emu;

	/* Initialization of the instruction set */
	init_instructions();

	/* Make the emulator. Specified in the EIP and ESP of argument */
	emu = create_emu(MEMORY_SIZE);

	/* Read binary given by the argument */
	read_binary(emu, "Continuum40.bin");


	/* To those specified the initial value of the registers */
	emu->eip = 0x00457D60; //start of function 0x457D60
	uint32_t KEY = 0xF53E944B;
	emu->registers[EAX] = 0x0012F8F8;
	emu->registers[ECX] = 0x0012F8F8;
	emu->registers[EDX] = KEY; //<-- Key
	emu->registers[EBX] = 0xFFFFFFFF;
	emu->registers[ESP] = 0x0012E8D0;
	emu->registers[EBP] = 0x0012F91C;
	emu->registers[ESI] = KEY; //<-- Key
	emu->registers[EDI] = 0x00000400;

	//Fix esp and [esp+4] value this is to fake emulate passing key into function
	set_memory32(emu, 0x0012E8D0, KEY);
	set_memory32(emu, 0x0012E8D4, KEY);

	//Write pointer at 0x0012F8F8 that goes to address 0x0012F880 where virtual buffer
	set_memory32(emu, 0x0012F8F8, 0x0012F880);

	//zero the 80 byte virtual buffer
	unsigned int i;
	unsigned int j = 0;

	for( i = 0x0012F880; i < 0x0012F880+80; i++)
		set_memory8(emu, i, 0);

	/*
	Step 1 start registers and stuff
	EAX: 0012FB20
	ECX: 0012FB20
	EDX: 752B2633
	EBX: FFFFFFFF
	ESP: 0012EAF4
	EBP: 0012FB44
	ESI: 752B2633
	EDI: 00000400 //<-Count down starts at 1024 and goes down.

	__declspec(naked) void stepOne(int buffer, int key)
	{
	    __asm{
	        push ebp
	        mov ebp, esp
	        push ebx
	        push key
	        mov ebx, key
	        mov ecx, buffer
	        call [0x457D60]
	        pop ebx
	        leave
	        ret
	    }
	}
	*/

	while (emu->eip != 0x00458BD0) {
		uint8_t code = get_code8(emu, 0);

		/* And outputs the binary to be run with the current program counter */
		if (debug) {
			printf("EIP = %X, Code = %02X\n", emu->eip, code);
		}
        
		//dump_registers(emu);
		if (instructions[code] == NULL) {
			printf("\n\nNot Implemented: %x\n", code);
			break;
		}

		/* Execution of an instruction */
		instructions[code](emu);
		dump_registers(emu);
		printf("\n--------------------------------\n");
		/* EIP - The end of the program Once but becomes 0 */
		if (emu->eip == 0x00) {
			printf("\n\nEnd of program.\n\n");
			break;
		}
	}

//Buffer (1) = 
//5C F9 17 30 36 8C 48 BA 6B DA 94 4D 25 C5 5F 71 CF 6F E0 6C 3C 
//92 C F3 5A BA F2 39 EE 5D 20 BF AF 51 1F 11 88 8B FB 7 C7 2 F 
//23 D4 AB 77 72 3A 6F 7F 79 CD 5 DD D7 8D AD 24 13 D3 0 F4 ED 99
// 2 99 F4 83 7D FF 69 FD BC EF 23 90 C6 C9 B1 
	printf("Buffer (1) = \n");
	for(i=0x0012F880;i<0x0012F880+80;i++) {
		printf("%X ", get_memory8(emu, i));
	}
	printf("\n");
//Generated this values
//7E 9D FD 9C ED 71 9B 39 5E 12 46 37 A 8 AD DF 1F 55 E2 F6 CE EB EE 23 3 41 1F 5E
//A8 B1 5F 4D 38 74 60 46 50 A2 B5 12 7B 3A 41 A5 F2 C3 9E CB 14 7 7 DA A1 54 61
//D4 E0 E0 41 4E 7 16 F9 FD 4A 28 53 97 6D 4D 21 F2 91 2A 26 34 BB 9D B7 BA
//newest generated values
//81 9C 2F F6 45 5 17 A7 60 5F 9F DC 1B BF 77 7E D8 E1 D1 E2 79 5D 46 4 65 D5 73 9
//E 45 80 D5 32 8B EC A8 3E C7 9 65 D5 E9 E EA 0 83 1 6E 58 29 37 71 E5 3D 94 E3 6
//6 50 9 4A 9 72 73 52 53 4A 69 78 DF 8 2 4F 2E 67 55 F9 A3 C2 9A 35 8F
	dump_stack(emu);
	destroy_emu(emu);
	system("pause");
	return 0;
}
