/****************************************************************/
/*                                                              */
/*                          fatdir.c                            */
/*                            DOS-C                             */
/*                                                              */
/*                 FAT File System dir Functions                */
/*                                                              */
/*                      Copyright (c) 1995                      */
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

/* $Logfile:   C:/usr/patv/dos-c/src/fs/fatdir.c_v  $ */
#ifdef VERSION_STRINGS
static BYTE *fatdirRcsId = "$Header:   C:/usr/patv/dos-c/src/fs/fatdir.c_v   1.10   06 Dec 1998  8:44:36   patv  $";
#endif

/* $Log:   C:/usr/patv/dos-c/src/fs/fatdir.c_v  $
 * 
 *    Rev 1.10   06 Dec 1998  8:44:36   patv
 * Bug fixes.
 * 
 *    Rev 1.9   22 Jan 1998  4:09:00   patv
 * Fixed pointer problems affecting SDA
 * 
 *    Rev 1.8   04 Jan 1998 23:14:36   patv
 * Changed Log for strip utility
 * 
 *    Rev 1.7   03 Jan 1998  8:36:02   patv
 * Converted data area to SDA format
 * 
 *    Rev 1.6   16 Jan 1997 12:46:30   patv
 * pre-Release 0.92 feature additions
 * 
 *    Rev 1.5   29 May 1996 21:15:18   patv
 * bug fixes for v0.91a
 * 
 *    Rev 1.4   19 Feb 1996  3:20:12   patv
 * Added NLS, int2f and config.sys processing
 * 
 *    Rev 1.2   01 Sep 1995 17:48:38   patv
 * First GPL release.
 * 
 *    Rev 1.1   30 Jul 1995 20:50:24   patv
 * Eliminated version strings in ipl
 * 
 *    Rev 1.0   02 Jul 1995  8:04:34   patv
 * Initial revision.
 */
/* $EndLog$ */

VOID pop_dmp(dmatch FAR *, struct f_node FAR *);

