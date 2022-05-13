// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// ITPlay .IT module loader
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <kb@nwn.de>
//   -many many many many small memory allocation bugs fixed. the heap looked
//    more like a swiss cheese than like anything else after the player had
//    run. but now, everything's initialised, alloced and freed correctly
//    (i hope).
//   -added IT2.14 decompression. Thanks to daniel for handing me that
//    not-fully-working source which i needed to completely find out the
//    compression (see ITSEX.CPP).
//   -added MIDI command and filter setting reading. This will become use-
//    ful as soon i have a software mixing routine with filters, at the
//    moment its only use is to prevent confusion of filter and pitch
//    envelopes

#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <stdio.h>
#include "itplay.h"
#include "mcp.h"
#include "err.h"
#include "binfile.h"

int itplayerclass::module::load(binfile &file)
{
  int i,j,k,n;

  nchan=0;
  ninst=0;
  nsampi=0;
  nsamp=0;
  npat=0;
  nord=0;
  orders=0;
  patlens=0;
  patterns=0;
  samples=0;
  instruments=0;
  sampleinfos=0;
  midicmds=0;
  deltapacked=0;
  message=0;

  struct
  {
    unsigned long sig;
    unsigned char name[26];
    unsigned short philite;
    unsigned short nords;
    unsigned short nins;
    unsigned short nsmps;
    unsigned short npats;
    unsigned short cwtv;
    unsigned short cmwt;
    unsigned short flags;
    unsigned short special;
    unsigned char gvol;
    unsigned char mvol;
    unsigned char ispd;
    unsigned char itmp;
    unsigned char chsep;
    unsigned char pwd;
    unsigned short msglen;
    unsigned long msgoff;
    unsigned long _d3;
    unsigned char pan[64];
    unsigned char vol[64];
  } hdr;

  file.read(&hdr, sizeof(hdr));
  if (hdr.sig!=0x4D504D49)
    return errFormSig;

  if ((hdr.flags&4)&&(hdr.cmwt<0x200))
    return errFormOldVer;

  char signedsamp=!(hdr.cwtv<0x202);
  deltapacked=(hdr.cmwt==0x215);


  unsigned char ords[256];
  unsigned long sampoff[100];
  unsigned long insoff[100];
  unsigned long patoff[200];

  file.read(ords, hdr.nords);
  file.read(insoff, hdr.nins*4);
  file.read(sampoff, hdr.nsmps*4);
  file.read(patoff, hdr.npats*4);

  memcpy(name, hdr.name, 26);
  for ( n=0; n<26; n++ )
    if (!name[n])
      name[n]=32;
  name[26]=0;
  linearfreq=!!(hdr.flags&8);

  inispeed=hdr.ispd;
  initempo=hdr.itmp;
  inigvol=hdr.gvol;

  chsep=(hdr.flags&1)?hdr.chsep:0;
  instmode=hdr.flags&4;
  linear=hdr.flags&8;
  oldfx=hdr.flags&16;
  geffect=hdr.flags&32;
  instmode=hdr.flags&2;

  memcpy(inipan, hdr.pan, 64);
  memcpy(inivol, hdr.vol, 64);

  if (hdr.special&4)
  {
    short usage;
    char  dummy[8];
    file.read(&usage,2);
    for (i=0; i<usage; i++)
      file.read(dummy,8);
  }


  if (hdr.special&8)
  {
    midicmds=new char *[9+16+128];

    char cmd[33];
    memset(cmd,0,sizeof(cmd));

    for (i=0; i<(9+16+128); i++)
    {
      file.read(cmd,32);
      for ( n=0; n<32; n++ )
        if (!cmd[n])
          cmd[n]=32;
      midicmds[i]= new char[strlen(cmd)+1];
      strcpy(midicmds[i],cmd);
      strupr(midicmds[i]);
    }
  }

  if (hdr.special&1)
  {
    char *msg=new char [hdr.msglen+1];
    if (!msg)
      return errAllocMem;
    file.seek(hdr.msgoff);
    file.read(msg, hdr.msglen);
    msg[hdr.msglen]=0;
    int linect=1;
    for (i=0; i<hdr.msglen; i++)
    {
      if (msg[i]==0)
        break;
      if (msg[i]==13)
        linect++;
    }
    message=new char *[linect+1];
    if (!message)
      return errAllocMem;
    *message=msg;
    linect=1;
    for (i=0; i<hdr.msglen; i++)
    {
      if (msg[i]==0)
        break;
      if (msg[i]==13)
      {
        msg[i]=0;
        message[linect++]=msg+i+1;
      }
    }
    message[linect]=0;
  }
  npat=hdr.npats+1;

  nord=hdr.nords;
  for (i=nord-1; i>=0; i--)
  {
    if (ords[i]<254)
      break;
    nord--;
  }
  if (!nord)
    return errFormMiss;

  for (i=0; i<nord; i++)
    if (ords[i]==255)
      break;
  for (i=i; i>0; i--)
    if (ords[i-1]!=254)
      break;
  endord=i;

  orders=new unsigned short [nord];

  if (!orders)
    return errAllocMem;

  for (i=0; i<nord; i++)
    orders[i]=(ords[i]==254)?0xFFFF:(ords[i]>=hdr.npats)?hdr.npats:ords[i];

  patlens=new unsigned short [npat];
  patterns=new unsigned char *[npat];

  if (!patlens||!patterns)
    return errAllocMem;

  patlens[npat-1]=64;
  patterns[npat-1]=new unsigned char [64];

  if (!patterns[npat-1])
    return errAllocMem;

  memset(patterns[npat-1], 0, 64);

  int maxchan=0;

  for (k=0; k<hdr.npats; k++)
  {
    if (!patoff[k])
    {
      patlens[k]=64;
      patterns[k]=new unsigned char [patlens[k]];
      if (!patterns[k])
        return errAllocMem;
      memset(patterns[k], 0, patlens[k]);
      continue;
    }
    file.seek(patoff[k]);

    unsigned short patlen, patrows;
    file.read(&patlen,2);
    file.read(&patrows,2);
    file.seekcur(4);

    unsigned char *patbuf=new unsigned char [patlen];
    if (!patbuf)
      return errAllocMem;

    file.read(patbuf, patlen);

    patlens[k]=patrows;
    patterns[k]=new unsigned char [(6*64+1)*patlens[k]];
    if (!patterns[k])
      return errAllocMem;

    unsigned char lastmask[64]={0};
    unsigned char lastnote[64]={0};
    unsigned char lastins[64]={0};
    unsigned char lastvolpan[64]={0};
    unsigned char lastcmd[64]={0};
    unsigned char lastdata[64]={0};
    unsigned char *pp=patbuf;
    unsigned char *wp=patterns[k];
    for (i=0; i<patlens[k]; i++)
    {
      while (1)
      {
        int chvar=*pp++;
        if (!chvar)
          break;
        char chn=(chvar-1)&63;

        if (chvar&128)
          lastmask[chn]=*pp++;
        int c=lastmask[chn];
        if (!c)
          continue;
        if (maxchan<=chn)
          maxchan=chn+1;
        if (c&1)
        {
          lastnote[chn]=(*pp<=120)?(*pp+1):*pp;
          pp++;
        }
        if (c&2)
          lastins[chn]=*pp++;
        if (c&4)
        {
          lastvolpan[chn]=1+*pp++;
        }
        if (c&8)
        {
          lastcmd[chn]=*pp++;
          lastdata[chn]=*pp++;
        }
        *wp++=chn+1;
        *wp++=(c&0x11)?lastnote[chn]:0;
        *wp++=(c&0x22)?lastins[chn]:0;
        *wp++=(c&0x44)?lastvolpan[chn]:0;
        *wp++=(c&0x88)?lastcmd[chn]:0;
        *wp++=(c&0x88)?lastdata[chn]:0;
      }
      *wp++=0;
    }
    delete patbuf;
    patterns[k]=(unsigned char*)realloc(patterns[k], wp-patterns[k]);

  }

  if (!maxchan)
    return errFormSupp;
  nchan=maxchan;

  nsampi=hdr.nsmps;
  nsamp=hdr.nsmps;

  sampleinfos= new sampleinfo [nsampi];
  samples=new sample [nsamp];
  if (!sampleinfos||!samples)
    return errAllocMem;
  memset(sampleinfos, 0, nsampi*sizeof(sampleinfo));
  memset(samples, 0, nsamp*sizeof(sample));

  for (i=0; i<hdr.nsmps; i++)
    samples[i].handle=0xFFFF;

  for (i=0; i<hdr.nsmps; i++)
  {
    struct
    {
      unsigned long sig;
      char dosname[13];
      unsigned char gvl;
      unsigned char flags;
      unsigned char vol;
      char name[26];
      unsigned char cvt;
      unsigned char dfp;
      unsigned long length;
      unsigned long loopstart;
      unsigned long loopend;
      unsigned long c5spd;
      unsigned long sloopstart;
      unsigned long sloopend;
      unsigned long off;
      unsigned char vis;
      unsigned char vid;
      unsigned char vir;
      unsigned char vit;
    } shdr;
    file.seek(sampoff[i]);
    file.read(&shdr, sizeof(shdr));

    sampoff[i]=shdr.off;
    strupr(shdr.dosname);

    sampleinfo &sip=sampleinfos[i];
    sample &sp=samples[i];

    memcpy(sp.name, shdr.name, 26);
    for ( n=0; n<26; n++ )
      if (!sp.name[n])
        sp.name[n]=32;
    sp.name[26]=0;

    if (!(shdr.flags&1))
      continue;

    sp.vol=shdr.vol;
    sp.gvl=shdr.gvl;
    sp.vir=shdr.vir;
    sp.vid=shdr.vid;
    sp.vit=shdr.vit;
    sp.vis=shdr.vis;
    sp.dfp=shdr.dfp;
    sp.handle=i;
    sp.normnote=-mcpGetNote8363(shdr.c5spd);
    sip.length=shdr.length;
    sip.samprate=shdr.c5spd>>((shdr.flags&2)?1:0);
    sip.samprate=8363;

    sip.loopstart=shdr.loopstart;
    sip.loopend=shdr.loopend;
    sip.sloopstart=shdr.sloopstart;
    sip.sloopend=shdr.sloopend;
    sip.type=((shdr.flags&2)?mcpSamp16Bit:0)|((shdr.flags&4)?mcpSampStereo:0)|(signedsamp?0:mcpSampUnsigned)|((shdr.flags&0x10)?mcpSampLoop:0)|((shdr.flags&0x40)?mcpSampBiDi:0)|((shdr.flags&0x80)?mcpSampSBiDi:0)|((shdr.flags&0x20)?mcpSampSLoop:0);

    sp.packed=(shdr.flags&8?1:0);

    sip.ptr=new char [(sip.length+512)<<((sip.type&mcpSamp16Bit)?1:0)];
    if (!sip.ptr)
      return errAllocMem;
  }

  for (i=0; i<hdr.nsmps; i++)
  {
    sampleinfo &sip=sampleinfos[i];
    if (!sip.ptr)
      continue;
    sample &sp=samples[i];
    file.seek(sampoff[i]);

    if (sp.packed) {
     if (sip.type & mcpSamp16Bit)
       decompress16(file, sip.ptr, sip.length, deltapacked);
     else
       decompress8(file, sip.ptr, sip.length, deltapacked);
    }
    else
      file.read(sip.ptr, sip.length<<((sip.type&mcpSamp16Bit)?1:0));

  }

  ninst=(hdr.flags&4)?hdr.nins:hdr.nsmps;
  instruments=new instrument [ninst];
  if (!instruments)
    return errAllocMem;
  memset(instruments, 0, ninst*sizeof(*instruments));

  for (k=0; k<ninst; k++)
  {
    struct envp
    {
      signed char v;
      unsigned short p;
    };
    struct env
    {
      unsigned char flags;
      unsigned char num;
      unsigned char lpb;
      unsigned char lpe;
      unsigned char slb;
      unsigned char sle;
      envp pts[25];
      unsigned char _d1;
    };
    struct itiheader
    {
      unsigned long sig;
      char dosname[13];
      unsigned char nna;
      unsigned char dct;
      unsigned char dca;
      unsigned short fadeout;
      unsigned char pps;
      unsigned char ppc;
      unsigned char gbv;
      unsigned char dfp;
      unsigned char rv;
      unsigned char rp;
      unsigned short tver;
      unsigned char nos;
      unsigned char _d3;
      char name[26];
      unsigned char ifp;
      unsigned char ifr;
      unsigned char mch;
      unsigned char mpr;
      unsigned short midibnk;
      unsigned char keytab[120][2];
      env envs[3];
    };
    itiheader ihdr;

    if (hdr.flags&4)
    {
      file.seek(insoff[k]);
      file.read(&ihdr, sizeof(ihdr));
    }
    else
    {
      memset(&ihdr, 0, sizeof(ihdr));
      for (i=0; i<120; i++)
      {
        ihdr.keytab[i][0]=i;
        ihdr.keytab[i][1]=k+1;
      }
      memcpy(ihdr.name, samples[k].name, 26);
      ihdr.name[26]=0;
      ihdr.dfp=0x80;
      ihdr.gbv=128;
    }
    instrument &ip=instruments[k];
    memcpy(ip.name, ihdr.name, 26);
    for ( n=0; n<26; n++ )
      if (!ip.name[n])
        ip.name[n]=32;
    ip.name[26]=0;
    ip.name[27]=0;
    ip.name[28]=0;
    ip.handle=k;
    ip.fadeout=ihdr.fadeout;
    ip.nna=ihdr.nna;
    ip.dct=ihdr.dct;
    ip.dca=ihdr.dca;
    ip.pps=ihdr.pps;
    ip.ppc=ihdr.ppc;
    ip.gbv=ihdr.gbv;
    ip.dfp=ihdr.dfp;
    ip.ifp=ihdr.ifp;
    ip.ifr=ihdr.ifr;
    ip.mch=ihdr.mch;
    ip.mpr=ihdr.mpr;
    ip.midibnk=ihdr.midibnk;
    ip.rv=ihdr.rv;
    ip.rp=ihdr.rp;

    memcpy(ip.keytab, ihdr.keytab, 120*2);

    for (i=0; i<3; i++)
    {
      env &ie=ihdr.envs[i];
      envelope &e=ip.envs[i];
      e.type=((ie.flags&1)?e.active:0)|
             ((ie.flags&2)?e.looped:0)|
             ((ie.flags&4)?e.slooped:0)|
             ((ie.flags&8)?e.carry:0)|
             ((ie.flags&128)?e.filter:0);
      e.len=ie.num-1;
      e.sloops=ie.slb;
      e.sloope=ie.sle;
      e.loops=ie.lpb;
      e.loope=ie.lpe;

      for (j=0; j<ie.num; j++)
      {
        e.y[j]=ie.pts[j].v;
        e.x[j]=ie.pts[j].p;
      }
    }
  }

  return 0;
}

void itplayerclass::module::free()
{
  int i;

  if (sampleinfos)
  {
    for (i=0; i<nsampi; i++)
      delete sampleinfos[i].ptr;
    delete sampleinfos;
  }
  delete samples;
  delete instruments;
  if (patterns)
  {
    for (i=0; i<npat; i++)
      delete patterns[i];
    delete patterns;
  }
  delete patlens;
  delete orders;
  if (message)
  {
    delete *message;
    delete message;
  }
  if (midicmds)
  {
    for (i=0; i<(9+16+128); i++)
      delete midicmds[i];
    delete midicmds;
  }
}
