
/****************************************************************/
/*                                                              */
/*                          config.c                            */
/*                            DOS-C                             */
/*                                                              */
/*                config.sys Processing Functions               */
/*                                                              */
/*                      Copyright (c) 1996                      */
/*                      Pasquale J. Villani                     */
/*                      All Rights Reserved                     */
/*                                                              */
/* This file is part of DOS-C.                                  */
/*                                                              */
/* DOS-C is free software; you can redistribute it and/or       */
/* modify it under the terms of the GNU General Public License  */
/* as published by the Free Software Foundation; either version */
/* 2, or (at your option) any later version.                    */
/*                                                              */
/* DOS-C is distributed in the hope that it will be useful, but */
/* WITHOUT ANY WARRANTY; without even the implied warranty of   */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See    */
/* the GNU General Public License for more details.             */
/*                                                              */
/* You should have received a copy of the GNU General Public    */
/* License along with DOS-C; see the file COPYING.  If not,     */
/* write to the Free Software Foundation, 675 Mass Ave,         */
/* Cambridge, MA 02139, USA.                                    */
/****************************************************************/


#include "../../hdr/portab.h"
#include "globals.h"

/* $Logfile:   C:/dos-c/src/kernel/config.c_v  $ */
#ifdef VERSION_STRINGS
static BYTE *RcsId = "$Header:   C:/dos-c/src/kernel/config.c_v   1.6   22 Jan 1998  4:09:24   patv  $";
#endif

/* $Log:   C:/dos-c/src/kernel/config.c_v  $
 * 
 *    Rev 1.6   22 Jan 1998  4:09:24   patv
 * Fixed pointer problems affecting SDA
 *
 *    Rev 1.5   04 Jan 1998 23:15:18   patv
 * Changed Log for strip utility
 * 
 *    Rev 1.4   04 Jan 1998 17:26:14   patv
 * Corrected subdirectory bug
 * 
 *    Rev 1.3   16 Jan 1997 12:46:50   patv
 * pre-Release 0.92 feature additions
 * 
 *    Rev 1.1   29 May 1996 21:03:44   patv
 * bug fixes for v0.91a
 * 
 *    Rev 1.0   19 Feb 1996  3:22:16   patv
 * Added NLS, int2f and config.sys processing
 */
/* $EndLog$ */

#if 0
#define dos_open(x,y)   _open((const char *)(x),(y))
#define dos_read(x,y,z) _read((int)(x),(void *)(y),(unsigned)(z))
#define dos_close       _close
#endif

#ifdef KDB
# include <alloc.h>

# define KernelAlloc(x) adjust_far((void far *)malloc((unsigned long)(x)))
#endif

BYTE FAR *lpBase;
static BYTE FAR *lpOldLast;
static COUNT nCfgLine;
static COUNT nPass;
static BYTE szLine[256];
static BYTE szBuf[256];

int     singleStep = 0;

VOID Buffers(BYTE *pLine);
VOID Break(BYTE *pLine);
VOID Device(BYTE *pLine);
VOID Files(BYTE *pLine);
VOID Fcbs(BYTE *pLine);
VOID Lastdrive(BYTE *pLine);
VOID Country(BYTE *pLine);
VOID InitPgm(BYTE *pLine);
VOID Switchar(BYTE *pLine);
VOID CfgFailure(BYTE *pLine);
VOID Stacks(BYTE *pLine);
BYTE *GetNumArg(BYTE *pLine, COUNT *pnArg);
BYTE *GetStringArg(BYTE *pLine, BYTE *pszString);
struct dhdr FAR *linkdev(struct dhdr FAR *dhp);
UWORD 
initdev (struct dhdr FAR *dhp, BYTE FAR *cmdTail);
int     SkipLine(char *pLine);

static VOID FAR *AlignParagraph(VOID FAR *lpPtr);
#ifndef I86
# define AlignParagraph(x) (x)
#endif

#define EOF 0x1a

struct table *LookUp(struct table *p, BYTE *token);

struct table
{
	BYTE    *entry;
	BYTE    pass;
	VOID    (*func)(BYTE *pLine);
};

