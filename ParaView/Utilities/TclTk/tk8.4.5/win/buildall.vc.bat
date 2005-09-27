@echo off

::  This is an example batchfile for building everything. Please
::  edit this (or make your own) for your needs and wants using
::  the instructions for calling makefile.vc found in makefile.vc
::
::  RCS: @(#) Id

echo Sit back and have a cup of coffee while this grinds through ;)
echo You asked for *everything*, remember?
echo.

title Building Tk, please wait...

if "%MSVCDir%" == "" call c:\dev\devstudio60\vc98\bin\vcvars32.bat
::if "%MSVCDir%" == "" call "C:\Program Files\Microsoft Developer Studio\vc98\bin\vcvars32.bat"

set INSTALLDIR=C:\Program Files\Tcl

:: Where is the Tcl source directory?
set TCLDIR=..\..\tcl_head_stock
::set TCLDIR=..\..\tcl8.4

:: Build the normal stuff along with the help file.
::
nmake -nologo -f makefile.vc release winhelp OPTS=none
if errorlevel 1 goto error

:: Build the static core, dlls and shell.
::
nmake -nologo -f makefile.vc release OPTS=static
if errorlevel 1 goto error

:: Build the special static libraries that use the dynamic runtime.
::
nmake -nologo -f makefile.vc core OPTS=static,msvcrt
if errorlevel 1 goto error

:: Build the core and shell for thread support.
::
nmake -nologo -f makefile.vc release OPTS=threads
if errorlevel 1 goto error

:: Build a static, thread support core library (no shell).
::
nmake -nologo -f makefile.vc core OPTS=static,threads
if errorlevel 1 goto error

:: Build the special static libraries the use the dynamic runtime,
:: but now with thread support.
::
nmake -nologo -f makefile.vc core OPTS=static,msvcrt,threads
if errorlevel 1 goto error

goto end

:error
echo *** BOOM! ***

:end
title Building Tk, please wait...DONE!
echo DONE!
pause

