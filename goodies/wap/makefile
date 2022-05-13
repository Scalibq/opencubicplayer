# WAP makefile for Watcom 10.x/11.0, quickly h4x0r3d together by KB

.silent

all: .symbolic
  @echo.
  @echo now making the WrfelAnimator PROFESSIONAL, please stand by...
  @echo.
  %make wap.exe
  @echo.
  @echo Have fun!

wap.obj: wap.cpp
  @echo wap.obj
  wpp386 /zq /otexan /zp1 /4r /s wap.cpp

wap.exe: wap.obj
  @echo wap.exe
  wlink option quiet file wap.obj name wap.exe system dos4g

clean: .symbolic
  @del wap.obj wap.exe >nul