static struct table  commands[] =
{
	{"break",       1,      Break},
	{"buffers",     1,      Buffers},
	{"command",     1,      InitPgm},
	{"country",     1,      Country},
	{"device",      2,      Device},
	{"fcbs",        1,      Fcbs},
	{"files",       1,      Files},
	{"lastdrive",   1,      Lastdrive },
	/* rem is never executed by locking out pass                    */
	{"rem",         0,      CfgFailure},
	{"shell",       1,      InitPgm},
	{"stacks",      1,      Stacks },
	{"switchar",    1,      Switchar },
	/* default action                                               */
	{"",            -1,     CfgFailure}
};

#ifndef KDB
BYTE FAR *KernelAlloc(WORD nBytes);
#endif
BYTE *pLineStart;

/* Do first time initialization.  Store last so that we can reset it    */
/* later.                                                               */
void
PreConfig(void)
{
	/* Set pass number                                              */
	nPass = 0;

	/* Initialize the base memory pointers                          */
	lpOldLast = lpBase = AlignParagraph((BYTE FAR *)&last);

	/* Begin by initializing our system buffers                     */
	buffers = (struct buffer FAR *)
		KernelAlloc(Config.cfgBuffers * sizeof(struct buffer));
#ifdef DEBUG
	printf("Preliminary buffer allocated at 0x%04x:0x%04x\n",
		FP_SEG(buffers), FP_OFF(buffers));
#endif

	/* Initialize the file table                                    */
	f_nodes = (struct f_node FAR *)
		KernelAlloc(Config.cfgFiles * sizeof(struct f_node));
	/* sfthead = (sfttbl FAR *)&basesft; */
	/* FCBp = (sfttbl FAR *)&FcbSft; */
	FCBp = (sfttbl FAR *)
		KernelAlloc(sizeof(sftheader)
		 + Config.cfgFiles * sizeof(sft));
	sfthead = (sfttbl FAR *)
		KernelAlloc(sizeof(sftheader)
		 + Config.cfgFiles * sizeof(sft));

#ifdef DEBUG
	printf("Preliminary f_node allocated at 0x%04x:0x%04x\n",
		FP_SEG(f_nodes), FP_OFF(f_nodes));
	printf("Preliminary FCB table allocated at 0x%04x:0x%04x\n",
		FP_SEG(FCBp), FP_OFF(FCBp));
	printf("Preliminary sft table allocated at 0x%04x:0x%04x\n",
		FP_SEG(sfthead), FP_OFF(sfthead));
#endif

	/* Done.  Now initialize the MCB structure                      */
	/* This next line is 8086 and 80x86 real mode specific          */
#ifdef DEBUG
	printf("Preliminary  allocation completed: top at 0x%04x:0x%04x\n",
		FP_SEG(lpBase), FP_OFF(lpBase));
#endif

#ifdef KDB
	lpBase = malloc(4096);
	first_mcb = FP_SEG(lpBase) + ((FP_OFF(lpBase) + 0x0f) >> 4);
#else
	first_mcb = FP_SEG(lpBase) + ((FP_OFF(lpBase) + 0x0f) >> 4);
#endif

	/* We expect ram_top as Kbytes, so convert to paragraphs */
	mcb_init((mcb FAR *)(MK_FP(first_mcb, 0)),
	 (ram_top << 6) - first_mcb - 1);
	nPass = 1;
}


/* Do second pass initialization.                                       */
/* Also, run config.sys to load drivers.                                */
void
PostConfig(void)
{
	/* Set pass number                                              */
	nPass = 2;

	/* Initialize the base memory pointers from last time.          */
	lpBase = AlignParagraph(lpOldLast);

	/* Begin by initializing our system buffers                     */
	buffers = (struct buffer FAR *)
		KernelAlloc(Config.cfgBuffers * sizeof(struct buffer));
#ifdef DEBUG
	printf("Buffer allocated at 0x%04x:0x%04x\n",
		FP_SEG(buffers), FP_OFF(buffers));
#endif

	/* Initialize the file table                                    */
	f_nodes = (struct f_node FAR *)
		KernelAlloc(Config.cfgFiles * sizeof(struct f_node));
	/* sfthead = (sfttbl FAR *)&basesft; */
	/* FCBp = (sfttbl FAR *)&FcbSft; */
	FCBp = (sfttbl FAR *)
		KernelAlloc(sizeof(sftheader)
		 + Config.cfgFiles * sizeof(sft));
	sfthead = (sfttbl FAR *)
		KernelAlloc(sizeof(sftheader)
		 + Config.cfgFiles * sizeof(sft));

#ifdef DEBUG
	printf("f_node allocated at 0x%04x:0x%04x\n",
		FP_SEG(f_nodes), FP_OFF(f_nodes));
	printf("FCB table allocated at 0x%04x:0x%04x\n",
		FP_SEG(FCBp), FP_OFF(FCBp));
	printf("sft table allocated at 0x%04x:0x%04x\n",
		FP_SEG(sfthead), FP_OFF(sfthead));
#endif
	if (Config.cfgStacks)
	{
		VOID FAR *stackBase = KernelAlloc(Config.cfgStacks * Config.cfgStackSize);
		init_stacks(stackBase, Config.cfgStacks, Config.cfgStackSize);
	
#ifdef  DEBUG
		printf("Stacks allocated at %04x:%04x\n",
		    FP_SEG(stackBase), FP_OFF(stackBase));
#endif
	}
#ifdef DEBUG
	printf("Allocation completed: top at 0x%04x:0x%04x\n",
		FP_SEG(lpBase), FP_OFF(lpBase));
#endif
}

