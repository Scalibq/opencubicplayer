// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// SIDPlay file type detection routines for the fileselector
//
// revision history: (please note changes here)
//  -kb980717  Tammo Hinrichs <opencp@gmx.net>
//    -first release


#include <string.h>
#include "pfilesel.h"

struct psidHeader
{
	//
	// All values in big-endian order.
	//
	char id[4];          // 'PSID'
        char version[2];    // 0x0001 or 0x0002
        char data[2];       // 16-bit offset to binary data in file
        char load[2];       // 16-bit C64 address to load file to
        char init[2];       // 16-bit C64 address of init subroutine
        char play[2];       // 16-bit C64 address of play subroutine
        char songs[2];      // number of songs
        char start[2];      // start song (1-256 !)
        char speed[4];      // 32-bit speed info
		                 // bit: 0=50 Hz, 1=CIA 1 Timer A (default: 60 Hz)
        char name[32];       // ASCII strings, 31 characters long and
	char author[32];     // terminated by a trailing zero
	char copyright[32];  //
        char flags[2];      // only version 0x0002
        char reserved[4];   // only version 0x0002
};



static int sidReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int len)
{
  if (len<sizeof(psidHeader)+2)
  {
    m.modtype=mtUnRead;
    return 0;
  }
  psidHeader *ph = (psidHeader *) buf;
  int i;

  if (!memcmp(ph->id,"PSID",4))
  {
    m.modtype=mtSID;
    m.channels=ph->songs[1];
    strcpy(m.modname, ph->name);
    strcpy(m.composer, ph->author);
    strcpy(m.comment, "(C) ");
    strcat(m.comment, ph->copyright);
    return 1;
  }

  if (buf[0]==0x00 & buf[1]>=0x03 & buf[2]==0x4c &
      buf[4]>=buf[1] & buf[5]==0x4c & buf[7]>=buf[1] )
  {
    m.modtype=mtSID;
    m.channels=1;

    char snginfo[33];
    snginfo[32]=0;
    memcpy(snginfo,buf+0x22,0x20);
    for (i=0; i<0x20; i++)
    {
      if (snginfo[i] && snginfo[i]<27)
        snginfo[i]|=0x40;
      if (snginfo[i]>=0x60)
        snginfo[i]=0;
    }
    if (strlen(snginfo)<6)
      strcpy(snginfo,"raw SID file");

    strcpy(m.modname, snginfo);
    return 1;
  }

  if (!memcmp(buf,"SIDPLAY INFOFILE",16) && (((char *)buf)[16]==0x0a || ((char *)buf)[16]==0x0d))
  {
    strcpy(m.modname, "SIDPlay info file");
    m.modtype=0xFF;
    return 0;
  }

  m.modtype=0xFF;
  return 0;
}


static int sidReadInfo(moduleinfostruct &, binfile &, const unsigned char *, int)
{
  return 0;
}

extern "C"
{
  mdbreadnforegstruct sidReadInfoReg = {sidReadMemInfo, sidReadInfo};
};