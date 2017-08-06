; File:
;                         nls.asm
; Description:
;     Assembly support routines for nls functions.
;
;                    Copyright (c) 1995, 1998
;                       Pasquale J. Villani
;                       All Rights Reserved
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
; $Logfile:   C:/usr/patv/dos-c/src/kernel/nlssupt.asv  $
;
; $Header:   C:/usr/patv/dos-c/src/kernel/nlssupt.asv   1.3   06 Dec 1998  8:46:56   patv  $
;
; $Log:   C:/usr/patv/dos-c/src/kernel/nlssupt.asv  $
;  
;     Rev 1.3   06 Dec 1998  8:46:56   patv
;  Bug fixes.
;  
;     Rev 1.2   16 Jan 1997 12:46:44   patv
;  pre-Release 0.92 feature additions
;  
;     Rev 1.1   29 May 1996 21:03:38   patv
;  bug fixes for v0.91a
;  
;     Rev 1.0   19 Feb 1996  3:24:04   patv
;  Added NLS, int2f and config.sys processing
; $EndLog$
;

		page    60,132
		title   Assembly support routines for nls functions.

IFDEF ??version
_TEXT           segment byte public 'CODE'
DGROUP          group   _DATA,_BSS,_BSSEND              ; small model
		assume  cs:_TEXT,ds:DGROUP,ss:DGROUP
_TEXT           ends

_DATA           segment word public 'DATA'
_DATA           ends

_BSS            segment word public 'BSS'
_BSS            ends

_BSSEND         segment byte public 'STACK'
_BSSEND         ends

ELSE
_TEXT           segment byte public 'CODE'
_TEXT           ends

_DATA           segment word public 'DATA'
_DATA           ends

CONST           segment word public 'CONST'
CONST           ends

_BSS            segment word public 'BSS'
_BSS            ends

_BSSEND         segment byte public 'STACK'
_BSSEND         ends

DGROUP          group   CONST,_DATA,_BSS,_BSSEND        ; small/tiny model
		assume  ds:DGROUP, ss:DGROUP
ENDIF

_TEXT           segment
		assume  cs: _TEXT
		public  _CharMapSrvc
		extrn   _internalUpcase:near
;
; CharMapSrvc:
;       User callable character mapping service.
;       Part of Function 38h
;
_CharMapSrvc    proc    far
		push    ds
		push    es
		push    bp
		push    si
		push    di
		push    dx
		push    cx
		push    bx
		
		mov     dx, seg DGROUP
		mov     ds, dx
		
		push    ax
		call    _internalUpcase
		add     sp, 2 
		
		pop     bx
		pop     cx
		pop     dx
		pop     di
		pop     si
		pop     bp
		pop     es
		pop     ds
		retf                            ; Return far
_CharMapSrvc    endp


_TEXT           ends

		end

