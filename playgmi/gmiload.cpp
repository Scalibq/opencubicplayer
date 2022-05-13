// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// GMIPlay MID/RMI file loader
//
// revision history: (please note changes here)
//  -nb980510   Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//    -first release
//  -kbwhenever Tammo Hinrichs <opencp@gmx.net>
//    -changed path searching for ULTRASND.INI and patch files
//  -ryg_xmas   Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
//    -the .FFF hack, part I - untested, not integrated. wish you a nice
//     time debugging it...
//  -fd990122   Felix Domke    <tmbinc@gmx.net>
//    -integrated the .FFF-hack, improved it, removed some silly bugs,
//     but it's still not a loader... (and it's far away from that.)
//     (some hours later: ok, some things are really loaded now.
//      envelopes are ignored completely, maybe THIS is the error...
//      you won't hear anything yet, sorry.)
//     anyway: i am wondering how much changes we need to support all
//     features of that fff :(
//  -fd990124   Felix Domke    <tmbinc@gmx.net>
//    -continued on the work. hmm, some sound will come out of your speakers
//     right now, but it sounds just TERRIBLE. :)
//  -ryg990125  Fabian Giesen  <fabian@jdcs.su.nw.schule.de>
//    -corrected tmbs formulas by combining beisert's ones with some from
//     GIPC (as Curtis Patzer, programmer of GIPC, would put it: a glorious
//     hack). and, believe me, the songs start to sound like they should :)
//    -added æ-law (mu-law) decoding table (nicer than your code, tmb)
//    -now displays also instrument names (if program number in GM range
//     this means <128)
//  -kb990208  Tammo Hinrichs <kb@vmo.nettrade.de>
//    -fixed some too obvious bugs
//     ( for (i=...;..;..)
//       {
//         ...
//         for (i=...;..;..) ... ;
//         ...
//       }; et al.)


//              can anybody explain me in which ranges all the values are? ;)

/*
        do THIS in your cp.ini...

[midi]
  usefff=yes                ; ...or just "no" if you still want to hear midis
  dir=d:\utoplite\          ; where are your DATs located? :)
                            ; note: if you use fffs converted by gipc,
                            ; just write "dir=", because gipc includes
                            ; the path to the .dat-file.
  fff=d:\utoplite\utopi_li.fff

*/

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <conio.h>
#include "psetting.h"
#include "binfstd.h"
#include "binfile.h"
#include "mcp.h"
#include "gmiplay.h"
#include "err.h"
#include "imsrtns.h"

/*
 * æ-law (mu-law) decoding table (shamelessly stolen from SOX)
 */

static int ulaw_exp_table[] = {
	 -32124,-31100,-30076,-29052,-28028,-27004,-25980,-24956,
	 -23932,-22908,-21884,-20860,-19836,-18812,-17788,-16764,
	 -15996,-15484,-14972,-14460,-13948,-13436,-12924,-12412,
	 -11900,-11388,-10876,-10364, -9852, -9340, -8828, -8316,
	  -7932, -7676, -7420, -7164, -6908, -6652, -6396, -6140,
	  -5884, -5628, -5372, -5116, -4860, -4604, -4348, -4092,
	  -3900, -3772, -3644, -3516, -3388, -3260, -3132, -3004,
	  -2876, -2748, -2620, -2492, -2364, -2236, -2108, -1980,
	  -1884, -1820, -1756, -1692, -1628, -1564, -1500, -1436,
	  -1372, -1308, -1244, -1180, -1116, -1052,  -988,  -924,
	   -876,  -844,  -812,  -780,  -748,  -716,  -684,  -652,
	   -620,  -588,  -556,  -524,  -492,  -460,  -428,  -396,
	   -372,  -356,  -340,  -324,  -308,  -292,  -276,  -260,
	   -244,  -228,  -212,  -196,  -180,  -164,  -148,  -132,
	   -120,  -112,  -104,   -96,   -88,   -80,   -72,   -64,
	    -56,   -48,   -40,   -32,   -24,   -16,    -8,     0,
	  32124, 31100, 30076, 29052, 28028, 27004, 25980, 24956,
	  23932, 22908, 21884, 20860, 19836, 18812, 17788, 16764,
	  15996, 15484, 14972, 14460, 13948, 13436, 12924, 12412,
	  11900, 11388, 10876, 10364,  9852,  9340,  8828,  8316,
	   7932,  7676,  7420,  7164,  6908,  6652,  6396,  6140,
	   5884,  5628,  5372,  5116,  4860,  4604,  4348,  4092,
	   3900,  3772,  3644,  3516,  3388,  3260,  3132,  3004,
	   2876,  2748,  2620,  2492,  2364,  2236,  2108,  1980,
	   1884,  1820,  1756,  1692,  1628,  1564,  1500,  1436,
	   1372,  1308,  1244,  1180,  1116,  1052,   988,   924,
	    876,   844,   812,   780,   748,   716,   684,   652,
	    620,   588,   556,   524,   492,   460,   428,   396,
	    372,   356,   340,   324,   308,   292,   276,   260,
	    244,   228,   212,   196,   180,   164,   148,   132,
	    120,   112,   104,    96,    88,    80,    72,    64,
	     56,    48,    40,    32,    24,    16,     8,     0};


int addpatchFFF( minstrument&, uint1, unsigned char, unsigned char,
                 sampleinfo&, unsigned short&);

int loadpatchFFF(minstrument &, uint1, unsigned char*,
                 sampleinfo *&, unsigned short &);

extern int midUseFFF;

char *midInstrumentPaths[4];
char *midInstrumentNames[256];

static inline unsigned short convshort(unsigned short a)
{
  return ((a&0xFF)<<8)|((a&0xFF00)>>8);
}

static inline unsigned long convlong(unsigned long a)
{
  return ((a&0xFF)<<24)|((a&0xFF00)<<8)|((a&0xFF0000)>>8)|((a&0xFF000000)>>24);
}

static inline unsigned long readvlnum(unsigned char *&ptr)
{
  unsigned long num=0;
  while (1)
  {
    num=(num<<7)|(*ptr&0x7F);
    if (!(*ptr++&0x80))
      break;
  }
  return num;
}

struct PATCHHEADER
{
  char header[12]; /* "GF1PATCH110" */
  char gravis_id[10];   /* "ID#000002" */
  char description[60];
  unsigned char instruments;
  char voices;
  char channels;
  unsigned short wave_forms;
  unsigned short master_volume;
  unsigned long data_size;
  char reserved[36];
};

struct INSTRUMENTDATA
{
  unsigned short instrument;
  char instrument_name[16];
  long instrument_size;
  char layers;
  char reserved[40];
};

