// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// SIDPlay mail player routines
//
// revision history: (please note changes here)
//  -kb980717  Tammo Hinrichs <opencp@gmx.net>
//    -first release

//
// 1997/09/27 21:34:26
//

#include "sidplay.h"
#include "myendian.h"
#include "6510_.h"

extern ubyte playRamRom;

// These texts are used to override the sidtune settings.

static char text_PAL_VBI[] = "50 Hz VBI (PAL)";
static char text_PAL_CIA[] = "CIA 1 Timer A (PAL)";
static char text_NTSC_VBI[] = "60 Hz VBI (NTSC)";
static char text_NTSC_CIA[] = "CIA 1 Timer A (NTSC)";

// Table used to check sidtune for usage of PlaySID digis.

static const uword c64addrTable[] =
{
	// PlaySID extended SID registers (0xd49d left out).
	//0xd41d, 0xd41e, 0xd41f,  // SID is too often written to as 32 bytes !
	0xd43d, 0xd43e, 0xd43f,
	0xd45d, 0xd45e, 0xd45f, 0xd47d, 0xd47e, 0xd47f
};

// Number of addresses in c64addrTable[].

static const int numberOfC64addr = sizeof(c64addrTable) / sizeof(uword);
static ubyte oldValues[numberOfC64addr];


char sidEmuInitializeSong(emuEngine &thisEmuEngine,
                          sidTune &thisTune,
                          uword songNumber)
{
	// Do a regular song initialization.
        char ret = sidEmuInitializeSongOld(thisEmuEngine,thisTune,songNumber);
	if (ret && (thisEmuEngine.config.digiPlayerScans!=0))
	{
		uword replayPC = thisTune.info.playAddr;
		// playRamRom was set by sidEmuInitializeSongOld(..).
		if ( replayPC == 0 )
		{
			playRamRom = c64mem1[1];
			if ((playRamRom & 2) != 0)  // isKernal ?
			{
				replayPC = readLEword(c64mem1+0x0314);  // IRQ
			}
			else
			{
				replayPC = readLEword(c64mem1+0xfffe);  // NMI
			}
		}
		
		// Run the music player for a couple of player calls and check for
		// changes in the PlaySID extended SID registers. If no digis are
		// used, apply a higher amplification on each SID voice. First
		// check also covers writings of the player INIT routine.
                char useDigis = false;
		int loops = thisEmuEngine.config.digiPlayerScans;
		while (loops)
		{
			for (int i = 0; i < numberOfC64addr; i++)
			{
				if (oldValues[i] != c64mem2[c64addrTable[i]])
				{
					useDigis = true;
					break;
				}
				oldValues[i] = c64mem2[c64addrTable[i]];
			}
			interpreter(replayPC,playRamRom,0,0,0);
			loops--;
			if (useDigis)
			{
				break;
			}
		};
		thisEmuEngine.amplifyThreeVoiceTunes(!useDigis);
		// Here re-init song, coz it was run.
		ret = sidEmuInitializeSongOld(thisEmuEngine,thisTune,songNumber);
	}
	return ret;
}


