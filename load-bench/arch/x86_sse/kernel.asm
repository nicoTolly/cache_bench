  global ld_vect
  global ld_vect_slw



 section .text
ld_vect:
  ; rdi is a pointer to array
  push rdi
  ; rsi is number of elements in array
  push rsi
  ; rdx contains number of iterations
  push rdx
  ; Now stack is aligned

  mov r10, rdx


; outer loop, loading array k times
align 16
.outer:
  ; rcx holds the number of words that have been loaded
  xor rcx,rcx
align 16
.inner:
  ; do 8 parallel loads
  ; hopefully this is enough to get rid of
  ; false dependencies
  movapd xmm0, [rdi + 8 * rcx] 
  movapd xmm1, [rdi + 8 * rcx + 16] 
  movapd xmm2, [rdi + 8 * rcx + 32] 
  movapd xmm3, [rdi + 8 * rcx + 48] 
  movapd xmm4, [rdi + 8 * rcx + 64] 
  movapd xmm5, [rdi + 8 * rcx + 80] 
  movapd xmm6, [rdi + 8 * rcx + 96] 
  movapd xmm7, [rdi + 8 * rcx + 112] 
  ; each vmov loads 2 words
  add rcx, 16
  cmp rcx, rsi
  jnz .inner
  dec rdx
  jnz .outer
  ; restoring registers
  pop rdx
  pop rsi
  pop rdi
  mov rax, 0
  ret

; end ld_vect

ld_vect_slw:
  ; rdi is a pointer to array
  push rdi
  ; rsi is number of elements in array
  push rsi
  ; rdx contains number of iterations
  push rdx
  ; Now stack is aligned

  shl rsi, 1
  mov r10, rdx


; outer loop, loading array k times
.out:
  ; rcx holds the number of words that have been loaded
  xor rcx,rcx
.in:
  ; do 4 parallel loads
  ; hopefully this is enough to get rid of
  ; false dependencies
  movapd xmm0, [rdi + 8 * rcx] 
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  ; each mov loads 2 words
  add rcx, 2
  cmp rcx, rsi
  jnz .in
  dec rdx
  jnz .out
  ; restoring registers
  pop rdx
  pop rsi
  pop rdi
  mov rax, 0
  ret

; end ld_vect
