/*
 * tclIOGT.c --
 *
 *	Implements a generic transformation exposing the underlying API
 *	at the script level.  Contributed by Andreas Kupries.
 *
 * Copyright (c) 2000 Ajuba Solutions
 * Copyright (c) 1999-2000 Andreas Kupries (a.kupries@westend.com)
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * CVS: Id
 */

#include "tclInt.h"
#include "tclPort.h"
#include "tclIO.h"


/*
 * Forward declarations of internal procedures.
 * First the driver procedures of the transformation.
 */

static int		TransformBlockModeProc _ANSI_ARGS_ ((
				ClientData instanceData, int mode));
static int		TransformCloseProc _ANSI_ARGS_ ((
				ClientData instanceData, Tcl_Interp* interp));
static int		TransformInputProc _ANSI_ARGS_ ((
				ClientData instanceData,
				char* buf, int toRead, int* errorCodePtr));
static int		TransformOutputProc _ANSI_ARGS_ ((
				ClientData instanceData, CONST char *buf,
				int toWrite, int* errorCodePtr));
static int		TransformSeekProc _ANSI_ARGS_ ((
				ClientData instanceData, long offset,
				int mode, int* errorCodePtr));
static int		TransformSetOptionProc _ANSI_ARGS_((
				ClientData instanceData, Tcl_Interp *interp,
				CONST char *optionName, CONST char *value));
static int		TransformGetOptionProc _ANSI_ARGS_((
				ClientData instanceData, Tcl_Interp *interp,
				CONST char *optionName, Tcl_DString *dsPtr));
static void		TransformWatchProc _ANSI_ARGS_ ((
				ClientData instanceData, int mask));
static int		TransformGetFileHandleProc _ANSI_ARGS_ ((
				ClientData instanceData, int direction,
				ClientData* handlePtr));
static int		TransformNotifyProc _ANSI_ARGS_ ((
				ClientData instanceData, int mask));
static Tcl_WideInt	TransformWideSeekProc _ANSI_ARGS_ ((
				ClientData instanceData, Tcl_WideInt offset,
				int mode, int* errorCodePtr));

/*
 * Forward declarations of internal procedures.
 * Secondly the procedures for handling and generating fileeevents.
 */

static void		TransformChannelHandlerTimer _ANSI_ARGS_ ((
				ClientData clientData));

/*
 * Forward declarations of internal procedures.
 * Third, helper procedures encapsulating essential tasks.
 */

typedef struct TransformChannelData TransformChannelData;

static int		ExecuteCallback _ANSI_ARGS_ ((
				TransformChannelData* ctrl, Tcl_Interp* interp,
				unsigned char* op, unsigned char* buf,
				int bufLen, int transmit, int preserve));

/*
 * Action codes to give to 'ExecuteCallback' (argument 'transmit')
 * confering to the procedure what to do with the result of the script
 * it calls.
 */

#define TRANSMIT_DONT  (0) /* No transfer to do */
#define TRANSMIT_DOWN  (1) /* Transfer to the underlying channel */
#define TRANSMIT_SELF  (2) /* Transfer into our channel. */
#define TRANSMIT_IBUF  (3) /* Transfer to internal input buffer */
#define TRANSMIT_NUM   (4) /* Transfer number to 'maxRead' */

/*
 * Codes for 'preserve' of 'ExecuteCallback'
 */

#define P_PRESERVE    (1)
#define P_NO_PRESERVE (0)

/*
 * Strings for the action codes delivered to the script implementing
 * a transformation. Argument 'op' of 'ExecuteCallback'.
 */

#define A_CREATE_WRITE	(UCHARP ("create/write"))
#define A_DELETE_WRITE	(UCHARP ("delete/write"))
#define A_FLUSH_WRITE	(UCHARP ("flush/write"))
#define A_WRITE		(UCHARP ("write"))

#define A_CREATE_READ	(UCHARP ("create/read"))
#define A_DELETE_READ	(UCHARP ("delete/read"))
#define A_FLUSH_READ	(UCHARP ("flush/read"))
#define A_READ		(UCHARP ("read"))

#define A_QUERY_MAXREAD (UCHARP ("query/maxRead"))
#define A_CLEAR_READ	(UCHARP ("clear/read"))

/*
 * Management of a simple buffer.
 */

typedef struct ResultBuffer ResultBuffer;

static void		ResultClear  _ANSI_ARGS_ ((ResultBuffer* r));
static void		ResultInit   _ANSI_ARGS_ ((ResultBuffer* r));
static int		ResultLength _ANSI_ARGS_ ((ResultBuffer* r));
static int		ResultCopy   _ANSI_ARGS_ ((ResultBuffer* r,
				unsigned char* buf, int toRead));
static void		ResultAdd    _ANSI_ARGS_ ((ResultBuffer* r,
				unsigned char* buf, int toWrite));

/*
 * This structure describes the channel type structure for tcl based
 * transformations.
 */

static Tcl_ChannelType transformChannelType = {
    "transform",			/* Type name. */
    TCL_CHANNEL_VERSION_2,
    TransformCloseProc,			/* Close proc. */
    TransformInputProc,			/* Input proc. */
    TransformOutputProc,		/* Output proc. */
    TransformSeekProc,			/* Seek proc. */
    TransformSetOptionProc,		/* Set option proc. */
    TransformGetOptionProc,		/* Get option proc. */
    TransformWatchProc,			/* Initialize notifier. */
    TransformGetFileHandleProc,		/* Get OS handles out of channel. */
    NULL,				/* close2proc */
    TransformBlockModeProc,		/* Set blocking/nonblocking mode.*/
    NULL,				/* Flush proc. */
    TransformNotifyProc,                /* Handling of events bubbling up */
    TransformWideSeekProc,		/* Wide seek proc */
};

