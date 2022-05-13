//
// mytypes.h,v 1.1.1.1 1996/12/30 22:49:03 ms Exp
//

#ifndef __MYTYPES_H
#define __MYTYPES_H



#ifdef true
  #undef true
#endif
#define true 1

#ifdef false
  #undef false
#endif
#define false 0


#ifdef TRUE
  #undef TRUE
#endif
#define TRUE 1

#ifdef FALSE
  #undef FALSE
#endif
#define FALSE 0


// Wanted: 8-bit signed/unsigned.
typedef signed char sbyte;
typedef unsigned char ubyte;

// Wanted: 16-bit signed/unsigned.
typedef signed short int sword;
typedef unsigned short int uword;

// Wanted: 32-bit signed/unsigned.
typedef signed long int sdword;
typedef unsigned long int udword;


// Some common type shortcuts.
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long int ulong;


typedef void (*ptr2func)();


#endif
