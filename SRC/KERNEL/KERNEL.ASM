;
; File:
;                          kernel.asm
; Description:
;                       kernel start-up code
;
;                    Copyright (c) 1995, 1996
;                       Pasquale J. Villani
;                       All Rights Reserved
;
; This file is part of DOS-C.
;
; DOS-C is free software; you can redistribute it and/or
; modify it under the terms of the GNU General public License
; as published by the Free Software Foundation; either version
; 2, or (at your option) any later version.
;
; DOS-C is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
; the GNU General public License for more details.
;
; You should have received a copy of the GNU General public
; License along with DOS-C; see the file COPYING.  If not,
; write to the Free Software Foundation, 675 Mass Ave,
; Cambridge, MA 02139, USA.
;
; $Header:   C:/usr/patv/dos-c/src/kernel/kernel.asv   1.11   06 Dec 1998  8:48:04   patv  $
;
; $Log:   C:/usr/patv/dos-c/src/kernel/kernel.asv  $
;
;   Rev 1.11   06 Dec 1998  8:48:04   patv
;Bug fixes.
;
;   Rev 1.10   03 Feb 1998 23:30:08   patv
;Added a start-up stack for loadable device drivers.  Need the separate
;stack so that all int 21h functions can be called.
;
;   Rev 1.9   22 Jan 1998  4:09:24   patv
;Fixed pointer problems affecting SDA
;
;   Rev 1.8   06 Jan 1998 20:12:32   patv
;Reduced device driver stack sizes.
;
;   Rev 1.7   04 Jan 1998 17:26:18   patv
;Corrected subdirectory bug
;
;   Rev 1.6   03 Jan 1998  8:36:50   patv
;Converted data area to SDA format
;
;   Rev 1.5   06 Feb 1997 22:43:18   patv
;Reduced stack sizes for block and clock devices.
;
;   Rev 1.4   06 Feb 1997 19:05:48   patv
;Added hooks for tsc command
;
;   Rev 1.3   29 May 1996 21:03:44   patv
;bug fixes for v0.91a
;
;   Rev 1.2   19 Feb 1996  3:24:06   patv
;Added NLS, int2f and config.sys processing
;
;   Rev 1.1   01 Sep 1995 17:54:24   patv
;First GPL release.
;
;   Rev 1.0   02 Jul 1995  9:05:44   patv
;Initial revision.
;
; $EndLog$
;

		page    60,132
		title   kernel start-up code

		include	segs.inc


_TEXT           segment byte public 'CODE'
		assume  cs:_TEXT

		extrn   _main:near
		extrn   _reinit_k:near
		extrn   _con_driver:near
		extrn	_ReqPktPtr:dword

STACK_SIZE      equ     384/2           ; stack allocated in words


IFNDEF ??version
 IF STANDALONE EQ 1
PUBLIC  __acrtused              ; trick used by MSC to force in startup
	__acrtused = 9876h
 ENDIF
ENDIF
		;
		;
		page



		;
		;
entry           proc    near
IF STANDALONE EQ 1
		extrn   _reinit_k:near
		jmp     short kernel_start      ; initialization entry
		jmp     _reinit_k               ; re-init entry
_OsText         dw      _TEXT                   ; Code Segment
_OsData         dw      DGROUP                  ; Data Segment
_OsVersion      dw      0100h                   ; OS version
ENDIF
		page
		;
		; kernel start-up
		;
IF STANDALONE EQ 1
kernel_start:   cli                     ; prevent interrupts while starting
		mov     ax,DGROUP
		mov     ss,ax
		mov     sp,offset DGROUP:tos
		; inititalize entry stack for high water tests
;               mov     di,seg stack_bottom
;               mov     es,di
;               mov     di,offset stack_bottom
;               mov     ax,offset last
;               sub     ax,di
;               sar     ax,1
;               mov     cx,ax
;               mov     ax,09090h
;               cld
;               rep     stosw
		; inititalize api stacks for high water tests
		mov     di,seg apistk_bottom
		mov     es,di
		mov     di,offset apistk_bottom
		mov     ax,offset apistk_top
		sub     ax,di
		sar     ax,1
		mov     cx,ax
		mov     ax,09090h
		cld
		rep     stosw
		; Now set up call frame
		mov     ax,ss
		mov     ds,ax
		mov     es,ax
		mov     bp,sp           ; and set up stack frame for c
		sti                     ; now enable them
		mov     _BootDrive,bx   ; tell where we came from
		mov     _NumFloppies,cx ; and how many
		
		mov     ax,ds
		mov     es,ax
		call    _main
		mov     ax,0
		push    ax
		call    _exit
		jmp     $
