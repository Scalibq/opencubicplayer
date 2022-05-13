// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIface text track/pattern display
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -fd981119   Felix Domke <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT'

#define NO_CPIFACE_IMPORT
#include <string.h>
#include <stdlib.h>
#include "poutput.h"
#include "psetting.h"
#include "cpiface.h"

#ifndef MEKKACOL

#define COLPTNOTE 0x0A
#define COLNOTE 0x0F
#define COLPITCH 0x02
#define COLSPEED 0x02
#define COLPAN 0x05
#define COLVOL 0x09
#define COLACT 0x04
#define COLINS 0x07
#define COLLNUM 0x07
#define COLHLNUM 0x04
#define COLBACK 0x08
#define COLMUTE 0x08
#define COLTITLE1 0x01
#define COLTITLE1H 0x09
#define COLTITLE2 0x07
#define COLTITLE2M 0x08
#define COLTITLE2H 0x0F
#define COLMARK 0x0F
#define COLHLAND 0xFF
#define COLHLOR 0x88

#else

#define COLPTNOTE 0xF2
#define COLNOTE 0xF0
#define COLPITCH 0xFA
#define COLSPEED 0xFA
#define COLPAN 0xFD
#define COLVOL 0xF9
#define COLACT 0xFC
#define COLINS 0xF8
#define COLLNUM 0xF8
#define COLHLNUM 0xF4
#define COLBACK 0xF7
#define COLMUTE 0xF7
#define COLTITLE1 0xF9
#define COLTITLE1H 0xF1
#define COLTITLE2 0xF8
#define COLTITLE2M 0xF7
#define COLTITLE2H 0xF0
#define COLMARK 0xF0
#define COLHLAND 0x77
#define COLHLOR 0x00

#endif

static int plTrackActive;

static int (*getcurpos)();
static int (*getpatlen)(int n);
static const char *(*getpatname)(int n);
static void (*seektrack)(int n, int c);
static int (*startrow)();
static int (*getnote)(short *bp, int small);
static int (*getins)(short *bp);
static int (*getvol)(short *bp);
static int (*getpan)(short *bp);
static void (*getfx)(short *bp, int n);
static void (*getgcmd)(short *bp, int n);

static int plPatternNum;
static short plPrepdPat;
static short plPatType;
static short plPatFirstLine;
static short plPatHeight;
static short plPatWidth;
static short plPatManualPat;
static short plPatManualRow;
static short (*plPatBuf)[132];
static short pathighlight[132];
static char pattitle1[133];
static short pattitle2[132];
static short patwidth;

enum
{
  cpiTrkFXIns=1,cpiTrkFXNote=2,cpiTrkFXVol=4,cpiTrkFXNoPan=8,
};

static void getfx2(short *bp, int n, int o)
{
  int p=0;
  if (o&cpiTrkFXIns)
    if (getins(bp+1))
    {
      writestring(bp, 0, COLINS, "i", 1);
      p++;
      bp+=3;
    }
  if (p==n)
    return;
  if (o&cpiTrkFXNote)
    if (getnote(bp, 0))
    {
      p++;
      bp+=3;
    }
  if (p==n)
    return;
  if (o&cpiTrkFXVol)
    if (getvol(bp+1))
    {
      writestring(bp, 0, COLVOL, "v", 1);
      p++;
      bp+=3;
    }
  if (p==n)
    return;
  if (!(o&cpiTrkFXNoPan))
    if (getpan(bp+1))
    {
      writestring(bp, 0, COLPAN, "p", 1);
      p++;
      bp+=3;
    }
  if (p==n)
    return;
  getfx(bp, n-p);
}

static void getscrollpos(int tr, int &firstchan, int &chnn)
{
  if (plNLChan>tr)
  {
    if (plSelCh<(tr>>1))
      firstchan=0;
    else
      if (plSelCh>=(plNLChan-(tr>>1)))
        firstchan=plNLChan-tr;
      else
        firstchan=plSelCh-((tr>>1)-1);
    chnn=tr;
  }
  else
  {
    firstchan=0;
    chnn=plNLChan;
  }
}


