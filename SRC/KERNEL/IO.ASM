;
; File:
;			      io.asm
; Description:
;                       DOS-C I/O Subsystem
;
;                       Copyright (c) 1998
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
; $Header$
;
; $Log$
; $EndLog$
;

		page    60,132
		title   DOS-C I/O Subsystem

		include	segs.inc

		extrn	ConTable
		extrn	LptTable
		extrn	ComTable
		extrn	uPrtNo:byte
		extrn	CommonNdRdExit
		extrn	_NumFloppies:word
		extrn	blk_stk_top:byte
		extrn	clk_stk_top:byte
		extrn   _blk_driver:near
		extrn   _clk_driver:near

;---------------------------------------------------
;
; Device entry points
;
; This really should be a struct and go into a request.inc file
;
cmdlen  equ     0                       ; Length of this command
unit    equ     1                       ; Subunit Specified
cmd     equ     2                       ; Command Code
status  equ     3                       ; Status
media   equ     13                      ; Media Descriptor
trans   equ     14                      ; Transfer Address
count   equ     18                      ; Count of blocks or characters
start   equ     20                      ; First block to transfer
vid	equ	22			; Volume id pointer
huge	equ	26			; First block (32-bit) to transfer

		page
;
; The following is the "array" of device driver headers for the internal
; devices.  There is one header per device including special aux: and prn: 
; pseudo devices.  These psuedo devices are necessary for printer 
; redirection, i.e., serial or parallel ports, and com port aux selection.
;
; The devices are linked into each other and terminate with a -1 next 
; pointer.  This saves some time on boot up and also allows us to throw all
; device initialization into a single io_init function that may be placed 
; into a discardable code segmemnt.
;
_IO_FIXED_DATA  segment word public 'CODE'
		assume cs:TGROUP

		;
		; The "CON" device
		;
		; This device is the standard console device used by
		; DOS-C and kernel
		;
		public  _con_dev
_con_dev        equ     $
		dd      _prn_dev
		dw      8013h           ; con device (stdin & stdout)
		dw      offset GenStrategy
		dw      offset ConIntr
		db      'CON     '

		;
		; Generic prn device that can be redirected via mode
		;
		public	_prn_dev
_prn_dev	dd	_aux_dev
		dw      0A040h
		dw      offset GenStrategy
		dw      offset PrnIntr
		db      'PRN     '

		;
		; Generic aux device that can be redirected via mode
		;
		public	_aux_dev
_aux_dev	dd      _Lpt1Dev
		dw      8000h
		dw      offset GenStrategy
		dw      offset AuxIntr
		db      'AUX     '

		;
		; Printer device drivers
		;
_Lpt1Dev	dd      _Lpt2Dev
		dw      0A040h
		dw      offset GenStrategy
		dw      offset Lpt1Intr
		db      'LPT1    '
_Lpt2Dev	dd      _Lpt3Dev
		dw      0A040h
		dw      offset GenStrategy
		dw      offset Lpt2Intr
		db      'LPT2    '
_Lpt3Dev	dd      _Com1Dev
		dw      0A040h
		dw      offset GenStrategy
		dw      offset Lpt3Intr
		db      'LPT3    '

		;
		; Com device drivers
		;
_Com1Dev	dd	_Com2Dev
		dw      8000h
		dw      offset GenStrategy
		dw      offset AuxIntr
		db      'COM1    '
_Com2Dev	dd      _Com3Dev
		dw      8000h
		dw      offset GenStrategy
		dw      offset Com2Intr
		db      'COM2    '
_Com3Dev	dd      _Com4Dev
		dw      8000h
		dw      offset GenStrategy
		dw      offset Com3Intr
		db      'COM3    '
_Com4Dev	dd	_clk_dev
		dw      8000h
		dw      offset GenStrategy
		dw      offset Com4Intr
		db      'COM4    '

		;
		; Header for clock device
		;
		public  _clk_dev
_clk_dev        equ     $
		dd      _blk_dev
		dw      8004h           ; clock device
		dw      offset GenStrategy
		dw      offset clk_entry
		db      'CLOCK$  '

		;
		; Header for device
		;
		public  _blk_dev
