/* 
 * tclWinConsole.c --
 *
 *	This file implements the Windows-specific console functions,
 *	and the "console" channel driver.
 *
 * Copyright (c) 1999 by Scriptics Corp.
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
 * The consoleMutex locks around access to the initialized variable, and it is
 * used to protect background threads from being terminated while they are
 * using APIs that hold locks.
 */

TCL_DECLARE_MUTEX(consoleMutex)

/*
 * Bit masks used in the flags field of the ConsoleInfo structure below.
 */

#define CONSOLE_PENDING	(1<<0)	/* Message is pending in the queue. */
#define CONSOLE_ASYNC	(1<<1)	/* Channel is non-blocking. */

/*
 * Bit masks used in the sharedFlags field of the ConsoleInfo structure below.
 */

#define CONSOLE_EOF	  (1<<2)  /* Console has reached EOF. */
#define CONSOLE_BUFFERED  (1<<3)  /* data was read into a buffer by the reader
				     thread */

#define CONSOLE_BUFFER_SIZE (8*1024)
/*
 * This structure describes per-instance data for a console based channel.
 */

typedef struct ConsoleInfo {
    HANDLE handle;
    int type;
    struct ConsoleInfo *nextPtr;/* Pointer to next registered console. */
    Tcl_Channel channel;	/* Pointer to channel structure. */
    int validMask;		/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, or TCL_EXCEPTION: indicates
				 * which operations are valid on the file. */
    int watchMask;		/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, or TCL_EXCEPTION: indicates
				 * which events should be reported. */
    int flags;			/* State flags, see above for a list. */
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
				 * to write to the console. */
    HANDLE startReader;		/* Auto-reset event used by the main thread to
				 * signal when the reader thread should attempt
				 * to read from the console. */
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
    int bytesRead;              /* number of bytes in the buffer */
    int offset;                 /* number of bytes read out of the buffer */
    char buffer[CONSOLE_BUFFER_SIZE];
                                /* Data consumed by reader thread. */
} ConsoleInfo;