/*
 * Possible values for 'flags' field in control structure, see below.
 */

#define CHANNEL_ASYNC		(1<<0) /* non-blocking mode */

/*
 * Definition of the structure containing the information about the
 * internal input buffer.
 */

struct ResultBuffer {
    unsigned char* buf;       /* Reference to the buffer area */
    int		   allocated; /* Allocated size of the buffer area */
    int		   used;      /* Number of bytes in the buffer, <= allocated */
};

/*
 * Additional bytes to allocate during buffer expansion
 */

#define INCREMENT (512)

/*
 * Number of milliseconds to wait before firing an event to flush
 * out information waiting in buffers (fileevent support).
 */

#define FLUSH_DELAY (5)

/*
 * Convenience macro to make some casts easier to use.
 */

#define UCHARP(x) ((unsigned char*) (x))
#define NO_INTERP ((Tcl_Interp*) NULL)

/*
 * Definition of a structure used by all transformations generated here to
 * maintain their local state.
 */

struct TransformChannelData {

    /*
     * General section. Data to integrate the transformation into the channel
     * system.
     */

    Tcl_Channel self;     /* Our own Channel handle */
    int readIsFlushed;    /* Flag to note wether in.flushProc was called or not
			   */
    int flags;            /* Currently CHANNEL_ASYNC or zero */
    int watchMask;        /* Current watch/event/interest mask */
    int mode;             /* mode of parent channel, OR'ed combination of
			   * TCL_READABLE, TCL_WRITABLE */
    Tcl_TimerToken timer; /* Timer for automatic flushing of information
			   * sitting in an internal buffer. Required for full
			   * fileevent support */
    /*
     * Transformation specific data.
     */

    int maxRead;            /* Maximum allowed number of bytes to read, as
			     * given to us by the tcl script implementing the
			     * transformation. */
    Tcl_Interp*    interp;  /* Reference to the interpreter which created the
			     * transformation. Used to execute the code
			     * below. */
    Tcl_Obj*       command; /* Tcl code to execute for a buffer */
    ResultBuffer   result;  /* Internal buffer used to store the result of a
			     * transformation of incoming data. Additionally
			     * serves as buffer of all data not yet consumed by
			     * the reader. */
};