_blk_dev        equ     $
		dd      -1
		dw      0000h           ; block device
		dw      offset GenStrategy
		dw      offset blk_entry
		db      4
		db      0,0,0,0,0,0,0

_IO_FIXED_DATA  ends
		page


;
; Temporary table until next release
;
_IO_FIXED_DATA  segment word public 'CODE'
		assume cs:TGROUP
DiskTable       db      0
_IO_FIXED_DATA	ends


;
; Local storage
;

_BSS            segment word public 'BSS'
		public	_ReqPktPtr
_ReqPktPtr	dd	(?)
uUnitNumber     dw      (?)
blk_dos_stk     dw      (?)
blk_dos_seg     dw      (?)
clk_dos_stk     dw      (?)
clk_dos_seg     dw      (?)
_BSS            ends

_IO_TEXT        segment byte public 'CODE'
		assume cs:TGROUP

		page
;
; Name:
;	GenStrategy
;
; Function:
;	Store the pointer to the request packet passed in es:bx
;
; Description:
;	Generic strategy routine.  Unlike the original multitasking versions, 
;	this version assumes that no more thank one device driver is active
;	at any time.  The request is stored into memory in the one and only
;	location available for that purpose.
;
GenStrategy	proc	far
		mov     word ptr cs:_ReqPktPtr,bx
		mov     word ptr cs:_ReqPktPtr+2,es
		ret
GenStrategy	endp


		page
;
; Name:
;	XXXXIntr
;
; Function:
;	Individual Interrupt routines for each device driver
;
; Description:
;	This is actually a single routine with entry points for each device. 
;	The name used for the entry point is the device name with Intr 
;	appended to it.
;
;	Funtionally, each device driver has an entry and an associated 
;	table.  The table is a structure that consists of a control byte 
;	followed by an array of pointers to C functions or assembly 
;	subroutines that implement the individual device driver functions.  
;	This allows the usage of common error dummy filler code to be used.  
;	It also allows standardization of the calling procedure for these 
;	internal device driver functions.
;
;	Assembler call/return convention:
;	Each driver function is entered by a jump into the function and 
;	exits by a jump to the appropriate success or error exit routine.  
;	This speeds up the call and return and helps to minimize the stack 
;	useage.  The contents of the request packet are passed to each 
;	routine in registers as follows:
;
;	      Register  Function		Description
;	      --------  --------		-----------
;		al	unit			Subunit Specified
;		ah	media			Media Descriptor
;		cx	count			Count of blocks or characters
;		dx	start			First block to transfer
;		es:di	trans			Transfer Address
;		ds:bx	reqptr			Request pointer
;		cs	kernel code segment
;		ds	kernel data segment
;
;	The exit routines generally set the status based on the individual 
;	routine.  For example, _IOSuccess will clear the count where 
;	_IOErrCnt will subtract the remaining amount in cx from the original 
;	count.  See each utility routine for expectations.
;
;	C call/return convention:
;	The C calling convention simply sets up the C stack and passes the
;	request packet pointer as a far pointer to the function.  Although 
;	the utility routine names are such that they are accesible from the 
;	C name space, they are cannot used.  Instead, the common interrupt 
;	code expects a return status to set in the request packet.  It is up 
;	to the device driver function to set the appropriate fields such as 
;	count when an error occurs.
;
;	How to differntiate between the two calling conventions:
;	This code is entirely table driven.  The table is a structure that 
;	is generally in the _IO_FIXED_DATA segment.  It consists of a flag 
;	byte followed by short pointers to the driver functions.  Selecting 
;	a driver type is accomplished by setting the type bit in the flag 
;	(see below).
;
;	  7   6   5   4   3   2   1   0
;	+---+---+---+---+---+---+---+---+
;	|   |   |   |   |   |   |   |   |
;	+---+---+---+---+---+---+---+---+
;	  | |       |                   |---	Number of table entries
;	  | |       +-------------------+
;	  | |       |-----------------------	Reserved
;	  | +-------+
;	  +---------------------------------	type bit (1 == C / 0 == asm)
;
ConIntr:
		push    si
		mov     si,offset TGROUP:ConTable
		jmp     short CharIntrEntry

