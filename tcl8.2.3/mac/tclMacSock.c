/* 
 * tclMacSock.c
 *
 *	Channel drivers for Macintosh sockets.
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
#include <AddressXlation.h>
#include <Aliases.h>
#undef Status
#include <Devices.h>
#include <Errors.h>
#include <Events.h>
#include <Files.h>
#include <Gestalt.h>
#include <MacTCP.h>
#include <Processes.h>
#include <Strings.h>

/*
 * The following variable is used to tell whether this module has been
 * initialized.
 */

static int initialized = 0;

/*
 * If debugging is on we may drop into the debugger to handle certain cases
 * that are not supposed to happen.  Otherwise, we change ignore the error
 * and most code should handle such errors ok.
 */

#ifndef TCL_DEBUG
    #define Debugger()
#endif

/*
 * The preferred buffer size for Macintosh channels.
 */

#define CHANNEL_BUF_SIZE	8192

/*
 * Port information structure.  Used to match service names
 * to a Tcp/Ip port number.
 */

typedef struct {
    char *name;			/* Name of service. */
    int port;			/* Port number. */
} PortInfo;

/*
 * This structure describes per-instance state of a tcp based channel.
 */

typedef struct TcpState {
    TCPiopb pb;			   /* Parameter block used by this stream. 
				    * This must be in the first position. */
    ProcessSerialNumber	psn;	   /* PSN used to wake up process. */
    StreamPtr tcpStream;	   /* Macintosh tcp stream pointer. */
    int port;			   /* The port we are connected to. */
    int flags;			   /* Bit field comprised of the flags
				    * described below.  */
    int checkMask;		   /* OR'ed combination of TCL_READABLE and
				    * TCL_WRITABLE as set by an asynchronous
				    * event handler. */
    int watchMask;		   /* OR'ed combination of TCL_READABLE and
				    * TCL_WRITABLE as set by Tcl_WatchFile. */
    Tcl_TcpAcceptProc *acceptProc; /* Proc to call on accept. */
    ClientData acceptProcData;	   /* The data for the accept proc. */
    wdsEntry dataSegment[2];       /* List of buffers to be written async. */
    rdsEntry rdsarray[5+1];	   /* Array used when cleaning out recieve 
				    * buffers on a closing socket. */
    Tcl_Channel channel;	   /* Channel associated with this socket. */
    int writeBufferSize;           /* Size of buffer to hold data for
                                    *  asynchronous writes. */
    void *writeBuffer;             /* Buffer for async write data. */
    struct TcpState *nextPtr;	   /* The next socket on the global socket
				    * list. */
} TcpState;

/*
 * This structure is used by domain name resolver callback.
 */

typedef struct DNRState {
    struct hostInfo hostInfo;	/* Data structure used by DNR functions. */
    int done;			/* Flag to determine when we are done. */
    ProcessSerialNumber psn;	/* Process to wake up when we are done. */
} DNRState;

/*
 * The following macros may be used to set the flags field of
 * a TcpState structure.
 */

#define TCP_ASYNC_SOCKET	(1<<0)  /* The socket is in async mode. */
#define TCP_ASYNC_CONNECT	(1<<1)  /* The socket is trying to connect. */
#define TCP_CONNECTED		(1<<2)  /* The socket is connected. */
#define TCP_PENDING		(1<<3)	/* A SocketEvent is on the queue. */
#define TCP_LISTENING 		(1<<4)  /* This socket is listening for
					 * a connection. */
#define TCP_LISTEN_CONNECT 	(1<<5)  /* Someone has connect to the
					 * listening port. */
#define TCP_REMOTE_CLOSED 	(1<<6)  /* The remote side has closed
					 * the connection. */
#define TCP_RELEASE	 	(1<<7)  /* The socket may now be released. */
#define TCP_WRITING		(1<<8)  /* A background write is in progress. */
#define TCP_SERVER_ZOMBIE	(1<<9)  /* The server can no longer accept connects. */

/*
 * The following structure is what is added to the Tcl event queue when
 * a socket event occurs.
 */

typedef struct SocketEvent {
    Tcl_Event header;		/* Information that is standard for
				 * all events. */
    TcpState *statePtr;		/* Socket descriptor that is ready. */
    StreamPtr tcpStream;	/* Low level Macintosh stream. */
} SocketEvent;

/*
 * Static routines for this file:
 */

static pascal void	CleanUpExitProc _ANSI_ARGS_((void));
static void		ClearZombieSockets _ANSI_ARGS_((void));
static void		CloseCompletionRoutine _ANSI_ARGS_((TCPiopb *pb));
static TcpState *	CreateSocket _ANSI_ARGS_((Tcl_Interp *interp,
			    int port, char *host, char *myAddr,  int myPort,
			    int server, int async));
static pascal void	DNRCompletionRoutine _ANSI_ARGS_((
			    struct hostInfo *hostinfoPtr,
			    DNRState *dnrStatePtr));
static void		FreeSocketInfo _ANSI_ARGS_((TcpState *statePtr));
static long		GetBufferSize _ANSI_ARGS_((void));
static OSErr		GetHostFromString _ANSI_ARGS_((char *name,
			    ip_addr *address));
static OSErr		GetLocalAddress _ANSI_ARGS_((unsigned long *addr));
static void		IOCompletionRoutine _ANSI_ARGS_((TCPiopb *pb));
static void		InitMacTCPParamBlock _ANSI_ARGS_((TCPiopb *pBlock,
			    int csCode));
static void		InitSockets _ANSI_ARGS_((void));
static TcpState *	NewSocketInfo _ANSI_ARGS_((StreamPtr stream));
static OSErr		ResolveAddress _ANSI_ARGS_((ip_addr tcpAddress,
			    Tcl_DString *dsPtr));
static void		SocketCheckProc _ANSI_ARGS_((ClientData clientData,
			    int flags));
static int		SocketEventProc _ANSI_ARGS_((Tcl_Event *evPtr,
			    int flags));
static void		SocketExitHandler _ANSI_ARGS_((ClientData clientData));
static void		SocketFreeProc _ANSI_ARGS_((ClientData clientData));
static int		SocketReady _ANSI_ARGS_((TcpState *statePtr));
static void		SocketSetupProc _ANSI_ARGS_((ClientData clientData,
			    int flags));
static void		TcpAccept _ANSI_ARGS_((TcpState *statePtr));
static int		TcpBlockMode _ANSI_ARGS_((ClientData instanceData, int mode));
static int		TcpClose _ANSI_ARGS_((ClientData instanceData,
			    Tcl_Interp *interp));
static int		TcpGetHandle _ANSI_ARGS_((ClientData instanceData,
		            int direction, ClientData *handlePtr));
static int		TcpGetOptionProc _ANSI_ARGS_((ClientData instanceData,
                            Tcl_Interp *interp, char *optionName,
			    Tcl_DString *dsPtr));
static int		TcpInput _ANSI_ARGS_((ClientData instanceData,
			    char *buf, int toRead, int *errorCodePtr));
static int		TcpOutput _ANSI_ARGS_((ClientData instanceData,
			    char *buf, int toWrite, int *errorCodePtr));
static void		TcpWatch _ANSI_ARGS_((ClientData instanceData,
		            int mask));
static int		WaitForSocketEvent _ANSI_ARGS_((TcpState *infoPtr,
		            int mask, int *errorCodePtr));

/*
 * This structure describes the channel type structure for TCP socket
 * based IO:
 */

static Tcl_ChannelType tcpChannelType = {
    "tcp",			/* Type name. */
    TcpBlockMode,		/* Set blocking or
                                 * non-blocking mode.*/
    TcpClose,			/* Close proc. */
    TcpInput,			/* Input proc. */
    TcpOutput,			/* Output proc. */
    NULL,			/* Seek proc. */
    NULL,			/* Set option proc. */
    TcpGetOptionProc,		/* Get option proc. */
    TcpWatch,			/* Initialize notifier. */
    TcpGetHandle		/* Get handles out of channel. */
};

/*
 * Universal Procedure Pointers (UPP) for various callback
 * routines used by MacTcp code.
 */

ResultUPP resultUPP = NULL;
TCPIOCompletionUPP completeUPP = NULL;
TCPIOCompletionUPP closeUPP = NULL;

/*
 * Built-in commands, and the procedures associated with them:
 */

static PortInfo portServices[] = {
    {"echo",		7},
    {"discard",		9},
    {"systat",		11},
    {"daytime",		13},
    {"netstat",		15},
    {"chargen",		19},
    {"ftp-data",	20},
    {"ftp",		21},
    {"telnet",		23},
    {"telneto",		24},
    {"smtp",		25},
    {"time",		37},
    {"whois",		43},
    {"domain",		53},
    {"gopher",		70},
    {"finger",		79},
    {"hostnames",	101},
    {"sunrpc",		111},
    {"nntp",		119},
    {"exec",		512},
    {"login",		513},
    {"shell",		514},
    {"printer",		515},
    {"courier",		530},
    {"uucp",		540},
    {NULL,		0},
};