struct f_node FAR *
dir_open (BYTE FAR *dirname)
{
	struct f_node FAR *fnp;
	COUNT drive;
	BYTE *p;
	WORD i;
	BYTE *s, *pszPath = &TempCDS.cdsCurrentPath[2];

	/* Allocate an fnode if possible - error return (0) if not.     */
	if((fnp = get_f_node()) == (struct f_node FAR *)0)
	{
		return (struct f_node FAR *)NULL;
	}
	
	/* Force the fnode into read-write mode                         */
	fnp -> f_mode = RDWR;

	/* and initialize temporary CDS					*/
	TempCDS.cdsFlags = 0;

	/* determine what drive we are using...                         */
	dirname = adjust_far(dirname);
	ParseDosName(dirname, &drive, (BYTE *)0, (BYTE *)0, (BYTE *)0);

	/* If the drive was specified, drive is non-negative and	*/
	/* corresponds to the one passed in, i.e., 0 = A, 1 = B, etc.	*/
	/* We use that and skip the "D:" part of the string.		*/
	/* Otherwise, just use the default drive			*/ 
	if(drive >= 0)
	{
		dirname += 2;		/* Assume FAT style drive	*/
		TempCDS.cdsDpb = &blk_devices[drive];
	}
	else
	{
		TempCDS.cdsDpb = &blk_devices[drive = default_drive];
	}
	fnp -> f_dpb = (struct dpb *)TempCDS.cdsDpb;
	TempCDS.cdsCurrentPath[0] = 'A' + drive;
	TempCDS.cdsCurrentPath[1] = ':';
	TempCDS.cdsJoinOffset = 2;
	
	if (drive >= nblkdev)
	{
		release_f_node(fnp);
		return NULL;
	}

	/* Generate full path name                                      */
	ParseDosPath(dirname,
	 (COUNT *)0, pszPath, (BYTE *)TempCDS.cdsDpb -> dpb_path);
	
	/* Perform all directory common handling after all special	*/
	/* handling has been performed.					*/
	++TempCDS.cdsDpb -> dpb_count;
	
	if(media_check((struct dpb *)TempCDS.cdsDpb) < 0)
	{
		--TempCDS.cdsDpb -> dpb_count;
		release_f_node(fnp);
		return (struct f_node FAR *)0;
	}
	fnp -> f_diroff = 0l;
	fnp -> f_flags.f_dmod = FALSE;  /* a brand new fnode		*/
	fnp -> f_flags.f_dnew = TRUE;
	fnp -> f_dsize = DIRENT_SIZE * TempCDS.cdsDpb -> dpb_dirents;
	

	/* Walk the directory tree to find the starting cluster		*/
	/*								*/
	/* Set the root flags since we always start from the root	*/

	fnp -> f_flags.f_droot = TRUE;
	for(p = pszPath; *p != '\0'; )
	{
		/* skip all path seperators				*/
		while(*p == '\\')
			++p;
		/* don't continue if we're at the end			*/
		if(*p == '\0')
			break;

		/* Convert the name into an absolute name for		*/
		/* comparison...					*/
		/* first the file name with trailing spaces...		*/
		for(i = 0; i < FNAME_SIZE; i++)
		{
			if(*p != '\0' && *p != '.' && *p != '/' && *p != '\\')
				TempBuffer[i] = *p++;
			else
				break;
		}

		for( ; i < FNAME_SIZE; i++)
			TempBuffer[i] = ' ';

		/* and the extension (don't forget to   */
		/* add trailing spaces)...              */
		if(*p == '.')
			++p;
		for(i = 0; i < FEXT_SIZE; i++)
		{
			if(*p != '\0' && *p != '.' && *p != '/' && *p != '\\')
				TempBuffer[i+FNAME_SIZE] = *p++;
			else
				break;
		}
		for( ; i < FEXT_SIZE; i++)
			TempBuffer[i+FNAME_SIZE] = ' ';

		/* Now search through the directory to  */
		/* find the entry...                    */
		i = FALSE;

		upMem((BYTE FAR *)TempBuffer, FNAME_SIZE+FEXT_SIZE);

		while(dir_read(fnp) == DIRENT_SIZE)
		{
			if(fnp -> f_dir.dir_name[0] != '\0' && fnp -> f_dir.dir_name[0] != DELETED)
			{
				if(fcmp((BYTE FAR *)TempBuffer, (BYTE FAR *)fnp -> f_dir.dir_name, FNAME_SIZE+FEXT_SIZE))
				{
					i = TRUE;
					break;
				}
			}
		}

		if(!i || !(fnp -> f_dir.dir_attrib & D_DIR))
		{
			--TempCDS.cdsDpb -> dpb_count;
			release_f_node(fnp);
			return (struct f_node FAR *)0;
		}
		else
		{
			/* make certain we've moved off */
			/* root                         */
			fnp -> f_flags.f_droot = FALSE;
			fnp -> f_flags.f_ddir = TRUE;
			/* set up for file read/write   */
			fnp -> f_offset = 0l;
			fnp -> f_highwater = 0l;
			fnp -> f_cluster = fnp -> f_dir.dir_start;
			fnp -> f_dirstart = fnp -> f_dir.dir_start;
			/* reset the directory flags    */
			fnp -> f_diroff = 0l;
			fnp -> f_flags.f_dmod = FALSE;
			fnp -> f_flags.f_dnew = TRUE;
			fnp -> f_dsize = DIRENT_SIZE * TempCDS.cdsDpb -> dpb_dirents;
		}
	}

	return fnp;
}