typedef struct ThreadSpecificData {
    /*
     * The following pointer refers to the head of the list of consoles
     * that are being watched for file events.
     */
    
    ConsoleInfo *firstConsolePtr;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * The following structure is what is added to the Tcl event queue when
 * console events are generated.
 */

typedef struct ConsoleEvent {
    Tcl_Event header;		/* Information that is standard for
				 * all events. */
    ConsoleInfo *infoPtr;	/* Pointer to console info structure.  Note
				 * that we still have to verify that the
				 * console exists before dereferencing this
				 * pointer. */
} ConsoleEvent;

/*
 * Declarations for functions used only in this file.
 */

static int		ApplicationType(Tcl_Interp *interp,
			    const char *fileName, char *fullName);
static void		BuildCommandLine(const char *executable, int argc, 
			    char **argv, Tcl_DString *linePtr);
static void		CopyChannel(HANDLE dst, HANDLE src);
static BOOL		HasConsole(void);
static TclFile		MakeFile(HANDLE handle);
static char *		MakeTempFile(Tcl_DString *namePtr);
static int		ConsoleBlockModeProc(ClientData instanceData, int mode);
static void		ConsoleCheckProc(ClientData clientData, int flags);
static int		ConsoleCloseProc(ClientData instanceData,
			    Tcl_Interp *interp);
static int		ConsoleEventProc(Tcl_Event *evPtr, int flags);
static void		ConsoleExitHandler(ClientData clientData);
static int		ConsoleGetHandleProc(ClientData instanceData,
			    int direction, ClientData *handlePtr);
static ThreadSpecificData *ConsoleInit(void);
static int		ConsoleInputProc(ClientData instanceData, char *buf,
			    int toRead, int *errorCode);
static int		ConsoleOutputProc(ClientData instanceData, char *buf,
			    int toWrite, int *errorCode);
static DWORD WINAPI	ConsoleReaderThread(LPVOID arg);
static void		ConsoleSetupProc(ClientData clientData, int flags);
static void		ConsoleWatchProc(ClientData instanceData, int mask);
static DWORD WINAPI	ConsoleWriterThread(LPVOID arg);
static void		ProcExitHandler(ClientData clientData);
static int		TempFileName(WCHAR name[MAX_PATH]);
static int		WaitForRead(ConsoleInfo *infoPtr, int blocking);

/*
 * This structure describes the channel type structure for command console
 * based IO.
 */

static Tcl_ChannelType consoleChannelType = {
    "console",			/* Type name. */
    ConsoleBlockModeProc,	/* Set blocking or non-blocking mode.*/
    ConsoleCloseProc,		/* Close proc. */
    ConsoleInputProc,		/* Input proc. */
    ConsoleOutputProc,		/* Output proc. */
    NULL,			/* Seek proc. */
    NULL,			/* Set option proc. */
    NULL,			/* Get option proc. */
    ConsoleWatchProc,		/* Set up notifier to watch the channel. */
    ConsoleGetHandleProc,	/* Get an OS handle from channel. */
};

/*
 *----------------------------------------------------------------------
 *
 * ConsoleInit --
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

static ThreadSpecificData *
ConsoleInit()
{
    ThreadSpecificData *tsdPtr;

    /*
     * Check the initialized flag first, then check again in the mutex.
     * This is a speed enhancement.
     */

    if (!initialized) {
	Tcl_MutexLock(&consoleMutex);
	if (!initialized) {
	    initialized = 1;
	    Tcl_CreateExitHandler(ProcExitHandler, NULL);
	}
	Tcl_MutexUnlock(&consoleMutex);
    }

    tsdPtr = (ThreadSpecificData *)TclThreadDataKeyGet(&dataKey);
    if (tsdPtr == NULL) {
	tsdPtr = TCL_TSD_INIT(&dataKey);
	tsdPtr->firstConsolePtr = NULL;
	Tcl_CreateEventSource(ConsoleSetupProc, ConsoleCheckProc, NULL);
	Tcl_CreateThreadExitHandler(ConsoleExitHandler, NULL);
    }
    return tsdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleExitHandler --
 *
 *	This function is called to cleanup the console module before
 *	Tcl is unloaded.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the console event source.
 *
 *----------------------------------------------------------------------
 */

static void
ConsoleExitHandler(
    ClientData clientData)	/* Old window proc */
{
    Tcl_DeleteEventSource(ConsoleSetupProc, ConsoleCheckProc, NULL);
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
    Tcl_MutexLock(&consoleMutex);
    initialized = 0;
    Tcl_MutexUnlock(&consoleMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleSetupProc --
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
ConsoleSetupProc(
    ClientData data,		/* Not used. */
    int flags)			/* Event flags as passed to Tcl_DoOneEvent. */
{
    ConsoleInfo *infoPtr;
    Tcl_Time blockTime = { 0, 0 };
    int block = 1;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return;
    }
    
    /*
     * Look to see if any events are already pending.  If they are, poll.
     */

    for (infoPtr = tsdPtr->firstConsolePtr; infoPtr != NULL; 
	    infoPtr = infoPtr->nextPtr) {
	if (infoPtr->watchMask & TCL_WRITABLE) {
	    if (WaitForSingleObject(infoPtr->writable, 0) != WAIT_TIMEOUT) {
		block = 0;
	    }
	}
	if (infoPtr->watchMask & TCL_READABLE) {
	    if (WaitForRead(infoPtr, 0) >= 0) {
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
 * ConsoleCheckProc --
 *
 *	This procedure is called by Tcl_DoOneEvent to check the console
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
ConsoleCheckProc(
    ClientData data,		/* Not used. */
    int flags)			/* Event flags as passed to Tcl_DoOneEvent. */
{
    ConsoleInfo *infoPtr;
    ConsoleEvent *evPtr;
    int needEvent;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return;
    }
    
    /*
     * Queue events for any ready consoles that don't already have events
     * queued.
     */

    for (infoPtr = tsdPtr->firstConsolePtr; infoPtr != NULL; 
	    infoPtr = infoPtr->nextPtr) {
	if (infoPtr->flags & CONSOLE_PENDING) {
	    continue;
	}
	
	/*
	 * Queue an event if the console is signaled for reading or writing.
	 */

	needEvent = 0;
	if (infoPtr->watchMask & TCL_WRITABLE) {
	    if (WaitForSingleObject(infoPtr->writable, 0) != WAIT_TIMEOUT) {
		needEvent = 1;
	    }
	}
	
	if (infoPtr->watchMask & TCL_READABLE) {
	    if (WaitForRead(infoPtr, 0) >= 0) {
		needEvent = 1;
	    }
	}

	if (needEvent) {
	    infoPtr->flags |= CONSOLE_PENDING;
	    evPtr = (ConsoleEvent *) ckalloc(sizeof(ConsoleEvent));
	    evPtr->header.proc = ConsoleEventProc;
	    evPtr->infoPtr = infoPtr;
	    Tcl_QueueEvent((Tcl_Event *) evPtr, TCL_QUEUE_TAIL);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ConsoleBlockModeProc --
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
ConsoleBlockModeProc(
    ClientData instanceData,	/* Instance data for channel. */
    int mode)			/* TCL_MODE_BLOCKING or
                                 * TCL_MODE_NONBLOCKING. */
{
    ConsoleInfo *infoPtr = (ConsoleInfo *) instanceData;
    
    /*
     * Consoles on Windows can not be switched between blocking and nonblocking,
     * hence we have to emulate the behavior. This is done in the input
     * function by checking against a bit in the state. We set or unset the
     * bit here to cause the input function to emulate the correct behavior.
     */

    if (mode == TCL_MODE_NONBLOCKING) {
	infoPtr->flags |= CONSOLE_ASYNC;
    } else {
	infoPtr->flags &= ~(CONSOLE_ASYNC);
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleCloseProc --
 *
 *	Closes a console based IO channel.
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
ConsoleCloseProc(
    ClientData instanceData,	/* Pointer to ConsoleInfo structure. */
    Tcl_Interp *interp)		/* For error reporting. */
{
    ConsoleInfo *consolePtr = (ConsoleInfo *) instanceData;
    int errorCode;
    ConsoleInfo *infoPtr, **nextPtrPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    errorCode = 0;
    
    /*
     * Clean up the background thread if necessary.  Note that this
     * must be done before we can close the file, since the 
     * thread may be blocking trying to read from the console.
     */
    
    if (consolePtr->readThread) {
	/*
	 * Forcibly terminate the background thread.  We cannot rely on the
	 * thread to cleanly terminate itself because we have no way of
	 * closing the handle without blocking in the case where the
	 * thread is in the middle of an I/O operation.  Note that we need
	 * to guard against terminating the thread while it is in the
	 * middle of Tcl_ThreadAlert because it won't be able to release
	 * the notifier lock.
	 */

	Tcl_MutexLock(&consoleMutex);
	TerminateThread(consolePtr->readThread, 0);

	/*
	 * Wait for the thread to terminate.  This ensures that we are
	 * completely cleaned up before we leave this function. 
	 */

	WaitForSingleObject(consolePtr->readThread, INFINITE);
	Tcl_MutexUnlock(&consoleMutex);

	CloseHandle(consolePtr->readThread);
	CloseHandle(consolePtr->readable);
	CloseHandle(consolePtr->startReader);
	consolePtr->readThread = NULL;
    }
    consolePtr->validMask &= ~TCL_READABLE;

    /*
     * Wait for the writer thread to finish the current buffer, then
     * terminate the thread and close the handles.  If the channel is
     * nonblocking, there should be no pending write operations.
     */
    
    if (consolePtr->writeThread) {
	WaitForSingleObject(consolePtr->writable, INFINITE);

	/*
	 * Forcibly terminate the background thread.  We cannot rely on the
	 * thread to cleanly terminate itself because we have no way of
	 * closing the handle without blocking in the case where the
	 * thread is in the middle of an I/O operation.  Note that we need
	 * to guard against terminating the thread while it is in the
	 * middle of Tcl_ThreadAlert because it won't be able to release
	 * the notifier lock.
	 */

	Tcl_MutexLock(&consoleMutex);
	TerminateThread(consolePtr->writeThread, 0);

	/*
	 * Wait for the thread to terminate.  This ensures that we are
	 * completely cleaned up before we leave this function. 
	 */

	WaitForSingleObject(consolePtr->writeThread, INFINITE);
	Tcl_MutexUnlock(&consoleMutex);

	CloseHandle(consolePtr->writeThread);
	CloseHandle(consolePtr->writable);
	CloseHandle(consolePtr->startWriter);
	consolePtr->writeThread = NULL;
    }
    consolePtr->validMask &= ~TCL_WRITABLE;


    /*
     * Don't close the Win32 handle if the handle is a standard channel
     * during the exit process.  Otherwise, one thread may kill the stdio
     * of another.
     */

    if (!TclInExit() 
	    || ((GetStdHandle(STD_INPUT_HANDLE) != consolePtr->handle)
		&& (GetStdHandle(STD_OUTPUT_HANDLE) != consolePtr->handle)
		&& (GetStdHandle(STD_ERROR_HANDLE) != consolePtr->handle))) {
	if (CloseHandle(consolePtr->handle) == FALSE) {
	    TclWinConvertError(GetLastError());
	    errorCode = errno;
	}
    }
    
    consolePtr->watchMask &= consolePtr->validMask;

    /*
     * Remove the file from the list of watched files.
     */

    for (nextPtrPtr = &(tsdPtr->firstConsolePtr), infoPtr = *nextPtrPtr;
	    infoPtr != NULL;
	    nextPtrPtr = &infoPtr->nextPtr, infoPtr = *nextPtrPtr) {
	if (infoPtr == (ConsoleInfo *)consolePtr) {
	    *nextPtrPtr = infoPtr->nextPtr;
	    break;
	}
    }
    if (consolePtr->writeBuf != NULL) {
	ckfree(consolePtr->writeBuf);
	consolePtr->writeBuf = 0;
    }
    ckfree((char*) consolePtr);

    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleInputProc --
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
ConsoleInputProc(
    ClientData instanceData,		/* Console state. */
    char *buf,				/* Where to store data read. */
    int bufSize,			/* How much space is available
                                         * in the buffer? */
    int *errorCode)			/* Where to store error code. */
{
    ConsoleInfo *infoPtr = (ConsoleInfo *) instanceData;
    DWORD count, bytesRead = 0;
    int result;

    *errorCode = 0;

    /*
     * Synchronize with the reader thread.
     */
    
    result = WaitForRead(infoPtr, (infoPtr->flags & CONSOLE_ASYNC) ? 0 : 1);
    
    /*
     * If an error occurred, return immediately.
     */
    
    if (result == -1) {
	*errorCode = errno;
	return -1;
    }

    if (infoPtr->readFlags & CONSOLE_BUFFERED) {
	/*
	 * Data is stored in the buffer.
	 */

	if (bufSize < (infoPtr->bytesRead - infoPtr->offset)) {
	    memcpy(buf, &infoPtr->buffer[infoPtr->offset], bufSize);
	    bytesRead = bufSize;
	    infoPtr->offset += bufSize;
	} else {
	    memcpy(buf, &infoPtr->buffer[infoPtr->offset], bufSize);
	    bytesRead = infoPtr->bytesRead - infoPtr->offset;

	    /*
	     * Reset the buffer
	     */
	    
	    infoPtr->readFlags &= ~CONSOLE_BUFFERED;
	    infoPtr->offset = 0;
	}

	return bytesRead;
    }
    
    /*
     * Attempt to read bufSize bytes.  The read will return immediately
     * if there is any data available.  Otherwise it will block until
     * at least one byte is available or an EOF occurs.
     */

    if (ReadConsole(infoPtr->handle, (LPVOID) buf, (DWORD) bufSize, &count,
		    (LPOVERLAPPED) NULL) == TRUE) {
	buf[count] = '\0';
	return count;
    }

    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleOutputProc --
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
ConsoleOutputProc(
    ClientData instanceData,		/* Console state. */
    char *buf,				/* The data buffer. */
    int toWrite,			/* How many bytes to write? */
    int *errorCode)			/* Where to store error code. */
{
    ConsoleInfo *infoPtr = (ConsoleInfo *) instanceData;
    DWORD bytesWritten, timeout;
    
    *errorCode = 0;
    timeout = (infoPtr->flags & CONSOLE_ASYNC) ? 0 : INFINITE;
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

    if (infoPtr->flags & CONSOLE_ASYNC) {
	/*
	 * The console is non-blocking, so copy the data into the output
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

	if (WriteFile(infoPtr->handle, (LPVOID) buf, (DWORD) toWrite,
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
 * ConsoleEventProc --
 *
 *	This function is invoked by Tcl_ServiceEvent when a file event
 *	reaches the front of the event queue.  This procedure invokes
 *	Tcl_NotifyChannel on the console.
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
ConsoleEventProc(
    Tcl_Event *evPtr,		/* Event to service. */
    int flags)			/* Flags that indicate what events to
				 * handle, such as TCL_FILE_EVENTS. */
{
    ConsoleEvent *consoleEvPtr = (ConsoleEvent *)evPtr;
    ConsoleInfo *infoPtr;
    int mask;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return 0;
    }

    /*
     * Search through the list of watched consoles for the one whose handle
     * matches the event.  We do this rather than simply dereferencing
     * the handle in the event so that consoles can be deleted while the
     * event is in the queue.
     */

    for (infoPtr = tsdPtr->firstConsolePtr; infoPtr != NULL;
	    infoPtr = infoPtr->nextPtr) {
	if (consoleEvPtr->infoPtr == infoPtr) {
	    infoPtr->flags &= ~(CONSOLE_PENDING);
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
     * Check to see if the console is readable.  Note
     * that we can't tell if a console is writable, so we always report it
     * as being writable unless we have detected EOF.
     */

    mask = 0;
    if (infoPtr->watchMask & TCL_WRITABLE) {
	if (WaitForSingleObject(infoPtr->writable, 0) != WAIT_TIMEOUT) {
	  mask = TCL_WRITABLE;
	}
    }

    if (infoPtr->watchMask & TCL_READABLE) {
	if (WaitForRead(infoPtr, 0) >= 0) {
	    if (infoPtr->readFlags & CONSOLE_EOF) {
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
 * ConsoleWatchProc --
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
ConsoleWatchProc(
    ClientData instanceData,		/* Console state. */
    int mask)				/* What events to watch for, OR-ed
                                         * combination of TCL_READABLE,
                                         * TCL_WRITABLE and TCL_EXCEPTION. */
{
    ConsoleInfo **nextPtrPtr, *ptr;
    ConsoleInfo *infoPtr = (ConsoleInfo *) instanceData;
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
	    infoPtr->nextPtr = tsdPtr->firstConsolePtr;
	    tsdPtr->firstConsolePtr = infoPtr;
	}
	Tcl_SetMaxBlockTime(&blockTime);
    } else {
	if (oldMask) {
	    /*
	     * Remove the console from the list of watched consoles.
	     */

	    for (nextPtrPtr = &(tsdPtr->firstConsolePtr), ptr = *nextPtrPtr;
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
 * ConsoleGetHandleProc --
 *
 *	Called from Tcl_GetChannelHandle to retrieve OS handles from
 *	inside a command consoleline based channel.
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
ConsoleGetHandleProc(
    ClientData instanceData,	/* The console state. */
    int direction,		/* TCL_READABLE or TCL_WRITABLE */
    ClientData *handlePtr)	/* Where to store the handle.  */
{
    ConsoleInfo *infoPtr = (ConsoleInfo *) instanceData;

    *handlePtr = (ClientData) infoPtr->handle;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * WaitForRead --
 *
 *	Wait until some data is available, the console is at
 *	EOF or the reader thread is blocked waiting for data (if the
 *	channel is in non-blocking mode).
 *
 * Results:
 *	Returns 1 if console is readable.  Returns 0 if there is no data
 *	on the console, but there is buffered data.  Returns -1 if an
 *	error occurred.  If an error occurred, the threads may not
 *	be synchronized.
 *
 * Side effects:
 *	Updates the shared state flags.  If no error occurred,
 *      the reader thread is blocked waiting for a signal from the 
 *      main thread.
 *
 *----------------------------------------------------------------------
 */

static int
WaitForRead(
    ConsoleInfo *infoPtr,		/* Console state. */
    int blocking)		/* Indicates whether call should be
				 * blocking or not. */
{
    DWORD timeout, count;
    HANDLE *handle = infoPtr->handle;
    INPUT_RECORD input;
    
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
	 * If the console has hit EOF, it is always readable.
	 */
	
	if (infoPtr->readFlags & CONSOLE_EOF) {
	    return 1;
	}
	
	if (PeekConsoleInput(handle, &input, 1, &count) == FALSE) {
            /*
	     * Check to see if the peek failed because of EOF.
	     */
	    
	    TclWinConvertError(GetLastError());
	    
	    if (errno == EOF) {
		infoPtr->readFlags |= CONSOLE_EOF;
		return 1;
	    }

	    /*
	     * Ignore errors if there is data in the buffer.
	     */
	    
	    if (infoPtr->readFlags & CONSOLE_BUFFERED) {
		return 0;
	    } else {
		return -1;
	    }
	}

	/*
	 * If there is data in the buffer, the console must be
	 * readable (since it is a line-oriented device).
	 */

	if (infoPtr->readFlags & CONSOLE_BUFFERED) {
	    return 1;
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
 * ConsoleReaderThread --
 *
 *	This function runs in a separate thread and waits for input
 *	to become available on a console.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Signals the main thread when input become available.  May
 *	cause the main thread to wake up by posting a message.  May
 *	one line from the console for each wait operation.
 *
 *----------------------------------------------------------------------
 */

static DWORD WINAPI
ConsoleReaderThread(LPVOID arg)
{
    ConsoleInfo *infoPtr = (ConsoleInfo *)arg;
    HANDLE *handle = infoPtr->handle;
    DWORD count;

    for (;;) {
	/*
	 * Wait for the main thread to signal before attempting to wait.
	 */

	WaitForSingleObject(infoPtr->startReader, INFINITE);

	count = 0;

	/* 
	 * Look for data on the console, but first ignore any events
	 * that are not KEY_EVENTs 
	 */
	if (ReadConsole(handle, infoPtr->buffer, CONSOLE_BUFFER_SIZE,
		&infoPtr->bytesRead, NULL) != FALSE) {
	    /*
	     * Data was stored in the buffer.
	     */
	    
	    infoPtr->readFlags |= CONSOLE_BUFFERED;
	} else {
	    DWORD err;
	    err = GetLastError();
	    
	    if (err == EOF) {
		infoPtr->readFlags = CONSOLE_EOF;
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

	Tcl_MutexLock(&consoleMutex);
	Tcl_ThreadAlert(infoPtr->threadId);
	Tcl_MutexUnlock(&consoleMutex);
    }
    return 0;			/* NOT REACHED */
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleWriterThread --
 *
 *	This function runs in a separate thread and writes data
 *	onto a console.
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
ConsoleWriterThread(LPVOID arg)
{

    ConsoleInfo *infoPtr = (ConsoleInfo *)arg;
    HANDLE *handle = infoPtr->handle;
    DWORD count, toWrite;
    char *buf;

    for (;;) {
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

	Tcl_MutexLock(&consoleMutex);
	Tcl_ThreadAlert(infoPtr->threadId);
	Tcl_MutexUnlock(&consoleMutex);
    }
    return 0;			/* NOT REACHED */
}



/*
 *----------------------------------------------------------------------
 *
 * TclWinOpenConsoleChannel --
 *
 *	Constructs a Console channel for the specified standard OS handle.
 *      This is a helper function to break up the construction of 
 *      channels into File, Console, or Serial.
 *
 * Results:
 *	Returns the new channel, or NULL.
 *
 * Side effects:
 *	May open the channel
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
TclWinOpenConsoleChannel(handle, channelName, permissions)
    HANDLE handle;
    char *channelName;
    int permissions;
{
    char encoding[4 + TCL_INTEGER_SPACE];
    ConsoleInfo *infoPtr;
    ThreadSpecificData *tsdPtr;
    DWORD id;

    tsdPtr = ConsoleInit();

    /*
     * See if a channel with this handle already exists.
     */
    
    infoPtr = (ConsoleInfo *) ckalloc((unsigned) sizeof(ConsoleInfo));
    memset(infoPtr, 0, sizeof(ConsoleInfo));

    infoPtr->validMask = permissions;
    infoPtr->handle = handle;

    wsprintfA(encoding, "cp%d", GetConsoleCP());
    
    /*
     * Use the pointer for the name of the result channel.
     * This keeps the channel names unique, since some may share
     * handles (stdin/stdout/stderr for instance).
     */

    wsprintfA(channelName, "file%lx", (int) infoPtr);
    
    infoPtr->channel = Tcl_CreateChannel(&consoleChannelType, channelName,
            (ClientData) infoPtr, permissions);

    infoPtr->threadId = Tcl_GetCurrentThread();

    if (permissions & TCL_READABLE) {
	infoPtr->readable = CreateEvent(NULL, TRUE, TRUE, NULL);
	infoPtr->startReader = CreateEvent(NULL, FALSE, FALSE, NULL);
	infoPtr->readThread = CreateThread(NULL, 8000, ConsoleReaderThread,
	        infoPtr, 0, &id);
	SetThreadPriority(infoPtr->readThread, THREAD_PRIORITY_HIGHEST); 
    }

    if (permissions & TCL_WRITABLE) {
	infoPtr->writable = CreateEvent(NULL, TRUE, TRUE, NULL);
	infoPtr->startWriter = CreateEvent(NULL, FALSE, FALSE, NULL);
	infoPtr->writeThread = CreateThread(NULL, 8000, ConsoleWriterThread,
	        infoPtr, 0, &id);
    }

    /*
     * Files have default translation of AUTO and ^Z eof char, which
     * means that a ^Z will be accepted as EOF when reading.
     */
    
    Tcl_SetChannelOption(NULL, infoPtr->channel, "-translation", "auto");
    Tcl_SetChannelOption(NULL, infoPtr->channel, "-eofchar", "\032 {}");
    Tcl_SetChannelOption(NULL, infoPtr->channel, "-encoding", encoding);

    return infoPtr->channel;
}
