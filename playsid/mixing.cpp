// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// SIDPlay mixing routines 
//
// revision history: (please note changes here)
//  -kb980717  Tammo Hinrichs <opencp@gmx.net>
//    -first release

//
// 1997/05/11 11:29:47
//

#include "mytypes.h"
#include "opstruct.h"
#include "samples.h"


extern sidOperator optr1, optr2, optr3;          // -> 6581_.cc
extern uword voice4_gainLeft, voice4_gainRight;

extern char sidpmute[4];

static const int maxLogicalVoices = 4;

static const int mix8monoMiddleIndex = 256*maxLogicalVoices/2;
static ubyte mix8mono[256*maxLogicalVoices];

static const int mix8stereoMiddleIndex = 256*(maxLogicalVoices/2)/2;
static ubyte mix8stereo[256*(maxLogicalVoices/2)];

static const int mix16monoMiddleIndex = 256*maxLogicalVoices/2;
static uword mix16mono[256*maxLogicalVoices];

static const int mix16stereoMiddleIndex = 256*(maxLogicalVoices/2)/2;
static uword mix16stereo[256*(maxLogicalVoices/2)];

sbyte *signedPanMix8 = 0;
sword *signedPanMix16 = 0;

static ubyte zero8bit;   // ``zero''-sample
static uword zero16bit;  // either signed or unsigned
udword splitBufferLen;

void* fill8bitMono( void* buffer, udword numberOfSamples );
void* fill8bitMonoControl( void* buffer, udword numberOfSamples );
void* fill8bitStereo( void* buffer, udword numberOfSamples );
void* fill8bitStereoControl( void* buffer, udword numberOfSamples );
void* fill8bitStereoSurround( void* buffer, udword numberOfSamples );
void* fill8bitsplit( void* buffer, udword numberOfSamples );
void* fill16bitMono( void* buffer, udword numberOfSamples );
void* fill16bitMonoControl( void* buffer, udword numberOfSamples );
void* fill16bitStereo( void* buffer, udword numberOfSamples );
void* fill16bitStereoControl( void* buffer, udword numberOfSamples );
void* fill16bitStereoSurround( void* buffer, udword numberOfSamples );
void* fill16bitsplit( void* buffer, udword numberOfSamples );


void MixerInit(char threeVoiceAmplify, ubyte zero8, uword zero16)
{
	zero8bit = zero8;
	zero16bit = zero16;
	
	int si;
	uword ui;
	
	float ampDiv = maxLogicalVoices;
	if (threeVoiceAmplify)
	{
		ampDiv = maxLogicalVoices-1;
	}
	
	ui = 0;
	for ( si = -128*maxLogicalVoices; si < 0; si++ )
		mix8mono[ui++] = (ubyte)(si/ampDiv) + zero8bit;
	for ( si = 0; si < 128*maxLogicalVoices; si++ )
		mix8mono[ui++] = (ubyte)(si/ampDiv) + zero8bit;

	ui = 0;
	for ( si = -128*(maxLogicalVoices/2); si < 0; si++ )
		mix8stereo[ui++] = (ubyte)(si/(ampDiv/2)) + zero8bit;
	for ( si = 0; si < 128*(maxLogicalVoices/2); si++ )
		mix8stereo[ui++] = (ubyte)(si/(ampDiv/2)) + zero8bit;

	ui = 0;
	for ( si = -128*maxLogicalVoices; si < 0; si++ )
		mix16mono[ui++] = (uword)(si*256/ampDiv) + zero16bit;
	for ( si = 0; si < 128*maxLogicalVoices; si++ )
		mix16mono[ui++] = (uword)(si*256/ampDiv) + zero16bit;

	ui = 0;
	for ( si = -128*(maxLogicalVoices/2); si < 0; si++ )
		mix16stereo[ui++] = (uword)(si*256/(ampDiv/2)) + zero16bit;
	for ( si = 0; si < 128*(maxLogicalVoices/2); si++ )
		mix16stereo[ui++] = (uword)(si*256/(ampDiv/2)) + zero16bit;
}


