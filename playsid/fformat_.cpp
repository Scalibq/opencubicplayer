// OpenCP Module Player
// copyright (c) '94-'98 Niklas Beisert <nbeisert@physik.tu-muenchen.de>
//
// SIDPlay File Format auxiliary routines
//
// revision history: (please note changes here)
//  -kb980717  Tammo Hinrichs <opencp@gmx.net>
//    -first release

//
// $Date: 1998/12/20 12:53:52 $
//

#include "fformat.h"
#include "myendian.h"

// Case-insensitive comparison of two strings up to ``sizeof(s2)'' characters.
int myStrNcaseCmp( char* s1, const char* s2 )
{
  char tmp = *(s1 +strlen(s2));
  *(s1 +strlen(s2)) = 0;
  int ret = stricmp( s1, s2 );
  *(s1 +strlen(s2)) = tmp;
  return ret;
}

// Case-insensitive comparison of two strings.
int myStrCaseCmp( char* s1, char* s2 )
{
	return stricmp(s1,s2);
}

// Own version of strdup, which uses new instead of malloc.
char* myStrDup(const char *source)
{
	char *dest;
	if ( (dest = new char[strlen(source)+1]) != 0)
	{
		strcpy(dest,source);
	}
	return dest;
}

// Return pointer to file name position in complete path.
char* fileNameWithoutPath(char* s)
{
	int last_slash_pos = -1;
	for ( unsigned pos = 0; pos < strlen(s); pos++ )
	{
#if defined(__MSDOS__) || defined(__WIN32__) || defined(_Windows)
		if ( s[pos] == '\\' )
#elif defined(__POWERPC__)
		if ( s[pos] == ':' )
#elif defined(__amigaos__)
		if ( s[pos] == ':' || s[pos] == '/' )
#else
		if ( s[pos] == '/' )
#endif
		    last_slash_pos = pos;
	}
	return( &s[last_slash_pos +1] );
}

// Return pointer to file name extension in file name.
char* fileExtOfFilename(char* s)
{
	int last_dot_pos = strlen(s)-1;  // assume no dot
	for ( unsigned pos = 0; pos < strlen(s); pos++ )
	{
		if ( s[pos] == '.' )
			last_dot_pos = pos;
	}
	return( &s[last_dot_pos] );
}

// Parse input string stream. Read and convert a hexa-decimal number up
// to a ``,'' or ``:'' or ``\0'' or end of stream.
udword readHex( istrstream& hexin )
{
	udword hexLong = 0;
	char c;
	do
	{
		hexin >> c;
		if ( !hexin )
			break;
		if (( c != ',') && ( c != ':' ) && ( c != 0 ))
		{
			// machine independed to_upper
			c &= 0xdf;
			( c < 0x3a ) ? ( c &= 0x0f ) : ( c -= ( 0x41 - 0x0a ));
			hexLong <<= 4;
			hexLong |= (udword)c;
		}
		else
		{
			if ( c == 0 )
				hexin.putback(c);
			break;
		}
	}  while ( hexin );
	return hexLong;
}

// Parse input string stream. Read and convert a decimal number up
// to a ``,'' or ``:'' or ``\0'' or end of stream.
udword readDec( istrstream& decin )
{
	udword hexLong = 0;
	char c;
	do
	{
		decin >> c;
		if ( !decin )
			break;
		if (( c != ',') && ( c != ':' ) && ( c != 0 ))
		{
			c &= 0x0f;
			hexLong *= 10;
			hexLong += (udword)c;
		}
		else
		{
			if ( c == 0 )
				decin.putback(c);
			break;
		}
	}  while ( decin );
	return hexLong;
}

// Search terminated string for next newline sequence.
// Skip it and return pointer to start of next line.
char* returnNextLine( char* s )
{
	// Unix: LF = 0x0A
	// Windows, DOS: CR,LF = 0x0D,0x0A
	// Mac: CR = 0x0D
	char c;
	while ((c = *s) != 0)
	{
		s++;                            // skip read character
		if (c == 0x0A)
		{
			break;                      // LF found
		}
		else if (c == 0x0D)
		{
			if (*s == 0x0A)
			{
				s++;                    // CR,LF found, skip LF
			}
			break;                      // CR or CR,LF found
		}
	}
	if (*s == 0)                        // end of string ?
	{
		return 0;                       // no next line available
	}
	return s;                           // next line available
}

// Skip any characters in an input string stream up to '='.
void skipToEqu( istrstream& parseStream )
{
	char c;
	do
	{
		parseStream >> c;
	}
	while ( c != '=' );
}

void copyStringValueToEOL( char* pSourceStr, char* pDestStr, int DestMaxLen )
{
	// Start at first character behind '='.
	while ( *pSourceStr != '=' )
	{
		pSourceStr++;
	}
	pSourceStr++;  // Skip '='.
	while (( DestMaxLen > 0 ) && ( *pSourceStr != 0 )
		   && ( *pSourceStr != '\n' ) && ( *pSourceStr != '\r' ))
	{
		*pDestStr++ = *pSourceStr++;
		DestMaxLen--;
	}
	*pDestStr++ = 0;
}