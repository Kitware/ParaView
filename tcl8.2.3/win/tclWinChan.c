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
			    char *buf, int toWrite, int *errorCode));
static int		FileSeekProc _ANSI_ARGS_((ClientData instanceData,
			    long offset, int mode, int *errorCode));
static void		FileSetupProc _ANSI_ARGS_((ClientData clientData,
			    int flags));
static void		FileWatchProc _ANSI_ARGS_((ClientData instanceData,
		            int mask));

			    
/*
 * This structure describes the channel type structure for file based IO.
 */

static Tcl_ChannelType fileChannelType = {
    "file",			/* Type name. */
    FileBlockProc,		/* Set blocking or non-blocking mode.*/
    FileCloseProc,		/* Close proc. */
    FileInputProc,		/* Input proc. */
    FileOutputProc,		/* Output proc. */
    FileSeekProc,		/* Seek proc. */
    NULL,			/* Set option proc. */
    NULL,			/* Get option proc. */
    FileWatchProc,		/* Set up the notifier to watch the channel. */
    FileGetHandleProc,		/* Get an OS handle from channel. */
};


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
    FileInfo **nextPtrPtr;
    int errorCode = 0;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * Remove the file from the watch list.
     */

    FileWatchProc(instanceData, 0);

    /*
     * Don't close the Win32 handle if the handle is a standard channel
     * during the exit process.  Otherwise, one thread may kill the stdio
     * of another.
     */

    if (!TclInExit() 
	    || ((GetStdHandle(STD_INPUT_HANDLE) != fileInfoPtr->handle)
		&& (GetStdHandle(STD_OUTPUT_HANDLE) != fileInfoPtr->handle)
		&& (GetStdHandle(STD_ERROR_HANDLE) != fileInfoPtr->handle))) {
	if (CloseHandle(fileInfoPtr->handle) == FALSE) {
	    TclWinConvertError(GetLastError());
	    errorCode = errno;
	}
    }
    for (nextPtrPtr = &(tsdPtr->firstFilePtr); (*nextPtrPtr) != NULL;
	 nextPtrPtr = &((*nextPtrPtr)->nextPtr)) {
	if ((*nextPtrPtr) == fileInfoPtr) {
	    (*nextPtrPtr) = fileInfoPtr->nextPtr;
	    break;
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
    ClientData instanceData;			/* File state. */
    long offset;				/* Offset to seek to. */
    int mode;					/* Relative to where
                                                 * should we seek? */
    int *errorCodePtr;				/* To store error code. */
{
    FileInfo *infoPtr = (FileInfo *) instanceData;
    DWORD moveMethod;
    DWORD newPos;

    *errorCodePtr = 0;
    if (mode == SEEK_SET) {
        moveMethod = FILE_BEGIN;
    } else if (mode == SEEK_CUR) {
        moveMethod = FILE_CURRENT;
    } else {
        moveMethod = FILE_END;
    }

    newPos = SetFilePointer(infoPtr->handle, offset, NULL, moveMethod);
    if (newPos == 0xFFFFFFFF) {
        TclWinConvertError(GetLastError());
        *errorCodePtr = errno;
	return -1;
    }
    return newPos;
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
    char *buf;				/* The data buffer. */
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
    FlushFileBuffers(infoPtr->handle);
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
TclpOpenFileChannel(interp, fileName, modeString, permissions)
    Tcl_Interp *interp;			/* Interpreter for error reporting;
                                         * can be NULL. */
    char *fileName;			/* Name of file to open. */
    char *modeString;			/* A list of POSIX open modes or
                                         * a string such as "rw". */
    int permissions;			/* If the open involves creating a
                                         * file, with what modes to create
                                         * it? */
{
    Tcl_Channel channel = 0;
    int seekFlag, mode, channelPermissions;
    DWORD accessMode, createMode, shareMode, flags, consoleParams, type;
    TCHAR *nativeName;
    Tcl_DString ds, buffer;
    DCB dcb;
    HANDLE handle;
    char channelName[16 + TCL_INTEGER_SPACE];
    TclFile readFile = NULL;
    TclFile writeFile = NULL;

    mode = TclGetOpenMode(interp, modeString, &seekFlag);
    if (mode == -1) {
        return NULL;
    }

    if (Tcl_TranslateFileName(interp, fileName, &ds) == NULL) {
	return NULL;
    }
    nativeName = Tcl_WinUtfToTChar(Tcl_DStringValue(&ds), 
	    Tcl_DStringLength(&ds), &buffer);

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
            Tcl_AppendResult(interp, "couldn't open \"", fileName, "\": ",
			     Tcl_PosixError(interp), (char *) NULL);
        }
        Tcl_DStringFree(&buffer);
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

    switch (type)
    {
    case FILE_TYPE_SERIAL:
	channel = TclWinOpenSerialChannel(handle, channelName,
	        channelPermissions);
	break;
    case FILE_TYPE_CONSOLE:
	channel = TclWinOpenConsoleChannel(handle, channelName,
	        channelPermissions);
	break;
    case FILE_TYPE_PIPE:
	if (channelPermissions & TCL_READABLE)
	{
	    readFile = TclWinMakeFile(handle);
	}
	if (channelPermissions & TCL_WRITABLE)
	{
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
	Tcl_AppendResult(interp, "couldn't open \"", fileName, "\": ",
		"bad file type", (char *) NULL);
	break;
    }

    Tcl_DStringFree(&buffer);
    Tcl_DStringFree(&ds);

    if (channel != NULL)
    {
	if (seekFlag) {
	    if (Tcl_Seek(channel, 0, SEEK_END) < 0) {
		if (interp != (Tcl_Interp *) NULL) {
		    Tcl_AppendResult(interp,
			    "could not seek to end of file on \"",
			    channelName, "\": ", Tcl_PosixError(interp),
			    (char *) NULL);
		}
		Tcl_Close(NULL, channel);
		return NULL;
	    }
	}
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
    DCB dcb;
    DWORD consoleParams;
    DWORD type;
    TclFile readFile = NULL;
    TclFile writeFile = NULL;

    if (mode == 0) {
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
    case FILE_TYPE_UNKNOWN:
	channel = TclWinOpenFileChannel(handle, channelName, mode, 0);
	break;
	
    default:
	/*
	 * The handle is of an unknown type, probably /dev/nul equivalent
	 * or possibly a closed handle.
	 */
	
	channel = NULL;
	break;

    }

    return channel;
}

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
	return NULL;
    }
    
    channel = Tcl_MakeFileChannel(handle, mode);

    if (channel == NULL) {
	return NULL;
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

