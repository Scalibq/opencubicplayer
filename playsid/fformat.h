//
// $Date: 1998/12/20 12:53:52 $
//

#ifndef __FFORMAT_H
#define __FFORMAT_H


#include <strstream.h>
#include "mytypes.h"


extern int myStrNcaseCmp( char* pDestStr, const char* pSourceStr );
extern int myStrCaseCmp( char* string1, char* string2 );
extern char* myStrDup(const char *source);
extern char* fileNameWithoutPath(char* s);
extern char* fileExtOfFilename(char* s);
extern udword readHex( istrstream& parseStream );
extern udword readDec( istrstream& parseStream );
extern char* returnNextLine( char* pBuffer );
extern void skipToEqu( istrstream& parseStream );
extern void copyStringValueToEOL( char* pSourceStr, char* pDestStr, int destMaxLen );


#endif
