
#include "../../hdr/portab.h"
#include "globals.h"
#include "proto.h"

extern UWORD _psp;

static psp FAR *lpPsp;
static BYTE FAR *lpEnviron;
static mcb FAR *lpArena;

BOOL EnvClearVar(BYTE *pszName);
COUNT EnvSizeUp(VOID);
VOID EnvCopy(BYTE FAR *lpszDest, BYTE FAR *lpszSrc);

void main(int argc, char *argv)
{
	UWORD uEnvSize;

	lpPsp = (psp FAR *)MK_FP(_psp, 0);
	lpEnviron = MK_FP(lpPsp -> ps_environ, 0);
	lpArena = MK_FP(lpPsp -> ps_environ - 1, 0);
	uEnvSize = lpArena -> m_size;

	printf("Starting size = %d\n", EnvSizeUp());
	EnvClearVar("PATH");
	printf("Ending size = %d\n", EnvSizeUp());
	EnvAlloc(512);
	printf("After EnvAlloc(), size = %d\n", EnvSizeUp());
	printf("COMSPEC = %s\n", EnvLookup("COMSPEC"));

	printf("EnvDump:\n");
	EnvDump();

	EnvSetVar("PATH", "c:\\;c:\\dos");
	printf("\nNew EnvDump:\n");
	EnvDump();

	EnvSetVar("PROMPT", "$n$g");
	printf("\nFinal EnvDump:\n");
	EnvDump();
}


BYTE *EnvLookup(pszName)
BYTE *pszName;
{
	BYTE FAR *lpszString;
	static BYTE szNameBuffer[MAX_CMDLINE];
	BYTE *pszLclString;
	COUNT nNameLen;

	strcpy(szNameBuffer, pszName);
	nNameLen = strlen(pszName);
	for(pszLclString = szNameBuffer; *pszLclString != '\0'; pszLclString++)
		*pszLclString = toupper(*pszLclString);

	lpszString = lpEnviron;
	while('\0' != *lpszString)
	{
		if(0 == fstrncmp(lpszString, (BYTE FAR *)szNameBuffer, nNameLen))
		{
			/* Match, skip the variable name */
			while(*lpszString && '=' != *lpszString)
				++lpszString;
			++lpszString;
			/* Now copy var into static buffer */
			for(pszLclString = szNameBuffer; *lpszString; )
			{
				*pszLclString++ = *lpszString++;
			}
			*pszLclString = '\0';
			return szNameBuffer;
		}
		else
		{
			while(*lpszString)
				*lpszString++;
			++lpszString;
		}
	}
	return (BYTE *)0;
}


BOOL EnvClearVar(BYTE *pszName)
{
	COUNT nNameLen;
	UWORD uNewEnv;
	BYTE FAR *lpszSrc, FAR *lpszDest;
	BOOL bError;
	BYTE szNameBuffer[MAX_CMDLINE], *pszLclString;

	strcpy(szNameBuffer, pszName);
	for(pszLclString = szNameBuffer; *pszLclString != '\0'; pszLclString++)
		*pszLclString = toupper(*pszLclString);
	nNameLen = strlen(pszName);
	lpszSrc = lpEnviron;
	uNewEnv = DosAllocMem(lpArena -> m_size, (BOOL FAR *)&bError);
	if(bError)
		return FALSE;
	lpszDest = MK_FP(uNewEnv, 0);

	/* Copy the old environment into the new and skip the one we	*/
	/* want to delete.						*/
	while('\0' != *lpszSrc)
	{
		if(0 == fstrncmp(lpszSrc, (BYTE FAR *)szNameBuffer, nNameLen))
		{
			/* Match, skip the source */
			while(*lpszSrc)
				++lpszSrc;
			++lpszSrc;
		}
		else
		{
			while(*lpszSrc)
				*lpszDest++ = *lpszSrc++;
			++lpszSrc;
			*lpszDest++ = '\0';
		}
	}
	/* Copy the terminating entry					*/
	*lpszDest++ = *lpszSrc++;


	/* Copy the invocation part					*/
	*((UWORD FAR *)lpszDest)++ = *((UWORD FAR *)lpszSrc)++;
	while(*lpszSrc)
		*lpszDest++ = *lpszSrc++;

	/* and finally null terminate.					*/
	*lpszDest++ = '\0';

	EnvCopy(lpEnviron, MK_FP(uNewEnv, 0));
	DosFreeMem(uNewEnv, (BOOL FAR *)&bError);
	return !bError;
}


