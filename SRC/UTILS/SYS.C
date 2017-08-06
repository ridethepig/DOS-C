
/****************************************************************/
/*								*/
/*			      sys.c				*/
/*			      DOS-C				*/
/*								*/
/*		       sys utility for DOS-C			*/
/*								*/
/*			Copyright (c) 1991			*/
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <dos.h>
#include <ctype.h>
#include <mem.h>
#include "../../hdr/portab.h"
#include "../../hdr/device.h"

BYTE *pgm = "sys";

BOOL fl_reset(VOID);
COUNT fl_rd_status(WORD);
COUNT fl_read(WORD, WORD, WORD, WORD, WORD, BYTE FAR *);
COUNT fl_write(WORD, WORD, WORD, WORD, WORD, BYTE FAR *);
BOOL fl_verify(WORD, WORD, WORD, WORD, WORD, BYTE FAR *);
BOOL fl_format(WORD, BYTE FAR *);
VOID get_boot(COUNT);
VOID put_boot(COUNT);
BOOL check_space(COUNT, BYTE *);
COUNT ltop(WORD *, WORD *, WORD *, COUNT, COUNT, LONG, byteptr);
BOOL copy(COUNT, BYTE *);
BOOL DiskReset(COUNT);
COUNT DiskRead(WORD, WORD, WORD, WORD, WORD, BYTE FAR *);
COUNT DiskWrite(WORD, WORD, WORD, WORD, WORD, BYTE FAR *);

/*								*/
/* special word packing prototypes				*/
/*								*/
#ifdef NATIVE
# define getlong(vp, lp) (*(LONG *)(lp)=*(LONG *)(vp))
# define getword(vp, wp) (*(WORD *)(wp)=*(WORD *)(vp))
# define getbyte(vp, bp) (*(BYTE *)(bp)=*(BYTE *)(vp))
# define fgetlong(vp, lp) (*(LONG FAR *)(lp)=*(LONG FAR *)(vp))
# define fgetword(vp, wp) (*(WORD FAR *)(wp)=*(WORD FAR *)(vp))
# define fgetbyte(vp, bp) (*(BYTE FAR *)(bp)=*(BYTE FAR *)(vp))
# define fputlong(lp, vp) (*(LONG FAR *)(vp)=*(LONG FAR *)(lp))
# define fputword(wp, vp) (*(WORD FAR *)(vp)=*(WORD FAR *)(wp))
# define fputbyte(bp, vp) (*(BYTE FAR *)(vp)=*(BYTE FAR *)(bp))
#else
VOID getword(VOID *, WORD *);
VOID getbyte(VOID *, BYTE *);
VOID fgetlong(VOID FAR *, LONG FAR *);
VOID fgetword(VOID FAR *, WORD FAR *);
VOID fgetbyte(VOID FAR *, BYTE FAR *);
VOID fputlong(LONG FAR *, VOID FAR *);
VOID fputword(WORD FAR *, VOID FAR *);
VOID fputbyte(BYTE FAR *, VOID FAR *);
#endif

#define	SEC_SIZE	512
#define NDEV		4
#define COPY_SIZE	16384
#define	NRETRY		5

static struct media_info
{
	ULONG	mi_size;		/* physical sector size		*/
	UWORD	mi_heads;		/* number of heads (sides)	*/
	UWORD	mi_cyls;		/* number of cyl/drive		*/
	UWORD	mi_sectors;		/* number of sectors/cyl	*/
	ULONG	mi_offset;		/* relative partition offset	*/
};

union
{
	BYTE	bytes[2 * SEC_SIZE];
	boot	boot_sector;
} buffer;

static struct media_info miarray[NDEV] =
{
	{720l, 2, 40, 9, 0l},
	{720l, 2, 40, 9, 0l},
	{720l, 2, 40, 9, 0l},
	{720l, 2, 40, 9, 0l}
};

#define PARTOFF	0x1be
#define N_PART 4

static struct
{
	BYTE	peBootable;
	BYTE	peBeginHead;
	BYTE	peBeginSector;
	UWORD	peBeginCylinder;
	BYTE	peFileSystem;
	BYTE	peEndHead;
	BYTE	peEndSector;
	UWORD	peEndCylinder;
	LONG	peStartSector;
	LONG	peSectors;
} partition[N_PART];

