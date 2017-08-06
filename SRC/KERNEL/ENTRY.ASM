;
; File:
;			   entry.asm
; Description:
;		       System call entry code
;
;			Copyright (c) 1998
;			Pasquale J. Villani
;			All Rights Reserved
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
; $Header:   C:/usr/patv/dos-c/src/kernel/entry.asv   1.1   06 Dec 1998  8:48:40   patv  $
;
; $Log:   C:/usr/patv/dos-c/src/kernel/entry.asv  $
;  
;     Rev 1.1   06 Dec 1998  8:48:40   patv
;  New int 21h handler code.
;  
;     Rev 1.0   07 Feb 1998 20:42:08   patv
;  Modified stack frame to match DOS standard
; $EndLog$

		page	60,132
		title	System call entry code

IFDEF ??version
_TEXT		segment	byte public 'CODE'
DGROUP		group	_FIXED_DATA,_DATA,_BSS,_BSSEND	; small model
		assume	cs:_TEXT,ds:DGROUP,ss:DGROUP
_TEXT		ends

_FIXED_DATA	segment para public 'DATA'
_FIXED_DATA	ends

_DATA		segment word public 'DATA'
_DATA		ends

_BSS		segment word public 'BSS'
_BSS		ends

_BSSEND		segment byte public 'STACK'
_BSSEND		ends

ELSE
_TEXT		segment	byte public 'CODE'
_TEXT		ends

_FIXED_DATA	segment para public 'DATA'
_FIXED_DATA	ends

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

		include	stacks.inc

_TEXT		segment	byte public 'CODE'
		extrn	_int21_syscall:near
		extrn	_int25_handler:NEAR, _int26_handler:NEAR
		extrn	_set_stack:FAR
		extrn	_restore_stack:FAR
		extrn	_error_tos:WORD
		extrn	_char_api_tos:WORD
		extrn	_disk_api_tos:WORD
		extrn	_lpUserStack:DWORD
		extrn	_user_r:DWORD
		extrn	_ErrorMode:byte
		extrn	_InDOS:byte
		extrn	_cu_psp:word
		extrn	_MachineId:word
		extrn	critical_sp:word

		extrn   _api_sp:word            ; api stacks - for context
		extrn   _api_ss:word            ; switching
		extrn   _usr_sp:word            ; user stacks
		extrn   _usr_ss:word
		extrn   int21regs_seg:word
		extrn   int21regs_off:word

		public	_cpm_entry
		public	_int20_handler
		public	_int21_handler
		public	_low_int25_handler
		public	_low_int26_handler
		public	_int27_handler


;
; MS-DOS CP/M style entry point
;
;	VOID FAR 
;	cpm_entry(iregs UserRegs)
;
; This one is a strange one.  The call is to psp:0005h but it returns to the 
; function after the call.  What we do is convert it to a normal call and 
; fudge the stack to look like an int 21h call.
;
		assume	cs:_TEXT
_cpm_entry	proc	far
		; Stack is:
		;	return offset
		;	psp seg
		;	000ah
		;
		push	bp		; trash old return address
		mov	bp,sp
		xchg	bp,2[bp]
		pop	bp
		pushf			; start setting up int 21h stack
		;
		; now stack is
		;	return offset
		;	psp seg
		;	flags
		;
		push	bp
		mov	bp,sp		; set up reference frame
		;
		; reference frame stack is
		;	return offset		bp + 6
		;	psp seg			bp + 4
		;	flags			bp + 2
		;	bp		<---	bp
		;
		push	ax
		mov	ax,2[bp]	; get the flags
		xchg	ax,6[bp]	; swap with return address
		mov	2[bp],ax
		pop	ax		; restore working registers
		pop	bp
		;
		; Done. Stack is
		;	flags
		;	psp seg (alias .COM cs)
		;	return offset
		;
		cmp	cl,024h
		jbe	cpm_error
		mov	ah,cl		; get the call # from cl to ah
		jmp	short _int21_handler	; do the system call
cpm_error:	mov	al,0
		iret
_cpm_entry	endp

