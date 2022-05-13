// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// Export _dllinfo for FSTYPES.DLL
//
// revision history: (please note changes here)
//  -kb980717   Tammo Hinrichs <opencp@gmx.net>
//    -first release

extern "C" {
  char *dllinfo = "readinfos _ampegpReadInfoReg _itpReadInfoReg _gmdReadInfoReg _xmpReadInfoReg _gmiReadInfoReg _wavReadInfoReg _sidReadInfoReg _wmapReadInfoReg";
}
