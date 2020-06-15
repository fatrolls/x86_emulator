#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <stdint.h>

/* Memory 10 MB */
#define MEMORY_SIZE ((1024 * 1024) * 10)

/* Program starting address */
#define PROGRAM_ORIGIN (0x00401000)

/* Stack start address */
#define STACK_BASE (0x7c00)

#define PREFIX_SEGMENT_OVERRIDE_ES (1)
#define PREFIX_SEGMENT_OVERRIDE_CS (1 << 1)
#define PREFIX_SEGMENT_OVERRIDE_SS (1 << 2)
#define PREFIX_SEGMENT_OVERRIDE_DS (1 << 3)
#define PREFIX_SEGMENT_OVERRIDE_FS (1 << 4)
#define PREFIX_SEGMENT_OVERRIDE_GS (1 << 5)
#define PREFIX_OPSIZE_MODE_32_BIT (1 << 6)
#define PREFIX_ADDRESS_MODE_32_BIT (1 << 7)
#define PREFIX_LOCK (1 << 8)
#define PREFIX_REPNE (1 << 9)
#define PREFIX_REPE (1 << 10)
#define PREFIX_REP (PREFIX_REPE | PREFIX_REPNE)

enum Register { EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI, REGISTERS_COUNT,
  AL = EAX, CL = ECX, DL = EDX, BL = EBX,
  AH = AL + 4, CH = CL + 4, DH = DL + 4, BH = BL + 4,
  AX = EAX, CX = ECX, DX = EDX, BX = EBX,
  SP = ESP, BP = EBP, SI = ESI, DI = ESI };

typedef struct {
  /* General-purpose register */
  uint32_t registers[REGISTERS_COUNT];
  /* EFLAGS register */
  uint32_t eflags;
  /* mode */
  uint32_t prefix_mode;
  /* The program counter */
  uint32_t eip;
  /* Memory (byte sequence) */
  uint8_t* memory;
} Emulator;

#endif
