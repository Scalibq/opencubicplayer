//
// 1997/05/11 11:29:47
//

#ifndef __6581_SAMPLES_H
  #define __6581_SAMPLES_H


#include "mytypes.h"
#include "myendian.h"

struct sampleChannel
{
        char Active;
	char Mode;
	ubyte Counter;  // Galway
	ubyte Repeat;
	ubyte Scale;
	ubyte SampleOrder;
	sbyte VolShift;
	
	uword Address;
	uword EndAddr;
	uword RepAddr;
	
	uword SamAddr;  // Galway
	uword SamLen;
	uword LoopWait;
	uword NullWait;
	
	uword Period;
#if defined(DIRECT_FIXPOINT) 
	cpuLword Period_stp;
	cpuLword Pos_stp;
	cpuLword PosAdd_stp;
#elif defined(PORTABLE_FIXPOINT)
	uword Period_stp, Period_pnt;
	uword Pos_stp, Pos_pnt;
	uword PosAdd_stp, PosAdd_pnt;
#else
	udword Period_stp;
	udword Pos_stp;
	udword PosAdd_stp;
#endif
};


extern sampleChannel ch4, ch5;

extern void sampleEmuCheckForInit();
extern void sampleEmuInit();          // precalculate tables + reset
extern void sampleEmuReset();         // reset some important variables

extern sbyte (*sampleEmuRout)();


#endif
