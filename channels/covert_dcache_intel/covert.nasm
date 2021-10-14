;---------------------------------------------------------------------------- Constants to be defined: nopsize:   Size of NOP instructions. (1 - 15) 
; repeat1:   Number of loop repetitions
;
; repeat2:   Number of instructions in loop
;
; t2_repeat1 t2_repeat2: as above but for thread2
;
; (c) Copyright 2013 by Agner Fog. GNU General Public License www.gnu.org/licenses
;-----------------------------------------------------------------------------
%define nthreads    2
%define t2_repeat1  1
%define t2_repeat2  1
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
  %define ZERO_ITR 250
%endif 

%ifdef all_ones 
  %define ONE_ITR 10000
%else 
  %define ONE_ITR 50
%endif 


%macro send_zero 0
; should not modify edx
  mov ebx, ZERO_ITR
  .loop0:
    %REP 50
        nop
    %ENDREP
  dec ebx 
  jnz .loop0
%endmacro

%macro send_small_zero 0
; should not modify edx
  mov ebx, 500
  %%loop0:
    %REP 50
        nop
    %ENDREP
  dec ebx 
  jnz %%loop0
%endmacro

%macro send_one 0
; should not modify edx
  mov ebx, ONE_ITR
  .loop0:
   %assign i 0
   %rep blocks
    clflush [rdi+64*i+400h]
    %assign i i+1
   %endrep
  dec ebx 
  jnz .loop0
%endmacro

;receiver
%macro t2_testcode 0 
  %assign i 0
  %rep blocks
    mov rax, [rdi+64*i]
    mfence
    %assign i i+1
  %endrep
%endmacro 

%macro testcode 0
  lfsr
%endmacro