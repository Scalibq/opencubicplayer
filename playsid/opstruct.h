//
// 1997/05/30 13:36:14
//

#ifndef __OPSTRUCT_H
#define __OPSTRUCT_H


#include "mytypes.h"
#include "myendian.h"


typedef sbyte (*ptr2sidFunc)(struct sidOperator *);
typedef uword (*ptr2sidUwordFunc)(struct sidOperator *);
typedef void (*ptr2sidVoidFunc)(struct sidOperator *);

struct sw_storage
{
        udword len;
#if defined(DIRECT_FIXPOINT)
	udword stp;
#else
	udword pnt;
        sdword stp;
#endif
};

struct sidOperator
{
	udword SIDfreq;
	uword SIDpulseWidth;
	ubyte SIDctrl;
	ubyte SIDAD, SIDSR;
	
	sidOperator* carrier;
	sidOperator* modulator;
        char sync;
	
        udword pulseIndex, newPulseIndex;
        udword curSIDfreq;
        udword curNoiseFreq;
	
	ubyte output;
	
	char filtVoiceMask;
        char filtEnabled;
        float filtLow, filtBand, filtHigh;
	sbyte filtIO;
	ubyte filtType;
	
	uword gainLeft, gainRight;  // volume in highbyte
	uword gainSource, gainDest;
	uword gainLeftCentered, gainRightCentered;
        char gainDirec;
	
	sdword cycleLenCount;
#if defined(DIRECT_FIXPOINT)
	cpuLword cycleLen, cycleAddLen;
#else
	udword cycleAddLenPnt;
        udword cycleLen, cycleLenPnt;
#endif
	
	ptr2sidFunc outProc;
	ptr2sidVoidFunc waveProc;

#if defined(DIRECT_FIXPOINT)
	cpuLword waveStep, waveStepAdd;
#else
        uword waveStep, waveStepAdd;
	udword waveStepPnt, waveStepAddPnt;
#endif
        uword waveStepOld;
	struct sw_storage wavePre[2];

#if defined(DIRECT_FIXPOINT) && defined(LARGE_NOISE_TABLE)
	cpuLword noiseReg;
#elif defined(DIRECT_FIXPOINT)
	cpuLBword noiseReg;
#else
	udword noiseReg;
#endif
	udword noiseStep, noiseStepAdd;
	ubyte noiseOutput;
        char noiseIsLocked;

	ubyte ADSRctrl;
        char gateOnCtrl, gateOffCtrl;
    ptr2sidUwordFunc ADSRproc;
	
#ifdef SID_FPUENVE
	float fenveStep, fenveStepAdd;
	udword enveStep;
#elif defined(DIRECT_FIXPOINT)
	cpuLword enveStep, enveStepAdd;
#else
	uword enveStep, enveStepAdd;
	udword enveStepPnt, enveStepAddPnt;
#endif
	ubyte enveVol, enveSusVol;
	uword enveShortAttackCount;
};


#endif
