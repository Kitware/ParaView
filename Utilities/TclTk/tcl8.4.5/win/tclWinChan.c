/* 
 * tclWinChan.c
 *
 *	Channel drivers for Windows channels based on files, command
 *	pipes and TCP sockets.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclWinInt.h"
#include "tclIO.h"

/*
 * State flags used in the info structures below.
 */

#define FILE_PENDING	(1<<0)	/* Message is pending in the queue. */
#define FILE_ASYNC	(1<<1)	/* Channel is non-blocking. */
#define FILE_APPEND	(1<<2)	/* File is in append mode. */

#define FILE_TYPE_SERIAL  (FILE_TYPE_PIPE+1)
#define FILE_TYPE_CONSOLE (FILE_TYPE_PIPE+2)

/*
 * The following structure contains per-instance data for a file based channel.
 */

typedef struct FileInfo {
    Tcl_Channel channel;	/* Pointer to channel structure. */
    int validMask;		/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, or TCL_EXCEPTION: indicates
				 * which operations are valid on the file. */
    int watchMask;		/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, or TCL_EXCEPTION: indicates
				 * which events should be reported. */
    int flags;			/* State flags, see above for a list. */
    HANDLE handle;		/* Input/output file. */
    struct FileInfo *nextPtr;	/* Pointer to next registered file. */
    int dirty;                  /* Boolean flag. Set if the OS may have data
				 * pending on the channel */
} FileInfo;

