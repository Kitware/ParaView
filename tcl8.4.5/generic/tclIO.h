/* 
 * tclIO.h --
 *
 *	This file provides the generic portions (those that are the same on
 *	all platforms and for all channel types) of Tcl's IO facilities.
 *
 * Copyright (c) 1998-2000 Ajuba Solutions
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

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
    struct ChannelState *state; /* Split out state information */

    ClientData instanceData;	/* Instance-specific data provided by
				 * creator of channel. */
    Tcl_ChannelType *typePtr;	/* Pointer to channel type structure. */

    struct Channel *downChanPtr;/* Refers to channel this one was stacked
				 * upon.  This reference is NULL for normal
				 * channels.  See Tcl_StackChannel. */
    struct Channel *upChanPtr;	/* Refers to the channel above stacked this
				 * one. NULL for the top most channel. */

    /*
     * Intermediate buffers to hold pre-read data for consumption by a
     * newly stacked transformation. See 'Tcl_StackChannel'.
     */
    ChannelBuffer *inQueueHead;	/* Points at first buffer in input queue. */
    ChannelBuffer *inQueueTail;	/* Points at last buffer in input queue. */
} Channel;

/*
 * struct ChannelState:
 *
 * One of these structures is allocated for each open channel. It contains data
 * specific to the channel but which belongs to the generic part of the Tcl
 * channel mechanism, and it points at an instance specific (and type
 * specific) * instance data, and at a channel type structure.
 */

typedef struct ChannelState {
    CONST char *channelName;	/* The name of the channel instance in Tcl
				 * commands. Storage is owned by the generic IO
				 * code, is dynamically allocated. */
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
    TclEolTranslation inputTranslation;
				/* What translation to apply for end of line
				 * sequences on input? */    
    TclEolTranslation outputTranslation;
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
    EventScriptRecord *scriptRecordPtr;
				/* Chain of all scripts registered for
				 * event handlers ("fileevent") on this
				 * channel. */

    int bufSize;		/* What size buffers to allocate? */
    Tcl_TimerToken timer;	/* Handle to wakeup timer for this channel. */
    CopyState *csPtr;		/* State of background copy, or NULL. */
    Channel *topChanPtr;	/* Refers to topmost channel in a stack.
				 * Never NULL. */
    Channel *bottomChanPtr;	/* Refers to bottommost channel in a stack.
				 * This channel can be relied on to live as
				 * long as the channel state. Never NULL. */
    struct ChannelState *nextCSPtr;
				/* Next in list of channels currently open. */
    Tcl_ThreadId managingThread; /* TIP #10: Id of the thread managing
				  * this stack of channels. */
} ChannelState;
    
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
#define CHANNEL_BLOCKED		(1<<11)	/* EWOULDBLOCK or EAGAIN occurred
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
#define CHANNEL_RAW_MODE	(1<<16)	/* When set, notes that the Raw API is
					 * being used. */
#define CHANNEL_TIMER_FEV       (1<<17) /* When set the event we are
					 * notified by is a fileevent
					 * generated by a timer. We
					 * don't know if the driver
					 * has more data and should
					 * not try to read from it. If
					 * the system needs more than
					 * is in the buffers out read
					 * routines will simulate a
					 * short read (0 characters
					 * read) */

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
