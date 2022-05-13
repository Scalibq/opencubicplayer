// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// CPIface text mode volume control interface code
//
// revision history: (please note changes here)
//  -fd9807??   Felix Domke    <tmbinc@gmx.net>
//    -first release
//  -kb990531   Tammo Hinrichs <opencp@gmx.net>
//    -put enumeration of volregs into the Init event handler
//     (was formerly in InitAll, thus preventing detection of
//      volregs only available AFTER the interface startup)
//  -ryg990609  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
//    -togglebutton support (and how i did it... god, forgive me :)
//  -fd991113   Felix Domke  <tmbinc@gmx.net>
//    -added support for scrolling

// cp.ini:
// [screen]
//   volctrl80=off         ; volctrl active by default only in 132column mode,
//   volctrl132=on         ; not in 80.
// and maybe:
// [sound]
//   volregs=...           ; (same as in dllinfo's volregs)

#include <conio.h>
#include <string.h>
#include "poutput.h"
#include "plinkman.h"
#include "psetting.h"
#include "cpiface.h"
#include "imsrtns.h"
#include "vol.h"

class cpiVolCtrl_c
{
  static enum modeenum { modeNone=0, mode80=1, mode52=2 } mode;
  static int focus, active;
  static int x0, y0, x1, y1, yoff;       // origin x, origin y, width, height
  static int AddVolsByName(char *n);
  static int GetVols();
  static int vols;                       // number of registered vols
  static struct regvolstruct             // the registered vols
  {
    ocpvolregstruct *volreg;
    int id;
  } vol[100];                            // TODO: dynamically? on the other hand: which display has more than 100 lines? :)
public:
  static int GetWin(cpitextmodequerystruct &q);
  static void SetWin(int xmin, int xwid, int ymin, int ywid);
  static void Draw(int focus);
  static int IProcessKey(unsigned short);
  static int AProcessKey(unsigned short);
  static int Event(int ev);
};

cpiVolCtrl_c::modeenum cpiVolCtrl_c::mode;
int cpiVolCtrl_c::focus, cpiVolCtrl_c::active;
int cpiVolCtrl_c::x0;
int cpiVolCtrl_c::y0;
int cpiVolCtrl_c::x1;
int cpiVolCtrl_c::y1;
int cpiVolCtrl_c::yoff;
int cpiVolCtrl_c::vols;
cpiVolCtrl_c::regvolstruct cpiVolCtrl_c::vol[100];

static int cpiVolCtrl_c::AddVolsByName(char *n)
{
  ocpvolregstruct *x=(ocpvolregstruct *)lnkGetSymbol(n);
  if(!x) return(0);
  int num=x->GetVolumes();
  for(int i=0; i<num; i++)
  {
    if(vols>=100) return(0);
    ocpvolstruct y;
    if(x->GetVolume(y, i))
    {
      vol[vols].volreg=x;
      vol[vols].id=i;
      vols++;
    }
  }
  return(!0);
}

static int cpiVolCtrl_c::GetVols()
{
  vols=0;
  char const *dllinfo=lnkReadInfoReg("volregs");
  if(dllinfo)
  {
    int num=cfCountSpaceList(dllinfo, 100);
    for(int i=0; i<num; i++)
    {
      char buf[100];
      cfGetSpaceListEntry(buf, dllinfo, 100);
      AddVolsByName(buf);
    }
  }
  dllinfo=cfGetProfileString("sound", "volregs", 0);
  if(dllinfo)
  {
    int num=cfCountSpaceList(dllinfo, 100);
    for(int i=0; i<num; i++)
    {
      char buf[100];
      cfGetSpaceListEntry(buf, dllinfo, 100);
      if(!AddVolsByName(buf)) return(0);
    }
  }
  return(!0);
}

static int cpiVolCtrl_c::GetWin(cpitextmodequerystruct &q)
{
  switch(cpiVolCtrl_c::mode)
  {
   case(modeNone):
    return(0);
   case(mode80):
    q.top=0;
    q.xmode=1;
    break;
   case(mode52):
    q.top=0;
    q.xmode=2;
    break;
   default:
    break;
  }
  q.killprio=128;                // don't know in which range they are..
  q.viewprio=20;
  q.size=1;                      // ?!?
  q.hgtmin=3;
  q.hgtmax=(!vols)?1:(vols+1);
  return(1);
}

