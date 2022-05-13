.386p
.model flat, prolog

_TEXT segment
extrn __GETDS: proc;
extrn cscheduler_: proc;
public Scheduler_, SwitchTask_;
                                ; 00 02 04 06 08 0a 0c 0e 10 12 14 16 18 1a 1c 1e 20 22 24 26 28 2a 2c 2e 30 32 34 36 38 3a
Scheduler_ proc
                                ; _eip_ __cs_ _flg_
        pushad
                                ; _edi_ _esi_ _ebp_ _esp_ _ebx_ _edx_ _ecx_ _eax_ _eip_ __cs_ _flg_
        push    ds
                                ; __ds_ _edi_ _esi_ _ebp_ _esp_ _ebx_ _edx_ _ecx_ _eax_ _eip_ __cs_ _flg_
        push    es
                                ; __es_ __ds_ _edi_ _esi_ _ebp_ _esp_ _ebx_ _edx_ _ecx_ _eax_ _eip_ __cs_ _flg_
        push    fs
                                ; __fs_ __es_ __ds_ _edi_ _esi_ _ebp_ _esp_ _ebx_ _edx_ _ecx_ _eax_ _eip_ __cs_ _flg_
        push    gs
                                ; __gs_ __fs_ __es_ __ds_ _edi_ _esi_ _ebp_ _esp_ _ebx_ _edx_ _ecx_ _eax_ _eip_ __cs_ _flg_
        mov     ebp, esp
        cld
        call __GETDS
        cmp _noswitch, 0
         ja nos
        push ds
        pop es
        mov _esp, esp
        mov _ss, ss
        call cscheduler_
        mov ss, _ss
        mov esp, _esp
nos:
;        pushfd
;        call    fword ptr ds:_oldisr
        pop     gs
        pop     fs
        pop     es
        pop     ds
        popad
;        mov eax, 0ffffffffh
;        mov eax, [eax]
        iret
Scheduler_ endp
SwitchTask_     proc
                                ; 00 02 04 06 08 0a 0c 0e 10 12 14 16 18 1a 1c 1e 20 22 24 26 28 2a 2c 2e 30 32 34 36 38 3a
;on int:                        ; _eip_ __cs_ _flg_
;on near call:                  ; _eip_
                push cs
                                ; _cs__ _eip_
                pushfd
                                ; _flg_ _cs__ _eip_
                mov eax, [esp+0]        ; flg
                mov ebx, [esp+4]        ; cs
                mov ecx, [esp+8]        ; eip
                mov [esp+0], ecx
                mov [esp+4], ebx
                mov [esp+8], eax
                cli
                jmp Scheduler_
SwitchTask_     endp

public _oldisr, _ss, _esp, _noswitch;
_noswitch       dd 0
_oldisr:        db 6 dup(?)
_esp            dd ?
_ss             dd ?
ends _TEXT
end
