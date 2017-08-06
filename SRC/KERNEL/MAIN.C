
/****************************************************************/
/*                                                              */
/*                           main.c                             */
/*                            DOS-C                             */
/*                                                              */
/*                    Main Kernel Functions                     */
/*                                                              */
/*                   Copyright (c) 1995, 1996                   */
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


#define MAIN
#include "../../hdr/portab.h"
#include "globals.h"

/* $Logfile:   C:/usr/patv/dos-c/src/kernel/main.c_v  $ */
#ifdef VERSION_STRINGS
static BYTE *mainRcsId = "$Header:   C:/usr/patv/dos-c/src/kernel/main.c_v   1.12   06 Dec 1998  8:45:30   patv  $";
#endif

/* $Log:   C:/usr/patv/dos-c/src/kernel/main.c_v  $
 * 
 *    Rev 1.12   06 Dec 1998  8:45:30   patv
 * Changed due to new I/O subsystem.
 * 
 *    Rev 1.11   22 Jan 1998  4:09:24   patv
 * Fixed pointer problems affecting SDA
 * 
 *    Rev 1.10   04 Jan 1998 23:15:20   patv
 * Changed Log for strip utility
 * 
 *    Rev 1.9   04 Jan 1998 17:26:16   patv
 * Corrected subdirectory bug
 * 
 *    Rev 1.8   03 Jan 1998  8:36:48   patv
 * Converted data area to SDA format
 * 
 *    Rev 1.7   06 Feb 1997 21:35:46   patv
 * Modified to support new version format and changed debug message to
 * output drive letter instead of number.
 * 
 *    Rev 1.6   22 Jan 1997 13:05:02   patv
 * Now does correct default drive initialization.
 * 
 *    Rev 1.5   16 Jan 1997 12:47:00   patv
 * pre-Release 0.92 feature additions
 * 
 *    Rev 1.3   29 May 1996 21:03:32   patv
 * bug fixes for v0.91a
 * 
 *    Rev 1.2   19 Feb 1996  3:21:36   patv
 * Added NLS, int2f and config.sys processing
 * 
 *    Rev 1.1   01 Sep 1995 17:54:18   patv
 * First GPL release.
 * 
 *    Rev 1.0   02 Jul 1995  8:33:18   patv
 * Initial revision.
 */
/* $EndLog$ */

static COUNT BlockIndex = 0;

VOID    configDone(VOID);
static void InitIO(void);

VOID init_kernel(VOID), signon(VOID), kernel(VOID);
static COUNT p_0(VOID);
VOID FsConfig(VOID);

VOID 
main (void)
{
#ifdef KDB
	BootDrive = 1;
#endif
        init_kernel();
#ifdef DEBUG
	/* Non-portable message kludge alert!				*/
        printf("KERNEL: Boot drive = %c\n", 'A' + BootDrive - 1);
#endif
        signon();
        kernel();
}



VOID 
reinit_k (void)
{
        REG COUNT i;

        /* Re-initialize drivers                        */
        syscon = &con_dev;

        /* Test ram for sane mcb's                      */
        if(DosMemCheck() != SUCCESS)
                panic("Memory corrupted");
}


static VOID 
init_kernel (void)
{
        COUNT i;
        
        cu_psp = DOS_PSP;//psp = program segment prefix    
        nblkdev = 0;
        maxbksize = 0x200;
        switchar = '/';
        
        /* Init oem hook - returns memory size in KB    */
        ram_top = init_oem();

#ifndef KDB
	for (i = 0x20; i <= 0x3f; i++)
	     setvec(i, empty_handler);
#endif

	/* Initialize IO subsystem					*/
	InitIO();
        syscon = (struct dhdr FAR *)&con_dev;
        clock = (struct dhdr FAR *)&clk_dev;

#ifndef KDB
	/* set interrupt vectors                                        */
	setvec(0x20, int20_handler);
	setvec(0x21, int21_handler);
	setvec(0x22, int22_handler);
	setvec(0x23, int23_handler);
	setvec(0x24, int24_handler);
	setvec(0x25, (VOID (INRPT FAR *)())low_int25_handler);
	setvec(0x26, (VOID (INRPT FAR *)())low_int26_handler);
	setvec(0x27, int27_handler);
	setvec(0x28, int28_handler);
	setvec(0x2f, int2f_handler);
#endif

        /* Initialize the screen handler for backspaces                 */
        scr_pos = 0;
        break_ena = TRUE;

        /* Do first initialization of system variable buffers so that   */
        /* we can read config.sys later.                                */
	lastdrive = Config.cfgLastdrive;
	PreConfig();

	/* Now config the temporary file system                         */
	FsConfig();

#ifndef KDB
	/* Now process CONFIG.SYS                                       */
	DoConfig();

	lastdrive = Config.cfgLastdrive;
	if (lastdrive < nblkdev)
		lastdrive = nblkdev;

        /* and do final buffer allocation.                              */
        PostConfig();
        
        /* Now config the final file system                             */
        FsConfig();

        /* and process CONFIG.SYS one last time to load device drivers. */
        DoConfig();
	configDone();
#endif

        /* Now to initialize all special flags, etc.                    */
        mem_access_mode = FIRST_FIT;
        verify_ena = FALSE;
        InDOS = 0;
        version_flags = 0;
        pDirFileNode = 0;
}

