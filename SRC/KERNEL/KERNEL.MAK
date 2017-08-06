#
# Makefile for Borland C++ 3.1 for kernel.exe
#
# $Header:   C:/usr/patv/dos-c/src/kernel/kernel.mav   1.9   06 Dec 1998  8:45:40   patv  $
#
# $Log:   C:/usr/patv/dos-c/src/kernel/kernel.mav  $
#
#   Rev 1.9   06 Dec 1998  8:45:40   patv
#Added new files for I/O subsystem.
#
#   Rev 1.8   22 Jan 1998 14:50:06   patv
#Outdated stacks.asm.
#
#   Rev 1.6   03 Jan 1998  8:36:50   patv
#Converted data area to SDA format
#
#   Rev 1.5   30 Jan 1997  7:55:54   patv
#Added TSC flag for trace support.
#
#   Rev 1.4   16 Jan 1997 12:46:42   patv
#pre-Release 0.92 feature additions
#
#   Rev 1.3   29 Aug 1996 13:07:34   patv
#Bug fixes for v0.91b
#
#   Rev 1.2   29 May 1996 21:03:32   patv
#bug fixes for v0.91a
#
#   Rev 1.1   19 Feb 1996  3:35:38   patv
#Added NLS, int2f and config.sys processing
#
#   Rev 1.0   02 Jul 1995  8:30:22   patv
#Initial revision.
#
# $EndLog$
#

RELEASE = 1.00

.AUTODEPEND

#
# Compiler and Options for Borland C++
# ------------------------------------
CC = bcc +kernel.cfg
ASM = TASM
LIB = TLIB
LINK = TLINK
LIBPATH = .
INCLUDEPATH = ..\HDR
CFLAGS          = -v -X -I. -D__STDC__=0 -DTSC -DDEBUG -DKERNEL -DI86 -DPROTO -DSHWR -DASMSUPT
AFLAGS          = /Mx /Zi /DSTANDALONE=1 /I..\..\HDR
LIBS            =..\..\LIB\DEVICE.LIB ..\..\LIB\LIBM.LIB

# where to copy source from
FSSRC = \
  ..\fs\fatfs.c \
  ..\fs\fatdir.c \
  ..\fs\fattab.c \
  ..\fs\dosfns.c \
  ..\fs\fcbfns.c \
  ..\fs\error.c
SUPTSRC = \
  ..\fs\prf.c \
  ..\fs\misc.c \
  ..\fs\dosnames.c \
  ..\fs\syspack.c
IOSRC = \
  ..\fs\blockio.c \
  ..\fs\chario.c

# what to delete when cleaning
COPIEDSRCA = \
  fatfs.c \
  fatdir.c \
  fattab.c \
  dosfns.c \
  fcbfns.c
COPIEDSRCB = \
  error.c \
  prf.c \
  misc.c \
  dosnames.c \
  syspack.c \
  blockio.c \
  chario.c

#               *Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#               *List Macros*


EXE_dependencies =  \
 kernel.obj \
 entry.obj \
 io.obj \
 blockio.obj \
 chario.obj \
 dosfns.obj \
 console.obj \
 printer.obj \
 serial.obj \
 dsk.obj \
 error.obj \
 fatdir.obj \
 fatfs.obj \
 fattab.obj \
 fcbfns.obj \
 initoem.obj \
 inthndlr.obj \
 irqstack.obj \
 ioctl.obj \
 main.obj \
 config.obj \
 memmgr.obj \
 misc.obj \
 newstuff.obj \
 nls.obj \
 dosnames.obj \
 prf.obj \
 strings.obj \
 sysclk.obj \
 syspack.obj \
 systime.obj \
 task.obj \
 int2f.obj \
 apisupt.obj \
 asmsupt.obj \
 execrh.obj \
 nlssupt.obj \
 procsupt.obj

#               *Explicit Rules*
all:            production

production:     populate kernel.exe
		tdstrip kernel.exe
		copy kernel.exe ..\..\dist
		del *.obj
		del kernel.exe

populate:       $(FSSRC) $(SUPTSRC) $(IOSRC)
		release $(RELEASE)
		..\utils\pop $(FSSRC)
		..\utils\pop $(SUPTSRC)
		..\utils\pop $(IOSRC)

clobber:        clean
		del kernel.exe

clean:
		..\utils\rmfiles $(COPIEDSRCA)
		..\utils\rmfiles $(COPIEDSRCB)
		del *.obj
		del *.bak
		del *.crf
		del *.xrf
		del *.map
		del *.lst
		del *.las

kernel.exe: kernel.cfg $(EXE_dependencies)
  $(LINK) /m/v/c/P-/L$(LIBPATH) @&&|
