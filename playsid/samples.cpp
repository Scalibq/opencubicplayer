// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// SIDPlay routines for PlaySID samples/galway noises
//
// revision history: (please note changes here)
//  -kb980717  Tammo Hinrichs <opencp@gmx.net>
//    -first release

//
// 1997/10/01 23:55:11
// 

#include "samples.h"


extern ubyte* c64mem1;
extern ubyte* c64mem2;
extern udword PCMfreq;
extern udword C64_clockSpeed;

static void sampleEmuTryStopAll();
static sbyte sampleEmuSilence();
static sbyte sampleEmu();
static sbyte sampleEmuTwo();

static void GalwayInit();
static sbyte GalwayReturnSample(void);
static inline void GetNextFour(void);
//static void SelectVolume(void);

sbyte (*sampleEmuRout)() = &sampleEmuSilence;

// ---

static udword sampleClock;

enum
{
	FM_NONE,
	FM_GALWAYON,
	FM_GALWAYOFF,
	FM_HUELSON,
	FM_HUELSOFF
};

// Sample Order Modes.
static const ubyte SO_LOWHIGH = 0;
static const ubyte SO_HIGHLOW = 1;


sampleChannel ch4, ch5;


static const sbyte galwayNoiseTab1[16] =
{
	0x80,0x91,0xa2,0xb3,0xc4,0xd5,0xe6,0xf7,
	0x08,0x19,0x2a,0x3b,0x4c,0x5d,0x6e,0x7f
};

static const sbyte sampleConvertTab[16] =
{
//  0x81,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,
//  0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x7f
	0x81,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,
	0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70
};

static ubyte galwayNoiseTab2[64*16];


static void channelReset(sampleChannel& ch)
{
	ch.Active = false;
	ch.Mode = FM_NONE;
	ch.Period = 0;
#if defined(DIRECT_FIXPOINT)
	ch.Pos_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	ch.Pos_stp = (ch.Pos_pnt = 0);
#else
	ch.Pos_stp = 0;
#endif
}

inline void channelFree(sampleChannel& ch, const uword regBase)
{
	ch.Active = false;
	ch.Mode = FM_NONE;
	c64mem2[regBase+0x1d] = 0x00;
}

inline void channelTryInit(sampleChannel& ch, const uword regBase)
{
	if ( ch.Active && ( ch.Mode == FM_GALWAYON ))
		return;

	ch.VolShift = ( 0 - (sbyte)c64mem2[regBase+0x1d] ) >> 1;
	c64mem2[regBase+0x1d] = 0x00;
	ch.Address = readLEword(c64mem2+regBase+0x1e);
	ch.EndAddr = readLEword(c64mem2+regBase+0x3d);
	if (ch.EndAddr <= ch.Address)
	{
		return;
	}
	ch.Repeat = c64mem2[regBase+0x3f];
	ch.RepAddr = readLEword(c64mem2+regBase+0x7e);
	ch.SampleOrder = c64mem2[regBase+0x7d];
	uword tempPeriod = readLEword(c64mem2+regBase+0x5d);
	if ( (ch.Scale=c64mem2[regBase+0x5f]) != 0 )
	{
		tempPeriod >>= ch.Scale;
	}
	if ( tempPeriod == 0 )
	{
		ch.Period = 0;
#if defined(DIRECT_FIXPOINT) 
		ch.Pos_stp.l = (ch.PosAdd_stp.l = 0);
#elif defined(PORTABLE_FIXPOINT)
		ch.Pos_stp = (ch.Pos_pnt = 0);
		ch.PosAdd_stp = (ch.PosAdd_pnt = 0);
#else
		ch.Pos_stp = (ch.PosAdd_stp = 0);
#endif
		ch.Active = false;
		ch.Mode = FM_NONE;
	}
	else
	{ 	  
		if ( tempPeriod != ch.Period ) 
		{
			ch.Period = tempPeriod;
#if defined(DIRECT_FIXPOINT) 
			ch.Pos_stp.l = sampleClock / ch.Period;
#elif defined(PORTABLE_FIXPOINT)
			udword tempDiv = sampleClock / ch.Period;
			ch.Pos_stp = tempDiv >> 16;
			ch.Pos_pnt = tempDiv & 0xFFFF;
#else
			ch.Pos_stp = sampleClock / ch.Period;
#endif
		}
#if defined(DIRECT_FIXPOINT) 
		ch.PosAdd_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
		ch.PosAdd_stp = (ch.PosAdd_pnt = 0);
#else
		ch.PosAdd_stp = 0;
#endif
		ch.Active = true;
		ch.Mode = FM_HUELSON;
	}
}

