// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// UNACE decompression routines
//
// revision history: (please note changes here)
//  -kb980717   whoever <i@suck.com>
//    -first release

// a very, very special version of uac_dcpr for arcace
// messed up very much :)

// WARNING: THIS SOURCE IS _NOT_ COVERED UNDER GNU GPL!
// - You may use this code, modified or unmodified, in any freeware
//   production, that is, you may NOT charge any money for it.
//   (should be enough, get the original license somewhere ;)

#include <stdlib.h>
#include <string.h>
#include "arcace.h"

extern void memset16(void volatile *, short, long);
#pragma aux memset16=\
"rep stosw" parm [edi] [ax] [ecx];


int  calc_dectabs(void);

//------------------------------ QUICKSORT ---------------------------------//
#define xchg_def(v1,v2) {int dummy;\
                         dummy=v1; \
                         v1=v2;    \
                         v2=dummy;}

void sortrange(int left, int right)
{
   int  zl = left,
        zr = right,
        hyphen;

   hyphen = sort_freq[right];

   //divides by hyphen the given range into 2 parts
   do
   {
      while (sort_freq[zl] > hyphen)
         zl++;
      while (sort_freq[zr] < hyphen)
         zr--;
      //found a too small (left side) and
      //a too big (right side) element-->exchange them
      if (zl <= zr)
      {
         xchg_def(sort_freq[zl], sort_freq[zr]);
         xchg_def(sort_org[zl], sort_org[zr]);
         zl++;
         zr--;
      }
   }
   while (zl < zr);

   //sort partial ranges - when very small, sort directly
   if (left < zr)
      if (left < zr - 1)
         sortrange(left, zr);
      else if (sort_freq[left] < sort_freq[zr])
      {
         xchg_def(sort_freq[left], sort_freq[zr]);
         xchg_def(sort_org[left], sort_org[zr]);
      }

   if (right > zl)
      if (zl < right - 1)
         sortrange(zl, right);
      else if (sort_freq[zl] < sort_freq[right])
      {
         xchg_def(sort_freq[zl], sort_freq[right]);
         xchg_def(sort_org[zl], sort_org[right]);
      }
}

void quicksort(int n)
{
   int  i;

   for (i = n + 1; i--;)
      sort_org[i] = i;
   sortrange(0, n);
}

//------------------------------ read bits ---------------------------------//
void readdat(void)
{
 int i;

 i = (size_rdb - 2) << 2;
 rpos -= size_rdb - 2;
 buf_rd[0] = buf_rd[size_rdb - 2];
 buf_rd[1] = buf_rd[size_rdb - 1];
 read_adds_blk((char *) & buf_rd[2], i);
}

#define addbits(bits)                                   \
{                                                       \
  rpos+=(bits_rd+=bits)>>5;                             \
  bits_rd&=31;                                          \
  if (rpos==(size_rdb-2)) readdat();                    \
  code_rd=(buf_rd[rpos] << bits_rd)                     \
    +((buf_rd[rpos+1] >> (32-bits_rd))&(!bits_rd-1));   \
}

// no comment-decompression

//------------------------- LZW DECOMPRESSION ------------------------------//
void wrchar(char ch)
{
   dcpr_do++;

   dcpr_text[dcpr_dpos] = ch;
   dcpr_dpos++;
   dcpr_dpos &= dcpr_dican;
}

void copystr(long d, int l)
{
   int  mpos;

   dcpr_do += l;

   mpos = dcpr_dpos - d;
   mpos &= dcpr_dican;

   if ((mpos >= dcpr_dicsiz - maxlength) || (dcpr_dpos >= dcpr_dicsiz - maxlength))
   {
      while (l--)
      {
         dcpr_text[dcpr_dpos] = dcpr_text[mpos];
         dcpr_dpos++;
         dcpr_dpos &= dcpr_dican;
         mpos++;
         mpos &= dcpr_dican;
      }
   }
   else
   {
      while (l--)
         dcpr_text[dcpr_dpos++] = dcpr_text[mpos++];
      dcpr_dpos &= dcpr_dican;
   }
}

