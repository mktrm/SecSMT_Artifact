
%define nthreads    2
%define t2_repeat1  1
%define t2_repeat2  1
%define repeat1  20
%define repeat2  1
%define repeat0 100000
%define noptype 2
; Define any undefined macros
%define blocks 1
%define codealign  32




%include "nops.inc"
%include "lfsr.inc"


%macro jump_blkn 2
%assign i %2
align 4096*2
%rep 64*i
 nop
%endrep 
%1: jmp BTB%+ i
%endmacro

%macro jump_blk 2
%assign i %2
align 4096*2
%rep 64*i
 nop
%endrep 
%1: jmp BTBB%+ i
%endmacro


%ifdef all_zeros
  %define ZERO_ITR 10000
%else 
  %define ZERO_ITR 500
%endif 


%ifdef all_ones 
  %define ONE_ITR 2000
%else 
  %define ONE_ITR 5
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
    jmp BTBB0
    %rep 60
      %assign j i+1
      jump_blk BTBB%+ i, j
      %assign i j 
    %endrep
    BTBB %+i:
   dec ebx
   jnz %%loop1

%endmacro




%macro t2_testinit3 0
 mov rax, rsi
%endmacro

%macro t2_testcode 0 

%assign i 0
jmp BTB0
%rep 60
  %assign j i+1
  jump_blkn BTB%+ i, j
  %assign i j 
%endrep
BTB %+i:

%endmacro 




%macro testcode 0
  lfsr
%endmacro