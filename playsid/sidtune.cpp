// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// SIDPlay SidTune class
//
// revision history: (please note changes here)
//  -kb980717  Tammo Hinrichs <opencp@gmx.net>
//    -first release
//

// $Date: 1998/12/20 12:53:53 $
//
// Information on usage of this class in "include/sidtune.h".
//

#include "sidtune.h"
#include "fformat.h"
#include "myendian.h"
#include "binfile.h"

static const char text_songNumberExceed[] = "WARNING: Selected song number was too high";
static const char text_unrecognizedFormat[] = "ERROR: Could not determine file format";
static const char text_notEnoughMemory[] = "ERROR: Not enough free memory";
static const char text_cantLoadFile[] = "ERROR: Could not load input file";
static const char text_dataTooLong[] = "ERROR: Music data size exceeds C64 memory";
static const char text_fatalInternal[] = "FATAL: Internal error - contact the developers";
static const char text_PAL_VBI[] =  "50 Hz VBI (PAL)     ";
static const char text_PAL_CIA[] =  "CIA 1 Timer A (PAL) ";
static const char text_NTSC_VBI[] = "60 Hz VBI (NTSC)    ";
static const char text_NTSC_CIA[] = "CIA 1 Timer A (NTSC)";
static const char text_noErrors[] = "No errors";
static const char text_na[] = "N/A";

static const char *defaultFileNameExt[] =
{
        ".sid", 0
};

// ------------------------------------------------- constructors, destructor

sidTune::sidTune()
{
	safeConstructor();
}

sidTune::sidTune( binfile &f )
{
	safeConstructor();
        filesConstructor( f );
	deleteFileBuffers();
}


sidTune::~sidTune()
{
	safeDestructor();
}


// -------------------------------------------------- public member functions

char sidTune::open( binfile &f )
{
	safeDestructor();
	safeConstructor();
        filesConstructor(f);
	deleteFileBuffers();
	return status;
}

char sidTune::setInfo( sidTuneInfo &)
{
	return true;
}

char sidTune::getInfo( sidTuneInfo &outInfo )
{
	outInfo = info;
	return true;
}


// First check, whether a song is valid. Then copy any song-specific
// variable information such a speed/clock setting to the info structure.
//
// This is a private member function. It is used only by player.cpp.
uword sidTune::selectSong(uword selectedSong)
{
	// Determine and set starting song number.
	if (selectedSong == 0)
	{
		selectedSong = info.startSong;
	}
	else if ((selectedSong > info.songs) || (selectedSong > classMaxSongs))
	{
		info.statusString = text_songNumberExceed;
		selectedSong = info.startSong;
	}
	// Retrieve song speed definition.
	info.songSpeed = songSpeed[selectedSong-1];
	// Assign song speed description string depending on clock speed.
	if (info.clockSpeed == SIDTUNE_CLOCK_PAL)
	{
		if (info.songSpeed == SIDTUNE_SPEED_VBI_PAL)
		{
			info.speedString = text_PAL_VBI;
		}
		else
		{
			info.speedString = text_PAL_CIA;
		}
	}
	else  //if (info.clockSpeed == SIDTUNE_CLOCK_NTSC)
	{
		if (info.songSpeed == SIDTUNE_SPEED_VBI_NTSC)
		{
			info.speedString = text_NTSC_VBI;
		}
		else
		{
			info.speedString = text_NTSC_CIA;
		}
	}
	return (info.currentSong=selectedSong);
}

void sidTune::setIRQaddress(uword address)
{
	info.irqAddr = address;
}

// ------------------------------------------------- private member functions

