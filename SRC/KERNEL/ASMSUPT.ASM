; File:
;			  asmsupt.asm
; Description:
;	Assembly support routines for miscellaneous functions
;
;		     Copyright (c) 1995, 1998
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
; $Logfile:   C:/usr/patv/dos-c/src/kernel/asmsupt.asv  $
;
; $Header:   C:/usr/patv/dos-c/src/kernel/asmsupt.asv   1.4   06 Dec 1998  8:46:50   patv  $
;
; $Log:   C:/usr/patv/dos-c/src/kernel/asmsupt.asv  $
; 
;    Rev 1.4   06 Dec 1998  8:46:50   patv
; Bug fixes.
; 
;    Rev 1.3   03 Jan 1998  8:36:44   patv
; Converted data area to SDA format
; 
;    Rev 1.2   29 May 1996 21:03:38   patv
; bug fixes for v0.91a
; 
;    Rev 1.1   01 Sep 1995 17:54:26   patv
; First GPL release.
; 
;    Rev 1.0   05 Jul 1995 11:38:42   patv
; Initial revision.
; $EndLog$
;

		page	60,132
		title	Assembly support routines for miscellaneous functions

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


_TEXT		segment	byte public 'CODE'
;
;	VOID bcopy(s, d, n)
;	REG BYTE *s, *d;
;	REG COUNT n;
;
;
		assume	cs:_TEXT
		public	_bcopy
_bcopy		proc	near
		push	bp			; Standard C entry
		mov	bp,sp
		push	si
		push	di
		push	es

		; Get the repitition count, n
		mov	cx,[bp+8]
		or	cx,cx
		jz	short bcopy_exit

		; Set both ds and es to same segment (for near copy)
		mov		ax,ds
		mov		es,ax

		; Get the source pointer, ss
		mov		si,[bp+4]

		; and the destination pointer, d
		mov		di,[bp+6]

		; Set direction to autoincrement
		cld	

		; And do the built-in byte copy, but do a 16-bit transfer
		; whenever possible.
		push	cx
		and	cx,1		; test for odd count
		pop cx
		jz	b_even
		movsb	
b_even:		shr	cx,1
		rep	movsw

		; Finally do a C exit to return
bcopy_exit:	pop	es
		pop	di
		pop	si
		pop	bp
		ret	
_bcopy		endp


;	
;	VOID fbcopy(s, d, n)
;	
;	REG VOID FAR *s, FAR *d;
;	REG COUNT n;
		assume	cs:_TEXT
		public	_fbcopy
_fbcopy		proc	near
		push	bp		; Standard C entry
		mov	bp,sp
		push	si
		push	di

		; Save ds, since we won't necessarily be within our
		; small/tiny environment
		push	ds
		push	es

		; Get the repititon count, n
		mov	cx,[bp+12]
		or	cx,cx
		jz	short fbcopy_exit

		; Get the far source pointer, s
		lds	si,DWORD PTR [bp+4]

		; Get the far destination pointer d
		les	di,DWORD PTR [bp+8]

		; Set direction to autoincrement
		cld	

		; And do the built-in byte copy, but do a 16-bit transfer
		; whenever possible.
		push	cx
		and	cx,1		; test for odd count
		pop cx
		jz	fb_even
		movsb	
fb_even:	shr	cx,1
		rep	movsw	

		; recover ds
fbcopy_exit:	pop	es
		pop	ds

		; Do a standard C exit and return
		pop	di
		pop	si
		pop	bp
		ret	
_fbcopy		endp
_TEXT		ENDS
		end

