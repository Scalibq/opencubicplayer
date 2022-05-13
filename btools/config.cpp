#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>

#define MAXLINE 256

char *trim(char *str);
char *xtrim(char *str);

/*
    there are:
          - dlls which are compiled only under one os
          - dlls which require different objs under different os/compilers
          - generally different build-ways
          - different defines


a temp makefile:

$:WATCOM$               means "only in watcom"
$:!WATCOM$              means "not in watcom"
$:WATCOM|GCC$           means "watcom or gcc"
$:WATCOM106$            means "only in watcom, version 10.6"
$:WATCOM11$             means "only in watcom, version 11" etc.
$:$                     is end of this if-block
$!$                     means "else"

beside this, CONF_WATCOM, CONF_WATCOM106 etc. is defined.

so we need to have this temp.

--->
  "Which compiler? *W*atcom, *G*CC: ", WATCOM, GCC
        $:WATCOM$
  "Which version of watcom? 1*0*.6, 1*1*.0: ", WATCOM106, WATCOM11
        $:$
        $:WATCOM$
  "Which build do you want? *W*in32, *D*OS: ", WIN32, DOS32
        $!$
  DOS32
        $:$
        $:WIN32$
  "Which GUI do you want? *T*extmode (emulated), *W*indowed: ", GUI_TEXTMODE, GUI_WINDOWED
        $!$
  GUI_TEXTMODE
        $:$

!makefile.:

...
...

!exp\cpiface.exp:

...
...

                        <---
*/

struct define_s
{
  char *name;
  define_s *next;
} define_first={"..", 0}, *define=&define_first;

void Define(char *name);
int IsDef(char *name);

int main(int argc, char **argv)
{
  printf("OpenCP-build-options. GPL.\n");
  if(argc!=2)
  {
    printf("usage: %s <temp>\n"
           " to generate the files inside :)\n", *argv);
    return(1);
  }
  FILE *temp=fopen(argv[1], "rt");
  if(!temp)
  {
    perror(argv[1]);
    return(2);
  }
  int line=0, errors=0;
  while(!feof(temp))
  {
    line++;
    char buffer[MAXLINE];
    fgets(buffer, MAXLINE, temp);
    if((*buffer)&&(buffer[strlen(buffer)-1]=='\n')) buffer[strlen(buffer)-1]=0;
    if((*buffer)&&(buffer[strlen(buffer)-1]=='\r')) buffer[strlen(buffer)-1]=0;
    xtrim(buffer);
    if(*buffer=='#') continue;
    if(*buffer=='!')
    {
      if(buffer[1])
        break;
      printf("line %d: error, no filename specified.\n", line);
    }

    if(*buffer!='\"')
    {
      for(int i=strlen(buffer)-1; i>=0; i--)
        if((isspace(buffer[i]))||(!i))
        {
          if(isspace(buffer[i])) buffer[i--]==0;
          if(IsDef(buffer+i))
          { 
            printf("line %d: '%s' already defined.\n", line, buffer+i);
            errors++;
            break;
          } else
          {
            Define(buffer+i);
            buffer[i]=0;
          }
        }
    } else
    {
      char question[MAXLINE];
      strcpy(question, buffer+1);
      for(int i=0; i<strlen(question); i++)
        if(question[i]=='\"') { question[i]=0; break; }
      if(i==strlen(buffer-1))
      {
        printf("line %d: missing end of \"-string.\n", line);
        errors++;
        continue;
      }
      int curq=0;
      int answer=0;
      while(!curq)
      {
        printf("%s", question);
        fflush(stdout);
        answer=getch();
        if(!answer) getch();
        printf("%c\n", answer);
        if(answer==27)
        {
          errors++;
          break;
        }
        for(int j=0; j<strlen(question); j++)
          if(question[j]=='&')
          {
            curq++;
            if(answer==question[j+1])
               break;
          }
        if(!question[j]) curq=0;
        if(!curq)
          printf("%c is not a valid answer.\n", answer);
      }

      if(answer==27) break;
      i+=2;
      if(buffer[i]!=',')
      {
        printf("line %d: missing ',' (mehr so: ->%s<-).\n", line, buffer+i);
        errors++;
        continue;
      }
      i++;
      trim(buffer+i);
      int last=i, cnt=0, ok=0;
      while(buffer[i])
      {
        cnt++;
        last=i;
        while(buffer[i]&&(buffer[i]!=',')) i++;
        buffer[i]=0;
        xtrim(buffer+last);
        if(curq==cnt)
          if(IsDef(buffer+last))
          { 
            printf("line %d: '%s' already defined.\n", line, buffer+last);
            errors++;
            break;
          } else
          {
            Define(buffer+last);
            buffer[i]=0;
          }
        i++;
      }
    }
  }
  if(errors)
  {
    printf("there were some errors.\n");
    fclose(temp);
    return(2);
  }
  return(0);
}

char *trim(char *str)
{
  int i=strlen(str)-1;
  while((i>=0)&&(str[i]==' ')) str[i--]=0;       // cut spaces on the right...
  i=0;
  while(str[i]==' ') i++;
  int a=0;
  while(str[i]) str[a++]=str[i++];               // ...and on the left.
  str[a]=0;
  return(str);
}

char *xtrim(char *str)
{
  int i=strlen(str)-1;
  while((i>=0)&&(str[i]==' ')) str[i--]=0;
  i=0;
  while(str[i]==' ') i++;
  int a=0;
  while(str[i])
  {
    if(((a)&&((str[a-1]!=' ')||(str[i]!=' ')))||(!a))
      str[a++]=str[i++]; else i++;
  }
  str[a]=0;
  return(str);
}

void Define(char *name)
{
  define_s *t;
  printf("defining %s\n", name);
  for(t=define; t->next; t=t->next);

  t->next=new define_s;
  t=t->next;
  t->next=0;
  t->name=new char[strlen(name)+1];
  strcpy(t->name, name);
}

int IsDef(char *name)
{
  define_s *t;
  for(t=define; t->next; t=t->next) if(!stricmp(name, t->name)) return(!0);
  return(0);
}
