// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIFace output routines / key handlers for the MCP system
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -fd981119   Felix Domke <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT'

#define NO_MCPBASE_IMPORT
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <dos.h>
#include "pfilesel.h"
#include "mcp.h"
#include "psetting.h"
#include "poutput.h"
#include "sets.h"

static short vol;
static short bal;
static short pan;
static char srnd;
static long amp;
static short speed;
static short pitch;
static short reverb;
static short chorus;
static char splock=1;
static char viewfx=0;
static char finespeed=8;

void mcpNormalize()
{
  speed=set.speed;
  pitch=set.pitch;
  pan=set.pan;
  bal=set.bal;
  vol=set.vol;
  amp=set.amp;
  srnd=set.srnd;
  reverb=set.reverb;
  chorus=set.chorus;
  mcpSet(-1, mcpMasterAmplify, 256*amp);
  mcpSet(-1, mcpMasterVolume, vol);
  mcpSet(-1, mcpMasterBalance, bal);
  mcpSet(-1, mcpMasterPanning, pan);
  mcpSet(-1, mcpMasterSurround, srnd);
  mcpSet(-1, mcpMasterPitch, pitch);
  mcpSet(-1, mcpMasterSpeed, speed);
  mcpSet(-1, mcpMasterReverb, reverb);
  mcpSet(-1, mcpMasterChorus, chorus);
  mcpSet(-1, mcpMasterFilter, set.filter);
}

void mcpDrawGStrings(short (*buf)[132])
{
  if (plScrWidth==80)
  {
    writestring(buf[0], 0, 0x09, " vol: úúúúúúúú ", 15);
    if (viewfx)
      writestring(buf[0], 15, 0x09, " echo: ú  rev: -úúúnúúú+  chr: -úúúnúúú+ ", 41);
    else
      writestring(buf[0], 15, 0x09, " srnd: ú  pan: lúúúmúúúr  bal: lúúúmúúúr ", 41);
    writestring(buf[0], 56, 0x09, " spd: ---%  pitch: ---% ", 24);
    if (splock)
      writestring(buf[0], 67, 0x09, "\x1D p", 3);
    writestring(buf[0], 6, 0x0F, "þþþþþþþþ", (vol+4)>>3);
    if (viewfx)
    {
      writestring(buf[0], 22, 0x0F, 0?"x":"o", 1);
      writestring(buf[0], 30+((reverb+70)>>4), 0x0F, "I", 1);
      writestring(buf[0], 46+((chorus+70)>>4), 0x0F, "I", 1);
    }
    else
    {
      writestring(buf[0], 22, 0x0F, srnd?"x":"o", 1);
      if (((pan+70)>>4)==4)
        writestring(buf[0], 34, 0x0F, "m", 1);
      else
      {
        writestring(buf[0], 30+((pan+70)>>4), 0x0F, "r", 1);
        writestring(buf[0], 38-((pan+70)>>4), 0x0F, "l", 1);
      }
      writestring(buf[0], 46+((bal+70)>>4), 0x0F, "I", 1);
    }
    writenum(buf[0], 62, 0x0F, speed*100/256, 10, 3);
    writenum(buf[0], 75, 0x0F, pitch*100/256, 10, 3);

    writestring(buf[1], 58, 0x09, "amp: ...% filter: ... ", 22);
    writenum(buf[1], 63, 0x0F, amp*100/64, 10, 3);
    writestring(buf[1], 76, 0x0F, (set.filter==1)?"AOI":(set.filter==2)?"FOI":"off", 3);
  }
  else
  {
    writestring(buf[0], 0, 0x09, "    volume: úúúúúúúúúúúúúúúú  ", 30);
    if (viewfx)
      writestring(buf[0], 30, 0x09, " echoactive: ú   reverb: -úúúúúúúmúúúúúúú+   chorus: -úúúúúúúmúúúúúúú+  ", 72);
    else
      writestring(buf[0], 30, 0x09, " surround: ú   panning: lúúúúúúúmúúúúúúúr   balance: lúúúúúúúmúúúúúúúr  ", 72);
    writestring(buf[0], 102, 0x09,  " speed: ---%   pitch: ---%    ", 30);
    writestring(buf[0], 12, 0x0F, "þþþþþþþþþþþþþþþþ", (vol+2)>>2);
    if (viewfx)
    {
      writestring(buf[0], 43, 0x0F, 0?"x":"o", 1);
      writestring(buf[0], 55+((reverb+68)>>3), 0x0F, "I", 1);
      writestring(buf[0], 83+((chorus+68)>>3), 0x0F, "I", 1);
    }
    else
    {
      writestring(buf[0], 41, 0x0F, srnd?"x":"o", 1);
      if (((pan+68)>>3)==8)
        writestring(buf[0], 62, 0x0F, "m", 1);
      else
      {
        writestring(buf[0], 54+((pan+68)>>3), 0x0F, "r", 1);
        writestring(buf[0], 70-((pan+68)>>3), 0x0F, "l", 1);
      }
      writestring(buf[0], 83+((bal+68)>>3), 0x0F, "I", 1);
    }
    writenum(buf[0], 110, 0x0F, speed*100/256, 10, 3);
    if (splock)
      writestring(buf[0], 115, 0x09, "\x1D", 1);
    writenum(buf[0], 124, 0x0F, pitch*100/256, 10, 3);

    writestring(buf[1], 81, 0x09, "              amplification: ...%  filter: ...     ", 52);
    writenum(buf[1], 110, 0x0F, amp*100/64, 10, 3);
    writestring(buf[1], 124, 0x0F, (set.filter==1)?"AOI":(set.filter==2)?"FOI":"off", 3);
  }
}