struct LAYERDATA
{
  char layer_duplicate;
  char layer;
  long layer_size;
  char samples;
  char reserved[40];
};

struct PATCHDATA
{
  char wave_name[7];

  unsigned char fractions;
  long wave_size;
  long start_loop;
  long end_loop;

  unsigned short sample_rate;
  long low_frequency;
  long high_frequency;
  long root_frequency;
  short tune;

  unsigned char balance;

  unsigned char envelope_rate[6];
  unsigned char envelope_offset[6];

  unsigned char tremolo_sweep;
  unsigned char tremolo_rate;
  unsigned char tremolo_depth;

  unsigned char vibrato_sweep;
  unsigned char vibrato_rate;
  unsigned char vibrato_depth;

   /* bit 5 = Turn sustaining on. (Env. pts. 3)*/
   /* bit 6 = Enable envelopes - 1 */
   /* bit 7 = fast release */
  char modes;

  short scale_frequency;
  unsigned short scale_factor;    /* from 0 to 2048 or 0 to 2 */

  char reserved[36];
};

static unsigned long pocttab[16]={2044, 4088, 8176, 16352, 32703, 65406, 130813, 261626, 523251, 1046502, 2093005, 4186009, 8372018, 16744036, 33488072, 66976145};
static unsigned short pnotetab[12]={32768, 34716, 36781, 38968, 41285, 43740, 46341, 49097, 52016, 55109, 58386, 61858};
static unsigned short pfinetab[16]={32768, 32887, 33005, 33125, 33245, 33365, 33486, 33607, 33728, 33850, 33973, 34095, 34219, 34343, 34467, 34591};
static unsigned short pxfinetab[16]={32768, 32775, 32783, 32790, 32798, 32805, 32812, 32820, 32827, 32835, 32842, 32849, 32857, 32864, 32872, 32879};

static signed short getnote(unsigned long frq)  // frq=freq*1000, res=(oct*12+note)*256 (and maybe +fine*16+xfine)
{
  signed short x;
  int i;
  for (i=0; i<15; i++)
    if (pocttab[i+1]>frq)
      break;
  x=(i-1)*12*256;
  frq=umuldiv(frq, 32768, pocttab[i]);
  for (i=0; i<11; i++)
    if (pnotetab[i+1]>frq)
      break;
  x+=i*256;
  frq=umuldiv(frq, 32768, pnotetab[i]);
  for (i=0; i<15; i++)
    if (pfinetab[i+1]>frq)
      break;
  x+=i*16;
  frq=umuldiv(frq, 32768, pfinetab[i]);
  for (i=0; i<15; i++)
    if (pxfinetab[i+1]>frq)
      break;
  return x+i;
}

static unsigned long getfreq(short note)
{
  int freq=pocttab[note/256/12+1];
  freq=umuldiv(freq, pnotetab[(note/256)%12], 32768);
  freq=umuldiv(freq, pfinetab[(note/16)&0xF], 32768);
  return(umuldiv(freq, pxfinetab[(note)&0xF], 32768));  // (x-)finetuning not VERY much tested.
}

/*
 * the .FFF-hack (loader? not really) begins *here*
 * (YES, i'm now using binfile-types, increases portability [as if that had
 *  any use... {nested braces rule! :) }])
 */

#ifdef __WATCOMC__
#pragma pack (push, 1)  // no structure alignment (tmb always forgets those)
#endif

struct FFF_CHUNK_HEADER
{
  char  id[4];
  uint4 size;
};

union FFF_ID
{
  struct
  {
    uint2 maj_id, min_id;
  };

  uint4  id;
  void  *ptr;
};

struct FFF_ENVELOPE_POINT
{
  uint2 next;

  union
  {
    uint2 rate;
    uint2 time;
  };
};

struct FFF_LFO
{
  uint2  freq;
  int2   depth;
  int2   sweep;
  uint1  shape;         // no ryg, a byte is NOT an uint8 ;) (fd)
  uint1  delay;         // a normal rounding error, nothing more :) (ryg)
};

struct FFF_ENVELOPE_RECORD
{
  int2                nattack;
  int2                nrelease;
  uint2               sustain_offset;
  uint2               sustain_rate;
  uint2               release_rate;
  uint1               hirange;
  uint1               d0;
  FFF_ENVELOPE_POINT *attack_points;
  FFF_ENVELOPE_POINT *release_points;
};

struct FFF_ENVP_CHUNK
{
  FFF_ID               id;
  uint1                num_envelopes;
  uint1                flags; // bit0=retrigger, rest unused/unimportant
  uint1                mode;
  uint1                indtype;
  FFF_ENVELOPE_RECORD *records;
};

struct FFF_PROG_CHUNK
{
  FFF_ID            id;
  FFF_ID            version;
};

struct FFF_WAVE_CHUNK
{
  FFF_ID            id;
  uint4             size;
  uint4             start;
  uint4             loopstart;
  uint4             loopend;
  uint4             m_start;
  uint4             sample_ratio;
  uint1             attenuation;
  uint1             low_note;
  uint1             high_note;
  uint1             format;
  uint1             m_format;
  FFF_ID            data_id;
};

struct FFF_LAYR_CHUNK
{
  FFF_ID            id;
  uint1             nwaves;
  uint1             flags;
  uint1             high_range;
  uint1             low_range;
  uint1             pan;
  uint1             pan_freq_scale;
  FFF_LFO           tremolo;
  FFF_LFO           vibrato;
  uint1             velocity_mode;
  uint1             attenuation;
  int2              freq_scale;
  uint1             freq_center;
  uint1             layer_event;
  FFF_ID            penv;
  FFF_ID            venv;
  FFF_WAVE_CHUNK   *waves;
};

struct FFF_PTCH_CHUNK
{
  FFF_ID            id;
  int2              nlayers;
  uint1             layer_mode;
  uint1             excl_mode;
  int2              excl_group;
  uint1             effect1;
  uint1             effect1_depth;
  uint1             effect2;
  uint1             effect2_depth;
  uint1             bank;
  uint1             program;
  FFF_LAYR_CHUNK   *iw_layer;
};

struct FFF_DATA_CHUNK
{
  FFF_ID            id;
  char              filename[256];
};

#ifdef __WATCOMC__
#pragma pack (pop)  // reset structure packing
#endif

struct FFF_ENVP_LIST
{
  FFF_ENVP_CHUNK *chunk;
  FFF_ENVP_LIST  *next;
};

struct FFF_PTCH_LIST
{
  FFF_PTCH_CHUNK *chunk;
  FFF_PTCH_LIST  *next;
};