/*
 *----------------------------------------------------------------------
 *
 * TclChannelTransform --
 *
 *	Implements the Tcl "testchannel transform" debugging command.
 *	This is part of the testing environment.  This sets up a tcl
 *	script (cmdObjPtr) to be used as a transform on the channel.
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
TclChannelTransform(interp, chan, cmdObjPtr)
    Tcl_Interp	*interp;	/* Interpreter for result. */
    Tcl_Channel chan;		/* Channel to transform. */
    Tcl_Obj	*cmdObjPtr;	/* Script to use for transform. */
{
    Channel			*chanPtr;	/* The actual channel. */
    ChannelState		*statePtr;	/* state info for channel */
    int				mode;		/* rw mode of the channel */
    TransformChannelData	*dataPtr;
    int				res;
    Tcl_DString			ds;

    if (chan == (Tcl_Channel) NULL) {
	return TCL_ERROR;
    }
    chanPtr	= (Channel *) chan;
    statePtr	= chanPtr->state;
    chanPtr	= statePtr->topChanPtr;
    chan	= (Tcl_Channel) chanPtr;
    mode	= (statePtr->flags & (TCL_READABLE|TCL_WRITABLE));

    /*
     * Now initialize the transformation state and stack it upon the
     * specified channel. One of the necessary things to do is to
     * retrieve the blocking regime of the underlying channel and to
     * use the same for us too.
     */

    dataPtr = (TransformChannelData*) ckalloc(sizeof(TransformChannelData));

    Tcl_DStringInit (&ds);
    Tcl_GetChannelOption(interp, chan, "-blocking", &ds);

    dataPtr->readIsFlushed = 0;
    dataPtr->flags	= 0;

    if (ds.string[0] == '0') {
	dataPtr->flags |= CHANNEL_ASYNC;
    }

    Tcl_DStringFree (&ds);

    dataPtr->self	= chan;
    dataPtr->watchMask	= 0;
    dataPtr->mode	= mode;
    dataPtr->timer	= (Tcl_TimerToken) NULL;
    dataPtr->maxRead	= 4096; /* Initial value not relevant */
    dataPtr->interp	= interp;
    dataPtr->command	= cmdObjPtr;

    Tcl_IncrRefCount(dataPtr->command);

    ResultInit(&dataPtr->result);

    dataPtr->self = Tcl_StackChannel(interp, &transformChannelType,
	    (ClientData) dataPtr, mode, chan);
    if (dataPtr->self == (Tcl_Channel) NULL) {
	Tcl_AppendResult(interp, "\nfailed to stack channel \"",
		Tcl_GetChannelName(chan), "\"", (char *) NULL);

	Tcl_DecrRefCount(dataPtr->command);
	ResultClear(&dataPtr->result);
	ckfree((VOID *) dataPtr);
	return TCL_ERROR;
    }

    /*
     * At last initialize the transformation at the script level.
     */

    if (dataPtr->mode & TCL_WRITABLE) {
	res = ExecuteCallback (dataPtr, NO_INTERP, A_CREATE_WRITE,
		NULL, 0, TRANSMIT_DONT, P_NO_PRESERVE);

	if (res != TCL_OK) {
	    Tcl_UnstackChannel(interp, chan);
	    return TCL_ERROR;
	}
    }

    if (dataPtr->mode & TCL_READABLE) {
	res = ExecuteCallback (dataPtr, NO_INTERP, A_CREATE_READ,
		NULL, 0, TRANSMIT_DONT, P_NO_PRESERVE);

	if (res != TCL_OK) {
	    ExecuteCallback (dataPtr, NO_INTERP, A_DELETE_WRITE,
		    NULL, 0, TRANSMIT_DONT, P_NO_PRESERVE);

	    Tcl_UnstackChannel(interp, chan);
	    return TCL_ERROR;
	}
    }

    return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	ExecuteCallback --
 *
 *	Executes the defined callback for buffer and
 *	operation.
 *
 *	Sideeffects:
 *		As of the executed tcl script.
 *
 *	Result:
 *		A standard TCL error code. In case of an
 *		error a message is left in the result area
 *		of the specified interpreter.
 *
 *------------------------------------------------------*
 */

static int
ExecuteCallback (dataPtr, interp, op, buf, bufLen, transmit, preserve)
    TransformChannelData* dataPtr;  /* Transformation with the callback */
    Tcl_Interp*           interp;   /* Current interpreter, possibly NULL */
    unsigned char*        op;       /* Operation invoking the callback */
    unsigned char*        buf;      /* Buffer to give to the script. */
    int			  bufLen;   /* Ands its length */
    int                   transmit; /* Flag, determines whether the result
				     * of the callback is sent to the
				     * underlying channel or not. */
    int                   preserve; /* Flag. If true the procedure will
				     * preserver the result state of all
				     * accessed interpreters. */
{
    /*
     * Step 1, create the complete command to execute. Do this by appending
     * operation and buffer to operate upon to a copy of the callback
     * definition. We *cannot* create a list containing 3 objects and then use
     * 'Tcl_EvalObjv', because the command may contain additional prefixed
     * arguments. Feather's curried commands would come in handy here.
     */

    Tcl_Obj* resObj;		    /* See below, switch (transmit) */
    int resLen;
    unsigned char* resBuf;
    Tcl_SavedResult ciSave;
    int res = TCL_OK;
    Tcl_Obj* command = Tcl_DuplicateObj (dataPtr->command);
    Tcl_Obj* temp;

    if (preserve) {
	Tcl_SaveResult (dataPtr->interp, &ciSave);
    }

    if (command == (Tcl_Obj*) NULL) {
        /* Memory allocation problem */
        res = TCL_ERROR;
        goto cleanup;
    }

    Tcl_IncrRefCount(command);

    temp = Tcl_NewStringObj((char*) op, -1);

    if (temp == (Tcl_Obj*) NULL) {
        /* Memory allocation problem */
        res = TCL_ERROR;
        goto cleanup;
    }

    res = Tcl_ListObjAppendElement(dataPtr->interp, command, temp);

    if (res != TCL_OK)
	goto cleanup;

    /*
     * Use a byte-array to prevent the misinterpretation of binary data
     * coming through as UTF while at the tcl level.
     */

    temp = Tcl_NewByteArrayObj(buf, bufLen);

    if (temp == (Tcl_Obj*) NULL) {
        /* Memory allocation problem */
	res = TCL_ERROR;
        goto cleanup;
    }

    res = Tcl_ListObjAppendElement (dataPtr->interp, command, temp);

    if (res != TCL_OK)
        goto cleanup;

    /*
     * Step 2, execute the command at the global level of the interpreter
     * used to create the transformation. Destroy the command afterward.
     * If an error occured and the current interpreter is defined and not
     * equal to the interpreter for the callback, then copy the error
     * message into current interpreter. Don't copy if in preservation mode.
     */

    res = Tcl_GlobalEvalObj (dataPtr->interp, command);
    Tcl_DecrRefCount (command);
    command = (Tcl_Obj*) NULL;

    if ((res != TCL_OK) && (interp != NO_INTERP) &&
	    (dataPtr->interp != interp) && !preserve) {
        Tcl_SetObjResult(interp, Tcl_GetObjResult(dataPtr->interp));
	return res;
    }

    /*
     * Step 3, transmit a possible conversion result to the underlying
     * channel, or ourselves.
     */

    switch (transmit) {
	case TRANSMIT_DONT:
	    /* nothing to do */
	    break;

	case TRANSMIT_DOWN:
	    resObj = Tcl_GetObjResult(dataPtr->interp);
	    resBuf = (unsigned char*) Tcl_GetByteArrayFromObj(resObj, &resLen);
	    Tcl_WriteRaw(Tcl_GetStackedChannel(dataPtr->self),
		    (char*) resBuf, resLen);
	    break;

	case TRANSMIT_SELF:
	    resObj = Tcl_GetObjResult (dataPtr->interp);
	    resBuf = (unsigned char*) Tcl_GetByteArrayFromObj(resObj, &resLen);
	    Tcl_WriteRaw(dataPtr->self, (char*) resBuf, resLen);
	    break;

	case TRANSMIT_IBUF:
	    resObj = Tcl_GetObjResult (dataPtr->interp);
	    resBuf = (unsigned char*) Tcl_GetByteArrayFromObj(resObj, &resLen);
	    ResultAdd(&dataPtr->result, resBuf, resLen);
	    break;

	case TRANSMIT_NUM:
	    /* Interpret result as integer number */
	    resObj = Tcl_GetObjResult (dataPtr->interp);
	    Tcl_GetIntFromObj(dataPtr->interp, resObj, &dataPtr->maxRead);
	    break;
    }

    Tcl_ResetResult(dataPtr->interp);

    if (preserve) {
	Tcl_RestoreResult(dataPtr->interp, &ciSave);
    }

    return res;

    cleanup:
    if (preserve) {
	Tcl_RestoreResult(dataPtr->interp, &ciSave);
    }

    if (command != (Tcl_Obj*) NULL) {
        Tcl_DecrRefCount(command);
    }

    return res;
}

/*
 *------------------------------------------------------*
 *
 *	TransformBlockModeProc --
 *
 *	Trap handler. Called by the generic IO system
 *	during option processing to change the blocking
 *	mode of the channel.
 *
 *	Sideeffects:
 *		Forwards the request to the underlying
 *		channel.
 *
 *	Result:
 *		0 if successful, errno when failed.
 *
 *------------------------------------------------------*
 */

static int
TransformBlockModeProc (instanceData, mode)
    ClientData  instanceData; /* State of transformation */
    int         mode;         /* New blocking mode */
{
    TransformChannelData* dataPtr = (TransformChannelData*) instanceData;

    if (mode == TCL_MODE_NONBLOCKING) {
        dataPtr->flags |= CHANNEL_ASYNC;
    } else {
        dataPtr->flags &= ~(CHANNEL_ASYNC);
    }
    return 0;
}

/*
 *------------------------------------------------------*
 *
 *	TransformCloseProc --
 *
 *	Trap handler. Called by the generic IO system
 *	during destruction of the transformation channel.
 *
 *	Sideeffects:
 *		Releases the memory allocated in
 *		'Tcl_TransformObjCmd'.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static int
TransformCloseProc (instanceData, interp)
    ClientData  instanceData;
    Tcl_Interp* interp;
{
    TransformChannelData* dataPtr = (TransformChannelData*) instanceData;

    /*
     * Important: In this procedure 'dataPtr->self' already points to
     * the underlying channel.
     */

    /*
     * There is no need to cancel an existing channel handler, this is already
     * done. Either by 'Tcl_UnstackChannel' or by the general cleanup in
     * 'Tcl_Close'.
     *
     * But we have to cancel an active timer to prevent it from firing on the
     * removed channel.
     */

    if (dataPtr->timer != (Tcl_TimerToken) NULL) {
        Tcl_DeleteTimerHandler (dataPtr->timer);
	dataPtr->timer = (Tcl_TimerToken) NULL;
    }

    /*
     * Now flush data waiting in internal buffers to output and input. The
     * input must be done despite the fact that there is no real receiver
     * for it anymore. But the scripts might have sideeffects other parts
     * of the system rely on (f.e. signaling the close to interested parties).
     */

    if (dataPtr->mode & TCL_WRITABLE) {
        ExecuteCallback (dataPtr, interp, A_FLUSH_WRITE,
		NULL, 0, TRANSMIT_DOWN, 1);
    }

    if ((dataPtr->mode & TCL_READABLE) && !dataPtr->readIsFlushed) {
	dataPtr->readIsFlushed = 1;
        ExecuteCallback (dataPtr, interp, A_FLUSH_READ,
		NULL, 0, TRANSMIT_IBUF, 1);
    }

    if (dataPtr->mode & TCL_WRITABLE) {
        ExecuteCallback (dataPtr, interp, A_DELETE_WRITE,
		NULL, 0, TRANSMIT_DONT, 1);
    }

    if (dataPtr->mode & TCL_READABLE) {
        ExecuteCallback (dataPtr, interp, A_DELETE_READ,
		NULL, 0, TRANSMIT_DONT, 1);
    }

    /*
     * General cleanup
     */

    ResultClear(&dataPtr->result);
    Tcl_DecrRefCount(dataPtr->command);
    ckfree((VOID*) dataPtr);

    return TCL_OK;
}