VOID EnvCopy(BYTE FAR *lpszDest, BYTE FAR *lpszSrc)
{
	UWORD uNewEnv;
	BOOL bError;

	while('\0' != *lpszSrc)
	{
		while(*lpszSrc)
			*lpszDest++ = *lpszSrc++;
		++lpszSrc;
		*lpszDest++ = '\0';
	}
	/* Copy the terminating entry					*/
	*lpszDest++ = *lpszSrc++;


	/* Copy the invocation part					*/
	*((UWORD FAR *)lpszDest)++ = *((UWORD FAR *)lpszSrc)++;
	while(*lpszSrc)
		*lpszDest++ = *lpszSrc++;

	/* and finally null terminate.					*/
	*lpszDest++ = '\0';
}


COUNT EnvSizeUp(VOID)
{
	UWORD uNewEnv;
	BOOL bError;
	BYTE FAR *lpszSrc = lpEnviron;
	COUNT nSize = 0;

	while('\0' != *lpszSrc)
	{
		while(*lpszSrc)
			lpszSrc++, ++nSize;
		++lpszSrc, ++nSize;
	}
	/* Count the terminating entry					*/
	lpszSrc++, ++nSize;


	/* Count the invocation part					*/
	((UWORD FAR *)lpszSrc)++;
	nSize += sizeof(UWORD);
	while(*lpszSrc)
		lpszSrc++, ++nSize;

	/* Count the terminating null ...				*/
	nSize++;

	/* and return the count.					*/
	return nSize;
}


BOOL EnvAlloc(COUNT nSize)
{
	COUNT nKlicks;
	BOOL bError;
	UWORD uSeg;

	/* Check for error						*/
	if(nSize <= EnvSizeUp())
		return FALSE;

	/* Do the allocation, then copy the environment			*/
	nKlicks = (nSize + PARASIZE - 1)/PARASIZE;
	uSeg = DosAllocMem(nKlicks, (BOOL FAR *)&bError);
	if(!bError)
	{
		EnvCopy(MK_FP(uSeg, 0), lpEnviron);
		DosFreeMem(FP_SEG(lpEnviron), bError);
		if(bError)
		{
			DosFreeMem(uSeg, &bError);
			return FALSE;
		}
		lpEnviron = MK_FP(uSeg, 0);
		return TRUE;
	}
	else
		return FALSE;
}

