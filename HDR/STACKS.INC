;
; File:
;			  stacks.inc
; Description:
;     		Macro support for register stack frame
;
;			Copyright (c) 1998
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
; $Logfile:   C:/dos-c/hdr/stacks.inv  $
;
; $Header:   C:/dos-c/hdr/stacks.inv   1.0   07 Feb 1998 20:59:16   patv  $
;
; $Log:   C:/dos-c/hdr/stacks.inv  $
;
;   Rev 1.0   07 Feb 1998 20:59:16   patv
;Modified stack frame to match DOS standard
; $EndLog$

		page	60,132
		title	Macro support for register stack frame

;
; Standard stack frame used throughout DOS-C
;
;	MS-DOS specific
;
;	+---------------+
;	|    irp hi	|	26
;	+---------------+
;	|    irp low	|	24
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
;	|	ax	|	0
;	+---------------+
;

RegFrame	struc
		reg_ax		dw (?)
		reg_bx		dw (?)
		reg_cx		dw (?)
		reg_dx		dw (?)
		reg_si		dw (?)
		reg_di		dw (?)
		reg_bp		dw (?)
		reg_ds		dw (?)
		reg_es		dw (?)
		reg_ip		dw (?)
		reg_cs		dw (?)
		reg_flags	dw (?)
		irp_low		dw (?)
		irp_hi		dw (?)
		ends


PUSH$ALL	macro
		push	es
		push	ds
		push	bp
		push	di
		push	si
		push	dx
		push	cx
		push	bx
		push	ax
		endm

POP$ALL		macro
		pop	ax
		pop	bx
		pop	cx
		pop	dx
		pop	si
		pop	di
		pop	bp
		pop	ds
		pop	es
		endm