typedef struct ThreadSpecificData {
    /*
     * List of all file channels currently open.
     */

    FileInfo *firstFilePtr;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * The following structure is what is added to the Tcl event queue when
 * file events are generated.
 */

typedef struct FileEvent {
    Tcl_Event header;		/* Information that is standard for
				 * all events. */
    FileInfo *infoPtr;		/* Pointer to file info structure.  Note
				 * that we still have to verify that the
				 * file exists before dereferencing this
				 * pointer. */
} FileEvent;

/*
 * Static routines for this file:
 */

static int		FileBlockProc _ANSI_ARGS_((ClientData instanceData,
			    int mode));
static void		FileChannelExitHandler _ANSI_ARGS_((
		            ClientData clientData));
static void		FileCheckProc _ANSI_ARGS_((ClientData clientData,
			    int flags));
static int		FileCloseProc _ANSI_ARGS_((ClientData instanceData,
		            Tcl_Interp *interp));
static int		FileEventProc _ANSI_ARGS_((Tcl_Event *evPtr, 
			    int flags));
static int		FileGetHandleProc _ANSI_ARGS_((ClientData instanceData,
		            int direction, ClientData *handlePtr));
static ThreadSpecificData *FileInit _ANSI_ARGS_((void));
static int		FileInputProc _ANSI_ARGS_((ClientData instanceData,
	            	    char *buf, int toRead, int *errorCode));
static int		FileOutputProc _ANSI_ARGS_((ClientData instanceData,
			    CONST char *buf, int toWrite, int *errorCode));
static int		FileSeekProc _ANSI_ARGS_((ClientData instanceData,
			    long offset, int mode, int *errorCode));
static Tcl_WideInt	FileWideSeekProc _ANSI_ARGS_((ClientData instanceData,
			    Tcl_WideInt offset, int mode, int *errorCode));
static void		FileSetupProc _ANSI_ARGS_((ClientData clientData,
			    int flags));
static void		FileWatchProc _ANSI_ARGS_((ClientData instanceData,
		            int mask));

			    
/*
 * This structure describes the channel type structure for file based IO.
 */

static Tcl_ChannelType fileChannelType = {
    "file",			/* Type name. */
    TCL_CHANNEL_VERSION_3,	/* v3 channel */
    FileCloseProc,		/* Close proc. */
    FileInputProc,		/* Input proc. */
    FileOutputProc,		/* Output proc. */
    FileSeekProc,		/* Seek proc. */
    NULL,			/* Set option proc. */
    NULL,			/* Get option proc. */
    FileWatchProc,		/* Set up the notifier to watch the channel. */
    FileGetHandleProc,		/* Get an OS handle from channel. */
    NULL,			/* close2proc. */
    FileBlockProc,		/* Set blocking or non-blocking mode.*/
    NULL,			/* flush proc. */
    NULL,			/* handler proc. */
    FileWideSeekProc,		/* Wide seek proc. */
};

#if defined(HAVE_NO_SEH) && defined(TCL_MEM_DEBUG)
static void *INITIAL_ESP,
            *INITIAL_EBP,
            *INITIAL_HANDLER,
            *RESTORED_ESP,
            *RESTORED_EBP,
            *RESTORED_HANDLER;
#endif /* HAVE_NO_SEH && TCL_MEM_DEBUG */


/*
 *----------------------------------------------------------------------
 *
 * FileInit --
 *
 *	This function creates the window used to simulate file events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates a new window and creates an exit handler. 
 *
 *----------------------------------------------------------------------
 */

static ThreadSpecificData *
FileInit()
{
    ThreadSpecificData *tsdPtr =
	(ThreadSpecificData *)TclThreadDataKeyGet(&dataKey);
    if (tsdPtr == NULL) {
	tsdPtr = TCL_TSD_INIT(&dataKey);
	tsdPtr->firstFilePtr = NULL;
	Tcl_CreateEventSource(FileSetupProc, FileCheckProc, NULL);
	Tcl_CreateThreadExitHandler(FileChannelExitHandler, NULL);
    }
    return tsdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FileChannelExitHandler --
 *
 *	This function is called to cleanup the channel driver before
 *	Tcl is unloaded.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destroys the communication window.
 *
 *----------------------------------------------------------------------
 */

static void
FileChannelExitHandler(clientData)
    ClientData clientData;	/* Old window proc */
{
    Tcl_DeleteEventSource(FileSetupProc, FileCheckProc, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * FileSetupProc --
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
FileSetupProc(data, flags)
    ClientData data;		/* Not used. */
    int flags;			/* Event flags as passed to Tcl_DoOneEvent. */
{
    FileInfo *infoPtr;
    Tcl_Time blockTime = { 0, 0 };
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return;
    }
    
    /*
     * Check to see if there is a ready file.  If so, poll.
     */

    for (infoPtr = tsdPtr->firstFilePtr; infoPtr != NULL; 
	    infoPtr = infoPtr->nextPtr) {
	if (infoPtr->watchMask) {
	    Tcl_SetMaxBlockTime(&blockTime);
	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FileCheckProc --
 *
 *	This procedure is called by Tcl_DoOneEvent to check the file
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
FileCheckProc(data, flags)
    ClientData data;		/* Not used. */
    int flags;			/* Event flags as passed to Tcl_DoOneEvent. */
{
    FileEvent *evPtr;
    FileInfo *infoPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return;
    }
    
    /*
     * Queue events for any ready files that don't already have events
     * queued (caused by persistent states that won't generate WinSock
     * events).
     */

    for (infoPtr = tsdPtr->firstFilePtr; infoPtr != NULL; 
	    infoPtr = infoPtr->nextPtr) {
	if (infoPtr->watchMask && !(infoPtr->flags & FILE_PENDING)) {
	    infoPtr->flags |= FILE_PENDING;
	    evPtr = (FileEvent *) ckalloc(sizeof(FileEvent));
	    evPtr->header.proc = FileEventProc;
	    evPtr->infoPtr = infoPtr;
	    Tcl_QueueEvent((Tcl_Event *) evPtr, TCL_QUEUE_TAIL);
	}
    }
}

/*----------------------------------------------------------------------
 *
 * FileEventProc --
 *
 *	This function is invoked by Tcl_ServiceEvent when a file event
 *	reaches the front of the event queue.  This procedure invokes
 *	Tcl_NotifyChannel on the file.
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
FileEventProc(evPtr, flags)
    Tcl_Event *evPtr;		/* Event to service. */
    int flags;			/* Flags that indicate what events to
				 * handle, such as TCL_FILE_EVENTS. */
{
    FileEvent *fileEvPtr = (FileEvent *)evPtr;
    FileInfo *infoPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return 0;
    }

    /*
     * Search through the list of watched files for the one whose handle
     * matches the event.  We do this rather than simply dereferencing
     * the handle in the event so that files can be deleted while the
     * event is in the queue.
     */

    for (infoPtr = tsdPtr->firstFilePtr; infoPtr != NULL;
	    infoPtr = infoPtr->nextPtr) {
	if (fileEvPtr->infoPtr == infoPtr) {
	    infoPtr->flags &= ~(FILE_PENDING);
	    Tcl_NotifyChannel(infoPtr->channel, infoPtr->watchMask);
	    break;
	}
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * FileBlockProc --
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
FileBlockProc(instanceData, mode)
    ClientData instanceData;	/* Instance data for channel. */
    int mode;			/* TCL_MODE_BLOCKING or
                                 * TCL_MODE_NONBLOCKING. */
{
    FileInfo *infoPtr = (FileInfo *) instanceData;
    
    /*
     * Files on Windows can not be switched between blocking and nonblocking,
     * hence we have to emulate the behavior. This is done in the input
     * function by checking against a bit in the state. We set or unset the
     * bit here to cause the input function to emulate the correct behavior.
     */

    if (mode == TCL_MODE_NONBLOCKING) {
	infoPtr->flags |= FILE_ASYNC;
    } else {
	infoPtr->flags &= ~(FILE_ASYNC);
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * FileCloseProc --
 *
 *	Closes the IO channel.
 *
 * Results:
 *	0 if successful, the value of errno if failed.
 *
 * Side effects:
 *	Closes the physical channel
 *
 *----------------------------------------------------------------------
 */

static int
FileCloseProc(instanceData, interp)
    ClientData instanceData;	/* Pointer to FileInfo structure. */
    Tcl_Interp *interp;		/* Not used. */
{
    FileInfo *fileInfoPtr = (FileInfo *) instanceData;
    int errorCode = 0;

    /*
     * Remove the file from the watch list.
     */

    FileWatchProc(instanceData, 0);

    /*
     * Don't close the Win32 handle if the handle is a standard channel
     * during the thread exit process.  Otherwise, one thread may kill
     * the stdio of another.
     */

    if (!TclInThreadExit() 
	    || ((GetStdHandle(STD_INPUT_HANDLE) != fileInfoPtr->handle)
		&& (GetStdHandle(STD_OUTPUT_HANDLE) != fileInfoPtr->handle)
		&& (GetStdHandle(STD_ERROR_HANDLE) != fileInfoPtr->handle))) {
	if (CloseHandle(fileInfoPtr->handle) == FALSE) {
	    TclWinConvertError(GetLastError());
	    errorCode = errno;
	}
    }

    ckfree((char *)fileInfoPtr);
    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * FileSeekProc --
 *
 *	Seeks on a file-based channel. Returns the new position.
 *
 * Results:
 *	-1 if failed, the new position if successful. If failed, it
 *	also sets *errorCodePtr to the error code.
 *
 * Side effects:
 *	Moves the location at which the channel will be accessed in
 *	future operations.
 *
 *----------------------------------------------------------------------
 */

static int
FileSeekProc(instanceData, offset, mode, errorCodePtr)
    ClientData instanceData;	/* File state. */
    long offset;		/* Offset to seek to. */
    int mode;			/* Relative to where should we seek? */
    int *errorCodePtr;		/* To store error code. */
{
    FileInfo *infoPtr = (FileInfo *) instanceData;
    DWORD moveMethod;
    DWORD newPos, newPosHigh;
    DWORD oldPos, oldPosHigh;

    *errorCodePtr = 0;
    if (mode == SEEK_SET) {
        moveMethod = FILE_BEGIN;
    } else if (mode == SEEK_CUR) {
        moveMethod = FILE_CURRENT;
    } else {
        moveMethod = FILE_END;
    }

    /*
     * Save our current place in case we need to roll-back the seek.
     */
    oldPosHigh = (DWORD)0;
    oldPos = SetFilePointer(infoPtr->handle, (LONG)0, &oldPosHigh,
	    FILE_CURRENT);
    if (oldPos == INVALID_SET_FILE_POINTER) {
	DWORD winError = GetLastError();
	if (winError != NO_ERROR) {
	    TclWinConvertError(winError);
	    *errorCodePtr = errno;
	    return -1;
	}
    }

    newPosHigh = (DWORD)(offset < 0 ? -1 : 0);
    newPos = SetFilePointer(infoPtr->handle, (LONG) offset, &newPosHigh,
			    moveMethod);
    if (newPos == INVALID_SET_FILE_POINTER) {
	DWORD winError = GetLastError();
	if (winError != NO_ERROR) {
	    TclWinConvertError(winError);
	    *errorCodePtr = errno;
	    return -1;
	}
    }

    /*
     * Check for expressability in our return type, and roll-back otherwise.
     */
    if (newPosHigh != 0) {
	*errorCodePtr = EOVERFLOW;
	SetFilePointer(infoPtr->handle, (LONG)oldPos, &oldPosHigh, FILE_BEGIN);
	return -1;
    }
    return (int) newPos;
}

/*
 *----------------------------------------------------------------------
 *
 * FileWideSeekProc --
 *
 *	Seeks on a file-based channel. Returns the new position.
 *
 * Results:
 *	-1 if failed, the new position if successful. If failed, it
 *	also sets *errorCodePtr to the error code.
 *
 * Side effects:
 *	Moves the location at which the channel will be accessed in
 *	future operations.
 *
 *----------------------------------------------------------------------
 */

static Tcl_WideInt
FileWideSeekProc(instanceData, offset, mode, errorCodePtr)
    ClientData instanceData;	/* File state. */
    Tcl_WideInt offset;		/* Offset to seek to. */
    int mode;			/* Relative to where should we seek? */
    int *errorCodePtr;		/* To store error code. */
{
    FileInfo *infoPtr = (FileInfo *) instanceData;
    DWORD moveMethod;
    DWORD newPos, newPosHigh;

    *errorCodePtr = 0;
    if (mode == SEEK_SET) {
        moveMethod = FILE_BEGIN;
    } else if (mode == SEEK_CUR) {
        moveMethod = FILE_CURRENT;
    } else {
        moveMethod = FILE_END;
    }

    newPosHigh = (DWORD)(offset >> 32);
    newPos = SetFilePointer(infoPtr->handle, (LONG) offset, &newPosHigh,
			    moveMethod);
    if (newPos == INVALID_SET_FILE_POINTER) {
	DWORD winError = GetLastError();
	if (winError != NO_ERROR) {
	    TclWinConvertError(winError);
	    *errorCodePtr = errno;
	    return -1;
	}
    }
    return ((Tcl_WideInt) newPos) | (((Tcl_WideInt) newPosHigh) << 32);
}

/*
 *----------------------------------------------------------------------
 *
 * FileInputProc --
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
FileInputProc(instanceData, buf, bufSize, errorCode)
    ClientData instanceData;		/* File state. */
    char *buf;				/* Where to store data read. */
    int bufSize;			/* How much space is available
                                         * in the buffer? */
    int *errorCode;			/* Where to store error code. */
{
    FileInfo *infoPtr;
    DWORD bytesRead;

    *errorCode = 0;
    infoPtr = (FileInfo *) instanceData;

    /*
     * Note that we will block on reads from a console buffer until a
     * full line has been entered.  The only way I know of to get
     * around this is to write a console driver.  We should probably
     * do this at some point, but for now, we just block.  The same
     * problem exists for files being read over the network.
     */

    if (ReadFile(infoPtr->handle, (LPVOID) buf, (DWORD) bufSize, &bytesRead,
            (LPOVERLAPPED) NULL) != FALSE) {
	return bytesRead;
    }
    
    TclWinConvertError(GetLastError());
    *errorCode = errno;
    if (errno == EPIPE) {
	return 0;
    }
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * FileOutputProc --
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
FileOutputProc(instanceData, buf, toWrite, errorCode)
    ClientData instanceData;		/* File state. */
    CONST char *buf;			/* The data buffer. */
    int toWrite;			/* How many bytes to write? */
    int *errorCode;			/* Where to store error code. */
{
    FileInfo *infoPtr = (FileInfo *) instanceData;
    DWORD bytesWritten;
    
    *errorCode = 0;

    /*
     * If we are writing to a file that was opened with O_APPEND, we need to
     * seek to the end of the file before writing the current buffer.
     */

    if (infoPtr->flags & FILE_APPEND) {
        SetFilePointer(infoPtr->handle, 0, NULL, FILE_END);
    }

    if (WriteFile(infoPtr->handle, (LPVOID) buf, (DWORD) toWrite, &bytesWritten,
            (LPOVERLAPPED) NULL) == FALSE) {
        TclWinConvertError(GetLastError());
        *errorCode = errno;
        return -1;
    }
    infoPtr->dirty = 1;
    return bytesWritten;
}

/*
 *----------------------------------------------------------------------
 *
 * FileWatchProc --
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
FileWatchProc(instanceData, mask)
    ClientData instanceData;		/* File state. */
    int mask;				/* What events to watch for; OR-ed
                                         * combination of TCL_READABLE,
                                         * TCL_WRITABLE and TCL_EXCEPTION. */
{
    FileInfo *infoPtr = (FileInfo *) instanceData;
    Tcl_Time blockTime = { 0, 0 };

    /*
     * Since the file is always ready for events, we set the block time
     * to zero so we will poll.
     */

    infoPtr->watchMask = mask & infoPtr->validMask;
    if (infoPtr->watchMask) {
	Tcl_SetMaxBlockTime(&blockTime);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FileGetHandleProc --
 *
 *	Called from Tcl_GetChannelHandle to retrieve OS handles from
 *	a file based channel.
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
FileGetHandleProc(instanceData, direction, handlePtr)
    ClientData instanceData;	/* The file state. */
    int direction;		/* TCL_READABLE or TCL_WRITABLE */
    ClientData *handlePtr;	/* Where to store the handle.  */
{
    FileInfo *infoPtr = (FileInfo *) instanceData;

    if (direction & infoPtr->validMask) {
	*handlePtr = (ClientData) infoPtr->handle;
	return TCL_OK;
    } else {
	return TCL_ERROR;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * TclpOpenFileChannel --
 *
 *	Open an File based channel on Unix systems.
 *
 * Results:
 *	The new channel or NULL. If NULL, the output argument
 *	errorCodePtr is set to a POSIX error.
 *
 * Side effects:
 *	May open the channel and may cause creation of a file on the
 *	file system.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
TclpOpenFileChannel(interp, pathPtr, mode, permissions)
    Tcl_Interp *interp;			/* Interpreter for error reporting;
                                         * can be NULL. */
    Tcl_Obj *pathPtr;			/* Name of file to open. */
    int mode;				/* POSIX mode. */
    int permissions;			/* If the open involves creating a
                                         * file, with what modes to create
                                         * it? */
{
    Tcl_Channel channel = 0;
    int channelPermissions;
    DWORD accessMode, createMode, shareMode, flags, consoleParams, type;
    CONST TCHAR *nativeName;
    DCB dcb;
    HANDLE handle;
    char channelName[16 + TCL_INTEGER_SPACE];
    TclFile readFile = NULL;
    TclFile writeFile = NULL;

    nativeName = (TCHAR*) Tcl_FSGetNativePath(pathPtr);
    if (nativeName == NULL) {
	return NULL;
    }
    
    switch (mode & (O_RDONLY | O_WRONLY | O_RDWR)) {
	case O_RDONLY:
	    accessMode = GENERIC_READ;
	    channelPermissions = TCL_READABLE;
	    break;
	case O_WRONLY:
	    accessMode = GENERIC_WRITE;
	    channelPermissions = TCL_WRITABLE;
	    break;
	case O_RDWR:
	    accessMode = (GENERIC_READ | GENERIC_WRITE);
	    channelPermissions = (TCL_READABLE | TCL_WRITABLE);
	    break;
	default:
	    panic("TclpOpenFileChannel: invalid mode value");
	    break;
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

    /*
     * If the file is being created, get the file attributes from the
     * permissions argument, else use the existing file attributes.
     */

    if (mode & O_CREAT) {
        if (permissions & S_IWRITE) {
            flags = FILE_ATTRIBUTE_NORMAL;
        } else {
            flags = FILE_ATTRIBUTE_READONLY;
        }
    } else {
	flags = (*tclWinProcs->getFileAttributesProc)(nativeName);
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

    handle = (*tclWinProcs->createFileProc)(nativeName, accessMode, 
	    shareMode, NULL, createMode, flags, (HANDLE) NULL);

    if (handle == INVALID_HANDLE_VALUE) {
	DWORD err;
	err = GetLastError();
	if ((err & 0xffffL) == ERROR_OPEN_FAILED) {
	    err = (mode & O_CREAT) ? ERROR_FILE_EXISTS : ERROR_FILE_NOT_FOUND;
	}
        TclWinConvertError(err);
	if (interp != (Tcl_Interp *) NULL) {
            Tcl_AppendResult(interp, "couldn't open \"", 
			     Tcl_GetString(pathPtr), "\": ",
			     Tcl_PosixError(interp), (char *) NULL);
        }
        return NULL;
    }
    
    type = GetFileType(handle);

    /*
     * If the file is a character device, we need to try to figure out
     * whether it is a serial port, a console, or something else.  We
     * test for the console case first because this is more common.
     */

    if (type == FILE_TYPE_CHAR) {
	if (GetConsoleMode(handle, &consoleParams)) {
	    type = FILE_TYPE_CONSOLE;
	} else {
	    dcb.DCBlength = sizeof( DCB ) ;
	    if (GetCommState(handle, &dcb)) {
		type = FILE_TYPE_SERIAL;
	    }
		    
	}
    }

    channel = NULL;

    switch (type) {
    case FILE_TYPE_SERIAL:
	/*
	 * Reopen channel for OVERLAPPED operation
	 * Normally this shouldn't fail, because the channel exists
	 */
	handle = TclWinSerialReopen(handle, nativeName, accessMode);
	if (handle == INVALID_HANDLE_VALUE) {
	    TclWinConvertError(GetLastError());
	    if (interp != (Tcl_Interp *) NULL) {
		Tcl_AppendResult(interp, "couldn't reopen serial \"",
			Tcl_GetString(pathPtr), "\": ",
			Tcl_PosixError(interp), (char *) NULL);
	    }
	    return NULL;
	}
	channel = TclWinOpenSerialChannel(handle, channelName,
	        channelPermissions);
	break;
    case FILE_TYPE_CONSOLE:
	channel = TclWinOpenConsoleChannel(handle, channelName,
	        channelPermissions);
	break;
    case FILE_TYPE_PIPE:
	if (channelPermissions & TCL_READABLE) {
	    readFile = TclWinMakeFile(handle);
	}
	if (channelPermissions & TCL_WRITABLE) {
	    writeFile = TclWinMakeFile(handle);
	}
	channel = TclpCreateCommandChannel(readFile, writeFile, NULL, 0, NULL);
	break;
    case FILE_TYPE_CHAR:
    case FILE_TYPE_DISK:
    case FILE_TYPE_UNKNOWN:
	channel = TclWinOpenFileChannel(handle, channelName,
					channelPermissions,
					(mode & O_APPEND) ? FILE_APPEND : 0);
	break;

    default:
	/*
	 * The handle is of an unknown type, probably /dev/nul equivalent
	 * or possibly a closed handle.  
	 */
	
	channel = NULL;
	Tcl_AppendResult(interp, "couldn't open \"", 
			 Tcl_GetString(pathPtr), "\": ",
			 "bad file type", (char *) NULL);
	break;
    }

    return channel;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MakeFileChannel --
 *
 *	Creates a Tcl_Channel from an existing platform specific file
 *	handle.
 *
 * Results:
 *	The Tcl_Channel created around the preexisting file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_MakeFileChannel(rawHandle, mode)
    ClientData rawHandle;	/* OS level handle */
    int mode;			/* ORed combination of TCL_READABLE and
                                 * TCL_WRITABLE to indicate file mode. */
{
    char channelName[16 + TCL_INTEGER_SPACE];
    Tcl_Channel channel = NULL;
    HANDLE handle = (HANDLE) rawHandle;
    HANDLE dupedHandle;
    DCB dcb;
    DWORD consoleParams, type;
    TclFile readFile = NULL;
    TclFile writeFile = NULL;
    BOOL result;

    if (mode == 0) {
	return NULL;
    }

    /*
     * GetFileType() returns FILE_TYPE_UNKNOWN for invalid handles.
     */

    type = GetFileType(handle);

    /*
     * If the file is a character device, we need to try to figure out
     * whether it is a serial port, a console, or something else.  We
     * test for the console case first because this is more common.
     */

    if (type == FILE_TYPE_CHAR) {
	if (GetConsoleMode(handle, &consoleParams)) {
	    type = FILE_TYPE_CONSOLE;
	} else {
	    dcb.DCBlength = sizeof( DCB ) ;
	    if (GetCommState(handle, &dcb)) {
		type = FILE_TYPE_SERIAL;
	    }
	}
    }

    switch (type)
    {
    case FILE_TYPE_SERIAL:
	channel = TclWinOpenSerialChannel(handle, channelName, mode);
	break;
    case FILE_TYPE_CONSOLE:
	channel = TclWinOpenConsoleChannel(handle, channelName, mode);
	break;
    case FILE_TYPE_PIPE:
	if (mode & TCL_READABLE)
	{
	    readFile = TclWinMakeFile(handle);
	}
	if (mode & TCL_WRITABLE)
	{
	    writeFile = TclWinMakeFile(handle);
	}
	channel = TclpCreateCommandChannel(readFile, writeFile, NULL, 0, NULL);
	break;

    case FILE_TYPE_DISK:
    case FILE_TYPE_CHAR:
	channel = TclWinOpenFileChannel(handle, channelName, mode, 0);
	break;
	
    case FILE_TYPE_UNKNOWN:
    default:
	/*
	 * The handle is of an unknown type.  Test the validity of this OS
	 * handle by duplicating it, then closing the dupe.  The Win32 API
	 * doesn't provide an IsValidHandle() function, so we have to emulate
	 * it here.  This test will not work on a console handle reliably,
	 * which is why we can't test every handle that comes into this
	 * function in this way.
	 */

	result = DuplicateHandle(GetCurrentProcess(), handle,
		GetCurrentProcess(), &dupedHandle, 0, FALSE,
		DUPLICATE_SAME_ACCESS);

	if (result == 0) {
	    /* 
	     * Unable to make a duplicate. It's definately invalid at this
	     * point.
	     */

	    return NULL;
	}

	/*
	 * Use structured exception handling (Win32 SEH) to protect the close
	 * of this duped handle which might throw EXCEPTION_INVALID_HANDLE.
	 */

#ifdef HAVE_NO_SEH
# ifdef TCL_MEM_DEBUG
        __asm__ __volatile__ (
            "movl %%esp,  %0" "\n\t"
            "movl %%ebp,  %1" "\n\t"
            "movl %%fs:0, %2" "\n\t"
            : "=m"(INITIAL_ESP),
              "=m"(INITIAL_EBP),
              "=r"(INITIAL_HANDLER) );
# endif /* TCL_MEM_DEBUG */

        result = 0;

        __asm__ __volatile__ (
            "pushl %ebp" "\n\t"
            "pushl $__except_makefilechannel_handler" "\n\t"
            "pushl %fs:0" "\n\t"
            "movl  %esp, %fs:0");
#else
	__try {
#endif /* HAVE_NO_SEH */
	    CloseHandle(dupedHandle);
#ifdef HAVE_NO_SEH
        __asm__ __volatile__ (
            "jmp  makefilechannel_pop" "\n"
        "makefilechannel_reentry:" "\n\t"
            "movl %%fs:0, %%eax" "\n\t"
            "movl 0x8(%%eax), %%esp" "\n\t"
            "movl 0x8(%%esp), %%ebp" "\n"
            "movl $1, %0" "\n"
        "makefilechannel_pop:" "\n\t"
            "movl (%%esp), %%eax" "\n\t"
            "movl %%eax, %%fs:0" "\n\t"
            "add  $12, %%esp" "\n\t"
            : "=m"(result)
            :
            : "%eax");

# ifdef TCL_MEM_DEBUG
    __asm__ __volatile__ (
            "movl  %%esp,  %0" "\n\t"
            "movl  %%ebp,  %1" "\n\t"
            "movl  %%fs:0, %2" "\n\t"
            : "=m"(RESTORED_ESP),
              "=m"(RESTORED_EBP),
              "=r"(RESTORED_HANDLER) );

    if (INITIAL_ESP != RESTORED_ESP)
        panic("ESP restored incorrectly");
    if (INITIAL_EBP != RESTORED_EBP)
        panic("EBP restored incorrectly");
    if (INITIAL_HANDLER != RESTORED_HANDLER)
        panic("HANDLER restored incorrectly");
# endif /* TCL_MEM_DEBUG */

        if (result)
            return NULL;
#else
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	    /*
	     * Definately an invalid handle.  So, therefore, the original
	     * is invalid also.
	     */

	    return NULL;
	}
#endif /* HAVE_NO_SEH */

	/* Fall through, the handle is valid. */

	/*
	 * Create the undefined channel, anyways, because we know the handle
	 * is valid to something.
	 */

	channel = TclWinOpenFileChannel(handle, channelName, mode, 0);
    }

    return channel;
}
#ifdef HAVE_NO_SEH
static
__attribute__ ((cdecl))
EXCEPTION_DISPOSITION
_except_makefilechannel_handler(
    struct _EXCEPTION_RECORD *ExceptionRecord,
    void *EstablisherFrame,
    struct _CONTEXT *ContextRecord,
    void *DispatcherContext)
{
    __asm__ __volatile__ (
            "jmp makefilechannel_reentry");
    /* Nuke compiler warning about unused static function */
    _except_makefilechannel_handler(NULL, NULL, NULL, NULL);
    return 0; /* Function does not return */
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * TclpGetDefaultStdChannel --
 *
 *	Constructs a channel for the specified standard OS handle.
 *
 * Results:
 *	Returns the specified default standard channel, or NULL.
 *
 * Side effects:
 *	May cause the creation of a standard channel and the underlying
 *	file.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
TclpGetDefaultStdChannel(type)
    int type;			/* One of TCL_STDIN, TCL_STDOUT, TCL_STDERR. */
{
    Tcl_Channel channel;
    HANDLE handle;
    int mode;
    char *bufMode;
    DWORD handleId;		/* Standard handle to retrieve. */


    switch (type) {
	case TCL_STDIN:
	    handleId = STD_INPUT_HANDLE;
	    mode = TCL_READABLE;
	    bufMode = "line";
	    break;
	case TCL_STDOUT:
	    handleId = STD_OUTPUT_HANDLE;
	    mode = TCL_WRITABLE;
	    bufMode = "line";
	    break;
	case TCL_STDERR:
	    handleId = STD_ERROR_HANDLE;
	    mode = TCL_WRITABLE;
	    bufMode = "none";
	    break;
	default:
	    panic("TclGetDefaultStdChannel: Unexpected channel type");
	    break;
    }

    handle = GetStdHandle(handleId);

    /*
     * Note that we need to check for 0 because Windows may return 0 if this
     * is not a console mode application, even though this is not a valid
     * handle.
     */

    if ((handle == INVALID_HANDLE_VALUE) || (handle == 0)) {
	return (Tcl_Channel) NULL;
    }

    channel = Tcl_MakeFileChannel(handle, mode);

    if (channel == NULL) {
	return (Tcl_Channel) NULL;
    }

    /*
     * Set up the normal channel options for stdio handles.
     */

    if ((Tcl_SetChannelOption((Tcl_Interp *) NULL, channel, "-translation",
            "auto") == TCL_ERROR)
	    || (Tcl_SetChannelOption((Tcl_Interp *) NULL, channel, "-eofchar",
		    "\032 {}") == TCL_ERROR)
	    || (Tcl_SetChannelOption((Tcl_Interp *) NULL, channel,
		    "-buffering", bufMode) == TCL_ERROR)) {
        Tcl_Close((Tcl_Interp *) NULL, channel);
        return (Tcl_Channel) NULL;
    }
    return channel;
}



/*
 *----------------------------------------------------------------------
 *
 * TclWinOpenFileChannel --
 *
 *	Constructs a File channel for the specified standard OS handle.
 *      This is a helper function to break up the construction of 
 *      channels into File, Console, or Serial.
 *
 * Results:
 *	Returns the new channel, or NULL.
 *
 * Side effects:
 *	May open the channel and may cause creation of a file on the
 *	file system.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
TclWinOpenFileChannel(handle, channelName, permissions, appendMode)
    HANDLE handle;
    char *channelName;
    int permissions;
    int appendMode;
{
    FileInfo *infoPtr;
    ThreadSpecificData *tsdPtr;

    tsdPtr = FileInit();

    /*
     * See if a channel with this handle already exists.
     */
    
    for (infoPtr = tsdPtr->firstFilePtr; infoPtr != NULL; 
	 infoPtr = infoPtr->nextPtr) {
	if (infoPtr->handle == (HANDLE) handle) {
	    return (permissions == infoPtr->validMask) ? infoPtr->channel : NULL;
	}
    }

    infoPtr = (FileInfo *) ckalloc((unsigned) sizeof(FileInfo));
    infoPtr->nextPtr = tsdPtr->firstFilePtr;
    tsdPtr->firstFilePtr = infoPtr;
    infoPtr->validMask = permissions;
    infoPtr->watchMask = 0;
    infoPtr->flags = appendMode;
    infoPtr->handle = handle;
    infoPtr->dirty = 0;
    wsprintfA(channelName, "file%lx", (int) infoPtr);
    
    infoPtr->channel = Tcl_CreateChannel(&fileChannelType, channelName,
	    (ClientData) infoPtr, permissions);
    
    /*
     * Files have default translation of AUTO and ^Z eof char, which
     * means that a ^Z will be accepted as EOF when reading.
     */
    
    Tcl_SetChannelOption(NULL, infoPtr->channel, "-translation", "auto");
    Tcl_SetChannelOption(NULL, infoPtr->channel, "-eofchar", "\032 {}");

    return infoPtr->channel;
}


/*
 *----------------------------------------------------------------------
 *
 * TclWinFlushDirtyChannels --
 *
 *	Flush all dirty channels to disk, so that requesting the
 *	size of any file returns the correct value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information is actually written to disk now, rather than
 *	later.  Don't call this too often, or there will be a 
 *	performance hit (i.e. only call when we need to ask for
 *	the size of a file).
 *
 *----------------------------------------------------------------------
 */

void
TclWinFlushDirtyChannels ()
{
    FileInfo *infoPtr;
    ThreadSpecificData *tsdPtr;

    tsdPtr = FileInit();

    /*
     * Flush all channels which are dirty, i.e. may have data pending
     * in the OS
     */
    
    for (infoPtr = tsdPtr->firstFilePtr;
	 infoPtr != NULL; 
	 infoPtr = infoPtr->nextPtr) {
	if (infoPtr->dirty) {
	    FlushFileBuffers(infoPtr->handle);
	    infoPtr->dirty = 0;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpCutFileChannel --
 *
 *	Remove any thread local refs to this channel. See
 *	Tcl_CutChannel for more info.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes thread local list of valid channels.
 *
 *----------------------------------------------------------------------
 */

void
TclpCutFileChannel(chan)
    Tcl_Channel chan;			/* The channel being removed. Must
                                         * not be referenced in any
                                         * interpreter. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    Channel *chanPtr = (Channel *) chan;
    FileInfo *infoPtr;
    FileInfo **nextPtrPtr;
    int removed = 0;

    if (chanPtr->typePtr != &fileChannelType)
        return;

    infoPtr = (FileInfo *) chanPtr->instanceData;

    for (nextPtrPtr = &(tsdPtr->firstFilePtr); (*nextPtrPtr) != NULL;
	 nextPtrPtr = &((*nextPtrPtr)->nextPtr)) {
	if ((*nextPtrPtr) == infoPtr) {
	    (*nextPtrPtr) = infoPtr->nextPtr;
	    removed = 1;
	    break;
	}
    }

    /*
     * This could happen if the channel was created in one thread
     * and then moved to another without updating the thread
     * local data in each thread.
     */

    if (!removed)
        panic("file info ptr not on thread channel list");

}

/*
 *----------------------------------------------------------------------
 *
 * TclpSpliceFileChannel --
 *
 *	Insert thread local ref for this channel.
 *	Tcl_SpliceChannel for more info.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes thread local list of valid channels.
 *
 *----------------------------------------------------------------------
 */

void
TclpSpliceFileChannel(chan)
    Tcl_Channel chan;			/* The channel being removed. Must
                                         * not be referenced in any
                                         * interpreter. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    Channel *chanPtr = (Channel *) chan;
    FileInfo *infoPtr;

    if (chanPtr->typePtr != &fileChannelType)
        return;

    infoPtr = (FileInfo *) chanPtr->instanceData;

    infoPtr->nextPtr = tsdPtr->firstFilePtr;
    tsdPtr->firstFilePtr = infoPtr;
}
