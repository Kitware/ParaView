/* 
 * tclMacChan.c
 *
 *	Channel drivers for Macintosh channels for the
 *	console fds.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"
#include "tclMacInt.h"
#include <Aliases.h>
#include <Errors.h>
#include <Files.h>
#include <Gestalt.h>
#include <Processes.h>
#include <Strings.h>
#include <FSpCompat.h>
#include <MoreFiles.h>
#include <MoreFilesExtras.h>


/*
 * The following are flags returned by GetOpenMode.  They
 * are or'd together to determine how opening and handling
 * a file should occur.
 */

#define TCL_RDONLY		(1<<0)
#define TCL_WRONLY		(1<<1)
#define TCL_RDWR		(1<<2)
#define TCL_CREAT		(1<<3)
#define TCL_TRUNC		(1<<4)
#define TCL_APPEND		(1<<5)
#define TCL_ALWAYS_APPEND	(1<<6)
#define TCL_EXCL		(1<<7)
#define TCL_NOCTTY		(1<<8)
#define TCL_NONBLOCK		(1<<9)
#define TCL_RW_MODES 		(TCL_RDONLY|TCL_WRONLY|TCL_RDWR)

/*
 * This structure describes per-instance state of a 
 * macintosh file based channel.
 */

typedef struct FileState {
    short fileRef;		/* Macintosh file reference number. */
    Tcl_Channel fileChan;	/* Pointer to the channel for this file. */
    int watchMask;		/* OR'ed set of flags indicating which events
    				 * are being watched. */
    int appendMode;		/* Flag to tell if in O_APPEND mode or not. */
    int volumeRef;		/* Flag to tell if in O_APPEND mode or not. */
    int pending;		/* 1 if message is pending on queue. */
    struct FileState *nextPtr;	/* Pointer to next registered file. */
} FileState;

