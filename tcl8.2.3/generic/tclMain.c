/* 
 * tclMain.c --
 *
 *	Main program for Tcl shells and other Tcl-based applications.
 *
 * Copyright (c) 1988-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tcl.h"
#include "tclInt.h"

# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT

/*
 * The following code ensures that tclLink.c is linked whenever
 * Tcl is linked.  Without this code there's no reference to the
 * code in that file from anywhere in Tcl, so it may not be
 * linked into the application.
 */

EXTERN int Tcl_LinkVar();
int (*tclDummyLinkVarPtr)() = Tcl_LinkVar;

/*
 * Declarations for various library procedures and variables (don't want
 * to include tclPort.h here, because people might copy this file out of
 * the Tcl source directory to make their own modified versions).
 * Note:  "exit" should really be declared here, but there's no way to
 * declare it without causing conflicts with other definitions elsewher
 * on some systems, so it's better just to leave it out.
 */

extern int		isatty _ANSI_ARGS_((int fd));
extern char *		strcpy _ANSI_ARGS_((char *dst, CONST char *src));


/*
 *----------------------------------------------------------------------
 *
 * Tcl_Main --
 *
 *	Main program for tclsh and most other Tcl-based applications.
 *
 * Results:
 *	None. This procedure never returns (it exits the process when
 *	it's done.
 *
 * Side effects:
 *	This procedure initializes the Tk world and then starts
 *	interpreting commands;  almost anything could happen, depending
 *	on the script being interpreted.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_Main(argc, argv, appInitProc)
    int argc;			/* Number of arguments. */
    char **argv;		/* Array of argument strings. */
    Tcl_AppInitProc *appInitProc;
				/* Application-specific initialization
				 * procedure to call after most
				 * initialization but before starting to
				 * execute commands. */
{
    Tcl_Obj *resultPtr;
    Tcl_Obj *commandPtr = NULL;
    char buffer[1000], *args, *fileName;
    int code, gotPartial, tty, length;
    int exitCode = 0;
    Tcl_Channel inChannel, outChannel, errChannel;
    Tcl_Interp *interp;
    Tcl_DString argString;

    Tcl_FindExecutable(argv[0]);
    interp = Tcl_CreateInterp();
#ifdef TCL_MEM_DEBUG
    Tcl_InitMemory(interp);
#endif

    /*
     * Make command-line arguments available in the Tcl variables "argc"
     * and "argv".  If the first argument doesn't start with a "-" then
     * strip it off and use it as the name of a script file to process.
     */

    fileName = NULL;
    if ((argc > 1) && (argv[1][0] != '-')) {
	fileName = argv[1];
	argc--;
	argv++;
    }
    args = Tcl_Merge(argc-1, argv+1);
    Tcl_ExternalToUtfDString(NULL, args, -1, &argString);
    Tcl_SetVar(interp, "argv", Tcl_DStringValue(&argString), TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&argString);
    ckfree(args);

    if (fileName == NULL) {
	Tcl_ExternalToUtfDString(NULL, argv[0], -1, &argString);
    } else {
	fileName = Tcl_ExternalToUtfDString(NULL, fileName, -1, &argString);
    }

    TclFormatInt(buffer, argc-1);
    Tcl_SetVar(interp, "argc", buffer, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "argv0", Tcl_DStringValue(&argString), TCL_GLOBAL_ONLY);

    /*
     * Set the "tcl_interactive" variable.
     */

    tty = isatty(0);
    Tcl_SetVar(interp, "tcl_interactive",
	    ((fileName == NULL) && tty) ? "1" : "0", TCL_GLOBAL_ONLY);
    
    /*
     * Invoke application-specific initialization.
     */

    if ((*appInitProc)(interp) != TCL_OK) {
	errChannel = Tcl_GetStdChannel(TCL_STDERR);
	if (errChannel) {
	    Tcl_WriteChars(errChannel,
		    "application-specific initialization failed: ", -1);
	    Tcl_WriteObj(errChannel, Tcl_GetObjResult(interp));
	    Tcl_WriteChars(errChannel, "\n", 1);
	}
    }

    /*
     * If a script file was specified then just source that file
     * and quit.
     */

    if (fileName != NULL) {
	code = Tcl_EvalFile(interp, fileName);
	if (code != TCL_OK) {
	    errChannel = Tcl_GetStdChannel(TCL_STDERR);
	    if (errChannel) {
		/*
		 * The following statement guarantees that the errorInfo
		 * variable is set properly.
		 */

		Tcl_AddErrorInfo(interp, "");
		Tcl_WriteObj(errChannel, Tcl_GetVar2Ex(interp, "errorInfo",
			NULL, TCL_GLOBAL_ONLY));
		Tcl_WriteChars(errChannel, "\n", 1);
	    }
	    exitCode = 1;
	}
	goto done;
    }
    Tcl_DStringFree(&argString);

    /*
     * We're running interactively.  Source a user-specific startup
     * file if the application specified one and if the file exists.
     */

    Tcl_SourceRCFile(interp);

    /*
     * Process commands from stdin until there's an end-of-file.  Note
     * that we need to fetch the standard channels again after every
     * eval, since they may have been changed.
     */

    commandPtr = Tcl_NewObj();
    Tcl_IncrRefCount(commandPtr);

    inChannel = Tcl_GetStdChannel(TCL_STDIN);
    outChannel = Tcl_GetStdChannel(TCL_STDOUT);
    gotPartial = 0;
    while (1) {
	if (tty) {
	    Tcl_Obj *promptCmdPtr;

	    promptCmdPtr = Tcl_GetVar2Ex(interp,
		    (gotPartial ? "tcl_prompt2" : "tcl_prompt1"),
		    NULL, TCL_GLOBAL_ONLY);
	    if (promptCmdPtr == NULL) {
                defaultPrompt:
		if (!gotPartial && outChannel) {
		    Tcl_WriteChars(outChannel, "% ", 2);
		}
	    } else {
		code = Tcl_EvalObjEx(interp, promptCmdPtr, 0);
		inChannel = Tcl_GetStdChannel(TCL_STDIN);
		outChannel = Tcl_GetStdChannel(TCL_STDOUT);
		errChannel = Tcl_GetStdChannel(TCL_STDERR);
		if (code != TCL_OK) {
		    if (errChannel) {
			Tcl_WriteObj(errChannel, Tcl_GetObjResult(interp));
			Tcl_WriteChars(errChannel, "\n", 1);
		    }
		    Tcl_AddErrorInfo(interp,
			    "\n    (script that generates prompt)");
		    goto defaultPrompt;
		}
	    }
	    if (outChannel) {
		Tcl_Flush(outChannel);
	    }
	}
	if (!inChannel) {
	    goto done;
	}
        length = Tcl_GetsObj(inChannel, commandPtr);
	if (length < 0) {
	    goto done;
	}
	if ((length == 0) && Tcl_Eof(inChannel) && (!gotPartial)) {
	    goto done;
	}

        /*
         * Add the newline removed by Tcl_GetsObj back to the string.
         */

	Tcl_AppendToObj(commandPtr, "\n", 1);
	if (!TclObjCommandComplete(commandPtr)) {
	    gotPartial = 1;
	    continue;
	}

	gotPartial = 0;
	code = Tcl_RecordAndEvalObj(interp, commandPtr, 0);
	inChannel = Tcl_GetStdChannel(TCL_STDIN);
	outChannel = Tcl_GetStdChannel(TCL_STDOUT);
	errChannel = Tcl_GetStdChannel(TCL_STDERR);
	Tcl_SetObjLength(commandPtr, 0);
	if (code != TCL_OK) {
	    if (errChannel) {
		Tcl_WriteObj(errChannel, Tcl_GetObjResult(interp));
		Tcl_WriteChars(errChannel, "\n", 1);
	    }
	} else if (tty) {
	    resultPtr = Tcl_GetObjResult(interp);
	    Tcl_GetStringFromObj(resultPtr, &length);
	    if ((length > 0) && outChannel) {
		Tcl_WriteObj(outChannel, resultPtr);
		Tcl_WriteChars(outChannel, "\n", 1);
	    }
	}
#ifdef TCL_MEM_DEBUG
	if (tclMemDumpFileName != NULL) {
	    Tcl_DecrRefCount(commandPtr);
	    Tcl_DeleteInterp(interp);
	    Tcl_Exit(0);
	}
#endif
    }

    /*
     * Rather than calling exit, invoke the "exit" command so that
     * users can replace "exit" with some other command to do additional
     * cleanup on exit.  The Tcl_Eval call should never return.
     */

    done:
    if (commandPtr != NULL) {
	Tcl_DecrRefCount(commandPtr);
    }
    sprintf(buffer, "exit %d", exitCode);
    Tcl_Eval(interp, buffer);
}