ENDIF
entry           endp


		;
		; _exit
		;       perform an "exit" and quit
		;
		; exit(code)
		; int code;
		;
IF STANDALONE EQ 1
_exit           proc    near
		public  _exit
		
		cli
		hlt
		jmp     _exit

_exit           endp
ENDIF
		page


		;
		; NUL device strategy
		;
_nul_strtgy     proc    far
		public _nul_strtgy
		mov     word ptr _ReqPktPtr,bx       ;save rq headr
		mov     word ptr _ReqPktPtr+2,es
		ret
_nul_strtgy     endp

		;
		; NUL device interrupt
		;
_nul_intr       proc    far
		public _nul_intr
		push    es
		push    bx
		les     bx,_ReqPktPtr                ;es:bx--> rqheadr
		or      word ptr es:[bx+3],100h ;set "done" flag
		pop     bx
		pop     es
		ret
_nul_intr       endp

		page
;
; special interrupt routine for break key handling
;

;       
;       void interrupt break_key(void)
;       
	extrn   _break_handler:near
	assume  cs:_TEXT
		public  _break_key
_break_key      proc    far
		push    ax
		push    bx
		push    cx
		push    dx
		push    es
		push    ds
		push    si
		push    di
		push    bp
		mov     bp,DGROUP
		mov     ds,bp
		;       
		mov     word ptr DGROUP:intr_dos_stk,sp ; use internal stack
		mov     word ptr DGROUP:intr_dos_seg,ss
		pushf                           ; put flags in bx
		pop     bx
		cli                             ; no interrupts
		mov     ax,ds                   ; get stack seg
		mov     ss,ax
		mov     sp,offset DGROUP:intr_stk_top
		push    bx
		popf                            ; restore interrupt flag
		mov     bp,sp                   ; make a c frame
		call    _break_handler          ; call handler
		;
		pushf
		pop     bx
		cli                             ; no interrupts
		mov     sp,word ptr DGROUP:intr_dos_stk ; use dos stack
		mov     ss,word ptr DGROUP:intr_dos_seg
		push    bx
		popf
		pop     bp
		pop     di
		pop     si
		pop     ds
		pop     es
		pop     dx
		pop     cx
		pop     bx
		pop     ax
		iret    
_break_key      endp

_TEXT           ends


_FIXED_DATA     segment para public 'DATA'

; Because of the following bytes of data, THIS MODULE MUST BE THE FIRST
; IN THE LINK SEQUENCE.  THE BYTE AT DS:0004 determines the SDA format in 
; use.  A 0 indicates MS-DOS 3.X style, a 1 indicates MS-DOS 4.0-6.X style.
		public  DATASTART
DATASTART       label   byte
dos_data        db      0
		dw      kernel_start
		db      (?)             ; padding
		dw      1               ; Hardcoded MS-DOS 4.0+ style

		db      (26h - 0ch - (offset $ - DATASTART)) dup (?)

; Globally referenced variables - WARNING: DO NOT CHANGE ORDER
; BECAUSE THEY ARE DOCUMENTED AS UNDOCUMENTED (?) AND HAVE
; MANY MULTIPLEX PROGRAMS AND TSR'S ACCESSING THEM
		public  _NetRetry
_NetRetry       dw      3               ;-000c network retry count
		public  _NetDelay
_NetDelay       dw      1               ;-000a network delay count
		public  _DskBuffer
_DskBuffer      dd      -1              ;-0008 current dos disk buffer
		dw      (?)             ;-0004 Unread con input
		public  _first_mcb
_first_mcb      dw      (?)             ;-0002 Start of user memory
		public  _DPBp
		public  MARK0026H
; A reference seems to indicate that this should start at offset 26h.
MARK0026H       equ     $
_DPBp           dd      (?)             ; 0000 First drive Parameter Block
		public  _sfthead
_sfthead        dd      (?)             ; 0004 System File Table head
		public  _clock
_clock          dd      (?)             ; 0008 CLOCK$ device
		public  _syscon
_syscon         dd      (?)             ; 000c console device
		public  _maxbksize
_maxbksize      dw      (?)             ; 0010 Number of Drives in system
		public  _firstbuf;
_firstbuf       dd      (?)             ; 0012 head of buffers linked list
		public  _CDSp
