/* 
 * tclIO.c --
 *
 *	This file provides the generic portions (those that are the same on
 *	all platforms and for all channel types) of Tcl's IO facilities.
 *
 * Copyright (c) 1998 Scriptics Corporation
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"

/*
 * Make sure that both EAGAIN and EWOULDBLOCK are defined. This does not
 * compile on systems where neither is defined. We want both defined so
 * that we can test safely for both. In the code we still have to test for
 * both because there may be systems on which both are defined and have
 * different values.
 */

#if ((!defined(EWOULDBLOCK)) && (defined(EAGAIN)))
#   define EWOULDBLOCK EAGAIN
#endif
#if ((!defined(EAGAIN)) && (defined(EWOULDBLOCK)))
#   define EAGAIN EWOULDBLOCK
#endif
#if ((!defined(EAGAIN)) && (!defined(EWOULDBLOCK)))
    error one of EWOULDBLOCK or EAGAIN must be defined
#endif

/*
 * The following structure encapsulates the state for a background channel
 * copy.  Note that the data buffer for the copy will be appended to this
 * structure.
 */

typedef struct CopyState {
    struct Channel *readPtr;	/* Pointer to input channel. */
    struct Channel *writePtr;	/* Pointer to output channel. */
    int readFlags;		/* Original read channel flags. */
    int writeFlags;		/* Original write channel flags. */
    int toRead;			/* Number of bytes to copy, or -1. */
    int total;			/* Total bytes transferred (written). */
    Tcl_Interp *interp;		/* Interp that started the copy. */
    Tcl_Obj *cmdPtr;		/* Command to be invoked at completion. */
    int bufSize;		/* Size of appended buffer. */
    char buffer[1];		/* Copy buffer, this must be the last
				 * field. */
} CopyState;

/*
 * struct ChannelBuffer:
 *
 * Buffers data being sent to or from a channel.
 */

typedef struct ChannelBuffer {
    int nextAdded;		/* The next position into which a character
                                 * will be put in the buffer. */
    int nextRemoved;		/* Position of next byte to be removed
                                 * from the buffer. */
    int bufLength;		/* How big is the buffer? */
    struct ChannelBuffer *nextPtr;
    				/* Next buffer in chain. */
    char buf[4];		/* Placeholder for real buffer. The real
                                 * buffer occuppies this space + bufSize-4
                                 * bytes. This must be the last field in
                                 * the structure. */
} ChannelBuffer;

#define CHANNELBUFFER_HEADER_SIZE	(sizeof(ChannelBuffer) - 4)

/*
 * How much extra space to allocate in buffer to hold bytes from previous
 * buffer (when converting to UTF-8) or to hold bytes that will go to
 * next buffer (when converting from UTF-8).
 */
 
#define BUFFER_PADDING	    16
 
/*
 * The following defines the *default* buffer size for channels.
 */

#define CHANNELBUFFER_DEFAULT_SIZE	(1024 * 4)

/*
 * Structure to record a close callback. One such record exists for
 * each close callback registered for a channel.
 */

typedef struct CloseCallback {
    Tcl_CloseProc *proc;		/* The procedure to call. */
    ClientData clientData;		/* Arbitrary one-word data to pass
                                         * to the callback. */
    struct CloseCallback *nextPtr;	/* For chaining close callbacks. */
} CloseCallback;

/*
 * The following structure describes the information saved from a call to
 * "fileevent". This is used later when the event being waited for to
 * invoke the saved script in the interpreter designed in this record.
 */

typedef struct EventScriptRecord {
    struct Channel *chanPtr;	/* The channel for which this script is
                                 * registered. This is used only when an
                                 * error occurs during evaluation of the
                                 * script, to delete the handler. */
    Tcl_Obj *scriptPtr;		/* Script to invoke. */
    Tcl_Interp *interp;		/* In what interpreter to invoke script? */
    int mask;			/* Events must overlap current mask for the
                                 * stored script to be invoked. */
    struct EventScriptRecord *nextPtr;
    				/* Next in chain of records. */
} EventScriptRecord;

/*
 * struct Channel:
 *
 * One of these structures is allocated for each open channel. It contains data
 * specific to the channel but which belongs to the generic part of the Tcl
 * channel mechanism, and it points at an instance specific (and type
 * specific) * instance data, and at a channel type structure.
 */

typedef struct Channel {
    char *channelName;		/* The name of the channel instance in Tcl
                                 * commands. Storage is owned by the generic IO
                                 * code,  is dynamically allocated. */
    int	flags;			/* ORed combination of the flags defined
                                 * below. */
    Tcl_Encoding encoding;	/* Encoding to apply when reading or writing
				 * data on this channel.  NULL means no
				 * encoding is applied to data. */
    Tcl_EncodingState inputEncodingState;
				/* Current encoding state, used when converting
				 * input data bytes to UTF-8. */
    int inputEncodingFlags;	/* Encoding flags to pass to conversion
				 * routine when converting input data bytes to
				 * UTF-8.  May be TCL_ENCODING_START before
				 * converting first byte and TCL_ENCODING_END
				 * when EOF is seen. */
    Tcl_EncodingState outputEncodingState;
				/* Current encoding state, used when converting
				 * UTF-8 to output data bytes. */
    int outputEncodingFlags;	/* Encoding flags to pass to conversion
				 * routine when converting UTF-8 to output
				 * data bytes.  May be TCL_ENCODING_START
				 * before converting first byte and
				 * TCL_ENCODING_END when EOF is seen. */
    Tcl_EolTranslation inputTranslation;
				/* What translation to apply for end of line
                                 * sequences on input? */    
    Tcl_EolTranslation outputTranslation;
    				/* What translation to use for generating
                                 * end of line sequences in output? */
    int inEofChar;		/* If nonzero, use this as a signal of EOF
                                 * on input. */
    int outEofChar;             /* If nonzero, append this to the channel
                                 * when it is closed if it is open for
                                 * writing. */
    int unreportedError;	/* Non-zero if an error report was deferred
                                 * because it happened in the background. The
                                 * value is the POSIX error code. */
    ClientData instanceData;	/* Instance-specific data provided by
				 * creator of channel. */

    Tcl_ChannelType *typePtr;	/* Pointer to channel type structure. */
    int refCount;		/* How many interpreters hold references to
                                 * this IO channel? */
    CloseCallback *closeCbPtr;	/* Callbacks registered to be called when the
                                 * channel is closed. */
    char *outputStage;		/* Temporary staging buffer used when
				 * translating EOL before converting from
				 * UTF-8 to external form. */
    ChannelBuffer *curOutPtr;	/* Current output buffer being filled. */
    ChannelBuffer *outQueueHead;/* Points at first buffer in output queue. */
    ChannelBuffer *outQueueTail;/* Points at last buffer in output queue. */

    ChannelBuffer *saveInBufPtr;/* Buffer saved for input queue - eliminates
                                 * need to allocate a new buffer for "gets"
                                 * that crosses buffer boundaries. */
    ChannelBuffer *inQueueHead;	/* Points at first buffer in input queue. */
    ChannelBuffer *inQueueTail;	/* Points at last buffer in input queue. */

    struct ChannelHandler *chPtr;/* List of channel handlers registered
                                  * for this channel. */
    int interestMask;		/* Mask of all events this channel has
                                 * handlers for. */
    struct Channel *nextChanPtr;/* Next in list of channels currently open. */
    EventScriptRecord *scriptRecordPtr;
    				/* Chain of all scripts registered for
                                 * event handlers ("fileevent") on this
                                 * channel. */
    int bufSize;		/* What size buffers to allocate? */
    Tcl_TimerToken timer;	/* Handle to wakeup timer for this channel. */
    CopyState *csPtr;		/* State of background copy, or NULL. */
    struct Channel* supercedes; /* Refers to channel this one was stacked upon.
				   This reference is NULL for normal channels.
				   See Tcl_StackChannel. */

} Channel;
    
/*
 * Values for the flags field in Channel. Any ORed combination of the
 * following flags can be stored in the field. These flags record various
 * options and state bits about the channel. In addition to the flags below,
 * the channel can also have TCL_READABLE (1<<1) and TCL_WRITABLE (1<<2) set.
 */

#define CHANNEL_NONBLOCKING	(1<<3)	/* Channel is currently in
					 * nonblocking mode. */
#define CHANNEL_LINEBUFFERED	(1<<4)	/* Output to the channel must be
					 * flushed after every newline. */
#define CHANNEL_UNBUFFERED	(1<<5)	/* Output to the channel must always
					 * be flushed immediately. */
#define BUFFER_READY		(1<<6)	/* Current output buffer (the
					 * curOutPtr field in the
                                         * channel structure) should be
                                         * output as soon as possible even
                                         * though it may not be full. */
#define BG_FLUSH_SCHEDULED	(1<<7)	/* A background flush of the
					 * queued output buffers has been
                                         * scheduled. */
#define CHANNEL_CLOSED		(1<<8)	/* Channel has been closed. No
					 * further Tcl-level IO on the
                                         * channel is allowed. */
#define CHANNEL_EOF		(1<<9)	/* EOF occurred on this channel.
					 * This bit is cleared before every
                                         * input operation. */
#define CHANNEL_STICKY_EOF	(1<<10)	/* EOF occurred on this channel because
					 * we saw the input eofChar. This bit
                                         * prevents clearing of the EOF bit
                                         * before every input operation. */
#define CHANNEL_BLOCKED	(1<<11)	/* EWOULDBLOCK or EAGAIN occurred
					 * on this channel. This bit is
                                         * cleared before every input or
                                         * output operation. */
#define INPUT_SAW_CR		(1<<12)	/* Channel is in CRLF eol input
					 * translation mode and the last
                                         * byte seen was a "\r". */
#define INPUT_NEED_NL		(1<<15)	/* Saw a '\r' at end of last buffer,
					 * and there should be a '\n' at
					 * beginning of next buffer. */
#define CHANNEL_DEAD		(1<<13)	/* The channel has been closed by
					 * the exit handler (on exit) but
                                         * not deallocated. When any IO
                                         * operation sees this flag on a
                                         * channel, it does not call driver
                                         * level functions to avoid referring
                                         * to deallocated data. */
#define CHANNEL_NEED_MORE_DATA	(1<<14)	/* The last input operation failed
					 * because there was not enough data
					 * to complete the operation.  This
					 * flag is set when gets fails to
					 * get a complete line or when read
					 * fails to get a complete character.
					 * When set, file events will not be
					 * delivered for buffered data until
					 * the state of the channel changes. */

/*
 * For each channel handler registered in a call to Tcl_CreateChannelHandler,
 * there is one record of the following type. All of records for a specific
 * channel are chained together in a singly linked list which is stored in
 * the channel structure.
 */

typedef struct ChannelHandler {
    Channel *chanPtr;		/* The channel structure for this channel. */
    int mask;			/* Mask of desired events. */
    Tcl_ChannelProc *proc;	/* Procedure to call in the type of
                                 * Tcl_CreateChannelHandler. */
    ClientData clientData;	/* Argument to pass to procedure. */
    struct ChannelHandler *nextPtr;
    				/* Next one in list of registered handlers. */
} ChannelHandler;

/*
 * This structure keeps track of the current ChannelHandler being invoked in
 * the current invocation of ChannelHandlerEventProc. There is a potential
 * problem if a ChannelHandler is deleted while it is the current one, since
 * ChannelHandlerEventProc needs to look at the nextPtr field. To handle this
 * problem, structures of the type below indicate the next handler to be
 * processed for any (recursively nested) dispatches in progress. The
 * nextHandlerPtr field is updated if the handler being pointed to is deleted.
 * The nextPtr field is used to chain together all recursive invocations, so
 * that Tcl_DeleteChannelHandler can find all the recursively nested
 * invocations of ChannelHandlerEventProc and compare the handler being
 * deleted against the NEXT handler to be invoked in that invocation; when it
 * finds such a situation, Tcl_DeleteChannelHandler updates the nextHandlerPtr
 * field of the structure to the next handler.
 */

typedef struct NextChannelHandler {
    ChannelHandler *nextHandlerPtr;	/* The next handler to be invoked in
                                         * this invocation. */
    struct NextChannelHandler *nestedHandlerPtr;
					/* Next nested invocation of
                                         * ChannelHandlerEventProc. */
} NextChannelHandler;


/*
 * The following structure describes the event that is added to the Tcl
 * event queue by the channel handler check procedure.
 */

typedef struct ChannelHandlerEvent {
    Tcl_Event header;		/* Standard header for all events. */
    Channel *chanPtr;		/* The channel that is ready. */
    int readyMask;		/* Events that have occurred. */
} ChannelHandlerEvent;

/*
 * The following structure is used by Tcl_GetsObj() to encapsulates the
 * state for a "gets" operation.
 */
 
typedef struct GetsState {
    Tcl_Obj *objPtr;		/* The object to which UTF-8 characters
				 * will be appended. */
    char **dstPtr;		/* Pointer into objPtr's string rep where
				 * next character should be stored. */
    Tcl_Encoding encoding;	/* The encoding to use to convert raw bytes
				 * to UTF-8.  */
    ChannelBuffer *bufPtr;	/* The current buffer of raw bytes being
				 * emptied. */
    Tcl_EncodingState state;	/* The encoding state just before the last
				 * external to UTF-8 conversion in
				 * FilterInputBytes(). */
    int rawRead;		/* The number of bytes removed from bufPtr
				 * in the last call to FilterInputBytes(). */
    int bytesWrote;		/* The number of bytes of UTF-8 data
				 * appended to objPtr during the last call to
				 * FilterInputBytes(). */
    int charsWrote;		/* The corresponding number of UTF-8
				 * characters appended to objPtr during the
				 * last call to FilterInputBytes(). */
    int totalChars;		/* The total number of UTF-8 characters
				 * appended to objPtr so far, just before the
				 * last call to FilterInputBytes(). */
} GetsState;

/*
 * All static variables used in this file are collected into a single
 * instance of the following structure.  For multi-threaded implementations,
 * there is one instance of this structure for each thread.
 *
 * Notice that different structures with the same name appear in other
 * files.  The structure defined below is used in this file only.
 */

typedef struct ThreadSpecificData {

    /*
     * This variable holds the list of nested ChannelHandlerEventProc 
     * invocations.
     */
    NextChannelHandler *nestedHandlerPtr;

    /*
     * List of all channels currently open.
     */
    Channel *firstChanPtr;
#ifdef oldcode
    /*
     * Has a channel exit handler been created yet?
     */
    int channelExitHandlerCreated;

    /*
     * Has the channel event source been created and registered with the
     * notifier?
     */
    int channelEventSourceCreated;
#endif
    /*
     * Static variables to hold channels for stdin, stdout and stderr.
     */
    Tcl_Channel stdinChannel;
    int stdinInitialized;
    Tcl_Channel stdoutChannel;
    int stdoutInitialized;
    Tcl_Channel stderrChannel;
    int stderrInitialized;

} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;


/*
 * Static functions in this file:
 */

static ChannelBuffer *	AllocChannelBuffer _ANSI_ARGS_((int length));
static void		ChannelEventScriptInvoker _ANSI_ARGS_((
			    ClientData clientData, int flags));
static void		ChannelTimerProc _ANSI_ARGS_((
			    ClientData clientData));
static int		CheckChannelErrors _ANSI_ARGS_((Channel *chanPtr,
			    int direction));
static int		CheckFlush _ANSI_ARGS_((Channel *chanPtr,
			    ChannelBuffer *bufPtr, int newlineFlag));
static int		CheckForDeadChannel _ANSI_ARGS_((Tcl_Interp *interp,
			    Channel *chan));
static void		CheckForStdChannelsBeingClosed _ANSI_ARGS_((
			    Tcl_Channel chan));
static void		CleanupChannelHandlers _ANSI_ARGS_((
			    Tcl_Interp *interp, Channel *chanPtr));
static int		CloseChannel _ANSI_ARGS_((Tcl_Interp *interp,
                            Channel *chanPtr, int errorCode));
static void		CommonGetsCleanup _ANSI_ARGS_((Channel *chanPtr,
			    Tcl_Encoding encoding));
static int		CopyAndTranslateBuffer _ANSI_ARGS_((
			    Channel *chanPtr, char *result, int space));
static int		CopyData _ANSI_ARGS_((CopyState *csPtr, int mask));
static void		CopyEventProc _ANSI_ARGS_((ClientData clientData,
			    int mask));
static void		CreateScriptRecord _ANSI_ARGS_((
			    Tcl_Interp *interp, Channel *chanPtr,
                            int mask, Tcl_Obj *scriptPtr));
static void		DeleteChannelTable _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp));
static void		DeleteScriptRecord _ANSI_ARGS_((Tcl_Interp *interp,
        		    Channel *chanPtr, int mask));
static void		DiscardInputQueued _ANSI_ARGS_((
			    Channel *chanPtr, int discardSavedBuffers));
static void		DiscardOutputQueued _ANSI_ARGS_((
    			    Channel *chanPtr));
static int		DoRead _ANSI_ARGS_((Channel *chanPtr, char *srcPtr,
			    int slen));
static int		DoWrite _ANSI_ARGS_((Channel *chanPtr, char *src,
			    int srcLen));
static int		FilterInputBytes _ANSI_ARGS_((Channel *chanPtr,
			    GetsState *statePtr));
static int		FlushChannel _ANSI_ARGS_((Tcl_Interp *interp,
                            Channel *chanPtr, int calledFromAsyncFlush));
static Tcl_HashTable *	GetChannelTable _ANSI_ARGS_((Tcl_Interp *interp));
static int		GetInput _ANSI_ARGS_((Channel *chanPtr));
static void		PeekAhead _ANSI_ARGS_((Channel *chanPtr,
			    char **dstEndPtr, GetsState *gsPtr));
static int		ReadBytes _ANSI_ARGS_((Channel *chanPtr,
			    Tcl_Obj *objPtr, int charsLeft, int *offsetPtr));
static int		ReadChars _ANSI_ARGS_((Channel *chanPtr,
			    Tcl_Obj *objPtr, int charsLeft, int *offsetPtr,
			    int *factorPtr));
static void		RecycleBuffer _ANSI_ARGS_((Channel *chanPtr,
		            ChannelBuffer *bufPtr, int mustDiscard));
static int		SetBlockMode _ANSI_ARGS_((Tcl_Interp *interp,
		            Channel *chanPtr, int mode));
static void		StopCopy _ANSI_ARGS_((CopyState *csPtr));
static int		TranslateInputEOL _ANSI_ARGS_((Channel *chanPtr,
			    char *dst, CONST char *src, int *dstLenPtr,
			    int *srcLenPtr));
static int		TranslateOutputEOL _ANSI_ARGS_((Channel *chanPtr,
			    char *dst, CONST char *src, int *dstLenPtr,
			    int *srcLenPtr));
static void		UpdateInterest _ANSI_ARGS_((Channel *chanPtr));
static int		WriteBytes _ANSI_ARGS_((Channel *chanPtr,
			    CONST char *src, int srcLen));
static int		WriteChars _ANSI_ARGS_((Channel *chanPtr,
			    CONST char *src, int srcLen));


/*
 *---------------------------------------------------------------------------
 *
 * TclInitIOSubsystem --
 *
 *	Initialize all resources used by this subsystem on a per-process
 *	basis.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the memory subsystems.
 *
 *---------------------------------------------------------------------------
 */

void
TclInitIOSubsystem()
{
    /*
     * By fetching thread local storage we take care of
     * allocating it for each thread.
     */
    (void) TCL_TSD_INIT(&dataKey);
}   

/*
 *-------------------------------------------------------------------------
 *
 * TclFinalizeIOSubsystem --
 *
 *	Releases all resources used by this subsystem on a per-process 
 *	basis.  Closes all extant channels that have not already been 
 *	closed because they were not owned by any interp.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on encoding and memory subsystems.
 *
 *-------------------------------------------------------------------------
 */

	/* ARGSUSED */
