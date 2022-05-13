// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMIPlay Instrument display routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <string.h>
#include "binfile.h"
#include "mcp.h"
#include "gmiplay.h"
#include "poutput.h"
#include "cpiface.h"

char plInstUsed[256];
char plSampUsed[1024];
short plInstSampNum[256];

extern char plNoteStr[132][4];

static const minstrument *plMInstr;
static const sampleinfo *plSamples;

static void gmiDisplayIns40(short *buf, int n, int plInstMode)
{
  char col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[n]];
  writestring(buf, 0, col, (!plInstMode&&plInstUsed[n])?"þ##: ":" ##: ", 5);
  writenum(buf, 1, col, plMInstr[n].prognum, 16, 2, 0);
  writestring(buf, 5, col, plMInstr[n].name, 35);
}

static void gmiDisplayIns33(short *buf, int n, int plInstMode)
{
  char col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[n]];
  writestring(buf, 0, col, (!plInstMode&&plInstUsed[n])?"þ##: ":" ##: ", 5);
  writenum(buf, 1, col, plMInstr[n].prognum, 16, 2, 0);
  writestring(buf, 5, col, plMInstr[n].name, 28);
}

static void gmiDisplayIns52(short *buf, int n, int plInstMode)
{
  int i,j;
  for (i=0; n>=plInstSampNum[i+1]; i++);
  j=n-plInstSampNum[i];

  char col;
  writestring(buf, 0, 0, "", 52);

  const minstrument &ins=plMInstr[i];
  if (!j)
  {
    col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[i]];
    writestring(buf, 0, col, (!plInstMode&&plInstUsed[i])?"    þ##: ":"     ##: ", 9);
    writenum(buf, 5, col, ins.prognum, 16, 2, 0);
    writestring(buf, 9, col, ins.name, 16);
  }
  col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plSampUsed[plInstSampNum[i]+j]];
  const msample &sm=ins.samples[j];

  writestring(buf, 26, col, (!plInstMode&&plSampUsed[plInstSampNum[i]+j])?"þ##: ":" ##: ", 5);
  writenum(buf, 27, col, sm.sampnum, 16, 2);
  writestring(buf, 31, col, sm.name, 16);
}

static void gmiDisplayIns80(short *buf, int n, int plInstMode)
{
  char col;
  writestring(buf, 0, 0, "", 80);

  int i,j;
  for (i=0; n>=plInstSampNum[i+1]; i++);
  j=n-plInstSampNum[i];

  const minstrument &ins=plMInstr[i];
  if (!j)
  {
    col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[i]];
    writestring(buf, 0, col, (!plInstMode&&plInstUsed[i])?"þ##: ":" ##: ", 5);
    writenum(buf, 1, col, ins.prognum, 16, 2, 0);
    writestring(buf, 5, col, ins.name, 16);
  }
  col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plSampUsed[plInstSampNum[i]+j]];
  const msample &sm=ins.samples[j];

  writestring(buf, 22, col, (!plInstMode&&plSampUsed[plInstSampNum[i]+j])?"þ##: ":" ##: ", 5);
  writenum(buf, 23, col, sm.sampnum, 16, 2);
  writestring(buf, 27, col, sm.name, 16);

  if (sm.handle!=-1)
  {
    sampleinfo si=plSamples[sm.handle];

    if (si.type&mcpSampLoop)
    {
      writenum(buf, 44, col, si.loopend, 10, 6);
      writenum(buf, 51, col, si.loopend-si.loopstart, 10, 6);
      if (si.type&mcpSampBiDi)
        writestring(buf, 57, col, "\x1D", 1);
    }
    else
    {
      writenum(buf, 44, col, si.length, 10, 6);
      writestring(buf, 56, col, "-", 1);
    }
    writestring(buf, 59, col, (si.type&mcpSamp16Bit)?"16":" 8", 2);
    writestring(buf, 61, col, (si.type&mcpSampRedRate4)?"¬":(si.type&mcpSampRedRate2)?"«":(si.type&mcpSampRedBits)?"!":" ", 2);
    writenum(buf, 63, col, si.samprate, 10, 6);
    writestring(buf, 69, col, "Hz", 2);
    writestring(buf, 73, col, plNoteStr[(sm.normnote+12*256)>>8], 3);
    writenum(buf, 77, col, sm.normnote&0xFF, 16, 2, 0);
  }
}

