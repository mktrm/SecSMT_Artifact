
%define nthreads    2
%define t2_repeat1  5
%define t2_repeat2  1
%define repeat1  1
%define repeat2  1
%define repeat0 100000
%define noptype 2
; Define any undefined macros
%define blocks 1



%include "nops.inc"
%include "lfsr.inc"

%macro testdata 0 
 ALIGN   4096*8*16, DB 0
 TLB_DATA:
 times 128*(4096+64)  DB 0 ; at least 
%endmacro 

%macro testinit1 0
lea rsi, [TLB_DATA]
mov rax, rsi 
add rax, 4096+64
mov rbx, 128
loop_top:
mov [rax - (4096 + 64)], rax
add rax, (4096 + 64)
dec rbx
jnz loop_top

%endmacro 

%ifdef all_zeros
  %define ZERO_ITR 10000
%else 
  %define ZERO_ITR 700
%endif 


%ifdef all_ones 
  %define ONE_ITR 2000
%else 
  %define ONE_ITR 100
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
  mov ebx, 1000
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
  mov rsi, (4096+64)*64+TLB_DATA
  .loop1:     
    %rep 64
      mov rax, [rsi + (4096+64)*i]
      %assign i i+1
    %endrep
      nop
   dec ebx
   jnz .loop1

%endmacro




%macro t2_testinit3 0
 mov rax, rsi
%endmacro

%macro t2_testcode 0 
%assign i 0
%rep 55
  mov rax, [rsi + (4096+64)*i]
%assign i i+1
%endrep
%endmacro 




%macro testcode 0
  lfsr
%endmacro