char sidTune::placeSidTuneInC64mem( ubyte* c64buf )
{
	if (isCached && status)
	{
		// Check the size of the data.
		if ( info.c64dataLen > 65536 )
		{
			info.statusString = text_dataTooLong;
			return (status = false);
		}
		else
		{
			udword endPos = info.loadAddr + info.c64dataLen;
			if (endPos <= 65536)
			{
				// Copy data from cache to the correct destination.
				memcpy(c64buf+info.loadAddr,cachePtr+fileOffset,info.c64dataLen);
			}
			else
			{
				// Security - split data which would exceed the end of the C64 memory.
				// Memcpy could not detect this.
				memcpy(c64buf+info.loadAddr,cachePtr+fileOffset,info.c64dataLen-(endPos-65536));
				// Wrap the remaining data to the start address of the C64 memory.
				memcpy(c64buf,cachePtr+fileOffset+info.c64dataLen-(endPos-65536),(endPos-65536));
			}
			return (status = true);
		}
	}
	else
	{
		return (status = false);
	}
}



udword sidTune::loadFile(binfile &f, ubyte** bufferRef)
{
	udword fileLen = 0;
	status = false;
	// Open binary input file stream at end of file.

        fileLen = (udword)f.length();
        if ( *bufferRef != 0 )
          delete[] *bufferRef;  // free previously allocated memory
        *bufferRef = new ubyte[fileLen+1];
        if ( *bufferRef == 0 )
        {
          info.statusString = text_notEnoughMemory;
          fileLen = 0;  // returning 0 = error condition.
        }
        else
          *(*bufferRef+fileLen) = 0;

        f.seek(0);
        if (f.read( (ubyte*)*bufferRef, fileLen ) < fileLen)
          info.statusString = text_cantLoadFile;
        else
        {
          info.statusString = text_noErrors;
          status = true;
        }
	return fileLen;
}



void sidTune::deleteFileBuffers()
{
	// This function does not affect status and statusstring.
	// The filebuffers are global to the object.
	if ( fileBuf != 0 )
	{
		delete[] fileBuf;
		fileBuf = 0;
	}
	if ( fileBuf2 != 0 )
	{
		delete[] fileBuf2;
		fileBuf2 = 0;
	}
}


char sidTune::cacheRawData( void* sourceBuf, udword sourceBufLen )
{
	if ( cachePtr != 0 )
	{
		delete[] cachePtr;
	}
	isCached = false;
	if ( (cachePtr = new ubyte[sourceBufLen]) == 0 )
	{
		info.statusString = text_notEnoughMemory;
		return (status = false);
	}
	else
	{
		memcpy( cachePtr, (ubyte*)sourceBuf, sourceBufLen );
		cacheLen = sourceBufLen;
		isCached = true;
		info.statusString = text_noErrors;
		return (status = true);
	}
}


char sidTune::getCachedRawData( void* destBuf, udword destBufLen )
{
	if (( cachePtr == 0 ) || ( cacheLen > destBufLen ))
	{
                info.statusString = text_fatalInternal;
		return (status = false);
	}
	memcpy( (ubyte*)destBuf, cachePtr, cacheLen );
	info.dataFileLen = cacheLen;
	info.statusString = text_noErrors;
	return (status = true);
}


void sidTune::safeConstructor()
{
	// Initialize the object with some safe defaults.
	status = false;

	info.statusString = text_na;
	info.dataFileName = 0;
	info.dataFileLen = 0;
	info.infoFileName = 0;
	info.formatString = text_na;
	info.speedString = text_na;
	info.loadAddr = ( info.initAddr = ( info.playAddr = 0 ));
	info.songs = ( info.startSong = ( info.currentSong = 0 ));
	info.musPlayer = false;
	info.songSpeed = SIDTUNE_SPEED_VBI_PAL;
	info.clockSpeed = SIDTUNE_CLOCK_PAL;

	for ( int si = 0; si < classMaxSongs; si++ )
	{
		songSpeed[si] = SIDTUNE_SPEED_VBI_PAL;  // all: 50 Hz
	}

	cachePtr = 0;
	cacheLen = 0;

	fileBuf = ( fileBuf2 = 0 );
	fileOffset = 0;
	fileNameExtensions = defaultFileNameExt;

	for ( int sNum = 0; sNum < infoStringNum; sNum++ )
	{
		for ( int sPos = 0; sPos < infoStringLen; sPos++ )
		{
			infoString[sNum][sPos] = 0;
		}
	}
	info.numberOfInfoStrings = 0;

	info.numberOfCommentStrings = 1;
	info.commentString = new char * [info.numberOfCommentStrings];
	info.commentString[0] = myStrDup("--- SAVED WITH SIDPLAY V?.?? ---");

	fillUpWidth = 0;
}