typedef struct ThreadSpecificData {
    int initialized;		/* True after the thread initializes */
    FileState *firstFilePtr;	/* the head of the list of files managed
				 * that are being watched for file events. */
    Tcl_Channel stdinChannel;
    Tcl_Channel stdoutChannel;	/* Note - these seem unused */
    Tcl_Channel stderrChannel;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * The following structure is what is added to the Tcl event queue when
 * file events are generated.
 */

typedef struct FileEvent {
    Tcl_Event header;		/* Information that is standard for
				 * all events. */
    FileState *infoPtr;		/* Pointer to file info structure.  Note
				 * that we still have to verify that the
				 * file exists before dereferencing this
				 * pointer. */
} FileEvent;


/*
 * Static routines for this file:
 */

static int		CommonGetHandle _ANSI_ARGS_((ClientData instanceData,
		            int direction, ClientData *handlePtr));
static void		CommonWatch _ANSI_ARGS_((ClientData instanceData,
		            int mask));
static int		FileBlockMode _ANSI_ARGS_((ClientData instanceData,
			    int mode));
static void		FileChannelExitHandler _ANSI_ARGS_((
		            ClientData clientData));
static void		FileCheckProc _ANSI_ARGS_((ClientData clientData,
			    int flags));
static int		FileClose _ANSI_ARGS_((ClientData instanceData,
			    Tcl_Interp *interp));
static int		FileEventProc _ANSI_ARGS_((Tcl_Event *evPtr,
			    int flags));
static ThreadSpecificData *FileInit _ANSI_ARGS_((void));
static int		FileInput _ANSI_ARGS_((ClientData instanceData,
			    char *buf, int toRead, int *errorCode));
static int		FileOutput _ANSI_ARGS_((ClientData instanceData,
			    char *buf, int toWrite, int *errorCode));
static int		FileSeek _ANSI_ARGS_((ClientData instanceData,
			    long offset, int mode, int *errorCode));
static void		FileSetupProc _ANSI_ARGS_((ClientData clientData,
			    int flags));
static int		GetOpenMode _ANSI_ARGS_((Tcl_Interp *interp,
        		    CONST char *string));
static Tcl_Channel	OpenFileChannel _ANSI_ARGS_((CONST char *fileName, 
			    int mode, int permissions, int *errorCodePtr));
static int		StdIOBlockMode _ANSI_ARGS_((ClientData instanceData,
			    int mode));
static int		StdIOClose _ANSI_ARGS_((ClientData instanceData,
			    Tcl_Interp *interp));
static int		StdIOInput _ANSI_ARGS_((ClientData instanceData,
			    char *buf, int toRead, int *errorCode));
static int		StdIOOutput _ANSI_ARGS_((ClientData instanceData,
			    char *buf, int toWrite, int *errorCode));
static int		StdIOSeek _ANSI_ARGS_((ClientData instanceData,
			    long offset, int mode, int *errorCode));
static int		StdReady _ANSI_ARGS_((ClientData instanceData,
		            int mask));

/*
 * This structure describes the channel type structure for file based IO:
 */

static Tcl_ChannelType consoleChannelType = {
    "file",			/* Type name. */
    StdIOBlockMode,		/* Set blocking/nonblocking mode.*/
    StdIOClose,			/* Close proc. */
    StdIOInput,			/* Input proc. */
    StdIOOutput,		/* Output proc. */
    StdIOSeek,			/* Seek proc. */
    NULL,			/* Set option proc. */
    NULL,			/* Get option proc. */
    CommonWatch,		/* Initialize notifier. */
    CommonGetHandle		/* Get OS handles out of channel. */
};

/*
 * This variable describes the channel type structure for file based IO.
 */

static Tcl_ChannelType fileChannelType = {
    "file",			/* Type name. */
    FileBlockMode,		/* Set blocking or
                                 * non-blocking mode.*/
    FileClose,			/* Close proc. */
    FileInput,			/* Input proc. */
    FileOutput,			/* Output proc. */
    FileSeek,			/* Seek proc. */
    NULL,			/* Set option proc. */
    NULL,			/* Get option proc. */
    CommonWatch,		/* Initialize notifier. */
    CommonGetHandle		/* Get OS handles out of channel. */
};


/*
 * Hack to allow Mac Tk to override the TclGetStdChannels function.
 */
 
typedef void (*TclGetStdChannelsProc) _ANSI_ARGS_((Tcl_Channel *stdinPtr,
	Tcl_Channel *stdoutPtr, Tcl_Channel *stderrPtr));
	
TclGetStdChannelsProc getStdChannelsProc = NULL;


/*
 *----------------------------------------------------------------------
 *
 * FileInit --
 *
 *	This function initializes the file channel event source.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates a new event source.
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
FileChannelExitHandler(
    ClientData clientData)	/* Old window proc */
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
FileSetupProc(
    ClientData data,		/* Not used. */
    int flags)			/* Event flags as passed to Tcl_DoOneEvent. */
{
    FileState *infoPtr;
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
FileCheckProc(
    ClientData data,		/* Not used. */
    int flags)			/* Event flags as passed to Tcl_DoOneEvent. */
{
    FileEvent *evPtr;
    FileState *infoPtr;
    int sentMsg = 0;
    Tcl_Time blockTime = { 0, 0 };
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
	if (infoPtr->watchMask && !infoPtr->pending) {
	    infoPtr->pending = 1;
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
FileEventProc(
    Tcl_Event *evPtr,		/* Event to service. */
    int flags)			/* Flags that indicate what events to
				 * handle, such as TCL_FILE_EVENTS. */
{
    FileEvent *fileEvPtr = (FileEvent *)evPtr;
    FileState *infoPtr;
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
	    infoPtr->pending = 0;
	    Tcl_NotifyChannel(infoPtr->fileChan, infoPtr->watchMask);
	    break;
	}
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * StdIOBlockMode --
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
StdIOBlockMode(
    ClientData instanceData,		/* Unused. */
    int mode)				/* The mode to set. */
{
    /*
     * Do not allow putting stdin, stdout or stderr into nonblocking mode.
     */
    
    if (mode == TCL_MODE_NONBLOCKING) {
	return EFAULT;
    }
    
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * StdIOClose --
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
StdIOClose(
    ClientData instanceData,	/* Unused. */
    Tcl_Interp *interp)		/* Unused. */
{
    int fd, errorCode = 0;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * Invalidate the stdio cache if necessary.  Note that we assume that
     * the stdio file and channel pointers will become invalid at the same
     * time.
     * Do not close standard channels while in thread-exit.
     */

    fd = (int) ((FileState*)instanceData)->fileRef;
    if (!TclInExit()) {
	if (fd == 0) {
	    tsdPtr->stdinChannel = NULL;
	} else if (fd == 1) {
	    tsdPtr->stdoutChannel = NULL;
	} else if (fd == 2) {
	    tsdPtr->stderrChannel = NULL;
	} else {
	    panic("recieved invalid std file");
	}
    
	if (close(fd) < 0) {
	    errorCode = errno;
	}
    }
    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * CommonGetHandle --
 *
 *	Called from Tcl_GetChannelHandle to retrieve OS handles from inside
 *	a file based channel.
 *
 * Results:
 *	The appropriate handle or NULL if not present. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CommonGetHandle(
    ClientData instanceData,		/* The file state. */
    int direction,			/* Which handle to retrieve? */
    ClientData *handlePtr)
{
    if ((direction == TCL_READABLE) || (direction == TCL_WRITABLE)) {
	*handlePtr = (ClientData) ((FileState*)instanceData)->fileRef;
	return TCL_OK;
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * StdIOInput --
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

int
StdIOInput(
    ClientData instanceData,		/* Unused. */
    char *buf,				/* Where to store data read. */
    int bufSize,			/* How much space is available
                                         * in the buffer? */
    int *errorCode)			/* Where to store error code. */
{
    int fd;
    int bytesRead;			/* How many bytes were read? */

    *errorCode = 0;
    errno = 0;
    fd = (int) ((FileState*)instanceData)->fileRef;
    bytesRead = read(fd, buf, (size_t) bufSize);
    if (bytesRead > -1) {
        return bytesRead;
    }
    *errorCode = errno;
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * StdIOOutput--
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
StdIOOutput(
    ClientData instanceData,		/* Unused. */
    char *buf,				/* The data buffer. */
    int toWrite,			/* How many bytes to write? */
    int *errorCode)			/* Where to store error code. */
{
    int written;
    int fd;

    *errorCode = 0;
    errno = 0;
    fd = (int) ((FileState*)instanceData)->fileRef;
    written = write(fd, buf, (size_t) toWrite);
    if (written > -1) {
        return written;
    }
    *errorCode = errno;
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * StdIOSeek --
 *
 *	Seeks on an IO channel. Returns the new position.
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
StdIOSeek(
    ClientData instanceData,			/* Unused. */
    long offset,				/* Offset to seek to. */
    int mode,					/* Relative to where
                                                 * should we seek? */
    int *errorCodePtr)				/* To store error code. */
{
    int newLoc;
    int fd;

    *errorCodePtr = 0;
    fd = (int) ((FileState*)instanceData)->fileRef;
    newLoc = lseek(fd, offset, mode);
    if (newLoc > -1) {
        return newLoc;
    }
    *errorCodePtr = errno;
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PidObjCmd --
 *
 *      This procedure is invoked to process the "pid" Tcl command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *----------------------------------------------------------------------
 */

        /* ARGSUSED */
int
Tcl_PidObjCmd(dummy, interp, objc, objv)
    ClientData dummy;           /* Not used. */
    Tcl_Interp *interp;         /* Current interpreter. */
    int objc;                   /* Number of arguments. */
    Tcl_Obj *CONST *objv;       /* Argument strings. */
{
    ProcessSerialNumber psn;
    char buf[20]; 
    Tcl_Channel chan;
    Tcl_Obj *resultPtr;

    if (objc > 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "?channelId?");
        return TCL_ERROR;
    }
    if (objc == 1) {
        resultPtr = Tcl_GetObjResult(interp);
	GetCurrentProcess(&psn);
	sprintf(buf, "0x%08x%08x", psn.highLongOfPSN, psn.lowLongOfPSN);
        Tcl_SetStringObj(resultPtr, buf, -1);
    } else {
        chan = Tcl_GetChannel(interp, Tcl_GetString(objv[1]),
                NULL);
        if (chan == (Tcl_Channel) NULL) {
            return TCL_ERROR;
        } 
	/*
	 * We can't create pipelines on the Mac so
	 * this will always return an empty list.
	 */
    }
    
    return TCL_OK;
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
TclpGetDefaultStdChannel(
    int type)			/* One of TCL_STDIN, TCL_STDOUT, TCL_STDERR. */
{
    Tcl_Channel channel = NULL;
    int fd = 0;			/* Initializations needed to prevent */
    int mode = 0;		/* compiler warning (used before set). */
    char *bufMode = NULL;
    char channelName[16 + TCL_INTEGER_SPACE];
    int channelPermissions;
    FileState *fileState;

    /*
     * If the channels were not created yet, create them now and
     * store them in the static variables.
     */

    switch (type) {
	case TCL_STDIN:
	    fd = 0;
	    channelPermissions = TCL_READABLE;
	    bufMode = "line";
	    break;
	case TCL_STDOUT:
	    fd = 1;
	    channelPermissions = TCL_WRITABLE;
	    bufMode = "line";
	    break;
	case TCL_STDERR:
	    fd = 2;
	    channelPermissions = TCL_WRITABLE;
	    bufMode = "none";
	    break;
	default:
	    panic("TclGetDefaultStdChannel: Unexpected channel type");
	    break;
    }

    sprintf(channelName, "console%d", (int) fd);
    fileState = (FileState *) ckalloc((unsigned) sizeof(FileState));
    channel = Tcl_CreateChannel(&consoleChannelType, channelName,
	    (ClientData) fileState, channelPermissions);
    fileState->fileChan = channel;
    fileState->fileRef = fd;

    /*
     * Set up the normal channel options for stdio handles.
     */

    Tcl_SetChannelOption(NULL, channel, "-translation", "cr");
    Tcl_SetChannelOption(NULL, channel, "-buffering", bufMode);
    
    return channel;
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
TclpOpenFileChannel(
    Tcl_Interp *interp,			/* Interpreter for error reporting;
                                         * can be NULL. */
    char *fileName,			/* Name of file to open. */
    char *modeString,			/* A list of POSIX open modes or
                                         * a string such as "rw". */
    int permissions)			/* If the open involves creating a
                                         * file, with what modes to create
                                         * it? */
{
    Tcl_Channel chan;
    int mode;
    char *native;
    Tcl_DString ds, buffer;
    int errorCode;
    
    mode = GetOpenMode(interp, modeString);
    if (mode == -1) {
	return NULL;
    }

    if (Tcl_TranslateFileName(interp, fileName, &buffer) == NULL) {
	return NULL;
    }
    native = Tcl_UtfToExternalDString(NULL, Tcl_DStringValue(&buffer), 
    	    Tcl_DStringLength(&buffer), &ds);
    chan = OpenFileChannel(native, mode, permissions, &errorCode);
    Tcl_DStringFree(&ds);
    Tcl_DStringFree(&buffer);

    if (chan == NULL) {
	Tcl_SetErrno(errorCode);
	if (interp != (Tcl_Interp *) NULL) {
            Tcl_AppendResult(interp, "couldn't open \"", fileName, "\": ",
                    Tcl_PosixError(interp), (char *) NULL);
        }
	return NULL;
    }
    
    return chan;
}

/*
 *----------------------------------------------------------------------
 *
 * OpenFileChannel--
 *
 *	Opens a Macintosh file and creates a Tcl channel to control it.
 *
 * Results:
 *	A Tcl channel.
 *
 * Side effects:
 *	Will open a Macintosh file.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Channel
OpenFileChannel(
    CONST char *fileName,		/* Name of file to open (native). */
    int mode,				/* Mode for opening file. */
    int permissions,			/* If the open involves creating a
                                         * file, with what modes to create
                                         * it? */
    int *errorCodePtr)			/* Where to store error code. */
{
    int channelPermissions;
    Tcl_Channel chan;
    char macPermision;
    FSSpec fileSpec;
    OSErr err;
    short fileRef;
    FileState *fileState;
    char channelName[16 + TCL_INTEGER_SPACE];
    
    /*
     * Note we use fsRdWrShPerm instead of fsRdWrPerm which allows shared
     * writes on a file.  This isn't common on a mac but is common with 
     * Windows and UNIX and the feature is used by Tcl.
     */

    switch (mode & (TCL_RDONLY | TCL_WRONLY | TCL_RDWR)) {
	case TCL_RDWR:
	    channelPermissions = (TCL_READABLE | TCL_WRITABLE);
	    macPermision = fsRdWrShPerm;
	    break;
	case TCL_WRONLY:
	    /*
	     * Mac's fsRdPerm permission actually defaults to fsRdWrPerm because
	     * the Mac OS doesn't realy support write only access.  We explicitly
	     * set the permission fsRdWrShPerm so that we can have shared write
	     * access.
	     */
	    channelPermissions = TCL_WRITABLE;
	    macPermision = fsRdWrShPerm;
	    break;
	case TCL_RDONLY:
	default:
	    channelPermissions = TCL_READABLE;
	    macPermision = fsRdPerm;
	    break;
    }
     
    err = FSpLocationFromPath(strlen(fileName), fileName, &fileSpec);
    if ((err != noErr) && (err != fnfErr)) {
	*errorCodePtr = errno = TclMacOSErrorToPosixError(err);
	Tcl_SetErrno(errno);
	return NULL;
    }

    if ((err == fnfErr) && (mode & TCL_CREAT)) {
	err = HCreate(fileSpec.vRefNum, fileSpec.parID, fileSpec.name, 'MPW ', 'TEXT');
	if (err != noErr) {
	    *errorCodePtr = errno = TclMacOSErrorToPosixError(err);
	    Tcl_SetErrno(errno);
	    return NULL;
	}
    } else if ((mode & TCL_CREAT) && (mode & TCL_EXCL)) {
        *errorCodePtr = errno = EEXIST;
	Tcl_SetErrno(errno);
        return NULL;
    }

    err = HOpenDF(fileSpec.vRefNum, fileSpec.parID, fileSpec.name, macPermision, &fileRef);
    if (err != noErr) {
	*errorCodePtr = errno = TclMacOSErrorToPosixError(err);
	Tcl_SetErrno(errno);
	return NULL;
    }

    if (mode & TCL_TRUNC) {
	SetEOF(fileRef, 0);
    }
    
    sprintf(channelName, "file%d", (int) fileRef);
    fileState = (FileState *) ckalloc((unsigned) sizeof(FileState));
    chan = Tcl_CreateChannel(&fileChannelType, channelName, 
	(ClientData) fileState, channelPermissions);
    if (chan == (Tcl_Channel) NULL) {
	*errorCodePtr = errno = EFAULT;
	Tcl_SetErrno(errno);
	FSClose(fileRef);
	ckfree((char *) fileState);
        return NULL;
    }

    fileState->fileChan = chan;
    fileState->volumeRef = fileSpec.vRefNum;
    fileState->fileRef = fileRef;
    fileState->pending = 0;
    fileState->watchMask = 0;
    if (mode & TCL_ALWAYS_APPEND) {
	fileState->appendMode = true;
    } else {
	fileState->appendMode = false;
    }
        
    if ((mode & TCL_ALWAYS_APPEND) || (mode & TCL_APPEND)) {
        if (Tcl_Seek(chan, 0, SEEK_END) < 0) {
	    *errorCodePtr = errno = EFAULT;
	    Tcl_SetErrno(errno);
            Tcl_Close(NULL, chan);
            FSClose(fileRef);
            ckfree((char *) fileState);
            return NULL;
        }
    }
    
    return chan;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MakeFileChannel --
 *
 *	Makes a Tcl_Channel from an existing OS level file handle.
 *
 * Results:
 *	The Tcl_Channel created around the preexisting OS level file handle.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_MakeFileChannel(handle, mode)
    ClientData handle;		/* OS level handle. */
    int mode;			/* ORed combination of TCL_READABLE and
                                 * TCL_WRITABLE to indicate file mode. */
{
    /*
     * Not implemented yet.
     */

    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * FileBlockMode --
 *
 *	Set blocking or non-blocking mode on channel.  Macintosh files
 *	can never really be set to blocking or non-blocking modes.
 *	However, we don't generate an error - we just return success.
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
FileBlockMode(
    ClientData instanceData,		/* Unused. */
    int mode)				/* The mode to set. */
{
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * FileClose --
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
FileClose(
    ClientData instanceData,	/* Unused. */
    Tcl_Interp *interp)		/* Unused. */
{
    FileState *fileState = (FileState *) instanceData;
    int errorCode = 0;
    OSErr err;

    err = FSClose(fileState->fileRef);
    FlushVol(NULL, fileState->volumeRef);
    if (err != noErr) {
	errorCode = errno = TclMacOSErrorToPosixError(err);
	panic("error during file close");
    }

    ckfree((char *) fileState);
    Tcl_SetErrno(errorCode);
    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * FileInput --
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

int
FileInput(
    ClientData instanceData,	/* Unused. */
    char *buffer,				/* Where to store data read. */
    int bufSize,				/* How much space is available
                                 * in the buffer? */
    int *errorCodePtr)			/* Where to store error code. */
{
    FileState *fileState = (FileState *) instanceData;
    OSErr err;
    long length = bufSize;

    *errorCodePtr = 0;
    errno = 0;
    err = FSRead(fileState->fileRef, &length, buffer);
    if ((err == noErr) || (err == eofErr)) {
	return length;
    } else {
	switch (err) {
	    case ioErr:
		*errorCodePtr = errno = EIO;
	    case afpAccessDenied:
		*errorCodePtr = errno = EACCES;
	    default:
		*errorCodePtr = errno = EINVAL;
	}
        return -1;	
    }
    *errorCodePtr = errno;
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * FileOutput--
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
FileOutput(
    ClientData instanceData,		/* Unused. */
    char *buffer,			/* The data buffer. */
    int toWrite,			/* How many bytes to write? */
    int *errorCodePtr)			/* Where to store error code. */
{
    FileState *fileState = (FileState *) instanceData;
    long length = toWrite;
    OSErr err;

    *errorCodePtr = 0;
    errno = 0;
    
    if (fileState->appendMode == true) {
	FileSeek(instanceData, 0, SEEK_END, errorCodePtr);
	*errorCodePtr = 0;
    }
    
    err = FSWrite(fileState->fileRef, &length, buffer);
    if (err == noErr) {
	err = FlushFile(fileState->fileRef);
    } else {
	*errorCodePtr = errno = TclMacOSErrorToPosixError(err);
	return -1;
    }
    return length;
}

/*
 *----------------------------------------------------------------------
 *
 * FileSeek --
 *
 *	Seeks on an IO channel. Returns the new position.
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
FileSeek(
    ClientData instanceData,	/* Unused. */
    long offset,				/* Offset to seek to. */
    int mode,					/* Relative to where
                                 * should we seek? */
    int *errorCodePtr)			/* To store error code. */
{
    FileState *fileState = (FileState *) instanceData;
    IOParam pb;
    OSErr err;

    *errorCodePtr = 0;
    pb.ioCompletion = NULL;
    pb.ioRefNum = fileState->fileRef;
    if (mode == SEEK_SET) {
	pb.ioPosMode = fsFromStart;
    } else if (mode == SEEK_END) {
	pb.ioPosMode = fsFromLEOF;
    } else if (mode == SEEK_CUR) {
	err = PBGetFPosSync((ParmBlkPtr) &pb);
	if (pb.ioResult == noErr) {
	    if (offset == 0) {
		return pb.ioPosOffset;
	    }
	    offset += pb.ioPosOffset;
	}
	pb.ioPosMode = fsFromStart;
    }
    pb.ioPosOffset = offset;
    err = PBSetFPosSync((ParmBlkPtr) &pb);
    if (pb.ioResult == noErr){
	return pb.ioPosOffset;
    } else if (pb.ioResult == eofErr) {
	long currentEOF, newEOF;
	long buffer, i, length;
	
	err = PBGetEOFSync((ParmBlkPtr) &pb);
	currentEOF = (long) pb.ioMisc;
	if (mode == SEEK_SET) {
	    newEOF = offset;
	} else if (mode == SEEK_END) {
	    newEOF = offset + currentEOF;
	} else if (mode == SEEK_CUR) {
	    err = PBGetFPosSync((ParmBlkPtr) &pb);
	    newEOF = offset + pb.ioPosOffset;
	}
	
	/*
	 * Write 0's to the new EOF.
	 */
	pb.ioPosOffset = 0;
	pb.ioPosMode = fsFromLEOF;
	err = PBGetFPosSync((ParmBlkPtr) &pb);
	length = 1;
	buffer = 0;
	for (i = 0; i < (newEOF - currentEOF); i++) {
	    err = FSWrite(fileState->fileRef, &length, &buffer);
	}
	err = PBGetFPosSync((ParmBlkPtr) &pb);
	if (pb.ioResult == noErr){
	    return pb.ioPosOffset;
	}
    }
    *errorCodePtr = errno = TclMacOSErrorToPosixError(err);
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * CommonWatch --
 *
 *	Initialize the notifier to watch handles from this channel.
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
CommonWatch(
    ClientData instanceData,		/* The file state. */
    int mask)				/* Events of interest; an OR-ed
                                         * combination of TCL_READABLE,
                                         * TCL_WRITABLE and TCL_EXCEPTION. */
{
    FileState **nextPtrPtr, *ptr;
    FileState *infoPtr = (FileState *) instanceData;
    int oldMask = infoPtr->watchMask;
    ThreadSpecificData *tsdPtr;

    tsdPtr = FileInit();

    infoPtr->watchMask = mask;
    if (infoPtr->watchMask) {
	if (!oldMask) {
	    infoPtr->nextPtr = tsdPtr->firstFilePtr;
	    tsdPtr->firstFilePtr = infoPtr;
	}
    } else {
	if (oldMask) {
	    /*
	     * Remove the file from the list of watched files.
	     */

	    for (nextPtrPtr = &(tsdPtr->firstFilePtr), ptr = *nextPtrPtr;
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
 * GetOpenMode --
 *
 * Description:
 *	Computes a POSIX mode mask from a given string and also sets
 *	a flag to indicate whether the caller should seek to EOF during
 *	opening of the file.
 *
 * Results:
 *	On success, returns mode to pass to "open". If an error occurs, the
 *	returns -1 and if interp is not NULL, sets the interp's result to an
 *	error message.
 *
 * Side effects:
 *	Sets the integer referenced by seekFlagPtr to 1 if the caller
 *	should seek to EOF during opening the file.
 *
 * Special note:
 *	This code is based on a prototype implementation contributed
 *	by Mark Diekhans.
 *
 *----------------------------------------------------------------------
 */

static int
GetOpenMode(
    Tcl_Interp *interp,			/* Interpreter to use for error
					 * reporting - may be NULL. */
    CONST char *string)			/* Mode string, e.g. "r+" or
					 * "RDONLY CREAT". */
{
    int mode, modeArgc, c, i, gotRW;
    char **modeArgv, *flag;

    /*
     * Check for the simpler fopen-like access modes (e.g. "r").  They
     * are distinguished from the POSIX access modes by the presence
     * of a lower-case first letter.
     */

    mode = 0;
    /*
     * Guard against international characters before using byte oriented
     * routines.
     */

    if (!(string[0] & 0x80)
	    && islower(UCHAR(string[0]))) { /* INTL: ISO only. */
	switch (string[0]) {
	    case 'r':
		mode = TCL_RDONLY;
		break;
	    case 'w':
		mode = TCL_WRONLY|TCL_CREAT|TCL_TRUNC;
		break;
	    case 'a':
		mode = TCL_WRONLY|TCL_CREAT|TCL_APPEND;
		break;
	    default:
		error:
                if (interp != (Tcl_Interp *) NULL) {
                    Tcl_AppendResult(interp,
                            "illegal access mode \"", string, "\"",
                            (char *) NULL);
                }
		return -1;
	}
	if (string[1] == '+') {
	    mode &= ~(TCL_RDONLY|TCL_WRONLY);
	    mode |= TCL_RDWR;
	    if (string[2] != 0) {
		goto error;
	    }
	} else if (string[1] != 0) {
	    goto error;
	}
        return mode;
    }

    /*
     * The access modes are specified using a list of POSIX modes
     * such as TCL_CREAT.
     */

    if (Tcl_SplitList(interp, string, &modeArgc, &modeArgv) != TCL_OK) {
        if (interp != (Tcl_Interp *) NULL) {
            Tcl_AddErrorInfo(interp,
                    "\n    while processing open access modes \"");
            Tcl_AddErrorInfo(interp, string);
            Tcl_AddErrorInfo(interp, "\"");
        }
        return -1;
    }
    
    gotRW = 0;
    for (i = 0; i < modeArgc; i++) {
	flag = modeArgv[i];
	c = flag[0];
	if ((c == 'R') && (strcmp(flag, "RDONLY") == 0)) {
	    mode = (mode & ~TCL_RW_MODES) | TCL_RDONLY;
	    gotRW = 1;
	} else if ((c == 'W') && (strcmp(flag, "WRONLY") == 0)) {
	    mode = (mode & ~TCL_RW_MODES) | TCL_WRONLY;
	    gotRW = 1;
	} else if ((c == 'R') && (strcmp(flag, "RDWR") == 0)) {
	    mode = (mode & ~TCL_RW_MODES) | TCL_RDWR;
	    gotRW = 1;
	} else if ((c == 'A') && (strcmp(flag, "APPEND") == 0)) {
	    mode |= TCL_ALWAYS_APPEND;
	} else if ((c == 'C') && (strcmp(flag, "CREAT") == 0)) {
	    mode |= TCL_CREAT;
	} else if ((c == 'E') && (strcmp(flag, "EXCL") == 0)) {
	    mode |= TCL_EXCL;
	} else if ((c == 'N') && (strcmp(flag, "NOCTTY") == 0)) {
	    mode |= TCL_NOCTTY;
	} else if ((c == 'N') && (strcmp(flag, "NONBLOCK") == 0)) {
	    mode |= TCL_NONBLOCK;
	} else if ((c == 'T') && (strcmp(flag, "TRUNC") == 0)) {
	    mode |= TCL_TRUNC;
	} else {
            if (interp != (Tcl_Interp *) NULL) {
                Tcl_AppendResult(interp, "invalid access mode \"", flag,
                        "\": must be RDONLY, WRONLY, RDWR, APPEND, CREAT",
                        " EXCL, NOCTTY, NONBLOCK, or TRUNC", (char *) NULL);
            }
	    ckfree((char *) modeArgv);
	    return -1;
	}
    }
    ckfree((char *) modeArgv);
    if (!gotRW) {
        if (interp != (Tcl_Interp *) NULL) {
            Tcl_AppendResult(interp, "access mode must include either",
                    " RDONLY, WRONLY, or RDWR", (char *) NULL);
        }
	return -1;
    }
    return mode;
}
