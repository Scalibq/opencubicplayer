.386p
locals
jumps

.model flat

extrn _wstart2_          :PROC

.data
        dq 303
        ; because PEs needs to have some data...
        db "(c) 1998 by felix domke", 13, 10
        db "this file is part of the dllmt-package. (or even ocp)", 13, 10
.code
        dq 303
        ; ...and some code, this is required.
        ; remove this DQs and see the linkers crashing (at least wlink and
        ; tlink32)
main:
; "hey? there is no code?" you might think.
; well, not quite, since this exe isn't as useless as you might think.
; the cause is:
; in PE-executables, the linker won't generate direct fixups to imported
; symbols, it will create from this... (source)
; jmp _wstart2_
; ...some shit like this... (disassembled exefile)  
; jmp j_wstart2_
; j_wstart2_:    jmp _wstart2_
; (this might make the filesize smaller, in most cases, since a fixup needs
; some bytes... :)
; since we're in flatmode, this seems to be quite useless.
; more optimal would be a once
; jmp _wstart2_
; as we did it in the source..
; well, because at least tlink is creating such a jump for every imported
; symbol, even when it's not used, we can leave out the first jump... :)
; not really optimized, because the .exe has 4096bytes in EVERY case.
; but it's funny to waste your time with these things.
end main