COUNT 
dir_read (REG struct f_node FAR *fnp)
{
	REG i, j;
	struct buffer FAR *bp;

	/* Directories need to point to their current offset, not for   */
	/* next op. Therefore, if it is anything other than the first   */
	/* directory entry, we will update the offset on entry rather   */
	/* than wait until exit. If it was new, clear the special new   */
	/* flag.                                                        */
	if(fnp -> f_flags.f_dnew)
		fnp -> f_flags.f_dnew = FALSE;
	else
		fnp -> f_diroff += DIRENT_SIZE;

	/* Determine if we hit the end of the directory. If we have,    */
	/* bump the offset back to the end and exit. If not, fill the   */
	/* dirent portion of the fnode, clear the f_dmod bit and leave, */
	/* but only for root directories                                */
	if(!(fnp -> f_flags.f_droot)
	  && fnp -> f_diroff >= fnp -> f_dsize)
	{
		fnp -> f_diroff -= DIRENT_SIZE;
		return 0;
	}
	else
	{
		if(fnp -> f_flags.f_droot)
		{
			if((fnp -> f_diroff / fnp -> f_dpb -> dpb_secsize
			  + fnp -> f_dpb -> dpb_dirstrt)
			  >= fnp -> f_dpb -> dpb_data)
			{
				fnp -> f_flags.f_dfull = TRUE;
				return 0;
			}
			
			bp = getblock((LONG)(fnp -> f_diroff / fnp -> f_dpb -> dpb_secsize
				+ fnp -> f_dpb -> dpb_dirstrt),
				fnp -> f_dpb -> dpb_unit);
			bp -> b_flag &= ~(BFR_DATA | BFR_FAT);
			bp -> b_flag |= BFR_DIR;

		}
		else
		{
			REG UWORD secsize = fnp -> f_dpb -> dpb_secsize;

			/* Do a "seek" to the directory position        */
			fnp -> f_offset = fnp -> f_diroff;
			
			/* Search through the FAT to find the block     */
			/* that this entry is in.                       */
			if(map_cluster(fnp, XFR_READ) != SUCCESS)
			{
				fnp -> f_flags.f_dfull = TRUE;
				return 0;
			}
			
			/* If the returned cluster is FREE, return zero */
			/* bytes read.                                  */
			if(fnp -> f_cluster == FREE)
				return 0;

			/* If the returned cluster is LAST_CLUSTER or   */
			/* LONG_LAST_CLUSTER, return zero bytes read    */
			/* and set the directory as full.               */

			if(last_link(fnp))
			{
				fnp -> f_diroff -= DIRENT_SIZE;
				fnp -> f_flags.f_dfull = TRUE;
				return 0;
			}

			/* Compute the block within the cluster and the */
			/* offset within the block.                     */
			fnp -> f_sector =
			 (fnp -> f_offset / secsize)
			  & fnp -> f_dpb -> dpb_clsmask;
			fnp -> f_boff = fnp -> f_offset % secsize;

			/* Get the block we need from cache             */
			bp = getblock(
				(LONG)clus2phys(fnp -> f_cluster,
					fnp -> f_dpb -> dpb_clssize,
					fnp -> f_dpb -> dpb_data)
					 + fnp -> f_sector,
				fnp -> f_dpb -> dpb_unit);
			bp -> b_flag &= ~(BFR_DATA | BFR_FAT);
			bp -> b_flag |= BFR_DIR;
		}

		/* Now that we have the block for our entry, get the    */
		/* directory entry.                                     */
		if(bp != NULL)
			getdirent((BYTE FAR *)&bp -> b_buffer[fnp -> f_diroff % fnp -> f_dpb -> dpb_secsize],
				(struct dirent FAR *)&fnp -> f_dir);
		else
		{
			fnp -> f_flags.f_dfull = TRUE;
			return 0;
		}
				
		/* Update the fnode's directory info                    */
		fnp -> f_flags.f_dfull = FALSE;
		fnp -> f_flags.f_dmod = FALSE;

		/* and for efficiency, stop when we hit the first       */
		/* unused entry.                                        */
		if(fnp -> f_dir.dir_name[0] == '\0')
			return 0;
		else
			return DIRENT_SIZE;
	}
}