struct FFF_DATA_LIST
{
  FFF_DATA_CHUNK *chunk;
  FFF_DATA_LIST  *next;
};


FFF_ENVP_LIST *envp_list=0;
FFF_PTCH_LIST *ptch_list=0;
FFF_DATA_LIST *data_list=0;

void *getENVP(uint4 id)
{
  FFF_ENVP_LIST *l=envp_list;

  while (l) { if (l->chunk->id.id==id) return l->chunk; l=l->next; };

  return 0;
};

void *getDATA(uint4 id)
{
  FFF_DATA_LIST *l=data_list;

  while (l) { if (l->chunk->id.id==id) return l->chunk; l=l->next; };

  return 0;
};

int loadFFF(binfile &file)
{
  FFF_CHUNK_HEADER  hd;
  int               i, j, matched;

  file.ioctl(binfile::ioctlrrbufset, 0, 65536);  // yeah, we got the power of the new binfile :)

// pass one: read the .FFF file

  file.read(&hd, sizeof(hd));

  if (strncmp(hd.id, "FFFF", 4)) return 0;

  if (!file.eread(&hd, sizeof(hd))) return 0;

  while (1) //(strncmp(hd.id, "FFFF", 4)) ryg, i don't understand your code... (fd)  me too :) (ryg)
  {
    matched=0;


#ifdef __DEBUG__
    printf("[FFF] got %c%c%c%c-chunk\n", hd.id[0], hd.id[1], hd.id[2], hd.id[3]);
#endif

    if (!strncmp(hd.id, "ENVP", 4))
    {
      FFF_ENVP_LIST       *l;
      FFF_ENVELOPE_RECORD *lcr;

      matched=1;

      l=new FFF_ENVP_LIST;
      l->chunk=new FFF_ENVP_CHUNK;
      l->next=envp_list;
      envp_list=l;

      if (!file.eread(&l->chunk->id.id, 4)) return 0;
      if (!file.eread(&l->chunk->num_envelopes, 1)) return 0;
      if (!file.eread(&l->chunk->flags, 1)) return 0;
      if (!file.eread(&l->chunk->mode, 1)) return 0;
      if (!file.eread(&l->chunk->indtype, 1)) return 0;

      l->chunk->records=new FFF_ENVELOPE_RECORD[l->chunk->num_envelopes];
      lcr=l->chunk->records;

      for (i=0; i<l->chunk->num_envelopes; i++)
      {
        if (!file.eread(&lcr[i].nattack, 2)) return 0;
        if (!file.eread(&lcr[i].nrelease, 2)) return 0;
        if (!file.eread(&lcr[i].sustain_offset, 2)) return 0;
        if (!file.eread(&lcr[i].sustain_rate, 2)) return 0;
        if (!file.eread(&lcr[i].release_rate, 2)) return 0;
        if (!file.eread(&lcr[i].hirange, 1)) return 0;
        if (!file.eread(&lcr[i].d0, 1)) return 0;

        if (lcr[i].nattack)
          lcr[i].attack_points=new FFF_ENVELOPE_POINT[lcr[i].nattack];
        else
          lcr[i].attack_points=0;

        if (lcr[i].nrelease)
          lcr[i].release_points=new FFF_ENVELOPE_POINT[lcr[i].nrelease];
        else
          lcr[i].release_points=0;

        if (lcr[i].nattack && !file.eread(lcr[i].attack_points,
                sizeof(FFF_ENVELOPE_POINT)*lcr[i].nattack)) return 0;
        if (lcr[i].nrelease && !file.eread(lcr[i].release_points,
             sizeof(FFF_ENVELOPE_POINT)*lcr[i].nrelease)) return 0;
      };
    };

    if (!strncmp(hd.id, "PROG", 4))
    {
      matched=1;

      file.getl();
      file.getl();

#ifdef __DEBUG__
      printf("[FFF] skipped program\n");
#endif
    };

    if (!strncmp(hd.id, "PTCH", 4))
    {
      FFF_PTCH_LIST  *l;
      FFF_PTCH_CHUNK *c;
      FFF_LAYR_CHUNK *lr;

#ifdef __DEBUG__
      printf("[FFF] loading patch\n");
#endif

      matched=1;

      l=new FFF_PTCH_LIST;
      l->chunk=new FFF_PTCH_CHUNK;
      l->next=ptch_list;
      ptch_list=l;

      c=l->chunk;

      if (!file.eread(&c->id.id, 4)) return 0;
      if (!file.eread(&c->nlayers, 2)) return 0;
      if (!file.eread(&c->layer_mode, 1)) return 0;
      if (!file.eread(&c->excl_mode, 1)) return 0;
      if (!file.eread(&c->excl_group, 2)) return 0;
      if (!file.eread(&c->effect1, 1)) return 0;
      if (!file.eread(&c->effect1_depth, 1)) return 0;
      if (!file.eread(&c->effect2, 1)) return 0;
      if (!file.eread(&c->effect2_depth, 1)) return 0;
      if (!file.eread(&c->bank, 1)) return 0;
      if (!file.eread(&c->program, 1)) return 0;
      file.getl();      // actually, iw_layer isn't used on disk, but it's there (fd) yup (ryg)

#ifdef __DEBUG__
      printf("[FFF] %d:%d loading, %d layers.\n", c->bank, c->program, c->nlayers);
#endif

      c->iw_layer=new FFF_LAYR_CHUNK[c->nlayers];

      for (i=0; i<c->nlayers; i++)
      {
        FFF_CHUNK_HEADER  lrhd;
        FFF_WAVE_CHUNK   *wv;

        lr=&c->iw_layer[i];

        if (!file.eread(&lrhd, sizeof(lrhd))) return 0;

        if (strncmp(lrhd.id, "LAYR", 4)) // sachmal ryg, wielange warste schon auf als du DAS hier gecoded hast? merke: strcmp==0 means "gleich"! ;) (fd) 17 stunden, wieso? (ryg)
        {
          printf("[FFF] non-LAYR chunk in PTCH (malformed FFF file)\n");
          printf("[FFF] (found %c%c%c%c-chunk)\n", lrhd.id[0], lrhd.id[1], lrhd.id[2], lrhd.id[3]);
          return 0;
        };

        if (!file.eread(&lr->id.id, 4)) return 0;
        if (!file.eread(&lr->nwaves, 1)) return 0;
        if (!file.eread(&lr->flags, 1)) return 0;
        if (!file.eread(&lr->high_range, 1)) return 0;
        if (!file.eread(&lr->low_range, 1)) return 0;
        if (!file.eread(&lr->pan, 1)) return 0;
        if (!file.eread(&lr->pan_freq_scale, 1)) return 0;
        if (!file.eread(&lr->tremolo, sizeof(FFF_LFO))) return 0;
        if (!file.eread(&lr->vibrato, sizeof(FFF_LFO))) return 0;
        if (!file.eread(&lr->velocity_mode, 1)) return 0;
        if (!file.eread(&lr->attenuation, 1)) return 0;
        if (!file.eread(&lr->freq_scale, 2)) return 0;
        if (!file.eread(&lr->freq_center, 1)) return 0;
        if (!file.eread(&lr->layer_event, 1)) return 0;
        if (!file.eread(&lr->penv.id, 4)) return 0;
        if (!file.eread(&lr->venv.id, 4)) return 0;
        file.getl();

        lr->waves=new FFF_WAVE_CHUNK[lr->nwaves];

#ifdef __DEBUG__
        printf("[FFF] ... %d waves.\n", lr->nwaves);
#endif

        for (int j=0; j<lr->nwaves; j++)
        {
          FFF_CHUNK_HEADER  wvhd;

          wv=&lr->waves[j];

          if (!file.eread(&wvhd, sizeof(wvhd))) return 0;

          if (strncmp(wvhd.id, "WAVE", 4))
          {
            printf("[FFF] non-WAVE chunk in LAYR (malformed FFF file)\n");
            printf("[FFF] (found %c%c%c%c-chunk)\n", wvhd.id[0], wvhd.id[1], wvhd.id[2], wvhd.id[3]);
            return 0;
          };

          if (!file.eread(&wv->id.id, 4)) return 0;
          if (!file.eread(&wv->size, 4)) return 0;
          if (!file.eread(&wv->start, 4)) return 0;
          if (!file.eread(&wv->loopstart, 4)) return 0;
          if (!file.eread(&wv->loopend, 4)) return 0;
          if (!file.eread(&wv->m_start, 4)) return 0;
          if (!file.eread(&wv->sample_ratio, 4)) return 0;
          if (!file.eread(&wv->attenuation, 1)) return 0;
          if (!file.eread(&wv->low_note, 1)) return 0;
          if (!file.eread(&wv->high_note, 1)) return 0;
          if (!file.eread(&wv->format, 1)) return 0;
          if (!file.eread(&wv->m_format, 1)) return 0;
          if (!file.eread(&wv->data_id.id, 4)) return 0;
          printf("wave %d loaded.. (%d bytes, playable from %x to %x)\n", i, wv->size, wv->low_note, wv->high_note);
        };
      };
    };

    if (!strncmp(hd.id, "DATA", 4))
    {
#ifdef __DEBUG__
      printf("[FFF] read DATA chunk\n");
#endif
      FFF_DATA_LIST       *l;
      matched=1;

      l=new FFF_DATA_LIST;
      l->chunk=new FFF_DATA_CHUNK;
      l->next=data_list;
      data_list=l;
      if (!file.eread(&l->chunk->id.id, 4)) return 0;
      if (!file.eread(&l->chunk->filename[0], hd.size-4)) return 0;
    };

    if (!matched)
    {
      file.seekcur(hd.size);
#ifdef __DEBUG__
      printf("[FFF] skipped %c%c%c%c-chunk\n", hd.id[0], hd.id[1], hd.id[2], hd.id[3]);
#endif
    }

    if (!file.eread(&hd, sizeof(hd))) break;
  };

// pass 2: convert those IDs to pointers (yeeah)

  FFF_PTCH_LIST *l;

  l=ptch_list;

  while (l)
  {
    for (i=0; i<l->chunk->nlayers; i++)
    {
      FFF_LAYR_CHUNK *lc;

      lc=&l->chunk->iw_layer[i];

      if (lc->penv.id)
      {
        lc->penv.ptr=getENVP(lc->penv.id);
        if (! lc->penv.ptr )
        {
          printf ("penvelop id %x not found.\n", lc->penv.id);
          return 0;
        }
      }

      if (lc->venv.id)
      {
        lc->venv.ptr=getENVP(lc->venv.id);
        if (! lc->venv.ptr )
        {
          printf ("venvelop id %x not found.\n", lc->venv.id);
          return 0;
        }
      }

      for (j=0; j<lc->nwaves; j++)
      {
        lc->waves[j].data_id.ptr=getDATA(lc->waves[j].data_id.id);
        if (! lc->waves[j].data_id.ptr )
        {
          printf ("wavedata file #%x not found...\n", lc->waves[j].data_id.id);
          return 0;
        }
      }
    };

    l=l->next;
  };

  printf("done with that loading. be happy.\n");

  return 1;
};

