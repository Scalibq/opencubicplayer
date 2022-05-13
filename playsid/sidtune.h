//
// 1997/09/27 21:33:14
//

#ifndef __SIDTUNE_H
#define __SIDTUNE_H


#include <fstream.h>
#include "mytypes.h"

static const int classMaxSongs = 256;
static const int infoStringNum = 5;     // maximum
static const int infoStringLen = 80+1;  // 80 characters plus terminating zero

static const int SIDTUNE_SPEED_CIA_1A = 0;     // CIA 1 Timer A
static const int SIDTUNE_SPEED_VBI_PAL = 50;   // Hz
static const int SIDTUNE_SPEED_VBI_NTSC = 60;  // Hz

static const int SIDTUNE_CLOCK_PAL = 0;   // These are also used in the
static const int SIDTUNE_CLOCK_NTSC = 1;  // emulator engine !

// An instance of this structure is used to transport values to
// and from the ``sidTune-class'';
struct sidTuneInfo
{
	// Consider the following fields as read-only !
	//
	// Currently, the only way to get the class to accept values which
	// were written to these fields is creating a derived class.
	// 
	const char* formatString;   // the name of the identified file format
	const char* speedString;    // describing the speed a song is running at
	uword loadAddr;
	uword initAddr;
	uword playAddr;
	uword startSong;
	uword songs;
	//
	// Available after song initialization.
	//
	uword irqAddr;              // if (playAddr == 0), interrupt handler was
	                            // installed and starts calling the C64 player
	                            // at this address
	uword currentSong;          // the one that has been initialized
	ubyte songSpeed;            // intended speed, see top
	ubyte clockSpeed;           // -"-
        char musPlayer;             // true = install sidplayer routine
	uword lengthInSeconds;      // --- not yet supported ---
	//
	// Song title, credits, ...
	//
	ubyte numberOfInfoStrings;  // the number of available text info lines
	char* infoString[infoStringNum];
	char* nameString;           // name, author and copyright strings
	char* authorString;         // are duplicates of infoString[?]
	char* copyrightString;
	//
	uword numberOfCommentStrings;  // --- not yet supported ---
	char ** commentString;         // -"-
	//
	udword dataFileLen;         // length of single-file sidtune or raw data
	udword c64dataLen;          // length of raw C64 data
	char* dataFileName;         // a first file: e.g. ``*.DAT''
	char* infoFileName;         // a second file: e.g. ``*.SID''
	//
	const char* statusString;   // error/status message of last operation
};


class emuEngine;
class binfile;


class sidTune
{
	
 public:  // ----------------------------------------------------------------
	
	// --- constructors ---
	
        // Load a sidtune from a binfile.
    sidTune();
    sidTune( binfile &f);
	
    virtual ~sidTune();  // destructor
	
	// --- member functions ---
	
	// Load a sidtune from a file into an existing object.
    char open(binfile &f);

    char getInfo( struct sidTuneInfo& );
    char returnInfo( struct sidTuneInfo& outSidTuneInfo )  { return getInfo(outSidTuneInfo); }
    virtual char setInfo( struct sidTuneInfo& );  // dummy

    ubyte getSongSpeed()  { return info.songSpeed; }
    ubyte returnSongSpeed()  { return getSongSpeed(); }
	
    uword getPlayAddr()     { return info.playAddr; }
    uword returnPlayAddr()  { return getPlayAddr(); }
		
    friend char sidEmuInitializeSong(emuEngine &, sidTune &, uword songNum);
	
    friend char sidEmuInitializeSongOld(emuEngine &, sidTune &,     uword songNum);
	
    operator char()  { return status; }
    char getStatus()  { return status; }
    char returnStatus()  { return getStatus(); }
	
	
 protected:  // -------------------------------------------------------------
	
        char status;
	sidTuneInfo info;
	
	ubyte songSpeed[classMaxSongs];
	uword songLength[classMaxSongs];   // song lengths in seconds
	
	// holds text info from the format headers etc.
	char infoString[infoStringNum][infoStringLen];
	
	ubyte fillUpWidth;  // fill up saved text strings up to this width

        char isCached;
	ubyte* cachePtr;
	udword cacheLen;
	
	// Using external buffers for loading files instead of the interpreter
	// memory. This separates the sidtune objects from the interpreter.
	ubyte* fileBuf; 
	ubyte* fileBuf2;
	
	udword fileOffset;  // for files with header: offset to real data
	
	// Filename extensions to append for various file types.
	const char **fileNameExtensions;
	
	// --- protected member functions ---
	
	// Convert 32-bit PSID-style speed word to internal variables.
	void convertOldStyleSpeedToTables(udword oldStyleSpeed);
	
	// Copy C64 data from internal cache to C64 memory.
        char placeSidTuneInC64mem( ubyte* c64buf );
	
        udword loadFile( binfile &f, ubyte** bufferRef);
        char saveToOpenFile( ofstream& toFile, ubyte* buffer, udword bufLen );

	// Data caching.
        char cacheRawData( void* sourceBuffer, udword sourceBufLen );
        char getCachedRawData( void* destBuffer, udword destBufLen );
	
	// Support for various file formats.
	
        virtual char PSID_fileSupport(void* buffer, udword bufLen);
        virtual char RAW_fileSupport(void* buffer, udword bufLen);


	
 private:  // ---------------------------------------------------------------
	
	// --- private member functions ---

	void safeConstructor();
	void safeDestructor();
        void filesConstructor( binfile & );   
	
	uword selectSong(uword selectedSong);
	void setIRQaddress(uword address);
	
	void deleteFileBuffers();
	// Try to retrieve single-file sidtune from specified buffer.
        char getSidtuneFromFileBuffer(ubyte* buffer, udword bufferLen);
	// Cache the data of a single-file or two-file sidtune and its
	// corresponding file names.
	void acceptSidTune(const char* dataFileName, const char* infoFileName,
					   ubyte* dataFileBuf, udword dataLen );
};
	

#endif
