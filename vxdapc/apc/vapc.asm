.386p
            ; keep this in sync!
VAPC_IOCTL_GET_VERSION       equ 1
VAPC_IOCTL_REGISTER_HOST     equ 2
VAPC_IOCTL_UNREGISTER_HOST   equ 3

VAPC_VERMAJ   equ     0
VAPC_VERMIN   equ     20h
VAPC_DEV_ID   equ     5043h

.xlist
        include vmm.inc
        INCLUDE Shell.Inc
        include debug.inc
        include vwin32.inc
.list
            ; keep this in sync! (0.2)
VAPC_QUERY        struc
VAPC_QUERY_ESP    dd ?
VAPC_QUERY_PROC   dd ?
VAPC_QUERY_DONE   dd ?
VAPC_QUERY_RES    dd ?
VAPC_QUERY_STACKCOPY dd ?
VAPC_QUERY        ends

Declare_Virtual_Device VAPC, VAPC_VERMAJ, VAPC_VERMIN, VAPC_Control,\
                        VAPC_DEV_ID,, VAPC_API_Proc, VAPC_API_Proc

VxD_LOCKED_DATA_SEG
        align   4
        vapcHostThreadHandle  dd 0
        vapcHostProc          dd 0
VxD_LOCKED_DATA_ENDS

VxD_LOCKED_CODE_SEG

BeginProc VAPC_Control
        Control_Dispatch Device_Init,               VAPC_Device_Init
        Control_Dispatch Sys_Dynamic_Device_Init,   VAPC_Device_Init
        Control_Dispatch System_Exit,               VAPC_System_Exit
        Control_Dispatch Sys_Dynamic_Device_Exit,   VAPC_System_Exit
        Control_Dispatch W32_DeviceIOControl,       VAPC_W32_DeviceIOControl
        clc
        ret
EndProc VAPC_Control

BeginProc VAPC_Device_Init
        clc
        ret
EndProc   VAPC_Device_Init

BeginProc VAPC_System_Exit
        clc
        ret
EndProc   VAPC_System_Exit

BeginProc VAPC_W32_DeviceIOControl
                        ; registers:
                        ; ecx = dwService
                        ; ebx = dwDDB
                        ; edx = hDevice
                        ; esi = lpDIOCParms
        cmp ecx, DIOC_OPEN
          jne @f
        Trace_Out "VAPC: DIOC_OPEN"
        xor eax, eax
        ret
@@:
        cmp ecx, DIOC_CLOSEHANDLE
          jne @f
        Trace_Out "VAPC: DIOC_CLOSEHANDLE"
        xor eax, eax
        ret
@@:
        cmp ecx, VAPC_IOCTL_REGISTER_HOST
          je VAPC_Register_Host
        cmp ecx, VAPC_IOCTL_UNREGISTER_HOST
          je VAPC_UnRegister_Host
        cmp ecx, VAPC_IOCTL_GET_VERSION
          je VAPC_Get_Version_Handler

        pushfd
        pushad
        Trace_Out "Illegal ioctl. ecx: #ecx"
        popad
        popfd

        mov eax, -1
        ret
EndProc   VAPC_W32_DeviceIOControl

BeginProc VAPC_Get_Version_Handler
        push edi
        push esi
        mov edi, [esi.lpcbBytesReturned]
        mov esi, [esi.lpvOutBuffer]
        mov dword ptr [esi], (VAPC_VERMIN) OR (VAPC_VERMAJ SHL 16) 
        test edi, edi
          jz IOCTL_Exit
        mov dword ptr [edi], 4
IOCTL_Exit:
        pop esi
        pop edi
        xor eax, eax
        ret
EndProc   VAPC_Get_Version_Handler

BeginProc VAPC_Register_Host
        push edi
        push esi
        cmp vapcHostProc, 0           ; already a registered host?
        Trace_OutNZ "VAPC: already registered!"
          jnz IOCTL_EXIT

        mov esi, [esi.lpvInBuffer]

        mov eax, [esi]
        mov vapcHostProc, eax

        VMMCall Get_Cur_Thread_Handle
        mov vapcHostThreadHandle, edi

;        mov edi, [esi.lpcbBytesReturned]
        Trace_Out "VAPC: now Registered"

;        test edi, edi
;          jz IOCTL_Exit
;        mov dword ptr [edi], 0
        jmp IOCTL_Exit
EndProc   VAPC_Register_Host

BeginProc VAPC_UnRegister_Host
        push edi
        push esi
        mov vapcHostProc, 0

        mov edi, [esi.lpcbBytesReturned]
        test edi, edi
          jz IOCTL_Exit
        mov dword ptr [edi], 0
        jmp IOCTL_Exit
EndProc   VAPC_UnRegister_Host

BeginProc VAPC_API_Proc
        mov eax, [ebp].Client_EAX   ; keep this in sync!
        or eax, eax                 ; (0) GetVersion
          je API_GetVersion
        dec eax                     ; (1) DoCall
          jz API_DoCall
        dec eax                     ; (2) Yield
          jz API_Yield
        stc
        ret
EndProc   VAPC_API_Proc

BeginProc API_GetVersion
        mov [ebp].Client_EAX, (VAPC_VERMIN) OR (VAPC_VERMAJ SHL 16)
        ret
EndProc   API_GetVersion

BeginProc API_DoCall                        ; notice:
                                            ; _VWIN32_QueueUserApc can't be
                                            ; called at interrupt-time.
                                            ; (sounds easy, but is a real
                                            ;  problem, since the cp does
                                            ;  a lot calls in the irq.)
        push esi
        push ecx                            ; is register-preserving
        push edx                            ; neccessary in this case?
        mov esi, [ebp].Client_ECX
        mov [esi].VAPC_QUERY_DONE, 0
        VxDCall _VWIN32_QueueUserApc, <vapcHostProc, esi, vapcHostThreadHandle>
        mov [ebp].Client_EAX, eax
        pop edx
        pop ecx
        pop esi
        ret
EndProc   API_DoCall
BeginProc API_Yield
        VMMCall Time_Slice_Sleep
        clc
        ret
EndProc   API_Yield


VxD_LOCKED_CODE_ENDS

end