static void gmiDisplayIns132(short *buf, int n, int plInstMode)
{
  char col;
  writestring(buf, 0, 0, "", 132);

  int i,j;
  for (i=0; n>=plInstSampNum[i+1]; i++);
  j=n-plInstSampNum[i];

  const minstrument &ins=plMInstr[i];
  if (!j)
  {
    col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plInstUsed[i]];
    writestring(buf, 0, col, (!plInstMode&&plInstUsed[i])?"þ##: ":" ##: ", 5);
    writenum(buf, 1, col, ins.prognum, 16, 2, 0);
    writestring(buf, 5, col, ins.name, 16);
  }
  col=plInstMode?0x07:"\x08\x08\x0B\x0A"[plSampUsed[plInstSampNum[i]+j]];
  const msample &sm=ins.samples[j];

  writestring(buf, 22, col, (!plInstMode&&plSampUsed[plInstSampNum[i]+j])?"þ##: ":" ##: ", 5);
  writenum(buf, 23, col, sm.sampnum, 16, 2);
  writestring(buf, 27, col, sm.name, 16);

  if (sm.handle!=-1)
  {
    sampleinfo si=plSamples[sm.handle];

    if (si.type&mcpSampLoop)
    {
      writenum(buf, 44, col, si.loopend, 10, 6);
      writenum(buf, 51, col, si.loopend-si.loopstart, 10, 6);
      if (si.type&mcpSampBiDi)
        writestring(buf, 57, col, "\x1D", 1);
    }
    else
    {
      writenum(buf, 44, col, si.length, 10, 6);
      writestring(buf, 56, col, "-", 1);
    }
    writestring(buf, 59, col, (si.type&mcpSamp16Bit)?"16":" 8", 2);
    writestring(buf, 61, col, (si.type&mcpSampRedRate4)?"¬":(si.type&mcpSampRedRate2)?"«":(si.type&mcpSampRedBits)?"!":" ", 2);
    writenum(buf, 63, col, si.samprate, 10, 6);
    writestring(buf, 69, col, "Hz", 2);
    writestring(buf, 73, col, plNoteStr[(sm.normnote+12*256)>>8], 3);
    writenum(buf, 77, col, sm.normnote&0xFF, 16, 2, 0);
  }
}

static void gmiDisplayIns(short *buf, int len, int n, int plInstMode)
{
  switch (len)
  {
  case 33:
    gmiDisplayIns33(buf, n, plInstMode);
    break;
  case 40:
    gmiDisplayIns40(buf, n, plInstMode);
    break;
  case 52:
    gmiDisplayIns52(buf, n, plInstMode);
    break;
  case 80:
    gmiDisplayIns80(buf, n, plInstMode);
    break;
  case 132:
    gmiDisplayIns132(buf, n, plInstMode);
    break;
  }
}

static void gmiMarkIns()
{
  int i,j;
  for (i=0; i<256; i++)
    if (plInstUsed[i])
      plInstUsed[i]=1;
  for (i=0; i<1024; i++)
    if (plSampUsed[i])
      plSampUsed[i]=1;

  for (i=0; i<16; i++)
  {
    mchaninfo ci;
    midGetChanInfo(i, ci);
    unsigned char mute=midGetMute(i);

    if (!mute&&ci.notenum)
    {
      plInstUsed[ci.ins]=((plSelCh==i)||(plInstUsed[ci.ins]==3))?3:2;
      for (j=0; j<ci.notenum; j++)
//        if (ci.opt[j]&1)
        plSampUsed[plInstSampNum[ci.ins]+plMInstr[ci.ins].note[ci.note[j]]]=((plSelCh==i)||(plSampUsed[plInstSampNum[ci.ins]+plMInstr[ci.ins].note[ci.note[j]]]==3))?3:2;
    }
  }
}

void gmiClearInst()
{
  memset(plInstUsed, 0, 256);
  memset(plSampUsed, 0, 1024);
}

void gmiInsSetup(const midifile &mid)
{
  insdisplaystruct plInsDisplay;
  int i;
  int plInstNum=mid.instnum;
  plMInstr=mid.instruments;
  plSamples=mid.samples;
  int plSampNum=0;
  for (i=0; i<plInstNum; i++)
  {
    plInstSampNum[i]=plSampNum;
    plSampNum+=plMInstr[i].sampnum;
  }
  plInstSampNum[i]=plSampNum;

  plInsDisplay.Clear=gmiClearInst;
  plInsDisplay.n40=plInstNum;
  plInsDisplay.n52=plSampNum;
  plInsDisplay.n80=plSampNum;
  plInsDisplay.title80=" ##   instrument name                       length replen bit  samprate  base-\x0D      ";
  plInsDisplay.title132=" ##   instrument name                       length replen bit  samprate  base-\x0D      ";
  plInsDisplay.Mark=gmiMarkIns;
  plInsDisplay.Display=gmiDisplayIns;
  plInsDisplay.Done=0;
  gmiClearInst();
  plUseInstruments(plInsDisplay);
}