#ifndef IPL
COUNT 
dir_write (REG struct f_node FAR *fnp)
{
	struct buffer FAR *bp;

	/* Update the entry if it was modified by a write or create...  */
	if(fnp -> f_flags.f_dmod)
	{

		/* Root is a consecutive set of blocks, so handling is  */
		/* simple.                                              */
		if(fnp -> f_flags.f_droot)
		{
			bp =
			 getblock(
			  (LONG)(fnp -> f_diroff / fnp -> f_dpb -> dpb_secsize
			   + fnp -> f_dpb -> dpb_dirstrt),
			    fnp -> f_dpb -> dpb_unit);
			bp -> b_flag &= ~(BFR_DATA | BFR_FAT);
			bp -> b_flag |= BFR_DIR;
		}

		/* All other directories are just files. The only       */
		/* special handling is resetting the offset so that we  */
		/* can continually update the same directory entry.     */
		else
		{
			REG UWORD secsize = fnp -> f_dpb -> dpb_secsize;

			/* Do a "seek" to the directory position        */
			/* and convert the fnode to a directory fnode.  */
			fnp -> f_offset = fnp -> f_diroff;
			fnp -> f_back = LONG_LAST_CLUSTER;
			fnp -> f_cluster = fnp -> f_dirstart;

			/* Search through the FAT to find the block     */
			/* that this entry is in.                       */
			if(map_cluster(fnp, XFR_READ) != SUCCESS)
			{
				fnp -> f_flags.f_dfull = TRUE;
				release_f_node(fnp);
				return 0;
			}

			/* If the returned cluster is FREE, return zero */
			/* bytes read.                                  */
			if(fnp -> f_cluster == FREE)
			{
				release_f_node(fnp);
				return 0;
			}

			/* Compute the block within the cluster and the */
			/* offset within the block.                     */
			fnp -> f_sector =
			 (fnp -> f_offset / secsize)
			  & fnp -> f_dpb -> dpb_clsmask;
			fnp -> f_boff = fnp -> f_offset % secsize;


			/* Get the block we need from cache             */
			bp = getblock(
				(LONG)clus2phys(fnp -> f_cluster,
					fnp -> f_dpb -> dpb_clssize,
					fnp -> f_dpb -> dpb_data)
					 + fnp -> f_sector,
				fnp -> f_dpb -> dpb_unit);
			bp -> b_flag &= ~(BFR_DATA | BFR_FAT);
			bp -> b_flag |= BFR_DIR;
		}

		/* Now that we have a block, transfer the diectory      */
		/* entry into the block.                                */
		if(bp == NULL)
		{
			release_f_node(fnp);
			return 0;
		}
		putdirent((struct dirent FAR *)&fnp -> f_dir,
		 (VOID FAR *)&bp -> b_buffer[fnp -> f_diroff % fnp -> f_dpb -> dpb_secsize]);
		bp -> b_flag |= BFR_DIRTY;
	}
	return DIRENT_SIZE;
}
#endif


VOID 
dir_close (REG struct f_node FAR *fnp)
{
	REG COUNT disk = fnp -> f_dpb -> dpb_unit;

	/* Test for invalid f_nodes                                     */
	if(fnp == NULL)
		return;

#ifndef IPL
	/* Write out the entry                                          */
	dir_write(fnp);

#endif 
	/* Clear buffers after release                                  */
	flush_buffers(disk);
	setinvld(disk);

	/* and release this instance of the fnode                       */
	--(fnp -> f_dpb) -> dpb_count;
	release_f_node(fnp);
}


