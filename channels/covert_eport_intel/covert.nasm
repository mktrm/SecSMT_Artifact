
%define nthreads    4
%define t2_repeat1  40
%define t2_repeat2  10
%define repeat1  1
%define repeat2  1
%define repeat0 100000
%define noptype 2
; Define any undefined macros


%include "nops.inc"
%include "lfsr.inc"

%ifdef all_zeros
  %define ZERO_ITR 50000
%else 
  %define ZERO_ITR 400
%endif 


%ifdef all_ones 
  %define ONE_ITR 20000
%else 
  %define ONE_ITR 200
%endif 

%macro send_zero 0
; should not modify edx
  mov ebx, ZERO_ITR
  .loop1:
    %REP 40
        nop8
    %ENDREP
   dec ebx
   jnz .loop1

%endmacro 

%macro send_small_zero 0
; should not modify edx
  mov ebx, 500
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
  .loop0:
    %REP 20
        fabs
    %ENDREP
  dec ebx 
  jnz .loop0
%endmacro

%macro testcode 0
  lfsr
%endmacro

%macro t2_testcode 0 
fabs
%endmacro 
