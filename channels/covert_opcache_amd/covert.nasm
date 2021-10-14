
%define nthreads    4
%define t2_repeat1  200
%define t2_repeat2  1
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
  %define ZERO_ITR 90000
%else 
  %define ZERO_ITR 500
%endif 


%ifdef all_ones 
  %define ONE_ITR 2000
%else 
  %define ONE_ITR 100
%endif 

%macro send_zero 0
; should not modify edx
  mov ebx, ZERO_ITR
  .loop1:
    %REP 8
        nop
    %ENDREP
   dec ebx
   jnz .loop1 

%endmacro 
%macro ucacheBLKA 1
  %assign i (100)*(%1)
  jmp lbl%+ i
  %rep 8
    align 1<<12
    lbl%+ i: 
    %assign i i+1
    jmp lbl%+ i
  %endrep 
  lbl%+ i:
%endmacro 

%macro ucacheBLK 1
  %assign i (100)*(%1)
  jmp lbl%+ i
  align 1<<12
  lbl%+ i: 
    nop15
    nop15
    nop15
    nop15
  ; %rep 1
  ;   align 1<<12
  ;   lbl%+ i: 
  ;   %assign i i+1
  ;   jmp lbl%+ i
  ; %endrep 
  ; lbl%+ i:
%endmacro 

%macro send_small_zero 0
; should not modify edx
  mov ebx, 2000
  %%loop1:
    %REP 8
        nop
    %ENDREP
   dec ebx
   jnz %%loop1

%endmacro

%macro send_one 0
; should not modify edx
  mov ebx, ONE_ITR
  %%loop0:
     ucacheBLKA 7
  dec ebx 
  jnz %%loop0
%endmacro

%macro testcode 0
  lfsr
%endmacro



%macro t2_testcode 0         
    ucacheBLK 1
%endmacro 