/*
 * end of .FFF-hack
 */

static int loadsample(short          file,
                      minstrument&   ins,
                      unsigned char  j,
                      unsigned char  vox,
                      char           setnote,
                      unsigned char  sampnum,
                      unsigned char  *sampused,
                      sampleinfo     &sip,
                      unsigned short &samplenum)
{
  msample &s=ins.samples[j];

  PATCHDATA sh;
  read(file, &sh, sizeof(sh));

  unsigned char bit16=!!(sh.modes&1);
  if (bit16)
  {
    sh.wave_size>>=1;
    sh.start_loop>>=1;
    sh.end_loop>>=1;
  }

  if (setnote)
  {
    unsigned char lownote,highnote;
    lownote=(getnote(sh.low_frequency)+0x80)>>8;
    highnote=(getnote(sh.high_frequency)+0x80)>>8;
    int i;
    for (i=lownote; i<highnote; i++)
      if (sampused[i>>3]&(1<<(i&7)))
        break;
    if (i==highnote)
    {
      lseek(file, sh.wave_size<<bit16, SEEK_CUR);
      return 1;
    }
    memset(ins.note+lownote, j, highnote-lownote);
  }

  memcpy(s.name, sh.wave_name, 7);
  s.name[7]=0;
  s.sampnum=sampnum;
  s.handle=-1;
  s.normnote=getnote(sh.root_frequency);
  if ((s.normnote&0xFF)>=0xFE)
    s.normnote=(s.normnote+2)&~0xFF;
  if ((s.normnote&0xFF)<=0x02)
    s.normnote=s.normnote&~0xFF;
  sip.length=sh.wave_size;
  sip.loopstart=sh.start_loop;
  sip.loopend=sh.end_loop;
  sip.samprate=sh.sample_rate;
  sip.type=((sh.modes&4)?(mcpSampLoop|((sh.modes&8)?mcpSampBiDi:0)):0)|(bit16?mcpSamp16Bit:0)|((sh.modes&2)?mcpSampUnsigned:0);
  short q;
//  printf("env: ");
  for (q=0; q<6; q++)
  {
//    s.volrte[q]=((sh.envelope_rate[q]&63)<<(9-3*(sh.envelope_rate[q]>>6)))*1220*16/(64*vox)*2/3;
    s.volrte[q]=(((sh.envelope_rate[q]&63)*11025)>>(3*(sh.envelope_rate[q]>>6)))*14/vox;
    s.volpos[q]=sh.envelope_offset[q]<<8;
//    if (q<s.end) printf("%d:%d ", s.volrte[q], s.volpos[q]);
  }
//  printf("\n");
  s.end=(sh.modes&128)?3:6;
  s.sustain=(sh.modes&32)?3:7;
  s.tremswp=sh.tremolo_sweep*64/45;
  s.vibswp=sh.vibrato_sweep*64/45;
  s.tremdep=sh.tremolo_depth*4*256/255   /2;
  s.vibdep=sh.vibrato_depth*12*256/255   /4;
  s.tremrte=(sh.tremolo_rate*7+15)*65536/(300*64);
  s.vibrte=(sh.vibrato_rate*7+15)*65536/(300*64);

//  printf("   -> %d %d %d, %d %d %d\n", s.tremswp, s.tremrte, s.tremdep,
//                                         s.vibswp, s.vibrte, s.vibdep);

  if (sh.scale_factor<=2)
    s.sclfac=sh.scale_factor<<8;
  else
    s.sclfac=sh.scale_factor>>2;
  s.sclbas=sh.scale_frequency;

  unsigned char *smpp=new unsigned char[sip.length<<bit16];
  if (!smpp)
    return errAllocMem;
  read(file, smpp, sip.length<<bit16);
  sip.ptr=smpp;
  s.handle=samplenum++;
  return errOk;
}