static int DrvMap[4] = {0, 1, 0x80, 0x81};

COUNT drive, active;
UBYTE newboot[SEC_SIZE], oldboot[SEC_SIZE];

#define SBSIZE		51
#define SBOFFSET	11

#define SIZEOF_PARTENT	16

#define FAT12		0x01
#define	FAT16SMALL	0x04
#define	EXTENDED	0x05
#define	FAT16LARGE	0x06

#define N_RETRY		5

COUNT 
get_part (COUNT drive, COUNT idx)
{
	REG retry = N_RETRY;
	REG BYTE *p = (BYTE *)&buffer.bytes[PARTOFF + (idx * SIZEOF_PARTENT)];
	REG ret;
	BYTE packed_byte, pb1;

	do
	{
		ret = fl_read((WORD)DrvMap[drive], (WORD)0, (WORD)0, (WORD)1, (WORD)1, (byteptr)&buffer);
	} while (ret != 0 && --retry > 0);
	if(ret != 0)
		return FALSE;
	getbyte((VOID *)p, &partition[idx].peBootable);
	++p;
	getbyte((VOID *)p, &partition[idx].peBeginHead);
	++p;
	getbyte((VOID *)p, &packed_byte);
	partition[idx].peBeginSector = packed_byte & 0x3f;
	++p;
	getbyte((VOID *)p, &pb1);
	++p;
	partition[idx].peBeginCylinder = pb1 + ((0xc0 & packed_byte) << 2);
	getbyte((VOID *)p, &partition[idx].peFileSystem);
	++p;
	getbyte((VOID *)p, &partition[idx].peEndHead);
	++p;
	getbyte((VOID *)p, &packed_byte);
	partition[idx].peEndSector = packed_byte & 0x3f;
	++p;
	getbyte((VOID *)p, &pb1);
	++p;
	partition[idx].peEndCylinder = pb1 + ((0xc0 & packed_byte) << 2);
	getlong((VOID *)p, &partition[idx].peStartSector);
	p += sizeof(LONG);
	getlong((VOID *)p, &partition[idx].peSectors);
	return TRUE;
}


VOID main(argc, argv)
COUNT argc;
BYTE *argv[];
{
	if(argc != 2)
	{
		fprintf(stderr, "Useage: %s drive\n drive = A,B,etc.\n", pgm);
		exit(1);
	}

	drive = *argv[1] - (islower(*argv[1]) ? 'a' : 'A');
	if(drive < 0 || drive >= NDEV)
	{
		fprintf(stderr, "%s: drive out of range\n", pgm);
		exit(1);
	}

	if(!DiskReset(drive))
	{
		fprintf(stderr, "%s: cannot reset drive %c:",
			drive, 'A' + drive);
		exit(1);
	}

	get_boot(drive);

	if(!check_space(drive, oldboot))
	{
		fprintf(stderr, "%s: cannot transfer system files\n", pgm);
		exit(1);
	}

	put_boot(drive);

	if(!copy(drive, "ipl.sys"))
	{
		fprintf(stderr, "%s: cannot copy \"IPL.SYS\"\n", pgm);
		exit(1);
	}
	if(!copy(drive, "kernel.exe"))
	{
		fprintf(stderr, "%s: cannot copy \"KERNEL.EXE\"\n", pgm);
		exit(1);
	}
	if(!copy(drive, "boot.bin"))
	{
		fprintf(stderr, "%s: cannot copy \"BOOT.BIN\"\n", pgm);
		exit(1);
	}
	if(!copy(drive, "command.com"))
	{
		fprintf(stderr, "%s: cannot copy \"COMMAND.COM\"\n", pgm);
		exit(1);
	}
	exit(0);
}


