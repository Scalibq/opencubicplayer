// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// XMPlay instrument display routines
//
// revision history: (please note changes here)
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -removed all references to gmd structures to make this more flexible
//     therefore I had to add this file and search/replace some identifiers

#include <string.h>
#include "mcp.h"
#include "xmplay.h"
#include "poutput.h"
#include "pfilesel.h"
#include "cpiface.h"

static int instnum;
static int sampnum;
static char *plInstUsed;
static char *plSampUsed;
static unsigned char *plBigInstNum;
static unsigned short *plBigSampNum;

extern char plNoteStr[132][4];

static const xmpinstrument *plInstr;
static const sampleinfo *plSamples;
static const xmpsample *plModSamples;

static char plInstShowFreq;

static void xmpDisplayIns40(short *buf, int n, int plInstMode)
{
  char col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[n]];
  writestring(buf, 0, col, (!plInstMode&&plInstUsed[n])?"þ##: ":" ##: ", 5);
  writenum(buf, 1, col, n+1, 16, 2, 0);
  writestring(buf, 5, col, plInstr[n].name, 35);
}

static void xmpDisplayIns33(short *buf, int n, int plInstMode)
{
  char col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[n]];
  writestring(buf, 0, col, (!plInstMode&&plInstUsed[n])?"þ##: ":" ##: ", 5);
  writenum(buf, 1, col, n+1, 16, 2, 0);
  writestring(buf, 5, col, plInstr[n].name, 28);
}

static void xmpDisplayIns52(short *buf, int n, int plInstMode)
{
  char col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[n]];
  writestring(buf, 0, col, (!plInstMode&&plInstUsed[n])?"    þ##: ":"     ##: ", 9);
  writenum(buf, 5, col, n+1, 16, 2, 0);
  writestring(buf, 9, col, plInstr[n].name, 43);
}

static void xmpDisplayIns80(short *buf, int n, int plInstMode)
{
  char col;
  writestring(buf, 0, 0, "", 80);

  if (plBigInstNum[n]!=0xFF)
  {
    const xmpinstrument &ins=plInstr[plBigInstNum[n]];
    col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[plBigInstNum[n]]];
    writestring(buf, 0, col, (!plInstMode&&plInstUsed[plBigInstNum[n]])?"þ##: ":" ##: ", 5);
    writenum(buf, 1, col, plBigInstNum[n]+1, 16, 2, 0);
    writestring(buf, 5, col, ins.name, 31);
  }

  if (plBigSampNum[n]!=0xFFFF)
  {
    const xmpsample &sm=plModSamples[plBigSampNum[n]];
    col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plSampUsed[plBigSampNum[n]]];
    writestring(buf, 34, col, (!plInstMode&&plSampUsed[plBigSampNum[n]])?"þ###: ":" ###: ", 6);
    writenum(buf, 35, col, plBigSampNum[n], 16, 3, 0);
    const sampleinfo &si=plSamples[sm.handle];
    if (si.type&mcpSampLoop)
    {
      writenum(buf, 40, col, si.loopend, 10, 6);
      writenum(buf, 47, col, si.loopend-si.loopstart, 10, 6);
      if (si.type&mcpSampBiDi)
        writestring(buf, 53, col, "\x1D", 1);
    }
    else
    {
      writenum(buf, 40, col, si.length, 10, 6);
      writestring(buf, 52, col, "-", 1);
    }
    writestring(buf, 55, col, (si.type&mcpSamp16Bit)?"16":" 8", 2);
    writestring(buf, 57, col, (si.type&mcpSampRedRate4)?"¬":(si.type&mcpSampRedRate2)?"«":(si.type&mcpSampRedBits)?"!":" ", 2);

    if (!plInstShowFreq)
    {
      writestring(buf, 60, col, plNoteStr[(sm.normnote+60*256)>>8], 3);
      writenum(buf, 64, col, sm.normnote&0xFF, 16, 2, 0);
    }
    else
    if (plInstShowFreq==1)
      writenum(buf, 60, col, mcpGetFreq8363(-sm.normnote), 10, 6, 1);
    else
      writenum(buf, 60, col, si.samprate, 10, 6, 1);

    if (sm.stdvol!=-1)
      writenum(buf, 68, col, sm.stdvol, 16, 2, 0);
    else
      writestring(buf, 68, col, " -", 2);
    if (sm.stdpan!=-1)
      writenum(buf, 72, col, sm.stdpan, 16, 2, 0);
    else
      writestring(buf, 72, col, " -", 2);

    if (sm.volenv!=0xFFFF)
      writestring(buf, 76, col, "v", 1);
    if (sm.panenv!=0xFFFF)
      writestring(buf, 77, col, "p", 1);
    if (sm.vibdepth&&sm.vibrate)
      writestring(buf, 78, col, "~", 1);
    if (sm.volfade&&(sm.volfade!=0xFFFF))
      writestring(buf, 79, col, "\x19", 1);
  }
}