;
; Restart the int 21h system call.  Call never returns.
;
;	VOID
;	RestartSysCall(VOID);
;
; NOTE: On exit, DS must point to kernel stack, SS:SP user stack after
; PUSH$ALL and BP == SP.
;
_RestartSysCall	proc	near
		cli			; no interrupts
		mov	bp,word ptr _lpUserStack+2 ;Get frame
		mov	ss,bp
		mov	bp,word ptr _lpUserStack
		mov	sp,bp
		sti
		POP$ALL			; get the original regs
		jmp	short int21_reentry	; restart the system call
_RestartSysCall	endp


;
; Terminate the current process
;
;	VOID INRPT far 
;	int20_handler(iregs UserRegs)
;
		assume	cs:_TEXT
_int20_handler	proc	far
		mov	ah,0		; terminate through int 21h
_int20_handler	endp


;
; MS-DOS system call entry point
;
;	VOID INRPT far 
;	int21_handler(iregs UserRegs)
;
		assume	cs:_TEXT
_int21_handler	proc	far
		;
		; Create the stack frame for C call.  This is done to 
		; preserve machine state and provide a C structure for 
		; access to registers.
		;
		; Since this is an interrupt routine, CS, IP and flags were 
		; pushed onto the stack by the processor, completing the 
		; stack frame.
		;
		; NB: stack frame is MS-DOS dependent and not compatible
		; with compiler interrupt stack frames.
		;
		PUSH$ALL

		;
		; Create kernel refernce frame.
		;
		; NB: At this point, SS != DS and won't be set that way 
		; until later when which stack to run on is determined.
		;
		mov	bp,DGROUP
		mov	ds,bp

		;
		; Now DS is set, let's save our stack for rentry
		;
		mov	bp,ss
		mov	word ptr _lpUserStack+2,bp
		mov	word ptr _user_r+2,bp
		mov	bp,sp
		mov	word ptr _lpUserStack,bp	; store and init
		mov	word ptr _user_r,bp	; store and init

		;
		; Decide which stack to run on.
		;
		; Unlike previous versions of DOS-C, we need to do this here
		; to guarantee the user stack for critical error handling.
		; We need to do the int 24h from this stack location.
		;
		; There are actually four stacks to run on. The first is the 
		; user stack which is determined by system call number in 
		; AH.  The next is the error stack determined by _ErrorMode. 
		; Then there's the character stack also determined by system 
		; call number.  Finally, all others run on the disk stack.
		; They are evaluated in that order.

int21_reentry:	cmp	ah,33h
		je	int21_user
		cmp	ah,50h
		je	int21_user
		cmp	ah,51h
		je	int21_user
		cmp	ah,62h
		jne	int21_1

int21_user:	push	word ptr _user_r+2
		push	word ptr _user_r
		call	near ptr _int21_syscall
		pop	cx
		pop	cx
		jmp	int21_ret

int21_1:	sti
		cmp	_ErrorMode,0
		je	int21_2
		mov	bp,ds
		mov	ss,bp
		mov	bp,offset DGROUP:_error_tos
		mov	sp,bp
		cli
		push	word ptr _user_r+2
		push	word ptr _user_r
		call	near ptr _int21_syscall
		jmp	short int21_exit

int21_2:	inc	byte ptr _InDOS
		cmp	ah,0ch
		jg	int21_3
		mov	bp,ds
		mov	ss,bp
		mov	bp,offset DGROUP:_char_api_tos
		mov	sp,bp
		cli
		push	word ptr _user_r+2
		push	word ptr _user_r
		call	near ptr _int21_syscall
		jmp	short int21_exit

int21_3:	mov	bp,ds
		mov	ss,bp
		mov	bp,offset DGROUP:_disk_api_tos
		mov	sp,bp
		cli
		;
		; Push the far pointer to the register frame for 
		; int21_syscall and remainder of kernel.
		;
		push	word ptr _user_r+2
		push	word ptr _user_r
		call	near ptr _int21_syscall

		;
		; Recover registers from system call.  Registers and flags 
		; were modified by the system call.
		;
