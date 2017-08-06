
/****************************************************************/
/*                                                              */
/*                          chario.c                            */
/*                           DOS-C                              */
/*                                                              */
/*    Character device functions and device driver interface    */
/*                                                              */
/*                      Copyright (c) 1994                      */
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
/*                                                              */
/****************************************************************/

#include "../../hdr/portab.h"

/* $Logfile:   C:/usr/patv/dos-c/src/fs/chario.c_v  $ */
#ifdef VERSION_STRINGS
static BYTE *charioRcsId = "$Header:   C:/usr/patv/dos-c/src/fs/chario.c_v   1.9   06 Dec 1998  8:43:36   patv  $";
#endif

/* $Log:   C:/usr/patv/dos-c/src/fs/chario.c_v  $
 * 
 *    Rev 1.9   06 Dec 1998  8:43:36   patv
 * changes in character I/O because of new I/O subsystem.
 * 
 *    Rev 1.8   11 Jan 1998  2:06:08   patv
 * Added functionality to ioctl.
 * 
 *    Rev 1.7   08 Jan 1998 21:36:40   patv
 * Changed automatic requestic packets to static to save stack space.
 * 
 *    Rev 1.6   04 Jan 1998 23:14:38   patv
 * Changed Log for strip utility
 * 
 *    Rev 1.5   30 Dec 1997  4:00:20   patv
 * Modified to support SDA
 * 
 *    Rev 1.4   16 Jan 1997 12:46:36   patv
 * pre-Release 0.92 feature additions
 * 
 *    Rev 1.3   29 May 1996 21:15:12   patv
 * bug fixes for v0.91a
 * 
 *    Rev 1.2   01 Sep 1995 17:48:42   patv
 * First GPL release.
 * 
 *    Rev 1.1   30 Jul 1995 20:50:26   patv
 * Eliminated version strings in ipl
 * 
 *    Rev 1.0   02 Jul 1995  8:05:44   patv
 * Initial revision.
 *
 */ 
/* $EndLog$ */

#include "globals.h"

static BYTE *con_name = "CON";

#if !defined(KERNEL) && !defined(IPL)
VOID INRPT FAR 
handle_break (void)
{
}
#endif

#ifdef PROTO
BOOL _sto(COUNT);
VOID kbfill(keyboard FAR *, UCOUNT, BOOL);
struct dhdr FAR *finddev(UWORD attr_mask);

#else
BOOL _sto();
VOID kbfill();
struct dhdr FAR *finddev();
#endif

/*      Return a pointer to the first driver in the chain that 
 *      matches the attributes. 
 */

struct dhdr FAR *finddev(UWORD attr_mask)
{
	struct dhdr far *dh;
	
	for (dh = nul_dev.dh_next; FP_OFF(dh) != 0xFFFF; dh = dh -> dh_next)
	{
	     if (dh -> dh_attr & attr_mask)
		 return dh;
	}

	/* return dev/null if no matching driver found */
	return &nul_dev;
}


BYTE *CharName(struct dhdr far *lpDhdr)
{
	struct dhdr far *dh;
	static BYTE szName[9];
	COUNT nIdx;

	/* Scan through the device list */
	for (dh = nul_dev.dh_next; FP_OFF(dh) != 0xFFFF; dh = dh -> dh_next)
	{
		if (dh == lpDhdr)
			break;
	}

	/* return name of /dev/null if no matching driver found */
	if(FP_OFF(dh) == 0xFFFF)
		dh = (struct dhdr far *)&nul_dev;

	/* Now convert it to a C string */
	for(nIdx = 0; nIdx < 8 && dh -> dh_name[nIdx] != ' '; nIdx++)
		szName[nIdx] = dh -> dh_name[nIdx];
	szName[nIdx] = '\0';
	return szName;
}

static BOOL 
_sto (COUNT c)
{
	BYTE buf = c;
	struct dhdr FAR *lpDevice;

	if(con_break())
	{
		handle_break();
		return FALSE;
	}
	CharReqHdr.r_length = sizeof(request);
	CharReqHdr.r_command = C_OUTPUT;
	CharReqHdr.r_count = 1;
	CharReqHdr.r_trans = (BYTE FAR *)(&buf);
	CharReqHdr.r_status = 0;
	execrh((request FAR *)&CharReqHdr,
		lpDevice = (struct dhdr FAR *)finddev(ATTR_STDOUT));
	if(CharReqHdr.r_status & S_ERROR)
		return char_error(&CharReqHdr, lpDevice);
	return TRUE;
}


VOID 
sto (COUNT c)
{
	/* Test for hold char                                   */
	con_hold();
	
	/* Display a printable character                        */
	if(c != HT)
		_sto(c);
	if(c == CR)
		scr_pos = 0;
	else if(c == BS)
	{
		if(scr_pos > 0)
			--scr_pos;
	}
	else if(c == HT)
	{
		do
			_sto(' ');
		while(++scr_pos & 7);
	}
	else if(c != LF && c != BS)
		++scr_pos;
}


VOID 
mod_sto (REG UCOUNT c)
{
	if(c < ' ' && c != HT)
	{
		sto('^');
		sto(c + '@');
	}
	else
		sto(c);
}


VOID 
destr_bs (void)
{
	sto(BS);
	sto(' ');
	sto(BS);
}


