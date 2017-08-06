
/****************************************************************/
/*								*/
/*			      main.c				*/
/*			      DOS-C				*/
/*								*/
/*			Main IPL Functions			*/
/*								*/
/*		     Copyright (c) 1991, 1997			*/
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


#define MAIN
#include "../../hdr/portab.h"
#include "globals.h"

/* $Logfile:   C:/dos-c/src/ipl/main.c_v  $ */
#ifndef IPL
static BYTE *mainRcsId = "$Header:   C:/dos-c/src/ipl/main.c_v   1.5   16 Jan 1997 12:46:10   patv  $";
#endif

/*
 * $Log:   C:/dos-c/src/ipl/main.c_v  $
 *	
 *	   Rev 1.5   16 Jan 1997 12:46:10   patv
 *	pre-Release 0.92 feature additions
 *	
 *	   Rev 1.4   13 Sep 1996 19:25:56   patv
 *	Fixed boot for hard drive
 *	
 *	   Rev 1.3   29 Aug 1996 13:06:50   patv
 *	Bug fixes for v0.91b
 *	
 *	   Rev 1.2   01 Sep 1995 17:44:38   patv
 *	First GPL release.
 *	
 *	   Rev 1.1   30 Jul 1995 20:48:10   patv
 *	Eliminated version strings in ipl
 * 
 *    Rev 1.0   02 Jul 1995  8:25:52   patv
 * Initial revision.
 *
 */

COUNT ver(), help(), dir(), type(), prompt(), boot_kernel(),
	set_drive(), mon86();
COUNT unknown();
VOID init_ipl(), signon(), ipl();

#define ram_top	0x8000

struct table  commands[] =
{
	{"?", help},
	{"help", help},
	{"ver", ver},
	{"dir", dir},
	{"type", type},
	{"prompt", prompt},
	{"boot", boot_kernel},
	{"drive", set_drive},
	{"", unknown}
};

static COUNT BlockIndex = 0;
static BYTE *tail;		/* an MS-DOS wart emulation		*/

static BYTE pr_string[LINESIZE];
static BYTE *dflt_pr_string = "\n$n$g ";
static BYTE *psignon = "\nDOS-C IPL v%d.%02d\n";
BYTE *id = "wb2gbf";
static BYTE *szDefaultKernel = "kernel.exe";


/* function protypes				*/
#ifdef PROTO
VOID signon(VOID);
VOID init_ipl(VOID);
COUNT boot_kernel(VOID);
COUNT set_drive(COUNT argc, BYTE *argv[]);
VOID ipl(VOID);
struct table *lookup(struct table *, BYTE *);
COUNT unknown(VOID);
COUNT ver(VOID);
COUNT help(VOID);
COUNT type(COUNT, BYTE *[]);
COUNT dir(COUNT, BYTE *[]);
#else
VOID signon();
VOID init_ipl();
COUNT boot_kernel();
VOID ipl();
struct table *lookup();
COUNT unknown();
COUNT ver();
COUNT help();
COUNT type();
COUNT dir();
#endif
VOID init_ipl(), signon(), ipl();


VOID main()
{
	static BYTE *szDrive[] =
		{"A", "B", "C", "D"};

#ifdef DEBUG
	MarkStack();
	printf("\n*** IPL started ***\nStack at 0x%04x:%04x\n",
		MarkSS, MarkSP);
#endif

	init_ipl();

#ifdef DEBUG
	MarkStack();
	printf("\n*** IPL initialized ***\nStack at 0x%04x:%04x\n",
		MarkSS, MarkSP);
#endif

	signon();
	printf("\nIPL Booting %s from drive %s:\n",
		szDefaultKernel,
		szDrive[default_drive]);
	boot_kernel();
	ipl();
}

static VOID signon()
{
	printf(psignon, MAJOR_RELEASE, MINOR_RELEASE);
}


static VOID
init_ipl()
{
	REG struct dhdr FAR *dhp = (struct dhdr FAR *)&nul_dev;
	REG COUNT i;
	struct dhdr FAR *link_dhdr();

	/* Initialize driver chain			*/
#ifdef DEBUG
	MarkStack();
	printf("Initialize driver chain\nStack at 0x%04x:%04x\n",
		MarkSS, MarkSP);
	printf("Device driver chain starts at 0x%04x:%04x\n",
		FP_SEG(dhp), FP_OFF(dhp));
#endif
	dhp = link_dhdr(dhp, (struct dhdr FAR *)&con_dev, NULL);
	dhp = link_dhdr(dhp, (struct dhdr FAR *)&blk_dev, NULL);
	syscon = (struct dhdr FAR *)&con_dev;

	/* Initialize the screen handler for backspaces			*/
#ifdef DEBUG
	MarkStack();
	printf("Initialize the screen handler for backspaces\n");
#endif
	scr_pos = 0;

	/* Initialize the file tables					*/
#ifdef DEBUG
	printf("Initialize the file tables\n");
#endif
	for(i = 0; i < NFILES; i++)
		f_nodes[i].f_count = 0;

	/* Initialzie the current directory structures			*/
	for(i = 0; i < NDEVS; i++)
		scopy("\\", blk_devices[i].dpb_path);

	/* Initialze the disk buffer management functions		*/
	init_buffers();

	/* Now to initialize all special flags, etc.			*/
	mem_access_mode = FIRST_FIT;
	break_ena = TRUE;
	verify_ena = FALSE;
	InDOS = 0;
	version_flags = 0;

	/* Figure out 'A' or 'C' drive					*/
	if(BootDrive != 0)
		BootDrive = 3;
	else
		BootDrive = 1;

	/* Log-in the default drive.					*/
        default_drive = BootDrive - 1;
}


