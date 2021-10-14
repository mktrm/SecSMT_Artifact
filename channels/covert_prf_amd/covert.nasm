
%define nthreads    2
%define t2_repeat1  10
%define t2_repeat2  100
%define repeat1  1
%define repeat2  1
%define repeat0 100000
%define WARMUPCOUNT 1<<31
; %define codealign 64
%define noptype 2
; Define any undefined macros


%include "nops.inc"
%include "lfsr.inc"


%ifdef all_zeros
  %define ZERO_ITR 100
%else 
  %define ZERO_ITR 450
%endif 


%ifdef all_ones 
  %define ONE_ITR 100
%else 
  %define ONE_ITR 20
%endif 

%macro send_zero 0
; should not modify edx
  mov ebx, ZERO_ITR
  %%loop1:
     %REP 1000
        nop
    %ENDREP
   dec ebx
   jnz %%loop1

%endmacro 

%macro send_small_zero 0 
; should not modify edx
;nop
  mov ebx, 200
  %%loop1:
  %rep 1000
   nop
  %endrep
  dec ebx
  jnz %%loop1

%endmacro


%macro send_one 0
  mov ebx, ONE_ITR

  %%loop1: 
    clflush [rdi+4096]
    clflush [rsi+4096]

    mfence
    ; movq xmm0, [rdi+4096] 
    mov  rax, [rdi+4096] 
    %REP 110
        mov rax, 10
        ; nop
    %ENDREP
    mov  rax, [rsi+4096] 

   dec ebx
   jnz %%loop1
%endmacro

%macro testcode 0
    lfsr
;    nop
%endmacro

%macro testinitc 0
    mov rsi, 256
    add rsi, rdi
%endmacro

; %macro t2_testcode 0         
    
;     clflush [rdi]
;     clflush [rsi]
;     ; rdrand rax


;     mfence

;     ; movq xmm0, [rdi]
;     mov rax, [rdi]
;     %rep 300
;     ;  lea  rbx, [rax*2+33]
;     mov rbx, 10
;     ; sqrtps xmm1, xmm0
;     %endrep 
;     ; movq xmm2, [rsi+12]
;     mov rax, [rsi+12]
; %endmacro 

%macro t2_testinit3 0
 clflush [rsi+512]
 clflush [rdi+512]
 mfence
 mov rax, [rdi+512]
;nop
%endmacro 

%macro t2_testcode 0
  ;movq mm1, rdx
 ;movq mm1, rax
 ;add rax, rax
 mov rax, 10
 ;nop
%endmacro

%macro t2_testafter1 0
 mov rbx, [rsi+512]
;nop
%endmacro

