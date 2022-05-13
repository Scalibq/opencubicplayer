struct layer
{
    int version;
    int lay;
    int error_protection;
    int bitrate_index;
    int sampling_frequency;
    int padding;
    int extension;
    int mode;
    int mode_ext;
    int copyright;
    int original;
    int emphasis;
};

struct frame_params
{
    layer       header;
    int         actual_mode;
    int         stereo;
    int         jsbound;
    int         sblimit;
    int         model;
    int frmlen;
};

void put1bit(int);
void putbits(unsigned int, int);
void update_CRC(unsigned int data, unsigned int length, unsigned int *crc);
void filter_subband(short *buffer,float *s,int k);
void psycho_anal(short*, int, int, double*);
void initpsychoanal(int sfreq_idx);
