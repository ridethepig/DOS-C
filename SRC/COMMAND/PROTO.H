/****************************************************************/
/*								*/
/*			    proto.h				*/
/*								*/
/*		     Global function prototypes			*/
/*								*/
/*			  August 9, 1991			*/
/*								*/
/*			Copyright (c) 1995			*/
/*			Pasquale J. Villani			*/
/*			All Rights Reserved			*/
/*								*/
/* This file is part of DOS-C.					*/
/*								*/
/* DOS-C is free software; you can redistribute it and/or	*/
/* modify it under the terms of the GNU General Public License	*/
/* as published by the Free Software Foundation; either version	*/
/* 2, or (at your option) any later version.			*/
/*								*/
/* DOS-C is distributed in the hope that it will be useful, but	*/
/* WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See	*/
/* the GNU General Public License for more details.		*/
/*								*/
/* You should have received a copy of the GNU General Public	*/
/* License along with DOS-C; see the file COPYING.  If not,	*/
/* write to the Free Software Foundation, 675 Mass Ave,		*/
/* Cambridge, MA 02139, USA.					*/
/****************************************************************/

/* $Logfile:   C:/dos-c/src/command/proto.h_v  $ */

/* $Log:   C:/dos-c/src/command/proto.h_v  $ 
   
      Rev 1.4   22 Feb 1998  9:44:18   patv
   Added critical error support.
   
      Rev 1.3   31 Jan 1998  8:12:30   patv
   Put preprocessor switch for version strings and changed log strings
   
      Rev 1.2   29 Aug 1996 13:07:06   patv
   Bug fixes for v0.91b
   
      Rev 1.1   01 Sep 1995 18:04:46   patv
   First GPL release.
   
      Rev 1.0   02 Jul 1995 10:02:32   patv
   Initial revision.
 */
/* $EndLog$ */

#ifdef MAIN
# ifdef VERSION_STRINGS
static BYTE *Proto_hRcsId = "$Header:   C:/dos-c/src/command/proto.h_v   1.4   22 Feb 1998  9:44:18   patv  $";
# endif
#endif

/* home.c								*/
VOID FindHome(VOID);
BOOL GoHome(VOID);

/* scan.c								*/
VOID scopy(BYTE *, BYTE *);
VOID expand(BYTE *, BYTE *);
VOID expandspl(BYTE *, BYTE *, COUNT, BYTE *);
BYTE *scan(BYTE *, BYTE *);
BYTE *scanspl(BYTE *, BYTE *, COUNT);
BYTE *skipwh(BYTE *);
BYTE *scan_seperator(BYTE *, BYTE *);

/* cmd.c								*/
VOID do_command(COUNT);

/* cmd_err.c								*/
VOID error_message(enum error_mess);

/* dos_lib.c								*/
COUNT DosOpen(BYTE FAR *, COUNT);
COUNT DosCreat(BYTE FAR *, COUNT);
COUNT DosClose(COUNT);
COUNT DosRead(COUNT, BYTE FAR *, COUNT);
COUNT DosWrite(COUNT, BYTE FAR *, COUNT);
LONG DosSeek(COUNT, COUNT, LONG);
COUNT DosPwd(COUNT, BYTE FAR *);
COUNT DosSetDrive(COUNT);
COUNT DosGetDrive(VOID);
COUNT DosCd(BYTE FAR *);
VOID DosSetDta(BYTE FAR *);
COUNT DosFindFirst(COUNT, BYTE FAR *);
COUNT DosFindNext(VOID);
COUNT DosFree(COUNT, COUNT FAR *, COUNT FAR *, COUNT FAR *, COUNT FAR *);
VOID DosExit(COUNT);
COUNT DosGetftime(COUNT, date *, time *);
COUNT DosSetftime(COUNT, date *, time *);
COUNT DosDelete(BYTE FAR *);
COUNT DosRename(BYTE FAR *, BYTE FAR *);
COUNT DosMkdir(BYTE FAR *);
COUNT DosRmdir(BYTE FAR *);
COUNT DosExec(BYTE FAR *, exec_blk FAR *);
VOID DosGetTime(COUNT *, COUNT *, COUNT *, COUNT *);
COUNT DosSetTime(COUNT *, COUNT *, COUNT *, COUNT *);
VOID DosSetVerify(BOOL);
BOOL DosGetVerify(VOID);
BOOL DosGetBreak(VOID);
VOID DosSetBreak(BOOL);
VOID DosGetDate(COUNT *, COUNT *, COUNT *, COUNT *);
COUNT DosSetDate(COUNT *, COUNT *, COUNT *);
UWORD DosRtnValue(VOID);
BOOL DosMemAlloc(COUNT, BOOL FAR *);
BOOL DosMemFree(UWORD, BOOL FAR *);
VOID DosParseFilename(BYTE FAR *, fcb FAR *, BITS);


