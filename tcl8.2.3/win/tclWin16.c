/*
 * tclWin16.c --
 *
 *      This file contains code for a 16-bit DLL to handle 32-to-16 bit
 *      thunking. This is necessary for the Win32s SynchSpawn() call.
 *
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#define STRICT

#include <windows.h>  
#include <toolhelp.h> 

#include <stdio.h>
#include <string.h>

static int                      WinSpawn(char *command);
static int                      DosSpawn(char *command, char *fromFileName,
				    char *toFileName);                                          
static int                      WaitForExit(int inst);

/*
 * The following data is used to construct a .pif file that wraps the
 * .bat file that runs the 16-bit application (that Jack built).  
 * The .pif file causes the .bat file to run in an iconified window.
 * Otherwise, when we try to exec something, a DOS box pops up, 
 * obscuring everything, and then almost immediately flickers out of
 * existence, which is rather disconcerting.
 */

static char pifData[545] = {
'\000', '\013', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\200', '\000', '\200', '\000', '\103', '\117', '\115', '\115', 
'\101', '\116', '\104', '\056', '\103', '\117', '\115', '\000', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040',
'\040', '\040', '\040', '\020', '\000', '\000', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040',
'\040', '\040', '\040', '\040', '\040', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\177', '\001', '\000', 
'\377', '\031', '\120', '\000', '\000', '\007', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\340', 
'\040', '\115', '\111', '\103', '\122', '\117', '\123', '\117', 
'\106', '\124', '\040', '\120', '\111', '\106', '\105', '\130', 
'\000', '\207', '\001', '\000', '\000', '\161', '\001', '\127', 
'\111', '\116', '\104', '\117', '\127', '\123', '\040', '\063',
'\070', '\066', '\040', '\063', '\056', '\060', '\000', '\005', 
'\002', '\235', '\001', '\150', '\000', '\200', '\002', '\200', 
'\000', '\144', '\000', '\062', '\000', '\000', '\004', '\000', 
'\000', '\000', '\004', '\000', '\000', '\002', '\020', '\002', 
'\000', '\037', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000', '\000', '\000', '\000', '\000', '\057', '\143', '\040', 
'\146', '\157', '\157', '\056', '\142', '\141', '\164', '\000', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\040', '\040', '\040', 
'\040', '\040', '\040', '\040', '\040', '\127', '\111', '\116', 
'\104', '\117', '\127', '\123', '\040', '\062', '\070', '\066', 
'\040', '\063', '\056', '\060', '\000', '\377', '\377', '\033', 
'\002', '\006', '\000', '\000', '\000', '\000', '\000', '\000', 
'\000'
};

static HINSTANCE hInstance;


/*
 *----------------------------------------------------------------------
 *
 * LibMain --
 *
 *      16-bit DLL entry point.
 *
 * Results:
 *      Returns 1.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int CALLBACK
LibMain(
    HINSTANCE hinst,
    WORD wDS,
    WORD cbHeap,
    LPSTR unused)
{
    hInstance   = hinst;
    wDS         = wDS;          /* lint. */
    cbHeap      = cbHeap;       /* lint. */
    unused      = unused;       /* lint. */

    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * UTProc --
 *
 *      Universal Thunk dispatch routine.  Executes a 16-bit DOS
 *      application or a 16-bit or 32-bit Windows application and
 *      waits for it to complete.
 *
 * Results:
 *      1 if the application could be run, 0 or -1 on failure.
 *
 * Side effects:
 *      Executes 16-bit code.
 *
 *----------------------------------------------------------------------
 */

