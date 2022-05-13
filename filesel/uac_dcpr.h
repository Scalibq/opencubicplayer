#ifndef __uac_dcpr_h
#define __uac_dcpr_h

int  calc_dectabs(void);
void dcpr_comm(int comm_size);
int  read_wd(unsigned int maxwd, unsigned short * code, unsigned char * wd, int max_el);
void dcpr_init(void);
int  dcpr_adds_blk(char * buf, unsigned int len);
void dcpr_init_file(void);

#endif /* __uac_dcpr_h */

