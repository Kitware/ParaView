/* 
 * tclWinPipe.c --
 *
 *	This file implements the Windows-specific exec pipeline functions,
 *	the "pipe" channel driver, and the "pid" Tcl command.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclWinInt.h"

#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

/*
 * The following variable is used to tell whether this module has been
 * initialized.
 */

static int initialized = 0;

/*
 * The pipeMutex locks around access to the initialized and procList variables,
 * and it is used to protect background threads from being terminated while
 * they are using APIs that hold locks.
 */

TCL_DECLARE_MUTEX(pipeMutex)

/*
 * The following defines identify the various types of applications that 
 * run under windows.  There is special case code for the various types.
 */

#define APPL_NONE	0
#define APPL_DOS	1
#define APPL_WIN3X	2
#define APPL_WIN32	3

/*
 * The following constants and structures are used to encapsulate the state
 * of various types of files used in a pipeline.
 */

#define WIN32S_PIPE 1		/* Win32s emulated pipe. */
#define WIN32S_TMPFILE 2	/* Win32s emulated temporary file. */
#define WIN_FILE 3		/* Basic Win32 file. */

/*
 * This structure encapsulates the common state associated with all file
 * types used in a pipeline.
 */

typedef struct WinFile {
    int type;			/* One of the file types defined above. */
    HANDLE handle;		/* Open file handle. */
} WinFile;

/*
 * The following structure is used to keep track of temporary files under
 * Win32s and delete the disk file when the open handle is closed.
 * The type field will be WIN32S_TMPFILE.
 */

typedef struct TmpFile {
    WinFile file;		/* Common part. */
    char name[MAX_PATH];	/* Name of temp file. */
} TmpFile;

/*
 * The following structure represents a synchronous pipe under Win32s.
 * The type field will be WIN32S_PIPE.  The handle field will refer to
 * an open file when Tcl is reading from the "pipe", otherwise it is
 * INVALID_HANDLE_VALUE.
 */

typedef struct WinPipe {
    WinFile file;		/* Common part. */
    struct WinPipe *otherPtr;	/* Pointer to the WinPipe structure that
				 * corresponds to the other end of this 
				 * pipe. */
    char *fileName;		/* The name of the staging file that gets 
				 * the data written to this pipe.  Malloc'd.
				 * and shared by both ends of the pipe.  Only
				 * when both ends are freed will fileName be
				 * freed and the file it refers to deleted. */
} WinPipe;

/*
 * This list is used to map from pids to process handles.
 */

typedef struct ProcInfo {
    HANDLE hProcess;
    DWORD dwProcessId;
    struct ProcInfo *nextPtr;
} ProcInfo;

static ProcInfo *procList;

/*
 * Bit masks used in the flags field of the PipeInfo structure below.
 */

#define PIPE_PENDING	(1<<0)	/* Message is pending in the queue. */
#define PIPE_ASYNC	(1<<1)	/* Channel is non-blocking. */

/*
 * Bit masks used in the sharedFlags field of the PipeInfo structure below.
 */

#define PIPE_EOF	(1<<2)	/* Pipe has reached EOF. */
#define PIPE_EXTRABYTE (1<<3)	/* The reader thread has consumed one byte. */

/*
 * This structure describes per-instance data for a pipe based channel.
 */

typedef struct PipeInfo {
    struct PipeInfo *nextPtr;	/* Pointer to next registered pipe. */
    Tcl_Channel channel;	/* Pointer to channel structure. */
    int validMask;		/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, or TCL_EXCEPTION: indicates
				 * which operations are valid on the file. */
    int watchMask;		/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, or TCL_EXCEPTION: indicates
				 * which events should be reported. */
    int flags;			/* State flags, see above for a list. */
    TclFile readFile;		/* Output from pipe. */
    TclFile writeFile;		/* Input from pipe. */
    TclFile errorFile;		/* Error output from pipe. */
    int numPids;		/* Number of processes attached to pipe. */
    Tcl_Pid *pidPtr;		/* Pids of attached processes. */
    Tcl_ThreadId threadId;	/* Thread to which events should be reported.
				 * This value is used by the reader/writer
				 * threads. */
    HANDLE writeThread;		/* Handle to writer thread. */
    HANDLE readThread;		/* Handle to reader thread. */
    HANDLE writable;		/* Manual-reset event to signal when the
				 * writer thread has finished waiting for
				 * the current buffer to be written. */
    HANDLE readable;		/* Manual-reset event to signal when the
				 * reader thread has finished waiting for
				 * input. */
    HANDLE startWriter;		/* Auto-reset event used by the main thread to
				 * signal when the writer thread should attempt
				 * to write to the pipe. */
    HANDLE startReader;		/* Auto-reset event used by the main thread to
				 * signal when the reader thread should attempt
				 * to read from the pipe. */
    DWORD writeError;		/* An error caused by the last background
				 * write.  Set to 0 if no error has been
				 * detected.  This word is shared with the
				 * writer thread so access must be
				 * synchronized with the writable object.
				 */
    char *writeBuf;		/* Current background output buffer.
				 * Access is synchronized with the writable
				 * object. */
    int writeBufLen;		/* Size of write buffer.  Access is
				 * synchronized with the writable
				 * object. */
    int toWrite;		/* Current amount to be written.  Access is
				 * synchronized with the writable object. */
    int readFlags;		/* Flags that are shared with the reader
				 * thread.  Access is synchronized with the
				 * readable object.  */
    char extraByte;		/* Buffer for extra character consumed by
				 * reader thread.  This byte is shared with
				 * the reader thread so access must be
				 * synchronized with the readable object. */
} PipeInfo;

