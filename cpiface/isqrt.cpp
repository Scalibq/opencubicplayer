// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// A fast sqrt routine
//
// revision history: (please note changes here)
// -doj980928  Dirk Jagdmann <doj@cubic.org>
//   -initial release
//   -as this totally sucks I will change to real asm file soon.
//
// -doj981105  Dirk Jagdmann <doj@cubic.org>
//   -changed the inline routine for non-Watcom to a #define statement

#ifdef __WATCOMC__
unsigned short isqrt(unsigned long);
#pragma aux isqrt parm [ebx] value [dx] modify [eax ebx ecx edx] = \
"  mov ecx,40000000h" \
"fastloop:" \
"    cmp ebx,ecx" \
"    jae near goloop" \
"    shr ecx,2" \
"  jnz fastloop" \
"  xor edx,edx" \
"  jmp loopend" \
"" \
"goloop: " \
"  mov edx,ecx" \
"  sub ebx,ecx" \
"  xor eax,eax" \
"  toobig: "\
"      add ebx,eax" \
"    shr ecx,2" \
"    jz loopend" \
"  sqrtloop:"\
"    mov eax,edx" \
"    add eax,ecx" \
"    shr edx,1" \
"    sub ebx,eax" \
"    jb toobig"\
"      or edx,ecx"\
"    shr ecx,2"\
"    jnz sqrtloop" \
"loopend:"
#endif

#ifndef __WATCOMC__
#include <math.h>
//static inline unsigned short isqrt(unsigned long a) {
//  return sqrt(a);
//}
#define isqrt(a) sqrt(a)
#endif

