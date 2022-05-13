;// OpenCP Module Player
;// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//
;// assembler routines for FPU mixer
;//
;// revision history: (please note changes here)
;//  -kbwhenever Tammo Hinrichs <opencp@gmx.net>
;//    -first release
;//  -ryg990426  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
;//    -extreeeeem kbchangesapplying+sklavenarbeitverrichting
;//     (was mir angst macht, ich finds nichmal schlimm)
;//  -ryg990504  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
;//    -added float postprocs, the key to player realtimeruling
;//  -kb990531   Tammo Hinrichs <opengp@gmx.net>
;//    -fixed mono playback
;//    -cubic spline interpolation now works

; dominators und doc rooles geiler floating point mixer mit volume ramps
; (die man gar nicht benutzen kann (kb sagt man kann). und mit
; ultra-rauschabstand und viel geil interpolation.
; wir sind besser als ihr...



.486
.model flat, prolog
smart
jumps
ideal


MAXVOICES               = 256  

; Voice Flags
FLAG_LOOP_ENABLE        = 32
FLAG_ENABLED            = 0100h
FLAG_DISABLED           = 0fffffeffh


UDATASEG

public _tempbuf
public _outbuf
public _nsamples
public _nvoices
public _freqw
public _freqf
public _smpposw
public _smpposf
public _loopend
public _looplen
public _volleft
public _volright
public _rampleft
public _rampright
public _voiceflags
public _ffreq
public _freso
public _fadeleft
public _fl1
public _fb1
public _faderight
public _isstereo
public _outfmt
public _voll
public _volr
public _ct0
public _ct1
public _ct2
public _ct3
public _postprocs
public _samprate


                  ; ------------------------
                  ; Mixer control structures
                  ; ------------------------


_tempbuf    dd ?                      ; pointer to 32 bit mix buffer (nsamples * 4)
_outbuf     dd ?                      ; pointer to mixed buffer (nsamples * 2)
_nsamples   dd ?                      ; # of samples to mix
_nvoices    dd ?                      ; # of voices to mix

_isstereo   dd ?                      ; flag for stereo output
_outfmt     dd ?                      ; output format

_freqw      dd MAXVOICES dup (?)      ; frequency (whole part)
_freqf      dd MAXVOICES dup (?)      ; frequency (fractional part)

_smpposw    dd MAXVOICES dup (?)      ; sample position (whole part (pointer!))
_smpposf    dd MAXVOICES dup (?)      ; sample position (fractional part)

_loopend    dd MAXVOICES dup (?)      ; pointer to loop end
_looplen    dd MAXVOICES dup (?)      ; loop length in samples

_volleft    dd MAXVOICES dup (?)      ; float: left volume (1.0=normal)
_volright   dd MAXVOICES dup (?)      ; float: rite volume (1.0=normal)
_rampleft   dd MAXVOICES dup (?)      ; float: left volramp (dvol/sample)
_rampright  dd MAXVOICES dup (?)      ; float: rite volramp (dvol/sample)
_voiceflags dd MAXVOICES dup (?)      ; voice status flags

_ffreq      dd MAXVOICES dup (?)      ; filter frequency (0<=x<=1)
_freso      dd MAXVOICES dup (?)      ; filter resonance (0<=x<1)

_fl1        dd MAXVOICES dup (?)      ; filter lp buffer
_fb1        dd MAXVOICES dup (?)      ; filter bp buffer

_ct0        dd 256 dup (?)            ; interpolation tab for s[-1]
_ct1        dd 256 dup (?)            ; interpolation tab for s[0]
_ct2        dd 256 dup (?)            ; interpolation tab for s[1]
_ct3        dd 256 dup (?)            ; interpolation tab for s[2]

_postprocs  dd ?                      ; pointer to postproc list
_samprate   dd ?                      ; sampling rate

DATASEG

_fadeleft  dd 0
_faderight dd 0


eins           dd 1.0
minuseins      dd -1.0
clampmax       dd  32767.0
clampmin       dd -32767.0
cremoveconst   dd 0.992

minampl        dd 0.0001

UDATASEG

magic1     dd ?
_voll      dd ?
_volr      dd ?
_volrl     dd ?
_volrr     dd ?

clipval    dd ?

mixlooplen dd ?  ; lenght of loop in samples
looptype   dd ?

ffrq       dd ?
frez       dd ?
fl1        dd ?
fb1        dd ?

CODESEG

proc   prepare_mixer
public prepare_mixer
       pushad

       xor  eax,eax
       mov [dword ptr _fadeleft], eax
       mov [dword ptr _faderight], eax
       mov [dword ptr _volrl], eax
       mov [dword ptr _volrr], eax

       xor ecx, ecx
@@fillloop:

       inc ecx
       cmp ecx, MAXVOICES
       jne @@fillloop

       popad
       ret
endp   prepare_mixer



proc mixer
public mixer

     pushad
     finit

     ; range check for declick values
     xor ebx,ebx
     mov eax, [_fadeleft]
     and eax, 7fffffffh
     cmp eax, [minampl]
     ja  @@nocutfl
     mov [_fadeleft], ebx
     @@nocutfl:
     mov eax, [_faderight]
     and eax, 7fffffffh
     cmp eax, [minampl]
     ja  @@nocutfr
     mov [_faderight], ebx
     @@nocutfr:

     ; clear and declick buffer
     mov  edi, [_tempbuf]
     mov  ecx, [_nsamples]
     or   ecx, ecx
     jz   @@endall
     mov  eax, [_isstereo]
     or   eax, eax
     jnz  @@clearst
       call clearbufm
       jmp  @@clearend
     @@clearst:
     call clearbufs
     @@clearend:

     mov  ecx, [_nvoices]
     dec  ecx

     @@MixNext:

       mov  eax, [_voiceflags + ecx*4]
       test eax, FLAG_ENABLED
       jz   @@SkipVoice
  
       ; set loop type
       mov  [looptype], eax

       ; calc l/r relative vols from vol/panning/amplification
       mov eax, [dword ptr _volleft + ecx*4]
       mov ebx, [dword ptr _volright + ecx*4]
       mov [dword ptr _voll], eax
       mov [dword ptr _volr], ebx

       mov eax, [dword ptr _rampleft + ecx*4]
       mov ebx, [dword ptr _rampright + ecx*4]
       mov [dword ptr _volrl], eax
       mov [dword ptr _volrr], ebx

       ; set up filter vals
       mov eax, [dword ptr _ffreq + ecx*4]
       mov [dword ptr ffrq],eax
       mov eax, [dword ptr _freso + ecx*4]
       mov [dword ptr frez],eax
       mov eax, [dword ptr _fl1 + ecx*4]
       mov [dword ptr fl1],eax
       mov eax, [dword ptr _fb1 + ecx*4]
       mov [dword ptr fb1],eax

  
       ; length of loop
       mov  eax, [_looplen + ecx*4]
       mov  [dword ptr mixlooplen], eax
  
       ; sample delta:
       mov ebx, [_freqw + ecx*4]
       mov esi, [_freqf + ecx*4]

       ; Sample base Pointer
       mov eax, [_smpposw +ecx*4]
  
       ; sample base ptr fraction part
       mov edx, [_smpposf +ecx*4]

       ; Loop end Pointer
       mov ebp, [_loopend +ecx*4]

       push ecx
       mov  edi, [_tempbuf]
       mov  ecx, [_voiceflags + ecx*4]
       and  ecx, 15
       mov  ecx, [mixers + ecx*4]
       call ecx
       pop  ecx

       ; calculate sample relative position
       mov  [_smpposw + ecx*4], eax
       mov  [_smpposf + ecx*4], edx

       ; update flags
       mov eax, [looptype]
       mov [_voiceflags + ecx*4], eax

       ; update volumes
       mov eax, [dword ptr _voll]
       mov [dword ptr _volleft + ecx*4], eax
       mov eax, [dword ptr _volr]
       mov [dword ptr _volright + ecx*4], eax

       ; update filter buffers
       mov eax, [dword ptr fl1]
       mov [dword ptr _fl1 + ecx*4], eax
       mov eax, [dword ptr fb1]
       mov [dword ptr _fb1 + ecx*4], eax

       @@SkipVoice:
       dec  ecx
     jns  @@MixNext

; ryg990504 - changes for floatpostprocs start here

     mov  esi, [_postprocs]

     @@PostprocLoop:
       or   esi, esi
       jz   @@PostprocEnd

       mov  edx, [_nsamples]
       mov  ecx, [_isstereo]
       mov  ebx, [_samprate]
       mov  eax, [_tempbuf]
       call [dword ptr esi]

       mov  esi, [esi+12]

     jmp  short @@PostprocLoop

   @@PostprocEnd:

; ryg990504 - changes for floatpostprocs end here

     mov  eax, [_outfmt]
     mov  eax, [clippers+eax*4]

     mov  edi, [_outbuf]
     mov  esi, [_tempbuf]
     mov  ecx, [_nsamples]

     mov  edx, [_isstereo]
     or   edx, edx
     jz  @@clipmono
       add ecx,ecx
     @@clipmono:

     call eax

     @@endall:

     popad
     ret

endp mixer




; clear routines:
; esi : 32 bit float buffer
; ecx : # of samples


; clears and declicks tempbuffer (mono)
proc clearbufm


     fld   [dword ptr cremoveconst]      ; (fc)
     fld   [dword ptr _fadeleft]         ; (fl) (fc)

     @@clloop:
       fst    [dword ptr edi]
       fmul   st,st(1)                   ; (fl') (fc) 
       lea    edi,[edi+4]
       dec    ecx
     jnz    @@clloop

     fstp  [dword ptr _fadeleft]         ; (fc)
     fstp  st                            ; -

     ret

endp clearbufm





; clears and declicks tempbuffer (stereo)
proc clearbufs

     ; edi : 32 bit float buffer
     ; ecx : # of samples

     fld   [dword ptr cremoveconst]      ; (fc)
     fld   [dword ptr _faderight]        ; (fr) (fc)
     fld   [dword ptr _fadeleft]         ; (fl) (fr) (fc)

     @@clloop:
       fst    [dword ptr edi]
       fmul   st,st(2)            ; (fl') (fr) (fc) 
       fxch   st(1)               ; (fr) (fl') (fc)
       fst    [dword ptr edi+4]
       fmul   st,st(2)            ; (fr') (fl') (fc)
       fxch   st(1)               ; (fl') (fr') (fc)
       lea    edi,[edi+8]
       dec    ecx
     jnz    @@clloop

     fstp  [dword ptr _fadeleft]   ; (fr') (fc)
     fstp  [dword ptr _faderight]  ; (fc)
     fstp  st                      ; -

     ret

endp clearbufs




; mixing routines:
; eax = sample base ptr.
; edi = dest ptr auf outbuffer
; ecx = # of samples to mix
; ebx = delta to next sample (whole part)
; edx = fraction of sample position
; esi = fraction of sample delta
; ebp = ptr to loop end


proc mix_0    ; mixing, MUTED
     ; quite sub-obtimal to do this with a loop, too, but this is really
     ; the only way to ensure maximum precision - and it's fully using
     ; the vast potential of the coder's lazyness.
     mov  ecx, [_nsamples]
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp
     mov   ebp,       eax
     shr   ebp,       2
     @@next:
       add   edx, esi
       adc   ebp, ebx
@@SM1: cmp   ebp, 12345678h
       jge   @@LoopHandler
       dec   ecx
     jnz   @@next
     @@ende:
     shl   ebp,2
     mov   eax,ebp
     ret
     @@LoopHandler:
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     mov   eax, [looptype]
     and   eax, FLAG_DISABLED
     mov   [looptype], eax
     jmp   @@ende
     @@loopme:
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mix_0


proc mixm_n    ; mixing, mono w/o interpolation
     mov  ecx, [_nsamples]
     fld   [dword ptr _voll]               ; (vl)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     shr   ebp,       2
     align dword
     @@next:                                ; (vl) 
       fld   [dword ptr ebp*4]              ; (wert) (vl) 
       fld   st(1)                          ; (vl) (wert) (vl)
       add   edx, esi
       lea   edi, [edi+4]
       adc   ebp, ebx
       fmulp st(1), st                      ; (left) (vl)
       fxch  st(1)                          ; (vl) (left) 
       fadd  [dword ptr _volrl]             ; (vl') (left)
       fxch  st(1)                          ; (left) (vl) 
       fadd  [dword ptr edi - 4]            ; (lfinal) (vl')
@@SM1: cmp   ebp, 12345678h
       jge   @@LoopHandler
       fstp  [dword ptr edi -4]             ; (vl') (-1)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_voll]                          ; -
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl')
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl) 
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                        ; (vl) (wert) (vl) 
       fmul  st,st(1)                     ; (left) (wert) (vl)
       fadd  [dword ptr edi-4]            ; (wert) (vl)
       fstp  [dword ptr edi-4]            ; (wert) (vl)
       fxch  st(1)                        ; (vl) (wert)
       fadd  [_volrl]                     ; (vl') (wert)
       fxch  st(1)                        ; (wert) (vl')
       lea   edi,[edi+4]
       dec   ecx
     jnz   @@fill
     ; update click-removal fade values
     fmul  st,st(1)                       ; (left) (vl)
     fadd  [_fadeleft]                    ; (fl') (vl)
     fstp  [_fadeleft]                    ; (vl)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixm_n



proc mixs_n    ; mixing, stereo w/o interpolation
     mov  ecx, [_nsamples]
     fld   [dword ptr _voll]               ; (vl)
     fld   [dword ptr _volr]               ; (vr) (vl)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     shr   ebp,       2
     align dword
     @@next:                                ; (vr) (vl)
       fld   [dword ptr ebp*4]              ; (wert) (vr) (vl)
       add   edx, esi
       lea   edi, [edi+8]
       adc   ebp, ebx
       fld   st(1)                          ; (vr) (wert) (vr) (vl)
       fld   st(3)                          ; (vl) (vr) (wert) (vr) (vl)
       fmul  st, st(2)                      ; (left) (vr) (wert) (vr) (vl)
       fxch  st(4)                          ; (vl)  (vr) (wert) (vr) (left)
       fadd  [dword ptr _volrl]             ; (vl') (vr) (wert) (vr) (left)
       fxch  st(2)                          ; (wert) (vr) (vl') (vr) (left)
       fmulp st(1)                          ; (right) (vl') (vr) (left)
       fxch  st(2)                          ; (vr) (vl') (right) (left)
       fadd  [dword ptr _volrr]             ; (vr') (vl') (right) (left)
       fxch  st(3)                          ; (left)  (vl') (right) (vr')
       fadd  [dword ptr edi - 8]            ; (lfinal) (vl') <right> (vr')
       fxch  st(2)                          ; (right) (vl') (lfinal) (vr')
       fadd  [dword ptr edi - 4]            ; (rfinal) (vl') (lfinal) (vr')
@@SM1: cmp   ebp, 12345678h
       jge   @@LoopHandler
       ; hier 1 cycle frei
       fstp  [dword ptr edi -4]             ; (vl') (lfinal) (vr')
       fxch  st(1)                          ; (lfinal) (vl) (vr) 
       fstp  [dword ptr edi -8]             ; (vl) (vr) 
       fxch  st(1)                          ; (vr) (vl)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_volr]                          ; (vl)
     fstp  [_voll]                          ; -
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl') (lfinal) (vr')
     fxch  st(1)                          ; (lfinal) (vl) (vr)
     fstp  [dword ptr edi -8]             ; (vl) (vr)
     fxch  st(1)                          ; (vr) (vl)
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     fxch  st(1)                          ; (vl) (vr)
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl) (vr)
     fxch  st(2)                          ; (vr) (vl) (wert)
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                          ; (vl) (vr) (vl) (wert)
       fmul  st,st(3)                       ; (left) (vr) (vl) (wert)
       fxch  st(1)                          ; (vr) (left) (vl) (wert)
       fld   st                             ; (vr) (vr) (left) (vl) (wert)
       fmul  st,st(4)                       ; (right) (vr) (left) (vl) (wert)
       fxch  st(2)                          ; (left) (vr) (right) (vl) (wert)
       fadd  [dword ptr edi-8]          
       fstp  [dword ptr edi-8]              ; (vr) (right) (vl) (wert)
       fxch  st(1)                          ; (right) (vr) (vl) (wert)
       fadd  [dword ptr edi-4]          
       fstp  [dword ptr edi-4]              ; (vr) (vl) (wert)
       fadd  [_volrr]                       ; (vr') (vl) (wert)
       fxch  st(1)                          ; (vl) (vr') (wert)
       lea   edi,[edi+8]
       dec   ecx
       fadd  [_volrl]                       ; (vl') (vr') (wert)
       fxch  st(1)                          ; (vr') (vl') (wert)
     jnz   @@fill
     ; update click-removal fade values
     fxch  st(2)                          ; (wert) (vl) (vr)
     fld   st                             ; (wert) (wert) (vl) (vr)
     fmul  st,st(2)                       ; (left) (wert) (vl) (vr)
     fxch  st(1)                          ; (wert) (left) (vl) (vr)
     fmul  st,st(3)                       ; (rite) (left) (vl) (vr)
     fxch  st(1)                          ; (left) (rite) (vl) (vr)
     fadd  [_fadeleft]                    ; (fl') (rite) (vl) (vr)
     fxch  st(1)                          ; (rite) (fl') (vl) (vr)
     fadd  [_faderight]                   ; (fr') (fl') (vl) (vr)
     fxch  st(1)                          ; (fl') (fr') (vl) (vr)
     fstp  [_fadeleft]                    ; (fr') (vl) (vr)
     fstp  [_faderight]                   ; (vl) (vr)
     fxch  st(1)                          ; (vr) (vl)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixs_n




proc mixm_i    ; mixing, mono+interpolation
     mov  ecx, [_nsamples]
     fld   [minuseins]                     ; (-1)
     fld   [dword ptr _voll]               ; (vl) (-1)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     mov   eax,       edx
     shr   eax,       9
     shr   ebp,       2
     or    eax,       3f800000h
     mov   [magic1],  eax

     align dword
     @@next:                                ; (vl) (-1)
       fld   [dword ptr ebp*4+0]            ; (a) (vl) (-1)
       fld   st(0)                          ; (a) (a) (vl) (-1)
       fld   st(3)                          ; (-1) (a) (a) (vl) (-1)
       fadd  [magic1]                       ; (t) (a) (a) (vl) (-1)
       fxch  st(1)                          ; (a) (t) (a) (vl) (-1)
       fsubr [dword ptr ebp*4+4]            ; (b-a) (t) (a) (vl) (-1)
       add   edx, esi
       lea   edi, [edi+4]
       adc   ebp, ebx
       fmulp st(1)                          ; ((b-a)*t) (a) (vl) (-1)
       mov   eax, edx
       shr   eax, 9
       faddp st(1)                          ; (wert) (vl) (-1)
       fld   st(1)                          ; (vl) (wert) (vl) (-1)
       fmulp st(1), st                      ; (left) (vl) (-1)
       fxch  st(1)                          ; (vl) (left) (-1)
       fadd  [dword ptr _volrl]             ; (vl') (left) (-1)
       fxch  st(1)                          ; (left) (vl) (-1)
       fadd  [dword ptr edi - 4]            ; (lfinal) (vl') (-1)
       or    eax, 3f800000h
@@SM1: cmp   ebp, 12345678h
       mov   [magic1], eax
       jge   @@LoopHandler
       ; hier 1 cycle frei
       fstp  [dword ptr edi -4]             ; (vl') (-1)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_voll]                          ; (whatever)
     fstp  st                               ; -
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl') (-1)
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl)  (-1)
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                        ; (vl) (wert) (vl)  (-1)
       fmul  st,st(1)                     ; (left) (wert) (vl) (-1)
       fadd  [dword ptr edi-4]
       fstp  [dword ptr edi-4]            ; (wert) (vl) (-1)
       fxch  st(1)                        ; (vl) (wert) (-1)
       fadd  [_volrl]                     ; (vl') (wert) (-1)
       fxch  st(1)                        ; (wert) (vl') (-1)
       lea   edi,[edi+4]
       dec   ecx
     jnz   @@fill
     ; update click-removal fade values
     fmul  st,st(1)                       ; (left) (vl) (-1)
     fadd  [_fadeleft]                    ; (fl') (vl) (-1)
     fstp  [_fadeleft]                    ; (vl) (-1)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixm_i




proc mixs_i    ; mixing, stereo+interpolation
     mov  ecx, [_nsamples]
     fld   [minuseins]                     ; (-1)
     fld   [dword ptr _voll]               ; (vl) (-1)
     fld   [dword ptr _volr]               ; (vr) (vl) (-1)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     mov   eax,       edx
     shr   eax,       9
     shr   ebp,       2
     or    eax,       3f800000h
     mov   [magic1],  eax

     align dword
     @@next:                                ; (vr) (vl) (-1)
       fld   [dword ptr ebp*4+0]            ; (a) (vr) (vl) (-1)
       fld   st(0)                          ; (a) (a) (vr) (vl) (-1)
       fld   st(4)                          ; (-1) (a) (a) (vr) (vl) (-1)
       fadd  [magic1]                       ; (t) (a) (a) (vr) (vl) (-1)
       fxch  st(1)                          ; (a) (t) (a) (vr) (vl) (-1)
       fsubr [dword ptr ebp*4+4]            ; (b-a) (t) (a) (vr) (vl) (-1)
       add   edx, esi
       lea   edi, [edi+8]
       adc   ebp, ebx
       fmulp st(1)                          ; ((b-a)*t) (a) (vr) (vl) (-1)
       mov   eax, edx
       shr   eax, 9
       faddp st(1)                          ; (wert) (vr) (vl) (-1)
       fld   st(1)                          ; (vr) (wert) (vr) (vl) (-1)
       fld   st(3)                          ; (vl) (vr) (wert) (vr) (vl) (-1)
       fmul  st, st(2)                      ; (left) (vr) (wert) (vr) (vl) (-1)
       fxch  st(4)                          ; (vl)  (vr) (wert) (vr) (left) (-1)
       fadd  [dword ptr _volrl]             ; (vl') (vr) (wert) (vr) (left) (-1)
       fxch  st(2)                          ; (wert) (vr) (vl') (vr) (left) (-1)
       fmulp st(1)                          ; (right) (vl') (vr) (left) (-1)
       fxch  st(2)                          ; (vr) (vl') (right) (left) (-1)
       fadd  [dword ptr _volrr]             ; (vr') (vl') (right) (left) (-1)
       fxch  st(3)                          ; (left)  (vl') (right) (vr') (-1)
       fadd  [dword ptr edi - 8]            ; (lfinal) (vl') <right> (vr') (-1)
       fxch  st(2)                          ; (right) (vl') (lfinal) (vr') (-1)
       fadd  [dword ptr edi - 4]            ; (rfinal) (vl') (lfinal) (vr') (-1)
       or    eax, 3f800000h
@@SM1: cmp   ebp, 12345678h
       mov   [magic1], eax
       jge   @@LoopHandler
       ; hier 1 cycle frei
       fstp  [dword ptr edi -4]             ; (vl') (lfinal) <vr'> (-1)
       fxch  st(1)                          ; (lfinal) (vl) (vr) (-1)
       fstp  [dword ptr edi -8]             ; (vl) (vr) (-1)
       fxch  st(1)                          ; (vr) (vl) (-1)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_volr]
     fstp  [_voll]
     fstp  st
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl') (lfinal) <vr'> (-1)
     fxch  st(1)                          ; (lfinal) (vl) (vr) (-1)
     fstp  [dword ptr edi -8]             ; (vl) (vr) (-1)
     fxch  st(1)                          ; (vr) (vl) (-1)
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     fxch  st(2)                          ; (-1) (vl) (vr)
     fstp  st                             ; (vl) (vr)
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl) (vr)
     fxch  st(2)                          ; (vr) (vl) (wert)
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                          ; (vl) (vr) (vl) (wert)
       fmul  st,st(3)                       ; (left) (vr) (vl) (wert)
       fxch  st(1)                          ; (vr) (left) (vl) (wert)
       fld   st                             ; (vr) (vr) (left) (vl) (wert)
       fmul  st,st(4)                       ; (right) (vr) (left) (vl) (wert)
       fxch  st(2)                          ; (left) (vr) (right) (vl) (wert)
       fadd  [dword ptr edi-8]              ; (vr) (vl) (wert)
       fstp  [dword ptr edi-8]              ; (vr) (right) (vl) (wert)
       fxch  st(1)                          ; (right) (vr) (vl) (wert)
       fadd  [dword ptr edi-4]              ; (vr) (vl) (wert)
       fstp  [dword ptr edi-4]              ; (vr) (vl) (wert)
       fadd  [_volrr]                       ; (vr') (vl) (wert)
       fxch  st(1)                          ; (vl) (vr') (wert)
       lea   edi,[edi+8]
       dec   ecx
       fadd  [_volrl]                       ; (vl') (vr') (wert)
       fxch  st(1)                          ; (vr') (vl') (wert)
     jnz   @@fill
     ; update click-removal fade values
     fld   st(2)                          ; (wert) (vr) (vl) (wert)
     fld   st                             ; (wert) (wert) (vr) (vl) (wert)
     fmul  st,st(3)                       ; (left) (wert) (vr) (vl) (wert)
     fxch  st(1)                          ; (wert) (left) (vr) (vl) (wert)
     fmul  st,st(2)                       ; (rite) (left) (vr) (vl) (wert)
     fxch  st(1)                          ; (left) (rite) (vr) (vl) (wert)
     fadd  [_fadeleft]                    ; (fl') (rite) (vr) (vl) (wert)
     fxch  st(1)                          ; (rite) (fl') (vr) (vl) (wert)
     fadd  [_faderight]                   ; (fr') (fl') (vr) (vl) (wert)
     fxch  st(1)                          ; (fl') (fr') (vr) (vl) (wert)
     fstp  [_fadeleft]                    ; (fr') (vr) (vl) (wert)
     fstp  [_faderight]                   ; (vr) (vl) (wert)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixs_i



proc mixm_i2    ; mixing, mono w/ cubic interpolation
     mov  ecx, [_nsamples]
     fld   [dword ptr _voll]               ; (vl)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     shr   ebp,       2
     mov   eax,edx
     shr   eax,24
     align dword
     @@next:                                ; (vl)
       fld   [dword ptr ebp*4]              ; (w0) (vl)
       fmul  [dword ptr _ct0+eax*4]         ; (w0') (vl)
       fld   [dword ptr ebp*4+4]            ; (w1) (w0') (vl)
       fmul  [dword ptr _ct1+eax*4]         ; (w1') (w0') (vl)
       fld   [dword ptr ebp*4+8]            ; (w2) (w1') (w0') (vl)
       fmul  [dword ptr _ct2+eax*4]         ; (w2') (w1') (w0') (vl)
       fld   [dword ptr ebp*4+12]           ; (w3) (w2') (w1') (w0') (vl)
       fmul  [dword ptr _ct3+eax*4]         ; (w3') (w2') (w1') (w0') (vl)
       fxch  st(2)                          ; (w1') (w2') (w3') (w0') (vl)
       faddp st(3),st                       ; (w2') (w3') (w0+w1) (vl)
       add   edx, esi
       lea   edi, [edi+4]
       faddp st(2),st                       ; (w2+w3) (w0+w1) (vl)
       adc   ebp, ebx
       mov   eax, edx
       faddp st(1),st                       ; (wert) (vl)
       shr   eax,24
       fld   st(1)                          ; (vl) (wert) (vl)
       fmulp st(1), st                      ; (left) (vl)
       fxch  st(1)                          ; (vl) (left) 
       fadd  [dword ptr _volrl]             ; (vl') (left)
       fxch  st(1)                          ; (left) (vl) 
       fadd  [dword ptr edi - 4]            ; (lfinal) (vl')
@@SM1: cmp   ebp, 12345678h
       jge   @@LoopHandler
       fstp  [dword ptr edi -4]             ; (vl') (-1)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_voll]                          ; -
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl')
     push  eax
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     pop   eax
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl) 
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                        ; (vl) (wert) (vl) 
       fmul  st,st(1)                     ; (left) (wert) (vl)
       fadd  [dword ptr edi-4]            ; (wert) (vl)
       fstp  [dword ptr edi-4]            ; (wert) (vl)
       fxch  st(1)                        ; (vl) (wert)
       fadd  [_volrl]                     ; (vl') (wert)
       fxch  st(1)                        ; (wert) (vl')
       lea   edi,[edi+4]
       dec   ecx
     jnz   @@fill
     ; update click-removal fade values
     fmul  st,st(1)                       ; (left) (vl)
     fadd  [_fadeleft]                    ; (fl') (vl)
     fstp  [_fadeleft]                    ; (vl)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     pop   eax
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixm_i2



proc mixs_i2    ; mixing, stereo w/ cubic interpolation
     mov  ecx, [_nsamples]
     fld   [dword ptr _voll]               ; (vl)
     fld   [dword ptr _volr]               ; (vr) (vl)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     shr   ebp,       2
     mov   eax, edx
     shr   eax, 24
     align dword
     @@next:                                ; (vr) (vl)
       fld   [dword ptr ebp*4]              ; (w0) (vr) (vl)
       fmul  [dword ptr _ct0+eax*4]         ; (w0') (vr) (vl)
       fld   [dword ptr ebp*4+4]            ; (w1) (w0') (vr) (vl)
       fmul  [dword ptr _ct1+eax*4]         ; (w1') (w0') (vr) (vl)
       fld   [dword ptr ebp*4+8]            ; (w2) (w1') (w0') (vr) (vl)
       fmul  [dword ptr _ct2+eax*4]         ; (w2') (w1') (w0') (vr) (vl)
       fld   [dword ptr ebp*4+12]           ; (w3) (w2') (w1') (w0') (vr) (vl)
       fmul  [dword ptr _ct3+eax*4]         ; (w3') (w2') (w1') (w0') (vr) (vl)
       fxch  st(2)                          ; (w1') (w2') (w3') (w0') (vr) (vl)
       faddp st(3),st                       ; (w2') (w3') (w0+w1) (vr) (vl)
       add   edx, esi
       lea   edi, [edi+8]
       faddp st(2),st                       ; (w2+w3) (w0+w1) (vr) (vl)
       adc   ebp, ebx
       mov   eax, edx
       faddp st(1),st                       ; (wert) (vr) (vl)
       shr   eax,24
       fld   st(1)                          ; (vr) (wert) (vr) (vl)
       fld   st(3)                          ; (vl) (vr) (wert) (vr) (vl)
       fmul  st, st(2)                      ; (left) (vr) (wert) (vr) (vl)
       fxch  st(4)                          ; (vl)  (vr) (wert) (vr) (left)
       fadd  [dword ptr _volrl]             ; (vl') (vr) (wert) (vr) (left)
       fxch  st(2)                          ; (wert) (vr) (vl') (vr) (left)
       fmulp st(1)                          ; (right) (vl') (vr) (left)
       fxch  st(2)                          ; (vr) (vl') (right) (left)
       fadd  [dword ptr _volrr]             ; (vr') (vl') (right) (left)
       fxch  st(3)                          ; (left)  (vl') (right) (vr')
       fadd  [dword ptr edi - 8]            ; (lfinal) (vl') <right> (vr')
       fxch  st(2)                          ; (right) (vl') (lfinal) (vr')
       fadd  [dword ptr edi - 4]            ; (rfinal) (vl') (lfinal) (vr')
@@SM1: cmp   ebp, 12345678h
       jge   @@LoopHandler
       ; hier 1 cycle frei
       fstp  [dword ptr edi -4]             ; (vl') (lfinal) (vr')
       fxch  st(1)                          ; (lfinal) (vl) (vr) 
       fstp  [dword ptr edi -8]             ; (vl) (vr) 
       fxch  st(1)                          ; (vr) (vl)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_volr]                          ; (vl)
     fstp  [_voll]                          ; -
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl') (lfinal) (vr')
     fxch  st(1)                          ; (lfinal) (vl) (vr)
     fstp  [dword ptr edi -8]             ; (vl) (vr)
     fxch  st(1)                          ; (vr) (vl)
     push  eax
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     pop   eax
     fxch  st(1)                          ; (vl) (vr)
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl) (vr)
     fxch  st(2)                          ; (vr) (vl) (wert)
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                          ; (vl) (vr) (vl) (wert)
       fmul  st,st(3)                       ; (left) (vr) (vl) (wert)
       fxch  st(1)                          ; (vr) (left) (vl) (wert)
       fld   st                             ; (vr) (vr) (left) (vl) (wert)
       fmul  st,st(4)                       ; (right) (vr) (left) (vl) (wert)
       fxch  st(2)                          ; (left) (vr) (right) (vl) (wert)
       fadd  [dword ptr edi-8]          
       fstp  [dword ptr edi-8]              ; (vr) (right) (vl) (wert)
       fxch  st(1)                          ; (right) (vr) (vl) (wert)
       fadd  [dword ptr edi-4]          
       fstp  [dword ptr edi-4]              ; (vr) (vl) (wert)
       fadd  [_volrr]                       ; (vr') (vl) (wert)
       fxch  st(1)                          ; (vl) (vr') (wert)
       lea   edi,[edi+8]
       dec   ecx
       fadd  [_volrl]                       ; (vl') (vr') (wert)
       fxch  st(1)                          ; (vr') (vl') (wert)
     jnz   @@fill
     ; update click-removal fade values
     fxch  st(2)                          ; (wert) (vl) (vr)
     fld   st                             ; (wert) (wert) (vl) (vr)
     fmul  st,st(2)                       ; (left) (wert) (vl) (vr)
     fxch  st(1)                          ; (wert) (left) (vl) (vr)
     fmul  st,st(3)                       ; (rite) (left) (vl) (vr)
     fxch  st(1)                          ; (left) (rite) (vl) (vr)
     fadd  [_fadeleft]                    ; (fl') (rite) (vl) (vr)
     fxch  st(1)                          ; (rite) (fl') (vl) (vr)
     fadd  [_faderight]                   ; (fr') (fl') (vl) (vr)
     fxch  st(1)                          ; (fl') (fr') (vl) (vr)
     fstp  [_fadeleft]                    ; (fr') (vl) (vr)
     fstp  [_faderight]                   ; (vl) (vr)
     fxch  st(1)                          ; (vr) (vl)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     pop   eax
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixs_i2


proc mixm_nf    ; mixing, mono w/o interpolation, FILTERED
     mov  ecx, [_nsamples]
     fld   [dword ptr _voll]               ; (vl)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     shr   ebp,       2
     align dword
     @@next:                                ; (vl) 
       fld   [dword ptr ebp*4]              ; (wert) (vl) 

       ;FILTER HIER:
       ;b=reso*b+freq*(in-l);
       ;l+=freq*b;
       fsub  [dword ptr fl1]                ; (in-l) ..
       fmul  [dword ptr ffrq]               ; (f*(in-l)) ..
       fld   [dword ptr fb1]                ; (b) (f*(in-l)) ..
       fmul  [dword ptr frez]               ; (r*b) (f*(in-l)) ..
       faddp st(1),st                       ; (b') ..
       fst   [dword ptr fb1]
       fmul  [dword ptr ffrq]               ; (f*b') ..
       fadd  [dword ptr fl1]                ; (l') ..
       fst   [dword ptr fl1]                ; (out) (vr) (vl)

       fld   st(1)                          ; (vl) (wert) (vl)
       add   edx, esi
       lea   edi, [edi+4]
       adc   ebp, ebx
       fmulp st(1), st                      ; (left) (vl)
       fxch  st(1)                          ; (vl) (left) 
       fadd  [dword ptr _volrl]             ; (vl') (left)
       fxch  st(1)                          ; (left) (vl) 
       fadd  [dword ptr edi - 4]            ; (lfinal) (vl')
@@SM1: cmp   ebp, 12345678h
       jge   @@LoopHandler
       fstp  [dword ptr edi -4]             ; (vl') (-1)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_voll]                          ; -
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl')
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl) 
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                        ; (vl) (wert) (vl) 
       fmul  st,st(1)                     ; (left) (wert) (vl)
       fadd  [dword ptr edi-4]            ; (wert) (vl)
       fstp  [dword ptr edi-4]            ; (wert) (vl)
       fxch  st(1)                        ; (vl) (wert)
       fadd  [_volrl]                     ; (vl') (wert)
       fxch  st(1)                        ; (wert) (vl')
       lea   edi,[edi+4]
       dec   ecx
     jnz   @@fill
     ; update click-removal fade values
     fmul  st,st(1)                       ; (left) (vl)
     fadd  [_fadeleft]                    ; (fl') (vl)
     fstp  [_fadeleft]                    ; (vl)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixm_nf



proc mixs_nf    ; mixing, stereo w/o interpolation, FILTERED
     mov  ecx, [_nsamples]
     fld   [dword ptr _voll]               ; (vl)
     fld   [dword ptr _volr]               ; (vr) (vl)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     shr   ebp,       2
     align dword
     @@next:                                ; (vr) (vl)
       fld   [dword ptr ebp*4]              ; (wert) (vr) (vl)

       ;FILTER HIER:
       ;b=reso*b+freq*(in-l);
       ;l+=freq*b;
       fsub  [dword ptr fl1]                ; (in-l) ..
       fmul  [dword ptr ffrq]               ; (f*(in-l)) ..
       fld   [dword ptr fb1]                ; (b) (f*(in-l)) ..
       fmul  [dword ptr frez]               ; (r*b) (f*(in-l)) ..
       faddp st(1),st                       ; (b') ..
       fst   [dword ptr fb1]
       fmul  [dword ptr ffrq]               ; (f*b') ..
       fadd  [dword ptr fl1]                ; (l') ..
       fst   [dword ptr fl1]                ; (out) (vr) (vl)

       add   edx, esi
       lea   edi, [edi+8]
       adc   ebp, ebx
       fld   st(1)                          ; (vr) (wert) (vr) (vl)
       fld   st(3)                          ; (vl) (vr) (wert) (vr) (vl)
       fmul  st, st(2)                      ; (left) (vr) (wert) (vr) (vl)
       fxch  st(4)                          ; (vl)  (vr) (wert) (vr) (left)
       fadd  [dword ptr _volrl]             ; (vl') (vr) (wert) (vr) (left)
       fxch  st(2)                          ; (wert) (vr) (vl') (vr) (left)
       fmulp st(1)                          ; (right) (vl') (vr) (left)
       fxch  st(2)                          ; (vr) (vl') (right) (left)
       fadd  [dword ptr _volrr]             ; (vr') (vl') (right) (left)
       fxch  st(3)                          ; (left)  (vl') (right) (vr')
       fadd  [dword ptr edi - 8]            ; (lfinal) (vl') <right> (vr')
       fxch  st(2)                          ; (right) (vl') (lfinal) (vr')
       fadd  [dword ptr edi - 4]            ; (rfinal) (vl') (lfinal) (vr')
@@SM1: cmp   ebp, 12345678h
       jge   @@LoopHandler
       ; hier 1 cycle frei
       fstp  [dword ptr edi -4]             ; (vl') (lfinal) (vr')
       fxch  st(1)                          ; (lfinal) (vl) (vr) 
       fstp  [dword ptr edi -8]             ; (vl) (vr) 
       fxch  st(1)                          ; (vr) (vl)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_volr]                          ; (vl)
     fstp  [_voll]                          ; -
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl') (lfinal) (vr')
     fxch  st(1)                          ; (lfinal) (vl) (vr)
     fstp  [dword ptr edi -8]             ; (vl) (vr)
     fxch  st(1)                          ; (vr) (vl)
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     fxch  st(1)                          ; (vl) (vr)
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl) (vr)
     fxch  st(2)                          ; (vr) (vl) (wert)
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                          ; (vl) (vr) (vl) (wert)
       fmul  st,st(3)                       ; (left) (vr) (vl) (wert)
       fxch  st(1)                          ; (vr) (left) (vl) (wert)
       fld   st                             ; (vr) (vr) (left) (vl) (wert)
       fmul  st,st(4)                       ; (right) (vr) (left) (vl) (wert)
       fxch  st(2)                          ; (left) (vr) (right) (vl) (wert)
       fadd  [dword ptr edi-8]          
       fstp  [dword ptr edi-8]              ; (vr) (right) (vl) (wert)
       fxch  st(1)                          ; (right) (vr) (vl) (wert)
       fadd  [dword ptr edi-4]          
       fstp  [dword ptr edi-4]              ; (vr) (vl) (wert)
       fadd  [_volrr]                       ; (vr') (vl) (wert)
       fxch  st(1)                          ; (vl) (vr') (wert)
       lea   edi,[edi+8]
       dec   ecx
       fadd  [_volrl]                       ; (vl') (vr') (wert)
       fxch  st(1)                          ; (vr') (vl') (wert)
     jnz   @@fill
     ; update click-removal fade values
     fxch  st(2)                          ; (wert) (vl) (vr)
     fld   st                             ; (wert) (wert) (vl) (vr)
     fmul  st,st(2)                       ; (left) (wert) (vl) (vr)
     fxch  st(1)                          ; (wert) (left) (vl) (vr)
     fmul  st,st(3)                       ; (rite) (left) (vl) (vr)
     fxch  st(1)                          ; (left) (rite) (vl) (vr)
     fadd  [_fadeleft]                    ; (fl') (rite) (vl) (vr)
     fxch  st(1)                          ; (rite) (fl') (vl) (vr)
     fadd  [_faderight]                   ; (fr') (fl') (vl) (vr)
     fxch  st(1)                          ; (fl') (fr') (vl) (vr)
     fstp  [_fadeleft]                    ; (fr') (vl) (vr)
     fstp  [_faderight]                   ; (vl) (vr)
     fxch  st(1)                          ; (vr) (vl)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixs_nf




proc mixm_if    ; mixing, mono+interpolation, FILTERED
     mov  ecx, [_nsamples]
     fld   [minuseins]                     ; (-1)
     fld   [dword ptr _voll]               ; (vl) (-1)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     mov   eax,       edx
     shr   eax,       9
     shr   ebp,       2
     or    eax,       3f800000h
     mov   [magic1],  eax

     align dword
     @@next:                                ; (vl) (-1)
       fld   [dword ptr ebp*4+0]            ; (a) (vl) (-1)
       fld   st(0)                          ; (a) (a) (vl) (-1)
       fld   st(3)                          ; (-1) (a) (a) (vl) (-1)
       fadd  [magic1]                       ; (t) (a) (a) (vl) (-1)
       fxch  st(1)                          ; (a) (t) (a) (vl) (-1)
       fsubr [dword ptr ebp*4+4]            ; (b-a) (t) (a) (vl) (-1)
       add   edx, esi
       lea   edi, [edi+4]
       adc   ebp, ebx
       fmulp st(1)                          ; ((b-a)*t) (a) (vl) (-1)
       mov   eax, edx
       shr   eax, 9
       faddp st(1)                          ; (wert) (vl) (-1)

       ;FILTER HIER:
       ;b=reso*b+freq*(in-l);
       ;l+=freq*b;
       fsub  [dword ptr fl1]                ; (in-l) ..
       fmul  [dword ptr ffrq]               ; (f*(in-l)) ..
       fld   [dword ptr fb1]                ; (b) (f*(in-l)) ..
       fmul  [dword ptr frez]               ; (r*b) (f*(in-l)) ..
       faddp st(1),st                       ; (b') ..
       fst   [dword ptr fb1]
       fmul  [dword ptr ffrq]               ; (f*b') ..
       fadd  [dword ptr fl1]                ; (l') ..
       fst   [dword ptr fl1]                ; (out) (vr) (vl)

       fld   st(1)                          ; (vl) (wert) (vl) (-1)
       fmulp st(1), st                      ; (left) (vl) (-1)
       fxch  st(1)                          ; (vl) (left) (-1)
       fadd  [dword ptr _volrl]             ; (vl') (left) (-1)
       fxch  st(1)                          ; (left) (vl) (-1)
       fadd  [dword ptr edi - 4]            ; (lfinal) (vl') (-1)
       or    eax, 3f800000h
@@SM1: cmp   ebp, 12345678h
       mov   [magic1], eax
       jge   @@LoopHandler
       ; hier 1 cycle frei
       fstp  [dword ptr edi -4]             ; (vl') (-1)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_voll]                          ; (whatever)
     fstp  st                               ; -
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl') (-1)
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl)  (-1)
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                        ; (vl) (wert) (vl)  (-1)
       fmul  st,st(1)                     ; (left) (wert) (vl) (-1)
       fadd  [dword ptr edi-4]
       fstp  [dword ptr edi-4]            ; (wert) (vl) (-1)
       fxch  st(1)                        ; (vl) (wert) (-1)
       fadd  [_volrl]                     ; (vl') (wert) (-1)
       fxch  st(1)                        ; (wert) (vl') (-1)
       lea   edi,[edi+4]
       dec   ecx
     jnz   @@fill
     ; update click-removal fade values
     fmul  st,st(1)                       ; (left) (vl) (-1)
     fadd  [_fadeleft]                    ; (fl') (vl) (-1)
     fstp  [_fadeleft]                    ; (vl) (-1)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixm_if




proc mixs_if    ; mixing, stereo+interpolation, FILTERED
     mov  ecx, [_nsamples]
     fld   [minuseins]                     ; (-1)
     fld   [dword ptr _voll]               ; (vl) (-1)
     fld   [dword ptr _volr]               ; (vr) (vl) (-1)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     mov   eax,       edx
     shr   eax,       9
     shr   ebp,       2
     or    eax,       3f800000h
     mov   [magic1],  eax

     align dword
     @@next:                                ; (vr) (vl) (-1)
       fld   [dword ptr ebp*4+0]            ; (a) (vr) (vl) (-1)
       fld   st(0)                          ; (a) (a) (vr) (vl) (-1)
       fld   st(4)                          ; (-1) (a) (a) (vr) (vl) (-1)
       fadd  [magic1]                       ; (t) (a) (a) (vr) (vl) (-1)
       fxch  st(1)                          ; (a) (t) (a) (vr) (vl) (-1)
       fsubr [dword ptr ebp*4+4]            ; (b-a) (t) (a) (vr) (vl) (-1)
       add   edx, esi
       lea   edi, [edi+8]
       adc   ebp, ebx
       fmulp st(1)                          ; ((b-a)*t) (a) (vr) (vl) (-1)
       mov   eax, edx
       shr   eax, 9
       faddp st(1)                          ; (wert) (vr) (vl) (-1)

       ;FILTER HIER:
       ;b=reso*b+freq*(in-l);
       ;l+=freq*b;
       fsub  [dword ptr fl1]                ; (in-l) ..
       fmul  [dword ptr ffrq]               ; (f*(in-l)) ..
       fld   [dword ptr fb1]                ; (b) (f*(in-l)) ..
       fmul  [dword ptr frez]               ; (r*b) (f*(in-l)) ..
       faddp st(1),st                       ; (b') ..
       fst   [dword ptr fb1]
       fmul  [dword ptr ffrq]               ; (f*b') ..
       fadd  [dword ptr fl1]                ; (l') ..
       fst   [dword ptr fl1]                ; (out) (vr) (vl)

       fld   st(1)                          ; (vr) (wert) (vr) (vl) (-1)
       fld   st(3)                          ; (vl) (vr) (wert) (vr) (vl) (-1)
       fmul  st, st(2)                      ; (left) (vr) (wert) (vr) (vl) (-1)
       fxch  st(4)                          ; (vl)  (vr) (wert) (vr) (left) (-1)
       fadd  [dword ptr _volrl]             ; (vl') (vr) (wert) (vr) (left) (-1)
       fxch  st(2)                          ; (wert) (vr) (vl') (vr) (left) (-1)
       fmulp st(1)                          ; (right) (vl') (vr) (left) (-1)
       fxch  st(2)                          ; (vr) (vl') (right) (left) (-1)
       fadd  [dword ptr _volrr]             ; (vr') (vl') (right) (left) (-1)
       fxch  st(3)                          ; (left)  (vl') (right) (vr') (-1)
       fadd  [dword ptr edi - 8]            ; (lfinal) (vl') <right> (vr') (-1)
       fxch  st(2)                          ; (right) (vl') (lfinal) (vr') (-1)
       fadd  [dword ptr edi - 4]            ; (rfinal) (vl') (lfinal) (vr') (-1)
       or    eax, 3f800000h
@@SM1: cmp   ebp, 12345678h
       mov   [magic1], eax
       jge   @@LoopHandler
       ; hier 1 cycle frei
       fstp  [dword ptr edi -4]             ; (vl') (lfinal) <vr'> (-1)
       fxch  st(1)                          ; (lfinal) (vl) (vr) (-1)
       fstp  [dword ptr edi -8]             ; (vl) (vr) (-1)
       fxch  st(1)                          ; (vr) (vl) (-1)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_volr]
     fstp  [_voll]
     fstp  st
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl') (lfinal) <vr'> (-1)
     fxch  st(1)                          ; (lfinal) (vl) (vr) (-1)
     fstp  [dword ptr edi -8]             ; (vl) (vr) (-1)
     fxch  st(1)                          ; (vr) (vl) (-1)
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     fxch  st(2)                          ; (-1) (vl) (vr)
     fstp  st                             ; (vl) (vr)
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl) (vr)
     fxch  st(2)                          ; (vr) (vl) (wert)
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                          ; (vl) (vr) (vl) (wert)
       fmul  st,st(3)                       ; (left) (vr) (vl) (wert)
       fxch  st(1)                          ; (vr) (left) (vl) (wert)
       fld   st                             ; (vr) (vr) (left) (vl) (wert)
       fmul  st,st(4)                       ; (right) (vr) (left) (vl) (wert)
       fxch  st(2)                          ; (left) (vr) (right) (vl) (wert)
       fadd  [dword ptr edi-8]              ; (vr) (vl) (wert)
       fstp  [dword ptr edi-8]              ; (vr) (right) (vl) (wert)
       fxch  st(1)                          ; (right) (vr) (vl) (wert)
       fadd  [dword ptr edi-4]              ; (vr) (vl) (wert)
       fstp  [dword ptr edi-4]              ; (vr) (vl) (wert)
       fadd  [_volrr]                       ; (vr') (vl) (wert)
       fxch  st(1)                          ; (vl) (vr') (wert)
       lea   edi,[edi+8]
       dec   ecx
       fadd  [_volrl]                       ; (vl') (vr') (wert)
       fxch  st(1)                          ; (vr') (vl') (wert)
     jnz   @@fill
     ; update click-removal fade values
     fld   st(2)                          ; (wert) (vr) (vl) (wert)
     fld   st                             ; (wert) (wert) (vr) (vl) (wert)
     fmul  st,st(3)                       ; (left) (wert) (vr) (vl) (wert)
     fxch  st(1)                          ; (wert) (left) (vr) (vl) (wert)
     fmul  st,st(2)                       ; (rite) (left) (vr) (vl) (wert)
     fxch  st(1)                          ; (left) (rite) (vr) (vl) (wert)
     fadd  [_fadeleft]                    ; (fl') (rite) (vr) (vl) (wert)
     fxch  st(1)                          ; (rite) (fl') (vr) (vl) (wert)
     fadd  [_faderight]                   ; (fr') (fl') (vr) (vl) (wert)
     fxch  st(1)                          ; (fl') (fr') (vr) (vl) (wert)
     fstp  [_fadeleft]                    ; (fr') (vr) (vl) (wert)
     fstp  [_faderight]                   ; (vr) (vl) (wert)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixs_if



proc mixm_i2f    ; mixing, mono w/ cubic interpolation, FILTERED
     mov  ecx, [_nsamples]
     fld   [dword ptr _voll]               ; (vl)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     shr   ebp,       2
     mov   eax,edx
     shr   eax,24
     align dword
     @@next:                                ; (vl)
       fld   [dword ptr ebp*4]              ; (w0) (vl)
       fmul  [dword ptr _ct0+eax*4]         ; (w0') (vl)
       fld   [dword ptr ebp*4+4]            ; (w1) (w0') (vl)
       fmul  [dword ptr _ct1+eax*4]         ; (w1') (w0') (vl)
       fld   [dword ptr ebp*4+8]            ; (w2) (w1') (w0') (vl)
       fmul  [dword ptr _ct2+eax*4]         ; (w2') (w1') (w0') (vl)
       fld   [dword ptr ebp*4+12]           ; (w3) (w2') (w1') (w0') (vl)
       fmul  [dword ptr _ct3+eax*4]         ; (w3') (w2') (w1') (w0') (vl)
       fxch  st(2)                          ; (w1') (w2') (w3') (w0') (vl)
       faddp st(3),st                       ; (w2') (w3') (w0+w1) (vl)
       add   edx, esi
       lea   edi, [edi+4]
       faddp st(2),st                       ; (w2+w3) (w0+w1) (vl)
       adc   ebp, ebx
       mov   eax, edx
       faddp st(1),st                       ; (wert) (vl)


       ;FILTER HIER:
       ;b=reso*b+freq*(in-l);
       ;l+=freq*b;
       fsub  [dword ptr fl1]                ; (in-l) ..
       fmul  [dword ptr ffrq]               ; (f*(in-l)) ..
       fld   [dword ptr fb1]                ; (b) (f*(in-l)) ..
       fmul  [dword ptr frez]               ; (r*b) (f*(in-l)) ..
       faddp st(1),st                       ; (b') ..
       fst   [dword ptr fb1]
       fmul  [dword ptr ffrq]               ; (f*b') ..
       fadd  [dword ptr fl1]                ; (l') ..
       fst   [dword ptr fl1]                ; (out) (vr) (vl)


       shr   eax,24
       fld   st(1)                          ; (vl) (wert) (vl)
       fmulp st(1), st                      ; (left) (vl)
       fxch  st(1)                          ; (vl) (left) 
       fadd  [dword ptr _volrl]             ; (vl') (left)
       fxch  st(1)                          ; (left) (vl) 
       fadd  [dword ptr edi - 4]            ; (lfinal) (vl')
@@SM1: cmp   ebp, 12345678h
       jge   @@LoopHandler
       fstp  [dword ptr edi -4]             ; (vl') (-1)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_voll]                          ; -
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl')
     push  eax
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     pop   eax
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl) 
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                        ; (vl) (wert) (vl) 
       fmul  st,st(1)                     ; (left) (wert) (vl)
       fadd  [dword ptr edi-4]            ; (wert) (vl)
       fstp  [dword ptr edi-4]            ; (wert) (vl)
       fxch  st(1)                        ; (vl) (wert)
       fadd  [_volrl]                     ; (vl') (wert)
       fxch  st(1)                        ; (wert) (vl')
       lea   edi,[edi+4]
       dec   ecx
     jnz   @@fill
     ; update click-removal fade values
     fmul  st,st(1)                       ; (left) (vl)
     fadd  [_fadeleft]                    ; (fl') (vl)
     fstp  [_fadeleft]                    ; (vl)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     pop   eax
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixm_i2f



proc mixs_i2f    ; mixing, stereo w/ cubic interpolation, FILTERED
     mov  ecx, [_nsamples]
     fld   [dword ptr _voll]               ; (vl)
     fld   [dword ptr _volr]               ; (vr) (vl)
     shr   ebp, 2
     mov   [dword ptr @@SM1+2], ebp        ; set loop end position
     mov   ebp,       eax
     shr   ebp,       2
     mov   eax, edx
     shr   eax, 24
     align dword
     @@next:                                ; (vr) (vl)
       fld   [dword ptr ebp*4]              ; (w0) (vr) (vl)
       fmul  [dword ptr _ct0+eax*4]         ; (w0') (vr) (vl)
       fld   [dword ptr ebp*4+4]            ; (w1) (w0') (vr) (vl)
       fmul  [dword ptr _ct1+eax*4]         ; (w1') (w0') (vr) (vl)
       fld   [dword ptr ebp*4+8]            ; (w2) (w1') (w0') (vr) (vl)
       fmul  [dword ptr _ct2+eax*4]         ; (w2') (w1') (w0') (vr) (vl)
       fld   [dword ptr ebp*4+12]           ; (w3) (w2') (w1') (w0') (vr) (vl)
       fmul  [dword ptr _ct3+eax*4]         ; (w3') (w2') (w1') (w0') (vr) (vl)
       fxch  st(2)                          ; (w1') (w2') (w3') (w0') (vr) (vl)
       faddp st(3),st                       ; (w2') (w3') (w0+w1) (vr) (vl)
       add   edx, esi
       lea   edi, [edi+8]
       faddp st(2),st                       ; (w2+w3) (w0+w1) (vr) (vl)
       adc   ebp, ebx
       mov   eax, edx
       faddp st(1),st                       ; (wert) (vr) (vl)

       ;FILTER HIER:
       ;b=reso*b+freq*(in-l);
       ;l+=freq*b;
       fsub  [dword ptr fl1]                ; (in-l) ..
       fmul  [dword ptr ffrq]               ; (f*(in-l)) ..
       fld   [dword ptr fb1]                ; (b) (f*(in-l)) ..
       fmul  [dword ptr frez]               ; (r*b) (f*(in-l)) ..
       faddp st(1),st                       ; (b') ..
       fst   [dword ptr fb1]
       fmul  [dword ptr ffrq]               ; (f*b') ..
       fadd  [dword ptr fl1]                ; (l') ..
       fst   [dword ptr fl1]                ; (out) (vr) (vl)

       shr   eax,24
       fld   st(1)                          ; (vr) (wert) (vr) (vl)
       fld   st(3)                          ; (vl) (vr) (wert) (vr) (vl)
       fmul  st, st(2)                      ; (left) (vr) (wert) (vr) (vl)
       fxch  st(4)                          ; (vl)  (vr) (wert) (vr) (left)
       fadd  [dword ptr _volrl]             ; (vl') (vr) (wert) (vr) (left)
       fxch  st(2)                          ; (wert) (vr) (vl') (vr) (left)
       fmulp st(1)                          ; (right) (vl') (vr) (left)
       fxch  st(2)                          ; (vr) (vl') (right) (left)
       fadd  [dword ptr _volrr]             ; (vr') (vl') (right) (left)
       fxch  st(3)                          ; (left)  (vl') (right) (vr')
       fadd  [dword ptr edi - 8]            ; (lfinal) (vl') <right> (vr')
       fxch  st(2)                          ; (right) (vl') (lfinal) (vr')
       fadd  [dword ptr edi - 4]            ; (rfinal) (vl') (lfinal) (vr')
@@SM1: cmp   ebp, 12345678h
       jge   @@LoopHandler
       ; hier 1 cycle frei
       fstp  [dword ptr edi -4]             ; (vl') (lfinal) (vr')
       fxch  st(1)                          ; (lfinal) (vl) (vr) 
       fstp  [dword ptr edi -8]             ; (vl) (vr) 
       fxch  st(1)                          ; (vr) (vl)
       dec   ecx
     jnz   @@next
     @@ende:
     fstp  [_volr]                          ; (vl)
     fstp  [_voll]                          ; -
     shl   ebp,2
     mov   eax,ebp
     ret

     @@LoopHandler:
     fstp  [dword ptr edi -4]             ; (vl') (lfinal) (vr')
     fxch  st(1)                          ; (lfinal) (vl) (vr)
     fstp  [dword ptr edi -8]             ; (vl) (vr)
     fxch  st(1)                          ; (vr) (vl)
     push  eax
     mov   eax,[looptype]
     test  eax, FLAG_LOOP_ENABLE
     jnz   @@loopme
     pop   eax
     fxch  st(1)                          ; (vl) (vr)
     sub   edx,esi
     sbb   ebp,ebx
     fld   [dword ptr ebp*4]              ; (wert) (vl) (vr)
     fxch  st(2)                          ; (vr) (vl) (wert)
     @@fill:  ; sample ends -> fill rest of buffer with last sample value
       fld   st(1)                          ; (vl) (vr) (vl) (wert)
       fmul  st,st(3)                       ; (left) (vr) (vl) (wert)
       fxch  st(1)                          ; (vr) (left) (vl) (wert)
       fld   st                             ; (vr) (vr) (left) (vl) (wert)
       fmul  st,st(4)                       ; (right) (vr) (left) (vl) (wert)
       fxch  st(2)                          ; (left) (vr) (right) (vl) (wert)
       fadd  [dword ptr edi-8]          
       fstp  [dword ptr edi-8]              ; (vr) (right) (vl) (wert)
       fxch  st(1)                          ; (right) (vr) (vl) (wert)
       fadd  [dword ptr edi-4]          
       fstp  [dword ptr edi-4]              ; (vr) (vl) (wert)
       fadd  [_volrr]                       ; (vr') (vl) (wert)
       fxch  st(1)                          ; (vl) (vr') (wert)
       lea   edi,[edi+8]
       dec   ecx
       fadd  [_volrl]                       ; (vl') (vr') (wert)
       fxch  st(1)                          ; (vr') (vl') (wert)
     jnz   @@fill
     ; update click-removal fade values
     fxch  st(2)                          ; (wert) (vl) (vr)
     fld   st                             ; (wert) (wert) (vl) (vr)
     fmul  st,st(2)                       ; (left) (wert) (vl) (vr)
     fxch  st(1)                          ; (wert) (left) (vl) (vr)
     fmul  st,st(3)                       ; (rite) (left) (vl) (vr)
     fxch  st(1)                          ; (left) (rite) (vl) (vr)
     fadd  [_fadeleft]                    ; (fl') (rite) (vl) (vr)
     fxch  st(1)                          ; (rite) (fl') (vl) (vr)
     fadd  [_faderight]                   ; (fr') (fl') (vl) (vr)
     fxch  st(1)                          ; (fl') (fr') (vl) (vr)
     fstp  [_fadeleft]                    ; (fr') (vl) (vr)
     fstp  [_faderight]                   ; (vl) (vr)
     fxch  st(1)                          ; (vr) (vl)
     mov  eax, [looptype]
     and  eax, FLAG_DISABLED
     mov  [looptype], eax
     jmp  @@ende

     @@loopme: ; sample loops -> jump to loop start
     pop   eax
     sub   ebp,[mixlooplen]
     dec   ecx
     jz    @@ende
     jmp   @@next
endp mixs_i2f




; clip routines:
; esi : 32 bit float input buffer
; edi : 8/16 bit output buffer
; ecx : # of samples to clamp (*2 if stereo!)

proc clip_16s     ; convert/clip samples, 16bit signed

     fld   [dword ptr clampmin]          ; (min)
     fld   [dword ptr clampmax]          ; (max) (min)
     mov   bx,  32766
     mov   dx, -32767

     @@lp:
       fld    [dword ptr esi]     ; (ls) (max) (min)
       fcom   st(1)
       fnstsw ax
       sahf
       ja     @@max
       fcom   st(2)
       fstsw  ax
       sahf
       jb     @@min
       fistp  [word ptr edi]      ; (max) (min)
       @@next:
       add    esi, 4
       add    edi, 2
       dec    ecx
     jnz    @@lp
     jmp    @@ende

     @@max:
     fstp   st                  ; (max) (min)
     mov    [word ptr edi], bx
     jmp    @@next
     @@min:
     fstp   st                  ; (max) (min) 
     mov    [word ptr edi], dx
     jmp    @@next

     @@ende:
     fstp  st                     ; (min)
     fstp  st                     ; -
     ret

endp clip_16s



proc clip_16u     ; convert/clip samples, 16bit unsigned

     fld   [dword ptr clampmin]          ; (min)
     fld   [dword ptr clampmax]          ; (max) (min)
     mov   bx,  32767
     mov   dx, -32767

     @@lp:
     fld    [dword ptr esi]     ; (ls) (max) (min)
     fcom   st(1)
     fnstsw ax
     sahf
     ja     @@max
     fcom   st(2)
     fstsw  ax
     sahf
     jb     @@min
     fistp  [word ptr clipval]      ; (max) (min)
     mov    ax,[word ptr clipval]
     @@next:
     xor    ax,08000h
     mov    [word ptr edi],ax
     add    esi, 4
     add    edi, 2
     dec    ecx
     jnz    @@lp
     jmp    @@ende


     @@max:
     fstp   st                  ; (max) (min)
     mov    ax, bx
     jmp    @@next
     @@min:
     fstp   st                  ; (max) (min) 
     mov    ax, dx
     jmp    @@next

     @@ende:
     fstp  st                     ; (min)
     fstp  st                     ; -
     ret

endp clip_16u


proc clip_8s     ; convert/clip samples, 8bit signed

     fld   [dword ptr clampmin]          ; (min)
     fld   [dword ptr clampmax]          ; (max) (min)
     mov   bx,  32767
     mov   dx, -32767

     @@lp:
     fld    [dword ptr esi]     ; (ls) (max) (min)
     fcom   st(1)
     fnstsw ax
     sahf
     ja     @@max
     fcom   st(2)
     fstsw  ax
     sahf
     jb     @@min
     fistp  [word ptr clipval]      ; (max) (min)
     mov    ax,[word ptr clipval]
     @@next:
     mov    [byte ptr edi],ah
     add    esi, 4
     add    edi, 1
     dec    ecx
     jnz    @@lp
     jmp    @@ende


     @@max:
     fstp   st                  ; (max) (min)
     mov    ax, bx
     jmp    @@next
     @@min:
     fstp   st                  ; (max) (min) 
     mov    ax, dx
     jmp    @@next

     @@ende:
     fstp  st                     ; (min)
     fstp  st                     ; -
     ret

endp clip_8s



proc clip_8u     ; convert/clip samples, 8bit unsigned

     fld   [dword ptr clampmin]          ; (min)
     fld   [dword ptr clampmax]          ; (max) (min)
     mov   bx,  32767
     mov   dx, -32767

     @@lp:
     fld    [dword ptr esi]     ; (ls) (max) (min)
     fcom   st(1)
     fnstsw ax
     sahf
     ja     @@max
     fcom   st(2)
     fstsw  ax
     sahf
     jb     @@min
     fistp  [word ptr clipval]      ; (max) (min)
     mov    ax,[word ptr clipval]
     @@next:
     xor    ax,08000h
     mov    [byte ptr edi],ah
     add    esi, 4
     add    edi, 1
     dec    ecx
     jnz    @@lp
     jmp    @@ende


     @@max:
     fstp   st                  ; (max) (min)
     mov    ax, bx
     jmp    @@next
     @@min:
     fstp   st                  ; (max) (min) 
     mov    ax, dx
     jmp    @@next

     @@ende:
     fstp  st                     ; (min)
     fstp  st                     ; -
     ret

endp clip_8u


clippers dd offset clip_8s, offset clip_8u, offset clip_16s, offset clip_16u

mixers dd offset mixm_n, offset mixs_n, offset mixm_i, offset mixs_i
       dd offset mixm_i2, offset mixs_i2, offset mix_0, offset mix_0
       dd offset mixm_nf, offset mixs_nf, offset mixm_if, offset mixs_if
       dd offset mixm_i2f, offset mixs_i2f, offset mix_0, offset mix_0


; additional routines for display mixer



; parm
;   eax : channel #
;   ecx : no of samples

proc getchanvol
public getchanvol

     pushad

     fldz                                  ; (0)
     mov  [_nsamples],ecx

     mov  ebx, [_voiceflags + eax*4]
     test ebx, FLAG_ENABLED
     jz   @@SkipVoice
     mov  ebx, [_looplen + eax*4]
     mov  [dword ptr mixlooplen], ebx
     mov  ebx, [_freqw + eax*4]
     mov  esi, [_freqf + eax*4]
     mov  edx, [_smpposf +eax*4]
     mov  edi, [_loopend +eax*4]
     shr  edi, 2
     mov  ebp, [_smpposw +eax*4]
     shr  ebp, 2

     @@next:                                ; (sum)
       fld   [dword ptr ebp*4]              ; (wert) (sum)
       test  [dword ptr ebp*4],80000000h
       jnz   @@neg
       faddp st(1), st                      ; (sum')
       jmp   @@goon
       @@neg:
       fsubp st(1), st
       @@goon:
       add   edx, esi
       adc   ebp, ebx
       cmp   ebp, edi
       jge   @@LoopHandler
       dec   ecx
     jnz   @@next
     jmp   @@SkipVoice
     @@LoopHandler:
     test  [_voiceflags + eax*4], FLAG_LOOP_ENABLE
     jz    @@SkipVoice
     sub   ebp, [_looplen + eax*4]
     dec   ecx
     jz    @@SkipVoice
     jmp   @@next
     @@SkipVoice:
     fidiv [_nsamples]                      ; (sum'')
     fld   st(0)                            ; (s) (s)
     fmul  [_volleft + eax*4]               ; (sl) (s)
     fstp  [_voll]                          ; (s)
     fmul  [_volright + eax*4]              ; (sr)
     fstp  [_volr]                          ; -

     popad

     ret

endp getchanvol


end