UCOUNT 
_sti (void)
{
	UBYTE cb;
	struct dhdr FAR *lpDevice;

	CharReqHdr.r_length = sizeof(request);
	CharReqHdr.r_command = C_INPUT;
	CharReqHdr.r_count = 1;
	CharReqHdr.r_trans = (BYTE FAR *)&cb;
	CharReqHdr.r_status = 0;
	execrh((request FAR *)&CharReqHdr,
		lpDevice = (struct dhdr FAR *)finddev(ATTR_STDIN));
	if(CharReqHdr.r_status & S_ERROR)
		return char_error(&CharReqHdr, lpDevice);
	if(cb == CTL_C)
	{
		handle_break();
		return CTL_C;
	}
	else
		return cb;
}

VOID 
con_hold (void)
{
	CharReqHdr.r_unit = 0;
	CharReqHdr.r_status = 0;
	CharReqHdr.r_command = C_NDREAD;
	CharReqHdr.r_length = sizeof(request);
	execrh((request FAR *)&CharReqHdr, (struct dhdr FAR *)finddev(ATTR_STDIN));
	if(CharReqHdr.r_status & S_BUSY)
		return;
	if(CharReqHdr.r_ndbyte == CTL_S)
	{
		_sti();
		while(_sti() != CTL_Q)
			/* just wait */;
	}
}


BOOL 
con_break (void)
{
	CharReqHdr.r_unit = 0;
	CharReqHdr.r_status = 0;
	CharReqHdr.r_command = C_NDREAD;
	CharReqHdr.r_length = sizeof(request);
	execrh((request FAR *)&CharReqHdr, (struct dhdr FAR *)finddev(ATTR_STDIN));
	if(CharReqHdr.r_status & S_BUSY)
		return FALSE;
	if(CharReqHdr.r_ndbyte == CTL_C)
	{
		_sti();
		return TRUE;
	}
	else
		return FALSE;
}


BOOL 
KbdBusy (void)
{
	CharReqHdr.r_unit = 0;
	CharReqHdr.r_status = 0;
	CharReqHdr.r_command = C_ISTAT;
	CharReqHdr.r_length = sizeof(request);
	execrh((request FAR *)&CharReqHdr, (struct dhdr FAR *)finddev(ATTR_STDIN));
	if(CharReqHdr.r_status & S_BUSY)
		return TRUE;
	else
		return FALSE;
}

VOID 
KbdFlush (void)
{
	CharReqHdr.r_unit = 0;
	CharReqHdr.r_status = 0;
	CharReqHdr.r_command = C_IFLUSH;
	CharReqHdr.r_length = sizeof(request);
	execrh((request FAR *)&CharReqHdr, (struct dhdr FAR *)finddev(ATTR_STDIN));
}


static VOID kbfill(kp, c, ctlf)
keyboard FAR *kp;
UCOUNT c;
BOOL ctlf;
{
	if(kp -> kb_count > kp -> kb_size)
	{
		sto(BELL);
		return;
	}
	kp -> kb_buf[kp -> kb_count++] = c;
	if(!ctlf)
		mod_sto(c);
	else
		sto(c);
}


VOID sti(kp)
keyboard FAR *kp;
{
	REG UWORD c, cu_pos = scr_pos;
	WORD init_count = kp -> kb_count;
#ifndef NOSPCL
	static BYTE local_buffer[LINESIZE];
#endif

	if(kp -> kb_size == 0)
		return;
	if(kp -> kb_size <= kp -> kb_count || kp -> kb_buf[kp -> kb_count] != CR)
		kp -> kb_count = 0;
	FOREVER
	{
		switch(c = _sti())
		{
		case CTL_F:
			continue;

#ifndef NOSPCL
		case SPCL:
			switch(c = _sti())
			{
			case LEFT:
				goto backspace;

			case F3:
			{
				REG COUNT i;

				for(i = kp -> kb_count; local_buffer[i] != '\0'; i++)
				{
					c = local_buffer[kp -> kb_count];
					if(c == '\r' || c == '\n')
						break;
					kbfill(kp, c, FALSE);
				}
				break;
			}

			case RIGHT:
				c = local_buffer[kp -> kb_count];
				if(c == '\r' || c == '\n')
					break;
				kbfill(kp, c, FALSE);
				break;
			}
			break;
#endif

		case CTL_BS:
		case BS:
		backspace:
			if(kp -> kb_count > 0)
			{
				if(kp -> kb_buf[kp -> kb_count - 1] >= ' ')
				{
					destr_bs();
				}
				else if((kp -> kb_buf[kp -> kb_count - 1] < ' ')
				 && (kp -> kb_buf[kp -> kb_count - 1] != HT))
				{
					destr_bs();
					destr_bs();
				}
				else if(kp -> kb_buf[kp -> kb_count - 1] == HT)
				{
					do
					{
						destr_bs();
					}
					while((scr_pos > cu_pos) && (scr_pos & 7));
				}
				--kp -> kb_count;
			}
			break;

		case CR:
			kbfill(kp, CR, TRUE);
			kbfill(kp, LF, TRUE);
#ifndef NOSPCL
			fbcopy((BYTE FAR *)kp -> kb_buf,
			 (BYTE FAR *)local_buffer, (COUNT)kp -> kb_count);
			local_buffer[kp -> kb_count] = '\0';
#endif
			return;

		case LF:
			sto(CR);
			sto(LF);
			break;

		case ESC:
			sto('\\');
			sto(CR);
			sto(LF);
			for(c = 0; c < cu_pos; c++)
				sto(' ');
			kp -> kb_count = init_count;
			break;

		default:
			kbfill(kp, c, FALSE);
			break;
		}
	}
}