BOOL EnvSetVar(BYTE *pszName, BYTE *pszValue)
{
	BYTE *pszOldVal;

	/* See if it's already defined.  If it is, then we have a	*/
	/* operation.  Otherwise, we do an append.			*/
	pszOldVal = EnvLookup(pszName);
	if(pszOldVal)	/* Replace operation				*/
	{
		COUNT nSize, nNameLen;
		UWORD uEnvSize;
		psp FAR *lpPsp;
		mcb FAR *lpArena;
		UWORD uNewEnv, uArenaSeg;
		BYTE FAR *lpszSrc, FAR *lpszDest;
		BOOL bError;
		BYTE szNameBuffer[MAX_CMDLINE], *pszLclString;

		if((strlen(pszName)+strlen(pszValue)+2-strlen(pszOldVal))
		 >= EnvSizeUp())
			return FALSE;

		strcpy(szNameBuffer, pszName);
		nNameLen = strlen(szNameBuffer);
		for(pszLclString = szNameBuffer; *pszLclString != '\0'; pszLclString++)
			*pszLclString = toupper(*pszLclString);
		lpszSrc = lpEnviron;
		uArenaSeg = FP_SEG(lpEnviron) - 1;
		lpArena = MK_FP(uArenaSeg, 0);
		uNewEnv = DosAllocMem(lpArena -> m_size, (BOOL FAR *)&bError);
		if(bError)
			return FALSE;
		lpszDest = MK_FP(uNewEnv, 0);

		/* Copy the old environment into the new and append the	*/
		/* new one.						*/
		while('\0' != *lpszSrc)
		{
			if(0 == fstrncmp(lpszSrc, (BYTE FAR *)szNameBuffer, nNameLen))
			{
				/* Copy in the new string */
				for(pszLclString = szNameBuffer; *pszLclString; )
					*lpszDest++ = *pszLclString++;
				*lpszDest++ = '=';
				for(pszLclString = pszValue; *pszLclString; )
					*lpszDest++ = *pszLclString++;
				*lpszDest++ = '\0';

				/* Skip past the source */
				while(*lpszSrc)
					lpszSrc++;
				++lpszSrc;
			}
			else
			{
				while(*lpszSrc)
					*lpszDest++ = *lpszSrc++;
				++lpszSrc;
				*lpszDest++ = '\0';
			}
		}

		/* Copy the terminating entry					*/
		*lpszDest++ = *lpszSrc++;

		/* Copy the invocation part					*/
		*((UWORD FAR *)lpszDest)++ = *((UWORD FAR *)lpszSrc)++;
		while(*lpszSrc)
			*lpszDest++ = *lpszSrc++;

		/* and finally null terminate.					*/
		*lpszDest++ = '\0';

		EnvCopy(lpEnviron, MK_FP(uNewEnv, 0));
		DosFreeMem(uNewEnv, (BOOL FAR *)&bError);
		return !bError;
	}
	else		/* Append operation				*/
	{
		COUNT nSize;
		UWORD uEnvSize;
		psp FAR *lpPsp;
		mcb FAR *lpArena;
		UWORD uNewEnv, uArenaSeg;
		BYTE FAR *lpszSrc, FAR *lpszDest;
		BOOL bError;
		BYTE szNameBuffer[MAX_CMDLINE], *pszLclString;

		if((strlen(pszName)+strlen(pszValue)+2) >= EnvSizeUp())
			return FALSE;

		strcpy(szNameBuffer, pszName);
		for(pszLclString = szNameBuffer; *pszLclString != '\0'; pszLclString++)
			*pszLclString = toupper(*pszLclString);
		lpszSrc = lpEnviron;
		uArenaSeg = FP_SEG(lpEnviron) - 1;
		lpArena = MK_FP(uArenaSeg, 0);
		uNewEnv = DosAllocMem(lpArena -> m_size, (BOOL FAR *)&bError);
		if(bError)
			return FALSE;
		lpszDest = MK_FP(uNewEnv, 0);

		/* Copy the old environment into the new and append the	*/
		/* new one.						*/
		while('\0' != *lpszSrc)
		{
			while(*lpszSrc)
				*lpszDest++ = *lpszSrc++;
			++lpszSrc;
			*lpszDest++ = '\0';
		}

		/* Append the new one.					*/
		for(pszLclString = szNameBuffer; *pszLclString; )
			*lpszDest++ = *pszLclString++;
		*lpszDest++ = '=';
		for(pszLclString = pszValue; *pszLclString; )
			*lpszDest++ = *pszLclString++;
		*lpszDest++ = '\0';

		/* Copy the terminating entry					*/
		*lpszDest++ = *lpszSrc++;

		/* Copy the invocation part					*/
		*((UWORD FAR *)lpszDest)++ = *((UWORD FAR *)lpszSrc)++;
		while(*lpszSrc)
			*lpszDest++ = *lpszSrc++;

		/* and finally null terminate.					*/
		*lpszDest++ = '\0';

		EnvCopy(lpEnviron, MK_FP(uNewEnv, 0));
		DosFreeMem(uNewEnv, (BOOL FAR *)&bError);
		return !bError;
	}
	return TRUE;
}


BOOL EnvDump(VOID)
{
	BYTE FAR *lpszEnv;
	BYTE *pszLclBuffer;
	COUNT nCount;

	for(lpszEnv = lpEnviron; *lpszEnv != '\0'; )
	{
		static BYTE szBuffer[MAX_CMDLINE];

		for(pszLclBuffer = szBuffer, nCount = 0;
		 nCount < MAX_CMDLINE; nCount++)
		{
			*pszLclBuffer++ = *lpszEnv++;
			if(!*lpszEnv)
				break;
		}
		*pszLclBuffer++ = '\0';
		++lpszEnv;
		printf("%s\n", szBuffer);
	}
	return TRUE;
}
