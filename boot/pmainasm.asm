;// OpenCP Module Player
;// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//
;// ASM routines for CP.EXE (includes exception handlers etc.)
;//
;// revision history: (please note changes here)
;//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//    -first release
;//  -fd981220   Felix Domke    <tmbinc@gmx.net>
;//    -added public _pagefaultstack

.386p
.model flat,prolog
locals

.data?

db 1024 dup (?)
pagefaultstack:
_pagefaultstack:

public _pagefaultstack;

.code

extrn __GETDS:proc

public _pagefaultchain
_pagefaultchain dp 0
public _zerodivchain
_zerodivchain dp 0

public crithnd_
crithnd_ proc
  mov al,0
  test ah,20h
  jnz @@ok
    mov al,3
@@ok:
  iretd
endp

public breakhnd_
breakhnd_ proc
  iretd
endp

extrn faultproc_:proc

public pagefaulthnd_
pagefaulthnd_ proc
  push eax
  mov eax,cs
  cmp ax,word ptr [esp+20]
  pop eax
  jne @@chain

  push eax
  push edx
  push ebx
  cld
  mov edx,[esp+24]
  push ds
  push es
  call __GETDS
  mov eax,ds
  mov es,eax
  mov word ptr pagefaultstack-4,ss
  mov dword ptr pagefaultstack-8,esp
  mov ss,eax
  mov ebx,0
  lea esp,pagefaultstack-8
  mov eax,cr2
  call faultproc_
  lss esp,[esp]
  pop es
  pop ds
  pop ebx
  pop edx
  pop eax
  retf

@@chain:
  jmp cs:_pagefaultchain
endp

public zerodivhnd_
zerodivhnd_ proc
  push eax
  mov eax,cs
  cmp ax,word ptr [esp+20]
  pop eax
  jne @@chain

  push eax
  push edx
  push ebx
  cld
  mov edx,[esp+24]
  push ds
  push es
  call __GETDS
  mov eax,ds
  mov es,eax
  mov word ptr pagefaultstack-4,ss
  mov dword ptr pagefaultstack-8,esp
  mov ss,eax
  mov ebx,1
  lea esp,pagefaultstack-8
  call faultproc_
  lss esp,[esp]
  pop es
  pop ds
  pop ebx
  pop edx
  pop eax
  retf

@@chain:
  jmp cs:_zerodivchain
endp

end
