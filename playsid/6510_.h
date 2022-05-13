//
// 1997/05/30 13:36:14
//

#ifndef __6510_H
#define __6510_H


#include "mytypes.h"


extern char sidKeysOff[32];
extern char sidKeysOn[32];
extern ubyte optr3readWave;
extern ubyte optr3readEnve;
extern ubyte* c64mem1;
extern ubyte* c64mem2;

extern char c64memAlloc();
extern char c64memFree();
extern void c64memClear();
extern void c64memReset(int clockSpeed, ubyte randomSeed);
extern ubyte c64memRamRom(uword address);

extern void initInterpreter(int memoryMode);
extern char interpreter(uword pc, ubyte ramrom, ubyte a, ubyte x, ubyte y);


#endif