/*
 *------------------------------------------------------*
 *
 *	TransformInputProc --
 *
 *	Called by the generic IO system to convert read data.
 *
 *	Sideeffects:
 *		As defined by the conversion.
 *
 *	Result:
 *		A transformed buffer.
 *
 *------------------------------------------------------*
 */

static int
TransformInputProc (instanceData, buf, toRead, errorCodePtr)
    ClientData instanceData;
    char*      buf;
    int	       toRead;
    int*       errorCodePtr;
{
    TransformChannelData* dataPtr = (TransformChannelData*) instanceData;
    int gotBytes, read, res, copied;
    Tcl_Channel downChan;

    /* should assert (dataPtr->mode & TCL_READABLE) */

    if (toRead == 0) {
	/* Catch a no-op.
	 */
	return 0;
    }

    gotBytes = 0;
    downChan = Tcl_GetStackedChannel(dataPtr->self);

    while (toRead > 0) {
        /*
	 * Loop until the request is satisfied (or no data is available from
	 * below, possibly EOF).
	 */

        copied    = ResultCopy (&dataPtr->result, UCHARP (buf), toRead);

	toRead   -= copied;
	buf      += copied;
	gotBytes += copied;

	if (toRead == 0) {
	    /* The request was completely satisfied from our buffers.
	     * We can break out of the loop and return to the caller.
	     */
	    return gotBytes;
	}

	/*
	 * Length (dataPtr->result) == 0, toRead > 0 here . Use the incoming
	 * 'buf'! as target to store the intermediary information read
	 * from the underlying channel.
	 *
	 * Ask the tcl level how much data it allows us to read from
	 * the underlying channel. This feature allows the transform to
	 * signal EOF upstream although there is none downstream. Useful
	 * to control an unbounded 'fcopy', either through counting bytes,
	 * or by pattern matching.
	 */

	ExecuteCallback (dataPtr, NO_INTERP, A_QUERY_MAXREAD,
		NULL, 0, TRANSMIT_NUM /* -> maxRead */, 1);

	if (dataPtr->maxRead >= 0) {
	    if (dataPtr->maxRead < toRead) {
	        toRead = dataPtr->maxRead;
	    }
	} /* else: 'maxRead < 0' == Accept the current value of toRead */

	if (toRead <= 0) {
	    return gotBytes;
	}

	read = Tcl_ReadRaw(downChan, buf, toRead);

	if (read < 0) {
	    /* Report errors to caller. EAGAIN is a special situation.
	     * If we had some data before we report that instead of the
	     * request to re-try.
	     */

	    if ((Tcl_GetErrno() == EAGAIN) && (gotBytes > 0)) {
	        return gotBytes;
	    }

	    *errorCodePtr = Tcl_GetErrno();
	    return -1;      
	}

	if (read == 0) {
	    /*
	     * Check wether we hit on EOF in the underlying channel or
	     * not. If not differentiate between blocking and
	     * non-blocking modes. In non-blocking mode we ran
	     * temporarily out of data. Signal this to the caller via
	     * EWOULDBLOCK and error return (-1). In the other cases
	     * we simply return what we got and let the caller wait
	     * for more. On the other hand, if we got an EOF we have
	     * to convert and flush all waiting partial data.
	     */

	    if (! Tcl_Eof (downChan)) {
	        if ((gotBytes == 0) && (dataPtr->flags & CHANNEL_ASYNC)) {
		    *errorCodePtr = EWOULDBLOCK;
		    return -1;
		} else {
		    return gotBytes;
		}
	    } else {
	        if (dataPtr->readIsFlushed) {
		    /* Already flushed, nothing to do anymore
		     */
		    return gotBytes;
		}

		dataPtr->readIsFlushed = 1;

		ExecuteCallback (dataPtr, NO_INTERP, A_FLUSH_READ,
			NULL, 0, TRANSMIT_IBUF, P_PRESERVE);

		if (ResultLength (&dataPtr->result) == 0) {
		    /* we had nothing to flush */
		    return gotBytes;
		}

		continue; /* at: while (toRead > 0) */
	    }
	} /* read == 0 */

	/* Transform the read chunk and add the result to our
	 * read buffer (dataPtr->result)
	 */

	res = ExecuteCallback (dataPtr, NO_INTERP, A_READ,
		UCHARP (buf), read, TRANSMIT_IBUF, P_PRESERVE);

	if (res != TCL_OK) {
	    *errorCodePtr = EINVAL;
	    return -1;
	}
    } /* while toRead > 0 */

    return gotBytes;
}

