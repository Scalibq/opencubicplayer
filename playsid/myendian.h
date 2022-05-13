//
// 1997/09/27 21:33:14
//

#ifndef __MYENDIAN_H
  #define __MYENDIAN_H


#include "compconf.h"
#include "mytypes.h"

// For optional direct memory hacks.
// First value in memory = index 0.
// Second value in memory = index 1.

// For a pair of bytes/words/longwords.
#ifdef LO
  #undef LO
#endif
#ifdef HI
  #undef HI
#endif

// For two pairs of bytes/words/longwords.
#ifdef LOLO
  #undef LOLO
#endif
#ifdef LOHI
  #undef LOHI
#endif
#ifdef HILO
  #undef HILO
#endif
#ifdef HIHI
  #undef HIHI
#endif

#if defined(IS_BIG_ENDIAN)
// byte-order: HI..4321..LO
  #define LO 1
  #define HI 0
  #define LOLO 3
  #define LOHI 2
  #define HILO 1
  #define HIHI 0
#elif defined(IS_LITTLE_ENDIAN)
// byte-order: LO..1234..HI
  #define LO 0
  #define HI 1
  #define LOLO 0
  #define LOHI 1
  #define HILO 2
  #define HIHI 3
#else
#error Define the endianess of the CPU in ``include/compconf.h'' !
#endif

union cpuLword
{
	uword w[2];  // single 16-bit low and high word
	udword l;    // complete 32-bit longword
};

union cpuWord
{
	ubyte b[2];  // single 8-bit low and high byte
	uword w;     // complete 16-bit word
};

union cpuLBword
{
	ubyte b[4];  // single 8-bit bytes
	udword l;    // complete 32-bit longword
};

// ---
// The function name sucks, but is provided out of backwards compatibility.

// Convert high-byte and low-byte to 16-bit word.
// Used to read 16-bit words in little-endian order.
inline uword m68k(ubyte hi, ubyte lo)
{
  return(( (uword)hi << 8 ) + (uword)lo );
}

// Convert high bytes and low bytes of MSW and LSW to 32-bit word.
// Used to read 32-bit words in little-endian order.
inline udword m68k(ubyte hihi, ubyte hilo, ubyte hi, ubyte lo)
{
  return(( (udword)hihi << 24 ) + ( (udword)hilo << 16 ) + 
		 ( (udword)hi << 8 ) + (udword)lo );
}

// ---

// Convert high-byte and low-byte to 16-bit word.
// Used to read 16-bit words in little-endian order.
inline uword readEndian(ubyte hi, ubyte lo)
{
  return(( (uword)hi << 8 ) + (uword)lo );
}

// Convert high bytes and low bytes of MSW and LSW to 32-bit word.
// Used to read 32-bit words in little-endian order.
inline udword readEndian(ubyte hihi, ubyte hilo, ubyte hi, ubyte lo)
{
  return(( (udword)hihi << 24 ) + ( (udword)hilo << 16 ) + 
		 ( (udword)hi << 8 ) + (udword)lo );
}

// Read a little-endian 16-bit word from two bytes in memory.
inline uword readLEword(ubyte ptr[2])
{
#if defined(IS_LITTLE_ENDIAN)
	return *((uword*)ptr);
#else
	return readEndian(ptr[1],ptr[0]);
#endif
}

// Write a big-endian 16-bit word to two bytes in memory.
inline void writeLEword(ubyte ptr[2], uword someWord)
{
#if defined(IS_LITTLE_ENDIAN)
	*((uword*)ptr) = someWord;
#else
	ptr[0] = (someWord & 0xFF);
	ptr[1] = (someWord >> 8);
#endif
}



// Read a big-endian 16-bit word from two bytes in memory.
inline uword readBEword(ubyte ptr[2])
{
#if defined(IS_BIG_ENDIAN)
	return *((uword*)ptr);
#else
	return ( (((uword)ptr[0])<<8) + ((uword)ptr[1]) );
#endif
}

// Read a big-endian 32-bit word from four bytes in memory.
inline udword readBEdword(ubyte ptr[4])
{
#if defined(IS_BIG_ENDIAN)
	return *((udword*)ptr);
#else
	return ( (((udword)ptr[0])<<24) + (((udword)ptr[1])<<16)
			+ (((udword)ptr[2])<<8) + ((udword)ptr[3]) );
#endif
}

// Write a big-endian 16-bit word to two bytes in memory.
inline void writeBEword(ubyte ptr[2], uword someWord)
{
#if defined(IS_BIG_ENDIAN)
	*((uword*)ptr) = someWord;
#else
	ptr[0] = someWord >> 8;
	ptr[1] = someWord & 0xFF;
#endif
}

// Write a big-endian 32-bit word to four bytes in memory.
inline void writeBEdword(ubyte ptr[4], udword someDword)
{
#if defined(IS_BIG_ENDIAN)
	*((udword*)ptr) = someDword;
#else
	ptr[0] = someDword >> 24;
	ptr[1] = (someDword>>16) & 0xFF;
	ptr[2] = (someDword>>8) & 0xFF;
	ptr[3] = someDword & 0xFF;
#endif
}


// 
// NOTE: The following are currently not used by SIDPLAY.
//

// Convert 16-bit little-endian word to big-endian order
// or vice versa.
inline uword m68kword( uword intelword )
{
  uword hi = intelword >> 8;
  uword lo = intelword & 255;
  return(( lo << 8 ) + hi );
}

// Convert 32-bit little-endian word to big-endian order
// or vice versa.
inline udword m68kdword( udword inteldword )
{
  udword hihi = inteldword >> 24;
  udword hilo = ( inteldword >> 16 ) & 0xFF;
  udword hi = ( inteldword >> 8 ) & 0xFF;
  udword lo = inteldword & 0xFF;
  return(( lo << 24 ) + ( hi << 16 ) + ( hilo << 8 ) + hihi );
}


#endif
