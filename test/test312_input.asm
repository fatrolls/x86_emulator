BITS 32
  org 0x7c00
start:
  mov edx, 0x03f8
  in al, dx
  jmp 0
