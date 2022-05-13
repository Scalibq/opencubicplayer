// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CP hypertext help viewer
//
// revision history: (please note changes here)
//  -fg980812  Fabian Giesen <gfabian@jdcs.su.nw.schule.de>
//    -first alpha (hey, and it was written on my BIRTHDAY!)
//    -some pages of html documentation converted
//  -fg980813  Fabian Giesen <gfabian@jdcs.su.nw.schule.de>
//    -added pgup/pgdown support
//    -changed drawing method. stopped all flickering this way. but the code
//     did not get better during this bugfix... :)
//    -changed helpfile loader to load in "boot" phase, not in first use
//     I did this because the helpfile was sometimes not found when you
//     changed the directory via DOS shell.
//    -some speedups
//    -the "description" field is now displayed at top/right of the window
//    -again converted some documentation
//  -fg980814  Fabian Giesen <gfabian@jdcs.su.nw.schule.de>
//    -added support for compressed helpfiles
//    -added percentage display at right side of description
//    -added jumping to Contents page via Alt-C
//    -added jumping to Index page via Alt-I
//    -added jumping to License page via Alt-L
//    -html conversion again
//  -fg980820  Fabian Giesen <gfabian@jdcs.su.nw.schule.de>
//    -now searches CP.HLP in data path
//    -decided that code could be much cleaner but too lazy to change it :)
//    -maybe i'll add context sensitivity (if there is interest)
//    -well, uhm, html documentation is still not fully converted (shame
//     over me... :))
//  -fg_dunno  Fabian Giesen <gfabian@jdcs.su.nw.schule.de>
//    -added support for CP.HLP in CP.PAK because kb said CPHELPER would be
//     part of the next release
//  -fg980924  Fabian Giesen <gfabian@jdcs.su.nw.schule.de>
//    -changed keyboard use/handling a lot based on advices (commands? :)
//     from kb.
//    -finally the help compiler supports colour codes (i don't know why I
//     write this here).
//    -added fileselector support (yes!)
//    -made this possible by splitting this up in one "host" and two
//     "wrappers" for fileselector/player interface
//    -and, you won't believe, it still works :)
//  -kbwhenever Tammo Hinrichs <opencp@gmx.net>
//    -some minor cosmetical changes
//  -fg981118  Fabian Giesen <fabian@jdcs.su.nw.schule.de>
//    -note: please use this email now (the old one still works, but I don't
//     give promises about how long this will be)
//    -changed keyboard control again according to suggestions kb made
//     (1. kannst du mir das nicht einfach mailen?  2. warum sagst du mir
//      nicht sofort, das du lynx-keyboard-handling willst  3. sag mal, bin
//      ich eigentlich dein codesklave? :)
//    -detailed changes: up/down now also selects links
//                       this ugly tab/shift-tab handling removed
//                       pgup/pgdown updates current link
//                       keyboard handling much more lynx-like
//    -this code even got worse during change of the keyboard-handling
//     (but using it got even nicer)
//    -hope kb likes this as it is (nich wahr, meister, sei ma zufrieden! :)
//    -fixed this nasty bug which crashed opencp (seemed to be some un-
//     initialized pointer, or something else...)
//  -ryg981121  Fabian Giesen <fabian@jdcs.su.nw.schule.de>
//    -now i'm using my handle instead of my name for changelog (big change,
//     eh?)
//    -fixed some stupid bugs with pgup/pgdown handling
//    -also fixed normal scrolling (hope it works correctly now...)
//    -no comment from kb about my "lynx style key handling" yet...
//  -fd981205  Felix Domke <tmbinc@gmx.net>
//    -included the stdlib.h AGAIN AND AGAIN. hopefully this version will now
//     finally reach the repository ;)
//     (still using my realname... ;)
//  -fd981206   Felix Domke    <tmbinc@gmx.net>
//    -edited for new binfile

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <dos.h>
#include <io.h>
#include "psetting.h"
#include "poutput.h"
#include "plinkman.h"
#include "err.h"
#include "pmain.h"
#include "binfile.h"
#include "binfstd.h"
#include "binfpak.h"
#include "inflate.h"
#include "cphelper.h"

short plWinFirstLine, plWinHeight, plHelpHeight, plHelpScroll;

int       Helpfile_ID=1213219663;
int       Helpfile_Ver=0x011000;
int       Helppages;
int       HelpfileErr=hlpErrNoFile;
helppage *Page, *curpage;
link     *curlink;
int       link_ind, fsmode=0;

// Helpfile Commands