static void cpiVolCtrl_c::SetWin(int xmin, int xwid, int ymin, int ywid)
{
  x0=xmin;
  y0=ymin;
  x1=xwid;
  y1=ywid;
}

static void cpiVolCtrl_c::Draw(int focus)
{
  short buf[800];
  memset(buf, 0, 160);
  if(vols)
    writestring(buf, 3, focus?0x9:0x1, "volume control", x1);
  else
    writestring(buf, 3, focus?0x9:0x1, "volume control: no volume regs", x1);
  displaystrattr(y0, x0, buf, x1);

  if (!vols)
    return;

  int ll=0;
  for(int i=0; i<vols; i++)
  {
    ocpvolstruct x;
    vol[i].volreg->GetVolume(x, vol[i].id);

    char name[256];
    strcpy(name, x.name);

    if (strchr(name, '\t'))
      *strchr(name, '\t')=0;                   // don't forget this one :)

    if (strlen(name)>ll) ll=strlen(name);
  }
  int barlen=x1-ll-5;
  if(barlen<4) { barlen=4; ll=x1-9; }

  if ((active-yoff)<0)
    yoff=active;
  if ((active-yoff)>=(y1-1))
    yoff=active-y1+2;

  if ((yoff+(y1-1))>vols)
    yoff=(y1-1)-vols;
  if (yoff<0) // shouldn't happen, unless we get a bigger window than we need
    yoff=0;

  int su=0, sd=0;

  if (vols>(y1-1))
    su=sd=1;

  if (yoff<=(vols-y1-1))
    sd++;
  if (yoff)
    su++;

  for(i=yoff; i<(yoff+(y1-1)); i++)
  {
    ocpvolstruct x;
    int hc=focus?((i==active)?7:8):8;
    vol[i].volreg->GetVolume(x, vol[i].id);

    char  name[256];     // you won't do labels with >255 chars, WILL YOU? :)
    strncpy(name, x.name, ll);
    name[ll]=0;
    if (strchr(name, '\t'))
      *strchr(name, '\t')=0;                              // nice hack.

//  if(strlen(x.name)>ll) x.name[ll]=0;                 // ugly hack, but who cares? :)
    *buf=' ';
    if (!(i-yoff))
      if (su--)
        writestring(buf, 0, su?0x07:0x08, "\x18", 1);
    if (i==(yoff+y1-2))
      if (sd--)
        writestring(buf, 0, sd?0x07:0x08, "\x19", 1);

    writestring(buf, 1, hc, name, ll);                    // ('len' seems to be ingnored by writestring)
    writestring(buf, ll+1, hc, " [", ll);
    writestring(buf, ll+barlen+3, hc, "] ", ll);

    if (!x.min && x.max<0)          // this hack enables togglebuttons (ouch)
    {
      // --- how to do toglebuttons ---
      // set volreg.min to 0, volreg.max to -<choices>, and use \t as
      // delimiter in the name-field (for the values).
      // hurts, but it works :)

      char   sbuf[512], *ptr=&sbuf[0];
      int    i;

      strcpy(sbuf, x.name);
      for (i=x.val+1, ptr=&sbuf[0]; i && *ptr; ptr++)
        if (*ptr=='\t') i--;

      memsetw(&buf[ll+3], (hc<<8)+0x20, barlen);

      if (!*ptr || i)
      {
        strcpy(sbuf, "(NULL)");
        ptr=&sbuf[0];
      }

      if (strchr(ptr, '\t'))
        *strchr(ptr, '\t')=0;

      if (strlen(ptr)>=barlen)
        ptr[barlen]=0;

      int of=(barlen-strlen(ptr))>>1;

      for (int po=of; po<strlen(ptr)+of; po++)
        buf[po+ll+3]=ptr[po-of]|((hc-7)?0x800:0x900);
    }
    else
    {
      int p=((x.val-x.min)*(barlen))/(x.max-x.min);       // range: 0..barlen

      p=(p>barlen)?barlen:(p<0)?0:p;

      for (int po=0; po<barlen; po++)
      {
        int p4=(po<<2)/barlen;                    // range: 0..4
        if(po<p)
          buf[po+ll+3]='þ'| ( (i==active && focus)? (("\x01\x09\x0b\x0f"[p4>3?3:p4])<<8):0x0800);
        else
          buf[po+ll+3]='ú'|(hc<<8);
      }
    }

    displaystrattr(y0+i+1-yoff, x0, buf, x1);
  }
}

