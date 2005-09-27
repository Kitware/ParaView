/* 
 * stub16.c 
 *
 *	A helper program used for running 16-bit DOS applications under
 *	Windows 95.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#define STRICT

#include <windows.h>
#include <stdio.h>

static HANDLE		CreateTempFile(void);

/*
 *---------------------------------------------------------------------------
 *
 * main
 *
 *	Entry point for the 32-bit console mode app used by Windows 95 to
 *	help run the 16-bit program specified on the command line.
 *
 *	1. EOF on a pipe that connects a detached 16-bit process and a
 *	32-bit process is never seen.  So, this process runs the 16-bit
 *	process _attached_, and then it is run detached from the calling
 *	32-bit process.  
 * 
 *	2. If a 16-bit process blocks reading from or writing to a pipe,
 *	it never wakes up, and eventually brings the whole system down
 *	with it if you try to kill the process.  This app simulates
 *	pipes.  If any of the stdio handles is a pipe, this program
 *	accumulates information into temp files and forwards it to or
 *	from the DOS application as appropriate.  This means that this
 *	program must receive EOF from a stdin pipe before it will actually
 *	start the DOS app, and the DOS app must finish generating stdout
 *	or stderr before the data will be sent to the next stage of the
 *	pipe.  If the stdio handles are not pipes, no accumulation occurs
 *	and the data is passed straight through to and from the DOS
 *	application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The child process is created and this process waits for it to
 *	complete.
 *
 *---------------------------------------------------------------------------
 */

int
main()
{
    DWORD dwRead, dwWrite;
    char *cmdLine;
    HANDLE hStdInput, hStdOutput, hStdError;
    HANDLE hFileInput, hFileOutput, hFileError;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char buf[8192];
    DWORD result;

    hFileInput = INVALID_HANDLE_VALUE;
    hFileOutput = INVALID_HANDLE_VALUE;
    hFileError = INVALID_HANDLE_VALUE;
    result = 1;

    /*
     * Don't get command line from argc, argv, because the command line
     * tokenizer will have stripped off all the escape sequences needed
     * for quotes and backslashes, and then we'd have to put them all
     * back in again.  Get the raw command line and parse off what we
     * want ourselves.  The command line should be of the form:
     *
     * stub16.exe program arg1 arg2 ...
     */

    cmdLine = strchr(GetCommandLine(), ' ');
    if (cmdLine == NULL) {
	return 1;
    }
    cmdLine++;

    hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    hStdError = GetStdHandle(STD_ERROR_HANDLE);

    if (GetFileType(hStdInput) == FILE_TYPE_PIPE) {
	hFileInput = CreateTempFile();
	if (hFileInput == INVALID_HANDLE_VALUE) {
	    goto cleanup;
	}
	while (ReadFile(hStdInput, buf, sizeof(buf), &dwRead, NULL) != FALSE) {
	    if (dwRead == 0) {
		break;
	    }
	    if (WriteFile(hFileInput, buf, dwRead, &dwWrite, NULL) == FALSE) {
		goto cleanup;
	    }
	}
	SetFilePointer(hFileInput, 0, 0, FILE_BEGIN);
	SetStdHandle(STD_INPUT_HANDLE, hFileInput);
    }
    if (GetFileType(hStdOutput) == FILE_TYPE_PIPE) {
	hFileOutput = CreateTempFile();
	if (hFileOutput == INVALID_HANDLE_VALUE) {
	    goto cleanup;
	}
	SetStdHandle(STD_OUTPUT_HANDLE, hFileOutput);
    }
    if (GetFileType(hStdError) == FILE_TYPE_PIPE) {
	hFileError = CreateTempFile();
	if (hFileError == INVALID_HANDLE_VALUE) {
	    goto cleanup;
	}
	SetStdHandle(STD_ERROR_HANDLE, hFileError);
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (CreateProcess(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, 
	    &pi) == FALSE) {
	goto cleanup;
    }

    WaitForInputIdle(pi.hProcess, 5000);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &result);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (hFileOutput != INVALID_HANDLE_VALUE) {
	SetFilePointer(hFileOutput, 0, 0, FILE_BEGIN);
	while (ReadFile(hFileOutput, buf, sizeof(buf), &dwRead, NULL) != FALSE) {
	    if (dwRead == 0) {
		break;
	    }
	    if (WriteFile(hStdOutput, buf, dwRead, &dwWrite, NULL) == FALSE) {
		break;
	    }
	}
    }
    if (hFileError != INVALID_HANDLE_VALUE) {
	SetFilePointer(hFileError, 0, 0, FILE_BEGIN);
	while (ReadFile(hFileError, buf, sizeof(buf), &dwRead, NULL) != FALSE) {
	    if (dwRead == 0) {
		break;
	    }
	    if (WriteFile(hStdError, buf, dwRead, &dwWrite, NULL) == FALSE) {
		break;
	    }
	}
    }

cleanup:
    if (hFileInput != INVALID_HANDLE_VALUE) {
	CloseHandle(hFileInput);
    }
    if (hFileOutput != INVALID_HANDLE_VALUE) {
	CloseHandle(hFileOutput);
    }
    if (hFileError != INVALID_HANDLE_VALUE) {
	CloseHandle(hFileError);
    }
    CloseHandle(hStdInput);
    CloseHandle(hStdOutput);
    CloseHandle(hStdError);
    ExitProcess(result);
    return 1;
}

static HANDLE
CreateTempFile()
{
    char name[MAX_PATH];
    SECURITY_ATTRIBUTES sa;

    if (GetTempPath(sizeof(name), name) == 0) {
	return INVALID_HANDLE_VALUE;
    }
    if (GetTempFileName(name, "tcl", 0, name) == 0) {
	return INVALID_HANDLE_VALUE;
    }

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    return CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, &sa, 
	    CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
	    NULL);
}