VOID FsConfig(VOID)
{
        REG COUNT i;
        date Date;
        time Time;

	/* Get the start-up date and time				*/
	Date = dos_getdate();
	Time = dos_gettime();

        /* Initialize the file tables                                   */
        for(i = 0; i < Config.cfgFiles; i++)
                f_nodes[i].f_count = 0;

        /* The system file tables need special handling and are "hand   */
        /* built. Included is the stdin, stdout, stdaux and atdprn.     */
        sfthead -> sftt_next = (sfttbl FAR *)-1;
        sfthead -> sftt_count = Config.cfgFiles;
        for(i = 0; i < sfthead -> sftt_count ; i++)
        {
                sfthead -> sftt_table[i].sft_count = 0;
                sfthead -> sftt_table[i].sft_status = -1;
        }
        /* 0 is /dev/con (stdin)                                        */
        sfthead -> sftt_table[0].sft_count = 1;
        sfthead -> sftt_table[0].sft_mode = SFT_MREAD;
        sfthead -> sftt_table[0].sft_attrib = 0;
        sfthead -> sftt_table[0].sft_flags =
          (con_dev.dh_attr & ~SFT_MASK) | SFT_FDEVICE | SFT_FEOF | SFT_FSTDIN | SFT_FSTDOUT;
        sfthead -> sftt_table[0].sft_psp = DOS_PSP;
        sfthead -> sftt_table[0].sft_date = Date;
        sfthead -> sftt_table[0].sft_time = Time;
        fbcopy(
                (VOID FAR *)"CON        ",
                (VOID FAR *)sfthead -> sftt_table[0].sft_name, 11);
        sfthead -> sftt_table[0].sft_dev = (struct dhdr FAR *)&con_dev;

        /* 1 is /dev/con (stdout)                                       */
        sfthead -> sftt_table[1].sft_count = 1;
        sfthead -> sftt_table[1].sft_mode = SFT_MWRITE;
        sfthead -> sftt_table[1].sft_attrib = 0;
        sfthead -> sftt_table[1].sft_flags = 
          (con_dev.dh_attr & ~SFT_MASK) | SFT_FDEVICE | SFT_FEOF | SFT_FSTDIN  | SFT_FSTDOUT;
        sfthead -> sftt_table[1].sft_psp = DOS_PSP;
        sfthead -> sftt_table[1].sft_date = Date;
        sfthead -> sftt_table[1].sft_time = Time;
        fbcopy(
                (VOID FAR *)"CON        ",
                (VOID FAR *)sfthead -> sftt_table[1].sft_name, 11);
        sfthead -> sftt_table[1].sft_dev = (struct dhdr FAR *)&con_dev;

        /* 2 is /dev/con (stderr)                                       */
        sfthead -> sftt_table[2].sft_count = 1;
        sfthead -> sftt_table[2].sft_mode = SFT_MWRITE;
        sfthead -> sftt_table[2].sft_attrib = 0;
        sfthead -> sftt_table[2].sft_flags = 
          (con_dev.dh_attr & ~SFT_MASK) | SFT_FDEVICE | SFT_FEOF | SFT_FSTDIN  | SFT_FSTDOUT;
        sfthead -> sftt_table[2].sft_psp = DOS_PSP;
        sfthead -> sftt_table[2].sft_date = Date;
        sfthead -> sftt_table[2].sft_time = Time;
        fbcopy(
                (VOID FAR *)"CON        ",
                (VOID FAR *)sfthead -> sftt_table[2].sft_name, 11);
        sfthead -> sftt_table[2].sft_dev = (struct dhdr FAR *)&con_dev;

        /* 3 is /dev/aux						*/
        sfthead -> sftt_table[3].sft_count = 1;
        sfthead -> sftt_table[3].sft_mode = SFT_MRDWR;
        sfthead -> sftt_table[3].sft_attrib = 0;
        sfthead -> sftt_table[3].sft_flags = 
          (aux_dev.dh_attr & ~SFT_MASK) | SFT_FDEVICE;
        sfthead -> sftt_table[3].sft_psp = DOS_PSP;
        sfthead -> sftt_table[3].sft_date = Date;
        sfthead -> sftt_table[3].sft_time = Time;
        fbcopy(
                (VOID FAR *)"AUX        ",
                (VOID FAR *)sfthead -> sftt_table[3].sft_name, 11);
        sfthead -> sftt_table[3].sft_dev = (struct dhdr FAR *)&aux_dev;

        /* 4 is /dev/prn						*/
        sfthead -> sftt_table[4].sft_count = 1;
        sfthead -> sftt_table[4].sft_mode = SFT_MWRITE;
        sfthead -> sftt_table[4].sft_attrib = 0;
        sfthead -> sftt_table[4].sft_flags =
          (prn_dev.dh_attr & ~SFT_MASK) | SFT_FDEVICE;
        sfthead -> sftt_table[4].sft_psp = DOS_PSP;
        sfthead -> sftt_table[4].sft_date = Date;
        sfthead -> sftt_table[4].sft_time = Time;
        fbcopy(
                (VOID FAR *)"PRN        ",
                (VOID FAR *)sfthead -> sftt_table[4].sft_name, 11);
        sfthead -> sftt_table[4].sft_dev = (struct dhdr FAR *)&prn_dev;

        /* Log-in the default drive.                                    */
        /* Get the boot drive from the ipl and use it for default.      */
        default_drive = BootDrive - 1;

        /* Initialzie the current directory structures                  */
        for(i = 0; i < NDEVS; i++)
                scopy("\\", blk_devices[i].dpb_path);

        /* Initialze the disk buffer management functions               */
        init_buffers();
}


