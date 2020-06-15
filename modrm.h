#ifndef MODRM_H_
#define MODRM_H_

#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"

/* Structure that represents the ModRM */
typedef struct {
  uint8_t mod;

  /* opcode and reg_index the same alias */
  union {
    uint8_t opcode;
    uint8_t reg_index;
  };

  uint8_t rm;

  /* Use when SIB is a combination of mod / rm necessary*/
  uint8_t sib;

  union {
    int8_t disp8; /* disp8 is signed integer*/
    int16_t disp16;
    uint32_t disp32;
  };
} ModRM;

/* ModRM, SIB, To analyze the displacement */
void parse_modrm(Emulator* emu, ModRM* modrm, bool mode_16bit);

/* ModRM To calculate the effective address of the memory on the basis of the content */
uint32_t calc_memory_address16(Emulator* emu, ModRM* modrm);
uint32_t calc_memory_address32(Emulator* emu, ModRM* modrm);

/* Memory / register accessor 32-bit version */
uint32_t get_rm32(Emulator* emu, ModRM* modrm);
void set_rm32(Emulator* emu, ModRM* modrm, uint32_t value);
uint32_t get_r32(Emulator* emu, ModRM* modrm);
void set_r32(Emulator* emu, ModRM* modrm, uint32_t value);

/* Memory / register accessor 16-bit version */
uint16_t get_rm16(Emulator* emu, ModRM* modrm);
void set_rm16(Emulator* emu, ModRM* modrm, uint16_t value);
uint16_t get_r16(Emulator* emu, ModRM* modrm);
void set_r16(Emulator* emu, ModRM* modrm, uint16_t value);

/* Memory / register accessor 8-bit version */
uint8_t get_rm8(Emulator* emu, ModRM* modrm);
void set_rm8(Emulator* emu, ModRM* modrm, uint8_t value);
uint8_t get_r8(Emulator* emu, ModRM* modrm);
void set_r8(Emulator* emu, ModRM* modrm, uint8_t value);

#endif