typedef struct ThreadSpecificData {
    /*
     * The following pointer refers to the head of the list of pipes
     * that are being watched for file events.
     */
    
    PipeInfo *firstPipePtr;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * The following structure is what is added to the Tcl event queue when
 * pipe events are generated.
 */

typedef struct PipeEvent {
    Tcl_Event header;		/* Information that is standard for
				 * all events. */
    PipeInfo *infoPtr;		/* Pointer to pipe info structure.  Note
				 * that we still have to verify that the
				 * pipe exists before dereferencing this
				 * pointer. */
} PipeEvent;

/*
 * Declarations for functions used only in this file.
 */

static int		ApplicationType(Tcl_Interp *interp,
			    const char *fileName, char *fullName);
static void		BuildCommandLine(const char *executable, int argc, 
			    char **argv, Tcl_DString *linePtr);
static void		CopyChannel(HANDLE dst, HANDLE src);
static BOOL		HasConsole(void);
static char *		MakeTempFile(Tcl_DString *namePtr);
static int		PipeBlockModeProc(ClientData instanceData, int mode);
static void		PipeCheckProc(ClientData clientData, int flags);
static int		PipeClose2Proc(ClientData instanceData,
			    Tcl_Interp *interp, int flags);
static int		PipeEventProc(Tcl_Event *evPtr, int flags);
static void		PipeExitHandler(ClientData clientData);
static int		PipeGetHandleProc(ClientData instanceData,
			    int direction, ClientData *handlePtr);
static void		PipeInit(void);
static int		PipeInputProc(ClientData instanceData, char *buf,
			    int toRead, int *errorCode);
static int		PipeOutputProc(ClientData instanceData, char *buf,
			    int toWrite, int *errorCode);
static DWORD WINAPI	PipeReaderThread(LPVOID arg);
static void		PipeSetupProc(ClientData clientData, int flags);
static void		PipeWatchProc(ClientData instanceData, int mask);
static DWORD WINAPI	PipeWriterThread(LPVOID arg);
static void		ProcExitHandler(ClientData clientData);
static int		TempFileName(WCHAR name[MAX_PATH]);
static int		WaitForRead(PipeInfo *infoPtr, int blocking);

/*
 * This structure describes the channel type structure for command pipe
 * based IO.
 */

static Tcl_ChannelType pipeChannelType = {
    "pipe",			/* Type name. */
    PipeBlockModeProc,		/* Set blocking or non-blocking mode.*/
    TCL_CLOSE2PROC,		/* Close proc. */
    PipeInputProc,		/* Input proc. */
    PipeOutputProc,		/* Output proc. */
    NULL,			/* Seek proc. */
    NULL,			/* Set option proc. */
    NULL,			/* Get option proc. */
    PipeWatchProc,		/* Set up notifier to watch the channel. */
    PipeGetHandleProc,		/* Get an OS handle from channel. */
    PipeClose2Proc
};

/*
 *----------------------------------------------------------------------
 *
 * PipeInit --
 *
 *	This function initializes the static variables for this file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates a new event source.
 *
 *----------------------------------------------------------------------
 */

static void
PipeInit()
{
    ThreadSpecificData *tsdPtr;

    /*
     * Check the initialized flag first, then check again in the mutex.
     * This is a speed enhancement.
     */

    if (!initialized) {
	Tcl_MutexLock(&pipeMutex);
	if (!initialized) {
	    initialized = 1;
	    procList = NULL;
	    Tcl_CreateExitHandler(ProcExitHandler, NULL);
	}
	Tcl_MutexUnlock(&pipeMutex);
    }

    tsdPtr = (ThreadSpecificData *)TclThreadDataKeyGet(&dataKey);
    if (tsdPtr == NULL) {
	tsdPtr = TCL_TSD_INIT(&dataKey);
	tsdPtr->firstPipePtr = NULL;
	Tcl_CreateEventSource(PipeSetupProc, PipeCheckProc, NULL);
	Tcl_CreateThreadExitHandler(PipeExitHandler, NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PipeExitHandler --
 *
 *	This function is called to cleanup the pipe module before
 *	Tcl is unloaded.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the pipe event source.
 *
 *----------------------------------------------------------------------
 */

static void
PipeExitHandler(
    ClientData clientData)	/* Old window proc */
{
    Tcl_DeleteEventSource(PipeSetupProc, PipeCheckProc, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * ProcExitHandler --
 *
 *	This function is called to cleanup the process list before
 *	Tcl is unloaded.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets the process list.
 *
 *----------------------------------------------------------------------
 */

static void
ProcExitHandler(
    ClientData clientData)	/* Old window proc */
{
    Tcl_MutexLock(&pipeMutex);
    initialized = 0;
    Tcl_MutexUnlock(&pipeMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * PipeSetupProc --
 *
 *	This procedure is invoked before Tcl_DoOneEvent blocks waiting
 *	for an event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adjusts the block time if needed.
 *
 *----------------------------------------------------------------------
 */

void
PipeSetupProc(
    ClientData data,		/* Not used. */
    int flags)			/* Event flags as passed to Tcl_DoOneEvent. */
{
    PipeInfo *infoPtr;
    Tcl_Time blockTime = { 0, 0 };
    int block = 1;
    WinFile *filePtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return;
    }
    
    /*
     * Look to see if any events are already pending.  If they are, poll.
     */

    for (infoPtr = tsdPtr->firstPipePtr; infoPtr != NULL; 
	    infoPtr = infoPtr->nextPtr) {
	if (infoPtr->watchMask & TCL_WRITABLE) {
	    filePtr = (WinFile*) infoPtr->writeFile;
	    if ((filePtr->type == WIN32S_PIPE) 
		    || (WaitForSingleObject(infoPtr->writable, 0)
			    != WAIT_TIMEOUT)) {
		block = 0;
	    }
	}
	if (infoPtr->watchMask & TCL_READABLE) {
	    filePtr = (WinFile*) infoPtr->readFile;
	    if ((filePtr->type == WIN32S_PIPE) 
		    || (WaitForRead(infoPtr, 0) >= 0)) {
		block = 0;
	    }
	}
    }
    if (!block) {
	Tcl_SetMaxBlockTime(&blockTime);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PipeCheckProc --
 *
 *	This procedure is called by Tcl_DoOneEvent to check the pipe
 *	event source for events. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May queue an event.
 *
 *----------------------------------------------------------------------
 */

static void
PipeCheckProc(
    ClientData data,		/* Not used. */
    int flags)			/* Event flags as passed to Tcl_DoOneEvent. */
{
    PipeInfo *infoPtr;
    PipeEvent *evPtr;
    WinFile *filePtr;
    int needEvent;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return;
    }
    
    /*
     * Queue events for any ready pipes that don't already have events
     * queued.
     */

    for (infoPtr = tsdPtr->firstPipePtr; infoPtr != NULL; 
	    infoPtr = infoPtr->nextPtr) {
	if (infoPtr->flags & PIPE_PENDING) {
	    continue;
	}
	
	/*
	 * Queue an event if the pipe is signaled for reading or writing.
	 */

	needEvent = 0;
	filePtr = (WinFile*) infoPtr->writeFile;
	if (infoPtr->watchMask & TCL_WRITABLE) {
	    if ((filePtr->type == WIN32S_PIPE)
		    || (WaitForSingleObject(infoPtr->writable, 0)
			    != WAIT_TIMEOUT)) {
		needEvent = 1;
	    }
	}
	
	filePtr = (WinFile*) infoPtr->readFile;
	if (infoPtr->watchMask & TCL_READABLE) {
	    if ((filePtr->type == WIN32S_PIPE)
		    || (WaitForRead(infoPtr, 0) >= 0)) {
		needEvent = 1;
	    }
	}

	if (needEvent) {
	    infoPtr->flags |= PIPE_PENDING;
	    evPtr = (PipeEvent *) ckalloc(sizeof(PipeEvent));
	    evPtr->header.proc = PipeEventProc;
	    evPtr->infoPtr = infoPtr;
	    Tcl_QueueEvent((Tcl_Event *) evPtr, TCL_QUEUE_TAIL);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclWinMakeFile --
 *
 *	This function constructs a new TclFile from a given data and
 *	type value.
 *
 * Results:
 *	Returns a newly allocated WinFile as a TclFile.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TclFile
TclWinMakeFile(
    HANDLE handle)		/* Type-specific data. */
{
    WinFile *filePtr;

    filePtr = (WinFile *) ckalloc(sizeof(WinFile));
    filePtr->type = WIN_FILE;
    filePtr->handle = handle;

    return (TclFile)filePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TempFileName --
 *
 *	Gets a temporary file name and deals with the fact that the
 *	temporary file path provided by Windows may not actually exist
 *	if the TMP or TEMP environment variables refer to a 
 *	non-existent directory.
 *
 * Results:    
 *	0 if error, non-zero otherwise.  If non-zero is returned, the
 *	name buffer will be filled with a name that can be used to 
 *	construct a temporary file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TempFileName(name)
    WCHAR name[MAX_PATH];	/* Buffer in which name for temporary 
				 * file gets stored. */
{
    TCHAR *prefix;

    prefix = (tclWinProcs->useWide) ? (TCHAR *) L"TCL" : (TCHAR *) "TCL";
    if ((*tclWinProcs->getTempPathProc)(MAX_PATH, name) != 0) {
	if ((*tclWinProcs->getTempFileNameProc)((TCHAR *) name, prefix, 0, 
		name) != 0) {
	    return 1;
	}
    }
    if (tclWinProcs->useWide) {
	((WCHAR *) name)[0] = '.';
	((WCHAR *) name)[1] = '\0';
    } else {
	((char *) name)[0] = '.';
	((char *) name)[1] = '\0';
    }
    return (*tclWinProcs->getTempFileNameProc)((TCHAR *) name, prefix, 0, 
	    name);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpMakeFile --
 *
 *	Make a TclFile from a channel.
 *
 * Results:
 *	Returns a new TclFile or NULL on failure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TclFile
TclpMakeFile(channel, direction)
    Tcl_Channel channel;	/* Channel to get file from. */
    int direction;		/* Either TCL_READABLE or TCL_WRITABLE. */
{
    HANDLE handle;

    if (Tcl_GetChannelHandle(channel, direction, 
	    (ClientData *) &handle) == TCL_OK) {
	return TclWinMakeFile(handle);
    } else {
	return (TclFile) NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpOpenFile --
 *
 *	This function opens files for use in a pipeline.
 *
 * Results:
 *	Returns a newly allocated TclFile structure containing the
 *	file handle.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TclFile
TclpOpenFile(path, mode)
    CONST char *path;		/* The name of the file to open. */
    int mode;			/* In what mode to open the file? */
{
    HANDLE handle;
    DWORD accessMode, createMode, shareMode, flags;
    Tcl_DString ds;
    TCHAR *nativePath;
    
    /*
     * Map the access bits to the NT access mode.
     */

    switch (mode & (O_RDONLY | O_WRONLY | O_RDWR)) {
	case O_RDONLY:
	    accessMode = GENERIC_READ;
	    break;
	case O_WRONLY:
	    accessMode = GENERIC_WRITE;
	    break;
	case O_RDWR:
	    accessMode = (GENERIC_READ | GENERIC_WRITE);
	    break;
	default:
	    TclWinConvertError(ERROR_INVALID_FUNCTION);
	    return NULL;
    }

    /*
     * Map the creation flags to the NT create mode.
     */

    switch (mode & (O_CREAT | O_EXCL | O_TRUNC)) {
	case (O_CREAT | O_EXCL):
	case (O_CREAT | O_EXCL | O_TRUNC):
	    createMode = CREATE_NEW;
	    break;
	case (O_CREAT | O_TRUNC):
	    createMode = CREATE_ALWAYS;
	    break;
	case O_CREAT:
	    createMode = OPEN_ALWAYS;
	    break;
	case O_TRUNC:
	case (O_TRUNC | O_EXCL):
	    createMode = TRUNCATE_EXISTING;
	    break;
	default:
	    createMode = OPEN_EXISTING;
	    break;
    }

    nativePath = Tcl_WinUtfToTChar(path, -1, &ds);

    /*
     * If the file is not being created, use the existing file attributes.
     */

    flags = 0;
    if (!(mode & O_CREAT)) {
	flags = (*tclWinProcs->getFileAttributesProc)(nativePath);
	if (flags == 0xFFFFFFFF) {
	    flags = 0;
	}
    }

    /*
     * Set up the file sharing mode.  We want to allow simultaneous access.
     */

    shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    /*
     * Now we get to create the file.
     */

    handle = (*tclWinProcs->createFileProc)(nativePath, accessMode, 
	    shareMode, NULL, createMode, flags, NULL);
    Tcl_DStringFree(&ds);

    if (handle == INVALID_HANDLE_VALUE) {
	DWORD err;
	
	err = GetLastError();
	if ((err & 0xffffL) == ERROR_OPEN_FAILED) {
	    err = (mode & O_CREAT) ? ERROR_FILE_EXISTS : ERROR_FILE_NOT_FOUND;
	}
        TclWinConvertError(err);
        return NULL;
    }

    /*
     * Seek to the end of file if we are writing.
     */

    if (mode & O_WRONLY) {
	SetFilePointer(handle, 0, NULL, FILE_END);
    }

    return TclWinMakeFile(handle);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpCreateTempFile --
 *
 *	This function opens a unique file with the property that it
 *	will be deleted when its file handle is closed.  The temporary
 *	file is created in the system temporary directory.
 *
 * Results:
 *	Returns a valid TclFile, or NULL on failure.
 *
 * Side effects:
 *	Creates a new temporary file.
 *
 *----------------------------------------------------------------------
 */

TclFile
TclpCreateTempFile(contents)
    CONST char *contents;	/* String to write into temp file, or NULL. */
{
    WCHAR name[MAX_PATH];
    HANDLE handle;

    if (TempFileName(name) == 0) {
	return NULL;
    }

    handle = (*tclWinProcs->createFileProc)((TCHAR *) name, 
	    GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
	    FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
	goto error;
    }

    /*
     * Write the file out, doing line translations on the way.
     */

    if (contents != NULL) {
	DWORD result, length;
	CONST char *p;
	
	for (p = contents; *p != '\0'; p++) {
	    if (*p == '\n') {
		length = p - contents;
		if (length > 0) {
		    if (!WriteFile(handle, contents, length, &result, NULL)) {
			goto error;
		    }
		}
		if (!WriteFile(handle, "\r\n", 2, &result, NULL)) {
		    goto error;
		}
		contents = p+1;
	    }
	}
	length = p - contents;
	if (length > 0) {
	    if (!WriteFile(handle, contents, length, &result, NULL)) {
		goto error;
	    }
	}
	if (SetFilePointer(handle, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF) {
	    goto error;
	}
    }

    /*
     * Under Win32s a file created with FILE_FLAG_DELETE_ON_CLOSE won't
     * actually be deleted when it is closed, so we have to do it ourselves.
     */

    if (TclWinGetPlatformId() == VER_PLATFORM_WIN32s) {
	TmpFile *tmpFilePtr = (TmpFile *) ckalloc(sizeof(TmpFile));
	tmpFilePtr->file.type = WIN32S_TMPFILE;
	tmpFilePtr->file.handle = handle;
	lstrcpyA(tmpFilePtr->name, (char *) name);
	return (TclFile) tmpFilePtr;
    } else {
	return TclWinMakeFile(handle);
    }

  error:
    TclWinConvertError(GetLastError());
    CloseHandle(handle);
    (*tclWinProcs->deleteFileProc)((TCHAR *) name);
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpCreatePipe --
 *
 *      Creates an anonymous pipe.  Under Win32s, creates a temp file
 *	that is used to simulate a pipe.
 *
 * Results:
 *      Returns 1 on success, 0 on failure. 
 *
 * Side effects:
 *      Creates a pipe.
 *
 *----------------------------------------------------------------------
 */

int
TclpCreatePipe(
    TclFile *readPipe,	/* Location to store file handle for
				 * read side of pipe. */
    TclFile *writePipe)	/* Location to store file handle for
				 * write side of pipe. */
{
    HANDLE readHandle, writeHandle;

    if (CreatePipe(&readHandle, &writeHandle, NULL, 0) != 0) {
	*readPipe = TclWinMakeFile(readHandle);
	*writePipe = TclWinMakeFile(writeHandle);
	return 1;
    }

    if (TclWinGetPlatformId() == VER_PLATFORM_WIN32s) {
	WinPipe *readPipePtr, *writePipePtr;
	char buf[MAX_PATH];
	int bytes;

	if (TempFileName((WCHAR *) buf) != 0) {
	    bytes = strlen((char *) buf) + 1;
	    readPipePtr = (WinPipe *) ckalloc(sizeof(WinPipe));
	    writePipePtr = (WinPipe *) ckalloc(sizeof(WinPipe));

	    readPipePtr->file.type = WIN32S_PIPE;
	    readPipePtr->otherPtr = writePipePtr;
	    readPipePtr->fileName = (char *) ckalloc(bytes);
	    lstrcpyA(readPipePtr->fileName, buf);
	    readPipePtr->file.handle = INVALID_HANDLE_VALUE;
	    writePipePtr->file.type = WIN32S_PIPE;
	    writePipePtr->otherPtr = readPipePtr;
	    writePipePtr->fileName = readPipePtr->fileName;
	    writePipePtr->file.handle = INVALID_HANDLE_VALUE;

	    *readPipe = (TclFile) readPipePtr;
	    *writePipe = (TclFile) writePipePtr;

	    return 1;
	}
    }

    TclWinConvertError(GetLastError());
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpCloseFile --
 *
 *	Closes a pipeline file handle.  These handles are created by
 *	TclpOpenFile, TclpCreatePipe, or TclpMakeFile.
 *
 * Results:
 *	0 on success, -1 on failure.
 *
 * Side effects:
 *	The file is closed and deallocated.
 *
 *----------------------------------------------------------------------
 */

int
TclpCloseFile(
    TclFile file)	/* The file to close. */
{
    WinFile *filePtr = (WinFile *) file;
    WinPipe *pipePtr;

    switch (filePtr->type) {
	case WIN_FILE:
	case WIN32S_TMPFILE:
	    /*
	     * Don't close the Win32 handle if the handle is a standard channel
	     * during the exit process.  Otherwise, one thread may kill the stdio
	     * of another.
	     */

	    if (!TclInExit() 
		    || ((GetStdHandle(STD_INPUT_HANDLE) != filePtr->handle)
			    && (GetStdHandle(STD_OUTPUT_HANDLE) != filePtr->handle)
			    && (GetStdHandle(STD_ERROR_HANDLE) != filePtr->handle))) {
		if (CloseHandle(filePtr->handle) == FALSE) {
		    TclWinConvertError(GetLastError());
		    ckfree((char *) filePtr);
		    return -1;
		}
	    }
	    /*
	     * Simulate deleting the file on close for Win32s.
	     */

	    if (filePtr->type == WIN32S_TMPFILE) {
		DeleteFileA(((TmpFile *) filePtr)->name);
	    }
	    break;

	case WIN32S_PIPE:
	    pipePtr = (WinPipe *) file;

	    if (pipePtr->otherPtr != NULL) {
		pipePtr->otherPtr->otherPtr = NULL;
	    } else {
		if (pipePtr->file.handle != INVALID_HANDLE_VALUE) {
		    CloseHandle(pipePtr->file.handle);
		}
		DeleteFileA(pipePtr->fileName);
		ckfree((char *) pipePtr->fileName);
	    }
	    break;

	default:
	    panic("TclpCloseFile: unexpected file type");
    }

    ckfree((char *) filePtr);
    return 0;
}

/*
 *--------------------------------------------------------------------------
 *
 * TclpGetPid --
 *
 *	Given a HANDLE to a child process, return the process id for that
 *	child process.
 *
 * Results:
 *	Returns the process id for the child process.  If the pid was not 
 *	known by Tcl, either because the pid was not created by Tcl or the 
 *	child process has already been reaped, -1 is returned.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------------------
 */

unsigned long
TclpGetPid(
    Tcl_Pid pid)		/* The HANDLE of the child process. */
{
    ProcInfo *infoPtr;

    Tcl_MutexLock(&pipeMutex);
    for (infoPtr = procList; infoPtr != NULL; infoPtr = infoPtr->nextPtr) {
	if (infoPtr->hProcess == (HANDLE) pid) {
	    Tcl_MutexUnlock(&pipeMutex);
	    return infoPtr->dwProcessId;
	}
    }
    Tcl_MutexUnlock(&pipeMutex);
    return (unsigned long) -1;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpCreateProcess --
 *
 *	Create a child process that has the specified files as its 
 *	standard input, output, and error.  The child process runs
 *	synchronously under Win32s and asynchronously under Windows NT
 *	and Windows 95, and runs with the same environment variables
 *	as the creating process.
 *
 *	The complete Windows search path is searched to find the specified 
 *	executable.  If an executable by the given name is not found, 
 *	automatically tries appending ".com", ".exe", and ".bat" to the 
 *	executable name.
 *
 * Results:
 *	The return value is TCL_ERROR and an error message is left in
 *	the interp's result if there was a problem creating the child 
 *	process.  Otherwise, the return value is TCL_OK and *pidPtr is
 *	filled with the process id of the child process.
 * 
 * Side effects:
 *	A process is created.
 *	
 *----------------------------------------------------------------------
 */

int
TclpCreateProcess(
    Tcl_Interp *interp,		/* Interpreter in which to leave errors that
				 * occurred when creating the child process.
				 * Error messages from the child process
				 * itself are sent to errorFile. */
    int argc,			/* Number of arguments in following array. */
    char **argv,		/* Array of argument strings.  argv[0]
				 * contains the name of the executable
				 * converted to native format (using the
				 * Tcl_TranslateFileName call).  Additional
				 * arguments have not been converted. */
    TclFile inputFile,		/* If non-NULL, gives the file to use as
				 * input for the child process.  If inputFile
				 * file is not readable or is NULL, the child
				 * will receive no standard input. */
    TclFile outputFile,		/* If non-NULL, gives the file that
				 * receives output from the child process.  If
				 * outputFile file is not writeable or is
				 * NULL, output from the child will be
				 * discarded. */
    TclFile errorFile,		/* If non-NULL, gives the file that
				 * receives errors from the child process.  If
				 * errorFile file is not writeable or is NULL,
				 * errors from the child will be discarded.
				 * errorFile may be the same as outputFile. */
    Tcl_Pid *pidPtr)		/* If this procedure is successful, pidPtr
				 * is filled with the process id of the child
				 * process. */
{
    int result, applType, createFlags;
    Tcl_DString cmdLine;	/* Complete command line (TCHAR). */
    STARTUPINFOA startInfo;
    PROCESS_INFORMATION procInfo;
    SECURITY_ATTRIBUTES secAtts;
    HANDLE hProcess, h, inputHandle, outputHandle, errorHandle;
    char execPath[MAX_PATH * TCL_UTF_MAX];
    WinFile *filePtr;

    PipeInit();

    applType = ApplicationType(interp, argv[0], execPath);
    if (applType == APPL_NONE) {
	return TCL_ERROR;
    }

    result = TCL_ERROR;
    Tcl_DStringInit(&cmdLine);

    if (TclWinGetPlatformId() == VER_PLATFORM_WIN32s) {
	/*
	 * Under Win32s, there are no pipes.  In order to simulate pipe
	 * behavior, the child processes are run synchronously and their
	 * I/O is redirected from/to temporary files before the next 
	 * stage of the pipeline is started.
	 */

	MSG msg;
	DWORD status;
	DWORD args[4];
	void *trans[5];
	char *inputFileName, *outputFileName;
	Tcl_DString inputTempFile, outputTempFile;

	BuildCommandLine(execPath, argc, argv, &cmdLine);

	ZeroMemory(&startInfo, sizeof(startInfo));
	startInfo.cb = sizeof(startInfo);

	Tcl_DStringInit(&inputTempFile);
	Tcl_DStringInit(&outputTempFile);
	outputHandle = INVALID_HANDLE_VALUE;

	inputFileName = NULL;
	outputFileName = NULL;
	if (inputFile != NULL) {
	    filePtr = (WinFile *) inputFile;
	    switch (filePtr->type) {
		case WIN_FILE:
		case WIN32S_TMPFILE: {
		    h = INVALID_HANDLE_VALUE;
		    inputFileName = MakeTempFile(&inputTempFile);
		    if (inputFileName != NULL) {
			h = CreateFileA((char *) inputFileName, 
				GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 
				NULL);
		    }
		    if (h == INVALID_HANDLE_VALUE) {
			Tcl_AppendResult(interp, "couldn't duplicate input handle: ", 
				Tcl_PosixError(interp), (char *) NULL);
			goto end32s;
		    }
		    CopyChannel(h, filePtr->handle);
		    CloseHandle(h);
		    break;
		}
		case WIN32S_PIPE: {
		    inputFileName = (char *) ((WinPipe *) inputFile)->fileName;
		    break;
		}
	    }
	}
	if (inputFileName == NULL) {
	    inputFileName = "nul";
	}
	if (outputFile != NULL) {
	    filePtr = (WinFile *) outputFile;
	    if (filePtr->type == WIN_FILE) {
		outputFileName = MakeTempFile(&outputTempFile);
		if (outputFileName == NULL) {
		    Tcl_AppendResult(interp, "couldn't duplicate output handle: ",
			    Tcl_PosixError(interp), (char *) NULL);
		    goto end32s;
		}
		outputHandle = filePtr->handle;
	    } else if (filePtr->type == WIN32S_PIPE) {
		outputFileName = (char *) ((WinPipe *) outputFile)->fileName;
	    }
	}
	if (outputFileName == NULL) {
	    outputFileName = "nul";
	}

	if (applType == APPL_DOS) {
	    args[0] = (DWORD) Tcl_DStringValue(&cmdLine);
	    args[1] = (DWORD) inputFileName;
	    args[2] = (DWORD) outputFileName;
	    trans[0] = &args[0];
	    trans[1] = &args[1];
	    trans[2] = &args[2];
	    trans[3] = NULL;
	    if (TclWinSynchSpawn(args, 0, trans, pidPtr) != 0) {
		result = TCL_OK;
	    }
	} else if (applType == APPL_WIN3X) {
	    args[0] = (DWORD) Tcl_DStringValue(&cmdLine);
	    trans[0] = &args[0];
	    trans[1] = NULL;
	    if (TclWinSynchSpawn(args, 1, trans, pidPtr) != 0) {
		result = TCL_OK;
	    }
	} else {
	    if (CreateProcessA(NULL, Tcl_DStringValue(&cmdLine), 
		    NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, 
		    &startInfo, &procInfo) != 0) {
		CloseHandle(procInfo.hThread);
		while (1) {
		    if (GetExitCodeProcess(procInfo.hProcess, &status) == FALSE) {
			break;
		    }
		    if (status != STILL_ACTIVE) {
			break;
		    }
		    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == TRUE) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		    }
		}
		*pidPtr = (Tcl_Pid) procInfo.hProcess;
		if (*pidPtr != 0) {
		    TclWinAddProcess(procInfo.hProcess, procInfo.dwProcessId);
		}
		result = TCL_OK;
	    }
	}
	if (result != TCL_OK) {
	    TclWinConvertError(GetLastError());
	    Tcl_AppendResult(interp, "couldn't execute \"", argv[0],
		    "\": ", Tcl_PosixError(interp), (char *) NULL);
	}

	end32s:
	if (outputHandle != INVALID_HANDLE_VALUE) {
	    /*
	     * Now copy stuff from temp file to actual output handle. Don't
	     * close outputHandle because it is associated with the output
	     * file owned by the caller.
	     */

	    h = CreateFileA(outputFileName, GENERIC_READ, 0, NULL, OPEN_ALWAYS,
		    0, NULL);
	    if (h != INVALID_HANDLE_VALUE) {
		CopyChannel(outputHandle, h);
	    }
	    CloseHandle(h);
	}

	if (inputFileName == Tcl_DStringValue(&inputTempFile)) {
	    DeleteFileA(inputFileName);
	}
	
	if (outputFileName == Tcl_DStringValue(&outputTempFile)) {
	    DeleteFileA(outputFileName);
	}

	Tcl_DStringFree(&inputTempFile);
	Tcl_DStringFree(&outputTempFile);
        Tcl_DStringFree(&cmdLine);
	return result;
    }
    hProcess = GetCurrentProcess();

    /*
     * STARTF_USESTDHANDLES must be used to pass handles to child process.
     * Using SetStdHandle() and/or dup2() only works when a console mode 
     * parent process is spawning an attached console mode child process.
     */

    ZeroMemory(&startInfo, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);
    startInfo.dwFlags   = STARTF_USESTDHANDLES;
    startInfo.hStdInput	= INVALID_HANDLE_VALUE;
    startInfo.hStdOutput= INVALID_HANDLE_VALUE;
    startInfo.hStdError = INVALID_HANDLE_VALUE;

    secAtts.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAtts.lpSecurityDescriptor = NULL;
    secAtts.bInheritHandle = TRUE;

    /*
     * We have to check the type of each file, since we cannot duplicate 
     * some file types.  
     */

    inputHandle = INVALID_HANDLE_VALUE;
    if (inputFile != NULL) {
	filePtr = (WinFile *)inputFile;
	if (filePtr->type == WIN_FILE) {
	    inputHandle = filePtr->handle;
	}
    }
    outputHandle = INVALID_HANDLE_VALUE;
    if (outputFile != NULL) {
	filePtr = (WinFile *)outputFile;
	if (filePtr->type == WIN_FILE) {
	    outputHandle = filePtr->handle;
	}
    }
    errorHandle = INVALID_HANDLE_VALUE;
    if (errorFile != NULL) {
	filePtr = (WinFile *)errorFile;
	if (filePtr->type == WIN_FILE) {
	    errorHandle = filePtr->handle;
	}
    }

    /*
     * Duplicate all the handles which will be passed off as stdin, stdout
     * and stderr of the child process. The duplicate handles are set to
     * be inheritable, so the child process can use them.
     */

    if (inputHandle == INVALID_HANDLE_VALUE) {
	/* 
	 * If handle was not set, stdin should return immediate EOF.
	 * Under Windows95, some applications (both 16 and 32 bit!) 
	 * cannot read from the NUL device; they read from console
	 * instead.  When running tk, this is fatal because the child 
	 * process would hang forever waiting for EOF from the unmapped 
	 * console window used by the helper application.
	 *
	 * Fortunately, the helper application detects a closed pipe 
	 * as an immediate EOF and can pass that information to the 
	 * child process.
	 */

	if (CreatePipe(&startInfo.hStdInput, &h, &secAtts, 0) != FALSE) {
	    CloseHandle(h);
	}
    } else {
	DuplicateHandle(hProcess, inputHandle, hProcess, &startInfo.hStdInput,
		0, TRUE, DUPLICATE_SAME_ACCESS);
    }
    if (startInfo.hStdInput == INVALID_HANDLE_VALUE) {
	TclWinConvertError(GetLastError());
	Tcl_AppendResult(interp, "couldn't duplicate input handle: ",
		Tcl_PosixError(interp), (char *) NULL);
	goto end;
    }

    if (outputHandle == INVALID_HANDLE_VALUE) {
	/*
	 * If handle was not set, output should be sent to an infinitely 
	 * deep sink.  Under Windows 95, some 16 bit applications cannot
	 * have stdout redirected to NUL; they send their output to
	 * the console instead.  Some applications, like "more" or "dir /p", 
	 * when outputting multiple pages to the console, also then try and
	 * read from the console to go the next page.  When running tk, this
	 * is fatal because the child process would hang forever waiting
	 * for input from the unmapped console window used by the helper
	 * application.
	 *
	 * Fortunately, the helper application will detect a closed pipe
	 * as a sink.
	 */

	if ((TclWinGetPlatformId() == VER_PLATFORM_WIN32_WINDOWS) 
		&& (applType == APPL_DOS)) {
	    if (CreatePipe(&h, &startInfo.hStdOutput, &secAtts, 0) != FALSE) {
		CloseHandle(h);
	    }
	} else {
	    startInfo.hStdOutput = CreateFileA("NUL:", GENERIC_WRITE, 0,
		    &secAtts, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
    } else {
	DuplicateHandle(hProcess, outputHandle, hProcess, &startInfo.hStdOutput, 
		0, TRUE, DUPLICATE_SAME_ACCESS);
    }
    if (startInfo.hStdOutput == INVALID_HANDLE_VALUE) {
	TclWinConvertError(GetLastError());
	Tcl_AppendResult(interp, "couldn't duplicate output handle: ",
		Tcl_PosixError(interp), (char *) NULL);
	goto end;
    }

    if (errorHandle == INVALID_HANDLE_VALUE) {
	/*
	 * If handle was not set, errors should be sent to an infinitely
	 * deep sink.
	 */

	startInfo.hStdError = CreateFileA("NUL:", GENERIC_WRITE, 0,
		&secAtts, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
	DuplicateHandle(hProcess, errorHandle, hProcess, &startInfo.hStdError, 
		0, TRUE, DUPLICATE_SAME_ACCESS);
    } 
    if (startInfo.hStdError == INVALID_HANDLE_VALUE) {
	TclWinConvertError(GetLastError());
	Tcl_AppendResult(interp, "couldn't duplicate error handle: ",
		Tcl_PosixError(interp), (char *) NULL);
	goto end;
    }
    /* 
     * If we do not have a console window, then we must run DOS and
     * WIN32 console mode applications as detached processes. This tells
     * the loader that the child application should not inherit the
     * console, and that it should not create a new console window for
     * the child application.  The child application should get its stdio 
     * from the redirection handles provided by this application, and run
     * in the background.
     *
     * If we are starting a GUI process, they don't automatically get a 
     * console, so it doesn't matter if they are started as foreground or
     * detached processes.  The GUI window will still pop up to the
     * foreground.
     */

    if (TclWinGetPlatformId() == VER_PLATFORM_WIN32_NT) {
	if (HasConsole()) {
	    createFlags = 0;
	} else if (applType == APPL_DOS) {
	    /*
	     * Under NT, 16-bit DOS applications will not run unless they
	     * can be attached to a console.  If we are running without a
	     * console, run the 16-bit program as an normal process inside
	     * of a hidden console application, and then run that hidden
	     * console as a detached process.
	     */

	    startInfo.wShowWindow = SW_HIDE;
	    startInfo.dwFlags |= STARTF_USESHOWWINDOW;
	    createFlags = CREATE_NEW_CONSOLE;
	    Tcl_DStringAppend(&cmdLine, "cmd.exe /c ", -1);
	} else {
	    createFlags = DETACHED_PROCESS;
	} 
    } else {
	if (HasConsole()) {
	    createFlags = 0;
	} else {
	    createFlags = DETACHED_PROCESS;
	}
	
	if (applType == APPL_DOS) {
	    /*
	     * Under Windows 95, 16-bit DOS applications do not work well 
	     * with pipes:
	     *
	     * 1. EOF on a pipe between a detached 16-bit DOS application 
	     * and another application is not seen at the other
	     * end of the pipe, so the listening process blocks forever on 
	     * reads.  This inablity to detect EOF happens when either a 
	     * 16-bit app or the 32-bit app is the listener.  
	     *
	     * 2. If a 16-bit DOS application (detached or not) blocks when 
	     * writing to a pipe, it will never wake up again, and it
	     * eventually brings the whole system down around it.
	     *
	     * The 16-bit application is run as a normal process inside
	     * of a hidden helper console app, and this helper may be run
	     * as a detached process.  If any of the stdio handles is
	     * a pipe, the helper application accumulates information 
	     * into temp files and forwards it to or from the DOS 
	     * application as appropriate.  This means that DOS apps 
	     * must receive EOF from a stdin pipe before they will actually
	     * begin, and must finish generating stdout or stderr before 
	     * the data will be sent to the next stage of the pipe.
	     *
	     * The helper app should be located in the same directory as
	     * the tcl dll.
	     */

	    if (createFlags != 0) {
		startInfo.wShowWindow = SW_HIDE;
		startInfo.dwFlags |= STARTF_USESHOWWINDOW;
		createFlags = CREATE_NEW_CONSOLE;
	    }
	    Tcl_DStringAppend(&cmdLine, "tclpip" STRINGIFY(TCL_MAJOR_VERSION) 
		    STRINGIFY(TCL_MINOR_VERSION) ".dll ", -1);
	}
    }
    
    /*
     * cmdLine gets the full command line used to invoke the executable,
     * including the name of the executable itself.  The command line
     * arguments in argv[] are stored in cmdLine separated by spaces. 
     * Special characters in individual arguments from argv[] must be 
     * quoted when being stored in cmdLine.
     *
     * When calling any application, bear in mind that arguments that 
     * specify a path name are not converted.  If an argument contains 
     * forward slashes as path separators, it may or may not be 
     * recognized as a path name, depending on the program.  In general,
     * most applications accept forward slashes only as option 
     * delimiters and backslashes only as paths.
     *
     * Additionally, when calling a 16-bit dos or windows application, 
     * all path names must use the short, cryptic, path format (e.g., 
     * using ab~1.def instead of "a b.default").  
     */

    BuildCommandLine(execPath, argc, argv, &cmdLine);

    if ((*tclWinProcs->createProcessProc)(NULL, 
	    (TCHAR *) Tcl_DStringValue(&cmdLine), NULL, NULL, TRUE, 
	    createFlags, NULL, NULL, &startInfo, &procInfo) == 0) {
	TclWinConvertError(GetLastError());
	Tcl_AppendResult(interp, "couldn't execute \"", argv[0],
		"\": ", Tcl_PosixError(interp), (char *) NULL);
	goto end;
    }

    /*
     * This wait is used to force the OS to give some time to the DOS
     * process.
     */

    if (applType == APPL_DOS) {
	WaitForSingleObject(procInfo.hProcess, 50);
    }

    /* 
     * "When an application spawns a process repeatedly, a new thread 
     * instance will be created for each process but the previous 
     * instances may not be cleaned up.  This results in a significant 
     * virtual memory loss each time the process is spawned.  If there 
     * is a WaitForInputIdle() call between CreateProcess() and
     * CloseHandle(), the problem does not occur." PSS ID Number: Q124121
     */

    WaitForInputIdle(procInfo.hProcess, 5000);
    CloseHandle(procInfo.hThread);

    *pidPtr = (Tcl_Pid) procInfo.hProcess;
    if (*pidPtr != 0) {
	TclWinAddProcess(procInfo.hProcess, procInfo.dwProcessId);
    }
    result = TCL_OK;

    end:
    Tcl_DStringFree(&cmdLine);
    if (startInfo.hStdInput != INVALID_HANDLE_VALUE) {
        CloseHandle(startInfo.hStdInput);
    }
    if (startInfo.hStdOutput != INVALID_HANDLE_VALUE) {
        CloseHandle(startInfo.hStdOutput);
    }
    if (startInfo.hStdError != INVALID_HANDLE_VALUE) {
	CloseHandle(startInfo.hStdError);
    }
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * HasConsole --
 *
 *	Determines whether the current application is attached to a
 *	console.
 *
 * Results:
 *	Returns TRUE if this application has a console, else FALSE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static BOOL
HasConsole()
{
    HANDLE handle;
    
    handle = CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE,
	    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
	return TRUE;
    } else {
        return FALSE;
    }
}

/*
 *--------------------------------------------------------------------
 *
 * ApplicationType --
 *
 *	Search for the specified program and identify if it refers to a DOS,
 *	Windows 3.X, or Win32 program.  Used to determine how to invoke 
 *	a program, or if it can even be invoked.
 *
 *	It is possible to almost positively identify DOS and Windows 
 *	applications that contain the appropriate magic numbers.  However, 
 *	DOS .com files do not seem to contain a magic number; if the program 
 *	name ends with .com and could not be identified as a Windows .com
 *	file, it will be assumed to be a DOS application, even if it was
 *	just random data.  If the program name does not end with .com, no 
 *	such assumption is made.
 *
 *	The Win32 procedure GetBinaryType incorrectly identifies any 
 *	junk file that ends with .exe as a dos executable and some 
 *	executables that don't end with .exe as not executable.  Plus it 
 *	doesn't exist under win95, so I won't feel bad about reimplementing
 *	functionality.
 *
 * Results:
 *	The return value is one of APPL_DOS, APPL_WIN3X, or APPL_WIN32
 *	if the filename referred to the corresponding application type.
 *	If the file name could not be found or did not refer to any known 
 *	application type, APPL_NONE is returned and an error message is 
 *	left in interp.  .bat files are identified as APPL_DOS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ApplicationType(interp, originalName, fullName)
    Tcl_Interp *interp;		/* Interp, for error message. */
    const char *originalName;	/* Name of the application to find. */
    char fullName[];		/* Filled with complete path to 
				 * application. */
{
    int applType, i, nameLen, found;
    HANDLE hFile;
    TCHAR *rest;
    char *ext;
    char buf[2];
    DWORD attr, read;
    IMAGE_DOS_HEADER header;
    Tcl_DString nameBuf, ds;
    TCHAR *nativeName;
    WCHAR nativeFullPath[MAX_PATH];
    static char extensions[][5] = {"", ".com", ".exe", ".bat"};

    /* Look for the program as an external program.  First try the name
     * as it is, then try adding .com, .exe, and .bat, in that order, to
     * the name, looking for an executable.
     *
     * Using the raw SearchPath() procedure doesn't do quite what is 
     * necessary.  If the name of the executable already contains a '.' 
     * character, it will not try appending the specified extension when
     * searching (in other words, SearchPath will not find the program 
     * "a.b.exe" if the arguments specified "a.b" and ".exe").   
     * So, first look for the file as it is named.  Then manually append 
     * the extensions, looking for a match.  
     */

    applType = APPL_NONE;
    Tcl_DStringInit(&nameBuf);
    Tcl_DStringAppend(&nameBuf, originalName, -1);
    nameLen = Tcl_DStringLength(&nameBuf);

    for (i = 0; i < (int) (sizeof(extensions) / sizeof(extensions[0])); i++) {
	Tcl_DStringSetLength(&nameBuf, nameLen);
	Tcl_DStringAppend(&nameBuf, extensions[i], -1);
        nativeName = Tcl_WinUtfToTChar(Tcl_DStringValue(&nameBuf), 
		Tcl_DStringLength(&nameBuf), &ds);
	found = (*tclWinProcs->searchPathProc)(NULL, nativeName, NULL, 
		MAX_PATH, nativeFullPath, &rest);
	Tcl_DStringFree(&ds);
	if (found == 0) {
	    continue;
	}

	/*
	 * Ignore matches on directories or data files, return if identified
	 * a known type.
	 */

	attr = (*tclWinProcs->getFileAttributesProc)((TCHAR *) nativeFullPath);
	if ((attr == 0xffffffff) || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
	    continue;
	}
	strcpy(fullName, Tcl_WinTCharToUtf((TCHAR *) nativeFullPath, -1, &ds));
	Tcl_DStringFree(&ds);

	ext = strrchr(fullName, '.');
	if ((ext != NULL) && (stricmp(ext, ".bat") == 0)) {
	    applType = APPL_DOS;
	    break;
	}
	
	hFile = (*tclWinProcs->createFileProc)((TCHAR *) nativeFullPath, 
		GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
	    continue;
	}

	header.e_magic = 0;
	ReadFile(hFile, (void *) &header, sizeof(header), &read, NULL);
	if (header.e_magic != IMAGE_DOS_SIGNATURE) {
	    /* 
	     * Doesn't have the magic number for relocatable executables.  If 
	     * filename ends with .com, assume it's a DOS application anyhow.
	     * Note that we didn't make this assumption at first, because some
	     * supposed .com files are really 32-bit executables with all the
	     * magic numbers and everything.  
	     */

	    CloseHandle(hFile);
	    if ((ext != NULL) && (strcmp(ext, ".com") == 0)) {
		applType = APPL_DOS;
		break;
	    }
	    continue;
	}
	if (header.e_lfarlc != sizeof(header)) {
	    /* 
	     * All Windows 3.X and Win32 and some DOS programs have this value
	     * set here.  If it doesn't, assume that since it already had the 
	     * other magic number it was a DOS application.
	     */

	    CloseHandle(hFile);
	    applType = APPL_DOS;
	    break;
	}

	/* 
	 * The DWORD at header.e_lfanew points to yet another magic number.
	 */

	buf[0] = '\0';
	SetFilePointer(hFile, header.e_lfanew, NULL, FILE_BEGIN);
	ReadFile(hFile, (void *) buf, 2, &read, NULL);
	CloseHandle(hFile);

	if ((buf[0] == 'N') && (buf[1] == 'E')) {
	    applType = APPL_WIN3X;
	} else if ((buf[0] == 'P') && (buf[1] == 'E')) {
	    applType = APPL_WIN32;
	} else {
	    /*
	     * Strictly speaking, there should be a test that there
	     * is an 'L' and 'E' at buf[0..1], to identify the type as 
	     * DOS, but of course we ran into a DOS executable that 
	     * _doesn't_ have the magic number -- specifically, one
	     * compiled using the Lahey Fortran90 compiler.
	     */

	    applType = APPL_DOS;
	}
	break;
    }
    Tcl_DStringFree(&nameBuf);

    if (applType == APPL_NONE) {
	TclWinConvertError(GetLastError());
	Tcl_AppendResult(interp, "couldn't execute \"", originalName,
		"\": ", Tcl_PosixError(interp), (char *) NULL);
	return APPL_NONE;
    }

    if ((applType == APPL_DOS) || (applType == APPL_WIN3X)) {
	/* 
	 * Replace long path name of executable with short path name for 
	 * 16-bit applications.  Otherwise the application may not be able
	 * to correctly parse its own command line to separate off the 
	 * application name from the arguments.
	 */

	(*tclWinProcs->getShortPathNameProc)((TCHAR *) nativeFullPath, 
		nativeFullPath, MAX_PATH);
	strcpy(fullName, Tcl_WinTCharToUtf((TCHAR *) nativeFullPath, -1, &ds));
	Tcl_DStringFree(&ds);
    }
    return applType;
}

/*    
 *----------------------------------------------------------------------
 *
 * BuildCommandLine --
 *
 *	The command line arguments are stored in linePtr separated
 *	by spaces, in a form that CreateProcess() understands.  Special 
 *	characters in individual arguments from argv[] must be quoted 
 *	when being stored in cmdLine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
BuildCommandLine(
    CONST char *executable,	/* Full path of executable (including 
				 * extension).  Replacement for argv[0]. */
    int argc,			/* Number of arguments. */
    char **argv,		/* Argument strings in UTF. */
    Tcl_DString *linePtr)	/* Initialized Tcl_DString that receives the
				 * command line (TCHAR). */
{
    CONST char *arg, *start, *special;
    int quote, i;
    Tcl_DString ds;

    Tcl_DStringInit(&ds);

    /*
     * Prime the path.
     */
    
    Tcl_DStringAppend(&ds, Tcl_DStringValue(linePtr), -1);
    
    for (i = 0; i < argc; i++) {
	if (i == 0) {
	    arg = executable;
	} else {
	    arg = argv[i];
	    Tcl_DStringAppend(&ds, " ", 1);
	}

	quote = 0;
	if (argv[i][0] == '\0') {
	    quote = 1;
	} else {
	    for (start = argv[i]; *start != '\0'; start++) {
		if (isspace(*start)) { /* INTL: ISO space. */
		    quote = 1;
		    break;
		}
	    }
	}
	if (quote) {
	    Tcl_DStringAppend(&ds, "\"", 1);
	}

	start = arg;	    
	for (special = arg; ; ) {
	    if ((*special == '\\') && 
		    (special[1] == '\\' || special[1] == '"')) {
		Tcl_DStringAppend(&ds, start, special - start);
		start = special;
		while (1) {
		    special++;
		    if (*special == '"') {
			/* 
			 * N backslashes followed a quote -> insert 
			 * N * 2 + 1 backslashes then a quote.
			 */

			Tcl_DStringAppend(&ds, start, special - start);
			break;
		    }
		    if (*special != '\\') {
			break;
		    }
		}
		Tcl_DStringAppend(&ds, start, special - start);
		start = special;
	    }
	    if (*special == '"') {
		Tcl_DStringAppend(&ds, start, special - start);
		Tcl_DStringAppend(&ds, "\\\"", 2);
		start = special + 1;
	    }
	    if (*special == '\0') {
		break;
	    }
	    special++;
	}
	Tcl_DStringAppend(&ds, start, special - start);
	if (quote) {
	    Tcl_DStringAppend(&ds, "\"", 1);
	}
    }
    Tcl_WinUtfToTChar(Tcl_DStringValue(&ds), Tcl_DStringLength(&ds), linePtr);
    Tcl_DStringFree(&ds);
}

/*
 *----------------------------------------------------------------------
 *
 * MakeTempFile --
 *
 *	Helper function for TclpCreateProcess under Win32s.  Makes a 
 *	temporary file that _won't_ go away automatically when it's file
 *	handle is closed.  Used for simulated pipes, which are written
 *	in one pass and reopened and read in the next pass.
 *
 * Results:
 *	namePtr is filled with the name of the temporary file.
 *
 * Side effects:
 *	A temporary file with the name specified by namePtr is created.  
 *	The caller is responsible for deleting this temporary file.
 *
 *----------------------------------------------------------------------
 */

static char *
MakeTempFile(namePtr)
    Tcl_DString *namePtr;	/* Initialized Tcl_DString that is filled 
				 * with the name of the temporary file that 
				 * was created. */
{
    char name[MAX_PATH];

    if (TempFileName((WCHAR *) name) == 0) {
	return NULL;
    }

    Tcl_DStringAppend(namePtr, name, -1);
    return Tcl_DStringValue(namePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * CopyChannel --
 *
 *	Helper function used by TclpCreateProcess under Win32s.  Copies
 *	what remains of source file to destination file; source file 
 *	pointer need not be positioned at the beginning of the file if
 *	all of source file is not desired, but data is copied up to end 
 *	of source file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
CopyChannel(
    HANDLE dst,			/* Destination file. */
    HANDLE src)			/* Source file. */
{
    char buf[8192];
    DWORD dwRead, dwWrite;

    while (ReadFile(src, buf, sizeof(buf), &dwRead, NULL) != FALSE) {
	if (dwRead == 0) {
	    break;
	}
	if (WriteFile(dst, buf, dwRead, &dwWrite, NULL) == FALSE) {
	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpCreateCommandChannel --
 *
 *	This function is called by Tcl_OpenCommandChannel to perform
 *	the platform specific channel initialization for a command
 *	channel.
 *
 * Results:
 *	Returns a new channel or NULL on failure.
 *
 * Side effects:
 *	Allocates a new channel.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
TclpCreateCommandChannel(
    TclFile readFile,		/* If non-null, gives the file for reading. */
    TclFile writeFile,		/* If non-null, gives the file for writing. */
    TclFile errorFile,		/* If non-null, gives the file where errors
				 * can be read. */
    int numPids,		/* The number of pids in the pid array. */
    Tcl_Pid *pidPtr)		/* An array of process identifiers. */
{
    char channelName[16 + TCL_INTEGER_SPACE];
    int channelId;
    DWORD id;
    PipeInfo *infoPtr = (PipeInfo *) ckalloc((unsigned) sizeof(PipeInfo));
    OSVERSIONINFO os;
    int useThreads;

    /*
     * Fetch the OS version info.
     */

    os.dwOSVersionInfoSize = sizeof(os);
    GetVersionEx(&os);
    useThreads = (os.dwPlatformId != VER_PLATFORM_WIN32s);

    PipeInit();

    infoPtr->watchMask = 0;
    infoPtr->flags = 0;
    infoPtr->readFlags = 0;
    infoPtr->readFile = readFile;
    infoPtr->writeFile = writeFile;
    infoPtr->errorFile = errorFile;
    infoPtr->numPids = numPids;
    infoPtr->pidPtr = pidPtr;
    infoPtr->writeBuf = 0;
    infoPtr->writeBufLen = 0;
    infoPtr->writeError = 0;

    /*
     * Use one of the fds associated with the channel as the
     * channel id.
     */

    if (readFile) {
	WinPipe *pipePtr = (WinPipe *) readFile;
	if (pipePtr->file.type == WIN32S_PIPE
		&& pipePtr->file.handle == INVALID_HANDLE_VALUE) {
	    pipePtr->file.handle = CreateFileA(pipePtr->fileName, GENERIC_READ,
		    0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	channelId = (int) pipePtr->file.handle;
    } else if (writeFile) {
	channelId = (int) ((WinFile*)writeFile)->handle;
    } else if (errorFile) {
	channelId = (int) ((WinFile*)errorFile)->handle;
    } else {
	channelId = 0;
    }

    infoPtr->validMask = 0;

    infoPtr->threadId = Tcl_GetCurrentThread();

    if (readFile != NULL) {
	if (useThreads) {
	    /*
	     * Start the background reader thread.
	     */

	    infoPtr->readable = CreateEvent(NULL, TRUE, TRUE, NULL);
	    infoPtr->startReader = CreateEvent(NULL, FALSE, FALSE, NULL);
	    infoPtr->readThread = CreateThread(NULL, 8000, PipeReaderThread,
		    infoPtr, 0, &id);
	    SetThreadPriority(infoPtr->readThread, THREAD_PRIORITY_HIGHEST); 
	} else {
	    infoPtr->readThread = 0;
	}
        infoPtr->validMask |= TCL_READABLE;
    } else {
	infoPtr->readThread = 0;
    }
    if (writeFile != NULL) {
	if (useThreads) {
	    /*
	     * Start the background writeer thwrite.
	     */

	    infoPtr->writable = CreateEvent(NULL, TRUE, TRUE, NULL);
	    infoPtr->startWriter = CreateEvent(NULL, FALSE, FALSE, NULL);
	    infoPtr->writeThread = CreateThread(NULL, 8000, PipeWriterThread,
		    infoPtr, 0, &id);
	    SetThreadPriority(infoPtr->readThread, THREAD_PRIORITY_HIGHEST); 
	} else {
	    infoPtr->writeThread = 0;
	}
        infoPtr->validMask |= TCL_WRITABLE;
    }

    /*
     * For backward compatibility with previous versions of Tcl, we
     * use "file%d" as the base name for pipes even though it would
     * be more natural to use "pipe%d".
     * Use the pointer to keep the channel names unique, in case
     * channels share handles (stdin/stdout).
     */

    wsprintfA(channelName, "file%lx", infoPtr);
    infoPtr->channel = Tcl_CreateChannel(&pipeChannelType, channelName,
            (ClientData) infoPtr, infoPtr->validMask);

    /*
     * Pipes have AUTO translation mode on Windows and ^Z eof char, which
     * means that a ^Z will be appended to them at close. This is needed
     * for Windows programs that expect a ^Z at EOF.
     */

    Tcl_SetChannelOption((Tcl_Interp *) NULL, infoPtr->channel,
	    "-translation", "auto");
    Tcl_SetChannelOption((Tcl_Interp *) NULL, infoPtr->channel,
	    "-eofchar", "\032 {}");
    return infoPtr->channel;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetAndDetachPids --
 *
 *	Stores a list of the command PIDs for a command channel in
 *	the interp's result.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies the interp's result.
 *
 *----------------------------------------------------------------------
 */

void
TclGetAndDetachPids(
    Tcl_Interp *interp,
    Tcl_Channel chan)
{
    PipeInfo *pipePtr;
    Tcl_ChannelType *chanTypePtr;
    int i;
    char buf[TCL_INTEGER_SPACE];

    /*
     * Punt if the channel is not a command channel.
     */

    chanTypePtr = Tcl_GetChannelType(chan);
    if (chanTypePtr != &pipeChannelType) {
        return;
    }

    pipePtr = (PipeInfo *) Tcl_GetChannelInstanceData(chan);
    for (i = 0; i < pipePtr->numPids; i++) {
        wsprintfA(buf, "%lu", TclpGetPid(pipePtr->pidPtr[i]));
        Tcl_AppendElement(interp, buf);
        Tcl_DetachPids(1, &(pipePtr->pidPtr[i]));
    }
    if (pipePtr->numPids > 0) {
        ckfree((char *) pipePtr->pidPtr);
        pipePtr->numPids = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PipeBlockModeProc --
 *
 *	Set blocking or non-blocking mode on channel.
 *
 * Results:
 *	0 if successful, errno when failed.
 *
 * Side effects:
 *	Sets the device into blocking or non-blocking mode.
 *
 *----------------------------------------------------------------------
 */

static int
PipeBlockModeProc(
    ClientData instanceData,	/* Instance data for channel. */
    int mode)			/* TCL_MODE_BLOCKING or
                                 * TCL_MODE_NONBLOCKING. */
{
    PipeInfo *infoPtr = (PipeInfo *) instanceData;
    
    /*
     * Pipes on Windows can not be switched between blocking and nonblocking,
     * hence we have to emulate the behavior. This is done in the input
     * function by checking against a bit in the state. We set or unset the
     * bit here to cause the input function to emulate the correct behavior.
     */

    if (mode == TCL_MODE_NONBLOCKING) {
	infoPtr->flags |= PIPE_ASYNC;
    } else {
	infoPtr->flags &= ~(PIPE_ASYNC);
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * PipeClose2Proc --
 *
 *	Closes a pipe based IO channel.
 *
 * Results:
 *	0 on success, errno otherwise.
 *
 * Side effects:
 *	Closes the physical channel.
 *
 *----------------------------------------------------------------------
 */

static int
PipeClose2Proc(
    ClientData instanceData,	/* Pointer to PipeInfo structure. */
    Tcl_Interp *interp,		/* For error reporting. */
    int flags)			/* Flags that indicate which side to close. */
{
    PipeInfo *pipePtr = (PipeInfo *) instanceData;
    Tcl_Channel errChan;
    int errorCode, result;
    PipeInfo *infoPtr, **nextPtrPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    errorCode = 0;
    if ((!flags || (flags == TCL_CLOSE_READ))
	    && (pipePtr->readFile != NULL)) {
	/*
	 * Clean up the background thread if necessary.  Note that this
	 * must be done before we can close the file, since the 
	 * thread may be blocking trying to read from the pipe.
	 */

	if (pipePtr->readThread) {
	    /*
	     * Forcibly terminate the background thread.  We cannot rely on the
	     * thread to cleanly terminate itself because we have no way of
	     * closing the pipe handle without blocking in the case where the
	     * thread is in the middle of an I/O operation.  Note that we need
	     * to guard against terminating the thread while it is in the
	     * middle of Tcl_ThreadAlert because it won't be able to release
	     * the notifier lock.
	     */

	    Tcl_MutexLock(&pipeMutex);
	    TerminateThread(pipePtr->readThread, 0);

	    /*
	     * Wait for the thread to terminate.  This ensures that we are
	     * completely cleaned up before we leave this function. 
	     */

	    WaitForSingleObject(pipePtr->readThread, INFINITE);
	    Tcl_MutexUnlock(&pipeMutex);

	    CloseHandle(pipePtr->readThread);
	    CloseHandle(pipePtr->readable);
	    CloseHandle(pipePtr->startReader);
	    pipePtr->readThread = NULL;
	}
	if (TclpCloseFile(pipePtr->readFile) != 0) {
	    errorCode = errno;
	}
	pipePtr->validMask &= ~TCL_READABLE;
	pipePtr->readFile = NULL;
    }
    if ((!flags || (flags & TCL_CLOSE_WRITE))
	    && (pipePtr->writeFile != NULL)) {
	/*
	 * Wait for the writer thread to finish the current buffer, then
	 * terminate the thread and close the handles.  If the channel is
	 * nonblocking, there should be no pending write operations.
	 */

	if (pipePtr->writeThread) {
	    WaitForSingleObject(pipePtr->writable, INFINITE);

	    /*
	     * Forcibly terminate the background thread.  We cannot rely on the
	     * thread to cleanly terminate itself because we have no way of
	     * closing the pipe handle without blocking in the case where the
	     * thread is in the middle of an I/O operation.  Note that we need
	     * to guard against terminating the thread while it is in the
	     * middle of Tcl_ThreadAlert because it won't be able to release
	     * the notifier lock.
	     */

	    Tcl_MutexLock(&pipeMutex);
	    TerminateThread(pipePtr->writeThread, 0);

	    /*
	     * Wait for the thread to terminate.  This ensures that we are
	     * completely cleaned up before we leave this function. 
	     */

	    WaitForSingleObject(pipePtr->writeThread, INFINITE);
	    Tcl_MutexUnlock(&pipeMutex);


	    CloseHandle(pipePtr->writeThread);
	    CloseHandle(pipePtr->writable);
	    CloseHandle(pipePtr->startWriter);
	    pipePtr->writeThread = NULL;
	}
	if (TclpCloseFile(pipePtr->writeFile) != 0) {
	    if (errorCode == 0) {
		errorCode = errno;
	    }
	}
	pipePtr->validMask &= ~TCL_WRITABLE;
	pipePtr->writeFile = NULL;
    }

    pipePtr->watchMask &= pipePtr->validMask;

    /*
     * Don't free the channel if any of the flags were set.
     */

    if (flags) {
	return errorCode;
    }

    /*
     * Remove the file from the list of watched files.
     */

    for (nextPtrPtr = &(tsdPtr->firstPipePtr), infoPtr = *nextPtrPtr;
	    infoPtr != NULL;
	    nextPtrPtr = &infoPtr->nextPtr, infoPtr = *nextPtrPtr) {
	if (infoPtr == (PipeInfo *)pipePtr) {
	    *nextPtrPtr = infoPtr->nextPtr;
	    break;
	}
    }

    /*
     * Wrap the error file into a channel and give it to the cleanup
     * routine.  If we are running in Win32s, just delete the error file
     * immediately, because it was never used.
     */

    if (pipePtr->errorFile) {
	WinFile *filePtr;
	OSVERSIONINFO os;

	os.dwOSVersionInfoSize = sizeof(os);
	GetVersionEx(&os);
	if (os.dwPlatformId == VER_PLATFORM_WIN32s) {
	    TclpCloseFile(pipePtr->errorFile);
	    errChan = NULL;
	} else {
	    filePtr = (WinFile*)pipePtr->errorFile;
	    errChan = Tcl_MakeFileChannel((ClientData) filePtr->handle,
		    TCL_READABLE);
	    ckfree((char *) filePtr);
	}
    } else {
        errChan = NULL;
    }

    result = TclCleanupChildren(interp, pipePtr->numPids, pipePtr->pidPtr,
            errChan);

    if (pipePtr->numPids > 0) {
        ckfree((char *) pipePtr->pidPtr);
    }

    if (pipePtr->writeBuf != NULL) {
	ckfree(pipePtr->writeBuf);
    }

    ckfree((char*) pipePtr);

    if (errorCode == 0) {
        return result;
    }
    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * PipeInputProc --
 *
 *	Reads input from the IO channel into the buffer given. Returns
 *	count of how many bytes were actually read, and an error indication.
 *
 * Results:
 *	A count of how many bytes were read is returned and an error
 *	indication is returned in an output argument.
 *
 * Side effects:
 *	Reads input from the actual channel.
 *
 *----------------------------------------------------------------------
 */

static int
PipeInputProc(
    ClientData instanceData,		/* Pipe state. */
    char *buf,				/* Where to store data read. */
    int bufSize,			/* How much space is available
                                         * in the buffer? */
    int *errorCode)			/* Where to store error code. */
{
    PipeInfo *infoPtr = (PipeInfo *) instanceData;
    WinFile *filePtr = (WinFile*) infoPtr->readFile;
    DWORD count, bytesRead = 0;
    int result;

    *errorCode = 0;
    if (filePtr->type == WIN32S_PIPE) {
	if (((WinPipe *)filePtr)->otherPtr != NULL) {
	    panic("PipeInputProc: child process isn't finished writing");
	}
	if (filePtr->handle == INVALID_HANDLE_VALUE) {
	    filePtr->handle = CreateFileA(((WinPipe *)filePtr)->fileName,
		    GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
		    NULL);
	}
	if (filePtr->handle == INVALID_HANDLE_VALUE) {
	    goto error;
	}
    } else {
	/*
	 * Synchronize with the reader thread.
	 */

	result = WaitForRead(infoPtr, (infoPtr->flags & PIPE_ASYNC) ? 0 : 1);

	/*
	 * If an error occurred, return immediately.
	 */

	if (result == -1) {
	    *errorCode = errno;
	    return -1;
	}

	if (infoPtr->readFlags & PIPE_EXTRABYTE) {
	    /*
	     * The reader thread consumed 1 byte as a side effect of
	     * waiting so we need to move it into the buffer.
	     */

	    *buf = infoPtr->extraByte;
	    infoPtr->readFlags &= ~PIPE_EXTRABYTE;
	    buf++;
	    bufSize--;
	    bytesRead = 1;

	    /*
	     * If further read attempts would block, return what we have.
	     */

	    if (result == 0) {
		return bytesRead;
	    }
	}
    }

    /*
     * Attempt to read bufSize bytes.  The read will return immediately
     * if there is any data available.  Otherwise it will block until
     * at least one byte is available or an EOF occurs.
     */

    if (ReadFile(filePtr->handle, (LPVOID) buf, (DWORD) bufSize, &count,
	    (LPOVERLAPPED) NULL) == TRUE) {
	return bytesRead + count;
    } else if (bytesRead) {
	/*
	 * Ignore errors if we have data to return.
	 */

	return bytesRead;
    }

    error:
    TclWinConvertError(GetLastError());
    if (errno == EPIPE) {
	infoPtr->readFlags |= PIPE_EOF;
	return 0;
    }
    *errorCode = errno;
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * PipeOutputProc --
 *
 *	Writes the given output on the IO channel. Returns count of how
 *	many characters were actually written, and an error indication.
 *
 * Results:
 *	A count of how many characters were written is returned and an
 *	error indication is returned in an output argument.
 *
 * Side effects:
 *	Writes output on the actual channel.
 *
 *----------------------------------------------------------------------
 */

static int
PipeOutputProc(
    ClientData instanceData,		/* Pipe state. */
    char *buf,				/* The data buffer. */
    int toWrite,			/* How many bytes to write? */
    int *errorCode)			/* Where to store error code. */
{
    PipeInfo *infoPtr = (PipeInfo *) instanceData;
    WinFile *filePtr = (WinFile*) infoPtr->writeFile;
    DWORD bytesWritten, timeout;
    
    *errorCode = 0;
    timeout = (infoPtr->flags & PIPE_ASYNC) ? 0 : INFINITE;
    if (WaitForSingleObject(infoPtr->writable, timeout) == WAIT_TIMEOUT) {
	/*
	 * The writer thread is blocked waiting for a write to complete
	 * and the channel is in non-blocking mode.
	 */

	errno = EAGAIN;
	goto error;
    }
    
    /*
     * Check for a background error on the last write.
     */

    if (infoPtr->writeError) {
	TclWinConvertError(infoPtr->writeError);
	infoPtr->writeError = 0;
	goto error;
    }

    if (infoPtr->flags & PIPE_ASYNC) {
	/*
	 * The pipe is non-blocking, so copy the data into the output
	 * buffer and restart the writer thread.
	 */

	if (toWrite > infoPtr->writeBufLen) {
	    /*
	     * Reallocate the buffer to be large enough to hold the data.
	     */

	    if (infoPtr->writeBuf) {
		ckfree(infoPtr->writeBuf);
	    }
	    infoPtr->writeBufLen = toWrite;
	    infoPtr->writeBuf = ckalloc(toWrite);
	}
	memcpy(infoPtr->writeBuf, buf, toWrite);
	infoPtr->toWrite = toWrite;
	ResetEvent(infoPtr->writable);
	SetEvent(infoPtr->startWriter);
	bytesWritten = toWrite;
    } else {
	/*
	 * In the blocking case, just try to write the buffer directly.
	 * This avoids an unnecessary copy.
	 */

	if (WriteFile(filePtr->handle, (LPVOID) buf, (DWORD) toWrite,
		&bytesWritten, (LPOVERLAPPED) NULL) == FALSE) {
	    TclWinConvertError(GetLastError());
	    goto error;
	}
    }
    return bytesWritten;

    error:
    *errorCode = errno;
    return -1;

}

/*
 *----------------------------------------------------------------------
 *
 * PipeEventProc --
 *
 *	This function is invoked by Tcl_ServiceEvent when a file event
 *	reaches the front of the event queue.  This procedure invokes
 *	Tcl_NotifyChannel on the pipe.
 *
 * Results:
 *	Returns 1 if the event was handled, meaning it should be removed
 *	from the queue.  Returns 0 if the event was not handled, meaning
 *	it should stay on the queue.  The only time the event isn't
 *	handled is if the TCL_FILE_EVENTS flag bit isn't set.
 *
 * Side effects:
 *	Whatever the notifier callback does.
 *
 *----------------------------------------------------------------------
 */

static int
PipeEventProc(
    Tcl_Event *evPtr,		/* Event to service. */
    int flags)			/* Flags that indicate what events to
				 * handle, such as TCL_FILE_EVENTS. */
{
    PipeEvent *pipeEvPtr = (PipeEvent *)evPtr;
    PipeInfo *infoPtr;
    WinFile *filePtr;
    int mask;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return 0;
    }

    /*
     * Search through the list of watched pipes for the one whose handle
     * matches the event.  We do this rather than simply dereferencing
     * the handle in the event so that pipes can be deleted while the
     * event is in the queue.
     */

    for (infoPtr = tsdPtr->firstPipePtr; infoPtr != NULL;
	    infoPtr = infoPtr->nextPtr) {
	if (pipeEvPtr->infoPtr == infoPtr) {
	    infoPtr->flags &= ~(PIPE_PENDING);
	    break;
	}
    }

    /*
     * Remove stale events.
     */

    if (!infoPtr) {
	return 1;
    }

    /*
     * If we aren't on Win32s, check to see if the pipe is readable.  Note
     * that we can't tell if a pipe is writable, so we always report it
     * as being writable unless we have detected EOF.
     */

    filePtr = (WinFile*) ((PipeInfo*)infoPtr)->writeFile;
    mask = 0;
    if (infoPtr->watchMask & TCL_WRITABLE) {
	if ((filePtr->type == WIN32S_PIPE)
		|| (WaitForSingleObject(infoPtr->writable, 0)
			!= WAIT_TIMEOUT)) {
	    mask = TCL_WRITABLE;
	}
    }

    filePtr = (WinFile*) ((PipeInfo*)infoPtr)->readFile;
    if (infoPtr->watchMask & TCL_READABLE) {
	if ((filePtr->type == WIN32S_PIPE)
		|| (WaitForRead(infoPtr, 0) >= 0)) {
	    if (infoPtr->readFlags & PIPE_EOF) {
		mask = TCL_READABLE;
	    } else {
		mask |= TCL_READABLE;
	    }
	} 
    }

    /*
     * Inform the channel of the events.
     */

    Tcl_NotifyChannel(infoPtr->channel, infoPtr->watchMask & mask);
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * PipeWatchProc --
 *
 *	Called by the notifier to set up to watch for events on this
 *	channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
PipeWatchProc(
    ClientData instanceData,		/* Pipe state. */
    int mask)				/* What events to watch for, OR-ed
                                         * combination of TCL_READABLE,
                                         * TCL_WRITABLE and TCL_EXCEPTION. */
{
    PipeInfo **nextPtrPtr, *ptr;
    PipeInfo *infoPtr = (PipeInfo *) instanceData;
    int oldMask = infoPtr->watchMask;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * Since most of the work is handled by the background threads,
     * we just need to update the watchMask and then force the notifier
     * to poll once. 
     */

    infoPtr->watchMask = mask & infoPtr->validMask;
    if (infoPtr->watchMask) {
	Tcl_Time blockTime = { 0, 0 };
	if (!oldMask) {
	    infoPtr->nextPtr = tsdPtr->firstPipePtr;
	    tsdPtr->firstPipePtr = infoPtr;
	}
	Tcl_SetMaxBlockTime(&blockTime);
    } else {
	if (oldMask) {
	    /*
	     * Remove the pipe from the list of watched pipes.
	     */

	    for (nextPtrPtr = &(tsdPtr->firstPipePtr), ptr = *nextPtrPtr;
		 ptr != NULL;
		 nextPtrPtr = &ptr->nextPtr, ptr = *nextPtrPtr) {
		if (infoPtr == ptr) {
		    *nextPtrPtr = ptr->nextPtr;
		    break;
		}
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PipeGetHandleProc --
 *
 *	Called from Tcl_GetChannelHandle to retrieve OS handles from
 *	inside a command pipeline based channel.
 *
 * Results:
 *	Returns TCL_OK with the fd in handlePtr, or TCL_ERROR if
 *	there is no handle for the specified direction. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
PipeGetHandleProc(
    ClientData instanceData,	/* The pipe state. */
    int direction,		/* TCL_READABLE or TCL_WRITABLE */
    ClientData *handlePtr)	/* Where to store the handle.  */
{
    PipeInfo *infoPtr = (PipeInfo *) instanceData;
    WinFile *filePtr; 

    if (direction == TCL_READABLE && infoPtr->readFile) {
	filePtr = (WinFile*) infoPtr->readFile;
	if (filePtr->type == WIN32S_PIPE) {
	    if (filePtr->handle == INVALID_HANDLE_VALUE) {
		filePtr->handle = CreateFileA(((WinPipe *)filePtr)->fileName,
			GENERIC_READ, 0, NULL, OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, NULL);
	    }
	    if (filePtr->handle == INVALID_HANDLE_VALUE) {
		return TCL_ERROR;
	    }
	}
	*handlePtr = (ClientData) filePtr->handle;
	return TCL_OK;
    }
    if (direction == TCL_WRITABLE && infoPtr->writeFile) {
	filePtr = (WinFile*) infoPtr->writeFile;
	*handlePtr = (ClientData) filePtr->handle;
	return TCL_OK;
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_WaitPid --
 *
 *	Emulates the waitpid system call.
 *
 * Results:
 *	Returns 0 if the process is still alive, -1 on an error, or
 *	the pid on a clean close.  
 *
 * Side effects:
 *	Unless WNOHANG is set and the wait times out, the process
 *	information record will be deleted and the process handle
 *	will be closed.
 *
 *----------------------------------------------------------------------
 */

Tcl_Pid
Tcl_WaitPid(
    Tcl_Pid pid,
    int *statPtr,
    int options)
{
    ProcInfo *infoPtr, **prevPtrPtr;
    int flags;
    Tcl_Pid result;
    DWORD ret;

    PipeInit();

    /*
     * If no pid is specified, do nothing.
     */
    
    if (pid == 0) {
	*statPtr = 0;
	return 0;
    }

    /*
     * Find the process on the process list.
     */

    Tcl_MutexLock(&pipeMutex);
    prevPtrPtr = &procList;
    for (infoPtr = procList; infoPtr != NULL;
	    prevPtrPtr = &infoPtr->nextPtr, infoPtr = infoPtr->nextPtr) {
	 if (infoPtr->hProcess == (HANDLE) pid) {
	    break;
	}
    }
    Tcl_MutexUnlock(&pipeMutex);

    /*
     * If the pid is not one of the processes we know about (we started it)
     * then do nothing.
     */
    		     
    if (infoPtr == NULL) {
        *statPtr = 0;
	return 0;
    }

    /*
     * Officially "wait" for it to finish. We either poll (WNOHANG) or
     * wait for an infinite amount of time.
     */
    
    if (options & WNOHANG) {
	flags = 0;
    } else {
	flags = INFINITE;
    }
    ret = WaitForSingleObject(infoPtr->hProcess, flags);
    if (ret == WAIT_TIMEOUT) {
	*statPtr = 0;
	if (options & WNOHANG) {
	    return 0;
	} else {
	    result = 0;
	}
    } else if (ret != WAIT_FAILED) {
	GetExitCodeProcess(infoPtr->hProcess, (DWORD*)statPtr);
	*statPtr = ((*statPtr << 8) & 0xff00);
	result = pid;
    } else {
	errno = ECHILD;
        *statPtr = ECHILD;
	result = (Tcl_Pid) -1;
    }

    /*
     * Remove the process from the process list and close the process handle.
     */

    CloseHandle(infoPtr->hProcess);
    *prevPtrPtr = infoPtr->nextPtr;
    ckfree((char*)infoPtr);

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclWinAddProcess --
 *
 *     Add a process to the process list so that we can use
 *     Tcl_WaitPid on the process.
 *
 * Results:
 *     None
 *
 * Side effects:
 *	Adds the specified process handle to the process list so
 *	Tcl_WaitPid knows about it.
 *
 *----------------------------------------------------------------------
 */

void
TclWinAddProcess(hProcess, id)
    HANDLE hProcess;           /* Handle to process */
    DWORD id;                  /* Global process identifier */
{
    ProcInfo *procPtr = (ProcInfo *) ckalloc(sizeof(ProcInfo));
    procPtr->hProcess = hProcess;
    procPtr->dwProcessId = id;
    Tcl_MutexLock(&pipeMutex);
    procPtr->nextPtr = procList;
    procList = procPtr;
    Tcl_MutexUnlock(&pipeMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PidObjCmd --
 *
 *	This procedure is invoked to process the "pid" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_PidObjCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST *objv)	/* Argument strings. */
{
    Tcl_Channel chan;
    Tcl_ChannelType *chanTypePtr;
    PipeInfo *pipePtr;
    int i;
    Tcl_Obj *resultPtr;
    char buf[TCL_INTEGER_SPACE];

    if (objc > 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "?channelId?");
	return TCL_ERROR;
    }
    if (objc == 1) {
	resultPtr = Tcl_GetObjResult(interp);
	wsprintfA(buf, "%lu", (unsigned long) getpid());
	Tcl_SetStringObj(resultPtr, buf, -1);
    } else {
        chan = Tcl_GetChannel(interp, Tcl_GetStringFromObj(objv[1], NULL),
		NULL);
        if (chan == (Tcl_Channel) NULL) {
	    return TCL_ERROR;
	}
	chanTypePtr = Tcl_GetChannelType(chan);
	if (chanTypePtr != &pipeChannelType) {
	    return TCL_OK;
	}

        pipePtr = (PipeInfo *) Tcl_GetChannelInstanceData(chan);
	resultPtr = Tcl_GetObjResult(interp);
        for (i = 0; i < pipePtr->numPids; i++) {
	    wsprintfA(buf, "%lu", TclpGetPid(pipePtr->pidPtr[i]));
	    Tcl_ListObjAppendElement(/*interp*/ NULL, resultPtr,
		    Tcl_NewStringObj(buf, -1));
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WaitForRead --
 *
 *	Wait until some data is available, the pipe is at
 *	EOF or the reader thread is blocked waiting for data (if the
 *	channel is in non-blocking mode).
 *
 * Results:
 *	Returns 1 if pipe is readable.  Returns 0 if there is no data
 *	on the pipe, but there is buffered data.  Returns -1 if an
 *	error occurred.  If an error occurred, the threads may not
 *	be synchronized.
 *
 * Side effects:
 *	Updates the shared state flags and may consume 1 byte of data
 *	from the pipe.  If no error occurred, the reader thread is
 *	blocked waiting for a signal from the main thread.
 *
 *----------------------------------------------------------------------
 */

static int
WaitForRead(
    PipeInfo *infoPtr,		/* Pipe state. */
    int blocking)		/* Indicates whether call should be
				 * blocking or not. */
{
    DWORD timeout, count;
    HANDLE *handle = ((WinFile *) infoPtr->readFile)->handle;

    while (1) {
	/*
	 * Synchronize with the reader thread.
	 */
       
	timeout = blocking ? INFINITE : 0;
	if (WaitForSingleObject(infoPtr->readable, timeout) == WAIT_TIMEOUT) {
	    /*
	     * The reader thread is blocked waiting for data and the channel
	     * is in non-blocking mode.
	     */

	    errno = EAGAIN;
	    return -1;
	}

	/*
	 * At this point, the two threads are synchronized, so it is safe
	 * to access shared state.
	 */


	/*
	 * If the pipe has hit EOF, it is always readable.
	 */

	if (infoPtr->readFlags & PIPE_EOF) {
	    return 1;
	}
    
	/*
	 * Check to see if there is any data sitting in the pipe.
	 */

	if (PeekNamedPipe(handle, (LPVOID) NULL, (DWORD) 0,
		(LPDWORD) NULL, &count, (LPDWORD) NULL) != TRUE) {
	    TclWinConvertError(GetLastError());
	    /*
	     * Check to see if the peek failed because of EOF.
	     */

	    if (errno == EPIPE) {
		infoPtr->readFlags |= PIPE_EOF;
		return 1;
	    }

	    /*
	     * Ignore errors if there is data in the buffer.
	     */

	    if (infoPtr->readFlags & PIPE_EXTRABYTE) {
		return 0;
	    } else {
		return -1;
	    }
	}

	/*
	 * We found some data in the pipe, so it must be readable.
	 */

	if (count > 0) {
	    return 1;
	}

	/*
	 * The pipe isn't readable, but there is some data sitting
	 * in the buffer, so return immediately.
	 */

	if (infoPtr->readFlags & PIPE_EXTRABYTE) {
	    return 0;
	}

	/*
	 * There wasn't any data available, so reset the thread and
	 * try again.
	 */
    
	ResetEvent(infoPtr->readable);
	SetEvent(infoPtr->startReader);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PipeReaderThread --
 *
 *	This function runs in a separate thread and waits for input
 *	to become available on a pipe.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Signals the main thread when input become available.  May
 *	cause the main thread to wake up by posting a message.  May
 *	consume one byte from the pipe for each wait operation.
 *
 *----------------------------------------------------------------------
 */

static DWORD WINAPI
PipeReaderThread(LPVOID arg)
{
    PipeInfo *infoPtr = (PipeInfo *)arg;
    HANDLE *handle = ((WinFile *) infoPtr->readFile)->handle;
    DWORD count, err;
    int done = 0;

    while (!done) {
	/*
	 * Wait for the main thread to signal before attempting to wait.
	 */

	WaitForSingleObject(infoPtr->startReader, INFINITE);

	/*
	 * Try waiting for 0 bytes.  This will block until some data is
	 * available on NT, but will return immediately on Win 95.  So,
	 * if no data is available after the first read, we block until
	 * we can read a single byte off of the pipe.
	 */

	if ((ReadFile(handle, NULL, 0, &count, NULL) == FALSE)
		|| (PeekNamedPipe(handle, NULL, 0, NULL, &count,
			NULL) == FALSE)) {
	    /*
	     * The error is a result of an EOF condition, so set the
	     * EOF bit before signalling the main thread.
	     */

	    err = GetLastError();
	    if (err == ERROR_BROKEN_PIPE) {
		infoPtr->readFlags |= PIPE_EOF;
		done = 1;
	    } else if (err == ERROR_INVALID_HANDLE) {
		break;
	    }
	} else if (count == 0) {
	    if (ReadFile(handle, &(infoPtr->extraByte), 1, &count, NULL)
		    != FALSE) {
		/*
		 * One byte was consumed as a side effect of waiting
		 * for the pipe to become readable.
		 */

		infoPtr->readFlags |= PIPE_EXTRABYTE;
	    } else {
		err = GetLastError();
		if (err == ERROR_BROKEN_PIPE) {
		    /*
		     * The error is a result of an EOF condition, so set the
		     * EOF bit before signalling the main thread.
		     */

		    infoPtr->readFlags |= PIPE_EOF;
		    done = 1;
		} else if (err == ERROR_INVALID_HANDLE) {
		    break;
		}
	    }
	}

		
	/*
	 * Signal the main thread by signalling the readable event and
	 * then waking up the notifier thread.
	 */

	SetEvent(infoPtr->readable);
	
	/*
	 * Alert the foreground thread.  Note that we need to treat this like
	 * a critical section so the foreground thread does not terminate
	 * this thread while we are holding a mutex in the notifier code.
	 */

	Tcl_MutexLock(&pipeMutex);
	Tcl_ThreadAlert(infoPtr->threadId);
	Tcl_MutexUnlock(&pipeMutex);
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * PipeWriterThread --
 *
 *	This function runs in a separate thread and writes data
 *	onto a pipe.
 *
 * Results:
 *	Always returns 0.
 *
 * Side effects:
 *	Signals the main thread when an output operation is completed.
 *	May cause the main thread to wake up by posting a message.  
 *
 *----------------------------------------------------------------------
 */

static DWORD WINAPI
PipeWriterThread(LPVOID arg)
{

    PipeInfo *infoPtr = (PipeInfo *)arg;
    HANDLE *handle = ((WinFile *) infoPtr->writeFile)->handle;
    DWORD count, toWrite;
    char *buf;
    int done = 0;

    while (!done) {
	/*
	 * Wait for the main thread to signal before attempting to write.
	 */

	WaitForSingleObject(infoPtr->startWriter, INFINITE);

	buf = infoPtr->writeBuf;
	toWrite = infoPtr->toWrite;

	/*
	 * Loop until all of the bytes are written or an error occurs.
	 */

	while (toWrite > 0) {
	    if (WriteFile(handle, buf, toWrite, &count, NULL) == FALSE) {
		infoPtr->writeError = GetLastError();
		done = 1; 
		break;
	    } else {
		toWrite -= count;
		buf += count;
	    }
	}
	
	/*
	 * Signal the main thread by signalling the writable event and
	 * then waking up the notifier thread.
	 */

	SetEvent(infoPtr->writable);

	/*
	 * Alert the foreground thread.  Note that we need to treat this like
	 * a critical section so the foreground thread does not terminate
	 * this thread while we are holding a mutex in the notifier code.
	 */

	Tcl_MutexLock(&pipeMutex);
	Tcl_ThreadAlert(infoPtr->threadId);
	Tcl_MutexUnlock(&pipeMutex);
    }
    return 0;
}

