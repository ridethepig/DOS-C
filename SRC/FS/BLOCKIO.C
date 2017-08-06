/****************************************************************/
/*								*/
/*			    blockio.c				*/
/*			      DOS-C				*/
/*								*/
/*	Block cache functions and device driver interface	*/
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
/*								*/
/****************************************************************/


#include "../../hdr/portab.h"
#include "globals.h"

/* $Logfile:   C:/usr/patv/dos-c/src/fs/blockio.c_v  $ */
#ifdef VERSION_STRINGS
static BYTE *blockioRcsId = "$Header:   C:/usr/patv/dos-c/src/fs/blockio.c_v   1.8   06 Dec 1998  8:43:16   patv  $";
#endif

/* $Log:   C:/usr/patv/dos-c/src/fs/blockio.c_v  $
 * 
 *    Rev 1.8   06 Dec 1998  8:43:16   patv
 * Changes in block I/O because of new I/O subsystem.
 * 
 *    Rev 1.7   22 Jan 1998  4:09:00   patv
 * Fixed pointer problems affecting SDA
 * 
 *    Rev 1.6   04 Jan 1998 23:14:36   patv
 * Changed Log for strip utility
 * 
 *    Rev 1.5   03 Jan 1998  8:36:02   patv
 * Converted data area to SDA format
 * 
 *    Rev 1.4   16 Jan 1997 12:46:34   patv
 * pre-Release 0.92 feature additions
 * 
 *    Rev 1.3   29 May 1996 21:15:10   patv
 * bug fixes for v0.91a
 * 
 *    Rev 1.2   01 Sep 1995 17:48:46   patv
 * First GPL release.
 * 
 *    Rev 1.1   30 Jul 1995 20:50:28   patv
 * Eliminated version strings in ipl
 * 
 *    Rev 1.0   02 Jul 1995  8:04:06   patv
 * Initial revision.
 */
/* $EndLog$ */


/************************************************************************/
/*									*/
/*			block cache routines				*/
/*									*/
/************************************************************************/


#ifdef I86
static BOOL Is64kBoundry(VOID FAR *lpPtr);


/*									*/
/* Test if buffer will stradle a 64k boundry.  Buffer is assumed to be	*/
/* BUFFERSIZE long.							*/
/*									*/
static BOOL Is64kBoundary(VOID FAR *lpPtr)
{
	ULONG lTemp;

	/* First, convert the segmented pointer to linear address	*/
	lTemp = FP_SEG(lpPtr);
	lTemp = (lTemp << 4) + FP_OFF(lpPtr);

	/* Now, clear out the top of the linear address above the 64K	*/
	/* boundary and add BUFFERSIZE to it.  If we get a carry above	*/
	/* the 64k boundary, then return TRUE.				*/
	lTemp &= 0x0000ffffl;
	lTemp += BUFFERSIZE;
	return (lTemp & 0xffff0000l) != 0 ? TRUE : FALSE;
}
#endif

/*									*/
/* Initialize the buffer structure					*/
/*									*/
VOID 
init_buffers (void)
{
	REG WORD i;

	for (i = 0; i < Config.cfgBuffers; ++i)
	{
		buffers[i].b_unit = 0;
		buffers[i].b_flag = 0;
		buffers[i].b_blkno = 0;
		buffers[i].b_copies = 0;
		buffers[i].b_offset = 0;
		if (i < (Config.cfgBuffers - 1))
			buffers[i].b_next = &buffers[i+1];
		else
			buffers[i].b_next = NULL;
	}
	firstbuf = &buffers[0];
	lastbuf  = &buffers[Config.cfgBuffers-1];
#ifdef I86
	/* Eliminate any buffers that straddle 64K boundaries because	*/
	/* DMA can't handle them.					*/

	/* First, test that the first buffer that is safe.		*/
	/* There is an underlying assumption here that BUFFERSIZE is	*/
	/* not 64k.  If it is, this code will fail but then again there	*/
	/* is no way to make it work because it means that every buffer	*/
        /* is on a 64k boundary!					*/
	i = 0;
	if(Is64kBoundary(firstbuf -> b_buffer))
	{
		firstbuf = &buffers[++i];
	}

	/* Now start from the (presumably) buffer[1] and link out any	*/
	/* buffer that does straddle a boundary.			*/
	/* Note that i is either 0 or 1 based on the above test.	*/
	/* In any event, if i == 0 the Is64kBoundary in this loop will	*/
	/* not be true because of the above test so there's no problem	*/
	/* with referencing buffers[i - 1].				*/
	for (; i < Config.cfgBuffers; ++i)
	{
		if(Is64kBoundary(buffers[i].b_buffer))
		{
			buffers[i - 1].b_next = buffers[i].b_next;
			++i;
		}
	}
#endif
}
 

