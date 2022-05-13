/*

                wma4cp, AFS-reader
                                                (c) 1999 by Felix Domke
                                                licensed in the same way
                                                as the OpenCP.

        TODO:
          better error handling
          allow seeking
          complete rewrite
*/
// #define ASFDEBUG

#include <conio.h>
#include <stdio.h>
#include "binfile.h"
#include "asfread.h"

#define INITGUID
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <basetyps.h>
#include <objbase.h>
/*
   this is a HACK. but i don't care. RYG has to rewrite this, yes.


   (it parses some streams, at least all i have. send me bad streams,
    but PLEASE not the whole stream, only the beginning (<100k). tnx.)
*/

void GUID2String(char *, GUID);

DEFINE_GUID (CLSID_CAsfHeaderObjectV0, 0x75B22630, 0x668E, 0x11CF, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C);
DEFINE_GUID (CLSID_CAsfPropertyObjectV2, 0x8cabdca1, 0xa947, 0x11cf, 0x8e, 0xe4, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65);
DEFINE_GUID (CLSID_CAsfClockObject0, 0x5fbf03b5, 0xa92e, 0x11cf, 0x8e, 0xe3, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65);
DEFINE_GUID (CLSID_CStreamPropertiesObjectV1, 0xb7dc0791, 0xa9b7, 0x11cf, 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65);
DEFINE_GUID (CLSID_CAsfCodecObject, 0x86d15240, 0x311d, 0x11d0, 0xa3, 0xa4, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6);
DEFINE_GUID (CLSID_CASFDataObjectV0, 0x75b22636, 0x668e, 0x11cf, 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c);


errstat asfreader::open(binfile &file)
{
#ifdef ASFDEBUG
  printf("opening asf.\n");
#endif
  src=&file;
  src->seek(0);
  format=0;
  bread=0;
  rbread=0;
  btg=0;
  error=0;
  blen=packets=0;
  data=0;
  while ((!format) || (!data) && (!src->eof()) && (!kbhit()) && (!error))
    ReadChunk();
#ifdef ASFDEBUG
  printf("format: %x, data: %x\n", format, data);
#endif
  if ((!format) || (!data) || error) return -1;
#ifdef ASFDEBUG
  printf("blocklen: %d. packets: %d\n", blen, packets);
#endif
  npackets=packets;
  openmode(moderead|modeseek, 0, packets*blen);
  return 0;
}

binfilepos asfreader::rawseek(binfilepos pos)
{
//  printf("asfreader: seeking to %d\n", pos);
  if (!pos)
  {
    src->seek(data);
    btg=bread=rbread=0;
    packets=npackets;
  }
  return rbread;
}

errstat asfreader::rawclose()
{
  if (format) delete[] (char*)format;
  return 0;
}