void decompress(void)
{
   int  c,
        lg,
        i,
        k;
   unsigned long dist;

   while (dcpr_do < dcpr_do_max)
   {
      if (!blocksize)
         if (!calc_dectabs())
            return;

      addbits(dcpr_wd_mn[(c = dcpr_code_mn[code_rd >> (32 - maxwd_mn)])]);
      blocksize--;
      if (c > 255)
      {
         if (c > 259)
         {
            if ((c -= 260) > 1)
            {
               dist = (code_rd >> (33 - c)) + (1L << (c - 1));
               addbits(c - 1);
            }
            else
               dist = c;
            dcpr_olddist[(dcpr_oldnum = (dcpr_oldnum + 1) & 3)] = dist;
            i = 2;
            if (dist > maxdis2)
            {
               i++;
               if (dist > maxdis3)
                  i++;
            }
         }
         else
         {
            dist = dcpr_olddist[(dcpr_oldnum - (c &= 255)) & 3];
            for (k = c + 1; k--;)
               dcpr_olddist[(dcpr_oldnum - k) & 3] = dcpr_olddist[(dcpr_oldnum - k + 1) & 3];
            dcpr_olddist[dcpr_oldnum] = dist;
            i = 2;
            if (c > 1)
               i++;
         }
         addbits(dcpr_wd_lg[(lg = dcpr_code_lg[code_rd >> (32 - maxwd_lg)])]);
         dist++;
         lg += i;
         copystr(dist, lg);
      }
      else
         wrchar(c);
   }
}

//-------------------------- HUFFMAN ROUTINES ------------------------------//
int  makecode(int maxwd, int size1_t, unsigned char * wd, unsigned short * code)
{
   int maxc,
        size2_t,
        l,
        c,
        i,
        max_make_code;

   memcpy(sort_freq, wd, (size1_t + 1) * sizeof(char));
   if (size1_t)
      quicksort(size1_t);
   else
      sort_org[0] = 0;
   sort_freq[size1_t + 1] = size2_t = c = 0;
   while (sort_freq[size2_t])
      size2_t++;
   if (size2_t < 2)
   {
      i = sort_org[0];
      wd[i] = 1;
      size2_t += (size2_t == 0);
   }
   size2_t--;

   max_make_code = 1 << maxwd;
   for (i = size2_t + 1; i-- && c < max_make_code;)
   {
      maxc = 1 << (maxwd - sort_freq[i]);
      l = sort_org[i];
      if (c + maxc > max_make_code)
      {
         dcpr_do = dcpr_do_max;
         return (0);
      }
      memset16(&code[c], l, maxc);
      c += maxc;
   }
   return (1);
}

int  read_wd(int maxwd, unsigned short * code, unsigned char * wd, int max_el)
{
   int c,
        i,
        j,
        num_el,
        l,
        uplim,
        lolim;

   memset(wd, 0, max_el * sizeof(char));
   memset(code, 0, (1 << maxwd) * sizeof(short));

   num_el = code_rd >> (32 - 9);
   addbits(9);
   if (num_el > max_el)
      num_el = max_el;

   lolim = code_rd >> (32 - 4);
   addbits(4);
   uplim = code_rd >> (32 - 4);
   addbits(4);

   for (i = -1; ++i <= uplim;)
   {
      wd_svwd[i] = code_rd >> (32 - 3);
      addbits(3);
   }
   if (!makecode(maxwd_svwd, uplim, wd_svwd, code))
      return (0);
   j = 0;
   while (j <= num_el)
   {
      c = code[code_rd >> (32 - maxwd_svwd)];
      addbits(wd_svwd[c]);
      if (c < uplim)
         wd[j++] = c;
      else
      {
         l = (code_rd >> 28) + 4;
         addbits(4);
         while (l-- && j <= num_el)
            wd[j++] = 0;
      }
   }
   if (uplim)
      for (i = 0; ++i <= num_el;)
         wd[i] = (wd[i] + wd[i - 1]) % uplim;
   for (i = -1; ++i <= num_el;)
      if (wd[i])
         wd[i] += lolim;

   return (makecode(maxwd, num_el, wd, code));

}

