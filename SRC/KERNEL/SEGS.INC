; File:
;                           segs.inc
; Description:
;     Segment definitions for the kernel
;
;                       Copyright (c) 1998
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
; $Logfile:   C:/usr/patv/dos-c/src/kernel/segs.inv  $
;
; $Header:   C:/usr/patv/dos-c/src/kernel/segs.inv   1.0   06 Dec 1998  8:12:36   patv  $
;
; $Log:   C:/usr/patv/dos-c/src/kernel/segs.inv  $
; 
;    Rev 1.0   06 Dec 1998  8:12:36   patv
; Initial revision.
; $EndLog
;

IFDEF ??version
_TEXT           segment byte public 'CODE'
TGROUP          group   _TEXT,_IO_TEXT,_IO_FIXED_DATA
DGROUP          group   _FIXED_DATA,_DATA,_BSS,_BSSEND  ; small model
		assume  cs:TGROUP,ds:DGROUP,ss:DGROUP
_TEXT           ends

_IO_TEXT        segment byte public 'CODE'
_IO_TEXT        ends

_IO_FIXED_DATA  segment word public 'CODE'
_IO_FIXED_DATA  ends

_FIXED_DATA     segment para public 'DATA'
_FIXED_DATA     ends

_DATA           segment word public 'DATA'
_DATA           ends

_BSS            segment word public 'BSS'
_BSS            ends

_BSSEND         segment byte public 'STACK'
_BSSEND         ends

ELSE
_TEXT           segment byte public 'CODE'
_TEXT           ends

_IO_TEXT        segment byte public 'CODE'
_IO_TEXT        ends

_IO_FIXED_DATA  segment word public 'CODE'
_IO_FIXED_DATA  ends

_FIXED_DATA     segment para public 'DATA'
_FIXED_DATA     ends

_DATA           segment word public 'DATA'
_DATA           ends

CONST           segment word public 'CONST'
CONST           ends

_BSS            segment word public 'BSS'
_BSS            ends

_BSSEND         segment byte public 'STACK'
_BSSEND         ends

TGROUP          group   _TEXT,_IO_TEXT,_IO_FIXED_DATA
		assume  cs:TGROUP

DGROUP          group   CONST,_DATA,_BSS,_BSSEND        ; small/tiny model
		assume  ds:DGROUP, ss:DGROUP
ENDIF
