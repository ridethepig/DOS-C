; File:
;                         irqstack.asm
; Description:
;     Assembly support routines for hardware stack support
;
;                    Copyright (c) 1997, 1998
;                          Svante Frey
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
; $Logfile:   C:/usr/patv/dos-c/src/kernel/irqstack.asv  $
;
; $Header:   C:/usr/patv/dos-c/src/kernel/irqstack.asv   1.2   06 Dec 1998  8:49:08   patv  $
;
; $Log:   C:/usr/patv/dos-c/src/kernel/irqstack.asv  $
;  
;     Rev 1.2   06 Dec 1998  8:49:08   patv
;  Bug fixes.
;  
;     Rev 1.1   22 Jan 1997 13:15:34   patv
;  pre-0.92 Svante Frey bug fixes
;  
;     Rev 1.0   16 Jan 1997 21:43:44   patv
;  Initial revision.
; $EndLog$
;

		page    60,132
		title   Assembly support routines for hardware stack support

;       Code for stack switching during hardware interrupts.

_TEXT           segment WORD PUBLIC 'CODE'
		assume  cs:_TEXT

old_vectors     dd      16 dup(?)
stack_size      dw      ?
stack_top       dw      ?
stack_offs      dw      ?
stack_seg       dw      ?

irq_0:          push    bx
		mov     bx, 0 * 4
		jmp     short general_irq_service

irq_1:          push    bx
		mov     bx, 1 * 4
		jmp     short general_irq_service

irq_2:          push    bx
		mov     bx, 2 * 4
		jmp     short general_irq_service

irq_3:          push    bx
		mov     bx, 3 * 4
		jmp     short general_irq_service

irq_4:          push    bx
		mov     bx, 4 * 4
		jmp     short general_irq_service

irq_5:          push    bx
		mov     bx, 5 * 4
		jmp     short general_irq_service

irq_6:          push    bx
		mov     bx, 6 * 4
		jmp     short general_irq_service

irq_7:          push    bx
		mov     bx, 7 * 4
		jmp     short general_irq_service

irq_08:         push    bx
		mov     bx, 8 * 4
		jmp     short general_irq_service

irq_09:         push    bx
		mov     bx, 9 * 4
		jmp     short general_irq_service

irq_0a:         push    bx
		mov     bx, 0ah * 4
		jmp     short general_irq_service

irq_0b:         push    bx
		mov     bx, 0bh * 4
		jmp     short general_irq_service

irq_0c:         push    bx
		mov     bx, 0ch * 4
		jmp     short general_irq_service

irq_0d:         push    bx
		mov     bx, 0dh * 4
		jmp     short general_irq_service

irq_0e:         push    bx
		mov     bx, 0eh * 4
		jmp     short general_irq_service

irq_0f:         push    bx
		mov     bx, 0fh * 4
;                jmp     short general_irq_service

General_irq_service:
		push    dx
		push    ax
		push    ds

		mov     ax, cs
		mov     ds, ax
		
		mov     ax, stack_top
		cmp     ax, stack_offs
		jbe     dont_switch

		mov     dx, ss          
		mov     ax, sp

		mov     ss, stack_seg
		mov     sp, stack_top

		push    dx              ; save old SS:SP on new stack
		push    ax

		mov     ax, stack_size
		sub     stack_top, ax
		
		pushf
		call    old_vectors[bx]

		cli
		add     stack_top, ax

		pop     ax              ; get stored SS:SP
		pop     dx
		
		mov     ss, dx          ; switch back to old stack
		mov     sp, ax

		pop     ds              ; restore registers and return
		pop     ax              
		pop     dx          
		pop     bx
		iret

dont_switch:    pushf
		call    dword ptr old_vectors[bx]
		pop     ds
		pop     ax
		pop     dx
		pop     bx
		iret

public  _init_stacks
; VOID    init_stacks(VOID FAR *stack_base, COUNT nStacks, WORD stackSize);

_init_stacks    proc    near
		push    bp
		mov     bp, sp
		push    ds
		push    di
		push    si
		

		push    cs
		pop     ds

		mov     bx, [bp+4]
		mov     dx, [bp+6]
		mov     ax, [bp+8]
		mov     cx, [bp+0ah]

		mov     stack_size, cx
		mov     stack_offs, bx
		mov     stack_seg, dx

		mul     cx
		add     ax, bx
		mov     stack_top, ax

		xor     ax, ax
		mov     ds, ax
		
		push    cs
		pop     es

		mov     di, offset old_vectors
		mov     si, 8 * 4
		mov     cx, 10h
		rep     movsw

		mov     si, 70h * 4
		mov     cx, 10h
		rep     movsw
		
		push    ds
		pop     es
	       
		mov     di, 8 * 4
		mov     dx, offset irq_0
		call    set_vect

		mov     di, 70h * 4
		call    set_vect

		pop     si
		pop     di
		pop     ds
		pop     bp
		ret
_init_stacks    endp
		
set_vect        proc
		mov     cx, 8
		
set_next:       mov     ax, dx
		cli
		stosw
		mov     ax, cs
		stosw
		sti
		add     dx, offset irq_1 - offset irq_0
		loop    set_next

		ret
set_vect        endp

_TEXT           ENDS

		end

