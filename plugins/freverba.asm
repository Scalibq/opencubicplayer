;// OpenCP Module Player
;// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//
;// assembler routines for float reverb
;//
;// revision history: (please note changes here)
;//  -ryg990513  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
;//    -first release


; AUA?!?
;
; ehem... das SOLL mal optimiert sein... isses momentan natuerlich noch
; nicht... aber asm ist ne gute basis, insofern :)
;
; achso, das erste was ich aendern werde ist die separate behandlung der
; links/rechts-combfilter. weil das unrult :)
;
; und... mir faellt gerade auf, ich benutze an keiner stelle mehr als 4
; stackpositionen... da muss man doch mit overlapping und zusammenschieben
; was hinkriegen koennen :)

.486
.model flat, prolog
smart
jumps
ideal


UDATASEG

extrn          _initfail: dword
extrn          _leftl: dword
extrn          _rightl: dword
extrn          _llen: dword
extrn          _lpos: dword
extrn          _gains: dword
extrn          _outgain: dword
extrn          _vf: dword

buf            dd ?
len            dd ?
rate           dd ?
stereo         dd ?
origr          dd ?

DATASEG

einviertel     dd 0.25

CODESEG

proc   _process_reverb
public _process_reverb
     pushad
     finit

     mov    [buf], eax
     mov    [len], ebx
     mov    [rate], ecx
     mov    [stereo], edx

     cmp    [_initfail], 0
     jne    @@bye

     cmp    [stereo], 0
     jne    @@stereostart
     
     mov    ecx, [len]
     mov    esi, [buf]
     lea    esi, [esi+ecx*4]
     neg    ecx

     align  dword
@@monoloop:
     call   inc_positions

     fldz                              ; (asum)

     mov    edx, 3                     ; die 4 comb filter lines durchgehen
@@monocomb:
     mov    ebx, [_lpos+edx*4]
     shl    ebx, 2
     add    ebx, [_leftl+edx*4]

     fld    [dword ptr esi+ecx*4]      ; (inp) (asum)
     fld    [dword ptr einviertel]     ; (0.25) (inp) (asum)
     fmulp  st(1), st                  ; (inp*0.25) (asum)
     fld    [dword ptr _gains+edx*4]   ; (gain) (inp*0.25) (asum)
     fld    [dword ptr ebx]            ; (line) (gain) (inp*0.25) (asum)
     fmulp  st(1), st                  ; (line*gain) (inp*0.25) (asum)
     faddp  st(1), st                  ; (comb) (asum)
     fst    [dword ptr ebx]            ; (comb) (asum)
     faddp  st(1), st                  ; (asum)

     dec    edx
     jns    @@monocomb

     mov    ebx, [_lpos+4*4]
     shl    ebx, 2
     add    ebx, [_leftl+4*4]

     fld    [dword ptr ebx]            ; (aline1) (asum)
     fxch   st(1)                      ; (asum) (aline1)
     fld    [dword ptr _gains+4*4]     ; (again1) (asum) (aline1)
     fld    st(2)                      ; (aline1) (again1) (asum) (aline1)
     fxch   st(1)                      ; (again1) (aline1) (asum) (aline1)
     fmulp  st(1), st                  ; (apass1-1) (asum) (aline1)
     faddp  st(1), st                  ; (apass1-2) (aline1)
     fst    [dword ptr ebx]            ; (apass1-2) (aline1)
     fldz                              ; (0) (apass1-2) (aline1)
     fsubrp st(1), st                  ; (-apass1-2) (aline1)

     mov    ebx, [_lpos+5*4]
     shl    ebx, 2
     add    ebx, [_leftl+5*4]

     fld    [dword ptr _gains+4*4]     ; (again1) (-apass1-2) (aline1)
     fmulp  st(1), st                  ; (-apass1-3) (aline1)
     faddp  st(1), st                  ; (apass1)

     fld    [dword ptr ebx]            ; (aline2) (apass1)
     fxch   st(1)                      ; (apass1) (aline2)
     fld    [dword ptr _gains+5*4]     ; (again2) (apass1) (aline2)
     fld    st(2)                      ; (aline2) (again2) (apass1) (aline2)
     fxch   st(1)                      ; (again2) (aline2) (apass1) (aline2)
     fmulp  st(1), st                  ; (apass2-1) (apass1) (aline2)
     faddp  st(1), st                  ; (apass2-2) (aline2)
     fst    [dword ptr ebx]            ; (apass2-2) (aline2)
     fldz                              ; (0) (apass2-2) (aline2)
     fsubrp st(1), st                  ; (-apass2-2) (aline2)

     fld    [dword ptr _gains+5*4]     ; (again2) (-apass2-2) (aline2)
     fmulp  st(1), st                  ; (-apass2-3) (aline2)
     faddp  st(1), st                  ; (rvb out)

     fld    [dword ptr _outgain]       ; (outgain) (rvb out)
     fld    [dword ptr esi+ecx*4]      ; (inp) (outgain) (rvb out)
     fld    [dword ptr _vf]            ; (vf) (inp) (outgain) (rvb out)

     fmulp  st(1), st                  ; (inp mixed) (outgain) (rvb out)
     fxch   st(2)                      ; (rvb out) (outgain) (inp mixed)
     fmulp  st(1), st                  ; (rvb out mix) (inp mixed)
     faddp  st(1), st                  ; (final)
     fstp   [dword ptr esi+ecx*4]      ; empty

     inc    ecx                        ; count+inc position
     jnz    @@monoloop

     jmp    @@bye