void sidTune::safeDestructor()
{
	// Remove copy of comment field.
	udword strNum = 0;
	// Check and remove every available line.
	while (info.numberOfCommentStrings-- > 0)
	{
		if (info.commentString[strNum] != 0)
		{
			delete[] info.commentString[strNum];
			info.commentString[strNum] = 0;
		}
		strNum++;  // next string
	};
	delete[] info.commentString;  // free the array pointer

	if ( info.infoFileName != 0 )
		delete[] info.infoFileName;
	if ( info.dataFileName != 0 )
		delete[] info.dataFileName;
	if ( cachePtr != 0 )
		delete[] cachePtr;
	deleteFileBuffers();

	status = false;
}


char sidTune::getSidtuneFromFileBuffer( ubyte* buffer, udword bufferLen )
{
        char foundFormat = false;
	// Here test for the possible single file formats. ------------------
	if ( PSID_fileSupport( buffer, bufferLen ))
	{
		foundFormat = true;
	}
        else if ( RAW_fileSupport( buffer, bufferLen ))
        {
          foundFormat = true;
        }
        else
        {
                // No further single-file-formats available. --------------------
                info.formatString = text_na;
                info.statusString = text_unrecognizedFormat;
                status = false;
        }
	if ( foundFormat )
	{
		info.c64dataLen = bufferLen - fileOffset;
		cacheRawData( buffer, bufferLen );
		info.statusString = text_noErrors;
		status = true;
	}
	return foundFormat;
}


void sidTune::acceptSidTune(const char*, const char*, ubyte* dataBuf, udword dataLen )
{
        info.dataFileName = 0;
        info.infoFileName = 0;
	info.dataFileLen = dataLen;
	info.c64dataLen = dataLen - fileOffset;
	cacheRawData( dataBuf, dataLen );
}


// Initializing the object based upon what we find in the specified file.

void sidTune::filesConstructor( binfile &f )
{
	fileBuf = 0;  // will later point to the buffered file
	// Try to load the single specified file.
        if (( info.dataFileLen = loadFile(f,&fileBuf)) != 0 )
	{
		if ( PSID_fileSupport(fileBuf,info.dataFileLen ))
		{
                        acceptSidTune(0,0,fileBuf,info.dataFileLen);
			return;
		}
                if ( RAW_fileSupport(fileBuf,info.dataFileLen ))
		{
                        acceptSidTune(0,0,fileBuf,info.dataFileLen);
			return;
		}
	} // if loaddatafile
	else
	{
		// returned fileLen was 0 = error. The info.statusString is
		// already set then.
		info.formatString = text_na;
		status = false;
		return;
	}
}


void sidTune::convertOldStyleSpeedToTables(udword oldStyleSpeed)
{
	// Create the speed/clock setting tables.
	//
	// This does not take into account the PlaySID bug upon evaluating the
	// SPEED field. It would most likely break compatibility to lots of
	// sidtunes, which have been converted from .SID format and vice versa.
	// The .SID format did the bit-wise/song-wise evaluation of the SPEED
	// value correctly, like it was described in the PlaySID documentation.

	int toDo = ((info.songs <= classMaxSongs) ? info.songs : classMaxSongs);
	for (int s = 0; s < toDo; s++)
	{
		if (( (oldStyleSpeed>>(s&31)) & 1 ) == 0 )
		{
			songSpeed[s] = SIDTUNE_SPEED_VBI_PAL;  // 50 Hz
		}
		else
		{
			songSpeed[s] = SIDTUNE_SPEED_CIA_1A;   // CIA 1 Timer A
		}
	}
}

