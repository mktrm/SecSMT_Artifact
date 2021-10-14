;---------------------------------------------------------------------------- Constants to be defined: nopsize:   Size of NOP instructions. (1 - 15) 
; repeat1:   Number of loop repetitions
;
; repeat2:   Number of instructions in loop
;
; t2_repeat1 t2_repeat2: as above but for thread2
;
; (c) Copyright 2013 by Agner Fog. GNU General Public License www.gnu.org/licenses
;-----------------------------------------------------------------------------

%define nthreads    4
%define t2_repeat1  100
%define t2_repeat2  1
%define repeat1  1
%define repeat2  1
%define repeat0 100
%define noptype 2

%include "nops.inc"
%include "lfsr.inc"

%macro testdata 0 
timespec: 
  tv_sec dq 0
  tv_nsec dq 1
%endmacro



%macro send_zero 0
    nop
%endmacro

%macro send_one 0
    nop
%endmacro


;receiver
%macro t2_testcode 0 
  ; should not modify edx
  ;mov dqword [rdi], 1
  ;mov dqword [rdi+8], 0
  mov rax, 35
  mov rdi, timespec
  xor rsi, rsi
  syscall
%endmacro 

%macro testcode 0
  nop
%endmacro