void
TclFinalizeIOSubsystem()
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    Channel *chanPtr;			/* Iterates over open channels. */
    Channel *nextChanPtr;		/* Iterates over open channels. */


    for (chanPtr = tsdPtr->firstChanPtr; chanPtr != (Channel *) NULL;
             chanPtr = nextChanPtr) {
        nextChanPtr = chanPtr->nextChanPtr;

        /*
         * Set the channel back into blocking mode to ensure that we wait
         * for all data to flush out.
         */
        
        (void) Tcl_SetChannelOption(NULL, (Tcl_Channel) chanPtr,
                "-blocking", "on");

        if ((chanPtr == (Channel *) tsdPtr->stdinChannel) ||
                (chanPtr == (Channel *) tsdPtr->stdoutChannel) ||
                (chanPtr == (Channel *) tsdPtr->stderrChannel)) {

            /*
             * Decrement the refcount which was earlier artificially bumped
             * up to keep the channel from being closed.
             */

            chanPtr->refCount--;
        }

        if (chanPtr->refCount <= 0) {

	    /*
             * Close it only if the refcount indicates that the channel is not
             * referenced from any interpreter. If it is, that interpreter will
             * close the channel when it gets destroyed.
             */

            (void) Tcl_Close((Tcl_Interp *) NULL, (Tcl_Channel) chanPtr);

        } else {

            /*
             * The refcount is greater than zero, so flush the channel.
             */

            Tcl_Flush((Tcl_Channel) chanPtr);

            /*
             * Call the device driver to actually close the underlying
             * device for this channel.
             */
            
	    if (chanPtr->typePtr->closeProc != TCL_CLOSE2PROC) {
		(chanPtr->typePtr->closeProc)(chanPtr->instanceData,
			(Tcl_Interp *) NULL);
	    } else {
		(chanPtr->typePtr->close2Proc)(chanPtr->instanceData,
			(Tcl_Interp *) NULL, 0);
	    }

            /*
             * Finally, we clean up the fields in the channel data structure
             * since all of them have been deleted already. We mark the
             * channel with CHANNEL_DEAD to prevent any further IO operations
             * on it.
             */

            chanPtr->instanceData = (ClientData) NULL;
            chanPtr->flags |= CHANNEL_DEAD;
        }
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetStdChannel --
 *
 *	This function is used to change the channels that are used
 *	for stdin/stdout/stderr in new interpreters.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetStdChannel(channel, type)
    Tcl_Channel channel;
    int type;			/* One of TCL_STDIN, TCL_STDOUT, TCL_STDERR. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    switch (type) {
	case TCL_STDIN:
	    tsdPtr->stdinInitialized = 1;
	    tsdPtr->stdinChannel = channel;
	    break;
	case TCL_STDOUT:
	    tsdPtr->stdoutInitialized = 1;
	    tsdPtr->stdoutChannel = channel;
	    break;
	case TCL_STDERR:
	    tsdPtr->stderrInitialized = 1;
	    tsdPtr->stderrChannel = channel;
	    break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetStdChannel --
 *
 *	Returns the specified standard channel.
 *
 * Results:
 *	Returns the specified standard channel, or NULL.
 *
 * Side effects:
 *	May cause the creation of a standard channel and the underlying
 *	file.
 *
 *----------------------------------------------------------------------
 */
Tcl_Channel
Tcl_GetStdChannel(type)
    int type;			/* One of TCL_STDIN, TCL_STDOUT, TCL_STDERR. */
{
    Tcl_Channel channel = NULL;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    /*
     * If the channels were not created yet, create them now and
     * store them in the static variables. 
     */

    switch (type) {
	case TCL_STDIN:
	    if (!tsdPtr->stdinInitialized) {
		tsdPtr->stdinChannel = TclpGetDefaultStdChannel(TCL_STDIN);
		tsdPtr->stdinInitialized = 1;

		/*
                 * Artificially bump the refcount to ensure that the channel
                 * is only closed on exit.
                 *
                 * NOTE: Must only do this if stdinChannel is not NULL. It
                 * can be NULL in situations where Tcl is unable to connect
                 * to the standard input.
                 */

                if (tsdPtr->stdinChannel != (Tcl_Channel) NULL) {
                    (void) Tcl_RegisterChannel((Tcl_Interp *) NULL,
                            tsdPtr->stdinChannel);
                }
	    }
	    channel = tsdPtr->stdinChannel;
	    break;
	case TCL_STDOUT:
	    if (!tsdPtr->stdoutInitialized) {
		tsdPtr->stdoutChannel = TclpGetDefaultStdChannel(TCL_STDOUT);
		tsdPtr->stdoutInitialized = 1;
                if (tsdPtr->stdoutChannel != (Tcl_Channel) NULL) {
                    (void) Tcl_RegisterChannel((Tcl_Interp *) NULL,
                            tsdPtr->stdoutChannel);
                }
	    }
	    channel = tsdPtr->stdoutChannel;
	    break;
	case TCL_STDERR:
	    if (!tsdPtr->stderrInitialized) {
		tsdPtr->stderrChannel = TclpGetDefaultStdChannel(TCL_STDERR);
		tsdPtr->stderrInitialized = 1;
                if (tsdPtr->stderrChannel != (Tcl_Channel) NULL) {
                    (void) Tcl_RegisterChannel((Tcl_Interp *) NULL,
                            tsdPtr->stderrChannel);
                }
	    }
	    channel = tsdPtr->stderrChannel;
	    break;
    }
    return channel;
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateCloseHandler
 *
 *	Creates a close callback which will be called when the channel is
 *	closed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Causes the callback to be called in the future when the channel
 *	will be closed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateCloseHandler(chan, proc, clientData)
    Tcl_Channel chan;		/* The channel for which to create the
                                 * close callback. */
    Tcl_CloseProc *proc;	/* The callback routine to call when the
                                 * channel will be closed. */
    ClientData clientData;	/* Arbitrary data to pass to the
                                 * close callback. */
{
    Channel *chanPtr;
    CloseCallback *cbPtr;

    chanPtr = (Channel *) chan;

    cbPtr = (CloseCallback *) ckalloc((unsigned) sizeof(CloseCallback));
    cbPtr->proc = proc;
    cbPtr->clientData = clientData;

    cbPtr->nextPtr = chanPtr->closeCbPtr;
    chanPtr->closeCbPtr = cbPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteCloseHandler --
 *
 *	Removes a callback that would have been called on closing
 *	the channel. If there is no matching callback then this
 *	function has no effect.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The callback will not be called in the future when the channel
 *	is eventually closed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteCloseHandler(chan, proc, clientData)
    Tcl_Channel chan;		/* The channel for which to cancel the
                                 * close callback. */
    Tcl_CloseProc *proc;	/* The procedure for the callback to
                                 * remove. */
    ClientData clientData;	/* The callback data for the callback
                                 * to remove. */
{
    Channel *chanPtr;
    CloseCallback *cbPtr, *cbPrevPtr;

    chanPtr = (Channel *) chan;
    for (cbPtr = chanPtr->closeCbPtr, cbPrevPtr = (CloseCallback *) NULL;
             cbPtr != (CloseCallback *) NULL;
             cbPtr = cbPtr->nextPtr) {
        if ((cbPtr->proc == proc) && (cbPtr->clientData == clientData)) {
            if (cbPrevPtr == (CloseCallback *) NULL) {
                chanPtr->closeCbPtr = cbPtr->nextPtr;
            }
            ckfree((char *) cbPtr);
            break;
        } else {
            cbPrevPtr = cbPtr;
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetChannelTable --
 *
 *	Gets and potentially initializes the channel table for an
 *	interpreter. If it is initializing the table it also inserts
 *	channels for stdin, stdout and stderr if the interpreter is
 *	trusted.
 *
 * Results:
 *	A pointer to the hash table created, for use by the caller.
 *
 * Side effects:
 *	Initializes the channel table for an interpreter. May create
 *	channels for stdin, stdout and stderr.
 *
 *----------------------------------------------------------------------
 */

static Tcl_HashTable *
GetChannelTable(interp)
    Tcl_Interp *interp;
{
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_Channel stdinChan, stdoutChan, stderrChan;

    hTblPtr = (Tcl_HashTable *) Tcl_GetAssocData(interp, "tclIO", NULL);
    if (hTblPtr == (Tcl_HashTable *) NULL) {
        hTblPtr = (Tcl_HashTable *) ckalloc((unsigned) sizeof(Tcl_HashTable));
        Tcl_InitHashTable(hTblPtr, TCL_STRING_KEYS);

        (void) Tcl_SetAssocData(interp, "tclIO",
                (Tcl_InterpDeleteProc *) DeleteChannelTable,
                (ClientData) hTblPtr);

        /*
         * If the interpreter is trusted (not "safe"), insert channels
         * for stdin, stdout and stderr (possibly creating them in the
         * process).
         */

        if (Tcl_IsSafe(interp) == 0) {
            stdinChan = Tcl_GetStdChannel(TCL_STDIN);
            if (stdinChan != NULL) {
                Tcl_RegisterChannel(interp, stdinChan);
            }
            stdoutChan = Tcl_GetStdChannel(TCL_STDOUT);
            if (stdoutChan != NULL) {
                Tcl_RegisterChannel(interp, stdoutChan);
            }
            stderrChan = Tcl_GetStdChannel(TCL_STDERR);
            if (stderrChan != NULL) {
                Tcl_RegisterChannel(interp, stderrChan);
            }
        }

    }
    return hTblPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteChannelTable --
 *
 *	Deletes the channel table for an interpreter, closing any open
 *	channels whose refcount reaches zero. This procedure is invoked
 *	when an interpreter is deleted, via the AssocData cleanup
 *	mechanism.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes the hash table of channels. May close channels. May flush
 *	output on closed channels. Removes any channeEvent handlers that were
 *	registered in this interpreter.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteChannelTable(clientData, interp)
    ClientData clientData;	/* The per-interpreter data structure. */
    Tcl_Interp *interp;		/* The interpreter being deleted. */
{
    Tcl_HashTable *hTblPtr;	/* The hash table. */
    Tcl_HashSearch hSearch;	/* Search variable. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    Channel *chanPtr;	/* Channel being deleted. */
    EventScriptRecord *sPtr, *prevPtr, *nextPtr;
    				/* Variables to loop over all channel events
                                 * registered, to delete the ones that refer
                                 * to the interpreter being deleted. */
    
    /*
     * Delete all the registered channels - this will close channels whose
     * refcount reaches zero.
     */
    
    hTblPtr = (Tcl_HashTable *) clientData;
    for (hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch);
             hPtr != (Tcl_HashEntry *) NULL;
             hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch)) {

        chanPtr = (Channel *) Tcl_GetHashValue(hPtr);

        /*
         * Remove any fileevents registered in this interpreter.
         */
        
        for (sPtr = chanPtr->scriptRecordPtr,
                 prevPtr = (EventScriptRecord *) NULL;
                 sPtr != (EventScriptRecord *) NULL;
                 sPtr = nextPtr) {
            nextPtr = sPtr->nextPtr;
            if (sPtr->interp == interp) {
                if (prevPtr == (EventScriptRecord *) NULL) {
                    chanPtr->scriptRecordPtr = nextPtr;
                } else {
                    prevPtr->nextPtr = nextPtr;
                }

                Tcl_DeleteChannelHandler((Tcl_Channel) chanPtr,
                        ChannelEventScriptInvoker, (ClientData) sPtr);

		Tcl_DecrRefCount(sPtr->scriptPtr);
                ckfree((char *) sPtr);
            } else {
                prevPtr = sPtr;
            }
        }

        /*
         * Cannot call Tcl_UnregisterChannel because that procedure calls
         * Tcl_GetAssocData to get the channel table, which might already
         * be inaccessible from the interpreter structure. Instead, we
         * emulate the behavior of Tcl_UnregisterChannel directly here.
         */

        Tcl_DeleteHashEntry(hPtr);
        chanPtr->refCount--;
        if (chanPtr->refCount <= 0) {
            if (!(chanPtr->flags & BG_FLUSH_SCHEDULED)) {
                (void) Tcl_Close(interp, (Tcl_Channel) chanPtr);
            }
        }
    }
    Tcl_DeleteHashTable(hTblPtr);
    ckfree((char *) hTblPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * CheckForStdChannelsBeingClosed --
 *
 *	Perform special handling for standard channels being closed. When
 *	given a standard channel, if the refcount is now 1, it means that
 *	the last reference to the standard channel is being explicitly
 *	closed. Now bump the refcount artificially down to 0, to ensure the
 *	normal handling of channels being closed will occur. Also reset the
 *	static pointer to the channel to NULL, to avoid dangling references.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Manipulates the refcount on standard channels. May smash the global
 *	static pointer to a standard channel.
 *
 *----------------------------------------------------------------------
 */

static void
CheckForStdChannelsBeingClosed(chan)
    Tcl_Channel chan;
{
    Channel *chanPtr = (Channel *) chan;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if ((chan == tsdPtr->stdinChannel) && (tsdPtr->stdinInitialized)) {
        if (chanPtr->refCount < 2) {
            chanPtr->refCount = 0;
            tsdPtr->stdinChannel = NULL;
            return;
        }
    } else if ((chan == tsdPtr->stdoutChannel) && (tsdPtr->stdoutInitialized)) {
        if (chanPtr->refCount < 2) {
            chanPtr->refCount = 0;
            tsdPtr->stdoutChannel = NULL;
            return;
        }
    } else if ((chan == tsdPtr->stderrChannel) && (tsdPtr->stderrInitialized)) {
        if (chanPtr->refCount < 2) {
            chanPtr->refCount = 0;
            tsdPtr->stderrChannel = NULL;
            return;
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RegisterChannel --
 *
 *	Adds an already-open channel to the channel table of an interpreter.
 *	If the interpreter passed as argument is NULL, it only increments
 *	the channel refCount.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May increment the reference count of a channel.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_RegisterChannel(interp, chan)
    Tcl_Interp *interp;		/* Interpreter in which to add the channel. */
    Tcl_Channel chan;		/* The channel to add to this interpreter
                                 * channel table. */
{
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    int new;			/* Is the hash entry new or does it exist? */
    Channel *chanPtr;		/* The actual channel. */

    chanPtr = (Channel *) chan;

    if (chanPtr->channelName == (char *) NULL) {
        panic("Tcl_RegisterChannel: channel without name");
    }
    if (interp != (Tcl_Interp *) NULL) {
        hTblPtr = GetChannelTable(interp);
        hPtr = Tcl_CreateHashEntry(hTblPtr, chanPtr->channelName, &new);
        if (new == 0) {
            if (chan == (Tcl_Channel) Tcl_GetHashValue(hPtr)) {
                return;
            }

	    /* Andreas Kupries <a.kupries@westend.com>, 12/13/1998
	     * "Trf-Patch for filtering channels"
	     *
	     * This is the change to 'Tcl_RegisterChannel'.
	     *
	     * Explanation:
	     *		The moment a channel is stacked upon another he
	     *		takes the identity of the channel he supercedes,
	     *		i.e. he gets the *same* name. Because of this we
	     *		cannot check for duplicate names anymore, they
	     *		have to be allowed now.
	     */

	    /* panic("Tcl_RegisterChannel: duplicate channel names"); */
        }
        Tcl_SetHashValue(hPtr, (ClientData) chanPtr);
    }
    chanPtr->refCount++;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UnregisterChannel --
 *
 *	Deletes the hash entry for a channel associated with an interpreter.
 *	If the interpreter given as argument is NULL, it only decrements the
 *	reference count.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Deletes the hash entry for a channel associated with an interpreter.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UnregisterChannel(interp, chan)
    Tcl_Interp *interp;		/* Interpreter in which channel is defined. */
    Tcl_Channel chan;		/* Channel to delete. */
{
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    Channel *chanPtr;		/* The real IO channel. */

    chanPtr = (Channel *) chan;
    
    if (interp != (Tcl_Interp *) NULL) {
        hTblPtr = (Tcl_HashTable *) Tcl_GetAssocData(interp, "tclIO", NULL);
        if (hTblPtr == (Tcl_HashTable *) NULL) {
            return TCL_OK;
        }
        hPtr = Tcl_FindHashEntry(hTblPtr, chanPtr->channelName);
        if (hPtr == (Tcl_HashEntry *) NULL) {
            return TCL_OK;
        }
        if ((Channel *) Tcl_GetHashValue(hPtr) != chanPtr) {
            return TCL_OK;
        }
        Tcl_DeleteHashEntry(hPtr);

        /*
         * Remove channel handlers that refer to this interpreter, so that they
         * will not be present if the actual close is delayed and more events
         * happen on the channel. This may occur if the channel is shared
         * between several interpreters, or if the channel has async
         * flushing active.
         */
    
        CleanupChannelHandlers(interp, chanPtr);
    }

    chanPtr->refCount--;
    
    /*
     * Perform special handling for standard channels being closed. If the
     * refCount is now 1 it means that the last reference to the standard
     * channel is being explicitly closed, so bump the refCount down
     * artificially to 0. This will ensure that the channel is actually
     * closed, below. Also set the static pointer to NULL for the channel.
     */

    CheckForStdChannelsBeingClosed(chan);

    /*
     * If the refCount reached zero, close the actual channel.
     */

    if (chanPtr->refCount <= 0) {

        /*
         * Ensure that if there is another buffer, it gets flushed
         * whether or not we are doing a background flush.
         */

        if ((chanPtr->curOutPtr != NULL) &&
                (chanPtr->curOutPtr->nextAdded >
                        chanPtr->curOutPtr->nextRemoved)) {
            chanPtr->flags |= BUFFER_READY;
        }
        chanPtr->flags |= CHANNEL_CLOSED;
        if (!(chanPtr->flags & BG_FLUSH_SCHEDULED)) {
            if (Tcl_Close(interp, chan) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_GetChannel --
 *
 *	Finds an existing Tcl_Channel structure by name in a given
 *	interpreter. This function is public because it is used by
 *	channel-type-specific functions.
 *
 * Results:
 *	A Tcl_Channel or NULL on failure. If failed, interp's result
 *	object contains an error message.  *modePtr is filled with the
 *	modes in which the channel was opened.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Channel
Tcl_GetChannel(interp, chanName, modePtr)
    Tcl_Interp *interp;		/* Interpreter in which to find or create
                                 * the channel. */
    char *chanName;		/* The name of the channel. */
    int *modePtr;		/* Where to store the mode in which the
                                 * channel was opened? Will contain an ORed
                                 * combination of TCL_READABLE and
                                 * TCL_WRITABLE, if non-NULL. */
{
    Channel *chanPtr;		/* The actual channel. */
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    char *name;			/* Translated name. */

    /*
     * Substitute "stdin", etc.  Note that even though we immediately
     * find the channel using Tcl_GetStdChannel, we still need to look
     * it up in the specified interpreter to ensure that it is present
     * in the channel table.  Otherwise, safe interpreters would always
     * have access to the standard channels.
     */

    name = chanName;
    if ((chanName[0] == 's') && (chanName[1] == 't')) {
	chanPtr = NULL;
	if (strcmp(chanName, "stdin") == 0) {
	    chanPtr = (Channel *)Tcl_GetStdChannel(TCL_STDIN);
	} else if (strcmp(chanName, "stdout") == 0) {
	    chanPtr = (Channel *)Tcl_GetStdChannel(TCL_STDOUT);
	} else if (strcmp(chanName, "stderr") == 0) {
	    chanPtr = (Channel *)Tcl_GetStdChannel(TCL_STDERR);
	}
	if (chanPtr != NULL) {
	    name = chanPtr->channelName;
	}
    }
    
    hTblPtr = GetChannelTable(interp);
    hPtr = Tcl_FindHashEntry(hTblPtr, name);
    if (hPtr == (Tcl_HashEntry *) NULL) {
        Tcl_AppendResult(interp, "can not find channel named \"",
                chanName, "\"", (char *) NULL);
        return NULL;
    }

    chanPtr = (Channel *) Tcl_GetHashValue(hPtr);
    if (modePtr != NULL) {
        *modePtr = (chanPtr->flags & (TCL_READABLE|TCL_WRITABLE));
    }
    
    return (Tcl_Channel) chanPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateChannel --
 *
 *	Creates a new entry in the hash table for a Tcl_Channel
 *	record.
 *
 * Results:
 *	Returns the new Tcl_Channel.
 *
 * Side effects:
 *	Creates a new Tcl_Channel instance and inserts it into the
 *	hash table.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_CreateChannel(typePtr, chanName, instanceData, mask)
    Tcl_ChannelType *typePtr;	/* The channel type record. */
    char *chanName;		/* Name of channel to record. */
    ClientData instanceData;	/* Instance specific data. */
    int mask;			/* TCL_READABLE & TCL_WRITABLE to indicate
                                 * if the channel is readable, writable. */
{
    Channel *chanPtr;		/* The channel structure newly created. */
    CONST char *name;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    chanPtr = (Channel *) ckalloc((unsigned) sizeof(Channel));
    
    if (chanName != (char *) NULL) {
        chanPtr->channelName = ckalloc((unsigned) (strlen(chanName) + 1));
        strcpy(chanPtr->channelName, chanName);
    } else {
        panic("Tcl_CreateChannel: NULL channel name");
    }

    chanPtr->flags = mask;

    /*
     * Set the channel to system default encoding.
     */

    chanPtr->encoding = NULL;
    name = Tcl_GetEncodingName(NULL);
    if (strcmp(name, "binary") != 0) {
    	chanPtr->encoding = Tcl_GetEncoding(NULL, name);
    }
    chanPtr->inputEncodingState = NULL;
    chanPtr->inputEncodingFlags = TCL_ENCODING_START;
    chanPtr->outputEncodingState = NULL;
    chanPtr->outputEncodingFlags = TCL_ENCODING_START;

    /*
     * Set the channel up initially in AUTO input translation mode to
     * accept "\n", "\r" and "\r\n". Output translation mode is set to
     * a platform specific default value. The eofChar is set to 0 for both
     * input and output, so that Tcl does not look for an in-file EOF
     * indicator (e.g. ^Z) and does not append an EOF indicator to files.
     */

    chanPtr->inputTranslation = TCL_TRANSLATE_AUTO;
    chanPtr->outputTranslation = TCL_PLATFORM_TRANSLATION;
    chanPtr->inEofChar = 0;
    chanPtr->outEofChar = 0;

    chanPtr->unreportedError = 0;
    chanPtr->instanceData = instanceData;
    chanPtr->typePtr = typePtr;
    chanPtr->refCount = 0;
    chanPtr->closeCbPtr = (CloseCallback *) NULL;
    chanPtr->curOutPtr = (ChannelBuffer *) NULL;
    chanPtr->outQueueHead = (ChannelBuffer *) NULL;
    chanPtr->outQueueTail = (ChannelBuffer *) NULL;
    chanPtr->saveInBufPtr = (ChannelBuffer *) NULL;
    chanPtr->inQueueHead = (ChannelBuffer *) NULL;
    chanPtr->inQueueTail = (ChannelBuffer *) NULL;
    chanPtr->chPtr = (ChannelHandler *) NULL;
    chanPtr->interestMask = 0;
    chanPtr->scriptRecordPtr = (EventScriptRecord *) NULL;
    chanPtr->bufSize = CHANNELBUFFER_DEFAULT_SIZE;
    chanPtr->timer = NULL;
    chanPtr->csPtr = NULL;
    chanPtr->supercedes = (Channel*) NULL;

    chanPtr->outputStage = NULL;
    if ((chanPtr->encoding != NULL) && (chanPtr->flags & TCL_WRITABLE)) {
	chanPtr->outputStage = (char *)
		ckalloc((unsigned) (chanPtr->bufSize + 2));
    }

    /*
     * Link the channel into the list of all channels; create an on-exit
     * handler if there is not one already, to close off all the channels
     * in the list on exit.
     */

    chanPtr->nextChanPtr = tsdPtr->firstChanPtr;
    tsdPtr->firstChanPtr = chanPtr;

    /*
     * Install this channel in the first empty standard channel slot, if
     * the channel was previously closed explicitly.
     */

    if ((tsdPtr->stdinChannel == NULL) && (tsdPtr->stdinInitialized == 1)) {
	Tcl_SetStdChannel((Tcl_Channel)chanPtr, TCL_STDIN);
        Tcl_RegisterChannel((Tcl_Interp *) NULL, (Tcl_Channel) chanPtr);
    } else if ((tsdPtr->stdoutChannel == NULL) && (tsdPtr->stdoutInitialized == 1)) {
	Tcl_SetStdChannel((Tcl_Channel)chanPtr, TCL_STDOUT);
        Tcl_RegisterChannel((Tcl_Interp *) NULL, (Tcl_Channel) chanPtr);
    } else if ((tsdPtr->stderrChannel == NULL) && (tsdPtr->stderrInitialized == 1)) {
	Tcl_SetStdChannel((Tcl_Channel)chanPtr, TCL_STDERR);
        Tcl_RegisterChannel((Tcl_Interp *) NULL, (Tcl_Channel) chanPtr);
    } 
    return (Tcl_Channel) chanPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_StackChannel --
 *
 *	Replaces an entry in the hash table for a Tcl_Channel
 *	record. The replacement is a new channel with same name,
 *	it supercedes the replaced channel. Input and output of
 *	the superceded channel is now going through the newly
 *	created channel and allows the arbitrary filtering/manipulation
 *	of the dataflow.
 *
 *	Andreas Kupries <a.kupries@westend.com>, 12/13/1998
 *	"Trf-Patch for filtering channels"
 *
 * Results:
 *	Returns the new Tcl_Channel, which actually contains the
 *      saved information about prevChan.
 *
 * Side effects:
 *    A new channel structure is allocated and linked below
 *    the existing channel.  The channel operations and client
 *    data of the existing channel are copied down to the newly
 *    created channel, and the current channel has its operations
 *    replaced by the new typePtr.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_StackChannel(interp, typePtr, instanceData, mask, prevChan)
    Tcl_Interp*      interp;       /* The interpreter we are working in */
    Tcl_ChannelType *typePtr;	   /* The channel type record for the new
				    * channel. */
    ClientData       instanceData; /* Instance specific data for the new
				    * channel. */
    int              mask;	   /* TCL_READABLE & TCL_WRITABLE to indicate
				    * if the channel is readable, writable. */
    Tcl_Channel      prevChan;	   /* The channel structure to replace */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    Channel            *chanPtr, *pt;
    int                 interest = 0;

    /*
     * AK, 06/30/1999
     *
     * Tcl_StackChannel differs from Tcl_ReplaceChannel of the
     * original "Trf" patch. Instead of seeing the
     * newly created structure as the *new* channel to cover the specified
     * one use it to *save* the current state of the specified channel and
     * then reinitialize the current structure for the given transformation.
     *
     * Advantages:
     * - No splicing into the (thread-)global list of channels (or the per-
     *   interp hash-tables).
     * - Users of the C-API still have valid channel references even after
     *   the call to this procedure.
     *
     * Disadvantages:
     * - Untested code.
     */

    /*
     * Find the given channel in the list of all channels.
     */

    pt     = (Channel*) tsdPtr->firstChanPtr;

    while (pt != (Channel *) prevChan) {
	pt = pt->nextChanPtr;
    }

    /*
     * 'pt == prevChan' now (or NULL, if not found).
     */

    if (!pt) {
        return (Tcl_Channel) NULL;
    }

    /*
     * Here we check if the given "mask" matches the "flags"
     * of the already existing channel.
     *
     *	  | - | R | W | RW |
     *	--+---+---+---+----+	<=>  0 != (chan->mask & prevChan->mask)
     *	- |   |   |   |    |
     *	R |   | + |   | +  |	The superceding channel is allowed to
     *	W |   |   | + | +  |	restrict the capabilities of the
     *	RW|   | + | + | +  |	superceded one !
     *	--+---+---+---+----+
     */

    if ((mask & Tcl_GetChannelMode (prevChan)) == 0) {
        return (Tcl_Channel) NULL;
    }

    chanPtr = (Channel *) ckalloc((unsigned) sizeof(Channel));

    /*
     * If there is some interest in the channel, remove it, break
     * down the whole chain. It will be reconstructed later.
     */

    interest = pt->interestMask;

    pt->interestMask = 0;

    if (interest) {
        (pt->typePtr->watchProc) (pt->instanceData, 0);
    }

    /*
     * Save some of the current state into the new structure,
     * reinitialize the parts which will stay with the transformation.
     *
     * Remarks:
     * - We cannot discard the buffers, and they cannot be used from the
     *   transformation placed later into the 'pt' structure. Save them,
     *   and believe that Tcl_SetChannelOption (buffering, none) will do
     *   the right thing.
     * - encoding and EOL-translation control information is initialized
     *   to values for 'binary'. This is later reinforced via
     *   Tcl_SetChanneloption to get the handling of flags and the event
     *   system right.
     * - The 'interestMask' of the saved channel is cleared, but the
     *   transformations WatchProc is used to establish the connection
     *   between transformation and underlying channel. This should
     *   reestablish the correct mask.
     * - TTO = Transform Takes Over.   The hidden channel no longer
     *         needs to perform this function.
     */

    chanPtr->channelName = (char *) ckalloc (strlen(pt->channelName)+1);
    strcpy (chanPtr->channelName, pt->channelName);

    chanPtr->flags               = pt->flags;           /* Save */

    chanPtr->encoding            = (Tcl_Encoding) NULL; /* == 'binary' */
    chanPtr->inputEncodingState  = (Tcl_EncodingState) NULL;
    chanPtr->inputEncodingFlags  = TCL_ENCODING_START;
    chanPtr->outputEncodingState = (Tcl_EncodingState) NULL;
    chanPtr->outputEncodingFlags = TCL_ENCODING_START;

    chanPtr->inputTranslation    = TCL_TRANSLATE_LF; /* == 'binary' */
    chanPtr->outputTranslation   = TCL_TRANSLATE_LF; /* == 'binary' */
    chanPtr->inEofChar           = pt->inEofChar;         /* Save */
    chanPtr->outEofChar          = pt->outEofChar;        /* Save */

    chanPtr->unreportedError     = pt->unreportedError;   /* Save */
    chanPtr->instanceData        = pt->instanceData;      /* Save */
    chanPtr->typePtr             = pt->typePtr;           /* Save */
    chanPtr->refCount            = 0;   /* None, as the structure is covered */
    chanPtr->closeCbPtr          = (CloseCallback*) NULL; /* TTO */

    chanPtr->outputStage         = (char*) NULL;
    chanPtr->curOutPtr           = pt->curOutPtr;    /* Save */
    chanPtr->outQueueHead        = pt->outQueueHead; /* Save */
    chanPtr->outQueueTail        = pt->outQueueTail; /* Save */
    chanPtr->saveInBufPtr        = pt->saveInBufPtr; /* Save */
    chanPtr->inQueueHead         = pt->inQueueHead;  /* Save */
    chanPtr->inQueueTail         = pt->inQueueTail;  /* Save */

    chanPtr->chPtr               = (ChannelHandler *) NULL;  /* TTO */
    chanPtr->interestMask        = 0;
    chanPtr->nextChanPtr         = (Channel*) NULL;     /* Is not in list! */
    chanPtr->scriptRecordPtr     = (EventScriptRecord *) NULL; /* TTO */
    chanPtr->bufSize             = CHANNELBUFFER_DEFAULT_SIZE;
    chanPtr->timer               = (Tcl_TimerToken) NULL;      /* TTO */
    chanPtr->csPtr               = (CopyState*) NULL;          /* TTO */

    /*
     * Place new block at the head of a possibly existing list of previously
     * stacked channels, then do the missing initializations of translation
     * and buffer system.
     */

    chanPtr->supercedes          = pt->supercedes;

    Tcl_SetChannelOption (interp, (Tcl_Channel) chanPtr,
	"-translation", "binary");
    Tcl_SetChannelOption (interp, (Tcl_Channel) chanPtr,
	"-buffering",   "none");

    /*
     * Save accomplished, now reinitialize the (old) structure for the
     * transformation.
     *
     * - The information about encoding and eol-translation is taken
     *   without change.  There is no need to fiddle with
     *   refCount et. al.
     *
     * Don't forget to use the same blocking mode as the old channel.
     */

    pt->flags               = mask | (chanPtr->flags & CHANNEL_NONBLOCKING);

    /*
     * EDITORS NOTE:  all the lines with "take it as is" should get
     * deleted once this code has been debugged.
     */

    /* pt->encoding,            take it as is */
    /* pt->inputEncodingState,  take it as is */
    /* pt->inputEncodingFlags,  take it as is */
    /* pt->outputEncodingState, take it as is */
    /* pt->outputEncodingFlags, take it as is */

    /* pt->inputTranslation,    take it as is */
    /* pt->outputTranslation,   take it as is */

    /*
     * No special EOF character, that condition is determined by the
     * old channel
     */

    pt->inEofChar           = 0;
    pt->outEofChar          = 0;

    pt->unreportedError     = 0; /* No errors yet */
    pt->instanceData        = instanceData; /* Transformation state */
    pt->typePtr             = typePtr;      /* Transformation type */
    /* pt->refCount,            take it as it is */
    /* pt->closeCbPtr,          take it as it is */

    /* pt->outputStage,         take it as it is */
    pt->curOutPtr           = (ChannelBuffer *) NULL;
    pt->outQueueHead        = (ChannelBuffer *) NULL;
    pt->outQueueTail        = (ChannelBuffer *) NULL;
    pt->saveInBufPtr        = (ChannelBuffer *) NULL;
    pt->inQueueHead         = (ChannelBuffer *) NULL;
    pt->inQueueTail         = (ChannelBuffer *) NULL;

    /* pt->chPtr,               take it as it is */
    /* pt->interestMask,        take it as it is */
    /* pt->nextChanPtr,         take it as it is */
    /* pt->scriptRecordPtr,     take it as it is */
    pt->bufSize             = CHANNELBUFFER_DEFAULT_SIZE;
    /* pt->timer,               take it as it is */
    /* pt->csPtr,               take it as it is */

    /*
     * Have the transformation reference the new structure containing
     * the saved channel.
     */

    pt->supercedes          = chanPtr;

    /*
     * Don't forget to reinitialize the output buffer used for encodings.
     */

    if ((chanPtr->encoding != NULL) && (chanPtr->flags & TCL_WRITABLE)) {
        chanPtr->outputStage = (char *)
	    ckalloc((unsigned) (chanPtr->bufSize + 2));
    }

    /*
     * Event handling: If the information in the old channel shows
     * that there was interest in some events call the 'WatchProc'
     * of the transformation to establish the proper connection
     * between them.
     */

    if (interest) {
        (pt->typePtr->watchProc) (pt->instanceData, interest);
    }

    /*
     * The superceded channel is effectively unregistered
     * We cannot decrement its reference count because that
     * can cause it to get garbage collected out from under us.
     * Don't add the following code:
     *
     * chanPtr->supercedes->refCount --;
     */

    return (Tcl_Channel) chanPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UnstackChannel --
 *
 *	Unstacks an entry in the hash table for a Tcl_Channel
 *	record. This is the reverse to 'Tcl_StackChannel'.
 *	The old, superceded channel is uncovered and re-registered
 *	in the appropriate data structures.
 *
 * Results:
 *	Returns the old Tcl_Channel, i.e. the one which was stacked over.
 *
 * Side effects:
 *	See above.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_UnstackChannel (interp, chan)
    Tcl_Interp* interp; /* The interpreter we are working in */
    Tcl_Channel chan;   /* The channel to unstack */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    Channel* chanPtr = (Channel*) chan;

    if (chanPtr->supercedes != (Channel*) NULL) {
        /*
	 * Instead of manipulating the per-thread / per-interp list/hashtable
	 * of registered channels we wind down the state of the transformation,
	 * and then restore the state of underlying channel into the old
	 * structure.
	 */

	Tcl_DString ds; /* storage to copy option information */
        Channel    top; /* Save area for current transformation */
	Channel*   chanDownPtr = chanPtr->supercedes;
	int interest;   /* interest mask of transformation before destruct. */

	/*
	 * Event handling: Disallow the delivery of events from the
	 * old, now uncovered channel to the transformation.
	 *
	 * This is done before everything else to avoid problems
	 * after our heavy-duty shuffling of pointers around.
	 */

	interest = chanPtr->interestMask;
        (chanPtr->typePtr->watchProc) (chanPtr->instanceData, 0);

	/*
	 * Save the transformation, then restore the old channel from the
	 * first structure downstream. The overall effect is that
	 * transformation and underlying channel swapped places, and the
	 * transformation is cut loose from the stack. Without the latter
	 * a Tcl_Close on the transformation would be impossible, as that
	 * procedure will free the structure, making 'top' unusable.
	 *
	 * Beware, same information must not take part in the swap,
	 * use correction code to ensure this.
	 */

	memcpy ((void*) &top,        (void*) chanPtr,     sizeof (Channel));
	memcpy ((void*) chanPtr,     (void*) chanDownPtr, sizeof (Channel));
	top.supercedes = (Channel*) NULL;
	memcpy ((void*) chanDownPtr, (void*) &top,        sizeof (Channel));

	chanPtr->refCount        = chanDownPtr->refCount;
	chanPtr->closeCbPtr      = chanDownPtr->closeCbPtr;
	chanPtr->chPtr           = chanDownPtr->chPtr;
	chanPtr->nextChanPtr     = chanDownPtr->nextChanPtr;
	chanPtr->scriptRecordPtr = chanDownPtr->scriptRecordPtr;
	chanPtr->timer           = chanDownPtr->timer;
	chanPtr->csPtr           = chanDownPtr->csPtr;

	chanDownPtr->refCount        = 0;
	chanDownPtr->closeCbPtr      = (CloseCallback*) NULL;
	chanDownPtr->chPtr           = (ChannelHandler*) NULL;
	chanDownPtr->nextChanPtr     = (Channel*) NULL;
	chanDownPtr->scriptRecordPtr = (EventScriptRecord*) NULL;
	chanDownPtr->timer           = (Tcl_TimerToken) NULL;
	chanDownPtr->csPtr           = (CopyState*) NULL;

	/*
	 * Now it is possible to wind down the transformation (in 'top'),
	 * especially to copy the current encoding and translation control
	 * information down.
	 */
	
	/*
	 * Move the currently active encoding from the transformation
	 * to the now uncovered channel. We assume here that this
	 * channel uses 'encoding binary' (==> encoding == NULL, etc.
	 * This allows us to simply copy the pointers without having to
	 * think about refcounts and deallocation of the old encoding.
	 */

	chanPtr->encoding            = chanDownPtr->encoding;
	chanPtr->inputEncodingState  = chanDownPtr->inputEncodingState;
	chanPtr->inputEncodingFlags  = chanDownPtr->inputEncodingFlags;
	chanPtr->outputEncodingState = chanDownPtr->outputEncodingState;
	chanPtr->outputEncodingFlags = chanDownPtr->outputEncodingFlags;

	/*
	 * Prevent the accidential removal of the encoding during
	 * the destruction of the transformation channel.
	 */

	chanDownPtr->encoding            = (Tcl_Encoding) NULL;
	chanDownPtr->inputEncodingState  = (Tcl_EncodingState) NULL;
	chanDownPtr->inputEncodingFlags  = TCL_ENCODING_START;
	chanDownPtr->outputEncodingState = (Tcl_EncodingState) NULL;
	chanDownPtr->outputEncodingFlags = TCL_ENCODING_START;

	/*
	 * And don't forget to reenable the EOL-translation used by the
	 * transformation. Using a DString to do this *is* a bit awkward,
	 * but still the best way to handle the complexities here, like
	 * flag manipulation and event system.
	 */

	Tcl_DStringInit (&ds);
	Tcl_GetChannelOption (interp, (Tcl_Channel) chanDownPtr,
            "-translation", &ds);
	Tcl_SetChannelOption (interp, (Tcl_Channel) chanPtr,
	    "-translation", ds.string);

	Tcl_DStringSetLength (&ds, 0);

	Tcl_GetChannelOption (interp, (Tcl_Channel) chanDownPtr,
	    "-buffering", &ds);
	Tcl_SetChannelOption (interp, (Tcl_Channel) chanPtr,
	    "-buffering", ds.string);

	Tcl_DStringFree (&ds);

	/*
	 * Now a little trick: Add the transformation structure to the
	 * per-thread list of existing channels (which it never were
	 * part of so far), or Tcl_Close/FlushChannel will panic
	 * ("damaged channel list").
	 *
	 * Afterward do a regular close upon the transformation.
	 * This may cause flushing of data into the old channel (if the
	 * transformation remembered its own channel in itself).
	 *
	 * We know that its refCount dropped to 0.
	 */

	chanDownPtr->nextChanPtr = tsdPtr->firstChanPtr;
	tsdPtr->firstChanPtr     = chanDownPtr;

	Tcl_Close (interp, (Tcl_Channel)chanDownPtr);

	/*
	 * Event handling: If the information from the now destroyed
	 * transformation shows that there was interest in some events
	 * call the 'WatchProc' of the now uncovered channel to renew
	 * that interest with underlying channels or the driver.
	 */

	if (interest) {
	    chanPtr->interestMask = 0;
	    (chanPtr->typePtr->watchProc) (chanPtr->instanceData,
		interest);
	}

    } else {
        /* This channel does not cover another one.
	 * Simply do a close, if necessary.
	 */

        if (chanPtr->refCount == 0) {
	    Tcl_Close (interp, chan);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetStackedChannel --
 *
 *	Determines wether the specified channel is stacked upon another.
 *
 * Results:
 *	NULL if the channel is not stacked upon another one, or a reference
 *	to the channel it is stacked upon. This reference can be used in
 *	queries, but modification is not allowed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_GetStackedChannel(chan)
    Tcl_Channel chan;
{
  Channel* chanPtr = (Channel*) chan;
  return (Tcl_Channel) chanPtr->supercedes;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelMode --
 *
 *	Computes a mask indicating whether the channel is open for
 *	reading and writing.
 *
 * Results:
 *	An OR-ed combination of TCL_READABLE and TCL_WRITABLE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetChannelMode(chan)
    Tcl_Channel chan;		/* The channel for which the mode is
                                 * being computed. */
{
    Channel *chanPtr;		/* The actual channel. */

    chanPtr = (Channel *) chan;
    return (chanPtr->flags & (TCL_READABLE | TCL_WRITABLE));
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelName --
 *
 *	Returns the string identifying the channel name.
 *
 * Results:
 *	The string containing the channel name. This memory is
 *	owned by the generic layer and should not be modified by
 *	the caller.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_GetChannelName(chan)
    Tcl_Channel chan;		/* The channel for which to return the name. */
{
    Channel *chanPtr;		/* The actual channel. */

    chanPtr = (Channel *) chan;
    return chanPtr->channelName;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelType --
 *
 *	Given a channel structure, returns the channel type structure.
 *
 * Results:
 *	Returns a pointer to the channel type structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_ChannelType *
Tcl_GetChannelType(chan)
    Tcl_Channel chan;		/* The channel to return type for. */
{
    Channel *chanPtr;		/* The actual channel. */

    chanPtr = (Channel *) chan;
    return chanPtr->typePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelHandle --
 *
 *	Returns an OS handle associated with a channel.
 *
 * Results:
 *	Returns TCL_OK and places the handle in handlePtr, or returns
 *	TCL_ERROR on failure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetChannelHandle(chan, direction, handlePtr)
    Tcl_Channel chan;		/* The channel to get file from. */
    int direction;		/* TCL_WRITABLE or TCL_READABLE. */
    ClientData *handlePtr;	/* Where to store handle */
{
    Channel *chanPtr;		/* The actual channel. */
    ClientData handle;
    int result;

    chanPtr = (Channel *) chan;
    result = (chanPtr->typePtr->getHandleProc)(chanPtr->instanceData,
	    direction, &handle);
    if (handlePtr) {
	*handlePtr = handle;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelInstanceData --
 *
 *	Returns the client data associated with a channel.
 *
 * Results:
 *	The client data.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ClientData
Tcl_GetChannelInstanceData(chan)
    Tcl_Channel chan;		/* Channel for which to return client data. */
{
    Channel *chanPtr;		/* The actual channel. */

    chanPtr = (Channel *) chan;
    return chanPtr->instanceData;
}

/*
 *---------------------------------------------------------------------------
 *
 * AllocChannelBuffer --
 *
 *	A channel buffer has BUFFER_PADDING bytes extra at beginning to
 *	hold any bytes of a native-encoding character that got split by
 *	the end of the previous buffer and need to be moved to the
 *	beginning of the next buffer to make a contiguous string so it
 *	can be converted to UTF-8.
 *
 *	A channel buffer has BUFFER_PADDING bytes extra at the end to
 *	hold any bytes of a native-encoding character (generated from a
 *	UTF-8 character) that overflow past the end of the buffer and
 *	need to be moved to the next buffer.
 *
 * Results:
 *	A newly allocated channel buffer.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static ChannelBuffer *
AllocChannelBuffer(length)
    int length;			/* Desired length of channel buffer. */
{
    ChannelBuffer *bufPtr;
    int n;

    n = length + CHANNELBUFFER_HEADER_SIZE + BUFFER_PADDING + BUFFER_PADDING;
    bufPtr = (ChannelBuffer *) ckalloc((unsigned) n);
    bufPtr->nextAdded	= BUFFER_PADDING;
    bufPtr->nextRemoved	= BUFFER_PADDING;
    bufPtr->bufLength	= length + BUFFER_PADDING;
    bufPtr->nextPtr	= (ChannelBuffer *) NULL;
    return bufPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * RecycleBuffer --
 *
 *	Helper function to recycle input and output buffers. Ensures
 *	that two input buffers are saved (one in the input queue and
 *	another in the saveInBufPtr field) and that curOutPtr is set
 *	to a buffer. Only if these conditions are met is the buffer
 *	freed to the OS.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May free a buffer to the OS.
 *
 *----------------------------------------------------------------------
 */

static void
RecycleBuffer(chanPtr, bufPtr, mustDiscard)
    Channel *chanPtr;		/* Channel for which to recycle buffers. */
    ChannelBuffer *bufPtr;	/* The buffer to recycle. */
    int mustDiscard;		/* If nonzero, free the buffer to the
                                 * OS, always. */
{
    /*
     * Do we have to free the buffer to the OS?
     */

    if (mustDiscard) {
        ckfree((char *) bufPtr);
        return;
    }
    
    /*
     * Only save buffers for the input queue if the channel is readable.
     */
    
    if (chanPtr->flags & TCL_READABLE) {
        if (chanPtr->inQueueHead == (ChannelBuffer *) NULL) {
            chanPtr->inQueueHead = bufPtr;
            chanPtr->inQueueTail = bufPtr;
            goto keepit;
        }
        if (chanPtr->saveInBufPtr == (ChannelBuffer *) NULL) {
            chanPtr->saveInBufPtr = bufPtr;
            goto keepit;
        }
    }

    /*
     * Only save buffers for the output queue if the channel is writable.
     */

    if (chanPtr->flags & TCL_WRITABLE) {
        if (chanPtr->curOutPtr == (ChannelBuffer *) NULL) {
            chanPtr->curOutPtr = bufPtr;
            goto keepit;
        }
    }

    /*
     * If we reached this code we return the buffer to the OS.
     */

    ckfree((char *) bufPtr);
    return;

keepit:
    bufPtr->nextRemoved = BUFFER_PADDING;
    bufPtr->nextAdded = BUFFER_PADDING;
    bufPtr->nextPtr = (ChannelBuffer *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DiscardOutputQueued --
 *
 *	Discards all output queued in the output queue of a channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Recycles buffers.
 *
 *----------------------------------------------------------------------
 */

static void
DiscardOutputQueued(chanPtr)
    Channel *chanPtr;		/* The channel for which to discard output. */
{
    ChannelBuffer *bufPtr;
    
    while (chanPtr->outQueueHead != (ChannelBuffer *) NULL) {
        bufPtr = chanPtr->outQueueHead;
        chanPtr->outQueueHead = bufPtr->nextPtr;
        RecycleBuffer(chanPtr, bufPtr, 0);
    }
    chanPtr->outQueueHead = (ChannelBuffer *) NULL;
    chanPtr->outQueueTail = (ChannelBuffer *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * CheckForDeadChannel --
 *
 *	This function checks is a given channel is Dead.
 *      (A channel that has been closed but not yet deallocated.)
 *
 * Results:
 *	True (1) if channel is Dead, False (0) if channel is Ok
 *
 * Side effects:
 *      None
 *
 *----------------------------------------------------------------------
 */

static int
CheckForDeadChannel(interp, chanPtr)
    Tcl_Interp *interp;		/* For error reporting (can be NULL) */
    Channel    *chanPtr;	/* The channel to check. */
{
    if (chanPtr->flags & CHANNEL_DEAD) {
        Tcl_SetErrno(EINVAL);
	if (interp) {
	    Tcl_AppendResult(interp,
			     "unable to access channel: invalid channel",
			     (char *) NULL);   
	}
	return 1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * FlushChannel --
 *
 *	This function flushes as much of the queued output as is possible
 *	now. If calledFromAsyncFlush is nonzero, it is being called in an
 *	event handler to flush channel output asynchronously.
 *
 * Results:
 *	0 if successful, else the error code that was returned by the
 *	channel type operation.
 *
 * Side effects:
 *	May produce output on a channel. May block indefinitely if the
 *	channel is synchronous. May schedule an async flush on the channel.
 *	May recycle memory for buffers in the output queue.
 *
 *----------------------------------------------------------------------
 */

static int
FlushChannel(interp, chanPtr, calledFromAsyncFlush)
    Tcl_Interp *interp;			/* For error reporting during close. */
    Channel *chanPtr;			/* The channel to flush on. */
    int calledFromAsyncFlush;		/* If nonzero then we are being
                                         * called from an asynchronous
                                         * flush callback. */
{
    ChannelBuffer *bufPtr;		/* Iterates over buffered output
                                         * queue. */
    int toWrite;			/* Amount of output data in current
                                         * buffer available to be written. */
    int written;			/* Amount of output data actually
                                         * written in current round. */
    int errorCode = 0;			/* Stores POSIX error codes from
                                         * channel driver operations. */
    int wroteSome = 0;			/* Set to one if any data was
					 * written to the driver. */

    /*
     * Prevent writing on a dead channel -- a channel that has been closed
     * but not yet deallocated. This can occur if the exit handler for the
     * channel deallocation runs before all channels are deregistered in
     * all interpreters.
     */
    
    if (CheckForDeadChannel(interp,chanPtr)) return -1;
    
    /*
     * Loop over the queued buffers and attempt to flush as
     * much as possible of the queued output to the channel.
     */

    while (1) {

        /*
         * If the queue is empty and there is a ready current buffer, OR if
         * the current buffer is full, then move the current buffer to the
         * queue.
         */
        
        if (((chanPtr->curOutPtr != (ChannelBuffer *) NULL) &&
                (chanPtr->curOutPtr->nextAdded == chanPtr->curOutPtr->bufLength))
                || ((chanPtr->flags & BUFFER_READY) &&
                        (chanPtr->outQueueHead == (ChannelBuffer *) NULL))) {
            chanPtr->flags &= (~(BUFFER_READY));
            chanPtr->curOutPtr->nextPtr = (ChannelBuffer *) NULL;
            if (chanPtr->outQueueHead == (ChannelBuffer *) NULL) {
                chanPtr->outQueueHead = chanPtr->curOutPtr;
            } else {
                chanPtr->outQueueTail->nextPtr = chanPtr->curOutPtr;
            }
            chanPtr->outQueueTail = chanPtr->curOutPtr;
            chanPtr->curOutPtr = (ChannelBuffer *) NULL;
        }
        bufPtr = chanPtr->outQueueHead;

        /*
         * If we are not being called from an async flush and an async
         * flush is active, we just return without producing any output.
         */

        if ((!calledFromAsyncFlush) &&
                (chanPtr->flags & BG_FLUSH_SCHEDULED)) {
            return 0;
        }

        /*
         * If the output queue is still empty, break out of the while loop.
         */

        if (bufPtr == (ChannelBuffer *) NULL) {
            break;	/* Out of the "while (1)". */
        }

        /*
         * Produce the output on the channel.
         */
        
        toWrite = bufPtr->nextAdded - bufPtr->nextRemoved;
        written = (chanPtr->typePtr->outputProc) (chanPtr->instanceData,
                (char *) bufPtr->buf + bufPtr->nextRemoved, toWrite,
		&errorCode);
            
	/*
         * If the write failed completely attempt to start the asynchronous
         * flush mechanism and break out of this loop - do not attempt to
         * write any more output at this time.
         */

        if (written < 0) {
            
            /*
             * If the last attempt to write was interrupted, simply retry.
             */
            
            if (errorCode == EINTR) {
                errorCode = 0;
                continue;
            }

            /*
             * If the channel is non-blocking and we would have blocked,
             * start a background flushing handler and break out of the loop.
             */

            if ((errorCode == EWOULDBLOCK) || (errorCode == EAGAIN)) {
		if (chanPtr->flags & CHANNEL_NONBLOCKING) {
		    if (!(chanPtr->flags & BG_FLUSH_SCHEDULED)) {
			chanPtr->flags |= BG_FLUSH_SCHEDULED;
			UpdateInterest(chanPtr);
                    }
                    errorCode = 0;
                    break;
		} else {
		    panic("Blocking channel driver did not block on output");
                }
            }

            /*
             * Decide whether to report the error upwards or defer it.
             */

            if (calledFromAsyncFlush) {
                if (chanPtr->unreportedError == 0) {
                    chanPtr->unreportedError = errorCode;
                }
            } else {
                Tcl_SetErrno(errorCode);
		if (interp != NULL) {
		    Tcl_SetResult(interp,
			    Tcl_PosixError(interp), TCL_VOLATILE);
		}
            }

            /*
             * When we get an error we throw away all the output
             * currently queued.
             */

            DiscardOutputQueued(chanPtr);
            continue;
        } else {
	    wroteSome = 1;
	}

        bufPtr->nextRemoved += written;

        /*
         * If this buffer is now empty, recycle it.
         */

        if (bufPtr->nextRemoved == bufPtr->nextAdded) {
            chanPtr->outQueueHead = bufPtr->nextPtr;
            if (chanPtr->outQueueHead == (ChannelBuffer *) NULL) {
                chanPtr->outQueueTail = (ChannelBuffer *) NULL;
            }
            RecycleBuffer(chanPtr, bufPtr, 0);
        }
    }	/* Closes "while (1)". */

    /*
     * If we wrote some data while flushing in the background, we are done.
     * We can't finish the background flush until we run out of data and
     * the channel becomes writable again.  This ensures that all of the
     * pending data has been flushed at the system level.
     */

    if (chanPtr->flags & BG_FLUSH_SCHEDULED) {
	if (wroteSome) {
	    return errorCode;
	} else if (chanPtr->outQueueHead == (ChannelBuffer *) NULL) {
	    chanPtr->flags &= (~(BG_FLUSH_SCHEDULED));
	    (chanPtr->typePtr->watchProc)(chanPtr->instanceData,
		    chanPtr->interestMask);
	}
    }

    /*
     * If the channel is flagged as closed, delete it when the refCount
     * drops to zero, the output queue is empty and there is no output
     * in the current output buffer.
     */

    if ((chanPtr->flags & CHANNEL_CLOSED) && (chanPtr->refCount <= 0) &&
            (chanPtr->outQueueHead == (ChannelBuffer *) NULL) &&
            ((chanPtr->curOutPtr == (ChannelBuffer *) NULL) ||
                    (chanPtr->curOutPtr->nextAdded ==
                            chanPtr->curOutPtr->nextRemoved))) {
        return CloseChannel(interp, chanPtr, errorCode);
    }
    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * CloseChannel --
 *
 *	Utility procedure to close a channel and free its associated
 *	resources.
 *
 * Results:
 *	0 on success or a POSIX error code if the operation failed.
 *
 * Side effects:
 *	May close the actual channel; may free memory.
 *
 *----------------------------------------------------------------------
 */

static int
CloseChannel(interp, chanPtr, errorCode)
    Tcl_Interp *interp;			/* For error reporting. */
    Channel *chanPtr;			/* The channel to close. */
    int errorCode;			/* Status of operation so far. */
{
    int result = 0;			/* Of calling driver close
                                         * operation. */
    Channel *prevChanPtr;		/* Preceding channel in list of
                                         * all channels - used to splice a
                                         * channel out of the list on close. */
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (chanPtr == NULL) {
        return result;
    }
    
    /*
     * No more input can be consumed so discard any leftover input.
     */

    DiscardInputQueued(chanPtr, 1);

    /*
     * Discard a leftover buffer in the current output buffer field.
     */

    if (chanPtr->curOutPtr != (ChannelBuffer *) NULL) {
        ckfree((char *) chanPtr->curOutPtr);
        chanPtr->curOutPtr = (ChannelBuffer *) NULL;
    }
    
    /*
     * The caller guarantees that there are no more buffers
     * queued for output.
     */

    if (chanPtr->outQueueHead != (ChannelBuffer *) NULL) {
        panic("TclFlush, closed channel: queued output left");
    }

    /*
     * If the EOF character is set in the channel, append that to the
     * output device.
     */

    if ((chanPtr->outEofChar != 0) && (chanPtr->flags & TCL_WRITABLE)) {
        int dummy;
        char c;

        c = (char) chanPtr->outEofChar;
        (chanPtr->typePtr->outputProc) (chanPtr->instanceData, &c, 1, &dummy);
    }

    /*
     * Remove TCL_READABLE and TCL_WRITABLE from chanPtr->flags, so
     * that close callbacks can not do input or output (assuming they
     * squirreled the channel away in their clientData). This also
     * prevents infinite loops if the callback calls any C API that
     * could call FlushChannel.
     */

    chanPtr->flags &= (~(TCL_READABLE|TCL_WRITABLE));
        
    /*
     * Splice this channel out of the list of all channels.
     */

    if (chanPtr == tsdPtr->firstChanPtr) {
        tsdPtr->firstChanPtr = chanPtr->nextChanPtr;
    } else {
        for (prevChanPtr = tsdPtr->firstChanPtr;
                 (prevChanPtr != (Channel *) NULL) &&
                     (prevChanPtr->nextChanPtr != chanPtr);
                 prevChanPtr = prevChanPtr->nextChanPtr) {
            /* Empty loop body. */
        }
        if (prevChanPtr == (Channel *) NULL) {
            panic("FlushChannel: damaged channel list");
        }
        prevChanPtr->nextChanPtr = chanPtr->nextChanPtr;
    }

    /*
     * Close and free the channel driver state.
     */
            
    if (chanPtr->typePtr->closeProc != TCL_CLOSE2PROC) {
	result = (chanPtr->typePtr->closeProc)(chanPtr->instanceData, interp);
    } else {
	result = (chanPtr->typePtr->close2Proc)(chanPtr->instanceData, interp,
		0);
    }
    
    if (chanPtr->channelName != (char *) NULL) {
        ckfree(chanPtr->channelName);
    }
    Tcl_FreeEncoding(chanPtr->encoding);
    if (chanPtr->outputStage != NULL) {
	ckfree((char *) chanPtr->outputStage);
    }
    
    /*
     * If we are being called synchronously, report either
     * any latent error on the channel or the current error.
     */
        
    if (chanPtr->unreportedError != 0) {
        errorCode = chanPtr->unreportedError;
    }
    if (errorCode == 0) {
        errorCode = result;
        if (errorCode != 0) {
            Tcl_SetErrno(errorCode);
        }
    }

    /* Andreas Kupries <a.kupries@westend.com>, 12/13/1998
     * "Trf-Patch for filtering channels"
     *
     * This is the change to 'CloseChannel'.
     *
     * Explanation
     *		Closing a filtering channel closes the one it
     *		superceded too. This basically ripples through
     *		the whole chain of filters until it reaches
     *		the underlying normal channel.
     *
     *		This is done by reintegrating the superceded
     *		channel into the (thread) global list of open
     *		channels and then invoking a regular close.
     *		There is no need to handle the complexities of
     *		this process by ourselves.
     *
     *		*Note*
     *		This has to be done after the call to the
     *		'closeProc' of the filtering channel to allow
     *		that one to flush internal buffers into
     *		the underlying channel.
     */

    if (chanPtr->supercedes != (Channel*) NULL) {
	/*
	 * Insert the channel we were stacked upon back into
	 * the list of open channels, then do a regular close.
	 */

	chanPtr->supercedes->nextChanPtr = tsdPtr->firstChanPtr;
	tsdPtr->firstChanPtr             = chanPtr->supercedes;
	chanPtr->supercedes->refCount --; /* is deregistered */
	Tcl_Close (interp, (Tcl_Channel) chanPtr->supercedes);
    }

    /*
     * Cancel any outstanding timer.
     */

    Tcl_DeleteTimerHandler(chanPtr->timer);

    /*
     * Mark the channel as deleted by clearing the type structure.
     */

    chanPtr->typePtr = NULL;

    Tcl_EventuallyFree((ClientData) chanPtr, TCL_DYNAMIC);

    return errorCode;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Close --
 *
 *	Closes a channel.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Closes the channel if this is the last reference.
 *
 * NOTE:
 *	Tcl_Close removes the channel as far as the user is concerned.
 *	However, it may continue to exist for a while longer if it has
 *	a background flush scheduled. The device itself is eventually
 *	closed and the channel record removed, in CloseChannel, above.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_Close(interp, chan)
    Tcl_Interp *interp;			/* Interpreter for errors. */
    Tcl_Channel chan;			/* The channel being closed. Must
                                         * not be referenced in any
                                         * interpreter. */
{
    ChannelHandler *chPtr, *chNext;	/* Iterate over channel handlers. */
    CloseCallback *cbPtr;		/* Iterate over close callbacks
                                         * for this channel. */
    EventScriptRecord *ePtr, *eNextPtr;	/* Iterate over eventscript records. */
    Channel *chanPtr;			/* The real IO channel. */
    int result;				/* Of calling FlushChannel. */
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    NextChannelHandler *nhPtr;

    if (chan == (Tcl_Channel) NULL) {
        return TCL_OK;
    }
    
    /*
     * Perform special handling for standard channels being closed. If the
     * refCount is now 1 it means that the last reference to the standard
     * channel is being explicitly closed, so bump the refCount down
     * artificially to 0. This will ensure that the channel is actually
     * closed, below. Also set the static pointer to NULL for the channel.
     */

    CheckForStdChannelsBeingClosed(chan);

    chanPtr = (Channel *) chan;
    if (chanPtr->refCount > 0) {
        panic("called Tcl_Close on channel with refCount > 0");
    }

    /*
     * Remove any references to channel handlers for this channel that
     * may be about to be invoked.
     */

    for (nhPtr = tsdPtr->nestedHandlerPtr;
             nhPtr != (NextChannelHandler *) NULL;
             nhPtr = nhPtr->nestedHandlerPtr) {
        if (nhPtr->nextHandlerPtr &&
		(nhPtr->nextHandlerPtr->chanPtr == chanPtr)) {
	    nhPtr->nextHandlerPtr = NULL;
        }
    }

    /*
     * Remove all the channel handler records attached to the channel
     * itself.
     */
        
    for (chPtr = chanPtr->chPtr;
             chPtr != (ChannelHandler *) NULL;
             chPtr = chNext) {
        chNext = chPtr->nextPtr;
        ckfree((char *) chPtr);
    }
    chanPtr->chPtr = (ChannelHandler *) NULL;
    
    
    /*
     * Cancel any pending copy operation.
     */

    StopCopy(chanPtr->csPtr);

    /*
     * Must set the interest mask now to 0, otherwise infinite loops
     * will occur if Tcl_DoOneEvent is called before the channel is
     * finally deleted in FlushChannel. This can happen if the channel
     * has a background flush active.
     */
        
    chanPtr->interestMask = 0;
    
    /*
     * Remove any EventScript records for this channel.
     */

    for (ePtr = chanPtr->scriptRecordPtr;
             ePtr != (EventScriptRecord *) NULL;
             ePtr = eNextPtr) {
        eNextPtr = ePtr->nextPtr;
	Tcl_DecrRefCount(ePtr->scriptPtr);
        ckfree((char *) ePtr);
    }
    chanPtr->scriptRecordPtr = (EventScriptRecord *) NULL;
        
    /*
     * Invoke the registered close callbacks and delete their records.
     */

    while (chanPtr->closeCbPtr != (CloseCallback *) NULL) {
        cbPtr = chanPtr->closeCbPtr;
        chanPtr->closeCbPtr = cbPtr->nextPtr;
        (cbPtr->proc) (cbPtr->clientData);
        ckfree((char *) cbPtr);
    }

    /*
     * Ensure that the last output buffer will be flushed.
     */
    
    if ((chanPtr->curOutPtr != (ChannelBuffer *) NULL) &&
           (chanPtr->curOutPtr->nextAdded > chanPtr->curOutPtr->nextRemoved)) {
        chanPtr->flags |= BUFFER_READY;
    }

    /*
     * If this channel supports it, close the read side, since we don't need it
     * anymore and this will help avoid deadlocks on some channel types.
     */

    if (chanPtr->typePtr->closeProc == TCL_CLOSE2PROC) {
	result = (chanPtr->typePtr->close2Proc)(chanPtr->instanceData, interp,
		TCL_CLOSE_READ);
    } else {
	result = 0;
    }

    /*
     * The call to FlushChannel will flush any queued output and invoke
     * the close function of the channel driver, or it will set up the
     * channel to be flushed and closed asynchronously.
     */

    chanPtr->flags |= CHANNEL_CLOSED;
    if ((FlushChannel(interp, chanPtr, 0) != 0) || (result != 0)) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Write --
 *
 *	Puts a sequence of bytes into an output buffer, may queue the
 *	buffer for output if it gets full, and also remembers whether the
 *	current buffer is ready e.g. if it contains a newline and we are in
 *	line buffering mode.
 *
 * Results:
 *	The number of bytes written or -1 in case of error. If -1,
 *	Tcl_GetErrno will return the error code.
 *
 * Side effects:
 *	May buffer up output and may cause output to be produced on the
 *	channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Write(chan, src, srcLen)
    Tcl_Channel chan;			/* The channel to buffer output for. */
    char *src;				/* Data to queue in output buffer. */
    int srcLen;				/* Length of data in bytes, or < 0 for
					 * strlen(). */
{
    Channel *chanPtr;

    chanPtr = (Channel *) chan;
    if (CheckChannelErrors(chanPtr, TCL_WRITABLE) != 0) {
	return -1;
    }
    if (srcLen < 0) {
        srcLen = strlen(src);
    }
    return DoWrite(chanPtr, src, srcLen);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_WriteChars --
 *
 *	Takes a sequence of UTF-8 characters and converts them for output
 *	using the channel's current encoding, may queue the buffer for
 *	output if it gets full, and also remembers whether the current
 *	buffer is ready e.g. if it contains a newline and we are in
 *	line buffering mode.
 *
 * Results:
 *	The number of bytes written or -1 in case of error. If -1,
 *	Tcl_GetErrno will return the error code.
 *
 * Side effects:
 *	May buffer up output and may cause output to be produced on the
 *	channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_WriteChars(chan, src, len)
    Tcl_Channel chan;		/* The channel to buffer output for. */
    CONST char *src;		/* UTF-8 characters to queue in output buffer. */
    int len;			/* Length of string in bytes, or < 0 for 
				 * strlen(). */
{
    Channel *chanPtr;

    chanPtr = (Channel *) chan;
    if (CheckChannelErrors(chanPtr, TCL_WRITABLE) != 0) {
	return -1;
    }
    if (len < 0) {
        len = strlen(src);
    }
    if (chanPtr->encoding == NULL) {
	/*
	 * Inefficient way to convert UTF-8 to byte-array, but the  
	 * code parallels the way it is done for objects.
	 */

	Tcl_Obj *objPtr;
	int result;

	objPtr = Tcl_NewStringObj(src, len);
	src = (char *) Tcl_GetByteArrayFromObj(objPtr, &len);
	result = WriteBytes(chanPtr, src, len);
	Tcl_DecrRefCount(objPtr);
	return result;
    }
    return WriteChars(chanPtr, src, len);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_WriteObj --
 *
 *	Takes the Tcl object and queues its contents for output.  If the 
 *	encoding of the channel is NULL, takes the byte-array representation 
 *	of the object and queues those bytes for output.  Otherwise, takes 
 *	the characters in the UTF-8 (string) representation of the object 
 *	and converts them for output using the channel's current encoding.  
 *	May flush internal buffers to output if one becomes full or is ready 
 *	for some other reason, e.g. if it contains a newline and the channel 
 *	is in line buffering mode.
 *
 * Results:
 *	The number of bytes written or -1 in case of error. If -1, 
 *	Tcl_GetErrno() will return the error code.
 *
 * Side effects:
 *	May buffer up output and may cause output to be produced on the
 *	channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_WriteObj(chan, objPtr)
    Tcl_Channel chan;		/* The channel to buffer output for. */
    Tcl_Obj *objPtr;		/* The object to write. */
{
    Channel *chanPtr;
    char *src;
    int srcLen;

    chanPtr = (Channel *) chan;
    if (CheckChannelErrors(chanPtr, TCL_WRITABLE) != 0) {
	return -1;
    }
    if (chanPtr->encoding == NULL) {
	src = (char *) Tcl_GetByteArrayFromObj(objPtr, &srcLen);
	return WriteBytes(chanPtr, src, srcLen);
    } else {
	src = Tcl_GetStringFromObj(objPtr, &srcLen);
	return WriteChars(chanPtr, src, srcLen);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * WriteBytes --
 *
 *	Write a sequence of bytes into an output buffer, may queue the
 *	buffer for output if it gets full, and also remembers whether the
 *	current buffer is ready e.g. if it contains a newline and we are in
 *	line buffering mode.
 *
 * Results:
 *	The number of bytes written or -1 in case of error. If -1,
 *	Tcl_GetErrno will return the error code.
 *
 * Side effects:
 *	May buffer up output and may cause output to be produced on the
 *	channel.
 *
 *----------------------------------------------------------------------
 */

static int
WriteBytes(chanPtr, src, srcLen)
    Channel *chanPtr;		/* The channel to buffer output for. */
    CONST char *src;		/* Bytes to write. */
    int srcLen;			/* Number of bytes to write. */
{
    ChannelBuffer *bufPtr;
    char *dst;
    int dstLen, dstMax, sawLF, savedLF, total, toWrite;
    
    total = 0;
    sawLF = 0;
    savedLF = 0;

    /*
     * Loop over all bytes in src, storing them in output buffer with
     * proper EOL translation.
     */

    while (srcLen + savedLF > 0) {
	bufPtr = chanPtr->curOutPtr;
	if (bufPtr == NULL) {
	    bufPtr = AllocChannelBuffer(chanPtr->bufSize);
	    chanPtr->curOutPtr	= bufPtr;
	}
	dst = bufPtr->buf + bufPtr->nextAdded;
	dstMax = bufPtr->bufLength - bufPtr->nextAdded;
	dstLen = dstMax;

	toWrite = dstLen;
	if (toWrite > srcLen) {
	    toWrite = srcLen;
	}

	if (savedLF) {
	    /*
	     * A '\n' was left over from last call to TranslateOutputEOL()
	     * and we need to store it in this buffer.  If the channel is
	     * line-based, we will need to flush it.
	     */

	    *dst++ = '\n';
	    dstLen--;
	    sawLF++;
	}
	sawLF += TranslateOutputEOL(chanPtr, dst, src, &dstLen, &toWrite);
	dstLen += savedLF;
	savedLF = 0;

	if (dstLen > dstMax) {
	    savedLF = 1;
	    dstLen = dstMax;
	}
	bufPtr->nextAdded += dstLen;
	if (CheckFlush(chanPtr, bufPtr, sawLF) != 0) {
	    return -1;
	}
	total += dstLen;
	src += toWrite;
	srcLen -= toWrite;
	sawLF = 0;
    }
    return total;
}

/*
 *----------------------------------------------------------------------
 *
 * WriteChars --
 *
 *	Convert UTF-8 bytes to the channel's external encoding and
 *	write the produced bytes into an output buffer, may queue the 
 *	buffer for output if it gets full, and also remembers whether the
 *	current buffer is ready e.g. if it contains a newline and we are in
 *	line buffering mode.
 *
 * Results:
 *	The number of bytes written or -1 in case of error. If -1,
 *	Tcl_GetErrno will return the error code.
 *
 * Side effects:
 *	May buffer up output and may cause output to be produced on the
 *	channel.
 *
 *----------------------------------------------------------------------
 */

static int
WriteChars(chanPtr, src, srcLen)
    Channel *chanPtr;		/* The channel to buffer output for. */
    CONST char *src;		/* UTF-8 string to write. */
    int srcLen;			/* Length of UTF-8 string in bytes. */
{
    ChannelBuffer *bufPtr;
    char *dst, *stage;
    int saved, savedLF, sawLF, total, toWrite, flags;
    int dstWrote, dstLen, stageLen, stageMax, stageRead;
    Tcl_Encoding encoding;
    char safe[BUFFER_PADDING];
    
    total = 0;
    sawLF = 0;
    savedLF = 0;
    saved = 0;
    encoding = chanPtr->encoding;

    /*
     * Loop over all UTF-8 characters in src, storing them in staging buffer
     * with proper EOL translation.
     */

    while (srcLen + savedLF > 0) {
	stage = chanPtr->outputStage;
	stageMax = chanPtr->bufSize;
	stageLen = stageMax;

	toWrite = stageLen;
	if (toWrite > srcLen) {
	    toWrite = srcLen;
	}

	if (savedLF) {
	    /*
	     * A '\n' was left over from last call to TranslateOutputEOL()
	     * and we need to store it in the staging buffer.  If the
	     * channel is line-based, we will need to flush the output
	     * buffer (after translating the staging buffer).
	     */
	    
	    *stage++ = '\n';
	    stageLen--;
	    sawLF++;
	}
	sawLF += TranslateOutputEOL(chanPtr, stage, src, &stageLen, &toWrite);

	stage -= savedLF;
	stageLen += savedLF;
	savedLF = 0;

	if (stageLen > stageMax) {
	    savedLF = 1;
	    stageLen = stageMax;
	}
	src += toWrite;
	srcLen -= toWrite;

	flags = chanPtr->outputEncodingFlags;
	if (srcLen == 0) {
	    flags |= TCL_ENCODING_END;
	}

	/*
	 * Loop over all UTF-8 characters in staging buffer, converting them
	 * to external encoding, storing them in output buffer.
	 */

	while (stageLen + saved > 0) {
	    bufPtr = chanPtr->curOutPtr;
	    if (bufPtr == NULL) {
		bufPtr = AllocChannelBuffer(chanPtr->bufSize);
		chanPtr->curOutPtr = bufPtr;
	    }
	    dst = bufPtr->buf + bufPtr->nextAdded;
	    dstLen = bufPtr->bufLength - bufPtr->nextAdded;

	    if (saved != 0) {
		/*
		 * Here's some translated bytes left over from the last
		 * buffer that we need to stick at the beginning of this
		 * buffer.
		 */
		 
		memcpy((VOID *) dst, (VOID *) safe, (size_t) saved);
		bufPtr->nextAdded += saved;
		dst += saved;
		dstLen -= saved;
		saved = 0;
	    }

	    Tcl_UtfToExternal(NULL, encoding, stage, stageLen, flags,
		    &chanPtr->outputEncodingState, dst,
		    dstLen + BUFFER_PADDING, &stageRead, &dstWrote, NULL);
	    if (stageRead + dstWrote == 0) {
		/*
		 * We have an incomplete UTF-8 character at the end of the
		 * staging buffer.  It will get moved to the beginning of the
		 * staging buffer followed by more bytes from src.
		 */

		src -= stageLen;
		srcLen += stageLen;
		stageLen = 0;
		savedLF = 0;
		break;
	    }
	    bufPtr->nextAdded += dstWrote;
	    if (bufPtr->nextAdded > bufPtr->bufLength) {
		/*
		 * When translating from UTF-8 to external encoding, we
		 * allowed the translation to produce a character that
		 * crossed the end of the output buffer, so that we would
		 * get a completely full buffer before flushing it.  The
		 * extra bytes will be moved to the beginning of the next
		 * buffer.
		 */

		saved = bufPtr->nextAdded - bufPtr->bufLength;
		memcpy((VOID *) safe, (VOID *) (dst + dstLen), (size_t) saved);
		bufPtr->nextAdded = bufPtr->bufLength;
	    }
	    if (CheckFlush(chanPtr, bufPtr, sawLF) != 0) {
		return -1;
	    }

	    total += dstWrote;
	    stage += stageRead;
	    stageLen -= stageRead;
	    sawLF = 0;
	}
    }
    return total;
}

/*
 *---------------------------------------------------------------------------
 *
 * TranslateOutputEOL --
 *
 *	Helper function for WriteBytes() and WriteChars().  Converts the
 *	'\n' characters in the source buffer into the appropriate EOL
 *	form specified by the output translation mode.
 *
 *	EOL translation stops either when the source buffer is empty
 *	or the output buffer is full.
 *
 *	When converting to CRLF mode and there is only 1 byte left in
 *	the output buffer, this routine stores the '\r' in the last
 *	byte and then stores the '\n' in the byte just past the end of the 
 *	buffer.  The caller is responsible for passing in a buffer that
 *	is large enough to hold the extra byte.
 *
 * Results:
 *	The return value is 1 if a '\n' was translated from the source
 *	buffer, or 0 otherwise -- this can be used by the caller to
 *	decide to flush a line-based channel even though the channel
 *	buffer is not full.
 *
 *	*dstLenPtr is filled with how many bytes of the output buffer
 *	were used.  As mentioned above, this can be one more that
 *	the output buffer's specified length if a CRLF was stored.
 *
 *	*srcLenPtr is filled with how many bytes of the source buffer
 *	were consumed.  
 *
 * Side effects:
 *	It may be obvious, but bears mentioning that when converting
 *	in CRLF mode (which requires two bytes of storage in the output
 *	buffer), the number of bytes consumed from the source buffer
 *	will be less than the number of bytes stored in the output buffer.
 *
 *---------------------------------------------------------------------------
 */

static int
TranslateOutputEOL(chanPtr, dst, src, dstLenPtr, srcLenPtr)
    Channel *chanPtr;		/* Channel being read, for translation and
				 * buffering modes. */
    char *dst;			/* Output buffer filled with UTF-8 chars by
				 * applying appropriate EOL translation to
				 * source characters. */
    CONST char *src;		/* Source UTF-8 characters. */
    int *dstLenPtr;		/* On entry, the maximum length of output
				 * buffer in bytes.  On exit, the number of
				 * bytes actually used in output buffer. */
    int *srcLenPtr;		/* On entry, the length of source buffer.
				 * On exit, the number of bytes read from
				 * the source buffer. */
{
    char *dstEnd;
    int srcLen, newlineFound;
    
    newlineFound = 0;
    srcLen = *srcLenPtr;

    switch (chanPtr->outputTranslation) {
	case TCL_TRANSLATE_LF: {
	    for (dstEnd = dst + srcLen; dst < dstEnd; ) {
		if (*src == '\n') {
		    newlineFound = 1;
		}
		*dst++ = *src++;
	    }
	    *dstLenPtr = srcLen;
	    break;
	}
	case TCL_TRANSLATE_CR: {
	    for (dstEnd = dst + srcLen; dst < dstEnd;) {
		if (*src == '\n') {
		    *dst++ = '\r';
		    newlineFound = 1;
		    src++;
		} else {
		    *dst++ = *src++;
		}
	    }
	    *dstLenPtr = srcLen;
	    break;
	}
	case TCL_TRANSLATE_CRLF: {
	    /*
	     * Since this causes the number of bytes to grow, we
	     * start off trying to put 'srcLen' bytes into the
	     * output buffer, but allow it to store more bytes, as
	     * long as there's still source bytes and room in the
	     * output buffer.
	     */

	    char *dstStart, *dstMax;
	    CONST char *srcStart;
	    
	    dstStart = dst;
	    dstMax = dst + *dstLenPtr;

	    srcStart = src;
	    
	    if (srcLen < *dstLenPtr) {
		dstEnd = dst + srcLen;
	    } else {
		dstEnd = dst + *dstLenPtr;
	    }
	    while (dst < dstEnd) {
		if (*src == '\n') {
		    if (dstEnd < dstMax) {
			dstEnd++;
		    }
		    *dst++ = '\r';
		    newlineFound = 1;
		}
		*dst++ = *src++;
	    }
	    *srcLenPtr = src - srcStart;
	    *dstLenPtr = dst - dstStart;
	    break;
	}
	default: {
	    break;
	}
    }
    return newlineFound;
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckFlush --
 *
 *	Helper function for WriteBytes() and WriteChars().  If the
 *	channel buffer is ready to be flushed, flush it.
 *
 * Results:
 *	The return value is -1 if there was a problem flushing the
 *	channel buffer, or 0 otherwise.
 *
 * Side effects:
 *	The buffer will be recycled if it is flushed.
 *
 *---------------------------------------------------------------------------
 */

static int
CheckFlush(chanPtr, bufPtr, newlineFlag)
    Channel *chanPtr;		/* Channel being read, for buffering mode. */
    ChannelBuffer *bufPtr;	/* Channel buffer to possibly flush. */
    int newlineFlag;		/* Non-zero if a the channel buffer
				 * contains a newline. */
{
    /*
     * The current buffer is ready for output:
     * 1. if it is full.
     * 2. if it contains a newline and this channel is line-buffered.
     * 3. if it contains any output and this channel is unbuffered.
     */

    if ((chanPtr->flags & BUFFER_READY) == 0) {
	if (bufPtr->nextAdded == bufPtr->bufLength) {
	    chanPtr->flags |= BUFFER_READY;
	} else if (chanPtr->flags & CHANNEL_LINEBUFFERED) {
	    if (newlineFlag != 0) {
		chanPtr->flags |= BUFFER_READY;
	    }
	} else if (chanPtr->flags & CHANNEL_UNBUFFERED) {
	    chanPtr->flags |= BUFFER_READY;
	}
    }
    if (chanPtr->flags & BUFFER_READY) {
	if (FlushChannel(NULL, chanPtr, 0) != 0) {
	    return -1;
	}
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_Gets --
 *
 *	Reads a complete line of input from the channel into a Tcl_DString.
 *
 * Results:
 *	Length of line read (in characters) or -1 if error, EOF, or blocked.
 *	If -1, use Tcl_GetErrno() to retrieve the POSIX error code for the
 *	error or condition that occurred.
 *
 * Side effects:
 *	May flush output on the channel.  May cause input to be consumed
 *	from the channel.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_Gets(chan, lineRead)
    Tcl_Channel chan;		/* Channel from which to read. */
    Tcl_DString *lineRead;	/* The line read will be appended to this
				 * DString as UTF-8 characters.  The caller
				 * must have initialized it and is responsible
				 * for managing the storage. */
{
    Tcl_Obj *objPtr;
    int charsStored, length;
    char *string;

    objPtr = Tcl_NewObj();
    charsStored = Tcl_GetsObj(chan, objPtr);
    if (charsStored > 0) {
	string = Tcl_GetStringFromObj(objPtr, &length);
	Tcl_DStringAppend(lineRead, string, length);
    }
    Tcl_DecrRefCount(objPtr);
    return charsStored;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_GetsObj --
 *
 *	Accumulate input from the input channel until end-of-line or
 *	end-of-file has been seen.  Bytes read from the input channel
 *	are converted to UTF-8 using the encoding specified by the
 *	channel.
 *
 * Results:
 *	Number of characters accumulated in the object or -1 if error,
 *	blocked, or EOF.  If -1, use Tcl_GetErrno() to retrieve the
 *	POSIX error code for the error or condition that occurred.
 *
 * Side effects:
 *	Consumes input from the channel.
 *
 *	On reading EOF, leave channel pointing at EOF char.
 *	On reading EOL, leave channel pointing after EOL, but don't
 *	return EOL in dst buffer.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_GetsObj(chan, objPtr)
    Tcl_Channel chan;		/* Channel from which to read. */
    Tcl_Obj *objPtr;		/* The line read will be appended to this
				 * object as UTF-8 characters. */
{
    GetsState gs;
    Channel *chanPtr;
    int inEofChar, skip, copiedTotal;
    ChannelBuffer *bufPtr;
    Tcl_Encoding encoding;
    char *dst, *dstEnd, *eol, *eof;
    Tcl_EncodingState oldState;
    int oldLength, oldFlags, oldRemoved;

    chanPtr = (Channel *) chan;
    if (CheckChannelErrors(chanPtr, TCL_READABLE) != 0) {
	copiedTotal = -1;
	goto done;
    }

    bufPtr = chanPtr->inQueueHead;
    encoding = chanPtr->encoding;

    /*
     * Preserved so we can restore the channel's state in case we don't
     * find a newline in the available input.
     */

    Tcl_GetStringFromObj(objPtr, &oldLength);
    oldFlags = chanPtr->inputEncodingFlags;
    oldState = chanPtr->inputEncodingState;
    oldRemoved = BUFFER_PADDING;
    if (bufPtr != NULL) {
	oldRemoved = bufPtr->nextRemoved;
    }

    /*
     * If there is no encoding, use "iso8859-1" -- Tcl_GetsObj() doesn't
     * produce ByteArray objects.  To avoid circularity problems,
     * "iso8859-1" is builtin to Tcl.
     */

    if (encoding == NULL) {
	encoding = Tcl_GetEncoding(NULL, "iso8859-1");
    }

    /*
     * Object used by FilterInputBytes to keep track of how much data has
     * been consumed from the channel buffers.
     */

    gs.objPtr		= objPtr;
    gs.dstPtr		= &dst;
    gs.encoding		= encoding;
    gs.bufPtr		= bufPtr;
    gs.state		= oldState;
    gs.rawRead		= 0;
    gs.bytesWrote	= 0;
    gs.charsWrote	= 0;
    gs.totalChars	= 0;

    dst = objPtr->bytes + oldLength;
    dstEnd = dst;

    skip = 0;
    eof = NULL;
    inEofChar = chanPtr->inEofChar;

    while (1) {
	if (dst >= dstEnd) {
	    if (FilterInputBytes(chanPtr, &gs) != 0) {
		goto restore;
	    }
	    dstEnd = dst + gs.bytesWrote;
	}
	
	/*
	 * Remember if EOF char is seen, then look for EOL anyhow, because
	 * the EOL might be before the EOF char.
	 */

	if (inEofChar != '\0') {
	    for (eol = dst; eol < dstEnd; eol++) {
		if (*eol == inEofChar) {
		    dstEnd = eol;
		    eof = eol;
		    break;
		}
	    }
	}

	/*
	 * On EOL, leave current file position pointing after the EOL, but
	 * don't store the EOL in the output string.
	 */

	eol = dst;
	switch (chanPtr->inputTranslation) {
	    case TCL_TRANSLATE_LF: {
		for (eol = dst; eol < dstEnd; eol++) {
		    if (*eol == '\n') {
			skip = 1;
			goto goteol;
		    }
		}
		break;
	    }
	    case TCL_TRANSLATE_CR: {
		for (eol = dst; eol < dstEnd; eol++) {
		    if (*eol == '\r') {
			skip = 1;
			goto goteol;
		    }
		}
		break;
	    }
	    case TCL_TRANSLATE_CRLF: {
		for (eol = dst; eol < dstEnd; eol++) {
		    if (*eol == '\r') {
			eol++;
			if (eol >= dstEnd) {
			    int offset;
			    
			    offset = eol - objPtr->bytes;
			    dst = dstEnd;
			    if (FilterInputBytes(chanPtr, &gs) != 0) {
				goto restore;
			    }
			    dstEnd = dst + gs.bytesWrote;
			    eol = objPtr->bytes + offset;
			    if (eol >= dstEnd) {
				skip = 0;
				goto goteol;
			    }
			}
			if (*eol == '\n') {
			    eol--;
			    skip = 2;
			    goto goteol;
			}
		    }
		}
		break;
	    }
	    case TCL_TRANSLATE_AUTO: {
		skip = 1;
		if (chanPtr->flags & INPUT_SAW_CR) {
		    chanPtr->flags &= ~INPUT_SAW_CR;
		    if (*eol == '\n') {
			/*
			 * Skip the raw bytes that make up the '\n'.
			 */

			char tmp[1 + TCL_UTF_MAX];
			int rawRead;

			bufPtr = gs.bufPtr;
			Tcl_ExternalToUtf(NULL, gs.encoding,
				bufPtr->buf + bufPtr->nextRemoved,
				gs.rawRead, chanPtr->inputEncodingFlags,
				&gs.state, tmp, 1 + TCL_UTF_MAX, &rawRead,
				NULL, NULL);
			bufPtr->nextRemoved += rawRead;
			gs.rawRead -= rawRead;
			gs.bytesWrote--;
			gs.charsWrote--;
			memmove(dst, dst + 1, (size_t) (dstEnd - dst));
			dstEnd--;
		    }
		}
		for (eol = dst; eol < dstEnd; eol++) {
		    if (*eol == '\r') {
			eol++;
			if (eol == dstEnd) {
			    /*
			     * If buffer ended on \r, peek ahead to see if a
			     * \n is available.
			     */

			    int offset;
			    
			    offset = eol - objPtr->bytes;
			    dst = dstEnd;
			    PeekAhead(chanPtr, &dstEnd, &gs);
			    eol = objPtr->bytes + offset;
			    if (eol >= dstEnd) {
				eol--;
				chanPtr->flags |= INPUT_SAW_CR;
				goto goteol;
			    }
			}
			if (*eol == '\n') {
			    skip++;
			}
			eol--;
			goto goteol;
		    } else if (*eol == '\n') {
			goto goteol;
		    }
		}
	    }
	}
	if (eof != NULL) {
	    /*
	     * EOF character was seen.  On EOF, leave current file position
	     * pointing at the EOF character, but don't store the EOF
	     * character in the output string.
	     */

	    dstEnd = eof;
	    chanPtr->flags |= (CHANNEL_EOF | CHANNEL_STICKY_EOF);
	    chanPtr->inputEncodingFlags |= TCL_ENCODING_END;
	}
	if (chanPtr->flags & CHANNEL_EOF) {
	    skip = 0;
	    eol = dstEnd;
	    if (eol == objPtr->bytes) {
		/*
		 * If we didn't produce any bytes before encountering EOF,
		 * caller needs to see -1.
		 */

		Tcl_SetObjLength(objPtr, 0);
		CommonGetsCleanup(chanPtr, encoding);
		copiedTotal = -1;
		goto done;
	    }
	    goto goteol;
	}
	dst = dstEnd;
    }

    /*
     * Found EOL or EOF, but the output buffer may now contain too many
     * UTF-8 characters.  We need to know how many raw bytes correspond to
     * the number of UTF-8 characters we want, plus how many raw bytes
     * correspond to the character(s) making up EOL (if any), so we can
     * remove the correct number of bytes from the channel buffer.
     */
     
    goteol:
    bufPtr = gs.bufPtr;
    chanPtr->inputEncodingState = gs.state;
    Tcl_ExternalToUtf(NULL, gs.encoding, bufPtr->buf + bufPtr->nextRemoved,
	    gs.rawRead, chanPtr->inputEncodingFlags,
	    &chanPtr->inputEncodingState, dst, eol - dst + skip + TCL_UTF_MAX,
	    &gs.rawRead, NULL, &gs.charsWrote);
    bufPtr->nextRemoved += gs.rawRead;

    /*
     * Recycle all the emptied buffers.
     */

    Tcl_SetObjLength(objPtr, eol - objPtr->bytes);
    CommonGetsCleanup(chanPtr, encoding);
    chanPtr->flags &= ~CHANNEL_BLOCKED;
    copiedTotal = gs.totalChars + gs.charsWrote - skip;
    goto done;

    /*
     * Couldn't get a complete line.  This only happens if we get a error
     * reading from the channel or we are non-blocking and there wasn't
     * an EOL or EOF in the data available.
     */

    restore:
    bufPtr = chanPtr->inQueueHead;
    bufPtr->nextRemoved = oldRemoved;

    for (bufPtr = bufPtr->nextPtr; bufPtr != NULL; bufPtr = bufPtr->nextPtr) {
	bufPtr->nextRemoved = BUFFER_PADDING;
    }
    CommonGetsCleanup(chanPtr, encoding);

    chanPtr->inputEncodingState = oldState;
    chanPtr->inputEncodingFlags = oldFlags;
    Tcl_SetObjLength(objPtr, oldLength);

    /*
     * We didn't get a complete line so we need to indicate to UpdateInterest
     * that the gets blocked.  It will wait for more data instead of firing
     * a timer, avoiding a busy wait.  This is where we are assuming that the
     * next operation is a gets.  No more file events will be delivered on 
     * this channel until new data arrives or some operation is performed
     * on the channel (e.g. gets, read, fconfigure) that changes the blocking
     * state.  Note that this means a file event will not be delivered even
     * though a read would be able to consume the buffered data.
     */

    chanPtr->flags |= CHANNEL_NEED_MORE_DATA;
    copiedTotal = -1;

    done:
    /*
     * Update the notifier state so we don't block while there is still
     * data in the buffers.
     */

    UpdateInterest(chanPtr);
    return copiedTotal;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterInputBytes --
 *
 *	Helper function for Tcl_GetsObj.  Produces UTF-8 characters from
 *	raw bytes read from the channel.  
 *
 *	Consumes available bytes from channel buffers.  When channel
 *	buffers are exhausted, reads more bytes from channel device into
 *	a new channel buffer.  It is the caller's responsibility to
 *	free the channel buffers that have been exhausted.
 *
 * Results:
 *	The return value is -1 if there was an error reading from the
 *	channel, 0 otherwise.
 *
 * Side effects:
 *	Status object keeps track of how much data from channel buffers
 *	has been consumed and where UTF-8 bytes should be stored.
 *
 *---------------------------------------------------------------------------
 */
 
static int
FilterInputBytes(chanPtr, gsPtr)
    Channel *chanPtr;		/* Channel to read. */
    GetsState *gsPtr;		/* Current state of gets operation. */
{
    ChannelBuffer *bufPtr;
    char *raw, *rawStart, *rawEnd;
    char *dst;
    int offset, toRead, dstNeeded, spaceLeft, result, rawLen, length;
    Tcl_Obj *objPtr;
#define ENCODING_LINESIZE   30	/* Lower bound on how many bytes to convert
				 * at a time.  Since we don't know a priori
				 * how many bytes of storage this many source
				 * bytes will use, we actually need at least
				 * ENCODING_LINESIZE * TCL_MAX_UTF bytes of
				 * room. */

    objPtr = gsPtr->objPtr;

    /*
     * Subtract the number of bytes that were removed from channel buffer
     * during last call.
     */

    bufPtr = gsPtr->bufPtr;
    if (bufPtr != NULL) {
	bufPtr->nextRemoved += gsPtr->rawRead;
	if (bufPtr->nextRemoved >= bufPtr->nextAdded) {
	    bufPtr = bufPtr->nextPtr;
	}
    }
    gsPtr->totalChars += gsPtr->charsWrote;

    if ((bufPtr == NULL) || (bufPtr->nextAdded == BUFFER_PADDING)) {
	/*
	 * All channel buffers were exhausted and the caller still hasn't
	 * seen EOL.  Need to read more bytes from the channel device.
	 * Side effect is to allocate another channel buffer.
	 */
	 
	read:
        if (chanPtr->flags & CHANNEL_BLOCKED) {
            if (chanPtr->flags & CHANNEL_NONBLOCKING) {
		gsPtr->charsWrote = 0;
		gsPtr->rawRead = 0;
		return -1;
	    }
            chanPtr->flags &= ~CHANNEL_BLOCKED;
        }
	if (GetInput(chanPtr) != 0) {
	    gsPtr->charsWrote = 0;
	    gsPtr->rawRead = 0;
	    return -1;
	}
	bufPtr = chanPtr->inQueueTail;
	gsPtr->bufPtr = bufPtr;
    }

    /*
     * Convert some of the bytes from the channel buffer to UTF-8.  Space in
     * objPtr's string rep is used to hold the UTF-8 characters.  Grow the
     * string rep if we need more space.
     */

    rawStart = bufPtr->buf + bufPtr->nextRemoved;
    raw = rawStart;
    rawEnd = bufPtr->buf + bufPtr->nextAdded;
    rawLen = rawEnd - rawStart;

    dst = *gsPtr->dstPtr;
    offset = dst - objPtr->bytes;
    toRead = ENCODING_LINESIZE;
    if (toRead > rawLen) {
	toRead = rawLen;
    }
    dstNeeded = toRead * TCL_UTF_MAX + 1;
    spaceLeft = objPtr->length - offset - TCL_UTF_MAX - 1;
    if (dstNeeded > spaceLeft) {
	length = offset * 2;
	if (offset < dstNeeded) {
	    length = offset + dstNeeded;
	}
	length += TCL_UTF_MAX + 1;
	Tcl_SetObjLength(objPtr, length);
	spaceLeft = length - offset;
	dst = objPtr->bytes + offset;
	*gsPtr->dstPtr = dst;
    }
    gsPtr->state = chanPtr->inputEncodingState;
    result = Tcl_ExternalToUtf(NULL, gsPtr->encoding, raw, rawLen,
	    chanPtr->inputEncodingFlags, &chanPtr->inputEncodingState,
	    dst, spaceLeft, &gsPtr->rawRead, &gsPtr->bytesWrote,
	    &gsPtr->charsWrote); 
    if (result == TCL_CONVERT_MULTIBYTE) {
	/*
	 * The last few bytes in this channel buffer were the start of a
	 * multibyte sequence.  If this buffer was full, then move them to
	 * the next buffer so the bytes will be contiguous.  
	 */

	ChannelBuffer *nextPtr;
	int extra;
	
	nextPtr = bufPtr->nextPtr;
	if (bufPtr->nextAdded < bufPtr->bufLength) {
	    if (gsPtr->rawRead > 0) {
		/*
		 * Some raw bytes were converted to UTF-8.  Fall through,
		 * returning those UTF-8 characters because a EOL might be
		 * present in them.
		 */
	    } else if (chanPtr->flags & CHANNEL_EOF) {
		/*
		 * There was a partial character followed by EOF on the
		 * device.  Fall through, returning that nothing was found.
		 */

		 bufPtr->nextRemoved = bufPtr->nextAdded;
	    } else {
		/*
		 * There are no more cached raw bytes left.  See if we can
		 * get some more.
		 */

		goto read;
	    }
	} else {
	    if (nextPtr == NULL) {
		nextPtr = AllocChannelBuffer(chanPtr->bufSize);
		bufPtr->nextPtr = nextPtr;
		chanPtr->inQueueTail = nextPtr;
	    }
	    extra = rawLen - gsPtr->rawRead;
	    memcpy((VOID *) (nextPtr->buf + BUFFER_PADDING - extra),
		    (VOID *) (raw + gsPtr->rawRead), (size_t) extra);
	    nextPtr->nextRemoved -= extra;
	    bufPtr->nextAdded -= extra;
	}
    }

    gsPtr->bufPtr = bufPtr;
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * PeekAhead --
 *
 *	Helper function used by Tcl_GetsObj().  Called when we've seen a
 *	\r at the end of the UTF-8 string and want to look ahead one
 *	character to see if it is a \n.
 *
 * Results:
 *	*gsPtr->dstPtr is filled with a pointer to the start of the range of
 *	UTF-8 characters that were found by peeking and *dstEndPtr is filled
 *	with a pointer to the bytes just after the end of the range.
 *
 * Side effects:
 *	If no more raw bytes were available in one of the channel buffers,
 *	tries to perform a non-blocking read to get more bytes from the
 *	channel device.
 *
 *---------------------------------------------------------------------------
 */

static void
PeekAhead(chanPtr, dstEndPtr, gsPtr)
    Channel *chanPtr;		/* The channel to read. */
    char **dstEndPtr;		/* Filled with pointer to end of new range
				 * of UTF-8 characters. */
    GetsState *gsPtr;		/* Current state of gets operation. */
{
    ChannelBuffer *bufPtr;
    Tcl_DriverBlockModeProc *blockModeProc;
    int bytesLeft;

    bufPtr = gsPtr->bufPtr;

    /*
     * If there's any more raw input that's still buffered, we'll peek into
     * that.  Otherwise, only get more data from the channel driver if it
     * looks like there might actually be more data.  The assumption is that
     * if the channel buffer is filled right up to the end, then there
     * might be more data to read.
     */

    blockModeProc = NULL;
    if (bufPtr->nextPtr == NULL) {
	bytesLeft = bufPtr->nextAdded - (bufPtr->nextRemoved + gsPtr->rawRead);
	if (bytesLeft == 0) {
	    if (bufPtr->nextAdded < bufPtr->bufLength) {
		/*
		 * Don't peek ahead if last read was short read.
		 */
		 
		goto cleanup;
	    }
	    if ((chanPtr->flags & CHANNEL_NONBLOCKING) == 0) {
		blockModeProc = chanPtr->typePtr->blockModeProc;
		if (blockModeProc == NULL) {
		    /*
		     * Don't peek ahead if cannot set non-blocking mode.
		     */

		    goto cleanup;
		}
		(*blockModeProc)(chanPtr->instanceData, TCL_MODE_NONBLOCKING);
	    }
	}
    }
    if (FilterInputBytes(chanPtr, gsPtr) == 0) {
	*dstEndPtr = *gsPtr->dstPtr + gsPtr->bytesWrote;
    }
    if (blockModeProc != NULL) {
	(*blockModeProc)(chanPtr->instanceData, TCL_MODE_BLOCKING);
    }
    return;

    cleanup:
    bufPtr->nextRemoved += gsPtr->rawRead;
    gsPtr->rawRead = 0;
    gsPtr->totalChars += gsPtr->charsWrote;
    gsPtr->bytesWrote = 0;
    gsPtr->charsWrote = 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * CommonGetsCleanup --
 *
 *	Helper function for Tcl_GetsObj() to restore the channel after
 *	a "gets" operation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Encoding may be freed.
 *
 *---------------------------------------------------------------------------
 */
 
static void
CommonGetsCleanup(chanPtr, encoding)
    Channel *chanPtr;
    Tcl_Encoding encoding;
{
    ChannelBuffer *bufPtr, *nextPtr;
    
    bufPtr = chanPtr->inQueueHead;
    for ( ; bufPtr != NULL; bufPtr = nextPtr) {
	nextPtr = bufPtr->nextPtr;
	if (bufPtr->nextRemoved < bufPtr->nextAdded) {
	    break;
	}
	RecycleBuffer(chanPtr, bufPtr, 0);
    }
    chanPtr->inQueueHead = bufPtr;
    if (bufPtr == NULL) {
	chanPtr->inQueueTail = NULL;
    } else {
	/*
	 * If any multi-byte characters were split across channel buffer
	 * boundaries, the split-up bytes were moved to the next channel
	 * buffer by FilterInputBytes().  Move the bytes back to their
	 * original buffer because the caller could change the channel's
	 * encoding which could change the interpretation of whether those
	 * bytes really made up multi-byte characters after all.
	 */
	 
	nextPtr = bufPtr->nextPtr;
	for ( ; nextPtr != NULL; nextPtr = bufPtr->nextPtr) {
	    int extra;

	    extra = bufPtr->bufLength - bufPtr->nextAdded;
	    if (extra > 0) {
		memcpy((VOID *) (bufPtr->buf + bufPtr->nextAdded),
			(VOID *) (nextPtr->buf + BUFFER_PADDING - extra),
			(size_t) extra);
		bufPtr->nextAdded += extra;
		nextPtr->nextRemoved = BUFFER_PADDING;
	    }
	    bufPtr = nextPtr;
	}
    }
    if (chanPtr->encoding == NULL) {
	Tcl_FreeEncoding(encoding);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Read --
 *
 *	Reads a given number of bytes from a channel.  EOL and EOF
 *	translation is done on the bytes being read, so the the number
 *	of bytes consumed from the channel may not be equal to the
 *	number of bytes stored in the destination buffer.
 *
 *	No encoding conversions are applied to the bytes being read.
 *
 * Results:
 *	The number of bytes read, or -1 on error. Use Tcl_GetErrno()
 *	to retrieve the error code for the error that occurred.
 *
 * Side effects:
 *	May cause input to be buffered.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Read(chan, dst, bytesToRead)
    Tcl_Channel chan;		/* The channel from which to read. */
    char *dst;			/* Where to store input read. */
    int bytesToRead;		/* Maximum number of bytes to read. */
{
    Channel *chanPtr;		
    
    chanPtr = (Channel *) chan;
    if (CheckChannelErrors(chanPtr, TCL_READABLE) != 0) {
	return -1;
    }

    return DoRead(chanPtr, dst, bytesToRead);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_ReadChars --
 *
 *	Reads from the channel until the requested number of characters
 *	have been seen, EOF is seen, or the channel would block.  EOL
 *	and EOF translation is done.  If reading binary data, the raw
 *	bytes are wrapped in a Tcl byte array object.  Otherwise, the raw
 *	bytes are converted to UTF-8 using the channel's current encoding
 *	and stored in a Tcl string object.
 *
 * Results:
 *	The number of characters read, or -1 on error. Use Tcl_GetErrno()
 *	to retrieve the error code for the error that occurred.
 *
 * Side effects:
 *	May cause input to be buffered.
 *
 *---------------------------------------------------------------------------
 */
 
int
Tcl_ReadChars(chan, objPtr, toRead, appendFlag)
    Tcl_Channel chan;		/* The channel to read. */
    Tcl_Obj *objPtr;		/* Input data is stored in this object. */
    int toRead;			/* Maximum number of characters to store,
				 * or -1 to read all available data (up to EOF
				 * or when channel blocks). */
    int appendFlag;		/* If non-zero, data read from the channel
				 * will be appended to the object.  Otherwise,
				 * the data will replace the existing contents
				 * of the object. */

{
    Channel *chanPtr;
    int offset, factor, copied, copiedNow, result;
    ChannelBuffer *bufPtr;
    Tcl_Encoding encoding;
#define UTF_EXPANSION_FACTOR	1024
    
    chanPtr = (Channel *) chan;
    if (CheckChannelErrors(chanPtr, TCL_READABLE) != 0) {
	copied = -1;
	goto done;
    }

    encoding = chanPtr->encoding;
    factor = UTF_EXPANSION_FACTOR;

    if (appendFlag == 0) {
	if (encoding == NULL) {
	    Tcl_SetByteArrayLength(objPtr, 0);
	} else {
	    Tcl_SetObjLength(objPtr, 0);
	}
	offset = 0;
    } else {
	if (encoding == NULL) {
	    Tcl_GetByteArrayFromObj(objPtr, &offset);
	} else {
	    Tcl_GetStringFromObj(objPtr, &offset);
	}
    }

    for (copied = 0; (unsigned) toRead > 0; ) {
	copiedNow = -1;
	if (chanPtr->inQueueHead != NULL) {
	    if (encoding == NULL) {
		copiedNow = ReadBytes(chanPtr, objPtr, toRead, &offset);
	    } else {
		copiedNow = ReadChars(chanPtr, objPtr, toRead, &offset,
			&factor);
	    }

	    /*
	     * If the current buffer is empty recycle it.
	     */

	    bufPtr = chanPtr->inQueueHead;
	    if (bufPtr->nextRemoved == bufPtr->nextAdded) {
		ChannelBuffer *nextPtr;

		nextPtr = bufPtr->nextPtr;
		RecycleBuffer(chanPtr, bufPtr, 0);
		chanPtr->inQueueHead = nextPtr;
		if (nextPtr == NULL) {
		    chanPtr->inQueueTail = nextPtr;
		}
	    }
	}
	if (copiedNow < 0) {
	    if (chanPtr->flags & CHANNEL_EOF) {
		break;
	    }
	    if (chanPtr->flags & CHANNEL_BLOCKED) {
		if (chanPtr->flags & CHANNEL_NONBLOCKING) {
		    break;
		}
		chanPtr->flags &= ~CHANNEL_BLOCKED;
	    }
	    result = GetInput(chanPtr);
	    if (result != 0) {
		if (result == EAGAIN) {
		    break;
		}
		copied = -1;
		goto done;
	    }
	} else {
	    copied += copiedNow;
	    toRead -= copiedNow;
	}
    }
    chanPtr->flags &= ~CHANNEL_BLOCKED;
    if (encoding == NULL) {
	Tcl_SetByteArrayLength(objPtr, offset);
    } else {
	Tcl_SetObjLength(objPtr, offset);
    }

    done:
    /*
     * Update the notifier state so we don't block while there is still
     * data in the buffers.
     */

    UpdateInterest(chanPtr);
    return copied;
}
/*
 *---------------------------------------------------------------------------
 *
 * ReadBytes --
 *
 *	Reads from the channel until the requested number of bytes have
 *	been seen, EOF is seen, or the channel would block.  Bytes from
 *	the channel are stored in objPtr as a ByteArray object.  EOL
 *	and EOF translation are done.
 *
 *	'bytesToRead' can safely be a very large number because
 *	space is only allocated to hold data read from the channel
 *	as needed.
 *
 * Results:
 *	The return value is the number of bytes appended to the object
 *	and *offsetPtr is filled with the total number of bytes in the
 *	object (greater than the return value if there were already bytes
 *	in the object).
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
ReadBytes(chanPtr, objPtr, bytesToRead, offsetPtr)
    Channel *chanPtr;		/* The channel to read. */
    int bytesToRead;		/* Maximum number of characters to store,
				 * or < 0 to get all available characters.
				 * Characters are obtained from the first
				 * buffer in the queue -- even if this number
				 * is larger than the number of characters
				 * available in the first buffer, only the
				 * characters from the first buffer are
				 * returned. */
    Tcl_Obj *objPtr;		/* Input data is appended to this ByteArray
				 * object.  Its length is how much space
				 * has been allocated to hold data, not how
				 * many bytes of data have been stored in the
				 * object. */
    int *offsetPtr;		/* On input, contains how many bytes of
				 * objPtr have been used to hold data.  On
				 * output, filled with how many bytes are now
				 * being used. */
{
    int toRead, srcLen, srcRead, dstWrote, offset, length;
    ChannelBuffer *bufPtr;
    char *src, *dst;

    offset = *offsetPtr;

    bufPtr = chanPtr->inQueueHead; 
    src = bufPtr->buf + bufPtr->nextRemoved;
    srcLen = bufPtr->nextAdded - bufPtr->nextRemoved;

    toRead = bytesToRead;
    if ((unsigned) toRead > (unsigned) srcLen) {
	toRead = srcLen;
    }

    dst = (char *) Tcl_GetByteArrayFromObj(objPtr, &length);
    if (toRead > length - offset - 1) {
	/*
	 * Double the existing size of the object or make enough room to
	 * hold all the characters we may get from the source buffer,
	 * whichever is larger.
	 */

	length = offset * 2;
	if (offset < toRead) {
	    length = offset + toRead + 1;
	}
	dst = (char *) Tcl_SetByteArrayLength(objPtr, length);
    }
    dst += offset;

    if (chanPtr->flags & INPUT_NEED_NL) {
	chanPtr->flags &= ~INPUT_NEED_NL;
	if ((srcLen == 0) || (*src != '\n')) {
	    *dst = '\r';
	    *offsetPtr += 1;
	    return 1;
	}
	*dst++ = '\n';
	src++;
	srcLen--;
	toRead--;
    }

    srcRead = srcLen;
    dstWrote = toRead;
    if (TranslateInputEOL(chanPtr, dst, src, &dstWrote, &srcRead) != 0) {
	if (dstWrote == 0) {
	    return -1;
	}
    }
    bufPtr->nextRemoved += srcRead;
    *offsetPtr += dstWrote;
    return dstWrote;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadChars --
 *
 *	Reads from the channel until the requested number of UTF-8
 *	characters have been seen, EOF is seen, or the channel would
 *	block.  Raw bytes from the channel are converted to UTF-8
 *	and stored in objPtr.  EOL and EOF translation is done.
 *
 *	'charsToRead' can safely be a very large number because
 *	space is only allocated to hold data read from the channel
 *	as needed.
 *
 * Results:
 *	The return value is the number of characters appended to
 *	the object, *offsetPtr is filled with the number of bytes that
 *	were appended, and *factorPtr is filled with the expansion
 *	factor used to guess how many bytes of UTF-8 to allocate to
 *	hold N source bytes.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
ReadChars(chanPtr, objPtr, charsToRead, offsetPtr, factorPtr)
    Channel *chanPtr;		/* The channel to read. */
    int charsToRead;		/* Maximum number of characters to store,
				 * or -1 to get all available characters.
				 * Characters are obtained from the first
				 * buffer in the queue -- even if this number
				 * is larger than the number of characters
				 * available in the first buffer, only the
				 * characters from the first buffer are
				 * returned. */
    Tcl_Obj *objPtr;		/* Input data is appended to this object.
				 * objPtr->length is how much space has been
				 * allocated to hold data, not how many bytes
				 * of data have been stored in the object. */
    int *offsetPtr;		/* On input, contains how many bytes of
				 * objPtr have been used to hold data.  On
				 * output, filled with how many bytes are now
				 * being used. */
    int *factorPtr;		/* On input, contains a guess of how many
				 * bytes need to be allocated to hold the
				 * result of converting N source bytes to
				 * UTF-8.  On output, contains another guess
				 * based on the data seen so far. */
{
    int toRead, factor, offset, spaceLeft, length;
    int srcLen, srcRead, dstNeeded, dstRead, dstWrote, numChars;
    ChannelBuffer *bufPtr;
    char *src, *dst;
    Tcl_EncodingState oldState;

    factor = *factorPtr;
    offset = *offsetPtr;

    bufPtr = chanPtr->inQueueHead; 
    src = bufPtr->buf + bufPtr->nextRemoved;
    srcLen = bufPtr->nextAdded - bufPtr->nextRemoved;

    toRead = charsToRead;
    if ((unsigned) toRead > (unsigned) srcLen) {
	toRead = srcLen;
    }

    /*
     * 'factor' is how much we guess that the bytes in the source buffer
     * will expand when converted to UTF-8 chars.  This guess comes from
     * analyzing how many characters were produced by the previous
     * pass.
     */

    dstNeeded = toRead * factor / UTF_EXPANSION_FACTOR;
    spaceLeft = objPtr->length - offset - TCL_UTF_MAX - 1;

    if (dstNeeded > spaceLeft) {
	/*
	 * Double the existing size of the object or make enough room to
	 * hold all the characters we want from the source buffer,
	 * whichever is larger.
	 */

	length = offset * 2;
	if (offset < dstNeeded) {
	    length = offset + dstNeeded;
	}
	spaceLeft = length - offset;
	length += TCL_UTF_MAX + 1;
	Tcl_SetObjLength(objPtr, length);
    }
    if (toRead == srcLen) {
	/*
	 * Want to convert the whole buffer in one pass.  If we have
	 * enough space, convert it using all available space in object
	 * rather than using the factor.
	 */

	dstNeeded = spaceLeft;
    }
    dst = objPtr->bytes + offset;

    oldState = chanPtr->inputEncodingState;
    if (chanPtr->flags & INPUT_NEED_NL) {
	/*
	 * We want a '\n' because the last character we saw was '\r'.
	 */
	 
	chanPtr->flags &= ~INPUT_NEED_NL;
	Tcl_ExternalToUtf(NULL, chanPtr->encoding, src, srcLen,
		chanPtr->inputEncodingFlags, &chanPtr->inputEncodingState,
		dst, TCL_UTF_MAX + 1, &srcRead, &dstWrote, &numChars);
	if ((dstWrote > 0) && (*dst == '\n')) {
	    /*
	     * The next char was a '\n'.  Consume it and produce a '\n'.
	     */
	     
	    bufPtr->nextRemoved += srcRead;
	} else {
	    /*
	     * The next char was not a '\n'.  Produce a '\r'.
	     */

	    *dst = '\r';
	}
	chanPtr->inputEncodingFlags &= ~TCL_ENCODING_START;
	*offsetPtr += 1;
        return 1;
    }

    Tcl_ExternalToUtf(NULL, chanPtr->encoding, src, srcLen,
	    chanPtr->inputEncodingFlags, &chanPtr->inputEncodingState, dst,
	    dstNeeded + TCL_UTF_MAX, &srcRead, &dstWrote, &numChars);
    if (srcRead == 0) {
	/*
	 * Not enough bytes in src buffer to make a complete char.  Copy
	 * the bytes to the next buffer to make a new contiguous string,
	 * then tell the caller to fill the buffer with more bytes.
	 */

	ChannelBuffer *nextPtr;
	
	nextPtr = bufPtr->nextPtr;
	if (nextPtr == NULL) {
	    /*
	     * There isn't enough data in the buffers to complete the next
	     * character, so we need to wait for more data before the next
	     * file event can be delivered.
	     */

	    chanPtr->flags |= CHANNEL_NEED_MORE_DATA;
	    return -1;
	}
	nextPtr->nextRemoved -= srcLen;
	memcpy((VOID *) (nextPtr->buf + nextPtr->nextRemoved), (VOID *) src,
		(size_t) srcLen);
	RecycleBuffer(chanPtr, bufPtr, 0);
	chanPtr->inQueueHead = nextPtr;
	return ReadChars(chanPtr, objPtr, charsToRead, offsetPtr, factorPtr);
    }

    dstRead = dstWrote;
    if (TranslateInputEOL(chanPtr, dst, dst, &dstWrote, &dstRead) != 0) {
	/*
	 * Hit EOF char.  How many bytes of src correspond to where the
	 * EOF was located in dst?
	 */
	 
	if (dstWrote == 0) {
	    return -1;
	}
	chanPtr->inputEncodingState = oldState;
	Tcl_ExternalToUtf(NULL, chanPtr->encoding, src, srcLen,
		chanPtr->inputEncodingFlags, &chanPtr->inputEncodingState,
		dst, dstRead + TCL_UTF_MAX, &srcRead, &dstWrote, &numChars);
	TranslateInputEOL(chanPtr, dst, dst, &dstWrote, &dstRead);
    } 

    /*
     * The number of characters that we got may be less than the number
     * that we started with because "\r\n" sequences may have been
     * turned into just '\n' in dst.
     */

    numChars -= (dstRead - dstWrote);

    if ((unsigned) numChars > (unsigned) toRead) {
	/*
	 * Got too many chars.
	 */

	char *eof;

	eof = Tcl_UtfAtIndex(dst, toRead);
	chanPtr->inputEncodingState = oldState;
	Tcl_ExternalToUtf(NULL, chanPtr->encoding, src, srcLen,
		chanPtr->inputEncodingFlags, &chanPtr->inputEncodingState,
		dst, eof - dst + TCL_UTF_MAX, &srcRead, &dstWrote, &numChars);
	dstRead = dstWrote;
	TranslateInputEOL(chanPtr, dst, dst, &dstWrote, &dstRead);
	numChars -= (dstRead - dstWrote);
    }
    chanPtr->inputEncodingFlags &= ~TCL_ENCODING_START;

    bufPtr->nextRemoved += srcRead;
    if (dstWrote > srcRead + 1) {
	*factorPtr = dstWrote * UTF_EXPANSION_FACTOR / srcRead;
    }
    *offsetPtr += dstWrote;
    return numChars;
}

/*
 *---------------------------------------------------------------------------
 *
 * TranslateInputEOL --
 *
 *	Perform input EOL and EOF translation on the source buffer,
 *	leaving the translated result in the destination buffer.  
 *
 * Results:
 *	The return value is 1 if the EOF character was found when copying
 *	bytes to the destination buffer, 0 otherwise.  
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
TranslateInputEOL(chanPtr, dstStart, srcStart, dstLenPtr, srcLenPtr)
    Channel *chanPtr;		/* Channel being read, for EOL translation
				 * and EOF character. */
    char *dstStart;		/* Output buffer filled with chars by
				 * applying appropriate EOL translation to
				 * source characters. */
    CONST char *srcStart;	/* Source characters. */
    int *dstLenPtr;		/* On entry, the maximum length of output
				 * buffer in bytes; must be <= *srcLenPtr.  On
				 * exit, the number of bytes actually used in
				 * output buffer. */
    int *srcLenPtr;		/* On entry, the length of source buffer.
				 * On exit, the number of bytes read from
				 * the source buffer. */
{
    int dstLen, srcLen, inEofChar;
    CONST char *eof;

    dstLen = *dstLenPtr;

    eof = NULL;
    inEofChar = chanPtr->inEofChar;
    if (inEofChar != '\0') {
	/*
	 * Find EOF in translated buffer then compress out the EOL.  The
	 * source buffer may be much longer than the destination buffer --
	 * we only want to return EOF if the EOF has been copied to the
	 * destination buffer.
	 */

	CONST char *src, *srcMax;

	srcMax = srcStart + *srcLenPtr;
	for (src = srcStart; src < srcMax; src++) {
	    if (*src == inEofChar) {
		eof = src;
		srcLen = src - srcStart;
		if (srcLen < dstLen) {
		    dstLen = srcLen;
		}
		*srcLenPtr = srcLen;
		break;
	    }
	}
    }
    switch (chanPtr->inputTranslation) {
	case TCL_TRANSLATE_LF: {
	    if (dstStart != srcStart) {
		memcpy((VOID *) dstStart, (VOID *) srcStart, (size_t) dstLen);
	    }
	    srcLen = dstLen;
	    break;
	}
	case TCL_TRANSLATE_CR: {
	    char *dst, *dstEnd;
	    
	    if (dstStart != srcStart) {
		memcpy((VOID *) dstStart, (VOID *) srcStart, (size_t) dstLen);
	    }
	    dstEnd = dstStart + dstLen;
	    for (dst = dstStart; dst < dstEnd; dst++) {
		if (*dst == '\r') {
		    *dst = '\n';
		}
	    }
	    srcLen = dstLen;
	    break;
	}
	case TCL_TRANSLATE_CRLF: {
	    char *dst;
	    CONST char *src, *srcEnd, *srcMax;
	    
	    dst = dstStart;
	    src = srcStart;
	    srcEnd = srcStart + dstLen;
	    srcMax = srcStart + *srcLenPtr;

	    for ( ; src < srcEnd; ) {
		if (*src == '\r') {
		    src++;
		    if (src >= srcMax) {
			chanPtr->flags |= INPUT_NEED_NL;
		    } else if (*src == '\n') {
			*dst++ = *src++;
		    } else {
			*dst++ = '\r';
		    }
		} else {
		    *dst++ = *src++;
		}
	    }
	    srcLen = src - srcStart;
	    dstLen = dst - dstStart;
	    break;
	}
	case TCL_TRANSLATE_AUTO: {
	    char *dst;
	    CONST char *src, *srcEnd, *srcMax;

	    dst = dstStart;
	    src = srcStart;
	    srcEnd = srcStart + dstLen;
	    srcMax = srcStart + *srcLenPtr;

	    if ((chanPtr->flags & INPUT_SAW_CR) && (src < srcMax)) {
		if (*src == '\n') {
		    src++;
		}
		chanPtr->flags &= ~INPUT_SAW_CR;
	    }
	    for ( ; src < srcEnd; ) {
		if (*src == '\r') {
		    src++;
		    if (src >= srcMax) {
			chanPtr->flags |= INPUT_SAW_CR;
		    } else if (*src == '\n') {
			if (srcEnd < srcMax) {
			    srcEnd++;
			}
			src++;
		    }
		    *dst++ = '\n';
		} else {
		    *dst++ = *src++;
		}
	    }
	    srcLen = src - srcStart;
	    dstLen = dst - dstStart;
	    break;
	}
	default: {		/* lint. */
	    return 0;
	}
    }
    *dstLenPtr = dstLen;

    if ((eof != NULL) && (srcStart + srcLen >= eof)) {
	/*
	 * EOF character was seen in EOL translated range.  Leave current
	 * file position pointing at the EOF character, but don't store the
	 * EOF character in the output string.
	 */

	chanPtr->flags |= (CHANNEL_EOF | CHANNEL_STICKY_EOF);
	chanPtr->inputEncodingFlags |= TCL_ENCODING_END;
	chanPtr->flags &= ~(INPUT_SAW_CR | INPUT_NEED_NL);
	return 1;
    }

    *srcLenPtr = srcLen;
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Ungets --
 *
 *	Causes the supplied string to be added to the input queue of
 *	the channel, at either the head or tail of the queue.
 *
 * Results:
 *	The number of bytes stored in the channel, or -1 on error.
 *
 * Side effects:
 *	Adds input to the input queue of a channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Ungets(chan, str, len, atEnd)
    Tcl_Channel chan;		/* The channel for which to add the input. */
    char *str;			/* The input itself. */
    int len;			/* The length of the input. */
    int atEnd;			/* If non-zero, add at end of queue; otherwise
                                 * add at head of queue. */    
{
    Channel *chanPtr;		/* The real IO channel. */
    ChannelBuffer *bufPtr;	/* Buffer to contain the data. */
    int i, flags;

    chanPtr = (Channel *) chan;
    
    /*
     * CheckChannelErrors clears too many flag bits in this one case.
     */
     
    flags = chanPtr->flags;
    if (CheckChannelErrors(chanPtr, TCL_READABLE) != 0) {
	len = -1;
	goto done;
    }
    chanPtr->flags = flags;

    /*
     * If we have encountered a sticky EOF, just punt without storing.
     * (sticky EOF is set if we have seen the input eofChar, to prevent
     * reading beyond the eofChar). Otherwise, clear the EOF flags, and
     * clear the BLOCKED bit. We want to discover these conditions anew
     * in each operation.
     */

    if (chanPtr->flags & CHANNEL_STICKY_EOF) {
	goto done;
    }
    chanPtr->flags &= (~(CHANNEL_BLOCKED | CHANNEL_EOF));

    bufPtr = AllocChannelBuffer(len);
    for (i = 0; i < len; i++) {
        bufPtr->buf[i] = str[i];
    }
    bufPtr->nextAdded += len;

    if (chanPtr->inQueueHead == (ChannelBuffer *) NULL) {
        bufPtr->nextPtr = (ChannelBuffer *) NULL;
        chanPtr->inQueueHead = bufPtr;
        chanPtr->inQueueTail = bufPtr;
    } else if (atEnd) {
        bufPtr->nextPtr = (ChannelBuffer *) NULL;
        chanPtr->inQueueTail->nextPtr = bufPtr;
        chanPtr->inQueueTail = bufPtr;
    } else {
        bufPtr->nextPtr = chanPtr->inQueueHead;
        chanPtr->inQueueHead = bufPtr;
    }

    done:
    /*
     * Update the notifier state so we don't block while there is still
     * data in the buffers.
     */

    UpdateInterest(chanPtr);
    return len;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Flush --
 *
 *	Flushes output data on a channel.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May flush output queued on this channel.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Flush(chan)
    Tcl_Channel chan;			/* The Channel to flush. */
{
    int result;				/* Of calling FlushChannel. */
    Channel *chanPtr;			/* The actual channel. */

    chanPtr = (Channel *) chan;
    if (CheckChannelErrors(chanPtr, TCL_WRITABLE) != 0) {
	return -1;
    }

    /*
     * Force current output buffer to be output also.
     */
    
    if ((chanPtr->curOutPtr != NULL)
	    && (chanPtr->curOutPtr->nextAdded > 0)) {
        chanPtr->flags |= BUFFER_READY;
    }
    
    result = FlushChannel(NULL, chanPtr, 0);
    if (result != 0) {
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DiscardInputQueued --
 *
 *	Discards any input read from the channel but not yet consumed
 *	by Tcl reading commands.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May discard input from the channel. If discardLastBuffer is zero,
 *	leaves one buffer in place for back-filling.
 *
 *----------------------------------------------------------------------
 */

static void
DiscardInputQueued(chanPtr, discardSavedBuffers)
    Channel *chanPtr;		/* Channel on which to discard
                                 * the queued input. */
    int discardSavedBuffers;	/* If non-zero, discard all buffers including
                                 * last one. */
{
    ChannelBuffer *bufPtr, *nxtPtr;	/* Loop variables. */

    bufPtr = chanPtr->inQueueHead;
    chanPtr->inQueueHead = (ChannelBuffer *) NULL;
    chanPtr->inQueueTail = (ChannelBuffer *) NULL;
    for (; bufPtr != (ChannelBuffer *) NULL; bufPtr = nxtPtr) {
        nxtPtr = bufPtr->nextPtr;
        RecycleBuffer(chanPtr, bufPtr, discardSavedBuffers);
    }

    /*
     * If discardSavedBuffers is nonzero, must also discard any previously
     * saved buffer in the saveInBufPtr field.
     */
    
    if (discardSavedBuffers) {
        if (chanPtr->saveInBufPtr != (ChannelBuffer *) NULL) {
            ckfree((char *) chanPtr->saveInBufPtr);
            chanPtr->saveInBufPtr = (ChannelBuffer *) NULL;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetInput --
 *
 *	Reads input data from a device into a channel buffer.  
 *
 * Results:
 *	The return value is the Posix error code if an error occurred while
 *	reading from the file, or 0 otherwise.  
 *
 * Side effects:
 *	Reads from the underlying device.
 *
 *---------------------------------------------------------------------------
 */

static int
GetInput(chanPtr)
    Channel *chanPtr;		/* Channel to read input from. */
{
    int toRead;			/* How much to read? */
    int result;			/* Of calling driver. */
    int nread;			/* How much was read from channel? */
    ChannelBuffer *bufPtr;	/* New buffer to add to input queue. */

    /*
     * Prevent reading from a dead channel -- a channel that has been closed
     * but not yet deallocated, which can happen if the exit handler for
     * channel cleanup has run but the channel is still registered in some
     * interpreter.
     */
    
    if (CheckForDeadChannel(NULL, chanPtr)) {
	return EINVAL;
    }

    /*
     * See if we can fill an existing buffer. If we can, read only
     * as much as will fit in it. Otherwise allocate a new buffer,
     * add it to the input queue and attempt to fill it to the max.
     */

    bufPtr = chanPtr->inQueueTail;
    if ((bufPtr != NULL) && (bufPtr->nextAdded < bufPtr->bufLength)) {
        toRead = bufPtr->bufLength - bufPtr->nextAdded;
    } else {
	bufPtr = chanPtr->saveInBufPtr;
	chanPtr->saveInBufPtr = NULL;
	if (bufPtr == NULL) {
	    bufPtr = AllocChannelBuffer(chanPtr->bufSize);
	}
        bufPtr->nextPtr = (ChannelBuffer *) NULL;

        toRead = chanPtr->bufSize;
        if (chanPtr->inQueueTail == NULL) {
            chanPtr->inQueueHead = bufPtr;
        } else {
            chanPtr->inQueueTail->nextPtr = bufPtr;
        }
        chanPtr->inQueueTail = bufPtr;
    }
      
    /*
     * If EOF is set, we should avoid calling the driver because on some
     * platforms it is impossible to read from a device after EOF.
     */

    if (chanPtr->flags & CHANNEL_EOF) {
	return 0;
    }

    nread = (*chanPtr->typePtr->inputProc)(chanPtr->instanceData,
	    bufPtr->buf + bufPtr->nextAdded, toRead, &result);

    if (nread > 0) {
	bufPtr->nextAdded += nread;

	/*
	 * If we get a short read, signal up that we may be BLOCKED. We
	 * should avoid calling the driver because on some platforms we
	 * will block in the low level reading code even though the
	 * channel is set into nonblocking mode.
	 */
            
	if (nread < toRead) {
	    chanPtr->flags |= CHANNEL_BLOCKED;
	}
    } else if (nread == 0) {
	chanPtr->flags |= CHANNEL_EOF;
	chanPtr->inputEncodingFlags |= TCL_ENCODING_END;
    } else if (nread < 0) {
	if ((result == EWOULDBLOCK) || (result == EAGAIN)) {
	    chanPtr->flags |= CHANNEL_BLOCKED;
	    result = EAGAIN;
	}
	Tcl_SetErrno(result);
	return result;
    } 
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Seek --
 *
 *	Implements seeking on Tcl Channels. This is a public function
 *	so that other C facilities may be implemented on top of it.
 *
 * Results:
 *	The new access point or -1 on error. If error, use Tcl_GetErrno()
 *	to retrieve the POSIX error code for the error that occurred.
 *
 * Side effects:
 *	May flush output on the channel. May discard queued input.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Seek(chan, offset, mode)
    Tcl_Channel chan;		/* The channel on which to seek. */
    int offset;			/* Offset to seek to. */
    int mode;			/* Relative to which location to seek? */
{
    Channel *chanPtr;		/* The real IO channel. */
    ChannelBuffer *bufPtr;
    int inputBuffered, outputBuffered;
    int result;			/* Of device driver operations. */
    int curPos;			/* Position on the device. */
    int wasAsync;		/* Was the channel nonblocking before the
                                 * seek operation? If so, must restore to
                                 * nonblocking mode after the seek. */

    chanPtr = (Channel *) chan;
    if (CheckChannelErrors(chanPtr, TCL_WRITABLE | TCL_READABLE) != 0) {
	return -1;
    }

    /*
     * Disallow seek on dead channels -- channels that have been closed but
     * not yet been deallocated. Such channels can be found if the exit
     * handler for channel cleanup has run but the channel is still
     * registered in an interpreter.
     */

    if (CheckForDeadChannel(NULL,chanPtr)) return -1;

    /*
     * Disallow seek on channels whose type does not have a seek procedure
     * defined. This means that the channel does not support seeking.
     */

    if (chanPtr->typePtr->seekProc == (Tcl_DriverSeekProc *) NULL) {
        Tcl_SetErrno(EINVAL);
        return -1;
    }

    /*
     * Compute how much input and output is buffered. If both input and
     * output is buffered, cannot compute the current position.
     */

    for (bufPtr = chanPtr->inQueueHead, inputBuffered = 0;
             bufPtr != (ChannelBuffer *) NULL;
             bufPtr = bufPtr->nextPtr) {
        inputBuffered += (bufPtr->nextAdded - bufPtr->nextRemoved);
    }
    for (bufPtr = chanPtr->outQueueHead, outputBuffered = 0;
             bufPtr != (ChannelBuffer *) NULL;
             bufPtr = bufPtr->nextPtr) {
        outputBuffered += (bufPtr->nextAdded - bufPtr->nextRemoved);
    }
    if ((chanPtr->curOutPtr != (ChannelBuffer *) NULL) &&
           (chanPtr->curOutPtr->nextAdded > chanPtr->curOutPtr->nextRemoved)) {
        chanPtr->flags |= BUFFER_READY;
        outputBuffered +=
            (chanPtr->curOutPtr->nextAdded - chanPtr->curOutPtr->nextRemoved);
    }

    if ((inputBuffered != 0) && (outputBuffered != 0)) {
        Tcl_SetErrno(EFAULT);
        return -1;
    }

    /*
     * If we are seeking relative to the current position, compute the
     * corrected offset taking into account the amount of unread input.
     */

    if (mode == SEEK_CUR) {
        offset -= inputBuffered;
    }

    /*
     * Discard any queued input - this input should not be read after
     * the seek.
     */

    DiscardInputQueued(chanPtr, 0);

    /*
     * Reset EOF and BLOCKED flags. We invalidate them by moving the
     * access point. Also clear CR related flags.
     */

    chanPtr->flags &=
        (~(CHANNEL_EOF | CHANNEL_STICKY_EOF | CHANNEL_BLOCKED | INPUT_SAW_CR));
    
    /*
     * If the channel is in asynchronous output mode, switch it back
     * to synchronous mode and cancel any async flush that may be
     * scheduled. After the flush, the channel will be put back into
     * asynchronous output mode.
     */

    wasAsync = 0;
    if (chanPtr->flags & CHANNEL_NONBLOCKING) {
        wasAsync = 1;
        result = 0;
        if (chanPtr->typePtr->blockModeProc != NULL) {
            result = (chanPtr->typePtr->blockModeProc) (chanPtr->instanceData,
                    TCL_MODE_BLOCKING);
        }
        if (result != 0) {
            Tcl_SetErrno(result);
            return -1;
        }
        chanPtr->flags &= (~(CHANNEL_NONBLOCKING));
        if (chanPtr->flags & BG_FLUSH_SCHEDULED) {
            chanPtr->flags &= (~(BG_FLUSH_SCHEDULED));
        }
    }
    
    /*
     * If the flush fails we cannot recover the original position. In
     * that case the seek is not attempted because we do not know where
     * the access position is - instead we return the error. FlushChannel
     * has already called Tcl_SetErrno() to report the error upwards.
     * If the flush succeeds we do the seek also.
     */
    
    if (FlushChannel(NULL, chanPtr, 0) != 0) {
        curPos = -1;
    } else {

        /*
         * Now seek to the new position in the channel as requested by the
         * caller.
         */

        curPos = (chanPtr->typePtr->seekProc) (chanPtr->instanceData,
                (long) offset, mode, &result);
        if (curPos == -1) {
            Tcl_SetErrno(result);
        }
    }
    
    /*
     * Restore to nonblocking mode if that was the previous behavior.
     *
     * NOTE: Even if there was an async flush active we do not restore
     * it now because we already flushed all the queued output, above.
     */
    
    if (wasAsync) {
        chanPtr->flags |= CHANNEL_NONBLOCKING;
        result = 0;
        if (chanPtr->typePtr->blockModeProc != NULL) {
            result = (chanPtr->typePtr->blockModeProc) (chanPtr->instanceData,
                    TCL_MODE_NONBLOCKING);
        }
        if (result != 0) {
            Tcl_SetErrno(result);
            return -1;
        }
    }

    return curPos;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Tell --
 *
 *	Returns the position of the next character to be read/written on
 *	this channel.
 *
 * Results:
 *	A nonnegative integer on success, -1 on failure. If failed,
 *	use Tcl_GetErrno() to retrieve the POSIX error code for the
 *	error that occurred.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Tell(chan)
    Tcl_Channel chan;			/* The channel to return pos for. */
{
    Channel *chanPtr;			/* The actual channel to tell on. */
    ChannelBuffer *bufPtr;
    int inputBuffered, outputBuffered;
    int result;				/* Of calling device driver. */
    int curPos;				/* Position on device. */

    chanPtr = (Channel *) chan;
    if (CheckChannelErrors(chanPtr, TCL_WRITABLE | TCL_READABLE) != 0) {
	return -1;
    }

    /*
     * Disallow tell on dead channels -- channels that have been closed but
     * not yet been deallocated. Such channels can be found if the exit
     * handler for channel cleanup has run but the channel is still
     * registered in an interpreter.
     */

    if (CheckForDeadChannel(NULL,chanPtr)) {
	return -1;
    }

    /*
     * Disallow tell on channels whose type does not have a seek procedure
     * defined. This means that the channel does not support seeking.
     */

    if (chanPtr->typePtr->seekProc == (Tcl_DriverSeekProc *) NULL) {
        Tcl_SetErrno(EINVAL);
        return -1;
    }

    /*
     * Compute how much input and output is buffered. If both input and
     * output is buffered, cannot compute the current position.
     */

    for (bufPtr = chanPtr->inQueueHead, inputBuffered = 0;
             bufPtr != (ChannelBuffer *) NULL;
             bufPtr = bufPtr->nextPtr) {
        inputBuffered += (bufPtr->nextAdded - bufPtr->nextRemoved);
    }
    for (bufPtr = chanPtr->outQueueHead, outputBuffered = 0;
             bufPtr != (ChannelBuffer *) NULL;
             bufPtr = bufPtr->nextPtr) {
        outputBuffered += (bufPtr->nextAdded - bufPtr->nextRemoved);
    }
    if ((chanPtr->curOutPtr != (ChannelBuffer *) NULL) &&
           (chanPtr->curOutPtr->nextAdded > chanPtr->curOutPtr->nextRemoved)) {
        chanPtr->flags |= BUFFER_READY;
        outputBuffered +=
            (chanPtr->curOutPtr->nextAdded - chanPtr->curOutPtr->nextRemoved);
    }

    if ((inputBuffered != 0) && (outputBuffered != 0)) {
        Tcl_SetErrno(EFAULT);
        return -1;
    }

    /*
     * Get the current position in the device and compute the position
     * where the next character will be read or written.
     */

    curPos = (chanPtr->typePtr->seekProc) (chanPtr->instanceData,
            (long) 0, SEEK_CUR, &result);
    if (curPos == -1) {
        Tcl_SetErrno(result);
        return -1;
    }
    if (inputBuffered != 0) {
        return (curPos - inputBuffered);
    }
    return (curPos + outputBuffered);
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckChannelErrors --
 *
 *	See if the channel is in an ready state and can perform the
 *	desired operation.
 *
 * Results:
 *	The return value is 0 if the channel is OK, otherwise the
 *	return value is -1 and errno is set to indicate the error.
 *
 * Side effects:
 *	May clear the EOF and/or BLOCKED bits if reading from channel.
 *
 *---------------------------------------------------------------------------
 */
 
static int
CheckChannelErrors(chanPtr, direction)
    Channel *chanPtr;	    /* Channel to check. */
    int direction;	    /* Test if channel supports desired operation:
			     * TCL_READABLE, TCL_WRITABLE. */
{
    /*
     * Check for unreported error.
     */

    if (chanPtr->unreportedError != 0) {
        Tcl_SetErrno(chanPtr->unreportedError);
        chanPtr->unreportedError = 0;
        return -1;
    }

    /*
     * Fail if the channel is not opened for desired operation.
     */

    if ((chanPtr->flags & direction) == 0) {
        Tcl_SetErrno(EACCES);
        return -1;
    }

    /*
     * Fail if the channel is in the middle of a background copy.
     */

    if (chanPtr->csPtr != NULL) {
	Tcl_SetErrno(EBUSY);
	return -1;
    }

    if (direction == TCL_READABLE) {
	/*
	 * If we have not encountered a sticky EOF, clear the EOF bit
	 * (sticky EOF is set if we have seen the input eofChar, to prevent
	 * reading beyond the eofChar). Also, always clear the BLOCKED bit.
	 * We want to discover these conditions anew in each operation.
	 */
	
	if ((chanPtr->flags & CHANNEL_STICKY_EOF) == 0) {
	    chanPtr->flags &= ~CHANNEL_EOF;
	}
	chanPtr->flags &= ~(CHANNEL_BLOCKED | CHANNEL_NEED_MORE_DATA);
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Eof --
 *
 *	Returns 1 if the channel is at EOF, 0 otherwise.
 *
 * Results:
 *	1 or 0, always.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Eof(chan)
    Tcl_Channel chan;			/* Does this channel have EOF? */
{
    Channel *chanPtr;		/* The real channel structure. */

    chanPtr = (Channel *) chan;
    return ((chanPtr->flags & CHANNEL_STICKY_EOF) ||
            ((chanPtr->flags & CHANNEL_EOF) && (Tcl_InputBuffered(chan) == 0)))
        ? 1 : 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InputBlocked --
 *
 *	Returns 1 if input is blocked on this channel, 0 otherwise.
 *
 * Results:
 *	0 or 1, always.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_InputBlocked(chan)
    Tcl_Channel chan;			/* Is this channel blocked? */
{
    Channel *chanPtr;		/* The real channel structure. */

    chanPtr = (Channel *) chan;
    return (chanPtr->flags & CHANNEL_BLOCKED) ? 1 : 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InputBuffered --
 *
 *	Returns the number of bytes of input currently buffered in the
 *	internal buffer of a channel.
 *
 * Results:
 *	The number of input bytes buffered, or zero if the channel is not
 *	open for reading.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_InputBuffered(chan)
    Tcl_Channel chan;			/* The channel to query. */
{
    Channel *chanPtr;
    int bytesBuffered;
    ChannelBuffer *bufPtr;

    chanPtr = (Channel *) chan;
    for (bytesBuffered = 0, bufPtr = chanPtr->inQueueHead;
             bufPtr != (ChannelBuffer *) NULL;
             bufPtr = bufPtr->nextPtr) {
        bytesBuffered += (bufPtr->nextAdded - bufPtr->nextRemoved);
    }
    return bytesBuffered;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetChannelBufferSize --
 *
 *	Sets the size of buffers to allocate to store input or output
 *	in the channel. The size must be between 10 bytes and 1 MByte.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the size of buffers subsequently allocated for this channel.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetChannelBufferSize(chan, sz)
    Tcl_Channel chan;			/* The channel whose buffer size
                                         * to set. */
    int sz;				/* The size to set. */
{
    Channel *chanPtr;
    
    /*
     * If the buffer size is smaller than 10 bytes or larger than one MByte,
     * do not accept the requested size and leave the current buffer size.
     */
    
    if (sz < 10) {
        return;
    }
    if (sz > (1024 * 1024)) {
        return;
    }

    chanPtr = (Channel *) chan;
    chanPtr->bufSize = sz;

    if (chanPtr->outputStage != NULL) {
	ckfree((char *) chanPtr->outputStage);
	chanPtr->outputStage = NULL;
    }
    if ((chanPtr->encoding != NULL) && (chanPtr->flags & TCL_WRITABLE)) {
	chanPtr->outputStage = (char *)
		ckalloc((unsigned) (chanPtr->bufSize + 2));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelBufferSize --
 *
 *	Retrieves the size of buffers to allocate for this channel.
 *
 * Results:
 *	The size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetChannelBufferSize(chan)
    Tcl_Channel chan;		/* The channel for which to find the
                                 * buffer size. */
{
    Channel *chanPtr;

    chanPtr = (Channel *) chan;
    return chanPtr->bufSize;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_BadChannelOption --
 *
 *	This procedure generates a "bad option" error message in an
 *	(optional) interpreter.  It is used by channel drivers when 
 *      a invalid Set/Get option is requested. Its purpose is to concatenate
 *      the generic options list to the specific ones and factorize
 *      the generic options error message string.
 *
 * Results:
 *	TCL_ERROR.
 *
 * Side effects:
 *	An error message is generated in interp's result object to
 *	indicate that a command was invoked with the a bad option
 *	The message has the form
 *		bad option "blah": should be one of 
 *              <...generic options...>+<...specific options...>
 *	"blah" is the optionName argument and "<specific options>"
 *	is a space separated list of specific option words.
 *      The function takes good care of inserting minus signs before
 *      each option, commas after, and an "or" before the last option.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_BadChannelOption(interp, optionName, optionList)
    Tcl_Interp *interp;			/* Current interpreter. (can be NULL)*/
    char *optionName;			/* 'bad option' name */
    char *optionList;			/* Specific options list to append 
					 * to the standard generic options.
					 * can be NULL for generic options 
					 * only.
					 */
{
    if (interp) {
	CONST char *genericopt = 
	    	"blocking buffering buffersize encoding eofchar translation";
	char **argv;
	int  argc, i;
	Tcl_DString ds;

	Tcl_DStringInit(&ds);
	Tcl_DStringAppend(&ds, (char *) genericopt, -1);
	if (optionList && (*optionList)) {
	    Tcl_DStringAppend(&ds, " ", 1);
	    Tcl_DStringAppend(&ds, optionList, -1);
	}
	if (Tcl_SplitList(interp, Tcl_DStringValue(&ds), 
	      	  &argc, &argv) != TCL_OK) {
	    panic("malformed option list in channel driver");
	}
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "bad option \"", optionName, 
		 "\": should be one of ", (char *) NULL);
	argc--;
	for (i = 0; i < argc; i++) {
	    Tcl_AppendResult(interp, "-", argv[i], ", ", (char *) NULL);
	}
	Tcl_AppendResult(interp, "or -", argv[i], (char *) NULL);
	Tcl_DStringFree(&ds);
	ckfree((char *) argv);
    }
    Tcl_SetErrno(EINVAL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelOption --
 *
 *	Gets a mode associated with an IO channel. If the optionName arg
 *	is non NULL, retrieves the value of that option. If the optionName
 *	arg is NULL, retrieves a list of alternating option names and
 *	values for the given channel.
 *
 * Results:
 *	A standard Tcl result. Also sets the supplied DString to the
 *	string value of the option(s) returned.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetChannelOption(interp, chan, optionName, dsPtr)
    Tcl_Interp *interp;		/* For error reporting - can be NULL. */
    Tcl_Channel chan;		/* Channel on which to get option. */
    char *optionName;		/* Option to get. */
    Tcl_DString *dsPtr;		/* Where to store value(s). */
{
    size_t len;			/* Length of optionName string. */
    char optionVal[128];	/* Buffer for sprintf. */
    Channel *chanPtr = (Channel *) chan;
    int flags;

    /*
     * If we are in the middle of a background copy, use the saved flags.
     */

    if (chanPtr->csPtr) {
	if (chanPtr == chanPtr->csPtr->readPtr) {
	    flags = chanPtr->csPtr->readFlags;
	} else {
	    flags = chanPtr->csPtr->writeFlags;
	}
    } else {
	flags = chanPtr->flags;
    }

    /*
     * Disallow options on dead channels -- channels that have been closed but
     * not yet been deallocated. Such channels can be found if the exit
     * handler for channel cleanup has run but the channel is still
     * registered in an interpreter.
     */

    if (CheckForDeadChannel(interp,chanPtr)) return TCL_ERROR;

    /*
     * If the optionName is NULL it means that we want a list of all
     * options and values.
     */
    
    if (optionName == (char *) NULL) {
        len = 0;
    } else {
        len = strlen(optionName);
    }
    
    if ((len == 0) || ((len > 2) && (optionName[1] == 'b') &&
            (strncmp(optionName, "-blocking", len) == 0))) {
        if (len == 0) {
            Tcl_DStringAppendElement(dsPtr, "-blocking");
        }
        Tcl_DStringAppendElement(dsPtr,
		(flags & CHANNEL_NONBLOCKING) ? "0" : "1");
        if (len > 0) {
            return TCL_OK;
        }
    }
    if ((len == 0) || ((len > 7) && (optionName[1] == 'b') &&
            (strncmp(optionName, "-buffering", len) == 0))) {
        if (len == 0) {
            Tcl_DStringAppendElement(dsPtr, "-buffering");
        }
        if (flags & CHANNEL_LINEBUFFERED) {
            Tcl_DStringAppendElement(dsPtr, "line");
        } else if (flags & CHANNEL_UNBUFFERED) {
            Tcl_DStringAppendElement(dsPtr, "none");
        } else {
            Tcl_DStringAppendElement(dsPtr, "full");
        }
        if (len > 0) {
            return TCL_OK;
        }
    }
    if ((len == 0) || ((len > 7) && (optionName[1] == 'b') &&
            (strncmp(optionName, "-buffersize", len) == 0))) {
        if (len == 0) {
            Tcl_DStringAppendElement(dsPtr, "-buffersize");
        }
        TclFormatInt(optionVal, chanPtr->bufSize);
        Tcl_DStringAppendElement(dsPtr, optionVal);
        if (len > 0) {
            return TCL_OK;
        }
    }
    if ((len == 0) ||
	    ((len > 2) && (optionName[1] == 'e') &&
		    (strncmp(optionName, "-encoding", len) == 0))) {
	if (len == 0) {
	    Tcl_DStringAppendElement(dsPtr, "-encoding");
	}
	if (chanPtr->encoding == NULL) {
	    Tcl_DStringAppendElement(dsPtr, "binary");
	} else {
	    Tcl_DStringAppendElement(dsPtr,
		    Tcl_GetEncodingName(chanPtr->encoding));
	}
	if (len > 0) {
	    return TCL_OK;
	}
    }
    if ((len == 0) ||
            ((len > 2) && (optionName[1] == 'e') &&
                    (strncmp(optionName, "-eofchar", len) == 0))) {
        if (len == 0) {
            Tcl_DStringAppendElement(dsPtr, "-eofchar");
        }
        if (((flags & (TCL_READABLE|TCL_WRITABLE)) ==
                (TCL_READABLE|TCL_WRITABLE)) && (len == 0)) {
            Tcl_DStringStartSublist(dsPtr);
        }
        if (flags & TCL_READABLE) {
            if (chanPtr->inEofChar == 0) {
                Tcl_DStringAppendElement(dsPtr, "");
            } else {
                char buf[4];

                sprintf(buf, "%c", chanPtr->inEofChar);
                Tcl_DStringAppendElement(dsPtr, buf);
            }
        }
        if (flags & TCL_WRITABLE) {
            if (chanPtr->outEofChar == 0) {
                Tcl_DStringAppendElement(dsPtr, "");
            } else {
                char buf[4];

                sprintf(buf, "%c", chanPtr->outEofChar);
                Tcl_DStringAppendElement(dsPtr, buf);
            }
        }
        if (((flags & (TCL_READABLE|TCL_WRITABLE)) ==
                (TCL_READABLE|TCL_WRITABLE)) && (len == 0)) {
            Tcl_DStringEndSublist(dsPtr);
        }
        if (len > 0) {
            return TCL_OK;
        }
    }
    if ((len == 0) ||
            ((len > 1) && (optionName[1] == 't') &&
                    (strncmp(optionName, "-translation", len) == 0))) {
        if (len == 0) {
            Tcl_DStringAppendElement(dsPtr, "-translation");
        }
        if (((flags & (TCL_READABLE|TCL_WRITABLE)) ==
                (TCL_READABLE|TCL_WRITABLE)) && (len == 0)) {
            Tcl_DStringStartSublist(dsPtr);
        }
        if (flags & TCL_READABLE) {
            if (chanPtr->inputTranslation == TCL_TRANSLATE_AUTO) {
                Tcl_DStringAppendElement(dsPtr, "auto");
            } else if (chanPtr->inputTranslation == TCL_TRANSLATE_CR) {
                Tcl_DStringAppendElement(dsPtr, "cr");
            } else if (chanPtr->inputTranslation == TCL_TRANSLATE_CRLF) {
                Tcl_DStringAppendElement(dsPtr, "crlf");
            } else {
                Tcl_DStringAppendElement(dsPtr, "lf");
            }
        }
        if (flags & TCL_WRITABLE) {
            if (chanPtr->outputTranslation == TCL_TRANSLATE_AUTO) {
                Tcl_DStringAppendElement(dsPtr, "auto");
            } else if (chanPtr->outputTranslation == TCL_TRANSLATE_CR) {
                Tcl_DStringAppendElement(dsPtr, "cr");
            } else if (chanPtr->outputTranslation == TCL_TRANSLATE_CRLF) {
                Tcl_DStringAppendElement(dsPtr, "crlf");
            } else {
                Tcl_DStringAppendElement(dsPtr, "lf");
            }
        }
        if (((flags & (TCL_READABLE|TCL_WRITABLE)) ==
                (TCL_READABLE|TCL_WRITABLE)) && (len == 0)) {
            Tcl_DStringEndSublist(dsPtr);
        }
        if (len > 0) {
            return TCL_OK;
        }
    }
    if (chanPtr->typePtr->getOptionProc != (Tcl_DriverGetOptionProc *) NULL) {
	/*
	 * let the driver specific handle additional options
	 * and result code and message.
	 */

        return (chanPtr->typePtr->getOptionProc) (chanPtr->instanceData,
		  interp, optionName, dsPtr);
    } else {
	/*
	 * no driver specific options case.
	 */

        if (len == 0) {
            return TCL_OK;
        }
	return Tcl_BadChannelOption(interp, optionName, NULL);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_SetChannelOption --
 *
 *	Sets an option on a channel.
 *
 * Results:
 *	A standard Tcl result.  On error, sets interp's result object
 *	if interp is not NULL.
 *
 * Side effects:
 *	May modify an option on a device.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_SetChannelOption(interp, chan, optionName, newValue)
    Tcl_Interp *interp;		/* For error reporting - can be NULL. */
    Tcl_Channel chan;		/* Channel on which to set mode. */
    char *optionName;		/* Which option to set? */
    char *newValue;		/* New value for option. */
{
    int newMode;		/* New (numeric) mode to sert. */
    Channel *chanPtr;		/* The real IO channel. */
    size_t len;			/* Length of optionName string. */
    int argc;
    char **argv;
    
    chanPtr = (Channel *) chan;

    /*
     * If the channel is in the middle of a background copy, fail.
     */

    if (chanPtr->csPtr) {
	if (interp) {
	    Tcl_AppendResult(interp,
	         "unable to set channel options: background copy in progress",
		 (char *) NULL);
	}
        return TCL_ERROR;
    }


    /*
     * Disallow options on dead channels -- channels that have been closed but
     * not yet been deallocated. Such channels can be found if the exit
     * handler for channel cleanup has run but the channel is still
     * registered in an interpreter.
     */

    if (CheckForDeadChannel(NULL,chanPtr)) return TCL_ERROR;
    
    len = strlen(optionName);

    if ((len > 2) && (optionName[1] == 'b') &&
            (strncmp(optionName, "-blocking", len) == 0)) {
        if (Tcl_GetBoolean(interp, newValue, &newMode) == TCL_ERROR) {
            return TCL_ERROR;
        }
        if (newMode) {
            newMode = TCL_MODE_BLOCKING;
        } else {
            newMode = TCL_MODE_NONBLOCKING;
        }
	return SetBlockMode(interp, chanPtr, newMode);
    } else if ((len > 7) && (optionName[1] == 'b') &&
            (strncmp(optionName, "-buffering", len) == 0)) {
        len = strlen(newValue);
        if ((newValue[0] == 'f') && (strncmp(newValue, "full", len) == 0)) {
            chanPtr->flags &=
                (~(CHANNEL_UNBUFFERED|CHANNEL_LINEBUFFERED));
        } else if ((newValue[0] == 'l') &&
                (strncmp(newValue, "line", len) == 0)) {
            chanPtr->flags &= (~(CHANNEL_UNBUFFERED));
            chanPtr->flags |= CHANNEL_LINEBUFFERED;
        } else if ((newValue[0] == 'n') &&
                (strncmp(newValue, "none", len) == 0)) {
            chanPtr->flags &= (~(CHANNEL_LINEBUFFERED));
            chanPtr->flags |= CHANNEL_UNBUFFERED;
        } else {
            if (interp) {
                Tcl_AppendResult(interp, "bad value for -buffering: ",
                        "must be one of full, line, or none",
                        (char *) NULL);
                return TCL_ERROR;
            }
        }
	return TCL_OK;
    } else if ((len > 7) && (optionName[1] == 'b') &&
            (strncmp(optionName, "-buffersize", len) == 0)) {
        chanPtr->bufSize = atoi(newValue);	/* INTL: "C", UTF safe. */
        if ((chanPtr->bufSize < 10) || (chanPtr->bufSize > (1024 * 1024))) {
            chanPtr->bufSize = CHANNELBUFFER_DEFAULT_SIZE;
        }
    } else if ((len > 2) && (optionName[1] == 'e') &&
	    (strncmp(optionName, "-encoding", len) == 0)) {
	Tcl_Encoding encoding;

	if ((newValue[0] == '\0') || (strcmp(newValue, "binary") == 0)) {
	    encoding = NULL;
	} else {
	    encoding = Tcl_GetEncoding(interp, newValue);
	    if (encoding == NULL) {
		return TCL_ERROR;
	    }
	}
	Tcl_FreeEncoding(chanPtr->encoding);
	chanPtr->encoding = encoding;
	chanPtr->inputEncodingState = NULL;
	chanPtr->inputEncodingFlags = TCL_ENCODING_START;
	chanPtr->outputEncodingState = NULL;
	chanPtr->outputEncodingFlags = TCL_ENCODING_START;
	chanPtr->flags &= ~CHANNEL_NEED_MORE_DATA;
	UpdateInterest(chanPtr);
    } else if ((len > 2) && (optionName[1] == 'e') &&
            (strncmp(optionName, "-eofchar", len) == 0)) {
        if (Tcl_SplitList(interp, newValue, &argc, &argv) == TCL_ERROR) {
            return TCL_ERROR;
        }
        if (argc == 0) {
            chanPtr->inEofChar = 0;
            chanPtr->outEofChar = 0;
        } else if (argc == 1) {
            if (chanPtr->flags & TCL_WRITABLE) {
                chanPtr->outEofChar = (int) argv[0][0];
            }
            if (chanPtr->flags & TCL_READABLE) {
                chanPtr->inEofChar = (int) argv[0][0];
            }
        } else if (argc != 2) {
            if (interp) {
                Tcl_AppendResult(interp,
                        "bad value for -eofchar: should be a list of one or",
                        " two elements", (char *) NULL);
            }
            ckfree((char *) argv);
            return TCL_ERROR;
        } else {
            if (chanPtr->flags & TCL_READABLE) {
                chanPtr->inEofChar = (int) argv[0][0];
            }
            if (chanPtr->flags & TCL_WRITABLE) {
                chanPtr->outEofChar = (int) argv[1][0];
            }
        }
        if (argv != (char **) NULL) {
            ckfree((char *) argv);
        }
	return TCL_OK;
    } else if ((len > 1) && (optionName[1] == 't') &&
            (strncmp(optionName, "-translation", len) == 0)) {
	char *readMode, *writeMode;

        if (Tcl_SplitList(interp, newValue, &argc, &argv) == TCL_ERROR) {
            return TCL_ERROR;
        }

        if (argc == 1) {
	    readMode = (chanPtr->flags & TCL_READABLE) ? argv[0] : NULL;
	    writeMode = (chanPtr->flags & TCL_WRITABLE) ? argv[0] : NULL;
	} else if (argc == 2) {
	    readMode = (chanPtr->flags & TCL_READABLE) ? argv[0] : NULL;
	    writeMode = (chanPtr->flags & TCL_WRITABLE) ? argv[1] : NULL;
	} else {
            if (interp) {
                Tcl_AppendResult(interp,
                        "bad value for -translation: must be a one or two",
                        " element list", (char *) NULL);
            }
            ckfree((char *) argv);
            return TCL_ERROR;
	}

	if (readMode) {
	    if (*readMode == '\0') {
		newMode = chanPtr->inputTranslation;
	    } else if (strcmp(readMode, "auto") == 0) {
		newMode = TCL_TRANSLATE_AUTO;
	    } else if (strcmp(readMode, "binary") == 0) {
		newMode = TCL_TRANSLATE_LF;
		chanPtr->inEofChar = 0;
		Tcl_FreeEncoding(chanPtr->encoding);		    
		chanPtr->encoding = NULL;
	    } else if (strcmp(readMode, "lf") == 0) {
		newMode = TCL_TRANSLATE_LF;
	    } else if (strcmp(readMode, "cr") == 0) {
		newMode = TCL_TRANSLATE_CR;
	    } else if (strcmp(readMode, "crlf") == 0) {
		newMode = TCL_TRANSLATE_CRLF;
	    } else if (strcmp(readMode, "platform") == 0) {
		newMode = TCL_PLATFORM_TRANSLATION;
	    } else {
		if (interp) {
		    Tcl_AppendResult(interp,
			    "bad value for -translation: ",
			    "must be one of auto, binary, cr, lf, crlf,",
			    " or platform", (char *) NULL);
		}
		ckfree((char *) argv);
		return TCL_ERROR;
	    }

	    /*
	     * Reset the EOL flags since we need to look at any buffered
	     * data to see if the new translation mode allows us to
	     * complete the line.
	     */

	    if (newMode != chanPtr->inputTranslation) {
		chanPtr->inputTranslation = (Tcl_EolTranslation) newMode;
		chanPtr->flags &= ~(INPUT_SAW_CR);
		chanPtr->flags &= ~(CHANNEL_NEED_MORE_DATA);
		UpdateInterest(chanPtr);
	    }
	}
	if (writeMode) {
	    if (*writeMode == '\0') {
		/* Do nothing. */
	    } else if (strcmp(writeMode, "auto") == 0) {
		/*
		 * This is a hack to get TCP sockets to produce output
		 * in CRLF mode if they are being set into AUTO mode.
		 * A better solution for achieving this effect will be
		 * coded later.
		 */

		if (strcmp(chanPtr->typePtr->typeName, "tcp") == 0) {
		    chanPtr->outputTranslation = TCL_TRANSLATE_CRLF;
		} else {
		    chanPtr->outputTranslation = TCL_PLATFORM_TRANSLATION;
		}
	    } else if (strcmp(writeMode, "binary") == 0) {
		chanPtr->outEofChar = 0;
		chanPtr->outputTranslation = TCL_TRANSLATE_LF;
		Tcl_FreeEncoding(chanPtr->encoding);		    
		chanPtr->encoding = NULL;
	    } else if (strcmp(writeMode, "lf") == 0) {
		chanPtr->outputTranslation = TCL_TRANSLATE_LF;
	    } else if (strcmp(writeMode, "cr") == 0) {
		chanPtr->outputTranslation = TCL_TRANSLATE_CR;
	    } else if (strcmp(writeMode, "crlf") == 0) {
		chanPtr->outputTranslation = TCL_TRANSLATE_CRLF;
	    } else if (strcmp(writeMode, "platform") == 0) {
		chanPtr->outputTranslation = TCL_PLATFORM_TRANSLATION;
	    } else {
		if (interp) {
		    Tcl_AppendResult(interp,
			    "bad value for -translation: ",
			    "must be one of auto, binary, cr, lf, crlf,",
			    " or platform", (char *) NULL);
		}
		ckfree((char *) argv);
		return TCL_ERROR;
	    }
	}
        ckfree((char *) argv);            
        return TCL_OK;
    } else if (chanPtr->typePtr->setOptionProc != NULL) {
        return (*chanPtr->typePtr->setOptionProc)(chanPtr->instanceData,
                interp, optionName, newValue);
    } else {
	return Tcl_BadChannelOption(interp, optionName, (char *) NULL);
    }

    /*
     * If bufsize changes, need to get rid of old utility buffer.
     */

    if (chanPtr->saveInBufPtr != NULL) {
	RecycleBuffer(chanPtr, chanPtr->saveInBufPtr, 1);
	chanPtr->saveInBufPtr = NULL;
    }
    if (chanPtr->inQueueHead != NULL) {
	if ((chanPtr->inQueueHead->nextPtr == NULL)
		&& (chanPtr->inQueueHead->nextAdded ==
			chanPtr->inQueueHead->nextRemoved)) {
	    RecycleBuffer(chanPtr, chanPtr->inQueueHead, 1);
	    chanPtr->inQueueHead = NULL;
	    chanPtr->inQueueTail = NULL;
	}
    }

    /*
     * If encoding or bufsize changes, need to update output staging buffer.
     */

    if (chanPtr->outputStage != NULL) {
	ckfree((char *) chanPtr->outputStage);
	chanPtr->outputStage = NULL;
    }
    if ((chanPtr->encoding != NULL) && (chanPtr->flags & TCL_WRITABLE)) {
	chanPtr->outputStage = (char *) 
		ckalloc((unsigned) (chanPtr->bufSize + 2));
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CleanupChannelHandlers --
 *
 *	Removes channel handlers that refer to the supplied interpreter,
 *	so that if the actual channel is not closed now, these handlers
 *	will not run on subsequent events on the channel. This would be
 *	erroneous, because the interpreter no longer has a reference to
 *	this channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes channel handlers.
 *
 *----------------------------------------------------------------------
 */

static void
CleanupChannelHandlers(interp, chanPtr)
    Tcl_Interp *interp;
    Channel *chanPtr;
{
    EventScriptRecord *sPtr, *prevPtr, *nextPtr;

    /*
     * Remove fileevent records on this channel that refer to the
     * given interpreter.
     */
    
    for (sPtr = chanPtr->scriptRecordPtr,
             prevPtr = (EventScriptRecord *) NULL;
             sPtr != (EventScriptRecord *) NULL;
             sPtr = nextPtr) {
        nextPtr = sPtr->nextPtr;
        if (sPtr->interp == interp) {
            if (prevPtr == (EventScriptRecord *) NULL) {
                chanPtr->scriptRecordPtr = nextPtr;
            } else {
                prevPtr->nextPtr = nextPtr;
            }

            Tcl_DeleteChannelHandler((Tcl_Channel) chanPtr,
                    ChannelEventScriptInvoker, (ClientData) sPtr);

	    Tcl_DecrRefCount(sPtr->scriptPtr);
            ckfree((char *) sPtr);
        } else {
            prevPtr = sPtr;
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_NotifyChannel --
 *
 *	This procedure is called by a channel driver when a driver
 *	detects an event on a channel.  This procedure is responsible
 *	for actually handling the event by invoking any channel
 *	handler callbacks.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whatever the channel handler callback procedure does.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_NotifyChannel(channel, mask)
    Tcl_Channel channel;	/* Channel that detected an event. */
    int mask;			/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, or TCL_EXCEPTION: indicates
				 * which events were detected. */
{
    Channel *chanPtr = (Channel *) channel;
    ChannelHandler *chPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    NextChannelHandler nh;

    /* Walk all channels in a stack ! and notify them in order.
     */

    while (chanPtr !=  (Channel *) NULL) {
        /*
	 * Preserve the channel struct in case the script closes it.
	 */
     
        Tcl_Preserve((ClientData) channel);

	/*
	 * If we are flushing in the background, be sure to call FlushChannel
	 * for writable events.  Note that we have to discard the writable
	 * event so we don't call any write handlers before the flush is
	 * complete.
	 */

	if ((chanPtr->flags & BG_FLUSH_SCHEDULED) && (mask & TCL_WRITABLE)) {
	    FlushChannel(NULL, chanPtr, 1);
	    mask &= ~TCL_WRITABLE;
	}

	/*
	 * Add this invocation to the list of recursive invocations of
	 * ChannelHandlerEventProc.
	 */
    
	nh.nextHandlerPtr = (ChannelHandler *) NULL;
	nh.nestedHandlerPtr = tsdPtr->nestedHandlerPtr;
	tsdPtr->nestedHandlerPtr = &nh;
    
	for (chPtr = chanPtr->chPtr; chPtr != (ChannelHandler *) NULL; ) {

	    /*
	     * If this channel handler is interested in any of the events that
	     * have occurred on the channel, invoke its procedure.
	     */
        
	  if ((chPtr->mask & mask) != 0) {
              nh.nextHandlerPtr = chPtr->nextPtr;
	      (*(chPtr->proc))(chPtr->clientData, mask);
	      chPtr = nh.nextHandlerPtr;
	  } else {
              chPtr = chPtr->nextPtr;
	  }
	}

	/*
	 * Update the notifier interest, since it may have changed after
	 * invoking event handlers. Skip that if the channel was deleted
	 * in the call to the channel handler.
	 */

	if (chanPtr->typePtr != NULL) {
	    UpdateInterest(chanPtr);

	    /* Walk down the stack.
	     */
	  chanPtr = chanPtr-> supercedes;
	} else {
	    /* Stop walking the chain, the whole stack was destroyed!
	     */
	    chanPtr = (Channel*) NULL;
	}

	Tcl_Release((ClientData) channel);

	tsdPtr->nestedHandlerPtr = nh.nestedHandlerPtr;

	channel = (Tcl_Channel) chanPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateInterest --
 *
 *	Arrange for the notifier to call us back at appropriate times
 *	based on the current state of the channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May schedule a timer or driver handler.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateInterest(chanPtr)
    Channel *chanPtr;		/* Channel to update. */
{
    int mask = chanPtr->interestMask;

    /*
     * If there are flushed buffers waiting to be written, then
     * we need to watch for the channel to become writable.
     */

    if (chanPtr->flags & BG_FLUSH_SCHEDULED) {
	mask |= TCL_WRITABLE;
    }

    /*
     * If there is data in the input queue, and we aren't waiting for more
     * data, then we need to schedule a timer so we don't block in the
     * notifier.  Also, cancel the read interest so we don't get duplicate
     * events.
     */

    if (mask & TCL_READABLE) {
	if (!(chanPtr->flags & CHANNEL_NEED_MORE_DATA)
		&& (chanPtr->inQueueHead != (ChannelBuffer *) NULL)
		&& (chanPtr->inQueueHead->nextRemoved <
			chanPtr->inQueueHead->nextAdded)) {
	    mask &= ~TCL_READABLE;
	    if (!chanPtr->timer) {
		chanPtr->timer = Tcl_CreateTimerHandler(0, ChannelTimerProc,
			(ClientData) chanPtr);
	    }
	}
    }
    (chanPtr->typePtr->watchProc)(chanPtr->instanceData, mask);
}

/*
 *----------------------------------------------------------------------
 *
 * ChannelTimerProc --
 *
 *	Timer handler scheduled by UpdateInterest to monitor the
 *	channel buffers until they are empty.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May invoke channel handlers.
 *
 *----------------------------------------------------------------------
 */

static void
ChannelTimerProc(clientData)
    ClientData clientData;
{
    Channel *chanPtr = (Channel *) clientData;

    if (!(chanPtr->flags & CHANNEL_NEED_MORE_DATA)
	    && (chanPtr->interestMask & TCL_READABLE)
	    && (chanPtr->inQueueHead != (ChannelBuffer *) NULL)
	    && (chanPtr->inQueueHead->nextRemoved <
		    chanPtr->inQueueHead->nextAdded)) {
	/*
	 * Restart the timer in case a channel handler reenters the
	 * event loop before UpdateInterest gets called by Tcl_NotifyChannel.
	 */

	chanPtr->timer = Tcl_CreateTimerHandler(0, ChannelTimerProc,
			(ClientData) chanPtr);
	Tcl_NotifyChannel((Tcl_Channel)chanPtr, TCL_READABLE);
 
   } else {
	chanPtr->timer = NULL;
	UpdateInterest(chanPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateChannelHandler --
 *
 *	Arrange for a given procedure to be invoked whenever the
 *	channel indicated by the chanPtr arg becomes readable or
 *	writable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	From now on, whenever the I/O channel given by chanPtr becomes
 *	ready in the way indicated by mask, proc will be invoked.
 *	See the manual entry for details on the calling sequence
 *	to proc.  If there is already an event handler for chan, proc
 *	and clientData, then the mask will be updated.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateChannelHandler(chan, mask, proc, clientData)
    Tcl_Channel chan;		/* The channel to create the handler for. */
    int mask;			/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, and TCL_EXCEPTION:
				 * indicates conditions under which
				 * proc should be called. Use 0 to
                                 * disable a registered handler. */
    Tcl_ChannelProc *proc;	/* Procedure to call for each
				 * selected event. */
    ClientData clientData;	/* Arbitrary data to pass to proc. */
{
    ChannelHandler *chPtr;
    Channel *chanPtr;

    chanPtr = (Channel *) chan;
    
    /*
     * Check whether this channel handler is not already registered. If
     * it is not, create a new record, else reuse existing record (smash
     * current values).
     */

    for (chPtr = chanPtr->chPtr;
             chPtr != (ChannelHandler *) NULL;
             chPtr = chPtr->nextPtr) {
        if ((chPtr->chanPtr == chanPtr) && (chPtr->proc == proc) &&
                (chPtr->clientData == clientData)) {
            break;
        }
    }
    if (chPtr == (ChannelHandler *) NULL) {
        chPtr = (ChannelHandler *) ckalloc((unsigned) sizeof(ChannelHandler));
        chPtr->mask = 0;
        chPtr->proc = proc;
        chPtr->clientData = clientData;
        chPtr->chanPtr = chanPtr;
        chPtr->nextPtr = chanPtr->chPtr;
        chanPtr->chPtr = chPtr;
    }

    /*
     * The remainder of the initialization below is done regardless of
     * whether or not this is a new record or a modification of an old
     * one.
     */

    chPtr->mask = mask;

    /*
     * Recompute the interest mask for the channel - this call may actually
     * be disabling an existing handler.
     */
    
    chanPtr->interestMask = 0;
    for (chPtr = chanPtr->chPtr;
	 chPtr != (ChannelHandler *) NULL;
	 chPtr = chPtr->nextPtr) {
	chanPtr->interestMask |= chPtr->mask;
    }

    UpdateInterest(chanPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteChannelHandler --
 *
 *	Cancel a previously arranged callback arrangement for an IO
 *	channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If a callback was previously registered for this chan, proc and
 *	 clientData , it is removed and the callback will no longer be called
 *	when the channel becomes ready for IO.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteChannelHandler(chan, proc, clientData)
    Tcl_Channel chan;		/* The channel for which to remove the
                                 * callback. */
    Tcl_ChannelProc *proc;	/* The procedure in the callback to delete. */
    ClientData clientData;	/* The client data in the callback
                                 * to delete. */
    
{
    ChannelHandler *chPtr, *prevChPtr;
    Channel *chanPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    NextChannelHandler *nhPtr;

    chanPtr = (Channel *) chan;

    /*
     * Find the entry and the previous one in the list.
     */

    for (prevChPtr = (ChannelHandler *) NULL, chPtr = chanPtr->chPtr;
             chPtr != (ChannelHandler *) NULL;
             chPtr = chPtr->nextPtr) {
        if ((chPtr->chanPtr == chanPtr) && (chPtr->clientData == clientData)
                && (chPtr->proc == proc)) {
            break;
        }
        prevChPtr = chPtr;
    }
    
    /*
     * If not found, return without doing anything.
     */

    if (chPtr == (ChannelHandler *) NULL) {
        return;
    }

    /*
     * If ChannelHandlerEventProc is about to process this handler, tell it to
     * process the next one instead - we are going to delete *this* one.
     */

    for (nhPtr = tsdPtr->nestedHandlerPtr;
             nhPtr != (NextChannelHandler *) NULL;
             nhPtr = nhPtr->nestedHandlerPtr) {
        if (nhPtr->nextHandlerPtr == chPtr) {
            nhPtr->nextHandlerPtr = chPtr->nextPtr;
        }
    }

    /*
     * Splice it out of the list of channel handlers.
     */
    
    if (prevChPtr == (ChannelHandler *) NULL) {
        chanPtr->chPtr = chPtr->nextPtr;
    } else {
        prevChPtr->nextPtr = chPtr->nextPtr;
    }
    ckfree((char *) chPtr);

    /*
     * Recompute the interest list for the channel, so that infinite loops
     * will not result if Tcl_DeleteChannelHandler is called inside an
     * event.
     */

    chanPtr->interestMask = 0;
    for (chPtr = chanPtr->chPtr;
             chPtr != (ChannelHandler *) NULL;
             chPtr = chPtr->nextPtr) {
        chanPtr->interestMask |= chPtr->mask;
    }

    UpdateInterest(chanPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteScriptRecord --
 *
 *	Delete a script record for this combination of channel, interp
 *	and mask.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes a script record and cancels a channel event handler.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteScriptRecord(interp, chanPtr, mask)
    Tcl_Interp *interp;		/* Interpreter in which script was to be
                                 * executed. */
    Channel *chanPtr;		/* The channel for which to delete the
                                 * script record (if any). */
    int mask;			/* Events in mask must exactly match mask
                                 * of script to delete. */
{
    EventScriptRecord *esPtr, *prevEsPtr;

    for (esPtr = chanPtr->scriptRecordPtr,
             prevEsPtr = (EventScriptRecord *) NULL;
             esPtr != (EventScriptRecord *) NULL;
             prevEsPtr = esPtr, esPtr = esPtr->nextPtr) {
        if ((esPtr->interp == interp) && (esPtr->mask == mask)) {
            if (esPtr == chanPtr->scriptRecordPtr) {
                chanPtr->scriptRecordPtr = esPtr->nextPtr;
            } else {
                prevEsPtr->nextPtr = esPtr->nextPtr;
            }

            Tcl_DeleteChannelHandler((Tcl_Channel) chanPtr,
                    ChannelEventScriptInvoker, (ClientData) esPtr);
            
	    Tcl_DecrRefCount(esPtr->scriptPtr);
            ckfree((char *) esPtr);

            break;
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CreateScriptRecord --
 *
 *	Creates a record to store a script to be executed when a specific
 *	event fires on a specific channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Causes the script to be stored for later execution.
 *
 *----------------------------------------------------------------------
 */

static void
CreateScriptRecord(interp, chanPtr, mask, scriptPtr)
    Tcl_Interp *interp;			/* Interpreter in which to execute
                                         * the stored script. */
    Channel *chanPtr;			/* Channel for which script is to
                                         * be stored. */
    int mask;				/* Set of events for which script
                                         * will be invoked. */
    Tcl_Obj *scriptPtr;			/* Pointer to script object. */
{
    EventScriptRecord *esPtr;

    for (esPtr = chanPtr->scriptRecordPtr;
             esPtr != (EventScriptRecord *) NULL;
             esPtr = esPtr->nextPtr) {
        if ((esPtr->interp == interp) && (esPtr->mask == mask)) {
	    Tcl_DecrRefCount(esPtr->scriptPtr);
	    esPtr->scriptPtr = (Tcl_Obj *) NULL;
            break;
        }
    }
    if (esPtr == (EventScriptRecord *) NULL) {
        esPtr = (EventScriptRecord *) ckalloc((unsigned)
                sizeof(EventScriptRecord));
        Tcl_CreateChannelHandler((Tcl_Channel) chanPtr, mask,
                ChannelEventScriptInvoker, (ClientData) esPtr);
        esPtr->nextPtr = chanPtr->scriptRecordPtr;
        chanPtr->scriptRecordPtr = esPtr;
    }
    esPtr->chanPtr = chanPtr;
    esPtr->interp = interp;
    esPtr->mask = mask;
    Tcl_IncrRefCount(scriptPtr);
    esPtr->scriptPtr = scriptPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ChannelEventScriptInvoker --
 *
 *	Invokes a script scheduled by "fileevent" for when the channel
 *	becomes ready for IO. This function is invoked by the channel
 *	handler which was created by the Tcl "fileevent" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whatever the script does.
 *
 *----------------------------------------------------------------------
 */

static void
ChannelEventScriptInvoker(clientData, mask)
    ClientData clientData;	/* The script+interp record. */
    int mask;			/* Not used. */
{
    Tcl_Interp *interp;		/* Interpreter in which to eval the script. */
    Channel *chanPtr;		/* The channel for which this handler is
                                 * registered. */
    EventScriptRecord *esPtr;	/* The event script + interpreter to eval it
                                 * in. */
    int result;			/* Result of call to eval script. */

    esPtr = (EventScriptRecord *) clientData;

    chanPtr = esPtr->chanPtr;
    mask = esPtr->mask;
    interp = esPtr->interp;
    
    /*
     * We must preserve the interpreter so we can report errors on it
     * later.  Note that we do not need to preserve the channel because
     * that is done by Tcl_NotifyChannel before calling channel handlers.
     */
    
    Tcl_Preserve((ClientData) interp);
    result = Tcl_EvalObjEx(interp, esPtr->scriptPtr, TCL_EVAL_GLOBAL);

    /*
     * On error, cause a background error and remove the channel handler
     * and the script record.
     *
     * NOTE: Must delete channel handler before causing the background error
     * because the background error may want to reinstall the handler.
     */
    
    if (result != TCL_OK) {
	if (chanPtr->typePtr != NULL) {
	    DeleteScriptRecord(interp, chanPtr, mask);
	}
        Tcl_BackgroundError(interp);
    }
    Tcl_Release((ClientData) interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FileEventObjCmd --
 *
 *	This procedure implements the "fileevent" Tcl command. See the
 *	user documentation for details on what it does. This command is
 *	based on the Tk command "fileevent" which in turn is based on work
 *	contributed by Mark Diekhans.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May create a channel handler for the specified channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_FileEventObjCmd(clientData, interp, objc, objv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Interpreter in which the channel
                                         * for which to create the handler
                                         * is found. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    Channel *chanPtr;			/* The channel to create
                                         * the handler for. */
    Tcl_Channel chan;			/* The opaque type for the channel. */
    char *chanName;
    int modeIndex;			/* Index of mode argument. */
    int mask;
    static char *modeOptions[] = {"readable", "writable", NULL};
    static int maskArray[] = {TCL_READABLE, TCL_WRITABLE};

    if ((objc != 3) && (objc != 4)) {
	Tcl_WrongNumArgs(interp, 1, objv, "channelId event ?script?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[2], modeOptions, "event name", 0,
	    &modeIndex) != TCL_OK) {
	return TCL_ERROR;
    }
    mask = maskArray[modeIndex];

    chanName = Tcl_GetString(objv[1]);
    chan = Tcl_GetChannel(interp, chanName, NULL);
    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }
    chanPtr = (Channel *) chan;
    if ((chanPtr->flags & mask) == 0) {
        Tcl_AppendResult(interp, "channel is not ",
                (mask == TCL_READABLE) ? "readable" : "writable",
                (char *) NULL);
        return TCL_ERROR;
    }
    
    /*
     * If we are supposed to return the script, do so.
     */

    if (objc == 3) {
	EventScriptRecord *esPtr;
	for (esPtr = chanPtr->scriptRecordPtr;
             esPtr != (EventScriptRecord *) NULL;
             esPtr = esPtr->nextPtr) {
	    if ((esPtr->interp == interp) && (esPtr->mask == mask)) {
		Tcl_SetObjResult(interp, esPtr->scriptPtr);
		break;
	    }
	}
        return TCL_OK;
    }

    /*
     * If we are supposed to delete a stored script, do so.
     */

    if (*(Tcl_GetString(objv[3])) == '\0') {
        DeleteScriptRecord(interp, chanPtr, mask);
        return TCL_OK;
    }

    /*
     * Make the script record that will link between the event and the
     * script to invoke. This also creates a channel event handler which
     * will evaluate the script in the supplied interpreter.
     */

    CreateScriptRecord(interp, chanPtr, mask, objv[3]);
    
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclTestChannelCmd --
 *
 *	Implements the Tcl "testchannel" debugging command and its
 *	subcommands. This is part of the testing environment but must be
 *	in this file instead of tclTest.c because it needs access to the
 *	fields of struct Channel.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TclTestChannelCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Interpreter for result. */
    int argc;			/* Count of additional args. */
    char **argv;		/* Additional arg strings. */
{
    char *cmdName;		/* Sub command. */
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_HashSearch hSearch;	/* Search variable. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    Channel *chanPtr;		/* The actual channel. */
    Tcl_Channel chan;		/* The opaque type. */
    size_t len;			/* Length of subcommand string. */
    int IOQueued;		/* How much IO is queued inside channel? */
    ChannelBuffer *bufPtr;	/* For iterating over queued IO. */
    char buf[TCL_INTEGER_SPACE];/* For sprintf. */
    
    if (argc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " subcommand ?additional args..?\"", (char *) NULL);
        return TCL_ERROR;
    }
    cmdName = argv[1];
    len = strlen(cmdName);

    chanPtr = (Channel *) NULL;

    if (argc > 2) {
        chan = Tcl_GetChannel(interp, argv[2], NULL);
        if (chan == (Tcl_Channel) NULL) {
            return TCL_ERROR;
        }
        chanPtr = (Channel *) chan;
    }


    if ((cmdName[0] == 'i') && (strncmp(cmdName, "info", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " info channelName\"", (char *) NULL);
            return TCL_ERROR;
        }
        Tcl_AppendElement(interp, argv[2]);
        Tcl_AppendElement(interp, chanPtr->typePtr->typeName);
        if (chanPtr->flags & TCL_READABLE) {
            Tcl_AppendElement(interp, "read");
        } else {
            Tcl_AppendElement(interp, "");
        }
        if (chanPtr->flags & TCL_WRITABLE) {
            Tcl_AppendElement(interp, "write");
        } else {
            Tcl_AppendElement(interp, "");
        }
        if (chanPtr->flags & CHANNEL_NONBLOCKING) {
            Tcl_AppendElement(interp, "nonblocking");
        } else {
            Tcl_AppendElement(interp, "blocking");
        }
        if (chanPtr->flags & CHANNEL_LINEBUFFERED) {
            Tcl_AppendElement(interp, "line");
        } else if (chanPtr->flags & CHANNEL_UNBUFFERED) {
            Tcl_AppendElement(interp, "none");
        } else {
            Tcl_AppendElement(interp, "full");
        }
        if (chanPtr->flags & BG_FLUSH_SCHEDULED) {
            Tcl_AppendElement(interp, "async_flush");
        } else {
            Tcl_AppendElement(interp, "");
        }
        if (chanPtr->flags & CHANNEL_EOF) {
            Tcl_AppendElement(interp, "eof");
        } else {
            Tcl_AppendElement(interp, "");
        }
        if (chanPtr->flags & CHANNEL_BLOCKED) {
            Tcl_AppendElement(interp, "blocked");
        } else {
            Tcl_AppendElement(interp, "unblocked");
        }
        if (chanPtr->inputTranslation == TCL_TRANSLATE_AUTO) {
            Tcl_AppendElement(interp, "auto");
            if (chanPtr->flags & INPUT_SAW_CR) {
                Tcl_AppendElement(interp, "saw_cr");
            } else {
                Tcl_AppendElement(interp, "");
            }
        } else if (chanPtr->inputTranslation == TCL_TRANSLATE_LF) {
            Tcl_AppendElement(interp, "lf");
            Tcl_AppendElement(interp, "");
        } else if (chanPtr->inputTranslation == TCL_TRANSLATE_CR) {
            Tcl_AppendElement(interp, "cr");
            Tcl_AppendElement(interp, "");
        } else if (chanPtr->inputTranslation == TCL_TRANSLATE_CRLF) {
            Tcl_AppendElement(interp, "crlf");
            if (chanPtr->flags & INPUT_SAW_CR) {
                Tcl_AppendElement(interp, "queued_cr");
            } else {
                Tcl_AppendElement(interp, "");
            }
        }
        if (chanPtr->outputTranslation == TCL_TRANSLATE_AUTO) {
            Tcl_AppendElement(interp, "auto");
        } else if (chanPtr->outputTranslation == TCL_TRANSLATE_LF) {
            Tcl_AppendElement(interp, "lf");
        } else if (chanPtr->outputTranslation == TCL_TRANSLATE_CR) {
            Tcl_AppendElement(interp, "cr");
        } else if (chanPtr->outputTranslation == TCL_TRANSLATE_CRLF) {
            Tcl_AppendElement(interp, "crlf");
        }
        for (IOQueued = 0, bufPtr = chanPtr->inQueueHead;
                 bufPtr != (ChannelBuffer *) NULL;
                 bufPtr = bufPtr->nextPtr) {
            IOQueued += bufPtr->nextAdded - bufPtr->nextRemoved;
        }
        TclFormatInt(buf, IOQueued);
        Tcl_AppendElement(interp, buf);
        
        IOQueued = 0;
        if (chanPtr->curOutPtr != (ChannelBuffer *) NULL) {
            IOQueued = chanPtr->curOutPtr->nextAdded -
                chanPtr->curOutPtr->nextRemoved;
        }
        for (bufPtr = chanPtr->outQueueHead;
                 bufPtr != (ChannelBuffer *) NULL;
                 bufPtr = bufPtr->nextPtr) {
            IOQueued += (bufPtr->nextAdded - bufPtr->nextRemoved);
        }
        TclFormatInt(buf, IOQueued);
        Tcl_AppendElement(interp, buf);
        
        TclFormatInt(buf, Tcl_Tell((Tcl_Channel) chanPtr));
        Tcl_AppendElement(interp, buf);

        TclFormatInt(buf, chanPtr->refCount);
        Tcl_AppendElement(interp, buf);

        return TCL_OK;
    }

    if ((cmdName[0] == 'i') &&
            (strncmp(cmdName, "inputbuffered", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required",
                    (char *) NULL);
            return TCL_ERROR;
        }
        
        for (IOQueued = 0, bufPtr = chanPtr->inQueueHead;
                 bufPtr != (ChannelBuffer *) NULL;
                 bufPtr = bufPtr->nextPtr) {
            IOQueued += bufPtr->nextAdded - bufPtr->nextRemoved;
        }
        TclFormatInt(buf, IOQueued);
        Tcl_AppendResult(interp, buf, (char *) NULL);
        return TCL_OK;
    }
        
    if ((cmdName[0] == 'm') && (strncmp(cmdName, "mode", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required",
                    (char *) NULL);
            return TCL_ERROR;
        }
        
        if (chanPtr->flags & TCL_READABLE) {
            Tcl_AppendElement(interp, "read");
        } else {
            Tcl_AppendElement(interp, "");
        }
        if (chanPtr->flags & TCL_WRITABLE) {
            Tcl_AppendElement(interp, "write");
        } else {
            Tcl_AppendElement(interp, "");
        }
        return TCL_OK;
    }
    
    if ((cmdName[0] == 'n') && (strncmp(cmdName, "name", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required",
                    (char *) NULL);
            return TCL_ERROR;
        }
        Tcl_AppendResult(interp, chanPtr->channelName, (char *) NULL);
        return TCL_OK;
    }
    
    if ((cmdName[0] == 'o') && (strncmp(cmdName, "open", len) == 0)) {
        hTblPtr = (Tcl_HashTable *) Tcl_GetAssocData(interp, "tclIO", NULL);
        if (hTblPtr == (Tcl_HashTable *) NULL) {
            return TCL_OK;
        }
        for (hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch);
                 hPtr != (Tcl_HashEntry *) NULL;
                 hPtr = Tcl_NextHashEntry(&hSearch)) {
            Tcl_AppendElement(interp, Tcl_GetHashKey(hTblPtr, hPtr));
        }
        return TCL_OK;
    }

    if ((cmdName[0] == 'o') &&
            (strncmp(cmdName, "outputbuffered", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required",
                    (char *) NULL);
            return TCL_ERROR;
        }
        
        IOQueued = 0;
        if (chanPtr->curOutPtr != (ChannelBuffer *) NULL) {
            IOQueued = chanPtr->curOutPtr->nextAdded -
                chanPtr->curOutPtr->nextRemoved;
        }
        for (bufPtr = chanPtr->outQueueHead;
                 bufPtr != (ChannelBuffer *) NULL;
                 bufPtr = bufPtr->nextPtr) {
            IOQueued += (bufPtr->nextAdded - bufPtr->nextRemoved);
        }
        TclFormatInt(buf, IOQueued);
        Tcl_AppendResult(interp, buf, (char *) NULL);
        return TCL_OK;
    }
        
    if ((cmdName[0] == 'q') &&
            (strncmp(cmdName, "queuedcr", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required",
                    (char *) NULL);
            return TCL_ERROR;
        }
        
        Tcl_AppendResult(interp,
                (chanPtr->flags & INPUT_SAW_CR) ? "1" : "0",
                (char *) NULL);
        return TCL_OK;
    }
    
    if ((cmdName[0] == 'r') && (strncmp(cmdName, "readable", len) == 0)) {
        hTblPtr = (Tcl_HashTable *) Tcl_GetAssocData(interp, "tclIO", NULL);
        if (hTblPtr == (Tcl_HashTable *) NULL) {
            return TCL_OK;
        }
        for (hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch);
                 hPtr != (Tcl_HashEntry *) NULL;
                 hPtr = Tcl_NextHashEntry(&hSearch)) {
            chanPtr = (Channel *) Tcl_GetHashValue(hPtr);
            if (chanPtr->flags & TCL_READABLE) {
                Tcl_AppendElement(interp, Tcl_GetHashKey(hTblPtr, hPtr));
            }
        }
        return TCL_OK;
    }

    if ((cmdName[0] == 'r') && (strncmp(cmdName, "refcount", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required",
                    (char *) NULL);
            return TCL_ERROR;
        }
        
        TclFormatInt(buf, chanPtr->refCount);
        Tcl_AppendResult(interp, buf, (char *) NULL);
        return TCL_OK;
    }
    
    if ((cmdName[0] == 't') && (strncmp(cmdName, "type", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required",
                    (char *) NULL);
            return TCL_ERROR;
        }
        Tcl_AppendResult(interp, chanPtr->typePtr->typeName, (char *) NULL);
        return TCL_OK;
    }
    
    if ((cmdName[0] == 'w') && (strncmp(cmdName, "writable", len) == 0)) {
        hTblPtr = (Tcl_HashTable *) Tcl_GetAssocData(interp, "tclIO", NULL);
        if (hTblPtr == (Tcl_HashTable *) NULL) {
            return TCL_OK;
        }
        for (hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch);
                 hPtr != (Tcl_HashEntry *) NULL;
                 hPtr = Tcl_NextHashEntry(&hSearch)) {
            chanPtr = (Channel *) Tcl_GetHashValue(hPtr);
            if (chanPtr->flags & TCL_WRITABLE) {
                Tcl_AppendElement(interp, Tcl_GetHashKey(hTblPtr, hPtr));
            }
        }
        return TCL_OK;
    }

    Tcl_AppendResult(interp, "bad option \"", cmdName, "\": should be ",
            "info, open, readable, or writable",
            (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclTestChannelEventCmd --
 *
 *	This procedure implements the "testchannelevent" command. It is
 *	used to test the Tcl channel event mechanism. It is present in
 *	this file instead of tclTest.c because it needs access to the
 *	internal structure of the channel.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates, deletes and returns channel event handlers.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TclTestChannelEventCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    Tcl_Obj *resultListPtr;
    Channel *chanPtr;
    EventScriptRecord *esPtr, *prevEsPtr, *nextEsPtr;
    char *cmd;
    int index, i, mask, len;

    if ((argc < 3) || (argc > 5)) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " channelName cmd ?arg1? ?arg2?\"", (char *) NULL);
        return TCL_ERROR;
    }
    chanPtr = (Channel *) Tcl_GetChannel(interp, argv[1], NULL);
    if (chanPtr == (Channel *) NULL) {
        return TCL_ERROR;
    }
    cmd = argv[2];
    len = strlen(cmd);
    if ((cmd[0] == 'a') && (strncmp(cmd, "add", (unsigned) len) == 0)) {
        if (argc != 5) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " channelName add eventSpec script\"", (char *) NULL);
            return TCL_ERROR;
        }
        if (strcmp(argv[3], "readable") == 0) {
            mask = TCL_READABLE;
        } else if (strcmp(argv[3], "writable") == 0) {
            mask = TCL_WRITABLE;
        } else if (strcmp(argv[3], "none") == 0) {
            mask = 0;
	} else {
            Tcl_AppendResult(interp, "bad event name \"", argv[3],
                    "\": must be readable, writable, or none", (char *) NULL);
            return TCL_ERROR;
        }

        esPtr = (EventScriptRecord *) ckalloc((unsigned)
                sizeof(EventScriptRecord));
        esPtr->nextPtr = chanPtr->scriptRecordPtr;
        chanPtr->scriptRecordPtr = esPtr;
        
        esPtr->chanPtr = chanPtr;
        esPtr->interp = interp;
        esPtr->mask = mask;
	esPtr->scriptPtr = Tcl_NewStringObj(argv[4], -1);
	Tcl_IncrRefCount(esPtr->scriptPtr);

        Tcl_CreateChannelHandler((Tcl_Channel) chanPtr, mask,
                ChannelEventScriptInvoker, (ClientData) esPtr);
        
        return TCL_OK;
    }

    if ((cmd[0] == 'd') && (strncmp(cmd, "delete", (unsigned) len) == 0)) {
        if (argc != 4) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " channelName delete index\"", (char *) NULL);
            return TCL_ERROR;
        }
        if (Tcl_GetInt(interp, argv[3], &index) == TCL_ERROR) {
            return TCL_ERROR;
        }
        if (index < 0) {
            Tcl_AppendResult(interp, "bad event index: ", argv[3],
                    ": must be nonnegative", (char *) NULL);
            return TCL_ERROR;
        }
        for (i = 0, esPtr = chanPtr->scriptRecordPtr;
                 (i < index) && (esPtr != (EventScriptRecord *) NULL);
                 i++, esPtr = esPtr->nextPtr) {
	    /* Empty loop body. */
        }
        if (esPtr == (EventScriptRecord *) NULL) {
            Tcl_AppendResult(interp, "bad event index ", argv[3],
                    ": out of range", (char *) NULL);
            return TCL_ERROR;
        }
        if (esPtr == chanPtr->scriptRecordPtr) {
            chanPtr->scriptRecordPtr = esPtr->nextPtr;
        } else {
            for (prevEsPtr = chanPtr->scriptRecordPtr;
                     (prevEsPtr != (EventScriptRecord *) NULL) &&
                         (prevEsPtr->nextPtr != esPtr);
                     prevEsPtr = prevEsPtr->nextPtr) {
                /* Empty loop body. */
            }
            if (prevEsPtr == (EventScriptRecord *) NULL) {
                panic("TclTestChannelEventCmd: damaged event script list");
            }
            prevEsPtr->nextPtr = esPtr->nextPtr;
        }
        Tcl_DeleteChannelHandler((Tcl_Channel) chanPtr,
                ChannelEventScriptInvoker, (ClientData) esPtr);
	Tcl_DecrRefCount(esPtr->scriptPtr);
        ckfree((char *) esPtr);

        return TCL_OK;
    }

    if ((cmd[0] == 'l') && (strncmp(cmd, "list", (unsigned) len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " channelName list\"", (char *) NULL);
            return TCL_ERROR;
        }
	resultListPtr = Tcl_GetObjResult(interp);
        for (esPtr = chanPtr->scriptRecordPtr;
                 esPtr != (EventScriptRecord *) NULL;
                 esPtr = esPtr->nextPtr) {
	    if (esPtr->mask) {
 	        Tcl_ListObjAppendElement(interp, resultListPtr, Tcl_NewStringObj(
		    (esPtr->mask == TCL_READABLE) ? "readable" : "writable", -1));
 	    } else {
 	        Tcl_ListObjAppendElement(interp, resultListPtr, 
                    Tcl_NewStringObj("none", -1));
	    }
  	    Tcl_ListObjAppendElement(interp, resultListPtr, esPtr->scriptPtr);
        }
	Tcl_SetObjResult(interp, resultListPtr);
        return TCL_OK;
    }

    if ((cmd[0] == 'r') && (strncmp(cmd, "removeall", (unsigned) len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " channelName removeall\"", (char *) NULL);
            return TCL_ERROR;
        }
        for (esPtr = chanPtr->scriptRecordPtr;
                 esPtr != (EventScriptRecord *) NULL;
                 esPtr = nextEsPtr) {
            nextEsPtr = esPtr->nextPtr;
            Tcl_DeleteChannelHandler((Tcl_Channel) chanPtr,
                    ChannelEventScriptInvoker, (ClientData) esPtr);
	    Tcl_DecrRefCount(esPtr->scriptPtr);
            ckfree((char *) esPtr);
        }
        chanPtr->scriptRecordPtr = (EventScriptRecord *) NULL;
        return TCL_OK;
    }

    if  ((cmd[0] == 's') && (strncmp(cmd, "set", (unsigned) len) == 0)) {
        if (argc != 5) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " channelName delete index event\"", (char *) NULL);
            return TCL_ERROR;
        }
        if (Tcl_GetInt(interp, argv[3], &index) == TCL_ERROR) {
            return TCL_ERROR;
        }
        if (index < 0) {
            Tcl_AppendResult(interp, "bad event index: ", argv[3],
                    ": must be nonnegative", (char *) NULL);
            return TCL_ERROR;
        }
        for (i = 0, esPtr = chanPtr->scriptRecordPtr;
                 (i < index) && (esPtr != (EventScriptRecord *) NULL);
                 i++, esPtr = esPtr->nextPtr) {
	    /* Empty loop body. */
        }
        if (esPtr == (EventScriptRecord *) NULL) {
            Tcl_AppendResult(interp, "bad event index ", argv[3],
                    ": out of range", (char *) NULL);
            return TCL_ERROR;
        }

        if (strcmp(argv[4], "readable") == 0) {
            mask = TCL_READABLE;
        } else if (strcmp(argv[4], "writable") == 0) {
            mask = TCL_WRITABLE;
        } else if (strcmp(argv[4], "none") == 0) {
            mask = 0;
	} else {
            Tcl_AppendResult(interp, "bad event name \"", argv[4],
                    "\": must be readable, writable, or none", (char *) NULL);
            return TCL_ERROR;
        }
	esPtr->mask = mask;
        Tcl_CreateChannelHandler((Tcl_Channel) chanPtr, mask,
                ChannelEventScriptInvoker, (ClientData) esPtr);
	return TCL_OK;
    }    
    Tcl_AppendResult(interp, "bad command ", cmd, ", must be one of ",
            "add, delete, list, set, or removeall", (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCopyChannel --
 *
 *	This routine copies data from one channel to another, either
 *	synchronously or asynchronously.  If a command script is
 *	supplied, the operation runs in the background.  The script
 *	is invoked when the copy completes.  Otherwise the function
 *	waits until the copy is completed before returning.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May schedule a background copy operation that causes both
 *	channels to be marked busy.
 *
 *----------------------------------------------------------------------
 */

int
TclCopyChannel(interp, inChan, outChan, toRead, cmdPtr)
    Tcl_Interp *interp;		/* Current interpreter. */
    Tcl_Channel inChan;		/* Channel to read from. */
    Tcl_Channel outChan;	/* Channel to write to. */
    int toRead;			/* Amount of data to copy, or -1 for all. */
    Tcl_Obj *cmdPtr;		/* Pointer to script to execute or NULL. */
{
    Channel *inPtr = (Channel *) inChan;
    Channel *outPtr = (Channel *) outChan;
    int readFlags, writeFlags;
    CopyState *csPtr;
    int nonBlocking = (cmdPtr) ? CHANNEL_NONBLOCKING : 0;

    if (inPtr->csPtr) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), "channel \"",
		Tcl_GetChannelName(inChan), "\" is busy", NULL);
	return TCL_ERROR;
    }
    if (outPtr->csPtr) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), "channel \"",
		Tcl_GetChannelName(outChan), "\" is busy", NULL);
	return TCL_ERROR;
    }

    readFlags = inPtr->flags;
    writeFlags = outPtr->flags;

    /*
     * Set up the blocking mode appropriately.  Background copies need
     * non-blocking channels.  Foreground copies need blocking channels.
     * If there is an error, restore the old blocking mode.
     */

    if (nonBlocking != (readFlags & CHANNEL_NONBLOCKING)) {
	if (SetBlockMode(interp, inPtr,
		nonBlocking ? TCL_MODE_NONBLOCKING : TCL_MODE_BLOCKING)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
    }	    
    if (inPtr != outPtr) {
	if (nonBlocking != (writeFlags & CHANNEL_NONBLOCKING)) {
	    if (SetBlockMode(NULL, outPtr,
		    nonBlocking ? TCL_MODE_BLOCKING : TCL_MODE_NONBLOCKING)
		    != TCL_OK) {
		if (nonBlocking != (readFlags & CHANNEL_NONBLOCKING)) {
		    SetBlockMode(NULL, inPtr,
			    (readFlags & CHANNEL_NONBLOCKING)
			    ? TCL_MODE_NONBLOCKING : TCL_MODE_BLOCKING);
		    return TCL_ERROR;
		}
	    }
	}
    }

    /*
     * Make sure the output side is unbuffered.
     */

    outPtr->flags = (outPtr->flags & ~(CHANNEL_LINEBUFFERED))
	| CHANNEL_UNBUFFERED;

    /*
     * Allocate a new CopyState to maintain info about the current copy in
     * progress.  This structure will be deallocated when the copy is
     * completed.
     */

    csPtr = (CopyState*) ckalloc(sizeof(CopyState) + inPtr->bufSize);
    csPtr->bufSize = inPtr->bufSize;
    csPtr->readPtr = inPtr;
    csPtr->writePtr = outPtr;
    csPtr->readFlags = readFlags;
    csPtr->writeFlags = writeFlags;
    csPtr->toRead = toRead;
    csPtr->total = 0;
    csPtr->interp = interp;
    if (cmdPtr) {
	Tcl_IncrRefCount(cmdPtr);
    }
    csPtr->cmdPtr = cmdPtr;
    inPtr->csPtr = csPtr;
    outPtr->csPtr = csPtr;

    /*
     * Start copying data between the channels.
     */

    return CopyData(csPtr, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * CopyData --
 *
 *	This function implements the lowest level of the copying
 *	mechanism for TclCopyChannel.
 *
 * Results:
 *	Returns TCL_OK on success, else TCL_ERROR.
 *
 * Side effects:
 *	Moves data between channels, may create channel handlers.
 *
 *----------------------------------------------------------------------
 */

static int
CopyData(csPtr, mask)
    CopyState *csPtr;		/* State of copy operation. */
    int mask;			/* Current channel event flags. */
{
    Tcl_Interp *interp;
    Tcl_Obj *cmdPtr, *errObj = NULL;
    Tcl_Channel inChan, outChan;
    int result = TCL_OK;
    int size;
    int total;

    inChan = (Tcl_Channel)csPtr->readPtr;
    outChan = (Tcl_Channel)csPtr->writePtr;
    interp = csPtr->interp;
    cmdPtr = csPtr->cmdPtr;

    /*
     * Copy the data the slow way, using the translation mechanism.
     */

    while (csPtr->toRead != 0) {

	/*
	 * Check for unreported background errors.
	 */

	if (csPtr->readPtr->unreportedError != 0) {
	    Tcl_SetErrno(csPtr->readPtr->unreportedError);
	    csPtr->readPtr->unreportedError = 0;
	    goto readError;
	}
	if (csPtr->writePtr->unreportedError != 0) {
	    Tcl_SetErrno(csPtr->writePtr->unreportedError);
	    csPtr->writePtr->unreportedError = 0;
	    goto writeError;
	}
	
	/*
	 * Read up to bufSize bytes.
	 */

	if ((csPtr->toRead == -1)
		|| (csPtr->toRead > csPtr->bufSize)) {
	    size = csPtr->bufSize;
	} else {
	    size = csPtr->toRead;
	}
	size = DoRead(csPtr->readPtr, csPtr->buffer, size);

	if (size < 0) {
	    readError:
	    errObj = Tcl_NewObj();
	    Tcl_AppendStringsToObj(errObj, "error reading \"",
		    Tcl_GetChannelName(inChan), "\": ",
		    Tcl_PosixError(interp), (char *) NULL);
	    break;
	} else if (size == 0) {
	    /*
	     * We had an underflow on the read side.  If we are at EOF,
	     * then the copying is done, otherwise set up a channel
	     * handler to detect when the channel becomes readable again.
	     */
	    
	    if (Tcl_Eof(inChan)) {
		break;
	    } else if (!(mask & TCL_READABLE)) {
		if (mask & TCL_WRITABLE) {
		    Tcl_DeleteChannelHandler(outChan, CopyEventProc,
			    (ClientData) csPtr);
		}
		Tcl_CreateChannelHandler(inChan, TCL_READABLE,
			CopyEventProc, (ClientData) csPtr);
	    }
	    return TCL_OK;
	}

	/*
	 * Now write the buffer out.
	 */

	size = DoWrite(csPtr->writePtr, csPtr->buffer, size);
	if (size < 0) {
	    writeError:
	    errObj = Tcl_NewObj();
	    Tcl_AppendStringsToObj(errObj, "error writing \"",
		    Tcl_GetChannelName(outChan), "\": ",
		    Tcl_PosixError(interp), (char *) NULL);
	    break;
	}

	/*
	 * Check to see if the write is happening in the background.  If so,
	 * stop copying and wait for the channel to become writable again.
	 */

	if (csPtr->writePtr->flags & BG_FLUSH_SCHEDULED) {
	    if (!(mask & TCL_WRITABLE)) {
		if (mask & TCL_READABLE) {
		    Tcl_DeleteChannelHandler(outChan, CopyEventProc,
			    (ClientData) csPtr);
		}
		Tcl_CreateChannelHandler(outChan, TCL_WRITABLE,
			CopyEventProc, (ClientData) csPtr);
	    }
	    return TCL_OK;
	}

	/*
	 * Update the current byte count if we care.
	 */

	if (csPtr->toRead != -1) {
	    csPtr->toRead -= size;
	}
	csPtr->total += size;

	/*
	 * For background copies, we only do one buffer per invocation so
	 * we don't starve the rest of the system.
	 */

	if (cmdPtr) {
	    /*
	     * The first time we enter this code, there won't be a
	     * channel handler established yet, so do it here.
	     */

	    if (mask == 0) {
		Tcl_CreateChannelHandler(outChan, TCL_WRITABLE,
			CopyEventProc, (ClientData) csPtr);
	    }
	    return TCL_OK;
	}
    }

    /*
     * Make the callback or return the number of bytes transferred.
     * The local total is used because StopCopy frees csPtr.
     */

    total = csPtr->total;
    if (cmdPtr) {
	/*
	 * Get a private copy of the command so we can mutate it
	 * by adding arguments.  Note that StopCopy frees our saved
	 * reference to the original command obj.
	 */

	cmdPtr = Tcl_DuplicateObj(cmdPtr);
	Tcl_IncrRefCount(cmdPtr);
	StopCopy(csPtr);
	Tcl_Preserve((ClientData) interp);

	Tcl_ListObjAppendElement(interp, cmdPtr, Tcl_NewIntObj(total));
	if (errObj) {
	    Tcl_ListObjAppendElement(interp, cmdPtr, errObj);
	}
	if (Tcl_EvalObjEx(interp, cmdPtr, TCL_EVAL_GLOBAL) != TCL_OK) {
	    Tcl_BackgroundError(interp);
	    result = TCL_ERROR;
	}
	Tcl_DecrRefCount(cmdPtr);
	Tcl_Release((ClientData) interp);
    } else {
	StopCopy(csPtr);
	if (errObj) {
	    Tcl_SetObjResult(interp, errObj);
	    result = TCL_ERROR;
	} else {
	    Tcl_ResetResult(interp);
	    Tcl_SetIntObj(Tcl_GetObjResult(interp), total);
	}
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DoRead --
 *
 *	Reads a given number of bytes from a channel.
 *
 * Results:
 *	The number of characters read, or -1 on error. Use Tcl_GetErrno()
 *	to retrieve the error code for the error that occurred.
 *
 * Side effects:
 *	May cause input to be buffered.
 *
 *----------------------------------------------------------------------
 */

static int
DoRead(chanPtr, bufPtr, toRead)
    Channel *chanPtr;		/* The channel from which to read. */
    char *bufPtr;		/* Where to store input read. */
    int toRead;			/* Maximum number of bytes to read. */
{
    int copied;			/* How many characters were copied into
                                 * the result string? */
    int copiedNow;		/* How many characters were copied from
                                 * the current input buffer? */
    int result;			/* Of calling GetInput. */
    
    /*
     * If we have not encountered a sticky EOF, clear the EOF bit. Either
     * way clear the BLOCKED bit. We want to discover these anew during
     * each operation.
     */

    if (!(chanPtr->flags & CHANNEL_STICKY_EOF)) {
        chanPtr->flags &= ~CHANNEL_EOF;
    }
    chanPtr->flags &= ~(CHANNEL_BLOCKED | CHANNEL_NEED_MORE_DATA);
    
    for (copied = 0; copied < toRead; copied += copiedNow) {
        copiedNow = CopyAndTranslateBuffer(chanPtr, bufPtr + copied,
                toRead - copied);
        if (copiedNow == 0) {
            if (chanPtr->flags & CHANNEL_EOF) {
		goto done;
            }
            if (chanPtr->flags & CHANNEL_BLOCKED) {
                if (chanPtr->flags & CHANNEL_NONBLOCKING) {
		    goto done;
                }
                chanPtr->flags &= (~(CHANNEL_BLOCKED));
            }
            result = GetInput(chanPtr);
            if (result != 0) {
                if (result != EAGAIN) {
                    copied = -1;
                }
		goto done;
            }
        }
    }

    chanPtr->flags &= (~(CHANNEL_BLOCKED));

    done:
    /*
     * Update the notifier state so we don't block while there is still
     * data in the buffers.
     */

    UpdateInterest(chanPtr);
    return copied;
}

/*
 *----------------------------------------------------------------------
 *
 * CopyAndTranslateBuffer --
 *
 *	Copy at most one buffer of input to the result space, doing
 *	eol translations according to mode in effect currently.
 *
 * Results:
 *	Number of bytes stored in the result buffer (as opposed to the
 *	number of bytes read from the channel).  May return
 *	zero if no input is available to be translated.
 *
 * Side effects:
 *	Consumes buffered input. May deallocate one buffer.
 *
 *----------------------------------------------------------------------
 */

static int
CopyAndTranslateBuffer(chanPtr, result, space)
    Channel *chanPtr;		/* The channel from which to read input. */
    char *result;		/* Where to store the copied input. */
    int space;			/* How many bytes are available in result
                                 * to store the copied input? */
{
    int bytesInBuffer;		/* How many bytes are available to be
                                 * copied in the current input buffer? */
    int copied;			/* How many characters were already copied
                                 * into the destination space? */
    ChannelBuffer *bufPtr;	/* The buffer from which to copy bytes. */
    int i;			/* Iterates over the copied input looking
                                 * for the input eofChar. */
    
    /*
     * If there is no input at all, return zero. The invariant is that either
     * there is no buffer in the queue, or if the first buffer is empty, it
     * is also the last buffer (and thus there is no input in the queue).
     * Note also that if the buffer is empty, we leave it in the queue.
     */
    
    if (chanPtr->inQueueHead == (ChannelBuffer *) NULL) {
        return 0;
    }
    bufPtr = chanPtr->inQueueHead;
    bytesInBuffer = bufPtr->nextAdded - bufPtr->nextRemoved;

    copied = 0;
    switch (chanPtr->inputTranslation) {
        case TCL_TRANSLATE_LF: {
            if (bytesInBuffer == 0) {
                return 0;
            }

	    /*
             * Copy the current chunk into the result buffer.
             */

	    if (bytesInBuffer < space) {
		space = bytesInBuffer;
	    }
	    memcpy((VOID *) result,
		    (VOID *) (bufPtr->buf + bufPtr->nextRemoved),
		    (size_t) space);
	    bufPtr->nextRemoved += space;
	    copied = space;
            break;
	}
        case TCL_TRANSLATE_CR: {
	    char *end;
	    
            if (bytesInBuffer == 0) {
                return 0;
            }

	    /*
             * Copy the current chunk into the result buffer, then
             * replace all \r with \n.
             */

	    if (bytesInBuffer < space) {
		space = bytesInBuffer;
	    }
	    memcpy((VOID *) result,
		    (VOID *) (bufPtr->buf + bufPtr->nextRemoved),
		    (size_t) space);
	    bufPtr->nextRemoved += space;
	    copied = space;

	    for (end = result + copied; result < end; result++) {
		if (*result == '\r') {
		    *result = '\n';
		}
            }
            break;
	}
        case TCL_TRANSLATE_CRLF: {
	    char *src, *end, *dst;
	    int curByte;
	    
            /*
             * If there is a held-back "\r" at EOF, produce it now.
             */
            
	    if (bytesInBuffer == 0) {
                if ((chanPtr->flags & (INPUT_SAW_CR | CHANNEL_EOF)) ==
                        (INPUT_SAW_CR | CHANNEL_EOF)) {
                    result[0] = '\r';
                    chanPtr->flags &= ~INPUT_SAW_CR;
                    return 1;
                }
                return 0;
            }

            /*
             * Copy the current chunk and replace "\r\n" with "\n"
             * (but not standalone "\r"!).
             */

	    if (bytesInBuffer < space) {
		space = bytesInBuffer;
	    }
	    memcpy((VOID *) result,
		    (VOID *) (bufPtr->buf + bufPtr->nextRemoved),
		    (size_t) space);
	    bufPtr->nextRemoved += space;
	    copied = space;

	    end = result + copied;
	    dst = result;
	    for (src = result; src < end; src++) {
		curByte = *src;
		if (curByte == '\n') {
                    chanPtr->flags &= ~INPUT_SAW_CR;
		} else if (chanPtr->flags & INPUT_SAW_CR) {
		    chanPtr->flags &= ~INPUT_SAW_CR;
		    *dst = '\r';
		    dst++;
		}
		if (curByte == '\r') {
		    chanPtr->flags |= INPUT_SAW_CR;
		} else {
		    *dst = (char) curByte;
		    dst++;
		}
	    }
	    copied = dst - result;
	    break;
	}
        case TCL_TRANSLATE_AUTO: {
	    char *src, *end, *dst;
	    int curByte;
	
            if (bytesInBuffer == 0) {
                return 0;
            }

            /*
             * Loop over the current buffer, converting "\r" and "\r\n"
             * to "\n".
             */

	    if (bytesInBuffer < space) {
		space = bytesInBuffer;
	    }
	    memcpy((VOID *) result,
		    (VOID *) (bufPtr->buf + bufPtr->nextRemoved),
		    (size_t) space);
	    bufPtr->nextRemoved += space;
	    copied = space;

	    end = result + copied;
	    dst = result;
	    for (src = result; src < end; src++) {
		curByte = *src;
		if (curByte == '\r') {
		    chanPtr->flags |= INPUT_SAW_CR;
		    *dst = '\n';
		    dst++;
		} else {
		    if ((curByte != '\n') || 
			    !(chanPtr->flags & INPUT_SAW_CR)) {
			*dst = (char) curByte;
			dst++;
		    }
		    chanPtr->flags &= ~INPUT_SAW_CR;
		}
	    }
	    copied = dst - result;
            break;
	}
        default: {
            panic("unknown eol translation mode");
	}
    }

    /*
     * If an in-stream EOF character is set for this channel, check that
     * the input we copied so far does not contain the EOF char.  If it does,
     * copy only up to and excluding that character.
     */
    
    if (chanPtr->inEofChar != 0) {
        for (i = 0; i < copied; i++) {
            if (result[i] == (char) chanPtr->inEofChar) {
		/*
		 * Set sticky EOF so that no further input is presented
		 * to the caller.
		 */
		
		chanPtr->flags |= (CHANNEL_EOF | CHANNEL_STICKY_EOF);
		chanPtr->inputEncodingFlags |= TCL_ENCODING_END;
		copied = i;
                break;
            }
        }
    }

    /*
     * If the current buffer is empty recycle it.
     */

    if (bufPtr->nextRemoved == bufPtr->nextAdded) {
        chanPtr->inQueueHead = bufPtr->nextPtr;
        if (chanPtr->inQueueHead == (ChannelBuffer *) NULL) {
            chanPtr->inQueueTail = (ChannelBuffer *) NULL;
        }
        RecycleBuffer(chanPtr, bufPtr, 0);
    }

    /*
     * Return the number of characters copied into the result buffer.
     * This may be different from the number of bytes consumed, because
     * of EOL translations.
     */

    return copied;
}

/*
 *----------------------------------------------------------------------
 *
 * DoWrite --
 *
 *	Puts a sequence of characters into an output buffer, may queue the
 *	buffer for output if it gets full, and also remembers whether the
 *	current buffer is ready e.g. if it contains a newline and we are in
 *	line buffering mode.
 *
 * Results:
 *	The number of bytes written or -1 in case of error. If -1,
 *	Tcl_GetErrno will return the error code.
 *
 * Side effects:
 *	May buffer up output and may cause output to be produced on the
 *	channel.
 *
 *----------------------------------------------------------------------
 */

static int
DoWrite(chanPtr, src, srcLen)
    Channel *chanPtr;			/* The channel to buffer output for. */
    char *src;				/* Data to write. */
    int srcLen;				/* Number of bytes to write. */
{
    ChannelBuffer *outBufPtr;		/* Current output buffer. */
    int foundNewline;			/* Did we find a newline in output? */
    char *dPtr;
    char *sPtr;				/* Search variables for newline. */
    int crsent;				/* In CRLF eol translation mode,
                                         * remember the fact that a CR was
                                         * output to the channel without
                                         * its following NL. */
    int i;				/* Loop index for newline search. */
    int destCopied;			/* How many bytes were used in this
                                         * destination buffer to hold the
                                         * output? */
    int totalDestCopied;		/* How many bytes total were
                                         * copied to the channel buffer? */
    int srcCopied;			/* How many bytes were copied from
                                         * the source string? */
    char *destPtr;			/* Where in line to copy to? */

    /*
     * If we are in network (or windows) translation mode, record the fact
     * that we have not yet sent a CR to the channel.
     */

    crsent = 0;
    
    /*
     * Loop filling buffers and flushing them until all output has been
     * consumed.
     */

    srcCopied = 0;
    totalDestCopied = 0;

    while (srcLen > 0) {
        
        /*
         * Make sure there is a current output buffer to accept output.
         */

        if (chanPtr->curOutPtr == (ChannelBuffer *) NULL) {
            chanPtr->curOutPtr = AllocChannelBuffer(chanPtr->bufSize);
        }

        outBufPtr = chanPtr->curOutPtr;

        destCopied = outBufPtr->bufLength - outBufPtr->nextAdded;
        if (destCopied > srcLen) {
            destCopied = srcLen;
        }
        
        destPtr = outBufPtr->buf + outBufPtr->nextAdded;
        switch (chanPtr->outputTranslation) {
            case TCL_TRANSLATE_LF:
                srcCopied = destCopied;
                memcpy((VOID *) destPtr, (VOID *) src, (size_t) destCopied);
                break;
            case TCL_TRANSLATE_CR:
                srcCopied = destCopied;
                memcpy((VOID *) destPtr, (VOID *) src, (size_t) destCopied);
                for (dPtr = destPtr; dPtr < destPtr + destCopied; dPtr++) {
                    if (*dPtr == '\n') {
                        *dPtr = '\r';
                    }
                }
                break;
            case TCL_TRANSLATE_CRLF:
                for (srcCopied = 0, dPtr = destPtr, sPtr = src;
                     dPtr < destPtr + destCopied;
                     dPtr++, sPtr++, srcCopied++) {
                    if (*sPtr == '\n') {
                        if (crsent) {
                            *dPtr = '\n';
                            crsent = 0;
                        } else {
                            *dPtr = '\r';
                            crsent = 1;
                            sPtr--, srcCopied--;
                        }
                    } else {
                        *dPtr = *sPtr;
                    }
                }
                break;
            case TCL_TRANSLATE_AUTO:
                panic("Tcl_Write: AUTO output translation mode not supported");
            default:
                panic("Tcl_Write: unknown output translation mode");
        }

        /*
         * The current buffer is ready for output if it is full, or if it
         * contains a newline and this channel is line-buffered, or if it
         * contains any output and this channel is unbuffered.
         */

        outBufPtr->nextAdded += destCopied;
        if (!(chanPtr->flags & BUFFER_READY)) {
            if (outBufPtr->nextAdded == outBufPtr->bufLength) {
                chanPtr->flags |= BUFFER_READY;
            } else if (chanPtr->flags & CHANNEL_LINEBUFFERED) {
                for (sPtr = src, i = 0, foundNewline = 0;
                         (i < srcCopied) && (!foundNewline);
                         i++, sPtr++) {
                    if (*sPtr == '\n') {
                        foundNewline = 1;
                        break;
                    }
                }
                if (foundNewline) {
                    chanPtr->flags |= BUFFER_READY;
                }
            } else if (chanPtr->flags & CHANNEL_UNBUFFERED) {
                chanPtr->flags |= BUFFER_READY;
            }
        }
        
        totalDestCopied += srcCopied;
        src += srcCopied;
        srcLen -= srcCopied;

        if (chanPtr->flags & BUFFER_READY) {
            if (FlushChannel(NULL, chanPtr, 0) != 0) {
                return -1;
            }
        }
    } /* Closes "while" */

    return totalDestCopied;
}

/*
 *----------------------------------------------------------------------
 *
 * CopyEventProc --
 *
 *	This routine is invoked as a channel event handler for
 *	the background copy operation.  It is just a trivial wrapper
 *	around the CopyData routine.
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
CopyEventProc(clientData, mask)
    ClientData clientData;
    int mask;
{
    (void) CopyData((CopyState *)clientData, mask);
}

/*
 *----------------------------------------------------------------------
 *
 * StopCopy --
 *
 *	This routine halts a copy that is in progress.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes any pending channel handlers and restores the blocking
 *	and buffering modes of the channels.  The CopyState is freed.
 *
 *----------------------------------------------------------------------
 */

static void
StopCopy(csPtr)
    CopyState *csPtr;		/* State for bg copy to stop . */
{
    int nonBlocking;

    if (!csPtr) {
	return;
    }

    /*
     * Restore the old blocking mode and output buffering mode.
     */

    nonBlocking = (csPtr->readFlags & CHANNEL_NONBLOCKING);
    if (nonBlocking != (csPtr->readPtr->flags & CHANNEL_NONBLOCKING)) {
	SetBlockMode(NULL, csPtr->readPtr,
		nonBlocking ? TCL_MODE_NONBLOCKING : TCL_MODE_BLOCKING);
    }
    if (csPtr->writePtr != csPtr->writePtr) {
	if (nonBlocking != (csPtr->writePtr->flags & CHANNEL_NONBLOCKING)) {
	    SetBlockMode(NULL, csPtr->writePtr,
		    nonBlocking ? TCL_MODE_NONBLOCKING : TCL_MODE_BLOCKING);
	}
    }
    csPtr->writePtr->flags &= ~(CHANNEL_LINEBUFFERED | CHANNEL_UNBUFFERED);
    csPtr->writePtr->flags |=
	csPtr->writeFlags & (CHANNEL_LINEBUFFERED | CHANNEL_UNBUFFERED);
	    

    if (csPtr->cmdPtr) {
	Tcl_DeleteChannelHandler((Tcl_Channel)csPtr->readPtr, CopyEventProc,
	    (ClientData)csPtr);
	if (csPtr->readPtr != csPtr->writePtr) {
	    Tcl_DeleteChannelHandler((Tcl_Channel)csPtr->writePtr,
		    CopyEventProc, (ClientData)csPtr);
	}
        Tcl_DecrRefCount(csPtr->cmdPtr);
    }
    csPtr->readPtr->csPtr = NULL;
    csPtr->writePtr->csPtr = NULL;
    ckfree((char*) csPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * SetBlockMode --
 *
 *	This function sets the blocking mode for a channel and updates
 *	the state flags.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Modifies the blocking mode of the channel and possibly generates
 *	an error.
 *
 *----------------------------------------------------------------------
 */

static int
SetBlockMode(interp, chanPtr, mode)
    Tcl_Interp *interp;		/* Interp for error reporting. */
    Channel *chanPtr;		/* Channel to modify. */
    int mode;			/* One of TCL_MODE_BLOCKING or
				 * TCL_MODE_NONBLOCKING. */
{
    int result = 0;
    if (chanPtr->typePtr->blockModeProc != NULL) {
	result = (chanPtr->typePtr->blockModeProc) (chanPtr->instanceData,
		mode);
    }
    if (result != 0) {
	Tcl_SetErrno(result);
	if (interp != (Tcl_Interp *) NULL) {
	    Tcl_AppendResult(interp, "error setting blocking mode: ",
		    Tcl_PosixError(interp), (char *) NULL);
	}
	return TCL_ERROR;
    }
    if (mode == TCL_MODE_BLOCKING) {
	chanPtr->flags &= (~(CHANNEL_NONBLOCKING | BG_FLUSH_SCHEDULED));
    } else {
	chanPtr->flags |= CHANNEL_NONBLOCKING;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetChannelNames --
 *
 *	Return the names of all open channels in the interp.
 *
 * Results:
 *	TCL_OK or TCL_ERROR.
 *
 * Side effects:
 *	Interp result modified with list of channel names.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetChannelNames(interp)
    Tcl_Interp *interp;		/* Interp for error reporting. */
{
    Channel *chanPtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    char *name;

    Tcl_ResetResult(interp);
    chanPtr = tsdPtr->firstChanPtr;
    while (chanPtr != NULL) {
        if (chanPtr == (Channel *) tsdPtr->stdinChannel) {
	    name = "stdin";
	} else if (chanPtr == (Channel *) tsdPtr->stdoutChannel) {
	    name = "stdout";
	} else if (chanPtr == (Channel *) tsdPtr->stderrChannel) {
	    name = "stderr";
	} else {
	    name = chanPtr->channelName;
	}
	Tcl_AppendElement(interp, name);
	chanPtr = chanPtr->nextChanPtr;
    }
    return TCL_OK;
}