static void xmpDisplayIns132(short *buf, int n, int plInstMode)
{
  char col;
  writestring(buf, 0, 0, "", 132);

  if (plBigInstNum[n]!=0xFF)
  {
    const xmpinstrument &ins=plInstr[plBigInstNum[n]];
    col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[plBigInstNum[n]]];
    writestring(buf, 0, col, (!plInstMode&&plInstUsed[plBigInstNum[n]])?"þ##: ":" ##: ", 5);
    writenum(buf, 1, col, plBigInstNum[n]+1, 16, 2, 0);
    writestring(buf, 5, col, ins.name, 35);
  }

  if (plBigSampNum[n]!=0xFFFF)
  {
    const xmpsample &sm=plModSamples[plBigSampNum[n]];
    col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plSampUsed[plBigSampNum[n]]];
    writestring(buf, 34, col, (!plInstMode&&plSampUsed[plBigSampNum[n]])?"þ###: ":" ###: ", 6);
    writenum(buf, 35, col, plBigSampNum[n], 16, 3, 0);
    writestring(buf, 40, col, sm.name, 28);
    const sampleinfo &si=plSamples[sm.handle];
    if (si.type&mcpSampLoop)
    {
      writenum(buf, 70, col, si.loopend, 10, 6);
      writenum(buf, 77, col, si.loopend-si.loopstart, 10, 6);
      if (si.type&mcpSampBiDi)
        writestring(buf, 83, col, "\x1D", 1);
    }
    else
    {
      writenum(buf, 70, col, si.length, 10, 6);
      writestring(buf, 82, col, "-", 1);
    }
    writestring(buf, 85, col, (si.type&mcpSamp16Bit)?"16":" 8", 2);
    writestring(buf, 87, col, (si.type&mcpSampRedRate4)?"¬":(si.type&mcpSampRedRate2)?"«":(si.type&mcpSampRedBits)?"!":" ", 2);

    if (!plInstShowFreq)
    {
      writestring(buf, 90, col, plNoteStr[(sm.normnote+60*256)>>8], 3);
      writenum(buf, 94, col, sm.normnote&0xFF, 16, 2, 0);
    }
    else
    if (plInstShowFreq==1)
      writenum(buf, 90, col, mcpGetFreq8363(-sm.normnote), 10, 6, 1);
    else
      writenum(buf, 90, col, si.samprate, 10, 6, 1);

    if (sm.stdvol!=-1)
      writenum(buf, 98, col, sm.stdvol, 16, 2, 0);
    else
      writestring(buf, 98, col, " -", 2);
    if (sm.stdpan!=-1)
      writenum(buf, 102, col, sm.stdpan, 16, 2, 0);
    else
      writestring(buf, 102, col, " -", 2);

    if (sm.volenv!=0xFFFF)
      writestring(buf, 106, col, "v", 1);
    if (sm.panenv!=0xFFFF)
      writestring(buf, 107, col, "p", 1);
    if (sm.vibdepth&&sm.vibrate)
      writestring(buf, 108, col, "~", 1);

    if (sm.volfade&&(sm.volfade!=0xFFFF))
      writenum(buf, 110, col, sm.volfade, 16, 4, 1);
    else
      writestring(buf, 113, col, "-", 1);
  }
}