#define   CMD_NORMAL     1
#define   CMD_BRIGHT     2
#define   CMD_HYPERLINK  3
#define   CMD_CENTERED   4
#define   CMD_CHCOLOUR   5
#define   CMD_RAWCHAR    6
#define   CMD_LINEFEED  10             // this is a pseudo-command...
#define   CMD_MAX       31

// Useful macros...

#define   MIN(a,b)      ((a)<(b)?(a):(b))
#define   MAX(a,b)      ((a)>(b)?(a):(b))
#define   ABS(a)        ((a)<0?-(a):(a))

// With this procedures I don't need the string library

void *mymemset(void *what, int with, int len)
{
  char *to=(char *) what;

  for (int i=0; i<len; i++) *to++=with;

  return what;
};

void *mymemcpy(void *dst, void *src, int len)
{
  char *fr=(char *) src;
  char *to=(char *) dst;

  for (int i=0; i<len; i++) *to++=*fr++;

  return dst;
};

char *mystrcpy(char *dst, char *src)
{
  char *fr=src;
  char *to=dst;

  while (*fr) *to++=*fr++;
  *to++=0;

  return dst;
};

char *mystrncpy(char *dst, char *src, int n)
{
  char *fr=src;
  char *to=dst;
  int   i=0;

  while (*fr && (i++<n)) *to++=*fr++;
  *to++=0;

  return dst;
};

char *mystrcat(char *dst, char *src)
{
  char *fr=src;
  char *to=dst;

  while (*to) to++;
  while (*fr) *to++=*fr++;
  *to++=0;

  return dst;
};

char *mystrchr(char *str, char ch)
{
  char *fr=str;

  while (*fr) { if (*fr==ch) return fr; fr++; };

  return NULL;
};

int mystrlen(char *str)
{
  char *fr=str;
  int   len=0;

  while (*fr++) len++;

  return len;
};

// ---------------------------- here starts the viewer

static int doReadVersion100Helpfile(binfile &file)
{
  Helppages=file.getl();

  Page=new helppage[Helppages];

  for (int i=0; i<Helppages; i++)
  {
    mymemset(Page[i].name, 0, 128); char len=file.getc();
    file.read(Page[i].name, len);

    mymemset(Page[i].desc, 0, 128); len=file.getc();
    file.read(Page[i].desc, len);

    Page[i].size=file.getl();
    Page[i].lines=file.getl();

    Page[i].links=NULL;
    Page[i].rendered=NULL;
  };

  for (int i=0; i<Helppages; i++)
  {
    Page[i].data=new char[Page[i].size];
    file.read(Page[i].data, Page[i].size);
  };

  return hlpErrOk;
};

static int doReadVersion110Helpfile(binfile &file)
{
  int  *compdatasize;
  char *inbuf;

  Helppages=file.getl();

  Page=new helppage[Helppages];
  compdatasize=new int[Helppages];

  for (int i=0; i<Helppages; i++)
  {
    mymemset(Page[i].name, 0, 128); char len=file.getc();
    file.read(Page[i].name, len);

    mymemset(Page[i].desc, 0, 128); len=file.getc();
    file.read(Page[i].desc, len);

    Page[i].size=file.getl();
    Page[i].lines=file.getl();
    compdatasize[i]=file.getl();

    Page[i].links=NULL;
    Page[i].rendered=NULL;
  };

  for (int i=0; i<Helppages; i++)
  {
    Page[i].data=new char[Page[i].size];
    inbuf=new char[compdatasize[i]];

    file.read(inbuf, compdatasize[i]);
    inflate(Page[i].data, inbuf);

    delete[] inbuf;
  };

  delete[] compdatasize;

  return hlpErrOk;
};

static int doReadHelpFile(binfile &file)
{
  int version;

  if (file.getl()!=Helpfile_ID) return hlpErrBadFile;

  version=file.getl(); if (version>Helpfile_Ver) return hlpErrTooNew;
  if (version<0x10000) return hlpErrBadFile;

  switch (version >> 8) {
    case 0x100: return doReadVersion100Helpfile(file);
    case 0x110: return doReadVersion110Helpfile(file);
    default: return hlpErrBadFile;
  };
};

static char plReadHelpExternal()
{
  char     helpname[_MAX_PATH];
  sbinfile bf;

  if (Page && (HelpfileErr==hlpErrOk)) return 1;

  mystrcpy(helpname, cfDataDir);
  mystrcat(helpname, "CP.HLP");

  if (!bf.open(helpname, sbinfile::openro))
  {
    HelpfileErr=doReadHelpFile(bf);

    bf.close();
  }
  else
  {
    HelpfileErr=hlpErrNoFile;

    return 0;
  };

  return (HelpfileErr==hlpErrOk);
}

