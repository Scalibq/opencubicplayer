.silent
.erase

all: .symbolic cphost.exe vocp.dll

clean: .symbolic
   if exist *.obj del *.obj
   if exist *.lib del *.lib
   if exist *.res del *.res
   if exist vocp.dll del vocp.dll
   if exist cphost.exe del cphost.exe

cpp_opt = /bt=nt /otexan /zq

cphost_obj = cphost.obj
cphost_rc = cphost.rc

vocp_obj = ocp.obj devpdx5.obj
vocp_slibs = dsound.lib kernel32.lib user32.lib
vocp_libs = cphost.lib

cphost.exe: $(cphost_obj) $(cphost_rc)
  *wlink op quiet op stack=100000 file { $(cphost_obj) } name $*.exe system nt_win option version=2.6.0 runtime windows=4.0
  wrc /q /bt=nt $(cphost_rc) $*.exe

cphost.lib: cphost.exe
  *wlib -n -q $*.lib +$*.exe

vocp.dll: $(vocp_obj) $(vocp_libs)
  *wlink op quiet file { $($*_obj) } name $*.dll system nt_dll option version=2.6.0 library { $($*_libs) $($*_slibs) }

.cpp.obj:
  wpp386 $*.cpp $(cpp_opt)
