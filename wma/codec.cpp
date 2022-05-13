/*

                wma4cp, ACM-Codec interface
                                                (c) 1999 by Felix Domke
                                                licensed in the same way
                                                as the OpenCP.
*/
#include <stdio.h>
#include "binfile.h"
#include "err.h"
#include "codec.h"

                        // .... fix this
#include "asfread.h"

#define WAVE_FORMAT_WMA         (0x160)
#define WMA_UUID_DECODE         "1A0F78F0-EC8A-11d2-BBBE-006008320064"
#define WMA_UUID_ENCODE         "F6DC9830-BC79-11d2-A9D0-006097926036"

errstat acmcodec::rawclose()
{
  DriverProc(dinst, hnd, ACMDM_STREAM_CLOSE, (long)&padsi, 0);
  delete padsi.pwfxSrc;
  delete padsi.pwfxDst;
  DriverProc(dinst, hnd, DRV_CLOSE, 0, 0);
  if (&memwma==src) memwma.close();
  return 0;
}

errstat acmcodec::open(binfile &file, WAVEFORMATEX* pfmt, DRIVERPROC drv, void *_hnd, int dobuffer)
{
  src=&file;
  DriverProc=drv;
  hnd=_hnd;
  fmtdst=pfmt;

  int err=InitDriver();
  if (err) return err;
  WAVEFORMATEX *pfmtsrc=(WAVEFORMATEX *)src->ioctl(asfreader::ioctlgetformat);        // not asfreader, more a "formatreader". later.
  if (!pfmtsrc)
  {
    printf("no source format given!\n");
    return -1;
  }
  fmtsrc=(WAVEFORMATEX*)new char[sizeof(WAVEFORMATEX)+pfmtsrc->cbSize];
  memcpy(fmtsrc, pfmtsrc, sizeof(WAVEFORMATEX)+pfmtsrc->cbSize);
  int ratio=fmtdst->nAvgBytesPerSec/fmtsrc->nAvgBytesPerSec;
  printf("compression ratio: 1:%d\n", ratio);

  if (dobuffer)
  {
    memwmab=new char[src->length()];
    if (!memwmab) return errAllocMem;
    int r;
    if (memwma.open(memwmab, r=src->read(memwmab, src->length()), mbinfile::openfree))
      return errGen;
    src=&memwma;
  }
  openmode(moderead|modeseek, 0, src->length()*ratio);

  err=InitCODEC(fmtsrc, fmtdst);
  if (err) return err;

  srclen=4*fmtsrc->nBlockAlign;
  buflen=GetLenForSrc(srclen);
  if (!srclen || !buflen) return -1;
  buf=new short[buflen];
  if (!buf) return errAllocMem;
  srcbuf=new short[srclen];
  if (!srcbuf) return errAllocMem;
  bufp=0;

  return 0;
}

errstat acmcodec::InitDriver()
{
  if (!DriverProc(0, 0, DRV_LOAD, 0, 0))
  {
    printf("DRV_LOAD failed.\n");
    return -1;
  }
  paod.cbStruct=sizeof(paod);
  paod.fccType=ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
  paod.fccComp=0;
  paod.dwVersion=3;
  paod.dwFlags=0;
  paod.dwError=0;
  paod.pszSectionName=0;
  paod.pszAliasName=0;
  paod.dnDevNode=0;

  dinst=DriverProc(0, hnd, DRV_OPEN, 0, (long)&paod);
  if (!dinst)
  {
    printf("DRV_OPEN failed.\n");
    return -1;
  }

  if (paod.dwError!=MMSYSERR_NOERROR)
  {
    printf("DRV_OPEN returned error %d.\n", paod.dwError);
    return -1;
  }

  dd.cbStruct=sizeof(ACMDRIVERDETAILS);
  if (DriverProc(dinst, hnd, ACMDM_DRIVER_DETAILS, (long)&dd, 0))
  {
    printf("ACMDM_DRIVER_DETAILS failed!\n");
    return -1;
  }
  return 0;
}

