/* 
 * tclMacBGMain.c --
 *
 *	Main program for Macintosh Background Only Application shells.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tcl.h"
#include "tclInt.h"
#include "tclMacInt.h"
#include <Resources.h>
#include <Notification.h>
#include <Strings.h>

/*
 * This variable is used to get out of the modal loop of the
 * notification manager.
 */

int NotificationIsDone = 0;

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

static Tcl_Interp *interp;	/* Interpreter for application. */

#ifdef TCL_MEM_DEBUG
static char dumpFile[100];	/* Records where to dump memory allocation
				 * information. */
static int quitFlag = 0;	/* 1 means "checkmem" command was called,
				 * so the application should quit and dump
				 * memory allocation information. */
#endif

/*
 * Forward references for procedures defined later in this file:
 */

#ifdef TCL_MEM_DEBUG
static int		CheckmemCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char *argv[]));
#endif
void TclMacDoNotification(char *mssg);
void TclMacNotificationResponse(NMRecPtr nmRec); 
int Tcl_MacBGNotifyObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv);


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
    Tcl_Obj *prompt1NamePtr = NULL;
    Tcl_Obj *prompt2NamePtr = NULL;
    Tcl_Obj *commandPtr = NULL;
    char buffer[1000], *args, *fileName;
    int code, tty;
    int exitCode = 0;

    Tcl_FindExecutable(argv[0]);
    interp = Tcl_CreateInterp();
#ifdef TCL_MEM_DEBUG
    Tcl_InitMemory(interp);
    Tcl_CreateCommand(interp, "checkmem", CheckmemCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
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
    Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
    ckfree(args);
    TclFormatInt(buffer, argc-1);
    Tcl_SetVar(interp, "argc", buffer, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "argv0", (fileName != NULL) ? fileName : argv[0],
	    TCL_GLOBAL_ONLY);

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
	Tcl_DString errStr;

	Tcl_DStringInit(&errStr);
	Tcl_DStringAppend(&errStr,
		"application-specific initialization failed: \n", -1);
	Tcl_DStringAppend(&errStr, Tcl_GetStringResult(interp), -1);
	Tcl_DStringAppend(&errStr, "\n", 1);
	TclMacDoNotification(Tcl_DStringValue(&errStr));
	Tcl_DStringFree(&errStr);
	goto done;
    }

    /*
     * Install the BGNotify command:
     */
    
    if ( Tcl_CreateObjCommand(interp, "bgnotify", Tcl_MacBGNotifyObjCmd, NULL,
             (Tcl_CmdDeleteProc *) NULL) == NULL) {
        goto done;
    }
    
    /*
     * If a script file was specified then just source that file
     * and quit.  In this Mac BG Application version, we will try the
     * resource fork first, then the file system second...
     */

    if (fileName != NULL) {
        Str255 resName;
        Handle resource;
        
        strcpy((char *) resName + 1, fileName);
        resName[0] = strlen(fileName);
        resource = GetNamedResource('TEXT',resName);
        if (resource != NULL) {
            code = Tcl_MacEvalResource(interp, fileName, -1, NULL);
        } else {
            code = Tcl_EvalFile(interp, fileName);
        }
        
	if (code != TCL_OK) {
            Tcl_DString errStr;
            
            Tcl_DStringInit(&errStr);
            Tcl_DStringAppend(&errStr, " Error sourcing resource or file: ", -1);
            Tcl_DStringAppend(&errStr, fileName, -1);
            Tcl_DStringAppend(&errStr, "\n\nError was: ", -1);
            Tcl_DStringAppend(&errStr, Tcl_GetStringResult(interp), -1);
            TclMacDoNotification(Tcl_DStringValue(&errStr));
	    Tcl_DStringFree(&errStr);
        }
	goto done;
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
    if (prompt1NamePtr != NULL) {
	Tcl_DecrRefCount(prompt1NamePtr);
    }
    if (prompt2NamePtr != NULL) {
	Tcl_DecrRefCount(prompt2NamePtr);
    }
    sprintf(buffer, "exit %d", exitCode);
    Tcl_Eval(interp, buffer);
}

/*----------------------------------------------------------------------
 *
 * TclMacDoNotification --
 *
 *	This posts an error message using the Notification manager.
 *
 * Results:
 *	Post a Notification Manager dialog.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void 
TclMacDoNotification(mssg)
    char *mssg;
{
    NMRec errorNot;
    EventRecord *theEvent = NULL;
    OSErr err;
    char *ptr;
    
    errorNot.qType = nmType;
    errorNot.nmMark = 0;
    errorNot.nmIcon = 0;
    errorNot.nmSound = (Handle) -1;

    for ( ptr = mssg; *ptr != '\0'; ptr++) {
        if (*ptr == '\n') {
            *ptr = '\r';
        }
    }
        
    c2pstr(mssg);
    errorNot.nmStr = (StringPtr) mssg;

    errorNot.nmResp = NewNMProc(TclMacNotificationResponse);
    errorNot.nmRefCon = SetCurrentA5();
    
    NotificationIsDone = 0;
    
    /*
     * Cycle while waiting for the user to click on the
     * notification box.  Don't take any events off the event queue,
     * since we want Tcl to do this but we want to block till the notification
     * has been handled...
     */
    
    err = NMInstall(&errorNot);
    if (err == noErr) { 
        while (!NotificationIsDone) {
            WaitNextEvent(0, theEvent, 20, NULL);
        }
        NMRemove(&errorNot);
    }
    	
    p2cstr((unsigned char *) mssg);
}

void 
TclMacNotificationResponse(nmRec) 
    NMRecPtr nmRec;
{
    int curA5;
    
    curA5 = SetCurrentA5();
    SetA5(nmRec->nmRefCon);
    
    NotificationIsDone = 1;
    
    SetA5(curA5);
    
}

int 
Tcl_MacBGNotifyObjCmd(clientData, interp, objc, objv)
    ClientData clientData;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj **objv;	
{
    Tcl_Obj *resultPtr;
    
    resultPtr = Tcl_GetObjResult(interp);
    
    if ( objc != 2 ) {
        Tcl_WrongNumArgs(interp, 1, objv, "message");
        return TCL_ERROR;
    }
    
    TclMacDoNotification(Tcl_GetString(objv[1]));
    return TCL_OK;
           
}


/*
 *----------------------------------------------------------------------
 *
 * CheckmemCmd --
 *
 *	This is the command procedure for the "checkmem" command, which
 *	causes the application to exit after printing information about
 *	memory usage to the file passed to this command as its first
 *	argument.
 *
 * Results:
 *	Returns a standard Tcl completion code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#ifdef TCL_MEM_DEBUG

	/* ARGSUSED */
static int
CheckmemCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Interpreter for evaluation. */
    int argc;				/* Number of arguments. */
    char *argv[];			/* String values of arguments. */
{
    extern char *tclMemDumpFileName;
    if (argc != 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" fileName\"", (char *) NULL);
	return TCL_ERROR;
    }
    strcpy(dumpFile, argv[1]);
    tclMemDumpFileName = dumpFile;
    quitFlag = 1;
    return TCL_OK;
}
#endif
