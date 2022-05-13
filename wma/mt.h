// (98-05-01, 21:43, created)

/*

  description:

  this is the declaration of the mt_c-class, which is the interface
  to the actual mt-code.

  portability:
  all systems - all c++-compilers

*/

#ifndef __MT_H
#define __MT_H

#ifdef Yield
#undef Yield
#endif
class mt_c
{
public:
 virtual void Yield()=0;
                        // gives time to other tasks

 virtual int New(void *adress, int flags, int stackspace, int prio)=0;
                        // 'adress' is a _cdecl'd function,
                        // 'flags' should be '0'
                        // 'stackspace' is the stackspace in bytes and
                        // 'prio' is the priority, 1 for highest, oo for lowest
 virtual int CNew(void *adress, void *that, int flags, int stackspace, int prio)=0;
                        // like New(), but 'that' will be passed as first
                        // parm.
 virtual void Delete(int handle)=0;
                        // kills a task.
};

mt_c *GetMT();

#endif