int mcpSetProcessKey(unsigned short key)
{
  switch (key)
  {
  case '-':
    if (vol>=2)
      vol-=2;
    mcpSet(-1, mcpMasterVolume, vol);
    break;
  case '+':
    if (vol<=62)
      vol+=2;
    mcpSet(-1, mcpMasterVolume, vol);
    break;
  case '/':
    if ((bal-=4)<-64)
      bal=-64;
    mcpSet(-1, mcpMasterBalance, bal);
    break;
  case '*':
    if ((bal+=4)>64)
      bal=64;
    mcpSet(-1, mcpMasterBalance, bal);
    break;
  case ',':
    if ((pan-=4)<-64)
      pan=-64;
    mcpSet(-1, mcpMasterPanning, pan);
    break;
  case '.':
    if ((pan+=4)>64)
      pan=64;
    mcpSet(-1, mcpMasterPanning, pan);
    break;
  case 0x3c00: //f2
    if ((vol-=8)<0)
      vol=0;
    mcpSet(-1, mcpMasterVolume, vol);
    break;
  case 0x3d00: //f3
    if ((vol+=8)>64)
      vol=64;
    mcpSet(-1, mcpMasterVolume, vol);
    break;
  case 0x3e00: //f4
    mcpSet(-1, mcpMasterSurround, srnd=!srnd);
    break;
  case 0x3f00: //f5
    if ((pan-=16)<-64)
      pan=-64;
    mcpSet(-1, mcpMasterPanning, pan);
    break;
  case 0x4000: //f6
    if ((pan+=16)>64)
      pan=64;
    mcpSet(-1, mcpMasterPanning, pan);
    break;
  case 0x4100: //f7
    if ((bal-=16)<-64)
      bal=-64;
    mcpSet(-1, mcpMasterBalance, bal);
    break;
  case 0x4200: //f8
    if ((bal+=16)>64)
      bal=64;
    mcpSet(-1, mcpMasterBalance, bal);
    break;
  case 0x4300: //f9
    if ((speed-=finespeed)<16)
      speed=16;
    mcpSet(-1, mcpMasterSpeed, speed);
    if (splock)
      mcpSet(-1, mcpMasterPitch, pitch=speed);
    break;
  case 0x4400: //f10
    if ((speed+=finespeed)>2048)
      speed=2048;
    mcpSet(-1, mcpMasterSpeed, speed);
    if (splock)
      mcpSet(-1, mcpMasterPitch, pitch=speed);
    break;
  case 0x8500: //f11
    if ((pitch-=finespeed)<16)
      pitch=16;
    mcpSet(-1, mcpMasterPitch, pitch);
    if (splock)
      mcpSet(-1, mcpMasterSpeed, speed=pitch);
    break;
  case 0x8600: //f12
    if ((pitch+=finespeed)>2048)
      pitch=2048;
    mcpSet(-1, mcpMasterPitch, pitch);
    if (splock)
      mcpSet(-1, mcpMasterSpeed, speed=pitch);
    break;
  case 0x5f00: // ctrl f2
    if ((amp-=4)<4)
      amp=4;
    mcpSet(-1, mcpMasterAmplify, 256*amp);
    break;
  case 0x6000: // ctrl f3
    if ((amp+=4)>508)
      amp=508;
    mcpSet(-1, mcpMasterAmplify, 256*amp);
    break;
  case 0x6100: // ctrl f4
    viewfx^=1;
    break;
  case 0x6200: // ctrl f5
    if ((reverb-=8)<-64)
      reverb=-64;
    mcpSet(-1, mcpMasterReverb, reverb);
    break;
  case 0x6300: // ctrl f6
    if ((reverb+=8)>64)
      reverb=64;
    mcpSet(-1, mcpMasterReverb, reverb);
    break;
  case 0x6400: // ctrl f7
    if ((chorus-=8)<-64)
      chorus=-64;
    mcpSet(-1, mcpMasterChorus, chorus);
    break;
  case 0x6500: // ctrl f8
    if ((chorus+=8)>64)
      chorus=64;
    mcpSet(-1, mcpMasterChorus, chorus);
    break;
  case 0x8900: // ctrl f11
    finespeed=(finespeed==8)?1:8;
    break;
  case 0x8a00: // ctrl f12
    splock^=1;
    break;
  case 8:
    mcpSet(-1, mcpMasterFilter, set.filter=(set.filter==1)?2:(set.filter==2)?0:1);
    break;
  case 0x6a00:
    mcpNormalize();
    break;
  case 0x6900:
    set.pan=pan;
    set.bal=bal;
    set.vol=vol;
    set.speed=speed;
    set.pitch=pitch;
    set.amp=amp;
    set.reverb=reverb;
    set.chorus=chorus;
    set.srnd=srnd;
    break;
  case 0x6b00:
    pan=64;
    bal=0;
    vol=64;
    speed=256;
    pitch=256;
    chorus=0;
    reverb=0;
    amp=64;
    mcpSet(-1, mcpMasterVolume, vol);
    mcpSet(-1, mcpMasterBalance, bal);
    mcpSet(-1, mcpMasterPanning, pan);
    mcpSet(-1, mcpMasterSurround, srnd);
    mcpSet(-1, mcpMasterPitch, pitch);
    mcpSet(-1, mcpMasterSpeed, speed);
    mcpSet(-1, mcpMasterReverb, reverb);
    mcpSet(-1, mcpMasterChorus, chorus);
    mcpSet(-1, mcpMasterAmplify, 256*amp);
    break;
  default:
    return 0;
  }
  return 1;
}

void mcpSetFadePars(int i)
{
  mcpSet(-1, mcpMasterPitch, pitch*i/64);
  mcpSet(-1, mcpMasterSpeed, speed*i/64);
  mcpSet(-1, mcpMasterVolume, vol*i/64);
}
