// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CP hypertext help viewer
//
// revision history: (please note changes here)
//  -fg980924  Fabian Giesen <gfabian@jdcs.su.nw.schule.de>
//    -first version (mainly for wrappers)

#ifndef _cphelper_h
#define _cphelper_h

typedef struct link {
          int   posx, posy, len;
          void *ref;
        } link;

typedef struct llink {
          int   posx, posy, len;
          void *ref;
          struct llink *next;
        } link_list;

typedef struct helppage {
          char   name[128];
          char   desc[128];
          char  *data;
          short *rendered;
          int    linkcount;
          link  *links;
          int    size, lines;
        } helppage;

#define hlpErrOk       0
#define hlpErrNoFile   1
#define hlpErrBadFile  2
#define hlpErrTooNew   3

extern helppage *brDecodeRef(char *name);
extern void brRenderPage(helppage *pg);
extern void brSetPage(helppage *pg);
extern void brDisplayHelp();
extern void brSetWinStart(short fl);
extern void brSetWinHeight(short h);
extern int brHelpKey(unsigned short key);

#endif