kernel.obj+
entry.obj+
io.obj+
blockio.obj+
chario.obj+
dosfns.obj+
console.obj+
printer.obj+
serial.obj+
dsk.obj+
error.obj+
fatdir.obj+
fatfs.obj+
fattab.obj+
fcbfns.obj+
initoem.obj+
inthndlr.obj+
ioctl.obj+
main.obj+
config.obj+
memmgr.obj+
misc.obj+
newstuff.obj+
nls.obj+
dosnames.obj+
prf.obj+
strings.obj+
sysclk.obj+
syspack.obj+
systime.obj+
task.obj+
int2f.obj+
irqstack.obj+
apisupt.obj+
asmsupt.obj+
execrh.obj+
nlssupt.obj+
procsupt.obj
kernel          # exe file
kernel          # map file
$(LIBS)
|


#               *Individual File Dependencies*
kernel.obj: kernel.cfg kernel.asm segs.inc
	$(ASM) $(AFLAGS) kernel,,;

console.obj: kernel.cfg console.asm 
	$(ASM) $(AFLAGS) console,,;

printer.obj: kernel.cfg printer.asm 
	$(ASM) $(AFLAGS) printer,,;

serial.obj: kernel.cfg serial.asm 
	$(ASM) $(AFLAGS) serial,,;

entry.obj: kernel.cfg entry.asm 
	$(ASM) $(AFLAGS) entry,,;

blockio.obj: kernel.cfg blockio.c globals.h

chario.obj: kernel.cfg chario.c globals.h

dosfns.obj: kernel.cfg dosfns.c globals.h

dsk.obj: kernel.cfg dsk.c globals.h

error.obj: kernel.cfg error.c globals.h

fatdir.obj: kernel.cfg fatdir.c globals.h

fatfs.obj: kernel.cfg fatfs.c globals.h

fattab.obj: kernel.cfg fattab.c globals.h

fcbfns.obj: kernel.cfg fcbfns.c globals.h

initoem.obj: kernel.cfg initoem.c globals.h

inthndlr.obj: kernel.cfg inthndlr.c globals.h

ioctl.obj: kernel.cfg ioctl.c globals.h

main.obj: kernel.cfg main.c globals.h

config.obj: kernel.cfg config.c globals.h

memmgr.obj: kernel.cfg memmgr.c globals.h

misc.obj: kernel.cfg misc.c globals.h

nls.obj: kernel.cfg nls.c globals.h

newstuff.obj: kernel.cfg newstuff.c globals.h

dosnames.obj: kernel.cfg dosnames.c globals.h

prf.obj: kernel.cfg prf.c globals.h

strings.obj: kernel.cfg strings.c globals.h

sysclk.obj: kernel.cfg sysclk.c globals.h

syspack.obj: kernel.cfg syspack.c globals.h

systime.obj: kernel.cfg systime.c globals.h

task.obj: kernel.cfg task.c globals.h

apisupt.obj: kernel.cfg apisupt.asm 
	$(ASM) $(AFLAGS) APISUPT,,;

asmsupt.obj: kernel.cfg asmsupt.asm 
	$(ASM) $(AFLAGS) ASMSUPT,,;

execrh.obj: kernel.cfg execrh.asm 
	$(ASM) $(AFLAGS) EXECRH,,;

int2f.obj: kernel.cfg int2f.asm 
	$(ASM) $(AFLAGS) int2f,,;

io.obj: kernel.cfg io.asm segs.inc
	$(ASM) $(AFLAGS) io,,;

irqstack.obj: kernel.cfg irqstack.asm 
	$(ASM) $(AFLAGS) irqstack.asm,,;

nlssupt.obj: kernel.cfg nlssupt.asm 
	$(ASM) $(AFLAGS) nlssupt,,;

procsupt.obj: kernel.cfg procsupt.asm 
	$(ASM) $(AFLAGS) PROCSUPT,,;

proto.h:        $(CSRC) $(LIBCSRC1) $(LIBCSRC2) $(LIBCSRC3) $(LIBCSRC4)
		echo /* proto.h generated by make */ > proto.h
		echo #undef _P >> proto.h
		..\utils\proto $(CSRC)
		..\utils\proto $(LIBCSRC1)
		..\utils\proto $(LIBCSRC2)
		..\utils\proto $(LIBCSRC3)
		..\utils\proto $(LIBCSRC4)
		echo Don't forget to edit proto.h for version control

#               *Compiler Configuration File*
kernel.cfg: kernel.mak
  copy &&|
-f-
-ff-
-O
-Oe
-Ob
-Z
-k-
-vi-
-wpro
-weas
-wpre
$(CFLAGS)
| kernel.cfg


