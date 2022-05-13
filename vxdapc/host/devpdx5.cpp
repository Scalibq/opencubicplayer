// ------------------------------------------------------------------------
// This source code is (c) KB
// This code is part of the "Indoor Demo System" by Tammo Hinrichs, based
// on the Indoor Music System by Niklas Beisert / pascal.
// slightly modified by tmb.
//
// DevPDX5 - sound device for DirectSound 5 or higher
// ------------------------------------------------------------------------

#define PRE_BUFFER
/* unless somebody find a way to allocate mem in the global space. */
/* (LocalAlloc, GlobalAlloc *AND* VirtualAlloc (even with MEM_COMMIT)
   WILL NOT WORK. */
#include <stdio.h>
#include <stdlib.h>

#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <dsound.h>

#define CRLF "\r\n"
void AddLog(const char *fmt, ...);
void SetError(int err);
extern __declspec(dllimport) HWND hWnd;

#undef DSBSIZE_MAX
#define DSBSIZE_MAX (128*1024)

#include "imsdev.h"
#include "player.h"
#include "imsrtns.h"
#include "imswin.h"

static long buflen;
static signed long playpos;

static LPDIRECTSOUND lpds;
static int lpdsOpened=0;
static DSCAPS dscaps;

static WAVEFORMATEX pbformat, sbformat;
static WAVEFORMATEX *wbformat;
static DSBUFFERDESC pbdesc, sbdesc;
static LPDIRECTSOUNDBUFFER pbuffer=NULL, sbuffer=NULL, wbuffer=NULL;
static DSBCAPS pbcaps;

static int  locked;
static void *lockbuf,*lockbuf2;
static DWORD locklen,locklen2;

static char *tempbuf;

static int writeprim;

extern "C" extern sounddevice plrDirectSound;

static long dmaGetBufPos()
{
  long pc;
  wbuffer->GetCurrentPosition((DWORD *)&pc,0);
  return pc;
}

signed long lastpos=0;
static void advance(int pos)
{
  playpos+=(pos-(playpos%buflen)+buflen)%buflen;

  int tocopy=(buflen+pos-lastpos)%buflen;
  int lockok=0;
  while (!lockok)
    {
      switch (wbuffer->Lock(lastpos%buflen,tocopy,&lockbuf,&locklen,&lockbuf2,&locklen2,0))
	{
	case DS_OK:
	  lockok=1;
	  break;
	case DSERR_BUFFERLOST:
	  if FAILED(wbuffer->Restore())
		     lockok=2;
	  break;
	default:
	  lockok=2;
	}
    }
  if (lockok==1)
    {
      memcpyf(lockbuf,tempbuf+(lastpos%buflen),locklen);
      memcpyf(lockbuf2,tempbuf,locklen2);
    }
  wbuffer->Unlock(lockbuf,locklen,lockbuf2,locklen2);
  lastpos=pos;
}

static long gettimer()
{
  int realpos=playpos+((dmaGetBufPos()-(playpos%buflen)+buflen)%buflen);
  return imuldiv(realpos, 65536, wbformat->nAvgBytesPerSec);
}

static int getpos()
{
  return dmaGetBufPos();
}


static void dxpSetOptions(int rate, int opt)
{
#ifdef DEBUG
  AddLog("dxpSetOptions(%d, %x);" CRLF, rate, opt);
#endif

  unsigned char stereo=!!(opt&PLR_STEREO);
  unsigned char bit16=!!(opt&PLR_16BIT);

  if (rate<5000)
    rate=5000;
  if (rate>64000)
    rate=64000;

  if (!(dscaps.dwFlags&DSCAPS_PRIMARYMONO))
    stereo=1;
  if (!(dscaps.dwFlags&DSCAPS_PRIMARYSTEREO))
    stereo=0;
  if (!(dscaps.dwFlags&DSCAPS_PRIMARY8BIT))
    bit16=1;
  if (!(dscaps.dwFlags&DSCAPS_PRIMARY16BIT))
    bit16=0;
  plrRate=rate;
  plrOpt=(stereo?PLR_STEREO:0)|(bit16?PLR_16BIT:0)|PLR_SIGNEDOUT;

  if (writeprim)
    {
      memsetb(&pbformat, 0, sizeof(WAVEFORMATEX));
      pbformat.wFormatTag = WAVE_FORMAT_PCM;
      pbformat.nChannels  = plrOpt&PLR_STEREO?2:1;
      pbformat.nSamplesPerSec = plrRate;
      pbformat.wBitsPerSample = plrOpt&PLR_16BIT?16:8;
      pbformat.nBlockAlign = pbformat.wBitsPerSample/8*pbformat.nChannels;
      pbformat.nAvgBytesPerSec = pbformat.nSamplesPerSec*pbformat.nBlockAlign;
      if FAILED(pbuffer->SetFormat(&pbformat))
	{
	  AddLog("DEVPDX5: Couldn't set primary buffer format, using secondary buffer." CRLF);
	  if (writeprim)
	    {
	      lpds->SetCooperativeLevel(hWnd,DSSCL_PRIORITY);
	      writeprim=0;
	    }
	}
      else
	{
	  pbuffer->GetFormat(&pbformat,sizeof(pbformat),NULL);
	  plrRate=pbformat.nSamplesPerSec;
	  plrOpt=(pbformat.nChannels==2?PLR_STEREO:0)|(pbformat.wBitsPerSample==16?PLR_16BIT:0)|PLR_SIGNEDOUT;
	}
    }
}

