#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <string.h>
#include "mtw.h"

extern mt_c *mt;
volatile int curtask=0, maxtask=0;

void DebugBreak();
#pragma aux DebugBreak="int 3";

taskinfostruct task[maxtasks];

extern "C" void cscheduler()
{
/*  *((char*)(0xB8000))=(curtask/10)+'0';
  *((char*)(0xB8001))=0x70;
  *((char*)(0xB8002))=(curtask%10)+'0';
  *((char*)(0xB8003))=0x70; */
  if(task[curtask].flags&TASKNOSWITCH) return;
  task[curtask].esp=esp;
  task[curtask].ss=ss;
  curtask++;

  while((!(task[curtask].flags&TASKUSED))||(task[curtask].flags&TASKDELETE)||(task[curtask].curprio>0))
  {
    if(task[curtask].flags&TASKUSED) task[curtask].curprio--;
    curtask++;
    if(curtask>=maxtasks) curtask=0;
    if(curtask>maxtask) curtask=0;
  }
  task[curtask].curprio=task[curtask].prio-1;
  esp=task[curtask].esp;
  ss=task[curtask].ss;
}

int mtw_c::New(void *adress, int flags, int stackspace, int prio)
{
 for(int i=1; i<maxtasks; i++)
 {
  if(!(task[i].flags&TASKUSED))
  {
   if(i>maxtask) maxtask=i;
   task[i].prio=task[i].curprio=prio;
   task[i].stack=malloc(stackspace);
   if(!task[i].stack)
   {
    return(-1);
   }
   task[i].esp=((long)task[i].stack)+stackspace-0x50;
   task[i].ss=GetSS();
   ((long*)task[i].esp)[0]=GetGS();
   ((long*)task[i].esp)[1]=GetFS();
   ((long*)task[i].esp)[2]=GetES();
   ((long*)task[i].esp)[3]=GetDS();
   ((long*)task[i].esp)[4]=0;           // edi
   ((long*)task[i].esp)[5]=0;           // esi
   ((long*)task[i].esp)[6]=task[i].esp; // ebp
   ((long*)task[i].esp)[7]=task[i].esp; // esp
   ((long*)task[i].esp)[8]=0;           // ebx
   ((long*)task[i].esp)[9]=0;           // edx
   ((long*)task[i].esp)[10]=0;          // ecx
   ((long*)task[i].esp)[11]=0;          // eax
   ((long*)task[i].esp)[12]=(long)adress;//eip
   ((long*)task[i].esp)[13]=GetCS();    // cs
   ((long*)task[i].esp)[14]=0x204;      // eflags
   ((long*)task[i].esp)[15]=(long)taskDeleteCurrent;
   task[i].flags=flags|TASKUSED;
   task[i].what=0;
/*   task[i].what=new char[100];
   char module[20];
   char symbol[50];
   int diff=0;
   dll->GetNearestName(adress, module, symbol, diff);
   sprintf(task[i].what, "%s.%s+0x%x", module, symbol, diff); */
   return(i);
  }
 }
 return(-1);
}

int mtw_c::CNew(void *adress, void *that, int flags, int stackspace, int prio)
{
  for(int i=1; i<maxtasks; i++)
  {
    if(!(task[i].flags&TASKUSED))
    {
      if(i>maxtask) maxtask=i;
      task[i].prio=task[i].curprio=prio;
      task[i].stack=malloc(stackspace);
      if(!task[i].stack)
        return(-1);
      task[i].esp=((long)task[i].stack)+stackspace-0x50;
      task[i].ss=GetSS();
      ((long*)task[i].esp)[0]=GetGS();
      ((long*)task[i].esp)[1]=GetFS();
      ((long*)task[i].esp)[2]=GetES();
      ((long*)task[i].esp)[3]=GetDS();
      ((long*)task[i].esp)[4]=0;           // edi
      ((long*)task[i].esp)[5]=0;           // esi
      ((long*)task[i].esp)[6]=task[i].esp; // ebp
      ((long*)task[i].esp)[7]=task[i].esp; // esp
      ((long*)task[i].esp)[8]=0;           // ebx
      ((long*)task[i].esp)[9]=0;           // edx
      ((long*)task[i].esp)[10]=0;          // ecx
      ((long*)task[i].esp)[11]=0;          // eax
      ((long*)task[i].esp)[12]=(long)adress;//eip
      ((long*)task[i].esp)[13]=GetCS();    // cs
      ((long*)task[i].esp)[14]=0x204;      // eflags
      ((long*)task[i].esp)[15]=(long)taskDeleteCurrent;
      ((long*)task[i].esp)[16]=(long)that;
      task[i].flags=flags|TASKUSED;
      task[i].what=0;
/*   task[i].what=new char[100];
      char module[20];
      char symbol[50];
      int diff=0;
      dll->GetNearestName(adress, module, symbol, diff);
      sprintf(task[i].what, "%s.%s+0x%x", module, symbol, diff); */
      return(i);
    }
  }
  return(-1);
}

void mtw_c::Delete(int handle)
{
  printf("task abgegangen.\n");
  if(handle==curtask) task[handle].flags|=TASKDELETE; else
  if(handle>0)
  {
    if(task[handle].stack) free(task[handle].stack);
    task[handle].stack=0;
    task[handle].flags=0;
  }
}

mtw_c::mtw_c()
{
  curtask=0;
  maxtask=1;
  task[0].prio=task[0].curprio=1;
  task[0].flags=TASKUSED;
  task[0].stack=0;
}

void taskDeleteCurrent()
{
  mt->Delete(curtask);
  while(1) SwitchTask();
}

mtw_c::~mtw_c()
{
  task[curtask].flags|=TASKNOSWITCH;
  for(int i=1; i<maxtasks; i++)
    if(task[i].flags&TASKUSED) Delete(i);
}

void mtw_c::Yield()
{
  SwitchTask();
}
