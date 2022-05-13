/* zutil.h -- internal interface and configuration of the compression library
 * Copyright (C) 1995-1998 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * This is a modified version for the openCP help compiler, DO NOT DISTRIBUTE
 * OR ASK JEAN-LOUP GAILLY ABOUT THIS. CONTACT THE openCP DEVELOPER TEAM FOR
 * QUESTIONS. THANKS.
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* @(#) $Id: zutil.h,v 1.1.1.1 1998/12/20 12:53:47 cvs Exp $ */

#ifndef _Z_UTIL_H
#define _Z_UTIL_H

#include "zlib.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifndef local
#  define local static
#endif

typedef unsigned char  uch;
typedef uch FAR uchf;
typedef unsigned short ush;
typedef ush FAR ushf;
typedef unsigned long  ulg;

extern const char *z_errmsg[10];

#define ERR_MSG(err) z_errmsg[Z_NEED_DICT-(err)]

#define ERR_RETURN(strm,err) \
  return (strm->msg = (char*)ERR_MSG(err), (err))

#ifndef DEF_WBITS
#  define DEF_WBITS MAX_WBITS
#endif

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2

#define MIN_MATCH  3
#define MAX_MATCH  258

#define PRESET_DICT 0x20

#ifdef MSDOS
#  define OS_CODE  0x00
#  if defined(__TURBOC__) || defined(__BORLANDC__)
#    if(__STDC__ == 1) && (defined(__LARGE__) || defined(__COMPACT__))
       void _Cdecl farfree( void *block );
       void *_Cdecl farmalloc( unsigned long nbytes );
#    else
#     include <alloc.h>
#    endif
#  else
#    include <malloc.h>
#  endif
#endif

#ifdef WIN32
#  define OS_CODE  0x0b
#endif

#ifndef OS_CODE
#  define OS_CODE  0x03
#endif

#ifndef F_OPEN
#  define F_OPEN(name, mode) fopen((name), (mode))
#endif

#ifdef HAVE_STRERROR
   extern char *strerror OF((int));
#  define zstrerror(errnum) strerror(errnum)
#else
#  define zstrerror(errnum) ""
#endif

#define HAVE_MEMCPY
#ifdef SMALL_MEDIUM
#  define zmemcpy _fmemcpy
#  define zmemcmp _fmemcmp
#  define zmemzero(dest, len) _fmemset(dest, 0, len)
#else
#  define zmemcpy memcpy
#  define zmemcmp memcmp
#  define zmemzero(dest, len) memset(dest, 0, len)
#endif

#define Assert(cond,msg)
#define Trace(x)
#define Tracev(x)
#define Tracevv(x)
#define Tracec(c,x)
#define Tracecv(c,x)


typedef uLong (ZEXPORT *check_func) OF((uLong check, const Bytef *buf,
				       uInt len));
voidpf zcalloc OF((voidpf opaque, unsigned items, unsigned size));
void   zcfree  OF((voidpf opaque, voidpf ptr));

#define ZALLOC(strm, items, size) \
           (*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr)  (*((strm)->zfree))((strm)->opaque, (voidpf)(addr))
#define TRY_FREE(s, p) {if (p) ZFREE(s, p);}

#endif /* _Z_UTIL_H */
