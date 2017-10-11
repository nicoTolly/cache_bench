global get_cycles

section .text

get_cycles:
  ; rbx is modified by cpuid
  ; preserving it is mandatory
  push rbx
  xor rax, rax
  cpuid
  rdtsc 
  shl rdx, 32
  or rax, rdx
  ; restore registers
  pop rbx
  ret
