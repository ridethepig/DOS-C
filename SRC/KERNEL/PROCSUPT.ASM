;
; File:
;			  procsupt.asm
; Description:
;     Assembly support routines for process handling, etc.
;
;		      Copyright (c) 1995,1998
;			Pasquale J. Villani
;			All Rights Reserved
;
; This file is part of DOS-C.
;
; DOS-C is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version
; 2, or (at your option) any later version.
;
; DOS-C is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
; the GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public
; License along with DOS-C; see the file COPYING.  If not,
; write to the Free Software Foundation, 675 Mass Ave,
; Cambridge, MA 02139, USA.
;
; $Logfile:   C:/usr/patv/dos-c/src/kernel/procsupt.asv  $
;
; $Header:   C:/usr/patv/dos-c/src/kernel/procsupt.asv   1.4   06 Dec 1998  8:46:44   patv  $
;
; $Log:   C:/usr/patv/dos-c/src/kernel/procsupt.asv  $
;
;   Rev 1.4   06 Dec 1998  8:46:44   patv
;Bug fixes.
;
;   Rev 1.3   07 Feb 1998 20:42:08   patv
;Modified stack fram to match DOS standard
;
;   Rev 1.2   29 May 1996 21:03:36   patv
;bug fixes for v0.91a
;
;   Rev 1.1   01 Sep 1995 17:54:24   patv
;First GPL release.
;
;   Rev 1.0   02 Jul 1995  9:05:58   patv
;Initial revision.
; $EndLog$
;

		page	60,132
		title	Assembly support routines for process handling, etc.

IFDEF ??version
_TEXT		segment	byte public 'CODE'
DGROUP		group	_DATA,_BSS,_BSSEND		; small model
		assume	cs:_TEXT,ds:DGROUP,ss:DGROUP
_TEXT		ends

_DATA		segment word public 'DATA'
_DATA		ends

_BSS		segment word public 'BSS'
_BSS		ends

_BSSEND		segment byte public 'STACK'
_BSSEND		ends

ELSE
_TEXT		segment	byte public 'CODE'
_TEXT		ends

_DATA		segment word public 'DATA'
_DATA		ends

CONST		segment word public 'CONST'
CONST		ends

_BSS		segment word public 'BSS'
_BSS		ends

_BSSEND		segment byte public 'STACK'
_BSSEND		ends

DGROUP		group	CONST,_DATA,_BSS,_BSSEND	; small/tiny model
		assume	ds:DGROUP, ss:DGROUP
ENDIF


		extrn	_api_sp:word		; api stacks - for context
		extrn	_api_ss:word		; switching
		extrn	_usr_sp:word		; user stacks
		extrn	_usr_ss:word

		extrn	_kstackp:near		; kernel stack
		extrn	_ustackp:near		; new task stack

		extrn	_break_flg:near		; break detected flag
		extrn	_int21_entry:far	; far call system services

		include	stacks.inc

_TEXT      	segment
		assume	cs: _TEXT

;
;	Special call for switching processes
;
;	void interrupt far exec_user(irp)
;	iregs far *irp;
;
		public	_exec_user
_exec_user	proc far

		PUSH$ALL
		mov	ax,DGROUP
		mov	ds,ax
		mov	bp,sp
		cld
		cli	
;
;
;
		mov	ax,WORD PTR [bp.irp_low]	; irp (user ss:sp)
		mov	dx,WORD PTR [bp.irp_hi]
		mov	sp,ax			; set-up user stack
		mov	ss,dx
		sti
;
		POP$ALL
		iret	

_exec_user		endp





;
;	Special call for switching processes during break handling
;
;	void interrupt far handle_break()
;
;
;	+---------------+
;	|     flags	|	22
;	+---------------+
;	|	cs	|	20
;	+---------------+
;	|	ip	|	18
;	+---------------+
;	|	es	|	16
;	+---------------+
;	|	ds	|	14
;	+---------------+
;	|	bp	|	12
;	+---------------+
;	|	di	|	10
;	+---------------+
;	|	si	|	8
;	+---------------+
;	|	dx	|	6
;	+---------------+
;	|	cx	|	4
;	+---------------+
;	|	bx	|	2
;	+---------------+
;	|	ax	|	0	<--- bp & sp after mov bp,sp
;	+---------------+
;
		public	_handle_break
_handle_break	proc far

		PUSH$ALL
		mov	ax,DGROUP
		mov	ds,ax
		mov	bp,sp
		assume DS: DGROUP
		cld

		; handler body - start out by restoring stack
		pushf
		cli	

		; save background stack
		mov	word ptr DGROUP:_api_ss,ss
		mov	word ptr DGROUP:_api_sp,sp

		; restore foreground stack here
		mov	ss,word ptr DGROUP:_usr_ss
		mov	sp,word ptr DGROUP:_usr_sp

		; get all the user registers back
		POP$ALL

		; do the int 23 handler and see if it returns
		int	23h

		; we're back, must have been users handler
		PUSH$ALL
		mov	bp,sp
		mov	ax,DGROUP
		mov	ds,ax
		assume DS: DGROUP

		; test for far return or iret
		cmp	sp,_usr_sp
		jz	hbrk1		; it was far ret

		; restart int 21 from the top
hbrk1:		POP$ALL
		call	_int21_entry
		iret

hbrk2:		popf			; clear the flag from the stack
		jnc	hbrk1		; user wants to restart
		POP$ALL
		mov	ax,4c00h	; exit
		mov	byte ptr _break_flg,0ffh ; set break detected flag
		call	_int21_entry
		iret

		
_handle_break	endp


;
; interrupt enable and disable routines
;
		public	_enable
_enable		proc near
		sti
		ret
_enable		endp

		public	_disable
_disable	proc near
		cli
		ret
_disable	endp

_TEXT		ends

		end