static int cpiVolCtrl_c::IProcessKey(unsigned short key)
{
  switch (key)
  {
   case 'm':
    if((focus)||(mode==modeNone))
    {
      mode=(modeenum)(((int)mode+1)%3);
      if((mode==mode52)&&(plScrWidth<132)) mode=modeNone;
      if(mode!=modeNone)
        cpiTextSetMode("volctrl");
      cpiTextRecalc();
    } else
    {
      cpiTextSetMode("volctrl");
    }
    break;
   case 0x4800: // arup
    if(focus&&vols)
    {
      active--;
      if(active<0) active=vols-1;
      Draw(focus);
      break;
    }
    return 0;
   case 0x5000: // ardown
    if(focus&&vols)
    {
      active++;
      if (active>(vols-1)) active=0;
      Draw(focus);
      break;
    }
    return 0;
   case 0x4d00: // arright
    if(focus&&vols)
    {
      ocpvolstruct x;
      vol[active].volreg->GetVolume(x, vol[active].id);

      if (!x.min && x.max<0)
      {
        x.val++;
        if (x.val>=-x.max) x.val=0;
        if (x.val<0) x.val=-x.max-1;
      }
      else
      {
        x.val+=x.step;
        if(x.val>x.max) x.val=x.max;
        if(x.val<x.min) x.val=x.min;
      }

      vol[active].volreg->SetVolume(x, vol[active].id);
      break;
    }
    return 0;
   case 0x4b00: // arleft
    if(focus&&vols)
    {
      ocpvolstruct x;
      vol[active].volreg->GetVolume(x, vol[active].id);

      if (!x.min && x.max<0)
      {
        x.val--;
        if (x.val>=-x.max) x.val=0;
        if (x.val<0) x.val=-x.max-1;
      }
      else
      {
        x.val-=x.step;
        if(x.val>x.max) x.val=x.max;
        if(x.val<x.min) x.val=x.min;
      }

      vol[active].volreg->SetVolume(x, vol[active].id);
      break;
    }
    return 0;
   case 'x': case 'X':
    if(mode)
    {
      mode=mode52;
      if(plScrWidth<132) mode=mode80;
      cpiTextRecalc();
    }
    return 0;
   case 0x2d00: //alt-x
    if(mode)
    {
      mode=mode80;
      cpiTextRecalc();
    }
    return 0;
   default:
    return 0;
  }
  return 1;
}

static int cpiVolCtrl_c::AProcessKey(unsigned short key)
{
  return 1;
}

static int cpiVolCtrl_c::Event(int ev)
{
//      0          1           2          3          4             5
// cpievOpen, cpievClose, cpievInit, cpievDone, cpievInitAll, cpievDoneAll,
// cpievGetFocus, cpievLoseFocus, cpievSetMode,      ?
//      6              7               8            42
  switch(ev)
  {
   case cpievInit:
    GetVols();
    mode=modeNone;
    return(!!vols);
   case cpievInitAll:
    return(1);
   case cpievOpen:
    mode=modeNone;
    return(1);
   case cpievGetFocus:
    focus=!0;
    return(1);
   case cpievLoseFocus:
    focus=0;
    return(1);
   case cpievSetMode:
    if(cfGetProfileBool("screen", plScrWidth<132?"volctrl80":"volctrl132", plScrWidth<132?0:!0, plScrWidth<132?0:!0))
    {
      mode=mode52;
      if(plScrWidth<132) mode=mode80;
      cpiTextRecalc();
    }
    return(1);
  }
  return 0;
}

extern "C"
{
  cpitextmoderegstruct cpiVolCtrl={"volctrl",
                                   cpiVolCtrl_c::GetWin,
                                   cpiVolCtrl_c::SetWin,
                                   cpiVolCtrl_c::Draw,
                                   cpiVolCtrl_c::IProcessKey,
                                   cpiVolCtrl_c::AProcessKey,
                                   cpiVolCtrl_c::Event};
};
