%define nthreads    2
%define t2_repeat1  2
%define t2_repeat2  1
%define repeat1  1
%define repeat2  1
%define repeat0 100000
%define noptype 2
%define codealign  16
; Define any undefined macros


%include "nops.inc"
%include "lfsr.inc"

%ifdef all_zeros
  %define ZERO_ITR 10
%else 
  %define ZERO_ITR 1000
%endif 


%ifdef all_ones 
  %define ONE_ITR 10
%else 
  %define ONE_ITR 30
%endif 

%macro send_zero 0
; should not modify edx
  mov ebx, ZERO_ITR
  .loop1:
    %REP 8
        nop8
    %ENDREP
   dec ebx
   jnz .loop1 

%endmacro 

%macro send_small_zero 0
; should not modify edx
  mov ebx, 2000
  %%loop1:
    %REP 8
        nop8
    %ENDREP
   dec ebx
   jnz %%loop1

%endmacro

%macro jump_blk 2
align 1<<6
%1: jmp  %2
%endmacro




%macro send_one 0
; should not modify edx
  mov ebx, ONE_ITR
  loop1:
  jmp BTBb0

    %assign i 0
    %rep 50
    %assign j i+1 
    jump_blk BTBb%+ i, BTBb%+ j
    %assign i j 
  %endrep
  BTBb %+i:

  dec ebx 
  jnz loop1
%endmacro

%macro testcode 0
  lfsr

%endmacro

%macro t2_testcode 0 
%rep 64
nop
%endrep

%assign i 0
jmp BTB0 

%rep 100
  %assign j i+1 
  jump_blk BTB%+ i, BTB%+ j
  %assign i j 
%endrep
BTB %+i:



%endmacro 

