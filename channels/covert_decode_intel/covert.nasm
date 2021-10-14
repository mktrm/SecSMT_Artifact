
%define nthreads    4
%define t2_repeat1  20
%define t2_repeat2  50
%define repeat1  1
%define repeat2  1
%define repeat0 100000
%define noptype 2
; Define any undefined macros


%include "nops.inc"
%include "lfsr.inc"

%ifdef all_zeros
  %define ZERO_ITR 1000
%else 
  %define ZERO_ITR 30
%endif 

%ifdef all_ones 
  %define ONE_ITR 1000
%else 
  %define ONE_ITR 200
%endif 

%macro cacheMiss 0
 clflush [rdi]
 mfence
 mov rax, [rdi]
%endmacro

%macro send_zero 0
; should not modify edx
  mov ebx, ZERO_ITR
  .loop1:
   cacheMiss
   dec ebx
   jnz .loop1
%endmacro

%macro send_small_zero 0
; should not modify edx
  mov ebx, 9
  .loop1:
   cacheMiss
   dec ebx
   jnz .loop1
%endmacro

%macro send_one 0
; should not modify edx
  mov ebx, ONE_ITR
  .loop0:
    %REP 50
        nop
    %ENDREP
  dec ebx 
  jnz .loop0
%endmacro

;receiver
%macro t2_testcode 0 
    nop
%endmacro 

%macro testcode 0
  lfsr
%endmacro