@@stereostart:
     mov    ecx, [len]
     mov    esi, [buf]
     lea    esi, [esi+ecx*8]
     neg    ecx
     
     jmp    @@stereoloop

     align  dword
@@stereoloop:
     call   inc_positions
     
     mov    eax, [esi+ecx*8+4]         ; originalwert rechts fuer spaeter
     xor    eax, 080000000h            ; speichern (und negieren :)
     mov    [origr], eax               

; linnings

     fldz                              ; (asum)

     mov    edx, 3                     ; die 4 comb filter lines durchgehen
@@stereocomb1:
     mov    ebx, [_lpos+edx*4]
     shl    ebx, 2
     add    ebx, [_leftl+edx*4]

     fld    [dword ptr esi+ecx*8]      ; (inp) (asum)
     fld    [dword ptr einviertel]     ; (0.25) (inp) (asum)
     fmulp  st(1), st                  ; (inp*0.25) (asum)
     fld    [dword ptr _gains+edx*4]   ; (gain) (inp*0.25) (asum)
     fld    [dword ptr ebx]            ; (line) (gain) (inp*0.25) (asum)
     fmulp  st(1), st                  ; (line*gain) (inp*0.25) (asum)
     faddp  st(1), st                  ; (comb) (asum)
     fst    [dword ptr ebx]            ; (comb) (asum)
     faddp  st(1), st                  ; (asum)

     dec    edx
     jns    @@stereocomb1

     mov    ebx, [_lpos+4*4]
     shl    ebx, 2
     add    ebx, [_leftl+4*4]

     fld    [dword ptr ebx]            ; (aline1) (asum)
     fxch   st(1)                      ; (asum) (aline1)
     fld    [dword ptr _gains+4*4]     ; (again1) (asum) (aline1)
     fld    st(2)                      ; (aline1) (again1) (asum) (aline1)
     fxch   st(1)                      ; (again1) (aline1) (asum) (aline1)
     fmulp  st(1), st                  ; (apass1-1) (asum) (aline1)
     faddp  st(1), st                  ; (apass1-2) (aline1)
     fst    [dword ptr ebx]            ; (apass1-2) (aline1)
     fldz                              ; (0) (apass1-2) (aline1)
     fsubrp st(1), st                  ; (-apass1-2) (aline1)

     mov    ebx, [_lpos+5*4]
     shl    ebx, 2
     add    ebx, [_leftl+5*4]

     fld    [dword ptr _gains+4*4]     ; (again1) (-apass1-2) (aline1)
     fmulp  st(1), st                  ; (-apass1-3) (aline1)
     faddp  st(1), st                  ; (apass1)

     fld    [dword ptr ebx]            ; (aline2) (apass1)
     fxch   st(1)                      ; (apass1) (aline2)
     fld    [dword ptr _gains+5*4]     ; (again2) (apass1) (aline2)
     fld    st(2)                      ; (aline2) (again2) (apass1) (aline2)
     fxch   st(1)                      ; (again2) (aline2) (apass1) (aline2)
     fmulp  st(1), st                  ; (apass2-1) (apass1) (aline2)
     faddp  st(1), st                  ; (apass2-2) (aline2)
     fst    [dword ptr ebx]            ; (apass2-2) (aline2)
     fldz                              ; (0) (apass2-2) (aline2)
     fsubrp st(1), st                  ; (-apass2-2) (aline2)

     fld    [dword ptr _gains+5*4]     ; (again2) (-apass2-2) (aline2)
     fmulp  st(1), st                  ; (-apass2-3) (aline2)
     faddp  st(1), st                  ; (rvb out l)

     fld    [dword ptr _outgain]       ; (outgain) (rvb out l)
     fld    [dword ptr esi+ecx*8+4]    ; (inp r) (outgain) (rvb out l)
     fld    [dword ptr _vf]            ; (vf) (inp) (outgain) (rvb out)

     fmulp  st(1), st                  ; (inp mixed) (outgain) (rvb out)
     fxch   st(2)                      ; (rvb out) (outgain) (inp mixed)
     fmulp  st(1), st                  ; (rvb out mix) (inp mixed)
     faddp  st(1), st                  ; (final)
     fstp   [dword ptr esi+ecx*8+4]    ; empty

