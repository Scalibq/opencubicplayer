;// OpenCP Module Player
;// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//
;// assembler routines for Quality Mixer
;//
;// revision history: (please note changes here)
;//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//    -first release
;//  -kb980717 Tammo Hinrichs <opencp@gmx.net>
;//    -some pentium optimization on inner loops
;//  -kbwhenever Tammo Hinrichs <opencp@gmx.net>
;//    -used CS addressing here and there for more optimal cache usage

.386
.model flat,prolog
locals

.data

argdb macro name
  org $-1
  name db 0
endm

argdw macro name
  org $-2
  name dw 0
endm

argdd macro name
  org $-4
  name dd 0
endm

MIXQ_PLAYING equ 1
MIXQ_PAUSED equ 2
MIXQ_LOOPED equ 4
MIXQ_PINGPONGLOOP equ 8
MIXQ_PLAY16BIT equ 16
MIXQ_INTERPOLATE equ 32
MIXQ_INTERPOLATEMAX equ 64
MIXQ_PLAYSTEREO equ 128

channel struc
  chsamp dd ?
  chlength dd ?
  chloopstart dd ?
  chloopend dd ?
  chreplen dd ?
  chstep dd ?
  chpos dd ?
  chfpos dw ?
  chstatus db ?,?
ends

playrout dd 0

playquiet proc
  ret
endp

playmono proc
  lea ecx,[edi+2*ecx]
  xor bl,bl
@@lp:
    mov bh,[esi]
    add edi,2
    add ebx,edx
    adc esi,ebp
    mov [edi-2],bx
  cmp edi,ecx
  jb @@lp
  ret
endp

playmono16 proc
  lea ecx,[edi+2*ecx]
  xor bl,bl
@@lp:
    mov bx,[esi+esi]
    add edi,2
    add ebx,edx
    adc esi,ebp
    mov [edi-2],bx
  cmp edi,ecx
  jb @@lp
  ret
endp

playmonoi proc
  lea ecx,[edi+2*ecx]
  mov @@endp,ecx
  xor eax,eax
  mov ecx,ebx

@@lp:
    shr ecx,19
    add edi,2
    mov eax,ecx
    mov cl,[esi]
    mov al,[esi+1]
    add ebx,edx
    mov bx,cs:[4*ecx+1234]
      argdd @@intr1
    adc esi,ebp
    add bx,cs:[4*eax+1234]
      argdd @@intr2

    mov [edi-2],bx
    mov ecx,ebx
  cmp edi,1234
    argdd @@endp
  jb @@lp
  ret

setupmonoi:
  mov @@intr1,ebx
  add ebx,2
  mov @@intr2,ebx
  sub ebx,2
  ret
endp

playmonoi16 proc
  lea ecx,[edi+2*ecx]
  mov @@endp,ecx
  mov eax,eax
  mov ecx,ebx

@@lp:
    shr ecx,19
    add edi,2
    mov eax,ecx
    mov cl,[esi+esi+1]
    mov al,[esi+esi+3]
    mov bx,cs:[4*ecx+1234]
      argdd @@intr1
    mov cl,[esi+esi]
    add bx,[4*eax+1234]
      argdd @@intr2
    mov al,[esi+esi+2]
    add bx,cs:[4*ecx+1234]
      argdd @@intr3
    add ebx,edx
    adc esi,ebp
    add bx,[4*eax+1234]
      argdd @@intr4

    mov [edi-2],bx
    mov ecx,ebx
  cmp edi,1234
    argdd @@endp
  jb @@lp
  ret

setupmonoi16:
  mov @@intr1,ebx
  add ebx,2
  mov @@intr2,ebx
  add ebx,4*32*256
  mov @@intr4,ebx
  sub ebx,2
  mov @@intr3,ebx
  sub ebx,4*32*256
  ret
endp

playmonoi2 proc
  lea ecx,[edi+2*ecx]
  mov @@endp,ecx
  mov eax,eax
  mov ecx,ebx

@@lp:
    shr ecx,20
    add edi,2
    mov eax,ecx
    mov cl,[esi+0]
    mov al,[esi+1]
    mov bx,cs:[8*ecx+1234]
      argdd @@intr1
    mov cl,[esi+2]
    add bx,[8*eax+1234]
      argdd @@intr2

    add ebx,edx
    adc esi,ebp
    add bx,cs:[8*ecx+1234]
      argdd @@intr3

    mov [edi-2],bx
    mov ecx,ebx
  cmp edi,1234
    argdd @@endp
  jb @@lp
  ret

