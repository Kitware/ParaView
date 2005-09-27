/*
 * tclWinSerial.c --
 *
 *  This file implements the Windows-specific serial port functions,
 *  and the "serial" channel driver.
 *
 * Copyright (c) 1999 by Scriptics Corp.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Serial functionality implemented by Rolf.Schroedter@dlr.de
 *
 * RCS: @(#) Id
 */

#include "tclWinInt.h"

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

/*
 * The following variable is used to tell whether this module has been
 * initialized.
 */

static int initialized = 0;

/*
 * The serialMutex locks around access to the initialized variable, and it is
 * used to protect background threads from being terminated while they are
 * using APIs that hold locks.
 */

TCL_DECLARE_MUTEX(serialMutex)

/*
 * Bit masks used in the flags field of the SerialInfo structure below.
 */

#define SERIAL_PENDING  (1<<0)  /* Message is pending in the queue. */
#define SERIAL_ASYNC    (1<<1)  /* Channel is non-blocking. */

/*
 * Bit masks used in the sharedFlags field of the SerialInfo structure below.
 */

#define SERIAL_EOF      (1<<2)  /* Serial has reached EOF. */
#define SERIAL_ERROR    (1<<4)

/*
 * Default time to block between checking status on the serial port.
 */
#define SERIAL_DEFAULT_BLOCKTIME    10  /* 10 msec */

/*
 * Define Win32 read/write error masks returned by ClearCommError()
 */
#define SERIAL_READ_ERRORS      ( CE_RXOVER | CE_OVERRUN | CE_RXPARITY \
                                | CE_FRAME  | CE_BREAK )
#define SERIAL_WRITE_ERRORS     ( CE_TXFULL | CE_PTO )

/*
 * This structure describes per-instance data for a serial based channel.
 */

typedef struct SerialInfo {
    HANDLE handle;
    struct SerialInfo *nextPtr; /* Pointer to next registered serial. */
    Tcl_Channel channel;        /* Pointer to channel structure. */
    int validMask;              /* OR'ed combination of TCL_READABLE,
                                 * TCL_WRITABLE, or TCL_EXCEPTION: indicates
                                 * which operations are valid on the file. */
    int watchMask;              /* OR'ed combination of TCL_READABLE,
                                 * TCL_WRITABLE, or TCL_EXCEPTION: indicates
                                 * which events should be reported. */
    int flags;                  /* State flags, see above for a list. */
    int readable;               /* flag that the channel is readable */
    int writable;               /* flag that the channel is writable */
    int blockTime;              /* max. blocktime in msec */
    unsigned int lastEventTime;	/* Time in milliseconds since last readable event */
				/* Next readable event only after blockTime */
    DWORD error;                /* pending error code returned by
                                 * ClearCommError() */
    DWORD lastError;            /* last error code, can be fetched with
                                 * fconfigure chan -lasterror */
    DWORD sysBufRead;           /* Win32 system buffer size for read ops, 
                                 * default=4096 */
    DWORD sysBufWrite;          /* Win32 system buffer size for write ops, 
                                 * default=4096 */

    Tcl_ThreadId threadId;      /* Thread to which events should be reported.
                                 * This value is used by the reader/writer
                                 * threads. */
    OVERLAPPED osRead;          /* OVERLAPPED structure for read operations */
    OVERLAPPED osWrite;         /* OVERLAPPED structure for write operations */
    HANDLE writeThread;         /* Handle to writer thread. */
    CRITICAL_SECTION csWrite;   /* Writer thread synchronisation */
    HANDLE evWritable;          /* Manual-reset event to signal when the
                                 * writer thread has finished waiting for
                                 * the current buffer to be written. */
    HANDLE evStartWriter;       /* Auto-reset event used by the main thread to
                                 * signal when the writer thread should attempt
                                 * to write to the serial. */
    HANDLE evStopWriter;	/* Auto-reset event used by the main thread to
                                 * signal when the writer thread should close.
				 */
    DWORD writeError;           /* An error caused by the last background
                                 * write.  Set to 0 if no error has been
                                 * detected.  This word is shared with the
                                 * writer thread so access must be
                                 * synchronized with the evWritable object.
                                 */
    char *writeBuf;             /* Current background output buffer.
                                 * Access is synchronized with the evWritable
                                 * object. */
    int writeBufLen;            /* Size of write buffer.  Access is
                                 * synchronized with the evWritable
                                 * object. */
    int toWrite;                /* Current amount to be written.  Access is
                                 * synchronized with the evWritable object. */
    int writeQueue;             /* Number of bytes pending in output queue.
                                 * Offset to DCB.cbInQue.
                                 * Used to query [fconfigure -queue] */
} SerialInfo;