; rechts

     fldz                              ; (asum)

     mov    edx, 3                     ; die 4 comb filter lines durchgehen
@@stereocomb2:
     mov    ebx, [_lpos+edx*4]
     shl    ebx, 2
     add    ebx, [_rightl+edx*4]

     fld    [dword ptr origr]          ; (inp) (asum)
     fld    [dword ptr einviertel]     ; (0.25) (inp) (asum)
     fmulp  st(1), st                  ; (inp*0.25) (asum)
     fld    [dword ptr _gains+edx*4]   ; (gain) (inp*0.25) (asum)
     fld    [dword ptr ebx]            ; (line) (gain) (inp*0.25) (asum)
     fmulp  st(1), st                  ; (line*gain) (inp*0.25) (asum)
     faddp  st(1), st                  ; (comb) (asum)
     fst    [dword ptr ebx]            ; (comb) (asum)
     faddp  st(1), st                  ; (asum)

     dec    edx
     jns    @@stereocomb2

     mov    ebx, [_lpos+4*4]
     shl    ebx, 2
     add    ebx, [_rightl+4*4]

     fld    [dword ptr ebx]            ; (aline1) (asum)
     fxch   st(1)                      ; (asum) (aline1)
     fld    [dword ptr _gains+4*4]     ; (again1) (asum) (aline1)
     fld    st(2)                      ; (aline1) (again1) (asum) (aline1)
     fxch   st(1)                      ; (again1) (aline1) (asum) (aline1)
     fmulp  st(1), st                  ; (apass1-1) (asum) (aline1)
     faddp  st(1), st                  ; (apass1-2) (aline1)
     fst    [dword ptr ebx]            ; (apass1-2) (aline1)
     fldz                              ; (0) (apass1-2) (aline1)
     fsubrp st(1), st                  ; (-apass1-2) (aline1)

     mov    ebx, [_lpos+5*4]
     shl    ebx, 2
     add    ebx, [_rightl+5*4]

     fld    [dword ptr _gains+4*4]     ; (again1) (-apass1-2) (aline1)
     fmulp  st(1), st                  ; (-apass1-3) (aline1)
     faddp  st(1), st                  ; (apass1)

     fld    [dword ptr ebx]            ; (aline2) (apass1)
     fxch   st(1)                      ; (apass1) (aline2)
     fld    [dword ptr _gains+5*4]     ; (again2) (apass1) (aline2)
     fld    st(2)                      ; (aline2) (again2) (apass1) (aline2)
     fxch   st(1)                      ; (again2) (aline2) (apass1) (aline2)
     fmulp  st(1), st                  ; (apass2-1) (apass1) (aline2)
     faddp  st(1), st                  ; (apass2-2) (aline2)
     fst    [dword ptr ebx]            ; (apass2-2) (aline2)
     fldz                              ; (0) (apass2-2) (aline2)
     fsubrp st(1), st                  ; (-apass2-2) (aline2)

     fld    [dword ptr _gains+5*4]     ; (again2) (-apass2-2) (aline2)
     fmulp  st(1), st                  ; (-apass2-3) (aline2)
     faddp  st(1), st                  ; (rvb out r)

     fld    [dword ptr _outgain]       ; (outgain) (rvb out r)
     fld    [dword ptr esi+ecx*8]      ; (inp l) (outgain) (rvb out r)
     fld    [dword ptr _vf]            ; (vf) (inp) (outgain) (rvb out)

     fmulp  st(1), st                  ; (inp mixed) (outgain) (rvb out)
     fxch   st(2)                      ; (rvb out) (outgain) (inp mixed)
     fmulp  st(1), st                  ; (rvb out mix) (inp mixed)
     faddp  st(1), st                  ; (final)
     fstp   [dword ptr esi+ecx*8]      ; empty

@@stdoloop:
     inc    ecx                        ; count+inc position
     jnz    @@stereoloop

@@bye:
     popad
     ret

endp   _process_reverb

proc   inc_positions
     mov    edx, 5                     
@@incloop:
       mov    eax, [_lpos+edx*4]       ; ich habe hier nen jb durch
       inc    eax                      ; die komische sbb/and-kombination
       cmp    eax, [_llen+edx*4]       ; ersetzt-dieser abschnitt ist 
       sbb    ebx, ebx                 ; ohnehin schon langsam genug,
       and    eax, ebx                 ; da muss ich nicht auchnoch gross
       mov    [_lpos+edx*4], eax       ; rumjumpen
       dec    edx                      
     jns    @@incloop                  ; DAS IST KRANK.

     ret
endp   inc_positions

end