/*
 *------------------------------------------------------*
 *
 *	TransformOutputProc --
 *
 *	Called by the generic IO system to convert data
 *	waiting to be written.
 *
 *	Sideeffects:
 *		As defined by the transformation.
 *
 *	Result:
 *		A transformed buffer.
 *
 *------------------------------------------------------*
 */

static int
TransformOutputProc (instanceData, buf, toWrite, errorCodePtr)
    ClientData instanceData;
    CONST char*      buf;
    int        toWrite;
    int*       errorCodePtr;
{
    TransformChannelData* dataPtr = (TransformChannelData*) instanceData;
    int res;

    /* should assert (dataPtr->mode & TCL_WRITABLE) */

    if (toWrite == 0) {
	/* Catch a no-op.
	 */
	return 0;
    }

    res = ExecuteCallback (dataPtr, NO_INTERP, A_WRITE,
	    UCHARP (buf), toWrite,
	    TRANSMIT_DOWN, P_NO_PRESERVE);

    if (res != TCL_OK) {
        *errorCodePtr = EINVAL;
	return -1;
    }

    return toWrite;
}

/*
 *------------------------------------------------------*
 *
 *	TransformSeekProc --
 *
 *	This procedure is called by the generic IO level
 *	to move the access point in a channel.
 *
 *	Sideeffects:
 *		Moves the location at which the channel
 *		will be accessed in future operations.
 *		Flushes all transformation buffers, then
 *		forwards it to the underlying channel.
 *
 *	Result:
 *		-1 if failed, the new position if
 *		successful. An output argument contains
 *		the POSIX error code if an error
 *		occurred, or zero.
 *
 *------------------------------------------------------*
 */

