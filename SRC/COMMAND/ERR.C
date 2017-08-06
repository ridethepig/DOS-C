/****************************************************************/
/*								*/
/*			      err.c				*/
/*								*/
/*		     command.com Error Support			*/
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

/* $Logfile:   C:/usr/patv/dos-c/src/command/err.c_v  $ */

/* $Log:   C:/usr/patv/dos-c/src/command/err.c_v  $ 
 * 
 *    Rev 1.5   06 Dec 1998  8:41:44   patv
 * New error handling algorithms.
 * 
 *    Rev 1.4   22 Feb 1998  9:44:16   patv
 * Added critical error support.
 * 
 *    Rev 1.3   31 Jan 1998  8:12:30   patv
 * Put preprocessor switch for version strings and changed log strings
 * 
 *    Rev 1.2   29 Aug 1996 13:06:52   patv
 * Bug fixes for v0.91b
 * 
 *    Rev 1.1   01 Sep 1995 18:04:48   patv
 * First GPL release.
 * 
 *    Rev 1.0   02 Jul 1995  9:59:56   patv
 * Initial revision.
 */
/* $EndLog$ */

#ifdef VERSION_STRINGS
static char *RcsId = "$Header:   C:/usr/patv/dos-c/src/command/err.c_v   1.5   06 Dec 1998  8:41:44   patv  $";
#endif

#include "../../hdr/portab.h"
#include <conio.h>
#include "globals.h"
#include "proto.h"


static BYTE *GetMsgString(COUNT uError);
static VOID PrintString(BYTE *pszString);

static BYTE szBuffer[512];		/* special buffer to avoid printf */


VOID error_message(index)
enum error_mess index;
{
	printf(error_messages[index], error_mess_str);
	printf("\n\n");
	error_mess_str = "";
}

/* The following code is DOS and 8086 specific			*/
#pragma argsused
VOID INRPT FAR
CriticalError(
UWORD uRegBp, UWORD uRegDi, UWORD uRegSi, UWORD uRegDs, UWORD uRegEs,
UWORD uRegDx, UWORD uRegCx, UWORD uRegBx, UWORD uRegAx
)
{
	UWORD uFlags, uDrive, uError;
	struct dhdr FAR *lpDevice;
	BYTE *pszMsg, *pszOr = "";
	static BYTE szDevice[9];
	COUNT nIdx;

	uFlags = uRegAx >> 8;
	uDrive = uRegAx & 0xff;
	uError = uRegDi & 0xff;
	lpDevice = MK_FP(uRegBp, uRegSi);

	/* Determine the error message string			*/
	pszMsg = GetMsgString(uError);

loop:
	/* If it's a character device, get the device name and 	*/
	/* put out the character message.			*/
	if(uFlags & EFLG_CHAR)
	{
		for(nIdx = 0; nIdx < 8; nIdx++)
		{
			if(lpDevice -> dh_name[nIdx] == ' ')
				break;
			szDevice[nIdx] = lpDevice -> dh_name[nIdx];
		}
		szDevice[nIdx] = '\0';

		sprintf(szBuffer, "\r\n\n%s error on %s\r\n",
		 pszMsg, szDevice);
		PrintString(szBuffer);
	}
	/* Otherwise it's a block device and just print the	*/
	/* message.						*/
	else
	{
		sprintf(szBuffer, "\r\n\n%s error on drive %c\r\n",
		 pszMsg, uDrive + 'A');
		PrintString(szBuffer);
	}


	if(uFlags & EFLG_ABORT)		/* Handler can abort	*/
	{
		pszOr = "or ";
		PrintString("Abort, ");
	}

	if(uFlags & EFLG_RETRY)		/* Handler can retry	*/
	{
		pszOr = "or ";
		PrintString("Retry, ");
	}

	if(uFlags & EFLG_IGNORE)	/* Handler can ignore	*/
	{
		pszOr = "or ";
		PrintString("Continue, ");
	}

	/* Fail is always the last option			*/
	sprintf(szBuffer, "%sFail? ", pszOr);
	PrintString(szBuffer);

	/* Clear out AL for return value.			*/
	uRegAx &= 0xff00;

	/* Get the reply from the user and act on it		*/
	nIdx = getche();
	PrintString("\r\n");
	switch(nIdx)
	{
	case 'A':
	case 'a':
		if(!(uFlags & EFLG_ABORT))
			goto loop;
		uRegAx |= ABORT;
		return;

	case 'R':
	case 'r':
		if(!(uFlags & EFLG_RETRY))
			goto loop;
		uRegAx |= RETRY;
		return;

	case 'C':
	case 'c':
		if(!(uFlags & EFLG_IGNORE))
			goto loop;
		uRegAx |= CONTINUE;
		return;

	case 'F':
	case 'f':
		uRegAx |= FAIL;
		return;

	default:
		goto loop;
	}
}


static BYTE *
GetMsgString(COUNT nError)
{
	switch(nError)
	{
	case E_WRPRT:
		return "Write Protect";

	case E_UNIT:
		return "Unknown Unit";

	case E_NOTRDY:
		return "Device Not Ready";

	case E_CMD:
		return "Unknown Command";

	case E_CRC:
		return "Crc Error";

	case E_LENGTH:
		return "Bad Length";

	case E_SEEK:
		return "Seek Error";

	case E_MEDIA:
		return "Unknown Media";

	case E_NOTFND:
		return "Sector Not Found";

	case E_PAPER:
		return "No Paper";

	case E_WRITE:
		return "Write Fault";

	case E_READ:
		return "Read Fault";

	case E_FAILURE:
	default:
		return "General Failure";
	}
}

static VOID PrintString(BYTE *pszString)
{
	while(*pszString)
	{
		_DL = *pszString++;
		_AH = 0x02;
		asm int 21h
	}
}


/* This code is for small/tiny model only so we can make	*/
/* a .COM file out of the interpreter.				*/
VOID InitErr(VOID)
{
	asm {
		xor	ax,ax
		mov	es,ax
		mov	si,24h*4
		mov	ax,offset CriticalError
		mov	es:[si],ax
		mov	ax,cs
		mov	es:[si+2],ax
	}
}