/*									*/
/*	Return the address of a buffer structure containing the		*/
/*	requested block.						*/
/*									*/
/*	returns:							*/
/*		requested block with data				*/
/*	failure:							*/
/*		returns NULL						*/
/*									*/
struct buffer FAR *
getblock (LONG blkno, COUNT dsk)
{
	REG struct buffer FAR *bp;
	REG struct buffer FAR *lbp;
	REG struct buffer FAR *mbp;
	REG WORD imsave;

	/* Search through buffers to see if the required block	*/
	/* is already in a buffer				*/

	bp = firstbuf;
	lbp = NULL;
	mbp = NULL;
	while(bp != NULL)
	{
		if ((bp -> b_flag & BFR_VALID) && (bp -> b_unit == dsk) 
		     && (bp -> b_blkno == blkno) )
		{
			/* found it -- rearrange LRU links	*/
			if(lbp != NULL)
			{
				lbp -> b_next = bp -> b_next;
				bp -> b_next  = firstbuf;
				firstbuf = bp;
			}
			return(bp);
		}
		else
		{
			mbp = lbp;	/* move along to next buffer */
			lbp = bp;
			bp  = bp -> b_next;
		}
	}
	/* The block we need is not in a buffer, we must make a buffer	*/
	/* available, and fill it with the desired block		*/

	/* detach lru buffer						*/
	if(mbp != NULL)
		mbp -> b_next = NULL;
	lbp -> b_next = firstbuf;
	firstbuf = lbp;
	if(flush1(lbp) && fill(lbp, blkno, dsk)) /* success		*/
		mbp = lbp;
	else
		mbp = NULL;			/* failure		*/
	return (mbp);
}


BOOL
getbuf (struct buffer FAR **pbp, LONG blkno, COUNT dsk)
{
	REG struct buffer FAR *bp;
	REG struct buffer FAR *lbp;
	REG struct buffer FAR *mbp;
	REG WORD imsave;

	/* Search through buffers to see if the required block	*/
	/* is already in a buffer				*/

	bp = firstbuf;
	lbp = NULL;
	mbp = NULL;
	while(bp != NULL)
	{
		if ((bp -> b_flag & BFR_VALID) && (bp -> b_unit == dsk) 
		     && (bp -> b_blkno == blkno) )
		{
			/* found it -- rearrange LRU links	*/
			if(lbp != NULL)
			{
				lbp -> b_next = bp -> b_next;
				bp -> b_next  = firstbuf;
				firstbuf = bp;
			}
			*pbp = bp;
			return TRUE;
		}
		else
		{
			mbp = lbp;	/* move along to next buffer */
			lbp = bp;
			bp  = bp -> b_next;
		}
	}
	/* The block we need is not in a buffer, we must make a buffer	*/
	/* available, and fill it with the desired block		*/

	/* detach lru buffer						*/
	if(mbp != NULL)
		mbp -> b_next = NULL;
	lbp -> b_next = firstbuf;
	firstbuf = lbp;
	if(flush1(lbp))				/* success		*/
	{
		*pbp = lbp;
		return TRUE;
	}
	else					/* failure		*/
	{
		*pbp = NULL;
		return FALSE;
	}
}


/*									*/
/*	Mark all buffers for a disk as not valid			*/
/*									*/
VOID 
setinvld (REG COUNT dsk)
{
	REG struct buffer FAR *bp;

	bp = firstbuf;
	while (bp)
	{
		if(bp -> b_unit == dsk)
			bp -> b_flag = 0;
		bp = bp -> b_next;
	}
}