setupmonoi2:
  mov @@intr1,ecx
  add ecx,2
  mov @@intr2,ecx
  add ecx,2
  mov @@intr3,ecx
  sub ecx,4
  ret
endp

playmonoi216 proc
  lea ecx,[edi+2*ecx]
  mov @@endp,ecx
  mov ecx,ebx
  mov eax,eax

@@lp:
    shr ecx,20
    add edi,2
    mov eax,ecx

    mov cl,[esi+esi+1]
    mov al,[esi+esi+3]

    mov bx,cs:[8*ecx+1234]
      argdd @@intr1

    mov cl,[esi+esi+5]
    add bx,[8*eax+1234]
      argdd @@intr2

    mov al,[esi+esi+0]
    add bx,cs:[8*ecx+1234]
      argdd @@intr3

    mov cl,[esi+esi+2]
    add bx,[8*eax+1234]
      argdd @@intr4

    mov al,[esi+esi+4]
    add bx,cs:[8*ecx+1234]
      argdd @@intr5

    add ebx,edx
    adc esi,ebp
    add bx,[8*eax+1234]
      argdd @@intr6

    mov [edi-2],bx
    mov ecx,ebx
  cmp edi,1234
    argdd @@endp
  jb @@lp
  ret

setupmonoi216:
  mov @@intr1,ecx
  add ecx,2
  mov @@intr2,ecx
  add ecx,2
  mov @@intr3,ecx
  add ecx,8*16*256
  mov @@intr6,ecx
  sub ecx,2
  mov @@intr5,ecx
  sub ecx,2
  mov @@intr4,ecx
  sub ecx,8*16*256
  ret
endp

public mixqPlayChannel_
mixqPlayChannel_ proc buf:dword, len:dword, chan:dword, quiet:dword
local inloop:byte, filllen:dword
  mov filllen,0

  mov edi,chan
  cmp quiet,0
  jne @@pquiet

  test [edi].chstatus,MIXQ_INTERPOLATE
  jnz @@intr
    mov eax,offset playmono
    test [edi].chstatus,MIXQ_PLAY16BIT
    jz @@intrfini
      mov eax,offset playmono16
  jmp @@intrfini
@@intr:
  test [edi].chstatus,MIXQ_INTERPOLATEMAX
  jnz @@intrmax
    mov eax,offset playmonoi
    test [edi].chstatus,MIXQ_PLAY16BIT
    jz @@intrfini
      mov eax,offset playmonoi16
  jmp @@intrfini
@@intrmax:
    mov eax,offset playmonoi2
    test [edi].chstatus,MIXQ_PLAY16BIT
    jz @@intrfini
      mov eax,offset playmonoi216
@@intrfini:
  mov playrout,eax
  jmp @@bigloop

@@pquiet:
  mov playrout,offset playquiet

@@bigloop:
  mov ecx,len
  mov ebx,[edi].chstep
  mov edx,[edi].chpos
  mov si,[edi].chfpos
  mov inloop,0
  cmp ebx,0
  je @@playecx
  jg @@forward
    neg ebx
    mov eax,edx
    test [edi].chstatus,MIXQ_LOOPED
    jz @@maxplaylen
    cmp edx,[edi].chloopstart
    jb @@maxplaylen
    sub eax,[edi].chloopstart
    mov inloop,1
    jmp @@maxplaylen
@@forward:
    mov eax,[edi].chlength
    neg si
    sbb eax,edx
    test [edi].chstatus,MIXQ_LOOPED
    jz @@maxplaylen
    cmp edx,[edi].chloopend
    jae @@maxplaylen
    sub eax,[edi].chlength
    add eax,[edi].chloopend
    mov inloop,1

@@maxplaylen:
  xor edx,edx
  shld edx,eax,16
  shl esi,16
  shld eax,esi,16
  add eax,ebx
  adc edx,0
  sub eax,1
  sbb edx,0
  cmp edx,ebx
  jae @@playecx
  div ebx
  cmp ecx,eax
  jb @@playecx
    mov ecx,eax
    cmp inloop,0
    jnz @@playecx
      and [edi].chstatus,not MIXQ_PLAYING
      mov eax,len
      sub eax,ecx
      add filllen,eax
      mov len,ecx

