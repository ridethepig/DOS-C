;
; File:
;                          console.asm
; Description:
;                      Console device driver
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
		title   Console device driver

		include	io.inc


_BSS           segment word public 'BSS'
uScanCode       db      (?)			; Scan code for con: device
_BSS           ends

_IO_FIXED_DATA  segment word public 'CODE'
		assume cs:TGROUP

		public	ConTable
ConTable        db      0Ah
		dw      offset _IOExit
		dw      offset _IOExit
		dw      offset _IOExit
		dw      offset _IOCommandError
		dw      offset ConRead
		dw      offset CommonNdRdExit
		dw      offset _IOExit
		dw      offset ConInpFlush
		dw      offset ConWrite
		dw      offset ConWrite
		dw      offset _IOExit
_IO_FIXED_DATA  ends

PRT_SCREEN	equ	7200h
CTL_P		equ	10h

_IO_TEXT        segment byte public 'CODE'
		assume cs:TGROUP

		page
;
; Name:
;	ConRead
;
; Function:
;	Read to address in es:di characters from the keyboard.  Cx contains 
;	a count of how many characters are to be transferred.
;
; Description:
;	Calls KbdRdChar to read the characters.  Destroys ax.
;
		public	ConRead
ConRead         proc    far
		jcxz    ConRead2                ; Exit if read of zero

ConRead1:
		call    KbdRdChar               ; Get a char from kbd in al
		stosb                           ; Store al to es:[di]
		loop    ConRead1                ; Loop until all are read

ConRead2:
		jmp     _IOExit
ConRead         endp


		page
;
; Name:
;	KbdRdChar
;
; Function:
;	Read a character from the keyboard.
;
; Description:
;	This subroutine reads a character fromthe keyboard.  It also handles 
;	a couple of special functions.  It converts the print screen key to 
;	a control-P.  It also accounts for extended scan codes by saving off 
;	the high byte of the return and returning it if it was non-zero on 
;	the previous read.
;
		public	KbdRdChar
KbdRdChar       proc    near
		xor     ax,ax                   ; Zero the scratch register
		xchg    byte ptr ds:[uScanCode],al ; and swap with scan code
		or      al,al                   ; Test to see if it was set
		jnz     KbdRdRtn                ; Exit if it was, returning it
		int     16h                     ; get keybd char in al, ah=scan
		or      ax,ax                   ; Zero ?
		jz      KbdRdChar               ; Loop if it is
		cmp     ax,PRT_SCREEN		; Print screen?
		jne     KbdRd1                  ; Nope, keep going
		mov     al,CTL_P			; Yep, make it ^P
KbdRd1:
		or      al,al                   ; Extended key?
		jnz     KbdRdRtn                ; Nope, just exit
		mov     byte ptr ds:[uScanCode],ah ; Yep, save the scan code
KbdRdRtn:
		retn
KbdRdChar       endp



		public	CommonNdRdExit
CommonNdRdExit       proc    far
		mov     al,uScanCode            ; Test for last scan code
		or      al,al                   ; Was it zero ?
		jnz     ConNdRd2                ; Jump if there's a char waiting
		mov     ah,1
		int     16h                     ; Get status, if zf=0  al=char
		jz      ConNdRd4                ; Jump if chrar available
		or      ax,ax                   ; Zero ?
		jnz     ConNdRd1                ; Jump if not zero
		int     16h                     ; get status, if zf=0  al=char
		jmp     short CommonNdRdExit

ConNdRd1:
		cmp     ax,PRT_SCREEN		; Was print screen key pressed?
		jne     ConNdRd2                ; Jump if not
		mov     al,CTL_P

ConNdRd2:
		lds     bx,_ReqPktPtr		; Set the status
		mov     [bx+0Dh],al

ConNdRd3:
		jmp     _IOExit

ConNdRd4:
		jmp     _IODone
CommonNdRdExit       endp



		public	ConInpFlush
ConInpFlush     proc    far
		call    KbdInpChar
		jmp     _IOExit
ConInpFlush     endp



KbdInpChar      proc    near
		mov     byte ptr ds:[uScanCode],0
KbdInpCh1:
		mov     ah,1
		int     16h                     ; get status, if zf=0  al=char
		jz      KbdInpRtn               ; Jump if zero
		xor     ah,ah                   ; Zero register
		int     16h                     ; get keybd char in al, ah=scan
		jmp     short KbdInpCh1
KbdInpRtn:
		retn
KbdInpChar      endp


		public	ConWrite
ConWrite        proc    far
		jcxz    ConNdRd3                ; Exit if nothing to write
ConWr1:
		mov     al,es:[di]
		inc     di
		int     29h                     ; Do fast output call
		loop    ConWr1                  ; Loop if more to output
		jmp     _IOExit
ConWrite        endp

CBreak:
		mov     cs:uScanCode,3          ; Put a ^C into the buffer
IntRetn:
		iret

		public  _int29_handler
_int29_handler  proc    far
		push    ax
		push    si
		push    di
		push    bp
		push    bx
		mov     ah,0Eh
		mov     bh,0
		mov     bl,7
		int     10h                     ; write char al, teletype mode
		pop     bx
		pop     bp
		pop     di
		pop     si
		pop     ax
		iret
_int29_handler  endp

		
		public  _break_handler
_break_handler  proc    near
		ret
_break_handler  endp


_IO_TEXT        ends

		end

