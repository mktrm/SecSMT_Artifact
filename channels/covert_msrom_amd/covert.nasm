
%define nthreads    2
%define t2_repeat1  8
%define t2_repeat2  10
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
  %define ZERO_ITR 800
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
    %rep 100
    nop
    %endrep
   dec ebx
   jnz %%loop1

%endmacro 

%macro send_small_zero 0
; should not modify edx
  mov ebx, 150
  %%loop1:
    %rep 100
    nop
    %endrep
   dec ebx
   jnz %%loop1

%endmacro


%macro send_one 0
  mov ebx, ONE_ITR
  %%loop1:
    %REP 10
      repnz scasb 
    %ENDREP
   dec ebx
   jnz %%loop1
%endmacro

%macro testcode 0
  lfsr
%endmacro

%macro t2_testinit3 0
mov rcx, 0
%endmacro 


%macro t2_testcode 0         
    repnz scasb 
%endmacro 