int loadpatch(minstrument    &ins,
              uint1          program,
              unsigned char  *sampused,
              sampleinfo     *&smps,
              unsigned short &samplenum)
{
  if (midUseFFF)
    return loadpatchFFF(ins, program, sampused, smps, samplenum);

  ins.sampnum=0;
  *ins.name=0;

  short file;
  char path[_MAX_PATH];

  for (int p=0; p<4; p++)
  {
    strcpy(path,midInstrumentPaths[p]);
    strcat(path,midInstrumentNames[program]);
    file=open(path, O_RDONLY|O_BINARY);
    if (file>=0)
      break;
  }
  if (file<0)
  {
    printf("Error: Patch file %s not found!\n", midInstrumentNames[program]);
    return errFileMiss;
  }

  PATCHHEADER ph;
  read(file, &ph, sizeof(ph));

  if (memcmp(ph.header, "GF1PATCH110", 12))
    return errFormStruc;
  if (ph.instruments<1)
    return errFormStruc;

  INSTRUMENTDATA ih;
  read(file, &ih, sizeof(ih));

  if (ih.layers<1)
    return errFormStruc;
  strcpy(ins.name, ih.instrument_name);
  ins.name[16]=0;
  if (!*ins.name)
  {
    char name[_MAX_FNAME];
    _splitpath(midInstrumentNames[program], 0, 0, name, 0);
    strcpy(ins.name, name);
  }

  LAYERDATA lh;
  read(file, &lh, sizeof(lh));

  ins.samples=new msample[lh.samples];
  smps=new sampleinfo[lh.samples];
  if (!ins.samples||!smps)
    return errFormStruc;
  ins.sampnum=lh.samples;
  memset(smps, 0, lh.samples*sizeof(*smps));

  memset(ins.note, 0xFF, 0x80);

  int i;
  int cursamp=0;
  for (i=0; i<ins.sampnum; i++)
  {
    signed char st=loadsample(file, ins, cursamp, ph.voices, 1, i, sampused, smps[cursamp], samplenum);
    if (st<errOk)
      return st;
    if (st!=1)
      cursamp++;
  }
  ins.sampnum=cursamp;

  unsigned char lowest=0xFF;
  for (i=0; i<0x80; i++)
    if (ins.note[i]!=0xFF)
    {
      lowest=ins.note[i];
      break;
    }

  for (i=0; i<0x80; i++)
    if (ins.note[i]!=0xFF)
      lowest=ins.note[i];
    else
      ins.note[i]=lowest;

  close(file);

  return errOk;
}


int addpatch( minstrument    &ins,
              uint1          program,
              unsigned char  sn,
              unsigned char  sampnum,
              sampleinfo     &sip,
              unsigned short &samplenum)
{
  if (midUseFFF)
    return addpatchFFF(ins, program, sn, sampnum, sip, samplenum);

  msample &s=ins.samples[sn];

  short file;
  char path[_MAX_PATH];

  for (int p=0; p<4; p++)
  {
    strcpy(path,midInstrumentPaths[p]);
    strcat(path,midInstrumentNames[program]);
    file=open(path, O_RDONLY|O_BINARY);
    if (file>=0)
      break;
  }
  if (file<0)
  {
    printf("Error: Patch file %s not found!\n", midInstrumentNames[program]);
    return errFileMiss;
  }

  PATCHHEADER ph;
  read(file, &ph, sizeof(ph));

  if (memcmp(ph.header, "GF1PATCH110", 12))
    return errFormStruc;
  if (ph.instruments<1)
    return errFormStruc;

  INSTRUMENTDATA ih;
  read(file, &ih, sizeof(ih));

  if (ih.layers<1)
  {
    msample &s=ins.samples[sn];

    strcpy(s.name, "no sample");
    s.handle=-1;
    s.sampnum=sampnum;
    s.normnote=getnote(440000);
    sip.length=1;
    sip.loopstart=0;
    sip.loopend=0;
    sip.samprate=44100;
    sip.type=0;
    short q;
    for (q=0; q<6; q++)
    {
      s.volpos[q]=0;
      s.volrte[q]=0;
    }
    s.end=1;
    s.sustain=-1;
    s.vibdep=0;
    s.vibrte=0;
    s.vibswp=0;
    s.tremdep=0;
    s.tremrte=0;
    s.tremswp=0;
    s.sclfac=256;
    s.sclbas=60;

    unsigned char *smpp=new unsigned char[1];
    if (!smpp)
      return errAllocMem;
    *smpp=0;
    sip.ptr=smpp;
    s.handle=samplenum++;
    return 0;
  }

  LAYERDATA lh;
  read(file, &lh, sizeof(lh));

  if (lh.samples!=1)
    return errFormStruc;

  int st=loadsample(file, ins, sn, ph.voices, 0, sampnum, 0, sip, samplenum);
  if (st)
    return st;

  strcpy(s.name, ih.instrument_name);
  s.name[16]=0;
  if (!*s.name)
  {
    char name[_MAX_FNAME];
    _splitpath(midInstrumentNames[program], 0, 0, name, 0);
    strcpy(s.name, name);
  }

  close(file);

  return errOk;
}


