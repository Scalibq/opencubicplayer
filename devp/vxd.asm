; VxD-API-Calls and thunks, for devpVxD
.386
.model flat

public VxDInit_, GetAddress_
locals

                  ; sync! (0.2)
VAPC_QUERY        struc
VAPC_QUERY_ESP    dd ?
VAPC_QUERY_PROC   dd ?
VAPC_QUERY_DONE   dd ?
VAPC_QUERY_RES    dd ?
VAPC_QUERY_STACKCOPY dd ?
VAPC_QUERY        ends

.code
VxDInit_        proc

        mov ax, 1600h           ; win inst check
        int 2fh
        test al, 7fh
          jz  @@NoAPI

        push es
        xor edi, edi
        mov ax, 1684h                 ; get vxd api entry point
        mov bx, 5043h
        int 2fh
        mov api_seg, es
        mov api_off, edi
        mov ax, es
        pop es
        or ax, ax
         jz  @@NoAPI

        xor ebx, ebx                  ; get current vm
        mov eax, 1683h
        int 2fh
        mov [myvm], ebx
        mov eax, 0
        call fword ptr api_off
         jc @@APIError
        ret
@@APIError:
        mov eax, -1
        ret
@@NoAPI:
        mov eax, -1
        ret
VxDInit_        endp

GetAddress_      proc         ; "THUNK"
        mov eax, offset query

        mov [eax].VAPC_QUERY_ESP, esp
        add [eax].VAPC_QUERY_ESP, 4       ; ret-adr

        push ebx
        push ecx
        push edx

        mov [eax].VAPC_QUERY_PROC, -1
        mov [eax].VAPC_QUERY_STACKCOPY, 4

        mov ecx, eax

        mov eax, 1
        call fword ptr api_off
         jc @@APIError

        mov ebx, [myvm]
@@NotDoneLoop:
        mov eax, 2
        call fword ptr api_off
        cmp query.VAPC_QUERY_DONE, 0
          jz @@NotDoneLoop
@@Done:
        mov eax, [ecx].VAPC_QUERY_RES
        jmp @@Ret
@@APIError:
        mov eax, -1
@@Ret:
        pop edx
        pop ecx
        pop ebx
        ret
GetAddress_      endp

APCThunk  macro name, stackbytes
        public _&name
.code
_&name      proc
        sub esp, size VAPC_QUERY
        mov eax, esp

        mov [eax].VAPC_QUERY_ESP, eax
        add [eax].VAPC_QUERY_ESP, 4 + size VAPC_QUERY     ; ret-adr

        push ebx
        push ecx
        push edx

        mov ebx, [_a&name]
        mov [eax].VAPC_QUERY_PROC, ebx
        mov [eax].VAPC_QUERY_STACKCOPY, stackbytes

        mov ecx, eax

        mov ebx, [myvm]

        mov eax, 1
        call fword ptr api_off
         jc @@APIError

        mov ebx, [myvm]
        mov edx, 10000
@@NotDoneLoop:
        mov eax, 2
        call fword ptr api_off
        cmp [ecx].VAPC_QUERY_DONE, 0
          jne @@Done
;        dec ecx                       ; timeout :( doesn't work... :)
;          jnz @@NotDoneLoop
        jmp @@NotDoneLoop
        jmp @@APIError
@@Done:
        mov eax, [ecx].VAPC_QUERY_RES
        jmp @@Ret
@@APIError:
        mov eax, -1
@@Ret:
        pop edx
        pop ecx
        pop ebx
        add esp, size VAPC_QUERY
        ret
_&name     endp
.data
public _a&name
_a&name  dd -2
    endm

.data

api_off         dd 0
api_seg         dw 0
myvm            dd 0

APCThunk apcLoadLibrary, 4
APCThunk apcFreeLibrary, 4
APCThunk apcGetProcAddress, 8

APCThunk vplrOpt, 0
APCThunk vplrRate, 0
APCThunk vplrGetBufPos, 0
APCThunk vplrGetPlayPos, 0
APCThunk vplrAdvanceTo, 4
APCThunk vplrGetTimer, 0
APCThunk vplrSetOptions, 8
APCThunk vplrPlay, 8
APCThunk vplrStop, 0
APCThunk vplrGetDeviceStruct, 0
APCThunk vplrDetect, 4
APCThunk vplrInit, 4
APCThunk vplrClose, 0

public _query
_query:
query   VAPC_QUERY  <0, 0, 0, 0, 0>

end