static char plReadHelpPack()
{
  pakbinfile bf;

  if (Page && (HelpfileErr==hlpErrOk)) return 1;

  if (!bf.open("CP.HLP"))
  {
    HelpfileErr=doReadHelpFile(bf);

    bf.close();
  }
  else
  {
    HelpfileErr=hlpErrNoFile;

    return 0;
  };

  return (HelpfileErr==hlpErrOk);
}

helppage *brDecodeRef(char *name)
{
  for (int i=0; i<Helppages; i++) if (!stricmp(Page[i].name, name)) return &Page[i];

  return NULL;
};

static link *firstLinkOnPage(helppage *pg)
{
  if (!pg->linkcount) return NULL;

  return &pg->links[0];
};

static int linkOnCurrentPage(link *lnk)
{
  int y;

  if (!lnk) return 0;

  y=lnk->posy;
  if ((y>=plHelpScroll) && (y<plHelpScroll+plWinHeight)) return 1;

  return 0;
};

void brRenderPage(helppage *pg)
{
  link_list *lst, *endlst;
  int        lcount;
  char       linebuf[160];
  char       *data;
  char       attr;
  int        x, y, i;

  if (pg->rendered)
  {
    delete[] pg->rendered;
    pg->rendered=NULL;
  };

  if (pg->links)
  {
    delete[] pg->links;
    pg->links=NULL;
  };

  lst=endlst=NULL; lcount=0; x=y=0; attr=0x07;

  pg->rendered=new short[80*MAX(pg->lines, plWinHeight)];
  mymemset(pg->rendered, 0, 160*MAX(pg->lines, plWinHeight));
  mymemset(linebuf, 0, 160);

  data=pg->data; i=pg->size;

  while (i>0)
  {
    if (*data<CMD_MAX)
    {
      switch (*data)
      {
        case CMD_NORMAL: attr=0x07; break;
        case CMD_BRIGHT: attr=0x0f; break;
        case CMD_HYPERLINK:
          char linkbuf[256];
          int  llen;

          data++; i--; mystrcpy(linkbuf, data);

          if (!endlst)
          {
            lst=new link_list; endlst=lst;
          }
          else
          {
            endlst->next=new link_list; endlst=endlst->next;
          };

          *mystrchr(linkbuf, ',')=0;
          endlst->ref=(void *) brDecodeRef(linkbuf);

          i-=mystrchr(data, ',')-data+1;
          data+=mystrchr(data, ',')-data+1;

          llen=0;

          endlst->posx=x; endlst->posy=y;

          while (*data)
          {
            if (x<80)
            {
              linebuf[(x<<1)+0]=*data;
              linebuf[(x<<1)+1]=0x03;
              x++; llen++;
            };

            data++; i--;
          };

          endlst->len=llen;

          lcount++;

          break;
        case CMD_CENTERED:
          data++; i--;

          x=40-(mystrlen(data) >> 1); if (x<0) x=0;

          while (*data)
          {
            if (x<80)
            {
              linebuf[(x<<1)+0]=*data;
              linebuf[(x<<1)+1]=attr;
              x++;
            };

            data++; i--;
          };

          break;
        case CMD_CHCOLOUR: data++; i--; attr=*data; break;
        case CMD_RAWCHAR:
          data++; i--;

          if (x<80)
          {
            linebuf[(x<<1)+0]=*data;
            linebuf[(x<<1)+1]=attr;
            x++;
          };

          break;
        case CMD_LINEFEED:
          mymemcpy(&pg->rendered[y*80], linebuf, 160); x=0; y++;
          mymemset(linebuf, 0, 160);
          break;
      };

      data++; i--;
    }
    else
    {
      if (x<80)
      {
        linebuf[(x<<1)+0]=*data;
        linebuf[(x<<1)+1]=attr;
        x++;
      };

      data++; i--;
    };
  };

  pg->links=new link[lcount];
  pg->linkcount=lcount;

  for (i=0; i<lcount; i++)
  {
    pg->links[i].posx=lst->posx;
    pg->links[i].posy=lst->posy;
    pg->links[i].len=lst->len;
    pg->links[i].ref=lst->ref;

    endlst=lst; lst=lst->next; delete endlst;
  };
};

void brSetPage(helppage *page)
{
  if (!page) return;

  if (curpage)
  {
    if (curpage->rendered)
    {
      delete[] curpage->rendered;
      curpage->rendered=NULL;
    };

    if (curpage->links)
    {
      delete[] curpage->links;
      curpage->links=NULL;
    };
  };

  curpage=page; brRenderPage(curpage);

  plHelpHeight=curpage->lines; plHelpScroll=0;

  curlink=firstLinkOnPage(curpage);
  if (!curlink) link_ind=-1; else link_ind=0;
};