static void setattrgrey(short *buf, unsigned short len)
{
  short i;
  for (i=0; i<len; i++)
  {
    ((char*)buf)[1]=COLMUTE;
    buf++;
  }
}

static void preparetrack1(short *bp)
{
  getnote(bp, 2);
}

static void preparetrack2(short *bp)
{
  getnote(bp, 1);
}


static void preparetrack3(short *bp)
{
  getnote(bp, 0);
}

static void preparetrack3f(short *bp)
{
  if (!getnote(bp, 0))
    getfx2(bp, 1, cpiTrkFXVol);
}


static void preparetrack6nf(short *bp)
{
  getnote(bp, 0);
  getfx2(bp+3, 1, cpiTrkFXVol);
}

static void preparetrack17invff(short *bp)
{
  getins(bp);
  getnote(bp+3, 0);
  getvol(bp+7);
  getfx2(bp+10, 2, 0);
}

static void preparetrack14invff(short *bp)
{
  getins(bp);
  getnote(bp+2, 0);
  getvol(bp+5);
  getfx2(bp+7, 2, 0);
}

static void preparetrack14nvff(short *bp)
{
  getnote(bp, 0);
  getvol(bp+4);
  getfx2(bp+7, 2, 0);
}

static void preparetrack26invpffff(short *bp)
{
  getins(bp);
  getnote(bp+3, 0);
  getvol(bp+7);
  getpan(bp+10);
  getfx2(bp+13, 4, cpiTrkFXNoPan);
}

static void preparetrack16fffff(short *bp)
{
  getfx2(bp, 5, cpiTrkFXIns|cpiTrkFXNote|cpiTrkFXVol);
}

static void preparetrack8nvf(short *bp)
{
  getnote(bp, 0);
  getvol(bp+3);
  getfx2(bp+5, 1, 0);
}

static void preparetrack8inf(short *bp)
{
  getins(bp);
  getnote(bp+2, 0);
  getfx2(bp+5, 1, cpiTrkFXVol);
}

struct patviewtype
{
  unsigned char maxch;
  unsigned char gcmd;
  unsigned char width;
  const char *title;
  const char *paused;
  const char *normal;
  const char *selected;
  void (*putcmd)(short *bp);
};

