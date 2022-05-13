class binfile;

struct ampegencoderparams
{
  int lay;
  int mode;
  int sampfreq;
  int bitrate;
  int model;
  int crc;
  int extension;
  int copyright;
  int original;
  int emphasis;
};

int initencoder(binfile &out, const ampegencoderparams &par);
int encodeframe(void *buf, int len);
void doneencoder();