PrnIntr:
		push    si
		push    ax
		xor     ax,ax
		jmp     short LptCmnIntr

Lpt1Intr:
		push    si
		push    ax
		xor     al,al
		mov     ah,1
		jmp     short LptCmnIntr

Lpt2Intr:
		push    si
		push    ax
		mov     al,1
		mov     ah,2
		jmp     short LptCmnIntr

Lpt3Intr:
		push    si
		push    ax
		mov     al,2
		mov     ah,3

LptCmnIntr:
		mov     si,offset TGROUP:LptTable
		mov     cs:uPrtNo,ah
		jmp     short DiskIntrEntry


AuxIntr:
		push    si
		push    ax
		xor     al,al
		jmp     short ComCmnIntr

Com2Intr:
		push    si
		push    ax
		mov     al,1
		jmp     short ComCmnIntr

Com3Intr:
		push    si
		push    ax
		mov     al,2
		jmp     short ComCmnIntr

Com4Intr:
		push    si
		push    ax
		mov     al,3
		jmp     short ComCmnIntr

ComCmnIntr:
		mov     si,offset ComTable
		jmp     short DiskIntrEntry


DskIntr:
		push    si
		mov     si,offset TGROUP:DiskTable
CharIntrEntry:
		push    ax
DiskIntrEntry:
		push    cx
		push    dx
		push    di
		push    bp
		push    ds
		push    es
		push    bx
		mov     byte ptr cs:uUnitNumber,al
		lds     bx,cs:_ReqPktPtr
		test	byte ptr cs:[si],80h
		je	AsmType

		mov     al,[bx+cmd]
		cmp     al,cs:[si]
		ja      _IOCommandError
		cbw
		shl     ax,1
		add     si,ax
		xchg    di,ax

		push	ds
		push	bx
		mov	bp,sp
		mov     ax,DGROUP
		mov     ds,ax
		cld
		call	word ptr cs:[si+1]
		pop	cx
		pop	cx
		jmp	short StoreStatus

AsmType:	mov     al,[bx+unit]
		mov     ah,[bx+media]
		mov     cx,[bx+count]
		mov     dx,[bx+start]
		xchg    di,ax
		mov     al,[bx+cmd]
		cmp     al,cs:[si]
		ja      _IOCommandError
		cbw
		shl     ax,1
		add     si,ax
		xchg    di,ax

		les     di,dword ptr [bx+trans]
		push    ax
		mov     ax,DGROUP
		mov     ds,ax
		pop     ax
		cld
		jmp     word ptr cs:[si+1]

		page
;
; Name:
;	_IOXXXXXXX
;
; Function:
;	Exit routines for internal device drivers.
;
; Description:
;	These routines are the exit for internal device drivers.  _IOSuccess 
;	is for read/write functions and correctly returns for a successful 
;	read/write operation by setting the remainng count to zero.  _IOExit 
;	simply sets success bit and returns.  _IODone returns complete and 
;	busy status.  _IOCommandError returns and error status for invalid 
;	commands.  _IOErrCnt corrects the remaining bytes for errors that 
;	occurred during partial read/write operation.  _IOErrorExit is a 
;	generic error exit that sets done and error.
;
		public	_IOSuccess
_IOSuccess:
		lds	bx,_ReqPktPtr
		xor	ax,ax
		mov	[bx+count],ax

		public	_IOExit
_IOExit		proc    far
		mov     ah,1
_IOExit		endp

StoreStatus     proc    far
		lds     bx,cs:_ReqPktPtr
		mov     [bx+status],ax
		pop     bx
		pop     es
		pop     ds
		pop     bp
		pop     di
		pop     dx
		pop     cx
		pop     ax
		pop     si
		ret
StoreStatus     endp


		public	_IODone
_IODone		proc    far
		mov     ah,3
		jmp     short StoreStatus