static void preparepatgen(int pat, const patviewtype &pt)
{
  int i;
  plPrepdPat=pat;
  int plen=getpatlen(pat);

  int firstchan,chnn;
  getscrollpos(pt.maxch, firstchan, chnn);

  strcpy(pattitle1, "   pattern view:  order ");
  convnum(pat, pattitle1+strlen(pattitle1), 16, 3, 0);
  strcat(pattitle1, ", ");
  convnum(pt.maxch, pattitle1+strlen(pattitle1), 10, 2, 1);
  strcat(pattitle1, " channels,  ");
  strcat(pattitle1, pt.title);
  const char *pname=getpatname(pat);
  if (pname&&*pname)
  {
    strcat(pattitle1, ": ");
    strcat(pattitle1, pname);
  }

  short p0=4+4*pt.gcmd;
  patwidth=p0+pt.width*chnn+4;
  writestring(pattitle2, 0, COLTITLE2, "row", 132);
  if (patwidth<=132)
    writestring(pattitle2, patwidth-3, COLTITLE2, "row", 3);
  switch (pt.gcmd)
  {
  case 0:
    break;
  case 1:
    writestring(pattitle2, 4, plPause?COLTITLE2M:COLTITLE2, "gbl", 3);
    break;
  case 2:
    writestring(pattitle2, 5, plPause?COLTITLE2M:COLTITLE2, "global", 6);
    break;
  case 3:
    writestring(pattitle2, 5, plPause?COLTITLE2M:COLTITLE2, "global cmd", 10);
    break;
  default:
    writestring(pattitle2, pt.gcmd*2-4, plPause?COLTITLE2M:COLTITLE2, "global commands", 15);
    break;
  }

  short patmask[132];

  writestring(patmask, 0, COLLNUM, "00 ", 132);
  if (patwidth<=132)
    writestring(patmask, patwidth-3, COLLNUM, "00", 2);
  writestring(patmask, 3, COLBACK, "÷", 1);
  writestring(patmask, 3+pt.gcmd*4, COLBACK, "÷", 1);
  if (!plPause)
    for (i=0; i<pt.gcmd; i++)
      writestring(patmask, 4+4*i, COLBACK, "תתת", 3);

  short n0=p0+(pt.width+1)/2-1;
  if (pt.width==4)
    n0--;
  for (i=0; i<chnn; i++)
  {
    char chpaus=plMuteCh[i+firstchan];
    char sel=((i+firstchan)==plSelCh);
    writenum(pattitle2, n0+pt.width*i, sel?COLTITLE2H:chpaus?COLTITLE2M:COLLNUM, i+firstchan+1, 10, (pt.width==1)?1:2, pt.width>2);
    writestring(patmask, p0+pt.width*i, COLBACK, chpaus?pt.paused:sel?pt.selected:pt.normal, pt.width);
  }

  int firstrow=20;
  int firstprow=0;
  int firstpat=pat;

  while (firstpat)
  {
    firstpat--;
    firstrow-=getpatlen(firstpat);
    if (firstrow<=0)
    {
      firstprow-=firstrow;
      firstrow=0;
      break;
    }
  }
  for (i=0; i<firstrow; i++)
    writestring(plPatBuf[i], 0, COLBACK, "", 132);

  while (firstpat<plPatternNum)
  {
    if (!getpatlen(firstpat))
    {
      firstpat++;
      continue;
    }
    int curlen=getpatlen(firstpat);
    int lastprow=curlen;
    if ((firstrow+curlen-firstprow)>(plen+60))
      lastprow=plen+60-firstrow+firstprow;

    for (i=firstprow; i<lastprow; i++)
    {
      writestringattr(plPatBuf[i+firstrow-firstprow], 0, patmask, 132);
      writenum(plPatBuf[i+firstrow-firstprow], 0, i?COLLNUM:COLHLNUM, i, 16, 2, 0);
      if (patwidth<=132)
        writenum(plPatBuf[i+firstrow-firstprow], patwidth-3, i?COLLNUM:COLHLNUM, i, 16, 2, 0);
    }

    if (pt.gcmd)
    {
      seektrack(firstpat, -1);
      while (1)
      {
        int currow=startrow();
        if (currow==-1)
          break;
        if ((currow>=firstprow)&&(currow<lastprow))
        {
          short *bp=plPatBuf[currow+firstrow-firstprow]+4;
          getgcmd(bp, pt.gcmd);
          if (plPause)
            setattrgrey(bp, pt.gcmd*4);
        }
      }
    }

    for (i=0; i<chnn; i++)
    {
      seektrack(firstpat, i+firstchan);
      int chpaus=plMuteCh[i+firstchan];
      while (1)
      {
        int currow=startrow();
        if (currow==-1)
          break;
        if ((currow>=firstprow)&&(currow<lastprow))
        {
          short *bp=plPatBuf[currow+firstrow-firstprow]+p0+i*pt.width;
          pt.putcmd(bp);
          if (chpaus)
            setattrgrey(bp, pt.width);
        }
      }
    }

    firstrow+=lastprow-firstprow;
    firstprow=0;
    firstpat++;

    if (firstrow==(plen+60))
      break;
  }

  for (i=firstrow; i<(plen+60); i++)
    writestring(plPatBuf[i], 0, COLBACK, "", 132);
}