typedef struct ThreadSpecificData {
    /*
     * The following pointer refers to the head of the list of serials
     * that are being watched for file events.
     */

    SerialInfo *firstSerialPtr;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * The following structure is what is added to the Tcl event queue when
 * serial events are generated.
 */

typedef struct SerialEvent {
    Tcl_Event header;       /* Information that is standard for
                             * all events. */
    SerialInfo *infoPtr;    /* Pointer to serial info structure.  Note
                             * that we still have to verify that the
                             * serial exists before dereferencing this
                             * pointer. */
} SerialEvent;

/*
 * We don't use timeouts.
 */

static COMMTIMEOUTS no_timeout = {
    0,               /* ReadIntervalTimeout */
    0,               /* ReadTotalTimeoutMultiplier */
    0,               /* ReadTotalTimeoutConstant */
    0,               /* WriteTotalTimeoutMultiplier */
    0,               /* WriteTotalTimeoutConstant */
};

/*
 * Declarations for functions used only in this file.
 */

static int      SerialBlockProc(ClientData instanceData, int mode);
static void     SerialCheckProc(ClientData clientData, int flags);
static int      SerialCloseProc(ClientData instanceData,
                Tcl_Interp *interp);
static int      SerialEventProc(Tcl_Event *evPtr, int flags);
static void     SerialExitHandler(ClientData clientData);
static int      SerialGetHandleProc(ClientData instanceData,
                int direction, ClientData *handlePtr);
static ThreadSpecificData *SerialInit(void);
static int      SerialInputProc(ClientData instanceData, char *buf,
                int toRead, int *errorCode);
static int      SerialOutputProc(ClientData instanceData, CONST char *buf,
                int toWrite, int *errorCode);
static void     SerialSetupProc(ClientData clientData, int flags);
static void     SerialWatchProc(ClientData instanceData, int mask);
static void     ProcExitHandler(ClientData clientData);
static int       SerialGetOptionProc _ANSI_ARGS_((ClientData instanceData,
                Tcl_Interp *interp, CONST char *optionName,
                Tcl_DString *dsPtr));
static int       SerialSetOptionProc _ANSI_ARGS_((ClientData instanceData,
                Tcl_Interp *interp, CONST char *optionName,
                CONST char *value));
static DWORD WINAPI     SerialWriterThread(LPVOID arg);

/*
 * This structure describes the channel type structure for command serial
 * based IO.
 */

static Tcl_ChannelType serialChannelType = {
    "serial",                   /* Type name. */
    TCL_CHANNEL_VERSION_2,      /* v2 channel */
    SerialCloseProc,            /* Close proc. */
    SerialInputProc,            /* Input proc. */
    SerialOutputProc,           /* Output proc. */
    NULL,                       /* Seek proc. */
    SerialSetOptionProc,        /* Set option proc. */
    SerialGetOptionProc,        /* Get option proc. */
    SerialWatchProc,            /* Set up notifier to watch the channel. */
    SerialGetHandleProc,        /* Get an OS handle from channel. */
    NULL,                       /* close2proc. */
    SerialBlockProc,            /* Set blocking or non-blocking mode.*/
    NULL,                       /* flush proc. */
    NULL,                       /* handler proc. */
};

/*
 *----------------------------------------------------------------------
 *
 * SerialInit --
 *
 *  This function initializes the static variables for this file.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Creates a new event source.
 *
 *----------------------------------------------------------------------
 */

static ThreadSpecificData *
SerialInit()
{
    ThreadSpecificData *tsdPtr;

    /*
     * Check the initialized flag first, then check it again in the mutex.
     * This is a speed enhancement.
     */

    if (!initialized) {
        Tcl_MutexLock(&serialMutex);
        if (!initialized) {
            initialized = 1;
            Tcl_CreateExitHandler(ProcExitHandler, NULL);
        }
        Tcl_MutexUnlock(&serialMutex);
    }

    tsdPtr = (ThreadSpecificData *)TclThreadDataKeyGet(&dataKey);
    if (tsdPtr == NULL) {
        tsdPtr = TCL_TSD_INIT(&dataKey);
        tsdPtr->firstSerialPtr = NULL;
        Tcl_CreateEventSource(SerialSetupProc, SerialCheckProc, NULL);
        Tcl_CreateThreadExitHandler(SerialExitHandler, NULL);
    }
    return tsdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * SerialExitHandler --
 *
 *  This function is called to cleanup the serial module before
 *  Tcl is unloaded.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Removes the serial event source.
 *
 *----------------------------------------------------------------------
 */

static void
SerialExitHandler(
    ClientData clientData)  /* Old window proc */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    SerialInfo *infoPtr;

    /*
     * Clear all eventually pending output.
     * Otherwise Tcl's exit could totally block,
     * because it performs a blocking flush on all open channels.
     * Note that serial write operations may be blocked due to handshake.
     */
    for (infoPtr = tsdPtr->firstSerialPtr; infoPtr != NULL;
            infoPtr = infoPtr->nextPtr) {
        PurgeComm(infoPtr->handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR 
            | PURGE_RXCLEAR);

    }
    Tcl_DeleteEventSource(SerialSetupProc, SerialCheckProc, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * ProcExitHandler --
 *
 *  This function is called to cleanup the process list before
 *  Tcl is unloaded.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Resets the process list.
 *
 *----------------------------------------------------------------------
 */

static void
ProcExitHandler(
    ClientData clientData)  /* Old window proc */
{
    Tcl_MutexLock(&serialMutex);
    initialized = 0;
    Tcl_MutexUnlock(&serialMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * SerialBlockTime --
 *
 *  Wrapper to set Tcl's block time in msec
 *
 * Results:
 *  None.
 *----------------------------------------------------------------------
 */

static void
SerialBlockTime(
    int msec)          /* milli-seconds */
{
    Tcl_Time blockTime;

    blockTime.sec  =  msec / 1000;
    blockTime.usec = (msec % 1000) * 1000;
    Tcl_SetMaxBlockTime(&blockTime);
}
/*
 *----------------------------------------------------------------------
 *
 * SerialGetMilliseconds --
 *
 *  Get current time in milliseconds,
 *  Don't care about integer overruns
 *
 * Results:
 *  None.
 *----------------------------------------------------------------------
 */

static unsigned int
SerialGetMilliseconds(
    void)
{
    Tcl_Time time;

    TclpGetTime(&time);

    return (time.sec * 1000 + time.usec / 1000);
}
/*
 *----------------------------------------------------------------------
 *
 * SerialSetupProc --
 *
 *  This procedure is invoked before Tcl_DoOneEvent blocks waiting
 *  for an event.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Adjusts the block time if needed.
 *
 *----------------------------------------------------------------------
 */

void
SerialSetupProc(
    ClientData data,    /* Not used. */
    int flags)          /* Event flags as passed to Tcl_DoOneEvent. */
{
    SerialInfo *infoPtr;
    int block = 1;
    int msec = INT_MAX; /* min. found block time */
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
        return;
    }

    /*
     * Look to see if any events handlers installed. If they are, do not block.
     */

    for (infoPtr = tsdPtr->firstSerialPtr; infoPtr != NULL;
            infoPtr = infoPtr->nextPtr) {

        if (infoPtr->watchMask & TCL_WRITABLE) {
            if (WaitForSingleObject(infoPtr->evWritable, 0) != WAIT_TIMEOUT) {
                block = 0;
                msec = min( msec, infoPtr->blockTime );
            }
        }
        if( infoPtr->watchMask & TCL_READABLE ) {
            block = 0;
            msec = min( msec, infoPtr->blockTime );
        }
    }

    if (!block) {
        SerialBlockTime(msec);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SerialCheckProc --
 *
 *  This procedure is called by Tcl_DoOneEvent to check the serial
 *  event source for events.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  May queue an event.
 *
 *----------------------------------------------------------------------
 */

static void
SerialCheckProc(
    ClientData data,    /* Not used. */
    int flags)          /* Event flags as passed to Tcl_DoOneEvent. */
{
    SerialInfo *infoPtr;
    SerialEvent *evPtr;
    int needEvent;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    COMSTAT cStat;
    unsigned int time;

    if (!(flags & TCL_FILE_EVENTS)) {
        return;
    }

    /*
     * Queue events for any ready serials that don't already have events
     * queued.
     */

    for (infoPtr = tsdPtr->firstSerialPtr; infoPtr != NULL;
            infoPtr = infoPtr->nextPtr) {
        if (infoPtr->flags & SERIAL_PENDING) {
            continue;
        }

        needEvent = 0;

        /*
         * If WRITABLE watch mask is set
         * look for infoPtr->evWritable object
         */
        if (infoPtr->watchMask & TCL_WRITABLE) {
            if (WaitForSingleObject(infoPtr->evWritable, 0) != WAIT_TIMEOUT) {
                infoPtr->writable = 1;
                needEvent = 1;
            }
        }
        
        /*
         * If READABLE watch mask is set
         * call ClearCommError to poll cbInQue
         * Window errors are ignored here
         */

        if( infoPtr->watchMask & TCL_READABLE ) {
            if( ClearCommError( infoPtr->handle, &infoPtr->error, &cStat ) ) {
                /*
                 * Look for characters already pending in windows queue.
                 * If they are, poll.
                 */

                if( infoPtr->watchMask & TCL_READABLE ) {
                    /*
                     * force fileevent after serial read error
                     */
                    if( (cStat.cbInQue > 0) ||
                            (infoPtr->error & SERIAL_READ_ERRORS) ) {
                        infoPtr->readable = 1;
			time = SerialGetMilliseconds();
			if ((unsigned int) (time - infoPtr->lastEventTime)
				>= (unsigned int) infoPtr->blockTime) {
			    needEvent = 1;
			    infoPtr->lastEventTime = time;
			}
                    }
                }
            }
        }

        /*
         * Queue an event if the serial is signaled for reading or writing.
         */
        if (needEvent) {
            infoPtr->flags |= SERIAL_PENDING;
            evPtr = (SerialEvent *) ckalloc(sizeof(SerialEvent));
            evPtr->header.proc = SerialEventProc;
            evPtr->infoPtr = infoPtr;
            Tcl_QueueEvent((Tcl_Event *) evPtr, TCL_QUEUE_TAIL);
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SerialBlockProc --
 *
 *  Set blocking or non-blocking mode on channel.
 *
 * Results:
 *  0 if successful, errno when failed.
 *
 * Side effects:
 *  Sets the device into blocking or non-blocking mode.
 *
 *----------------------------------------------------------------------
 */

static int
SerialBlockProc(
    ClientData instanceData,    /* Instance data for channel. */
    int mode)                   /* TCL_MODE_BLOCKING or
                                 * TCL_MODE_NONBLOCKING. */
{
    int errorCode = 0;

    SerialInfo *infoPtr = (SerialInfo *) instanceData;

    /*
     * Only serial READ can be switched between blocking & nonblocking
     * using COMMTIMEOUTS.
     * Serial write emulates blocking & nonblocking by the SerialWriterThread.
     */

    if (mode == TCL_MODE_NONBLOCKING) {
        infoPtr->flags |= SERIAL_ASYNC;
    } else {
        infoPtr->flags &= ~(SERIAL_ASYNC);
    }
    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * SerialCloseProc --
 *
 *  Closes a serial based IO channel.
 *
 * Results:
 *  0 on success, errno otherwise.
 *
 * Side effects:
 *  Closes the physical channel.
 *
 *----------------------------------------------------------------------
 */

static int
SerialCloseProc(
    ClientData instanceData,    /* Pointer to SerialInfo structure. */
    Tcl_Interp *interp)         /* For error reporting. */
{
    SerialInfo *serialPtr = (SerialInfo *) instanceData;
    int errorCode, result = 0;
    SerialInfo *infoPtr, **nextPtrPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    DWORD exitCode;

    errorCode = 0;

    if (serialPtr->validMask & TCL_READABLE) {
        PurgeComm(serialPtr->handle, PURGE_RXABORT | PURGE_RXCLEAR);
        CloseHandle(serialPtr->osRead.hEvent);
    }
    serialPtr->validMask &= ~TCL_READABLE;
 
    if (serialPtr->validMask & TCL_WRITABLE) {

        /*
         * Generally we cannot wait for a pending write operation
         * because it may hang due to handshake
         *    WaitForSingleObject(serialPtr->evWritable, INFINITE);
         */

	/*
	 * The thread may have already closed on it's own.  Check it's
	 * exit code.
	 */

	GetExitCodeThread(serialPtr->writeThread, &exitCode);

	if (exitCode == STILL_ACTIVE) {
	    /*
	     * Set the stop event so that if the writer thread is
	     * blocked in SerialWriterThread on WaitForMultipleEvents, it
	     * will exit cleanly.
	     */

	    SetEvent(serialPtr->evStopWriter);

	    /*
	     * Wait at most 20 milliseconds for the writer thread to
	     * close.
	     */

	    if (WaitForSingleObject(serialPtr->writeThread, 20)
		    == WAIT_TIMEOUT) {
		/*
		 * Forcibly terminate the background thread as a last
		 * resort.  Note that we need to guard against
		 * terminating the thread while it is in the middle of
		 * Tcl_ThreadAlert because it won't be able to release
		 * the notifier lock.
		 */

		Tcl_MutexLock(&serialMutex);

		/* BUG: this leaks memory */
		TerminateThread(serialPtr->writeThread, 0);

		Tcl_MutexUnlock(&serialMutex);
	    }
	}

        CloseHandle(serialPtr->writeThread);
	CloseHandle(serialPtr->osWrite.hEvent);
	DeleteCriticalSection(&serialPtr->csWrite);
        CloseHandle(serialPtr->evWritable);
        CloseHandle(serialPtr->evStartWriter);
        CloseHandle(serialPtr->evStopWriter);
        serialPtr->writeThread = NULL;

        PurgeComm(serialPtr->handle, PURGE_TXABORT | PURGE_TXCLEAR);
    }
    serialPtr->validMask &= ~TCL_WRITABLE;

    /*
     * Don't close the Win32 handle if the handle is a standard channel
     * during the thread exit process.  Otherwise, one thread may kill
     * the stdio of another.
     */

    if (!TclInThreadExit()
	|| ((GetStdHandle(STD_INPUT_HANDLE) != serialPtr->handle)
	&& (GetStdHandle(STD_OUTPUT_HANDLE) != serialPtr->handle)
	&& (GetStdHandle(STD_ERROR_HANDLE) != serialPtr->handle))) {
	    if (CloseHandle(serialPtr->handle) == FALSE) {
		TclWinConvertError(GetLastError());
		errorCode = errno;
	    }
    }

    serialPtr->watchMask &= serialPtr->validMask;

    /*
     * Remove the file from the list of watched files.
     */

    for (nextPtrPtr = &(tsdPtr->firstSerialPtr), infoPtr = *nextPtrPtr;
	    infoPtr != NULL;
	    nextPtrPtr = &infoPtr->nextPtr, infoPtr = *nextPtrPtr) {
        if (infoPtr == (SerialInfo *)serialPtr) {
            *nextPtrPtr = infoPtr->nextPtr;
            break;
        }
    }

    /*
     * Wrap the error file into a channel and give it to the cleanup
     * routine.
     */
    if (serialPtr->writeBuf != NULL) {
        ckfree(serialPtr->writeBuf);
        serialPtr->writeBuf = NULL;
    }
    ckfree((char*) serialPtr);

    if (errorCode == 0) {
        return result;
    }
    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * blockingRead --
 *
 *  Perform a blocking read into the buffer given. Returns
 *  count of how many bytes were actually read, and an error indication.
 *
 * Results:
 *  A count of how many bytes were read is returned and an error
 *  indication is returned.
 *
 * Side effects:
 *  Reads input from the actual channel.
 *
 *----------------------------------------------------------------------
 */
static int
blockingRead( 
    SerialInfo *infoPtr,    /* Serial info structure */
    LPVOID buf,             /* The input buffer pointer */
    DWORD  bufSize,         /* The number of bytes to read */
    LPDWORD  lpRead,        /* Returns number of bytes read */ 
    LPOVERLAPPED osPtr )    /* OVERLAPPED structure */
{
    /*
     *  Perform overlapped blocking read. 
     *  1. Reset the overlapped event
     *  2. Start overlapped read operation
     *  3. Wait for completion
     */

    /* 
     * Set Offset to ZERO, otherwise NT4.0 may report an error.
     */
    osPtr->Offset = osPtr->OffsetHigh = 0;
    ResetEvent(osPtr->hEvent);
    if (! ReadFile(infoPtr->handle, buf, bufSize, lpRead, osPtr) ) {
        if (GetLastError() != ERROR_IO_PENDING) {
            /* ReadFile failed, but it isn't delayed. Report error. */
            return FALSE;
        } else {   
            /* Read is pending, wait for completion, timeout ? */
            if (! GetOverlappedResult(infoPtr->handle, osPtr, lpRead, TRUE) ) {
                return FALSE;
            }
        }
    } else {
        /* ReadFile completed immediately. */
    }
    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * blockingWrite --
 *
 *  Perform a blocking write from the buffer given. Returns
 *  count of how many bytes were actually written, and an error indication.
 *
 * Results:
 *  A count of how many bytes were written is returned and an error
 *  indication is returned.
 *
 * Side effects:
 *  Writes output to the actual channel.
 *
 *----------------------------------------------------------------------
 */
static int
blockingWrite(
    SerialInfo *infoPtr,    /* Serial info structure */
    LPVOID  buf,            /* The output buffer pointer */
    DWORD   bufSize,        /* The number of bytes to write */
    LPDWORD lpWritten,      /* Returns number of bytes written */ 
    LPOVERLAPPED osPtr )    /* OVERLAPPED structure */
{
    int result;
    /*
    *  Perform overlapped blocking write. 
    *  1. Reset the overlapped event
    *  2. Remove these bytes from the output queue counter
    *  3. Start overlapped write operation
    *  3. Remove these bytes from the output queue counter
    *  4. Wait for completion
    *  5. Adjust the output queue counter
    */
    ResetEvent(osPtr->hEvent);

    EnterCriticalSection(&infoPtr->csWrite);
    infoPtr->writeQueue -= bufSize;
	/* 
	* Set Offset to ZERO, otherwise NT4.0 may report an error 
	*/
	osPtr->Offset = osPtr->OffsetHigh = 0;
    result = WriteFile(infoPtr->handle, buf, bufSize, lpWritten, osPtr);
    LeaveCriticalSection(&infoPtr->csWrite);

    if (result == FALSE ) {
        int err = GetLastError();
        switch (err) {
        case ERROR_IO_PENDING:
            /* Write is pending, wait for completion */
            if (! GetOverlappedResult(infoPtr->handle, osPtr, lpWritten, TRUE) ) {
                return FALSE;
            }
            break;
        case ERROR_COUNTER_TIMEOUT:
            /* Write timeout handled in SerialOutputProc */
            break;
        default:
            /* WriteFile failed, but it isn't delayed. Report error */
            return FALSE;
        }
    } else {
        /* WriteFile completed immediately. */
    }

    EnterCriticalSection(&infoPtr->csWrite);
    infoPtr->writeQueue += (*lpWritten - bufSize);
    LeaveCriticalSection(&infoPtr->csWrite);

    return TRUE;
}

/*
 *----------------------------------------------------------------------
 *
 * SerialInputProc --
 *
 *  Reads input from the IO channel into the buffer given. Returns
 *  count of how many bytes were actually read, and an error indication.
 *
 * Results:
 *  A count of how many bytes were read is returned and an error
 *  indication is returned in an output argument.
 *
 * Side effects:
 *  Reads input from the actual channel.
 *
 *----------------------------------------------------------------------
 */
static int
SerialInputProc(
    ClientData instanceData,    /* Serial state. */
    char *buf,                  /* Where to store data read. */
    int bufSize,                /* How much space is available
                                 * in the buffer? */
    int *errorCode)             /* Where to store error code. */
{
    SerialInfo *infoPtr = (SerialInfo *) instanceData;
    DWORD bytesRead = 0;
    COMSTAT cStat;

    *errorCode = 0;

    /*
     * Check if there is a CommError pending from SerialCheckProc
     */
    if( infoPtr->error & SERIAL_READ_ERRORS ){
        goto commError;
    }

    /*
     * Look for characters already pending in windows queue.
     * This is the mainly restored good old code from Tcl8.0
     */

    if( ClearCommError( infoPtr->handle, &infoPtr->error, &cStat ) ) {
        /*
         * Check for errors here, but not in the evSetup/Check procedures
         */

        if( infoPtr->error & SERIAL_READ_ERRORS ) {
            goto commError;
        }
        if( infoPtr->flags & SERIAL_ASYNC ) {
            /*
             * NON_BLOCKING mode:
             * Avoid blocking by reading more bytes than available
             * in input buffer
             */

            if( cStat.cbInQue > 0 ) {
                if( (DWORD) bufSize > cStat.cbInQue ) {
                    bufSize = cStat.cbInQue;
                }
            } else {
                errno = *errorCode = EAGAIN;
                return -1;
            }
        } else {
            /*
             * BLOCKING mode:
             * Tcl trys to read a full buffer of 4 kBytes here
             */

            if( cStat.cbInQue > 0 ) {
                if( (DWORD) bufSize > cStat.cbInQue ) {
                    bufSize = cStat.cbInQue;
                }
            } else {
                bufSize = 1;
            }
        }
    }

    if( bufSize == 0 ) {
        return bytesRead = 0;
    }

    /*
    *  Perform blocking read. Doesn't block in non-blocking mode, 
    *  because we checked the number of available bytes.
    */
    if (blockingRead(infoPtr, (LPVOID) buf, (DWORD) bufSize, &bytesRead,
            &infoPtr->osRead) == FALSE) {
        goto error;
    }
    return bytesRead;

error:
    TclWinConvertError(GetLastError());
    *errorCode = errno;
    return -1;

commError:
    infoPtr->lastError = infoPtr->error;  /* save last error code */
    infoPtr->error = 0;                   /* reset error code */
    *errorCode = EIO;                     /* to return read-error only once */
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * SerialOutputProc --
 *
 *  Writes the given output on the IO channel. Returns count of how
 *  many characters were actually written, and an error indication.
 *
 * Results:
 *  A count of how many characters were written is returned and an
 *  error indication is returned in an output argument.
 *
 * Side effects:
 *  Writes output on the actual channel.
 *
 *----------------------------------------------------------------------
 */

static int
SerialOutputProc(
    ClientData instanceData,    /* Serial state. */
    CONST char *buf,            /* The data buffer. */
    int toWrite,                /* How many bytes to write? */
    int *errorCode)             /* Where to store error code. */
{
    SerialInfo *infoPtr = (SerialInfo *) instanceData;
    DWORD bytesWritten, timeout;

    *errorCode = 0;

    /*
     * At EXIT Tcl trys to flush all open channels in blocking mode.
     * We avoid blocking output after ExitProc or CloseHandler(chan)
     * has been called by checking the corrresponding variables.
     */
    if( ! initialized || TclInExit() ) {
        return toWrite;
    }

    /*
     * Check if there is a CommError pending from SerialCheckProc
     */
    if( infoPtr->error & SERIAL_WRITE_ERRORS ){
        infoPtr->lastError = infoPtr->error;  /* save last error code */
        infoPtr->error = 0;                   /* reset error code */
        errno = EIO;            
        goto error;
    }

    timeout = (infoPtr->flags & SERIAL_ASYNC) ? 0 : INFINITE;
    if (WaitForSingleObject(infoPtr->evWritable, timeout) == WAIT_TIMEOUT) {
        /*
         * The writer thread is blocked waiting for a write to complete
         * and the channel is in non-blocking mode.
         */

        errno = EWOULDBLOCK;
        goto error1;
    }
    /*
     * Check for a background error on the last write.
     */

    if (infoPtr->writeError) {
        TclWinConvertError(infoPtr->writeError);
        infoPtr->writeError = 0;
        goto error1;
    }

    /*
     * Remember the number of bytes in output queue
     */
    EnterCriticalSection(&infoPtr->csWrite);
    infoPtr->writeQueue += toWrite;
    LeaveCriticalSection(&infoPtr->csWrite);

    if (infoPtr->flags & SERIAL_ASYNC) {
        /*
         * The serial is non-blocking, so copy the data into the output
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
            infoPtr->writeBuf = ckalloc((unsigned int) toWrite);
        }
        memcpy(infoPtr->writeBuf, buf, (size_t) toWrite);
        infoPtr->toWrite = toWrite;
        ResetEvent(infoPtr->evWritable);
        SetEvent(infoPtr->evStartWriter);
        bytesWritten = (DWORD) toWrite;

    } else {
        /*
        * In the blocking case, just try to write the buffer directly.
        * This avoids an unnecessary copy.
        */
        if (! blockingWrite(infoPtr, (LPVOID) buf, (DWORD) toWrite,
                &bytesWritten, &infoPtr->osWrite) ) {
            goto writeError;
        }
        if (bytesWritten != (DWORD) toWrite) {
            /* Write timeout */
            infoPtr->lastError |= CE_PTO;
            errno = EIO;
            goto error;
        }
    }

    return (int) bytesWritten;

writeError:
    TclWinConvertError(GetLastError());

error:
    /* 
     * Reset the output queue counter on error during blocking output 
     */
/*
    EnterCriticalSection(&infoPtr->csWrite);
    infoPtr->writeQueue = 0;
    LeaveCriticalSection(&infoPtr->csWrite);
*/
  error1: 
    *errorCode = errno;
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * SerialEventProc --
 *
 *  This function is invoked by Tcl_ServiceEvent when a file event
 *  reaches the front of the event queue.  This procedure invokes
 *  Tcl_NotifyChannel on the serial.
 *
 * Results:
 *  Returns 1 if the event was handled, meaning it should be removed
 *  from the queue.  Returns 0 if the event was not handled, meaning
 *  it should stay on the queue.  The only time the event isn't
 *  handled is if the TCL_FILE_EVENTS flag bit isn't set.
 *
 * Side effects:
 *  Whatever the notifier callback does.
 *
 *----------------------------------------------------------------------
 */

static int
SerialEventProc(
    Tcl_Event *evPtr,   /* Event to service. */
    int flags)          /* Flags that indicate what events to
                         * handle, such as TCL_FILE_EVENTS. */
{
    SerialEvent *serialEvPtr = (SerialEvent *)evPtr;
    SerialInfo *infoPtr;
    int mask;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
        return 0;
    }

    /*
     * Search through the list of watched serials for the one whose handle
     * matches the event.  We do this rather than simply dereferencing
     * the handle in the event so that serials can be deleted while the
     * event is in the queue.
     */

    for (infoPtr = tsdPtr->firstSerialPtr; infoPtr != NULL;
            infoPtr = infoPtr->nextPtr) {
        if (serialEvPtr->infoPtr == infoPtr) {
            infoPtr->flags &= ~(SERIAL_PENDING);
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
     * Check to see if the serial is readable.  Note
     * that we can't tell if a serial is writable, so we always report it
     * as being writable unless we have detected EOF.
     */

    mask = 0;
    if( infoPtr->watchMask & TCL_WRITABLE ) {
        if( infoPtr->writable ) {
            mask |= TCL_WRITABLE;
            infoPtr->writable = 0;
        }
    }

    if( infoPtr->watchMask & TCL_READABLE ) {
        if( infoPtr->readable ) {
            mask |= TCL_READABLE;
            infoPtr->readable = 0;
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
 * SerialWatchProc --
 *
 *  Called by the notifier to set up to watch for events on this
 *  channel.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static void
SerialWatchProc(
    ClientData instanceData,     /* Serial state. */
    int mask)                    /* What events to watch for, OR-ed
                                  * combination of TCL_READABLE,
                                  * TCL_WRITABLE and TCL_EXCEPTION. */
{
    SerialInfo **nextPtrPtr, *ptr;
    SerialInfo *infoPtr = (SerialInfo *) instanceData;
    int oldMask = infoPtr->watchMask;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * Since the file is always ready for events, we set the block time
     * so we will poll.
     */

    infoPtr->watchMask = mask & infoPtr->validMask;
    if (infoPtr->watchMask) {
        if (!oldMask) {
            infoPtr->nextPtr = tsdPtr->firstSerialPtr;
            tsdPtr->firstSerialPtr = infoPtr;
        }
        SerialBlockTime(infoPtr->blockTime);
    } else {
        if (oldMask) {
            /*
             * Remove the serial port from the list of watched serial ports.
             */

            for (nextPtrPtr = &(tsdPtr->firstSerialPtr), ptr = *nextPtrPtr;
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
 * SerialGetHandleProc --
 *
 *  Called from Tcl_GetChannelHandle to retrieve OS handles from
 *  inside a command serial port based channel.
 *
 * Results:
 *  Returns TCL_OK with the fd in handlePtr, or TCL_ERROR if
 *  there is no handle for the specified direction.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static int
SerialGetHandleProc(
    ClientData instanceData,    /* The serial state. */
    int direction,              /* TCL_READABLE or TCL_WRITABLE */
    ClientData *handlePtr)      /* Where to store the handle.  */
{
    SerialInfo *infoPtr = (SerialInfo *) instanceData;

    *handlePtr = (ClientData) infoPtr->handle;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SerialWriterThread --
 *
 *      This function runs in a separate thread and writes data
 *      onto a serial.
 *
 * Results:
 *      Always returns 0.
 *
 * Side effects:
 *      Signals the main thread when an output operation is completed.
 *      May cause the main thread to wake up by posting a message.  
 *
 *----------------------------------------------------------------------
 */

static DWORD WINAPI
SerialWriterThread(LPVOID arg)
{

    SerialInfo *infoPtr = (SerialInfo *)arg;
    DWORD bytesWritten, toWrite, waitResult;
    char *buf;
    OVERLAPPED myWrite; /* have an own OVERLAPPED in this thread */
    HANDLE wEvents[2];

    /*
     * The stop event takes precedence by being first in the list.
     */
    wEvents[0] = infoPtr->evStopWriter;
    wEvents[1] = infoPtr->evStartWriter;

    for (;;) {
        /*
         * Wait for the main thread to signal before attempting to write.
         */

	waitResult = WaitForMultipleObjects(2, wEvents, FALSE, INFINITE);

	if (waitResult != (WAIT_OBJECT_0 + 1)) {
	    /*
	     * The start event was not signaled.  It might be the stop event
	     * or an error, so exit.
	     */

	    break;
	}

        buf = infoPtr->writeBuf;
        toWrite = infoPtr->toWrite;

        myWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        /*
         * Loop until all of the bytes are written or an error occurs.
         */

        while (toWrite > 0) {
            /*
            *  Check for pending writeError
            *  Ignore all write operations until the user has been notified
            */
            if (infoPtr->writeError) {
                break;
            }
            if (blockingWrite(infoPtr, (LPVOID) buf, (DWORD) toWrite, 
                    &bytesWritten, &myWrite) == FALSE) {
                infoPtr->writeError = GetLastError();
                break;
            }
            if (bytesWritten != toWrite) {
                /* Write timeout */
                infoPtr->writeError = ERROR_WRITE_FAULT;
                break;
            }
            toWrite -= bytesWritten;
            buf += bytesWritten;
        }

        CloseHandle(myWrite.hEvent);
        /*
         * Signal the main thread by signalling the evWritable event and
         * then waking up the notifier thread.
         */
        SetEvent(infoPtr->evWritable);

        /*
         * Alert the foreground thread.  Note that we need to treat this like
         * a critical section so the foreground thread does not terminate
         * this thread while we are holding a mutex in the notifier code.
         */

        Tcl_MutexLock(&serialMutex);
        Tcl_ThreadAlert(infoPtr->threadId);
        Tcl_MutexUnlock(&serialMutex);
    }

    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * TclWinSerialReopen --
 *
 *  Reopens the serial port with the OVERLAPPED FLAG set
 *
 * Results:
 *  Returns the new handle, or INVALID_HANDLE_VALUE
 *  Normally there shouldn't be any error, 
 *  because the same channel has previously been succeesfully opened.
 *
 * Side effects:
 *  May close the original handle
 *
 *----------------------------------------------------------------------
 */

HANDLE
TclWinSerialReopen(handle, name, access)
    HANDLE handle;
    CONST TCHAR *name;
    DWORD access;
{
    ThreadSpecificData *tsdPtr;

    tsdPtr = SerialInit();

    /* 
    * Multithreaded I/O needs the overlapped flag set
    * otherwise ClearCommError blocks under Windows NT/2000 until serial
    * output is finished
    */
    if (CloseHandle(handle) == FALSE) {
        return INVALID_HANDLE_VALUE;
    }
    handle = (*tclWinProcs->createFileProc)(name, access, 
                0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    return handle;
}
/*
 *----------------------------------------------------------------------
 *
 * TclWinOpenSerialChannel --
 *
 *  Constructs a Serial port channel for the specified standard OS handle.
 *      This is a helper function to break up the construction of
 *      channels into File, Console, or Serial.
 *
 * Results:
 *  Returns the new channel, or NULL.
 *
 * Side effects:
 *  May open the channel
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
TclWinOpenSerialChannel(handle, channelName, permissions)
    HANDLE handle;
    char *channelName;
    int permissions;
{
    SerialInfo *infoPtr;
    ThreadSpecificData *tsdPtr;
    DWORD id;

    tsdPtr = SerialInit();

    infoPtr = (SerialInfo *) ckalloc((unsigned) sizeof(SerialInfo));
    memset(infoPtr, 0, sizeof(SerialInfo));

    infoPtr->validMask = permissions;
    infoPtr->handle = handle;

    /*
     * Use the pointer to keep the channel names unique, in case
     * the handles are shared between multiple channels (stdin/stdout).
     */

    wsprintfA(channelName, "file%lx", (int) infoPtr);

    infoPtr->channel = Tcl_CreateChannel(&serialChannelType, channelName,
            (ClientData) infoPtr, permissions);

    infoPtr->readable = 0; 
    infoPtr->writable = 1;
    infoPtr->toWrite = infoPtr->writeQueue = 0;
    infoPtr->blockTime = SERIAL_DEFAULT_BLOCKTIME;
    infoPtr->lastEventTime = 0;
    infoPtr->lastError = infoPtr->error = 0;
    infoPtr->threadId = Tcl_GetCurrentThread();
    infoPtr->sysBufRead = infoPtr->sysBufWrite = 4096;

    SetupComm(handle, infoPtr->sysBufRead, infoPtr->sysBufWrite);
    PurgeComm(handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR 
            | PURGE_RXCLEAR);

    /*
     * default is blocking
     */
    SetCommTimeouts(handle, &no_timeout);


    if (permissions & TCL_READABLE) {
        infoPtr->osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
    if (permissions & TCL_WRITABLE) {
        /* 
        * Initially the channel is writable
        * and the writeThread is idle.
        */ 
        infoPtr->osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        infoPtr->evWritable = CreateEvent(NULL, TRUE, TRUE, NULL);
        infoPtr->evStartWriter = CreateEvent(NULL, FALSE, FALSE, NULL);
	infoPtr->evStopWriter = CreateEvent(NULL, FALSE, FALSE, NULL);
        InitializeCriticalSection(&infoPtr->csWrite);
        infoPtr->writeThread = CreateThread(NULL, 256, SerialWriterThread,
            infoPtr, 0, &id);
    }

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
 * SerialErrorStr --
 *
 *  Converts a Win32 serial error code to a list of readable errors
 *
 *----------------------------------------------------------------------
 */
static void
SerialErrorStr(error, dsPtr)
    DWORD error;           /* Win32 serial error code */
    Tcl_DString *dsPtr;    /* Where to store string */
{
    if( (error & CE_RXOVER) != 0) {
                Tcl_DStringAppendElement(dsPtr, "RXOVER");
    }
    if( (error & CE_OVERRUN) != 0) {
                Tcl_DStringAppendElement(dsPtr, "OVERRUN");
    }
    if( (error & CE_RXPARITY) != 0) {
                Tcl_DStringAppendElement(dsPtr, "RXPARITY");
    }
    if( (error & CE_FRAME) != 0) {
                Tcl_DStringAppendElement(dsPtr, "FRAME");
    }
    if( (error & CE_BREAK) != 0) {
                Tcl_DStringAppendElement(dsPtr, "BREAK");
    }
    if( (error & CE_TXFULL) != 0) {
                Tcl_DStringAppendElement(dsPtr, "TXFULL");
    }
    if( (error & CE_PTO) != 0) {    /* PTO used to signal WRITE-TIMEOUT */
                Tcl_DStringAppendElement(dsPtr, "TIMEOUT");
    }
    if( (error & ~((DWORD) (SERIAL_READ_ERRORS | SERIAL_WRITE_ERRORS))) != 0) {
                char buf[TCL_INTEGER_SPACE + 1];
                wsprintfA(buf, "%d", error);
                Tcl_DStringAppendElement(dsPtr, buf);
    }
}
/*
 *----------------------------------------------------------------------
 *
 * SerialModemStatusStr --
 *
 *  Converts a Win32 modem status list of readable flags
 *
 *----------------------------------------------------------------------
 */
static void
SerialModemStatusStr(status, dsPtr)
    DWORD status;          /* Win32 modem status */
    Tcl_DString *dsPtr;    /* Where to store string */
{
    Tcl_DStringAppendElement(dsPtr, "CTS");
    Tcl_DStringAppendElement(dsPtr, (status & MS_CTS_ON)  ?  "1" : "0");
    Tcl_DStringAppendElement(dsPtr, "DSR");
    Tcl_DStringAppendElement(dsPtr, (status & MS_DSR_ON)   ? "1" : "0");
    Tcl_DStringAppendElement(dsPtr, "RING");
    Tcl_DStringAppendElement(dsPtr, (status & MS_RING_ON)  ? "1" : "0");
    Tcl_DStringAppendElement(dsPtr, "DCD");
    Tcl_DStringAppendElement(dsPtr, (status & MS_RLSD_ON)  ? "1" : "0");
}

/*
 *----------------------------------------------------------------------
 *
 * SerialSetOptionProc --
 *
 *  Sets an option on a channel.
 *
 * Results:
 *  A standard Tcl result. Also sets the interp's result on error if
 *  interp is not NULL.
 *
 * Side effects:
 *  May modify an option on a device.
 *
 *----------------------------------------------------------------------
 */
static int
SerialSetOptionProc(instanceData, interp, optionName, value)
    ClientData instanceData;    /* File state. */
    Tcl_Interp *interp;         /* For error reporting - can be NULL. */
    CONST char *optionName;     /* Which option to set? */
    CONST char *value;          /* New value for option. */
{
    SerialInfo *infoPtr;
    DCB dcb;
    BOOL result, flag;
    size_t len, vlen;
    Tcl_DString ds;
    CONST TCHAR *native;
    int argc;
    CONST char **argv;
    
    infoPtr = (SerialInfo *) instanceData;
    
    /* 
    * Parse options
    */
    len = strlen(optionName);
    vlen = strlen(value);

    /* 
    * Option -mode baud,parity,databits,stopbits
    */
    if ((len > 2) && (strncmp(optionName, "-mode", len) == 0)) {
        
        if (! GetCommState(infoPtr->handle, &dcb)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't get comm state", (char *) NULL);
            }
            return TCL_ERROR;
        }
        native = Tcl_WinUtfToTChar(value, -1, &ds);
        result = (*tclWinProcs->buildCommDCBProc)(native, &dcb);
        Tcl_DStringFree(&ds);
        
        if (result == FALSE) {
            if (interp) {
                Tcl_AppendResult(interp,
                    "bad value for -mode: should be baud,parity,data,stop",
                    (char *) NULL);
            }
            return TCL_ERROR;
        }

        /* Default settings for serial communications */ 
        dcb.fBinary = TRUE;
        dcb.fErrorChar = FALSE;
        dcb.fNull = FALSE;
        dcb.fAbortOnError = FALSE;

        if (! SetCommState(infoPtr->handle, &dcb) ) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't set comm state", (char *) NULL);
            }
            return TCL_ERROR;
        }
        return TCL_OK;
    }
    
    /* 
    * Option -handshake none|xonxoff|rtscts|dtrdsr
    */
    if ((len > 1) && (strncmp(optionName, "-handshake", len) == 0)) {
        
        if (! GetCommState(infoPtr->handle, &dcb)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't get comm state", (char *) NULL);
            }
            return TCL_ERROR;
        }
        /* 
        * Reset all handshake options
        * DTR and RTS are ON by default
        */
        dcb.fOutX = dcb.fInX = FALSE;
        dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = dcb.fDsrSensitivity = FALSE;
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        dcb.fTXContinueOnXoff = FALSE;

        /* 
        * Adjust the handshake limits.
        * Yes, the XonXoff limits seem to influence even hardware handshake
        */
        dcb.XonLim = (WORD) (infoPtr->sysBufRead*1/2);
        dcb.XoffLim = (WORD) (infoPtr->sysBufRead*1/4);
        
        if (strnicmp(value, "NONE", vlen) == 0) {
            /* leave all handshake options disabled */
        } else if (strnicmp(value, "XONXOFF", vlen) == 0) {
            dcb.fOutX = dcb.fInX = TRUE;
        } else if (strnicmp(value, "RTSCTS", vlen) == 0) {
            dcb.fOutxCtsFlow = TRUE;
            dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        } else if (strnicmp(value, "DTRDSR", vlen) == 0) {
            dcb.fOutxDsrFlow = TRUE;
            dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
        } else {
            if (interp) {
                Tcl_AppendResult(interp, "bad value for -handshake: ",
                    "must be one of xonxoff, rtscts, dtrdsr or none",
                    (char *) NULL);
                return TCL_ERROR;
            }
        }
        
        if (! SetCommState(infoPtr->handle, &dcb)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't set comm state", (char *) NULL);
            }
            return TCL_ERROR;
        }
        return TCL_OK;
    }
    
    /* 
    * Option -xchar {\x11 \x13}
    */
    if ((len > 1) && (strncmp(optionName, "-xchar", len) == 0)) {
        
        if (! GetCommState(infoPtr->handle, &dcb)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't get comm state", (char *) NULL);
            }
            return TCL_ERROR;
        }
        
        if (Tcl_SplitList(interp, value, &argc, &argv) == TCL_ERROR) {
            return TCL_ERROR;
        }
        if (argc == 2) {
            dcb.XonChar  = argv[0][0];
            dcb.XoffChar = argv[1][0];
        } else {
            if (interp) {
                Tcl_AppendResult(interp,
                    "bad value for -xchar: should be a list of two elements",
                    (char *) NULL);
            }
            return TCL_ERROR;
        }
        
        if (! SetCommState(infoPtr->handle, &dcb)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't set comm state", (char *) NULL);
            }
            return TCL_ERROR;
        }
        return TCL_OK;
    }
    
    /* 
    * Option -ttycontrol {DTR 1 RTS 0 BREAK 0}
    */
    if ((len > 4) && (strncmp(optionName, "-ttycontrol", len) == 0)) {
        
        if (Tcl_SplitList(interp, value, &argc, &argv) == TCL_ERROR) {
            return TCL_ERROR;
        }
        if ((argc % 2) == 1) {
            if (interp) {
                Tcl_AppendResult(interp,
                    "bad value for -ttycontrol: should be a list of signal,value pairs",
                    (char *) NULL);
            }
            return TCL_ERROR;
        }
        while (argc > 1) {
            if (Tcl_GetBoolean(interp, argv[1], &flag) == TCL_ERROR) {
                return TCL_ERROR;
            }
            if (strnicmp(argv[0], "DTR", strlen(argv[0])) == 0) {
                if (! EscapeCommFunction(infoPtr->handle, flag ?
                        (DWORD) SETDTR : (DWORD) CLRDTR)) {
                    if (interp) {
                        Tcl_AppendResult(interp, 
                            "can't set DTR signal", (char *) NULL);
                    }
                    return TCL_ERROR;
                }
            } else if (strnicmp(argv[0], "RTS", strlen(argv[0])) == 0) {
                if (! EscapeCommFunction(infoPtr->handle, flag ?
                        (DWORD) SETRTS : (DWORD) CLRRTS)) {
                    if (interp) {
                        Tcl_AppendResult(interp, 
                            "can't set RTS signal", (char *) NULL);
                    }
                    return TCL_ERROR;
                }
            } else if (strnicmp(argv[0], "BREAK", strlen(argv[0])) == 0) {
                if (! EscapeCommFunction(infoPtr->handle, flag ?
                        (DWORD) SETBREAK : (DWORD) CLRBREAK)) {
                    if (interp) {
                        Tcl_AppendResult(interp, 
                            "can't set BREAK signal", (char *) NULL);
                    }
                    return TCL_ERROR;
                }
            } else {
                if (interp) {
                    Tcl_AppendResult(interp, 
                        "bad signal for -ttycontrol: must be DTR, RTS or BREAK", 
                        (char *) NULL);
                }
                return TCL_ERROR;
            }
            argc -= 2, argv += 2;
        } /* while (argc > 1) */
        
        return TCL_OK;
    }
    
    /* 
    * Option -sysbuffer {read_size write_size}
    * Option -sysbuffer read_size 
    */
    if ((len > 1) && (strncmp(optionName, "-sysbuffer", len) == 0)) {
        
        /*
        * -sysbuffer 4096 or -sysbuffer {64536 4096}
        */
        size_t inSize = (size_t) -1, outSize = (size_t) -1;
        
        if (Tcl_SplitList(interp, value, &argc, &argv) == TCL_ERROR) {
            return TCL_ERROR;
        }
        if (argc == 1) {
            inSize = atoi(argv[0]);
            outSize = infoPtr->sysBufWrite;
        } else if (argc == 2) {
            inSize  = atoi(argv[0]);
            outSize = atoi(argv[1]);
        }
        if ( (inSize <= 0) || (outSize <= 0) ) {
            if (interp) {
                Tcl_AppendResult(interp,
                    "bad value for -sysbuffer: should be a list of one or two integers > 0",
                    (char *) NULL);
            }
            return TCL_ERROR;
        }
        if (! SetupComm(infoPtr->handle, inSize, outSize)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't setup comm buffers", (char *) NULL);
            }
            return TCL_ERROR;
        }
        infoPtr->sysBufRead  = inSize;
        infoPtr->sysBufWrite = outSize;
        
         /* 
        * Adjust the handshake limits.
        * Yes, the XonXoff limits seem to influence even hardware handshake
        */
        if (! GetCommState(infoPtr->handle, &dcb)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't get comm state", (char *) NULL);
            }
            return TCL_ERROR;
        }
        dcb.XonLim = (WORD) (infoPtr->sysBufRead*1/2);
        dcb.XoffLim = (WORD) (infoPtr->sysBufRead*1/4);
        if (! SetCommState(infoPtr->handle, &dcb)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't set comm state", (char *) NULL);
            }
            return TCL_ERROR;
        }
        return TCL_OK;
    }

    /* 
    * Option -pollinterval msec
    */
    if ((len > 1) && (strncmp(optionName, "-pollinterval", len) == 0)) {
        
        if ( Tcl_GetInt(interp, value, &(infoPtr->blockTime)) != TCL_OK ) {
            return TCL_ERROR;
        }
        return TCL_OK;
    }
    
    /* 
    * Option -timeout msec
    */
    if ((len > 2) && (strncmp(optionName, "-timeout", len) == 0)) {
        int msec;
        COMMTIMEOUTS tout = {0,0,0,0,0};
  
        if ( Tcl_GetInt(interp, value, &msec) != TCL_OK ) {
            return TCL_ERROR;
        }
        tout.ReadTotalTimeoutConstant = msec;
        if (! SetCommTimeouts(infoPtr->handle, &tout)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't set comm timeouts", (char *) NULL);
            }
            return TCL_ERROR;
        }

        return TCL_OK;
    }
    
    return Tcl_BadChannelOption(interp, optionName,
        "mode handshake pollinterval sysbuffer timeout ttycontrol xchar");
}

/*
 *----------------------------------------------------------------------
 *
 * SerialGetOptionProc --
 *
 *  Gets a mode associated with an IO channel. If the optionName arg
 *  is non NULL, retrieves the value of that option. If the optionName
 *  arg is NULL, retrieves a list of alternating option names and
 *  values for the given channel.
 *
 * Results:
 *  A standard Tcl result. Also sets the supplied DString to the
 *  string value of the option(s) returned.
 *
 * Side effects:
 *  The string returned by this function is in static storage and
 *  may be reused at any time subsequent to the call.
 *
 *----------------------------------------------------------------------
 */
static int
SerialGetOptionProc(instanceData, interp, optionName, dsPtr)
    ClientData instanceData;    /* File state. */
    Tcl_Interp *interp;         /* For error reporting - can be NULL. */
    CONST char *optionName;     /* Option to get. */
    Tcl_DString *dsPtr;         /* Where to store value(s). */
{
    SerialInfo *infoPtr;
    DCB dcb;
    size_t len;
    int valid = 0;  /* flag if valid option parsed */
    
    infoPtr = (SerialInfo *) instanceData;
    
    if (optionName == NULL) {
        len = 0;
    } else {
        len = strlen(optionName);
    }
    
    /*
    * get option -mode
    */
    
    if (len == 0) {
        Tcl_DStringAppendElement(dsPtr, "-mode");
    }
    if ((len == 0) ||
        ((len > 2) && (strncmp(optionName, "-mode", len) == 0))) {
        
        char parity;
        char *stop;
        char buf[2 * TCL_INTEGER_SPACE + 16];
        
        if (! GetCommState(infoPtr->handle, &dcb)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't get comm state", (char *) NULL);
            }
            return TCL_ERROR;
        }
        
        valid = 1;
        parity = 'n';
        if (dcb.Parity <= 4) {
            parity = "noems"[dcb.Parity];
        }
        stop = (dcb.StopBits == ONESTOPBIT) ? "1" :
        (dcb.StopBits == ONE5STOPBITS) ? "1.5" : "2";
        
        wsprintfA(buf, "%d,%c,%d,%s", dcb.BaudRate, parity,
            dcb.ByteSize, stop);
        Tcl_DStringAppendElement(dsPtr, buf);
    }
    
    /*
    * get option -pollinterval
    */
    
    if (len == 0) {
        Tcl_DStringAppendElement(dsPtr, "-pollinterval");
    }
    if ((len == 0) ||
        ((len > 1) && (strncmp(optionName, "-pollinterval", len) == 0))) {
        char buf[TCL_INTEGER_SPACE + 1];
        
        valid = 1;
        wsprintfA(buf, "%d", infoPtr->blockTime);
        Tcl_DStringAppendElement(dsPtr, buf);
    }
    
    /*
    * get option -sysbuffer
    */
    
    if (len == 0) {
        Tcl_DStringAppendElement(dsPtr, "-sysbuffer");
        Tcl_DStringStartSublist(dsPtr);
    }
    if ((len == 0) ||
        ((len > 1) && (strncmp(optionName, "-sysbuffer", len) == 0))) {

        char buf[TCL_INTEGER_SPACE + 1];
        valid = 1;

        wsprintfA(buf, "%d", infoPtr->sysBufRead);
        Tcl_DStringAppendElement(dsPtr, buf);
        wsprintfA(buf, "%d", infoPtr->sysBufWrite);
        Tcl_DStringAppendElement(dsPtr, buf);
    }
    if (len == 0) {
        Tcl_DStringEndSublist(dsPtr);
    }
    
    /*
    * get option -xchar
    */
    
    if (len == 0) {
        Tcl_DStringAppendElement(dsPtr, "-xchar");
        Tcl_DStringStartSublist(dsPtr);
    }
    if ((len == 0) ||
        ((len > 1) && (strncmp(optionName, "-xchar", len) == 0))) {

        char buf[4];
        valid = 1;

        if (! GetCommState(infoPtr->handle, &dcb)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't get comm state", (char *) NULL);
            }
            return TCL_ERROR;
        }
        sprintf(buf, "%c", dcb.XonChar);
        Tcl_DStringAppendElement(dsPtr, buf);
        sprintf(buf, "%c", dcb.XoffChar);
        Tcl_DStringAppendElement(dsPtr, buf);
    }
    if (len == 0) {
        Tcl_DStringEndSublist(dsPtr);
    }

    /*
    * get option -lasterror
    * option is readonly and returned by [fconfigure chan -lasterror]
    * but not returned by unnamed [fconfigure chan]
    */
    
    if ( (len > 1) && (strncmp(optionName, "-lasterror", len) == 0) ) {
        valid = 1;
        SerialErrorStr(infoPtr->lastError, dsPtr);
    }
    
    /*
    * get option -queue
    * option is readonly and returned by [fconfigure chan -queue]
    */
    
    if ((len > 1) && (strncmp(optionName, "-queue", len) == 0)) {
        char buf[TCL_INTEGER_SPACE + 1];
        COMSTAT cStat;
        DWORD error;
	int inBuffered, outBuffered, count;

        valid = 1;

        /* 
        * Query the pending data in Tcl's internal queues
        */
        inBuffered  = Tcl_InputBuffered(infoPtr->channel);
	outBuffered = Tcl_OutputBuffered(infoPtr->channel);

        /*
        * Query the number of bytes in our output queue:
        *     1. The bytes pending in the output thread
        *     2. The bytes in the system drivers buffer
        * The writer thread should not interfere this action.
        */
        EnterCriticalSection(&infoPtr->csWrite);
        ClearCommError( infoPtr->handle, &error, &cStat );
        count = (int)cStat.cbOutQue + infoPtr->writeQueue;
        LeaveCriticalSection(&infoPtr->csWrite);

        wsprintfA(buf, "%d", inBuffered + cStat.cbInQue); 
        Tcl_DStringAppendElement(dsPtr, buf);
        wsprintfA(buf, "%d", outBuffered + count); 
        Tcl_DStringAppendElement(dsPtr, buf);
    }

    /*
    * get option -ttystatus
    * option is readonly and returned by [fconfigure chan -ttystatus]
    * but not returned by unnamed [fconfigure chan]
    */
    if ( (len > 4) && (strncmp(optionName, "-ttystatus", len) == 0) ) {
        
        DWORD status;
        
        if (! GetCommModemStatus(infoPtr->handle, &status)) {
            if (interp) {
                Tcl_AppendResult(interp, 
                    "can't get tty status", (char *) NULL);
            }
            return TCL_ERROR;
        }
        valid = 1;
        SerialModemStatusStr(status, dsPtr);
    }
    
    if (valid) {
        return TCL_OK;
    } else {
        return Tcl_BadChannelOption(interp, optionName,
            "mode pollinterval lasterror queue sysbuffer ttystatus xchar");
    }
}