VOID put_boot(drive)
COUNT drive;
{
	COUNT i;
	COUNT ifd;
	WORD head, track, sector, ret;
	WORD count;

	if(drive >= 2)
	{
		head = partition[active].peBeginHead;
		sector = partition[active].peBeginSector;
		track = partition[active].peBeginCylinder;
	}
	else
	{
		head = 0;
		sector = 1;
		track = 0;
	}

	if((ifd = open("boot.bin", O_RDONLY | O_BINARY)) < 0)
	{
		fprintf(stderr, "%s: can't open\"boot.bin\"\n", pgm);
		exit(1);
	}

	if(read(ifd, newboot, SEC_SIZE) < SEC_SIZE)
	{
		fprintf(stderr, "%s: error read \"boot.bin\"", pgm);
		exit(1);
	}

	close(ifd);

	if((i = DiskRead(DrvMap[drive], head, track, sector, 1, (BYTE far *)oldboot)) != 0)
	{
		fprintf(stderr, "%s: disk read error (code = 0x%02x)\n", pgm, i & 0xff);
		exit(1);
	}

	memcpy(&newboot[SBOFFSET], &oldboot[SBOFFSET], SBSIZE);

	if((i = DiskWrite(DrvMap[drive], head, track, sector, 1, (BYTE far *)newboot)) != 0)
	{
		fprintf(stderr, "%s: disk write error (code = 0x%02x)\n", pgm, i & 0xff);
		exit(1);
	}
}


VOID get_boot(drive)
COUNT drive;
{
	COUNT i;
	COUNT ifd;
	WORD head, track, sector, ret;
	WORD count;

	if(drive >= 2)
	{
		head = partition[active].peBeginHead;
		sector = partition[active].peBeginSector;
		track = partition[active].peBeginCylinder;
	}
	else
	{
		head = 0;
		sector = 1;
		track = 0;
	}

	if((i = DiskRead(DrvMap[drive], head, track, sector, 1, (BYTE far *)oldboot)) != 0)
	{
		fprintf(stderr, "%s: disk read error (code = 0x%02x)\n", pgm, i & 0xff);
		exit(1);
	}
}


BOOL check_space(drive, BlkBuffer)
COUNT drive;
BYTE *BlkBuffer;
{
	BYTE *bpbp;
	BYTE nfat;
	UWORD nfsect;
	ULONG hidden, count;
	ULONG block;
	UBYTE nreserved;
	UCOUNT i;
	WORD track, head, sector;
	UBYTE buffer[SEC_SIZE];
	ULONG bpb_huge;
	UWORD bpb_nsize;

	/* get local information				*/
	getbyte((VOID *)&BlkBuffer[BT_BPB + BPB_NFAT], &nfat);
	getword((VOID *)&BlkBuffer[BT_BPB + BPB_NFSECT], &nfsect);
	getlong((VOID *)&BlkBuffer[BT_BPB + BPB_HIDDEN], &hidden);
	getbyte((VOID *)&BlkBuffer[BT_BPB + BPB_NRESERVED], &nreserved);

	getlong((VOID *)&BlkBuffer[BT_BPB + BPB_HUGE], &bpb_huge);
	getword((VOID *)&BlkBuffer[BT_BPB + BPB_NSIZE], &bpb_nsize);

	count = miarray[drive].mi_size = bpb_nsize == 0 ?
	 bpb_huge : bpb_nsize;

	/* Fix media information for disk			*/
	getword((&(((BYTE *)&BlkBuffer[BT_BPB])[BPB_NHEADS])), &miarray[drive].mi_heads);
	head = miarray[drive].mi_heads;
	getword((&(((BYTE *)&BlkBuffer[BT_BPB])[BPB_NSECS])), &miarray[drive].mi_sectors);
	if(miarray[drive].mi_size == 0)
		getlong(&((((BYTE *)&BlkBuffer[BT_BPB])[BPB_HUGE])), &miarray[drive].mi_size);
	sector = miarray[drive].mi_sectors;
	if(head == 0 || sector == 0)
	{
		fprintf(stderr, "Drive initialization failure\n");
		exit(1);
	}
	miarray[drive].mi_cyls = count / (head * sector);

	return;
}

