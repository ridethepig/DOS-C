rem echo off
rem
rem Build file for dos-c
rem

cd lib
call ..\src\utils\makelibm
cd ..

cd src\drivers
make -f device production
cd ..\..

cd src\boot
make -f boot production
cd ..\..

rem cd src\ipl
rem make -f ipl production
rem cd ..\..

cd src\kernel
make -f kernel production
cd ..\..

cd src\utils
make -f sys production
cd ..\..

cd src\command
make -f command production
make -f help production
cd ..\..

rem $Header:   C:/usr/patv/dos-c/build.bav   1.1   06 Dec 1998  8:52:26   patv  $
rem
rem $Log:   C:/usr/patv/dos-c/build.bav  $
rem  
rem     Rev 1.1   06 Dec 1998  8:52:26   patv
rem  Temporarily eliminated ipl from build.
rem  
rem     Rev 1.0   05 Jul 1995 18:44:06   patv
rem  Initial revision.
rem