_IODone		endp

		public	_IOCommandError
_IOCommandError	proc	near
		mov     al,3
_IOCommandError	endp

		public	_IOErrCnt
_IOErrCnt: 
		lds     bx,cs:_ReqPktPtr
		sub     [bx+count],cx
		public	_IOErrorExit
_IOErrorExit:
		mov     ah,81h
		jmp     short StoreStatus

		page
;
; Name:
;	GetUnitNum
;
; Function:
;	Return the internally set unit number.
;
; Description:
;	Simply return the contents of uUnitNumber.  This version relies on 
;	no segment registers and makes a safe call regardless of driver 
;	state.
;
		public	GetUnitNum
GetUnitNum	proc	near
		mov	dx,cs:uUnitNumber
		ret
GetUnitNum	endp

		page
		;
		; These are still old style DOS-C drivers.  I'll replace 
		; them in the next release
		;


		;
		; block device interrupt
		;
		; NOTE: This code is not standard device driver handlers
		; It is written for sperate code and data space.
		;
blk_entry       proc    far
		pushf
		push    ax
		push    bx
		push    cx
		push    dx
		push    bp
		push    si
		push    di
		push    ds
		push    es

		; small model
		mov     ax,DGROUP                       ; correct for segments
		mov     ds,ax                           ; ax to carry segment
		mov     word ptr DGROUP:blk_dos_stk,sp  ; use internal stack
		mov     word ptr DGROUP:blk_dos_seg,ss
		pushf                                   ; put flags in bx
		pop     bx
		cli                                     ; no interrupts
		mov     ss,ax
		mov     sp,offset DGROUP:blk_stk_top
		push    bx
		popf                                    ; restore interrupt flag
		mov     bp,sp                           ; make a c frame
		push    word ptr _ReqPktPtr+2
		push    word ptr _ReqPktPtr
		call    _blk_driver
		pop     cx
		pop     cx
		les     bx,dword ptr _ReqPktPtr         ; now return completion code
		mov     word ptr es:[bx].status,ax      ; mark operation complete
		cli                                     ; no interrupts
		mov     sp,word ptr DGROUP:blk_dos_stk  ; use dos stack
		mov     ss,word ptr DGROUP:blk_dos_seg
		pop     es
		pop     ds
		pop     di
		pop     si
		pop     bp
		pop     dx
		pop     cx
		pop     bx
		pop     ax
		popf
		ret     
blk_entry       endp


		page

		;
		; clock device interrupt
		;
		; NOTE: This code is not standard device driver handlers
		; It is written for sperate code and data space.
		;
clk_entry       proc    far
		pushf
		push    ax
		push    bx
		push    cx
		push    dx
		push    bp
		push    si
		push    di
		push    ds
		push    es

		; small model
		mov     ax,DGROUP                       ; correct for segments
		mov     ds,ax                           ; ax to carry segment
		mov     word ptr DGROUP:clk_dos_stk,sp  ; use internal stack
		mov     word ptr DGROUP:clk_dos_seg,ss
		pushf                                   ; put flags in bx
		pop     bx
		cli                                     ; no interrupts
		mov     ss,ax
		mov     sp,offset DGROUP:clk_stk_top
		push    bx
		popf                                    ; restore interrupt flag
		mov     bp,sp                           ; make a c frame
		push    word ptr _ReqPktPtr+2
		push    word ptr _ReqPktPtr
		call    _clk_driver
		pop     cx
		pop     cx
		les     bx,dword ptr _ReqPktPtr         ; now return completion code
		mov     word ptr es:[bx].status,ax      ; mark operation complete
		cli                                     ; no interrupts
		mov     sp,word ptr DGROUP:clk_dos_stk  ; use dos stack
		mov     ss,word ptr DGROUP:clk_dos_seg
		pop     es
		pop     ds
		pop     di
		pop     si
		pop     bp
		pop     dx
		pop     cx
		pop     bx
		pop     ax
		popf
		ret     
clk_entry       endp
_IO_TEXT        ends

		end