/*									*/
/*			Flush all buffers for a disk			*/
/*									*/
/*	returns:							*/
/*		TRUE on success						*/
/*									*/
BOOL 
flush_buffers (REG COUNT dsk)
{
	REG struct buffer FAR *bp;
	REG BOOL ok = TRUE;

	bp = firstbuf;
	while (bp)
	{
		if(bp -> b_unit == dsk)
			if(!flush1(bp))
				ok = FALSE;
		bp = bp -> b_next;
	}
	return ok;
}


/*									*/
/*	Write one disk buffer						*/
/*									*/
BOOL 
flush1 (struct buffer FAR *bp)
{
	REG WORD ok;

	if ((bp -> b_flag & BFR_VALID) && (bp -> b_flag & BFR_DIRTY))
	{
		ok = dskxfer(bp -> b_unit, bp -> b_blkno,
			 (VOID FAR *)bp -> b_buffer, DSKWRITE);
		if(bp -> b_flag & BFR_FAT)
		{
			int i = bp -> b_copies - 1;

			do
				ok &= dskxfer(bp -> b_unit,
				 bp -> b_blkno + i * bp -> b_offset,
				 (VOID FAR *)bp -> b_buffer, DSKWRITE);
			while (--i > 0);
		}
	}
	else
		ok = TRUE;
	bp -> b_flag &= ~BFR_DIRTY;	/* even if error, mark not dirty */
	if(!ok)				/* otherwise system has trouble  */
		bp -> b_flag &= ~BFR_VALID;	/* continuing.		 */
	return(ok);
}


/*									*/
/*	Write all disk buffers						*/
/*									*/
BOOL 
flush (void)
{
	REG struct buffer FAR *bp;
	REG BOOL ok;

	ok = TRUE;
	bp = firstbuf;
	while(bp)
	{
		if (!flush1(bp))
			ok = FALSE;
		bp -> b_flag &= ~BFR_VALID;
		bp = bp -> b_next;
	}
	return(ok);
}


/*									*/
/*	Fill the indicated disk buffer with the current track and sector */
/*									*/
BOOL 
fill (REG struct buffer FAR *bp, LONG blkno, COUNT dsk)
{
	REG WORD ok;

	if((bp -> b_flag & BFR_VALID) && (bp -> b_flag & BFR_DIRTY))
		ok = flush1(bp);
	else
		ok = TRUE;

	if(ok && (bp -> b_flag & BFR_VALID) && (blkno == bp -> b_blkno))
		return OK;

	if(ok)
		ok = dskxfer(dsk, blkno, (VOID FAR *)bp -> b_buffer, DSKREAD);
	bp -> b_flag = BFR_VALID | BFR_DATA;
	bp -> b_blkno = blkno;
	bp -> b_unit = dsk;
	return(ok);
}


/************************************************************************/
/*									*/
/*		Device Driver Interface Functions			*/
/*									*/
/************************************************************************/

/*									*/
/* Transfer a block to/from disk					*/
/*									*/
BOOL 
dskxfer (COUNT dsk, LONG blkno, VOID FAR *buf, COUNT mode)
{
	REG struct dpb *dpbp = &blk_devices[dsk];

	for(;;)
	{
		IoReqHdr.r_length = sizeof(request);
		IoReqHdr.r_unit = dpbp -> dpb_subunit;
		IoReqHdr.r_command =
		 mode == DSKWRITE ?
		  (verify_ena ? C_OUTVFY : C_OUTPUT)
		  : C_INPUT;
		IoReqHdr.r_status = 0;
		IoReqHdr.r_meddesc = dpbp -> dpb_mdb;
		IoReqHdr.r_trans = (BYTE FAR *)buf;
		IoReqHdr.r_count = 1;
		if(blkno > MAXSHORT)
		{
			IoReqHdr.r_start = HUGECOUNT;
			IoReqHdr.r_huge = blkno - 1;
		}
		else
			IoReqHdr.r_start = blkno - 1;
		execrh((request FAR *)&IoReqHdr, dpbp -> dpb_device);
		if(!(IoReqHdr.r_status & S_ERROR) && (IoReqHdr.r_status & S_DONE))
			break;
		else
		{
		loop:
			switch(block_error(&IoReqHdr,
				dpbp -> dpb_unit, dpbp -> dpb_device))
			{
			case ABORT:
			case FAIL:
				return FALSE;

			case RETRY:
				continue;

			case CONTINUE:
				break;

			default:
				goto loop;
			}
		}
	}
	return TRUE;
}