int21_exit:	sti
		mov	bp,word ptr _user_r+2
		mov	ss,bp
		mov	bp,word ptr _user_r	; store and init
		mov	sp,bp
		dec	byte ptr _InDOS
		cli
int21_ret:	POP$ALL

		;
		; ... and return.
		;
		iret	
_int21_handler	endp



;
; Terminate the current process
;
;	VOID INRPT far 
;	int27_handler(iregs UserRegs)
;
		assume	cs:_TEXT
_int27_handler	proc	far
		;
		; First convert the memory to paragraphs
		;
		add	dx,0fh		; round up
		rcr	dx,1
		shr	dx,1
		shr	dx,1
		shr	dx,1
		;
		; ... then use the standard system call
		;
		mov	ax,3100h
		jmp	_int21_handler	; terminate through int 21h
_int27_handler	endp

;
; I really do need to get rid of this because it's the only thing stopping 
; us from being ROMABLE.
;
stkframe        dd      ?

_low_int25_handler  proc far
		sti
		pushf
		push    ax
		push    cx
		push    dx
		push    bx
		push    sp
		push    bp
		push    si
		push    di
		push    ds
		push    es
		
		mov     word ptr cs:stkframe, sp     ; save stack frame
		mov     word ptr cs:stkframe+2, ss

		cld
		mov     ax, seg DGROUP
		mov     ds, ax

		mov     word ptr DGROUP:_api_sp, offset DGROUP:_disk_api_tos
		mov     word ptr DGROUP:_api_ss, ds

		call    far ptr _set_stack

		push    word ptr cs:stkframe+2
		push    word ptr cs:stkframe
		call    _int25_handler
		add     sp, 4

		call    far ptr _restore_stack

		pop     es
		pop     ds
		pop     di
		pop     si
		pop     bp
		pop     bx      ; pop off sp value
		pop     bx
		pop     dx
		pop     cx
		pop     ax
		popf
		retf            ; Bug-compatiblity with MS-DOS.
		                ; This function is supposed to leave the original
		                ; flag image on the stack.
_low_int25_handler  endp


_low_int26_handler  proc far
		sti
		pushf
		push    ax
		push    cx
		push    dx
		push    bx
		push    sp
		push    bp
		push    si
		push    di
		push    ds
		push    es
		
		mov     word ptr cs:stkframe, sp     ; save stack frame
		mov     word ptr cs:stkframe+2, ss

		cld
		mov     ax, seg DGROUP
		mov     ds, ax

		mov     word ptr DGROUP:_api_sp, offset DGROUP:_disk_api_tos
		mov     word ptr DGROUP:_api_ss, ds

		call    far ptr _set_stack

		push    word ptr cs:stkframe+2
		push    word ptr cs:stkframe
		call    _int26_handler
		add     sp, 4
		
		call    far ptr _restore_stack

		pop     es
		pop     ds
		pop     di
		pop     si
		pop     bp
		pop     bx      ; pop off sp value
		pop     bx
		pop     dx
		pop     cx
		pop     ax
		popf
		retf            
_low_int26_handler  endp


CONTINUE	equ	00h
RETRY		equ	01h
ABORT		equ	02h
FAIL		equ	03h

OK_IGNORE	equ	20h
OK_RETRY	equ	10h
OK_FAIL		equ	08h

PSP_PARENT	equ	16h
PSP_USERSP	equ	2eh
PSP_USERSS	equ	30h


;
; Default Int 24h handler -- always returns fail
;
		public	_int24_handler
_int24_handler:	mov	al,FAIL
		iret

;
; COUNT
; CriticalError(COUNT nFlag, COUNT nDrive, COUNT nError, struct dhdr FAR *lpDevice);
;
		public	_CriticalError
_CriticalError	proc	near
		;
		; Skip critical error routine if handler is active
		;
		cmp	byte ptr ds:_ErrorMode,0
		je	CritErr05		; Jump if equal

		mov	ax,FAIL
		retn
		;
		; Do local error processing
		;