static patviewtype pat6480={64,3,1,"(notes only)"," ","ת","ש",preparetrack1};
static patviewtype pat64132={64,15,1,"(notes only)"," ","ת","ש",preparetrack1};
static patviewtype pat64132m={64,0,2,"(notes only)","  ","תת","שש",preparetrack2};
static patviewtype pat4880={48,6,1,"(notes only)"," ","ת","ש",preparetrack1};
static patviewtype pat48132={48,7,2,"(notes only)","  ","תת","שש",preparetrack2};
static patviewtype pat3280={32,3,2,"(notes only)","  ","תת","שש",preparetrack2};
static patviewtype pat32132={32,7,3,"(notes only)","   ","תתת","ששש",preparetrack3};
static patviewtype pat32132f={32,7,3,"(notes & fx)","   ","תתת","ששש",preparetrack3f};
static patviewtype pat2480={24,1,3,"(notes only)","   ","תתת","ששש",preparetrack3};
static patviewtype pat2480f={24,1,3,"(notes & fx)","   ","תתת","ששש",preparetrack3f};
static patviewtype pat24132={24,7,4,"(notes only)","   ³","תתת³","ששש³",preparetrack3};
static patviewtype pat24132f={24,7,4,"(notes & fx)","   ³","תתת³","ששש³",preparetrack3f};
static patviewtype pat1680={16,3,4,"(notes only)","   ³","תתת³","ששש³",preparetrack3};
static patviewtype pat1680f={16,3,4,"(notes & fx)","   ³","תתת³","ששש³",preparetrack3f};
static patviewtype pat16132={16,3,7,"(note, fx)","      ³","תתת   ³","שששתתת³",preparetrack6nf};
static patviewtype pat880={8,1,9,"(ins, note, fx)","        ³","  תתת   ³","תתשששתתת³",preparetrack8inf};
static patviewtype pat880f={8,1,9,"(note, vol, fx)","        ³","תתת  תתת³","שששתתששש³",preparetrack8nvf};
static patviewtype pat8132={8,3,14,"(note, vol, fx, fx)","             ³","תתת תת תתתתתת³","ששש שש שששששש³",preparetrack14nvff};
static patviewtype pat8132f={8,3,14,"(ins, note, vol, fx, fx)","             ³","  תתת  תתתתתת³","תתשששתתשששששש³",preparetrack14invff};
static patviewtype pat480={4,2,17,"(ins, note, vol, fx, fx)","                ³","תת תתת תת תתתתתת³","שש ששש שש שששששש³",preparetrack17invff};
static patviewtype pat480f={4,3,16,"(fx, fx, fx, fx, fx)","               ³","ששששששששששששששש³","תתתתתתתתתתתתתתת³", preparetrack16fffff};
static patviewtype pat4132={4,5,26,"(ins, note, vol, pan, fx, fx, fx, fx)","                         ³","תת תתת תת תת תתתתתתתתתתתת³","שש ששש שש שש שששששששששששש³",preparetrack26invpffff};

