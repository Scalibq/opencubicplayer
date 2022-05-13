#ifndef __ARCACE2_HPP
#define __ARCACE2_HPP
#include "binfile.h"

int read_adds_blk(char* buffer, int len);

#define size_rdb  8192
#define size_wrb  8192
#define size_buf  8192

#define maxdic      22
#define maxwd_mn    11
#define maxwd_lg    11
#define maxwd_svwd   7
#define maxlength  259
#define maxdis2    255
#define maxdis3   8191
#define maxcode   (255+4+maxdic)
#define svwd_cnt    15
#define max_cd_mn (256+4+(maxdic+1)-1)
#define max_cd_lg (256-1)


extern unsigned long *buf_rd ;
extern char  *buf    ;
extern char  *buf_wr ;
extern unsigned char *readbuf;

extern
short rpos          ,
      dcpr_do       ,
      dcpr_do_max   ,
      blocksize     ,
      dcpr_dic      ,
      dcpr_oldnum   ,
      bits_rd       ,
      dcpr_frst_file;
extern
unsigned short dcpr_code_mn[1 << maxwd_mn],
       dcpr_code_lg[1 << maxwd_lg];
extern
unsigned char dcpr_wd_mn[maxcode + 2],
      dcpr_wd_lg[maxcode + 2];
extern
unsigned char wd_svwd[svwd_cnt];
extern
unsigned long dcpr_dpos      ,
      cpr_dpos2      ,
      dcpr_dicsiz    ,
      dcpr_dican     ,
      dcpr_size      ,
      code_rd        ;
extern
unsigned long dcpr_olddist[4];
extern
char *dcpr_text      ;


extern unsigned short sort_org[];
extern unsigned char sort_freq[];

struct aceheaderstruct
{
 short crc;
 short size;
 char type;
 short flags;
};

struct acexheaderstruct
{
 short crc;
 short size;
 char type;
 short flags;
 long addsize;
};

struct acefileheaderstruct
{
 short crc;
 short size;
 char type;            // 1
 short flags;          // &0x8000
 long packsize;
 long unpsize;
 long ftime;
 long attributes;
 long filecrc;
 struct
 {
  unsigned char type;
  unsigned char qual;
  unsigned short parm;
 } tech;
 short lame;
 short filenamesize;
 char filename[];
};

#define size_headrdb (sizeof(acefileheaderstruct)-20) // (some bytes less esp. for Amiga)

extern aceheaderstruct *aheader;
extern acexheaderstruct *axheader;
extern acefileheaderstruct *afheader;

//archive-header-flags
#define ACE_LIM256   1024
#define ACE_MULT_VOL 2048
#define ACE_AV       4096
#define ACE_RECOV    8192
#define ACE_LOCK    16384
#define ACE_SOLID   32768

//file-header-flags
#define ACE_ADDSIZE  1
#define ACE_PASSW    16384
#define ACE_SP_BEF   4096
#define ACE_SP_AFTER 8192
#define ACE_COMM     2

//block types
#define MAIN_BLK 0
#define FILE_BLK 1
#define REC_BLK  2

//known compression types
#define TYPE_STORE 0
#define TYPE_LZ1   1

#define ERR_MEM      1
#define ERR_FILES    2
#define ERR_FOUND    3
#define ERR_FULL     4
#define ERR_OPEN     5
#define ERR_READ     6
#define ERR_WRITE    7
#define ERR_CLINE    8
#define ERR_CRC      9
#define ERR_OTHER   10
#define ERR_USER   255

extern binfile *garchive;
extern int solid;

#endif