inline void syncEm()
{
        char sync1, sync2, sync3;
	optr1.cycleLenCount--;
	optr2.cycleLenCount--;
	optr3.cycleLenCount--;
	sync1 = (optr1.modulator->cycleLenCount <= 0);
	sync2 = (optr2.modulator->cycleLenCount <= 0);
	sync3 = (optr3.modulator->cycleLenCount <= 0);
	if (optr1.sync && sync1)
	{
		optr1.cycleLenCount = 0;
#if defined(DIRECT_FIXPOINT)
		optr1.waveStep.l = 0;
#else
		optr1.waveStep = (optr1.waveStepPnt = 0);
#endif
	}
	if (optr2.sync && sync2)
	{
		optr2.cycleLenCount = 0;
#if defined(DIRECT_FIXPOINT)
		optr2.waveStep.l = 0;
#else
		optr2.waveStep = (optr2.waveStepPnt = 0);
#endif
	}
	if (optr3.sync && sync3)
	{
		optr3.cycleLenCount = 0;
#if defined(DIRECT_FIXPOINT)
		optr3.waveStep.l = 0;
#else
		optr3.waveStep = (optr3.waveStepPnt = 0);
#endif
	}
}


//
// -------------------------------------------------------------------- 8-bit
//

void* fill8bitMono( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
	    *buffer8bit++ = mix8mono[(unsigned)(mix8monoMiddleIndex
											+(*optr1.outProc)(&optr1)
											+(*optr2.outProc)(&optr2)
											+(*optr3.outProc)(&optr3)
											+(*sampleEmuRout)())];
		syncEm();
	}
	return buffer8bit;
}

void* fill8bitMonoControl( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainLeft+(*optr1.outProc)(&optr1)]
			+signedPanMix8[optr2.gainLeft+(*optr2.outProc)(&optr2)]
			+signedPanMix8[optr3.gainLeft+(*optr3.outProc)(&optr3)]
			+signedPanMix8[voice4_gainLeft+(*sampleEmuRout)()];
		syncEm();
	}
	return buffer8bit;
}

void* fill8bitStereo( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
  	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		// left
	    *buffer8bit++ = mix8stereo[(unsigned)(mix8stereoMiddleIndex
											 +(*optr1.outProc)(&optr1)
											 +(*optr3.outProc)(&optr3))];
		// right
	    *buffer8bit++ = mix8stereo[(unsigned)(mix8stereoMiddleIndex
											 +(*optr2.outProc)(&optr2)
											 +(*sampleEmuRout)())];
		syncEm();
	}
	return buffer8bit;
}

void* fill8bitStereoControl( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (*optr1.outProc)(&optr1);
		voice2data = (*optr2.outProc)(&optr2);
		voice3data = (*optr3.outProc)(&optr3);
		voice4data = (*sampleEmuRout)();
		// left
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainLeft+voice1data]
                        +signedPanMix8[optr2.gainLeft+voice2data]
			+signedPanMix8[optr3.gainLeft+voice3data]
			+signedPanMix8[voice4_gainLeft+voice4data];
		// right
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainRight+voice1data]
			+signedPanMix8[optr2.gainRight+voice2data]
			+signedPanMix8[optr3.gainRight+voice3data]
			+signedPanMix8[voice4_gainRight+voice4data];
		syncEm();
	}
	return buffer8bit;
}

void* fill8bitStereoSurround( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (*optr1.outProc)(&optr1);
		voice2data = (*optr2.outProc)(&optr2);
		voice3data = (*optr3.outProc)(&optr3);
		voice4data = (*sampleEmuRout)();
		// left
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainLeft+voice1data]
		    +signedPanMix8[optr2.gainLeft+voice2data]
			+signedPanMix8[optr3.gainLeft+voice3data]
			+signedPanMix8[voice4_gainLeft+voice4data];
		// right
		*buffer8bit++ = zero8bit
			-signedPanMix8[optr1.gainRight+voice1data]
			-signedPanMix8[optr2.gainRight+voice2data]
			-signedPanMix8[optr3.gainRight+voice3data]
			-signedPanMix8[voice4_gainRight+voice4data];
		syncEm();
	}
	return buffer8bit;
}

void* fill8bitsplit( void* buffer, udword numberOfSamples )
{
	ubyte* v1buffer8bit = (ubyte*)buffer;
	ubyte* v2buffer8bit = v1buffer8bit + splitBufferLen;
	ubyte* v3buffer8bit = v2buffer8bit + splitBufferLen;
	ubyte* v4buffer8bit = v3buffer8bit + splitBufferLen;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*v1buffer8bit++ = zero8bit+(*optr1.outProc)(&optr1);
		*v2buffer8bit++ = zero8bit+(*optr2.outProc)(&optr2);
		*v3buffer8bit++ = zero8bit+(*optr3.outProc)(&optr3);
		*v4buffer8bit++ = zero8bit+(*sampleEmuRout)();
		syncEm();
	}
	return v1buffer8bit;
}

//
// ------------------------------------------------------------------- 16-bit
//