static void gmdDrawPattern(char sel)
{
  int pos=getcurpos();
  short pat=pos>>8;
  int currow=pos&0xFF;
  int curpat=pat;
  short crow=currow;
  if (plPatManualPat!=-1)
  {
    pat=plPatManualPat;
    crow=plPatManualRow;
  }
  while (!getpatlen(pat))
  {
    pat++;
    crow=0;
    if (pat>=plPatternNum)
      pat=0;
  }

  if ((plPrepdPat!=pat)||plChanChanged)
    if (plPatWidth==80)
      switch (plPatType)
      {
        case 0: preparepatgen(pat, pat6480); break;
        case 1: preparepatgen(pat, pat6480); break;
        case 2: preparepatgen(pat, pat4880); break;
        case 3: preparepatgen(pat, pat4880); break;
        case 4: preparepatgen(pat, pat3280); break;
        case 5: preparepatgen(pat, pat3280); break;
        case 6: preparepatgen(pat, pat2480); break;
        case 7: preparepatgen(pat, pat2480f); break;
        case 8: preparepatgen(pat, pat1680); break;
        case 9: preparepatgen(pat, pat1680f); break;
        case 10: preparepatgen(pat, pat880); break;
        case 11: preparepatgen(pat, pat880f); break;
        case 12: preparepatgen(pat, pat480f); break;
        case 13: preparepatgen(pat, pat480); break;
      }
    else
      switch (plPatType)
      {
        case 0: preparepatgen(pat, pat64132m); break;
        case 1: preparepatgen(pat, pat64132); break;
        case 2: preparepatgen(pat, pat48132); break;
        case 3: preparepatgen(pat, pat48132); break;
        case 4: preparepatgen(pat, pat32132); break;
        case 5: preparepatgen(pat, pat32132f); break;
        case 6: preparepatgen(pat, pat24132); break;
        case 7: preparepatgen(pat, pat24132f); break;
        case 8: preparepatgen(pat, pat16132); break;
        case 9: preparepatgen(pat, pat16132); break;
        case 10: preparepatgen(pat, pat8132f); break;
        case 11: preparepatgen(pat, pat8132); break;
        case 12: preparepatgen(pat, pat4132); break;
        case 13: preparepatgen(pat, pat4132); break;
      }

  displaystr(plPatFirstLine-2, 0, sel?COLTITLE1H:COLTITLE1, pattitle1, plPatWidth);
  displaystrattr(plPatFirstLine-1, 0, pattitle2, plPatWidth);
  int i,j,row;
  int plen=getpatlen(pat);
  for (i=0, row=crow-plPatHeight/3; i<plPatHeight; i++, row++)
  {
    if ((row!=crow)&&((plPatManualPat==-1)||(row!=currow)||(pat!=curpat)))
    {
      displaystrattr(plPatFirstLine+i, 0, plPatBuf[20+row], plPatWidth);
      continue;
    }
    writestringattr(pathighlight, 0, plPatBuf[20+row], plPatWidth);
    if ((row==currow)&&(pat==curpat))
    {
      writestring(pathighlight, 2, COLMARK, "\x10", 1);
      if (patwidth<=132)
        writestring(pathighlight, patwidth-4, COLMARK, "\x11", 1);
    }
    if (row==crow)
      for (j=0; j<((patwidth<132)?patwidth:132); j++)
        pathighlight[j]=(pathighlight[j]|(COLHLOR<<8))&((COLHLAND<<8)|0xFF);
    displaystrattr(plPatFirstLine+i, 0, pathighlight, plPatWidth);
  }
}

static int gmdTrkProcessKey(unsigned short key)
{
  switch (key)
  {
  case 0x4900: //pgup
    if (plPatManualPat==-1)
    {
      if (plPatType>1)
      {
        plPatType-=2;
        plPrepdPat=-1;
      }
    }
    else
    {
      plPatManualRow-=8;
      if (plPatManualRow<0)
      {
        plPatManualPat--;
        if (plPatManualPat<0)
          plPatManualPat=plPatternNum-1;
        while (!getpatlen(plPatManualPat))
          plPatManualPat--;
        plPatManualRow=getpatlen(plPatManualPat)-1;
      }
    }
    break;
  case 0x5100: //pgdn
    if (plPatManualPat==-1)
    {
      if (plPatType<12)
      {
        plPatType+=2;
        plPrepdPat=-1;
      }
    }
    else
    {
      plPatManualRow+=8;
      if (plPatManualRow>=getpatlen(plPatManualPat))
      {
        plPatManualPat++;
        while ((plPatManualPat<plPatternNum)&&!getpatlen(plPatManualPat))
          plPatManualPat++;
        if (plPatManualPat>=plPatternNum)
          plPatManualPat=0;
        plPatManualRow=0;
      }
    }
    break;
/*
  case 0x8400: //ctrl-pgup
    if (plInstType)
      plInstScroll--;
    break;
  case 0x7600: //ctrl-pgdn
    if (plInstType)
      plInstScroll++;
    break;
*/
  case 0x9700: //alt-home
  case 0x4700: //home
    plPatType=(plNLChan<=4)?13:(plNLChan<=8)?11:(plNLChan<=16)?9:(plNLChan<=24)?7:(plNLChan<=32)?5:(plNLChan<=48)?3:1;
//    winrecalc();
    break;
  case 9: // tab
    if (plPatManualPat==-1)
    {
      plPatType^=1;
      plPrepdPat=-1;
    }
    else
    {
      if (plPatType<13)
      {
        plPatType++;
        plPrepdPat=-1;
      }
    }
    break;
  case 0x0F00: // shift-tab
  case 0xA500:
    if (plPatManualPat==-1)
    {
      plPatType^=1;
      plPrepdPat=-1;
    }
    else
    {
      if (plPatType)
      {
        plPatType--;
        plPrepdPat=-1;
      }
    }
    break;
  case ' ':
    if (plPatManualPat==-1)
    {
      int p=getcurpos();
      plPatManualPat=p>>8;
      plPatManualRow=p&0xFF;
    }
    else
      plPatManualPat=-1;
    break;
  default:
    return 0;
  }
  return 1;
}