static int dxpPlay(void *&buf, int &len)
{
#ifdef DEBUG
  AddLog("dxpplay();" CRLF);
#endif
  // set primary buffer format as close as possible to the
  // desired output format (not too critical)
  memsetb(&pbformat, 0, sizeof(WAVEFORMATEX));
  pbformat.wFormatTag = WAVE_FORMAT_PCM;
  pbformat.nChannels  = plrOpt&PLR_STEREO?2:1;
  pbformat.nSamplesPerSec = plrRate;
  pbformat.wBitsPerSample = plrOpt&PLR_16BIT?16:8;
  pbformat.nBlockAlign = pbformat.wBitsPerSample/8*pbformat.nChannels;
  pbformat.nAvgBytesPerSec = pbformat.nSamplesPerSec*pbformat.nBlockAlign;
  if FAILED(pbuffer->SetFormat(&pbformat))
    {
      if (writeprim)
	{
	  lpds->SetCooperativeLevel(hWnd,DSSCL_PRIORITY);
	  writeprim=0;
	}
    }
  pbuffer->GetFormat(&pbformat,sizeof(pbformat),NULL);

  if (writeprim)
    {
      pbcaps.dwSize=sizeof(pbcaps);
      pbuffer->GetCaps(&pbcaps);
      len=pbcaps.dwBufferBytes;
      wbuffer=pbuffer;
      wbformat=&pbformat;
    }
  else
    {
      // allocate a secondary buffer
      if (len<DSBSIZE_MIN) len=DSBSIZE_MIN;
      if (len>DSBSIZE_MAX) len=DSBSIZE_MAX;

      memsetb(&sbformat, 0, sizeof(WAVEFORMATEX));
      sbformat.wFormatTag = WAVE_FORMAT_PCM;
      sbformat.nChannels  = plrOpt&PLR_STEREO?2:1;
      sbformat.nSamplesPerSec = plrRate;
      sbformat.wBitsPerSample = plrOpt&PLR_16BIT?16:8;
      sbformat.nBlockAlign = sbformat.wBitsPerSample/8*sbformat.nChannels;
      sbformat.nAvgBytesPerSec = sbformat.nSamplesPerSec*sbformat.nBlockAlign;

      memsetb(&sbdesc, 0, sizeof(DSBUFFERDESC));
      sbdesc.dwSize  = sizeof(DSBUFFERDESC);
      sbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_GLOBALFOCUS|DSBCAPS_STICKYFOCUS;
      sbdesc.dwBufferBytes = len;
      sbdesc.lpwfxFormat=&sbformat;
      if FAILED(lpds->CreateSoundBuffer(&sbdesc, &sbuffer, NULL))
	{
	  AddLog("DEVPDX5: Couldn't create sound buffer." CRLF);
	  SetError(1);
	  return 0;
	}
      
      wbuffer=sbuffer;
      wbformat=&sbformat;
    }

  buflen=len;
#ifdef DEBUG
  AddLog("buflen: %d" CRLF, len);
#endif
  plrGetBufPos=getpos;
  plrGetPlayPos=getpos;
  plrAdvanceTo=advance;
  plrGetTimer=gettimer;

  // alloc temp buffer and clear buffers
  if FAILED(wbuffer->Lock(0,0,&lockbuf,&locklen,0,0,DSBLOCK_ENTIREBUFFER))
    {
      wbuffer->Unlock(lockbuf,0,0,0);
      if (writeprim)
	{
	  AddLog("DEVPDX5: Couldn't lock primary buffer. Trying secondary." CRLF);
	  writeprim=0;
	  lpds->SetCooperativeLevel(hWnd,DSSCL_PRIORITY);
	  return dxpPlay(buf,len);
	}
      else
	{
	  AddLog("DEVPDX5: Couldn't lock secondary buffer. Giving up." CRLF);
	  SetError(1);
	  sbuffer->Release();
	  return 0;
	}
    }
  else
    {
      memsetb(lockbuf,0,locklen);
      wbuffer->Unlock(lockbuf,0,0,0);
    }

  // start playing
  if FAILED(wbuffer->Play(0,0,DSBPLAY_LOOPING))
    {
      if (writeprim)
	{
	  AddLog("DEVPDX5: Couldn't play on primary buffer. Trying secondary." CRLF);
	  writeprim=0;
	  lpds->SetCooperativeLevel(hWnd,DSSCL_PRIORITY);
	  return dxpPlay(buf,len);
	}
      else
	{
	  AddLog("DEVPDX5: Couldn't play on secondary buffer. Giving up." CRLF);
	  SetError(1);
	  sbuffer->Release();
	  return 0;
	}
    }

#ifdef PRE_BUFFER
  tempbuf = (char*)buf;  // new char[len];        *PREALLOCATED BUFFER*
#else
  tempbuf=(char*)VirtualAlloc(0, len, MEM_COMMIT, PAGE_READWRITE);
#endif
#ifdef DEBUG
  AddLog("my buffer is at %x" CRLF, tempbuf);
#endif
  AddLog("playing..." CRLF);
  memsetb(tempbuf,0,len);
#ifndef PRE_BUFFER
  buf=tempbuf;
#endif

  locked=0;
  playpos=-buflen;

  return 1;
}

