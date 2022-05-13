;// OpenCP Module Player
;// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//
;// DOS4GFIX - Bugfix for IRQs >7 under Rational Systems' DOS4G/W extender
;//
;// revision history: (please note changes here)
;//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//    -first release


.386
.model flat,cpp
locals

.code

oldint17hand dp 0
oldint70rmhand dd 8 dup (0)
datasel dw 0
pmsel dw 0

int17hand proc
  cmp ah,042h
  je @@passup
  jmp cs:oldint17hand
@@passup:

  push ds
  push ebx
  push eax
  mov ds,cs:datasel
  mov @@intnum,al
  add @@intnum,70h
  movzx eax,al
  mov ebx,oldint70rmhand[eax*4]
  xchg ebx,ds:[(70h+eax)*4]

  int 0
org $-1
@@intnum db 0

  xchg ebx,ds:[(70h+eax)*4]
  pop eax
  pop ebx
  pop ds

  iretd
endp

public InitDOS4GFix_
InitDOS4GFix_ proc
local rmseg:word
  pushad

  mov datasel,ds

  mov ax,100h
  mov bx,4
  int 31h
  jc @@err
  mov pmsel,dx
  mov rmseg,ax

  push es
  mov ax,3517h
  int 21h
  mov dword ptr oldint17hand,ebx
  mov word ptr oldint17hand+4,es
  pop es

  push ds
  lea edx,int17hand
  push cs
  pop ds
  mov ax,2517h
  int 21h
  pop ds

  movzx edi,rmseg
  shl edi,4

  xor ecx,ecx
@@loop:
    mov dword ptr [edi+8*ecx],4200b850h
    mov dword ptr [edi+8*ecx+4],0cf5817cdh
    add byte ptr [edi+8*ecx+2],cl

    push ecx
    mov ax,200h
    lea ebx,[ecx+70h]
    int 31h
    shl edx,16
    shrd edx,ecx,16
    pop ecx
    mov dword ptr oldint70rmhand[ecx*4],edx

    push ecx
    mov ax,201h
    lea ebx,[ecx+70h]
    lea edx,[ecx*8]
    mov cx,rmseg
    int 31h
    pop ecx

  @@skipirq:
  inc ecx
  cmp ecx,8
  jne @@loop

  popad
  mov eax,1
  jmp @@done
@@err:
  popad
  xor eax,eax
@@done:
  ret
endp

public CloseDOS4GFix_
CloseDOS4GFix_ proc
  pushad

  push ds
  lds edx,cs:oldint17hand
  mov ax,2517h
  int 21h
  pop ds

  xor ecx,ecx
@@loop:
    push ecx
    mov ax,201h
    lea ebx,[ecx+70h]
    mov edx,dword ptr oldint70rmhand[ecx*4]
    shld ecx,edx,16
    int 31h
    pop ecx
  inc ecx
  cmp ecx,8
  jne @@loop

  mov ax,101h
  mov dx,pmsel
  int 31h

  popad
  ret
endp

end