VOID 
init_device (struct dhdr FAR *dhp, BYTE FAR *cmdLine)
{
        request rq;
        ULONG memtop = ((ULONG)ram_top) << 10;
        ULONG maxmem = memtop - ((ULONG)FP_SEG(dhp) << 4);

        if (maxmem >= 0x10000) maxmem = 0xFFFF;

        rq.r_unit = 0;
        rq.r_status = 0;
        rq.r_command = C_INIT;
        rq.r_length = sizeof(request);
        rq.r_endaddr = MK_FP(FP_SEG(dhp), maxmem);
        rq.r_bpbptr = (void FAR *)(cmdLine? cmdLine : "\n");
        rq.r_firstunit = nblkdev;

        execrh((request FAR *)&rq, dhp);
        
        /* check for a block device and update  device control block    */
        if(!(dhp -> dh_attr & ATTR_CHAR) && (rq.r_nunits != 0))
        {
                REG COUNT Index;
                
                for(Index = 0; Index < rq.r_nunits; Index++, BlockIndex++)
                {
                        if (nblkdev)
                            blk_devices[nblkdev - 1].dpb_next = &blk_devices[nblkdev];

                        blk_devices[nblkdev].dpb_unit = nblkdev;
                        blk_devices[nblkdev].dpb_subunit = Index;
                        blk_devices[nblkdev].dpb_device = dhp;
                        blk_devices[nblkdev].dpb_flags = M_CHANGED;
                        
                        ++nblkdev;
                }
                blk_devices[nblkdev - 1].dpb_next = (void FAR *)0xFFFFFFFF;
        }
        DPBp = &blk_devices[0];
}
            
struct dhdr FAR *
link_dhdr(struct dhdr FAR *lp, struct dhdr FAR *dhp, BYTE FAR *cmdLine)
{
        lp -> dh_next = dhp;
        init_device(dhp, cmdLine);
        return dhp;
}


static COUNT boot_kernel()
{
	COUNT rc;
	exec_blk exb;

	/* Test for user break				*/
	/* if(con_break())
		return TRUE; */
	
	if(!KbdBusy())
	{
		if(_sti() == ESC)
			return TRUE;
	}

	printf("\nBooting DOS-C ");

#ifndef IPLDBG
	/* If not - initialize the exec block		*/
	/* and go for it!				*/
	exb.exec.env_seg = 0;
	exb.exec.cmd_line = "";
	exb.exec.fcb_1 = exb.exec.fcb_2 = (fcb far *)0;
	rc = DosExec(P_WAIT, (exec_blk far *)&exb, (BYTE far *)szDefaultKernel);

	/* if we're here, unable to load the kernel, so	*/
	/* give the user the reason and exit		*/
	printf("\nIPL cannot load kernel\n\n");
	switch(rc)
	{
	case DE_FILENOTFND:
		printf("File %s not found\n", szDefaultKernel);
		break;

	case DE_INVLDDATA:
		printf("Inavlid data\n");
		break;

	default:
		printf("Unknown reason (rc = %d)\n", rc);
		break;
	}
#endif
	return FALSE;
}


static VOID ipl()
{
	register struct table *p;
	register BYTE *lp;
	struct table *lookup();
	BYTE *scan(), *skipwh();
	static COUNT argc;
	static BYTE *argv[16];
	static BYTE args[16][LINESIZE];

	scopy(dflt_pr_string, pr_string);
	for(;;)
	{
		put_prompt(pr_string);
		kb_buf.kb_size = LINESIZE - 1;
		kb_buf.kb_count = 0;
		sti((keyboard far *)&kb_buf);
		kb_buf.kb_buf[kb_buf.kb_count] = '\0';
		for(argc = 0; argc < 16; argc++)
		{
			argv[argc] = (BYTE *)0;
			args[argc][0] = '\0';
		}
		lp = scan(kb_buf.kb_buf, args[0]);
		argv[0] = args[0];
		/* this kludge is for an MS-DOS wart emulation */
		tail = skipwh(lp);
		for(argc = 1; ; argc++)
		{
			lp = scan(lp, args[argc]);
			if(*args[argc] == '\0')
				break;
			else
				argv[argc] = args[argc];
		}
		if(*argv[0] != '\0')
		{
			p = lookup(commands, argv[0]);
			(*(p -> func))(argc, argv);
		}
	}
}


