// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMDPlay instrument display routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <string.h>
#include "mcp.h"
#include "gmdinst.h"
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

static const gmdinstrument *plInstr;
static const sampleinfo *plSamples;
static const gmdsample *plModSamples;

static char plInstShowFreq;

static void gmdDisplayIns40(short *buf, int n, int plInstMode)
{
  char col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[n]];
  writestring(buf, 0, col, (!plInstMode&&plInstUsed[n])?"�##: ":" ##: ", 5);
  writenum(buf, 1, col, n+1, 16, 2, 0);
  writestring(buf, 5, col, plInstr[n].name, 35);
}

static void gmdDisplayIns33(short *buf, int n, int plInstMode)
{
  char col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[n]];
  writestring(buf, 0, col, (!plInstMode&&plInstUsed[n])?"�##: ":" ##: ", 5);
  writenum(buf, 1, col, n+1, 16, 2, 0);
  writestring(buf, 5, col, plInstr[n].name, 28);
}

static void gmdDisplayIns52(short *buf, int n, int plInstMode)
{
  char col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[n]];
  writestring(buf, 0, col, (!plInstMode&&plInstUsed[n])?"    �##: ":"     ##: ", 9);
  writenum(buf, 5, col, n+1, 16, 2, 0);
  writestring(buf, 9, col, plInstr[n].name, 43);
}

static void gmdDisplayIns80(short *buf, int n, int plInstMode)
{
  char col;
  writestring(buf, 0, 0, "", 80);

  if (plBigInstNum[n]!=0xFF)
  {
    const gmdinstrument &ins=plInstr[plBigInstNum[n]];
    col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[plBigInstNum[n]]];
    writestring(buf, 0, col, (!plInstMode&&plInstUsed[plBigInstNum[n]])?"�##: ":" ##: ", 5);
    writenum(buf, 1, col, plBigInstNum[n]+1, 16, 2, 0);
    writestring(buf, 5, col, ins.name, 31);
  }

  if (plBigSampNum[n]!=0xFFFF)
  {
    const gmdsample &sm=plModSamples[plBigSampNum[n]];
    col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plSampUsed[plBigSampNum[n]]];
    writestring(buf, 34, col, (!plInstMode&&plSampUsed[plBigSampNum[n]])?"�###: ":" ###: ", 6);
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
    writestring(buf, 57, col, (si.type&mcpSampRedRate4)?"�":(si.type&mcpSampRedRate2)?"�":(si.type&mcpSampRedBits)?"!":" ", 2);

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

static void gmdDisplayIns132(short *buf, int n, int plInstMode)
{
  char col;
  writestring(buf, 0, 0, "", 132);

  if (plBigInstNum[n]!=0xFF)
  {
    const gmdinstrument &ins=plInstr[plBigInstNum[n]];
    col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[plBigInstNum[n]]];
    writestring(buf, 0, col, (!plInstMode&&plInstUsed[plBigInstNum[n]])?"�##: ":" ##: ", 5);
    writenum(buf, 1, col, plBigInstNum[n]+1, 16, 2, 0);
    writestring(buf, 5, col, ins.name, 35);
  }

  if (plBigSampNum[n]!=0xFFFF)
  {
    const gmdsample &sm=plModSamples[plBigSampNum[n]];
    col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plSampUsed[plBigSampNum[n]]];
    writestring(buf, 34, col, (!plInstMode&&plSampUsed[plBigSampNum[n]])?"�###: ":" ###: ", 6);
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
    writestring(buf, 87, col, (si.type&mcpSampRedRate4)?"�":(si.type&mcpSampRedRate2)?"�":(si.type&mcpSampRedBits)?"!":" ", 2);

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

static void gmdDisplayIns(short *buf, int len, int n, int plInstMode)
{
  switch (len)
  {
  case 33:
    gmdDisplayIns33(buf, n, plInstMode);
    break;
  case 40:
    gmdDisplayIns40(buf, n, plInstMode);
    break;
  case 52:
    gmdDisplayIns52(buf, n, plInstMode);
    break;
  case 80:
    gmdDisplayIns80(buf, n, plInstMode);
    break;
  case 132:
    gmdDisplayIns132(buf, n, plInstMode);
    break;
  }
}

static void (*Mark)(char *, char *);

static void gmdMark()
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

void gmdInstClear()
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

void gmdInstSetup(const gmdinstrument *ins, int nins, const gmdsample *smp, int nsmp, const sampleinfo *smpi, int, int type, void (*MarkyBoy)(char *, char *))
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
    const gmdinstrument &ins=plInstr[i];
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
    const gmdinstrument &ins=plInstr[i];
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
  plInsDisplay.Clear=gmdInstClear;
  plInsDisplay.n40=instnum;
  plInsDisplay.n52=instnum;
  plInsDisplay.n80=biginstlen;
  plInsDisplay.title80=plInstShowFreq?
                         " ##   instrument name / song message    length replen bit samprate vol pan  flgs":
                         " ##   instrument name / song message    length replen bit  bas\x0D ft vol pan  flgs";
  plInsDisplay.title132=plInstShowFreq?
                          " ##   instrument name / song message       sample name                length replen bit samprate vol pan  fl  fade           ":
                          " ##   instrument name / song message       sample name                length replen bit  bas\x0D ft vol pan  fl  fade           ";
  plInsDisplay.Mark=gmdMark;
  plInsDisplay.Display=gmdDisplayIns;
  plInsDisplay.Done=Done;
  gmdInstClear();
  plUseInstruments(plInsDisplay);
}