int  calc_dectabs(void)
{
 if(!read_wd(maxwd_mn, dcpr_code_mn, dcpr_wd_mn, max_cd_mn)
      || !read_wd(maxwd_lg, dcpr_code_lg, dcpr_wd_lg, max_cd_lg))
      return (0);

 blocksize = code_rd >> (32 - 15);
 addbits(15);

 return (1);
}

//---------------------------- BLOCK ROUTINES ------------------------------//
int decompress_blk(char * buf, int len)
{
 long old_pos=dcpr_dpos;
 int i;

 dcpr_do=0;
 if((dcpr_do_max=len-maxlength)>dcpr_size) dcpr_do_max=dcpr_size;
 if((long)dcpr_size>0&&dcpr_do_max)
 {
  decompress();
  if(dcpr_do<=len)
  {
   if(old_pos+dcpr_do>dcpr_dicsiz)
   {
    i=dcpr_dicsiz-old_pos;
    memcpy(buf, &dcpr_text[old_pos], i);
    memcpy(&buf[i], dcpr_text, dcpr_do-i);
   } else
    memcpy(buf, &dcpr_text[old_pos], dcpr_do);
  }
 }
 dcpr_size-=dcpr_do;
 return (dcpr_do);
}

int unstore(char * buf, int len)
{
 int rd=0, i, pos=0;

 while((i=read_adds_blk((char *)buf_rd, (int)((i=((len>dcpr_size)?dcpr_size:len))>size_rdb?size_rdb:i)))!=0)
 {
  rd+=i;
  len-=i;
  dcpr_size-=i;
  memcpy(&buf[pos], buf_rd, i);
  pos+=i;
 }
 for(i=0; i<rd; i++)
 {
  dcpr_text[dcpr_dpos]=buf[i];
  dcpr_dpos++;
  dcpr_dpos&=dcpr_dican;
 }
 return (int)rd;
}

int  dcpr_adds_blk(char * buf, unsigned int len)
{
 int r;

 switch(afheader->tech.type)
 {
  case TYPE_STORE:
   r=unstore(buf, len);
   break;
  case TYPE_LZ1:
   r=decompress_blk(buf, len);
   break;
  default:
//   f_err=ERR_OTHER;             // illegal compression method
   r=0;
 }
// rd_crc=getcrc(rd_crc, buf, r);
 return r;
}


//----------------------------- INIT ROUTINES ------------------------------//
void dcpr_init(void)
{
 dcpr_frst_file=1;
 dcpr_dic=20;
 while((dcpr_text=(char*)malloc(dcpr_dicsiz=(long)1<<dcpr_dic))==NULL) dcpr_dic--;
 dcpr_dican=dcpr_dicsiz-1;
}

void dcpr_init_file(void)
{
 int i;

 if(afheader->flags&ACE_PASSW)
 {
//  f_err=ERR_OTHER;
  return;
 }

// rd_crc=CRC_MASK;
 dcpr_size=afheader->unpsize;
 if(afheader->tech.type==TYPE_LZ1)
 {
  if((afheader->tech.parm&15)+10>dcpr_dic)
  {
//   f_err=ERR_MEM;
   return;
  }
  i=size_rdb*sizeof(long);
  read_adds_blk((char*)buf_rd, i);
  code_rd = buf_rd[0];
  bits_rd = rpos = 0;
  blocksize = 0;
 }
 if(!solid||dcpr_frst_file)          // ??
  dcpr_dpos=0;

 dcpr_oldnum=0;
 memset(dcpr_olddist, 0, sizeof(dcpr_olddist));

 dcpr_frst_file=0;
}