inline ubyte channelProcess(sampleChannel& ch, const uword regBase)
{
#if defined(DIRECT_FIXPOINT) 
	uword sampleIndex = ch.PosAdd_stp.w[HI] + ch.Address;
#elif defined(PORTABLE_FIXPOINT)
	uword sampleIndex = ch.PosAdd_stp + ch.Address;
#else
	uword sampleIndex = (ch.PosAdd_stp >> 16) + ch.Address;
#endif
    if ( sampleIndex >= ch.EndAddr )
	{
		if ( ch.Repeat != 0xFF )
		{ 
			if ( ch.Repeat != 0 )  
				ch.Repeat--;
			else
			{
				channelFree(ch,regBase);
				return 8;
			}
		}
		sampleIndex = ( ch.Address = ch.RepAddr );
#if defined(DIRECT_FIXPOINT) 
		ch.PosAdd_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
		ch.PosAdd_stp = (ch.PosAdd_pnt = 0);
#else
		ch.PosAdd_stp = 0;
#endif
		if ( sampleIndex >= ch.EndAddr )
		{
			channelFree(ch,regBase);
			return 8;
		}
	}  
  
	ubyte tempSample = c64mem1[sampleIndex];
	if (ch.SampleOrder == SO_LOWHIGH)
	{
		if (ch.Scale == 0)
		{
#if defined(DIRECT_FIXPOINT) 
			if (ch.PosAdd_stp.w[LO] >= 0x8000)
#elif defined(PORTABLE_FIXPOINT)
			if ( ch.PosAdd_pnt >= 0x8000 )
#else
		    if ( (ch.PosAdd_stp & 0x8000) != 0 )
#endif
			{
			    tempSample >>= 4;
			}
		}
		// AND 15 further below.
	}
	else  // if (ch.SampleOrder == SO_HIGHLOW)
	{
		if (ch.Scale == 0)
		{
#if defined(DIRECT_FIXPOINT) 
			if ( ch.PosAdd_stp.w[LO] < 0x8000 )
#elif defined(PORTABLE_FIXPOINT)
		    if ( ch.PosAdd_pnt < 0x8000 )
#else
		    if ( (ch.PosAdd_stp & 0x8000) == 0 )
#endif
			{
			    tempSample >>= 4;
			}
			// AND 15 further below.
		}
		else  // if (ch.Scale != 0)
		{
			tempSample >>= 4;
		}
	}
	
#if defined(DIRECT_FIXPOINT) 
	ch.PosAdd_stp.l += ch.Pos_stp.l;
#elif defined(PORTABLE_FIXPOINT)
	udword temp = (udword)ch.PosAdd_pnt + (udword)ch.Pos_pnt;
	ch.PosAdd_pnt = temp & 0xffff;
	ch.PosAdd_stp += ( ch.Pos_stp + ( temp > 65535 ));
#else
	ch.PosAdd_stp += ch.Pos_stp;
#endif
	
	return (tempSample&0x0F);
}

// ---

void sampleEmuReset()
{
	channelReset(ch4);
	channelReset(ch5);
	sampleClock = (udword) (((C64_clockSpeed / 2.0) / PCMfreq) * 65536UL);
	sampleEmuRout = &sampleEmuSilence;
	if ( c64mem2 != 0 )
	{
		channelFree(ch4,0xd400);
		channelFree(ch5,0xd500);
	}
}

void sampleEmuInit()
{
	int k = 0;
	for ( int i = 0; i < 16; i++ )
	{
		int l = 0;
		for ( int j = 0; j < 64; j++ )
		{
			galwayNoiseTab2[k++] = galwayNoiseTab1[l];
			l = (l+i) & 15;
		}
	}
	sampleEmuReset();
}

static sbyte sampleEmuSilence()
{
	return 0;
}

inline void sampleEmuTryStopAll()
{
	if ( !ch4.Active && !ch5.Active )
	{
		sampleEmuRout = &sampleEmuSilence;
	}
}

void sampleEmuCheckForInit()
{
	// Try first sample channel.
	switch ( c64mem2[0xd41d] ) 
	{
	 case 0xFF:
	 case 0xFE:
		channelTryInit(ch4,0xd400);
		break;
	 case 0xFD:
		channelFree(ch4,0xd400);
		break;
	 case 0xFC:
		channelTryInit(ch4,0xd400);
		break;
	 case 0x00:
		break;
	 default:
		GalwayInit();
		break;
	}
	
	if (ch4.Mode == FM_HUELSON)
	{
		sampleEmuRout = &sampleEmu;
	}
	
	// Try second sample channel.
	switch ( c64mem2[0xd51d] ) 
	{
	 case 0xFF:
	 case 0xFE:
		channelTryInit(ch5,0xd500);
		break;
	 case 0xFD:
		channelFree(ch5,0xd500);
		break;
	 case 0xFC:
		channelTryInit(ch5,0xd500);
		break;
	 default:
		break;
	}
	
	if (ch5.Mode == FM_HUELSON)
	{
		sampleEmuRout = &sampleEmuTwo;
	}
	sampleEmuTryStopAll();
}

static sbyte sampleEmu()
{
	// One sample channel. Return signed 8-bit sample.
	if ( !ch4.Active )
		return 0;
	else
		return (sampleConvertTab[channelProcess(ch4,0xd400)]>>ch4.VolShift);
}

