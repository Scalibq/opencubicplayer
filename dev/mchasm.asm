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

.code

public mixAddAbs16M_
mixAddAbs16M_ proc ;// esi=buf, edi=len
  xor ecx,ecx
  shl edi,1
  add edi,esi
  jmp @@loop

  @@neg:
      sub ecx,eax
    cmp esi,edi
    jae @@loopend
  @@loop:
      movsx eax,word ptr [esi]
      add esi,2
      xor eax,0FFFF8000h
      js @@neg
      add ecx,eax
    cmp esi,edi
    jb @@loop
  @@loopend:

@@exit:
  ret
endp

public mixAddAbs16MS_
mixAddAbs16MS_ proc ;// esi=buf, edi=len
  xor ecx,ecx
  shl edi,1
  add edi,esi
  jmp @@loop

  @@neg:
      sub ecx,eax
    cmp esi,edi
    jae @@loopend
  @@loop:
      movsx eax,word ptr [esi]
      add esi,2
      test eax,080000000h
      js @@neg
      add ecx,eax
    cmp esi,edi
    jb @@loop
  @@loopend:

@@exit:
  ret
endp

public mixAddAbs16S_
mixAddAbs16S_ proc ;// esi=buf, edi=len
  xor ecx,ecx
  shl edi,2
  add edi,esi
  jmp @@loop

  @@neg:
      sub ecx,eax
    cmp esi,edi
    jae @@loopend
  @@loop:
      movsx eax,word ptr [esi]
      add esi,4
      xor eax,0FFFF8000h
      js @@neg
      add ecx,eax
    cmp esi,edi
    jb @@loop
  @@loopend:

@@exit:
  ret
endp

public mixAddAbs16SS_
mixAddAbs16SS_ proc ;// esi=buf, edi=len
  xor ecx,ecx
  shl edi,2
  add edi,esi
  jmp @@loop

  @@neg:
      sub ecx,eax
    cmp esi,edi
    jae @@loopend
  @@loop:
      movsx eax,word ptr [esi]
      add esi,4
      test eax,080000000h
      js @@neg
      add ecx,eax
    cmp esi,edi
    jb @@loop
  @@loopend:

@@exit:
  ret
endp

public mixAddAbs8M_
mixAddAbs8M_ proc ;// esi=buf, edi=len
  xor ecx,ecx
  add edi,esi
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

@@exit:
  shl ecx,8
  ret
endp

public mixAddAbs8MS_
mixAddAbs8MS_ proc ;// esi=buf, edi=len
  xor ecx,ecx
  add edi,esi
  jmp @@loop

  @@neg:
      sub ecx,eax
    cmp esi,edi
    jae @@loopend
  @@loop:
      movsx eax,byte ptr [esi]
      inc esi
      test eax,080000000h
      js @@neg
      add ecx,eax
    cmp esi,edi
    jb @@loop
  @@loopend:

@@exit:
  shl ecx,8
  ret
endp

public mixAddAbs8S_
mixAddAbs8S_ proc ;// esi=buf, edi=len
  xor ecx,ecx
  shl edi,1
  add edi,esi
  jmp @@loop

  @@neg:
      sub ecx,eax
    cmp esi,edi
    jae @@loopend
  @@loop:
      movsx eax,byte ptr [esi]
      add esi,2
      xor eax,0FFFFFF80h
      js @@neg
      add ecx,eax
    cmp esi,edi
    jb @@loop
  @@loopend:

@@exit:
  shl ecx,8
  ret
endp

public mixAddAbs8SS_
mixAddAbs8SS_ proc ;// esi=buf, edi=len
  xor ecx,ecx
  shl edi,1
  add edi,esi
  jmp @@loop

  @@neg:
      sub ecx,eax
    cmp esi,edi
    jae @@loopend
  @@loop:
      movsx eax,byte ptr [esi]
      add esi,2
      test eax,080000000h
      js @@neg
      add ecx,eax
    cmp esi,edi
    jb @@loop
  @@loopend:

@@exit:
  shl ecx,8
  ret
endp

;//********************************************************************

public mixGetMasterSampleMS8M_
mixGetMasterSampleMS8M_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done
  push ebp
  mov ebp,edx
  xor ebx,ebx
  shl ebp,16
  shr edx,16
  xor al,al

@@lp:
    mov ah,byte ptr [esi]
    add ebx,ebp
    mov word ptr [edi],ax
    adc esi,edx
    add edi,2
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleMU8M_
mixGetMasterSampleMU8M_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done
  push ebp
  mov ebp,edx
  xor ebx,ebx
  shl ebp,16
  shr edx,16
  xor al,al

@@lp:
    add ebx,ebp
    mov ah,byte ptr [esi]
    adc esi,edx
    xor ah,80h
    mov word ptr [edi],ax
    add edi,2
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleMS8S_
mixGetMasterSampleMS8S_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor ebx,ebx
  xor al,al
  shl ebp,16
  shr edx,16

@@lp:
    mov ah,byte ptr [esi]
    add ebx,ebp
    adc esi,edx
    mov word ptr [edi],ax
    mov word ptr [edi+2],ax
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleMU8S_
mixGetMasterSampleMU8S_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor ebx,ebx
  xor al,al
  shl ebp,16
  shr edx,16

@@lp:
    add ebx,ebp
    mov ah,byte ptr [esi]
    adc esi,edx
    xor ah,80h
    mov word ptr [edi],ax
    mov word ptr [edi+2],ax
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSS8M_
mixGetMasterSampleSS8M_ proc;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  shl ebp,16
  shr edx,16
  shr esi,1

@@lp:
    xor eax,eax
    xor ebx,ebx
    mov ah,byte ptr [esi+esi]
    add ah,byte ptr [esi+esi+1]
    clc
    jnl @@o
      stc
  @@o:
    rcr ax,1
    add ebx,ebp
    adc esi,edx
    mov word ptr [edi],ax
    add edi,2
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSU8M_
mixGetMasterSampleSU8M_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  shl ebp,16
  xor ebx,ebx
  shr edx,16
  shr esi,1

@@lp:
    xor eax,eax
    mov ah,byte ptr [esi+esi]
    add ah,byte ptr [esi+esi+1]
    rcr ax,1
    xor ah,80h
    add ebx,ebp
    mov word ptr [edi],ax
    adc esi,edx
    add edi,2
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSS8S_
mixGetMasterSampleSS8S_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor eax,eax
  xor ebx,ebx
  shr edx,16
  shl ebp,16
  shr esi,1

@@lp:
    mov ah,byte ptr [esi+esi+1]
    rol eax,16
    mov ah,byte ptr [esi+esi]
    add ebx,ebp
    mov dword ptr [edi],eax
    adc esi,edx
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSU8S_
mixGetMasterSampleSU8S_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor eax,eax
  xor ebx,ebx
  shr edx,16
  shl ebp,16
  shr esi,1

@@lp:
    mov ah,byte ptr [esi+esi+1]
    rol eax,16
    mov ah,byte ptr [esi+esi]
    xor eax,80008000h
    add ebx,ebp
    mov dword ptr [edi],eax
    adc esi,edx
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSS8SR_
mixGetMasterSampleSS8SR_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor eax,eax
  xor ebx,ebx
  shr edx,16
  shl ebp,16
  shr esi,1

@@lp:
    mov ah,byte ptr [esi+esi]
    rol eax,16
    mov ah,byte ptr [esi+esi+1]
    add ebx,ebp
    mov dword ptr [edi],eax
    adc esi,edx
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSU8SR_
mixGetMasterSampleSU8SR_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor eax,eax
  xor ebx,ebx
  shr edx,16
  shl ebp,16
  shr esi,1