void brDisplayHelp()
{
  int curlinky;

  if ((plHelpScroll+plWinHeight)>plHelpHeight)
    plHelpScroll=plHelpHeight-plWinHeight;

  if (plHelpScroll<0)
    plHelpScroll=0;

  if (curlink) curlinky=(curlink->posy)-plHelpScroll; else curlinky=-1;

  displaystr(plWinFirstLine-1, 0, 0x09, "   OpenCP help ][", 20);

  char destbuffer[60];
  char strbuffer[256];
  char numbuffer[4];

  if (HelpfileErr==hlpErrOk)
    mystrcpy(strbuffer, curpage->desc);
  else
    mystrcpy(strbuffer, "Error!");

  convnum(100*plHelpScroll/MAX(plHelpHeight-plWinHeight, 1), numbuffer, 10,
          3);

  mystrcat(strbuffer, "-");
  mystrcat(strbuffer, numbuffer);
  mystrcat(strbuffer, "%");

  mymemset(destbuffer, 0x20, 60);
  int descxp=MAX(0, 59-mystrlen(strbuffer));

  mystrncpy(&destbuffer[descxp], strbuffer, 59-descxp);
  displaystr(plWinFirstLine-1, 20, 0x08, destbuffer, 59);

  int y;

  if (HelpfileErr!=hlpErrOk)
  {
    char errormsg[80];

    mystrcpy(errormsg, "Error: ");

    switch (HelpfileErr)
    {
      case hlpErrNoFile:
        mystrcat(errormsg, "Helpfile \"CP.HLP\" is not present"); break;
      case hlpErrBadFile:
        mystrcat(errormsg, "Helpfile \"CP.HLP\" is corrupted"); break;
      case hlpErrTooNew:
        mystrcat(errormsg, "Helpfile version is too new. Please update."); break;
      default:
        mystrcat(errormsg, "Currently undefined help error");
    };

    displayvoid(plWinFirstLine, 0, 80);

    displaystr(plWinFirstLine+1, 4, 0x04, errormsg, 74);

    for (y=2; y<plWinHeight; y++)
        displayvoid(y+plWinFirstLine, 0, 80);
  }
  else
  {
    for (y=0; y<plWinHeight; y++)
    {
      if (y!=curlinky)
      {
        displaystrattr(y+plWinFirstLine, 0,
                       &curpage->rendered[(y+plHelpScroll)*80], 80);
      }
      else
      {
        int yp=(y+plHelpScroll)*80;

        if (curlink->posx!=0)
        {
          displaystrattr(y+plWinFirstLine, 0,
                         &curpage->rendered[yp],
                         curlink->posx);
        };

        int xp=curlink->posx+curlink->len;

        displaystrattr(y+plWinFirstLine, xp,
                       &curpage->rendered[yp+xp],
                       79-xp);

        char dummystr[82];
		
		int i, off;

        for (i=0, off=yp+curlink->posx; curpage->rendered[off] & 0xff;
             i++, off++)
             dummystr[i]=curpage->rendered[off] & 0xff;

        dummystr[i]=0;

        displaystr(y+plWinFirstLine, curlink->posx, 4, dummystr, curlink->len);

        // and all this just to prevent flickering. ARG!
      }
    }
  }
}

static void processActiveHyperlink()
{
  if (curlink) brSetPage((helppage *) curlink->ref);
};

void brSetWinStart(short fl)
{
  plWinFirstLine=fl;
};

void brSetWinHeight(short h)
{
  plWinHeight=h;
};

