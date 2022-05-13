NAME  = vapc
AFLAGS= -coff -nologo -W3 -c -Cx -DMASM6 -DBLD_COFF
LFLAGS= /VXD /NODEFAULTLIB /NOLOGO

all :	$(NAME).vxd

$(NAME).vxd: $(NAME).def $(NAME).obj
	link $(LFLAGS) /DEF:$(NAME).def $(NAME).obj

$(NAME).obj: $(NAME).asm
	ml $(AFLAGS) $<

clean:
	if exist *.exp del *.exp > nul
	if exist *.lib del *.lib > nul
	if exist *.lnk del *.lnk > nul
	if exist *.obj del *.obj > nul
