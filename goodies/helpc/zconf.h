/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995-1998 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * This is a modified version for the openCP help compiler, DO NOT DISTRIBUTE
 * OR ASK JEAN-LOUP GAILLY ABOUT THIS. CONTACT THE openCP DEVELOPER TEAM FOR
 * QUESTIONS. THANKS.
 */

/* @(#) $Id: zconf.h,v 1.1.1.1 1998/12/20 12:53:47 cvs Exp $ */

#ifndef _ZCONF_H
#define _ZCONF_H

#if (defined(_WIN32) || defined(__WIN32__)) && !defined(WIN32)
#  define WIN32
#endif
#if defined(__GNUC__) || defined(WIN32) || defined(__386__) || defined(i386)
#  ifndef __32BIT__
#    define __32BIT__
#  endif
#endif
#if defined(__MSDOS__) && !defined(MSDOS)
#  define MSDOS
#endif

#ifdef MSDOS
#  define UNALIGNED_OK
#endif

#define STDC

#ifndef MAX_MEM_LEVEL
#  define MAX_MEM_LEVEL 9
#endif

#ifndef MAX_WBITS
#  define MAX_WBITS   15 /* 32K LZ77 window */
#endif

#ifndef OF
#  define OF(args)  args
#endif

#ifndef ZEXPORT
#  define ZEXPORT
#endif
#ifndef ZEXPORTVA
#  define ZEXPORTVA
#endif
#ifndef ZEXTERN
#  define ZEXTERN extern
#endif

#ifndef FAR
#   define FAR
#endif

typedef unsigned char  Byte;  
typedef unsigned int   uInt;  
typedef unsigned long  uLong; 

typedef Byte  FAR Bytef;
typedef char  FAR charf;
typedef int   FAR intf;
typedef uInt  FAR uIntf;
typedef uLong FAR uLongf;

typedef void FAR *voidpf;
typedef void     *voidp;

#include <sys/types.h>
#include <unistd.h>   
#define z_off_t  off_t

#endif