_CDSp           dd      (?)             ; 0016 Current Directory Structure
		public  _FCBp
_FCBp           dd      (?)             ; 001a FCB table pointer
		public  _nprotfcb
_nprotfcb       dw      (?)             ; 001e number of protected fcbs
		public  _nblkdev
_nblkdev        db      (?)             ; 0020 number of block devices
		public  _lastdrive
_lastdrive      db      (?)             ; 0021 value of last drive
		public  _nul_dev
_nul_dev        label   dword           ; 0022 device chain root
		dd      -1
		dw      8004h           ; attributes = char device, NUL bit set
		dw      offset _nul_strtgy
		dw      offset _nul_intr
		db      'NUL     '
		public  _njoined
_njoined        db      (?)             ; 0034 number of joined devices
		dw      0               ; 0035 DOS 4 pointer to special names (always zero in DOS 5)
setverPtr       dw      0,0             ; 0037 setver list
		dw      0               ; 003B cs offset for fix a20
		dw      0               ; 003D psp of last umb exec
		dw      1               ; 003F number of buffers
		dw      1               ; 0041 size of pre-read buffer
		public  _BootDrive
_BootDrive      dw      (?)             ; 0043 drive we booted from
		db      0               ; 0044 cpu type (1 if >=386)
		dw      0               ; 0045 Extended memory in KBytes
