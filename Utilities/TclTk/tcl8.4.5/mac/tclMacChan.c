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
#include "tclIO.h"

#ifdef __MSL__
#include <unix.mac.h>
#define TCL_FILE_CREATOR (__getcreator(0))
#else
#define TCL_FILE_CREATOR 'MPW '
#endif

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
			    CONST char *buf, int toWrite, int *errorCode));
static int		FileSeek _ANSI_ARGS_((ClientData instanceData,
			    long offset, int mode, int *errorCode));
static void		FileSetupProc _ANSI_ARGS_((ClientData clientData,
			    int flags));
static Tcl_Channel	OpenFileChannel _ANSI_ARGS_((CONST char *fileName, 
			    int mode, int permissions, int *errorCodePtr));
static int		StdIOBlockMode _ANSI_ARGS_((ClientData instanceData,
			    int mode));
static int		StdIOClose _ANSI_ARGS_((ClientData instanceData,
			    Tcl_Interp *interp));
static int		StdIOInput _ANSI_ARGS_((ClientData instanceData,
			    char *buf, int toRead, int *errorCode));
static int		StdIOOutput _ANSI_ARGS_((ClientData instanceData,
			    CONST char *buf, int toWrite, int *errorCode));
static int		StdIOSeek _ANSI_ARGS_((ClientData instanceData,
			    long offset, int mode, int *errorCode));
static int		StdReady _ANSI_ARGS_((ClientData instanceData,
		            int mask));

/*
 * This structure describes the channel type structure for file based IO:
 */

static Tcl_ChannelType consoleChannelType = {
    "file",			/* Type name. */
    (Tcl_ChannelTypeVersion)StdIOBlockMode,		/* Set blocking/nonblocking mode.*/
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
    (Tcl_ChannelTypeVersion)FileBlockMode,		/* Set blocking or
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
    if (!TclInThreadExit()) {
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
    CONST char *buf,			/* The data buffer. */
    int toWrite,			/* How many bytes to write? */
    int *errorCode)			/* Where to store error code. */
{
    int written;
    int fd;

    *errorCode = 0;
    errno = 0;
    fd = (int) ((FileState*)instanceData)->fileRef;
    written = write(fd, (void*)buf, (size_t) toWrite);
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
    ClientData instanceData,	/* Unused. */
    long offset,		/* Offset to seek to. */
    int mode,			/* Relative to where should we seek? */
    int *errorCodePtr)		/* To store error code. */
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
 *	Open a File based channel on MacOS systems.
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
    Tcl_Obj *pathPtr,			/* Name of file to open. */
    int mode,				/* POSIX open mode. */
    int permissions)			/* If the open involves creating a
                                         * file, with what modes to create
                                         * it? */
{
    Tcl_Channel chan;
    CONST char *native;
    int errorCode;
    
    native = Tcl_FSGetNativePath(pathPtr);
    if (native == NULL) {
	return NULL;
    }
    chan = OpenFileChannel(native, mode, permissions, &errorCode);

    if (chan == NULL) {
	Tcl_SetErrno(errorCode);
	if (interp != (Tcl_Interp *) NULL) {
            Tcl_AppendResult(interp, "couldn't open \"", 
			     Tcl_GetString(pathPtr), "\": ",
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
    ThreadSpecificData *tsdPtr;
    
    tsdPtr = FileInit();

    /*
     * Note we use fsRdWrShPerm instead of fsRdWrPerm which allows shared
     * writes on a file.  This isn't common on a mac but is common with 
     * Windows and UNIX and the feature is used by Tcl.
     */

    switch (mode & (O_RDONLY | O_WRONLY | O_RDWR)) {
	case O_RDWR:
	    channelPermissions = (TCL_READABLE | TCL_WRITABLE);
	    macPermision = fsRdWrShPerm;
	    break;
	case O_WRONLY:
	    /*
	     * Mac's fsRdPerm permission actually defaults to fsRdWrPerm because
	     * the Mac OS doesn't realy support write only access.  We explicitly
	     * set the permission fsRdWrShPerm so that we can have shared write
	     * access.
	     */
	    channelPermissions = TCL_WRITABLE;
	    macPermision = fsRdWrShPerm;
	    break;
	case O_RDONLY:
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

    if ((err == fnfErr) && (mode & O_CREAT)) {
	err = HCreate(fileSpec.vRefNum, fileSpec.parID, fileSpec.name, TCL_FILE_CREATOR, 'TEXT');
	if (err != noErr) {
	    *errorCodePtr = errno = TclMacOSErrorToPosixError(err);
	    Tcl_SetErrno(errno);
	    return NULL;
	}
    } else if ((mode & O_CREAT) && (mode & O_EXCL)) {
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

    if (mode & O_TRUNC) {
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
    fileState->nextPtr = tsdPtr->firstFilePtr;
    tsdPtr->firstFilePtr = fileState;
    fileState->volumeRef = fileSpec.vRefNum;
    fileState->fileRef = fileRef;
    fileState->pending = 0;
    fileState->watchMask = 0;
    if (mode & O_APPEND) {
	fileState->appendMode = true;
    } else {
	fileState->appendMode = false;
    }
        
    if ((mode & O_APPEND) || (mode & O_APPEND)) {
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
    CONST char *buffer,			/* The data buffer. */
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
    long offset,		/* Offset to seek to. */
    int mode,			/* Relative to where should we seek? */
    int *errorCodePtr)		/* To store error code. */
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
    FileState *infoPtr = (FileState *) instanceData;
    Tcl_Time blockTime = { 0, 0 };

    infoPtr->watchMask = mask;
    if (infoPtr->watchMask) {
	Tcl_SetMaxBlockTime(&blockTime);
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
    FileState *infoPtr;
    FileState **nextPtrPtr;
    int removed = 0;

    if (chanPtr->typePtr != &fileChannelType)
        return;

    infoPtr = (FileState *) chanPtr->instanceData;

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
    FileState *infoPtr;

    if (chanPtr->typePtr != &fileChannelType)
        return;

    infoPtr = (FileState *) chanPtr->instanceData;

    infoPtr->nextPtr = tsdPtr->firstFilePtr;
    tsdPtr->firstFilePtr = infoPtr;
}
