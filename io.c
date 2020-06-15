#include "io.h"

#include <stdio.h>
#include "emulator.h"

#define KEYBOARD_IO (0x03f8)

uint8_t io_in8(uint16_t address)
{
  switch (address) {
    case KEYBOARD_IO:
      return getchar();
      break;
    default:
      return 0;
  }
}

void io_out8(uint16_t address, uint8_t value)
{
  switch (address) {
    case KEYBOARD_IO:
      // putchar(value);
      printf("%c", value);
      break;
  }
}
