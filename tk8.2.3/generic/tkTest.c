/* 
 * tkTest.c --
 *
 *	This file contains C command procedures for a bunch of additional
 *	Tcl commands that are used for testing out Tcl's C interfaces.
 *	These commands are not normally included in Tcl applications;
 *	they're only used for testing.
 *
 * Copyright (c) 1993-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkPort.h"
#include "tkText.h"

#ifdef __WIN32__
#include "tkWinInt.h"
#endif

#ifdef MAC_TCL
#include "tkScrollbar.h"
#endif

#ifdef __UNIX__
#include "tkUnixInt.h"
#endif

/*
 * The following data structure represents the master for a test
 * image:
 */

typedef struct TImageMaster {
    Tk_ImageMaster master;	/* Tk's token for image master. */
    Tcl_Interp *interp;		/* Interpreter for application. */
    int width, height;		/* Dimensions of image. */
    char *imageName;		/* Name of image (malloc-ed). */
    char *varName;		/* Name of variable in which to log
				 * events for image (malloc-ed). */
} TImageMaster;

/*
 * The following data structure represents a particular use of a
 * particular test image.
 */

typedef struct TImageInstance {
    TImageMaster *masterPtr;	/* Pointer to master for image. */
    XColor *fg;			/* Foreground color for drawing in image. */
    GC gc;			/* Graphics context for drawing in image. */
} TImageInstance;

/*
 * The type record for test images:
 */

static int		ImageCreate _ANSI_ARGS_((Tcl_Interp *interp,
			    char *name, int argc, char **argv,
			    Tk_ImageType *typePtr, Tk_ImageMaster master,
			    ClientData *clientDataPtr));
static ClientData	ImageGet _ANSI_ARGS_((Tk_Window tkwin,
			    ClientData clientData));
static void		ImageDisplay _ANSI_ARGS_((ClientData clientData,
			    Display *display, Drawable drawable, 
			    int imageX, int imageY, int width,
			    int height, int drawableX,
			    int drawableY));
static void		ImageFree _ANSI_ARGS_((ClientData clientData,
			    Display *display));
static void		ImageDelete _ANSI_ARGS_((ClientData clientData));

static Tk_ImageType imageType = {
    "test",			/* name */
    ImageCreate,		/* createProc */
    ImageGet,			/* getProc */
    ImageDisplay,		/* displayProc */
    ImageFree,			/* freeProc */
    ImageDelete,		/* deleteProc */
    (Tk_ImageType *) NULL	/* nextPtr */
};

/*
 * One of the following structures describes each of the interpreters
 * created by the "testnewapp" command.  This information is used by
 * the "testdeleteinterps" command to destroy all of those interpreters.
 */

typedef struct NewApp {
    Tcl_Interp *interp;		/* Token for interpreter. */
    struct NewApp *nextPtr;	/* Next in list of new interpreters. */
} NewApp;

static NewApp *newAppPtr = NULL;
				/* First in list of all new interpreters. */

/*
 * Declaration for the square widget's class command procedure:
 */

extern int SquareObjCmd _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int objc, Tcl_Obj * CONST objv[]));

typedef struct CBinding {
    Tcl_Interp *interp;
    char *command;
    char *delete;
} CBinding;

/*
 * Header for trivial configuration command items.
 */

#define ODD TK_CONFIG_USER_BIT
#define EVEN (TK_CONFIG_USER_BIT << 1)

enum {
    NONE,
    ODD_TYPE, 
    EVEN_TYPE
};

typedef struct TrivialCommandHeader {
    Tcl_Interp *interp;			/* The interp that this command 
					 * lives in. */
    Tk_OptionTable optionTable;		/* The option table that go with
					 * this command. */
    Tk_Window tkwin;			/* For widgets, the window associated
					 * with this widget. */
    Tcl_Command widgetCmd;		/* For widgets, the command associated
					 * with this widget. */
} TrivialCommandHeader;



/*
 * Forward declarations for procedures defined later in this file:
 */

static int		CBindingEvalProc _ANSI_ARGS_((ClientData clientData, 
			    Tcl_Interp *interp, XEvent *eventPtr,
			    Tk_Window tkwin, KeySym keySym));
static void		CBindingFreeProc _ANSI_ARGS_((ClientData clientData));
int			Tktest_Init _ANSI_ARGS_((Tcl_Interp *interp));
static int		ImageCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, char **argv));
static int		TestcbindCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, char **argv));
static int		TestbitmapObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj * CONST objv[]));
static int		TestborderObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj * CONST objv[]));
static int		TestcolorObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj * CONST objv[]));
static int		TestcursorObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj * CONST objv[]));
static int		TestdeleteappsCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, char **argv));
static int		TestfontObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static int		TestmakeexistCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, char **argv));
static int		TestmenubarCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, char **argv));
#if defined(__WIN32__) || defined(MAC_TCL)
static int		TestmetricsCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, char **argv));
#endif
static int		TestobjconfigObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj * CONST objv[]));
static int		TestpropCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, char **argv));
static int		TestsendCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, char **argv));
static int		TesttextCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, char **argv));
#if !(defined(__WIN32__) || defined(MAC_TCL))
static int		TestwrapperCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int argc, char **argv));
#endif
static void		TrivialCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static int		TrivialConfigObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj * CONST objv[]));
static void		TrivialEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));

/*
 * External (platform specific) initialization routine:
 */

extern int		TkplatformtestInit _ANSI_ARGS_((
			    Tcl_Interp *interp));
extern int              TclThread_Init _ANSI_ARGS_((Tcl_Interp *interp));

#if !(defined(__WIN32__) && defined(MAC_TCL))
#define TkplatformtestInit(x) TCL_OK
#endif

/*
 *----------------------------------------------------------------------
 *
 * Tktest_Init --
 *
 *	This procedure performs intialization for the Tk test
 *	suite exensions.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in the interp's result if an error occurs.
 *
 * Side effects:
 *	Creates several test commands.
 *
 *----------------------------------------------------------------------
 */

