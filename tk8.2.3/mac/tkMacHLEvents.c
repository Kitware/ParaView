/* 
 * tkMacHLEvents.c --
 *
 *	Implements high level event support for the Macintosh.  Currently, 
 *	the only event that really does anything is the Quit event.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tcl.h"
#include "tclMacInt.h"
#include "tkMacInt.h"

#include <Aliases.h>
#include <AppleEvents.h>
#include <SegLoad.h>
#include <ToolUtils.h>

/*
 * This is a Tcl_Event structure that the Quit AppleEvent handler
 * uses to schedule the tkReallyKillMe function.
 */
 
typedef struct KillEvent {
    Tcl_Event header;		/* Information that is standard for
				 * all events. */
    Tcl_Interp *interp;		/* Interp that was passed to the
                                 * Quit AppleEvent */
} KillEvent;

/*
 * Static functions used only in this file.
 */

static pascal OSErr QuitHandler _ANSI_ARGS_((AppleEvent* event,
	AppleEvent* reply, long refcon));
static pascal OSErr OappHandler _ANSI_ARGS_((AppleEvent* event,
	AppleEvent* reply, long refcon));
static pascal OSErr OdocHandler _ANSI_ARGS_((AppleEvent* event,
	AppleEvent* reply, long refcon));
static pascal OSErr PrintHandler _ANSI_ARGS_((AppleEvent* event,
	AppleEvent* reply, long refcon));
static pascal OSErr ScriptHandler _ANSI_ARGS_((AppleEvent* event,
	AppleEvent* reply, long refcon));
static int MissedAnyParameters _ANSI_ARGS_((AppleEvent *theEvent));
static int ReallyKillMe _ANSI_ARGS_((Tcl_Event *eventPtr, int flags));

