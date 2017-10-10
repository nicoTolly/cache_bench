global get_cycles

section .text

get_cycles:
  ; align stack
  sub rsp, 8
  xor rax, rax
  cpuid
  rdtsc 
  shl rdx, 32
  or rax, rdx
  add rsp, 8
  ret