int
Tktest_Init(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{
    static int initialized = 0;

    /*
     * Create additional commands for testing Tk.
     */

    if (Tcl_PkgProvide(interp, "Tktest", TK_VERSION) == TCL_ERROR) {
        return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp, "square", SquareObjCmd,
	    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testcbind", TestcbindCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testbitmap", TestbitmapObjCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testborder", TestborderObjCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testcolor", TestcolorObjCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testcursor", TestcursorObjCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testdeleteapps", TestdeleteappsCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testembed", TkpTestembedCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testobjconfig", TestobjconfigObjCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "testfont", TestfontObjCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testmakeexist", TestmakeexistCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testmenubar", TestmenubarCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
#if defined(__WIN32__) || defined(MAC_TCL)
    Tcl_CreateCommand(interp, "testmetrics", TestmetricsCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
#endif
    Tcl_CreateCommand(interp, "testprop", TestpropCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testsend", TestsendCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "testtext", TesttextCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
#if !(defined(__WIN32__) || defined(MAC_TCL))
    Tcl_CreateCommand(interp, "testwrapper", TestwrapperCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);
#endif

#ifdef TCL_THREADS
    if (TclThread_Init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
#endif
    
    /*
     * Create test image type.
     */

    if (!initialized) {
	initialized = 1;
	Tk_CreateImageType(&imageType);
    }

    /*
     * And finally add any platform specific test commands.
     */
    
    return TkplatformtestInit(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * TestcbindCmd --
 *
 *	This procedure implements the "testcbinding" command.  It provides
 *	a set of functions for testing C bindings in tkBind.c.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Depends on option;  see below.
 *
 *----------------------------------------------------------------------
 */

static int
TestcbindCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    TkWindow *winPtr;
    Tk_Window tkwin;
    ClientData object;
    CBinding *cbindPtr;
    
    
    if (argc < 4 || argc > 5) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" bindtag pattern command ?deletecommand?", (char *) NULL);
	return TCL_ERROR;
    }

    tkwin = (Tk_Window) clientData;

    if (argv[1][0] == '.') {
	winPtr = (TkWindow *) Tk_NameToWindow(interp, argv[1], tkwin);
	if (winPtr == NULL) {
	    return TCL_ERROR;
	}
	object = (ClientData) winPtr->pathName;
    } else {
	winPtr = (TkWindow *) clientData;
	object = (ClientData) Tk_GetUid(argv[1]);
    }

    if (argv[3][0] == '\0') {
	return Tk_DeleteBinding(interp, winPtr->mainPtr->bindingTable,
		object, argv[2]);
    }

    cbindPtr = (CBinding *) ckalloc(sizeof(CBinding));
    cbindPtr->interp = interp;
    cbindPtr->command =
	    strcpy((char *) ckalloc(strlen(argv[3]) + 1), argv[3]);
    if (argc == 4) {
	cbindPtr->delete = NULL;
    } else {
	cbindPtr->delete =
		strcpy((char *) ckalloc(strlen(argv[4]) + 1), argv[4]);
    }

    if (TkCreateBindingProcedure(interp, winPtr->mainPtr->bindingTable,
	    object, argv[2], CBindingEvalProc, CBindingFreeProc,
	    (ClientData) cbindPtr) == 0) {
	ckfree((char *) cbindPtr->command);
	if (cbindPtr->delete != NULL) {
	    ckfree((char *) cbindPtr->delete);
	}
	ckfree((char *) cbindPtr);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static int
CBindingEvalProc(clientData, interp, eventPtr, tkwin, keySym)
    ClientData clientData;
    Tcl_Interp *interp;
    XEvent *eventPtr;
    Tk_Window tkwin;
    KeySym keySym;
{
    CBinding *cbindPtr;

    cbindPtr = (CBinding *) clientData;
    
    return Tcl_GlobalEval(interp, cbindPtr->command);
}

static void
CBindingFreeProc(clientData)
    ClientData clientData;
{
    CBinding *cbindPtr = (CBinding *) clientData;
    
    if (cbindPtr->delete != NULL) {
	Tcl_GlobalEval(cbindPtr->interp, cbindPtr->delete);
	ckfree((char *) cbindPtr->delete);
    }
    ckfree((char *) cbindPtr->command);
    ckfree((char *) cbindPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TestbitmapObjCmd --
 *
 *	This procedure implements the "testbitmap" command, which is used
 *	to test color resource handling in tkBitmap tmp.c.
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
TestbitmapObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window for application. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "bitmap");
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, TkDebugBitmap(Tk_MainWindow(interp),
	    Tcl_GetString(objv[1])));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestborderObjCmd --
 *
 *	This procedure implements the "testborder" command, which is used
 *	to test color resource handling in tkBorder.c.
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
TestborderObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window for application. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "border");
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, TkDebugBorder(Tk_MainWindow(interp),
	    Tcl_GetString(objv[1])));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestcolorObjCmd --
 *
 *	This procedure implements the "testcolor" command, which is used
 *	to test color resource handling in tkColor.c.
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
TestcolorObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window for application. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "color");
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, TkDebugColor(Tk_MainWindow(interp),
	    Tcl_GetString(objv[1])));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestcursorObjCmd --
 *
 *	This procedure implements the "testcursor" command, which is used
 *	to test color resource handling in tkCursor.c.
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
TestcursorObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window for application. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "cursor");
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, TkDebugCursor(Tk_MainWindow(interp),
	    Tcl_GetString(objv[1])));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestdeleteappsCmd --
 *
 *	This procedure implements the "testdeleteapps" command.  It cleans
 *	up all the interpreters left behind by the "testnewapp" command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	All the intepreters created by previous calls to "testnewapp"
 *	get deleted.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestdeleteappsCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    NewApp *nextPtr;

    while (newAppPtr != NULL) {
	nextPtr = newAppPtr->nextPtr;
	Tcl_DeleteInterp(newAppPtr->interp);
	ckfree((char *) newAppPtr);
	newAppPtr = nextPtr;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestobjconfigObjCmd --
 *
 *	This procedure implements the "testobjconfig" command,
 *	which is used to test the procedures in tkConfig.c.
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
TestobjconfigObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window for application. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    static char *options[] = {"alltypes", "chain1", "chain2",
	    "configerror", "delete", "info", "internal", "new",
	    "notenoughparams", "twowindows", (char *) NULL};
    enum {
	ALL_TYPES,
	CHAIN1,
	CHAIN2,
	CONFIG_ERROR,
	DEL,			/* Can't use DELETE: VC++ compiler barfs. */
	INFO,
	INTERNAL,
	NEW,
	NOT_ENOUGH_PARAMS,
	TWO_WINDOWS
    };
    static Tk_OptionTable tables[11];	/* Holds pointers to option tables
					 * created by commands below; indexed
					 * with same values as "options"
					 * array. */
    Tk_Window mainWin = (Tk_Window) clientData;
    Tk_Window tkwin;
    int index, result = TCL_OK;

    /*
     * Structures used by the "chain1" subcommand and also shared by
     * the "chain2" subcommand:
     */

    typedef struct ExtensionWidgetRecord {
	TrivialCommandHeader header;
	Tcl_Obj *base1ObjPtr;
	Tcl_Obj *base2ObjPtr;
	Tcl_Obj *extension3ObjPtr;
	Tcl_Obj *extension4ObjPtr;
	Tcl_Obj *extension5ObjPtr;
    } ExtensionWidgetRecord;
    static Tk_OptionSpec baseSpecs[] = {
	{TK_OPTION_STRING,
		"-one", "one", "One", "one",
		Tk_Offset(ExtensionWidgetRecord, base1ObjPtr), -1},
	{TK_OPTION_STRING,
		"-two", "two", "Two", "two",
		Tk_Offset(ExtensionWidgetRecord, base2ObjPtr), -1},
	{TK_OPTION_END}
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "command");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "command", 0, &index)
	    != TCL_OK) {
	return TCL_ERROR;
    }

    switch (index) {
	case ALL_TYPES: {
	    typedef struct TypesRecord {
		TrivialCommandHeader header;
		Tcl_Obj *booleanPtr;
		Tcl_Obj *integerPtr;
		Tcl_Obj *doublePtr;
		Tcl_Obj *stringPtr;
		Tcl_Obj *stringTablePtr;
		Tcl_Obj *colorPtr;
		Tcl_Obj *fontPtr;
		Tcl_Obj *bitmapPtr;
		Tcl_Obj *borderPtr;
		Tcl_Obj *reliefPtr;
		Tcl_Obj *cursorPtr;
		Tcl_Obj *activeCursorPtr;
		Tcl_Obj *justifyPtr;
		Tcl_Obj *anchorPtr;
		Tcl_Obj *pixelPtr;
		Tcl_Obj *mmPtr;
	    } TypesRecord;
	    TypesRecord *recordPtr;
	    static char *stringTable[] = {"one", "two", "three", "four", 
		    (char *) NULL};
	    static Tk_OptionSpec typesSpecs[] = {
		{TK_OPTION_BOOLEAN,
			"-boolean", "boolean", "Boolean",
			"1", Tk_Offset(TypesRecord, booleanPtr), -1, 0, 0, 0x1},
		{TK_OPTION_INT,
			"-integer", "integer", "Integer",
			"7", Tk_Offset(TypesRecord, integerPtr), -1, 0, 0, 0x2},
		{TK_OPTION_DOUBLE,
			"-double", "double", "Double",
			"3.14159", Tk_Offset(TypesRecord, doublePtr), -1, 0, 0,
			0x4},
		{TK_OPTION_STRING,
			"-string", "string", "String",
			"foo", Tk_Offset(TypesRecord, stringPtr), -1, 
			TK_CONFIG_NULL_OK, 0, 0x8},
		{TK_OPTION_STRING_TABLE,
			"-stringtable", "StringTable", "stringTable",
			"one", Tk_Offset(TypesRecord, stringTablePtr), -1,
			TK_CONFIG_NULL_OK, (ClientData) stringTable, 0x10},
		{TK_OPTION_COLOR,
			"-color", "color", "Color",
			"red", Tk_Offset(TypesRecord, colorPtr), -1, 
			TK_CONFIG_NULL_OK, (ClientData) "black", 0x20},
		{TK_OPTION_FONT,
			"-font", "font", "Font",
			"Helvetica 12",
			Tk_Offset(TypesRecord, fontPtr), -1,
			TK_CONFIG_NULL_OK, 0, 0x40},
		{TK_OPTION_BITMAP,
			"-bitmap", "bitmap", "Bitmap",
			"gray50",
			Tk_Offset(TypesRecord, bitmapPtr), -1,
			TK_CONFIG_NULL_OK, 0, 0x80},
		{TK_OPTION_BORDER,
			"-border", "border", "Border",
			"blue", Tk_Offset(TypesRecord, borderPtr), -1,
			TK_CONFIG_NULL_OK, (ClientData) "white", 0x100},
		{TK_OPTION_RELIEF,
			"-relief", "relief", "Relief",
			"raised",
			Tk_Offset(TypesRecord, reliefPtr), -1,
			TK_CONFIG_NULL_OK, 0, 0x200},
		{TK_OPTION_CURSOR,
			"-cursor", "cursor", "Cursor",
			"xterm",
			Tk_Offset(TypesRecord, cursorPtr), -1,
			TK_CONFIG_NULL_OK, 0, 0x400},
		{TK_OPTION_JUSTIFY,
			"-justify", (char *) NULL, (char *) NULL,
			"left",
			Tk_Offset(TypesRecord, justifyPtr), -1,
			TK_CONFIG_NULL_OK, 0, 0x800},
		{TK_OPTION_ANCHOR,
			"-anchor", "anchor", "Anchor",
			(char *) NULL,
			Tk_Offset(TypesRecord, anchorPtr), -1,
			TK_CONFIG_NULL_OK, 0, 0x1000},
		{TK_OPTION_PIXELS,
			"-pixel", "pixel", "Pixel",
			"1", Tk_Offset(TypesRecord, pixelPtr), -1,
			TK_CONFIG_NULL_OK, 0, 0x2000},
		{TK_OPTION_SYNONYM,
			"-synonym", (char *) NULL, (char *) NULL,
			(char *) NULL, 0, -1, 0, (ClientData) "-color",
			0x8000},
		{TK_OPTION_END}
	    };
	    Tk_OptionTable optionTable;
	    Tk_Window tkwin;
	    optionTable = Tk_CreateOptionTable(interp,
		    typesSpecs);
	    tables[index] = optionTable;
	    tkwin = Tk_CreateWindowFromPath(interp, (Tk_Window) clientData, 
		    Tcl_GetStringFromObj(objv[2], NULL), (char *) NULL);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    Tk_SetClass(tkwin, "Test");

	    recordPtr = (TypesRecord *) ckalloc(sizeof(TypesRecord));
	    recordPtr->header.interp = interp;
	    recordPtr->header.optionTable = optionTable;
	    recordPtr->header.tkwin = tkwin;
	    recordPtr->booleanPtr = NULL;
	    recordPtr->integerPtr = NULL;
	    recordPtr->doublePtr = NULL;
	    recordPtr->stringPtr = NULL;
	    recordPtr->colorPtr = NULL;
	    recordPtr->fontPtr = NULL;
	    recordPtr->bitmapPtr = NULL;
	    recordPtr->borderPtr = NULL;
	    recordPtr->reliefPtr = NULL;
	    recordPtr->cursorPtr = NULL;
	    recordPtr->justifyPtr = NULL;
	    recordPtr->anchorPtr = NULL;
	    recordPtr->pixelPtr = NULL;
	    recordPtr->mmPtr = NULL;
	    recordPtr->stringTablePtr = NULL;
	    result = Tk_InitOptions(interp, (char *) recordPtr, optionTable,
		    tkwin);
	    if (result == TCL_OK) {
		recordPtr->header.widgetCmd = Tcl_CreateObjCommand(interp,
			Tcl_GetStringFromObj(objv[2], NULL),
			TrivialConfigObjCmd, (ClientData) recordPtr,
			TrivialCmdDeletedProc);
		Tk_CreateEventHandler(tkwin, StructureNotifyMask,
			TrivialEventProc, (ClientData) recordPtr);
		result = Tk_SetOptions(interp, (char *) recordPtr,
			optionTable, objc - 3, objv + 3, tkwin,
			(Tk_SavedOptions *) NULL, (int *) NULL);
		if (result != TCL_OK) {
		    Tk_DestroyWindow(tkwin);
		}
	    } else {
		Tk_DestroyWindow(tkwin);
		ckfree((char *) recordPtr);
	    }
	    if (result == TCL_OK) {
		Tcl_SetObjResult(interp, objv[2]);
	    }
	    break;
	}

	case CHAIN1: {
	    ExtensionWidgetRecord *recordPtr;
	    Tk_Window tkwin;
	    Tk_OptionTable optionTable;

	    tkwin = Tk_CreateWindowFromPath(interp, (Tk_Window) clientData, 
		    Tcl_GetStringFromObj(objv[2], NULL), (char *) NULL);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    Tk_SetClass(tkwin, "Test");
	    optionTable = Tk_CreateOptionTable(interp, baseSpecs);
	    tables[index] = optionTable;

	    recordPtr = (ExtensionWidgetRecord *) ckalloc(
	    	    sizeof(ExtensionWidgetRecord));
	    recordPtr->header.interp = interp;
	    recordPtr->header.optionTable = optionTable;
	    recordPtr->header.tkwin = tkwin;
	    recordPtr->base1ObjPtr = recordPtr->base2ObjPtr = NULL;
	    recordPtr->extension3ObjPtr = recordPtr->extension4ObjPtr = NULL;
	    result = Tk_InitOptions(interp, (char *) recordPtr, optionTable,
		    tkwin);
	    if (result == TCL_OK) {
		result = Tk_SetOptions(interp, (char *) recordPtr, optionTable,
			objc - 3, objv + 3, tkwin, (Tk_SavedOptions *) NULL,
			(int *) NULL);
		if (result != TCL_OK) {
		    Tk_FreeConfigOptions((char *) recordPtr, optionTable,
			    tkwin);
		}
	    }
	    if (result == TCL_OK) {
		recordPtr->header.widgetCmd = Tcl_CreateObjCommand(interp,
			Tcl_GetStringFromObj(objv[2], NULL),
			TrivialConfigObjCmd, (ClientData) recordPtr,
			TrivialCmdDeletedProc);
		Tk_CreateEventHandler(tkwin, StructureNotifyMask,
			TrivialEventProc, (ClientData) recordPtr);
		Tcl_SetObjResult(interp, objv[2]);
	    }
	    break;
	}

	case CHAIN2: {
	    ExtensionWidgetRecord *recordPtr;
	    static Tk_OptionSpec extensionSpecs[] = {
		{TK_OPTION_STRING,
			"-three", "three", "Three", "three",
			Tk_Offset(ExtensionWidgetRecord, extension3ObjPtr),
			-1},
		{TK_OPTION_STRING,
			"-four", "four", "Four", "four",
			Tk_Offset(ExtensionWidgetRecord, extension4ObjPtr),
			-1},
		{TK_OPTION_STRING,
			"-two", "two", "Two", "two and a half",
			Tk_Offset(ExtensionWidgetRecord, base2ObjPtr),
			-1},
		{TK_OPTION_STRING,
			"-oneAgain", "oneAgain", "OneAgain", "one again",
			Tk_Offset(ExtensionWidgetRecord, extension5ObjPtr),
			-1},
		{TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
			(char *) NULL, 0, -1, 0, (ClientData) baseSpecs}
	    };
	    Tk_Window tkwin;
	    Tk_OptionTable optionTable;

	    tkwin = Tk_CreateWindowFromPath(interp, (Tk_Window) clientData, 
		    Tcl_GetStringFromObj(objv[2], NULL), (char *) NULL);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    Tk_SetClass(tkwin, "Test");
	    optionTable = Tk_CreateOptionTable(interp, extensionSpecs);
	    tables[index] = optionTable;

	    recordPtr = (ExtensionWidgetRecord *) ckalloc(
	    	    sizeof(ExtensionWidgetRecord));
	    recordPtr->header.interp = interp;
	    recordPtr->header.optionTable = optionTable;
	    recordPtr->header.tkwin = tkwin;
	    recordPtr->base1ObjPtr = recordPtr->base2ObjPtr = NULL;
	    recordPtr->extension3ObjPtr = recordPtr->extension4ObjPtr = NULL;
	    recordPtr->extension5ObjPtr = NULL;
	    result = Tk_InitOptions(interp, (char *) recordPtr, optionTable,
		    tkwin);
	    if (result == TCL_OK) {
		result = Tk_SetOptions(interp, (char *) recordPtr, optionTable,
			objc - 3, objv + 3, tkwin, (Tk_SavedOptions *) NULL,
			(int *) NULL);
		if (result != TCL_OK) {
		    Tk_FreeConfigOptions((char *) recordPtr, optionTable,
			    tkwin);
		}
	    }
	    if (result == TCL_OK) {
		recordPtr->header.widgetCmd = Tcl_CreateObjCommand(interp,
			Tcl_GetStringFromObj(objv[2], NULL),
			TrivialConfigObjCmd, (ClientData) recordPtr,
			TrivialCmdDeletedProc);
		Tk_CreateEventHandler(tkwin, StructureNotifyMask,
			TrivialEventProc, (ClientData) recordPtr);
		Tcl_SetObjResult(interp, objv[2]);
	    }
	    break;
	}

	case CONFIG_ERROR: {
	    typedef struct ErrorWidgetRecord {
		Tcl_Obj *intPtr;
	    } ErrorWidgetRecord;
	    ErrorWidgetRecord widgetRecord;
	    static Tk_OptionSpec errorSpecs[] = {
		{TK_OPTION_INT, 
			"-int", "integer", "Integer",
			"bogus", Tk_Offset(ErrorWidgetRecord, intPtr)},
		{TK_OPTION_END}
	    };
	    Tk_OptionTable optionTable;

	    widgetRecord.intPtr = NULL;
	    optionTable = Tk_CreateOptionTable(interp, errorSpecs);
	    tables[index] = optionTable;
	    return Tk_InitOptions(interp, (char *) &widgetRecord, optionTable,
		    (Tk_Window) NULL);
	}

	case DEL: {
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "tableName");
		return TCL_ERROR;
	    }
	    if (Tcl_GetIndexFromObj(interp, objv[2], options, "table", 0,
		    &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (tables[index] != NULL) {
		Tk_DeleteOptionTable(tables[index]);
	    }
	    break;
	}

	case INFO: {
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "tableName");
		return TCL_ERROR;
	    }
	    if (Tcl_GetIndexFromObj(interp, objv[2], options, "table", 0,
		    &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    Tcl_SetObjResult(interp, TkDebugConfig(interp, tables[index]));
	    break;
	}

	case INTERNAL: {
	    /*
	     * This command is similar to the "alltypes" command except
	     * that it stores all the configuration options as internal
	     * forms instead of objects.
	     */

	    typedef struct InternalRecord {
		TrivialCommandHeader header;
		int boolean;
		int integer;
		double doubleValue;
		char *string;
		int index;
		XColor *colorPtr;
		Tk_Font tkfont;
		Pixmap bitmap;
		Tk_3DBorder border;
		int relief;
		Tk_Cursor cursor;
		Tk_Justify justify;
		Tk_Anchor anchor;
		int pixels;
		double mm;
		Tk_Window tkwin;
	    } InternalRecord;
	    InternalRecord *recordPtr;
	    static char *internalStringTable[] = {
		    "one", "two", "three", "four", (char *) NULL
	    };
	    static Tk_OptionSpec internalSpecs[] = {
		{TK_OPTION_BOOLEAN,
			"-boolean", "boolean", "Boolean",
			"1", -1, Tk_Offset(InternalRecord, boolean), 0, 0, 0x1},
		{TK_OPTION_INT,
			"-integer", "integer", "Integer",
			"148962237", -1, Tk_Offset(InternalRecord, integer),
			0, 0, 0x2},
		{TK_OPTION_DOUBLE,
			"-double", "double", "Double",
			"3.14159", -1, Tk_Offset(InternalRecord, doubleValue),
			0, 0, 0x4},
		{TK_OPTION_STRING,
			"-string", "string", "String",
			"foo", -1, Tk_Offset(InternalRecord, string), 
			TK_CONFIG_NULL_OK, 0, 0x8},
		{TK_OPTION_STRING_TABLE,
			"-stringtable", "StringTable", "stringTable",
			"one", -1, Tk_Offset(InternalRecord, index),
			TK_CONFIG_NULL_OK, (ClientData) internalStringTable,
			0x10},
		{TK_OPTION_COLOR,
			"-color", "color", "Color",
			"red", -1, Tk_Offset(InternalRecord, colorPtr), 
			TK_CONFIG_NULL_OK, (ClientData) "black", 0x20},
		{TK_OPTION_FONT,
			"-font", "font", "Font",
			"Helvetica 12", -1, Tk_Offset(InternalRecord, tkfont),
			TK_CONFIG_NULL_OK, 0, 0x40},
		{TK_OPTION_BITMAP,
			"-bitmap", "bitmap", "Bitmap",
			"gray50", -1, Tk_Offset(InternalRecord, bitmap),
			TK_CONFIG_NULL_OK, 0, 0x80},
		{TK_OPTION_BORDER,
			"-border", "border", "Border",
			"blue", -1, Tk_Offset(InternalRecord, border),
			TK_CONFIG_NULL_OK, (ClientData) "white", 0x100},
		{TK_OPTION_RELIEF,
			"-relief", "relief", "Relief",
			"raised", -1, Tk_Offset(InternalRecord, relief),
			TK_CONFIG_NULL_OK, 0, 0x200},
		{TK_OPTION_CURSOR,
			"-cursor", "cursor", "Cursor",
			"xterm", -1, Tk_Offset(InternalRecord, cursor),
			TK_CONFIG_NULL_OK, 0, 0x400},
		{TK_OPTION_JUSTIFY,
			"-justify", (char *) NULL, (char *) NULL,
			"left", -1, Tk_Offset(InternalRecord, justify),
			TK_CONFIG_NULL_OK, 0, 0x800},
		{TK_OPTION_ANCHOR,
			"-anchor", "anchor", "Anchor",
			(char *) NULL, -1, Tk_Offset(InternalRecord, anchor),
			TK_CONFIG_NULL_OK, 0, 0x1000},
		{TK_OPTION_PIXELS,
			"-pixel", "pixel", "Pixel",
			"1", -1, Tk_Offset(InternalRecord, pixels),
			TK_CONFIG_NULL_OK, 0, 0x2000},
		{TK_OPTION_WINDOW,
			"-window", "window", "Window",
			(char *) NULL, -1, Tk_Offset(InternalRecord, tkwin),
			TK_CONFIG_NULL_OK, 0, 0},
		{TK_OPTION_SYNONYM,
			"-synonym", (char *) NULL, (char *) NULL,
			(char *) NULL, -1, -1, 0, (ClientData) "-color",
			0x8000},
		{TK_OPTION_END}
	    };
	    Tk_OptionTable optionTable;
	    Tk_Window tkwin;
	    optionTable = Tk_CreateOptionTable(interp, internalSpecs);
	    tables[index] = optionTable;
	    tkwin = Tk_CreateWindowFromPath(interp, (Tk_Window) clientData, 
		    Tcl_GetStringFromObj(objv[2], NULL), (char *) NULL);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    Tk_SetClass(tkwin, "Test");

	    recordPtr = (InternalRecord *) ckalloc(sizeof(InternalRecord));
	    recordPtr->header.interp = interp;
	    recordPtr->header.optionTable = optionTable;
	    recordPtr->header.tkwin = tkwin;
	    recordPtr->boolean = 0;
	    recordPtr->integer = 0;
	    recordPtr->doubleValue = 0.0;
	    recordPtr->string = NULL;
	    recordPtr->index = 0;
	    recordPtr->colorPtr = NULL;
	    recordPtr->tkfont = NULL;
	    recordPtr->bitmap = None;
	    recordPtr->border = NULL;
	    recordPtr->relief = TK_RELIEF_FLAT;
	    recordPtr->cursor = NULL;
	    recordPtr->justify = TK_JUSTIFY_LEFT;
	    recordPtr->anchor = TK_ANCHOR_N;
	    recordPtr->pixels = 0;
	    recordPtr->mm = 0.0;
	    recordPtr->tkwin = NULL;
	    result = Tk_InitOptions(interp, (char *) recordPtr, optionTable,
		    tkwin);
	    if (result == TCL_OK) {
		recordPtr->header.widgetCmd = Tcl_CreateObjCommand(interp,
			Tcl_GetStringFromObj(objv[2], NULL),
			TrivialConfigObjCmd, (ClientData) recordPtr,
			TrivialCmdDeletedProc);
		Tk_CreateEventHandler(tkwin, StructureNotifyMask,
			TrivialEventProc, (ClientData) recordPtr);
		result = Tk_SetOptions(interp, (char *) recordPtr,
			optionTable, objc - 3, objv + 3, tkwin,
			(Tk_SavedOptions *) NULL, (int *) NULL);
		if (result != TCL_OK) {
		    Tk_DestroyWindow(tkwin);
		}
	    } else {
		Tk_DestroyWindow(tkwin);
		ckfree((char *) recordPtr);
	    }
	    if (result == TCL_OK) {
		Tcl_SetObjResult(interp, objv[2]);
	    }
	    break;
	}

	case NEW: {
	    typedef struct FiveRecord {
		TrivialCommandHeader header;
		Tcl_Obj *one;
		Tcl_Obj *two;
		Tcl_Obj *three;
		Tcl_Obj *four;
		Tcl_Obj *five;
	    } FiveRecord;
	    FiveRecord *recordPtr;
	    static Tk_OptionSpec smallSpecs[] = {
		{TK_OPTION_INT,
			"-one", "one", "One",
			"1",
			Tk_Offset(FiveRecord, one), -1},
		{TK_OPTION_INT,
			"-two", "two", "Two",
			"2",
			Tk_Offset(FiveRecord, two), -1},
		{TK_OPTION_INT,
			"-three", "three", "Three",
			"3",
			Tk_Offset(FiveRecord, three), -1},
		{TK_OPTION_INT,
			"-four", "four", "Four",
			"4",
			Tk_Offset(FiveRecord, four), -1},
		{TK_OPTION_STRING,
			"-five", NULL, NULL,
			NULL,
			Tk_Offset(FiveRecord, five), -1},
		{TK_OPTION_END}
	    };

	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "new name ?options?");
		return TCL_ERROR;
	    }

	    recordPtr = (FiveRecord *) ckalloc(sizeof(FiveRecord));
	    recordPtr->header.interp = interp;
	    recordPtr->header.optionTable = Tk_CreateOptionTable(interp,
		    smallSpecs);
	    tables[index] = recordPtr->header.optionTable;
	    recordPtr->header.tkwin = NULL;
	    recordPtr->one = recordPtr->two = recordPtr->three = NULL;
	    recordPtr->four = recordPtr->five = NULL;
	    Tcl_SetObjResult(interp, objv[2]);
	    result = Tk_InitOptions(interp, (char *) recordPtr, 
		    recordPtr->header.optionTable, (Tk_Window) NULL);
	    if (result == TCL_OK) {
		result = Tk_SetOptions(interp, (char *) recordPtr,
			recordPtr->header.optionTable, objc - 3, objv + 3,
			(Tk_Window) NULL, (Tk_SavedOptions *) NULL,
			(int *) NULL);
		if (result == TCL_OK) {
		    recordPtr->header.widgetCmd = Tcl_CreateObjCommand(interp, 
			    Tcl_GetStringFromObj(objv[2], NULL),
			    TrivialConfigObjCmd, (ClientData) recordPtr,
			    TrivialCmdDeletedProc);
		} else {
		    Tk_FreeConfigOptions((char *) recordPtr,
			    recordPtr->header.optionTable, (Tk_Window) NULL);
		}
	    }
	    if (result != TCL_OK) {
		ckfree((char *) recordPtr);
	    }

	    break;
	}
	case NOT_ENOUGH_PARAMS: {
	    typedef struct NotEnoughRecord {
		Tcl_Obj *fooObjPtr;
	    } NotEnoughRecord;
	    NotEnoughRecord record;
	    static Tk_OptionSpec errorSpecs[] = {
		{TK_OPTION_INT, 
			"-foo", "foo", "Foo",
			"0", Tk_Offset(NotEnoughRecord, fooObjPtr)},
		{TK_OPTION_END}
	    };
	    Tcl_Obj *newObjPtr = Tcl_NewStringObj("-foo", -1);
	    Tk_OptionTable optionTable;

	    record.fooObjPtr = NULL;

	    tkwin = Tk_CreateWindowFromPath(interp, mainWin,
		    ".config", (char *) NULL);
	    Tk_SetClass(tkwin, "Config");
	    optionTable = Tk_CreateOptionTable(interp, errorSpecs);
	    tables[index] = optionTable;
	    Tk_InitOptions(interp, (char *) &record, optionTable, tkwin);
	    if (Tk_SetOptions(interp, (char *) &record, optionTable,
		    1, &newObjPtr, tkwin, (Tk_SavedOptions *) NULL,
		    (int *) NULL)
		    != TCL_OK) {
		result = TCL_ERROR;
	    }
	    Tcl_DecrRefCount(newObjPtr);
	    Tk_FreeConfigOptions( (char *) &record, optionTable, tkwin);
	    Tk_DestroyWindow(tkwin);
	    return result;
	}

	case TWO_WINDOWS: {
	    typedef struct SlaveRecord {
		TrivialCommandHeader header;
		Tcl_Obj *windowPtr;
	    } SlaveRecord;
	    SlaveRecord *recordPtr;
	    static Tk_OptionSpec slaveSpecs[] = {
		{TK_OPTION_WINDOW,
			"-window", "window", "Window",
			".bar", Tk_Offset(SlaveRecord, windowPtr), -1,
			TK_CONFIG_NULL_OK},
	       {TK_OPTION_END}
	    };
	    Tk_Window tkwin = Tk_CreateWindowFromPath(interp,
		    (Tk_Window) clientData,
		    Tcl_GetStringFromObj(objv[2], NULL), (char *) NULL);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    Tk_SetClass(tkwin, "Test");

	    recordPtr = (SlaveRecord *) ckalloc(sizeof(SlaveRecord));
	    recordPtr->header.interp = interp;
	    recordPtr->header.optionTable = Tk_CreateOptionTable(interp,
		    slaveSpecs);
	    tables[index] = recordPtr->header.optionTable;
	    recordPtr->header.tkwin = tkwin;
	    recordPtr->windowPtr = NULL;

	    result = Tk_InitOptions(interp,  (char *) recordPtr, 
		    recordPtr->header.optionTable, tkwin);
	    if (result == TCL_OK) {
		result = Tk_SetOptions(interp, (char *) recordPtr, 
			recordPtr->header.optionTable, objc - 3, objv + 3,
			tkwin, (Tk_SavedOptions *) NULL, (int *) NULL);
		if (result == TCL_OK) {
		    recordPtr->header.widgetCmd = Tcl_CreateObjCommand(interp,
			    Tcl_GetStringFromObj(objv[2], NULL),
			    TrivialConfigObjCmd, (ClientData) recordPtr,
			    TrivialCmdDeletedProc);
		    Tk_CreateEventHandler(tkwin, StructureNotifyMask,
			    TrivialEventProc, (ClientData) recordPtr);
		    Tcl_SetObjResult(interp, objv[2]);
		} else {
		    Tk_FreeConfigOptions((char *) recordPtr, 
			    recordPtr->header.optionTable, tkwin);
		}
	    }
	    if (result != TCL_OK) {
		Tk_DestroyWindow(tkwin);
		ckfree((char *) recordPtr);
	    }
		
	}
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TrivialConfigObjCmd --
 *
 *	This command is used to test the configuration package. It only
 *	handles the "configure" and "cget" subcommands.
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
TrivialConfigObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window for application. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int result = TCL_OK;
    static char *options[] = {"cget", "configure", "csave", (char *) NULL};
    enum {
	CGET, CONFIGURE, CSAVE
    };
    Tcl_Obj *resultObjPtr;
    int index, mask;
    TrivialCommandHeader *headerPtr = (TrivialCommandHeader *) clientData;
    Tk_Window tkwin = headerPtr->tkwin;
    Tk_SavedOptions saved;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "command",
	    0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_Preserve(clientData);
    
    switch (index) {
	case CGET: {
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "option");
		result = TCL_ERROR;
		goto done;
	    }
	    resultObjPtr = Tk_GetOptionValue(interp, (char *) clientData, 
		    headerPtr->optionTable, objv[2], tkwin);
	    if (resultObjPtr != NULL) {
		Tcl_SetObjResult(interp, resultObjPtr);
		result = TCL_OK;
	    } else {
		result = TCL_ERROR;
	    }
	    break;
	}
	case CONFIGURE: {
	    if (objc == 2) {
		resultObjPtr = Tk_GetOptionInfo(interp, (char *) clientData, 
			headerPtr->optionTable, (Tcl_Obj *) NULL, tkwin);
		if (resultObjPtr == NULL) {
		    result = TCL_ERROR;
		} else {
		    Tcl_SetObjResult(interp, resultObjPtr);
		}
	    } else if (objc == 3) {
		resultObjPtr = Tk_GetOptionInfo(interp, (char *) clientData,
			headerPtr->optionTable, objv[2], tkwin);
		if (resultObjPtr == NULL) {
		    result = TCL_ERROR;
		} else {
		    Tcl_SetObjResult(interp, resultObjPtr);
		}
	    } else {
		result = Tk_SetOptions(interp, (char *) clientData,
			headerPtr->optionTable, objc - 2, objv + 2, 
			tkwin, (Tk_SavedOptions *) NULL, &mask);
		if (result == TCL_OK) {
		    Tcl_SetIntObj(Tcl_GetObjResult(interp), mask);
		}
	    }
	    break;
	}
	case CSAVE: {
	    result = Tk_SetOptions(interp, (char *) clientData,
			headerPtr->optionTable, objc - 2, objv + 2, 
			tkwin, &saved, &mask);
	    Tk_FreeSavedOptions(&saved);
	    if (result == TCL_OK) {
		Tcl_SetIntObj(Tcl_GetObjResult(interp), mask);
	    }
	    break;
	}
    }