errstat acmcodec::InitCODEC(WAVEFORMATEX *src, WAVEFORMATEX *dst)
{
  padsi.cbStruct=sizeof(ACMDRVSTREAMINSTANCE);
  if (src->wFormatTag==WAVE_FORMAT_WMA) // insert the special UUID
  {
    padsi.pwfxSrc=(WAVEFORMATEX*)new char[sizeof(WAVEFORMATEX)+src->cbSize+0x29];
    memcpy(padsi.pwfxSrc, src, sizeof(WAVEFORMATEX)+src->cbSize);
    strcpy((char*)(((int)padsi.pwfxSrc)+sizeof(WAVEFORMATEX)+4), WMA_UUID_DECODE);
    padsi.pwfxSrc->cbSize=0x29;
  } else
  {
    padsi.pwfxSrc=(WAVEFORMATEX*)new char[sizeof(WAVEFORMATEX)+src->cbSize];
    memcpy(padsi.pwfxSrc, src, sizeof(WAVEFORMATEX)+src->cbSize);
  }

  if (dst->wFormatTag==WAVE_FORMAT_WMA)
  {
    padsi.pwfxDst=(WAVEFORMATEX*)new char[sizeof(WAVEFORMATEX)+dst->cbSize+0x29];
    memcpy(padsi.pwfxDst, dst, sizeof(WAVEFORMATEX)+dst->cbSize);
    strcpy((char*)(((int)padsi.pwfxDst)+sizeof(WAVEFORMATEX)+4), WMA_UUID_ENCODE);
    padsi.pwfxDst->cbSize=0x29;
  } else
  {
    padsi.pwfxDst=(WAVEFORMATEX*)new char[sizeof(WAVEFORMATEX)+dst->cbSize];
    memcpy(padsi.pwfxDst, dst, sizeof(WAVEFORMATEX)+dst->cbSize);
  }

  padsi.dwCallback=0;
  padsi.pwfltr=0;
  padsi.dwInstance=0;
  padsi.fdwOpen=ACM_STREAMOPENF_NONREALTIME;

  int err=DriverProc(dinst, hnd, ACMDM_STREAM_OPEN, (long)&padsi, 0);
  if (err!=MMSYSERR_NOERROR)
  {
    printf("Error %d occured while ACMDM_STREAM_OPEN.\n", err);
    if (err==ACMERR_NOTPOSSIBLE)
      printf("ACMERR_NOTPOSSIBLE\n");
    if (err==MMSYSERR_NOTSUPPORTED)
      printf("MMSYSERR_NOTSUPPORTED\n");
    return -1;
  }
  first=!0;
  return 0;
}

binfilepos acmcodec::rawread(void *buffer, binfilepos len)
{
  int olen=len;
  if (!buf) return 0;
  int r=0;
  while (len)
  {
    if (!bufp)
      bufp=Decode();
    if (!bufp) break;
    int l=len;
    if (bufp<l) l=bufp;
    memcpy((char*)buffer+r, buf, l);
    bufp-=l;
    memcpy(buf, ((char*)buf)+l, bufp);
    len-=l;
    r+=l;
  }
  return r;
}

binfilepos acmcodec::Decode()
{
  ACMDRVSTREAMHEADER adsh;
  adsh.cbStruct=sizeof(ACMDRVSTREAMHEADER);
  adsh.fdwStatus=ACMSTREAMHEADER_STATUSF_INQUEUE;   // ??
  adsh.dwUser=0;
  adsh.pbSrc=(unsigned char*)srcbuf;
  int rl=src->read(adsh.pbSrc, srclen);
  if (rl<fmtsrc->nBlockAlign) return 0;
  adsh.cbSrcLength=rl;
  adsh.dwSrcUser=0;
  adsh.pbDst=(unsigned char*)buf;
  adsh.cbDstLength=buflen;
  adsh.dwDstUser=0;
  adsh.dwDriver=padsi.dwDriver;
  adsh.fdwDriver=padsi.fdwDriver;

  if (first)
    adsh.fdwConvert=ACM_STREAMCONVERTF_START;
  else
    adsh.fdwConvert=0;

  first=0;
  int err=DriverProc(dinst, hnd, ACMDM_STREAM_CONVERT, (long)&padsi, (long)&adsh);
  if (err)
  {
    printf("error %d while ACMDM_STREAM_CONVERT\n", err);
    return 0;
  }

//  printf("converted %d to %d bytes.\n", adsh.cbSrcLengthUsed, adsh.cbDstLengthUsed);
  return adsh.cbDstLengthUsed;
}

int acmcodec::GetLenForSrc(int &len)
{
  ACMDRVSTREAMSIZE adss;
  adss.cbStruct=sizeof(ACMDRVSTREAMSIZE);
  adss.fdwSize=ACM_STREAMSIZEF_SOURCE;
  adss.cbSrcLength=len;
  adss.cbDstLength=0;

  int err=DriverProc(dinst, hnd, ACMDM_STREAM_SIZE, (long)&padsi, (long)&adss);
/*  if (err)
  {
    printf("error %d while ACMDM_STREAM_SIZE\n", err);
    return 0;
  } */
  if (err) return 0;

  len=adss.cbSrcLength;

  return (adss.cbDstLength);
}

binfilepos acmcodec::rawseek(binfilepos pos)
{
  bufp=0;
  int ratio=buflen/srclen;
  return src->seek((pos/buflen)*srclen)*ratio;
}
