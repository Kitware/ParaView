/* 
 * tkConsole.c --
 *
 *	This file implements a Tcl console for systems that may not
 *	otherwise have access to a console.  It uses the Text widget
 *	and provides special access via a console command.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tk.h"
#include <string.h>

#include "tkInt.h"

/*
 * A data structure of the following type holds information for each console
 * which a handler (i.e. a Tcl command) has been defined for a particular
 * top-level window.
 */

typedef struct ConsoleInfo {
    Tcl_Interp *consoleInterp;	/* Interpreter for the console. */
    Tcl_Interp *interp;		/* Interpreter to send console commands. */
} ConsoleInfo;

typedef struct ThreadSpecificData {
    Tcl_Interp *gStdoutInterp;
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;
static int consoleInitialized = 0;

/* 
 * The Mutex below is used to lock access to the consoleIntialized flag
 */

TCL_DECLARE_MUTEX(consoleMutex)

/*
 * Forward declarations for procedures defined later in this file:
 *
 * The first three will be used in the tk app shells...
 */
 
void	TkConsolePrint _ANSI_ARGS_((Tcl_Interp *interp,
			    int devId, char *buffer, long size));

static int	ConsoleCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
static void	ConsoleDeleteProc _ANSI_ARGS_((ClientData clientData));
static void	ConsoleEventProc _ANSI_ARGS_((ClientData clientData,
		    XEvent *eventPtr));
static int	InterpreterCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));

static int	ConsoleInput _ANSI_ARGS_((ClientData instanceData,
		    char *buf, int toRead, int *errorCode));
static int	ConsoleOutput _ANSI_ARGS_((ClientData instanceData,
		    char *buf, int toWrite, int *errorCode));
static int	ConsoleClose _ANSI_ARGS_((ClientData instanceData,
		    Tcl_Interp *interp));
static void	ConsoleWatch _ANSI_ARGS_((ClientData instanceData,
		    int mask));
static int	ConsoleHandle _ANSI_ARGS_((ClientData instanceData,
		    int direction, ClientData *handlePtr));

/*
 * This structure describes the channel type structure for file based IO:
 */

static Tcl_ChannelType consoleChannelType = {
    "console",			/* Type name. */
    NULL,			/* Always non-blocking.*/
    ConsoleClose,		/* Close proc. */
    ConsoleInput,		/* Input proc. */
    ConsoleOutput,		/* Output proc. */
    NULL,			/* Seek proc. */
    NULL,			/* Set option proc. */
    NULL,			/* Get option proc. */
    ConsoleWatch,		/* Watch for events on console. */
    ConsoleHandle,		/* Get a handle from the device. */
};


#ifdef __WIN32__

#include <windows.h>

/*
 *----------------------------------------------------------------------
 *
 * ShouldUseConsoleChannel
 *
 * 	Check to see if console window should be used for a given
 *      standard channel
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the console channel and installs it as the standard
 *	channels.
 *
 *----------------------------------------------------------------------
 */
static int ShouldUseConsoleChannel(type)
    int type;
{
    DWORD handleId;		/* Standard handle to retrieve. */
    DCB dcb;
    DWORD consoleParams;
    DWORD fileType;
    int mode;
    char *bufMode;
    HANDLE handle;

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
	    return 0;
	    break;
    }

    handle = GetStdHandle(handleId);

    /*
     * Note that we need to check for 0 because Windows will return 0 if this
     * is not a console mode application, even though this is not a valid
     * handle. 
     */

    if ((handle == INVALID_HANDLE_VALUE) || (handle == 0)) {
	return 1;
    }
    fileType = GetFileType(handle);

    /*
     * If the file is a character device, we need to try to figure out
     * whether it is a serial port, a console, or something else.  We
     * test for the console case first because this is more common.
     */

    if (fileType == FILE_TYPE_CHAR) {
	dcb.DCBlength = sizeof( DCB ) ;
	if (!GetConsoleMode(handle, &consoleParams) &&
		!GetCommState(handle, &dcb)) {
	    /*
	     * Don't use a CHAR type channel for stdio, otherwise Tk
	     * runs into trouble with the MS DevStudio debugger.
	     */
	    
	    return 1;
	}
    } else if (fileType == FILE_TYPE_UNKNOWN) {
	return 1;
    } else if (Tcl_GetStdChannel(type) == NULL) {
	return 1;
    }

    return 0;
}
#else
/*
 * Mac should always use a console channel, Unix should if it's trying to
 */

#define ShouldUseConsoleChannel(chan) (1)
#endif

/*
 *----------------------------------------------------------------------
 *
 * Tk_InitConsoleChannels --
 *
 * 	Create the console channels and install them as the standard
 * 	channels.  All I/O will be discarded until TkConsoleInit is
 * 	called to attach the console to a text widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the console channel and installs it as the standard
 *	channels.
 *
 *----------------------------------------------------------------------
 */