/* This code must be executed after device drivers has been loaded */
VOID    configDone(VOID)
{
	COUNT   i;

	first_mcb = FP_SEG(lpBase) + ((FP_OFF(lpBase) + 0x0f) >> 4);
	
	/* We expect ram_top as Kbytes, so convert to paragraphs */
	mcb_init((mcb FAR *)(MK_FP(first_mcb, 0)),
	 (ram_top << 6) - first_mcb - 1);
	
	/* The standard handles should be reopened here, because
	   we may have loaded new console or printer drivers in CONFIG.SYS */
}

VOID DoConfig(VOID)
{
	COUNT nFileDesc;
	COUNT nRetCode;
	BYTE    *pLine, *pTmp;
	BOOL    bEof;

	/* Check to see if we have a config.sys file.  If not, just     */
	/* exit since we don't force the user to have one.              */
	if((nFileDesc = dos_open((BYTE FAR *)"config.sys", 0)) < 0)
	{
#ifdef DEBUG
		printf("CONFIG.SYS not found\n");
#endif
		return;
	}

	/* Have one -- initialize.                                      */
	nCfgLine = 0;
	bEof = 0;
	pLine = szLine;

	/* Read each line into the buffer and then parse the line,      */
	/* do the table lookup and execute the handler for that         */
	/* function.                                                    */
	while (!bEof)
	{
		struct table *pEntry;
		UWORD bytesLeft = 0;
		
		if (pLine > szLine) bytesLeft = LINESIZE - (pLine - szLine);
		
		if (bytesLeft)
		{
		    fbcopy(pLine, szLine, LINESIZE - bytesLeft);
		    pLine = szLine + bytesLeft;
		}

		/* Read a line from config                              */
		/* Interrupt processing if read error or no bytes read  */
		if ((nRetCode = dos_read(nFileDesc, pLine, LINESIZE - bytesLeft)) <= 0) 
		    break;

		/* If the buffer was not filled completely, append a 
		   CTRL-Z character to mark where the file ends */
		
		if (nRetCode + bytesLeft < LINESIZE) 
		    szLine[nRetCode + bytesLeft] = EOF;

		/* Process the buffer, line by line */
		pLine = szLine;

		while (!bEof && *pLine != EOF)
		{
			for (pTmp = pLine; pTmp - szLine < LINESIZE; pTmp++)
			{
				if (*pTmp == '\r' || *pTmp == EOF) 
				    break;
			}

			if (pTmp - szLine >= LINESIZE)
			    break;
		
			if (*pTmp == EOF) 
			    bEof = TRUE;

			*pTmp = '\0';
			pLineStart = pLine;

			/* Skip leading white space and get verb.               */
			pLine = scan(pLine, szBuf);

			/* Translate the verb to lower case ...                 */
			for(pTmp = szBuf; *pTmp != '\0'; pTmp++)
				*pTmp = tolower(*pTmp);

			/* If the line was blank, skip it.  Otherwise, look up  */
			/* the verb and execute the appropriate function.       */
			if(*szBuf != '\0')
			{
				pEntry = LookUp(commands, szBuf);
					
				if(pEntry -> pass < 0 || pEntry -> pass == nPass)
				{        
					if (!singleStep || !SkipLine(pLineStart))
					{
						skipwh(pLine);
						
						if('=' != *pLine)
							CfgFailure(pLine);
						else (*(pEntry -> func))(++pLine);
					}
				}
			}
skipLine:               nCfgLine++;
			pLine += strlen(pLine) + 1;
		}
	}
	dos_close(nFileDesc);
}

