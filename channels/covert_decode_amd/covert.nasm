
%define nthreads    2
%define t2_repeat1  1
%define t2_repeat2  500 
%define repeat1  1
%define repeat2  1
%define repeat0 100000
%define WARMUPCOUNT 1<<31
; %define codealign 64
%define noptype 2
; Define any undefined macros


%include "nops.inc"
%include "lfsr.inc"

%macro cacheMiss 0
 clflush [rdi]
 mfence
 mov rax, [rdi]
%endmacro

%ifdef all_zeros
  %define ZERO_ITR 100
%else 
  %define ZERO_ITR 8
%endif 


%ifdef all_ones 
  %define ONE_ITR 100
%else 
  %define ONE_ITR 200
%endif 

%macro send_zero 0
; should not modify edx
  mov ebx, ZERO_ITR
  %%loop1:
    cacheMiss
    lfence
   dec ebx
   jnz %%loop1

%endmacro 

%macro send_small_zero 0
; should not modify edx
  mov ebx, 2
  %%loop1:
   cacheMiss
   lfence
   dec ebx
   jnz %%loop1

%endmacro

%macro ucacheBLKA 1
  %assign i (100)*(%1)
  jmp %%lbl%+ i
  align 1<<12
  %rep 8
    %%lbl%+ i: 
    nop
    %assign i i+1
    jmp %%lbl%+ i
  %endrep 
  %%lbl%+ i:
%endmacro 

%macro send_one 0
  mov ebx, ONE_ITR
  %%loop1:
    ucacheBLKA 1
   dec ebx
   jnz %%loop1
%endmacro

%macro testcode 0
  lfsr
%endmacro


%macro t2_testcode 0         
    nop
%endmacro 
