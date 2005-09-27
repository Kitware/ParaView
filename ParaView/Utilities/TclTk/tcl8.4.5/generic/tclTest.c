/* 
 * tclTest.c --
 *
 *	This file contains C command procedures for a bunch of additional
 *	Tcl commands that are used for testing out Tcl's C interfaces.
 *	These commands are not normally included in Tcl applications;
 *	they're only used for testing.
 *
 * Copyright (c) 1993-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 Ajuba Solutions.
 * Copyright (c) 2003 by Kevin B. Kenny.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#define TCL_TEST
#include "tclInt.h"
#include "tclPort.h"

/*
 * Required for Testregexp*Cmd
 */
#include "tclRegexp.h"

/*
 * Required for TestlocaleCmd
 */
#include <locale.h>

/*
 * Required for the TestChannelCmd and TestChannelEventCmd
 */
#include "tclIO.h"

/*
 * Declare external functions used in Windows tests.
 */

/*
 * Dynamic string shared by TestdcallCmd and DelCallbackProc;  used
 * to collect the results of the various deletion callbacks.
 */

static Tcl_DString delString;
static Tcl_Interp *delInterp;

/*
 * One of the following structures exists for each asynchronous
 * handler created by the "testasync" command".
 */

typedef struct TestAsyncHandler {
    int id;				/* Identifier for this handler. */
    Tcl_AsyncHandler handler;		/* Tcl's token for the handler. */
    char *command;			/* Command to invoke when the
					 * handler is invoked. */
    struct TestAsyncHandler *nextPtr;	/* Next is list of handlers. */
} TestAsyncHandler;

static TestAsyncHandler *firstHandler = NULL;

/*
 * The dynamic string below is used by the "testdstring" command
 * to test the dynamic string facilities.
 */

static Tcl_DString dstring;

/*
 * The command trace below is used by the "testcmdtraceCmd" command
 * to test the command tracing facilities.
 */

static Tcl_Trace cmdTrace;

/*
 * One of the following structures exists for each command created
 * by TestdelCmd:
 */

typedef struct DelCmd {
    Tcl_Interp *interp;		/* Interpreter in which command exists. */
    char *deleteCmd;		/* Script to execute when command is
				 * deleted.  Malloc'ed. */
} DelCmd;

/*
 * The following is used to keep track of an encoding that invokes a Tcl
 * command. 
 */

typedef struct TclEncoding {
    Tcl_Interp *interp;
    char *toUtfCmd;
    char *fromUtfCmd;
} TclEncoding;

/*
 * The counter below is used to determine if the TestsaveresultFree
 * routine was called for a result.
 */

static int freeCount;

/*
 * Boolean flag used by the "testsetmainloop" and "testexitmainloop"
 * commands.
 */
static int exitMainLoop = 0;

/*
 * Event structure used in testing the event queue management procedures.
 */
typedef struct TestEvent {
    Tcl_Event header;		/* Header common to all events */
    Tcl_Interp* interp;		/* Interpreter that will handle the event */
    Tcl_Obj* command;		/* Command to evaluate when the event occurs */
    Tcl_Obj* tag;		/* Tag for this event used to delete it */
} TestEvent;

/*
 * Forward declarations for procedures defined later in this file:
 */

int			Tcltest_Init _ANSI_ARGS_((Tcl_Interp *interp));
static int		AsyncHandlerProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int code));
static void		CleanupTestSetassocdataTests _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp));
static void		CmdDelProc1 _ANSI_ARGS_((ClientData clientData));
static void		CmdDelProc2 _ANSI_ARGS_((ClientData clientData));
static int		CmdProc1 _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		CmdProc2 _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static void		CmdTraceDeleteProc _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp,
			    int level, char *command, Tcl_CmdProc *cmdProc,
			    ClientData cmdClientData, int argc,
			    char **argv));
static void		CmdTraceProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int level, char *command,
			    Tcl_CmdProc *cmdProc, ClientData cmdClientData,
                            int argc, char **argv));
static int		CreatedCommandProc _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp,
			    int argc, CONST char **argv));
static int		CreatedCommandProc2 _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp,
			    int argc, CONST char **argv));
static void		DelCallbackProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp));
static int		DelCmdProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static void		DelDeleteProc _ANSI_ARGS_((ClientData clientData));
static void		EncodingFreeProc _ANSI_ARGS_((ClientData clientData));
static int		EncodingToUtfProc _ANSI_ARGS_((ClientData clientData,
			    CONST char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst,
			    int dstLen, int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr));
static int		EncodingFromUtfProc _ANSI_ARGS_((ClientData clientData,
			    CONST char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst,
			    int dstLen, int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr));
static void		ExitProcEven _ANSI_ARGS_((ClientData clientData));
static void		ExitProcOdd _ANSI_ARGS_((ClientData clientData));
static int              GetTimesCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, CONST char **argv));
static void		MainLoop _ANSI_ARGS_((void));
static int              NoopCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, CONST char **argv));
static int              NoopObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int		ObjTraceProc _ANSI_ARGS_(( ClientData clientData,
						   Tcl_Interp* interp,
						   int level,
						   CONST char* command,
						   Tcl_Command commandToken,
						   int objc,
						   Tcl_Obj *CONST objv[] ));
static void		ObjTraceDeleteProc _ANSI_ARGS_(( ClientData ));
static void		PrintParse _ANSI_ARGS_((Tcl_Interp *interp,
						Tcl_Parse *parsePtr));
static void		SpecialFree _ANSI_ARGS_((char *blockPtr));
static int		StaticInitProc _ANSI_ARGS_((Tcl_Interp *interp));
static int		TestaccessprocCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		PretendTclpAccess _ANSI_ARGS_((CONST char *path,
			   int mode));
static int		TestAccessProc1 _ANSI_ARGS_((CONST char *path,
			   int mode));
static int		TestAccessProc2 _ANSI_ARGS_((CONST char *path,
			   int mode));
static int		TestAccessProc3 _ANSI_ARGS_((CONST char *path,
			   int mode));
static int		TestasyncCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestcmdinfoCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestcmdtokenCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestcmdtraceCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestchmodCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestcreatecommandCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestdcallCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestdelCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestdelassocdataCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestdstringCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestencodingObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[]));
static int		TestevalexObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[]));
static int		TestevalobjvObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[]));
static int		TesteventObjCmd _ANSI_ARGS_((ClientData unused,
						     Tcl_Interp* interp,
						     int argc,
						     Tcl_Obj *CONST objv[]));
static int		TesteventProc _ANSI_ARGS_((Tcl_Event* event,
						   int flags));
static int		TesteventDeleteProc _ANSI_ARGS_((
			    Tcl_Event* event,
			    ClientData clientData));
static int		TestexithandlerCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestexprlongCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestexprparserObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int		TestexprstringCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestfileCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));
static int		TestfilelinkCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));
static int		TestfeventCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestgetassocdataCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestgetplatformCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestgetvarfullnameCmd _ANSI_ARGS_((
			    ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *CONST objv[]));
static int		TestinterpdeleteCmd _ANSI_ARGS_((ClientData dummy,
		            Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestlinkCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestlocaleCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int		TestMathFunc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, Tcl_Value *args,
			    Tcl_Value *resultPtr));
static int		TestMathFunc2 _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, Tcl_Value *args,
			    Tcl_Value *resultPtr));
static int		TestmainthreadCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestsetmainloopCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestexitmainloopCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static Tcl_Channel	PretendTclpOpenFileChannel _ANSI_ARGS_((
			    Tcl_Interp *interp, CONST char *fileName,
			    CONST char *modeString, int permissions));
static Tcl_Channel	TestOpenFileChannelProc1 _ANSI_ARGS_((
			    Tcl_Interp *interp, CONST char *fileName,
			    CONST char *modeString, int permissions));
static Tcl_Channel	TestOpenFileChannelProc2 _ANSI_ARGS_((
			    Tcl_Interp *interp, CONST char *fileName,
			    CONST char *modeString, int permissions));
static Tcl_Channel	TestOpenFileChannelProc3 _ANSI_ARGS_((
			    Tcl_Interp *interp, CONST char *fileName,
			    CONST char *modeString, int permissions));
static int		TestpanicCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestparserObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int		TestparsevarObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int		TestparsevarnameObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int		TestregexpObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static void		TestregexpXflags _ANSI_ARGS_((char *string,
			    int length, int *cflagsPtr, int *eflagsPtr));
static int		TestsaveresultCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static void		TestsaveresultFree _ANSI_ARGS_((char *blockPtr));
static int		TestsetassocdataCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestsetCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestsetobjerrorcodeCmd _ANSI_ARGS_((
			    ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *CONST objv[]));
static int		TestopenfilechannelprocCmd _ANSI_ARGS_((
			    ClientData dummy, Tcl_Interp *interp, int argc,
			    CONST char **argv));
static int		TestsetplatformCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TeststaticpkgCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		PretendTclpStat _ANSI_ARGS_((CONST char *path,
			    struct stat *buf));
static int		TestStatProc1 _ANSI_ARGS_((CONST char *path,
			    struct stat *buf));
static int		TestStatProc2 _ANSI_ARGS_((CONST char *path,
			    struct stat *buf));
static int		TestStatProc3 _ANSI_ARGS_((CONST char *path,
			    struct stat *buf));
static int		TeststatprocCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TesttranslatefilenameCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestupvarCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int              TestWrongNumArgsObjCmd _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *CONST objv[]));
static int              TestGetIndexFromObjStructObjCmd _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp,
			    int objc, Tcl_Obj *CONST objv[]));
static int		TestChannelCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, CONST char **argv));
static int		TestChannelEventCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, CONST char **argv));
/* Filesystem testing */

static int		TestFilesystemObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[]));
static int		TestSimpleFilesystemObjCmd _ANSI_ARGS_((
			    ClientData dummy, Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[]));

static void             TestReport _ANSI_ARGS_ ((CONST char* cmd, Tcl_Obj* arg1, 
			    Tcl_Obj* arg2));

static Tcl_Obj*         TestReportGetNativePath _ANSI_ARGS_ ((
			    Tcl_Obj* pathObjPtr));

static int		TestReportStat _ANSI_ARGS_ ((Tcl_Obj *path,
			    Tcl_StatBuf *buf));
static int		TestReportAccess _ANSI_ARGS_ ((Tcl_Obj *path,
			    int mode));
static Tcl_Channel	TestReportOpenFileChannel _ANSI_ARGS_ ((
			    Tcl_Interp *interp, Tcl_Obj *fileName,
			    int mode, int permissions));
static int		TestReportMatchInDirectory _ANSI_ARGS_ ((
			    Tcl_Interp *interp, Tcl_Obj *resultPtr,
			    Tcl_Obj *dirPtr, CONST char *pattern,
			    Tcl_GlobTypeData *types));
static int		TestReportChdir _ANSI_ARGS_ ((Tcl_Obj *dirName));
static int		TestReportLstat _ANSI_ARGS_ ((Tcl_Obj *path,
			    Tcl_StatBuf *buf));
static int		TestReportCopyFile _ANSI_ARGS_ ((Tcl_Obj *src,
			    Tcl_Obj *dst));
static int		TestReportDeleteFile _ANSI_ARGS_ ((Tcl_Obj *path));
static int		TestReportRenameFile _ANSI_ARGS_ ((Tcl_Obj *src,
			    Tcl_Obj *dst));
static int		TestReportCreateDirectory _ANSI_ARGS_ ((Tcl_Obj *path));
static int		TestReportCopyDirectory _ANSI_ARGS_ ((Tcl_Obj *src,
			    Tcl_Obj *dst, Tcl_Obj **errorPtr));
static int		TestReportRemoveDirectory _ANSI_ARGS_ ((Tcl_Obj *path,
			    int recursive, Tcl_Obj **errorPtr));
static int		TestReportLoadFile _ANSI_ARGS_ ((Tcl_Interp *interp,
			    Tcl_Obj *fileName, 
			    Tcl_LoadHandle *handlePtr,
			    Tcl_FSUnloadFileProc **unloadProcPtr));
static Tcl_Obj *	TestReportLink _ANSI_ARGS_ ((Tcl_Obj *path,
			    Tcl_Obj *to, int linkType));
static CONST char**	TestReportFileAttrStrings _ANSI_ARGS_ ((
			    Tcl_Obj *fileName, Tcl_Obj **objPtrRef));
static int		TestReportFileAttrsGet _ANSI_ARGS_ ((Tcl_Interp *interp,
			    int index, Tcl_Obj *fileName, Tcl_Obj **objPtrRef));
static int		TestReportFileAttrsSet _ANSI_ARGS_ ((Tcl_Interp *interp,
			    int index, Tcl_Obj *fileName, Tcl_Obj *objPtr));
static int		TestReportUtime _ANSI_ARGS_ ((Tcl_Obj *fileName,
			    struct utimbuf *tval));
static int		TestReportNormalizePath _ANSI_ARGS_ ((
			    Tcl_Interp *interp, Tcl_Obj *pathPtr,
			    int nextCheckpoint));
static int		TestReportInFilesystem _ANSI_ARGS_ ((Tcl_Obj *pathPtr, ClientData *clientDataPtr));
static void		TestReportFreeInternalRep _ANSI_ARGS_ ((ClientData clientData));
static ClientData	TestReportDupInternalRep _ANSI_ARGS_ ((ClientData clientData));

static int		SimpleStat _ANSI_ARGS_ ((Tcl_Obj *path,
			    Tcl_StatBuf *buf));
static int		SimpleAccess _ANSI_ARGS_ ((Tcl_Obj *path,
			    int mode));
static Tcl_Channel	SimpleOpenFileChannel _ANSI_ARGS_ ((
			    Tcl_Interp *interp, Tcl_Obj *fileName,
			    int mode, int permissions));
static Tcl_Obj*         SimpleListVolumes _ANSI_ARGS_ ((void));
static int              SimplePathInFilesystem _ANSI_ARGS_ ((
			    Tcl_Obj *pathPtr, ClientData *clientDataPtr));
static Tcl_Obj*         SimpleCopy _ANSI_ARGS_ ((Tcl_Obj *pathPtr));
static int              TestNumUtfCharsCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));

static Tcl_Filesystem testReportingFilesystem = {
    "reporting",
    sizeof(Tcl_Filesystem),
    TCL_FILESYSTEM_VERSION_1,
    &TestReportInFilesystem, /* path in */
    &TestReportDupInternalRep,
    &TestReportFreeInternalRep,
    NULL, /* native to norm */
    NULL, /* convert to native */
    &TestReportNormalizePath,
    NULL, /* path type */
    NULL, /* separator */
    &TestReportStat,
    &TestReportAccess,
    &TestReportOpenFileChannel,
    &TestReportMatchInDirectory,
    &TestReportUtime,
    &TestReportLink,
    NULL /* list volumes */,
    &TestReportFileAttrStrings,
    &TestReportFileAttrsGet,
    &TestReportFileAttrsSet,
    &TestReportCreateDirectory,
    &TestReportRemoveDirectory, 
    &TestReportDeleteFile,
    &TestReportCopyFile,
    &TestReportRenameFile,
    &TestReportCopyDirectory, 
    &TestReportLstat,
    &TestReportLoadFile,
    NULL /* cwd */,
    &TestReportChdir
};

static Tcl_Filesystem simpleFilesystem = {
    "simple",
    sizeof(Tcl_Filesystem),
    TCL_FILESYSTEM_VERSION_1,
    &SimplePathInFilesystem,
    NULL,
    NULL,
    /* No internal to normalized, since we don't create any
     * pure 'internal' Tcl_Obj path representations */
    NULL,
    /* No create native rep function, since we don't use it
     * or 'Tcl_FSNewNativePath' */
    NULL,
    /* Normalize path isn't needed - we assume paths only have
     * one representation */
    NULL,
    NULL,
    NULL,
    &SimpleStat,
    &SimpleAccess,
    &SimpleOpenFileChannel,
    NULL,
    NULL,
    /* We choose not to support symbolic links inside our vfs's */
    NULL,
    &SimpleListVolumes,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, 
    NULL,
    /* No copy file - fallback will occur at Tcl level */
    NULL,
    /* No rename file - fallback will occur at Tcl level */
    NULL,
    /* No copy directory - fallback will occur at Tcl level */
    NULL, 
    /* Use stat for lstat */
    NULL,
    /* No load - fallback on core implementation */
    NULL,
    /* We don't need a getcwd or chdir - fallback on Tcl's versions */
    NULL,
    NULL
};


/*
 * External (platform specific) initialization routine, these declarations
 * explicitly don't use EXTERN since this code does not get compiled
 * into the library:
 */

extern int		TclplatformtestInit _ANSI_ARGS_((Tcl_Interp *interp));
extern int		TclThread_Init _ANSI_ARGS_((Tcl_Interp *interp));

