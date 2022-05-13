;// OpenCP Module Player
;// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//
;// Mixer asm routines for display etc
;//
;// revision history: (please note changes here)
;//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//    -first release

.386
.model flat,prolog
locals

.data

MCP_PLAYING equ 1
MCP_PAUSED equ 2
MCP_LOOPED equ 4
MCP_PINGPONGLOOP equ 8
MCP_PLAY16BIT equ 16
MCP_INTERPOLATE equ 32
MCP_MAX equ 64
MCP_PLAY32BIT equ 128

channel struc
  chsamp dd ?
  chlength dd ?
  chloopstart dd ?
  chloopend dd ?
  chreplen dd ?
  chstep dd ?
  chpos dd ?
  chfpos dw ?
  chstatus dw ?
  chvoltabs dd 2 dup (?)
ends

playrout dd 0
public _mixIntrpolTab
_mixIntrpolTab dd 0
public _mixIntrpolTab2
_mixIntrpolTab2 dd 0
interpolate dd 0
play16bit dd 0
play32bit dd 0
render16bit dd 0
voltabs dd 2 dup (0)


argdd macro name
  org $-4
  name dd 0
endm

argdw macro name
  org $-2
  name dw 0
endm

playmono proc
  cmp interpolate,0
  jnz playmonoi

  cmp play16bit,0
  jnz playmono16

  cmp play32bit,0
  jnz playmono32

  cmp ebp,0
  je @@done

  mov eax,voltabs[0]
  mov @@vol1,eax
  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov bl,[esi]
    add edx,1234
      argdd @@edx
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add [edi],eax
    add edi,4
  dec ebp
  jnz @@lp

@@done:
  ret
endp

playmono16 proc
  cmp ebp,0
  je @@done

  mov eax,voltabs[0]
  mov @@vol1,eax
  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov bl,[esi+esi+1]
    add edx,1234
      argdd @@edx
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add [edi],eax
    add edi,4
  dec ebp
  jnz @@lp

@@done:
  ret
endp

playmono32 proc
  cmp ebp,0
  je @@done

  fld [edi].chvoltabs[0]
  fld [gscale]
  fmul
  fstp [scale]

  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    fld  dword ptr [esi*4]
    fld  dword ptr [scale]
    fmul
    fistp [integer]
    mov eax, [integer]

    add edx,1234
      argdd @@edx

    adc esi,1234
      argdd @@ebp
    add [edi],eax
    add edi,4
  dec ebp
  jnz @@lp

@@done:
  ret
endp



playmonoi proc
  cmp render16bit,0
  jnz playmonoir

  cmp play16bit,0
  jnz playmonoi16

  cmp play32bit,0
  jnz playmono32            ; !!


  cmp ebp,0
  je @@done

  mov eax,_mixIntrpolTab
  mov @@int0,eax
  inc eax
  mov @@int1,eax
  mov eax,voltabs[0]
  mov @@vol1,eax
  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov ecx,edx
    shr ecx,20
    mov cl,[esi]
    mov bl,[ecx+ecx+1234]
      argdd @@int0
    mov cl,[esi+1]
    add bl,[ecx+ecx+1234]
      argdd @@int1

    add edx,1234
      argdd @@edx
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add [edi],eax
    add edi,4
  dec ebp
  jnz @@lp

@@done:
  ret
endp

playmonoi16 proc
  cmp ebp,0
  je @@done

  mov eax,_mixIntrpolTab
  mov @@int0,eax
  inc eax
  mov @@int1,eax
  mov eax,voltabs[0]
  mov @@vol1,eax
  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov ecx,edx
    shr ecx,20
    mov cl,[esi+esi+1]
    mov bl,[ecx+ecx+1234]
      argdd @@int0
    mov cl,[esi+esi+3]
    add bl,[ecx+ecx+1234]
      argdd @@int1

    add edx,1234
      argdd @@edx
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add [edi],eax
    add edi,4
  dec ebp
  jnz @@lp

@@done:
  ret
endp