int brHelpKey(unsigned short key)
{
  link *link2;

  switch (key)
  {
  case 0x4800: //up
    if (curpage->linkcount)
    {
      link2=&curpage->links[MAX(link_ind-1, 0)];

      if (link2!=curlink)
      {
        if (plHelpScroll-link2->posy>1)
          plHelpScroll--;
        else
        {
          link_ind=MAX(link_ind-1, 0);
          curlink=link2;

          if (link2->posy<plHelpScroll) plHelpScroll=link2->posy;
        }
      }
      else
        if (plHelpScroll>0) plHelpScroll--;
    }
    else
      if (plHelpScroll>0) plHelpScroll--;

    break;
  case 0x5000: //down
    if (curpage->linkcount)
    {
      link2=&curpage->links[MIN(link_ind+1, curpage->linkcount-1)];

      if (link2->posy-plHelpScroll>plWinHeight)
        plHelpScroll++;
      else
      {
        link_ind=MIN(link_ind+1, curpage->linkcount-1);
        curlink=link2;

        if (link2->posy>(plHelpScroll+plWinHeight))
          plHelpScroll=link2->posy;
        else if (link2->posy==(plHelpScroll+plWinHeight))
          plHelpScroll++;
      }
    }
    else
      if (plHelpScroll<plHelpHeight-1) plHelpScroll++;

    break;
  case 0x4900: //pgup
    plHelpScroll-=plWinHeight;

    if (plHelpScroll<0) plHelpScroll=0;

    if (curpage->linkcount)
    {
      if (!linkOnCurrentPage(curlink))
      {
        int bestmatch, bestd;

        bestd=2000000; bestmatch=-1;

        for (int i=curpage->linkcount-1; i>=0; i--)
        {
          int d=ABS(plHelpScroll+plWinHeight-curpage->links[i].posy-1);
          if (d<bestd) { bestd=d; bestmatch=i; };
        };

        link_ind=bestmatch;
        curlink=&curpage->links[link_ind];
      };
    };

    break;
  case 0x5100: //pgdn
    plHelpScroll+=plWinHeight;

    if (plHelpScroll>(plHelpHeight-plWinHeight))
      plHelpScroll=plHelpHeight-plWinHeight;

    if (curpage->linkcount)
    {
      if (!linkOnCurrentPage(curlink))
      {
        int bestmatch, bestd;

        bestd=2000000; bestmatch=-1;

        for (int i=0; i<curpage->linkcount; i++)
        {
          int d=ABS(plHelpScroll-curpage->links[i].posy);
          if (d<bestd) { bestd=d; bestmatch=i; };
        };

        link_ind=bestmatch;
        curlink=&curpage->links[link_ind];
      };
    };

    break;
  case 0x4700: //home
    plHelpScroll=0;

    break;
  case 0x4F00: //end
    plHelpScroll=plHelpHeight-plWinHeight;

    break;
  case 0x2e00: //alt-c
    brSetPage(brDecodeRef("Contents"));

    break;
  case 0x1700: //alt-i
    brSetPage(brDecodeRef("Index"));

    break;
  case 0x2600: //alt-l
    brSetPage(brDecodeRef("License"));

    break;
/*case 0x0009: //tab
    if (curpage->linkcount)
    {
      if (linkOnCurrentPage(curlink))
      {
        link_ind=(link_ind+1)%curpage->linkcount;
        curlink=&curpage->links[link_ind];
      };

      if (!linkOnCurrentPage(curlink)) plHelpScroll=curlink->posy;
    };

    break;
  case 0x0f00: //shift-tab
    if (curpage->linkcount)
    {
      if (linkOnCurrentPage(curlink))
      {
        link_ind--; if (link_ind<0) link_ind=curpage->linkcount-1;
        curlink=&curpage->links[link_ind];
      };

      if (!linkOnCurrentPage(curlink)) plHelpScroll=curlink->posy;
    };

    break;*/
  case 0x3920: case 0x3900: case 0x0020: case 0x000D: // space/enter
    processActiveHyperlink();
    break;
  default:
    return 0;
  }
  if ((plHelpScroll+plWinHeight)>plHelpHeight)
    plHelpScroll=plHelpHeight-plWinHeight;
  if (plHelpScroll<0)
    plHelpScroll=0;
  return 1;
}

static int hlpGlobalInit()
{
  plHelpHeight=plHelpScroll=0;

  if (!plReadHelpExternal())
  {
    if (!plReadHelpPack()) return errGen;
  };

  curpage=NULL;


  helppage *pg=brDecodeRef("Contents");
  if (!pg) HelpfileErr=hlpErrBadFile; else brSetPage(pg);

  return errOk;
};

void hlpFreePages()
{
  for (int i=0; i<Helppages; i++)
  {
    if (Page[i].data)
    {
      delete[] Page[i].data;
      Page[i].data=NULL;
    };

    if (Page[i].rendered)
    {
      delete[] Page[i].rendered;
      Page[i].rendered=NULL;
    };

    if (Page[i].links)
    {
      delete[] Page[i].links;
      Page[i].links=NULL;
    };
  };

  delete Page;
  Page=NULL;

  curpage=NULL;
  curlink=NULL;

  Helppages=link_ind=0;
  HelpfileErr=hlpErrNoFile;
};

static void hlpGlobalClose()
{
  hlpFreePages();
};

extern "C"
{
  initcloseregstruct hlpReg = { hlpGlobalInit, hlpGlobalClose };

  char *dllinfo = "initcloseafter _hlpReg";
}