static VOID signon()
{
        printf("\nDOS-C compatibility %d.%02d\n%s\n",
                os_major, os_minor, copyright);
        printf(os_release,
         REVISION_MAJOR, REVISION_MINOR, REVISION_SEQ,
         BUILD);
}


static VOID kernel()
{
        seg asize;
        BYTE FAR *ep, *sp;
        COUNT ret_code;
#ifndef KDB
        static BYTE *path = "PATH=";
#endif

#ifdef KDB
        kdb();
#else
        /* create the master environment area                           */
        if(DosMemAlloc(0x20, FIRST_FIT, (seg FAR *)&master_env, (seg FAR *)&asize) < 0)
                fatal("cannot allocate master environment space");

        /* populate it with the minimum environment                     */
        ++master_env;       
        ep = MK_FP(master_env, 0);   
        
        for(sp = path; *sp != 0; )
                *ep++ = *sp++;

        *ep++ = '\0';
        *ep++ = '\0';
        *((int FAR *)ep) = 0;
        ep += sizeof(int);    
#endif
	RootPsp = ~0;
        ret_code = p_0();
        exit(ret_code);
}

/* process 0                                                            */
static COUNT
p_0(VOID)
{
        exec_blk exb;
        CommandTail Cmd;
        BYTE FAR *szfInitialPrgm = (BYTE FAR *)Config.cfgInit;
        int     rc;

        /* Execute command.com /P from the drive we just booted from    */
        exb.exec.env_seg = master_env;
        strcpy(Cmd.ctBuffer, Config.cfgInitTail);
        
        for (Cmd.ctCount = 0; Cmd.ctCount < 127; Cmd.ctCount++)
            if (Cmd.ctBuffer[Cmd.ctCount] == '\r') break;

        exb.exec.cmd_line = (CommandTail FAR *)&Cmd;
        exb.exec.fcb_1 = exb.exec.fcb_2 = (fcb FAR *)0;
#ifdef DEBUG
        printf("Process 0 starting: %s\n\n", (BYTE *)szfInitialPrgm);
#endif 
        if((rc = DosExec(0, (exec_blk FAR *)&exb, szfInitialPrgm)) != SUCCESS)
        {
                printf("\nBad or missing Command Interpreter: %d\n", rc);
                return -1;
        }
        else
        {
                printf("\nSystem shutdown complete\nReboot now.\n");
                return 0;
        }
}

extern BYTE FAR *lpBase;

/* If cmdLine is NULL, this is an internal driver */

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
        
        if (cmdLine) lpBase = rq.r_endaddr;
        
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
            
struct dhdr FAR *link_dhdr(struct dhdr FAR *lp, struct dhdr FAR *dhp, BYTE FAR *cmdLine)
{
        lp -> dh_next = dhp;
        init_device(dhp, cmdLine);
        return dhp;
}


static void InitIO(void)
{
	/* Initialize driver chain                                      */
        nul_dev.dh_next = (struct dhdr FAR *)&con_dev;
	setvec(0x29, int29_handler);    /* Requires Fast Con Driver     */
	init_device((struct dhdr FAR *)&con_dev, NULL);
	init_device((struct dhdr FAR *)&clk_dev, NULL);
	init_device((struct dhdr FAR *)&blk_dev, NULL);
}