@@playecx:
  push ebp
  push edi
  push ecx

  mov bx,[edi].chfpos
  shl ebx,16
  mov eax,buf

  mov edx,[edi].chstep
  shl edx,16

  mov esi,[edi].chpos
  mov ebp,[edi].chstep
  sar ebp,16
  add esi,[edi].chsamp
  mov edi,eax

  call playrout

  pop ecx
  pop edi
  pop ebp

  add buf,ecx
  add buf,ecx
  sub len,ecx

  mov eax,[edi].chstep
  imul ecx
  shld edx,eax,16
  add [edi].chfpos,ax
  adc [edi].chpos,edx
  mov eax,[edi].chpos

  cmp inloop,0
  jz @@fill

  cmp [edi].chstep,0
  jge @@forward2
    cmp eax,[edi].chloopstart
    jge @@exit
    test [edi].chstatus,MIXQ_PINGPONGLOOP
    jnz @@pong
      add eax,[edi].chreplen
      jmp @@loopiflen
    @@pong:
      neg [edi].chstep
      neg [edi].chfpos
      adc eax,0
      neg eax
      add eax,[edi].chloopstart
      add eax,[edi].chloopstart
      jmp @@loopiflen
@@forward2:
    cmp eax,[edi].chloopend
    jb @@exit
    test [edi].chstatus,MIXQ_PINGPONGLOOP
    jnz @@ping
      sub eax,[edi].chreplen
      jmp @@loopiflen
    @@ping:
      neg [edi].chstep
      neg [edi].chfpos
      adc eax,0
      neg eax
      add eax,[edi].chloopend
      add eax,[edi].chloopend

@@loopiflen:
  mov [edi].chpos,eax
  cmp len,0
  jne @@bigloop

@@fill:
  cmp filllen,0
  je @@exit
  mov eax,[edi].chlength
  mov [edi].chpos,eax
  add eax,[edi].chsamp
  test [edi].chstatus,MIXQ_PLAY16BIT
  jnz @@fill16
    mov ah,[eax]
    mov al,0
    jmp @@filldo
@@fill16:
    mov ax,[eax+eax]
@@filldo:
  mov ecx,filllen
  mov edi,buf
  rep stosw

@@exit:
  ret
endp

public mixqAmplifyChannel_
mixqAmplifyChannel_ proc ;//esi=src, edi=dst, ebx=vol, ecx=len, edx=step
  shl ebx,9
  mov bl,[esi+1]

@@ploop:
    mov eax,[ebx+ebx+1234]
      argdd @@voltab1
    mov bl,[esi]
    add esi,2
    add eax,[ebx+ebx+1234]
      argdd @@voltab2
    mov bl,[esi+1]
    movsx eax,ax
    add [edi],eax
    add edi,edx
  dec ecx
  jnz @@ploop
  ret

setupampchan:
  mov @@voltab1,eax
  add eax,512
  mov @@voltab2,eax
  sub eax,512
  ret
endp

public mixqAmplifyChannelUp_
mixqAmplifyChannelUp_ proc ;//esi=src, edi=dst, ebx=vol, ecx=len, edx=step
  shl ebx,9
  mov bl,[esi+1]

@@ploop:
    mov eax,[ebx+ebx+1234]
      argdd @@voltab1
    mov bl,[esi]
    add esi,2
    add eax,[ebx+ebx+1234]
      argdd @@voltab2
    add ebx,512
    movsx eax,ax
    mov bl,[esi+1]
    add [edi],eax
    add edi,edx
  dec ecx
  jnz @@ploop
  ret

setupampchanup:
  mov @@voltab1,eax
  add eax,512
  mov @@voltab2,eax
  sub eax,512
  ret
endp

public mixqAmplifyChannelDown_
mixqAmplifyChannelDown_ proc ;//esi=src, edi=dst, ebx=vol, ecx=len, edx=step
  shl ebx,9
  mov bl,[esi+1]

@@ploop:
    mov eax,[ebx+ebx+1234]
      argdd @@voltab1
    mov bl,[esi]
    add esi,2
    add eax,[ebx+ebx+1234]
      argdd @@voltab2
    sub ebx,512
    movsx eax,ax
    mov bl,[esi+1]
    add [edi],eax
    add edi,edx
  dec ecx
  jnz @@ploop
  ret

setupampchandown:
  mov @@voltab1,eax
  add eax,512
  mov @@voltab2,eax
  sub eax,512
  ret
endp

public mixqSetupAddresses_
mixqSetupAddresses_ proc
  call setupampchan
  call setupampchanup
  call setupampchandown
  call setupmonoi
  call setupmonoi16
  call setupmonoi2
  call setupmonoi216
  ret
endp

end