CritErr05:
		;
		; C Entry
		;
		push	bp
		mov	bp,sp
		push	si
		push	di
		;
		; Get parameters
		;
		mov	ah,byte ptr [bp+4]	; nFlags
		mov	al,byte ptr [bp+6]	; nDrive
		mov	di,word ptr [bp+8]	; nError
		;
		;	make bp:si point to dev header
		;
		mov	si,word ptr [bp+10]	; lpDevice Offset
		mov	bp,word ptr [bp+12]	; lpDevice segment
		;
		; Now save real ss:sp and retry info in internal stack
		;
		cli
		mov	es,ds:_cu_psp
		push	word ptr es:PSP_USERSS
		push	word ptr es:PSP_USERSP
		push	word ptr ds:_MachineId
		push	word ptr ds:int21regs_seg
		push	word ptr ds:int21regs_off
		push	word ptr ds:_api_sp
		push	word ptr ds:_api_ss
		push	word ptr ds:_usr_sp
		push	word ptr ds:_usr_ss
		push	word ptr ds:_user_r+2
		push	word ptr ds:_user_r
		mov	ds:critical_sp,sp
		;
		; do some clean up because user may never return
		;
		inc	byte ptr ds:_ErrorMode
		dec	byte ptr ds:_InDOS
		;
		; switch to user's stack
		;
		mov	ss,es:PSP_USERSS
		mov	sp,es:PSP_USERSP
		;
		; and call critical error handler
		;
		int	24h			; DOS Critical error handler

		;
		; recover context
		;
		cld
		cli
		mov     bp, seg DGROUP
		mov	ds,bp
		mov	ss,bp
		mov	sp,ds:critical_sp
		pop	word ptr ds:_user_r
		pop	word ptr ds:_user_r+2
		pop	word ptr ds:_usr_ss
		pop	word ptr ds:_usr_sp
		pop	word ptr ds:_api_ss
		pop	word ptr ds:_api_sp
		pop	word ptr ds:int21regs_off
		pop	word ptr ds:int21regs_seg
		pop	word ptr ds:_MachineId
		mov	es,ds:_cu_psp
		pop	word ptr es:PSP_USERSP
		pop	word ptr es:PSP_USERSS
		sti				; Enable interrupts
		;
		; clear flags
		;
		mov	byte ptr ds:_ErrorMode,0
		inc	byte ptr ds:_InDOS
		;
		; Check for ignore and force fail if not ok
		cmp	al,CONTINUE
		jne	CritErr10		; not ignore, keep testing
		test	bh,OK_IGNORE
		jnz	CritErr10
		mov	al,FAIL
		;
		; Check for retry and force fail if not ok
		;
CritErr10:
		cmp	al,RETRY
		jne	CritErr20		; not retry, keep testing
		test	bh,OK_RETRY
		jnz	CritErr20
		mov	al,FAIL
		;
		; You know the drill, but now it's different.
		; check for fail and force abort if not ok
		;
CritErr20:
		cmp	al,FAIL
		jne	CritErr30		; not fail, do exit processing
		test	bh,OK_FAIL
		jnz	CritErr30
		mov	al,ABORT
		;
		; OK, if it's abort we do extra processing.  Otherwise just
		; exit.
		;
CritErr30:
		cmp	al,ABORT
		je	CritErrAbort		; process abort

CritErrExit:
		xor	ah,ah			; clear out top for return
		pop	di
		pop	si
		pop	bp
		ret	

		;
		; Abort processing.
		;
CritErrAbort:
		mov	ax,ds:_cu_psp
		mov	es,ax
		cmp	ax,es:PSP_PARENT
		mov	al,FAIL
		jz	CritErrExit
		cli
		mov	bp,word ptr _user_r+2	;Get frame
		mov	ss,bp
		mov	es,bp
		mov	bp,word ptr _user_r
		mov	sp,bp
		mov	byte ptr ds:_ErrorMode,1	; flag abort
		mov	ax,4C00h
		mov	es:reg_ax,ax
		sti
		jmp	int21_reentry		; restart the system call
_CriticalError	endp
_TEXT		ends
		end
