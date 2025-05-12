#pragma once
#include "tier1/utlvector.h"

class CSplitString: public CUtlVector<char*, CUtlMemory<char*, int> >
{
public:
	CSplitString();
	CSplitString(const char *pString, const char *pSeparator);
	CSplitString(const char *pString, const char **pSeparators, int nSeparators);
	~CSplitString();

	void Set(const char *pString, const char **pSeparators, int nSeparators);

	//
	// NOTE: If you want to make Construct() public and implement Purge() here, you'll have to free m_szBuffer there
	//
private:
	void Construct(const char *pString, const char **pSeparators, int nSeparators);
	void PurgeAndDeleteElements();
private:
	char *m_szBuffer; // a copy of original string, with '\0' instead of separators
};
