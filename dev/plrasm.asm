;// OpenCP Module Player
;// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//
;// aux assembler routines for player devices system
;//
;// revision history: (please note changes here)
;//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//    -first release

.386
.model flat,prolog
locals

.code

public plrClearBuf_
plrClearBuf_ proc ;//edi=buf, ecx=len, eax=unsigned
  cmp ecx,0
  je @@done
  cmp eax,0
  je @@signed
    mov eax,80008000h
@@signed:
  test ebx,2
  jz @@ok1
    stosw
    dec ecx
@@ok1:
  shr ecx,1
  jnc @@ok2
    mov [edi+4*ecx],ax
@@ok2:

  rep stosd

@@done:
  ret
endp

blockbeg2 macro exp
blockexp=exp
blocksize=(1 shl exp)
  push ecx
  and ecx,blocksize-1
  sub ecx,blocksize
  neg ecx
  sub esi,ecx
  sub esi,ecx
  sub edi,ecx
  mov eax,(@@blockend-@@block) shr blockexp
  mul cl
  add eax,offset @@block
  pop ecx
  shr ecx,blockexp
  inc ecx
  jmp eax
@@block:
endm

blockend2 macro
@@blockend:
  add esi,blocksize*2
  add edi,blocksize
  dec ecx
  jnz @@block
endm

public plr16to8_
plr16to8_ proc ;//esi=buf16, edi=buf8, ecx=len
  blockbeg2 6
    i=0
    rept blocksize
      db 8ah, 46h, 2*i+1     ;mov al,ds:[esi+2*i+1]
      db 88h, 47h, i         ;mov ds:[edi+i],al
      i=i+1
    endm
  blockend2
  ret
endp

end
