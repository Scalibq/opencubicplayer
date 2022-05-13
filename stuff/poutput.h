#ifndef __POUTPUT_H
#define __POUTPUT_H

#ifdef WIN32
#include "w32idata.h"
#endif

void plSetTextMode(unsigned char x);
void plSetBarFont();
char *convnum(unsigned long num, char *buf, unsigned char radix, unsigned short len, char clip0=1);
void writenum(short *buf, unsigned short ofs, unsigned char attr, unsigned long num, unsigned char radix, unsigned short len, char clip0=1);
void writestring(short *buf, unsigned short ofs, unsigned char attr, const char *str, unsigned short len);
void writestringattr(short *buf, unsigned short ofs, const void *str, unsigned short len);
void markstring(short *buf, unsigned short ofs, unsigned short len);
void displaystr(unsigned short y, unsigned short x, unsigned char attr, const char *str, unsigned short len);
void displaystrattr(unsigned short y, unsigned short x, const short *buf, unsigned short len);
void displaystrattrdi(unsigned short y, unsigned short x, const unsigned char *txt, const unsigned char *attr, unsigned short len);
void displayvoid(unsigned short y, unsigned short x, unsigned short len);

//      they're just dummy function in win32
// #ifdef DOS32
void plSetGraphMode(unsigned char size);
int  plSetGraphPage(unsigned char x);
void gdrawchar(unsigned short x, unsigned short y, unsigned char c, unsigned char f, unsigned char b);
void gdrawchart(unsigned short x, unsigned short y, unsigned char c, unsigned char f);
void gdrawcharp(unsigned short x, unsigned short y, unsigned char c, unsigned char f, void *picp);
void gdrawchar8(unsigned short x, unsigned short y, unsigned char c, unsigned char f, unsigned char b);
void gdrawchar8t(unsigned short x, unsigned short y, unsigned char c, unsigned char f);
void gdrawchar8p(unsigned short x, unsigned short y, unsigned char c, unsigned char f, void *picp);
void gdrawstr(unsigned short y, unsigned short x, const char *s, unsigned short len, unsigned char f, unsigned char b);
void gupdatestr(unsigned short y, unsigned short x, const short *str, unsigned short len, short *old);
// #endif

enum { vidNorm, vidVESA, vidMDA };

int ekbhit();
int egetch();

#if defined(DOS32) || (defined(WIN32)&&defined(NO_POUTPUT_IMPORT))

extern short plScrHeight;
extern short plScrWidth;
extern char plVidType;
extern unsigned char plScrType;
extern unsigned short plScrRowBytes;
extern int plScrMode;
extern char *plVidMem;                  // usually 0xA0000 on banked modes
extern int plLFB;                       // 0 or !0

#else
extern_data short plScrHeight;
extern_data short plScrWidth;
extern_data char plVidType;
extern_data unsigned char plScrType;
extern_data unsigned short plScrRowBytes;
extern_data int plScrMode;
extern_data char *plVidMem;
extern_data int plLFB;

#endif

void drawbar(unsigned short x, unsigned short yb, unsigned short yh, unsigned long hgt, unsigned long c);

#endif
