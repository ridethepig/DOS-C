/****************************************************************/
/*								*/
/*			     prompt.c				*/
/*								*/
/*		      command.com prompt command		*/
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

/* $Logfile:   C:/dos-c/src/command/prompt.c_v  $ */

/* $Log:   C:/dos-c/src/command/prompt.c_v  $ 
 * 
 *    Rev 1.3   31 Jan 1998  8:12:28   patv
 * Put preprocessor switch for version strings and changed log strings
 * 
 *    Rev 1.2   29 Aug 1996 13:06:56   patv
 * Bug fixes for v0.91b
 * 
 *    Rev 1.1   01 Sep 1995 18:04:42   patv
 * First GPL release.
 * 
 *    Rev 1.0   02 Jul 1995 10:02:14   patv
 * Initial revision.
 */
/* $EndLog$ */

#ifdef VERSION_STRINGS
static char *RcsId = "$Header:   C:/dos-c/src/command/prompt.c_v   1.3   31 Jan 1998  8:12:28   patv  $";
#endif

#include "../../hdr/portab.h"
#include "globals.h"
#include "proto.h"

/* put_prompt - print prompt obeying $x commands */
VOID put_prompt(fmt)
REG BYTE *fmt;
{
	REG BYTE c;
	BYTE *s;
	BYTE directory[NAMEMAX];

	while ((c = *fmt++) != '\0')
	{
		if (c != '$')
		{
			if(c != '\n' && c != '\r')
				put_console(c);
			continue;
		}
		c = *fmt++;
		switch (toupper(c))
		{
		case 'P':
			DosPwd(default_drive + 1, (BYTE FAR *)directory);
			printf("%c:\\%s", 'A' + default_drive, directory);
			continue;

		case 'N':
			put_console('A' + default_drive);
			continue;

		case 'G':
			put_console('>');
			continue;

		case 'L':
			put_console('<');
			continue;

		case 'B':
			put_console('|');
			continue;

		case 'E':
			put_console('=');
			continue;

		default:
			put_console(c);
			continue;
		}
	}
}


