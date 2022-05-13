#ifndef __CDAUDIO_H
#define __CDAUDIO_H

unsigned char cdInit();
void cdClose();
unsigned char cdIsCDDrive(unsigned short drive);
unsigned short cdGetTracks(unsigned short drive, unsigned long *starts, unsigned char &first, unsigned short maxtracks);
void cdLockTray(unsigned short drive, unsigned char lock);
unsigned long cdGetHeadLocation(unsigned short drive, unsigned short &stat);
void cdPlay(unsigned short drive, unsigned long start, unsigned long len);
void cdStop(unsigned short drive);
void cdRestart(unsigned short drive);
void cdSetVolumes(unsigned short drive, unsigned char left, unsigned char right);
void cdGetVolumes(unsigned short drive, unsigned char &left, unsigned char &right);


#endif
