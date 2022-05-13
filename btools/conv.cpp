// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// build tool: converts .EXPs to .LIBs using WLIB
//
// revision history: (please note changes here)
//  -fd981014   Felix Domke <tmbinc@gmx.net>
//    -first release

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#define WLIB_LIMIT      50
                                /* sometimes WLIB crashes when the response-
                                   file is too big. every 50 symbols WLIB
                                   is called with a partitial response-file
                                   to expand the library. */

void main(int argc, char **argv)
{
 if((argc!=3)&&(argc!=4))
 {
  printf("Usage: %s <.exp> <.lib> [module]\n"
         "  to build the .lib from the .exp (using either the .lib-filename without\n"
         "  .lib as modulename, or \"module\", if specified.\n", *argv);
  exit(1);
 }
 char module[_MAX_FNAME];
 if(argc==3)
  _splitpath(argv[2], 0, 0, module, 0);
 else
  strcpy(module, argv[3]);
 FILE *src, *dst=0;
 if(!(src=fopen(argv[1], "rt")))
 {
  perror(argv[1]);
  exit(2);
 }
 remove(argv[2]);
 char *tempfile="TEMP.LBC";              // todo: get a not-used filename... (I know, there IS a function, but I can't remember the name :)
 int c=0;
 while(!feof(src))
 {
  if((!dst)&&(!(dst=fopen(tempfile, "wt"))))
  {
   perror(tempfile);
   fclose(src);
   exit(3);
  }
  char line[100]="";
  fgets(line, 100, src);
  if(!strlen(line)) continue;
  if(line[0]=='#') continue;
  if(line[0]==';') continue;
  char symbol[100];
  if(line[0]=='\'')
   strcpy(symbol, line+1);
  else
   strcpy(symbol, line);
  if(strlen(symbol)&&(symbol[strlen(symbol)-1]=='\n')) symbol[strlen(symbol)-1]=0;
  if(strlen(symbol)&&(symbol[strlen(symbol)-1]=='\r')) symbol[strlen(symbol)-1]=0;
  if(strlen(symbol)&&(symbol[strlen(symbol)-1]=='\'')) symbol[strlen(symbol)-1]=0;
  if(!strlen(symbol)) continue;
  fprintf(dst, "++%s.%s\n", symbol, module);
  if(c++>WLIB_LIMIT)
  {
   char wlib_command[100];
   sprintf(wlib_command, "wlib -q -b %s @%s", argv[2], tempfile);
   //printf("%s\n", wlib_command);
   fclose(dst);
   system(wlib_command);
   remove(tempfile);
   dst=0;
   c=0;
  }
 }
 if(dst)
 {
  char wlib_command[100];
  sprintf(wlib_command, "wlib -q -b %s @%s", argv[2], tempfile);
  //printf("%s\n", wlib_command);
  fclose(dst);
  if(c) system(wlib_command);
  remove(tempfile);
 }
 fclose(src);
}
