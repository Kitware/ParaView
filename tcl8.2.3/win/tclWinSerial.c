/*
 * Tclwinserial.c --
 *
 *  This file implements the Windows-specific serial port functions,
 *  and the "serial" channel driver.
 *
 * Copyright (c) 1999 by Scriptics Corp.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * Changes by Rolf.Schroedter@dlr.de June 25-27, 1999
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
 * Bit masks used in the flags field of the SerialInfo structure below.
 */

#define SERIAL_PENDING  (1<<0)  /* Message is pending in the queue. */
#define SERIAL_ASYNC    (1<<1)  /* Channel is non-blocking. */

/*
 * Bit masks used in the sharedFlags field of the SerialInfo structure below.
 */

#define SERIAL_EOF      (1<<2)  /* Serial has reached EOF. */
#define SERIAL_ERROR    (1<<4)
#define SERIAL_WRITE    (1<<5)  /* enables fileevent writable
                 * one time after write operation */

/*
 * Default time to block between checking status on the serial port.
 */
#define SERIAL_DEFAULT_BLOCKTIME    10  /* 10 msec */

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
    int writable;               /* flag that the channel is readable */
    int readable;               /* flag that the channel is readable */
    int blockTime;              /* max. blocktime in msec */
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

COMMTIMEOUTS timeout_sync  = {   /* Timouts for blocking mode */
    MAXDWORD,        /* ReadIntervalTimeout */
    MAXDWORD,        /* ReadTotalTimeoutMultiplier */
    MAXDWORD-1,      /* ReadTotalTimeoutConstant,
            MAXDWORD-1 works for both Win95/NT */
    0,               /* WriteTotalTimeoutMultiplier */
    0,               /* WriteTotalTimeoutConstant */
};