typedef struct ThreadSpecificData {
    /*
     * Every open socket has an entry on the following list.
     */
    
    TcpState *socketList;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * Globals for holding information about OS support for sockets.
 */

static int socketsTestInited = false;
static int hasSockets = false;
static short driverRefNum = 0;
static int socketNumber = 0;
static int socketBufferSize = CHANNEL_BUF_SIZE;
static ProcessSerialNumber applicationPSN;

/*
 *----------------------------------------------------------------------
 *
 * InitSockets --
 *
 *	Load the MacTCP driver and open the name resolver.  We also
 *	create several UPP's used by our code.  Lastly, we install
 *	a patch to ExitToShell to clean up socket connections if
 *	we are about to exit.
 *
 * Results:
 *	1 if successful, 0 on failure.
 *
 * Side effects:
 *	Creates a new event source, loads the MacTCP driver,
 *	registers an exit to shell callback.
 *
 *----------------------------------------------------------------------
 */

#define gestaltMacTCPVersion 'mtcp'
static void
InitSockets()
{
    ParamBlockRec pb; 
    OSErr err;
    long response;
    ThreadSpecificData *tsdPtr;
    
    if (! initialized) {
	/*
	 * Do process wide initialization.
	 */

	initialized = 1;
	    
	if (Gestalt(gestaltMacTCPVersion, &response) == noErr) {
	    hasSockets = true;
	} else {
	    hasSockets = false;
	}
    
	if (!hasSockets) {
	    return;
	}
    
	/*
	 * Load MacTcp driver and name server resolver.
	 */
	    
		    
	pb.ioParam.ioCompletion = 0L; 
	pb.ioParam.ioNamePtr = "\p.IPP"; 
	pb.ioParam.ioPermssn = fsCurPerm; 
	err = PBOpenSync(&pb); 
	if (err != noErr) {
	    hasSockets = 0;
	    return;
	}
	driverRefNum = pb.ioParam.ioRefNum; 
	    
	socketBufferSize = GetBufferSize();
	err = OpenResolver(NULL);
	if (err != noErr) {
	    hasSockets = 0;
	    return;
	}
    
	GetCurrentProcess(&applicationPSN);
	/*
	 * Create UPP's for various callback routines.
	 */
    
	resultUPP = NewResultProc(DNRCompletionRoutine);
	completeUPP = NewTCPIOCompletionProc(IOCompletionRoutine);
	closeUPP = NewTCPIOCompletionProc(CloseCompletionRoutine);
    
	/*
	 * Install an ExitToShell patch.  We use this patch instead
	 * of the Tcl exit mechanism because we need to ensure that
	 * these routines are cleaned up even if we crash or are forced
	 * to quit.  There are some circumstances when the Tcl exit
	 * handlers may not fire.
	 */
    
	TclMacInstallExitToShellPatch(CleanUpExitProc);
    }

    /*
     * Do per-thread initialization.
     */

    tsdPtr = (ThreadSpecificData *)TclThreadDataKeyGet(&dataKey);
    if (tsdPtr == NULL) {
	tsdPtr->socketList = NULL;
	Tcl_CreateEventSource(SocketSetupProc, SocketCheckProc, NULL);
	Tcl_CreateThreadExitHandler(SocketExitHandler, (ClientData) NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SocketExitHandler --
 *
 *	Callback invoked during exit clean up to deinitialize the
 *	socket module.
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
SocketExitHandler(
    ClientData clientData)              /* Not used. */
{
    if (hasSockets) {
	Tcl_DeleteEventSource(SocketSetupProc, SocketCheckProc, NULL);
	/* CleanUpExitProc();
	TclMacDeleteExitToShellPatch(CleanUpExitProc); */
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpHasSockets --
 *
 *	This function determines whether sockets are available on the
 *	current system and returns an error in interp if they are not.
 *	Note that interp may be NULL.
 *
 * Results:
 *	Returns TCL_OK if the system supports sockets, or TCL_ERROR with
 *	an error in interp.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclpHasSockets(
    Tcl_Interp *interp)		/* Interp for error messages. */
{
    InitSockets();

    if (hasSockets) {
	return TCL_OK;
    }
    if (interp != NULL) {
	Tcl_AppendResult(interp, "sockets are not available on this system",
		NULL);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * SocketSetupProc --
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

static void
SocketSetupProc(
    ClientData data,		/* Not used. */
    int flags)			/* Event flags as passed to Tcl_DoOneEvent. */
{
    TcpState *statePtr;
    Tcl_Time blockTime = { 0, 0 };
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return;
    }
    
    /*
     * Check to see if there is a ready socket.  If so, poll.
     */

    for (statePtr = tsdPtr->socketList; statePtr != NULL;
	    statePtr = statePtr->nextPtr) {
	if (statePtr->flags & TCP_RELEASE) {
	    continue;
	}
	if (SocketReady(statePtr)) {
	    Tcl_SetMaxBlockTime(&blockTime);
	    break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SocketCheckProc --
 *
 *	This procedure is called by Tcl_DoOneEvent to check the socket
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
SocketCheckProc(
    ClientData data,		/* Not used. */
    int flags)			/* Event flags as passed to Tcl_DoOneEvent. */
{
    TcpState *statePtr;
    SocketEvent *evPtr;
    TcpState dummyState;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return;
    }
    
    /*
     * Queue events for any ready sockets that don't already have events
     * queued (caused by persistent states that won't generate WinSock
     * events).
     */

    for (statePtr = tsdPtr->socketList; statePtr != NULL;
	    statePtr = statePtr->nextPtr) {
	/*
	 * Check to see if this socket is dead and needs to be cleaned
	 * up.  We use a dummy statePtr whose only valid field is the
	 * nextPtr to allow the loop to continue even if the element
	 * is deleted.
	 */

	if (statePtr->flags & TCP_RELEASE) {
	    if (!(statePtr->flags & TCP_PENDING)) {
		dummyState.nextPtr = statePtr->nextPtr;
		SocketFreeProc(statePtr);
		statePtr = &dummyState;
	    }
	    continue;
	}

	if (!(statePtr->flags & TCP_PENDING) && SocketReady(statePtr)) {
	    statePtr->flags |= TCP_PENDING;
	    evPtr = (SocketEvent *) ckalloc(sizeof(SocketEvent));
	    evPtr->header.proc = SocketEventProc;
	    evPtr->statePtr = statePtr;
	    evPtr->tcpStream = statePtr->tcpStream;
	    Tcl_QueueEvent((Tcl_Event *) evPtr, TCL_QUEUE_TAIL);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SocketReady --
 *
 *	This function checks the current state of a socket to see
 *	if any interesting conditions are present.
 *
 * Results:
 *	Returns 1 if an event that someone is watching is present, else
 *	returns 0.
 *
 * Side effects:
 *	Updates the checkMask for the socket to reflect any newly
 *	detected events.
 *
 *----------------------------------------------------------------------
 */

static int
SocketReady(
    TcpState *statePtr)
{
    TCPiopb statusPB;
    int foundSomething = 0;
    int didStatus = 0;
    int amount;
    OSErr err;

    if (statePtr->flags & TCP_LISTEN_CONNECT) {
	foundSomething = 1;
	statePtr->checkMask |= TCL_READABLE;
    }
    if (statePtr->watchMask & TCL_READABLE) {
	if (statePtr->checkMask & TCL_READABLE) {
	    foundSomething = 1;
	} else if (statePtr->flags & TCP_CONNECTED) {
	    statusPB.ioCRefNum = driverRefNum;
	    statusPB.tcpStream = statePtr->tcpStream;
	    statusPB.csCode = TCPStatus;
	    err = PBControlSync((ParmBlkPtr) &statusPB);
	    didStatus = 1;

	    /*
	     * We make the fchannel readable if 1) we get an error,
	     * 2) there is more data available, or 3) we detect
	     * that a close from the remote connection has arrived.
	     */

	    if ((err != noErr) ||
		    (statusPB.csParam.status.amtUnreadData > 0) ||
		    (statusPB.csParam.status.connectionState == 14)) {
		statePtr->checkMask |= TCL_READABLE;
		foundSomething = 1;
	    }
	}
    }
    if (statePtr->watchMask & TCL_WRITABLE) {
	if (statePtr->checkMask & TCL_WRITABLE) {
	    foundSomething = 1;
	} else if (statePtr->flags & TCP_CONNECTED) {
	    if (!didStatus) {
		statusPB.ioCRefNum = driverRefNum;
		statusPB.tcpStream = statePtr->tcpStream;
		statusPB.csCode = TCPStatus;
		err = PBControlSync((ParmBlkPtr) &statusPB);
	    }

	    /*
	     * If there is an error or there if there is room to
	     * send more data we make the channel writeable.
	     */

	    amount = statusPB.csParam.status.sendWindow - 
		statusPB.csParam.status.amtUnackedData;
	    if ((err != noErr) || (amount > 0)) {
		statePtr->checkMask |= TCL_WRITABLE;
		foundSomething = 1;
	    }
	}
    }
    return foundSomething;
}

/*
 *----------------------------------------------------------------------
 *
 * InitMacTCPParamBlock--
 *
 *	Initialize a MacTCP parameter block.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the parameter block.
 *
 *----------------------------------------------------------------------
 */

static void
InitMacTCPParamBlock(
    TCPiopb *pBlock,		/* Tcp parmeter block. */
    int csCode)			/* Tcp operation code. */
{
    memset(pBlock, 0, sizeof(TCPiopb));
    pBlock->ioResult = 1;
    pBlock->ioCRefNum = driverRefNum;
    pBlock->csCode = (short) csCode;
}

/*
 *----------------------------------------------------------------------
 *
 * TcpBlockMode --
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
TcpBlockMode(
    ClientData instanceData, 		/* Channel state. */
    int mode)				/* The mode to set. */
{
    TcpState *statePtr = (TcpState *) instanceData;
    
    if (mode == TCL_MODE_BLOCKING) {
	statePtr->flags &= ~TCP_ASYNC_SOCKET;
    } else {
	statePtr->flags |= TCP_ASYNC_SOCKET;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TcpClose --
 *
 *	Close the socket.
 *
 * Results:
 *	0 if successful, the value of errno if failed.
 *
 * Side effects:
 *	Closes the socket.
 *
 *----------------------------------------------------------------------
 */

static int
TcpClose(
    ClientData instanceData,		/* The socket to close. */
    Tcl_Interp *interp)			/* Interp for error messages. */
{
    TcpState *statePtr = (TcpState *) instanceData;
    StreamPtr tcpStream;
    TCPiopb closePB;
    OSErr err;

    tcpStream = statePtr->tcpStream;
    statePtr->flags &= ~TCP_CONNECTED;
    
    /*
     * If this is a server socket we can't use the statePtr
     * param block because it is in use.  However, we can 
     * close syncronously.
     */

    if ((statePtr->flags & TCP_LISTENING) ||
	    (statePtr->flags & TCP_LISTEN_CONNECT)) {
	InitMacTCPParamBlock(&closePB, TCPClose);
    	closePB.tcpStream = tcpStream;
    	closePB.ioCompletion = NULL; 
    	err = PBControlSync((ParmBlkPtr) &closePB);
    	if (err != noErr) {
    	    Debugger();
            panic("error closing server socket");
    	}
	statePtr->flags |= TCP_RELEASE;

	/*
	 * Server sockets are closed sync.  Therefor, we know it is OK to
	 * release the socket now.
	 */

	InitMacTCPParamBlock(&statePtr->pb, TCPRelease);
	statePtr->pb.tcpStream = statePtr->tcpStream;
	err = PBControlSync((ParmBlkPtr) &statePtr->pb);
	if (err != noErr) {
            panic("error releasing server socket");
	}

	/*
	 * Free the buffer space used by the socket and the 
	 * actual socket state data structure.
	 */

	ckfree((char *) statePtr->pb.csParam.create.rcvBuff);
	FreeSocketInfo(statePtr);
	return 0;
    }

    /*
     * If this socket is in the midddle on async connect we can just
     * abort the connect and release the stream right now.
     */
 
    if (statePtr->flags & TCP_ASYNC_CONNECT) {
	InitMacTCPParamBlock(&closePB, TCPClose);
    	closePB.tcpStream = tcpStream;
    	closePB.ioCompletion = NULL; 
    	err = PBControlSync((ParmBlkPtr) &closePB);
    	if (err != noErr) {
            panic("error closing async connect socket");
    	}
	statePtr->flags |= TCP_RELEASE;

	InitMacTCPParamBlock(&statePtr->pb, TCPRelease);
	statePtr->pb.tcpStream = statePtr->tcpStream;
	err = PBControlSync((ParmBlkPtr) &statePtr->pb);
	if (err != noErr) {
            panic("error releasing async connect socket");
	}

	/*
	 * Free the buffer space used by the socket and the 
	 * actual socket state data structure.
	 */

	ckfree((char *) statePtr->pb.csParam.create.rcvBuff);
	FreeSocketInfo(statePtr);
	return 0;
    }

    /*
     * Client sockets:
     * If a background write is in progress, don't close
     * the socket yet.  The completion routine for the 
     * write will take care of it.
     */
    
    if (!(statePtr->flags & TCP_WRITING)) {
	InitMacTCPParamBlock(&statePtr->pb, TCPClose);
    	statePtr->pb.tcpStream = tcpStream;
    	statePtr->pb.ioCompletion = closeUPP; 
    	statePtr->pb.csParam.close.userDataPtr = (Ptr) statePtr;
    	err = PBControlAsync((ParmBlkPtr) &statePtr->pb);
    	if (err != noErr) {
	    Debugger();
	    statePtr->flags |= TCP_RELEASE;
            /* return 0; */
    	}
    }

    SocketFreeProc(instanceData);
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * CloseCompletionRoutine --
 *
 *	Handles the close protocol for a Tcp socket.  This will do
 *	a series of calls to release all data currently buffered for
 *	the socket.  This is important to do to as it allows the remote
 *	connection to recieve and issue it's own close on the socket.
 *	Note that this function is running at interupt time and can't
 *	allocate memory or do much else except set state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The buffers for the socket are flushed.
 *
 *----------------------------------------------------------------------
 */

static void
CloseCompletionRoutine(
    TCPiopb *pbPtr)		/* Tcp parameter block. */
{
    TcpState *statePtr;
    OSErr err;
    
    if (pbPtr->csCode == TCPClose) {
	statePtr = (TcpState *) (pbPtr->csParam.close.userDataPtr);
    } else {
	statePtr = (TcpState *) (pbPtr->csParam.receive.userDataPtr);
    }

    /*
     * It's very bad if the statePtr is nNULL - we should probably panic...
     */

    if (statePtr == NULL) {
	Debugger();
	return;
    }
    
    WakeUpProcess(&statePtr->psn);

    /*
     * If there is an error we assume the remote side has already
     * close.  We are done closing as soon as we decide that the
     * remote connection has closed.
     */
    
    if (pbPtr->ioResult != noErr) {
	statePtr->flags |= TCP_RELEASE;
	return;
    }
    if (statePtr->flags & TCP_REMOTE_CLOSED) {
	statePtr->flags |= TCP_RELEASE;
	return;
    }
    
    /*
     * If we just did a recieve we need to return the buffers.
     * Otherwise, attempt to recieve more data until we recieve an
     * error (usually because we have no more data).
     */

    if (statePtr->pb.csCode == TCPNoCopyRcv) {
	InitMacTCPParamBlock(&statePtr->pb, TCPRcvBfrReturn);
    	statePtr->pb.tcpStream = statePtr->tcpStream;
	statePtr->pb.ioCompletion = closeUPP; 
	statePtr->pb.csParam.receive.rdsPtr = (Ptr) statePtr->rdsarray;
    	statePtr->pb.csParam.receive.userDataPtr = (Ptr) statePtr;
	err = PBControlAsync((ParmBlkPtr) &statePtr->pb);
    } else {
	InitMacTCPParamBlock(&statePtr->pb, TCPNoCopyRcv);
    	statePtr->pb.tcpStream = statePtr->tcpStream;
	statePtr->pb.ioCompletion = closeUPP; 
	statePtr->pb.csParam.receive.commandTimeoutValue = 1;
	statePtr->pb.csParam.receive.rdsPtr = (Ptr) statePtr->rdsarray;
	statePtr->pb.csParam.receive.rdsLength = 5;
    	statePtr->pb.csParam.receive.userDataPtr = (Ptr) statePtr;
	err = PBControlAsync((ParmBlkPtr) &statePtr->pb);
    }

    if (err != noErr) {
	statePtr->flags |= TCP_RELEASE;
    }
}
/*
 *----------------------------------------------------------------------
 *
 * SocketFreeProc --
 *
 *      This callback is invoked in order to delete
 *      the notifier data associated with a file handle.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Removes the SocketInfo from the global socket list.
 *
 *----------------------------------------------------------------------
 */

static void
SocketFreeProc(
    ClientData clientData)      /* Channel state. */
{
    TcpState *statePtr = (TcpState *) clientData;
    OSErr err;
    TCPiopb statusPB;

    /*
     * Get the status of this connection.  We need to do a
     * few tests to see if it's OK to release the stream now.
     */

    if (!(statePtr->flags & TCP_RELEASE)) {
	return;
    }
    statusPB.ioCRefNum = driverRefNum;
    statusPB.tcpStream = statePtr->tcpStream;
    statusPB.csCode = TCPStatus;
    err = PBControlSync((ParmBlkPtr) &statusPB);
    if ((statusPB.csParam.status.connectionState == 0) ||
	(statusPB.csParam.status.connectionState == 2)) {
	/*
	 * If the conection state is 0 then this was a client
	 * connection and it's closed.  If it is 2 then this a
	 * server client and we may release it.  If it isn't
	 * one of those values then we return and we'll try to
	 * clean up later.
	 */

    } else {
	return;
    }
    
    /*
     * The Close request is made async.  We know it's
     * OK to release the socket when the TCP_RELEASE flag
     * gets set.
     */

    InitMacTCPParamBlock(&statePtr->pb, TCPRelease);
    statePtr->pb.tcpStream = statePtr->tcpStream;
    err = PBControlSync((ParmBlkPtr) &statePtr->pb);
    if (err != noErr) {
        Debugger(); /* Ignoreing leaves stranded stream.  Is there an
		       alternative?  */
    }

    /*
     * Free the buffer space used by the socket and the 
     * actual socket state data structure.
     */

    ckfree((char *) statePtr->pb.csParam.create.rcvBuff);
    FreeSocketInfo(statePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TcpInput --
 *
 *	Reads input from the IO channel into the buffer given. Returns
 *	count of how many bytes were actually read, and an error 
 *	indication.
 *
 * Results:
 *	A count of how many bytes were read is returned.  A value of -1
 *	implies an error occured.  A value of zero means we have reached
 *	the end of data (EOF).
 *
 * Side effects:
 *	Reads input from the actual channel.
 *
 *----------------------------------------------------------------------
 */

int
TcpInput(
    ClientData instanceData,		/* Channel state. */
    char *buf, 				/* Where to store data read. */
    int bufSize, 			/* How much space is available
                                         * in the buffer? */
    int *errorCodePtr)			/* Where to store error code. */
{
    TcpState *statePtr = (TcpState *) instanceData;
    StreamPtr tcpStream;
    OSErr err;
    TCPiopb statusPB;
    int toRead, dataAvail;

    *errorCodePtr = 0;
    errno = 0;
    tcpStream = statePtr->tcpStream;

    if (bufSize == 0) {
        return 0;
    }
    toRead = bufSize;

    /*
     * First check to see if EOF was already detected, to prevent
     * calling the socket stack after the first time EOF is detected.
     */

    if (statePtr->flags & TCP_REMOTE_CLOSED) {
	return 0;
    }

    /*
     * If an asynchronous connect is in progress, attempt to wait for it
     * to complete before reading.
     */
    
    if ((statePtr->flags & TCP_ASYNC_CONNECT)
	    && ! WaitForSocketEvent(statePtr, TCL_READABLE, errorCodePtr)) {
	return -1;
    }

    /*
     * No EOF, and it is connected, so try to read more from the socket.
     * If the socket is blocking, we keep trying until there is data
     * available or the socket is closed.
     */

    while (1) {

	statusPB.ioCRefNum = driverRefNum;
	statusPB.tcpStream = tcpStream;
	statusPB.csCode = TCPStatus;
	err = PBControlSync((ParmBlkPtr) &statusPB);
	if (err != noErr) {
	    Debugger();
	    statePtr->flags |= TCP_REMOTE_CLOSED;
	    return 0;	/* EOF */
	}
	dataAvail = statusPB.csParam.status.amtUnreadData;
	if (dataAvail < bufSize) {
	    toRead = dataAvail;
	} else {
	    toRead = bufSize;
	}
	if (toRead != 0) {
	    /*
	     * Try to read the data.
	     */
	    
	    InitMacTCPParamBlock(&statusPB, TCPRcv);
	    statusPB.tcpStream = tcpStream;
	    statusPB.csParam.receive.rcvBuff = buf;
	    statusPB.csParam.receive.rcvBuffLen = toRead;
	    err = PBControlSync((ParmBlkPtr) &statusPB);

	    statePtr->checkMask &= ~TCL_READABLE;
	    switch (err) {
		case noErr:
		    /*
		     * The channel remains readable only if this read succeds
		     * and we had more data then the size of the buffer we were
		     * trying to fill.  Use the info from the call to status to
		     * determine this.
		     */

		    if (dataAvail > bufSize) {
			statePtr->checkMask |= TCL_READABLE;
		    }
		    return statusPB.csParam.receive.rcvBuffLen;
		case connectionClosing:
		    *errorCodePtr = errno = ESHUTDOWN;
		    statePtr->flags |= TCP_REMOTE_CLOSED;
		    return 0;
		case connectionDoesntExist:
		case connectionTerminated:
		    *errorCodePtr = errno = ENOTCONN;
		    statePtr->flags |= TCP_REMOTE_CLOSED;
		    return 0;
		case invalidStreamPtr:
		default:
		    *errorCodePtr = EINVAL;
		    return -1;
	    }
	}

	/*
	 * No data is available, so check the connection state to
	 * see why this is the case.  
	 */

	if (statusPB.csParam.status.connectionState == 14) {
	    statePtr->flags |= TCP_REMOTE_CLOSED;
	    return 0;
	}
	if (statusPB.csParam.status.connectionState != 8) {
	    Debugger();
	}
	statePtr->checkMask &= ~TCL_READABLE;
	if (statePtr->flags & TCP_ASYNC_SOCKET) {
	    *errorCodePtr = EWOULDBLOCK;
	    return -1;
	}

	/*
	 * In the blocking case, wait until the file becomes readable
	 * or closed and try again.
	 */

	if (!WaitForSocketEvent(statePtr, TCL_READABLE, errorCodePtr)) {
	    return -1;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TcpGetHandle --
 *
 *	Called from Tcl_GetChannelHandle to retrieve handles from inside
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
TcpGetHandle(
    ClientData instanceData,		/* The file state. */
    int direction,			/* Which handle to retrieve? */
    ClientData *handlePtr)
{
    TcpState *statePtr = (TcpState *) instanceData;

    *handlePtr = (ClientData) statePtr->tcpStream;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TcpOutput--
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
TcpOutput(
    ClientData instanceData, 		/* Channel state. */
    char *buf, 				/* The data buffer. */
    int toWrite, 			/* How many bytes to write? */
    int *errorCodePtr)			/* Where to store error code. */
{
    TcpState *statePtr = (TcpState *) instanceData;
    StreamPtr tcpStream;
    OSErr err;
    int amount;
    TCPiopb statusPB;

    *errorCodePtr = 0;
    tcpStream = statePtr->tcpStream;

    /*
     * If an asynchronous connect is in progress, attempt to wait for it
     * to complete before writing.
     */
    
    if ((statePtr->flags & TCP_ASYNC_CONNECT)
	    && ! WaitForSocketEvent(statePtr, TCL_WRITABLE, errorCodePtr)) {
	return -1;
    }

    /*
     * Loop until we have written some data, or an error occurs.
     */

    while (1) {
	statusPB.ioCRefNum = driverRefNum;
	statusPB.tcpStream = tcpStream;
	statusPB.csCode = TCPStatus;
	err = PBControlSync((ParmBlkPtr) &statusPB);
	if ((err == connectionDoesntExist) || ((err == noErr) && 
		(statusPB.csParam.status.connectionState == 14))) {
	    /*
	     * The remote connection is gone away.  Report an error
	     * and don't write anything.
	     */

	    *errorCodePtr = errno = EPIPE;
	    return -1;
	} else if (err != noErr) {
	    return -1;
	}
	amount = statusPB.csParam.status.sendWindow
	    - statusPB.csParam.status.amtUnackedData;

	/*
	 * Attempt to write the data to the socket if a background
	 * write isn't in progress and there is room in the output buffers.
	 */

	if (!(statePtr->flags & TCP_WRITING) && amount > 0) {
	    if (toWrite < amount) {
		amount = toWrite;
	    }

            /* We need to copy the data, otherwise the caller may overwrite
             * the buffer in the middle of our asynchronous call
             */
             
            if (amount > statePtr->writeBufferSize) {
                /* 
                 * need to grow write buffer 
                 */
                 
                if (statePtr->writeBuffer != (void *) NULL) {
                    ckfree(statePtr->writeBuffer);
                }
                statePtr->writeBuffer = (void *) ckalloc(amount);
                statePtr->writeBufferSize = amount;
            }
            memcpy(statePtr->writeBuffer, buf, amount);
            statePtr->dataSegment[0].ptr = statePtr->writeBuffer;

	    statePtr->dataSegment[0].length = amount;
	    statePtr->dataSegment[1].length = 0;
	    InitMacTCPParamBlock(&statePtr->pb, TCPSend);
	    statePtr->pb.ioCompletion = completeUPP;
	    statePtr->pb.tcpStream = tcpStream;
	    statePtr->pb.csParam.send.wdsPtr = (Ptr) statePtr->dataSegment;
	    statePtr->pb.csParam.send.pushFlag = 1;
	    statePtr->pb.csParam.send.userDataPtr = (Ptr) statePtr;
	    statePtr->flags |= TCP_WRITING;
	    err = PBControlAsync((ParmBlkPtr) &(statePtr->pb));
	    switch (err) {
		case noErr:
		    return amount;
		case connectionClosing:
		    *errorCodePtr = errno = ESHUTDOWN;
		    statePtr->flags |= TCP_REMOTE_CLOSED;
		    return -1;
		case connectionDoesntExist:
		case connectionTerminated:
		    *errorCodePtr = errno = ENOTCONN;
		    statePtr->flags |= TCP_REMOTE_CLOSED;
		    return -1;
		case invalidStreamPtr:
		default:
		    return -1;
	    }

	}

	/*
	 * The socket wasn't writable.  In the non-blocking case, return
	 * immediately, otherwise wait  until the file becomes writable
	 * or closed and try again.
	 */

	if (statePtr->flags & TCP_ASYNC_SOCKET) {
	    statePtr->checkMask &= ~TCL_WRITABLE;
	    *errorCodePtr = EWOULDBLOCK;
	    return -1;
	} else if (!WaitForSocketEvent(statePtr, TCL_WRITABLE, errorCodePtr)) {
	    return -1;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TcpGetOptionProc --
 *
 *	Computes an option value for a TCP socket based channel, or a
 *	list of all options and their values.
 *
 *	Note: This code is based on code contributed by John Haxby.
 *
 * Results:
 *	A standard Tcl result. The value of the specified option or a
 *	list of all options and	their values is returned in the
 *	supplied DString.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TcpGetOptionProc(
    ClientData instanceData, 		/* Socket state. */
    Tcl_Interp *interp,                 /* For error reporting - can be NULL.*/
    char *optionName, 			/* Name of the option to
                                         * retrieve the value for, or
                                         * NULL to get all options and
                                         * their values. */
    Tcl_DString *dsPtr)			/* Where to store the computed
                                         * value; initialized by caller. */
{
    TcpState *statePtr = (TcpState *) instanceData;
    int doPeerName = false, doSockName = false, doAll = false;
    ip_addr tcpAddress;
    char buffer[128];
    OSErr err;
    Tcl_DString dString;
    TCPiopb statusPB;
    int errorCode;

    /*
     * If an asynchronous connect is in progress, attempt to wait for it
     * to complete before accessing the socket state.
     */
    
    if ((statePtr->flags & TCP_ASYNC_CONNECT)
	    && ! WaitForSocketEvent(statePtr, TCL_WRITABLE, &errorCode)) {
	if (interp) {
	    /*
	     * fix the error message.
	     */

	    Tcl_AppendResult(interp, "connect is in progress and can't wait",
	    		NULL);
	}
	return TCL_ERROR;
    }
            
    /*
     * Determine which options we need to do.  Do all of them
     * if optionName is NULL.
     */

    if (optionName == (char *) NULL || optionName[0] == '\0') {
        doAll = true;
    } else {
	if (!strcmp(optionName, "-peername")) {
	    doPeerName = true;
	} else if (!strcmp(optionName, "-sockname")) {
	    doSockName = true;
	} else {
	    return Tcl_BadChannelOption(interp, optionName, 
	    		"peername sockname");
	}
    }

    /*
     * Get status on the stream.  Make sure to use a new pb struct because
     * the struct in the statePtr may be part of an asyncronous call.
     */

    statusPB.ioCRefNum = driverRefNum;
    statusPB.tcpStream = statePtr->tcpStream;
    statusPB.csCode = TCPStatus;
    err = PBControlSync((ParmBlkPtr) &statusPB);
    if ((err == connectionDoesntExist) ||
	((err == noErr) && (statusPB.csParam.status.connectionState == 14))) {
	/*
	 * The socket was probably closed on the other side of the connection.
	 */

	if (interp) {
	    Tcl_AppendResult(interp, "can't access socket info: ",
			     "connection reset by peer", NULL);
	}
	return TCL_ERROR;
    } else if (err != noErr) {
	if (interp) { 
	    Tcl_AppendResult(interp, "unknown socket error", NULL);
	}
	Debugger();
	return TCL_ERROR;
    }


    /*
     * Get the sockname for the socket.
     */

    Tcl_DStringInit(&dString);
    if (doAll || doSockName) {
	if (doAll) {
	    Tcl_DStringAppendElement(dsPtr, "-sockname");
	    Tcl_DStringStartSublist(dsPtr);
	}
	tcpAddress = statusPB.csParam.status.localHost;
	sprintf(buffer, "%d.%d.%d.%d", tcpAddress>>24,
		tcpAddress>>16 & 0xff, tcpAddress>>8 & 0xff,
		tcpAddress & 0xff);
	Tcl_DStringAppendElement(dsPtr, buffer);
	if (ResolveAddress(tcpAddress, &dString) == noErr) {
	    Tcl_DStringAppendElement(dsPtr, dString.string);
	} else {
	    Tcl_DStringAppendElement(dsPtr, "<unknown>");
	}
	sprintf(buffer, "%d", statusPB.csParam.status.localPort);
	Tcl_DStringAppendElement(dsPtr, buffer);
	if (doAll) {
	    Tcl_DStringEndSublist(dsPtr);
	}
    }

    /*
     * Get the peername for the socket.
     */

    if ((doAll || doPeerName) && (statePtr->flags & TCP_CONNECTED)) {
	if (doAll) {
	    Tcl_DStringAppendElement(dsPtr, "-peername");
	    Tcl_DStringStartSublist(dsPtr);
	}
	tcpAddress = statusPB.csParam.status.remoteHost;
	sprintf(buffer, "%d.%d.%d.%d", tcpAddress>>24,
		tcpAddress>>16 & 0xff, tcpAddress>>8 & 0xff,
		tcpAddress & 0xff);
	Tcl_DStringAppendElement(dsPtr, buffer);
	Tcl_DStringSetLength(&dString, 0);
	if (ResolveAddress(tcpAddress, &dString) == noErr) {
	    Tcl_DStringAppendElement(dsPtr, dString.string);
	} else {
	    Tcl_DStringAppendElement(dsPtr, "<unknown>");
	}
	sprintf(buffer, "%d", statusPB.csParam.status.remotePort);
	Tcl_DStringAppendElement(dsPtr, buffer);
	if (doAll) {
	    Tcl_DStringEndSublist(dsPtr);
	}
    }

    Tcl_DStringFree(&dString);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TcpWatch --
 *
 *	Initialize the notifier to watch this channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the watchMask for the channel.
 *
 *----------------------------------------------------------------------
 */

static void
TcpWatch(instanceData, mask)
    ClientData instanceData;		/* The file state. */
    int mask;				/* Events of interest; an OR-ed
                                         * combination of TCL_READABLE,
                                         * TCL_WRITABLE and TCL_EXCEPTION. */
{
    TcpState *statePtr = (TcpState *) instanceData;

    statePtr->watchMask = mask;
}

/*
 *----------------------------------------------------------------------
 *
 * NewSocketInfo --
 *
 *	This function allocates and initializes a new SocketInfo
 *	structure.
 *
 * Results:
 *	Returns a newly allocated SocketInfo.
 *
 * Side effects:
 *	Adds the socket to the global socket list, allocates memory.
 *
 *----------------------------------------------------------------------
 */

static TcpState *
NewSocketInfo(
    StreamPtr tcpStream)
{
    TcpState *statePtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    statePtr = (TcpState *) ckalloc((unsigned) sizeof(TcpState));
    statePtr->tcpStream = tcpStream;
    statePtr->psn = applicationPSN;
    statePtr->flags = 0;
    statePtr->checkMask = 0;
    statePtr->watchMask = 0;
    statePtr->acceptProc = (Tcl_TcpAcceptProc *) NULL;
    statePtr->acceptProcData = (ClientData) NULL;
    statePtr->writeBuffer = (void *) NULL;
    statePtr->writeBufferSize = 0;
    statePtr->nextPtr = tsdPtr->socketList;
    tsdPtr->socketList = statePtr;
    return statePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeSocketInfo --
 *
 *	This function deallocates a SocketInfo structure that is no
 *	longer needed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the socket from the global socket list, frees memory.
 *
 *----------------------------------------------------------------------
 */

static void
FreeSocketInfo(
    TcpState *statePtr)		/* The state pointer to free. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (statePtr == tsdPtr->socketList) {
	tsdPtr->socketList = statePtr->nextPtr;
    } else {
	TcpState *p;
	for (p = tsdPtr->socketList; p != NULL; p = p->nextPtr) {
	    if (p->nextPtr == statePtr) {
		p->nextPtr = statePtr->nextPtr;
		break;
	    }
	}
    }
    
    if (statePtr->writeBuffer != (void *) NULL) {
        ckfree(statePtr->writeBuffer);
    }
    
    ckfree((char *) statePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MakeTcpClientChannel --
 *
 *	Creates a Tcl_Channel from an existing client TCP socket.
 *
 * Results:
 *	The Tcl_Channel wrapped around the preexisting TCP socket.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_MakeTcpClientChannel(
    ClientData sock)	/* The socket to wrap up into a channel. */
{
    TcpState *statePtr;
    char channelName[20];

    if (TclpHasSockets(NULL) != TCL_OK) {
	return NULL;
    }
	
    statePtr = NewSocketInfo((StreamPtr) sock);
    /* TODO: do we need to set the port??? */
    
    sprintf(channelName, "sock%d", socketNumber++);
    
    statePtr->channel = Tcl_CreateChannel(&tcpChannelType, channelName,
            (ClientData) statePtr, (TCL_READABLE | TCL_WRITABLE));
    Tcl_SetChannelBufferSize(statePtr->channel, socketBufferSize);
    Tcl_SetChannelOption(NULL, statePtr->channel, "-translation", "auto crlf");
    return statePtr->channel;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateSocket --
 *
 *	This function opens a new socket and initializes the
 *	SocketInfo structure.
 *
 * Results:
 *	Returns a new SocketInfo, or NULL with an error in interp.
 *
 * Side effects:
 *	Adds a new socket to the socketList.
 *
 *----------------------------------------------------------------------
 */

static TcpState *
CreateSocket(
    Tcl_Interp *interp,		/* For error reporting; can be NULL. */
    int port,			/* Port number to open. */
    char *host,			/* Name of host on which to open port. */
    char *myaddr,		/* Optional client-side address */
    int myport,			/* Optional client-side port */
    int server,			/* 1 if socket should be a server socket,
				 * else 0 for a client socket. */
    int async)			/* 1 create async, 0 do sync. */
{
    ip_addr macAddr;
    OSErr err;
    TCPiopb pb;
    StreamPtr tcpStream;
    TcpState *statePtr;
    char * buffer;
    
    /*
     * Figure out the ip address from the host string.
     */

    if (host == NULL) {
	err = GetLocalAddress(&macAddr);
    } else {
	err = GetHostFromString(host, &macAddr);
    }
    if (err != noErr) {
	Tcl_SetErrno(EHOSTUNREACH);
	if (interp != (Tcl_Interp *) NULL) {
	    Tcl_AppendResult(interp, "couldn't open socket: ",
                        Tcl_PosixError(interp), (char *) NULL);
	}
	return (TcpState *) NULL;
    }
    
    /*
     * Create a MacTCP stream and create the state used for socket
     * transactions from here on out.
     */

    ClearZombieSockets();
    buffer = ckalloc(socketBufferSize);
    InitMacTCPParamBlock(&pb, TCPCreate);
    pb.csParam.create.rcvBuff = buffer;
    pb.csParam.create.rcvBuffLen = socketBufferSize;
    err = PBControlSync((ParmBlkPtr) &pb);
    if (err != noErr) {
        Tcl_SetErrno(0); /* TODO: set to ENOSR - maybe?*/
        if (interp != (Tcl_Interp *) NULL) {
	    Tcl_AppendResult(interp, "couldn't open socket: ",
		Tcl_PosixError(interp), (char *) NULL);
        }
	return (TcpState *) NULL;
    }

    tcpStream = pb.tcpStream;
    statePtr = NewSocketInfo(tcpStream);
    statePtr->port = port;
    
    if (server) {
        /* 
         * Set up server connection.
         */

	InitMacTCPParamBlock(&statePtr->pb, TCPPassiveOpen);
	statePtr->pb.tcpStream = tcpStream;
	statePtr->pb.csParam.open.localPort = statePtr->port;
	statePtr->pb.ioCompletion = completeUPP; 
	statePtr->pb.csParam.open.userDataPtr = (Ptr) statePtr;
	statePtr->flags |= TCP_LISTENING;
	err = PBControlAsync((ParmBlkPtr) &(statePtr->pb));

	/*
	 * If this is a server on port 0 then we need to wait until
	 * the dynamic port allocation is made by the MacTcp driver.
	 */

	if (statePtr->port == 0) {
	    EventRecord dummy;

	    while (statePtr->pb.csParam.open.localPort == 0) {
		WaitNextEvent(0, &dummy, 1, NULL);
		if (statePtr->pb.ioResult != 0) {
		    break;
		}
	    }
	    statePtr->port = statePtr->pb.csParam.open.localPort;
	}
	Tcl_SetErrno(EINPROGRESS);
    } else {
	/*
	 * Attempt to connect. The connect may fail at present with an
	 * EINPROGRESS but at a later time it will complete. The caller
	 * will set up a file handler on the socket if she is interested in
	 * being informed when the connect completes.
	 */

	InitMacTCPParamBlock(&statePtr->pb, TCPActiveOpen);
	statePtr->pb.tcpStream = tcpStream;
	statePtr->pb.csParam.open.remoteHost = macAddr;
	statePtr->pb.csParam.open.remotePort = port;
	statePtr->pb.csParam.open.localHost = 0;
	statePtr->pb.csParam.open.localPort = myport;
	statePtr->pb.csParam.open.userDataPtr = (Ptr) statePtr;
	statePtr->pb.ioCompletion = completeUPP;
	if (async) {
	    statePtr->flags |= TCP_ASYNC_CONNECT;
	    err = PBControlAsync((ParmBlkPtr) &(statePtr->pb));
	    Tcl_SetErrno(EINPROGRESS);
	} else {
	    err = PBControlSync((ParmBlkPtr) &(statePtr->pb));
	}
    }
    
    switch (err) {
	case noErr:
	    if (!async) {
		statePtr->flags |= TCP_CONNECTED;
	    }
	    return statePtr;
	case duplicateSocket:
	    Tcl_SetErrno(EADDRINUSE);
	    break;
	case openFailed:
	case connectionTerminated:
	    Tcl_SetErrno(ECONNREFUSED);
	    break;
	case invalidStreamPtr:
	case connectionExists:
	default:
	    /*
	     * These cases should never occur.  However, we will fail
	     * gracefully and hope Tcl can resume.  The alternative is to panic
	     * which is probably a bit drastic.
	     */

	    Debugger();
	    Tcl_SetErrno(err);
    }

    /*
     * We had error during the connection.  Release the stream
     * and file handle.  Also report to the interp.
     */

    pb.ioCRefNum = driverRefNum;
    pb.csCode = TCPRelease;
    pb.tcpStream = tcpStream;
    pb.ioCompletion = NULL; 
    err = PBControlSync((ParmBlkPtr) &pb);

    if (interp != (Tcl_Interp *) NULL) {
	Tcl_AppendResult(interp, "couldn't open socket: ",
	    Tcl_PosixError(interp), (char *) NULL);
    }

    ckfree(buffer);
    FreeSocketInfo(statePtr);
    return (TcpState *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_OpenTcpClient --
 *
 *	Opens a TCP client socket and creates a channel around it.
 *
 * Results:
 *	The channel or NULL if failed. On failure, the routine also
 *	sets the output argument errorCodePtr to the error code.
 *
 * Side effects:
 *	Opens a client socket and creates a new channel.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_OpenTcpClient(
    Tcl_Interp *interp, 		/* For error reporting; can be NULL. */
    int port, 				/* Port number to open. */
    char *host, 			/* Host on which to open port. */
    char *myaddr, 			/* Client-side address */
    int myport, 			/* Client-side port */
    int async)				/* If nonzero, attempt to do an
                                         * asynchronous connect. Otherwise
                                         * we do a blocking connect. 
                                         * - currently ignored */
{
    TcpState *statePtr;
    char channelName[20];

    if (TclpHasSockets(interp) != TCL_OK) {
	return NULL;
    }
	
    /*
     * Create a new client socket and wrap it in a channel.
     */

    statePtr = CreateSocket(interp, port, host, myaddr, myport, 0, async);
    if (statePtr == NULL) {
	return NULL;
    }
    
    sprintf(channelName, "sock%d", socketNumber++);

    statePtr->channel = Tcl_CreateChannel(&tcpChannelType, channelName,
            (ClientData) statePtr, (TCL_READABLE | TCL_WRITABLE));
    Tcl_SetChannelBufferSize(statePtr->channel, socketBufferSize);
    Tcl_SetChannelOption(NULL, statePtr->channel, "-translation", "auto crlf");
    return statePtr->channel;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_OpenTcpServer --
 *
 *	Opens a TCP server socket and creates a channel around it.
 *
 * Results:
 *	The channel or NULL if failed.
 *
 * Side effects:
 *	Opens a server socket and creates a new channel.
 *
 *----------------------------------------------------------------------
 */

Tcl_Channel
Tcl_OpenTcpServer(
    Tcl_Interp *interp,			/* For error reporting - may be
                                         * NULL. */
    int port,				/* Port number to open. */
    char *host,				/* Name of local host. */
    Tcl_TcpAcceptProc *acceptProc,	/* Callback for accepting connections
                                         * from new clients. */
    ClientData acceptProcData)		/* Data for the callback. */
{
    TcpState *statePtr;
    char channelName[20];

    if (TclpHasSockets(interp) != TCL_OK) {
	return NULL;
    }

    /*
     * Create a new client socket and wrap it in a channel.
     */

    statePtr = CreateSocket(interp, port, host, NULL, 0, 1, 1);
    if (statePtr == NULL) {
	return NULL;
    }

    statePtr->acceptProc = acceptProc;
    statePtr->acceptProcData = acceptProcData;

    sprintf(channelName, "sock%d", socketNumber++);

    statePtr->channel = Tcl_CreateChannel(&tcpChannelType, channelName,
            (ClientData) statePtr, 0);
    Tcl_SetChannelBufferSize(statePtr->channel, socketBufferSize);
    Tcl_SetChannelOption(NULL, statePtr->channel, "-translation", "auto crlf");
    return statePtr->channel;
}

/*
 *----------------------------------------------------------------------
 *
 * SocketEventProc --
 *
 *	This procedure is called by Tcl_ServiceEvent when a socket event
 *	reaches the front of the event queue.  This procedure is
 *	responsible for notifying the generic channel code.
 *
 * Results:
 *	Returns 1 if the event was handled, meaning it should be removed
 *	from the queue.  Returns 0 if the event was not handled, meaning
 *	it should stay on the queue.  The only time the event isn't
 *	handled is if the TCL_FILE_EVENTS flag bit isn't set.
 *
 * Side effects:
 *	Whatever the channel callback procedures do.
 *
 *----------------------------------------------------------------------
 */

static int
SocketEventProc(
    Tcl_Event *evPtr,		/* Event to service. */
    int flags)			/* Flags that indicate what events to
				 * handle, such as TCL_FILE_EVENTS. */
{
    TcpState *statePtr;
    SocketEvent *eventPtr = (SocketEvent *) evPtr;
    int mask = 0;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    if (!(flags & TCL_FILE_EVENTS)) {
	return 0;
    }

    /*
     * Find the specified socket on the socket list.
     */

    for (statePtr = tsdPtr->socketList; statePtr != NULL;
	    statePtr = statePtr->nextPtr) {
	if ((statePtr == eventPtr->statePtr) && 
		(statePtr->tcpStream == eventPtr->tcpStream)) {
	    break;
	}
    }

    /*
     * Discard events that have gone stale.
     */

    if (!statePtr) {
	return 1;
    }
    statePtr->flags &= ~(TCP_PENDING);
    if (statePtr->flags & TCP_RELEASE) {
	SocketFreeProc(statePtr);
	return 1;
    }


    /*
     * Handle connection requests directly.
     */

    if (statePtr->flags & TCP_LISTEN_CONNECT) {
	if (statePtr->checkMask & TCL_READABLE) {
	    TcpAccept(statePtr);
	}
	return 1;
    }

    /*
     * Mask off unwanted events then notify the channel.
     */

    mask = statePtr->checkMask & statePtr->watchMask;
    if (mask) {
	Tcl_NotifyChannel(statePtr->channel, mask);
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * WaitForSocketEvent --
 *
 *	Waits until one of the specified events occurs on a socket.
 *
 * Results:
 *	Returns 1 on success or 0 on failure, with an error code in
 *	errorCodePtr.
 *
 * Side effects:
 *	Processes socket events off the system queue.
 *
 *----------------------------------------------------------------------
 */

static int
WaitForSocketEvent(
    TcpState *statePtr,		/* Information about this socket. */
    int mask,			/* Events to look for. */
    int *errorCodePtr)		/* Where to store errors? */
{
    OSErr err;
    TCPiopb statusPB;
    EventRecord dummy;

    /*
     * Loop until we get the specified condition, unless the socket is
     * asynchronous.
     */
    
    do {
	statusPB.ioCRefNum = driverRefNum;
	statusPB.tcpStream = statePtr->tcpStream;
	statusPB.csCode = TCPStatus;
	err = PBControlSync((ParmBlkPtr) &statusPB);
	if (err != noErr) {
	    statePtr->checkMask |= (TCL_READABLE | TCL_WRITABLE);
	    return 1;
	}
	statePtr->checkMask = 0;
	if (statusPB.csParam.status.amtUnreadData > 0) {
	    statePtr->checkMask |= TCL_READABLE;
	}
	if (!(statePtr->flags & TCP_WRITING)
		&& (statusPB.csParam.status.sendWindow - 
			statusPB.csParam.status.amtUnackedData) > 0) {
	    statePtr->flags &= ~(TCP_ASYNC_CONNECT);
	    statePtr->checkMask |= TCL_WRITABLE;
	}
	if (mask & statePtr->checkMask) {
	    return 1;
	}

	/*
	 * Call the system to let other applications run while we
	 * are waiting for this event to occur.
	 */
	
	WaitNextEvent(0, &dummy, 1, NULL);
    } while (!(statePtr->flags & TCP_ASYNC_SOCKET));
    *errorCodePtr = EWOULDBLOCK;
    return 0;
} 

/*
 *----------------------------------------------------------------------
 *
 * TcpAccept --
 *	Accept a TCP socket connection.  This is called by the event 
 *	loop, and it in turns calls any registered callbacks for this
 *	channel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Evals the Tcl script associated with the server socket.
 *
 *----------------------------------------------------------------------
 */

static void
TcpAccept(
    TcpState *statePtr)
{
    TcpState *newStatePtr;
    StreamPtr tcpStream;
    char remoteHostname[255];
    OSErr err;
    ip_addr remoteAddress;
    long remotePort;
    char channelName[20];
    
    statePtr->flags &= ~TCP_LISTEN_CONNECT;
    statePtr->checkMask &= ~TCL_READABLE;

    /*
     * Transfer sever stream to new connection.
     */

    tcpStream = statePtr->tcpStream;
    newStatePtr = NewSocketInfo(tcpStream);
    newStatePtr->tcpStream = tcpStream;
    sprintf(channelName, "sock%d", socketNumber++);


    newStatePtr->flags |= TCP_CONNECTED;
    newStatePtr->channel = Tcl_CreateChannel(&tcpChannelType, channelName,
            (ClientData) newStatePtr, (TCL_READABLE | TCL_WRITABLE));
    Tcl_SetChannelBufferSize(newStatePtr->channel, socketBufferSize);
    Tcl_SetChannelOption(NULL, newStatePtr->channel, "-translation",
	    "auto crlf");

    remoteAddress = statePtr->pb.csParam.open.remoteHost;
    remotePort = statePtr->pb.csParam.open.remotePort;

    /*
     * Reopen passive connect.  Make new tcpStream the server.
     */

    ClearZombieSockets();
    InitMacTCPParamBlock(&statePtr->pb, TCPCreate);
    statePtr->pb.csParam.create.rcvBuff = ckalloc(socketBufferSize);
    statePtr->pb.csParam.create.rcvBuffLen = socketBufferSize;
    err = PBControlSync((ParmBlkPtr) &statePtr->pb);
    if (err != noErr) {
	/* 
	 * Hmmm...  We can't reopen the server.  We'll go ahead
	 * an continue - but we are kind of broken now...
	 */
	 Debugger();
	 statePtr->tcpStream = -1;
	 statePtr->flags |= TCP_SERVER_ZOMBIE;
    }

    tcpStream = statePtr->tcpStream = statePtr->pb.tcpStream;
    
    InitMacTCPParamBlock(&statePtr->pb, TCPPassiveOpen);
    statePtr->pb.tcpStream = tcpStream;
    statePtr->pb.csParam.open.localHost = 0;
    statePtr->pb.csParam.open.localPort = statePtr->port;
    statePtr->pb.ioCompletion = completeUPP; 
    statePtr->pb.csParam.open.userDataPtr = (Ptr) statePtr;
    statePtr->flags |= TCP_LISTENING;
    err = PBControlAsync((ParmBlkPtr) &(statePtr->pb));
    /*
     * TODO: deal with case where we can't recreate server socket...
     */

    /*
     * Finally we run the accept procedure.  We must do this last to make
     * sure we are in a nice clean state.  This Tcl code can do anything
     * including closing the server or client sockets we've just delt with.
     */

    if (statePtr->acceptProc != NULL) {
	sprintf(remoteHostname, "%d.%d.%d.%d", remoteAddress>>24,
		remoteAddress>>16 & 0xff, remoteAddress>>8 & 0xff,
		remoteAddress & 0xff);
		
	(statePtr->acceptProc)(statePtr->acceptProcData, newStatePtr->channel, 
	    remoteHostname, remotePort);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetHostName --
 *
 *	Returns the name of the local host.
 *
 * Results:
 *	A string containing the network name for this machine, or
 *	an empty string if we can't figure out the name.  The caller 
 *	must not modify or free this string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_GetHostName()
{
    static int  hostnameInited = 0;
    static char hostname[255];
    ip_addr ourAddress;
    Tcl_DString dString;
    OSErr err;
    
    if (hostnameInited) {
        return hostname;
    }
    
    if (TclpHasSockets(NULL) == TCL_OK) {
	err = GetLocalAddress(&ourAddress);
	if (err == noErr) {
	    /*
	     * Search for the doman name and return it if found.  Otherwise, 
	     * just print the IP number to a string and return that.
	     */

	    Tcl_DStringInit(&dString);
	    err = ResolveAddress(ourAddress, &dString);
	    if (err == noErr) {
		strcpy(hostname, dString.string);
	    } else {
		sprintf(hostname, "%d.%d.%d.%d", ourAddress>>24, ourAddress>>16 & 0xff,
		    ourAddress>>8 & 0xff, ourAddress & 0xff);
	    }
	    Tcl_DStringFree(&dString);
	    
	    hostnameInited = 1;
	    return hostname;
	}
    }

    hostname[0] = '\0';
    hostnameInited = 1;
    return hostname;
}

/*
 *----------------------------------------------------------------------
 *
 * ResolveAddress --
 *
 *	This function is used to resolve an ip address to it's full 
 *	domain name address.
 *
 * Results:
 *	An os err value.
 *
 * Side effects:
 *	Treats client data as int we set to true.
 *
 *----------------------------------------------------------------------
 */

static OSErr 
ResolveAddress(
    ip_addr tcpAddress, 	/* Address to resolve. */
    Tcl_DString *dsPtr)		/* Returned address in string. */
{
    int i;
    EventRecord dummy;
    DNRState dnrState;
    OSErr err;

    /*
     * Call AddrToName to resolve our ip address to our domain name.
     * The call is async, so we must wait for a callback to tell us
     * when to continue.
     */

     for (i = 0; i < NUM_ALT_ADDRS; i++) {
	dnrState.hostInfo.addr[i] = 0;
     }
    dnrState.done = 0;
    GetCurrentProcess(&(dnrState.psn));
    err = AddrToName(tcpAddress, &dnrState.hostInfo, resultUPP, (Ptr) &dnrState);
    if (err == cacheFault) {
	while (!dnrState.done) {
	    WaitNextEvent(0, &dummy, 1, NULL);
	}
    }
    
    /*
     * If there is no error in finding the domain name we set the
     * result into the dynamic string.  We also work around a bug in
     * MacTcp where an extranious '.' may be found at the end of the name.
     */

    if (dnrState.hostInfo.rtnCode == noErr) {
	i = strlen(dnrState.hostInfo.cname) - 1;
	if (dnrState.hostInfo.cname[i] == '.') {
	    dnrState.hostInfo.cname[i] = '\0';
	}
	Tcl_DStringAppend(dsPtr, dnrState.hostInfo.cname, -1);
    }
    
    return dnrState.hostInfo.rtnCode;
}

/*
 *----------------------------------------------------------------------
 *
 * DNRCompletionRoutine --
 *
 *	This function is called when the Domain Name Server is done
 *	seviceing our request.  It just sets a flag that we can poll
 *	in functions like Tcl_GetHostName to let them know to continue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Treats client data as int we set to true.
 *
 *----------------------------------------------------------------------
 */

static pascal void 
DNRCompletionRoutine(
    struct hostInfo *hostinfoPtr, 	/* Host infor struct. */
    DNRState *dnrStatePtr)		/* Completetion state. */
{
    dnrStatePtr->done = true;
    WakeUpProcess(&(dnrStatePtr->psn));
}

/*
 *----------------------------------------------------------------------
 *
 * CleanUpExitProc --
 *
 *	This procedure is invoked as an exit handler when ExitToShell
 *	is called.  It aborts any lingering socket connections.  This 
 *	must be called or the Mac OS will more than likely crash.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static pascal void
CleanUpExitProc()
{
    TCPiopb exitPB;
    TcpState *statePtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    while (tsdPtr->socketList != NULL) {
	statePtr = tsdPtr->socketList;
	tsdPtr->socketList = statePtr->nextPtr;

	/*
	 * Close and Release the connection.
	 */

	exitPB.ioCRefNum = driverRefNum;
	exitPB.csCode = TCPClose;
	exitPB.tcpStream = statePtr->tcpStream;
	exitPB.csParam.close.ulpTimeoutValue = 60 /* seconds */;
	exitPB.csParam.close.ulpTimeoutAction = 1 /* 1:abort 0:report */;
	exitPB.csParam.close.validityFlags = timeoutValue | timeoutAction;
	exitPB.ioCompletion = NULL; 
	PBControlSync((ParmBlkPtr) &exitPB);

	exitPB.ioCRefNum = driverRefNum;
	exitPB.csCode = TCPRelease;
	exitPB.tcpStream = statePtr->tcpStream;
	exitPB.ioCompletion = NULL; 
	PBControlSync((ParmBlkPtr) &exitPB);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetHostFromString --
 *
 *	Looks up the passed in domain name in the domain resolver.  It
 *	can accept strings of two types: 1) the ip number in string
 *	format, or 2) the domain name.
 *
 * Results:
 *	We return a ip address or 0 if there was an error or the 
 *	domain does not exist.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static OSErr
GetHostFromString(
    char *name, 		/* Host in string form. */
    ip_addr *address)		/* Returned IP address. */
{
    OSErr err;
    int i;
    EventRecord dummy;
    DNRState dnrState;
	
    if (TclpHasSockets(NULL) != TCL_OK) {
	return 0;
    }

    /*
     * Call StrToAddr to get the ip number for the passed in domain
     * name.  The call is async, so we must wait for a callback to 
     * tell us when to continue.
     */

    for (i = 0; i < NUM_ALT_ADDRS; i++) {
	dnrState.hostInfo.addr[i] = 0;
    }
    dnrState.done = 0;
    GetCurrentProcess(&(dnrState.psn));
    err = StrToAddr(name, &dnrState.hostInfo, resultUPP, (Ptr) &dnrState);
    if (err == cacheFault) {
	while (!dnrState.done) {
	    WaitNextEvent(0, &dummy, 1, NULL);
	}
    }
    
    /*
     * For some reason MacTcp may return a cachFault a second time via
     * the hostinfo block.  This seems to be a bug in MacTcp.  In this case 
     * we run StrToAddr again - which seems to then work just fine.
     */

    if (dnrState.hostInfo.rtnCode == cacheFault) {
	dnrState.done = 0;
	err = StrToAddr(name, &dnrState.hostInfo, resultUPP, (Ptr) &dnrState);
	if (err == cacheFault) {
	    while (!dnrState.done) {
		WaitNextEvent(0, &dummy, 1, NULL);
	    }
	}
    }

    if (dnrState.hostInfo.rtnCode == noErr) {
	*address = dnrState.hostInfo.addr[0];
    }
    
    return dnrState.hostInfo.rtnCode;
}

/*
 *----------------------------------------------------------------------
 *
 * IOCompletionRoutine --
 *
 *	This function is called when an asynchronous socket operation
 *	completes.  Since this routine runs as an interrupt handler, 
 *	it will simply set state to tell the notifier that this socket
 *	is now ready for action.  Note that this function is running at
 *	interupt time and can't allocate memory or do much else except 
 *      set state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets some state in the socket state.  May also wake the process
 *	if we are not currently running.
 *
 *----------------------------------------------------------------------
 */

static void
IOCompletionRoutine(
    TCPiopb *pbPtr)		/* Tcp parameter block. */
{
    TcpState *statePtr;
    
    if (pbPtr->csCode == TCPSend) {
    	statePtr = (TcpState *) pbPtr->csParam.send.userDataPtr;
    } else {
	statePtr = (TcpState *) pbPtr->csParam.open.userDataPtr;
    }
    
    /*
     * Always wake the process in case it's in WaitNextEvent.
     * If an error has a occured - just return.  We will deal
     * with the problem later.
     */

    WakeUpProcess(&statePtr->psn);
    if (pbPtr->ioResult != noErr) {
	return;
    }
    
    if (statePtr->flags & TCP_ASYNC_CONNECT) {
	statePtr->flags &= ~TCP_ASYNC_CONNECT;
	statePtr->flags |= TCP_CONNECTED;
	statePtr->checkMask |= TCL_READABLE & TCL_WRITABLE;
    } else if (statePtr->flags & TCP_LISTENING) {
	if (statePtr->port == 0) {
	    Debugger();
	}
	statePtr->flags &= ~TCP_LISTENING;
	statePtr->flags |= TCP_LISTEN_CONNECT;
	statePtr->checkMask |= TCL_READABLE;
    } else if (statePtr->flags & TCP_WRITING) {
	statePtr->flags &= ~TCP_WRITING;
	statePtr->checkMask |= TCL_WRITABLE;
	if (!(statePtr->flags & TCP_CONNECTED)) {
	    InitMacTCPParamBlock(&statePtr->pb, TCPClose);
    	    statePtr->pb.tcpStream = statePtr->tcpStream;
    	    statePtr->pb.ioCompletion = closeUPP; 
    	    statePtr->pb.csParam.close.userDataPtr = (Ptr) statePtr;
    	    if (PBControlAsync((ParmBlkPtr) &statePtr->pb) != noErr) {
	        statePtr->flags |= TCP_RELEASE;
    	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetLocalAddress --
 *
 *	Get the IP address for this machine.  The result is cached so
 *	the result is returned quickly after the first call.
 *
 * Results:
 *	Macintosh error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static OSErr 
GetLocalAddress(
    unsigned long *addr)	/* Returns host IP address. */
{
    struct GetAddrParamBlock pBlock;
    OSErr err = noErr;
    static unsigned long localAddress = 0;

    if (localAddress == 0) {
	memset(&pBlock, 0, sizeof(pBlock));
	pBlock.ioResult = 1;
	pBlock.csCode = ipctlGetAddr;
	pBlock.ioCRefNum = driverRefNum;
	err = PBControlSync((ParmBlkPtr) &pBlock);

	if (err != noErr) {
	    return err;
	}
	localAddress = pBlock.ourAddress;
    }
    
    *addr = localAddress;
    return noErr;
}

/*
 *----------------------------------------------------------------------
 *
 * GetBufferSize --
 *
 *	Get the appropiate buffer size for our machine & network.  This
 *	value will be used by the rest of Tcl & the MacTcp driver for
 *	the size of its buffers.  If out method for determining the
 *	optimal buffer size fails for any reason - we return a 
 *	reasonable default.
 *
 * Results:
 *	Size of optimal buffer in bytes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static long 
GetBufferSize()
{
    UDPiopb iopb;
    OSErr err = noErr;
    long bufferSize;
	
    memset(&iopb, 0, sizeof(iopb));
    err = GetLocalAddress(&iopb.csParam.mtu.remoteHost);
    if (err != noErr) {
	return CHANNEL_BUF_SIZE;
    }
    iopb.ioCRefNum = driverRefNum;
    iopb.csCode = UDPMaxMTUSize;
    err = PBControlSync((ParmBlkPtr)&iopb);
    if (err != noErr) {
	return CHANNEL_BUF_SIZE;
    }
    bufferSize = (iopb.csParam.mtu.mtuSize * 4) + 1024;
    if (bufferSize < CHANNEL_BUF_SIZE) {
	bufferSize = CHANNEL_BUF_SIZE;
    }
    return bufferSize;
}

/*
 *----------------------------------------------------------------------
 *
 * TclSockGetPort --
 *
 *	Maps from a string, which could be a service name, to a port.
 *	Used by socket creation code to get port numbers and resolve
 *	registered service names to port numbers.
 *
 * Results:
 *	A standard Tcl result.  On success, the port number is
 *	returned in portPtr. On failure, an error message is left in
 *	the interp's result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclSockGetPort(
    Tcl_Interp *interp, 	/* Interp for error messages. */
    char *string, 		/* Integer or service name */
    char *proto, 		/* "tcp" or "udp", typically - 
    				 * ignored on Mac - assumed to be tcp */
    int *portPtr)		/* Return port number */
{
    PortInfo *portInfoPtr = NULL;
    
    if (Tcl_GetInt(interp, string, portPtr) == TCL_OK) {
	if (*portPtr > 0xFFFF) {
	    Tcl_AppendResult(interp, "couldn't open socket: port number too high",
                (char *) NULL);
	    return TCL_ERROR;
	}
	if (*portPtr < 0) {
	    Tcl_AppendResult(interp, "couldn't open socket: negative port number",
                (char *) NULL);
	    return TCL_ERROR;
	}
	return TCL_OK;
    }
    for (portInfoPtr = portServices; portInfoPtr->name != NULL; portInfoPtr++) {
	if (!strcmp(portInfoPtr->name, string)) {
	    break;
	}
    }
    if (portInfoPtr != NULL && portInfoPtr->name != NULL) {
	*portPtr = portInfoPtr->port;
	Tcl_ResetResult(interp);
	return TCL_OK;
    }
    
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ClearZombieSockets --
 *
 *	This procedure looks through the socket list and removes the
 *	first stream it finds that is ready for release. This procedure 
 *	should be called before we ever try to create new Tcp streams
 *	to ensure we can least allocate one stream.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Tcp streams may be released.
 *
 *----------------------------------------------------------------------
 */

static void
ClearZombieSockets()
{
    TcpState *statePtr;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    for (statePtr = tsdPtr->socketList; statePtr != NULL;
	    statePtr = statePtr->nextPtr) {
	if (statePtr->flags & TCP_RELEASE) {
	    SocketFreeProc(statePtr);
	    return;
	}
    }
}