char midLoadMidi( midifile &m,
                  binfile &file,
                  unsigned long opt,
                  const char *gpp)
{
  m.free();

  m.opt=opt;

  unsigned char drumch2=(m.opt&MID_DRUMCH16)?15:16;

  unsigned long len;
  while (1)
  {
    //int stat;
    unsigned long type;
    file.read(&type,4);
    if (type==0x46464952)
    {
      file.getul();
      if (file.getul()!=0x44494D52)
        return errFormStruc;
      while (1)
      {
        if (file.getul()==0x61746164)
          break;
        file.seekcur(file.getul());
      }
      file.getul();
      continue;
    }
    if (type==*(unsigned long *)"MThd")
      break;
    file.seekcur(convlong(file.getl()));
  }

  len=convlong(file.getl());
  if (len<6)
    return errFormStruc;

  unsigned short trknum;
  unsigned short mtype;
  mtype=convshort(file.gets());
  trknum=convshort(file.gets());
  m.tempo=convshort(file.gets());
  file.seekcur(len-6);

  if (mtype>=3)
    return errFormSupp;
  if ((mtype==1)&&(trknum>64))
    return errFormSupp;

  m.tracknum=(mtype==1)?trknum:1;

  m.tracks=new miditrack[m.tracknum];
  if (!m.tracks)
    return errAllocMem;

  int i;
  for (i=0; i<m.tracknum; i++)
  {
    m.tracks[i].trk=0;
    m.tracks[i].trkend=0;
  }

  for (i=0; i<trknum; i++)
  {
    while (1)
    {
      unsigned long type;
      if (file.read(&type, 4)!=4)
        return errFormStruc;
      len=convlong(file.getl());
      if (type==*(unsigned long *)"MTrk")
        break;
      file.seekcur(len);
    }

    if (mtype!=2)
    {
      m.tracks[i].trk=new unsigned char [len];
      if (!m.tracks[i].trk)
        return errAllocMem;
      m.tracks[i].trkend=m.tracks[i].trk+len;
      file.read(m.tracks[i].trk, len);
    }
    else
    {
      unsigned long oldlen=m.tracks[0].trkend-m.tracks[0].trk;
      unsigned char *n=(unsigned char*)realloc(m.tracks[0].trk, oldlen+len);
      if (!n)
        return errAllocMem;
      m.tracks[0].trk=n;
      m.tracks[0].trkend=n+oldlen+len;
      file.read(m.tracks[0].trk+oldlen, len);
    }
  }

  unsigned char (*sampused)[16];
  unsigned char instused[0x81];
  unsigned char chaninst[16];
  sampused=new unsigned char [0x81][16];
  if (!sampused)
    return errAllocMem;

  memset(sampused, 0, 0x81*16);
  memset(instused, 0, 0x81);
  memset(chaninst, 0, 16);
  chaninst[9]=0x80;
  if (drumch2<16)
    chaninst[drumch2]=0x80;

  m.ticknum=0;
  for (i=0; i<m.tracknum; i++)
  {
    unsigned char *trkptr=m.tracks[i].trk;
    unsigned char status=0;
    unsigned long trackticks=0;
    while (trkptr<m.tracks[i].trkend)
    {
      trackticks+=readvlnum(trkptr);
      if (*trkptr&0x80)
        status=*trkptr++;
      if ((status==0xFF)||(status==0xF0)||(status==0xF7))
      {
        if (status==0xFF)
          trkptr++;
        unsigned long l=readvlnum(trkptr);
        trkptr+=l;
      }
      else
        switch (status&0xF0)
        {
        case 0x90:
          if (trkptr[1])
          {
            sampused[chaninst[status&0xF]][trkptr[0]>>3]|=1<<(trkptr[0]&7);
            instused[chaninst[status&0xF]]=1;
          }
          trkptr+=2;
          break;
        case 0x80: case 0xA0: case 0xB0: case 0xE0:
          trkptr+=2;
          break;
        case 0xD0:
          trkptr++;
          break;
        case 0xC0:
          if (((status&0xF)!=9)&&((status&0xF)!=drumch2))
          {
            chaninst[status&0xF]=trkptr[0];
            // shit!
            memset(sampused[trkptr[0]], 0xFF, 16);
            instused[trkptr[0]]=1;
          }
          trkptr++;
          break;
        }
    }
    if (m.ticknum<trackticks)
      m.ticknum=trackticks;
  }
  if (!m.ticknum)
    return errFormStruc;

  short j;
  m.instnum=0;
  for (i=0; i<0x81; i++)
    if (instused[i])
      m.instnum++;

  if (!m.instnum)
  {
    instused[0]=1;
    memset(sampused[0], 0xFF, 16);
    m.instnum++;
  }

  sampleinfo **smps=new sampleinfo *[m.instnum];
  m.instruments=new minstrument [m.instnum];
  if (!m.instruments||!smps)
    return errAllocMem;

  for (i=0; i<m.instnum; i++)
  {
    m.instruments[i].sampnum=0;
    m.instruments[i].samples=0;
    smps[i]=0;
  }

  if (!midInit(gpp))
  {
    midClose();
    return errFileMiss;
  }

  m.sampnum=0;
  memset(m.instmap, 0, 0x81);
  short inst=0;
  for (i=0; i<0x80; i++)
    if (instused[i])
    {
      int stat=loadpatch(m.instruments[inst], i, sampused[i], smps[inst], m.sampnum);
      if (stat)
      {
        midClose();
        return stat;
      }
      m.instruments[inst].prognum=i;
      m.instmap[i]=inst;
      inst++;
    }

  if (instused[0x80] && !midUseFFF) // fff-todo, blah...
  {
    short drums=0;
    for (i=0; i<0x80; i++)
      if (sampused[0x80][i>>3]&(1<<(i&7)))
        if (midInstrumentNames[i+0x80][0])
          drums++;
    m.instmap[0x80]=inst;
    minstrument &ins=m.instruments[inst];
    ins.prognum=0x80;
    ins.sampnum=drums;
    smps[inst]=new sampleinfo[drums];
    ins.samples=new msample[drums];
    if (!ins.samples)
    {
      midClose();
      return errAllocMem;
    }
    memset(smps[inst], 0, drums*sizeof(**smps));
    memset(ins.note, 0xFF, 0x80);
    strcpy(ins.name, "drums");
    unsigned char sn=0;
    for (i=0; i<0x80; i++)
      if (sampused[0x80][i>>3]&(1<<(i&7)))
        if (midInstrumentNames[i+0x80][0])
        {
          ins.note[i]=sn;
          int stat=addpatch(ins, i+0x80, sn, i, smps[inst][sn], m.sampnum);
          if (stat)
          {
            midClose();
            return stat;
          }
          sn++;
        }
    inst++;
  }
  delete sampused;

  m.samples=new sampleinfo[m.sampnum];
  memset(m.samples, 0, m.sampnum*sizeof(sampleinfo));

  int samplenum=0;
  for (i=0; i<inst; i++)
  {
    for (j=0; j<m.instruments[i].sampnum; j++)
      m.samples[samplenum++]=smps[i][j];
    delete smps[i];
  }
  delete smps;

  midClose();
  return errOk;
}

