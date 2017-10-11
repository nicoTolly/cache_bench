global get_cycles

section .text

get_cycles:
  ; rbx is modified by cpuid
  push rbx
  xor rax, rax
  cpuid
  rdtsc 
  shl rdx, 32
  or rax, rdx
  pop rbx
  ret
