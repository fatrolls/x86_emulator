# Makefile

binary_name = program

px86: modrm.o bios.o io.o emulator_function.o instruction.o main.c
	cc -o px86 modrm.o bios.o io.o emulator_function.o instruction.o main.c
	rm modrm.o bios.o io.o emulator_function.o instruction.o
emulator_function.o: emulator_function.h emulator_function.c
	cc -c emulator_function.c
instruction.o: instruction.h instruction.c modrm.o
	cc -c instruction.c
io.o: io.h io.c
	cc -c io.c
bios.o: bios.h bios.c
	cc -c bios.c
modrm.o: modrm.c
	cc -c modrm.c

test_asm:
	nasm -o program test/$(TARGET).asm

test_cfunc: test37_crt0.o
	cc -nostdlib -fno-asynchronous-unwind-tables -fno-stack-protector -m32 -o $(TARGET).o -c test/$(TARGET).c
	ld --entry=start --oformat=binary -Ttext 0x7c00 -m elf_i386 -o $(binary_name) test37_crt0.o $(TARGET).o
	rm $(TARGET).o test37_crt0.o
test37_crt0.o: test/test37_crt0.asm
	nasm -f elf -o test37_crt0.o test/test37_crt0.asm

clean:
	rm px86
	rm $(binary_name)