COMMTIMEOUTS timeout_async  = {   /* Timouts for non-blocking mode */
    0,               /* ReadIntervalTimeout */
    0,               /* ReadTotalTimeoutMultiplier */
    1,               /* ReadTotalTimeoutConstant */
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
static int      SerialOutputProc(ClientData instanceData, char *buf,
                int toWrite, int *errorCode);
static void     SerialSetupProc(ClientData clientData, int flags);
static void     SerialWatchProc(ClientData instanceData, int mask);
static void     ProcExitHandler(ClientData clientData);
static int	SerialGetOptionProc _ANSI_ARGS_((ClientData instanceData,
                Tcl_Interp *interp, char *optionName,
                Tcl_DString *dsPtr));
static int	SerialSetOptionProc _ANSI_ARGS_((ClientData instanceData,
                Tcl_Interp *interp, char *optionName,
                char *value));

/*
 * This structure describes the channel type structure for command serial
 * based IO.
 */

static Tcl_ChannelType serialChannelType = {
    "serial",               /* Type name. */
    SerialBlockProc,        /* Set blocking or non-blocking mode.*/
    SerialCloseProc,        /* Close proc. */
    SerialInputProc,        /* Input proc. */
    SerialOutputProc,       /* Output proc. */
    NULL,                   /* Seek proc. */
    SerialSetOptionProc,    /* Set option proc. */
    SerialGetOptionProc,    /* Get option proc. */
    SerialWatchProc,        /* Set up notifier to watch the channel. */
    SerialGetHandleProc,    /* Get an OS handle from channel. */
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
        if (!initialized) {
            initialized = 1;
            Tcl_CreateExitHandler(ProcExitHandler, NULL);
        }
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
    initialized = 0;
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

void
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

        if( infoPtr->watchMask & (TCL_WRITABLE | TCL_READABLE) ) {
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
    DWORD   cError;
    COMSTAT cStat;

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
         * If any READABLE or WRITABLE watch mask is set
         * call ClearCommError to poll cbInQue,cbOutQue
         * Window errors are ignored here
         */

        if( infoPtr->watchMask & (TCL_WRITABLE | TCL_READABLE) ) {
            if( ClearCommError( infoPtr->handle, &cError, &cStat ) ) {
		/*
		 * Look for empty output buffer.  If empty, poll.
		 */

                if( infoPtr->watchMask & TCL_WRITABLE ) {
                    if( ((infoPtr->flags & SERIAL_WRITE) != 0) && \
			    (cStat.cbOutQue == 0) ) {
                        /*
			 * allow only one fileevent after each callback
			 */

                        infoPtr->flags &= ~SERIAL_WRITE;
                        infoPtr->writable = 1;
                        needEvent = 1;
                    }
                }
		
                /*
                 * Look for characters already pending in windows queue.
		 * If they are, poll.
                 */

                if( infoPtr->watchMask & TCL_READABLE ) {
                    if( cStat.cbInQue > 0 ) {
                        infoPtr->readable = 1;
                        needEvent = 1;
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
    COMMTIMEOUTS *timeout;
    int errorCode = 0;

    SerialInfo *infoPtr = (SerialInfo *) instanceData;

    /*
     * Serial IO on Windows can not be switched between blocking & nonblocking,
     * hence we have to emulate the behavior. This is done in the input
     * function by checking against a bit in the state. We set or unset the
     * bit here to cause the input function to emulate the correct behavior.
     */

    if (mode == TCL_MODE_NONBLOCKING) {
        infoPtr->flags |= SERIAL_ASYNC;
        timeout = &timeout_async;
    } else {
        infoPtr->flags &= ~(SERIAL_ASYNC);
        timeout = &timeout_sync;
    }
    if (SetCommTimeouts(infoPtr->handle, timeout) == FALSE) {
        TclWinConvertError(GetLastError());
        errorCode = errno;
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

    errorCode = 0;
    serialPtr->validMask &= ~TCL_READABLE;
    serialPtr->validMask &= ~TCL_WRITABLE;

    /*
     * Don't close the Win32 handle if the handle is a standard channel
     * during the exit process.  Otherwise, one thread may kill the stdio
     * of another.
     */

    if (!TclInExit()
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

    ckfree((char*) serialPtr);

    if (errorCode == 0) {
        return result;
    }
    return errorCode;
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
    DWORD err;
    DWORD   cError;
    COMSTAT cStat;

    *errorCode = 0;

    /*
     * Look for characters already pending in windows queue.
     * This is the mainly restored good old code from Tcl8.0
     */

    if( ClearCommError( infoPtr->handle, &cError, &cStat ) ) {
        /*
	 * Check for errors here, but not in the evSetup/Check procedures
	 */

        if( cError != 0 ) {
            *errorCode = EIO;
            return -1;
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

    if (ReadFile(infoPtr->handle, (LPVOID) buf, (DWORD) bufSize, &bytesRead,
        NULL) == FALSE) {
        err = GetLastError();
        if (err != ERROR_IO_PENDING) {
            goto error;
        }
    }
    return bytesRead;

error:
    TclWinConvertError(GetLastError());
    *errorCode = errno;
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
    char *buf,                  /* The data buffer. */
    int toWrite,                /* How many bytes to write? */
    int *errorCode)             /* Where to store error code. */
{
    SerialInfo *infoPtr = (SerialInfo *) instanceData;
    DWORD bytesWritten, err;

    *errorCode = 0;

    /*
     * Check for a background error on the last write.
     * Allow one write-fileevent after each callback
     */

    if( toWrite ) {
        infoPtr->flags |= SERIAL_WRITE;
    }

    if (WriteFile(infoPtr->handle, (LPVOID) buf, (DWORD) toWrite,
            &bytesWritten, NULL) == FALSE) {
        err = GetLastError();
        if (err != ERROR_IO_PENDING) {
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

    tsdPtr = SerialInit();

    SetupComm(handle, 4096, 4096);
    PurgeComm(handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR
          | PURGE_RXCLEAR);

    /*
     * default is blocking
     */

    SetCommTimeouts(handle, &timeout_sync);

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


    infoPtr->readable = infoPtr->writable = 0;
    infoPtr->blockTime = SERIAL_DEFAULT_BLOCKTIME;

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
    char *optionName;           /* Which option to set? */
    char *value;                /* New value for option. */
{
    SerialInfo *infoPtr;
    DCB dcb;
    int len;
    BOOL result;
    Tcl_DString ds;
    TCHAR *native;
    
    infoPtr = (SerialInfo *) instanceData;
    
    len = strlen(optionName);
    if ((len > 1) && (strncmp(optionName, "-mode", len) == 0)) {
	if (GetCommState(infoPtr->handle, &dcb)) {
	    native = Tcl_WinUtfToTChar(value, -1, &ds);
	    result = (*tclWinProcs->buildCommDCBProc)(native, &dcb);
	    Tcl_DStringFree(&ds);
	    
	    if ((result == FALSE) ||
                    (SetCommState(infoPtr->handle, &dcb) == FALSE)) {
		/*
		 * one should separate the 2 errors...
		 */
		
		if (interp) {
		    Tcl_AppendResult(interp,
			    "bad value for -mode: should be ",
			    "baud,parity,data,stop", NULL);
		}
		return TCL_ERROR;
	    } else {
		return TCL_OK;
	    }
	} else {
	    if (interp) {
		Tcl_AppendResult(interp, "can't get comm state", NULL);
	    }
	    return TCL_ERROR;
	}
    } else if ((len > 1) &&
	    (strncmp(optionName, "-pollinterval", len) == 0)) {
	if ( Tcl_GetInt(interp, value, &(infoPtr->blockTime)) != TCL_OK ) {
	    return TCL_ERROR;
	}
    } else {
	return Tcl_BadChannelOption(interp, optionName,
		"mode pollinterval");
    }
    return TCL_OK;
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
    char *optionName;           /* Option to get. */
    Tcl_DString *dsPtr;         /* Where to store value(s). */
{
    SerialInfo *infoPtr;
    DCB dcb;
    int len;
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
        ((len > 1) && (strncmp(optionName, "-mode", len) == 0))) {
        valid = 1;
        if (GetCommState(infoPtr->handle, &dcb) == 0) {
	    /*
	     * shouldn't we flag an error instead ?
	     */
	    
            Tcl_DStringAppendElement(dsPtr, "");

        } else {
            char parity;
            char *stop;
            char buf[2 * TCL_INTEGER_SPACE + 16];

            parity = 'n';
            if (dcb.Parity < 4) {
                parity = "noems"[dcb.Parity];
            }

            stop = (dcb.StopBits == ONESTOPBIT) ? "1" :
            (dcb.StopBits == ONE5STOPBITS) ? "1.5" : "2";

            wsprintfA(buf, "%d,%c,%d,%s", dcb.BaudRate, parity,
            dcb.ByteSize, stop);
            Tcl_DStringAppendElement(dsPtr, buf);
        }
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

    if (valid) {
        return TCL_OK;
    } else {
        return Tcl_BadChannelOption(interp, optionName, "mode pollinterval");
    }
}
