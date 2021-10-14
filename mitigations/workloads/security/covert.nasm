;---------------------------------------------------------------------------- Constants to be defined: nopsize:   Size of NOP instructions. (1 - 15) 
; repeat1:   Number of loop repetitions
;
; repeat2:   Number of instructions in loop
;
; t2_repeat1 t2_repeat2: as above but for thread2
;
; (c) Copyright 2013 by Agner Fog. GNU General Public License www.gnu.org/licenses
;-----------------------------------------------------------------------------
%define nthreads    1
%define repeat1  100
%define repeat2  25
%define repeat0 100
%define noptype 2
%define processors 1,9
; Define any undefined macros


%include "../nops.inc"


%macro testcode 0 
   %if sender == 1
      mov byte al,  0x13 
      mov byte bl,  0x13 
      mov byte cl,  0x13 
      mov byte dl,  0x13 
   %elif sender == 2
      add rax, rax 
      add rbx, rbx
      add rcx, rcx
      add rdx, rdx
   %elif sender == 3
      xor rax, rax
      xor rbx, rbx
      xor rcx, rcx
      xor rdx, rdx
   %elif sender == 4
      bt  rax, 1
      bt  rbx, 1
      bt  rcx, 1 
      bt  rdx, 1 
   %elif sender == 5
      bts rax, 1
      bts rbx, 1
      bts rcx, 1
      bts rdx, 1
   %elif sender == 6
      movq xmm1, xmm1
      movq xmm2, xmm2
      movq xmm3, xmm3
      movq xmm4, xmm4
   %elif sender == 7
      vtestps ymm1, ymm1
      vtestps ymm2, ymm2
      vtestps ymm3, ymm3
      vtestps ymm4, ymm4
   %elif sender == 8
      lea rax,[4*eax+edi+40960]
      lea rbx,[8*eax+edi+409623]
      lea rcx,[4*eax+edi+40960]
      lea rdx,[8*eax+edi+409623]
   %elif sender == 9
      nop8
      nop8
      nop8
      nop8
   %elif sender == 10 
      mov rax, [rdi]
      mov rbx, [rdi]
      mov rcx, [rdi]
      mov rdx, [rdi]
   %elif sender == 11
      cpuid
      cpuid
      cpuid
      cpuid
   %elif sender == 12 
      lfence
      lfence
      lfence
      lfence
   %else
      nop
      nop
      nop
      nop
   %endif
%endmacro 