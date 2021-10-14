; ----------------------------------------------------------------------------------------
; Writes "Hello, World" to the console using only system calls. Runs on 64-bit Linux only.
; To assemble and run:
;
;     nasm -felf64 hello.asm && ld hello.o && ./a.out
; ----------------------------------------------------------------------------------------
%define nop15 db 66H, 66H, 66H, 66H, 66H, 66H, 66H, 0FH, 1FH, 84H, 00H, 00H, 00H, 00H, 00H
%define nop8  db 0FH, 1FH, 84H, 00H, 00H, 00H, 00H, 00H

%macro testcode 0
%if   mod==1
    add rax, rax
%elif mod==2
    xor rax, rax
%elif mod==3
    bt rax, 1
%elif mod==4
    bts rax, 1 
%elif mod==5
    movq xmm1, xmm2
%elif mod==6
    nop
%elif mod==7
    nop8
%elif mod==8
    nop15
%elif mod==9
    vtestps ymm1, ymm2
%elif mod==10
    pause
%elif mod==11
    lea rax,[4*rax+edi+40960]
    lea rdx,[8*eax+rdi+409623]
%elif mod==12
    cpuid
%elif mod==13
    rdrand rax
%elif mod==14
    rdseed rax
%endif 
%endmacro 

          global    _start

          section   .text
_start:   mov       rcx, 1000000
LOOP:     
          %rep 10
            testcode
          %endrep
          dec rcx
          jnz  LOOP

          mov       rax, 60                 ; system call for exit
          xor       rdi, rdi                ; exit code 0
          syscall                           ; invoke operating system to exit

          section   .data
message:  db        "Hello, World", 10      ; note the newline at the end