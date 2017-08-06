/****************************************************************/
/*								*/
/*			     ver.c				*/
/*								*/
/*		      command.com ver command			*/
/*								*/
/*		     Copyright (c) 1995, 1997			*/
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

/* $Logfile:   C:/dos-c/src/command/ver.c_v  $ */

/* $Log:   C:/dos-c/src/command/ver.c_v  $ 
 * 
 *    Rev 1.6   31 Jan 1998  8:12:26   patv
 * Put preprocessor switch for version strings and changed log strings
 * 
 *    Rev 1.5   06 Feb 1997 22:00:32   patv
 * Corrected compatibility printf
 * 
 *    Rev 1.4   06 Feb 1997 21:29:22   patv
 * Modified version command for new version designation
 * 
 *    Rev 1.3   22 Jan 1997 12:51:44   patv
 * Changed to support prjoect wide version.h file
 * 
 *    Rev 1.2   29 Aug 1996 13:06:58   patv
 * Bug fixes for v0.91b
 * 
 *    Rev 1.1   01 Sep 1995 18:04:46   patv
 * First GPL release.
 * 
 *    Rev 1.0   02 Jul 1995 10:02:28   patv
 * Initial revision.
 */
/* $EndLog$ */

#ifdef VERSION_STRINGS
static char *RcsId = "$Header:   C:/dos-c/src/command/ver.c_v   1.6   31 Jan 1998  8:12:26   patv  $";
#endif

#include "../../hdr/portab.h"
#include "globals.h"
#include "proto.h"

BOOL ver(nlFLAG)
BOOL nlFLAG;
{
	printf(dosc_version, REVISION_MAJOR, REVISION_MINOR, REVISION_SEQ);
	printf(version, MAJOR_RELEASE, MINOR_RELEASE);
	if(nlFLAG)
		printf("\n");
	return TRUE;
}