static void TrakSetWin(int, int wid, int ypos, int hgt)
{
  plPatFirstLine=ypos+2;
  plPatHeight=hgt-2;
  plPatWidth=wid;
}

static int TrakGetWin(cpitextmodequerystruct &q)
{
  if (!plTrackActive)
    return 0;

  q.hgtmin=3;
  q.hgtmax=100;
  q.xmode=1;
  q.size=2;
  q.top=0;
  q.killprio=64;
  q.viewprio=160;
  return 1;
}

static void TrakDraw(int focus)
{
  gmdDrawPattern(focus);
}

static int TrakIProcessKey(unsigned short key)
{
  switch (key)
  {
  case 't': case 'T':
    cpiTextSetMode("trak");
    break;
  case 'x': case 'X':
    plTrackActive=1;
    return 0;
  case 0x2d00: //alt-x
    plTrackActive=0;
    return 0;
  default:
    return 0;
  }
  return 1;
}

static int TrakAProcessKey(unsigned short key)
{
  switch (key)
  {
  case 't': case 'T':
    plTrackActive=!plTrackActive;
    cpiTextRecalc();
    break;
  default:
    return gmdTrkProcessKey(key);
  }
  return 1;
}

static int trkEvent(int ev)
{
  switch (ev)
  {
  case cpievInitAll:
    plTrackActive=cfGetProfileBool2(cfScreenSec, "screen", "pattern", 1, 1);
    return 0;
  case cpievInit:
    plPatBuf=new short [256+60][132];
    if (!plPatBuf)
      return 0;
    break;
  case cpievDone:
    delete plPatBuf;
    break;
  }
  return 1;
}

extern "C"
{
  cpitextmoderegstruct cpiTModeTrack = {"trak", TrakGetWin, TrakSetWin, TrakDraw, TrakIProcessKey, TrakAProcessKey, trkEvent};
}

void cpiTrkSetup(const cpitrakdisplaystruct &c, int npat)
{
  plPatternNum=npat;
  plPatManualPat=-1;
  plPrepdPat=-1;
  plPatType=(plNLChan<=4)?13:(plNLChan<=8)?11:(plNLChan<=16)?9:(plNLChan<=24)?7:(plNLChan<=32)?5:(plNLChan<=48)?3:1;
  getcurpos=c.getcurpos;
  getpatlen=c.getpatlen;
  getpatname=c.getpatname;
  seektrack=c.seektrack;
  startrow=c.startrow;
  getnote=c.getnote;
  getins=c.getins;
  getvol=c.getvol;
  getpan=c.getpan;
  getfx=c.getfx;
  getgcmd=c.getgcmd;
  cpiTextRegisterMode(&cpiTModeTrack);
}