/*
 *----------------------------------------------------------------------
 *
 * Tcltest_Init --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in the interp's result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcltest_Init(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{
    Tcl_ValueType t3ArgTypes[2];

    Tcl_Obj *listPtr;
    Tcl_Obj **objv;
    int objc, index;
    static CONST char *specialOptions[] = {
	"-appinitprocerror", "-appinitprocdeleteinterp",
	"-appinitprocclosestderr", "-appinitprocsetrcfile", (char *) NULL
    };

    if (Tcl_PkgProvide(interp, "Tcltest", TCL_VERSION) == TCL_ERROR) {
        return TCL_ERROR;
    }

    /*
     * Create additional commands and math functions for testing Tcl.
     */

    Tcl_CreateCommand(interp, "gettimes", GetTimesCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "noop", NoopCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "noop", NoopObjCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testwrongnumargs", TestWrongNumArgsObjCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testfilesystem", TestFilesystemObjCmd, 
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testsimplefilesystem", TestSimpleFilesystemObjCmd, 
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testgetindexfromobjstruct",
			 TestGetIndexFromObjStructObjCmd, (ClientData) 0,
			 (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testaccessproc", TestaccessprocCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testasync", TestasyncCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testchannel", TestChannelCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testchannelevent", TestChannelEventCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testchmod", TestchmodCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testcmdtoken", TestcmdtokenCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testcmdinfo", TestcmdinfoCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testcmdtrace", TestcmdtraceCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testcreatecommand", TestcreatecommandCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testdcall", TestdcallCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testdel", TestdelCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testdelassocdata", TestdelassocdataCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_DStringInit(&dstring);
    Tcl_CreateCommand(interp, "testdstring", TestdstringCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testencoding", TestencodingObjCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testevalex", TestevalexObjCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testevalobjv", TestevalobjvObjCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand( interp, "testevent", TesteventObjCmd,
			  (ClientData) 0, (Tcl_CmdDeleteProc*) NULL );
    Tcl_CreateCommand(interp, "testexithandler", TestexithandlerCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testexprlong", TestexprlongCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testexprparser", TestexprparserObjCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testexprstring", TestexprstringCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testfevent", TestfeventCmd, (ClientData) 0,
            (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testfilelink", TestfilelinkCmd, 
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testfile", TestfileCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testgetassocdata", TestgetassocdataCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testgetplatform", TestgetplatformCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testgetvarfullname",
	    TestgetvarfullnameCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testinterpdelete", TestinterpdeleteCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testlink", TestlinkCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testlocale", TestlocaleCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testopenfilechannelproc",
    	    TestopenfilechannelprocCmd, (ClientData) 0, 
    	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testpanic", TestpanicCmd, (ClientData) 0,
            (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testparser", TestparserObjCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testparsevar", TestparsevarObjCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testparsevarname", TestparsevarnameObjCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testregexp", TestregexpObjCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testsaveresult", TestsaveresultCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testsetassocdata", TestsetassocdataCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testsetnoerr", TestsetCmd,
            (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testseterr", TestsetCmd,
            (ClientData) TCL_LEAVE_ERR_MSG, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testsetobjerrorcode", 
	    TestsetobjerrorcodeCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testnumutfchars",
	    TestNumUtfCharsCmd, (ClientData) 0, 
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testsetplatform", TestsetplatformCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "teststaticpkg", TeststaticpkgCmd,
	    (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testtranslatefilename",
            TesttranslatefilenameCmd, (ClientData) 0,
            (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testupvar", TestupvarCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateMathFunc(interp, "T1", 0, (Tcl_ValueType *) NULL, TestMathFunc,
	    (ClientData) 123);
    Tcl_CreateMathFunc(interp, "T2", 0, (Tcl_ValueType *) NULL, TestMathFunc,
	    (ClientData) 345);
    Tcl_CreateCommand(interp, "teststatproc", TeststatprocCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testmainthread", TestmainthreadCmd, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testsetmainloop", TestsetmainloopCmd,
	    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testexitmainloop", TestexitmainloopCmd,
	    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    t3ArgTypes[0] = TCL_EITHER;
    t3ArgTypes[1] = TCL_EITHER;
    Tcl_CreateMathFunc(interp, "T3", 2, t3ArgTypes, TestMathFunc2,
	    (ClientData) 0);

#ifdef TCL_THREADS
    if (TclThread_Init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
#endif

    /*
     * Check for special options used in ../tests/main.test
     */

    listPtr = Tcl_GetVar2Ex(interp, "argv", NULL, TCL_GLOBAL_ONLY);
    if (listPtr != NULL) {
        if (Tcl_ListObjGetElements(interp, listPtr, &objc, &objv) != TCL_OK) {
	    return TCL_ERROR;
        }
        if (objc && (Tcl_GetIndexFromObj(NULL, objv[0], specialOptions, NULL,
		TCL_EXACT, &index) == TCL_OK)) {
	    switch (index) {
	        case 0: {
		    return TCL_ERROR;
	        }
	        case 1: {
		    Tcl_DeleteInterp(interp);
		    return TCL_ERROR;
	        }
	        case 2: {
		    int mode;
		    Tcl_UnregisterChannel(interp, 
			    Tcl_GetChannel(interp, "stderr", &mode));
		    return TCL_ERROR;
	        }
	        case 3: {
		    if (objc-1) {
		        Tcl_SetVar2Ex(interp, "tcl_rcFileName", NULL,
			       objv[1], TCL_GLOBAL_ONLY);
		    }
		    return TCL_ERROR;
	        }
	    }
        }
    }
	
    /*
     * And finally add any platform specific test commands.
     */
    
    return TclplatformtestInit(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * TestasyncCmd --
 *
 *	This procedure implements the "testasync" command.  It is used
 *	to test the asynchronous handler facilities of Tcl.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates, deletes, and invokes handlers.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestasyncCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    TestAsyncHandler *asyncPtr, *prevPtr;
    int id, code;
    static int nextId = 1;
    char buf[TCL_INTEGER_SPACE];

    if (argc < 2) {
	wrongNumArgs:
	Tcl_SetResult(interp, "wrong # args", TCL_STATIC);
	return TCL_ERROR;
    }
    if (strcmp(argv[1], "create") == 0) {
	if (argc != 3) {
	    goto wrongNumArgs;
	}
	asyncPtr = (TestAsyncHandler *) ckalloc(sizeof(TestAsyncHandler));
	asyncPtr->id = nextId;
	nextId++;
	asyncPtr->handler = Tcl_AsyncCreate(AsyncHandlerProc,
		(ClientData) asyncPtr);
	asyncPtr->command = (char *) ckalloc((unsigned) (strlen(argv[2]) + 1));
	strcpy(asyncPtr->command, argv[2]);
	asyncPtr->nextPtr = firstHandler;
	firstHandler = asyncPtr;
	TclFormatInt(buf, asyncPtr->id);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    } else if (strcmp(argv[1], "delete") == 0) {
	if (argc == 2) {
	    while (firstHandler != NULL) {
		asyncPtr = firstHandler;
		firstHandler = asyncPtr->nextPtr;
		Tcl_AsyncDelete(asyncPtr->handler);
		ckfree(asyncPtr->command);
		ckfree((char *) asyncPtr);
	    }
	    return TCL_OK;
	}
	if (argc != 3) {
	    goto wrongNumArgs;
	}
	if (Tcl_GetInt(interp, argv[2], &id) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (prevPtr = NULL, asyncPtr = firstHandler; asyncPtr != NULL;
		prevPtr = asyncPtr, asyncPtr = asyncPtr->nextPtr) {
	    if (asyncPtr->id != id) {
		continue;
	    }
	    if (prevPtr == NULL) {
		firstHandler = asyncPtr->nextPtr;
	    } else {
		prevPtr->nextPtr = asyncPtr->nextPtr;
	    }
	    Tcl_AsyncDelete(asyncPtr->handler);
	    ckfree(asyncPtr->command);
	    ckfree((char *) asyncPtr);
	    break;
	}
    } else if (strcmp(argv[1], "mark") == 0) {
	if (argc != 5) {
	    goto wrongNumArgs;
	}
	if ((Tcl_GetInt(interp, argv[2], &id) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[4], &code) != TCL_OK)) {
	    return TCL_ERROR;
	}
	for (asyncPtr = firstHandler; asyncPtr != NULL;
		asyncPtr = asyncPtr->nextPtr) {
	    if (asyncPtr->id == id) {
		Tcl_AsyncMark(asyncPtr->handler);
		break;
	    }
	}
	Tcl_SetResult(interp, (char *)argv[3], TCL_VOLATILE);
	return code;
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be create, delete, int, or mark",
		(char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AsyncHandlerProc(clientData, interp, code)
    ClientData clientData;	/* Pointer to TestAsyncHandler structure. */
    Tcl_Interp *interp;		/* Interpreter in which command was
				 * executed, or NULL. */
    int code;			/* Current return code from command. */
{
    TestAsyncHandler *asyncPtr = (TestAsyncHandler *) clientData;
    CONST char *listArgv[4], *cmd;
    char string[TCL_INTEGER_SPACE];

    TclFormatInt(string, code);
    listArgv[0] = asyncPtr->command;
    listArgv[1] = Tcl_GetString(Tcl_GetObjResult(interp));
    listArgv[2] = string;
    listArgv[3] = NULL;
    cmd = Tcl_Merge(3, listArgv);
    if (interp != NULL) {
	code = Tcl_Eval(interp, cmd);
    } else {
	/*
	 * this should not happen, but by definition of how async
	 * handlers are invoked, it's possible.  Better error
	 * checking is needed here.
	 */
    }
    ckfree((char *)cmd);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TestcmdinfoCmd --
 *
 *	This procedure implements the "testcmdinfo" command.  It is used
 *	to test Tcl_GetCommandInfo, Tcl_SetCommandInfo, and command creation
 *	and deletion.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates and deletes various commands and modifies their data.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestcmdinfoCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    Tcl_CmdInfo info;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option cmdName\"", (char *) NULL);
	return TCL_ERROR;
    }
    if (strcmp(argv[1], "create") == 0) {
	Tcl_CreateCommand(interp, argv[2], CmdProc1, (ClientData) "original",
		CmdDelProc1);
    } else if (strcmp(argv[1], "delete") == 0) {
	Tcl_DStringInit(&delString);
	Tcl_DeleteCommand(interp, argv[2]);
	Tcl_DStringResult(interp, &delString);
    } else if (strcmp(argv[1], "get") == 0) {
	if (Tcl_GetCommandInfo(interp, argv[2], &info) ==0) {
	    Tcl_SetResult(interp, "??", TCL_STATIC);
	    return TCL_OK;
	}
	if (info.proc == CmdProc1) {
	    Tcl_AppendResult(interp, "CmdProc1", " ",
		    (char *) info.clientData, (char *) NULL);
	} else if (info.proc == CmdProc2) {
	    Tcl_AppendResult(interp, "CmdProc2", " ",
		    (char *) info.clientData, (char *) NULL);
	} else {
	    Tcl_AppendResult(interp, "unknown", (char *) NULL);
	}
	if (info.deleteProc == CmdDelProc1) {
	    Tcl_AppendResult(interp, " CmdDelProc1", " ",
		    (char *) info.deleteData, (char *) NULL);
	} else if (info.deleteProc == CmdDelProc2) {
	    Tcl_AppendResult(interp, " CmdDelProc2", " ",
		    (char *) info.deleteData, (char *) NULL);
	} else {
	    Tcl_AppendResult(interp, " unknown", (char *) NULL);
	}
	Tcl_AppendResult(interp, " ", info.namespacePtr->fullName,
	        (char *) NULL);
	if (info.isNativeObjectProc) {
	    Tcl_AppendResult(interp, " nativeObjectProc", (char *) NULL);
	} else {
	    Tcl_AppendResult(interp, " stringProc", (char *) NULL);
	}
    } else if (strcmp(argv[1], "modify") == 0) {
	info.proc = CmdProc2;
	info.clientData = (ClientData) "new_command_data";
	info.objProc = NULL;
        info.objClientData = (ClientData) NULL;
	info.deleteProc = CmdDelProc2;
	info.deleteData = (ClientData) "new_delete_data";
	if (Tcl_SetCommandInfo(interp, argv[2], &info) == 0) {
	    Tcl_SetResult(interp, "0", TCL_STATIC);
	} else {
	    Tcl_SetResult(interp, "1", TCL_STATIC);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be create, delete, get, or modify",
		(char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

	/*ARGSUSED*/
static int
CmdProc1(clientData, interp, argc, argv)
    ClientData clientData;		/* String to return. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    Tcl_AppendResult(interp, "CmdProc1 ", (char *) clientData,
	    (char *) NULL);
    return TCL_OK;
}

	/*ARGSUSED*/
static int
CmdProc2(clientData, interp, argc, argv)
    ClientData clientData;		/* String to return. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    Tcl_AppendResult(interp, "CmdProc2 ", (char *) clientData,
	    (char *) NULL);
    return TCL_OK;
}

static void
CmdDelProc1(clientData)
    ClientData clientData;		/* String to save. */
{
    Tcl_DStringInit(&delString);
    Tcl_DStringAppend(&delString, "CmdDelProc1 ", -1);
    Tcl_DStringAppend(&delString, (char *) clientData, -1);
}

static void
CmdDelProc2(clientData)
    ClientData clientData;		/* String to save. */
{
    Tcl_DStringInit(&delString);
    Tcl_DStringAppend(&delString, "CmdDelProc2 ", -1);
    Tcl_DStringAppend(&delString, (char *) clientData, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TestcmdtokenCmd --
 *
 *	This procedure implements the "testcmdtoken" command. It is used
 *	to test Tcl_Command tokens and procedures such as
 *	Tcl_GetCommandFullName.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates and deletes various commands and modifies their data.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestcmdtokenCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    Tcl_Command token;
    int *l;
    char buf[30];

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option arg\"", (char *) NULL);
	return TCL_ERROR;
    }
    if (strcmp(argv[1], "create") == 0) {
	token = Tcl_CreateCommand(interp, argv[2], CmdProc1,
		(ClientData) "original", (Tcl_CmdDeleteProc *) NULL);
	sprintf(buf, "%p", (VOID *)token);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    } else if (strcmp(argv[1], "name") == 0) {
	Tcl_Obj *objPtr;

	if (sscanf(argv[2], "%p", &l) != 1) {
	    Tcl_AppendResult(interp, "bad command token \"", argv[2],
		    "\"", (char *) NULL);
	    return TCL_ERROR;
	}

	objPtr = Tcl_NewObj();
	Tcl_GetCommandFullName(interp, (Tcl_Command) l, objPtr);

	Tcl_AppendElement(interp,
	        Tcl_GetCommandName(interp, (Tcl_Command) l));
	Tcl_AppendElement(interp, Tcl_GetString(objPtr));
	Tcl_DecrRefCount(objPtr);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be create or name", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestcmdtraceCmd --
 *
 *	This procedure implements the "testcmdtrace" command. It is used
 *	to test Tcl_CreateTrace and Tcl_DeleteTrace.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates and deletes a command trace, and tests the invocation of
 *	a procedure by the command trace.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestcmdtraceCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    Tcl_DString buffer;
    int result;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option script\"", (char *) NULL);
	return TCL_ERROR;
    }

    if (strcmp(argv[1], "tracetest") == 0) {
	Tcl_DStringInit(&buffer);
	cmdTrace = Tcl_CreateTrace(interp, 50000,
	        (Tcl_CmdTraceProc *) CmdTraceProc, (ClientData) &buffer);
	result = Tcl_Eval(interp, argv[2]);
	if (result == TCL_OK) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, Tcl_DStringValue(&buffer), NULL);
	}
	Tcl_DeleteTrace(interp, cmdTrace);
	Tcl_DStringFree(&buffer);
    } else if (strcmp(argv[1], "deletetest") == 0) {
	/*
	 * Create a command trace then eval a script to check whether it is
	 * called. Note that this trace procedure removes itself as a
	 * further check of the robustness of the trace proc calling code in
	 * TclExecuteByteCode.
	 */
	
	cmdTrace = Tcl_CreateTrace(interp, 50000,
	        (Tcl_CmdTraceProc *) CmdTraceDeleteProc, (ClientData) NULL);
	Tcl_Eval(interp, argv[2]);
    } else if ( strcmp(argv[1], "resulttest" ) == 0 ) {
	/* Create an object-based trace, then eval a script. This is used
	 * to test return codes other than TCL_OK from the trace engine.
	 */
	static int deleteCalled;
	deleteCalled = 0;
	cmdTrace = Tcl_CreateObjTrace( interp, 50000,
				       TCL_ALLOW_INLINE_COMPILATION,
				       ObjTraceProc,
				       (ClientData) &deleteCalled,
				       ObjTraceDeleteProc );
	result = Tcl_Eval( interp, argv[ 2 ] );
	Tcl_DeleteTrace( interp, cmdTrace );
	if ( !deleteCalled ) {
	    Tcl_SetResult( interp, "Delete wasn't called", TCL_STATIC );
	    return TCL_ERROR;
	} else {
	    return result;
	}
	
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
			 "\": must be tracetest, deletetest or resulttest",
			 (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static void
CmdTraceProc(clientData, interp, level, command, cmdProc, cmdClientData,
        argc, argv)
    ClientData clientData;	/* Pointer to buffer in which the
				 * command and arguments are appended.
				 * Accumulates test result. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int level;			/* Current trace level. */
    char *command;		/* The command being traced (after
				 * substitutions). */
    Tcl_CmdProc *cmdProc;	/* Points to command's command procedure. */
    ClientData cmdClientData;	/* Client data associated with command
				 * procedure. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tcl_DString *bufPtr = (Tcl_DString *) clientData;
    int i;

    Tcl_DStringAppendElement(bufPtr, command);

    Tcl_DStringStartSublist(bufPtr);
    for (i = 0;  i < argc;  i++) {
	Tcl_DStringAppendElement(bufPtr, argv[i]);
    }
    Tcl_DStringEndSublist(bufPtr);
}

static void
CmdTraceDeleteProc(clientData, interp, level, command, cmdProc,
	cmdClientData, argc, argv)
    ClientData clientData;	/* Unused. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int level;			/* Current trace level. */
    char *command;		/* The command being traced (after
				 * substitutions). */
    Tcl_CmdProc *cmdProc;	/* Points to command's command procedure. */
    ClientData cmdClientData;	/* Client data associated with command
				 * procedure. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    /*
     * Remove ourselves to test whether calling Tcl_DeleteTrace within
     * a trace callback causes the for loop in TclExecuteByteCode that
     * calls traces to reference freed memory.
     */
    
    Tcl_DeleteTrace(interp, cmdTrace);
}

static int
ObjTraceProc( clientData, interp, level, command, token, objc, objv )
    ClientData clientData;	/* unused */
    Tcl_Interp* interp;		/* Tcl interpreter */
    int level;			/* Execution level */
    CONST char* command;	/* Command being executed */
    Tcl_Command token;		/* Command information */
    int objc;			/* Parameter count */
    Tcl_Obj *CONST objv[];	/* Parameter list */
{
    CONST char* word = Tcl_GetString( objv[ 0 ] );
    if ( !strcmp( word, "Error" ) ) {
	Tcl_SetObjResult( interp, Tcl_NewStringObj( command, -1 ) );
	return TCL_ERROR;
    } else if ( !strcmp( word, "Break" ) ) {
	return TCL_BREAK;
    } else if ( !strcmp( word, "Continue" ) ) {
	return TCL_CONTINUE;
    } else if ( !strcmp( word, "Return" ) ) {
	return TCL_RETURN;
    } else if ( !strcmp( word, "OtherStatus" ) ) {
	return 6;
    } else {
	return TCL_OK;
    }
}

static void
ObjTraceDeleteProc( clientData )
    ClientData clientData;
{
    int * intPtr = (int *) clientData;
    *intPtr = 1;		/* Record that the trace was deleted */
}

/*
 *----------------------------------------------------------------------
 *
 * TestcreatecommandCmd --
 *
 *	This procedure implements the "testcreatecommand" command. It is
 *	used to test that the Tcl_CreateCommand creates a new command in
 *	the namespace specified as part of its name, if any. It also
 *	checks that the namespace code ignore single ":"s in the middle
 *	or end of a command name.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates and deletes two commands ("test_ns_basic::createdcommand"
 *	and "value:at:").
 *
 *----------------------------------------------------------------------
 */

static int
TestcreatecommandCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    if (argc != 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option\"", (char *) NULL);
	return TCL_ERROR;
    }
    if (strcmp(argv[1], "create") == 0) {
	Tcl_CreateCommand(interp, "test_ns_basic::createdcommand",
		CreatedCommandProc, (ClientData) NULL,
		(Tcl_CmdDeleteProc *) NULL);
    } else if (strcmp(argv[1], "delete") == 0) {
	Tcl_DeleteCommand(interp, "test_ns_basic::createdcommand");
    } else if (strcmp(argv[1], "create2") == 0) {
	Tcl_CreateCommand(interp, "value:at:",
		CreatedCommandProc2, (ClientData) NULL,
		(Tcl_CmdDeleteProc *) NULL);
    } else if (strcmp(argv[1], "delete2") == 0) {
	Tcl_DeleteCommand(interp, "value:at:");
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be create, delete, create2, or delete2",
		(char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
CreatedCommandProc(clientData, interp, argc, argv)
    ClientData clientData;		/* String to return. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    Tcl_CmdInfo info;
    int found;

    found = Tcl_GetCommandInfo(interp, "test_ns_basic::createdcommand",
	    &info);
    if (!found) {
	Tcl_AppendResult(interp, "CreatedCommandProc could not get command info for test_ns_basic::createdcommand",
	        (char *) NULL);
	return TCL_ERROR;
    }
    Tcl_AppendResult(interp, "CreatedCommandProc in ",
	    info.namespacePtr->fullName, (char *) NULL);
    return TCL_OK;
}

static int
CreatedCommandProc2(clientData, interp, argc, argv)
    ClientData clientData;		/* String to return. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    Tcl_CmdInfo info;
    int found;

    found = Tcl_GetCommandInfo(interp, "value:at:", &info);
    if (!found) {
	Tcl_AppendResult(interp, "CreatedCommandProc2 could not get command info for test_ns_basic::createdcommand",
	        (char *) NULL);
	return TCL_ERROR;
    }
    Tcl_AppendResult(interp, "CreatedCommandProc2 in ",
	    info.namespacePtr->fullName, (char *) NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestdcallCmd --
 *
 *	This procedure implements the "testdcall" command.  It is used
 *	to test Tcl_CallWhenDeleted.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates and deletes interpreters.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestdcallCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    int i, id;

    delInterp = Tcl_CreateInterp();
    Tcl_DStringInit(&delString);
    for (i = 1; i < argc; i++) {
	if (Tcl_GetInt(interp, argv[i], &id) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (id < 0) {
	    Tcl_DontCallWhenDeleted(delInterp, DelCallbackProc,
		    (ClientData) (-id));
	} else {
	    Tcl_CallWhenDeleted(delInterp, DelCallbackProc,
		    (ClientData) id);
	}
    }
    Tcl_DeleteInterp(delInterp);
    Tcl_DStringResult(interp, &delString);
    return TCL_OK;
}

/*
 * The deletion callback used by TestdcallCmd:
 */

static void
DelCallbackProc(clientData, interp)
    ClientData clientData;		/* Numerical value to append to
					 * delString. */
    Tcl_Interp *interp;			/* Interpreter being deleted. */
{
    int id = (int) clientData;
    char buffer[TCL_INTEGER_SPACE];

    TclFormatInt(buffer, id);
    Tcl_DStringAppendElement(&delString, buffer);
    if (interp != delInterp) {
	Tcl_DStringAppendElement(&delString, "bogus interpreter argument!");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TestdelCmd --
 *
 *	This procedure implements the "testdcall" command.  It is used
 *	to test Tcl_CallWhenDeleted.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates and deletes interpreters.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestdelCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    DelCmd *dPtr;
    Tcl_Interp *slave;

    if (argc != 4) {
	Tcl_SetResult(interp, "wrong # args", TCL_STATIC);
	return TCL_ERROR;
    }

    slave = Tcl_GetSlave(interp, argv[1]);
    if (slave == NULL) {
	return TCL_ERROR;
    }

    dPtr = (DelCmd *) ckalloc(sizeof(DelCmd));
    dPtr->interp = interp;
    dPtr->deleteCmd = (char *) ckalloc((unsigned) (strlen(argv[3]) + 1));
    strcpy(dPtr->deleteCmd, argv[3]);

    Tcl_CreateCommand(slave, argv[2], DelCmdProc, (ClientData) dPtr,
	    DelDeleteProc);
    return TCL_OK;
}

static int
DelCmdProc(clientData, interp, argc, argv)
    ClientData clientData;		/* String result to return. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    DelCmd *dPtr = (DelCmd *) clientData;

    Tcl_AppendResult(interp, dPtr->deleteCmd, (char *) NULL);
    ckfree(dPtr->deleteCmd);
    ckfree((char *) dPtr);
    return TCL_OK;
}

static void
DelDeleteProc(clientData)
    ClientData clientData;		/* String command to evaluate. */
{
    DelCmd *dPtr = (DelCmd *) clientData;

    Tcl_Eval(dPtr->interp, dPtr->deleteCmd);
    Tcl_ResetResult(dPtr->interp);
    ckfree(dPtr->deleteCmd);
    ckfree((char *) dPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TestdelassocdataCmd --
 *
 *	This procedure implements the "testdelassocdata" command. It is used
 *	to test Tcl_DeleteAssocData.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Deletes an association between a key and associated data from an
 *	interpreter.
 *
 *----------------------------------------------------------------------
 */

static int
TestdelassocdataCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
                " data_key\"", (char *) NULL);
        return TCL_ERROR;
    }
    Tcl_DeleteAssocData(interp, argv[1]);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestdstringCmd --
 *
 *	This procedure implements the "testdstring" command.  It is used
 *	to test the dynamic string facilities of Tcl.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates, deletes, and invokes handlers.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestdstringCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    int count;

    if (argc < 2) {
	wrongNumArgs:
	Tcl_SetResult(interp, "wrong # args", TCL_STATIC);
	return TCL_ERROR;
    }
    if (strcmp(argv[1], "append") == 0) {
	if (argc != 4) {
	    goto wrongNumArgs;
	}
	if (Tcl_GetInt(interp, argv[3], &count) != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_DStringAppend(&dstring, argv[2], count);
    } else if (strcmp(argv[1], "element") == 0) {
	if (argc != 3) {
	    goto wrongNumArgs;
	}
	Tcl_DStringAppendElement(&dstring, argv[2]);
    } else if (strcmp(argv[1], "end") == 0) {
	if (argc != 2) {
	    goto wrongNumArgs;
	}
	Tcl_DStringEndSublist(&dstring);
    } else if (strcmp(argv[1], "free") == 0) {
	if (argc != 2) {
	    goto wrongNumArgs;
	}
	Tcl_DStringFree(&dstring);
    } else if (strcmp(argv[1], "get") == 0) {
	if (argc != 2) {
	    goto wrongNumArgs;
	}
	Tcl_SetResult(interp, Tcl_DStringValue(&dstring), TCL_VOLATILE);
    } else if (strcmp(argv[1], "gresult") == 0) {
	if (argc != 3) {
	    goto wrongNumArgs;
	}
	if (strcmp(argv[2], "staticsmall") == 0) {
	    Tcl_SetResult(interp, "short", TCL_STATIC);
	} else if (strcmp(argv[2], "staticlarge") == 0) {
	    Tcl_SetResult(interp, "first0 first1 first2 first3 first4 first5 first6 first7 first8 first9\nsecond0 second1 second2 second3 second4 second5 second6 second7 second8 second9\nthird0 third1 third2 third3 third4 third5 third6 third7 third8 third9\nfourth0 fourth1 fourth2 fourth3 fourth4 fourth5 fourth6 fourth7 fourth8 fourth9\nfifth0 fifth1 fifth2 fifth3 fifth4 fifth5 fifth6 fifth7 fifth8 fifth9\nsixth0 sixth1 sixth2 sixth3 sixth4 sixth5 sixth6 sixth7 sixth8 sixth9\nseventh0 seventh1 seventh2 seventh3 seventh4 seventh5 seventh6 seventh7 seventh8 seventh9\n", TCL_STATIC);
	} else if (strcmp(argv[2], "free") == 0) {
	    Tcl_SetResult(interp, (char *) ckalloc(100), TCL_DYNAMIC);
	    strcpy(interp->result, "This is a malloc-ed string");
	} else if (strcmp(argv[2], "special") == 0) {
	    interp->result = (char *) ckalloc(100);
	    interp->result += 4;
	    interp->freeProc = SpecialFree;
	    strcpy(interp->result, "This is a specially-allocated string");
	} else {
	    Tcl_AppendResult(interp, "bad gresult option \"", argv[2],
		    "\": must be staticsmall, staticlarge, free, or special",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	Tcl_DStringGetResult(interp, &dstring);
    } else if (strcmp(argv[1], "length") == 0) {
	char buf[TCL_INTEGER_SPACE];
	
	if (argc != 2) {
	    goto wrongNumArgs;
	}
	TclFormatInt(buf, Tcl_DStringLength(&dstring));
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    } else if (strcmp(argv[1], "result") == 0) {
	if (argc != 2) {
	    goto wrongNumArgs;
	}
	Tcl_DStringResult(interp, &dstring);
    } else if (strcmp(argv[1], "trunc") == 0) {
	if (argc != 3) {
	    goto wrongNumArgs;
	}
	if (Tcl_GetInt(interp, argv[2], &count) != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_DStringTrunc(&dstring, count);
    } else if (strcmp(argv[1], "start") == 0) {
	if (argc != 2) {
	    goto wrongNumArgs;
	}
	Tcl_DStringStartSublist(&dstring);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be append, element, end, free, get, length, ",
		"result, trunc, or start", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * The procedure below is used as a special freeProc to test how well
 * Tcl_DStringGetResult handles freeProc's other than free.
 */

static void SpecialFree(blockPtr)
    char *blockPtr;			/* Block to free. */
{
    ckfree(blockPtr - 4);
}

/*
 *----------------------------------------------------------------------
 *
 * TestencodingCmd --
 *
 *	This procedure implements the "testencoding" command.  It is used
 *	to test the encoding package.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Load encodings.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestencodingObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Tcl_Encoding encoding;
    int index, length;
    char *string;
    TclEncoding *encodingPtr;
    static CONST char *optionStrings[] = {
	"create",	"delete",	"path",
	NULL
    };
    enum options {
	ENC_CREATE,	ENC_DELETE,	ENC_PATH
    };
    
    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
	case ENC_CREATE: {
	    Tcl_EncodingType type;

	    if (objc != 5) {
		return TCL_ERROR;
	    }
	    encodingPtr = (TclEncoding *) ckalloc(sizeof(TclEncoding));
	    encodingPtr->interp = interp;

	    string = Tcl_GetStringFromObj(objv[3], &length);
	    encodingPtr->toUtfCmd = (char *) ckalloc((unsigned) (length + 1));
	    memcpy(encodingPtr->toUtfCmd, string, (unsigned) length + 1);

	    string = Tcl_GetStringFromObj(objv[4], &length);
	    encodingPtr->fromUtfCmd = (char *) ckalloc((unsigned) (length + 1));
	    memcpy(encodingPtr->fromUtfCmd, string, (unsigned) (length + 1));

	    string = Tcl_GetStringFromObj(objv[2], &length);

	    type.encodingName = string;
	    type.toUtfProc = EncodingToUtfProc;
	    type.fromUtfProc = EncodingFromUtfProc;
	    type.freeProc = EncodingFreeProc;
	    type.clientData = (ClientData) encodingPtr;
	    type.nullSize = 1;

	    Tcl_CreateEncoding(&type);
	    break;
	}
	case ENC_DELETE: {
	    if (objc != 3) {
		return TCL_ERROR;
	    }
	    encoding = Tcl_GetEncoding(NULL, Tcl_GetString(objv[2]));
	    Tcl_FreeEncoding(encoding);
	    Tcl_FreeEncoding(encoding);
	    break;
	}
	case ENC_PATH: {
	    if (objc == 2) {
		Tcl_SetObjResult(interp, TclGetLibraryPath());
	    } else {
		TclSetLibraryPath(objv[2]);
	    }
	    break;
	}
    }
    return TCL_OK;
}
static int 
EncodingToUtfProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	srcReadPtr, dstWrotePtr, dstCharsPtr)
    ClientData clientData;	/* TclEncoding structure. */
    CONST char *src;		/* Source string in specified encoding. */
    int srcLen;			/* Source string length in bytes. */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Current state. */
    char *dst;			/* Output buffer. */
    int dstLen;			/* The maximum length of output buffer. */
    int *srcReadPtr;		/* Filled with number of bytes read. */
    int *dstWrotePtr;		/* Filled with number of bytes stored. */
    int *dstCharsPtr;		/* Filled with number of chars stored. */
{
    int len;
    TclEncoding *encodingPtr;

    encodingPtr = (TclEncoding *) clientData;
    Tcl_GlobalEval(encodingPtr->interp, encodingPtr->toUtfCmd);

    len = strlen(Tcl_GetStringResult(encodingPtr->interp));
    if (len > dstLen) {
	len = dstLen;
    }
    memcpy(dst, Tcl_GetStringResult(encodingPtr->interp), (unsigned) len);
    Tcl_ResetResult(encodingPtr->interp);

    *srcReadPtr = srcLen;
    *dstWrotePtr = len;
    *dstCharsPtr = len;
    return TCL_OK;
}
static int 
EncodingFromUtfProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	srcReadPtr, dstWrotePtr, dstCharsPtr)
    ClientData clientData;	/* TclEncoding structure. */
    CONST char *src;		/* Source string in specified encoding. */
    int srcLen;			/* Source string length in bytes. */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Current state. */
    char *dst;			/* Output buffer. */
    int dstLen;			/* The maximum length of output buffer. */
    int *srcReadPtr;		/* Filled with number of bytes read. */
    int *dstWrotePtr;		/* Filled with number of bytes stored. */
    int *dstCharsPtr;		/* Filled with number of chars stored. */
{
    int len;
    TclEncoding *encodingPtr;

    encodingPtr = (TclEncoding *) clientData;
    Tcl_GlobalEval(encodingPtr->interp, encodingPtr->fromUtfCmd);

    len = strlen(Tcl_GetStringResult(encodingPtr->interp));
    if (len > dstLen) {
	len = dstLen;
    }
    memcpy(dst, Tcl_GetStringResult(encodingPtr->interp), (unsigned) len);
    Tcl_ResetResult(encodingPtr->interp);

    *srcReadPtr = srcLen;
    *dstWrotePtr = len;
    *dstCharsPtr = len;
    return TCL_OK;
}
static void
EncodingFreeProc(clientData)
    ClientData clientData;	/* ClientData associated with type. */
{
    TclEncoding *encodingPtr;

    encodingPtr = (TclEncoding *) clientData;
    ckfree((char *) encodingPtr->toUtfCmd);
    ckfree((char *) encodingPtr->fromUtfCmd);
    ckfree((char *) encodingPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TestevalexObjCmd --
 *
 *	This procedure implements the "testevalex" command.  It is
 *	used to test Tcl_EvalEx.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestevalexObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    Interp *iPtr = (Interp *) interp;
    int code, oldFlags, length, flags;
    char *string;

    if (objc == 1) {
	/*
	 * The command was invoked with no arguments, so just toggle
	 * the flag that determines whether we use Tcl_EvalEx.
	 */

	if (iPtr->flags & USE_EVAL_DIRECT) {
	    iPtr->flags &= ~USE_EVAL_DIRECT;
	    Tcl_SetResult(interp, "disabling direct evaluation", TCL_STATIC);
	} else {
	    iPtr->flags |= USE_EVAL_DIRECT;
	    Tcl_SetResult(interp, "enabling direct evaluation", TCL_STATIC);
	}
	return TCL_OK;
    }

    flags = 0;
    if (objc == 3) {
	string = Tcl_GetStringFromObj(objv[2], &length);
	if (strcmp(string, "global") != 0) {
	    Tcl_AppendResult(interp, "bad value \"", string,
		    "\": must be global", (char *) NULL);
	    return TCL_ERROR;
	}
	flags = TCL_EVAL_GLOBAL;
    } else if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "script ?global?");
        return TCL_ERROR;
    }
    Tcl_SetResult(interp, "xxx", TCL_STATIC);

    /*
     * Note, we have to set the USE_EVAL_DIRECT flag in the interpreter
     * in addition to calling Tcl_EvalEx.  This is needed so that even nested
     * commands are evaluated directly.
     */

    oldFlags = iPtr->flags;
    iPtr->flags |= USE_EVAL_DIRECT;
    string = Tcl_GetStringFromObj(objv[1], &length);
    code = Tcl_EvalEx(interp, string, length, flags); 
    iPtr->flags = (iPtr->flags & ~USE_EVAL_DIRECT)
	    | (oldFlags & USE_EVAL_DIRECT);
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TestevalobjvObjCmd --
 *
 *	This procedure implements the "testevalobjv" command.  It is
 *	used to test Tcl_EvalObjv.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestevalobjvObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    int evalGlobal;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "global word ?word ...?");
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[1], &evalGlobal) != TCL_OK) {
	return TCL_ERROR;
    }
    return Tcl_EvalObjv(interp, objc-2, objv+2,
	    (evalGlobal) ? TCL_EVAL_GLOBAL : 0);
}

/*
 *----------------------------------------------------------------------
 *
 * TesteventObjCmd --
 *
 *	This procedure implements a 'testevent' command.  The command
 *	is used to test event queue management.
 *
 * The command takes two forms:
 *	- testevent queue name position script
 *		Queues an event at the given position in the queue, and
 *		associates a given name with it (the same name may be
 *		associated with multiple events). When the event comes
 *		to the head of the queue, executes the given script at
 *		global level in the current interp. The position may be
 *		one of 'head', 'tail' or 'mark'.
 *	- testevent delete name
 *		Deletes any events associated with the given name from
 *		the queue.
 *
 * Return value:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	Manipulates the event queue as directed.
 *
 *----------------------------------------------------------------------
 */

static int
TesteventObjCmd( ClientData unused,      /* Not used */
		 Tcl_Interp* interp,     /* Tcl interpreter */
		 int objc,               /* Parameter count */
		 Tcl_Obj *CONST objv[] ) /* Parameter vector */
{
    
    static CONST char* subcommands[] = { /* Possible subcommands */
	"queue",
	"delete",
	NULL
    };
    int subCmdIndex;		/* Index of the chosen subcommand */
    static CONST char* positions[] = { /* Possible queue positions */
	"head",
	"tail",
	"mark",
	NULL
    };
    int posIndex;		/* Index of the chosen position */
    static CONST int posNum[] = { /* Interpretation of the chosen position */
	TCL_QUEUE_HEAD,
	TCL_QUEUE_TAIL,
	TCL_QUEUE_MARK
    };
    TestEvent* ev;		/* Event to be queued */

    if ( objc < 2 ) {
	Tcl_WrongNumArgs( interp, 1, objv, "subcommand ?args?" );
	return TCL_ERROR;
    }
    if ( Tcl_GetIndexFromObj( interp, objv[1], subcommands, "subcommand",
			      TCL_EXACT, &subCmdIndex ) != TCL_OK ) {
	return TCL_ERROR;
    }
    switch ( subCmdIndex ) {
    case 0:			/* queue */
	if ( objc != 5 ) {
	    Tcl_WrongNumArgs( interp, 2, objv, "name position script" );
	    return TCL_ERROR;
	}
	if ( Tcl_GetIndexFromObj( interp, objv[3], positions,
				  "position specifier", TCL_EXACT,
				  &posIndex ) != TCL_OK ) {
	    return TCL_ERROR;
	}
	ev = (TestEvent*) ckalloc( sizeof( TestEvent ) );
	ev->header.proc = TesteventProc;
	ev->header.nextPtr = NULL;
	ev->interp = interp;
	ev->command = objv[ 4 ];
	Tcl_IncrRefCount( ev->command );
	ev->tag = objv[ 2 ];
	Tcl_IncrRefCount( ev->tag );
	Tcl_QueueEvent( (Tcl_Event*) ev, posNum[ posIndex ] );
	break;

    case 1:			/* delete */
	if ( objc != 3 ) {
	    Tcl_WrongNumArgs( interp, 2, objv, "name" );
	    return TCL_ERROR;
	}
	Tcl_DeleteEvents( TesteventDeleteProc, objv[ 2 ] );
	break;
    }

    return TCL_OK;

}

/*
 *----------------------------------------------------------------------
 *
 * TesteventProc --
 *
 *	Delivers a test event to the Tcl interpreter as part of event
 *	queue testing.
 * 
 * Results:
 *	Returns 1 if the event has been serviced, 0 otherwise.
 *
 * Side effects:
 *	Evaluates the event's callback script, so has whatever
 *	side effects the callback has.  The return value of the
 *	callback script becomes the return value of this function.
 *	If the callback script reports an error, it is reported as
 *	a background error.
 *
 *----------------------------------------------------------------------
 */

static int
TesteventProc( Tcl_Event* event, /* Event to deliver */
	       int flags )	/* Current flags for Tcl_ServiceEvent */
{
    TestEvent * ev = (TestEvent *) event;
    Tcl_Interp* interp = ev->interp;
    Tcl_Obj* command = ev->command;
    int result = Tcl_EvalObjEx( interp, command,
				TCL_EVAL_GLOBAL | TCL_EVAL_DIRECT );
    int retval;
    if ( result != TCL_OK ) {
	Tcl_AddErrorInfo( interp,
			  "    (command bound to \"testevent\" callback)" );
	Tcl_BackgroundError( interp );
	return 1;		/* Avoid looping on errors */
    }
    if ( Tcl_GetBooleanFromObj( interp,
				Tcl_GetObjResult( interp ),
				&retval ) != TCL_OK ) {
	Tcl_AddErrorInfo( interp, 
			  "    (return value from \"testevent\" callback)" );
	Tcl_BackgroundError( interp );
	return 1;
    }
    if ( retval ) {
	Tcl_DecrRefCount( ev->tag );
	Tcl_DecrRefCount( ev->command );
    }
	
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * TesteventDeleteProc --
 *
 *	Removes some set of events from the queue.
 *
 * This procedure is used as part of testing event queue management.
 *
 * Results:
 *	Returns 1 if a given event should be deleted, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TesteventDeleteProc( Tcl_Event* event, /* Event to examine */
		     ClientData clientData ) /* Tcl_Obj containing the name
					      * of the event(s) to remove */
{
    TestEvent* ev;		/* Event to examine */
    char* evNameStr;
    Tcl_Obj* targetName;	/* Name of the event(s) to delete */
    char* targetNameStr;

    if ( event->proc != TesteventProc ) {
	return 0;
    }
    targetName = (Tcl_Obj*) clientData;
    targetNameStr = (char*) Tcl_GetStringFromObj( targetName, NULL );
    ev = (TestEvent*) event;
    evNameStr = Tcl_GetStringFromObj( ev->tag, NULL );
    if ( strcmp( evNameStr, targetNameStr ) == 0 ) {
	Tcl_DecrRefCount( ev->tag );
	Tcl_DecrRefCount( ev->command );
	return 1;
    } else {
	return 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TestexithandlerCmd --
 *
 *	This procedure implements the "testexithandler" command. It is
 *	used to test Tcl_CreateExitHandler and Tcl_DeleteExitHandler.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestexithandlerCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    int value;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
                " create|delete value\"", (char *) NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[2], &value) != TCL_OK) {
	return TCL_ERROR;
    }
    if (strcmp(argv[1], "create") == 0) {
	Tcl_CreateExitHandler((value & 1) ? ExitProcOdd : ExitProcEven,
		(ClientData) value);
    } else if (strcmp(argv[1], "delete") == 0) {
	Tcl_DeleteExitHandler((value & 1) ? ExitProcOdd : ExitProcEven,
		(ClientData) value);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be create or delete", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static void
ExitProcOdd(clientData)
    ClientData clientData;		/* Integer value to print. */
{
    char buf[16 + TCL_INTEGER_SPACE];

    sprintf(buf, "odd %d\n", (int) clientData);
    write(1, buf, strlen(buf));
}

static void
ExitProcEven(clientData)
    ClientData clientData;		/* Integer value to print. */
{
    char buf[16 + TCL_INTEGER_SPACE];

    sprintf(buf, "even %d\n", (int) clientData);
    write(1, buf, strlen(buf));
}

/*
 *----------------------------------------------------------------------
 *
 * TestexprlongCmd --
 *
 *	This procedure verifies that Tcl_ExprLong does not modify the
 *	interpreter result if there is no error.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestexprlongCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    long exprResult;
    char buf[4 + TCL_INTEGER_SPACE];
    int result;
    
    Tcl_SetResult(interp, "This is a result", TCL_STATIC);
    result = Tcl_ExprLong(interp, "4+1", &exprResult);
    if (result != TCL_OK) {
        return result;
    }
    sprintf(buf, ": %ld", exprResult);
    Tcl_AppendResult(interp, buf, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestexprstringCmd --
 *
 *	This procedure tests the basic operation of Tcl_ExprString.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestexprstringCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
                " expression\"", (char *) NULL);
        return TCL_ERROR;
    }
    return Tcl_ExprString(interp, argv[1]);
}

/*
 *----------------------------------------------------------------------
 *
 * TestfilelinkCmd --
 *
 *	This procedure implements the "testfilelink" command.  It is used
 *	to test the effects of creating and manipulating filesystem links
 *	in Tcl.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May create a link on disk.
 *
 *----------------------------------------------------------------------
 */

static int
TestfilelinkCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* The argument objects. */
{
    Tcl_Obj *contents;

    if (objc < 2 || objc > 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "source ?target?");
	return TCL_ERROR;
    }
    
    if (Tcl_FSConvertToPathType(interp, objv[1]) != TCL_OK) {
	return TCL_ERROR;
    }
    
    if (objc == 3) {
	/* Create link from source to target */
	contents = Tcl_FSLink(objv[1], objv[2], 
			TCL_CREATE_SYMBOLIC_LINK|TCL_CREATE_HARD_LINK);
	if (contents == NULL) {
	    Tcl_AppendResult(interp, "could not create link from \"", 
		    Tcl_GetString(objv[1]), "\" to \"", 
		    Tcl_GetString(objv[2]), "\": ", 
		    Tcl_PosixError(interp), (char *) NULL);
	    return TCL_ERROR;
	}
    } else {
	/* Read link */
	contents = Tcl_FSLink(objv[1], NULL, 0);
	if (contents == NULL) {
	    Tcl_AppendResult(interp, "could not read link \"", 
		    Tcl_GetString(objv[1]), "\": ", 
		    Tcl_PosixError(interp), (char *) NULL);
	    return TCL_ERROR;
	}
    }
    Tcl_SetObjResult(interp, contents);
    if (objc == 2) {
	/* 
	 * If we are creating a link, this will actually just
	 * be objv[3], and we don't own it
	 */
	Tcl_DecrRefCount(contents);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestgetassocdataCmd --
 *
 *	This procedure implements the "testgetassocdata" command. It is
 *	used to test Tcl_GetAssocData.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestgetassocdataCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    char *res;
    
    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
                " data_key\"", (char *) NULL);
        return TCL_ERROR;
    }
    res = (char *) Tcl_GetAssocData(interp, argv[1], NULL);
    if (res != NULL) {
        Tcl_AppendResult(interp, res, NULL);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestgetplatformCmd --
 *
 *	This procedure implements the "testgetplatform" command. It is
 *	used to retrievel the value of the tclPlatform global variable.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestgetplatformCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    static CONST char *platformStrings[] = { "unix", "mac", "windows" };
    TclPlatformType *platform;

#ifdef __WIN32__
    platform = TclWinGetPlatform();
#else
    platform = &tclPlatform;
#endif
    
    if (argc != 1) {
        Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
		(char *) NULL);
        return TCL_ERROR;
    }

    Tcl_AppendResult(interp, platformStrings[*platform], NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestinterpdeleteCmd --
 *
 *	This procedure tests the code in tclInterp.c that deals with
 *	interpreter deletion. It deletes a user-specified interpreter
 *	from the hierarchy, and subsequent code checks integrity.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Deletes one or more interpreters.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestinterpdeleteCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    Tcl_Interp *slaveToDelete;

    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " path\"", (char *) NULL);
        return TCL_ERROR;
    }
    slaveToDelete = Tcl_GetSlave(interp, argv[1]);
    if (slaveToDelete == (Tcl_Interp *) NULL) {
        return TCL_ERROR;
    }
    Tcl_DeleteInterp(slaveToDelete);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestlinkCmd --
 *
 *	This procedure implements the "testlink" command.  It is used
 *	to test Tcl_LinkVar and related library procedures.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates and deletes various variable links, plus returns
 *	values of the linked variables.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestlinkCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    static int intVar = 43;
    static int boolVar = 4;
    static double realVar = 1.23;
    static Tcl_WideInt wideVar = Tcl_LongAsWide(79);
    static char *stringVar = NULL;
    static int created = 0;
    char buffer[2*TCL_DOUBLE_SPACE];
    int writable, flag;
    Tcl_Obj *tmp;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option ?arg arg arg arg arg?\"", (char *) NULL);
	return TCL_ERROR;
    }
    if (strcmp(argv[1], "create") == 0) {
	if (argc != 7) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " ", argv[1],
		" intRO realRO boolRO stringRO wideRO\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (created) {
	    Tcl_UnlinkVar(interp, "int");
	    Tcl_UnlinkVar(interp, "real");
	    Tcl_UnlinkVar(interp, "bool");
	    Tcl_UnlinkVar(interp, "string");
	    Tcl_UnlinkVar(interp, "wide");
	}
	created = 1;
	if (Tcl_GetBoolean(interp, argv[2], &writable) != TCL_OK) {
	    return TCL_ERROR;
	}
	flag = (writable != 0) ? 0 : TCL_LINK_READ_ONLY;
	if (Tcl_LinkVar(interp, "int", (char *) &intVar,
		TCL_LINK_INT | flag) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Tcl_GetBoolean(interp, argv[3], &writable) != TCL_OK) {
	    return TCL_ERROR;
	}
	flag = (writable != 0) ? 0 : TCL_LINK_READ_ONLY;
	if (Tcl_LinkVar(interp, "real", (char *) &realVar,
		TCL_LINK_DOUBLE | flag) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Tcl_GetBoolean(interp, argv[4], &writable) != TCL_OK) {
	    return TCL_ERROR;
	}
	flag = (writable != 0) ? 0 : TCL_LINK_READ_ONLY;
	if (Tcl_LinkVar(interp, "bool", (char *) &boolVar,
		TCL_LINK_BOOLEAN | flag) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Tcl_GetBoolean(interp, argv[5], &writable) != TCL_OK) {
	    return TCL_ERROR;
	}
	flag = (writable != 0) ? 0 : TCL_LINK_READ_ONLY;
	if (Tcl_LinkVar(interp, "string", (char *) &stringVar,
		TCL_LINK_STRING | flag) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Tcl_GetBoolean(interp, argv[6], &writable) != TCL_OK) {
	    return TCL_ERROR;
	}
	flag = (writable != 0) ? 0 : TCL_LINK_READ_ONLY;
	if (Tcl_LinkVar(interp, "wide", (char *) &wideVar,
			TCL_LINK_WIDE_INT | flag) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else if (strcmp(argv[1], "delete") == 0) {
	Tcl_UnlinkVar(interp, "int");
	Tcl_UnlinkVar(interp, "real");
	Tcl_UnlinkVar(interp, "bool");
	Tcl_UnlinkVar(interp, "string");
	Tcl_UnlinkVar(interp, "wide");
	created = 0;
    } else if (strcmp(argv[1], "get") == 0) {
	TclFormatInt(buffer, intVar);
	Tcl_AppendElement(interp, buffer);
	Tcl_PrintDouble((Tcl_Interp *) NULL, realVar, buffer);
	Tcl_AppendElement(interp, buffer);
	TclFormatInt(buffer, boolVar);
	Tcl_AppendElement(interp, buffer);
	Tcl_AppendElement(interp, (stringVar == NULL) ? "-" : stringVar);
	/*
	 * Wide ints only have an object-based interface.
	 */
	tmp = Tcl_NewWideIntObj(wideVar);
	Tcl_AppendElement(interp, Tcl_GetString(tmp));
	Tcl_DecrRefCount(tmp);
    } else if (strcmp(argv[1], "set") == 0) {
	if (argc != 7) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " ", argv[1],
		    " intValue realValue boolValue stringValue wideValue\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argv[2][0] != 0) {
	    if (Tcl_GetInt(interp, argv[2], &intVar) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	if (argv[3][0] != 0) {
	    if (Tcl_GetDouble(interp, argv[3], &realVar) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	if (argv[4][0] != 0) {
	    if (Tcl_GetInt(interp, argv[4], &boolVar) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	if (argv[5][0] != 0) {
	    if (stringVar != NULL) {
		ckfree(stringVar);
	    }
	    if (strcmp(argv[5], "-") == 0) {
		stringVar = NULL;
	    } else {
		stringVar = (char *) ckalloc((unsigned) (strlen(argv[5]) + 1));
		strcpy(stringVar, argv[5]);
	    }
	}
	if (argv[6][0] != 0) {
	    tmp = Tcl_NewStringObj(argv[6], -1);
	    if (Tcl_GetWideIntFromObj(interp, tmp, &wideVar) != TCL_OK) {
		Tcl_DecrRefCount(tmp);
		return TCL_ERROR;
	    }
	    Tcl_DecrRefCount(tmp);
	}
    } else if (strcmp(argv[1], "update") == 0) {
	if (argc != 7) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " ", argv[1],
		    "intValue realValue boolValue stringValue wideValue\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (argv[2][0] != 0) {
	    if (Tcl_GetInt(interp, argv[2], &intVar) != TCL_OK) {
		return TCL_ERROR;
	    }
	    Tcl_UpdateLinkedVar(interp, "int");
	}
	if (argv[3][0] != 0) {
	    if (Tcl_GetDouble(interp, argv[3], &realVar) != TCL_OK) {
		return TCL_ERROR;
	    }
	    Tcl_UpdateLinkedVar(interp, "real");
	}
	if (argv[4][0] != 0) {
	    if (Tcl_GetInt(interp, argv[4], &boolVar) != TCL_OK) {
		return TCL_ERROR;
	    }
	    Tcl_UpdateLinkedVar(interp, "bool");
	}
	if (argv[5][0] != 0) {
	    if (stringVar != NULL) {
		ckfree(stringVar);
	    }
	    if (strcmp(argv[5], "-") == 0) {
		stringVar = NULL;
	    } else {
		stringVar = (char *) ckalloc((unsigned) (strlen(argv[5]) + 1));
		strcpy(stringVar, argv[5]);
	    }
	    Tcl_UpdateLinkedVar(interp, "string");
	}
	if (argv[6][0] != 0) {
	    tmp = Tcl_NewStringObj(argv[6], -1);
	    if (Tcl_GetWideIntFromObj(interp, tmp, &wideVar) != TCL_OK) {
		Tcl_DecrRefCount(tmp);
		return TCL_ERROR;
	    }
	    Tcl_DecrRefCount(tmp);
	    Tcl_UpdateLinkedVar(interp, "wide");
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": should be create, delete, get, set, or update",
		(char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestlocaleCmd --
 *
 *	This procedure implements the "testlocale" command.  It is used
 *	to test the effects of setting different locales in Tcl.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Modifies the current C locale.
 *
 *----------------------------------------------------------------------
 */

static int
TestlocaleCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* The argument objects. */
{
    int index;
    char *locale;

    static CONST char *optionStrings[] = {
    	"ctype", "numeric", "time", "collate", "monetary", 
	"all",	NULL
    };
    static int lcTypes[] = {
	LC_CTYPE, LC_NUMERIC, LC_TIME, LC_COLLATE, LC_MONETARY,
	LC_ALL
    };

    /*
     * LC_CTYPE, etc. correspond to the indices for the strings.
     */

    if (objc < 2 || objc > 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "category ?locale?");
	return TCL_ERROR;
    }
    
    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    if (objc == 3) {
	locale = Tcl_GetString(objv[2]);
    } else {
	locale = NULL;
    }
    locale = setlocale(lcTypes[index], locale);
    if (locale) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), locale, -1);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestMathFunc --
 *
 *	This is a user-defined math procedure to test out math procedures
 *	with no arguments.
 *
 * Results:
 *	A normal Tcl completion code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestMathFunc(clientData, interp, args, resultPtr)
    ClientData clientData;		/* Integer value to return. */
    Tcl_Interp *interp;			/* Not used. */
    Tcl_Value *args;			/* Not used. */
    Tcl_Value *resultPtr;		/* Where to store result. */
{
    resultPtr->type = TCL_INT;
    resultPtr->intValue = (int) clientData;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestMathFunc2 --
 *
 *	This is a user-defined math procedure to test out math procedures
 *	that do have arguments, in this case 2.
 *
 * Results:
 *	A normal Tcl completion code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestMathFunc2(clientData, interp, args, resultPtr)
    ClientData clientData;		/* Integer value to return. */
    Tcl_Interp *interp;			/* Used to report errors. */
    Tcl_Value *args;			/* Points to an array of two
					 * Tcl_Value structs for the 
					 * two arguments. */
    Tcl_Value *resultPtr;		/* Where to store the result. */
{
    int result = TCL_OK;
    
    /*
     * Return the maximum of the two arguments with the correct type.
     */
    
    if (args[0].type == TCL_INT) {
	int i0 = args[0].intValue;
	
	if (args[1].type == TCL_INT) {
	    int i1 = args[1].intValue;
	    
	    resultPtr->type = TCL_INT;
	    resultPtr->intValue = ((i0 > i1)? i0 : i1);
	} else if (args[1].type == TCL_DOUBLE) {
	    double d0 = i0;
	    double d1 = args[1].doubleValue;

	    resultPtr->type = TCL_DOUBLE;
	    resultPtr->doubleValue = ((d0 > d1)? d0 : d1);
	} else if (args[1].type == TCL_WIDE_INT) {
	    Tcl_WideInt w0 = Tcl_LongAsWide(i0);
	    Tcl_WideInt w1 = args[1].wideValue;

	    resultPtr->type = TCL_WIDE_INT;
	    resultPtr->wideValue = ((w0 > w1)? w0 : w1);
	} else {
	    Tcl_SetResult(interp, "T3: wrong type for arg 2", TCL_STATIC);
	    result = TCL_ERROR;
	}
    } else if (args[0].type == TCL_DOUBLE) {
	double d0 = args[0].doubleValue;
	
	if (args[1].type == TCL_INT) {
	    double d1 = args[1].intValue;
	    
	    resultPtr->type = TCL_DOUBLE;
	    resultPtr->doubleValue = ((d0 > d1)? d0 : d1);
	} else if (args[1].type == TCL_DOUBLE) {
	    double d1 = args[1].doubleValue;

	    resultPtr->type = TCL_DOUBLE;
	    resultPtr->doubleValue = ((d0 > d1)? d0 : d1);
	} else if (args[1].type == TCL_WIDE_INT) {
	    double d1 = Tcl_WideAsDouble(args[1].wideValue);

	    resultPtr->type = TCL_DOUBLE;
	    resultPtr->doubleValue = ((d0 > d1)? d0 : d1);
	} else {
	    Tcl_SetResult(interp, "T3: wrong type for arg 2", TCL_STATIC);
	    result = TCL_ERROR;
	}
    } else if (args[0].type == TCL_WIDE_INT) {
	Tcl_WideInt w0 = args[0].wideValue;
	
	if (args[1].type == TCL_INT) {
	    Tcl_WideInt w1 = Tcl_LongAsWide(args[1].intValue);
	    
	    resultPtr->type = TCL_WIDE_INT;
	    resultPtr->wideValue = ((w0 > w1)? w0 : w1);
	} else if (args[1].type == TCL_DOUBLE) {
	    double d0 = Tcl_WideAsDouble(w0);
	    double d1 = args[1].doubleValue;

	    resultPtr->type = TCL_DOUBLE;
	    resultPtr->doubleValue = ((d0 > d1)? d0 : d1);
	} else if (args[1].type == TCL_WIDE_INT) {
	    Tcl_WideInt w1 = args[1].wideValue;

	    resultPtr->type = TCL_WIDE_INT;
	    resultPtr->wideValue = ((w0 > w1)? w0 : w1);
	} else {
	    Tcl_SetResult(interp, "T3: wrong type for arg 2", TCL_STATIC);
	    result = TCL_ERROR;
	}
    } else {
	Tcl_SetResult(interp, "T3: wrong type for arg 1", TCL_STATIC);
	result = TCL_ERROR;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * CleanupTestSetassocdataTests --
 *
 *	This function is called when an interpreter is deleted to clean
 *	up any data left over from running the testsetassocdata command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Releases storage.
 *
 *----------------------------------------------------------------------
 */
	/* ARGSUSED */
static void
CleanupTestSetassocdataTests(clientData, interp)
    ClientData clientData;		/* Data to be released. */
    Tcl_Interp *interp;			/* Interpreter being deleted. */
{
    ckfree((char *) clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * TestparserObjCmd --
 *
 *	This procedure implements the "testparser" command.  It is
 *	used for testing the new Tcl script parser in Tcl 8.1.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestparserObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* The argument objects. */
{
    char *script;
    int length, dummy;
    Tcl_Parse parse;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "script length");
	return TCL_ERROR;
    }
    script = Tcl_GetStringFromObj(objv[1], &dummy);
    if (Tcl_GetIntFromObj(interp, objv[2], &length)) {
	return TCL_ERROR;
    }
    if (length == 0) {
	length = dummy;
    }
    if (Tcl_ParseCommand(interp, script, length, 0, &parse) != TCL_OK) {
	Tcl_AddErrorInfo(interp, "\n    (remainder of script: \"");
	Tcl_AddErrorInfo(interp, parse.term);
	Tcl_AddErrorInfo(interp, "\")");
	return TCL_ERROR;
    }

    /*
     * The parse completed successfully.  Just print out the contents
     * of the parse structure into the interpreter's result.
     */

    PrintParse(interp, &parse);
    Tcl_FreeParse(&parse);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestexprparserObjCmd --
 *
 *	This procedure implements the "testexprparser" command.  It is
 *	used for testing the new Tcl expression parser in Tcl 8.1.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestexprparserObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* The argument objects. */
{
    char *script;
    int length, dummy;
    Tcl_Parse parse;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "expr length");
	return TCL_ERROR;
    }
    script = Tcl_GetStringFromObj(objv[1], &dummy);
    if (Tcl_GetIntFromObj(interp, objv[2], &length)) {
	return TCL_ERROR;
    }
    if (length == 0) {
	length = dummy;
    }
    if (Tcl_ParseExpr(interp, script, length, &parse) != TCL_OK) {
	Tcl_AddErrorInfo(interp, "\n    (remainder of expr: \"");
	Tcl_AddErrorInfo(interp, parse.term);
	Tcl_AddErrorInfo(interp, "\")");
	return TCL_ERROR;
    }

    /*
     * The parse completed successfully.  Just print out the contents
     * of the parse structure into the interpreter's result.
     */

    PrintParse(interp, &parse);
    Tcl_FreeParse(&parse);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * PrintParse --
 *
 *	This procedure prints out the contents of a Tcl_Parse structure
 *	in the result of an interpreter.
 *
 * Results:
 *	Interp's result is set to a prettily formatted version of the
 *	contents of parsePtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
PrintParse(interp, parsePtr)
    Tcl_Interp *interp;		/* Interpreter whose result is to be set to
				 * the contents of a parse structure. */
    Tcl_Parse *parsePtr;	/* Parse structure to print out. */
{
    Tcl_Obj *objPtr;
    char *typeString;
    Tcl_Token *tokenPtr;
    int i;

    objPtr = Tcl_GetObjResult(interp);
    if (parsePtr->commentSize > 0) {
	Tcl_ListObjAppendElement((Tcl_Interp *) NULL, objPtr,
		Tcl_NewStringObj(parsePtr->commentStart,
			parsePtr->commentSize));
    } else {
	Tcl_ListObjAppendElement((Tcl_Interp *) NULL, objPtr,
		Tcl_NewStringObj("-", 1));
    }
    Tcl_ListObjAppendElement((Tcl_Interp *) NULL, objPtr,
	    Tcl_NewStringObj(parsePtr->commandStart, parsePtr->commandSize));
    Tcl_ListObjAppendElement((Tcl_Interp *) NULL, objPtr,
	    Tcl_NewIntObj(parsePtr->numWords));
    for (i = 0; i < parsePtr->numTokens; i++) {
	tokenPtr = &parsePtr->tokenPtr[i];
	switch (tokenPtr->type) {
	    case TCL_TOKEN_WORD:
		typeString = "word";
		break;
	    case TCL_TOKEN_SIMPLE_WORD:
		typeString = "simple";
		break;
	    case TCL_TOKEN_TEXT:
		typeString = "text";
		break;
	    case TCL_TOKEN_BS:
		typeString = "backslash";
		break;
	    case TCL_TOKEN_COMMAND:
		typeString = "command";
		break;
	    case TCL_TOKEN_VARIABLE:
		typeString = "variable";
		break;
	    case TCL_TOKEN_SUB_EXPR:
		typeString = "subexpr";
		break;
	    case TCL_TOKEN_OPERATOR:
		typeString = "operator";
		break;
	    default:
		typeString = "??";
		break;
	}
	Tcl_ListObjAppendElement((Tcl_Interp *) NULL, objPtr,
		Tcl_NewStringObj(typeString, -1));
	Tcl_ListObjAppendElement((Tcl_Interp *) NULL, objPtr,
		Tcl_NewStringObj(tokenPtr->start, tokenPtr->size));
	Tcl_ListObjAppendElement((Tcl_Interp *) NULL, objPtr,
		Tcl_NewIntObj(tokenPtr->numComponents));
    }
    Tcl_ListObjAppendElement((Tcl_Interp *) NULL, objPtr,
	    Tcl_NewStringObj(parsePtr->commandStart + parsePtr->commandSize,
	    -1));
}

/*
 *----------------------------------------------------------------------
 *
 * TestparsevarObjCmd --
 *
 *	This procedure implements the "testparsevar" command.  It is
 *	used for testing Tcl_ParseVar.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestparsevarObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* The argument objects. */
{
    CONST char *value;
    CONST char *name, *termPtr;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "varName");
	return TCL_ERROR;
    }
    name = Tcl_GetString(objv[1]);
    value = Tcl_ParseVar(interp, name, &termPtr);
    if (value == NULL) {
	return TCL_ERROR;
    }

    Tcl_AppendElement(interp, value);
    Tcl_AppendElement(interp, termPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestparsevarnameObjCmd --
 *
 *	This procedure implements the "testparsevarname" command.  It is
 *	used for testing the new Tcl script parser in Tcl 8.1.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestparsevarnameObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* The argument objects. */
{
    char *script;
    int append, length, dummy;
    Tcl_Parse parse;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "script length append");
	return TCL_ERROR;
    }
    script = Tcl_GetStringFromObj(objv[1], &dummy);
    if (Tcl_GetIntFromObj(interp, objv[2], &length)) {
	return TCL_ERROR;
    }
    if (length == 0) {
	length = dummy;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &append)) {
	return TCL_ERROR;
    }
    if (Tcl_ParseVarName(interp, script, length, &parse, append) != TCL_OK) {
	Tcl_AddErrorInfo(interp, "\n    (remainder of script: \"");
	Tcl_AddErrorInfo(interp, parse.term);
	Tcl_AddErrorInfo(interp, "\")");
	return TCL_ERROR;
    }

    /*
     * The parse completed successfully.  Just print out the contents
     * of the parse structure into the interpreter's result.
     */

    parse.commentSize = 0;
    parse.commandStart = script + parse.tokenPtr->size;
    parse.commandSize = 0;
    PrintParse(interp, &parse);
    Tcl_FreeParse(&parse);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestregexpObjCmd --
 *
 *	This procedure implements the "testregexp" command. It is
 *	used to give a direct interface for regexp flags.  It's identical
 *	to Tcl_RegexpObjCmd except for the -xflags option, and the
 *	consequences thereof (including the REG_EXPECT kludge).
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestregexpObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    int i, ii, indices, stringLength, match, about;
    int hasxflags, cflags, eflags;
    Tcl_RegExp regExpr;
    char *string;
    Tcl_Obj *objPtr;
    Tcl_RegExpInfo info;
    static CONST char *options[] = {
	"-indices",	"-nocase",	"-about",	"-expanded",
	"-line",	"-linestop",	"-lineanchor",
	"-xflags",
	"--",		(char *) NULL
    };
    enum options {
	REGEXP_INDICES, REGEXP_NOCASE,	REGEXP_ABOUT,	REGEXP_EXPANDED,
	REGEXP_MULTI,	REGEXP_NOCROSS,	REGEXP_NEWL,
	REGEXP_XFLAGS,
	REGEXP_LAST
    };

    indices = 0;
    about = 0;
    cflags = REG_ADVANCED;
    eflags = 0;
    hasxflags = 0;
    
    for (i = 1; i < objc; i++) {
	char *name;
	int index;

	name = Tcl_GetString(objv[i]);
	if (name[0] != '-') {
	    break;
	}
	if (Tcl_GetIndexFromObj(interp, objv[i], options, "switch", TCL_EXACT,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum options) index) {
	    case REGEXP_INDICES: {
		indices = 1;
		break;
	    }
	    case REGEXP_NOCASE: {
		cflags |= REG_ICASE;
		break;
	    }
	    case REGEXP_ABOUT: {
		about = 1;
		break;
	    }
	    case REGEXP_EXPANDED: {
		cflags |= REG_EXPANDED;
		break;
	    }
	    case REGEXP_MULTI: {
		cflags |= REG_NEWLINE;
		break;
	    }
	    case REGEXP_NOCROSS: {
		cflags |= REG_NLSTOP;
		break;
	    }
	    case REGEXP_NEWL: {
		cflags |= REG_NLANCH;
		break;
	    }
	    case REGEXP_XFLAGS: {
		hasxflags = 1;
		break;
	    }
	    case REGEXP_LAST: {
		i++;
		goto endOfForLoop;
	    }
	}
    }

    endOfForLoop:
    if (objc - i < hasxflags + 2 - about) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?switches? exp string ?matchVar? ?subMatchVar subMatchVar ...?");
	return TCL_ERROR;
    }
    objc -= i;
    objv += i;

    if (hasxflags) {
	string = Tcl_GetStringFromObj(objv[0], &stringLength);
	TestregexpXflags(string, stringLength, &cflags, &eflags);
	objc--;
	objv++;
    }

    regExpr = Tcl_GetRegExpFromObj(interp, objv[0], cflags);
    if (regExpr == NULL) {
	return TCL_ERROR;
    }
    objPtr = objv[1];

    if (about) {
	if (TclRegAbout(interp, regExpr) < 0) {
	    return TCL_ERROR;
	}
	return TCL_OK;
    }

    match = Tcl_RegExpExecObj(interp, regExpr, objPtr, 0 /* offset */,
	    objc-2 /* nmatches */, eflags);

    if (match < 0) {
	return TCL_ERROR;
    }
    if (match == 0) {
	/*
	 * Set the interpreter's object result to an integer object w/
	 * value 0. 
	 */
	
	Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
	if (objc > 2 && (cflags&REG_EXPECT) && indices) {
	    char *varName;
	    CONST char *value;
	    int start, end;
	    char resinfo[TCL_INTEGER_SPACE * 2];

	    varName = Tcl_GetString(objv[2]);
	    TclRegExpRangeUniChar(regExpr, -1, &start, &end);
	    sprintf(resinfo, "%d %d", start, end-1);
	    value = Tcl_SetVar(interp, varName, resinfo, 0);
	    if (value == NULL) {
		Tcl_AppendResult(interp, "couldn't set variable \"",
			varName, "\"", (char *) NULL);
		return TCL_ERROR;
	    }
	} else if (cflags & TCL_REG_CANMATCH) {
	    char *varName;
	    CONST char *value;
	    char resinfo[TCL_INTEGER_SPACE * 2];

	    Tcl_RegExpGetInfo(regExpr, &info);
	    varName = Tcl_GetString(objv[2]);
	    sprintf(resinfo, "%d", info.extendStart);
	    value = Tcl_SetVar(interp, varName, resinfo, 0);
	    if (value == NULL) {
		Tcl_AppendResult(interp, "couldn't set variable \"",
			varName, "\"", (char *) NULL);
		return TCL_ERROR;
	    }
	}
	return TCL_OK;
    }

    /*
     * If additional variable names have been specified, return
     * index information in those variables.
     */

    objc -= 2;
    objv += 2;

    Tcl_RegExpGetInfo(regExpr, &info);
    for (i = 0; i < objc; i++) {
	int start, end;
	Tcl_Obj *newPtr, *varPtr, *valuePtr;
	
	varPtr = objv[i];
	ii = ((cflags&REG_EXPECT) && i == objc-1) ? -1 : i;
	if (indices) {
	    Tcl_Obj *objs[2];

	    if (ii == -1) {
		TclRegExpRangeUniChar(regExpr, ii, &start, &end);
	    } else if (ii > info.nsubs) {
		start = -1;
		end = -1;
	    } else {
		start = info.matches[ii].start;
		end = info.matches[ii].end;
	    }

	    /*
	     * Adjust index so it refers to the last character in the
	     * match instead of the first character after the match.
	     */
	    
	    if (end >= 0) {
		end--;
	    }

	    objs[0] = Tcl_NewLongObj(start);
	    objs[1] = Tcl_NewLongObj(end);

	    newPtr = Tcl_NewListObj(2, objs);
	} else {
	    if (ii == -1) {
		TclRegExpRangeUniChar(regExpr, ii, &start, &end);
		newPtr = Tcl_GetRange(objPtr, start, end);
	    } else if (ii > info.nsubs) {
		newPtr = Tcl_NewObj();
	    } else {
		newPtr = Tcl_GetRange(objPtr, info.matches[ii].start,
			info.matches[ii].end - 1);
	    }
	}
	valuePtr = Tcl_ObjSetVar2(interp, varPtr, NULL, newPtr, 0);
	if (valuePtr == NULL) {
	    Tcl_DecrRefCount(newPtr);
	    Tcl_AppendResult(interp, "couldn't set variable \"",
		    Tcl_GetString(varPtr), "\"", (char *) NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * Set the interpreter's object result to an integer object w/ value 1. 
     */
	
    Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TestregexpXflags --
 *
 *	Parse a string of extended regexp flag letters, for testing.
 *
 * Results:
 *	No return value (you're on your own for errors here).
 *
 * Side effects:
 *	Modifies *cflagsPtr, a regcomp flags word, and *eflagsPtr, a
 *	regexec flags word, as appropriate.
 *
 *----------------------------------------------------------------------
 */

static void
TestregexpXflags(string, length, cflagsPtr, eflagsPtr)
    char *string;		/* The string of flags. */
    int length;			/* The length of the string in bytes. */
    int *cflagsPtr;		/* compile flags word */
    int *eflagsPtr;		/* exec flags word */
{
    int i;
    int cflags;
    int eflags;

    cflags = *cflagsPtr;
    eflags = *eflagsPtr;
    for (i = 0; i < length; i++) {
	switch (string[i]) {
	    case 'a': {
		cflags |= REG_ADVF;
		break;
	    }
	    case 'b': {
		cflags &= ~REG_ADVANCED;
		break;
	    }
	    case 'c': {
		cflags |= TCL_REG_CANMATCH;
		break;
	    }
	    case 'e': {
		cflags &= ~REG_ADVANCED;
		cflags |= REG_EXTENDED;
		break;
	    }
	    case 'q': {
		cflags &= ~REG_ADVANCED;
		cflags |= REG_QUOTE;
		break;
	    }
	    case 'o': {			/* o for opaque */
		cflags |= REG_NOSUB;
		break;
	    }
	    case 's': {			/* s for start */
		cflags |= REG_BOSONLY;
		break;
	    }
	    case '+': {
		cflags |= REG_FAKE;
		break;
	    }
	    case ',': {
		cflags |= REG_PROGRESS;
		break;
	    }
	    case '.': {
		cflags |= REG_DUMP;
		break;
	    }
	    case ':': {
		eflags |= REG_MTRACE;
		break;
	    }
	    case ';': {
		eflags |= REG_FTRACE;
		break;
	    }
	    case '^': {
		eflags |= REG_NOTBOL;
		break;
	    }
	    case '$': {
		eflags |= REG_NOTEOL;
		break;
	    }
	    case 't': {
		cflags |= REG_EXPECT;
		break;
	    }
	    case '%': {
		eflags |= REG_SMALL;
		break;
	    }
	}
    }

    *cflagsPtr = cflags;
    *eflagsPtr = eflags;
}

/*
 *----------------------------------------------------------------------
 *
 * TestsetassocdataCmd --
 *
 *	This procedure implements the "testsetassocdata" command. It is used
 *	to test Tcl_SetAssocData.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Modifies or creates an association between a key and associated
 *	data for this interpreter.
 *
 *----------------------------------------------------------------------
 */

static int
TestsetassocdataCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    char *buf;
    char *oldData;
    Tcl_InterpDeleteProc *procPtr;
    
    if (argc != 3) {
        Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
                " data_key data_item\"", (char *) NULL);
        return TCL_ERROR;
    }

    buf = ckalloc((unsigned) strlen(argv[2]) + 1);
    strcpy(buf, argv[2]);

    /*
     * If we previously associated a malloced value with the variable,
     * free it before associating a new value.
     */

    oldData = (char *) Tcl_GetAssocData(interp, argv[1], &procPtr);
    if ((oldData != NULL) && (procPtr == CleanupTestSetassocdataTests)) {
	ckfree(oldData);
    }
    
    Tcl_SetAssocData(interp, argv[1], CleanupTestSetassocdataTests, 
	(ClientData) buf);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestsetplatformCmd --
 *
 *	This procedure implements the "testsetplatform" command. It is
 *	used to change the tclPlatform global variable so all file
 *	name conversions can be tested on a single platform.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Sets the tclPlatform global variable.
 *
 *----------------------------------------------------------------------
 */

static int
TestsetplatformCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    size_t length;
    TclPlatformType *platform;

#ifdef __WIN32__
    platform = TclWinGetPlatform();
#else
    platform = &tclPlatform;
#endif
    
    if (argc != 2) {
        Tcl_AppendResult(interp, "wrong # arguments: should be \"", argv[0],
                " platform\"", (char *) NULL);
        return TCL_ERROR;
    }

    length = strlen(argv[1]);
    if (strncmp(argv[1], "unix", length) == 0) {
	*platform = TCL_PLATFORM_UNIX;
    } else if (strncmp(argv[1], "mac", length) == 0) {
	*platform = TCL_PLATFORM_MAC;
    } else if (strncmp(argv[1], "windows", length) == 0) {
	*platform = TCL_PLATFORM_WINDOWS;
    } else {
        Tcl_AppendResult(interp, "unsupported platform: should be one of ",
		"unix, mac, or windows", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TeststaticpkgCmd --
 *
 *	This procedure implements the "teststaticpkg" command.
 *	It is used to test the procedure Tcl_StaticPackage.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	When the packge given by argv[1] is loaded into an interpeter,
 *	variable "x" in that interpreter is set to "loaded".
 *
 *----------------------------------------------------------------------
 */

static int
TeststaticpkgCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    int safe, loaded;

    if (argc != 4) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		argv[0], " pkgName safe loaded\"", (char *) NULL);
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[2], &safe) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[3], &loaded) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage((loaded) ? interp : NULL, argv[1], StaticInitProc,
	    (safe) ? StaticInitProc : NULL);
    return TCL_OK;
}

static int
StaticInitProc(interp)
    Tcl_Interp *interp;			/* Interpreter in which package
					 * is supposedly being loaded. */
{
    Tcl_SetVar(interp, "x", "loaded", TCL_GLOBAL_ONLY);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TesttranslatefilenameCmd --
 *
 *	This procedure implements the "testtranslatefilename" command.
 *	It is used to test the Tcl_TranslateFileName command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TesttranslatefilenameCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    Tcl_DString buffer;
    CONST char *result;

    if (argc != 2) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		argv[0], " path\"", (char *) NULL);
	return TCL_ERROR;
    }
    result = Tcl_TranslateFileName(interp, argv[1], &buffer);
    if (result == NULL) {
	return TCL_ERROR;
    }
    Tcl_AppendResult(interp, result, NULL);
    Tcl_DStringFree(&buffer);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestupvarCmd --
 *
 *	This procedure implements the "testupvar2" command.  It is used
 *	to test Tcl_UpVar and Tcl_UpVar2.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates or modifies an "upvar" reference.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestupvarCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    int flags = 0;
    
    if ((argc != 5) && (argc != 6)) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		argv[0], " level name ?name2? dest global\"", (char *) NULL);
	return TCL_ERROR;
    }

    if (argc == 5) {
	if (strcmp(argv[4], "global") == 0) {
	    flags = TCL_GLOBAL_ONLY;
	} else if (strcmp(argv[4], "namespace") == 0) {
	    flags = TCL_NAMESPACE_ONLY;
	}
	return Tcl_UpVar(interp, argv[1], argv[2], argv[3], flags);
    } else {
	if (strcmp(argv[5], "global") == 0) {
	    flags = TCL_GLOBAL_ONLY;
	} else if (strcmp(argv[5], "namespace") == 0) {
	    flags = TCL_NAMESPACE_ONLY;
	}
	return Tcl_UpVar2(interp, argv[1], argv[2], 
		(argv[3][0] == 0) ? (char *) NULL : argv[3], argv[4],
		flags);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TestsetobjerrorcodeCmd --
 *
 *	This procedure implements the "testsetobjerrorcodeCmd".
 *	This tests up to five elements passed to the
 *	Tcl_SetObjErrorCode command.
 *
 * Results:
 *	A standard Tcl result. Always returns TCL_ERROR so that
 *	the error code can be tested.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestsetobjerrorcodeCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* The argument objects. */
{
    Tcl_Obj *listObjPtr;

    if (objc > 1) {
	listObjPtr = Tcl_ConcatObj(objc - 1, objv + 1);
    } else {
	listObjPtr = Tcl_NewObj();
    }
    Tcl_IncrRefCount(listObjPtr);
    Tcl_SetObjErrorCode(interp, listObjPtr);
    Tcl_DecrRefCount(listObjPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TestfeventCmd --
 *
 *	This procedure implements the "testfevent" command.  It is
 *	used for testing the "fileevent" command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates and deletes interpreters.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestfeventCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    static Tcl_Interp *interp2 = NULL;
    int code;
    Tcl_Channel chan;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option ?arg arg ...?", (char *) NULL);
	return TCL_ERROR;
    }
    if (strcmp(argv[1], "cmd") == 0) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " cmd script", (char *) NULL);
	    return TCL_ERROR;
	}
        if (interp2 != (Tcl_Interp *) NULL) {
            code = Tcl_GlobalEval(interp2, argv[2]);
	    Tcl_SetObjResult(interp, Tcl_GetObjResult(interp2));
            return code;
        } else {
            Tcl_AppendResult(interp,
                    "called \"testfevent code\" before \"testfevent create\"",
                    (char *) NULL);
            return TCL_ERROR;
        }
    } else if (strcmp(argv[1], "create") == 0) {
	if (interp2 != NULL) {
            Tcl_DeleteInterp(interp2);
	}
        interp2 = Tcl_CreateInterp();
	return Tcl_Init(interp2);
    } else if (strcmp(argv[1], "delete") == 0) {
	if (interp2 != NULL) {
            Tcl_DeleteInterp(interp2);
	}
	interp2 = NULL;
    } else if (strcmp(argv[1], "share") == 0) {
        if (interp2 != NULL) {
            chan = Tcl_GetChannel(interp, argv[2], NULL);
            if (chan == (Tcl_Channel) NULL) {
                return TCL_ERROR;
            }
            Tcl_RegisterChannel(interp2, chan);
        }
    }
    
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestpanicCmd --
 *
 *	Calls the panic routine.
 *
 * Results:
 *      Always returns TCL_OK. 
 *
 * Side effects:
 *	May exit application.
 *
 *----------------------------------------------------------------------
 */

static int
TestpanicCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    CONST char *argString;
    
    /*
     *  Put the arguments into a var args structure
     *  Append all of the arguments together separated by spaces
     */

    argString = Tcl_Merge(argc-1, argv+1);
    panic(argString);
    ckfree((char *)argString);
 
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TestchmodCmd --
 *
 *	Implements the "testchmod" cmd.  Used when testing "file"
 *	command.  The only attribute used by the Mac and Windows platforms
 *	is the user write flag; if this is not set, the file is
 *	made read-only.  Otehrwise, the file is made read-write.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Changes permissions of specified files.
 *
 *---------------------------------------------------------------------------
 */
 
static int
TestchmodCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    int i, mode;
    char *rest;

    if (argc < 2) {
	usage:
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" mode file ?file ...?", (char *) NULL);
	return TCL_ERROR;
    }

    mode = (int) strtol(argv[1], &rest, 8);
    if ((rest == argv[1]) || (*rest != '\0')) {
	goto usage;
    }

    for (i = 2; i < argc; i++) {
        Tcl_DString buffer;
	CONST char *translated;
        
        translated = Tcl_TranslateFileName(interp, argv[i], &buffer);
        if (translated == NULL) {
            return TCL_ERROR;
        }
	if (chmod(translated, (unsigned) mode) != 0) {
	    Tcl_AppendResult(interp, translated, ": ", Tcl_PosixError(interp),
		    (char *) NULL);
	    return TCL_ERROR;
	}
        Tcl_DStringFree(&buffer);
    }
    return TCL_OK;
}

static int
TestfileCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;			/* Number of arguments. */
    Tcl_Obj *CONST argv[];	/* The argument objects. */
{
    int force, i, j, result;
    Tcl_Obj *error = NULL;
    char *subcmd;
    
    if (argc < 3) {
	return TCL_ERROR;
    }

    force = 0;
    i = 2;
    if (strcmp(Tcl_GetString(argv[2]), "-force") == 0) {
        force = 1;
	i = 3;
    }

    if (argc - i > 2) {
	return TCL_ERROR;
    }

    for (j = i; j < argc; j++) {
        if (Tcl_FSGetNormalizedPath(interp, argv[j]) == NULL) {
	    return TCL_ERROR;
	}
    }

    subcmd = Tcl_GetString(argv[1]);
    
    if (strcmp(subcmd, "mv") == 0) {
	result = TclpObjRenameFile(argv[i], argv[i + 1]);
    } else if (strcmp(subcmd, "cp") == 0) {
        result = TclpObjCopyFile(argv[i], argv[i + 1]);
    } else if (strcmp(subcmd, "rm") == 0) {
        result = TclpObjDeleteFile(argv[i]);
    } else if (strcmp(subcmd, "mkdir") == 0) {
        result = TclpObjCreateDirectory(argv[i]);
    } else if (strcmp(subcmd, "cpdir") == 0) {
        result = TclpObjCopyDirectory(argv[i], argv[i + 1], &error);
    } else if (strcmp(subcmd, "rmdir") == 0) {
        result = TclpObjRemoveDirectory(argv[i], force, &error);
    } else {
        result = TCL_ERROR;
	goto end;
    }
	
    if (result != TCL_OK) {
	if (error != NULL) {
	    if (Tcl_GetString(error)[0] != '\0') {
		Tcl_AppendResult(interp, Tcl_GetString(error), " ", NULL);
	    }
	    Tcl_DecrRefCount(error);
	}
	Tcl_AppendResult(interp, Tcl_ErrnoId(), (char *) NULL);
    }

    end:

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TestgetvarfullnameCmd --
 *
 *	Implements the "testgetvarfullname" cmd that is used when testing
 *	the Tcl_GetVariableFullName procedure.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestgetvarfullnameCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* The argument objects. */
{
    char *name, *arg;
    int flags = 0;
    Tcl_Namespace *namespacePtr;
    Tcl_CallFrame frame;
    Tcl_Var variable;
    int result;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "name scope");
        return TCL_ERROR;
    }
    
    name = Tcl_GetString(objv[1]);

    arg = Tcl_GetString(objv[2]);
    if (strcmp(arg, "global") == 0) {
	flags = TCL_GLOBAL_ONLY;
    } else if (strcmp(arg, "namespace") == 0) {
	flags = TCL_NAMESPACE_ONLY;
    }

    /*
     * This command, like any other created with Tcl_Create[Obj]Command,
     * runs in the global namespace. As a "namespace-aware" command that
     * needs to run in a particular namespace, it must activate that
     * namespace itself.
     */

    if (flags == TCL_NAMESPACE_ONLY) {
	namespacePtr = Tcl_FindNamespace(interp, "::test_ns_var",
	        (Tcl_Namespace *) NULL, TCL_LEAVE_ERR_MSG);
	if (namespacePtr == NULL) {
	    return TCL_ERROR;
	}
	result = Tcl_PushCallFrame(interp, &frame, namespacePtr,
                /*isProcCallFrame*/ 0);
	if (result != TCL_OK) {
	    return result;
	}
    }
    
    variable = Tcl_FindNamespaceVar(interp, name, (Tcl_Namespace *) NULL,
	    (flags | TCL_LEAVE_ERR_MSG));

    if (flags == TCL_NAMESPACE_ONLY) {
	Tcl_PopCallFrame(interp);
    }
    if (variable == (Tcl_Var) NULL) {
	return TCL_ERROR;
    }
    Tcl_GetVariableFullName(interp, variable, Tcl_GetObjResult(interp));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetTimesCmd --
 *
 *	This procedure implements the "gettimes" command.  It is
 *	used for computing the time needed for various basic operations
 *	such as reading variables, allocating memory, sprintf, converting
 *	variables, etc.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Allocates and frees memory, sets a variable "a" in the interpreter.
 *
 *----------------------------------------------------------------------
 */

static int
GetTimesCmd(unused, interp, argc, argv)
    ClientData unused;		/* Unused. */
    Tcl_Interp *interp;		/* The current interpreter. */
    int argc;			/* The number of arguments. */
    CONST char **argv;		/* The argument strings. */
{
    Interp *iPtr = (Interp *) interp;
    int i, n;
    double timePer;
    Tcl_Time start, stop;
    Tcl_Obj *objPtr;
    Tcl_Obj **objv;
    CONST char *s;
    char newString[TCL_INTEGER_SPACE];

    /* alloc & free 100000 times */
    fprintf(stderr, "alloc & free 100000 6 word items\n");
    Tcl_GetTime(&start);
    for (i = 0;  i < 100000;  i++) {
	objPtr = (Tcl_Obj *) ckalloc(sizeof(Tcl_Obj));
	ckfree((char *) objPtr);
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per alloc+free\n", timePer/100000);
    
    /* alloc 5000 times */
    fprintf(stderr, "alloc 5000 6 word items\n");
    objv = (Tcl_Obj **) ckalloc(5000 * sizeof(Tcl_Obj *));
    Tcl_GetTime(&start);
    for (i = 0;  i < 5000;  i++) {
	objv[i] = (Tcl_Obj *) ckalloc(sizeof(Tcl_Obj));
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per alloc\n", timePer/5000);
    
    /* free 5000 times */
    fprintf(stderr, "free 5000 6 word items\n");
    Tcl_GetTime(&start);
    for (i = 0;  i < 5000;  i++) {
	ckfree((char *) objv[i]);
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per free\n", timePer/5000);

    /* Tcl_NewObj 5000 times */
    fprintf(stderr, "Tcl_NewObj 5000 times\n");
    Tcl_GetTime(&start);
    for (i = 0;  i < 5000;  i++) {
	objv[i] = Tcl_NewObj();
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per Tcl_NewObj\n", timePer/5000);
    
    /* Tcl_DecrRefCount 5000 times */
    fprintf(stderr, "Tcl_DecrRefCount 5000 times\n");
    Tcl_GetTime(&start);
    for (i = 0;  i < 5000;  i++) {
	objPtr = objv[i];
	Tcl_DecrRefCount(objPtr);
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per Tcl_DecrRefCount\n", timePer/5000);
    ckfree((char *) objv);

    /* TclGetString 100000 times */
    fprintf(stderr, "TclGetStringFromObj of \"12345\" 100000 times\n");
    objPtr = Tcl_NewStringObj("12345", -1);
    Tcl_GetTime(&start);
    for (i = 0;  i < 100000;  i++) {
	(void) TclGetString(objPtr);
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per TclGetStringFromObj of \"12345\"\n",
	    timePer/100000);

    /* Tcl_GetIntFromObj 100000 times */
    fprintf(stderr, "Tcl_GetIntFromObj of \"12345\" 100000 times\n");
    Tcl_GetTime(&start);
    for (i = 0;  i < 100000;  i++) {
	if (Tcl_GetIntFromObj(interp, objPtr, &n) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per Tcl_GetIntFromObj of \"12345\"\n",
	    timePer/100000);
    Tcl_DecrRefCount(objPtr);
    
    /* Tcl_GetInt 100000 times */
    fprintf(stderr, "Tcl_GetInt of \"12345\" 100000 times\n");
    Tcl_GetTime(&start);
    for (i = 0;  i < 100000;  i++) {
	if (Tcl_GetInt(interp, "12345", &n) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per Tcl_GetInt of \"12345\"\n",
	    timePer/100000);

    /* sprintf 100000 times */
    fprintf(stderr, "sprintf of 12345 100000 times\n");
    Tcl_GetTime(&start);
    for (i = 0;  i < 100000;  i++) {
	sprintf(newString, "%d", 12345);
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per sprintf of 12345\n",
	    timePer/100000);

    /* hashtable lookup 100000 times */
    fprintf(stderr, "hashtable lookup of \"gettimes\" 100000 times\n");
    Tcl_GetTime(&start);
    for (i = 0;  i < 100000;  i++) {
	(void) Tcl_FindHashEntry(&iPtr->globalNsPtr->cmdTable, "gettimes");
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per hashtable lookup of \"gettimes\"\n",
	    timePer/100000);

    /* Tcl_SetVar 100000 times */
    fprintf(stderr, "Tcl_SetVar of \"12345\" 100000 times\n");
    Tcl_GetTime(&start);
    for (i = 0;  i < 100000;  i++) {
	s = Tcl_SetVar(interp, "a", "12345", TCL_LEAVE_ERR_MSG);
	if (s == NULL) {
	    return TCL_ERROR;
	}
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per Tcl_SetVar of a to \"12345\"\n",
	    timePer/100000);

    /* Tcl_GetVar 100000 times */
    fprintf(stderr, "Tcl_GetVar of a==\"12345\" 100000 times\n");
    Tcl_GetTime(&start);
    for (i = 0;  i < 100000;  i++) {
	s = Tcl_GetVar(interp, "a", TCL_LEAVE_ERR_MSG);
	if (s == NULL) {
	    return TCL_ERROR;
	}
    }
    Tcl_GetTime(&stop);
    timePer = (stop.sec - start.sec)*1000000 + (stop.usec - start.usec);
    fprintf(stderr, "   %.3f usec per Tcl_GetVar of a==\"12345\"\n",
	    timePer/100000);
    
    Tcl_ResetResult(interp);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NoopCmd --
 *
 *	This procedure is just used to time the overhead involved in
 *	parsing and invoking a command.
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
NoopCmd(unused, interp, argc, argv)
    ClientData unused;		/* Unused. */
    Tcl_Interp *interp;		/* The current interpreter. */
    int argc;			/* The number of arguments. */
    CONST char **argv;		/* The argument strings. */
{
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NoopObjCmd --
 *
 *	This object-based procedure is just used to time the overhead
 *	involved in parsing and invoking a command.
 *
 * Results:
 *	Returns the TCL_OK result code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
NoopObjCmd(unused, interp, objc, objv)
    ClientData unused;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* The argument objects. */
{
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestsetCmd --
 *
 *	Implements the "testset{err,noerr}" cmds that are used when testing
 *	Tcl_Set/GetVar C Api with/without TCL_LEAVE_ERR_MSG flag
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *     Variables may be set.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestsetCmd(data, interp, argc, argv)
    ClientData data;			/* Additional flags for Get/SetVar2. */
    register Tcl_Interp *interp;	/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    int flags = (int) data;
    CONST char *value;

    if (argc == 2) {
        Tcl_SetResult(interp, "before get", TCL_STATIC);
	value = Tcl_GetVar2(interp, argv[1], (char *) NULL, flags);
        if (value == NULL) {
            return TCL_ERROR;
        }
	Tcl_AppendElement(interp, value);
        return TCL_OK;
    } else if (argc == 3) {
	Tcl_SetResult(interp, "before set", TCL_STATIC);
        value = Tcl_SetVar2(interp, argv[1], (char *) NULL, argv[2], flags);
        if (value == NULL) {
            return TCL_ERROR;
        }
	Tcl_AppendElement(interp, value);
	return TCL_OK;
    } else {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " varName ?newValue?\"", (char *) NULL);
	return TCL_ERROR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TestsaveresultCmd --
 *
 *	Implements the "testsaveresult" cmd that is used when testing
 *	the Tcl_SaveResult, Tcl_RestoreResult, and
 *	Tcl_DiscardResult interfaces.
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
static int
TestsaveresultCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    register Tcl_Interp *interp;	/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* The argument objects. */
{
    int discard, result, index;
    Tcl_SavedResult state;
    Tcl_Obj *objPtr;
    static CONST char *optionStrings[] = {
	"append", "dynamic", "free", "object", "small", NULL
    };
    enum options {
	RESULT_APPEND, RESULT_DYNAMIC, RESULT_FREE, RESULT_OBJECT, RESULT_SMALL
    };

    /*
     * Parse arguments
     */

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "type script discard");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetBooleanFromObj(interp, objv[3], &discard) != TCL_OK) {
	return TCL_ERROR;
    }

    objPtr = NULL;		/* Lint. */
    switch ((enum options) index) {
	case RESULT_SMALL:
	    Tcl_SetResult(interp, "small result", TCL_VOLATILE);
	    break;
	case RESULT_APPEND:
	    Tcl_AppendResult(interp, "append result", NULL);
	    break;
	case RESULT_FREE: {
	    char *buf = ckalloc(200);
	    strcpy(buf, "free result");
	    Tcl_SetResult(interp, buf, TCL_DYNAMIC);
	    break;
	}
	case RESULT_DYNAMIC:
	    Tcl_SetResult(interp, "dynamic result", TestsaveresultFree);
	    break;
	case RESULT_OBJECT:
	    objPtr = Tcl_NewStringObj("object result", -1);
	    Tcl_SetObjResult(interp, objPtr);
	    break;
    }

    freeCount = 0;
    Tcl_SaveResult(interp, &state);

    if (((enum options) index) == RESULT_OBJECT) {
	result = Tcl_EvalObjEx(interp, objv[2], 0);
    } else {
	result = Tcl_Eval(interp, Tcl_GetString(objv[2]));
    }

    if (discard) {
	Tcl_DiscardResult(&state);
    } else {
	Tcl_RestoreResult(interp, &state);
	result = TCL_OK;
    }

    switch ((enum options) index) {
	case RESULT_DYNAMIC: {
	    int present = interp->freeProc == TestsaveresultFree;
	    int called = freeCount;
	    Tcl_AppendElement(interp, called ? "called" : "notCalled");
	    Tcl_AppendElement(interp, present ? "present" : "missing");
	    break;
	}
	case RESULT_OBJECT:
	    Tcl_AppendElement(interp, Tcl_GetObjResult(interp) == objPtr
		    ? "same" : "different");
	    break;
	default:
	    break;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TestsaveresultFree --
 *
 *	Special purpose freeProc used by TestsaveresultCmd.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Increments the freeCount.
 *
 *----------------------------------------------------------------------
 */

static void
TestsaveresultFree(blockPtr)
    char *blockPtr;
{
    freeCount++;
}

/*
 *----------------------------------------------------------------------
 *
 * TeststatprocCmd  --
 *
 *	Implements the "testTclStatProc" cmd that is used to test the
 *	'TclStatInsertProc' & 'TclStatDeleteProc' C Apis.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TeststatprocCmd (dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    register Tcl_Interp *interp;	/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    TclStatProc_ *proc;
    int retVal;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option arg\"", (char *) NULL);
	return TCL_ERROR;
    }

    if (strcmp(argv[2], "TclpStat") == 0) {
	proc = PretendTclpStat;
    } else if (strcmp(argv[2], "TestStatProc1") == 0) {
	proc = TestStatProc1;
    } else if (strcmp(argv[2], "TestStatProc2") == 0) {
	proc = TestStatProc2;
    } else if (strcmp(argv[2], "TestStatProc3") == 0) {
	proc = TestStatProc3;
    } else {
	Tcl_AppendResult(interp, "bad arg \"", argv[1], "\": ",
		"must be TclpStat, ",
		"TestStatProc1, TestStatProc2, or TestStatProc3",
		(char *) NULL);
	return TCL_ERROR;
    }

    if (strcmp(argv[1], "insert") == 0) {
	if (proc == PretendTclpStat) {
	    Tcl_AppendResult(interp, "bad arg \"", argv[1], "\": ",
		   "must be ",
		   "TestStatProc1, TestStatProc2, or TestStatProc3",
		   (char *) NULL);
	    return TCL_ERROR;
	}
	retVal = TclStatInsertProc(proc);
    } else if (strcmp(argv[1], "delete") == 0) {
	retVal = TclStatDeleteProc(proc);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1], "\": ",
		"must be insert or delete", (char *) NULL);
	return TCL_ERROR;
    }

    if (retVal == TCL_ERROR) {
	Tcl_AppendResult(interp, "\"", argv[2], "\": ",
		"could not be ", argv[1], "ed", (char *) NULL);
    }

    return retVal;
}

static int PretendTclpStat(path, buf)
    CONST char *path;
    struct stat *buf;
{
    int ret;
    Tcl_Obj *pathPtr = Tcl_NewStringObj(path, -1);
#ifdef TCL_WIDE_INT_IS_LONG
    Tcl_IncrRefCount(pathPtr);
    ret = TclpObjStat(pathPtr, buf);
    Tcl_DecrRefCount(pathPtr);
    return ret;
#else /* TCL_WIDE_INT_IS_LONG */
    Tcl_StatBuf realBuf;
    Tcl_IncrRefCount(pathPtr);
    ret = TclpObjStat(pathPtr, &realBuf);
    Tcl_DecrRefCount(pathPtr);
    if (ret != -1) {
#   define OUT_OF_RANGE(x) \
	(((Tcl_WideInt)(x)) < Tcl_LongAsWide(LONG_MIN) || \
	 ((Tcl_WideInt)(x)) > Tcl_LongAsWide(LONG_MAX))
#   define OUT_OF_URANGE(x) \
	(((Tcl_WideUInt)(x)) > (Tcl_WideUInt)ULONG_MAX)

	/*
	 * Perform the result-buffer overflow check manually.
	 *
	 * Note that ino_t/ino64_t is unsigned...
	 */

        if (OUT_OF_URANGE(realBuf.st_ino) || OUT_OF_RANGE(realBuf.st_size)
#   ifdef HAVE_ST_BLOCKS
		|| OUT_OF_RANGE(realBuf.st_blocks)
#   endif
	    ) {
#   ifdef EOVERFLOW
	    errno = EOVERFLOW;
#   else
#       ifdef EFBIG
            errno = EFBIG;
#       else
#           error "what error should be returned for a value out of range?"
#       endif
#   endif
	    return -1;
	}

#   undef OUT_OF_RANGE
#   undef OUT_OF_URANGE

	/*
	 * Copy across all supported fields, with possible type
	 * coercions on those fields that change between the normal
	 * and lf64 versions of the stat structure (on Solaris at
	 * least.)  This is slow when the structure sizes coincide,
	 * but that's what you get for mixing interfaces...
	 */

	buf->st_mode    = realBuf.st_mode;
	buf->st_ino     = (ino_t) realBuf.st_ino;
	buf->st_dev     = realBuf.st_dev;
	buf->st_rdev    = realBuf.st_rdev;
	buf->st_nlink   = realBuf.st_nlink;
	buf->st_uid     = realBuf.st_uid;
	buf->st_gid     = realBuf.st_gid;
	buf->st_size    = (off_t) realBuf.st_size;
	buf->st_atime   = realBuf.st_atime;
	buf->st_mtime   = realBuf.st_mtime;
	buf->st_ctime   = realBuf.st_ctime;
#   ifdef HAVE_ST_BLOCKS
	buf->st_blksize = realBuf.st_blksize;
	buf->st_blocks  = (blkcnt_t) realBuf.st_blocks;
#   endif
    }
    return ret;
#endif /* TCL_WIDE_INT_IS_LONG */
}

/* Be careful in the compares in these tests, since the Macintosh puts a  
 * leading : in the beginning of non-absolute paths before passing them 
 * into the file command procedures.
 */

static int
TestStatProc1(path, buf)
    CONST char *path;
    struct stat *buf;
{
    memset(buf, 0, sizeof(struct stat));
    buf->st_size = 1234;
    return ((strstr(path, "testStat1%.fil") == NULL) ? -1 : 0);
}


static int
TestStatProc2(path, buf)
    CONST char *path;
    struct stat *buf;
{
    memset(buf, 0, sizeof(struct stat));
    buf->st_size = 2345;
    return ((strstr(path, "testStat2%.fil") == NULL) ? -1 : 0);
}


static int
TestStatProc3(path, buf)
    CONST char *path;
    struct stat *buf;
{
    memset(buf, 0, sizeof(struct stat));
    buf->st_size = 3456;
    return ((strstr(path, "testStat3%.fil") == NULL) ? -1 : 0);
}

/*
 *----------------------------------------------------------------------
 *
 * TestmainthreadCmd  --
 *
 *	Implements the "testmainthread" cmd that is used to test the
 *	'Tcl_GetCurrentThread' API.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestmainthreadCmd (dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    register Tcl_Interp *interp;	/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
  if (argc == 1) {
      Tcl_Obj *idObj = Tcl_NewLongObj((long)Tcl_GetCurrentThread());
      Tcl_SetObjResult(interp, idObj);
      return TCL_OK;
  } else {
      Tcl_SetResult(interp, "wrong # args", TCL_STATIC);
      return TCL_ERROR;
  }
}

/*
 *----------------------------------------------------------------------
 *
 * MainLoop --
 *
 *	A main loop set by TestsetmainloopCmd below.
 *
 * Results:
 * 	None.
 *
 * Side effects:
 *	Event handlers could do anything.
 *
 *----------------------------------------------------------------------
 */

static void
MainLoop(void)
{
    while (!exitMainLoop) {
	Tcl_DoOneEvent(0);
    }
    fprintf(stdout,"Exit MainLoop\n");
    fflush(stdout);
}

/*
 *----------------------------------------------------------------------
 *
 * TestsetmainloopCmd  --
 *
 *	Implements the "testsetmainloop" cmd that is used to test the
 *	'Tcl_SetMainLoop' API.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestsetmainloopCmd (dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    register Tcl_Interp *interp;	/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
  exitMainLoop = 0;
  Tcl_SetMainLoop(MainLoop);
  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestexitmainloopCmd  --
 *
 *	Implements the "testexitmainloop" cmd that is used to test the
 *	'Tcl_SetMainLoop' API.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestexitmainloopCmd (dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    register Tcl_Interp *interp;	/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
  exitMainLoop = 1;
  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestaccessprocCmd  --
 *
 *	Implements the "testTclAccessProc" cmd that is used to test the
 *	'TclAccessInsertProc' & 'TclAccessDeleteProc' C Apis.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestaccessprocCmd (dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    register Tcl_Interp *interp;	/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    TclAccessProc_ *proc;
    int retVal;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option arg\"", (char *) NULL);
	return TCL_ERROR;
    }

    if (strcmp(argv[2], "TclpAccess") == 0) {
	proc = PretendTclpAccess;
    } else if (strcmp(argv[2], "TestAccessProc1") == 0) {
	proc = TestAccessProc1;
    } else if (strcmp(argv[2], "TestAccessProc2") == 0) {
	proc = TestAccessProc2;
    } else if (strcmp(argv[2], "TestAccessProc3") == 0) {
	proc = TestAccessProc3;
    } else {
	Tcl_AppendResult(interp, "bad arg \"", argv[1], "\": ",
		"must be TclpAccess, ",
		"TestAccessProc1, TestAccessProc2, or TestAccessProc3",
		(char *) NULL);
	return TCL_ERROR;
    }

    if (strcmp(argv[1], "insert") == 0) {
	if (proc == PretendTclpAccess) {
	    Tcl_AppendResult(interp, "bad arg \"", argv[1], "\": ",
		   "must be ",
		   "TestAccessProc1, TestAccessProc2, or TestAccessProc3",
		   (char *) NULL);
	    return TCL_ERROR;
	}
	retVal = TclAccessInsertProc(proc);
    } else if (strcmp(argv[1], "delete") == 0) {
	retVal = TclAccessDeleteProc(proc);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1], "\": ",
		"must be insert or delete", (char *) NULL);
	return TCL_ERROR;
    }

    if (retVal == TCL_ERROR) {
	Tcl_AppendResult(interp, "\"", argv[2], "\": ",
		"could not be ", argv[1], "ed", (char *) NULL);
    }

    return retVal;
}

static int PretendTclpAccess(path, mode)
    CONST char *path;
    int mode;
{
    int ret;
    Tcl_Obj *pathPtr = Tcl_NewStringObj(path, -1);
    Tcl_IncrRefCount(pathPtr);
    ret = TclpObjAccess(pathPtr, mode);
    Tcl_DecrRefCount(pathPtr);
    return ret;
}

static int
TestAccessProc1(path, mode)
    CONST char *path;
    int mode;
{
    return ((strstr(path, "testAccess1%.fil") == NULL) ? -1 : 0);
}


static int
TestAccessProc2(path, mode)
    CONST char *path;
    int mode;
{
    return ((strstr(path, "testAccess2%.fil") == NULL) ? -1 : 0);
}


static int
TestAccessProc3(path, mode)
    CONST char *path;
    int mode;
{
    return ((strstr(path, "testAccess3%.fil") == NULL) ? -1 : 0);
}

/*
 *----------------------------------------------------------------------
 *
 * TestopenfilechannelprocCmd  --
 *
 *	Implements the "testTclOpenFileChannelProc" cmd that is used to test the
 *	'TclOpenFileChannelInsertProc' & 'TclOpenFileChannelDeleteProc' C Apis.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestopenfilechannelprocCmd (dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    register Tcl_Interp *interp;	/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    TclOpenFileChannelProc_ *proc;
    int retVal;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option arg\"", (char *) NULL);
	return TCL_ERROR;
    }

    if (strcmp(argv[2], "TclpOpenFileChannel") == 0) {
	proc = PretendTclpOpenFileChannel;
    } else if (strcmp(argv[2], "TestOpenFileChannelProc1") == 0) {
	proc = TestOpenFileChannelProc1;
    } else if (strcmp(argv[2], "TestOpenFileChannelProc2") == 0) {
	proc = TestOpenFileChannelProc2;
    } else if (strcmp(argv[2], "TestOpenFileChannelProc3") == 0) {
	proc = TestOpenFileChannelProc3;
    } else {
	Tcl_AppendResult(interp, "bad arg \"", argv[1], "\": ",
		"must be TclpOpenFileChannel, ",
		"TestOpenFileChannelProc1, TestOpenFileChannelProc2, or ",
		"TestOpenFileChannelProc3",
		(char *) NULL);
	return TCL_ERROR;
    }

    if (strcmp(argv[1], "insert") == 0) {
	if (proc == PretendTclpOpenFileChannel) {
	    Tcl_AppendResult(interp, "bad arg \"", argv[1], "\": ",
		   "must be ",
		   "TestOpenFileChannelProc1, TestOpenFileChannelProc2, or ",
		   "TestOpenFileChannelProc3",
		   (char *) NULL);
	    return TCL_ERROR;
	}
	retVal = TclOpenFileChannelInsertProc(proc);
    } else if (strcmp(argv[1], "delete") == 0) {
	retVal = TclOpenFileChannelDeleteProc(proc);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1], "\": ",
		"must be insert or delete", (char *) NULL);
	return TCL_ERROR;
    }

    if (retVal == TCL_ERROR) {
	Tcl_AppendResult(interp, "\"", argv[2], "\": ",
		"could not be ", argv[1], "ed", (char *) NULL);
    }

    return retVal;
}

static Tcl_Channel
PretendTclpOpenFileChannel(interp, fileName, modeString, permissions)
    Tcl_Interp *interp;                 /* Interpreter for error reporting;
					 * can be NULL. */
    CONST char *fileName;               /* Name of file to open. */
    CONST char *modeString;             /* A list of POSIX open modes or
					 * a string such as "rw". */
    int permissions;                    /* If the open involves creating a
					 * file, with what modes to create
					 * it? */
{
    Tcl_Channel ret;
    int mode, seekFlag;
    Tcl_Obj *pathPtr;
    mode = TclGetOpenMode(interp, modeString, &seekFlag);
    if (mode == -1) {
	return NULL;
    }
    pathPtr = Tcl_NewStringObj(fileName, -1);
    Tcl_IncrRefCount(pathPtr);
    ret = TclpOpenFileChannel(interp, pathPtr, mode, permissions);
    Tcl_DecrRefCount(pathPtr);
    if (ret != NULL) {
	if (seekFlag) {
	    if (Tcl_Seek(ret, (Tcl_WideInt)0, SEEK_END) < (Tcl_WideInt)0) {
		if (interp != (Tcl_Interp *) NULL) {
		    Tcl_AppendResult(interp,
		      "could not seek to end of file while opening \"",
		      fileName, "\": ", 
		      Tcl_PosixError(interp), (char *) NULL);
		}
		Tcl_Close(NULL, ret);
		return NULL;
	    }
	}
    }
    return ret;
}

static Tcl_Channel
TestOpenFileChannelProc1(interp, fileName, modeString, permissions)
    Tcl_Interp *interp;                 /* Interpreter for error reporting;
                                         * can be NULL. */
    CONST char *fileName;               /* Name of file to open. */
    CONST char *modeString;             /* A list of POSIX open modes or
                                         * a string such as "rw". */
    int permissions;                    /* If the open involves creating a
                                         * file, with what modes to create
                                         * it? */
{
    CONST char *expectname="testOpenFileChannel1%.fil";
    Tcl_DString ds;
    
    Tcl_DStringInit(&ds);
    Tcl_JoinPath(1, &expectname, &ds);

    if (!strcmp(Tcl_DStringValue(&ds), fileName)) {
	Tcl_DStringFree(&ds);
	return (PretendTclpOpenFileChannel(interp, "__testOpenFileChannel1%__.fil",
		modeString, permissions));
    } else {
	Tcl_DStringFree(&ds);
	return (NULL);
    }
}


static Tcl_Channel
TestOpenFileChannelProc2(interp, fileName, modeString, permissions)
    Tcl_Interp *interp;                 /* Interpreter for error reporting;
                                         * can be NULL. */
    CONST char *fileName;               /* Name of file to open. */
    CONST char *modeString;             /* A list of POSIX open modes or
                                         * a string such as "rw". */
    int permissions;                    /* If the open involves creating a
                                         * file, with what modes to create
                                         * it? */
{
    CONST char *expectname="testOpenFileChannel2%.fil";
    Tcl_DString ds;
    
    Tcl_DStringInit(&ds);
    Tcl_JoinPath(1, &expectname, &ds);

    if (!strcmp(Tcl_DStringValue(&ds), fileName)) {
	Tcl_DStringFree(&ds);
	return (PretendTclpOpenFileChannel(interp, "__testOpenFileChannel2%__.fil",
		modeString, permissions));
    } else {
	Tcl_DStringFree(&ds);
	return (NULL);
    }
}


static Tcl_Channel
TestOpenFileChannelProc3(interp, fileName, modeString, permissions)
    Tcl_Interp *interp;                 /* Interpreter for error reporting;
                                         * can be NULL. */
    CONST char *fileName;               /* Name of file to open. */
    CONST char *modeString;             /* A list of POSIX open modes or
                                         * a string such as "rw". */
    int permissions;                    /* If the open involves creating a
                                         * file, with what modes to create
                                         * it? */
{
    CONST char *expectname="testOpenFileChannel3%.fil";
    Tcl_DString ds;
    
    Tcl_DStringInit(&ds);
    Tcl_JoinPath(1, &expectname, &ds);

    if (!strcmp(Tcl_DStringValue(&ds), fileName)) {
	Tcl_DStringFree(&ds);
	return (PretendTclpOpenFileChannel(interp, "__testOpenFileChannel3%__.fil",
		modeString, permissions));
    } else {
	Tcl_DStringFree(&ds);
	return (NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TestChannelCmd --
 *
 *	Implements the Tcl "testchannel" debugging command and its
 *	subcommands. This is part of the testing environment.
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
static int
TestChannelCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Interpreter for result. */
    int argc;			/* Count of additional args. */
    CONST char **argv;		/* Additional arg strings. */
{
    CONST char *cmdName;	/* Sub command. */
    Tcl_HashTable *hTblPtr;	/* Hash table of channels. */
    Tcl_HashSearch hSearch;	/* Search variable. */
    Tcl_HashEntry *hPtr;	/* Search variable. */
    Channel *chanPtr;		/* The actual channel. */
    ChannelState *statePtr;	/* state info for channel */
    Tcl_Channel chan;		/* The opaque type. */
    size_t len;			/* Length of subcommand string. */
    int IOQueued;		/* How much IO is queued inside channel? */
    ChannelBuffer *bufPtr;	/* For iterating over queued IO. */
    char buf[TCL_INTEGER_SPACE];/* For sprintf. */
    int mode;			/* rw mode of the channel */
    
    if (argc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                " subcommand ?additional args..?\"", (char *) NULL);
        return TCL_ERROR;
    }
    cmdName = argv[1];
    len = strlen(cmdName);

    chanPtr = (Channel *) NULL;

    if (argc > 2) {
        chan = Tcl_GetChannel(interp, argv[2], &mode);
        if (chan == (Tcl_Channel) NULL) {
            return TCL_ERROR;
        }
        chanPtr		= (Channel *) chan;
	statePtr	= chanPtr->state;
        chanPtr		= statePtr->topChanPtr;
	chan		= (Tcl_Channel) chanPtr;
    } else {
	/* lint */
	statePtr	= NULL;
	chan		= NULL;
    }

    if ((cmdName[0] == 'c') && (strncmp(cmdName, "cut", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " cut channelName\"", (char *) NULL);
            return TCL_ERROR;
        }
        Tcl_CutChannel(chan);
        return TCL_OK;
    }

    if ((cmdName[0] == 'c') &&
	    (strncmp(cmdName, "clearchannelhandlers", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " clearchannelhandlers channelName\"", (char *) NULL);
            return TCL_ERROR;
        }
        Tcl_ClearChannelHandlers(chan);
        return TCL_OK;
    }

    if ((cmdName[0] == 'i') && (strncmp(cmdName, "info", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
                    " info channelName\"", (char *) NULL);
            return TCL_ERROR;
        }
        Tcl_AppendElement(interp, argv[2]);
        Tcl_AppendElement(interp, Tcl_ChannelName(chanPtr->typePtr));
        if (statePtr->flags & TCL_READABLE) {
            Tcl_AppendElement(interp, "read");
        } else {
            Tcl_AppendElement(interp, "");
        }
        if (statePtr->flags & TCL_WRITABLE) {
            Tcl_AppendElement(interp, "write");
        } else {
            Tcl_AppendElement(interp, "");
        }
        if (statePtr->flags & CHANNEL_NONBLOCKING) {
            Tcl_AppendElement(interp, "nonblocking");
        } else {
            Tcl_AppendElement(interp, "blocking");
        }
        if (statePtr->flags & CHANNEL_LINEBUFFERED) {
            Tcl_AppendElement(interp, "line");
        } else if (statePtr->flags & CHANNEL_UNBUFFERED) {
            Tcl_AppendElement(interp, "none");
        } else {
            Tcl_AppendElement(interp, "full");
        }
        if (statePtr->flags & BG_FLUSH_SCHEDULED) {
            Tcl_AppendElement(interp, "async_flush");
        } else {
            Tcl_AppendElement(interp, "");
        }
        if (statePtr->flags & CHANNEL_EOF) {
            Tcl_AppendElement(interp, "eof");
        } else {
            Tcl_AppendElement(interp, "");
        }
        if (statePtr->flags & CHANNEL_BLOCKED) {
            Tcl_AppendElement(interp, "blocked");
        } else {
            Tcl_AppendElement(interp, "unblocked");
        }
        if (statePtr->inputTranslation == TCL_TRANSLATE_AUTO) {
            Tcl_AppendElement(interp, "auto");
            if (statePtr->flags & INPUT_SAW_CR) {
                Tcl_AppendElement(interp, "saw_cr");
            } else {
                Tcl_AppendElement(interp, "");
            }
        } else if (statePtr->inputTranslation == TCL_TRANSLATE_LF) {
            Tcl_AppendElement(interp, "lf");
            Tcl_AppendElement(interp, "");
        } else if (statePtr->inputTranslation == TCL_TRANSLATE_CR) {
            Tcl_AppendElement(interp, "cr");
            Tcl_AppendElement(interp, "");
        } else if (statePtr->inputTranslation == TCL_TRANSLATE_CRLF) {
            Tcl_AppendElement(interp, "crlf");
            if (statePtr->flags & INPUT_SAW_CR) {
                Tcl_AppendElement(interp, "queued_cr");
            } else {
                Tcl_AppendElement(interp, "");
            }
        }
        if (statePtr->outputTranslation == TCL_TRANSLATE_AUTO) {
            Tcl_AppendElement(interp, "auto");
        } else if (statePtr->outputTranslation == TCL_TRANSLATE_LF) {
            Tcl_AppendElement(interp, "lf");
        } else if (statePtr->outputTranslation == TCL_TRANSLATE_CR) {
            Tcl_AppendElement(interp, "cr");
        } else if (statePtr->outputTranslation == TCL_TRANSLATE_CRLF) {
            Tcl_AppendElement(interp, "crlf");
        }
        for (IOQueued = 0, bufPtr = statePtr->inQueueHead;
	     bufPtr != (ChannelBuffer *) NULL;
	     bufPtr = bufPtr->nextPtr) {
            IOQueued += bufPtr->nextAdded - bufPtr->nextRemoved;
        }
        TclFormatInt(buf, IOQueued);
        Tcl_AppendElement(interp, buf);
        
        IOQueued = 0;
        if (statePtr->curOutPtr != (ChannelBuffer *) NULL) {
            IOQueued = statePtr->curOutPtr->nextAdded -
                statePtr->curOutPtr->nextRemoved;
        }
        for (bufPtr = statePtr->outQueueHead;
	     bufPtr != (ChannelBuffer *) NULL;
	     bufPtr = bufPtr->nextPtr) {
            IOQueued += (bufPtr->nextAdded - bufPtr->nextRemoved);
        }
        TclFormatInt(buf, IOQueued);
        Tcl_AppendElement(interp, buf);
        
        TclFormatInt(buf, (int)Tcl_Tell((Tcl_Channel) chanPtr));
        Tcl_AppendElement(interp, buf);

        TclFormatInt(buf, statePtr->refCount);
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
        
        for (IOQueued = 0, bufPtr = statePtr->inQueueHead;
	     bufPtr != (ChannelBuffer *) NULL;
	     bufPtr = bufPtr->nextPtr) {
            IOQueued += bufPtr->nextAdded - bufPtr->nextRemoved;
        }
        TclFormatInt(buf, IOQueued);
        Tcl_AppendResult(interp, buf, (char *) NULL);
        return TCL_OK;
    }

    if ((cmdName[0] == 'i') && (strncmp(cmdName, "isshared", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required", (char *) NULL);
            return TCL_ERROR;
        }
        
        TclFormatInt(buf, Tcl_IsChannelShared(chan));
        Tcl_AppendResult(interp, buf, (char *) NULL);
        return TCL_OK;
    }

    if ((cmdName[0] == 'i') && (strncmp(cmdName, "isstandard", len) == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "channel name required", (char *) NULL);
	    return TCL_ERROR;
	}
	
	TclFormatInt(buf, Tcl_IsStandardChannel(chan));
	Tcl_AppendResult(interp, buf, (char *) NULL);
	return TCL_OK;
    }

    if ((cmdName[0] == 'm') && (strncmp(cmdName, "mode", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required",
                    (char *) NULL);
            return TCL_ERROR;
        }
        
        if (statePtr->flags & TCL_READABLE) {
            Tcl_AppendElement(interp, "read");
        } else {
            Tcl_AppendElement(interp, "");
        }
        if (statePtr->flags & TCL_WRITABLE) {
            Tcl_AppendElement(interp, "write");
        } else {
            Tcl_AppendElement(interp, "");
        }
        return TCL_OK;
    }
    
    if ((cmdName[0] == 'm') && (strncmp(cmdName, "mthread", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required",
                    (char *) NULL);
            return TCL_ERROR;
        }

        TclFormatInt(buf, (long) Tcl_GetChannelThread(chan));
        Tcl_AppendResult(interp, buf, (char *) NULL);
        return TCL_OK;
    }

    if ((cmdName[0] == 'n') && (strncmp(cmdName, "name", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required",
                    (char *) NULL);
            return TCL_ERROR;
        }
        Tcl_AppendResult(interp, statePtr->channelName, (char *) NULL);
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
        if (statePtr->curOutPtr != (ChannelBuffer *) NULL) {
            IOQueued = statePtr->curOutPtr->nextAdded -
                statePtr->curOutPtr->nextRemoved;
        }
        for (bufPtr = statePtr->outQueueHead;
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
                (statePtr->flags & INPUT_SAW_CR) ? "1" : "0",
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
            chanPtr  = (Channel *) Tcl_GetHashValue(hPtr);
	    statePtr = chanPtr->state;
            if (statePtr->flags & TCL_READABLE) {
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
        
        TclFormatInt(buf, statePtr->refCount);
        Tcl_AppendResult(interp, buf, (char *) NULL);
        return TCL_OK;
    }

    if ((cmdName[0] == 's') && (strncmp(cmdName, "splice", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required", (char *) NULL);
            return TCL_ERROR;
        }

        Tcl_SpliceChannel(chan);
        return TCL_OK;
    }

    if ((cmdName[0] == 't') && (strncmp(cmdName, "type", len) == 0)) {
        if (argc != 3) {
            Tcl_AppendResult(interp, "channel name required",
                    (char *) NULL);
            return TCL_ERROR;
        }
        Tcl_AppendResult(interp, Tcl_ChannelName(chanPtr->typePtr),
		(char *) NULL);
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
	    statePtr = chanPtr->state;
            if (statePtr->flags & TCL_WRITABLE) {
                Tcl_AppendElement(interp, Tcl_GetHashKey(hTblPtr, hPtr));
            }
        }
        return TCL_OK;
    }

    if ((cmdName[0] == 't') && (strncmp(cmdName, "transform", len) == 0)) {
	/*
	 * Syntax: transform channel -command command
	 */

        if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " transform channelId -command cmd\"", (char *) NULL);
            return TCL_ERROR;
        }
	if (strcmp(argv[3], "-command") != 0) {
	    Tcl_AppendResult(interp, "bad argument \"", argv[3],
		    "\": should be \"-command\"", (char *) NULL);
	    return TCL_ERROR;
	}

	return TclChannelTransform(interp, chan,
		Tcl_NewStringObj(argv[4], -1));
    }

    if ((cmdName[0] == 'u') && (strncmp(cmdName, "unstack", len) == 0)) {
	/*
	 * Syntax: unstack channel
	 */

        if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " unstack channel\"", (char *) NULL);
            return TCL_ERROR;
        }
	return Tcl_UnstackChannel(interp, chan);
    }

    Tcl_AppendResult(interp, "bad option \"", cmdName, "\": should be ",
            "cut, clearchannelhandlers, info, isshared, mode, open, "
	    "readable, splice, writable, transform, unstack",
            (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TestChannelEventCmd --
 *
 *	This procedure implements the "testchannelevent" command. It is
 *	used to test the Tcl channel event mechanism.
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
static int
TestChannelEventCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    CONST char **argv;			/* Argument strings. */
{
    Tcl_Obj *resultListPtr;
    Channel *chanPtr;
    ChannelState *statePtr;	/* state info for channel */
    EventScriptRecord *esPtr, *prevEsPtr, *nextEsPtr;
    CONST char *cmd;
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
    statePtr = chanPtr->state;

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
        esPtr->nextPtr = statePtr->scriptRecordPtr;
        statePtr->scriptRecordPtr = esPtr;
        
        esPtr->chanPtr = chanPtr;
        esPtr->interp = interp;
        esPtr->mask = mask;
	esPtr->scriptPtr = Tcl_NewStringObj(argv[4], -1);
	Tcl_IncrRefCount(esPtr->scriptPtr);

        Tcl_CreateChannelHandler((Tcl_Channel) chanPtr, mask,
                TclChannelEventScriptInvoker, (ClientData) esPtr);
        
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
        for (i = 0, esPtr = statePtr->scriptRecordPtr;
	     (i < index) && (esPtr != (EventScriptRecord *) NULL);
	     i++, esPtr = esPtr->nextPtr) {
	    /* Empty loop body. */
        }
        if (esPtr == (EventScriptRecord *) NULL) {
            Tcl_AppendResult(interp, "bad event index ", argv[3],
                    ": out of range", (char *) NULL);
            return TCL_ERROR;
        }
        if (esPtr == statePtr->scriptRecordPtr) {
            statePtr->scriptRecordPtr = esPtr->nextPtr;
        } else {
            for (prevEsPtr = statePtr->scriptRecordPtr;
		 (prevEsPtr != (EventScriptRecord *) NULL) &&
		     (prevEsPtr->nextPtr != esPtr);
		 prevEsPtr = prevEsPtr->nextPtr) {
                /* Empty loop body. */
            }
            if (prevEsPtr == (EventScriptRecord *) NULL) {
                panic("TestChannelEventCmd: damaged event script list");
            }
            prevEsPtr->nextPtr = esPtr->nextPtr;
        }
        Tcl_DeleteChannelHandler((Tcl_Channel) chanPtr,
                TclChannelEventScriptInvoker, (ClientData) esPtr);
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
        for (esPtr = statePtr->scriptRecordPtr;
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
        for (esPtr = statePtr->scriptRecordPtr;
	     esPtr != (EventScriptRecord *) NULL;
	     esPtr = nextEsPtr) {
            nextEsPtr = esPtr->nextPtr;
            Tcl_DeleteChannelHandler((Tcl_Channel) chanPtr,
                    TclChannelEventScriptInvoker, (ClientData) esPtr);
	    Tcl_DecrRefCount(esPtr->scriptPtr);
            ckfree((char *) esPtr);
        }
        statePtr->scriptRecordPtr = (EventScriptRecord *) NULL;
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
        for (i = 0, esPtr = statePtr->scriptRecordPtr;
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
                TclChannelEventScriptInvoker, (ClientData) esPtr);
	return TCL_OK;
    }    
    Tcl_AppendResult(interp, "bad command ", cmd, ", must be one of ",
            "add, delete, list, set, or removeall", (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TestWrongNumArgsObjCmd --
 *
 *	Test the Tcl_WrongNumArgs function.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	Sets interpreter result.
 *
 *----------------------------------------------------------------------
 */

static int
TestWrongNumArgsObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    int i, length;
    char *msg;

    if (objc < 3) {
	/*
	 * Don't use Tcl_WrongNumArgs here, as that is the function
	 * we want to test!
	 */
	Tcl_SetResult(interp, "insufficient arguments", TCL_STATIC);
	return TCL_ERROR;
    }
    
    if (Tcl_GetIntFromObj(interp, objv[1], &i) != TCL_OK) {
	return TCL_ERROR;
    }

    msg = Tcl_GetStringFromObj(objv[2], &length);
    if (length == 0) {
	msg = NULL;
    }
    
    if (i > objc - 3) {
	/*
	 * Asked for more arguments than were given.
	 */
	Tcl_SetResult(interp, "insufficient arguments", TCL_STATIC);
	return TCL_ERROR;
    }

    Tcl_WrongNumArgs(interp, i, &(objv[3]), msg);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestGetIndexFromObjStructObjCmd --
 *
 *	Test the Tcl_GetIndexFromObjStruct function.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	Sets interpreter result.
 *
 *----------------------------------------------------------------------
 */

static int
TestGetIndexFromObjStructObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    char *ary[] = {
	"a", "b", "c", "d", "e", "f", (char *)NULL,(char *)NULL
    };
    int idx,target;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "argument targetvalue");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[1], ary, 2*sizeof(char *),
				  "dummy", 0, &idx) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[2], &target) != TCL_OK) {
	return TCL_ERROR;
    }
    if (idx != target) {
	char buffer[64];
	sprintf(buffer, "%d", idx);
	Tcl_AppendResult(interp, "index value comparison failed: got ",
			 buffer, NULL);
	sprintf(buffer, "%d", target);
	Tcl_AppendResult(interp, " when ", buffer, " expected", NULL);
	return TCL_ERROR;
    }
    Tcl_WrongNumArgs(interp, 3, objv, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestFilesystemObjCmd --
 *
 *	This procedure implements the "testfilesystem" command.  It is
 *	used to test Tcl_FSRegister, Tcl_FSUnregister, and can be used
 *	to test that the pluggable filesystem works.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Inserts or removes a filesystem from Tcl's stack.
 *
 *----------------------------------------------------------------------
 */

static int
TestFilesystemObjCmd(dummy, interp, objc, objv)
    ClientData dummy;
    Tcl_Interp *interp;
    int		objc;
    Tcl_Obj	*CONST objv[];
{
    int res, boolVal;
    char *msg;
    
    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "boolean");
	return TCL_ERROR;
    }
    if (Tcl_GetBooleanFromObj(interp, objv[1], &boolVal) != TCL_OK) {
	return TCL_ERROR;
    }
    if (boolVal) {
	res = Tcl_FSRegister((ClientData)interp, &testReportingFilesystem);
	msg = (res == TCL_OK) ? "registered" : "failed";
    } else {
	res = Tcl_FSUnregister(&testReportingFilesystem);
	msg = (res == TCL_OK) ? "unregistered" : "failed";
    }
    Tcl_SetResult(interp, msg, TCL_VOLATILE);
    return res;
}

static int 
TestReportInFilesystem(Tcl_Obj *pathPtr, ClientData *clientDataPtr)
{
    static Tcl_Obj* lastPathPtr = NULL;
    
    if (pathPtr == lastPathPtr) {
	/* Reject all files second time around */
        return -1;
    } else {
	Tcl_Obj * newPathPtr;
	/* Try to claim all files first time around */

	newPathPtr = Tcl_DuplicateObj(pathPtr);
	lastPathPtr = newPathPtr;
	Tcl_IncrRefCount(newPathPtr);
	if (Tcl_FSGetFileSystemForPath(newPathPtr) == NULL) {
	    /* Nothing claimed it.  Therefore we don't either */
	    Tcl_DecrRefCount(newPathPtr);
	    lastPathPtr = NULL;
	    return -1;
	} else {
	    lastPathPtr = NULL;
	    *clientDataPtr = (ClientData) newPathPtr;
	    return TCL_OK;
	}
    }
}

/* 
 * Simple helper function to extract the native vfs representation of a
 * path object, or NULL if no such representation exists.
 */
static Tcl_Obj* 
TestReportGetNativePath(Tcl_Obj* pathObjPtr) {
    return (Tcl_Obj*) Tcl_FSGetInternalRep(pathObjPtr, &testReportingFilesystem);
}

static void 
TestReportFreeInternalRep(ClientData clientData) {
    Tcl_Obj *nativeRep = (Tcl_Obj*)clientData;
    if (nativeRep != NULL) {
	/* Free the path */
	Tcl_DecrRefCount(nativeRep);
    }
}

static ClientData 
TestReportDupInternalRep(ClientData clientData) {
    Tcl_Obj *original = (Tcl_Obj*)clientData;
    Tcl_IncrRefCount(original);
    return clientData;
}

static void
TestReport(cmd, path, arg2)
    CONST char* cmd;
    Tcl_Obj* path;
    Tcl_Obj* arg2;
{
    Tcl_Interp* interp = (Tcl_Interp*) Tcl_FSData(&testReportingFilesystem);
    if (interp == NULL) {
	/* This is bad, but not much we can do about it */
    } else {
	/* 
	 * No idea why I decided to program this up using the
	 * old string-based API, but there you go.  We should
	 * convert it to objects.
	 */
	Tcl_SavedResult savedResult;
	Tcl_DString ds;
	Tcl_DStringInit(&ds);
	Tcl_DStringAppend(&ds, "lappend filesystemReport ",-1);
	Tcl_DStringStartSublist(&ds);
	Tcl_DStringAppendElement(&ds, cmd);
	if (path != NULL) {
	    Tcl_DStringAppendElement(&ds, Tcl_GetString(path));
	}
	if (arg2 != NULL) {
	    Tcl_DStringAppendElement(&ds, Tcl_GetString(arg2));
	}
	Tcl_DStringEndSublist(&ds);
	Tcl_SaveResult(interp, &savedResult);
	Tcl_Eval(interp, Tcl_DStringValue(&ds));
	Tcl_DStringFree(&ds);
	Tcl_RestoreResult(interp, &savedResult);
   }
}

static int
TestReportStat(path, buf)
    Tcl_Obj *path;		/* Path of file to stat (in current CP). */
    Tcl_StatBuf *buf;		/* Filled with results of stat call. */
{
    TestReport("stat",path, NULL);
    return Tcl_FSStat(TestReportGetNativePath(path),buf);
}
static int
TestReportLstat(path, buf)
    Tcl_Obj *path;		/* Path of file to stat (in current CP). */
    Tcl_StatBuf *buf;		/* Filled with results of stat call. */
{
    TestReport("lstat",path, NULL);
    return Tcl_FSLstat(TestReportGetNativePath(path),buf);
}
static int
TestReportAccess(path, mode)
    Tcl_Obj *path;		/* Path of file to access (in current CP). */
    int mode;                   /* Permission setting. */
{
    TestReport("access",path,NULL);
    return Tcl_FSAccess(TestReportGetNativePath(path),mode);
}
static Tcl_Channel
TestReportOpenFileChannel(interp, fileName, mode, permissions)
    Tcl_Interp *interp;                 /* Interpreter for error reporting;
					 * can be NULL. */
    Tcl_Obj *fileName;                  /* Name of file to open. */
    int mode;                           /* POSIX open mode. */
    int permissions;                    /* If the open involves creating a
					 * file, with what modes to create
					 * it? */
{
    TestReport("open",fileName, NULL);
    return TclpOpenFileChannel(interp, TestReportGetNativePath(fileName),
				 mode, permissions);
}

static int
TestReportMatchInDirectory(interp, resultPtr, dirPtr, pattern, types)
    Tcl_Interp *interp;		/* Interpreter to receive results. */
    Tcl_Obj *resultPtr;		/* Object to lappend results. */
    Tcl_Obj *dirPtr;	        /* Contains path to directory to search. */
    CONST char *pattern;	/* Pattern to match against. */
    Tcl_GlobTypeData *types;	/* Object containing list of acceptable types.
				 * May be NULL. */
{
    if (types != NULL && types->type & TCL_GLOB_TYPE_MOUNT) {
	TestReport("matchmounts",dirPtr, NULL);
	return TCL_OK;
    } else {
	TestReport("matchindirectory",dirPtr, NULL);
	return Tcl_FSMatchInDirectory(interp, resultPtr, 
				      TestReportGetNativePath(dirPtr), pattern, 
				      types);
    }
}
static int
TestReportChdir(dirName)
    Tcl_Obj *dirName;
{
    TestReport("chdir",dirName,NULL);
    return Tcl_FSChdir(TestReportGetNativePath(dirName));
}
static int
TestReportLoadFile(interp, fileName,  
		   handlePtr, unloadProcPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Obj *fileName;		/* Name of the file containing the desired
				 * code. */
    Tcl_LoadHandle *handlePtr;	/* Filled with token for dynamically loaded
				 * file which will be passed back to 
				 * (*unloadProcPtr)() to unload the file. */
    Tcl_FSUnloadFileProc **unloadProcPtr;	
				/* Filled with address of Tcl_FSUnloadFileProc
				 * function which should be used for
				 * this file. */
{
    TestReport("loadfile",fileName,NULL);
    return Tcl_FSLoadFile(interp, TestReportGetNativePath(fileName), NULL, NULL,
			  NULL, NULL, handlePtr, unloadProcPtr);
}
static Tcl_Obj *
TestReportLink(path, to, linkType)
    Tcl_Obj *path;		/* Path of file to readlink or link */
    Tcl_Obj *to;		/* Path of file to link to, or NULL */
    int linkType;
{
    TestReport("link",path,to);
    return Tcl_FSLink(TestReportGetNativePath(path), to, linkType);
}
static int
TestReportRenameFile(src, dst)
    Tcl_Obj *src;		/* Pathname of file or dir to be renamed
				 * (UTF-8). */
    Tcl_Obj *dst;		/* New pathname of file or directory
				 * (UTF-8). */
{
    TestReport("renamefile",src,dst);
    return Tcl_FSRenameFile(TestReportGetNativePath(src), 
			    TestReportGetNativePath(dst));
}
static int 
TestReportCopyFile(src, dst)
    Tcl_Obj *src;		/* Pathname of file to be copied (UTF-8). */
    Tcl_Obj *dst;		/* Pathname of file to copy to (UTF-8). */
{
    TestReport("copyfile",src,dst);
    return Tcl_FSCopyFile(TestReportGetNativePath(src), 
			    TestReportGetNativePath(dst));
}
static int
TestReportDeleteFile(path)
    Tcl_Obj *path;		/* Pathname of file to be removed (UTF-8). */
{
    TestReport("deletefile",path,NULL);
    return Tcl_FSDeleteFile(TestReportGetNativePath(path));
}
static int
TestReportCreateDirectory(path)
    Tcl_Obj *path;		/* Pathname of directory to create (UTF-8). */
{
    TestReport("createdirectory",path,NULL);
    return Tcl_FSCreateDirectory(TestReportGetNativePath(path));
}
static int
TestReportCopyDirectory(src, dst, errorPtr)
    Tcl_Obj *src;		/* Pathname of directory to be copied
				 * (UTF-8). */
    Tcl_Obj *dst;		/* Pathname of target directory (UTF-8). */
    Tcl_Obj **errorPtr;	        /* If non-NULL, to be filled with UTF-8 name 
                       	         * of file causing error. */
{
    TestReport("copydirectory",src,dst);
    return Tcl_FSCopyDirectory(TestReportGetNativePath(src), 
			    TestReportGetNativePath(dst), errorPtr);
}
static int
TestReportRemoveDirectory(path, recursive, errorPtr)
    Tcl_Obj *path;		/* Pathname of directory to be removed
				 * (UTF-8). */
    int recursive;		/* If non-zero, removes directories that
				 * are nonempty.  Otherwise, will only remove
				 * empty directories. */
    Tcl_Obj **errorPtr;	        /* If non-NULL, to be filled with UTF-8 name 
                       	         * of file causing error. */
{
    TestReport("removedirectory",path,NULL);
    return Tcl_FSRemoveDirectory(TestReportGetNativePath(path), recursive, 
				 errorPtr);
}
static CONST char**
TestReportFileAttrStrings(fileName, objPtrRef)
    Tcl_Obj* fileName;
    Tcl_Obj** objPtrRef;
{
    TestReport("fileattributestrings",fileName,NULL);
    return Tcl_FSFileAttrStrings(TestReportGetNativePath(fileName), objPtrRef);
}
static int
TestReportFileAttrsGet(interp, index, fileName, objPtrRef)
    Tcl_Interp *interp;		/* The interpreter for error reporting. */
    int index;			/* index of the attribute command. */
    Tcl_Obj *fileName;		/* filename we are operating on. */
    Tcl_Obj **objPtrRef;	/* for output. */
{
    TestReport("fileattributesget",fileName,NULL);
    return Tcl_FSFileAttrsGet(interp, index, 
			      TestReportGetNativePath(fileName), objPtrRef);
}
static int
TestReportFileAttrsSet(interp, index, fileName, objPtr)
    Tcl_Interp *interp;		/* The interpreter for error reporting. */
    int index;			/* index of the attribute command. */
    Tcl_Obj *fileName;		/* filename we are operating on. */
    Tcl_Obj *objPtr;		/* for input. */
{
    TestReport("fileattributesset",fileName,objPtr);
    return Tcl_FSFileAttrsSet(interp, index, 
			      TestReportGetNativePath(fileName), objPtr);
}
static int 
TestReportUtime (fileName, tval)
    Tcl_Obj* fileName;
    struct utimbuf *tval;
{
    TestReport("utime",fileName,NULL);
    return Tcl_FSUtime(TestReportGetNativePath(fileName), tval);
}
static int
TestReportNormalizePath(interp, pathPtr, nextCheckpoint)
    Tcl_Interp *interp;
    Tcl_Obj *pathPtr;
    int nextCheckpoint;
{
    TestReport("normalizepath",pathPtr,NULL);
    return nextCheckpoint;
}

static int 
SimplePathInFilesystem(Tcl_Obj *pathPtr, ClientData *clientDataPtr) {
    CONST char *str = Tcl_GetString(pathPtr);
    if (strncmp(str,"simplefs:/",10)) {
	return -1;
    }
    return TCL_OK;
}

/* 
 * Since TclCopyChannel insists on an interpreter, we use this
 * to simplify our test scripts.  Would be better if it could
 * copy without an interp
 */
static Tcl_Interp *simpleInterpPtr = NULL;
/* We use this to ensure we clean up after ourselves */
static Tcl_Obj *tempFile = NULL;

/* 
 * This is a very 'hacky' filesystem which is used just to 
 * test two important features of the vfs code: (1) that
 * you can load a shared library from a vfs, (2) that when
 * copying files from one fs to another, the 'mtime' is
 * preserved.
 * 
 * It treats any file in 'simplefs:/' as a file, and
 * artificially creates a real file on the fly which it uses
 * to extract information from.  The real file it uses is
 * whatever follows the trailing '/' (e.g. 'foo' in 'simplefs:/foo'),
 * and that file is assumed to exist in the native pwd, and is
 * copied over to the native temporary directory where it is
 * accessed.
 * 
 * Please do not consider this filesystem a model of how
 * things are to be done.  It is quite the opposite!  But, it
 * does allow us to test two important features.
 * 
 * Finally: this fs can only be used from one interpreter.
 */
static int
TestSimpleFilesystemObjCmd(dummy, interp, objc, objv)
    ClientData dummy;
    Tcl_Interp *interp;
    int		objc;
    Tcl_Obj	*CONST objv[];
{
    int res, boolVal;
    char *msg;
    
    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "boolean");
	return TCL_ERROR;
    }
    if (Tcl_GetBooleanFromObj(interp, objv[1], &boolVal) != TCL_OK) {
	return TCL_ERROR;
    }
    if (boolVal) {
	res = Tcl_FSRegister((ClientData)interp, &simpleFilesystem);
	msg = (res == TCL_OK) ? "registered" : "failed";
	simpleInterpPtr = interp;
    } else {
	if (tempFile != NULL) {
	    Tcl_FSDeleteFile(tempFile);
	    Tcl_DecrRefCount(tempFile);
	    tempFile = NULL;
	}
	res = Tcl_FSUnregister(&simpleFilesystem);
	msg = (res == TCL_OK) ? "unregistered" : "failed";
	simpleInterpPtr = NULL;
    }
    Tcl_SetResult(interp, msg, TCL_VOLATILE);
    return res;
}

/* 
 * Treats a file name 'simplefs:/foo' by copying the file 'foo'
 * in the current (native) directory to a temporary native file,
 * and then returns that native file.
 */
static Tcl_Obj*
SimpleCopy(pathPtr)
    Tcl_Obj *pathPtr;                   /* Name of file to copy. */
{
    int res;
    CONST char *str;
    Tcl_Obj *origPtr;
    Tcl_Obj *tempPtr;

    tempPtr = TclpTempFileName();
    Tcl_IncrRefCount(tempPtr);

    /* 
     * We assume the same name in the current directory is ok.
     */
    str = Tcl_GetString(pathPtr);
    origPtr = Tcl_NewStringObj(str+10,-1);
    Tcl_IncrRefCount(origPtr);

    res = TclCrossFilesystemCopy(simpleInterpPtr, origPtr, tempPtr);
    Tcl_DecrRefCount(origPtr);

    if (res != TCL_OK) {
	Tcl_FSDeleteFile(tempPtr);
	Tcl_DecrRefCount(tempPtr);
	return NULL;
    }
    return tempPtr;
}

static Tcl_Channel
SimpleOpenFileChannel(interp, pathPtr, mode, permissions)
    Tcl_Interp *interp;                 /* Interpreter for error reporting;
					 * can be NULL. */
    Tcl_Obj *pathPtr;                   /* Name of file to open. */
    int mode;             		/* POSIX open mode. */
    int permissions;                    /* If the open involves creating a
					 * file, with what modes to create
					 * it? */
{
    Tcl_Obj *tempPtr;
    Tcl_Channel chan;
    
    if ((mode != 0) && !(mode & O_RDONLY)) {
	Tcl_AppendResult(interp, "read-only", 
		(char *) NULL);
	return NULL;
    }
    
    tempPtr = SimpleCopy(pathPtr);
    
    if (tempPtr == NULL) {
	return NULL;
    }
    
    chan = Tcl_FSOpenFileChannel(interp, tempPtr, "r", permissions);

    if (tempFile != NULL) {
        Tcl_FSDeleteFile(tempFile);
	Tcl_DecrRefCount(tempFile);
	tempFile = NULL;
    }
    /* 
     * Store file pointer in this global variable so we can delete
     * it later 
     */
    tempFile = tempPtr;
    return chan;
}

static int
SimpleAccess(pathPtr, mode)
    Tcl_Obj *pathPtr;		/* Path of file to access (in current CP). */
    int mode;                   /* Permission setting. */
{
    /* All files exist */
    return TCL_OK;
}

static int
SimpleStat(pathPtr, bufPtr)
    Tcl_Obj *pathPtr;		/* Path of file to stat (in current CP). */
    Tcl_StatBuf *bufPtr;	/* Filled with results of stat call. */
{
    Tcl_Obj *tempPtr = SimpleCopy(pathPtr);
    if (tempPtr == NULL) {
	/* We just pretend the file exists anyway */
	return TCL_OK;
    } else {
	int res = Tcl_FSStat(tempPtr, bufPtr);
	Tcl_FSDeleteFile(tempPtr);
	Tcl_DecrRefCount(tempPtr);
	return res;
    }
}

static Tcl_Obj*
SimpleListVolumes(void)
{
    /* Add one new volume */
    Tcl_Obj *retVal;

    retVal = Tcl_NewStringObj("simplefs:/",-1);
    Tcl_IncrRefCount(retVal);
    return retVal;
}

/*
 * Used to check correct string-length determining in Tcl_NumUtfChars
 */
static int
TestNumUtfCharsCmd(clientData, interp, objc, objv)
    ClientData clientData;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST objv[];
{
    if (objc > 1) {
	int len = -1;
	if (objc > 2) {
	    (void) Tcl_GetStringFromObj(objv[1], &len);
	}
	len = Tcl_NumUtfChars(Tcl_GetString(objv[1]), len);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(len));
    }
    return TCL_OK;
}