done:
    Tcl_Release(clientData);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TrivialCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
TrivialCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    TrivialCommandHeader *headerPtr = (TrivialCommandHeader *) clientData;
    Tk_Window tkwin = headerPtr->tkwin;

    if (tkwin != NULL) {
	Tk_DestroyWindow(tkwin);
    } else if (headerPtr->optionTable != NULL) {
	/*
	 * This is a "new" object, which doesn't have a window, so
	 * we can't depend on cleaning up in the event procedure.
	 * Free its resources here.
	 */

	Tk_FreeConfigOptions((char *) clientData,
		headerPtr->optionTable, (Tk_Window) NULL);
	Tcl_EventuallyFree(clientData, TCL_DYNAMIC);
    }
}

/*
 *--------------------------------------------------------------
 *
 * TrivialEventProc --
 *
 *	A dummy event proc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.
 *
 *--------------------------------------------------------------
 */

static void
TrivialEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    TrivialCommandHeader *headerPtr = (TrivialCommandHeader *) clientData;

    if (eventPtr->type == DestroyNotify) {
	if (headerPtr->tkwin != NULL) {
	    Tk_FreeConfigOptions((char *) clientData,
		    headerPtr->optionTable, headerPtr->tkwin);
	    headerPtr->optionTable = NULL;
	    headerPtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(headerPtr->interp,
		    headerPtr->widgetCmd);
	}
	Tcl_EventuallyFree(clientData, TCL_DYNAMIC);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TestfontObjCmd --
 *
 *	This procedure implements the "testfont" command, which is used
 *	to test TkFont objects.
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
TestfontObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window for application. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    static char *options[] = {"counts", "subfonts", (char *) NULL};
    enum option {COUNTS, SUBFONTS};
    int index;
    Tk_Window tkwin;
    Tk_Font tkfont;
    
    tkwin = (Tk_Window) clientData;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "option fontName");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "command", 0, &index)
	    != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum option) index) {
	case COUNTS: {
	    Tcl_SetObjResult(interp, TkDebugFont(Tk_MainWindow(interp),
		    Tcl_GetString(objv[2])));
	    break;
	}
	case SUBFONTS: {
	    tkfont = Tk_AllocFontFromObj(interp, tkwin, objv[2]);
	    if (tkfont == NULL) {
		return TCL_ERROR;
	    }
	    TkpGetSubFonts(interp, tkfont);
	    Tk_FreeFont(tkfont);
	    break;
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ImageCreate --
 *
 *	This procedure is called by the Tk image code to create "test"
 *	images.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	The data structure for a new image is allocated.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ImageCreate(interp, name, argc, argv, typePtr, master, clientDataPtr)
    Tcl_Interp *interp;		/* Interpreter for application containing
				 * image. */
    char *name;			/* Name to use for image. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings for options (doesn't
				 * include image name or type). */
    Tk_ImageType *typePtr;	/* Pointer to our type record (not used). */
    Tk_ImageMaster master;	/* Token for image, to be used by us in
				 * later callbacks. */
    ClientData *clientDataPtr;	/* Store manager's token for image here;
				 * it will be returned in later callbacks. */
{
    TImageMaster *timPtr;
    char *varName;
    int i;

    varName = "log";
    for (i = 0; i < argc; i += 2) {
	if (strcmp(argv[i], "-variable") != 0) {
	    Tcl_AppendResult(interp, "bad option name \"", argv[i],
		    "\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if ((i+1) == argc) {
	    Tcl_AppendResult(interp, "no value given for \"", argv[i],
		    "\" option", (char *) NULL);
	    return TCL_ERROR;
	}
	varName = argv[i+1];
    }
    timPtr = (TImageMaster *) ckalloc(sizeof(TImageMaster));
    timPtr->master = master;
    timPtr->interp = interp;
    timPtr->width = 30;
    timPtr->height = 15;
    timPtr->imageName = (char *) ckalloc((unsigned) (strlen(name) + 1));
    strcpy(timPtr->imageName, name);
    timPtr->varName = (char *) ckalloc((unsigned) (strlen(varName) + 1));
    strcpy(timPtr->varName, varName);
    Tcl_CreateCommand(interp, name, ImageCmd, (ClientData) timPtr,
	    (Tcl_CmdDeleteProc *) NULL);
    *clientDataPtr = (ClientData) timPtr;
    Tk_ImageChanged(master, 0, 0, 30, 15, 30, 15);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ImageCmd --
 *
 *	This procedure implements the commands corresponding to individual
 *	images. 
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Forces windows to be created.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
ImageCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    TImageMaster *timPtr = (TImageMaster *) clientData;
    int x, y, width, height;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], "option ?arg arg ...?", (char *) NULL);
	return TCL_ERROR;
    }
    if (strcmp(argv[1], "changed") == 0) {
	if (argc != 8) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0],
		    " changed x y width height imageWidth imageHeight",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if ((Tcl_GetInt(interp, argv[2], &x) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[3], &y) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[4], &width) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[5], &height) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[6], &timPtr->width) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[7], &timPtr->height) != TCL_OK)) {
	    return TCL_ERROR;
	}
	Tk_ImageChanged(timPtr->master, x, y, width, height, timPtr->width,
		timPtr->height);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be changed", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ImageGet --
 *
 *	This procedure is called by Tk to set things up for using a
 *	test image in a particular widget.
 *
 * Results:
 *	The return value is a token for the image instance, which is
 *	used in future callbacks to ImageDisplay and ImageFree.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ClientData
