
%define nthreads    4
%define t2_repeat1  50
%define t2_repeat2  10
%define repeat1  1
%define repeat2  1
%define repeat0 100000
%define noptype 2
%define blocks 1
; Define any undefined macros


%include "nops.inc"
%include "lfsr.inc"

%ifdef all_zeros
  %define ZERO_ITR 10000
%else 
  %define ZERO_ITR 4000
%endif 

%ifdef all_ones 
  %define ONE_ITR 10000
%else 
  %define ONE_ITR 600
%endif 


%macro send_zero 0
; should not modify edx
  mov ebx, ZERO_ITR
  .loop0:
    %REP 4
        nop8
    %ENDREP
  dec ebx 
  jnz .loop0
%endmacro

%macro send_small_zero 0
; should not modify edx
  mov ebx, 1000
  %%loop0:
    %REP 4
        nop8
    %ENDREP
  dec ebx 
  jnz %%loop0
%endmacro

%macro send_one 0
; should not modify edx
  mov ebx, ONE_ITR
  .loop0:
    %REP 10
       vmovdqa ymm1, [rdi] 
    %ENDREP
  dec ebx 
  jnz .loop0
%endmacro

;receiver
%macro t2_testcode 0 
vmovdqa ymm1, [rdi]
%endmacro 

%macro testcode 0
  lfsr
%endmacro