void
Tk_InitConsoleChannels(interp)
    Tcl_Interp *interp;
{
    Tcl_Channel consoleChannel;

    /*
     * Ensure that we are getting the matching version of Tcl.  This is
     * really only an issue when Tk is loaded dynamically.
     */

    if (Tcl_InitStubs(interp, TCL_VERSION, 1) == NULL) {
        return;
    }

    Tcl_MutexLock(&consoleMutex);
    if (!consoleInitialized) {

	consoleInitialized = 1;
	
	/*
	 * check for STDIN, otherwise create it
	 *
	 * Don't do this check on the Mac, because it is hard to prevent
	 * callbacks from the SIOUX layer from opening stdout & stdin, but
	 * we don't want to use the SIOUX console.  Since the console is not
	 * actually created till something is written to the channel, it is
	 * okay to just ignore it here.
	 *
	 * This is still a bit of a hack, however, and should be cleaned up
	 * when we have a better abstraction for the console.
	 */

	if (ShouldUseConsoleChannel(TCL_STDIN)) {
	    consoleChannel = Tcl_CreateChannel(&consoleChannelType, "console0",
		    (ClientData) TCL_STDIN, TCL_READABLE);
	    if (consoleChannel != NULL) {
		Tcl_SetChannelOption(NULL, consoleChannel,
			"-translation", "lf");
		Tcl_SetChannelOption(NULL, consoleChannel,
			"-buffering", "none");
		Tcl_SetChannelOption(NULL, consoleChannel,
			"-encoding", "utf-8");
	    }
	    Tcl_SetStdChannel(consoleChannel, TCL_STDIN);
	}

	/*
	 * check for STDOUT, otherwise create it
	 */
	
	if (ShouldUseConsoleChannel(TCL_STDOUT)) {
	    consoleChannel = Tcl_CreateChannel(&consoleChannelType, "console1",
		    (ClientData) TCL_STDOUT, TCL_WRITABLE);
	    if (consoleChannel != NULL) {
		Tcl_SetChannelOption(NULL, consoleChannel,
			"-translation", "lf");
		Tcl_SetChannelOption(NULL, consoleChannel,
			"-buffering", "none");
		Tcl_SetChannelOption(NULL, consoleChannel,
			"-encoding", "utf-8");
	    }
	    Tcl_SetStdChannel(consoleChannel, TCL_STDOUT);
	}
	
	/*
	 * check for STDERR, otherwise create it
	 */
	
	if (ShouldUseConsoleChannel(TCL_STDERR)) {
	    consoleChannel = Tcl_CreateChannel(&consoleChannelType, "console2",
		    (ClientData) TCL_STDERR, TCL_WRITABLE);
	    if (consoleChannel != NULL) {
		Tcl_SetChannelOption(NULL, consoleChannel,
			"-translation", "lf");
		Tcl_SetChannelOption(NULL, consoleChannel,
			"-buffering", "none");
		Tcl_SetChannelOption(NULL, consoleChannel,
			"-encoding", "utf-8");
	    }
	    Tcl_SetStdChannel(consoleChannel, TCL_STDERR);
	}
    }
    Tcl_MutexUnlock(&consoleMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CreateConsoleWindow --
 *
 *	Initialize the console.  This code actually creates a new
 *	application and associated interpreter.  This effectivly hides
 *	the implementation from the main application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new console it created.
 *
 *----------------------------------------------------------------------
 */

int 
Tk_CreateConsoleWindow(interp)
    Tcl_Interp *interp;			/* Interpreter to use for prompting. */
{
    Tcl_Interp *consoleInterp;
    ConsoleInfo *info;
    Tk_Window mainWindow = Tk_MainWindow(interp);
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
#ifdef MAC_TCL
    static char initCmd[] = "source -rsrc {Console}";
#else
    static char initCmd[] = "source $tk_library/console.tcl";
#endif
    
    consoleInterp = Tcl_CreateInterp();
    if (consoleInterp == NULL) {
	goto error;
    }
    
    /*
     * Initialized Tcl and Tk.
     */

    if (Tcl_Init(consoleInterp) != TCL_OK) {
	goto error;
    }
    if (Tk_Init(consoleInterp) != TCL_OK) {
	goto error;
    }
    tsdPtr->gStdoutInterp = interp;
    
    /* 
     * Add console commands to the interp 
     */
    info = (ConsoleInfo *) ckalloc(sizeof(ConsoleInfo));
    info->interp = interp;
    info->consoleInterp = consoleInterp;
    Tcl_CreateCommand(interp, "console", ConsoleCmd, (ClientData) info,
	    (Tcl_CmdDeleteProc *) ConsoleDeleteProc);
    Tcl_CreateCommand(consoleInterp, "consoleinterp", InterpreterCmd,
	    (ClientData) info, (Tcl_CmdDeleteProc *) NULL);

    Tk_CreateEventHandler(mainWindow, StructureNotifyMask, ConsoleEventProc,
	    (ClientData) info);

    Tcl_Preserve((ClientData) consoleInterp);
    if (Tcl_Eval(consoleInterp, initCmd) == TCL_ERROR) {
	/* goto error; -- no problem for now... */
	printf("Eval error: %s", consoleInterp->result);
    }
    Tcl_Release((ClientData) consoleInterp);
    return TCL_OK;
    
    error:
    if (consoleInterp != NULL) {
    	Tcl_DeleteInterp(consoleInterp);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleOutput--
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
ConsoleOutput(instanceData, buf, toWrite, errorCode)
    ClientData instanceData;		/* Indicates which device to use. */
    char *buf;				/* The data buffer. */
    int toWrite;			/* How many bytes to write? */
    int *errorCode;			/* Where to store error code. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    *errorCode = 0;
    Tcl_SetErrno(0);

    if (tsdPtr->gStdoutInterp != NULL) {
	TkConsolePrint(tsdPtr->gStdoutInterp, (int) instanceData, buf, 
                toWrite);
    }
    
    return toWrite;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleInput --
 *
 *	Read input from the console.  Not currently implemented.
 *
 * Results:
 *	Always returns EOF.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ConsoleInput(instanceData, buf, bufSize, errorCode)
    ClientData instanceData;		/* Unused. */
    char *buf;				/* Where to store data read. */
    int bufSize;			/* How much space is available
                                         * in the buffer? */
    int *errorCode;			/* Where to store error code. */
{
    return 0;			/* Always return EOF. */
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleClose --
 *
 *	Closes the IO channel.
 *
 * Results:
 *	Always returns 0 (success).
 *
 * Side effects:
 *	Frees the dummy file associated with the channel.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ConsoleClose(instanceData, interp)
    ClientData instanceData;	/* Unused. */
    Tcl_Interp *interp;		/* Unused. */
{
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleWatch --
 *
 *	Called by the notifier to set up the console device so that
 *	events will be noticed. Since there are no events on the
 *	console, this routine just returns without doing anything.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
ConsoleWatch(instanceData, mask)
    ClientData instanceData;		/* Device ID for the channel. */
    int mask;				/* OR-ed combination of
                                         * TCL_READABLE, TCL_WRITABLE and
                                         * TCL_EXCEPTION, for the events
                                         * we are interested in. */
{
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleHandle --
 *
 *	Invoked by the generic IO layer to get a handle from a channel.
 *	Because console channels are not devices, this function always
 *	fails.
 *
 * Results:
 *	Always returns TCL_ERROR.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ConsoleHandle(instanceData, direction, handlePtr)
    ClientData instanceData;	/* Device ID for the channel. */
    int direction;		/* TCL_READABLE or TCL_WRITABLE to indicate
				 * which direction of the channel is being
				 * requested. */
    ClientData *handlePtr;	/* Where to store handle */
{
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleCmd --
 *
 *	The console command implements a Tcl interface to the various console
 *	options.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ConsoleCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    ConsoleInfo *info = (ConsoleInfo *) clientData;
    char c;
    size_t length;
    int result;
    Tcl_Interp *consoleInterp;
    Tcl_DString dString;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    
    c = argv[1][0];
    length = strlen(argv[1]);
    result = TCL_OK;
    consoleInterp = info->consoleInterp;
    Tcl_Preserve((ClientData) consoleInterp);
    Tcl_DStringInit(&dString);

    if ((c == 't') && (strncmp(argv[1], "title", length)) == 0) {
	Tcl_DStringAppend(&dString, "wm title . ", -1);
	if (argc == 3) {
	    Tcl_DStringAppendElement(&dString, argv[2]);
	}
	Tcl_Eval(consoleInterp, Tcl_DStringValue(&dString));
    } else if ((c == 'h') && (strncmp(argv[1], "hide", length)) == 0) {
	Tcl_DStringAppend(&dString, "wm withdraw . ", -1);
	Tcl_Eval(consoleInterp, Tcl_DStringValue(&dString));
    } else if ((c == 's') && (strncmp(argv[1], "show", length)) == 0) {
	Tcl_DStringAppend(&dString, "wm deiconify . ", -1);
	Tcl_Eval(consoleInterp, Tcl_DStringValue(&dString));
    } else if ((c == 'e') && (strncmp(argv[1], "eval", length)) == 0) {
	if (argc == 3) {
	    result = Tcl_Eval(consoleInterp, argv[2]);
	    Tcl_AppendResult(interp, Tcl_GetStringResult(consoleInterp),
		    (char *) NULL);
	} else {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " eval command\"", (char *) NULL);
	    return TCL_ERROR;
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": should be hide, show, or title",
		(char *) NULL);
        result = TCL_ERROR;
    }
    Tcl_DStringFree(&dString);
    Tcl_Release((ClientData) consoleInterp);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * InterpreterCmd --
 *
 *	This command allows the console interp to communicate with the
 *	main interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
InterpreterCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    ConsoleInfo *info = (ConsoleInfo *) clientData;
    char c;
    size_t length;
    int result;
    Tcl_Interp *otherInterp;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    
    c = argv[1][0];
    length = strlen(argv[1]);
    otherInterp = info->interp;
    Tcl_Preserve((ClientData) otherInterp);
    if ((c == 'e') && (strncmp(argv[1], "eval", length)) == 0) {
   	result = Tcl_GlobalEval(otherInterp, argv[2]);
    	Tcl_AppendResult(interp, otherInterp->result, (char *) NULL);
    } else if ((c == 'r') && (strncmp(argv[1], "record", length)) == 0) {
   	Tcl_RecordAndEval(otherInterp, argv[2], TCL_EVAL_GLOBAL);
	result = TCL_OK;
	Tcl_ResetResult(interp);
    	Tcl_AppendResult(interp, otherInterp->result, (char *) NULL);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": should be eval or record",
		(char *) NULL);
	result = TCL_ERROR;
    }
    Tcl_Release((ClientData) otherInterp);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleDeleteProc --
 *
 *	If the console command is deleted we destroy the console window
 *	and all associated data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new console it created.
 *
 *----------------------------------------------------------------------
 */

void 
ConsoleDeleteProc(clientData) 
    ClientData clientData;
{
    ConsoleInfo *info = (ConsoleInfo *) clientData;

    Tcl_DeleteInterp(info->consoleInterp);
    info->consoleInterp = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ConsoleEventProc --
 *
 *	This event procedure is registered on the main window of the
 *	slave interpreter.  If the user or a running script causes the
 *	main window to be destroyed, then we need to inform the console
 *	interpreter by invoking "tkConsoleExit".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Invokes the "tkConsoleExit" procedure in the console interp.
 *
 *----------------------------------------------------------------------
 */

static void
ConsoleEventProc(clientData, eventPtr)
    ClientData clientData;
    XEvent *eventPtr;
{
    ConsoleInfo *info = (ConsoleInfo *) clientData;
    Tcl_Interp *consoleInterp;
    Tcl_DString dString;
    
    if (eventPtr->type == DestroyNotify) {

	Tcl_DStringInit(&dString);
  
	consoleInterp = info->consoleInterp;

        /*
         * It is possible that the console interpreter itself has
         * already been deleted. In that case the consoleInterp
         * field will be set to NULL. If the interpreter is already
         * gone, we do not have to do any work here.
         */
        
        if (consoleInterp == (Tcl_Interp *) NULL) {
            return;
        }
        Tcl_Preserve((ClientData) consoleInterp);
	Tcl_DStringAppend(&dString, "tkConsoleExit", -1);
	Tcl_Eval(consoleInterp, Tcl_DStringValue(&dString));
	Tcl_DStringFree(&dString);
        Tcl_Release((ClientData) consoleInterp);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkConsolePrint --
 *
 *	Prints to the give text to the console.  Given the main interp
 *	this functions find the appropiate console interp and forwards
 *	the text to be added to that console.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkConsolePrint(interp, devId, buffer, size)
    Tcl_Interp *interp;		/* Main interpreter. */
    int devId;			/* TCL_STDOUT for stdout, TCL_STDERR for
                                 * stderr. */
    char *buffer;		/* Text buffer. */
    long size;			/* Size of text buffer. */
{
    Tcl_DString command, output;
    Tcl_CmdInfo cmdInfo;
    char *cmd;
    ConsoleInfo *info;
    Tcl_Interp *consoleInterp;
    int result;

    if (interp == NULL) {
	return;
    }
    
    if (devId == TCL_STDERR) {
	cmd = "tkConsoleOutput stderr ";
    } else {
	cmd = "tkConsoleOutput stdout ";
    }
    
    result = Tcl_GetCommandInfo(interp, "console", &cmdInfo);
    if (result == 0) {
	return;
    }
    info = (ConsoleInfo *) cmdInfo.clientData;
    
    Tcl_DStringInit(&output);
    Tcl_DStringAppend(&output, buffer, size);

    Tcl_DStringInit(&command);
    Tcl_DStringAppend(&command, cmd, (int) strlen(cmd));
    Tcl_DStringAppendElement(&command, output.string);

    consoleInterp = info->consoleInterp;
    Tcl_Preserve((ClientData) consoleInterp);
    Tcl_Eval(consoleInterp, command.string);
    Tcl_Release((ClientData) consoleInterp);
    
    Tcl_DStringFree(&command);
    Tcl_DStringFree(&output);
}
