
%define nthreads    2
%define t2_repeat1  1
%define t2_repeat2  1
%define repeat1  1
%define repeat2  1
%define repeat0 100000
%define noptype 2
; Define any undefined macros
%define blocks 2



%include "nops.inc"
%include "lfsr.inc"

%ifdef all_zeros
  %define ZERO_ITR 10000
%else 
  %define ZERO_ITR 400
%endif 


%ifdef all_ones 
  %define ONE_ITR 30
%else 
  %define ONE_ITR 1
%endif 

%macro send_zero 0
; should not modify edx
  mov ebx, ZERO_ITR
  %%loop1:
    %REP 50
        nop8
    %ENDREP
   dec ebx
   jnz %%loop1

%endmacro 

%macro send_small_zero 0
; should not modify edx
  mov ebx, 1500
  %%loop1:
    %REP 8
        nop8
    %ENDREP
   dec ebx
   jnz %%loop1

%endmacro

%macro send_one 0
; should not modify edx
  mov ebx, ONE_ITR
  %%loop1:
  %assign i 0
  %rep blocks
    clflush [blk%+ i]
    %assign i i+1
  %endrep 
  mfence
  dec ebx
  jnz %%loop1

%endmacro



%macro t2_testcode 0
%assign i 0
%rep blocks
blk%+ i:
  %assign i i+1
  align 64
  jmp blk%+ i
%endrep 
align 64 
blk%+ blocks:
 mfence
%endmacro 
 


%macro testcode 0
  lfsr
%endmacro