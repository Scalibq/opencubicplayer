// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Stub loader replacement
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -fd981014   Felix Domke <tmbinc@gmx.net>
//    -changed a cast about "execv"

#include <dos.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <stdio.h>

static int fileexist(const char *name)
{
  find_t f;
  return !_dos_findfirst(name, _A_NORMAL, &f);
}

static void searchlist(char *dest, const char *name, const char *path)
{
  if (!path)
    path="";
  while (*path)
  {
    if ((*path==' ')||(*path==';')||(*path=='\t'))
    {
      path++;
      continue;
    }
    const char *pe=path;
    while (*pe&&(*pe!=';'))
      pe++;
    memcpy(dest, path, pe-path);
    dest[pe-path]=0;
    path=pe;
    if (dest[strlen(dest)]!='\\')
      strcat(dest, "\\");
    strcat(dest, name);
    if (fileexist(dest))
      return;
  }
  *dest=0;
}

int main(int argc, char **argv)
{
  printf("opencp   (c) 1994-98 Niklas Beisert et al.\n"
         "  type cp -h for command line help\n\n");

  int i;

  char dos4gpath[90];
  if (getenv("CPEXTENDER"))
    strcpy(dos4gpath, getenv("CPEXTENDER"));
  else
    *dos4gpath=0;

  if (!*dos4gpath)
  {
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    _splitpath(argv[0], drive, dir, 0, 0);
    _makepath(dos4gpath, drive, dir, "DOS4GW", ".EXE");
    if (!fileexist(dos4gpath))
    {
      _makepath(dos4gpath, drive, dir, "DOS4G", ".EXE");
      if (!fileexist(dos4gpath))
        *dos4gpath=0;
    }
  }

  if (!*dos4gpath)
    searchlist(dos4gpath, "DOS4GW.EXE", getenv("DOS4GPATH"));
  if (!*dos4gpath)
    searchlist(dos4gpath, "DOS4G.EXE", getenv("DOS4GPATH"));
  if (!*dos4gpath)
    searchlist(dos4gpath, "DOS4GW.EXE", getenv("PATH"));
  if (!*dos4gpath)
    searchlist(dos4gpath, "DOS4G.EXE", getenv("PATH"));

  if (!dos4gpath || !*dos4gpath)
  {
    printf("Sorry, this programm needs DOS4GW.EXE to run\n");
    return 1;
  }

  char fname[_MAX_FNAME];
  _splitpath(dos4gpath, 0, 0, fname, 0);
  if (!memicmp(fname, "DOS4G", 5))
    putenv("DOS4G=QUIET");

  char cpexepath[_MAX_PATH];
  char *cpexe=getenv("CPEXE");
  if (!cpexe)
    strcpy(cpexepath, argv[0]);
  else
  {
    strcpy(cpexepath, cpexe);
    if (cpexepath[strlen(cpexepath)-1]!='\\')
      strcat(cpexepath, "\\");
    strcat(cpexepath, "cp.exe");
  }

  char cmdline[170];
  getcmd(cmdline);
  char *cmdp=cmdline;
  while ((*cmdp==' ')||(*cmdp=='\t'))
    cmdp++;
  char *envcmd=getenv("CMDLINE");
  if (envcmd)
  {
    while ((*envcmd==' ')||(*envcmd=='\t'))
      envcmd++;
    if (*envcmd)
      cmdp=envcmd;
  }
  int cmdlen=strlen(cmdp);
  char *cmdbuf=new char [cmdlen+strlen(cpexepath)+strlen(dos4gpath)+11];
  strcpy(cmdbuf, "CMDLINE=");
  strcat(cmdbuf, dos4gpath);
  strcat(cmdbuf, " ");
  char *newcmd=cmdbuf+strlen(cmdbuf);
  strcat(cmdbuf, cpexepath);
  strcat(cmdbuf, " ");
  strcat(cmdbuf, cmdp);
  putenv(cmdbuf);
  strncpy(cmdline, newcmd, 126);
  cmdline[126]=0;

  printf("loading player...\n");

  char * a[5];
  char **b=a;
  *b++=dos4gpath;
  *b++=cmdline;
  *b++=0;

  execv(dos4gpath, (const char * *)a);
  printf("could not execute %s: %s\n", dos4gpath, strerror(errno));
  return 1;
}