int WINAPI
UTProc(buf, func)
    void *buf;
    DWORD func;
{
    char **args;

    args = (char **) buf;
    if (func == 0) {
	return DosSpawn(args[0], args[1], args[2]);
    } else {
	return WinSpawn(args[0]);
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * WinSpawn --
 *
 *      Start a 16-bit or 32-bit Windows application with optional 
 *      command line arguments and wait for it to finish.  Windows 
 *      applications do not handle input/output redirection.
 *
 * Results:
 *      The return value is 1 if the application could be run, 0 otherwise.
 *
 * Side effects:
 *      Whatever the application does.
 *
 *-------------------------------------------------------------------------
 */

static int
WinSpawn(command)
    char *command;              /* The command line, consisting of the name
				 * of the executable to run followed by any
				 * number of arguments to the executable. */
{
    return WaitForExit(WinExec(command, SW_SHOW));
}

/*
 *---------------------------------------------------------------------------
 *
 * DosSpawn --
 *
 *      Start a 16-bit DOS program with optional command line arguments
 *      and wait for it to finish.  Input and output can be redirected
 *      from the specified files, but there is no such thing as stderr 
 *      under Win32s.
 *      
 *      This procedure to constructs a temporary .pif file that wraps a
 *      temporary .bat file that runs the 16-bit application.  The .bat
 *      file is necessary to get the redirection symbols '<' and '>' to 
 *      work, because WinExec() doesn't accept them.  The .pif file is
 *      necessary to cause the .bat file to run in an iconified window,
 *      to avoid having a large DOS box pop up, obscuring everything, and 
 *      then almost immediately flicker out of existence, which is rather 
 *      disconcerting.
 *
 * Results:
 *      The return value is 1 if the application could be run, 0 otherwise.
 *
 * Side effects:
 *      Whatever the application does.
 *
 *---------------------------------------------------------------------------
 */

static int
DosSpawn(command, fromFileName, toFileName)
    char *command;              /* The name of the program, plus any
				 * arguments, to be run. */
    char *fromFileName;         /* Standard input for the program is to be
				 * redirected from this file, or NULL for no
				 * standard input. */
    char *toFileName;           /* Standard output for the program is to be
				 * redirected to this file, or NULL to
				 * discard standard output. */
{
    int result;
    HFILE batFile, pifFile;
    char batFileName[144], pifFileName[144];

    GetTempFileName(0, "tcl", 0, batFileName);
    unlink(batFileName);
    strcpy(strrchr(batFileName, '.'), ".bat");
    batFile = _lcreat(batFileName, 0);

    GetTempFileName(0, "tcl", 0, pifFileName);
    unlink(pifFileName);
    strcpy(strrchr(pifFileName, '.'), ".pif");
    pifFile = _lcreat(pifFileName, 0);

    _lwrite(batFile, command, strlen(command));
    if (fromFileName == NULL) {
	_lwrite(batFile, " < nul", 6);
    } else {
	_lwrite(batFile, " < ", 3);
	_lwrite(batFile, fromFileName, strlen(fromFileName));
    }
    if (toFileName == NULL) {
	_lwrite(batFile, " > nul", 6);
    } else {
	_lwrite(batFile, " > ", 3);
	_lwrite(batFile, toFileName, strlen(toFileName));
    }
    _lwrite(batFile, "\r\n\032", 3);
    _lclose(batFile);

    strcpy(pifData + 0x1c8, batFileName);
    _lwrite(pifFile, pifData, sizeof(pifData));
    _lclose(pifFile);

    result = WaitForExit(WinExec(pifFileName, SW_MINIMIZE));

    unlink(pifFileName);
    unlink(batFileName);

    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * WaitForExit --
 *
 *      Wait until the application with the given instance handle has
 *      finished.  PeekMessage() is used to yield the processor; 
 *      otherwise, nothing else could execute on the system.
 *
 * Results:
 *      The return value is 1 if the process exited successfully,
 *      or 0 otherwise.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

static int
WaitForExit(inst)
    int inst;                   /* Identifies the instance handle of the
				 * process to wait for. */
{
    TASKENTRY te;
    MSG msg;
    UINT timer;

    if (inst < 32) {
	return 0;
    }

    te.dwSize = sizeof(te);
    te.hInst = 0;
    TaskFirst(&te);
    do {
	if (te.hInst == (HINSTANCE) inst) {
	    break;
	}
    } while (TaskNext(&te) != FALSE);

    if (te.hInst != (HINSTANCE) inst) {
	return 0;
    }

    timer = SetTimer(NULL, 0, 0, NULL);
    while (1) {
	if (GetMessage(&msg, NULL, 0, 0) != 0) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
	TaskFirst(&te);
	do {
	    if (te.hInst == (HINSTANCE) inst) {
		break;
	    }
	} while (TaskNext(&te) != FALSE);

	if (te.hInst != (HINSTANCE) inst) {
	    KillTimer(NULL, timer);
	    return 1;
	}
    }
}