void midifile::reset()
{
  tracks=0;
  instruments=0;
  samples=0;
}

void midifile::free()
{
  int i;
  if (tracks)
  {
    for (i=0; i<tracknum; i++)
      if (tracks[i].trk)
        delete tracks[i].trk;
    delete tracks;
  }
  if (instruments)
  {
    for (i=0; i<instnum; i++)
      if (instruments[i].samples)
        delete instruments[i].samples;
    delete instruments;
  }
  if (samples)
  {
    for (i=0; i<sampnum; i++)
      delete samples[i].ptr;
    delete samples;
  }

  reset();
}

void closeFFF()
{
  FFF_ENVP_LIST *el=envp_list;
  while (el)
  {
    FFF_ENVP_LIST t=*el;
    for (int i=0; i<t.chunk->num_envelopes; i++)
    {
      delete[] t.chunk->records[i].attack_points;
      delete[] t.chunk->records[i].release_points;
    }
    delete[] t.chunk->records;
    delete t.chunk;
    delete el;
    el=t.next;
  }

  FFF_PTCH_LIST *pl=ptch_list;
  while (pl)
  {
    FFF_PTCH_LIST t=*pl;
    for (int i=0; i<t.chunk->nlayers; i++)
      delete[] t.chunk->iw_layer[i].waves;

    delete[] t.chunk->iw_layer;
    delete t.chunk;

    delete pl;
    pl=t.next;
  }

  FFF_DATA_LIST *dl=data_list;
  while (dl)
  {
    FFF_DATA_LIST t=*dl;
    delete t.chunk;
    delete dl;
    dl=t.next;
  }
}

static inline int ulaw2linear(unsigned char u_val)
{
  return ulaw_exp_table[u_val];
}

int addpatchFFF( minstrument    &ins,
                 uint1          program,
                 unsigned char  sn,
                 unsigned char  sampnum,
                 sampleinfo     &sip,
                 unsigned short &samplenum)
{
  return 0;
}

/*
 * instrument name table for .FFF files (melodic bank, after GM standard)
 */

static char *gmins_melo[]={
  "Acoustic Piano 1", "Acoustic Piano 2", "Acoustic Piano 3",
  "Honky-tonk", "E-Piano 1", "E-Piano 2", "Harpsichord",
  "Clavinet", "Celesta", "Glockenspiel", "Music Box",
  "Vibraphone", "Marimbaphone", "Xylophone", "Tubular-bell",
  "Santur", "Organ 1", "Organ 2", "Organ 3", "Church Organ",
  "Reed Organ", "Accordion", "Harmonica", "Bandoneon",
  "Nylon-str. Guit.", "Steel-str. Guit.", "Jazz Guitar",
  "Clean Guitar", "Muted Guitar", "Overdrive Guitar",
  "Distortion Guit.", "Guitar Harmonics", "Acoustic Bass",
  "Fingered Bass", "Picked Bass", "Fretless Bass",
  "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2",
  "Violin", "Viola", "Cello", "Contrabass", "Tremolo String",
  "Pizzicato String", "Harp", "Timpani", "Strings",
  "Slow Strings", "Synth Strings 1", "Synth Strings 2", "Choir",
  "Voice Oohs", "SynVox", "Orchestra Hit", "Trumpet",
  "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass 1",
  "Synth Brass 1", "Synth Brass 2", "Soprano Saxophone",
  "Alto Saxophone", "Tenor Saxophone", "Baritone Saxophone",
  "Oboe", "English Horn", "Bassoon", "Clarinet", "Piccolo",
  "Flute", "Recorder", "Pan Flute", "Bottle Blow",
  "Shakuhachi", "Whistle", "Ocarina", "Square Wave", "Saw wave",
  "Synth Calliope", "Chiffer Lead", "Charang", "Solo Vox",
  "5th Saw Wave", "Bass & Lead", "Fantasia", "Warm Pad",
  "Polysynth", "Space Voice", "Bowed Glass", "Metal Pad",
  "Halo Pad", "Sweeo Pad", "Ice Rain", "Soundtrack", "Crystal",
  "Atmosphere", "Brightness", "Goblin", "Echo Drops",
  "Star Theme", "Sitar", "Banjo", "Shamisen", "Koto", "Kalima",
  "Bag Pipe", "Fiddle", "Shannai", "Tinkle Bell", "Agogo",
  "Steel Drums", "Woodblock", "Taiko", "Melodic Tom 1",
  "Synth Drum", "Reverse Cymbal", "Guitar FretNoise",
  "Breath Noise", "Seashore", "Bird", "Telephone 1",
  "Helicopter", "Applause", "Gun Shot"
};

extern char plNoteStr[132][4];