playmonoir proc
  cmp play16bit,0
  jne playmonoi16r

  cmp play32bit,0
    jne playmono32            ;!!

  cmp ebp,0
  je @@done

  mov eax,_mixIntrpolTab2
  mov @@int0,eax
  add eax,2
  mov @@int1,eax

  mov eax,voltabs[0]
  mov @@vol1,eax
  add eax,1024
  mov @@vol2,eax

  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov ecx,edx
    shr ecx,19
    mov cl,[esi]
    mov ebx,[4*ecx+1234]
      argdd @@int0
    mov cl,[esi+1]
    add ebx,[4*ecx+1234]
      argdd @@int1
    movzx ecx,bh
    movzx ebx,bl

    add edx,1234
      argdd @@edx
    mov eax,[4*ecx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add eax,[4*ebx+1234]
      argdd @@vol2
    add [edi],eax
    add edi,4
  dec ebp
  jnz @@lp

@@done:
  ret
endp

playmonoi16r proc
  cmp ebp,0
  je @@done

  mov eax,_mixIntrpolTab2
  mov @@int0,eax
  add eax,2
  mov @@int1,eax

  mov eax,voltabs[0]
  mov @@vol1,eax
  add eax,1024
  mov @@vol2,eax

  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov ecx,edx
    shr ecx,19
    mov cl,[esi+esi+1]
    mov ebx,[4*ecx+1234]
      argdd @@int0
    mov cl,[esi+esi+3]
    add ebx,[4*ecx+1234]
      argdd @@int1
    movzx ecx,bh
    movzx ebx,bl

    add edx,1234
      argdd @@edx
    mov eax,[4*ecx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add eax,[4*ebx+1234]
      argdd @@vol2
    add [edi],eax
    add edi,4
  dec ebp
  jnz @@lp

@@done:
  ret
endp

playodd proc
  cmp interpolate,0
  jnz playoddi

  cmp play16bit,0
  jnz playodd16

  cmp play32bit,0
  jne @@done            ;!!

  cmp ebp,0
  je @@done

  mov eax,voltabs[0]
  mov @@vol1,eax
  mov eax,voltabs[4]
  mov @@vol2,eax
  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov bl,[esi]
    add edx,1234
      argdd @@edx
    mov ax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add [edi],eax
    mov ax,[4*ebx+1234]
      argdd @@vol2
    add [edi+4],eax
    add edi,8
  dec ebp
  jnz @@lp

@@done:
  ret
endp

playodd16 proc
  cmp ebp,0
  je @@done

  mov eax,voltabs[0]
  mov @@vol1,eax
  mov eax,voltabs[4]
  mov @@vol2,eax
  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov bl,[esi+esi+1]
    add edx,1234
      argdd @@edx
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add [edi],eax
    mov ax,[4*ebx+1234]
      argdd @@vol2
    add [edi+4],eax
    add edi,8
  dec ebp
  jnz @@lp

@@done:
  ret
endp

playoddi proc
  cmp render16bit,0
  jnz playoddir

  cmp play16bit,0
  jnz playoddi16

  cmp play32bit,0
  jne @@done            ;!!

  cmp ebp,0
  je @@done

  mov eax,_mixIntrpolTab
  mov @@int0,eax
  inc eax
  mov @@int1,eax
  mov eax,voltabs[0]
  mov @@vol1,eax
  mov eax,voltabs[4]
  mov @@vol2,eax
  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov ecx,edx
    shr ecx,20
    mov cl,[esi]
    mov bl,[ecx+ecx+1234]
      argdd @@int0
    mov cl,[esi+1]
    add bl,[ecx+ecx+1234]
      argdd @@int1

    add edx,1234
      argdd @@edx
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add [edi],eax
    mov eax,[4*ebx+1234]
      argdd @@vol2
    add [edi+4],eax
    add edi,8
  dec ebp
  jnz @@lp

@@done:
  ret
endp

playoddi16 proc
  cmp ebp,0
  je @@done

  mov eax,_mixIntrpolTab
  mov @@int0,eax
  inc eax
  mov @@int1,eax
  mov eax,voltabs[0]
  mov @@vol1,eax
  mov eax,voltabs[4]
  mov @@vol2,eax
  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov ecx,edx
    shr ecx,20
    mov cl,[esi+esi+1]
    mov bl,[ecx+ecx+1234]
      argdd @@int0
    mov cl,[esi+esi+3]
    add bl,[ecx+ecx+1234]
      argdd @@int1

    add edx,1234
      argdd @@edx
    mov eax,[4*ebx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add [edi],eax
    mov eax,[4*ebx+1234]
      argdd @@vol2
    add [edi+4],eax
    add edi,8
  dec ebp
  jnz @@lp

@@done:
  ret
endp

playoddir proc
  cmp play16bit,0
  jnz playoddi16r

  cmp play32bit,0
  jne @@done            ;!!

  cmp ebp,0
  je @@done

  mov eax,_mixIntrpolTab2
  mov @@int0,eax
  add eax,2
  mov @@int1,eax
  mov eax,voltabs[0]
  mov @@vol1,eax
  add eax,1024
  mov @@vol1b,eax
  mov eax,voltabs[4]
  mov @@vol2,eax
  add eax,1024
  mov @@vol2b,eax
  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov ecx,edx
    shr ecx,19
    mov cl,[esi]
    mov ebx,[4*ecx+1234]
      argdd @@int0
    mov cl,[esi+1]
    add ebx,[4*ecx+1234]
      argdd @@int1
    movzx ecx,bh
    movzx ebx,bl

    add edx,1234
      argdd @@edx
    mov eax,[4*ecx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add eax,[4*ebx+1234]
      argdd @@vol1b
    add [edi],eax
    mov eax,[4*ecx+1234]
      argdd @@vol2
    add eax,[4*ebx+1234]
      argdd @@vol2b
    add [edi+4],eax
    add edi,8
  dec ebp
  jnz @@lp

@@done:
  ret
endp

playoddi16r proc
  cmp ebp,0
  je @@done

  mov eax,_mixIntrpolTab2
  mov @@int0,eax
  add eax,2
  mov @@int1,eax
  mov eax,voltabs[0]
  mov @@vol1,eax
  add eax,1024
  mov @@vol1b,eax
  mov eax,voltabs[4]
  mov @@vol2,eax
  add eax,1024
  mov @@vol2b,eax
  mov eax,[edi].chstep
  shl eax,16
  mov @@edx,eax
  mov eax,[edi].chstep
  sar eax,16
  mov @@ebp,eax

  mov dx,[edi].chfpos
  shl edx,16
  mov esi,[edi].chpos
  add esi,[edi].chsamp
  mov edi,ecx
  xor ebx,ebx

@@lp:
    mov ecx,edx
    shr ecx,19
    mov cl,[esi+esi+1]
    mov ebx,[4*ecx+1234]
      argdd @@int0
    mov cl,[esi+esi+3]
    add ebx,[4*ecx+1234]
      argdd @@int1
    movzx ecx,bh
    movzx ebx,bl

    add edx,1234
      argdd @@edx
    mov eax,[4*ecx+1234]
      argdd @@vol1
    adc esi,1234
      argdd @@ebp
    add eax,[4*ebx+1234]
      argdd @@vol1b
    add [edi],eax
    mov eax,[4*ecx+1234]
      argdd @@vol2
    add eax,[4*ebx+1234]
      argdd @@vol2b
    add [edi+4],eax
    add edi,8
  dec ebp
  jnz @@lp

@@done:
  ret
endp

public mixPlayChannel_
mixPlayChannel_ proc buf:dword, len:dword, chan:dword, stereo:dword
local inloop:byte
  mov interpolate,0
  mov play16bit,0
  mov play32bit,0
  mov render16bit,0

  mov edi,chan
  test [edi].chstatus,MCP_PLAYING
  jz @@exit
  test [edi].chstatus,MCP_INTERPOLATE
  jz @@nointr
    mov interpolate,1
    test [edi].chstatus,MCP_MAX
    jz @@nointr
      mov render16bit,1
@@nointr:
  test [edi].chstatus,MCP_PLAY16BIT
  jz @@no16bit
    mov play16bit,1
@@no16bit:
  test [edi].chstatus,MCP_PLAY32BIT
  jz @@no32bit
    mov play32bit,1
@@no32bit:

  cmp stereo,0
  jne @@pstereo

@@pmono:
  mov eax,[edi].chvoltabs[0]
  mov voltabs[0],eax
  mov playrout,offset playmono
  jmp @@bigloop

@@pstereo:
  mov eax,[edi].chvoltabs[0]
  mov ebx,[edi].chvoltabs[4]
  mov voltabs[0],eax
  mov voltabs[4],ebx
  mov playrout,offset playodd

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
    test [edi].chstatus,MCP_LOOPED
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
    test [edi].chstatus,MCP_LOOPED
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
      and [edi].chstatus,not MCP_PLAYING
      mov len,ecx

@@playecx:
  push ebp
  push edi
  push ecx
  mov eax,ecx
  mov ecx,buf
  mov ebp,eax

  call playrout

  pop ecx
  pop edi
  pop ebp

  mov eax,ecx
  shl eax,2
  cmp stereo,0
  je @@m2
    shl eax,1
@@m2:
  add buf,eax
  sub len,ecx

  mov eax,[edi].chstep
  imul ecx
  shld edx,eax,16
  add [edi].chfpos,ax
  adc [edi].chpos,edx
  mov eax,[edi].chpos

  cmp inloop,0
  jz @@exit

  cmp [edi].chstep,0
  jge @@forward2
    cmp eax,[edi].chloopstart
    jge @@exit
    test [edi].chstatus,MCP_PINGPONGLOOP
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
    test [edi].chstatus,MCP_PINGPONGLOOP
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

@@exit:
  ret
endp

;//*************************************************************************

public mixClip_
mixClip_ proc ;//esi=src, edi=dst, ebx=tab, ecx=len, edx=max
  mov @@amp1,ebx
  add ebx,512
  mov @@amp2,ebx
  add ebx,512
  mov @@amp3,ebx
  sub ebx,1024
  mov @@max,edx
  neg edx
  mov @@min,edx

  xor edx,edx
  mov dl,byte ptr @@min
  mov eax,[ebx+2*edx]
  mov dl,byte ptr @@min+1
  add eax,[ebx+512+2*edx]
  mov dl,byte ptr @@min+2
  add eax,[ebx+1024+2*edx]
  mov @@minv,ax
  mov dl,byte ptr @@max
  mov eax,[ebx+2*edx]
  mov dl,byte ptr @@max+1
  add eax,[ebx+512+2*edx]
  mov dl,byte ptr @@max+2
  add eax,[ebx+1024+2*edx]
  mov @@maxv,ax
  lea ecx,[2*ecx+edi]
  mov @@endp1,ecx
  mov @@endp2,ecx
  mov @@endp3,ecx
  xor ebx,ebx
  xor ecx,ecx
  xor edx,edx

@@lp:
    cmp dword ptr [esi],1234
      argdd @@min
    jl @@low
    cmp dword ptr [esi],1234
      argdd @@max
    jg @@high

    mov bl,[esi]
    mov cl,[esi+1]
    mov dl,[esi+2]
    mov eax,[2*ebx+1234]
      argdd @@amp1
    add eax,[2*ecx+1234]
      argdd @@amp2
    add eax,[2*edx+1234]
      argdd @@amp3
    mov [edi],ax
    add edi,2
    add esi,4
  cmp edi,1234
    argdd @@endp1
  jb @@lp
@@done:
  ret

@@low:
    mov word ptr [edi],1234
      argdw @@minv
    add edi,2
    add esi,4
  cmp edi,1234
    argdd @@endp2
  jb @@lp
  jmp @@done
@@high:
    mov word ptr [edi],1234
      argdw @@maxv
    add edi,2
    add esi,4
  cmp edi,1234
    argdd @@endp3
  jb @@lp
  jmp @@done
endp


;//*************************************************************************

public mixAddAbs_ ;// old routine, does not handle loops correctly!!!
mixAddAbs_ proc ;//eax=chan, edi=len
  test [eax].chstatus,MCP_PLAY16BIT
  jnz @@16bit
  test [eax].chstatus,MCP_PLAY32BIT
  jnz @@32bit

  mov edx,[eax].chreplen
  mov esi,[eax].chpos
  add esi,[eax].chsamp
  mov ebx,[eax].chlength
  add ebx,[eax].chsamp
  add edi,esi
  xor ecx,ecx
@@bigloop:
    push edi
    cmp edi,ebx
    ja @@less
      xor edx,edx
    jmp @@loop
  @@less:
      mov edi,ebx
    jmp @@loop

  @@neg:
      sub ecx,eax
    cmp esi,edi
    jae @@loopend
  @@loop:
      movsx eax,byte ptr [esi]
      inc esi
      xor eax,0FFFFFF80h
      js @@neg
      add ecx,eax
    cmp esi,edi
    jb @@loop
  @@loopend:
    pop edi
    cmp edx,0
    je @@exit
    sub edi,edx
    sub esi,edx
  jmp @@bigloop

@@16bit:
  mov edx,[eax].chreplen
  mov esi,[eax].chpos
  add esi,[eax].chsamp
  mov ebx,[eax].chlength
  add ebx,[eax].chsamp
  add edi,esi
  xor ecx,ecx
@@16bigloop:
    push edi
    cmp edi,ebx
    ja @@16less
      xor edx,edx
    jmp @@16loop
  @@16less:
      mov edi,ebx
    jmp @@16loop

  @@16neg:
      sub ecx,eax
    cmp esi,edi
    jae @@16loopend
  @@16loop:
      movsx eax,byte ptr [esi+esi+1]
      inc esi
      xor eax,0FFFFFF80h
      js @@16neg
      add ecx,eax
    cmp esi,edi
    jb @@16loop
  @@16loopend:
    pop edi
    cmp edx,0
    je @@exit
    sub edi,edx
    sub esi,edx
  jmp @@16bigloop

@@32bit:
  mov edx,[eax].chreplen
  mov esi,[eax].chpos
  add esi,[eax].chsamp
  mov ebx,[eax].chlength
  add ebx,[eax].chsamp
  add edi,esi
  xor ecx,ecx
@@32bigloop:
    push edi
    cmp edi,ebx
    ja @@32less
      xor edx,edx
    jmp @@32loop
  @@32less:
      mov edi,ebx
    jmp @@32loop

  @@32neg:
      sub ecx,eax
    cmp esi,edi
    jae @@32loopend
  @@32loop:
      fld dword ptr [esi*4]
      fistp [integer]
      mov eax, [integer]
      inc esi
      xor eax,0FFFFFF80h
      js @@32neg
      add ecx,eax
    cmp esi,edi
    jb @@32loop
  @@32loopend:
    pop edi
    cmp edx,0
    je @@exit
    sub edi,edx
    sub esi,edx
  jmp @@32bigloop

@@exit:
  ret
endp

.data
integer dd ?
scale   dd ?
gscale  dd 64.0
minuseins dd -1.9
end
