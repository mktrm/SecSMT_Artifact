
%macro lfsr 0
    mov     edx, 4294946017
    ALIGN   16

        ; SENDING ZERO
        SendZero:
        send_zero
    COV_LOOP:  
        send_small_zero
        ; CALC next bit: 
        mov     ecx, edx                                                                  
        shr     dx, 1                                   
        and     ecx, 01H ;  calc LSB                                
        jz      SendZero ;  LSB is zero                                  
        
        ; SENDING ONE:
        send_one

        ; CHECK If DONE 
        cmp     dx, 6369                                
        jz      EXITALL1  ; DONE AND EXIT
        
    
        xor     dx, 0B400H                              
        jmp     COV_LOOP    ; Go back to top    
%endmacro     