buf_info        dd      (?)             ; 0047 disk buffer chain
		dw      0               ; 004B 0 (DOS 4 = # hashing chains)
		dd      (?)             ; 004D pre-read buffer
		dw      0               ; 0051 # of sectors
		db      0               ; 0053 00=conv 01=HMA
		dw      0               ; 0054 deblock buf in conv
deblock_seg     dw      0               ; 0056 (offset always zero)
		db      3 dup (?)       ; 0058 unknown
		dw      0               ; 005B unknown
		db      0, 0FFh, 0      ; 005D unknown
		db      0               ; 0060 unknown
		dw      0               ; 0061 unknown
dmd_upper_link  db      0               ; 0063 upper memory link flag
		dw      0               ; 0064 unknown
dmd_upper_root  dw      0FFFFh          ; 0066 dmd_upper_root
		dw      0               ; 0068 para of last mem search
SysVarEnd       label   byte


; Some references seem to indicate that this data should start at 01fbh in
; order to maintain 100% MS-DOS compatibility.
		db      (01fbh - (SysVarEnd - DATASTART)) dup (?)

		public  MARK01FBH
MARK01FBH       equ     $
		db      128 dup (?)
		public  _kb_buf
_kb_buf db      129,0                   ; initialise buffer to empty
		db      128+1 dup (?)   ; room for 128 byte readline + LF
;
; Variables that follow are documented as part of the DOS 4.0-6.X swappable
; data area in Ralf Browns Interrupt List #56
;
; this byte is used for ^P support
		public  _PrinterEcho
_PrinterEcho    db      0               ;-34 -  0 = no printer echo, ~0 echo
		public  _verify_ena
_verify_ena     db      0               ; ~0, write with verify

; this byte is used for TAB's
		public _scr_pos
_scr_pos        db      0               ; Current Cursor Column
		public  _switchar
_switchar       db      '/'             ;-31 - switch char
		public  _mem_access_mode
_mem_access_mode db     0               ;-30 -  memory allocation strategy
		public  sharing_flag
sharing_flag    db      0               ; 00 = sharing module not loaded
					; 01 = sharing module loaded, but
					;      open/close for block devices
					;      disabled
					; FF = sharing module loaded,
					;      open/close for block devices
					;      enabled (not implemented)
		public  net_set_count
net_set_count   db      1               ;-28 -  count the name below was set
		public  net_name
net_name        db      '               ' ;-27 - 15 Character Network Name
		db      00                ; Terminating 0 byte


;
;       Variables contained the the "STATE_DATA" segment contain
;       information about the STATE of the current DOS Process. These
;       variables must be preserved regardless of the state of the INDOS
;       flag.
;
;       All variables that appear in "STATE_DATA" **MUST** be declared
;       in this file as the offsets from the INTERNAL_DATA variable are
;       critical to the DOS applications that modify this data area.
;
;
		public  _ErrorMode, _InDOS
		public  _CritErrLocus, _CritErrCode
		public  _CritErrAction, _CritErrClass
		public  _CritErrDev, _CritErrDrive
		public  _dta
		public  _cu_psp, _default_drive
		public  _break_ena
		public  _return_code, _return_mode
		public  _internal_data

		public  _CritPatch
_CritPatch      dw      0d0ch           ;-11 zero list of patched critical
		dw      0d0ch           ;    section variables
		dw      0d0ch
		dw      0d0ch
		dw      0d0ch
		db      (?)             ;-01 - unknown
_internal_data  label byte              ; <-- Address returned by INT21/5D06
_ErrorMode      db      0               ; 00 - Critical Error Flag
_InDOS          db      0               ; 01 - Indos Flag
_CritErrDrive   db      0               ; 02 - Drive on write protect error
_CritErrLocus   db      0               ; 03 - Error Locus
_CritErrCode    dw      0               ; 04 - DOS format error Code
_CritErrAction  db      0               ; 06 - Error Action Code
_CritErrClass   db      0               ; 07 - Error Class
_CritErrDev     dd      (?)             ; 08 - Failing Device Address
_dta            dd      (?)             ; 0C - current DTA
_cu_psp         dw      (?)             ; 10 - Current PSP
break_sp        dw      (?)             ; 12 - used in int 23
_return_code    db      0               ; 14 - return code from process
_return_mode    db      0               ; 15 - reason for process terminate
_default_drive  db      0               ; 16 - Current Drive
_break_ena      db      0               ; 17 - Break Flag
		db      0               ; 18 - flag, code page switching
		db	0		; 19 - flag, copy of 18 on int 24h abort

		public  _swap_always, _swap_indos
_swap_always    label   byte

		public  _Int21AX
_Int21AX        dw      0               ; 1A - AX from last Int 21

		public  owning_psp, _MachineId
owning_psp      dw      0               ; 1C - owning psp
_MachineId      dw      0               ; 1E - remote machine ID
		dw      0               ; 20 - First usable mcb
		dw      0               ; 22 - Best usable mcb
		dw      0               ; 24 - Last usable mcb
		dw      0               ; 26 - memory size in paragraphs
		dw      0               ; 28 - unknown
		db      0               ; 2A - unknown
		db      0               ; 2B - unknown
		db      0               ; 2C - unknown
		public  _break_flg
_break_flg      db      0               ; 2D - Program aborted by ^C
		db      0               ; 2E - unknown
		db      0               ; 2F - not referenced
		public  _DayOfMonth
_DayOfMonth     db      1               ; 30 - day of month
		public  _Month
_Month          db      1               ; 31 - month
		public  _YearsSince1980
_YearsSince1980 dw      0               ; 32 - year since 1980
daysSince1980   dw      0FFFFh          ; 34 - number of days since epoch
					; force rebuild on first clock read
		public  _DayOfWeek
_DayOfWeek      db      2               ; 36 - day of week
		public  _Year
_Year           dw      1980            ; 37 - year
		db      0               ; 39 - unknown
		public  _CharReqHdr
_CharReqHdr     label   byte
		public  _ClkReqHdr
_ClkReqHdr      db      30 dup (?)      ; 3A - Device driver request header
		dd      0               ; 58 - pointer to driver entry
		public  _MediaReqHdr
_MediaReqHdr    db      22 dup (?)      ; 5C - Device driver request header
		public  _IoReqHdr
_IoReqHdr       db      30 dup (?)      ; 72 - Device driver request header
		db      6 dup (?)       ; 90 - unknown
		public  _ClkRecord
_ClkRecord      db      6 dup (?)       ; 96 - CLOCK$ transfer record
		dw      (?)             ; 9C - unknown
		public  __PriPathBuffer
__PriPathBuffer db      80h dup (?)     ; 9E - buffer for file name
		public  __SecPathBuffer
__SecPathBuffer db      80h dup (?)     ;11E - buffer for file name
		public  _TempBuffer
_TempBuffer     db      21 dup (?)      ;19E - 21 byte srch state
		public  _SearchDir
_SearchDir      db      32 dup (?)      ;1B3 - 32 byte dir entry
		public  _TempCDS
_TempCDS        db      88 dup (?)      ;1D3 - TemporaryCDS buffer
		public  _DirEntBuffer
_DirEntBuffer   db      32 dup (?)      ;22B - space enough for 1 dir entry
		public  _wAttr
_wAttr          dw      0               ;24B - extended FCB file attribute


		; Pad to 0570h
		db      (250h - (offset $ - _internal_data)) dup (?)
		public  _lpUserStack
_lpUserStack    dd      (?)             ;250 - pointer to user stack frame

		; Pad to 057Ch
		db      (25ch - (offset $ - _internal_data)) dup (?)

		public  _tsr            ; used by break and critical error
_tsr            db      0               ;25C -  handlers during termination
		db      (?)             ;25D - padding
		public  term_psp
term_psp        dw  0                   ;25E - ???
		public  int24_esbp
int24_esbp      dw      2 dup (?)       ;260 - pointer to criticalerr DPB
		public  _user_r, int21regs_off, int21regs_seg
_user_r         label   dword
int21regs_off   dw      0               ;264 - pointer to int21h stack frame
int21regs_seg   dw      0
		public  critical_sp
critical_sp     dw      0               ;268 - critical error internal stack
		public  current_ddsc
current_ddsc    dw      2 dup (?)

		; Pad to 059ah 
		db      (27ah - (offset $ - _internal_data)) dup (?)
		public  current_device
current_device  dw      2 dup (?)       ;27A - ???
		public  _lpCurSft
_lpCurSft       dw      2 dup (?)       ;27e - Current SFT
		public  current_ldt
current_ldt     dw      2 dup (?)       ;282 - Current CDS
		public  _lpFcb
_lpFcb          dw      2 dup (?)       ;286 - pointer to callers FCB
		public  current_ifn
current_ifn     dw      0               ;28A - SFT index for next open

		; Pad to 05ceh 
		db      (2aeh - (offset $ - _internal_data)) dup (?)
		public  current_filepos
current_filepos dw      2 dup (?)       ;2AE - current offset in file

		; Pad to 05f0h 
		db      (2d0h - (offset $ - _internal_data)) dup (?)
		public  _prev_user_r
		public  prev_int21regs_off
		public  prev_int21regs_seg
_prev_user_r    label   dword
prev_int21regs_off      dw      0       ;2D0 - pointer to prev int 21 frame
prev_int21regs_seg      dw      0

		; Pad to 0620h 
		db      (300h - (offset $ - _internal_data)) dup (?)

		public  _FcbSearchBuffer        ; during FCB search 1st/next use bottom
_FcbSearchBuffer        label byte              ;  of error stack as scratch buffer
;               db      43 dup (?)              ;  - only used during int 21 call
		public  _LocalPath
_LocalPath      label   byte                    
;               db      67 dup (?)              
		; stacks are made to initialize to no-ops so that high-water
		; tesing can be performed
apistk_bottom   label   word
		dw      STACK_SIZE dup (?)      ;300 - Error Processing Stack
		public  _error_tos
_error_tos      label   word
		dw      STACK_SIZE dup (?)      ;480 - Disk Function Stack
		public  _disk_api_tos
_disk_api_tos   label   word
		dw      STACK_SIZE dup (?)      ;600 - Char Function Stack
		public  _char_api_tos
_char_api_tos   label   word
apistk_top      label   word

_VolChange      db      0               ;781 - volume change
_VirtOpen       db      0               ;782 - virtual open flag

		; controlled variables end at offset 78Ch so pad to end
		db      (78ch - (offset $ - _internal_data)) dup (?)
_swap_indos     label   byte
;
; end of controlled variables
;
_FIXED_DATA     ends

_BSS            segment word public 'BSS'
		public  _NumFloppies
_NumFloppies    dw      (?)
intr_dos_stk    dw      (?)
intr_dos_seg    dw      (?)


		public  _api_sp
_api_sp         dw      (?)             ; api stacks - for context
		public  _api_ss
_api_ss         dw      (?)             ; switching
		public  _usr_sp
_usr_sp         dw      (?)             ; user stacks
		public  _usr_ss
_usr_ss         dw      (?)
		public  _ram_top
_ram_top        dw      (?)

_BSS            ends



_BSSEND         segment byte public 'STACK'
; blockdev private stack
		public	blk_stk_top
		dw      256 dup (?)
blk_stk_top     label   byte

; clockdev private stack
		public	clk_stk_top
		dw      256 dup (?)
clk_stk_top     label   byte

; interrupt stack
		dw      256 dup (?)
intr_stk_top    label   byte

; int 21 api stack area
		public  stack_bottom
stack_bottom    label   word
		dw      512
tos             label   byte

		public  last
last            label   word                    ; must always be end of stack area
		public  _last
_last           label   byte                    ; and c version

_BSSEND         ends


		end     entry