static void xmpDisplayIns(short *buf, int len, int n, int plInstMode)
{
  switch (len)
  {
  case 33:
    xmpDisplayIns33(buf, n, plInstMode);
    break;
  case 40:
    xmpDisplayIns40(buf, n, plInstMode);
    break;
  case 52:
    xmpDisplayIns52(buf, n, plInstMode);
    break;
  case 80:
    xmpDisplayIns80(buf, n, plInstMode);
    break;
  case 132:
    xmpDisplayIns132(buf, n, plInstMode);
    break;
  }
}

static void (*Mark)(char *, char *);

static void xmpMark()
{
  int i;
  for (i=0; i<instnum; i++)
    if (plInstUsed[i])
      plInstUsed[i]=1;
  for (i=0; i<sampnum; i++)
    if (plSampUsed[i])
      plSampUsed[i]=1;
  Mark(plInstUsed, plSampUsed);
}

void xmpInstClear()
{
  memset(plInstUsed, 0, instnum);
  memset(plSampUsed, 0, sampnum);
}

static void Done()
{
  delete plInstUsed;
  delete plSampUsed;
  delete plBigInstNum;
  delete plBigSampNum;
}

void xmpInstSetup(const xmpinstrument *ins, int nins, const xmpsample *smp, int nsmp, const sampleinfo *smpi, int, int type, void (*MarkyBoy)(char *, char *))
{
  instnum=nins;
  sampnum=nsmp;
  plSampUsed=new char [sampnum];
  plInstUsed=new char [instnum];
  if (!plSampUsed||!plInstUsed)
    return;

  Mark=MarkyBoy;

  plInstr=ins;
  plModSamples=smp;
  plSamples=smpi;

  int i,j;
  int biginstlen=0;
  for (i=0; i<instnum; i++)
  {
    memset(plSampUsed, 0, sampnum);
    const xmpinstrument &ins=plInstr[i];
    for (j=0; j<128; j++)
      if (ins.samples[j]<sampnum)
        if (plModSamples[ins.samples[j]].handle<nsmp)
          plSampUsed[ins.samples[j]]=1;
    int num=0;
    for (j=0; j<sampnum; j++)
      if (plSampUsed[j])
        num++;
    biginstlen+=num?num:1;
  }
  plBigInstNum=new unsigned char [biginstlen];
  plBigSampNum=new unsigned short [biginstlen];
  if (!plBigInstNum||!plBigSampNum)
    return;
  memset(plBigInstNum, -1, biginstlen);
  memset(plBigSampNum, -1, biginstlen*2);

  biginstlen=0;
  for (i=0; i<instnum; i++)
  {
    memset(plSampUsed, 0, sampnum);
    const xmpinstrument &ins=plInstr[i];
    for (j=0; j<128; j++)
      if (ins.samples[j]<sampnum)
        if (plModSamples[ins.samples[j]].handle<nsmp)
          plSampUsed[ins.samples[j]]=1;
    int num=0;
    plBigInstNum[biginstlen]=i;
    for (j=0; j<sampnum; j++)
      if (plSampUsed[j])
        plBigSampNum[biginstlen+num++]=j;
    biginstlen+=num?num:1;
  }

  insdisplaystruct plInsDisplay;
  plInstShowFreq=type;
  plInsDisplay.Clear=xmpInstClear;
  plInsDisplay.n40=instnum;
  plInsDisplay.n52=instnum;
  plInsDisplay.n80=biginstlen;
  plInsDisplay.title80=plInstShowFreq?
                         " ##   instrument name / song message    length replen bit samprate vol pan  flgs":
                         " ##   instrument name / song message    length replen bit  bas\x0D ft vol pan  flgs";
  plInsDisplay.title132=plInstShowFreq?
                          " ##   instrument name / song message       sample name                length replen bit samprate vol pan  fl  fade           ":
                          " ##   instrument name / song message       sample name                length replen bit  bas\x0D ft vol pan  fl  fade           ";
  plInsDisplay.Mark=xmpMark;
  plInsDisplay.Display=xmpDisplayIns;
  plInsDisplay.Done=Done;
  xmpInstClear();
  plUseInstruments(plInsDisplay);
}