void* fill16bitMono( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
	    *buffer16bit++ = mix16mono[(unsigned)(mix16monoMiddleIndex
											 +(*optr1.outProc)(&optr1)
											 +(*optr2.outProc)(&optr2)
											 +(*optr3.outProc)(&optr3)
											 +(*sampleEmuRout)())];		syncEm();
	}
	return buffer16bit;
}

void* fill16bitMonoControl( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*buffer16bit++ = zero16bit
			+signedPanMix16[optr1.gainLeft+(*optr1.outProc)(&optr1)]
			+signedPanMix16[optr2.gainLeft+(*optr2.outProc)(&optr2)]
			+signedPanMix16[optr3.gainLeft+(*optr3.outProc)(&optr3)]
			+signedPanMix16[voice4_gainLeft+(*sampleEmuRout)()];
		syncEm();
	}
	return buffer16bit;
}

void* fill16bitStereo( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		// left
	    *buffer16bit++ = mix16stereo[(unsigned)(mix16stereoMiddleIndex
                             +(*optr1.outProc)(&optr1)
                             +(*optr3.outProc)(&optr3))];
		// right
	    *buffer16bit++ = mix16stereo[(unsigned)(mix16stereoMiddleIndex
                             +(*optr2.outProc)(&optr2)
                             +(*sampleEmuRout)())];
		syncEm();
	}
	return buffer16bit;
}


extern short v4outl, v4outr;

void* fill16bitStereoControl( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (*optr1.outProc)(&optr1);
		voice2data = (*optr2.outProc)(&optr2);
		voice3data = (*optr3.outProc)(&optr3);
		voice4data = (*sampleEmuRout)();
                v4outl = zero16bit+signedPanMix16[voice4_gainLeft+voice4data];
                v4outr = zero16bit+signedPanMix16[voice4_gainRight+voice4data];
		// left
                *buffer16bit =  zero16bit;
                if (!sidpmute[0])
                  *buffer16bit+=signedPanMix16[optr1.gainLeft+voice1data];
                if (!sidpmute[1])
                  *buffer16bit+=signedPanMix16[optr2.gainLeft+voice2data];
                if (!sidpmute[2])                                   
                  *buffer16bit+=signedPanMix16[optr3.gainLeft+voice3data];
                if (!sidpmute[3])
                  *buffer16bit+=signedPanMix16[voice4_gainLeft+voice4data];
                buffer16bit++;
		// right
                *buffer16bit = zero16bit;
                if (!sidpmute[0])
                  *buffer16bit+=signedPanMix16[optr1.gainRight+voice1data];
                if (!sidpmute[1])
                  *buffer16bit+=signedPanMix16[optr2.gainRight+voice2data];
                if (!sidpmute[2])                                   
                  *buffer16bit+=signedPanMix16[optr3.gainRight+voice3data];
                if (!sidpmute[3])
                  *buffer16bit+=signedPanMix16[voice4_gainRight+voice4data];
                buffer16bit++;
                syncEm();
	}
	return buffer16bit;
}

void* fill16bitStereoSurround( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (*optr1.outProc)(&optr1);
		voice2data = (*optr2.outProc)(&optr2);
		voice3data = (*optr3.outProc)(&optr3);
		voice4data = (*sampleEmuRout)();
		// left
		*buffer16bit++ = zero16bit
			+signedPanMix16[optr1.gainLeft+voice1data]
		    +signedPanMix16[optr2.gainLeft+voice2data]
			+signedPanMix16[optr3.gainLeft+voice3data]
			+signedPanMix16[voice4_gainLeft+voice4data];
		// right
		*buffer16bit++ = zero16bit
			-signedPanMix16[optr1.gainRight+voice1data]
			-signedPanMix16[optr2.gainRight+voice2data]
			-signedPanMix16[optr3.gainRight+voice3data]
			-signedPanMix16[voice4_gainRight+voice4data];
		syncEm();
	}
	return buffer16bit;
}

void* fill16bitsplit( void* buffer, udword numberOfSamples )
{
	sword* v1buffer16bit = (sword*)buffer;
	sword* v2buffer16bit = v1buffer16bit + splitBufferLen;
	sword* v3buffer16bit = v2buffer16bit + splitBufferLen;
	sword* v4buffer16bit = v3buffer16bit + splitBufferLen;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*v1buffer16bit++ = zero16bit+(*optr1.outProc)(&optr1);
		*v2buffer16bit++ = zero16bit+(*optr2.outProc)(&optr2);
		*v3buffer16bit++ = zero16bit+(*optr3.outProc)(&optr3);
		*v4buffer16bit++ = zero16bit+(*sampleEmuRout)();
		syncEm();
	}
	return v1buffer16bit;
}