void asfreader::ReadChunk()
{
#ifdef ASFDEBUG
  printf("@%x (", src->tell());
#endif
  GUID guid;
  if (!src->eread(&guid, sizeof(GUID)))
    return;
#ifdef INT8_SUPPORTED
  uint8 size;
  if (!src->eread(&size, 8))
    return;
#else
  uint4 size;
  if (!src->eread(&size, 4))
    return;
  src->seekcur(4);
#endif

#ifdef ASFDEBUG
  printf("%d bytes)\n", size);
#endif
  if ((size<24) || (size>src->length()))
  {
    error=!0;
    return;
  }

  if (IsEqualGUID(guid, CLSID_CAsfHeaderObjectV0))
  {
#ifdef ASFDEBUG
    printf("CLSID_CAsfHeaderObjectV0\n");
#endif
    uint4 num;
    if (!src->eread(&num, 4))
      return;
    src->seekcur(2);
#ifdef ASFDEBUG
    printf("------------------\n");
#endif
    for(int i=0; i<num; i++)
      ReadChunk();
#ifdef ASFDEBUG
    printf("------------------\n");
#endif
  } else if (IsEqualGUID(guid, CLSID_CAsfPropertyObjectV2))
  {
#ifdef ASFDEBUG
    printf("CLSID_CAsfPropertyObjectV2\n");
#endif
    src->seekcur(size-24);
  } else if (IsEqualGUID(guid, CLSID_CAsfClockObject0))
  {
#ifdef ASFDEBUG
    printf("CLSID_CAsfClockObject0\n");
#endif
    src->seekcur(size-24);
  } else if (IsEqualGUID(guid, CLSID_CStreamPropertiesObjectV1))
  {
#ifdef ASFDEBUG
    printf("CLSID_CStreamPropertiesObjectV1\n");
#endif
    src->seekcur(0x36);
    if (format) delete[] (char*)format;
    WAVEFORMATEX wmx;
    if (!src->eread(&wmx, sizeof(WAVEFORMATEX)))
      return;
    if (wmx.cbSize>0x100) return;     // something wrong?!?
    format=(WAVEFORMATEX*)new char[sizeof(WAVEFORMATEX)+wmx.cbSize];
    if (!format) return;
    memcpy(format, &wmx, sizeof(wmx));
    if (!src->eread(format+1, wmx.cbSize))
      return;
    src->seekcur(size-24-0x36-sizeof(WAVEFORMATEX)-wmx.cbSize);
  } else if (IsEqualGUID(guid, CLSID_CAsfCodecObject))
  {
#ifdef ASFDEBUG
    printf("CLSID_CAsfCodecObject\n");
#endif
    src->seekcur(size-24);
  } else if (IsEqualGUID(guid, CLSID_CASFDataObjectV0))
  {
#ifdef ASFDEBUG
    printf("CLSID_CASFDataObjectV0\n");
#endif
    GUID unknown;
    src->read(&unknown, 16);
#ifdef INT8_SUPPORTED
    uint8 _packets;
    src->read(&_packets, 8);
#else
    uint4 _packets;
    src->read(&_packets, 4);
    src->seekcur(4);
#endif
    packets=_packets;
    data=src->tell()+2;
    int pos=src->tell();
    src->seekcur(5);
    int t=src->getc()&8;
    src->seekcur(t?15:11);
#ifdef ASFDEBUG
    printf("%x\n", src->tell());
#endif
    blen=src->getl();
    if (blen>0x10000) blen=(blen>>8)&0xFF;       // HACK. SUXX.
#ifdef ASFDEBUG
    printf("blocklen: %d\n", blen);
#endif
    src->seek(pos+size-48);
  } else
  {
    char buffer[100];
    GUID2String(buffer, guid);
#ifdef ASFDEBUG
    printf("GUID: %s\n", buffer);
#endif
    src->seekcur(size-24);
  }
}

binfilepos asfreader::rawioctl(intm code, void *buf, binfilepos len)
{
  switch (code)
  {
    case(ioctlgetformat): return((binfilepos)format);
    case(ioctlrtell): return bread;
    case(ioctlrreof): return src->eof();
    default: return binfile::rawioctl(code, buf, len);
  }
}

binfilepos asfreader::rawread(void *buf, binfilepos len)
{
                                /// arg, todo: error checking!!!
//  printf("should read %d bytes, btg %d.\n", len, btg);
//  printf("seeking to %x\n", data+bread);
  src->seek(data+bread);
  int r=0;
  while (len)
  {
    int r1=len;

    if (r1>btg)
      r1=btg;
//    printf("reading %d bytes..\n", r1);
    r+=src->read(buf, r1);
    bread+=r1;
    buf=((char*)buf)+r1;
    btg-=r1;
    len-=r1;
    if (!btg)
    {
      char c;
      packets--;
      if (packets<0) break;
//      printf("out of bytes at %x\n", src->tell());
//      printf(" skipping [");
      while ((c=src->getc())==0)
      {
        /*printf("."); */
        bread++;
        if (src->eof())
          break;
      }
//      printf("]\n");
      bread++;
      if (c!=0x82)
      {
//        printf("ARG. Stream out of sync at %x (no 0x82)\n", src->tell());
        return 0;
      }
//      btg=1116;
      btg=blen;

      src->gets();
      bread+=2;

      int tseek=(src->getuc()&0x8)?23:19;
      bread++;
      src->seekcur(tseek);
      bread+=tseek;
//      printf("%x\n", src->tell());
//      printf("reading next header (at %x)\n", src->tell());
//      bread+=src->read(&hdr, 23);
//      btg=hdr.blen;
//      printf("BLOCK LEN %d\n", btg);
    }
  }
//  printf("ended up at %x\n", src->tell());
//  printf("read %d bytes.\n", r);
  rbread+=r;
  return r;
}

void GUID2String(char *dst, GUID src)
{
/*  sprintf(dst, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
    src.Data1, src.Data2, src.Data3, src.Data4[0], src.Data4[1], src.Data4[2], src.Data4[3], src.Data4[4], src.Data4[5], src.Data4[6], src.Data4[7]);  */
  sprintf(dst, "0x%08x, 0x%04x, 0x%04x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x",
    src.Data1, src.Data2, src.Data3, src.Data4[0], src.Data4[1], src.Data4[2], src.Data4[3], src.Data4[4], src.Data4[5], src.Data4[6], src.Data4[7]); 
}