static void dxpStop()
{
#ifdef DEBUG
  AddLog("stop();" CRLF);
#endif
  if (sbuffer)
    {
      sbuffer->Stop();
      sbuffer->Release();
    }
#ifndef PRE_BUFFER
  if (tempbuf)
    delete tempbuf;		//preallocated buffer
#else
  if (tempbuf)
    VirtualFree(tempbuf, 0, MEM_RELEASE);
#endif
  AddLog("stopped" CRLF);
  sbuffer=NULL;
  tempbuf=NULL;
}


static int dxpInit(const deviceinfo &card)
{
#ifdef DEBUG
  AddLog("dxpInit()" CRLF);
#endif

  writeprim=!!card.subtype;

  if(!lpdsOpened)
    {
      // open dsound object
      HRESULT r;
      if(FAILED(r=DirectSoundCreate(NULL,&lpds,NULL)))
	{
	  AddLog("DEVPDX5: Failed to create DirectSound-object." CRLF);
	  switch(r)
	    {
	    case DSERR_ALLOCATED: AddLog("The request failed because resources, such as a priority level, were already in use by another caller." CRLF); break;
	    case DSERR_INVALIDPARAM: AddLog("An invalid parameter was passed to the returning function." CRLF); break;
	    case DSERR_NOAGGREGATION: AddLog("The object does not support aggregation." CRLF); break;
	    case DSERR_NODRIVER: AddLog("No sound driver is available for use, or the given GUID is not a valid DirectSound device ID." CRLF); break;
	    case DSERR_OUTOFMEMORY: AddLog("The DirectSound subsystem could not allocate sufficient memory to complete the caller's request." CRLF); break;
	    default: AddLog("unknown error" CRLF); break;
	    }
	  SetError(1);
	  return 0;
	}
    }

  // set cooperative level
  if FAILED(lpds->SetCooperativeLevel(hWnd,writeprim?DSSCL_WRITEPRIMARY:DSSCL_PRIORITY))
    {
      AddLog("DEVPDX5: Failed to set cooperative level." CRLF);
      if (!writeprim)
	SetError(1);
      writeprim=0;
      if FAILED(lpds->SetCooperativeLevel(hWnd,DSSCL_PRIORITY))
		 return 0;
      else
	AddLog("DEVPDX5: Using secondary buffer instead." CRLF);
    }

  // get capabilities
  dscaps.dwSize = sizeof(DSCAPS);
  if FAILED(lpds->GetCaps(&dscaps))
    {
      AddLog("DEVPDX5: Failed to get ds-capabilities." CRLF);
      SetError(1);
      return 0;
    }

  // obtain primary buffer
  memsetb(&pbdesc, 0, sizeof(DSBUFFERDESC));
  pbdesc.dwSize  = sizeof(DSBUFFERDESC);
  pbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
  if FAILED(lpds->CreateSoundBuffer(&pbdesc, &pbuffer, NULL))
    {
      AddLog("DEVPDX5: Failed to create DirectSound-buffer." CRLF);
      SetError(1);
      return 0;
    }

  plrSetOptions=dxpSetOptions;
  plrPlay=dxpPlay;
  plrStop=dxpStop;
  return 1;
}

static void dxpClose()
{
  plrPlay=0;
  lpds->Release();
  lpdsOpened=0;
}

static int dxpDetect(deviceinfo &card)
{
#ifdef DEBUG
  AddLog("dxpDetect()" CRLF);
#endif

  // open dsound object
  if FAILED(DirectSoundCreate(NULL, &lpds, NULL))
    {
      AddLog("DEVPDX5: (detect) Failed to create DirectSound-object." CRLF);
      SetError(1);
      return 0;
    }
  lpdsOpened=1;

  // get capabilities
  dscaps.dwSize = sizeof(DSCAPS);
  if FAILED(lpds->GetCaps(&dscaps))
    {
      AddLog("DEVPDX5: (detect) Failed to get capabilities." CRLF);
      SetError(1);
      return 0;
    }

  if (card.subtype==1)
    {
      if FAILED(lpds->SetCooperativeLevel(hWnd,DSSCL_WRITEPRIMARY))
	{
	  AddLog("DEVPDX5: (detect) Failed to set primary cooperative level." CRLF);
	  return 0;
	}
    }

  card.dev=&plrDirectSound;
  card.chan=dscaps.dwFlags&DSCAPS_PRIMARYSTEREO?2:1;

  return 1;
}

extern "C" sounddevice plrDirectSound = {
  SS_PLAYER, 
    "DirectSound Player", 
    dxpDetect, 
    dxpInit, 
    dxpClose
    };
