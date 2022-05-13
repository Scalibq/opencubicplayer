// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// PACK.EXE - small utility for creating Quake .PAK files
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release

#include <stdlib.h>
#include <fstream.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

struct direntry
{
  char name[0x38];
  long off;
  long len;
};

int main(int argn, char **argv)
{
  if (argn!=2)
  {
    cout << "arg\n";
    return 1;
  }
  char *src=argv[1];
  char path[_MAX_PATH];
  char drive[_MAX_DRIVE];
  char dirn[_MAX_DIR];
  char name[_MAX_FNAME];
  char ext[_MAX_EXT];
  _splitpath(src, drive, dirn, name, ext);
  _makepath(path, drive, dirn, name, ".pak");
  char *dest=path;
  ifstream lst(src);
  int nfiles=0;
  direntry *dir=0;
  int off=12;
  int i;
  while (1)
  {
    char name[100];
    lst >> name;
    if (!lst.good())
      break;
    strlwr(name);
    int l=strlen(name);
    for (i=0; i<l; i++)
      if (name[i]=='\\')
        name[i]='/';

    direntry *olddir=dir;
    dir=new direntry [nfiles+1];
    memcpy(dir, olddir, 0x40*nfiles);
    delete olddir;
    strncpy(dir[nfiles].name, name, 0x38);

    for (i=0; i<l; i++)
      if (name[i]=='/')
        name[i]='\\';
    int f=open(name, O_RDONLY|O_BINARY);
    dir[nfiles].len=filelength(f);
    close(f);
    dir[nfiles].off=off;
    off+=dir[nfiles].len;
    nfiles++;
  }
  int df=open(dest, O_BINARY|O_WRONLY|O_TRUNC|O_CREAT, S_IREAD|S_IWRITE);
  write(df, "PACK", 4);
  write(df, &off, 4);
  off=nfiles*0x40;
  write(df, &off, 4);

  for (i=0; i<nfiles; i++)
  {
    char name[100];
    strcpy(name, dir[i].name);
    int l=strlen(name);
    int j;
    for (j=0; j<l; j++)
      if (name[j]=='/')
        name[j]='\\';
    int f=open(name,O_RDONLY|O_BINARY);
    if (f<0)
    {
      cout << dir[i].name << ": not found\n";
      return 1;
    }
    char *buf=new char [dir[i].len];
    if (!buf)
    {
      cout << dir[i].name << ": no mem\n";
      return 1;
    }
    read(f, buf, dir[i].len);
    write(df, buf, dir[i].len);
    delete buf;
    close(f);
  }

  write(df, dir, nfiles*0x40);
  close(df);
  return 0;
}