COUNT prompt(argc, argv)
COUNT argc;
BYTE *argv[];
{
	if(argc == 1)
		scopy(dflt_pr_string, pr_string);
	else
	{
		pr_string[0] = '\n';
		/* should be */
		/* scopy(argv[1], &pr_string[1]); */
		/* but to emulate an MS-DOS wart, is */
		scopy(tail, &pr_string[1]);
	}
	return TRUE;
}


/* put_prompt - print prompt obeying $x commands */
COUNT put_prompt(fmt)
register BYTE *fmt;
{
	register BYTE c;
	BYTE *s;

	while ((c = *fmt++) != '\0')
	{
		if (c != '$')
		{
			put_console(c);
			continue;
		}
		switch (c = *fmt++)
		{
		case 'p':
			put_console('A' + default_drive);
			put_console(':');
			for(s = blk_devices[default_drive].dpb_path; *s != '\0'; s++)
				put_console(*s);
			continue;

		case 'n':
			put_console('A' + default_drive);
			continue;

		case 'g':
			put_console('>');
			continue;

		case 'l':
			put_console('<');
			continue;

		case 'b':
			put_console('|');
			continue;

		case 'e':
			put_console('=');
			continue;

		default:
			put_console(c);
			continue;
		}
	}
	return TRUE;
}


struct table *lookup(p, token)
struct table *p;
BYTE *token;
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


COUNT unknown()
{
	printf("\nBad command or file name");
	return FALSE;
}


COUNT ver()
{
	printf(psignon, MAJOR_RELEASE, MINOR_RELEASE);
	return TRUE;
}


COUNT help()
{
	printf("\nSorry, help is not available");
	return TRUE;
}


COUNT set_drive(argc, argv)
COUNT argc;
BYTE *argv[];
{
	BYTE c;

	if(argc != 2)
	{
		printf("\nInvalid number of parameters\n");
		return FALSE;
	}

	printf("\nSetting drive to %s\n", argv[1]);
	c = tolower(argv[1][0]);
	if((argv[1][1] != ':') || ((c != 'a') && (c != 'c')))
	{
		printf("Invalid drive specification: %c\n, c");
		return FALSE;
	}

	/* log in the default drive					*/
	BootDrive = c - 'a' + 1;
	default_drive = BootDrive - 1;

	return TRUE;
}


COUNT type(argc, argv)
COUNT argc;
BYTE *argv[];
{
	COUNT fd;
	BYTE c;

	if(argc != 2)
	{
		printf("\nInvalid number of parameters\n");
		return FALSE;
	}
	if((fd = dos_open((BYTE far *)argv[1], 0)) < 0)
	{
		printf("\nFile not found");
		return FALSE;
	}
	else
		printf("\n");
	while(dos_read(fd, (BYTE far *)&c, 1) == 1)
		printf("%c", c);
	dos_close(fd);
	return TRUE;
}


COUNT dir(argc, argv)
COUNT argc;
BYTE *argv[];
{
	REG struct f_node *fnp;
	BYTE buf[13];
	COUNT count = 0;


	if(argc == 1)
	{
		if((fnp = dir_open((BYTE FAR *)".")) == (struct f_node *)0)
			goto quit;
	}
	else
	{
		if((fnp = dir_open((BYTE FAR *)argv[1])) == (struct f_node *)0)
			goto quit;
	}
	while(dir_read(fnp) == sizeof(struct dirent))
	{
		if(fnp -> f_dir.dir_name[0] != '\0' && fnp -> f_dir.dir_name[0] != DELETED)
		{
			WORD hour = TM_HOUR(fnp -> f_dir.dir_time);

			bcopy(fnp -> f_dir.dir_name, &buf[0], 8);
			buf[8] = ' ';
			bcopy(fnp -> f_dir.dir_ext, &buf[9], 3);
			buf[12] = '\0';
			printf("\n   %s %-10ld  %-2d-%-02d-%-02d  %-2d:%-02d%c",
				buf, fnp -> f_dir.dir_size,
				DT_MONTH(fnp -> f_dir.dir_date),
				DT_DAY(fnp -> f_dir.dir_date),
				(DT_YEAR(fnp -> f_dir.dir_date) + 1980) % 100,
				hour > 12 ? hour - 12 : (hour == 0) ? 12 : hour,
				TM_MIN(fnp -> f_dir.dir_time),
				hour >= 12 ? 'p' : 'a');
			++count;
		}
	}
	printf("\n%-10d file(s)\n", count);
quit:	dir_close(fnp);
	return TRUE;
}


