;// OpenCP Module Player
;// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//
;// WAVPlay assembler routines for amplifying/clipping
;//
;// revision history: (please note changes here)
;//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//    -first release

.386
.model flat,prolog
locals

.code

blockbeg6b macro exp
blockexp=exp
blocksize=(1 shl exp)
  push ecx
  and ecx,blocksize-1
  sub ecx,blocksize
  neg ecx
  sub esi,ecx
  sub esi,ecx
  sub esi,ecx
  sub esi,ecx
  sub edi,ecx
  sub edi,ecx
  sub edi,ecx
  sub edi,ecx
  mov eax,(@@blockend-@@block) shr blockexp
  mul cl
  add eax,offset @@block
  pop ecx
  shr ecx,blockexp
  inc ecx
  jmp eax
  ret
@@block:
endm

blockend6b macro
@@blockend:
  add esi,blocksize*4
  add edi,blocksize*4
  dec ecx
  jnz @@block
endm

public mixClipAlt2_
mixClipAlt2_ proc ;//esi=src, edi=dst, ebx=tab, ecx=len
  i=0
  push ebp
  xor edx,edx
  blockbeg6b 4
    nop
    nop
    rept blocksize
      mov dl,[esi+i+1]
      mov ebp,[ebx+4*edx]
      mov eax,[ebx+2*edx+1024]
      mov dl,[esi+i]
      add eax,[ebp+2*edx]
      mov [edi+i],ax
      i=i+4
    endm
  blockend6b
  pop ebp
  ret
endp

blockbeg6 macro exp
blockexp=exp
blocksize=(1 shl exp)
  push ecx
  and ecx,blocksize-1
  sub ecx,blocksize
  neg ecx
  sub esi,ecx
  sub esi,ecx
  sub edi,ecx
  sub edi,ecx
  mov eax,(@@blockend-@@block) shr blockexp
  mul cl
  add eax,offset @@block
  pop ecx
  shr ecx,blockexp
  inc ecx
  jmp eax
  ret
@@block:
endm

blockend6 macro
@@blockend:
  add esi,blocksize*2
  add edi,blocksize*2
  dec ecx
  jnz @@block
endm

public mixClipAlt_
mixClipAlt_ proc ;//esi=src, edi=dst, ebx=tab, ecx=len
  i=0
  push ebp
  xor edx,edx
  blockbeg6 4
    nop
    nop
    rept blocksize
      mov dl,[esi+i+1]
      mov ebp,[ebx+4*edx]
      mov eax,[ebx+2*edx+1024]
      mov dl,[esi+i]
      add eax,[ebp+2*edx]
      mov [edi+i],ax
      i=i+2
    endm
  blockend6
  pop ebp
  ret
endp

end
