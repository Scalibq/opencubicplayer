// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// MPPlay file type detection routines for fileselector
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -ryg990615  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
//    -99% faked VBR detection.. and I still don't really know why it
//     works, but... who cares?

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include "imsrtns.h"
#include "pfilesel.h"
#include "binfile.h"

#define VBRFRAMES      15

static int freqtab[3][3]={
  {44100, 48000, 32000}, {22050, 24000, 16000}, {11025, 12000, 8000}
};

static int ampegpReadMemInfo(moduleinfostruct &m, const unsigned char *buf, int len)
{
  unsigned char const *bufend=buf+len;

  if ((*(unsigned long*)buf==0x46464952)&&(*(unsigned long*)(buf+8)==0x45564157)&&(*(unsigned long*)(buf+12)==0x20746D66)&&(*(unsigned short*)(buf+20)==0x55))
  {
    int i;
    i=20;
    while (i<800)
    {
      if (*(unsigned long*)(buf+i-8)==0x61746164)
        break;
      i+=8+*(unsigned long*)(buf+i-4);
    }
    if (i>=800)
      return 0;
    buf+=i;
  }

  while ((*(long*)buf&0xE0FF)!=0xE0FF)
  {
    buf+=1;
    if (buf>=bufend)
      return 0;
  }

  unsigned long hdr=*(long*)buf;
  int layer=4-((hdr>>9)&3);
  if (layer==4)
    return 0;
  int ver=((hdr>>11)&1)?0:1;
  if (!((hdr>>12)&1))
    if (ver)
      ver=2;
    else
      return 0;
  if ((ver==2)&&(layer!=3))
    return 0;
  int rateidx=(hdr>>20)&15;
  int frqidx=(hdr>>18)&3;
  int padding=(hdr>>17)&1;
  int stereo="\x01\x01\x02\x00"[(hdr>>30)&3];
  if (frqidx==3)
    return 0;
  int rate;
  if (!ver)
    switch (layer)
    {
    case 1: rate="\x00\x04\x08\x0C\x10\x14\x18\x1C\x20\x24\x28\x2C\x30\x34\x38\x00"[rateidx]*8; break;
    case 2: rate="\x00\x04\x06\x07\x08\x0A\x0C\x0E\x10\x14\x18\x1C\x20\x28\x30\x00"[rateidx]*8; break;
    case 3: rate="\x00\x04\x05\x06\x07\x08\x0A\x0C\x0E\x10\x14\x18\x1C\x20\x28\x00"[rateidx]*8; break;
    default: return 0;
    }
  else
    switch (layer)
    {
    case 1: rate="\x00\x04\x06\x07\x08\x0A\x0C\x0E\x10\x12\x14\x16\x18\x1C\x20\x00"[rateidx]*8; break;
    case 2: rate="\x00\x01\x02\x03\x04\x05\x06\x07\x08\x0A\x0C\x0E\x10\x12\x14\x00"[rateidx]*8; break;
    case 3: rate="\x00\x01\x02\x03\x04\x05\x06\x07\x08\x0A\x0C\x0E\x10\x12\x14\x00"[rateidx]*8; break;
    default: return 0;
    }

  if (!rate)
    return 0;

  *m.modname=0;
  switch (layer)
  {
  case 1: strcat(m.modname, "Layer   I, "); break;
  case 2: strcat(m.modname, "Layer  II, "); break;
  case 3: strcat(m.modname, "Layer III, "); break;
  }
  switch (ver)
  {
  case 0:
    switch (frqidx)
    {
    case 0: strcat(m.modname, "44100 Hz, "); break;
    case 1: strcat(m.modname, "48000 Hz, "); break;
    case 2: strcat(m.modname, "32000 Hz, "); break;
    }
    break;
  case 1:
    switch (frqidx)
    {
    case 0: strcat(m.modname, "22050 Hz, "); break;
    case 1: strcat(m.modname, "24000 Hz, "); break;
    case 2: strcat(m.modname, "16000 Hz, "); break;
    }
    break;
  case 2:
    switch (frqidx)
    {
    case 0: strcat(m.modname, "11025 Hz, "); break;
    case 1: strcat(m.modname, "12000 Hz, "); break;
    case 2: strcat(m.modname, " 8000 Hz, "); break;
    }
    break;
  }

  int br=rate, lastbr=rate;

  for (int temp=0; temp<VBRFRAMES; temp++)
  {
    int skip;

    switch (layer)
    {
      case 1: skip=umuldiv(br, 12000, freqtab[ver][frqidx])+(padding<<2);
      case 2: skip=umuldiv(br, 144000, freqtab[ver][frqidx])+padding;
      case 3: skip=umuldiv(br, 144000, freqtab[ver][frqidx])+padding;
    }

    buf+=skip;

    while ((*(long*)buf&0xE0FF)!=0xE0FF)
    {
      buf+=1;
      if (buf>=bufend)
        break;
    }

    unsigned long hdr=*(long*)buf;
    layer=4-((hdr>>9)&3);
    if (layer==4)
      break;
    ver=((hdr>>11)&1)?0:1;
    if (!((hdr>>12)&1))
      if (ver)
        ver=2;
      else
        break;
    if ((ver==2)&&(layer!=3))
      break;
    frqidx=(hdr>>18)&3;
    padding=(hdr>>17)&1;
    stereo="\x01\x01\x02\x00"[(hdr>>30)&3];
    if (frqidx==3)
      break;

    lastbr=br;
    br=(hdr>>20)&15;

    if (!ver)
      switch (layer)
      {
      case 1: br="\x00\x04\x08\x0C\x10\x14\x18\x1C\x20\x24\x28\x2C\x30\x34\x38\x00"[br]*8; break;
      case 2: br="\x00\x04\x06\x07\x08\x0A\x0C\x0E\x10\x14\x18\x1C\x20\x28\x30\x00"[br]*8; break;
      case 3: br="\x00\x04\x05\x06\x07\x08\x0A\x0C\x0E\x10\x14\x18\x1C\x20\x28\x00"[br]*8; break;
      }
    else
      switch (layer)
      {
      case 1: br="\x00\x04\x06\x07\x08\x0A\x0C\x0E\x10\x12\x14\x16\x18\x1C\x20\x00"[br]*8; break;
      case 2: br="\x00\x01\x02\x03\x04\x05\x06\x07\x08\x0A\x0C\x0E\x10\x12\x14\x00"[br]*8; break;
      case 3: br="\x00\x01\x02\x03\x04\x05\x06\x07\x08\x0A\x0C\x0E\x10\x12\x14\x00"[br]*8; break;
      }

    if (lastbr!=br)
      break;
  }

  if (lastbr==br)
  {
    if (rate<100)
      strcat(m.modname, " ");
    if (rate<10)
      strcat(m.modname, " ");
    ultoa(rate, m.modname+strlen(m.modname), 10);
    strcat(m.modname, " kbps");
  }
  else
    strcat(m.modname, "VBR");

  m.channels=stereo?2:1;
  m.playtime=m.size/(rate*125);
//  m.modtype=mtMPx;
  return 0;
}

