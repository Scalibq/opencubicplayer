#ifndef __MTW_H
#define __MTW_H

#include "mt.h"

#define TASKUSED        1
#define TASKNOSWITCH    2
#define TASKDELETE      4
#define TASKWAITMESSAGE 8

struct taskinfostruct
{
 int flags;
 void *stack;
 int prio, curprio;
 long esp, ss;
 char *what;
};

extern volatile int curtask, maxtask;
const int maxtasks=300;
extern taskinfostruct task[maxtasks];
extern volatile int curtask;
extern "C" void interrupt Scheduler();
extern "C" void SwitchTask();
#pragma aux SwitchTask modify[eax ebx ecx edx 8087]
extern "C" long ss, esp, noswitch;


short GetCS();
short GetDS();
short GetES();
short GetFS();
short GetGS();
short GetSS();
extern "C" void cscheduler();

#pragma aux GetCS="mov ax, cs" value[ax];
#pragma aux GetDS="mov ax, ds" value[ax];
#pragma aux GetES="mov ax, es" value[ax];
#pragma aux GetFS="mov ax, fs" value[ax];
#pragma aux GetGS="mov ax, gs" value[ax];
#pragma aux GetSS="mov ax, ss" value[ax];

void taskDeleteCurrent();

class mtw_c: public mt_c
{
public:
  mtw_c();          // initializes the multitasking
  ~mtw_c();         // deinitializes the multitasking
  void Yield();          // gives time to other tasks
                        // (calls SwitchTask();)

  int New(void *adress, int flags, int stackspace, int prio);
                        // 'adress' is a _cdecl'd function,
                        // 'stackspace' is the stackspace in bytes and
                        // 'prio' is the priority, 1 for highest, oo for lowest
  int CNew(void *adress, void *that, int flags, int stackspace, int prio);
                        // like New(), but 'that' will be passed as first
                        // parm.
  void Delete(int handle);
                        // kills a task.
};

#endif