char sidEmuInitializeSongOld(emuEngine & thisEmuEngine,
							 sidTune & thisTune,
							 uword songNumber)
{
	if (!thisEmuEngine.isReady || !thisTune.status )
	{
		return false;
	}
	else
	{
		// ------------------------------------------- Determine clock/speed.
		
		// Get speed/clock setting for song and preselect
		// speed/clock descriptions strings, reg = song init akkumulator.
		ubyte reg = thisTune.selectSong(songNumber) -1;

		ubyte the_clock, the_speed;
		char* the_description;

		// ------------------------------------------------ Force song speed.
		
		if (thisEmuEngine.config.forceSongSpeed)
		{
			if (thisEmuEngine.config.clockSpeed == SIDTUNE_CLOCK_NTSC)
			{
				// Set clock speed to NTSC.
				the_clock = SIDTUNE_CLOCK_NTSC;
				if (thisTune.info.songSpeed == SIDTUNE_SPEED_CIA_1A)  // CIA speed ?
				{
					// Set speed mode to CIA. Unless default CIA timer
					// will be changed, the resulting speed will be 60 Hz.
					the_speed = SIDTUNE_SPEED_CIA_1A;
					the_description = text_NTSC_CIA;
				}
				else  // fixed speed
				{
					if ( ((thisTune.info.clockSpeed == SIDTUNE_CLOCK_PAL)
						  && (thisTune.info.songSpeed == SIDTUNE_SPEED_VBI_PAL))
						|| ((thisTune.info.clockSpeed == SIDTUNE_CLOCK_NTSC)
							&& (thisTune.info.songSpeed == SIDTUNE_SPEED_VBI_NTSC)) )
					{
						// Set speed mode to 60 Hz instead of 50|60 Hz.
						// This will be same speed as NTSC VBI.
						the_speed = SIDTUNE_SPEED_VBI_NTSC;
						the_description = text_NTSC_VBI;
					}
					else
					{
						// Set speed mode to fixed speed in Hz.
						the_speed = thisTune.info.songSpeed;
						the_description = text_NTSC_CIA;
					}
				}
			}
			else  //if (thisEmuEngine.config.clockSpeed == SIDTUNE_CLOCK_PAL)
			{
				// Set clock speed to PAL.
				the_clock = SIDTUNE_CLOCK_PAL;
				if (thisTune.info.songSpeed == SIDTUNE_SPEED_CIA_1A)  // CIA speed ?
				{
					// Set speed mode to CIA. Unless default CIA timer
					// will be changed, the resulting speed will be 60 Hz.
					the_speed = SIDTUNE_SPEED_CIA_1A;
					the_description = text_PAL_CIA;
				}
				else  // fixed speed
				{
					if ( ((thisTune.info.clockSpeed == SIDTUNE_CLOCK_NTSC)
						  && (thisTune.info.songSpeed == SIDTUNE_SPEED_VBI_NTSC))
						|| ((thisTune.info.clockSpeed == SIDTUNE_CLOCK_PAL)
							&& (thisTune.info.songSpeed == SIDTUNE_SPEED_VBI_PAL)) )
					{
						// Set speed mode to 50 Hz instead of 60|50 Hz.
						// This will be same speed as PAL VBI.
						the_speed = SIDTUNE_SPEED_VBI_PAL;
						the_description = text_PAL_VBI;
					}
					else
					{
						// Set speed mode to fixed speed in Hz.
						the_speed = thisTune.info.songSpeed;
						the_description = text_PAL_CIA;
					}
				}
			}
		}
		else  // ---------------------------------- Do not force clock speed.
		{
			// Set clock speed (PAL/NTSC) and song speed (VBI/CIA).
			the_clock = thisTune.info.clockSpeed;
			the_speed = thisTune.info.songSpeed;
			// Set speed string according to PAL/NTSC clock speed.
			if (thisTune.info.clockSpeed == SIDTUNE_CLOCK_PAL)
			{
				if (thisTune.info.songSpeed == SIDTUNE_SPEED_VBI_PAL)  // VBI ?
				{
					the_description = text_PAL_VBI;
				}
				else  //if (thisTune.info.songSpeed == SIDTUNE_SPEED_CIA_1A)
				{
					the_description = text_PAL_CIA;
				}
			}
			else  //if (thisTune.info.clockSpeed == SIDTUNE_CLOCK_NTSC)
			{
				if (thisTune.info.songSpeed == SIDTUNE_SPEED_VBI_NTSC)  // VBI ?
				{
					the_description = text_NTSC_VBI;
				}
				else  //if (thisTune.info.songSpeed == SIDTUNE_SPEED_CIA_1A)
				{
					the_description = text_NTSC_CIA;
				}
			}
		}
		// Here transfer the settings to the emulator engine.
		// From here we do not touch the SID clock speed setting.
		// -> emu/6581.cc
		extern void sidEmuSetReplayingSpeed(int clockMode, uword callsPerSec);
		sidEmuSetReplayingSpeed(the_clock,the_speed);
		thisTune.info.speedString = the_description;
 
		// ------------------------------------------------------------------
		
		thisEmuEngine.MPUreset();
				
		if ( !thisTune.placeSidTuneInC64mem(thisEmuEngine.MPUreturnRAMbase()) )
		{
			return false;
		}

		thisEmuEngine.amplifyThreeVoiceTunes(false);  // assume fourth voice (digis)
		if ( !thisEmuEngine.reset() )  // currently always returns true
		{
			return false;
		}

		for (int i = 0; i < numberOfC64addr; i++)
		{
			oldValues[i] = c64mem2[c64addrTable[i]];
		}
		
		// In PlaySID-mode the interpreter will ignore some of the parameters.
                //char retcode =
		interpreter(thisTune.info.initAddr,c64memRamRom(thisTune.info.initAddr),reg,reg,reg);
		playRamRom = c64memRamRom(thisTune.info.playAddr);
		
		// This code is only used to be able to print out the initial IRQ address.
		if (thisTune.info.playAddr == 0)
		{
			// Get the address of the interrupt handler.
			if ((c64mem1[1] & 2) != 0)  // isKernal ?
			{
				thisTune.setIRQaddress(readEndian(c64mem1[0x0315],c64mem1[0x0314]));  // IRQ
			}
			else
			{
				thisTune.setIRQaddress(readEndian(c64mem1[0xffff],c64mem1[0xfffe]));  // NMI
			}
		}
		else
		{
			thisTune.setIRQaddress(0);
		}

#if defined(SIDEMU_TIME_COUNT)
		thisEmuEngine.resetSecondsThisSong();
#endif
		return true;

	}  // top else (all okay)
}