int loadpatchFFF(minstrument    &ins,
                 uint1          program,
                 unsigned char  *sampused,
                 sampleinfo     *&smps,
                 unsigned short &samplenum)
{
  FFF_PTCH_LIST *pl=ptch_list;
  while (pl)
  {
    if (pl->chunk->program==program) break;
    pl=pl->next;
  }
  if (!pl)
  {
    printf("[FFF]: program %d not found!\n", program);
    return errGen;
  }

  FFF_PTCH_CHUNK &patch=*pl->chunk;
  FFF_LAYR_CHUNK &layer=*patch.iw_layer;       // todo: more layers?!? don't know how.. (fd)
  FFF_ENVP_CHUNK *envelope[2]={(FFF_ENVP_CHUNK*)layer.penv.ptr, (FFF_ENVP_CHUNK*)layer.venv.ptr};

//sprintf(ins.name, ".-.-.-", program);

  if (program<128)
    strcpy(ins.name, gmins_melo[program]);
  else
    sprintf(ins.name, "#%d", program);

  ins.prognum=program;
  ins.sampnum=layer.nwaves;
  ins.samples=new msample[layer.nwaves];

  smps=new sampleinfo[layer.nwaves];
  memset(smps, 0, layer.nwaves*sizeof(*smps));

  printf("loading program %d\n", program);

  for(int i=0; i<2; i++)
  {
    if(!(i?layer.venv.ptr:layer.penv.ptr)) continue;
    printf("%s envelope(s):\n", i?"volume":"pitch");
    printf("   retrigger: %d\n", envelope[i]->flags);
    printf("   mode     : %d\n", envelope[i]->mode);
    printf("   indtype  : %d\n", envelope[i]->indtype);
    for(int e=0; e<envelope[i]->num_envelopes; e++)
    {
      FFF_ENVELOPE_RECORD &rec=envelope[i]->records[e];
      printf("  env #%d: (hirange: %d)\n", e, rec.hirange);
      printf("   sustain_offset:        %d\n", rec.sustain_offset);
      printf("   sustain_rate  :        %d\n", rec.sustain_rate);
      printf("   release_rate  :        %d\n", rec.release_rate);
      printf("   attack_envelope: \n    next:   ");
      for(int n=0; n<rec.nattack; n++)
        printf("%04d ", rec.attack_points[n].next);
      printf("\n    r/t :   ");
      for(int n=0; n<rec.nattack; n++)
        printf("%04d ", rec.attack_points[n].rate);
      printf("\n   release_envelope:\n    next:   ");
      for(int n=0; n<rec.nrelease; n++)
        printf("%04d ", rec.release_points[n].next);
      printf("\n    r/t :   ");
      for(int n=0; n<rec.nrelease; n++)
        printf("%04d ", rec.release_points[n].rate);
      printf("\n");
    }
  }

  for (int smp=0; smp<layer.nwaves; smp++)
  {
    FFF_WAVE_CHUNK &wave=layer.waves[smp];
    msample &s=ins.samples[smp];
    s.handle=samplenum++;
    s.sampnum=smp;

    s.normnote=layer.freq_center<<8;

    int freq=(44100.0*1024.0/(float)wave.sample_ratio*(float)getfreq(s.normnote)/(float)1000);

    // ratio = 44100 * (root_freq * 1000) * 1024 / sample_rate / 1000
    // is this really right? (fd)  don't know, but seems to work (but some samples sound mistuned, hmmm...) (ryg)

    for (int n=wave.low_note; n<wave.high_note; n++)
      ins.note[n]=smp;

    for (int q=0; q<6; q++)
    {
      s.volpos[q]=0;
      s.volrte[q]=0;
    }

    s.volpos[0]=62976;
    s.volrte[0]=250<<8;
    s.volpos[1]=62976;
    s.volrte[1]=0;
    s.end=2;
    s.sustain=1;
    s.sclfac=256;       // ?!??
    s.sclbas=60;

                                        // todo: double check them again.
                                        // did it (and corrected 'em) (ryg)
/*  s.tremswp=(layer.tremolo.sweep-10)*2*64/45;
    s.tremrte=((float)layer.tremolo.freq*7.0/3.0+15.0)*65536.0/(300.0*64.0);
    s.tremdep=layer.tremolo.depth*19*4*256/255/2;

    s.vibswp=(layer.vibrato.sweep-10)*2*64/45;
    s.vibrte=((float)layer.vibrato.freq*7.0/3.0+15.0)*65536.0/(300.0*64.0);
    s.vibdep=layer.vibrato.depth*19*4*256/255/2;

    ryg 990125: seems to be incorrect (vibratos glide over the whole fft
                analyser :)  - trying to fix this by combining beisert's
                formulas for PAT with some from GIPC ("a glorious hack")
*/

    s.tremswp=(layer.tremolo.sweep-10)*2*64/45;
    s.tremrte=(((float)layer.tremolo.freq*7.0/3.0)+15.0)*65536.0/(300.0*64);
    s.tremdep=(layer.tremolo.depth*51*256)/(255*7);

    s.vibswp=(layer.vibrato.sweep-10)*2*64/45;
    s.vibrte=(((float)layer.vibrato.freq*7.0/3.0)+15.0)*65536.0/(300.0*64);
    s.vibdep=(layer.vibrato.depth*12*256)/(255*7);

    printf("   -> %d %d %d, %d %d %d\n", s.tremswp, s.tremrte, s.tremdep,
                                         s.vibswp, s.vibrte, s.vibdep);

    smps[smp].type =(wave.format&1)?0:mcpSamp16Bit;
    smps[smp].type|=(wave.format&2)?0:mcpSampUnsigned;
    smps[smp].type|=(wave.format&8)?mcpSampLoop:0;
    smps[smp].type|=(wave.format&16)?mcpSampBiDi:0;
//    smps[smp].type|=(wave.format&32)?mcpSampULaw:0;
    smps[smp].type|=(wave.format&32)?mcpSamp16Bit:0;

    smps[smp].ptr=new unsigned char[wave.size*((wave.format&1)?1:2)*((wave.format&32)?2:1)];
    smps[smp].length=wave.size;
    smps[smp].samprate=freq;
    smps[smp].loopstart=wave.loopstart>>4;
    smps[smp].loopend=wave.loopend>>4;

    smps[smp].sloopstart=0;
    smps[smp].sloopend=0;

    char *nf=plNoteStr[wave.low_note], *nt=plNoteStr[wave.high_note];
    sprintf(s.name, "%c%c%c to %c%c%c", nf[0], nf[1], nf[2],
                                        nt[0], nt[1], nt[2]);

    sbinfile dat;
    char filename[_MAX_PATH];
    strcpy(filename, cfGetProfileString("midi", "dir", "./"));
    strcat(filename, ((FFF_DATA_CHUNK*)wave.data_id.ptr)->filename);

    if (dat.open(filename, sbinfile::openro))
    {
      printf("[FFF]: %s not found.\n", filename);
      free (smps[smp].ptr);
      return errGen;
    }

    dat.seek(wave.start);

    if(wave.format&32) // ulaw
    {
      signed char *temp=new signed char[wave.size];
      short *d=(short*)smps[smp].ptr;
      dat.eread(temp, wave.size);
      for (int i=0; i<wave.size; i++)
        ((short*)d)[i]=ulaw2linear(temp[i]);
      delete[] temp;
    } else
      dat.eread(smps[smp].ptr, wave.size*((wave.format&1)?1:2));

    dat.close();
  }

  return errOk;
}
