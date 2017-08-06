;
; File:
;                          serial.asm
; Description:
;                      Serial device driver
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
		title   Serial device driver
		include	io.inc

_IO_FIXED_DATA  segment word public 'CODE'
		assume cs:TGROUP

		public	ComTable
ComTable        db      0Ah
		dw      offset _IOExit
		dw      offset _IOExit
		dw      offset _IOExit
		dw      offset _IOCommandError
		dw      offset ComRead
		dw      offset ComNdRead
		dw      offset _IOExit
		dw      offset ComInpFlush
		dw      offset ComWrite
		dw      offset ComWrite
		dw      offset ComOutStat

_IO_FIXED_DATA	ends


_IO_TEXT        segment byte public 'CODE'
		assume cs:TGROUP

		extrn	CommonNdRdExit:near

ComRead		proc	far
		jcxz	ComRd3
		call	GetComStat
		xor	ax,ax
		xchg	[bx],al
		or	al,al
		jnz	ComRd2
ComRd1:
		call	BiosRdCom
ComRd2:
		stosb
		loop	ComRd1

ComRd3:
		jmp	_IOExit
ComRead		endp


BiosRdCom	proc	near
		mov	ah,2
		call	ComIOCall
		test	ah,0Eh
		jz	BiosRdRetn
		add	sp,2
		xor	al,al
		or	al,0B0h
		jmp	_IOErrCnt
BiosRdRetn:
		retn
BiosRdCom	endp



ComNdRead	proc	far
		call	GetComStat
		mov	al,[bx]
		or	al,al
		jnz	ComNdRd1
		call	ComRdStatus
		test	ah,1
		jz	ComNdRtn
		test	al,20h
		jz	ComNdRtn
		call	BiosRdCom
		call	GetComStat
		mov	[bx],al
ComNdRd1:
		jmp	CommonNdRdExit
ComNdRtn:
		jmp	_IODone
ComNdRead	endp


ComOutStat	proc	far
		call	ComRdStatus
		test	al,20h
		jz	ComNdRtn
		test	ah,20h
		jz	ComNdRtn
		jmp	_IOExit
ComOutStat	endp


ComRdStatus	proc	near
		mov	ah,3
		call	ComIOCall
		retn
ComRdStatus	endp


ComIOCall	proc	near
		call	GetUnitNum
		int	14h			; RS-232 get char al, ah=return status
		retn
ComIOCall	endp


ComInpFlush	proc	far
		call	GetComStat
		mov	byte ptr [bx],0
		jmp	_IOExit
ComInpFlush	endp


ComWrite	proc	far
		jcxz	ComRd3
ComWr1:
		mov	al,es:[di]
		inc	di
		mov	ah,1
		call	ComIOCall
		test	ah,80h
		jz	ComWr2
		mov	al,0Ah
		jmp	_IOErrCnt
ComWr2:
		loop	ComWr1
		jmp	_IOExit
ComWrite	endp


GetComStat	proc	near
		call	GetUnitNum
		mov	bx,dx
		add	bx,offset ComStatArray
		retn
GetComStat	endp



_IO_TEXT	ends

_DATA           segment word public 'DATA'

ComStatArray	db	0, 0, 0, 0

_DATA		ends


		end
