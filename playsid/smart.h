//
// /home/ms/sidplay/include/RCS/smart.h,v
//

#include <iostream.h>
#include <iomanip.h>
#include <ctype.h>
#include "mytypes.h"


template <class T>
class smartPtrBase
{
 public:  // -----------------------------------------------------------------
	
	// --- constructors ---
	
	smartPtrBase(T* pBuffer, udword bufferLen)  
	{
		doFree = false;
		if ( bufferLen >= 1 )
		{
			pBufCurrent = ( pBufBegin = pBuffer );
			pBufEnd = pBufBegin + bufferLen;
			bufLen = bufferLen;
			status = true;
		}
		else
		{
			pBufCurrent = ( pBufBegin = ( pBufEnd = 0 ));
			bufLen = 0;
			status = false;
		}
	}
	
	smartPtrBase(udword bufferLen)  
	{
		doFree = false;
		if ( bufferLen >= 1 )
		{
			pBufCurrent = ( pBufBegin = new T[bufferLen] );
			if ( pBufBegin == 0 )
			{
				status = false;
			}
			doFree = true;
			pBufEnd = pBufBegin + bufferLen;
			bufLen = bufferLen;
			status = true;
		}
		else
		{
			pBufCurrent = ( pBufBegin = ( pBufEnd = 0 ));
			bufLen = 0;
			status = false;
		}
	}
	
	// --- destructor ---
	
	virtual ~smartPtrBase()
	{
		if ( doFree && (pBufBegin != 0) )
		{
			delete[] pBufBegin;
		}
	}
	
	// --- public member functions ---
	
	virtual T* tellBegin()  { return pBufBegin;	}
	virtual udword tellLength()  { return bufLen; }
	virtual udword tellPos()  { return (udword)(pBufCurrent-pBufBegin); }
  
        virtual char reset(T element)
	{
		if ( bufLen >= 1 )
		{
			pBufCurrent = pBufBegin;
			return (status = true);
		}
		else
		{
			return (status = false);
		}
	}

        virtual char good()
	{
		if ( pBufCurrent < pBufEnd )
			return true;
		else
			return false;
	}
	
        virtual char fail()  
	{
		if ( pBufCurrent == pBufEnd )
			return true;
		else
			return false;
	}
	
	virtual void operator ++()
	{
		if ( status && !fail() )
		{
			pBufCurrent++;
		}
		else
		{
			status = false;
		}
	}
	
	virtual void operator ++(int)
	{
		if ( status && !fail() )
		{
			pBufCurrent++;
		}
		else
		{
			status = false;
		}
	}
	
	virtual void operator --()
	{
		if ( status && (pBufCurrent > pBufBegin) )
		{
			pBufCurrent--;
		}
		else
		{
			status = false;
		}
	}
	
	virtual void operator --(int)
	{
		if ( status && (pBufCurrent > pBufBegin) )
		{
			pBufCurrent--;
		}
		else
		{
			status = false;
		}
	}
	
	virtual void operator +=(udword offset)
	{
		if ( status && ((pBufCurrent + offset) < pBufEnd) )
		{
			pBufCurrent += offset;
		}
		else
		{
			status = false;
		}
	}
	
	virtual void operator -=(udword offset)
	{
		if ( status && ((pBufCurrent - offset) >= pBufBegin) )
		{
			pBufCurrent -= offset;
		}
		else
		{
			status = false;
		}
	}
	
	T operator*()
	{
		if ( status && good() )
		{
			return *pBufCurrent;
		}
		else
		{
			status = false;
			return dummy;
		}
	}

	T& operator [](udword index)
	{
		if ( status && ((pBufCurrent + index) < pBufEnd) )
		{
			return pBufCurrent[index];
		}
		else
		{
			status = false;
			return dummy;
		}
	}

#if !defined(__BORLANDC__)
	virtual operator int()  { return status; }
#endif
        virtual operator char()  { return status; }
	
 protected:  // --------------------------------------------------------------
	
	T* pBufBegin;
	T* pBufEnd;
	T* pBufCurrent;
	udword bufLen;
        char status;
        char doFree;
	T dummy;
};


template <class TP>
class smartPtr : public smartPtrBase<TP>
{
 public:  // -----------------------------------------------------------------
	
	// --- constructors ---
	
	smartPtr(TP* pBuffer, udword bufferLen) : smartPtrBase<TP>(pBuffer, bufferLen)
	{
	}
	
	smartPtr(udword bufferLen) : smartPtrBase<TP>(bufferLen)
	{
	}
	
	// --- public member functions ---
	
};