#ifndef IPL
COUNT 
dos_findfirst (UCOUNT attr, BYTE FAR *name)
{
	REG struct f_node FAR *fnp;
	REG dmatch FAR *dmp = (dmatch FAR *)dta;
	REG COUNT i;
	COUNT nDrive;
	BYTE *p;

	/* The findfirst/findnext calls are probably the worst of the   */
	/* DOS calls. They must work somewhat on the fly (i.e. - open   */
	/* but never close). Since we don't want to lose fnodes every   */
	/* time a directory is searched, we will initialize the DOS     */
	/* dirmatch structure and then for every find, we will open the */
	/* current directory, do a seek and read, then close the fnode. */
	   
	/* Start out by initializing the dirmatch structure.            */
	dmp -> dm_drive = default_drive;
	dmp -> dm_entry = 0;
	dmp -> dm_cluster = 0;

	dmp -> dm_attr_srch = attr;

	/* Parse out the drive, file name and file extension.		*/
	ParseDosName(name, &nDrive, &LocalPath[2],
		SearchDir.dir_name, SearchDir.dir_ext);

	if(nDrive >= 0)
	{
		dmp -> dm_drive = nDrive;
	}
	else
		nDrive = default_drive;

	/* Now build a directory.					*/
	if(!LocalPath[2])
		strcpy(&LocalPath[2], ".");

	/* Build the match pattern out of the passed string             */
	for(p = SearchDir.dir_name, i = 0; i < FNAME_SIZE; i++)
	{
		/* test for a valid file name terminator                */
		if(*p != '\0')
		{
			/* If not a wildcard ('*'), just transfer       */
			if(*p != '*')
				dmp -> dm_name_pat[i] = *p++;
			else
			{
				/* swallow the wildcard                 */
				++p;

				/* fill with character wildcard (?)     */
				for( ; i < FNAME_SIZE; i++)
					dmp -> dm_name_pat[i] = '?';
				/* and skip to seperator                */
				while(*p != '\0'
				 && *p != '.' && *p != '/' && *p != '\\')
					++p;

				break;
			}
		}
		else
			break;
	}
	for( ; i < FNAME_SIZE; i++)
		dmp -> dm_name_pat[i] = ' ';

	/* and the extension (don't forget to add trailing spaces)...   */
	i = 0;
	for(p = SearchDir.dir_ext; i < FEXT_SIZE; i++)
	{
		if(*p != '\0' && *p != '.' && *p != '/' && *p != '\\')
		{
			if(*p != '*')
				dmp -> dm_name_pat[i+FNAME_SIZE] = *p++;
			else
			{
				for( ; i < FEXT_SIZE; i++)
					dmp -> dm_name_pat[i+FNAME_SIZE] = '?';
				break;
			}
		}
		else
			break;
	}
	for( ; i < FEXT_SIZE; i++)
		dmp -> dm_name_pat[i+FNAME_SIZE] = ' ';

	/* Now search through the directory to find the entry...        */
	upMem((BYTE FAR *)dmp -> dm_name_pat, FNAME_SIZE+FEXT_SIZE);

	/* Special handling - the volume id is only in the root         */
	/* directory and only searched for once.  So we need to open    */
	/* the root and return only the first entry that contains the   */
	/* volume id bit set.                                           */
	if(attr & D_VOLID)
	{
		/* Now open this directory so that we can read the      */
		/* fnode entry and do a match on it.                    */
		if((fnp = dir_open((BYTE FAR *)"\\")) == NULL)
			return DE_PATHNOTFND;

		/* Now do the search                                    */
		while(dir_read(fnp) == DIRENT_SIZE)
		{
			/* Test the attribute and return first found    */
			if(fnp -> f_dir.dir_attrib & D_VOLID)
			{
				pop_dmp(dmp, fnp);
				dir_close(fnp);
				return SUCCESS;
			}
		}

		/* Now that we've done our failed search, close it and  */
		/* return an error.                                     */
		dir_close(fnp);
		return DE_FILENOTFND;
	}

	/* Otherwise just do a normal find next                         */
	else
	{
		/* Complete building the directory from the passed in	*/
		/* name							*/
		if(nDrive >= 0)
		{
			LocalPath[0] = 'A' + nDrive;
		}
		else
		{
			LocalPath[0] = 'A' + default_drive;
		}
		LocalPath[1] = ':';

		/* Now open this directory so that we can read the      */
		/* fnode entry and do a match on it.                    */
		if((fnp = dir_open((BYTE FAR *)LocalPath)) == NULL)
			return DE_PATHNOTFND;

		pop_dmp(dmp, fnp);
		dmp -> dm_entry = 0;
		if(!fnp -> f_flags.f_droot)
		{        
			dmp -> dm_cluster = fnp -> f_dirstart;
			dmp -> dm_dirstart = fnp -> f_dirstart;
		}
		else
		{
			dmp -> dm_cluster = 0;
			dmp -> dm_dirstart = 0;
		}
		dir_close(fnp);
		return dos_findnext();
	}
}


