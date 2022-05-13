// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// SIDPlay RAW file loader
//
// revision history: (please note changes here)
//  -kb980717  Tammo Hinrichs <opencp@gmx.net>
//    -first release

//
// 1997/05/11 11:29:06
//

#include "raw_.h"

const char text_rawformat[] = "RAW sidplay file";

char sidTune::RAW_fileSupport( void* buffer, udword bufLen )
{
	// Remove any format description or format error string.
	info.formatString = 0;

        // Require a first minimum size (2 bytes load address, 6 bytes jumps,
        //                               1 "RTS" command ;)
        if (bufLen < 9)
	{
		return false;
	}

        // now check if file starts at $xx00 and has two JMP commands at
        // it's start

        ubyte *bytebuf = (unsigned char *)buffer;

        if (!(bytebuf[0]==0x00       &     // starts at $xx00
              bytebuf[1]>=0x03       &     // where xx is >=3
              bytebuf[2]==0x4c       &     // first byte JMP opcode
              bytebuf[4]>=bytebuf[1] &     // jump points forwards
              bytebuf[5]==0x4c       &     // fourth byte JMP opcode
              bytebuf[7]>=bytebuf[1] ))    // jump points forwards
        {
                return false;
        }

        uword loadaddr = bytebuf[1]<<8|bytebuf[0];

        fileOffset = 0;
        info.loadAddr = loadaddr-2;
        info.initAddr = loadaddr;
        info.playAddr = loadaddr+3;
        info.songs = 1;
        info.startSong = 1;

	if (info.songs > classMaxSongs)
	{
		info.songs = classMaxSongs;
	}

	// Create the speed/clock setting table.
        convertOldStyleSpeedToTables(0);


        // try to get song info from $xx20 (used in MANY newer
        // music routines)
        char snginfo[33];
        snginfo[32]=0;
        memcpy(snginfo,(char *)buffer+0x22,0x20);
        for (ubyte i=0; i<0x20; i++)
        {
          if (snginfo[i] && snginfo[i]<27)
            snginfo[i]|=0x40;
          if (snginfo[i]>=0x60)
            snginfo[i]=0;
        }
        if (strlen(snginfo)<6)
          strcpy(snginfo,"raw SID file");

	// Copy info strings, so they will not get lost.

        strcpy( &infoString[0][0], snginfo);
	info.nameString = &infoString[0][0];
	info.infoString[0] = &infoString[0][0];
        strcpy( &infoString[1][0], "");
	info.authorString = &infoString[1][0];
	info.infoString[1] = &infoString[1][0];
        strcpy( &infoString[2][0], "" );
	info.copyrightString = &infoString[2][0];
	info.infoString[2] = &infoString[2][0];
	info.numberOfInfoStrings = 3;

        info.formatString = text_rawformat;
	return true;
}