/* ../fs/prf.c								*/
COUNT printf(CONST BYTE *, ...);
COUNT sprintf(BYTE *, CONST BYTE *, ...);
VOID put_console(COUNT);
VOID scopy(BYTE *, BYTE *);
BYTE *skipwh(BYTE *);
COUNT strcmp(BYTE *, BYTE *);
BOOL dir(COUNT, BYTE *[]);

/* ../misc/strings.c							*/
COUNT strlen(BYTE *);
COUNT fstrlen(BYTE FAR *);
VOID strcpy(BYTE *, BYTE *);
VOID strncpy(REG BYTE *d, REG BYTE *s, COUNT l);
VOID fstrncpy(BYTE FAR *d, BYTE FAR *s, COUNT l);
VOID strcat(BYTE *, BYTE *);
COUNT strcmp(BYTE *, BYTE *);
COUNT strncmp(BYTE *, BYTE *, COUNT);
COUNT fstrncmp(BYTE FAR *, BYTE FAR *, COUNT);

/* ../misc/num.c							*/
COUNT tolower(COUNT);
COUNT toupper(COUNT);
COUNT atoi(BYTE *);

/* ../misc/dosopt.c							*/
BOOL iswild(BYTE *);
BOOL iseol(BYTE);
COUNT dosopt(BYTE *, BYTE FAR *, ...);
VOID expand(BYTE *, BYTE *);

/* env.c								*/
COUNT EnvSizeUp(VOID);
BYTE *EnvLookup(BYTE *);
BOOL EnvSetVar(BYTE *, BYTE *);
BOOL EnvDump(VOID);
BOOL EnvClearVar(BYTE *);
BOOL EnvAlloc(COUNT);
BOOL EnvFree(BYTE FAR *);

/* doslib.c								*/
COUNT DosOpen(BYTE FAR *FileName, COUNT FileAccess);
COUNT DosCreat(BYTE FAR *FileName, COUNT Attributes);
COUNT DosClose(COUNT Handle);
COUNT DosRead (COUNT fd, BYTE FAR *buffer, COUNT count);
COUNT DosWrite (COUNT fd, BYTE FAR *buffer, COUNT count);
LONG DosSeek(COUNT Handle, COUNT MoveMethod, LONG FOffset);
COUNT DosPwd(COUNT Drive, BYTE FAR *CurDir);
COUNT DosSetDrive(COUNT DriveNumber);
COUNT DosGetDrive(VOID);
COUNT DosCd(BYTE FAR *Dir);
VOID DosSetDta(BYTE FAR *DTA);
COUNT DosFindFirst(COUNT Attributes, BYTE FAR *FileName);
COUNT DosFindNext(VOID);
COUNT DosFree(COUNT Drive, COUNT FAR *Spc, COUNT FAR *Ac, COUNT FAR *Bpc,COUNT  FAR *Tc);
VOID DosExit(COUNT ReturnValue);
COUNT DosGetftime(COUNT fd, date *fdate, time *ftime);
COUNT DosSetftime(COUNT fd, date *fdate, time *ftime);
COUNT DosDelete(BYTE FAR *FileName);
COUNT DosRename(BYTE FAR *OldName, BYTE FAR *NewName);
COUNT DosMkdir(BYTE FAR *Dir);
COUNT DosRmdir(BYTE FAR *Dir);
COUNT DosExec(BYTE FAR *ProgName, exec_blk FAR *ProgArgs);
UWORD DosRtnValue(VOID);
VOID DosGetTime(COUNT *hp, COUNT *mp, COUNT *sp, COUNT *hdp);
COUNT DosSetTime(COUNT *hp, COUNT *mp, COUNT *sp, COUNT *hdp);
VOID DosSetVerify(BOOL mode);
BOOL DosGetVerify(VOID);
BOOL DosGetBreak(VOID);
VOID DosSetBreak(BOOL BreakFlag);
VOID DosGetDate(COUNT *wdp, COUNT *mp, COUNT *mdp, COUNT *yp);
COUNT DosSetDate(COUNT *mp, COUNT *mdp, COUNT *yp);
BOOL DosCkKbReady(VOID);
COUNT DosRdKb(VOID);
UCOUNT DosAllocMem(UCOUNT MemSize, BOOL FAR *err);
VOID DosFreeMem(UCOUNT SegmentMem, BOOL FAR *err);
VOID DosParseFilename(BYTE FAR *pParseInput, fcb FAR *pFileFCB, BITS ParseControl);
BOOL DosDupHandle(COUNT OldHandle, COUNT FAR *pNewHandle, COUNT FAR *pErrorCode);
BOOL DosForceDupHandle(COUNT OpenHandle, COUNT DuplicateHandle, COUNT FAR *pErrorCode);

VOID INRPT FAR
CriticalError(
UWORD uRegBp, UWORD uRegDi, UWORD uRegSi, UWORD uRegDs, UWORD uRegEs,
UWORD uRegDx, UWORD uRegCx, UWORD uRegBx, UWORD uRegAx
);