/*									*/
/* Do logical block number to physical head/track/sector mapping	*/
/*									*/
static COUNT ltop(trackp, sectorp, headp, unit, count, strt_sect, strt_addr)
WORD *trackp, *sectorp, *headp;
REG COUNT unit;
LONG strt_sect;
COUNT count;
byteptr strt_addr;
{
#ifdef I86
	ULONG ltemp;
#endif
	REG ls, ps;

#ifdef I86
	/* Adjust for segmented architecture				*/
	ltemp = (((ULONG)mk_segment(strt_addr) << 4) + mk_offset(strt_addr)) & 0xffff;
	/* Test for 64K boundary crossing and return count large	*/
	/* enough not to exceed the threshold.				*/
	count = (((ltemp + SEC_SIZE * count) & 0xffff0000l) != 0l)
		? (0xffffl - ltemp) / SEC_SIZE
		: count;
#endif

	*trackp = strt_sect / (miarray[unit].mi_heads * miarray[unit].mi_sectors);
	*sectorp = strt_sect % miarray[unit].mi_sectors + 1;
	*headp = (strt_sect % (miarray[unit].mi_sectors * miarray[unit].mi_heads))
		/ miarray[unit].mi_sectors;
	if(((ls = *headp * miarray[unit].mi_sectors + *sectorp - 1) + count) >
		(ps = miarray[unit].mi_heads * miarray[unit].mi_sectors))
		count = ps - ls;
	return count;
}


BOOL copy(drive, file)
COUNT drive;
BYTE *file;
{
	BYTE dest[64];
	COUNT ifd, ofd, ret;
	BYTE buffer[COPY_SIZE];
	struct ftime ftime;

	sprintf(dest, "%c:\\%s", 'A'+drive, file);
	if((ifd = open((BYTE FAR *)file, O_RDONLY | O_BINARY)) < 0)
	{
		fprintf(stderr, "%s: \"%s\" not found\n", pgm, file);
		return FALSE;
	}
	_fmode = O_BINARY;
	if((ofd = creat((BYTE FAR *)dest, S_IREAD | S_IWRITE)) < 0)
	{
		fprintf(stderr, "%s: can't create\"%s\"\n", pgm, dest);
		return FALSE;
	}
	while((ret = read(ifd, (VOID *)buffer, COPY_SIZE)) == COPY_SIZE)
		write(ofd, (VOID *)buffer, ret);
	if(ret >= 0)
		write(ofd, (VOID *)buffer, ret);
	getftime(ifd, &ftime);
	setftime(ofd, &ftime);
	close(ifd);
	close(ofd);
	return TRUE;
}


BOOL DiskReset(COUNT Drive)
{
	REG COUNT idx;

	/* Reset the drives						*/
	fl_reset();

	if(Drive >= 2 && Drive < NDEV)
	{
		COUNT RetCode;

		/* Retrieve all the partition information		*/
		for(RetCode = TRUE, idx = 0; RetCode && (idx < N_PART); idx++)
			RetCode = get_part(Drive, idx);
		if(!RetCode)
			return FALSE;

		/* Search for the first DOS partition and start		*/
		/* building the map for the hard drive			*/
		for(idx = 0; idx < N_PART; idx++)
		{
			if(partition[idx].peFileSystem == FAT12
			|| partition[idx].peFileSystem == FAT16SMALL
			|| partition[idx].peFileSystem == FAT16LARGE)
			{
				miarray[Drive].mi_offset
				 = partition[idx].peStartSector;
				active = idx;
				break;
			}
		}
	}

	return TRUE;
}

COUNT DiskRead(WORD drive, WORD head, WORD track, WORD sector, WORD count, BYTE FAR *buffer)
{
	int nRetriesLeft;

	for(nRetriesLeft = NRETRY; nRetriesLeft > 0; --nRetriesLeft)
	{
		if(fl_read(drive, head, track, sector, count, buffer) == count)
			return count;
	}
	return 0;
}

COUNT DiskWrite(WORD drive, WORD head, WORD track, WORD sector, WORD count, BYTE FAR *buffer)
{
	int nRetriesLeft;

	for(nRetriesLeft = NRETRY; nRetriesLeft > 0; --nRetriesLeft)
	{
		if(fl_write(drive, head, track, sector, count, buffer) == count)
			return count;
	}
	return 0;
}

