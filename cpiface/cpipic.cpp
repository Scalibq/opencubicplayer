// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIFace background picture loader
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -some minor changes
//  -doj980928  Dirk Jagdmann <doj@cubic.org>
//    -wrote a more compatible TGA loader which is found in the file tga.cpp
//    -changed this file to meet dependencies from the new TGA loader
//    -added comments
//  -doj980930  Dirk Jagdmann <doj@cubic.org>
//    -added gif loader
//  -doj981105  Dirk Jagdmann <doj@cubic.org>
//    -now the plOpenCPPict is cleared if no file is found / valid
//    -modified memory allocation a bit to remove unnecessary new/delete
//  -fd981119   Felix Domke <tmbinc@gmx.net>
//    -added the really important 'NO_CPIFACE_IMPORT'

#define NO_CPIFACE_IMPORT
#ifdef __WATCOMC__
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#endif
#ifndef __WATCOMC__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include "psetting.h"

#include "gif.h"
#include "tga.h"

unsigned char *plOpenCPPict=0; // an array containing the raw picture
unsigned char plOpenCPPal[768]; // the palette for the picture
extern char cfDataDir[]; // directory where pictures are stored

static const int picWidth=640;
static const int picHeight=384;

void plReadOpenCPPic()
{
  // get the resource containing background pictures
  const char *picstr=cfGetProfileString2(cfScreenSec, "screen", "usepics", "");

  // n contains the number of background pictures
  static int lastN=-1;
  int n=cfCountSpaceList(picstr, 12);
  // if there are no background pictures we can skip the rest
  if (n==0) return;
  // choose a new picture
  n=rand()%n;
  if(n==lastN) return; // we have loaded that picture already
  lastN=n;

  // get the according filename
  int i;
  char name[13];
  for (i=0; i<=n; i++)
    cfGetSpaceListEntry(name, picstr, 12);

  char path[_MAX_PATH];
  sprintf(path, "%s%s", cfDataDir, name);

  // allocate memory for a new picture
  if(plOpenCPPict==0) {
    plOpenCPPict=new unsigned char [picWidth*picHeight];
    if(plOpenCPPict==0) return;
    for(i=0; i<picWidth*picHeight; i++)
      plOpenCPPict[i]=0;
  }

  // read the file into a filecache
  int file=open(path, O_RDONLY|O_BINARY);
  if(file<0) return ;
  int filesize=lseek(file, 0, SEEK_END);
  if(filesize==-1) {
    close(file);
    return ;
  }
  if(lseek(file, 0, SEEK_SET)==-1) {
    close(file);
    return ;
  }
  unsigned char *filecache=new unsigned char[filesize];
  if(filecache==0) {
    close(file);
    return ;
  }
  if(read(file, filecache, filesize)!=filesize) {
    delete [] filecache;
    close(file);
    return ;
  }
  close(file);

  // read the picture
  GIF87read(filecache, filesize, plOpenCPPict, plOpenCPPal, picWidth, picHeight);
  TGAread(filecache, filesize, plOpenCPPict, plOpenCPPal, picWidth, picHeight);

  delete [] filecache;

  // determine if the lower or upper part of the palette is left blank for
  // our own screen colors
  int low=0;
  int high=0;

  for (i=0; i<picWidth*picHeight; i++)
    if (plOpenCPPict[i]<0x30)
      low=1;
    else
    if (plOpenCPPict[i]>=0xD0)
      high=1;

  int move=(low&&!high)*0x90;

  // if the color indices are bad, we have to move every color map entry
  if (move)
    for (i=0; i<picWidth*picHeight; i++)
      plOpenCPPict[i]+=0x30;

  // adjust the RGB palette to 6bit, because standard VGA is limited
  for (i=0x2FD; i>=0x90; i--)
    plOpenCPPal[i]=plOpenCPPal[i-move]>>2;
}