static sbyte sampleEmuTwo()
{
	sbyte sample = 0;
	if ( ch4.Active )
		sample += (sampleConvertTab[channelProcess(ch4,0xd400)]>>ch4.VolShift);
	if ( ch5.Active )
		sample += (sampleConvertTab[channelProcess(ch5,0xd500)]>>ch5.VolShift);
	return sample;
}

// ---
  
static inline void GetNextFour()
{
	uword tempMul = (uword)(c64mem1[ch4.Address+(uword)ch4.Counter])
		* ch4.LoopWait + ch4.NullWait;
	ch4.Counter--;
#if defined(DIRECT_FIXPOINT)
	if ( tempMul != 0 )
		ch4.Period_stp.l = sampleClock / tempMul;
	else
		ch4.Period_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	udword tempDiv; 
	if ( tempMul != 0 )
		tempDiv = sampleClock / tempMul;
	else
		tempDiv = 0;
	ch4.Period_stp = tempDiv >> 16;
	ch4.Period_pnt = tempDiv & 0xFFFF;
#else
	if ( tempMul != 0 )
		ch4.Period_stp = sampleClock / tempMul;
	else
		ch4.Period_stp = 0;
#endif
	ch4.Period = tempMul;
}

static void GalwayInit()
{
	if (ch4.Active) 
		return;
	
	sampleEmuRout = &sampleEmuSilence;
  
	ch4.Counter = c64mem2[0xd41d];  
	c64mem2[0xd41d] = 0; 
  
	if ((ch4.Address=readLEword(c64mem2+0xd41e)) == 0)
		return;
  
	if ((ch4.LoopWait=c64mem2[0xd43f]) == 0)
		return;
  
	if ((ch4.NullWait=c64mem2[0xd45d]) == 0)
		return;
  
	if (c64mem2[0xd43e] == 0)
		return;
	ch4.SamAddr = ((uword)c64mem2[0xd43e]&15) << 6;
  
	if ( c64mem2[0xd43d] == 0 )
		return;
	ch4.SamLen = (((uword)c64mem2[0xd43d])+1) & 0xfffe;
  
	ch4.Active = true;
	ch4.Mode = FM_GALWAYON;
  
	//GalwayFourStart:
#if defined(DIRECT_FIXPOINT)
	ch4.Pos_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	ch4.Pos_stp = 0;
	ch4.Pos_pnt = 0;
#else
	ch4.Pos_stp = 0;
#endif
	//SelectNewVolume();
	GetNextFour();
	ch4.Counter++;
  
	sampleEmuRout = &GalwayReturnSample;
}

static sbyte GalwayReturnSample()
{
#if defined(DIRECT_FIXPOINT)
	sbyte tempSample = galwayNoiseTab2[ ch4.SamAddr + (ch4.Pos_stp.w[HI] & 15) ];
#elif defined(PORTABLE_FIXPOINT)
	sbyte tempSample = galwayNoiseTab2[ ch4.SamAddr + ( ch4.Pos_stp & 15 )];
#else
	sbyte tempSample = galwayNoiseTab2[ ch4.SamAddr + ((ch4.Pos_stp >> 16) & 15) ];
#endif
	
#if defined(DIRECT_FIXPOINT)
	ch4.Pos_stp.l += ch4.Period_stp.l;
#elif defined(PORTABLE_FIXPOINT)
	udword temp = (udword)ch4.Pos_pnt;
	temp += (udword)ch4.Period_pnt;
	ch4.Pos_pnt = temp & 0xffff;
	ch4.Pos_stp += ( ch4.Period_stp + ( temp > 65535 ));
#else
	ch4.Pos_stp += ch4.Period_stp;
#endif

#if defined(DIRECT_FIXPOINT)
	if ( ch4.Pos_stp.w[HI] >= ch4.SamLen )
#elif defined(PORTABLE_FIXPOINT)
	if ( ch4.Pos_stp >= ch4.SamLen )
#else
	if ( (ch4.Pos_stp >> 16) >= ch4.SamLen )
#endif
	{
#if defined(DIRECT_FIXPOINT)
		ch4.Pos_stp.w[HI] = 0;
#elif defined(PORTABLE_FIXPOINT)
		ch4.Pos_stp = 0;
#else
		ch4.Pos_stp &= 0xFFFF;
#endif
	//GalwayFour:
	GetNextFour();
	if ( ch4.Counter == 0xff )  {
	  //GalwayFourEnd:
	  //SelectVolume();
	  ch4.Active = false;
	  ch4.Mode = FM_GALWAYOFF;
	  sampleEmuRout = &sampleEmuSilence;
	}
  }
  return tempSample;
}

//static void SelectVolume()
//{
//	(c64mem2[0xd418] & 15) << 2;
//}

//static void SelectNewVolume()
//{
//	if (( c64mem2[0xd418] & 15 ) < 12 )
//	{
//	}
//}