struct table *LookUp(struct table *p, BYTE *token)
{
	while(*(p -> entry) != '\0')
	{
		if(strcmp(p -> entry, token) == 0)
			break;
		else
			++p;
	}
	return p;
}
					    
BOOL    SkipLine(char *pLine)
{
	char    kbdbuf[16];
	keyboard *kp = (keyboard *)kbdbuf;
	char    *pKbd = &kp->kb_buf[0];

	kp->kb_size = 12;
	kp->kb_count = 0;
	
	printf("%s [Y,N]?", pLine);
	sti(kp);
	
	pKbd = skipwh(pKbd);
	
	if (*pKbd == 'n' || *pKbd == 'N')
		return TRUE;        
	    
	return FALSE;
}

BYTE *GetNumArg(BYTE *pLine, COUNT *pnArg)
{
	/* look for NUMBER                               */
	pLine = skipwh(pLine);
	if(!isnum(pLine))
	{
		CfgFailure(pLine);
		return (BYTE *)0;
	}
	return GetNumber(pLine, pnArg);
}


BYTE *GetStringArg(BYTE *pLine, BYTE *pszString)
{
	/* look for STRING                               */
	pLine = skipwh(pLine);

	/* just return whatever string is there, including null         */
	return scan(pLine, pszString);
}

static VOID Buffers(BYTE *pLine)
{
	COUNT nBuffers;

	/* Get the argument                                             */
	if(GetNumArg(pLine, &nBuffers) == (BYTE *)0)
		return;

	/* Got the value, assign either default or new value            */
	Config.cfgBuffers = max(Config.cfgBuffers, nBuffers);
}

static VOID Files(BYTE *pLine)
{
	COUNT nFiles;

	/* Get the argument                                             */
	if(GetNumArg(pLine, &nFiles) == (BYTE *)0)
		return;

	/* Got the value, assign either default or new value            */
	Config.cfgFiles = max(Config.cfgFiles, nFiles);
}

static VOID Lastdrive(BYTE *pLine)
{
	/* Format:   LASTDRIVE = letter         */
	COUNT   nFiles;
	BYTE    drv;

	pLine = skipwh(pLine);
	drv = *pLine & ~0x20;   

	if (drv < 'A' || drv > 'Z')
	{        
		CfgFailure(pLine);
		return;
	}
	drv -= 'A';
	Config.cfgLastdrive = max(Config.cfgLastdrive, drv);
}

static VOID Switchar(BYTE *pLine)
{
	/* Format: SWITCHAR = character         */
	
	GetStringArg(pLine, szBuf);
	switchar = *szBuf;
}

static VOID Fcbs(BYTE *pLine)
{
	/*  Format:     FCBS = totalFcbs [,protectedFcbs]    */
	
	if ((pLine = GetNumArg(pLine, &Config.cfgFcbs)) == 0)
	    return;

	pLine = skipwh(pLine);

	if (*pLine == ',')
	    GetNumArg(++pLine, &Config.cfgProtFcbs);

	if (Config.cfgProtFcbs > Config.cfgFcbs)
	    Config.cfgProtFcbs = Config.cfgFcbs;
}

static VOID Country(BYTE *pLine)
{
	/* Format: COUNTRY = countryCode, [codePage], filename  */
	UWORD   ctryCode;
	UWORD   codePage;

	if ((pLine = GetNumArg(pLine, &ctryCode)) == 0)
	    return;
	    
	pLine = skipwh(pLine);
	if (*pLine == ',')
	{
		pLine = skipwh(pLine);
		
		if (*pLine == ',') 
		{
		    codePage = 0;
		    ++pLine;
		} else {
		    if ((pLine = GetNumArg(pLine, &codePage)) == 0)
			return;
		}

		pLine = skipwh(pLine);
		if (*pLine == ',')
		{
		    GetStringArg(++pLine, szBuf);
		    
		    if (LoadCountryInfo(szBuf, ctryCode, codePage))
			return;
		}
	}
	CfgFailure(pLine);
}

