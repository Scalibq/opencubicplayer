;// OpenCP Module Player
;// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//
;// definition of some symbols needed by the linker
;//
;// revision history: (please note changes here)
;//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
;//    -first release

.386
.model flat,prolog

.code
public _dllstart_
_dllstart_:

extrn __LibMain:near

end __LibMain