ImageGet(tkwin, clientData)
    Tk_Window tkwin;		/* Token for window in which image will
				 * be used. */
    ClientData clientData;	/* Pointer to TImageMaster for image. */
{
    TImageMaster *timPtr = (TImageMaster *) clientData;
    TImageInstance *instPtr;
    char buffer[100];
    XGCValues gcValues;

    sprintf(buffer, "%s get", timPtr->imageName);
    Tcl_SetVar(timPtr->interp, timPtr->varName, buffer,
	    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE|TCL_LIST_ELEMENT);

    instPtr = (TImageInstance *) ckalloc(sizeof(TImageInstance));
    instPtr->masterPtr = timPtr;
    instPtr->fg = Tk_GetColor(timPtr->interp, tkwin, "#ff0000");
    gcValues.foreground = instPtr->fg->pixel;
    instPtr->gc = Tk_GetGC(tkwin, GCForeground, &gcValues);
    return (ClientData) instPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ImageDisplay --
 *
 *	This procedure is invoked to redisplay part or all of an
 *	image in a given drawable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The image gets partially redrawn, as an "X" that shows the
 *	exact redraw area.
 *
 *----------------------------------------------------------------------
 */

static void
ImageDisplay(clientData, display, drawable, imageX, imageY, width, height,
	drawableX, drawableY)
    ClientData clientData;	/* Pointer to TImageInstance for image. */
    Display *display;		/* Display to use for drawing. */
    Drawable drawable;		/* Where to redraw image. */
    int imageX, imageY;		/* Origin of area to redraw, relative to
				 * origin of image. */
    int width, height;		/* Dimensions of area to redraw. */
    int drawableX, drawableY;	/* Coordinates in drawable corresponding to
				 * imageX and imageY. */
{
    TImageInstance *instPtr = (TImageInstance *) clientData;
    char buffer[200 + TCL_INTEGER_SPACE * 6];

    sprintf(buffer, "%s display %d %d %d %d %d %d",
	    instPtr->masterPtr->imageName, imageX, imageY, width, height,
	    drawableX, drawableY);
    Tcl_SetVar(instPtr->masterPtr->interp, instPtr->masterPtr->varName, buffer,
	    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
    if (width > (instPtr->masterPtr->width - imageX)) {
	width = instPtr->masterPtr->width - imageX;
    }
    if (height > (instPtr->masterPtr->height - imageY)) {
	height = instPtr->masterPtr->height - imageY;
    }
    XDrawRectangle(display, drawable, instPtr->gc, drawableX, drawableY,
	    (unsigned) (width-1), (unsigned) (height-1));
    XDrawLine(display, drawable, instPtr->gc, drawableX, drawableY,
	    (int) (drawableX + width - 1), (int) (drawableY + height - 1));
    XDrawLine(display, drawable, instPtr->gc, drawableX,
	    (int) (drawableY + height - 1),
	    (int) (drawableX + width - 1), drawableY);
}

/*
 *----------------------------------------------------------------------
 *
 * ImageFree --
 *
 *	This procedure is called when an instance of an image is
 * 	no longer used.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information related to the instance is freed.
 *
 *----------------------------------------------------------------------
 */

static void
ImageFree(clientData, display)
    ClientData clientData;	/* Pointer to TImageInstance for instance. */
    Display *display;		/* Display where image was to be drawn. */
{
    TImageInstance *instPtr = (TImageInstance *) clientData;
    char buffer[200];

    sprintf(buffer, "%s free", instPtr->masterPtr->imageName);
    Tcl_SetVar(instPtr->masterPtr->interp, instPtr->masterPtr->varName, buffer,
	    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
    Tk_FreeColor(instPtr->fg);
    Tk_FreeGC(display, instPtr->gc);
    ckfree((char *) instPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ImageDelete --
 *
 *	This procedure is called to clean up a test image when
 *	an application goes away.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information about the image is deleted.
 *
 *----------------------------------------------------------------------
 */

static void
ImageDelete(clientData)
    ClientData clientData;	/* Pointer to TImageMaster for image.  When
				 * this procedure is called, no more
				 * instances exist. */
{
    TImageMaster *timPtr = (TImageMaster *) clientData;
    char buffer[100];

    sprintf(buffer, "%s delete", timPtr->imageName);
    Tcl_SetVar(timPtr->interp, timPtr->varName, buffer,
	    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE|TCL_LIST_ELEMENT);

    Tcl_DeleteCommand(timPtr->interp, timPtr->imageName);
    ckfree(timPtr->imageName);
    ckfree(timPtr->varName);
    ckfree((char *) timPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TestmakeexistCmd --
 *
 *	This procedure implements the "testmakeexist" command.  It calls
 *	Tk_MakeWindowExist on each of its arguments to force the windows
 *	to be created.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Forces windows to be created.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestmakeexistCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    Tk_Window mainWin = (Tk_Window) clientData;
    int i;
    Tk_Window tkwin;

    for (i = 1; i < argc; i++) {
	tkwin = Tk_NameToWindow(interp, argv[i], mainWin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	Tk_MakeWindowExist(tkwin);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestmenubarCmd --
 *
 *	This procedure implements the "testmenubar" command.  It is used
 *	to test the Unix facilities for creating space above a toplevel
 *	window for a menubar.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Changes menubar related stuff.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestmenubarCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
#ifdef __UNIX__
    Tk_Window mainWin = (Tk_Window) clientData;
    Tk_Window tkwin, menubar;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args;  must be \"", argv[0],
		" option ?arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }

    if (strcmp(argv[1], "window") == 0) {
	if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args;  must be \"", argv[0],
		    "window toplevel menubar\"", (char *) NULL);
	    return TCL_ERROR;
	}
	tkwin = Tk_NameToWindow(interp, argv[2], mainWin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	if (argv[3][0] == 0) {
	    TkUnixSetMenubar(tkwin, NULL);
	} else {
	    menubar = Tk_NameToWindow(interp, argv[3], mainWin);
	    if (menubar == NULL) {
		return TCL_ERROR;
	    }
	    TkUnixSetMenubar(tkwin, menubar);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be  window", (char *) NULL);
	return TCL_ERROR;
    }

    return TCL_OK;
#else
    Tcl_SetResult(interp, "testmenubar is supported only under Unix",
	    TCL_STATIC);
    return TCL_ERROR;
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TestmetricsCmd --
 *
 *	This procedure implements the testmetrics command. It provides
 *	a way to determine the size of various widget components.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef __WIN32__
static int
TestmetricsCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    char buf[TCL_INTEGER_SPACE];

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args;  must be \"", argv[0],
		" option ?arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }

    if (strcmp(argv[1], "cyvscroll") == 0) {
	sprintf(buf, "%d", GetSystemMetrics(SM_CYVSCROLL));
	Tcl_AppendResult(interp, buf, (char *) NULL);
    } else  if (strcmp(argv[1], "cxhscroll") == 0) {
	sprintf(buf, "%d", GetSystemMetrics(SM_CXHSCROLL));
	Tcl_AppendResult(interp, buf, (char *) NULL);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be cxhscroll or cyvscroll", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}
#endif
#ifdef MAC_TCL
static int
TestmetricsCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkWindow *winPtr;
    char buf[TCL_INTEGER_SPACE];

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args;  must be \"", argv[0],
		" option window\"", (char *) NULL);
	return TCL_ERROR;
    }

    winPtr = (TkWindow *) Tk_NameToWindow(interp, argv[2], tkwin);
    if (winPtr == NULL) {
	return TCL_ERROR;
    }
    
    if (strcmp(argv[1], "cyvscroll") == 0) {
	sprintf(buf, "%d", ((TkScrollbar *) winPtr->instanceData)->width);
	Tcl_AppendResult(interp, buf, (char *) NULL);
    } else  if (strcmp(argv[1], "cxhscroll") == 0) {
	sprintf(buf, "%d", ((TkScrollbar *) winPtr->instanceData)->width);
	Tcl_AppendResult(interp, buf, (char *) NULL);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be cxhscroll or cyvscroll", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * TestpropCmd --
 *
 *	This procedure implements the "testprop" command.  It fetches
 *	and prints the value of a property on a window.
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
TestpropCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    Tk_Window mainWin = (Tk_Window) clientData;
    int result, actualFormat;
    unsigned long bytesAfter, length, value;
    Atom actualType, propName;
    char *property, *p, *end;
    Window w;
    char buffer[30];

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args;  must be \"", argv[0],
		" window property\"", (char *) NULL);
	return TCL_ERROR;
    }

    w = strtoul(argv[1], &end, 0);
    propName = Tk_InternAtom(mainWin, argv[2]);
    property = NULL;
    result = XGetWindowProperty(Tk_Display(mainWin),
	    w, propName, 0, 100000, False, AnyPropertyType,
	    &actualType, &actualFormat, &length,
	    &bytesAfter, (unsigned char **) &property);
    if ((result == Success) && (actualType != None)) {
	if ((actualFormat == 8) && (actualType == XA_STRING)) {
	    for (p = property; ((unsigned long)(p-property)) < length; p++) {
		if (*p == 0) {
		    *p = '\n';
		}
	    }
	    Tcl_SetResult(interp, property, TCL_VOLATILE);
	} else {
	    for (p = property; length > 0; length--) {
		if (actualFormat == 32) {
		    value = *((long *) p);
		    p += sizeof(long);
		} else if (actualFormat == 16) {
		    value = 0xffff & (*((short *) p));
		    p += sizeof(short);
		} else {
		    value = 0xff & *p;
		    p += 1;
		}
		sprintf(buffer, "0x%lx", value);
		Tcl_AppendElement(interp, buffer);
	    }
	}
    }
    if (property != NULL) {
	XFree(property);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestsendCmd --
 *
 *	This procedure implements the "testsend" command.  It provides
 *	a set of functions for testing the "send" command and support
 *	procedure in tkSend.c.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Depends on option;  see below.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
TestsendCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
#if !(defined(__WIN32__) || defined(MAC_TCL))
    TkWindow *winPtr = (TkWindow *) clientData;
#endif

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args;  must be \"", argv[0],
		" option ?arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }

#if !(defined(__WIN32__) || defined(MAC_TCL))
    if (strcmp(argv[1], "bogus") == 0) {
	XChangeProperty(winPtr->dispPtr->display,
		RootWindow(winPtr->dispPtr->display, 0),
		winPtr->dispPtr->registryProperty, XA_INTEGER, 32,
		PropModeReplace,
		(unsigned char *) "This is bogus information", 6);
    } else if (strcmp(argv[1], "prop") == 0) {
	int result, actualFormat;
	unsigned long length, bytesAfter;
	Atom actualType, propName;
	char *property, *p, *end;
	Window w;

	if ((argc != 4) && (argc != 5)) {
	    Tcl_AppendResult(interp, "wrong # args;  must be \"", argv[0],
		    " prop window name ?value ?\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (strcmp(argv[2], "root") == 0) {
	    w = RootWindow(winPtr->dispPtr->display, 0);
	} else if (strcmp(argv[2], "comm") == 0) {
	    w = Tk_WindowId(winPtr->dispPtr->commTkwin);
	} else {
	    w = strtoul(argv[2], &end, 0);
	}
	propName = Tk_InternAtom((Tk_Window) winPtr, argv[3]);
	if (argc == 4) {
	    property = NULL;
	    result = XGetWindowProperty(winPtr->dispPtr->display,
		    w, propName, 0, 100000, False, XA_STRING,
		    &actualType, &actualFormat, &length,
		    &bytesAfter, (unsigned char **) &property);
	    if ((result == Success) && (actualType != None)
		    && (actualFormat == 8) && (actualType == XA_STRING)) {
		for (p = property; (p-property) < length; p++) {
		    if (*p == 0) {
			*p = '\n';
		    }
		}
		Tcl_SetResult(interp, property, TCL_VOLATILE);
	    }
	    if (property != NULL) {
		XFree(property);
	    }
	} else {
	    if (argv[4][0] == 0) {
		XDeleteProperty(winPtr->dispPtr->display, w, propName);
	    } else {
		for (p = argv[4]; *p != 0; p++) {
		    if (*p == '\n') {
			*p = 0;
		    }
		}
		XChangeProperty(winPtr->dispPtr->display,
			w, propName, XA_STRING, 8, PropModeReplace,
			(unsigned char *) argv[4], p-argv[4]);
	    }
	}
    } else if (strcmp(argv[1], "serial") == 0) {
	char buf[TCL_INTEGER_SPACE];
	
	sprintf(buf, "%d", tkSendSerial+1);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be bogus, prop, or serial", (char *) NULL);
	return TCL_ERROR;
    }
#endif
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TesttextCmd --
 *
 *	This procedure implements the "testtext" command.  It provides
 *	a set of functions for testing text widgets and the associated
 *	functions in tkText*.c.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Depends on option;  see below.
 *
 *----------------------------------------------------------------------
 */

static int
TesttextCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    TkText *textPtr;
    size_t len;
    int lineIndex, byteIndex, byteOffset;
    TkTextIndex index;
    char buf[64];
    Tcl_CmdInfo info;

    if (argc < 3) {
	return TCL_ERROR;
    }

    if (Tcl_GetCommandInfo(interp, argv[1], &info) == 0) {
	return TCL_ERROR;
    }
    textPtr = (TkText *) info.clientData;
    len = strlen(argv[2]);
    if (strncmp(argv[2], "byteindex", len) == 0) {
	if (argc != 5) {
	    return TCL_ERROR;
	}
	lineIndex = atoi(argv[3]) - 1;
	byteIndex = atoi(argv[4]);

	TkTextMakeByteIndex(textPtr->tree, lineIndex, byteIndex, &index);
    } else if (strncmp(argv[2], "forwbytes", len) == 0) {
	if (argc != 5) {
	    return TCL_ERROR;
	}
	if (TkTextGetIndex(interp, textPtr, argv[3], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	byteOffset = atoi(argv[4]);
	TkTextIndexForwBytes(&index, byteOffset, &index);
    } else if (strncmp(argv[2], "backbytes", len) == 0) {
	if (argc != 5) {
	    return TCL_ERROR;
	}
	if (TkTextGetIndex(interp, textPtr, argv[3], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	byteOffset = atoi(argv[4]);
	TkTextIndexBackBytes(&index, byteOffset, &index);
    } else {
	return TCL_ERROR;
    }

    TkTextSetMark(textPtr, "insert", &index);
    TkTextPrintIndex(&index, buf);
    sprintf(buf + strlen(buf), " %d", index.byteIndex);
    Tcl_AppendResult(interp, buf, NULL);

    return TCL_OK;
}

#if !(defined(__WIN32__) || defined(MAC_TCL))
/*
 *----------------------------------------------------------------------
 *
 * TestwrapperCmd --
 *
 *	This procedure implements the "testwrapper" command.  It 
 *	provides a way from Tcl to determine the extra window Tk adds
 *	in between the toplevel window and the window decorations.
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
TestwrapperCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window for application. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    TkWindow *winPtr, *wrapperPtr;
    Tk_Window tkwin;

    if (argc != 2) {
	Tcl_AppendResult(interp, "wrong # args;  must be \"", argv[0],
		" window\"", (char *) NULL);
	return TCL_ERROR;
    }
    
    tkwin = (Tk_Window) clientData;
    winPtr = (TkWindow *) Tk_NameToWindow(interp, argv[1], tkwin);
    if (winPtr == NULL) {
	return TCL_ERROR;
    }

    wrapperPtr = TkpGetWrapperWindow(winPtr);
    if (wrapperPtr != NULL) {
	char buf[TCL_INTEGER_SPACE];

	TkpPrintWindowId(buf, Tk_WindowId(wrapperPtr));
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    }
    return TCL_OK;
}
#endif
