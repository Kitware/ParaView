@echo off

::  This is an example batchfile for building everything. Please
::  edit this (or make your own) for your needs and wants using
::  the instructions for calling makefile.vc found in makefile.vc
::
::  RCS: @(#) Id

echo Sit back and have a cup of coffee while this grinds through ;)
echo You asked for *everything*, remember?
echo.

title Building Tcl, please wait...

if "%MSVCDir%" == "" call c:\dev\devstudio60\vc98\bin\vcvars32.bat
::if "%MSVCDir%" == "" call "C:\Program Files\Microsoft Developer Studio\vc98\bin\vcvars32.bat"

set INSTALLDIR=C:\devel\languages\tcl\8.4.13

:: Build the normal stuff along with the help file.
::
nmake -nologo -f makefile.vc release winhelp OPTS=none install
if errorlevel 1 goto error

:: Build the static core, dlls and shell.
::
::nmake -nologo -f makefile.vc release OPTS=static install
::if errorlevel 1 goto error

:: Build the special static libraries that use the dynamic runtime.
::
nmake -nologo -f makefile.vc release dlls OPTS=static,msvcrt install
if errorlevel 1 goto error

:: Build the core and shell for thread support.
::
nmake -nologo -f makefile.vc release OPTS=threads install
if errorlevel 1 goto error

:: Build a static, thread support core library (no shell).
::
::nmake -nologo -f makefile.vc release OPTS=static,threads install
::if errorlevel 1 goto error

:: Build the special static libraries the use the dynamic runtime,
:: but now with thread support.
::
nmake -nologo -f makefile.vc release dlls OPTS=static,msvcrt,threads install
if errorlevel 1 goto error

goto end

:error
echo *** BOOM! ***

:end
title Building Tcl, please wait...DONE!
echo DONE!
pause

