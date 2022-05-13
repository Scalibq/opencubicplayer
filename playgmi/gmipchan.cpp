// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMIPlay channel output routines
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -added "P" next to notes when sustain pedal is pressed

#include <stdlib.h>
#include <string.h>
#include "binfile.h"
#include "mcp.h"
#include "gmiplay.h"
#include "poutput.h"
#include "cpiface.h"

extern char plPause;
extern unsigned char plSelCh;
extern char plNoteStr[132][4];

static const minstrument *plChanMInstr;

static void drawchannel36(short *buf, int i)
{
  mchaninfo ci;
  midGetChanInfo(i, ci);
  unsigned char mute=midGetMute(i);

  unsigned char tcol=mute?0x08:0x0F;
  unsigned char tcold=mute?0x08:0x07;
  unsigned char tcolr=mute?0x08:0x0B;

  writestring(buf, 0, tcold, " -- -- -. תתת תתת תתת תתת תתת תתת   ", 36);

  if (!ci.notenum)
    return;
  writenum(buf,  1, tcol, plChanMInstr[ci.ins].prognum, 16, 2, 0);
  writenum(buf, 4, tcol, ci.gvol, 16, 2, 0);
  writestring(buf,  7, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
  writestring(buf,  8, tcol, " P"+(ci.pedal), 1);
  if (ci.notenum>6)
    ci.notenum=6;
  int j;
  for (j=0; j<ci.notenum; j++)
    writestring(buf, 10+4*j, (ci.opt[j]&1)?tcol:0x08, plNoteStr[ci.note[j]+12], 3);
}

static void drawchannel62(short *buf, int i)
{
  mchaninfo ci;
  midGetChanInfo(i, ci);
  unsigned char mute=midGetMute(i);

  unsigned char tcol=mute?0x08:0x0F;
  unsigned char tcold=mute?0x08:0x07;
  unsigned char tcolr=mute?0x08:0x0B;

  writestring(buf, 0, tcold, "                  -- -. תתת תתת תתת תתת תתת תתת תתת תתת תתת ", 62);

  if (!ci.notenum)
    return;
  writestring(buf,  1, tcol, plChanMInstr[ci.ins].name, 16);
  writenum(buf, 18, tcol, ci.gvol, 16, 2, 0);
  writestring(buf,  21, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
  writestring(buf,  22, tcol, " P"+(ci.pedal), 1);
  int j;
  if (ci.notenum>9)
    ci.notenum=9;
  for (j=0; j<ci.notenum; j++)
    writestring(buf, 24+4*j, (ci.opt[j]&1)?tcol:0x08, plNoteStr[ci.note[j]+12], 3);
}

static void drawchannel76(short *buf, int i)
{
  mchaninfo ci;
  midGetChanInfo(i, ci);
  unsigned char mute=midGetMute(i);

  unsigned char tcol=mute?0x08:0x0F;
  unsigned char tcold=mute?0x08:0x07;
  unsigned char tcolr=mute?0x08:0x0B;

  writestring(buf, 0, tcold, "               ³  ³ ³ ששש תת  ששש תת  ששש תת  ששש תת  ששש תת  ששש תת  ששש תת", 76);

  if (!ci.notenum)
    return;
  writestring(buf,  1, tcol, plChanMInstr[ci.ins].name, 14);
  writenum(buf, 16, tcol, ci.gvol, 16, 2, 0);
  writestring(buf, 19, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
  int j;
  if (ci.notenum>7)
    ci.notenum=7;
  for (j=0; j<ci.notenum; j++)
  {
    writestring(buf, 22+8*j, (ci.opt[j]&1)?tcol:0x08, plNoteStr[ci.note[j]+12], 3);
    writenum(buf, 26+8*j, (ci.opt[j]&1)?tcold:0x08, ci.vol[j], 16, 2, 0);
  }
}

static void drawchannel128(short *buf, int i)
{
  mchaninfo ci;
  midGetChanInfo(i, ci);
  unsigned char mute=midGetMute(i);

  unsigned char tcol=mute?0x08:0x0F;
  unsigned char tcold=mute?0x08:0x07;
  unsigned char tcolr=mute?0x08:0x0B;

  writestring(buf, 0, tcold, "                  ³  ³ ³     ³  ³  ³  ששש תת  ששש תת  ששש תת  ששש תת  ששש תת  ששש תת  ששש תת  ששש תת  ששש תת  ששש תת  ששש תת  ", 128);

  if (!ci.notenum)
    return;

  writestring(buf,  1, tcol, plChanMInstr[ci.ins].name, 16);
  writenum(buf, 19, tcol, ci.gvol, 16, 2, 0);
  writestring(buf, 22, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
  writestring(buf, 24, tcol, (ci.pitch<0)?"-":(ci.pitch>0)?"+":" ", 1);
  writenum(buf, 25, tcol, abs(ci.pitch), 16, 4, 0);
  writenum(buf, 30, tcol, ci.reverb, 16, 2, 0);
  writenum(buf, 33, tcol, ci.chorus, 16, 2, 0);
  int j;
  if (ci.notenum>11)
    ci.notenum=11;
  for (j=0; j<ci.notenum; j++)
  {
    writestring(buf, 38+8*j, (ci.opt[j]&1)?tcol:0x08, plNoteStr[ci.note[j]+12], 3);
    writenum(buf, 42+8*j, (ci.opt[j]&1)?tcold:0x08, ci.vol[j], 16, 2, 0);
  }
}

static void drawchannel44(short *buf, int i)
{
  mchaninfo ci;
  midGetChanInfo(i, ci);
  unsigned char mute=midGetMute(i);

  unsigned char tcol=mute?0x08:0x0F;
  unsigned char tcold=mute?0x08:0x07;
  unsigned char tcolr=mute?0x08:0x0B;

  writestring(buf, 0, tcold, " -- -- -. תתת תתת תתת תתת תתת תתת תתת תתת   ", 44);

  if (!ci.notenum)
    return;

  writenum(buf,  1, tcol, plChanMInstr[ci.ins].prognum, 16, 2, 0);
  writenum(buf, 4, tcol, ci.gvol, 16, 2, 0);
  writestring(buf,  7, tcol, "L123456MM9ABCDER"+(ci.pan>>4), 1);
  writestring(buf,  8, tcol, " P"+(ci.pedal), 1);
  int j;
  if (ci.notenum>8)
    ci.notenum=8;
  for (j=0; j<ci.notenum; j++)
    writestring(buf, 10+4*j, (ci.opt[j]&1)?tcol:0x08, plNoteStr[ci.note[j]+12], 3);
}

static void drawchannel(short *buf, int len, int i)
{
  switch (len)
  {
  case 36:
    drawchannel36(buf, i);
    break;
  case 44:
    drawchannel44(buf, i);
    break;
  case 62:
    drawchannel62(buf, i);
    break;
  case 76:
    drawchannel76(buf, i);
    break;
  case 128:
    drawchannel128(buf, i);
    break;
  }
}

void gmiChanSetup(const midifile &mid)
{
  plChanMInstr=mid.instruments;
  plUseChannels(drawchannel);
}