COUNT 
dos_findnext (void)
{
	REG dmatch FAR *dmp = (dmatch FAR *)dta;
	REG struct f_node FAR *fnp;
	BOOL found = FALSE;
	BYTE FAR *p, *q;

	/* assign our match parameters pointer.                         */
	dmp = (dmatch FAR *)dta;

	/* Allocate an fnode if possible - error return (0) if not.     */
	if((fnp = get_f_node()) == (struct f_node FAR *)0)
	{
		return DE_FILENOTFND;
	}

	/* Force the fnode into read-write mode                         */
	fnp -> f_mode = RDWR;

	/* Select the default to help non-drive specified path          */
	/* searches...                                                  */
	fnp -> f_dpb = &blk_devices[dmp -> dm_drive];
	++(fnp -> f_dpb) -> dpb_count;
	if(media_check(fnp -> f_dpb) < 0)
	{
		--(fnp -> f_dpb) -> dpb_count;
		release_f_node(fnp);
		return DE_FILENOTFND;
	}

	
	fnp -> f_dsize = DIRENT_SIZE * (fnp -> f_dpb) -> dpb_dirents;

	/* Search through the directory to find the entry, but do a     */
	/* seek first.                                                  */
	if(dmp -> dm_entry > 0)
		fnp -> f_diroff = (dmp -> dm_entry - 1) * DIRENT_SIZE;

	fnp -> f_offset = fnp -> f_highwater = fnp -> f_diroff;
	fnp -> f_cluster = dmp -> dm_cluster;
	fnp -> f_flags.f_dmod = dmp -> dm_flags.f_dmod;
	fnp -> f_flags.f_droot = dmp -> dm_flags.f_droot;
	fnp -> f_flags.f_dnew = dmp -> dm_flags.f_dnew;
	fnp -> f_flags.f_ddir = dmp -> dm_flags.f_ddir;
	fnp -> f_flags.f_dfull = dmp -> dm_flags.f_dfull;

	fnp -> f_dirstart = dmp -> dm_dirstart;
	
	/* Loop through the directory                                   */
	while(dir_read(fnp) == DIRENT_SIZE)
	{
		++dmp -> dm_entry;
		if(fnp -> f_dir.dir_name[0] != '\0' && fnp -> f_dir.dir_name[0] != DELETED)
		{
			if(fcmp_wild((BYTE FAR *)(dmp -> dm_name_pat), (BYTE FAR *)fnp -> f_dir.dir_name, FNAME_SIZE+FEXT_SIZE))
			{
				/* Test the attribute as the final step */
				if(fnp -> f_dir.dir_attrib & D_VOLID)
					continue;
				else if(!((~dmp -> dm_attr_srch & fnp -> f_dir.dir_attrib)
				 & (D_DIR | D_SYSTEM | D_HIDDEN)))
				{
					found = TRUE;
					break;
				}
				else
					continue;
			}
		}
	}

	/* If found, transfer it to the dmatch structure                */
	if(found)
		pop_dmp(dmp, fnp);

	/* return the result                                            */
	--(fnp -> f_dpb) -> dpb_count;
	release_f_node(fnp);

	return found ? SUCCESS : DE_FILENOTFND;
}

static VOID pop_dmp(dmp, fnp)
dmatch FAR *dmp;
struct f_node FAR *fnp;
{
	COUNT idx;
	BYTE FAR *p;
	BYTE FAR *q;

	dmp -> dm_attr_fnd = fnp -> f_dir.dir_attrib;
	dmp -> dm_time = fnp -> f_dir.dir_time;
	dmp -> dm_date = fnp -> f_dir.dir_date;
	dmp -> dm_size = fnp -> f_dir.dir_size;
	/* dmp -> dm_cluster = fnp -> f_cluster; */
	dmp -> dm_flags.f_droot = fnp -> f_flags.f_droot;
	dmp -> dm_flags.f_ddir = fnp -> f_flags.f_ddir;
	dmp -> dm_flags.f_dmod = fnp -> f_flags.f_dmod;
	dmp -> dm_flags.f_dnew = fnp -> f_flags.f_dnew;
	p = dmp -> dm_name;
	if(fnp -> f_dir.dir_name[0] == '.')
	{
		for(idx = 0, q = (BYTE FAR *)fnp -> f_dir.dir_name;
		 idx < FNAME_SIZE; idx++)
		{
			if(*q == ' ')
				break;
			*p++ = *q++;
		}
	}
	else
	{
		for(idx = 0, q = (BYTE FAR *)fnp -> f_dir.dir_name;
		 idx < FNAME_SIZE; idx++)
		{
			if(*q == ' ')
				break;
			*p++ = *q++;
		}
		if(fnp -> f_dir.dir_ext[0] != ' ')
		{
			*p++ = '.';
			for(idx = 0, q = (BYTE FAR *)fnp -> f_dir.dir_ext; idx < FEXT_SIZE; idx++)
			{
				if(*q == ' ')
					break;
				*p++ = *q++;
			}
		}
	}
	*p++ = NULL;
}
#endif