static char *mpstyles[]=
{
  "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge",
  "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B",
  "Rap", "Reggae", "Rock", "Techno", "Industrial", "Alternative", "Ska",
  "Death Metal", "Pranks", "SoundTrack", "Euro-Techno", "Ambient", "Trip-Hop",
  "Vocal", "Jazz Funk", "Fusion", "Trance", "Classical", "Instrumental",
  "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "Altern Rock",
  "Bass", "Soul"
};

static int ampegpReadInfo(moduleinfostruct &m, binfile &f, const unsigned char *buf, int len)
{
  unsigned char const *bufend=buf+len;

  if ((*(unsigned long*)buf==0x46464952)&&(*(unsigned long*)(buf+8)==0x45564157)&&(*(unsigned long*)(buf+12)==0x20746D66)&&(*(unsigned short*)(buf+20)==0x55))
  {
    int i;
    i=20;
    while (i<800)
    {
      if (*(unsigned long*)(buf+i-8)==0x61746164)
        break;
      i+=8+*(unsigned long*)(buf+i-4);
    }
    if (i>=800)
      return 0;
    buf+=i;
  }

  while ((*(long*)buf&0xE0FF)!=0xE0FF)
  {
    buf+=1;
    if (buf>=bufend)
      return 0;
  }

  unsigned long hdr=*(long*)buf;
  int layer=4-((hdr>>9)&3);
  if (layer==4)
    return 0;
  int ver=((hdr>>11)&1)?0:1;
  if (!((hdr>>12)&1))
    if (ver)
      ver=2;
    else
      return 0;
  if ((ver==2)&&(layer!=3))
    return 0;
  int rateidx=(hdr>>20)&15;
  int frqidx=(hdr>>18)&3;
  int stereo="\x01\x01\x02\x00"[(hdr>>30)&3];
  if (frqidx==3)
    return 0;
  int rate;
  if (!ver)
    switch (layer)
    {
    case 1: rate="\x00\x04\x08\x0C\x10\x14\x18\x1C\x20\x24\x28\x2C\x30\x34\x38\x00"[rateidx]*8; break;
    case 2: rate="\x00\x04\x06\x07\x08\x0A\x0C\x0E\x10\x14\x18\x1C\x20\x28\x30\x00"[rateidx]*8; break;
    case 3: rate="\x00\x04\x05\x06\x07\x08\x0A\x0C\x0E\x10\x14\x18\x1C\x20\x28\x00"[rateidx]*8; break;
    default: return 0;
    }
  else
    switch (layer)
    {
    case 1: rate="\x00\x04\x06\x07\x08\x0A\x0C\x0E\x10\x12\x14\x16\x18\x1C\x20\x00"[rateidx]*8; break;
    case 2: rate="\x00\x01\x02\x03\x04\x05\x06\x07\x08\x0A\x0C\x0E\x10\x12\x14\x00"[rateidx]*8; break;
    case 3: rate="\x00\x01\x02\x03\x04\x05\x06\x07\x08\x0A\x0C\x0E\x10\x12\x14\x00"[rateidx]*8; break;
    default: return 0;
    }

  if (!rate)
    return 0;

  m.modtype=mtMPx;
  unsigned char tag[128];

  for (int cpass=0; cpass<2; cpass++)
  {
    if (cpass)
      f.seekend(-128);
    else
      f.seek(0);

    f.read(tag, 128);
    if (!memcmp(tag, "TAG", 3))
    {
      if (memcmp(tag+3, "                              ", 30))
      {
        memcpy(m.modname, tag+3, 30);
        m.modname[30]=0;
        while (strlen(m.modname)&&(m.modname[strlen(m.modname)-1]==' '))
          m.modname[strlen(m.modname)-1]=0;
      }
      if (memcmp(tag+33, "                              ", 30))
      {
        memcpy(m.composer, tag+33, 30);
        m.composer[30]=0;
        while (strlen(m.composer)&&(m.composer[strlen(m.composer)-1]==' '))
          m.composer[strlen(m.composer)-1]=0;
      }
      if (memcmp(tag+63, "                              ", 30)||memcmp(tag+97, "                              ", 30))
      {
        memcpy(m.comment, tag+63, 30);
        m.comment[30]=' ';
        m.comment[31]=' ';
        memcpy(m.comment+32, tag+97, 30);
        m.comment[62]=0;
        while (strlen(m.comment)&&(m.comment[strlen(m.comment)-1]==' '))
          m.comment[strlen(m.comment)-1]=0;
      }
      if (tag[127]<43)
        strcpy(m.style, mpstyles[tag[127]]);
      if (memcmp(tag+93, "    ", 4))
      {
        memcpy(tag, tag+93, 4);
        tag[4]=0;
        m.date=atoi((char*)tag)<<16;
      }
      break;
    }
  }

  return 1;
}

extern "C"
{
  mdbreadnforegstruct ampegpReadInfoReg = {ampegpReadMemInfo, ampegpReadInfo};
};