/*
 *----------------------------------------------------------------------
 *
 * TkMacInitAppleEvents --
 *
 *	Initilize the Apple Events on the Macintosh.  This registers the
 *	core event handlers.
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
TkMacInitAppleEvents(
    Tcl_Interp *interp)		/* Interp to handle basic events. */
{
    OSErr err;
    AEEventHandlerUPP	OappHandlerUPP, OdocHandlerUPP,
	PrintHandlerUPP, QuitHandlerUPP, ScriptHandlerUPP;
	
    /*
     * Install event handlers for the core apple events.
     */
    QuitHandlerUPP = NewAEEventHandlerProc(QuitHandler);
    err = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
	    QuitHandlerUPP, (long) interp, false);

    OappHandlerUPP = NewAEEventHandlerProc(OappHandler);
    err = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
	    OappHandlerUPP, (long) interp, false);

    OdocHandlerUPP = NewAEEventHandlerProc(OdocHandler);
    err = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
	    OdocHandlerUPP, (long) interp, false);

    PrintHandlerUPP = NewAEEventHandlerProc(PrintHandler);
    err = AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,
	    PrintHandlerUPP, (long) interp, false);

    if (interp != NULL) {
	ScriptHandlerUPP = NewAEEventHandlerProc(ScriptHandler);
	err = AEInstallEventHandler('misc', 'dosc',
	    ScriptHandlerUPP, (long) interp, false);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacDoHLEvent --
 *
 *	Dispatch incomming highlevel events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the incoming event.
 *
 *----------------------------------------------------------------------
 */

void
TkMacDoHLEvent(
    EventRecord *theEvent)
{
    AEProcessAppleEvent(theEvent);

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * QuitHandler, OappHandler, etc. --
 *
 *	These are the core Apple event handlers.  Only the Quit event does
 *	anything interesting.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static pascal OSErr
QuitHandler(
    AppleEvent *theAppleEvent,
    AppleEvent *reply,
    long handlerRefcon)
{
    Tcl_Interp 	*interp = (Tcl_Interp *) handlerRefcon;
    KillEvent *eventPtr;
    
    /*
     * Call the exit command from the event loop, since you are not supposed
     * to call ExitToShell in an Apple Event Handler.  We put this at the head
     * of Tcl's event queue because this message usually comes when the Mac is
     * shutting down, and we want to kill the shell as quickly as possible.
     */
    
    eventPtr = (KillEvent *) ckalloc(sizeof(KillEvent));
    eventPtr->header.proc = ReallyKillMe;
    eventPtr->interp = interp;
     
    Tcl_QueueEvent((Tcl_Event *) eventPtr, TCL_QUEUE_HEAD);

    return noErr;
}

static pascal OSErr
OappHandler(
    AppleEvent *theAppleEvent,
    AppleEvent *reply,
    long handlerRefcon)
{
    return noErr;
}

static pascal OSErr
OdocHandler(
    AppleEvent *theAppleEvent,
    AppleEvent *reply,
    long handlerRefcon)
{
    Tcl_Interp 	*interp = (Tcl_Interp *) handlerRefcon;
    AEDescList fileSpecList;
    FSSpec file;
    OSErr err;
    DescType type;
    Size actual;
    long count;
    AEKeyword keyword;
    long index;
    Tcl_DString command;
    Tcl_DString pathName;
    Tcl_CmdInfo dummy;

    /*
     * Don't bother if we don't have an interp or
     * the open document procedure doesn't exist.
     */

    if ((interp == NULL) || 
    	(Tcl_GetCommandInfo(interp, "tkOpenDocument", &dummy)) == 0) {
    	return noErr;
    }
    
    /*
     * If we get any errors wil retrieving our parameters
     * we just return with no error.
     */

    err = AEGetParamDesc(theAppleEvent, keyDirectObject,
	    typeAEList, &fileSpecList);
    if (err != noErr) {
	return noErr;
    }

    err = MissedAnyParameters(theAppleEvent);
    if (err != noErr) {
	return noErr;
    }

    err = AECountItems(&fileSpecList, &count);
    if (err != noErr) {
	return noErr;
    }

    Tcl_DStringInit(&command);
    Tcl_DStringAppend(&command, "tkOpenDocument", -1);
    for (index = 1; index <= count; index++) {
	int length;
	Handle fullPath;
	
	err = AEGetNthPtr(&fileSpecList, index, typeFSS,
		&keyword, &type, (Ptr) &file, sizeof(FSSpec), &actual);
	if ( err != noErr ) {
	    continue;
	}

	err = FSpPathFromLocation(&file, &length, &fullPath);
	HLock(fullPath);
        Tcl_ExternalToUtfDString(NULL, *fullPath, length, &pathName);
	HUnlock(fullPath);
	DisposeHandle(fullPath);

	Tcl_DStringAppendElement(&command, Tcl_DStringValue(&pathName));
	Tcl_DStringFree(&pathName);
    }
    
    Tcl_GlobalEval(interp, Tcl_DStringValue(&command));

    Tcl_DStringFree(&command);
    return noErr;
}

static pascal OSErr
PrintHandler(
    AppleEvent *theAppleEvent,
    AppleEvent *reply,
    long handlerRefcon)
{
    return noErr;
}

/*
 *----------------------------------------------------------------------
 *
 * DoScriptHandler --
 *
 *	This handler process the do script event.  
 *
 * Results:
 *	Scedules the given event to be processed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
 
static pascal OSErr 
ScriptHandler(
    AppleEvent *theAppleEvent,
    AppleEvent *reply,
    long handlerRefcon)
{
    OSErr theErr;
    AEDescList theDesc;
    int tclErr = -1;
    Tcl_Interp *interp;
    char errString[128];

    interp = (Tcl_Interp *) handlerRefcon;

    /*
     * The do script event receives one parameter that should be data or a file.
     */
    theErr = AEGetParamDesc(theAppleEvent, keyDirectObject, typeWildCard,
	    &theDesc);
    if (theErr != noErr) {
	sprintf(errString, "AEDoScriptHandler: GetParamDesc error %d", theErr);
	theErr = AEPutParamPtr(reply, keyErrorString, typeChar, errString,
		strlen(errString));
    } else if (MissedAnyParameters(theAppleEvent)) {
	sprintf(errString, "AEDoScriptHandler: extra parameters");
	AEPutParamPtr(reply, keyErrorString, typeChar, errString,
		strlen(errString));
	theErr = -1771;
    } else {
	if (theDesc.descriptorType == (DescType)'TEXT') {
	    short length, i;
	    
	    length = GetHandleSize(theDesc.dataHandle);
	    SetHandleSize(theDesc.dataHandle, length + 1);
	    *(*theDesc.dataHandle + length) = '\0';
	    for (i=0; i<length; i++) {
		if ((*theDesc.dataHandle)[i] == '\r') {
		    (*theDesc.dataHandle)[i] = '\n';
		}
	    }

	    HLock(theDesc.dataHandle);
	    tclErr = Tcl_GlobalEval(interp, *theDesc.dataHandle);
	    HUnlock(theDesc.dataHandle);
	} else if (theDesc.descriptorType == (DescType)'alis') {
	    Boolean dummy;
	    FSSpec theFSS;
	    Handle fullPath;
	    int length;
	    
	    theErr = ResolveAlias(NULL, (AliasHandle)theDesc.dataHandle,
		    &theFSS, &dummy);
	    if (theErr == noErr) {
		FSpPathFromLocation(&theFSS, &length, &fullPath);
		HLock(fullPath);
		Tcl_EvalFile(interp, *fullPath);
		HUnlock(fullPath);
		DisposeHandle(fullPath);
	    } else {
		sprintf(errString, "AEDoScriptHandler: file not found");
		AEPutParamPtr(reply, keyErrorString, typeChar,
			errString, strlen(errString));
	    }
	} else {
	    sprintf(errString,
		    "AEDoScriptHandler: invalid script type '%-4.4s', must be 'alis' or 'TEXT'",
		    &theDesc.descriptorType);
	    AEPutParamPtr(reply, keyErrorString, typeChar,
		    errString, strlen(errString));
	    theErr = -1770;
	}
    }

    /*
     * If we actually go to run Tcl code - put the result in the reply.
     */
    if (tclErr >= 0) {
	if (tclErr == TCL_OK)  {
	    AEPutParamPtr(reply, keyDirectObject, typeChar,
		Tcl_GetStringResult(interp),
		strlen(Tcl_GetStringResult(interp)));
	} else {
	    AEPutParamPtr(reply, keyErrorString, typeChar,
		Tcl_GetStringResult(interp),
		strlen(Tcl_GetStringResult(interp)));
	    AEPutParamPtr(reply, keyErrorNumber, typeInteger,
		(Ptr) &tclErr, sizeof(int));
	}
    }
	
    AEDisposeDesc(&theDesc);

    return theErr;
}

/*
 *----------------------------------------------------------------------
 *
 * ReallyKillMe --
 *
 *	This proc tries to kill the shell by running exit, and if that 
 *      has not succeeded (e.g. because someone has renamed the exit 
 *      command), calls Tcl_Exit to really kill the shell.  Called from 
 *      an event scheduled by the "Quit" AppleEvent handler.
 *
 * Results:
 *	Kills the shell.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int 
ReallyKillMe(Tcl_Event *eventPtr, int flags) 
{
    Tcl_Interp *interp = ((KillEvent *) eventPtr)->interp;
    if (interp != NULL) {
        Tcl_GlobalEval(interp, "exit");
    }
    Tcl_Exit(0);
    
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * MissedAnyParameters --
 *
 *	Checks to see if parameters are still left in the event.  
 *
 * Results:
 *	True or false.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
 
static int 
MissedAnyParameters(
    AppleEvent *theEvent)
{
   DescType returnedType;
   Size actualSize;
   OSErr err;

   err = AEGetAttributePtr(theEvent, keyMissedKeywordAttr, typeWildCard, 
   		&returnedType, NULL, 0, &actualSize);
   
   return (err != errAEDescNotFound);
}
