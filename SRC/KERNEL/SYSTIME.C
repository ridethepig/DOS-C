
/****************************************************************/
/*								*/
/*			     systime.c				*/
/*								*/
/*		      DOS/C Date/Time Functions			*/
/*								*/
/*		        Copyright (c) 1998			*/
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


#include "../../hdr/portab.h"
#include "../../hdr/time.h"
#include "../../hdr/date.h"
#include "globals.h"

/* $Logfile:   C:/usr/patv/dos-c/src/kernel/systime.c_v  $ */
#ifdef VERSION_STRINGS
static BYTE *RcsId = "$Header:   C:/usr/patv/dos-c/src/kernel/systime.c_v   1.6   06 Dec 1998  8:47:30   patv  $";
#endif

/* $Log:   C:/usr/patv/dos-c/src/kernel/systime.c_v  $
 * 
 *    Rev 1.6   06 Dec 1998  8:47:30   patv
 * Bug fixes.
 * 
 *    Rev 1.5   04 Jan 1998 23:15:22   patv
 * Changed Log for strip utility
 * 
 *    Rev 1.4   03 Jan 1998  8:36:50   patv
 * Converted data area to SDA format
 * 
 *    Rev 1.3   29 May 1996 21:03:40   patv
 * bug fixes for v0.91a
 * 
 *    Rev 1.2   19 Feb 1996  3:21:34   patv
 * Added NLS, int2f and config.sys processing
 * 
 *    Rev 1.1   01 Sep 1995 17:54:16   patv
 * First GPL release.
 * 
 *    Rev 1.0   02 Jul 1995  8:32:20   patv
 * Initial revision.
 */
/* $EndLog$ */

static WORD days[2][13] =
{
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
	{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
};

static WORD ndays[2][13] =
{
	/*  1   2   3   4   5   6   7   8   9   10  11  12 */
	{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

extern BYTE
	Month,
	DayOfMonth,
	DayOfWeek;

extern COUNT
	Year,
	YearsSince1980;

extern 	request
	ClkReqHdr;

VOID DosGetTime(hp, mp, sp, hdp)
BYTE FAR *hp, FAR *mp, FAR *sp, FAR *hdp;
{
	ClkReqHdr.r_length = sizeof(request);
	ClkReqHdr.r_command = C_INPUT;
	ClkReqHdr.r_count = sizeof(struct ClockRecord);
	ClkReqHdr.r_trans = (BYTE FAR *)(&ClkRecord);
	ClkReqHdr.r_status = 0;
	execrh((request FAR *)&ClkReqHdr, (struct dhdr FAR *)clock);
	if(ClkReqHdr.r_status & S_ERROR)
		return;

	*hp =  ClkRecord.clkHours;
	*mp =  ClkRecord.clkMinutes;
	*sp =  ClkRecord.clkSeconds;
	*hdp = ClkRecord.clkHundredths;
}


COUNT DosSetTime(hp, mp, sp, hdp)
BYTE FAR *hp, FAR *mp, FAR *sp, FAR *hdp;
{
	DosGetDate((BYTE FAR *)&DayOfWeek, (BYTE FAR *)&Month,
	 (BYTE FAR *)&DayOfMonth, (COUNT FAR *)&Year);

	ClkRecord.clkHours = *hp;
	ClkRecord.clkMinutes = *mp;
	ClkRecord.clkSeconds = *sp;
	ClkRecord.clkHundredths = *hdp;

	YearsSince1980 = Year - 1980;
	ClkRecord.clkDays = DayOfMonth
	 + days[!(Year & 0x3)][Month - 1]
	 + ((YearsSince1980) * 365)
	 + ((YearsSince1980 + 3) / 4);

	ClkReqHdr.r_length = sizeof(request);
	ClkReqHdr.r_command = C_OUTPUT;
	ClkReqHdr.r_count = sizeof(struct ClockRecord);
	ClkReqHdr.r_trans = (BYTE FAR *)(&ClkRecord);
	ClkReqHdr.r_status = 0;
	execrh((request FAR *)&ClkReqHdr, (struct dhdr FAR *)clock);
	if(ClkReqHdr.r_status & S_ERROR)
		return char_error(&ClkReqHdr, (struct dhdr FAR *)clock);
	return SUCCESS;
}


VOID DosGetDate(wdp, mp, mdp, yp)
BYTE FAR *wdp, FAR *mp, FAR *mdp;
COUNT FAR *yp;
{
	COUNT count, c;

	ClkReqHdr.r_length = sizeof(request);
	ClkReqHdr.r_command = C_INPUT;
	ClkReqHdr.r_count = sizeof(struct ClockRecord);
	ClkReqHdr.r_trans = (BYTE FAR *)(&ClkRecord);
	ClkReqHdr.r_status = 0;
	execrh((request FAR *)&ClkReqHdr, (struct dhdr FAR *)clock);
	if(ClkReqHdr.r_status & S_ERROR)
		return;

	for(Year = 1980, c = ClkRecord.clkDays; c > 0; )
	{
		 count = !(Year & 0x3) ? 366 : 365;
		 if(c > count)
		 {
			++Year;
			c -= count;
		 }
		 else
			break;
	}

	/* c contains the days left and count the number of days for	*/
	/* that year.  Use this to index the table.			*/
	for(Month = 1; Month < 13; Month++)
	{
		if(days[count == 366][Month] > c)
		{
			DayOfMonth = c - days[count == 366][Month - 1];
			break;
		}
	}

	*mp = Month;
	*mdp = DayOfMonth;
	*yp = Year;

	/* Day of week is simple. Take mod 7, add 2 (for Tuesday	*/
	/* 1-1-80) and take mod again					*/
	
	DayOfWeek = (ClkRecord.clkDays - 1) % 7;
	DayOfWeek += 2;
	DayOfWeek %= 7;
	*wdp = DayOfWeek;
}


COUNT DosSetDate(mp, mdp, yp)
BYTE FAR *mp, FAR *mdp;
COUNT FAR *yp;
{
	Month = *mp;
	DayOfMonth = *mdp;
	Year = *yp;
	if(Year < 1980 || Year > 2099
	 || Month < 1 || Month > 12
	 || DayOfMonth < 1 || DayOfMonth > ndays[!(Year & 0x3)][Month])
		return DE_INVLDDATA;

	DosGetTime((BYTE FAR *)&ClkRecord.clkHours,
	 (BYTE FAR *)&ClkRecord.clkMinutes,
	 (BYTE FAR *)&ClkRecord.clkSeconds,
	 (BYTE FAR *)&ClkRecord.clkHundredths);

	YearsSince1980 = Year - 1980;
	ClkRecord.clkDays = DayOfMonth
	 + days[!(Year & 0x3)][Month - 1]
	 + ((YearsSince1980) * 365)
	 + ((YearsSince1980 + 3) / 4);

	ClkReqHdr.r_length = sizeof(request);
	ClkReqHdr.r_command = C_OUTPUT;
	ClkReqHdr.r_count = sizeof(struct ClockRecord);
	ClkReqHdr.r_trans = (BYTE FAR *)(&ClkRecord);
	ClkReqHdr.r_status = 0;
	execrh((request FAR *)&ClkReqHdr, (struct dhdr FAR *)clock);
	if(ClkReqHdr.r_status & S_ERROR)
		return char_error(&ClkReqHdr, (struct dhdr FAR *)clock);
	return SUCCESS;
}

