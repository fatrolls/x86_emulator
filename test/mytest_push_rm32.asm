BITS 32
  org 0x7c00
start:
  mov ebp, esp
  mov dword [ebp + 4], 5
  push 1
  push 16
  push dword [ebp + 4]
  pop eax
  jmp 0
