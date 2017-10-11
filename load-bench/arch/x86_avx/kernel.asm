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

  ; rdi is a number of double, multiply by 8 so we get a number of words to load
  ;shl rsi, 3
  mov r10, rdx


; outer loop, loading array k times
.outer:
  ; rcx holds the number of words that have been loaded
  xor rcx,rcx
.inner:
  ; do 4 parallel loads
  ; hopefully this is enough to get rid of
  ; false dependencies
  vmovapd ymm0, [rdi + 8 * rcx] 
  vmovapd ymm1, [rdi + 8 * rcx + 32] 
  vmovapd ymm2, [rdi + 8 * rcx + 64] 
  vmovapd ymm3, [rdi + 8 * rcx + 96] 
  vmovapd ymm4, [rdi + 8 * rcx + 128] 
  vmovapd ymm5, [rdi + 8 * rcx + 160] 
  vmovapd ymm6, [rdi + 8 * rcx + 192] 
  vmovapd ymm7, [rdi + 8 * rcx + 224] 
  ; each vmov loads 4 words
  add rcx, 32
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
  vmovapd ymm0, [rdi + 8 * rcx] 
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
  ; each vmov loads 4 words
  add rcx, 4
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