static int
TransformSeekProc (instanceData, offset, mode, errorCodePtr)
    ClientData  instanceData;	/* The channel to manipulate */
    long	offset;		/* Size of movement. */
    int         mode;		/* How to move */
    int*        errorCodePtr;	/* Location of error flag. */
{
    TransformChannelData* dataPtr	= (TransformChannelData*) instanceData;
    Tcl_Channel           parent        = Tcl_GetStackedChannel(dataPtr->self);
    Tcl_ChannelType*      parentType	= Tcl_GetChannelType(parent);
    Tcl_DriverSeekProc*   parentSeekProc = Tcl_ChannelSeekProc(parentType);

    if ((offset == 0) && (mode == SEEK_CUR)) {
        /* This is no seek but a request to tell the caller the current
	 * location. Simply pass the request down.
	 */

	return (*parentSeekProc) (Tcl_GetChannelInstanceData(parent),
		offset, mode, errorCodePtr);
    }

    /*
     * It is a real request to change the position. Flush all data waiting
     * for output and discard everything in the input buffers. Then pass
     * the request down, unchanged.
     */

    if (dataPtr->mode & TCL_WRITABLE) {
        ExecuteCallback (dataPtr, NO_INTERP, A_FLUSH_WRITE,
		NULL, 0, TRANSMIT_DOWN, P_NO_PRESERVE);
    }

    if (dataPtr->mode & TCL_READABLE) {
        ExecuteCallback (dataPtr, NO_INTERP, A_CLEAR_READ,
		NULL, 0, TRANSMIT_DONT, P_NO_PRESERVE);
	ResultClear(&dataPtr->result);
	dataPtr->readIsFlushed = 0;
    }

    return (*parentSeekProc) (Tcl_GetChannelInstanceData(parent),
	    offset, mode, errorCodePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TransformWideSeekProc --
 *
 *	This procedure is called by the generic IO level to move the
 *	access point in a channel, with a (potentially) 64-bit offset.
 *
 * Side effects:
 *	Moves the location at which the channel will be accessed in
 *	future operations.  Flushes all transformation buffers, then
 *	forwards it to the underlying channel.
 *
 * Result:
 *	-1 if failed, the new position if successful. An output
 *	argument contains the POSIX error code if an error occurred,
 *	or zero.
 *
 *----------------------------------------------------------------------
 */

static Tcl_WideInt
TransformWideSeekProc (instanceData, offset, mode, errorCodePtr)
    ClientData  instanceData;	/* The channel to manipulate */
    Tcl_WideInt offset;		/* Size of movement. */
    int         mode;		/* How to move */
    int*        errorCodePtr;	/* Location of error flag. */
{
    TransformChannelData* dataPtr =
	(TransformChannelData*) instanceData;
    Tcl_Channel parent =
	Tcl_GetStackedChannel(dataPtr->self);
    Tcl_ChannelType* parentType	=
	Tcl_GetChannelType(parent);
    Tcl_DriverSeekProc* parentSeekProc =
	Tcl_ChannelSeekProc(parentType);
    Tcl_DriverWideSeekProc* parentWideSeekProc =
	Tcl_ChannelWideSeekProc(parentType);
    ClientData parentData =
	Tcl_GetChannelInstanceData(parent);

    if ((offset == Tcl_LongAsWide(0)) && (mode == SEEK_CUR)) {
        /*
	 * This is no seek but a request to tell the caller the current
	 * location. Simply pass the request down.
	 */

	if (parentWideSeekProc != NULL) {
	    return (*parentWideSeekProc) (parentData, offset, mode,
		    errorCodePtr);
	}

	return Tcl_LongAsWide((*parentSeekProc) (parentData, 0, mode,
		errorCodePtr));
    }

    /*
     * It is a real request to change the position. Flush all data waiting
     * for output and discard everything in the input buffers. Then pass
     * the request down, unchanged.
     */

    if (dataPtr->mode & TCL_WRITABLE) {
        ExecuteCallback (dataPtr, NO_INTERP, A_FLUSH_WRITE,
		NULL, 0, TRANSMIT_DOWN, P_NO_PRESERVE);
    }

    if (dataPtr->mode & TCL_READABLE) {
        ExecuteCallback (dataPtr, NO_INTERP, A_CLEAR_READ,
		NULL, 0, TRANSMIT_DONT, P_NO_PRESERVE);
	ResultClear(&dataPtr->result);
	dataPtr->readIsFlushed = 0;
    }

    /*
     * If we have a wide seek capability, we should stick with that.
     */
    if (parentWideSeekProc != NULL) {
	return (*parentWideSeekProc) (parentData, offset, mode, errorCodePtr);
    }

    /*
     * We're transferring to narrow seeks at this point; this is a bit
     * complex because we have to check whether the seek is possible
     * first (i.e. whether we are losing information in truncating the
     * bits of the offset.)  Luckily, there's a defined error for what
     * happens when trying to go out of the representable range.
     */
    if (offset<Tcl_LongAsWide(LONG_MIN) || offset>Tcl_LongAsWide(LONG_MAX)) {
	*errorCodePtr = EOVERFLOW;
	return Tcl_LongAsWide(-1);
    }
    return Tcl_LongAsWide((*parentSeekProc) (parentData,
	    Tcl_WideAsLong(offset), mode, errorCodePtr));
}

/*
 *------------------------------------------------------*
 *
 *	TransformSetOptionProc --
 *
 *	Called by generic layer to handle the reconfi-
 *	guration of channel specific options. As this
 *	channel type does not have such, it simply passes
 *	all requests downstream.
 *
 *	Sideeffects:
 *		As defined by the channel downstream.
 *
 *	Result:
 *		A standard TCL error code.
 *
 *------------------------------------------------------*
 */

static int
TransformSetOptionProc (instanceData, interp, optionName, value)
    ClientData instanceData;
    Tcl_Interp *interp;
    CONST char *optionName;
    CONST char *value;
{
    TransformChannelData* dataPtr = (TransformChannelData*) instanceData;
    Tcl_Channel downChan = Tcl_GetStackedChannel(dataPtr->self);
    Tcl_DriverSetOptionProc *setOptionProc;

    setOptionProc = Tcl_ChannelSetOptionProc(Tcl_GetChannelType(downChan));
    if (setOptionProc != NULL) {
	return (*setOptionProc)(Tcl_GetChannelInstanceData(downChan),
		interp, optionName, value);
    }
    return TCL_ERROR;
}

/*
 *------------------------------------------------------*
 *
 *	TransformGetOptionProc --
 *
 *	Called by generic layer to handle requests for
 *	the values of channel specific options. As this
 *	channel type does not have such, it simply passes
 *	all requests downstream.
 *
 *	Sideeffects:
 *		As defined by the channel downstream.
 *
 *	Result:
 *		A standard TCL error code.
 *
 *------------------------------------------------------*
 */

static int
TransformGetOptionProc (instanceData, interp, optionName, dsPtr)
    ClientData   instanceData;
    Tcl_Interp*  interp;
    CONST char*        optionName;
    Tcl_DString* dsPtr;
{
    TransformChannelData* dataPtr = (TransformChannelData*) instanceData;
    Tcl_Channel downChan = Tcl_GetStackedChannel(dataPtr->self);
    Tcl_DriverGetOptionProc *getOptionProc;

    getOptionProc = Tcl_ChannelGetOptionProc(Tcl_GetChannelType(downChan));
    if (getOptionProc != NULL) {
	return (*getOptionProc)(Tcl_GetChannelInstanceData(downChan),
		interp, optionName, dsPtr);
    } else if (optionName == (CONST char*) NULL) {
	/*
	 * Request is query for all options, this is ok.
	 */
	return TCL_OK;
    }
    /*
     * Request for a specific option has to fail, we don't have any.
     */
    return TCL_ERROR;
}

/*
 *------------------------------------------------------*
 *
 *	TransformWatchProc --
 *
 *	Initialize the notifier to watch for events from
 *	this channel.
 *
 *	Sideeffects:
 *		Sets up the notifier so that a future
 *		event on the channel will be seen by Tcl.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */
	/* ARGSUSED */
static void
TransformWatchProc (instanceData, mask)
    ClientData instanceData;	/* Channel to watch */
    int        mask;		/* Events of interest */
{
    /* The caller expressed interest in events occuring for this
     * channel. We are forwarding the call to the underlying
     * channel now.
     */

    TransformChannelData* dataPtr = (TransformChannelData*) instanceData;
    Tcl_Channel     downChan;

    dataPtr->watchMask = mask;

    /* No channel handlers any more. We will be notified automatically
     * about events on the channel below via a call to our
     * 'TransformNotifyProc'. But we have to pass the interest down now.
     * We are allowed to add additional 'interest' to the mask if we want
     * to. But this transformation has no such interest. It just passes
     * the request down, unchanged.
     */

    downChan = Tcl_GetStackedChannel(dataPtr->self);

    (Tcl_GetChannelType(downChan))
	->watchProc(Tcl_GetChannelInstanceData(downChan), mask);

    /*
     * Management of the internal timer.
     */

    if ((dataPtr->timer != (Tcl_TimerToken) NULL) &&
	    (!(mask & TCL_READABLE) || (ResultLength(&dataPtr->result) == 0))) {

        /* A pending timer exists, but either is there no (more)
	 * interest in the events it generates or nothing is availablee
	 * for reading, so remove it.
	 */

        Tcl_DeleteTimerHandler (dataPtr->timer);
	dataPtr->timer = (Tcl_TimerToken) NULL;
    }

    if ((dataPtr->timer == (Tcl_TimerToken) NULL) &&
	    (mask & TCL_READABLE) && (ResultLength (&dataPtr->result) > 0)) {

        /* There is no pending timer, but there is interest in readable
	 * events and we actually have data waiting, so generate a timer
	 * to flush that.
	 */

	dataPtr->timer = Tcl_CreateTimerHandler (FLUSH_DELAY,
		TransformChannelHandlerTimer, (ClientData) dataPtr);
    }
}

/*
 *------------------------------------------------------*
 *
 *	TransformGetFileHandleProc --
 *
 *	Called from Tcl_GetChannelHandle to retrieve
 *	OS specific file handle from inside this channel.
 *
 *	Sideeffects:
 *		None.
 *
 *	Result:
 *		The appropriate Tcl_File or NULL if not
 *		present. 
 *
 *------------------------------------------------------*
 */
static int
TransformGetFileHandleProc (instanceData, direction, handlePtr)
    ClientData  instanceData;	/* Channel to query */
    int         direction;	/* Direction of interest */
    ClientData* handlePtr;	/* Place to store the handle into */
{
    /*
     * Return the handle belonging to parent channel.
     * IOW, pass the request down and the result up.
     */

    TransformChannelData* dataPtr = (TransformChannelData*) instanceData;

    return Tcl_GetChannelHandle(Tcl_GetStackedChannel(dataPtr->self),
	    direction, handlePtr);
}

/*
 *------------------------------------------------------*
 *
 *	TransformNotifyProc --
 *
 *	------------------------------------------------*
 *	Handler called by Tcl to inform us of activity
 *	on the underlying channel.
 *	------------------------------------------------*
 *
 *	Sideeffects:
 *		May process the incoming event by itself.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static int
TransformNotifyProc (clientData, mask)
    ClientData	   clientData; /* The state of the notified transformation */
    int		   mask;       /* The mask of occuring events */
{
    TransformChannelData* dataPtr = (TransformChannelData*) clientData;

    /*
     * An event occured in the underlying channel.  This
     * transformation doesn't process such events thus returns the
     * incoming mask unchanged.
     */

    if (dataPtr->timer != (Tcl_TimerToken) NULL) {
	/*
	 * Delete an existing timer. It was not fired, yet we are
	 * here, so the channel below generated such an event and we
	 * don't have to. The renewal of the interest after the
	 * execution of channel handlers will eventually cause us to
	 * recreate the timer (in TransformWatchProc).
	 */

	Tcl_DeleteTimerHandler (dataPtr->timer);
	dataPtr->timer = (Tcl_TimerToken) NULL;
    }

    return mask;
}

/*
 *------------------------------------------------------*
 *
 *	TransformChannelHandlerTimer --
 *
 *	Called by the notifier (-> timer) to flush out
 *	information waiting in the input buffer.
 *
 *	Sideeffects:
 *		As of 'Tcl_NotifyChannel'.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
TransformChannelHandlerTimer (clientData)
    ClientData clientData; /* Transformation to query */
{
    TransformChannelData* dataPtr = (TransformChannelData*) clientData;

    dataPtr->timer = (Tcl_TimerToken) NULL;

    if (!(dataPtr->watchMask & TCL_READABLE) ||
	    (ResultLength (&dataPtr->result) == 0)) {
	/* The timer fired, but either is there no (more)
	 * interest in the events it generates or nothing is available
	 * for reading, so ignore it and don't recreate it.
	 */

	return;
    }

    Tcl_NotifyChannel(dataPtr->self, TCL_READABLE);
}

/*
 *------------------------------------------------------*
 *
 *	ResultClear --
 *
 *	Deallocates any memory allocated by 'ResultAdd'.
 *
 *	Sideeffects:
 *		See above.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
ResultClear (r)
    ResultBuffer* r; /* Reference to the buffer to clear out */
{
    r->used = 0;

    if (r->allocated) {
        ckfree((char*) r->buf);
	r->buf       = UCHARP (NULL);
	r->allocated = 0;
    }
}

/*
 *------------------------------------------------------*
 *
 *	ResultInit --
 *
 *	Initializes the specified buffer structure. The
 *	structure will contain valid information for an
 *	emtpy buffer.
 *
 *	Sideeffects:
 *		See above.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
ResultInit (r)
    ResultBuffer* r; /* Reference to the structure to initialize */
{
    r->used      = 0;
    r->allocated = 0;
    r->buf       = UCHARP (NULL);
}

/*
 *------------------------------------------------------*
 *
 *	ResultLength --
 *
 *	Returns the number of bytes stored in the buffer.
 *
 *	Sideeffects:
 *		None.
 *
 *	Result:
 *		An integer, see above too.
 *
 *------------------------------------------------------*
 */

static int
ResultLength (r)
    ResultBuffer* r; /* The structure to query */
{
    return r->used;
}

/*
 *------------------------------------------------------*
 *
 *	ResultCopy --
 *
 *	Copies the requested number of bytes from the
 *	buffer into the specified array and removes them
 *	from the buffer afterward. Copies less if there
 *	is not enough data in the buffer.
 *
 *	Sideeffects:
 *		See above.
 *
 *	Result:
 *		The number of actually copied bytes,
 *		possibly less than 'toRead'.
 *
 *------------------------------------------------------*
 */

static int
ResultCopy (r, buf, toRead)
    ResultBuffer*  r;      /* The buffer to read from */
    unsigned char* buf;    /* The buffer to copy into */
    int		   toRead; /* Number of requested bytes */
{
    if (r->used == 0) {
        /* Nothing to copy in the case of an empty buffer.
	 */

        return 0;
    }

    if (r->used == toRead) {
        /* We have just enough. Copy everything to the caller.
	 */

        memcpy ((VOID*) buf, (VOID*) r->buf, (size_t) toRead);
	r->used = 0;
	return toRead;
    }

    if (r->used > toRead) {
        /* The internal buffer contains more than requested.
	 * Copy the requested subset to the caller, and shift
	 * the remaining bytes down.
	 */

        memcpy  ((VOID*) buf,    (VOID*) r->buf,            (size_t) toRead);
	memmove ((VOID*) r->buf, (VOID*) (r->buf + toRead),
		(size_t) r->used - toRead);

	r->used -= toRead;
	return toRead;
    }

    /* There is not enough in the buffer to satisfy the caller, so
     * take everything.
     */

    memcpy((VOID*) buf, (VOID*) r->buf, (size_t) r->used);
    toRead  = r->used;
    r->used = 0;
    return toRead;
}

/*
 *------------------------------------------------------*
 *
 *	ResultAdd --
 *
 *	Adds the bytes in the specified array to the
 *	buffer, by appending it.
 *
 *	Sideeffects:
 *		See above.
 *
 *	Result:
 *		None.
 *
 *------------------------------------------------------*
 */

static void
ResultAdd (r, buf, toWrite)
    ResultBuffer*  r;       /* The buffer to extend */
    unsigned char* buf;     /* The buffer to read from */
    int		   toWrite; /* The number of bytes in 'buf' */
{
    if ((r->used + toWrite) > r->allocated) {
        /* Extension of the internal buffer is required.
	 */

        if (r->allocated == 0) {
	    r->allocated = toWrite + INCREMENT;
	    r->buf       = UCHARP (ckalloc((unsigned) r->allocated));
	} else {
	    r->allocated += toWrite + INCREMENT;
	    r->buf        = UCHARP (ckrealloc((char*) r->buf,
		    (unsigned) r->allocated));
	}
    }

    /* now copy data */
    memcpy(r->buf + r->used, buf, (size_t) toWrite);
    r->used += toWrite;
}