static VOID Stacks(BYTE *pLine)
{
	/* Format:  STACKS = stacks [, stackSize]       */
	pLine = GetNumArg(pLine, &Config.cfgStacks);

	if (*pLine == ',')
	    GetNumArg(++pLine, &Config.cfgStackSize);

	if (Config.cfgStacks)
	{
	    if (Config.cfgStackSize < 32) Config.cfgStackSize = 32;
	    if (Config.cfgStackSize > 512) Config.cfgStackSize = 512;
	    if (Config.cfgStacks > 64) Config.cfgStacks = 64;
	}
}

static VOID InitPgm(BYTE *pLine)
{
	/* Get the string argument that represents the new init pgm     */
	pLine = GetStringArg(pLine, Config.cfgInit);

	/* Now take whatever tail is left and add it on as a single     */
	/* string.                                                      */
	strcpy(Config.cfgInitTail, pLine);

	/* and add a DOS new line just to be safe                       */
	strcat(Config.cfgInitTail, "\r\n");
}

static VOID   Break(BYTE *pLine)
{
	/* Format:      BREAK = (ON | OFF)      */
	BYTE *pTmp;

	GetStringArg(pLine, szBuf);
	break_ena = strcmp(szBuf, "OFF")? 1 : 0;
}

static VOID Device(BYTE *pLine)
{
	VOID FAR *driver_ptr;
	BYTE *pTmp;
	exec_blk eb;
	struct dhdr FAR *dhp;
	struct dhdr FAR *next_dhp;
	UWORD dev_seg = (((ULONG)FP_SEG(lpBase) << 4) + FP_OFF(lpBase) + 0xf) >> 4;
	
	/* Get the device driver name                                   */
	GetStringArg(pLine, szBuf);

	/* The driver is loaded at the top of allocated memory.         */             
	/* The device driver is paragraph aligned.                      */
	eb.load.reloc = eb.load.load_seg = dev_seg;
	dhp = MK_FP(dev_seg, 0);
	
#ifdef DEBUG
	printf("Loading device driver %s at segment %04x\n", 
	       szBuf, dev_seg);
#endif

	if (DosExec(3, &eb, szBuf) == SUCCESS)
	{
	    while(FP_OFF(dhp) != 0xFFFF)
	    {
		next_dhp = MK_FP(FP_SEG(dhp), FP_OFF(dhp -> dh_next));
		dhp -> dh_next = nul_dev.dh_next; 
		link_dhdr(&nul_dev, dhp, pLine); 
		dhp = next_dhp;
	    } 
	} else CfgFailure(pLine);
}

static VOID CfgFailure(BYTE *pLine)
{
	BYTE *pTmp = pLineStart;

	printf("CONFIG.SYS error in line %d\n", nCfgLine);
	printf(">>>%s\n", pTmp);
	while(++pTmp != pLine)
		printf(" ");
	printf("^\n");
}

#ifndef KDB
static BYTE FAR *
KernelAlloc(WORD nBytes)
{
	BYTE FAR *lpAllocated;

	lpBase = AlignParagraph(lpBase);
	lpAllocated = lpBase;

	if (0x10000 - FP_OFF(lpBase) <= nBytes)
	{
		UWORD newOffs = (FP_OFF(lpBase) + nBytes) & 0xFFFF;
		UWORD newSeg = FP_SEG(lpBase) + 0x1000;

		lpBase = MK_FP(newSeg, newOffs);
	} else lpBase += nBytes;

	return lpAllocated;
}
#endif

#ifdef I86
static VOID FAR *AlignParagraph(VOID FAR *lpPtr)
{
	ULONG lTemp;
	UWORD uSegVal;

	/* First, convert the segmented pointer to linear address	*/
	lTemp = FP_SEG(lpPtr);
	lTemp = (lTemp << 4) + FP_OFF(lpPtr);

	/* Next, round up the linear address to a paragraph boundary.	*/
	lTemp += 0x0f;
	lTemp &= 0xfffffff0l;

	/* Break it into segments.                                      */
	uSegVal = (UWORD)(lTemp >> 4);

	/* and return an adddress adjusted to the nearest paragraph     */
	/* boundary.                                                    */
	return MK_FP(uSegVal, 0);
}
#endif