@@lp:
    mov ah,byte ptr [esi+esi]
    rol eax,16
    mov ah,byte ptr [esi+esi+1]
    xor eax,80008000h
    add ebx,ebp
    mov dword ptr [edi],eax
    adc esi,edx
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleMS16M_
mixGetMasterSampleMS16M_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor ebx,ebx
  shr esi,1
  shl ebp,16
  shr edx,16
@@lp:
    mov eax,dword ptr [esi+esi]
    add ebx,ebp
    mov word ptr [edi],ax
    adc esi,edx
    add edi,2
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleMU16M_
mixGetMasterSampleMU16M_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor ebx,ebx
  shr esi,1
  shl ebp,16
  shr edx,16
@@lp:
    add ebx,ebp
    mov eax,dword ptr [esi+esi]
    adc esi,edx
    xor ah,80h
    mov word ptr [edi],ax
    add edi,2
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleMS16S_
mixGetMasterSampleMS16S_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor ebx,ebx
  shr esi,1
  shr ebp,16
  shr edx,16
@@lp:
    mov eax,dword ptr [esi+esi]
    add ebx,ebp
    mov word ptr [edi],ax
    adc esi,edx
    mov word ptr [edi+2],ax
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleMU16S_
mixGetMasterSampleMU16S_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor ebx,ebx
  shr esi,1
  shr ebp,16
  shr edx,16
@@lp:
    add ebx,ebp
    mov eax,dword ptr [esi+esi]
    adc esi,edx
    xor ah,80h
    mov word ptr [edi],ax
    mov word ptr [edi+2],ax
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSS16M_
mixGetMasterSampleSS16M_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  shl ebp,16
  xor ebx,ebx
  shr edx,16
  shr esi,2

@@lp:
    mov eax,dword ptr [4*esi-2]
    add eax,dword ptr [4*esi]
    clc
    jnl @@o
      stc
  @@o:
    rcr eax,17
    add ebx,ebp
    mov word ptr [edi],ax
    adc esi,edx
    add edi,2
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSU16M_
mixGetMasterSampleSU16M_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor ebx,ebx
  shl ebp,16
  shr edx,16
  shr esi,2
@@lp:
    mov eax,dword ptr [4*esi-2]
    add eax,dword ptr [4*esi]
    rcr eax,17
    add ebx,ebp
    adc esi,edx
    xor ah,80h
    mov word ptr [edi],ax
    add edi,2
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSS16S_
mixGetMasterSampleSS16S_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor ebx,ebx
  shr esi,2
  shl ebp,16
  shr edx,16
@@lp:
    mov eax,dword ptr [4*esi]
    add ebx,ebp
    mov dword ptr [edi],eax
    adc esi,edx
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSU16S_
mixGetMasterSampleSU16S_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor ebx,ebx
  shr esi,2
  shl ebp,16
  shr edx,16
@@lp:
    mov eax,dword ptr [4*esi]
    add ebx,ebp
    adc esi,edx
    xor eax,80008000h
    mov dword ptr [edi],eax
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSS16SR_
mixGetMasterSampleSS16SR_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor ebx,ebx
  shr esi,2
  shl ebp,16
  shr edx,16
@@lp:
    mov eax,dword ptr [4*esi]
    rol eax,16
    add ebx,ebp
    mov dword ptr [edi],eax
    adc esi,edx
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

public mixGetMasterSampleSU16SR_
mixGetMasterSampleSU16SR_ proc ;// edi=buf, esi=buf2, ecx=len, edx=step
  cmp ecx,0
  jz @@done

  push ebp
  mov ebp,edx
  xor ebx,ebx
  shr esi,2
  shl ebp,16
  shr edx,16
@@lp:
    mov eax,dword ptr [4*esi]
    add ebx,ebp
    adc esi,edx
    xor eax,80008000h
    rol eax,16
    mov dword ptr [edi],eax
    add edi,4
  dec ecx
  jnz @@lp
  pop ebp

@@done:
  ret
endp

end
