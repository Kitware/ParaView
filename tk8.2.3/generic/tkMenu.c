/* 
 * tkMenu.c --
 *
 * This file contains most of the code for implementing menus in Tk. It takes
 * care of all of the generic (platform-independent) parts of menus, and
 * is supplemented by platform-specific files. The geometry calculation
 * and drawing code for menus is in the file tkMenuDraw.c
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

/*
 * Notes on implementation of menus:
 *
 * Menus can be used in three ways:
 * - as a popup menu, either as part of a menubutton or standalone.
 * - as a menubar. The menu's cascade items are arranged according to
 * the specific platform to provide the user access to the menus at all
 * times
 * - as a tearoff palette. This is a window with the menu's items in it.
 *
 * The goal is to provide the Tk developer with a way to use a common
 * set of menus for all of these tasks.
 *
 * In order to make the bindings for cascade menus work properly under Unix,
 * the cascade menus' pathnames must be proper children of the menu that
 * they are cascade from. So if there is a menu .m, and it has two
 * cascades labelled "File" and "Edit", the cascade menus might have
 * the pathnames .m.file and .m.edit. Another constraint is that the menus
 * used for menubars must be children of the toplevel widget that they
 * are attached to. And on the Macintosh, the platform specific menu handle
 * for cascades attached to a menu bar must have a title that matches the
 * label for the cascade menu.
 *
 * To handle all of the constraints, Tk menubars and tearoff menus are
 * implemented using menu clones. Menu clones are full menus in their own
 * right; they have a Tk window and pathname associated with them; they have
 * a TkMenu structure and array of entries. However, they are linked with the
 * original menu that they were cloned from. The reflect the attributes of
 * the original, or "master", menu. So if an item is added to a menu, and
 * that menu has clones, then the item must be added to all of its clones
 * also. Menus are cloned when a menu is torn-off or when a menu is assigned
 * as a menubar using the "-menu" option of the toplevel's pathname configure
 * subcommand. When a clone is destroyed, only the clone is destroyed, but
 * when the master menu is destroyed, all clones are also destroyed. This
 * allows the developer to just deal with one set of menus when creating
 * and destroying.
 *
 * Clones are rather tricky when a menu with cascade entries is cloned (such
 * as a menubar). Not only does the menu have to be cloned, but each cascade
 * entry's corresponding menu must also be cloned. This maintains the pathname
 * parent-child hierarchy necessary for menubars and toplevels to work.
 * This leads to several special cases:
 *
 * 1. When a new menu is created, and it is pointed to by cascade entries in
 * cloned menus, the new menu has to be cloned to parallel the cascade
 * structure.
 * 2. When a cascade item is added to a menu that has been cloned, and the
 * menu that the cascade item points to exists, that menu has to be cloned.
 * 3. When the menu that a cascade entry points to is changed, the old
 * cloned cascade menu has to be discarded, and the new one has to be cloned.
 *
 */

#if 0

/*
 * used only to test for old config code
 */

#define __NO_OLD_CONFIG
#endif

#include "tkPort.h"
#include "tkMenu.h"

#define MENU_HASH_KEY "tkMenus"

typedef struct ThreadSpecificData {
    int menusInitialized;       /* Flag indicates whether thread-specific
				 * elements of the Windows Menu module
				 * have been initialized. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * The following flag indicates whether the process-wide state for
 * the Menu module has been intialized.  The Mutex protects access to
 * that flag.
 */

static int menusInitialized;
TCL_DECLARE_MUTEX(menuMutex)

/*
 * Configuration specs for individual menu entries. If this changes, be sure
 * to update code in TkpMenuInit that changes the font string entry.
 */

char *tkMenuStateStrings[] = {"active", "normal", "disabled", (char *) NULL};

static char *menuEntryTypeStrings[] = {"cascade", "checkbutton", "command", 
	"radiobutton", "separator", (char *) NULL};

Tk_OptionSpec tkBasicMenuEntryConfigSpecs[] = {
    {TK_OPTION_BORDER, "-activebackground", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_ACTIVE_BG, Tk_Offset(TkMenuEntry, activeBorderPtr), -1, 
	TK_OPTION_NULL_OK},
    {TK_OPTION_COLOR, "-activeforeground", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_ACTIVE_FG,
	Tk_Offset(TkMenuEntry, activeFgPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_STRING, "-accelerator", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_ACCELERATOR,
	Tk_Offset(TkMenuEntry, accelPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_BORDER, "-background", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_BG,
	Tk_Offset(TkMenuEntry, borderPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_BITMAP, "-bitmap", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_BITMAP,
	Tk_Offset(TkMenuEntry, bitmapPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_BOOLEAN, "-columnbreak", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_COLUMN_BREAK,
	-1, Tk_Offset(TkMenuEntry, columnBreak)},
    {TK_OPTION_STRING, "-command", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_COMMAND,
	Tk_Offset(TkMenuEntry, commandPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_FONT, "-font", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_FONT,
	Tk_Offset(TkMenuEntry, fontPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_COLOR, "-foreground", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_FG,
	Tk_Offset(TkMenuEntry, fgPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_BOOLEAN, "-hidemargin", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_HIDE_MARGIN,
	-1, Tk_Offset(TkMenuEntry, hideMargin)},
    {TK_OPTION_STRING, "-image", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_IMAGE,
	Tk_Offset(TkMenuEntry, imagePtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_STRING, "-label", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_LABEL,
	Tk_Offset(TkMenuEntry, labelPtr), -1, 0},
    {TK_OPTION_STRING_TABLE, "-state", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_STATE,
	-1, Tk_Offset(TkMenuEntry, state), 0,
	(ClientData) tkMenuStateStrings},
    {TK_OPTION_INT, "-underline", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_UNDERLINE, -1, Tk_Offset(TkMenuEntry, underline)},
    {TK_OPTION_END}
};

Tk_OptionSpec tkSeparatorEntryConfigSpecs[] = {
    {TK_OPTION_BORDER, "-background", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_BG,
	Tk_Offset(TkMenuEntry, borderPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_END}
};

Tk_OptionSpec tkCheckButtonEntryConfigSpecs[] = {
    {TK_OPTION_BOOLEAN, "-indicatoron", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_INDICATOR,
	-1, Tk_Offset(TkMenuEntry, indicatorOn)},
    {TK_OPTION_STRING, "-offvalue", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_OFF_VALUE,
	Tk_Offset(TkMenuEntry, offValuePtr), -1},
    {TK_OPTION_STRING, "-onvalue", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_ON_VALUE,
	Tk_Offset(TkMenuEntry, onValuePtr), -1},
    {TK_OPTION_COLOR, "-selectcolor", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_SELECT,
	Tk_Offset(TkMenuEntry, indicatorFgPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_STRING, "-selectimage", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_SELECT_IMAGE,
	Tk_Offset(TkMenuEntry, selectImagePtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_STRING, "-variable", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_CHECK_VARIABLE,
	Tk_Offset(TkMenuEntry, namePtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) tkBasicMenuEntryConfigSpecs}
};

Tk_OptionSpec tkRadioButtonEntryConfigSpecs[] = {
    {TK_OPTION_BOOLEAN, "-indicatoron", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_INDICATOR,
	-1, Tk_Offset(TkMenuEntry, indicatorOn)},
    {TK_OPTION_COLOR, "-selectcolor", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_SELECT,
	Tk_Offset(TkMenuEntry, indicatorFgPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_STRING, "-selectimage", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_SELECT_IMAGE, 
	Tk_Offset(TkMenuEntry, selectImagePtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_STRING, "-value", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_VALUE,
	Tk_Offset(TkMenuEntry, onValuePtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_STRING, "-variable", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_RADIO_VARIABLE,
	Tk_Offset(TkMenuEntry, namePtr), -1, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) tkBasicMenuEntryConfigSpecs}
};

Tk_OptionSpec tkCascadeEntryConfigSpecs[] = {
    {TK_OPTION_STRING, "-menu", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_MENU,
	Tk_Offset(TkMenuEntry, namePtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) tkBasicMenuEntryConfigSpecs}
};

Tk_OptionSpec tkTearoffEntryConfigSpecs[] = {
    {TK_OPTION_BORDER, "-background", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_BG,
	Tk_Offset(TkMenuEntry, borderPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_STRING_TABLE, "-state", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_STATE, -1, Tk_Offset(TkMenuEntry, state), 0,
	(ClientData) tkMenuStateStrings},
    {TK_OPTION_END}
};

static Tk_OptionSpec *specsArray[] = {
    tkCascadeEntryConfigSpecs, tkCheckButtonEntryConfigSpecs,
    tkBasicMenuEntryConfigSpecs, tkRadioButtonEntryConfigSpecs,
    tkSeparatorEntryConfigSpecs, tkTearoffEntryConfigSpecs};
    
/*
 * Menu type strings for use with Tcl_GetIndexFromObj.
 */

static char *menuTypeStrings[] = {"normal", "tearoff", "menubar",
	(char *) NULL};

Tk_OptionSpec tkMenuConfigSpecs[] = {
    {TK_OPTION_BORDER, "-activebackground", "activeBackground", 
	"Foreground", DEF_MENU_ACTIVE_BG_COLOR, 
	Tk_Offset(TkMenu, activeBorderPtr), -1, 0,
	(ClientData) DEF_MENU_ACTIVE_BG_MONO},
    {TK_OPTION_PIXELS, "-activeborderwidth", "activeBorderWidth",
        "BorderWidth", DEF_MENU_ACTIVE_BORDER_WIDTH,
        Tk_Offset(TkMenu, activeBorderWidthPtr), -1},
    {TK_OPTION_COLOR, "-activeforeground", "activeForeground", 
	"Background", DEF_MENU_ACTIVE_FG_COLOR, 
	Tk_Offset(TkMenu, activeFgPtr), -1, 0,
	(ClientData) DEF_MENU_ACTIVE_FG_MONO},
    {TK_OPTION_BORDER, "-background", "background", "Background",
	DEF_MENU_BG_COLOR, Tk_Offset(TkMenu, borderPtr), -1, 0,
	(ClientData) DEF_MENU_BG_MONO},
    {TK_OPTION_SYNONYM, "-bd", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-borderwidth"},
    {TK_OPTION_SYNONYM, "-bg", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-background"},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_MENU_BORDER_WIDTH,
	Tk_Offset(TkMenu, borderWidthPtr), -1, 0},
    {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_MENU_CURSOR,
	Tk_Offset(TkMenu, cursorPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_COLOR, "-disabledforeground", "disabledForeground",
	"DisabledForeground", DEF_MENU_DISABLED_FG_COLOR,
	Tk_Offset(TkMenu, disabledFgPtr), -1, TK_OPTION_NULL_OK,
	(ClientData) DEF_MENU_DISABLED_FG_MONO},
    {TK_OPTION_SYNONYM, "-fg", (char *) NULL, (char *) NULL,
	(char *) NULL, 0, -1, 0, (ClientData) "-foreground"},
    {TK_OPTION_FONT, "-font", "font", "Font",
	DEF_MENU_FONT, Tk_Offset(TkMenu, fontPtr), -1},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	DEF_MENU_FG, Tk_Offset(TkMenu, fgPtr), -1},
    {TK_OPTION_STRING, "-postcommand", "postCommand", "Command",
	DEF_MENU_POST_COMMAND, 
	Tk_Offset(TkMenu, postCommandPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
	DEF_MENU_RELIEF, Tk_Offset(TkMenu, reliefPtr), -1},
    {TK_OPTION_COLOR, "-selectcolor", "selectColor", "Background",
	DEF_MENU_SELECT_COLOR, Tk_Offset(TkMenu, indicatorFgPtr), -1, 0,
	(ClientData) DEF_MENU_SELECT_MONO},
    {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_MENU_TAKE_FOCUS,
	Tk_Offset(TkMenu, takeFocusPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_BOOLEAN, "-tearoff", "tearOff", "TearOff",
	DEF_MENU_TEAROFF, -1, Tk_Offset(TkMenu, tearoff)},
    {TK_OPTION_STRING, "-tearoffcommand", "tearOffCommand", 
	"TearOffCommand", DEF_MENU_TEAROFF_CMD,
	Tk_Offset(TkMenu, tearoffCommandPtr), -1, TK_OPTION_NULL_OK},
    {TK_OPTION_STRING, "-title", "title", "Title",
	DEF_MENU_TITLE,  Tk_Offset(TkMenu, titlePtr), -1,
	TK_OPTION_NULL_OK},
    {TK_OPTION_STRING_TABLE, "-type", "type", "Type",
	DEF_MENU_TYPE, Tk_Offset(TkMenu, menuTypePtr), -1, TK_OPTION_NULL_OK,
	(ClientData) menuTypeStrings},
    {TK_OPTION_END}
};

/*
 * Command line options. Put here because MenuCmd has to look at them
 * along with MenuWidgetObjCmd.
 */

static char *menuOptions[] = {
    "activate", "add", "cget", "clone", "configure", "delete", "entrycget",
    "entryconfigure", "index", "insert", "invoke", "post", "postcascade",
    "type", "unpost", "yposition", (char *) NULL
};
enum options {
    MENU_ACTIVATE, MENU_ADD, MENU_CGET, MENU_CLONE, MENU_CONFIGURE,
    MENU_DELETE, MENU_ENTRYCGET, MENU_ENTRYCONFIGURE, MENU_INDEX,
    MENU_INSERT, MENU_INVOKE, MENU_POST, MENU_POSTCASCADE, MENU_TYPE,
    MENU_UNPOST, MENU_YPOSITION
};

/*
 * Prototypes for static procedures in this file:
 */

static int		CloneMenu _ANSI_ARGS_((TkMenu *menuPtr,
			    Tcl_Obj *newMenuName, Tcl_Obj *newMenuTypeString));
static int		ConfigureMenu _ANSI_ARGS_((Tcl_Interp *interp,
			    TkMenu *menuPtr, int objc, Tcl_Obj *CONST objv[]));
static int		ConfigureMenuCloneEntries _ANSI_ARGS_((
			    Tcl_Interp *interp, TkMenu *menuPtr, int index,
			    int objc, Tcl_Obj *CONST objv[]));
static int		ConfigureMenuEntry _ANSI_ARGS_((TkMenuEntry *mePtr,
			    int objc, Tcl_Obj *CONST objv[]));
static void		DeleteMenuCloneEntries _ANSI_ARGS_((TkMenu *menuPtr,
			    int first, int last));
static void		DestroyMenuHashTable _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp));
static void		DestroyMenuInstance _ANSI_ARGS_((TkMenu *menuPtr));
static void		DestroyMenuEntry _ANSI_ARGS_((char *memPtr));
static int		GetIndexFromCoords
			    _ANSI_ARGS_((Tcl_Interp *interp, TkMenu *menuPtr,
			    char *string, int *indexPtr));
static int		MenuDoYPosition _ANSI_ARGS_((Tcl_Interp *interp,
			    TkMenu *menuPtr, Tcl_Obj *objPtr));
static int		MenuAddOrInsert _ANSI_ARGS_((Tcl_Interp *interp,
			    TkMenu *menuPtr, Tcl_Obj *indexPtr, int objc,
			    Tcl_Obj *CONST objv[]));
static int		MenuCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[]));
static void		MenuCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static TkMenuEntry *	MenuNewEntry _ANSI_ARGS_((TkMenu *menuPtr, int index,
			    int type));
static char *		MenuVarProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, char *name1, char *name2,
			    int flags));
static int		MenuWidgetObjCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[]));
static void		MenuWorldChanged _ANSI_ARGS_((
			    ClientData instanceData));
static int		PostProcessEntry _ANSI_ARGS_((TkMenuEntry *mePtr));
static void		RecursivelyDeleteMenu _ANSI_ARGS_((TkMenu *menuPtr));
static void		UnhookCascadeEntry _ANSI_ARGS_((TkMenuEntry *mePtr));

/*
 * The structure below is a list of procs that respond to certain window
 * manager events. One of these includes a font change, which forces
 * the geometry proc to be called.
 */

static TkClassProcs menuClass = {
    NULL,			/* createProc. */
    MenuWorldChanged		/* geometryProc. */
};

/*
 *--------------------------------------------------------------
 *
 * Tk_CreateMenuCmd --
 *
 *	Called by Tk at initialization time to create the menu
 *	command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
TkCreateMenuCmd(interp)
    Tcl_Interp *interp;		/* Interpreter we are creating the 
				 * command in. */
{
    TkMenuOptionTables *optionTablesPtr = 
	    (TkMenuOptionTables *) ckalloc(sizeof(TkMenuOptionTables));

    optionTablesPtr->menuOptionTable = 
	    Tk_CreateOptionTable(interp, tkMenuConfigSpecs);
    optionTablesPtr->entryOptionTables[TEAROFF_ENTRY] =
	    Tk_CreateOptionTable(interp, specsArray[TEAROFF_ENTRY]);
    optionTablesPtr->entryOptionTables[COMMAND_ENTRY] =
	    Tk_CreateOptionTable(interp, specsArray[COMMAND_ENTRY]);
    optionTablesPtr->entryOptionTables[CASCADE_ENTRY] =
	    Tk_CreateOptionTable(interp, specsArray[CASCADE_ENTRY]);
    optionTablesPtr->entryOptionTables[SEPARATOR_ENTRY] =
	    Tk_CreateOptionTable(interp, specsArray[SEPARATOR_ENTRY]);
    optionTablesPtr->entryOptionTables[RADIO_BUTTON_ENTRY] =
	    Tk_CreateOptionTable(interp, specsArray[RADIO_BUTTON_ENTRY]);
    optionTablesPtr->entryOptionTables[CHECK_BUTTON_ENTRY] =
	    Tk_CreateOptionTable(interp, specsArray[CHECK_BUTTON_ENTRY]);

    Tcl_CreateObjCommand(interp, "menu", MenuCmd,
	    (ClientData) optionTablesPtr, NULL);

    if (Tcl_IsSafe(interp)) {
	Tcl_HideCommand(interp, "menu", "menu");
    }

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * MenuCmd --
 *
 *	This procedure is invoked to process the "menu" Tcl
 *	command.  See the user documentation for details on
 *	what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
MenuCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument strings. */
{
    Tk_Window tkwin = Tk_MainWindow(interp);
    Tk_Window new;
    register TkMenu *menuPtr;
    TkMenuReferences *menuRefPtr;
    int i, index;
    int toplevel;
    char *windowName;
    static char *typeStringList[] = {"-type", (char *) NULL};
    TkMenuOptionTables *optionTablesPtr = (TkMenuOptionTables *) clientData;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?options?");
	return TCL_ERROR;
    }

    TkMenuInit();

    toplevel = 1;
    for (i = 2; i < (objc - 1); i++) {
	if (Tcl_GetIndexFromObj(NULL, objv[i], typeStringList, NULL, 0, &index)
		!= TCL_ERROR) {
	    if ((Tcl_GetIndexFromObj(NULL, objv[i + 1], menuTypeStrings, NULL,
		    0, &index) == TCL_OK) && (index == MENUBAR)) {
		toplevel = 0;
	    }
	    break;
	}
    }

    windowName = Tcl_GetStringFromObj(objv[1], NULL);
    new = Tk_CreateWindowFromPath(interp, tkwin, windowName, toplevel ? ""
	    : NULL);
    if (new == NULL) {
	return TCL_ERROR;
    }

    /*
     * Initialize the data structure for the menu.
     */

    menuPtr = (TkMenu *) ckalloc(sizeof(TkMenu));
    menuPtr->tkwin = new;
    menuPtr->display = Tk_Display(new);
    menuPtr->interp = interp;
    menuPtr->widgetCmd = Tcl_CreateObjCommand(interp,
	    Tk_PathName(menuPtr->tkwin), MenuWidgetObjCmd,
	    (ClientData) menuPtr, MenuCmdDeletedProc);
    menuPtr->entries = NULL;
    menuPtr->numEntries = 0;
    menuPtr->active = -1;
    menuPtr->borderPtr = NULL;
    menuPtr->borderWidthPtr = NULL;
    menuPtr->reliefPtr = NULL;
    menuPtr->activeBorderPtr = NULL;
    menuPtr->activeBorderWidthPtr = NULL;
    menuPtr->fontPtr = NULL;
    menuPtr->fgPtr = NULL;
    menuPtr->disabledFgPtr = NULL;
    menuPtr->activeFgPtr = NULL;
    menuPtr->indicatorFgPtr = NULL;
    menuPtr->tearoff = 0;
    menuPtr->tearoffCommandPtr = NULL;
    menuPtr->cursorPtr = None;
    menuPtr->takeFocusPtr = NULL;
    menuPtr->postCommandPtr = NULL;
    menuPtr->postCommandGeneration = 0;
    menuPtr->postedCascade = NULL;
    menuPtr->nextInstancePtr = NULL;
    menuPtr->masterMenuPtr = menuPtr;
    menuPtr->menuType = UNKNOWN_TYPE;
    menuPtr->menuFlags = 0;
    menuPtr->parentTopLevelPtr = NULL;
    menuPtr->menuTypePtr = NULL;
    menuPtr->titlePtr = NULL;
    menuPtr->errorStructPtr = NULL;
    menuPtr->optionTablesPtr = optionTablesPtr;
    TkMenuInitializeDrawingFields(menuPtr);

    Tk_SetClass(menuPtr->tkwin, "Menu");
    TkSetClassProcs(menuPtr->tkwin, &menuClass, (ClientData) menuPtr);
    if (Tk_InitOptions(interp, (char *) menuPtr,
	    menuPtr->optionTablesPtr->menuOptionTable, menuPtr->tkwin)
	    != TCL_OK) {
    	Tk_DestroyWindow(menuPtr->tkwin);
    	ckfree((char *) menuPtr);
    	return TCL_ERROR;
    }


    menuRefPtr = TkCreateMenuReferences(menuPtr->interp,
	    Tk_PathName(menuPtr->tkwin));
    menuRefPtr->menuPtr = menuPtr;
    menuPtr->menuRefPtr = menuRefPtr;
    if (TCL_OK != TkpNewMenu(menuPtr)) {
    	Tk_DestroyWindow(menuPtr->tkwin);
    	ckfree((char *) menuPtr);
    	return TCL_ERROR;
    }

    Tk_CreateEventHandler(new, ExposureMask|StructureNotifyMask|ActivateMask,
	    TkMenuEventProc, (ClientData) menuPtr);
    if (ConfigureMenu(interp, menuPtr, objc - 2, objv + 2) != TCL_OK) {
    	Tk_DestroyWindow(menuPtr->tkwin);
    	return TCL_ERROR;
    }

    /*
     * If a menu has a parent menu pointing to it as a cascade entry, the
     * parent menu needs to be told that this menu now exists so that
     * the platform-part of the menu is correctly updated.
     *
     * If a menu has an instance and has cascade entries, then each cascade
     * menu must also have a parallel instance. This is especially true on
     * the Mac, where each menu has to have a separate title everytime it is in
     * a menubar. For instance, say you have a menu .m1 with a cascade entry
     * for .m2, where .m2 does not exist yet. You then put .m1 into a menubar.
     * This creates a menubar instance for .m1, but since .m2 is not there,
     * nothing else happens. When we go to create .m2, we hook it up properly
     * with .m1. However, we now need to clone .m2 and assign the clone of .m2
     * to be the cascade entry for the clone of .m1. This is special case
     * #1 listed in the introductory comment.
     */
    
    if (menuRefPtr->parentEntryPtr != NULL) {
        TkMenuEntry *cascadeListPtr = menuRefPtr->parentEntryPtr;
        TkMenuEntry *nextCascadePtr;
        Tcl_Obj *newMenuName;
	Tcl_Obj *newObjv[2];

        while (cascadeListPtr != NULL) {

	    nextCascadePtr = cascadeListPtr->nextCascadePtr;
     
     	    /*
     	     * If we have a new master menu, and an existing cloned menu
	     * points to this menu in a cascade entry, we have to clone
	     * the new menu and point the entry to the clone instead
	     * of the menu we are creating. Otherwise, ConfigureMenuEntry
	     * will hook up the platform-specific cascade linkages now
	     * that the menu we are creating exists.
     	     */
     	     
     	    if ((menuPtr->masterMenuPtr != menuPtr)
     	    	    || ((menuPtr->masterMenuPtr == menuPtr)
     	    	    && ((cascadeListPtr->menuPtr->masterMenuPtr
		    == cascadeListPtr->menuPtr)))) {
		newObjv[0] = Tcl_NewStringObj("-menu", -1);
		newObjv[1] = Tcl_NewStringObj(Tk_PathName(menuPtr->tkwin), -1);
		Tcl_IncrRefCount(newObjv[0]);
		Tcl_IncrRefCount(newObjv[1]);
     	    	ConfigureMenuEntry(cascadeListPtr, 2, newObjv);
		Tcl_DecrRefCount(newObjv[0]);
		Tcl_DecrRefCount(newObjv[1]);
     	    } else {
		Tcl_Obj *normalPtr = Tcl_NewStringObj("normal", -1);
		Tcl_Obj *windowNamePtr = Tcl_NewStringObj(
			Tk_PathName(cascadeListPtr->menuPtr->tkwin), -1);

		Tcl_IncrRefCount(normalPtr);
		Tcl_IncrRefCount(windowNamePtr);
      	    	newMenuName = TkNewMenuName(menuPtr->interp,
     	    		windowNamePtr, menuPtr);
		Tcl_IncrRefCount(newMenuName);
            	CloneMenu(menuPtr, newMenuName, normalPtr);
    	            
                /*
                 * Now we can set the new menu instance to be the cascade entry
                 * of the parent's instance.
                 */

		newObjv[0] = Tcl_NewStringObj("-menu", -1);
		newObjv[1] = newMenuName;
		Tcl_IncrRefCount(newObjv[0]);
                ConfigureMenuEntry(cascadeListPtr, 2, newObjv);
		Tcl_DecrRefCount(normalPtr);
		Tcl_DecrRefCount(newObjv[0]);
		Tcl_DecrRefCount(newObjv[1]);
		Tcl_DecrRefCount(windowNamePtr);
            }
            cascadeListPtr = nextCascadePtr;
        }
    }
    
    /*
     * If there already exist toplevel widgets that refer to this menu,
     * find them and notify them so that they can reconfigure their
     * geometry to reflect the menu.
     */
 
    if (menuRefPtr->topLevelListPtr != NULL) {
    	TkMenuTopLevelList *topLevelListPtr = menuRefPtr->topLevelListPtr;
    	TkMenuTopLevelList *nextPtr;
    	Tk_Window listtkwin;
   	while (topLevelListPtr != NULL) {
    	
    	    /*
    	     * Need to get the next pointer first. TkSetWindowMenuBar
    	     * changes the list, so that the next pointer is different
    	     * after calling it.
    	     */
    	
    	    nextPtr = topLevelListPtr->nextPtr;
    	    listtkwin = topLevelListPtr->tkwin;
    	    TkSetWindowMenuBar(menuPtr->interp, listtkwin, 
    	    	    Tk_PathName(menuPtr->tkwin), Tk_PathName(menuPtr->tkwin));
    	    topLevelListPtr = nextPtr;
    	}
    }

    Tcl_SetResult(interp, Tk_PathName(menuPtr->tkwin), TCL_STATIC);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * MenuWidgetObjCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
MenuWidgetObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Information about menu widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument strings. */
{
    register TkMenu *menuPtr = (TkMenu *) clientData;
    register TkMenuEntry *mePtr;
    int result = TCL_OK;
    int option;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], menuOptions, "option", 0,
	    &option) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_Preserve((ClientData) menuPtr);

    switch ((enum options) option) {
	case MENU_ACTIVATE: {
	    int index;

	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "activate index");
		goto error;
	    }
	    if (TkGetMenuIndex(interp, menuPtr, objv[2], 0, &index)
		    != TCL_OK) {
		goto error;
	    }
	    if (menuPtr->active == index) {
		goto done;
	    }
	    if ((index >= 0) 
		    && ((menuPtr->entries[index]->type == SEPARATOR_ENTRY)
			    || (menuPtr->entries[index]->state
				    == ENTRY_DISABLED))) {
		index = -1;
	    }
	    result = TkActivateMenuEntry(menuPtr, index);
	    break;
	}
	case MENU_ADD:
	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "add type ?options?");
		goto error;
	    }

	    if (MenuAddOrInsert(interp, menuPtr, (Tcl_Obj *) NULL,
		    objc - 2, objv + 2) != TCL_OK) {
		goto error;
	    }
	    break;
	case MENU_CGET: {
	    Tcl_Obj *resultPtr;

	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "cget option");
		goto error;
	    }
	    resultPtr = Tk_GetOptionValue(interp, (char *) menuPtr,
		    menuPtr->optionTablesPtr->menuOptionTable, objv[2],
		    menuPtr->tkwin);
	    if (resultPtr == NULL) {
		goto error;
	    }
	    Tcl_SetObjResult(interp, resultPtr);
	    break;
	}
	case MENU_CLONE:
	    if ((objc < 3) || (objc > 4)) {
		Tcl_WrongNumArgs(interp, 1, objv,
			"clone newMenuName ?menuType?");
		goto error;
	    }
    	    result = CloneMenu(menuPtr, objv[2], (objc == 3) ? NULL : objv[3]);
	    break;
	case MENU_CONFIGURE: {
	    Tcl_Obj *resultPtr;

	    if (objc == 2) {
		resultPtr = Tk_GetOptionInfo(interp, (char *) menuPtr,
			menuPtr->optionTablesPtr->menuOptionTable,
			(Tcl_Obj *) NULL, menuPtr->tkwin);
		if (resultPtr == NULL) {
		    result = TCL_ERROR;
		} else {
		    result = TCL_OK;
		    Tcl_SetObjResult(interp, resultPtr);
		}
	    } else if (objc == 3) {
		resultPtr = Tk_GetOptionInfo(interp, (char *) menuPtr,
			menuPtr->optionTablesPtr->menuOptionTable,
			objv[2], menuPtr->tkwin);
		if (resultPtr == NULL) {
		    result = TCL_ERROR;
		} else {
		    result = TCL_OK;
		    Tcl_SetObjResult(interp, resultPtr);
		}
	    } else {
    		result = ConfigureMenu(interp, menuPtr, objc - 2, objv + 2);
	    }
	    if (result != TCL_OK) {
		goto error;
	    }
	    break;
	}
	case MENU_DELETE: {
	    int first, last;
	    
	    if ((objc != 3) && (objc != 4)) {
		Tcl_WrongNumArgs(interp, 1, objv, "delete first ?last?");
		goto error;
	    }
	    if (TkGetMenuIndex(interp, menuPtr, objv[2], 0, &first) 
		    != TCL_OK) {
		goto error;
	    }
	    if (objc == 3) {
		last = first;
	    } else {
		if (TkGetMenuIndex(interp, menuPtr, objv[3], 0, &last) 
			!= TCL_OK) {
		    goto error;
		}
	    }
	    if (menuPtr->tearoff && (first == 0)) {

		/*
		 * Sorry, can't delete the tearoff entry;  must reconfigure
		 * the menu.
		 */
		
		first = 1;
	    }
	    if ((first < 0) || (last < first)) {
		goto done;
	    }
	    DeleteMenuCloneEntries(menuPtr, first, last);
	    break;
	}
	case MENU_ENTRYCGET: {
	    int index;
	    Tcl_Obj *resultPtr;

	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "entrycget index option");
		goto error;
	    }
	    if (TkGetMenuIndex(interp, menuPtr, objv[2], 0, &index) 
		    != TCL_OK) {
		goto error;
	    }
	    if (index < 0) {
		goto done;
	    }
	    mePtr = menuPtr->entries[index];
	    Tcl_Preserve((ClientData) mePtr);
	    resultPtr = Tk_GetOptionValue(interp, (char *) mePtr, 
		    mePtr->optionTable, objv[3], menuPtr->tkwin);
	    Tcl_Release((ClientData) mePtr);
	    if (resultPtr == NULL) {
		goto error;
	    }
	    Tcl_SetObjResult(interp, resultPtr);
	    break;
	}
	case MENU_ENTRYCONFIGURE: {
	    int index;
	    Tcl_Obj *resultPtr;

	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 1, objv, 
			"entryconfigure index ?option value ...?");
		goto error;
	    }
	    if (TkGetMenuIndex(interp, menuPtr, objv[2], 0, &index)
		    != TCL_OK) {
		goto error;
	    }
	    if (index < 0) {
		goto done;
	    }
	    mePtr = menuPtr->entries[index];
	    Tcl_Preserve((ClientData) mePtr);
	    if (objc == 3) {
		resultPtr = Tk_GetOptionInfo(interp, (char *) mePtr,
			mePtr->optionTable, (Tcl_Obj *) NULL, menuPtr->tkwin);
		if (resultPtr == NULL) {
		    result = TCL_ERROR;
		} else {
		    result = TCL_OK;
		    Tcl_SetObjResult(interp, resultPtr);
		}
	    } else if (objc == 4) {
		resultPtr = Tk_GetOptionInfo(interp, (char *) mePtr,
			mePtr->optionTable, objv[3], menuPtr->tkwin);
		if (resultPtr == NULL) {
		    result = TCL_ERROR;
		} else {
		    result = TCL_OK;
		    Tcl_SetObjResult(interp, resultPtr);
		}
	    } else {
		result = ConfigureMenuCloneEntries(interp, menuPtr, index,
			objc - 3, objv + 3);
	    }
	    Tcl_Release((ClientData) mePtr);
	    break;
	}
	case MENU_INDEX: {
	    int index;

	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "index string");
		goto error;
	    }
	    if (TkGetMenuIndex(interp, menuPtr, objv[2], 0, &index) 
		    != TCL_OK) {
		goto error;
	    }
	    if (index < 0) {
		Tcl_SetResult(interp, "none", TCL_STATIC);
	    } else {
		Tcl_SetIntObj(Tcl_GetObjResult(interp), index);
	    }
	    break;
	}
	case MENU_INSERT:
	    if (objc < 4) {
		Tcl_WrongNumArgs(interp, 1, objv, 
			"insert index type ?options?");
		goto error;
	    }
	    if (MenuAddOrInsert(interp, menuPtr, objv[2], objc - 3,
		    objv + 3) != TCL_OK) {
		goto error;
	    }
	    break;
	case MENU_INVOKE: {
	    int index;

	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "invoke index");
		goto error;
	    }
	    if (TkGetMenuIndex(interp, menuPtr, objv[2], 0, &index)
		    != TCL_OK) {
		goto error;
	    }
	    if (index < 0) {
		goto done;
	    }
	    result = TkInvokeMenu(interp, menuPtr, index);
	    break;
	}
	case MENU_POST: {
	    int x, y;

	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "post x y");
		goto error;
	    }
	    if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK)
		    || (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
		goto error;
	    }

	    /*
	     * Tearoff menus are posted differently on Mac and Windows than
	     * non-tearoffs. TkpPostMenu does not actually map the menu's
	     * window on those platforms, and popup menus have to be
	     * handled specially.
	     */
	    
    	    if (menuPtr->menuType != TEAROFF_MENU) {
    		result = TkpPostMenu(interp, menuPtr, x, y);
    	    } else {
    		result = TkPostTearoffMenu(interp, menuPtr, x, y);
    	    }
	    break;
	}
	case MENU_POSTCASCADE: {
	    int index;

	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "postcascade index");
		goto error;
	    }

	    if (TkGetMenuIndex(interp, menuPtr, objv[2], 0, &index)
		    != TCL_OK) {
		goto error;
	    }
	    if ((index < 0) || (menuPtr->entries[index]->type 
		    != CASCADE_ENTRY)) {
		result = TkPostSubmenu(interp, menuPtr, (TkMenuEntry *) NULL);
	    } else {
		result = TkPostSubmenu(interp, menuPtr, 
			menuPtr->entries[index]);
	    }
	    break;
	}
	case MENU_TYPE: {
	    int index;

	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "type index");
		goto error;
	    }
	    if (TkGetMenuIndex(interp, menuPtr, objv[2], 0, &index) 
		    != TCL_OK) {
		goto error;
	    }
	    if (index < 0) {
		goto done;
	    }
	    if (menuPtr->entries[index]->type == TEAROFF_ENTRY) {
		Tcl_SetResult(interp, "tearoff", TCL_STATIC);
	    } else {
		Tcl_SetResult(interp,
			menuEntryTypeStrings[menuPtr->entries[index]->type],
			TCL_STATIC);
	    }
	    break;
	}
	case MENU_UNPOST:
	    if (objc != 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "unpost");
		goto error;
	    }
	    Tk_UnmapWindow(menuPtr->tkwin);
	    result = TkPostSubmenu(interp, menuPtr, (TkMenuEntry *) NULL);
	    break;
	case MENU_YPOSITION:
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 1, objv, "yposition index");
		goto error;
	    }
	    result = MenuDoYPosition(interp, menuPtr, objv[2]);
	    break;
    }
    done:
    Tcl_Release((ClientData) menuPtr);
    return result;

    error:
    Tcl_Release((ClientData) menuPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TkInvokeMenu --
 *
 *	Given a menu and an index, takes the appropriate action for the
 *	entry associated with that index.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	Commands may get excecuted; variables may get set; sub-menus may
 *	get posted.
 *
 *----------------------------------------------------------------------
 */

int
TkInvokeMenu(interp, menuPtr, index)
    Tcl_Interp *interp;		/* The interp that the menu lives in. */
    TkMenu *menuPtr;		/* The menu we are invoking. */
    int index;			/* The zero based index of the item we
    				 * are invoking */
{
    int result = TCL_OK;
    TkMenuEntry *mePtr;
    
    if (index < 0) {
    	goto done;
    }
    mePtr = menuPtr->entries[index];
    if (mePtr->state == ENTRY_DISABLED) {
	goto done;
    }
    Tcl_Preserve((ClientData) mePtr);
    if (mePtr->type == TEAROFF_ENTRY) {
	Tcl_DString ds;
	Tcl_DStringInit(&ds);
	Tcl_DStringAppend(&ds, "tkTearOffMenu ", -1);
	Tcl_DStringAppend(&ds, Tk_PathName(menuPtr->tkwin), -1);
	result = Tcl_Eval(interp, Tcl_DStringValue(&ds));
	Tcl_DStringFree(&ds);
    } else if ((mePtr->type == CHECK_BUTTON_ENTRY)
	    && (mePtr->namePtr != NULL)) {
	Tcl_Obj *valuePtr;

	if (mePtr->entryFlags & ENTRY_SELECTED) {
	    valuePtr = mePtr->offValuePtr;
	} else {
	    valuePtr = mePtr->onValuePtr;
	}
	if (valuePtr == NULL) {
	    valuePtr = Tcl_NewObj();
	}
	Tcl_IncrRefCount(valuePtr);
	if (Tcl_ObjSetVar2(interp, mePtr->namePtr, NULL, valuePtr,
		TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) == NULL) {
	    result = TCL_ERROR;
	}
	Tcl_DecrRefCount(valuePtr);
    } else if ((mePtr->type == RADIO_BUTTON_ENTRY)
	    && (mePtr->namePtr != NULL)) {
	Tcl_Obj *valuePtr = mePtr->onValuePtr;

	if (valuePtr == NULL) {
	    valuePtr = Tcl_NewObj();
	}
	Tcl_IncrRefCount(valuePtr);
	if (Tcl_ObjSetVar2(interp, mePtr->namePtr, NULL, valuePtr,
		TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) == NULL) {
	    result = TCL_ERROR;
	}
	Tcl_DecrRefCount(valuePtr);
    }
    if ((result == TCL_OK) && (mePtr->commandPtr != NULL)) {
	Tcl_Obj *commandPtr = mePtr->commandPtr;

	Tcl_IncrRefCount(commandPtr);
	result = Tcl_EvalObjEx(interp, commandPtr, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(commandPtr);
    }
    Tcl_Release((ClientData) mePtr);
    done:
    return result; 
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyMenuInstance --
 *
 *	This procedure is invoked by TkDestroyMenu
 *	to clean up the internal structure of a menu at a safe time
 *	(when no-one is using it anymore). Only takes care of one instance
 *	of the menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the menu is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyMenuInstance(menuPtr)
    TkMenu *menuPtr;	/* Info about menu widget. */
{
    int i;
    TkMenu *menuInstancePtr;
    TkMenuEntry *cascadePtr, *nextCascadePtr;
    Tcl_Obj *newObjv[2];
    TkMenu *parentMasterMenuPtr;
    TkMenuEntry *parentMasterEntryPtr;
    
    /*
     * If the menu has any cascade menu entries pointing to it, the cascade
     * entries need to be told that the menu is going away. We need to clear
     * the menu ptr field in the menu reference at this point in the code
     * so that everything else can forget about this menu properly. We also
     * need to reset -menu field of all entries that are not master menus
     * back to this entry name if this is a master menu pointed to by another
     * master menu. If there is a clone menu that points to this menu,
     * then this menu is itself a clone, so when this menu goes away,
     * the -menu field of the pointing entry must be set back to this
     * menu's master menu name so that later if another menu is created
     * the cascade hierarchy can be maintained.
     */

    TkpDestroyMenu(menuPtr);
    cascadePtr = menuPtr->menuRefPtr->parentEntryPtr;
    menuPtr->menuRefPtr->menuPtr = NULL;
    TkFreeMenuReferences(menuPtr->menuRefPtr);

    for (; cascadePtr != NULL; cascadePtr = nextCascadePtr) {
    	nextCascadePtr = cascadePtr->nextCascadePtr;
    	
    	if (menuPtr->masterMenuPtr != menuPtr) {
	    Tcl_Obj *menuNamePtr = Tcl_NewStringObj("-menu", -1);

	    parentMasterMenuPtr = cascadePtr->menuPtr->masterMenuPtr;
	    parentMasterEntryPtr =
		    parentMasterMenuPtr->entries[cascadePtr->index];
	    newObjv[0] = menuNamePtr;
	    newObjv[1] = parentMasterEntryPtr->namePtr;
	    Tcl_IncrRefCount(newObjv[0]);
	    Tcl_IncrRefCount(newObjv[1]);
    	    ConfigureMenuEntry(cascadePtr, 2, newObjv);
	    Tcl_DecrRefCount(newObjv[0]);
	    Tcl_DecrRefCount(newObjv[1]);
    	} else {
    	    ConfigureMenuEntry(cascadePtr, 0, (Tcl_Obj **) NULL);
    	}
    }
    
    if (menuPtr->masterMenuPtr != menuPtr) {
        for (menuInstancePtr = menuPtr->masterMenuPtr; 
        	menuInstancePtr != NULL;
        	menuInstancePtr = menuInstancePtr->nextInstancePtr) {
            if (menuInstancePtr->nextInstancePtr == menuPtr) {
                menuInstancePtr->nextInstancePtr = 
                	menuInstancePtr->nextInstancePtr->nextInstancePtr;
                break;
            }
        }
   } else if (menuPtr->nextInstancePtr != NULL) {
       panic("Attempting to delete master menu when there are still clones.");
   }

    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeConfigOptions handle all the standard option-related
     * stuff.
     */

    for (i = menuPtr->numEntries; --i >= 0; ) {
	/*
	 * As each menu entry is deleted from the end of the array of
	 * entries, decrement menuPtr->numEntries.  Otherwise, the act of
	 * deleting menu entry i will dereference freed memory attempting
	 * to queue a redraw for menu entries (i+1)...numEntries.
	 */
	 
	DestroyMenuEntry((char *) menuPtr->entries[i]);
	menuPtr->numEntries = i;
    }
    if (menuPtr->entries != NULL) {
	ckfree((char *) menuPtr->entries);
    }
    TkMenuFreeDrawOptions(menuPtr);
    Tk_FreeConfigOptions((char *) menuPtr, 
	    menuPtr->optionTablesPtr->menuOptionTable, menuPtr->tkwin);
}

/*
 *----------------------------------------------------------------------
 *
 * TkDestroyMenu --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a menu at a safe time
 *	(when no-one is using it anymore).  If called on a master instance,
 *	destroys all of the slave instances. If called on a non-master
 *	instance, just destroys that instance.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the menu is freed up.
 *
 *----------------------------------------------------------------------
 */

void
TkDestroyMenu(menuPtr)
    TkMenu *menuPtr;	/* Info about menu widget. */
{
    TkMenu *menuInstancePtr;
    TkMenuTopLevelList *topLevelListPtr, *nextTopLevelPtr;

    if (menuPtr->menuFlags & MENU_DELETION_PENDING) {
    	return;
    }
    
    /*
     * Now destroy all non-tearoff instances of this menu if this is a 
     * parent menu. Is this loop safe enough? Are there going to be
     * destroy bindings on child menus which kill the parent? If not,
     * we have to do a slightly more complex scheme.
     */
    
    if (menuPtr->masterMenuPtr == menuPtr) {
    	menuPtr->menuFlags |= MENU_DELETION_PENDING;
	while (menuPtr->nextInstancePtr != NULL) {
	    menuInstancePtr = menuPtr->nextInstancePtr;
	    menuPtr->nextInstancePtr = menuInstancePtr->nextInstancePtr;
    	    if (menuInstancePtr->tkwin != NULL) {
	     	Tk_DestroyWindow(menuInstancePtr->tkwin);
	    }
	}
    	menuPtr->menuFlags &= ~MENU_DELETION_PENDING;
    }

    /*
     * If any toplevel widgets have this menu as their menubar,
     * the geometry of the window may have to be recalculated.
     */
    
    topLevelListPtr = menuPtr->menuRefPtr->topLevelListPtr;
    while (topLevelListPtr != NULL) {
         nextTopLevelPtr = topLevelListPtr->nextPtr;
         TkpSetWindowMenuBar(topLevelListPtr->tkwin, NULL);
    	 topLevelListPtr = nextTopLevelPtr;
    }   
    DestroyMenuInstance(menuPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * UnhookCascadeEntry --
 *
 *	This entry is removed from the list of entries that point to the
 *	cascade menu. This is done in preparation for changing the menu
 *	that this entry points to.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	The appropriate lists are modified.
 *
 *----------------------------------------------------------------------
 */

static void
UnhookCascadeEntry(mePtr)
    TkMenuEntry *mePtr;			/* The cascade entry we are removing
					 * from the cascade list. */
{
    TkMenuEntry *cascadeEntryPtr;
    TkMenuEntry *prevCascadePtr;
    TkMenuReferences *menuRefPtr;

    menuRefPtr = mePtr->childMenuRefPtr;
    if (menuRefPtr == NULL) {
        return;
    }
    
    cascadeEntryPtr = menuRefPtr->parentEntryPtr;
    if (cascadeEntryPtr == NULL) {
    	return;
    }
    
    /*
     * Singularly linked list deletion. The two special cases are
     * 1. one element; 2. The first element is the one we want.
     */
 
    if (cascadeEntryPtr == mePtr) {
    	if (cascadeEntryPtr->nextCascadePtr == NULL) {

	    /*
	     * This is the last menu entry which points to this
	     * menu, so we need to clear out the list pointer in the
	     * cascade itself.
	     */
	
	    menuRefPtr->parentEntryPtr = NULL;
	    TkFreeMenuReferences(menuRefPtr);
    	} else {
    	    menuRefPtr->parentEntryPtr = cascadeEntryPtr->nextCascadePtr;
    	}
    	mePtr->nextCascadePtr = NULL;
    } else {
	for (prevCascadePtr = cascadeEntryPtr,
		cascadeEntryPtr = cascadeEntryPtr->nextCascadePtr;
		cascadeEntryPtr != NULL;
	        prevCascadePtr = cascadeEntryPtr,
		cascadeEntryPtr = cascadeEntryPtr->nextCascadePtr) {
    	    if (cascadeEntryPtr == mePtr){
    	    	prevCascadePtr->nextCascadePtr =
            	    	cascadeEntryPtr->nextCascadePtr;
    	    	cascadeEntryPtr->nextCascadePtr = NULL;
    	    	break;
    	    }
        }
    }
    mePtr->childMenuRefPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyMenuEntry --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a menu entry at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the menu entry is freed.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyMenuEntry(memPtr)
    char *memPtr;		/* Pointer to entry to be freed. */
{
    register TkMenuEntry *mePtr = (TkMenuEntry *) memPtr;
    TkMenu *menuPtr = mePtr->menuPtr;

    if (menuPtr->postedCascade == mePtr) {
	
    	/*
	 * Ignore errors while unposting the menu, since it's possible
	 * that the menu has already been deleted and the unpost will
	 * generate an error.
	 */

	TkPostSubmenu(menuPtr->interp, menuPtr, (TkMenuEntry *) NULL);
    }

    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeConfigOptions handle all the standard option-related
     * stuff.
     */

    if (mePtr->type == CASCADE_ENTRY) {
        UnhookCascadeEntry(mePtr);
    }
    if (mePtr->image != NULL) {
	Tk_FreeImage(mePtr->image);
    }
    if (mePtr->selectImage != NULL) {
	Tk_FreeImage(mePtr->selectImage);
    }
    if (((mePtr->type == CHECK_BUTTON_ENTRY) 
	    || (mePtr->type == RADIO_BUTTON_ENTRY))
	    && (mePtr->namePtr != NULL)) {
	char *varName = Tcl_GetStringFromObj(mePtr->namePtr, NULL);
	Tcl_UntraceVar(menuPtr->interp, varName,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MenuVarProc, (ClientData) mePtr);
    }
    TkpDestroyMenuEntry(mePtr);
    TkMenuEntryFreeDrawOptions(mePtr);
    Tk_FreeConfigOptions((char *) mePtr, mePtr->optionTable, menuPtr->tkwin);
    ckfree((char *) mePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * MenuWorldChanged --
 *
 *      This procedure is called when the world has changed in some
 *      way (such as the fonts in the system changing) and the widget needs
 *	to recompute all its graphics contexts and determine its new geometry.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Menu will be relayed out and redisplayed.
 *
 *---------------------------------------------------------------------------
 */
 
static void
MenuWorldChanged(instanceData)
    ClientData instanceData;	/* Information about widget. */
{
    TkMenu *menuPtr = (TkMenu *) instanceData;
    int i;
    
    TkMenuConfigureDrawOptions(menuPtr);
    for (i = 0; i < menuPtr->numEntries; i++) {
    	TkMenuConfigureEntryDrawOptions(menuPtr->entries[i],
		menuPtr->entries[i]->index);
	TkpConfigureMenuEntry(menuPtr->entries[i]);	
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureMenu --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a menu widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, font, etc. get set
 *	for menuPtr;  old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureMenu(interp, menuPtr, objc, objv)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register TkMenu *menuPtr;	/* Information about widget;  may or may
				 * not already have values for some fields. */
    int objc;			/* Number of valid entries in argv. */
    Tcl_Obj *CONST objv[];	/* Arguments. */
{
    int i;
    TkMenu *menuListPtr, *cleanupPtr;
    int result;
    
    for (menuListPtr = menuPtr->masterMenuPtr; menuListPtr != NULL;
	    menuListPtr = menuListPtr->nextInstancePtr) {
	menuListPtr->errorStructPtr = (Tk_SavedOptions *)
		ckalloc(sizeof(Tk_SavedOptions));
	result = Tk_SetOptions(interp, (char *) menuListPtr,
		menuListPtr->optionTablesPtr->menuOptionTable, objc, objv, 
		menuListPtr->tkwin, menuListPtr->errorStructPtr, (int *) NULL);
	if (result != TCL_OK) {
	    for (cleanupPtr = menuPtr->masterMenuPtr;
		    cleanupPtr != menuListPtr;
		    cleanupPtr = cleanupPtr->nextInstancePtr) {
		Tk_RestoreSavedOptions(cleanupPtr->errorStructPtr);
		ckfree((char *) cleanupPtr->errorStructPtr);
		cleanupPtr->errorStructPtr = NULL;
	    }
	    return TCL_ERROR;
	}

	/*
	 * When a menu is created, the type is in all of the arguments
	 * to the menu command. Let Tk_ConfigureWidget take care of
	 * parsing them, and then set the type after we can look at
	 * the type string. Once set, a menu's type cannot be changed
	 */
	
	if (menuListPtr->menuType == UNKNOWN_TYPE) {
	    Tcl_GetIndexFromObj(NULL, menuListPtr->menuTypePtr,
		    menuTypeStrings, NULL, 0, &menuListPtr->menuType);

	    /*
	     * Configure the new window to be either a pop-up menu
	     * or a tear-off menu.
	     * We don't do this for menubars since they are not toplevel
	     * windows. Also, since this gets called before CloneMenu has
	     * a chance to set the menuType field, we have to look at the
	     * menuTypeName field to tell that this is a menu bar.
	     */
	    
	    if (menuListPtr->menuType == MASTER_MENU) {
		TkpMakeMenuWindow(menuListPtr->tkwin, 1);
	    } else if (menuListPtr->menuType == TEAROFF_MENU) {
		TkpMakeMenuWindow(menuListPtr->tkwin, 0);
	    }
	}


	/*
	 * Depending on the -tearOff option, make sure that there is or
	 * isn't an initial tear-off entry at the beginning of the menu.
	 */
	
	if (menuListPtr->tearoff) {
	    if ((menuListPtr->numEntries == 0)
		    || (menuListPtr->entries[0]->type != TEAROFF_ENTRY)) {
		if (MenuNewEntry(menuListPtr, 0, TEAROFF_ENTRY) == NULL) {
		    if (menuListPtr->errorStructPtr != NULL) {
			for (cleanupPtr = menuPtr->masterMenuPtr;
				cleanupPtr != menuListPtr;
				cleanupPtr = cleanupPtr->nextInstancePtr) {
			    Tk_RestoreSavedOptions(cleanupPtr->errorStructPtr);
			    ckfree((char *) cleanupPtr->errorStructPtr);
			    cleanupPtr->errorStructPtr = NULL;
			}
			Tk_RestoreSavedOptions(cleanupPtr->errorStructPtr);
			ckfree((char *) cleanupPtr->errorStructPtr);
			cleanupPtr->errorStructPtr = NULL;
		    }
		    return TCL_ERROR;
		}
	    }
	} else if ((menuListPtr->numEntries > 0)
		&& (menuListPtr->entries[0]->type == TEAROFF_ENTRY)) {
	    int i;
	    
	    Tcl_EventuallyFree((ClientData) menuListPtr->entries[0],
	    	    DestroyMenuEntry);

	    for (i = 0; i < menuListPtr->numEntries - 1; i++) {
		menuListPtr->entries[i] = menuListPtr->entries[i + 1];
		menuListPtr->entries[i]->index = i;
	    }
	    menuListPtr->numEntries--;
	    if (menuListPtr->numEntries == 0) {
		ckfree((char *) menuListPtr->entries);
		menuListPtr->entries = NULL;
	    }
	}

	TkMenuConfigureDrawOptions(menuListPtr);
	
	/*
	 * After reconfiguring a menu, we need to reconfigure all of the
	 * entries in the menu, since some of the things in the children
	 * (such as graphics contexts) may have to change to reflect changes
	 * in the parent.
	 */
	
	for (i = 0; i < menuListPtr->numEntries; i++) {
	    TkMenuEntry *mePtr;
	
	    mePtr = menuListPtr->entries[i];
	    ConfigureMenuEntry(mePtr, 0, (Tcl_Obj **) NULL);
	}
	
	TkEventuallyRecomputeMenu(menuListPtr);
    }

    for (cleanupPtr = menuPtr->masterMenuPtr; cleanupPtr != NULL;
	    cleanupPtr = cleanupPtr->nextInstancePtr) {
	Tk_FreeSavedOptions(cleanupPtr->errorStructPtr);
	ckfree((char *) cleanupPtr->errorStructPtr);
	cleanupPtr->errorStructPtr = NULL;
    }

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * PostProcessEntry --
 *
 *	This is called by ConfigureMenuEntry to do all of the configuration
 *	after Tk_SetOptions is called. This is separate
 *	so that error handling is easier.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information such as label and accelerator get
 *	set for mePtr;  old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
PostProcessEntry(mePtr)
    TkMenuEntry *mePtr;			/* The entry we are configuring. */
{
    TkMenu *menuPtr = mePtr->menuPtr;
    int index = mePtr->index;
    char *name;
    Tk_Image image;

    /*
     * The code below handles special configuration stuff not taken
     * care of by Tk_ConfigureWidget, such as special processing for
     * defaults, sizing strings, graphics contexts, etc.
     */

    if (mePtr->labelPtr == NULL) {
	mePtr->labelLength = 0;
    } else {
	Tcl_GetStringFromObj(mePtr->labelPtr, &mePtr->labelLength);
    }
    if (mePtr->accelPtr == NULL) {
	mePtr->accelLength = 0;
    } else {
	Tcl_GetStringFromObj(mePtr->accelPtr, &mePtr->accelLength);
    }

    /*
     * If this is a cascade entry, the platform-specific data of the child
     * menu has to be updated. Also, the links that point to parents and
     * cascades have to be updated.
     */

    if ((mePtr->type == CASCADE_ENTRY) && (mePtr->namePtr != NULL)) {
 	TkMenuEntry *cascadeEntryPtr;
	int alreadyThere;
	TkMenuReferences *menuRefPtr;
	char *oldHashKey = NULL;	/* Initialization only needed to
					 * prevent compiler warning. */

	/*
	 * This is a cascade entry. If the menu that the cascade entry
	 * is pointing to has changed, we need to remove this entry
	 * from the list of entries pointing to the old menu, and add a
	 * cascade reference to the list of entries pointing to the
	 * new menu.
	 *
	 * BUG: We are not recloning for special case #3 yet.
	 */
	
	name = Tcl_GetStringFromObj(mePtr->namePtr, NULL);
	if (mePtr->childMenuRefPtr != NULL) {
	    oldHashKey = Tcl_GetHashKey(TkGetMenuHashTable(menuPtr->interp),
		    mePtr->childMenuRefPtr->hashEntryPtr);
	    if (strcmp(oldHashKey, name) != 0) {
		UnhookCascadeEntry(mePtr);
	    }
	}

	if ((mePtr->childMenuRefPtr == NULL) 
		|| (strcmp(oldHashKey, name) != 0)) {
	    menuRefPtr = TkCreateMenuReferences(menuPtr->interp, name);
	    mePtr->childMenuRefPtr = menuRefPtr;

	    if (menuRefPtr->parentEntryPtr == NULL) {
		menuRefPtr->parentEntryPtr = mePtr;
	    } else {
		alreadyThere = 0;
		for (cascadeEntryPtr = menuRefPtr->parentEntryPtr;
			cascadeEntryPtr != NULL;
			cascadeEntryPtr =
			cascadeEntryPtr->nextCascadePtr) {
		    if (cascadeEntryPtr == mePtr) {
			alreadyThere = 1;
			break;
		    }
		}
    
		/*
		 * Put the item at the front of the list.
		 */
	    
		if (!alreadyThere) {
		    mePtr->nextCascadePtr = menuRefPtr->parentEntryPtr;
		    menuRefPtr->parentEntryPtr = mePtr;
		}
	    }
	}
    }
    
    if (TkMenuConfigureEntryDrawOptions(mePtr, index) != TCL_OK) {
    	return TCL_ERROR;
    }

    if (TkpConfigureMenuEntry(mePtr) != TCL_OK) {
    	return TCL_ERROR;
    }
    
    /*
     * Get the images for the entry, if there are any.  Allocate the
     * new images before freeing the old ones, so that the reference
     * counts don't go to zero and cause image data to be discarded.
     */

    if (mePtr->imagePtr != NULL) {
	char *imageString = Tcl_GetStringFromObj(mePtr->imagePtr, NULL);
	image = Tk_GetImage(menuPtr->interp, menuPtr->tkwin, imageString,
		TkMenuImageProc, (ClientData) mePtr);
	if (image == NULL) {
	    return TCL_ERROR;
	}
    } else {
	image = NULL;
    }
    if (mePtr->image != NULL) {
	Tk_FreeImage(mePtr->image);
    }
    mePtr->image = image;
    if (mePtr->selectImagePtr != NULL) {
	char *selectImageString = Tcl_GetStringFromObj(
		mePtr->selectImagePtr, NULL);
	image = Tk_GetImage(menuPtr->interp, menuPtr->tkwin, selectImageString,
		TkMenuSelectImageProc, (ClientData) mePtr);
	if (image == NULL) {
	    return TCL_ERROR;
	}
    } else {
	image = NULL;
    }
    if (mePtr->selectImage != NULL) {
	Tk_FreeImage(mePtr->selectImage);
    }
    mePtr->selectImage = image;

    if ((mePtr->type == CHECK_BUTTON_ENTRY)
	    || (mePtr->type == RADIO_BUTTON_ENTRY)) {
	Tcl_Obj *valuePtr;
	char *name;

	if (mePtr->namePtr == NULL) {
	    if (mePtr->labelPtr == NULL) {
		mePtr->namePtr = NULL;
	    } else {
		mePtr->namePtr = Tcl_DuplicateObj(mePtr->labelPtr);
		Tcl_IncrRefCount(mePtr->namePtr);
	    }
	}
	if (mePtr->onValuePtr == NULL) {
	    if (mePtr->labelPtr == NULL) {
		mePtr->onValuePtr = NULL;
	    } else {
		mePtr->onValuePtr = Tcl_DuplicateObj(mePtr->labelPtr);
		Tcl_IncrRefCount(mePtr->onValuePtr);
	    }
	}

	/*
	 * Select the entry if the associated variable has the
	 * appropriate value, initialize the variable if it doesn't
	 * exist, then set a trace on the variable to monitor future
	 * changes to its value.
	 */
	
	if (mePtr->namePtr != NULL) {
	    valuePtr = Tcl_ObjGetVar2(menuPtr->interp, mePtr->namePtr, NULL,
		    TCL_GLOBAL_ONLY);
	} else {
	    valuePtr = NULL;
	}
	mePtr->entryFlags &= ~ENTRY_SELECTED;
	if (valuePtr != NULL) {
	    if (mePtr->onValuePtr != NULL) {
		char *value = Tcl_GetStringFromObj(valuePtr, NULL);
		char *onValue = Tcl_GetStringFromObj(mePtr->onValuePtr,
			NULL);


		if (strcmp(value, onValue) == 0) {
		    mePtr->entryFlags |= ENTRY_SELECTED;
		}
	    }
	} else {
	    if (mePtr->namePtr != NULL) {
		Tcl_ObjSetVar2(menuPtr->interp, mePtr->namePtr, NULL,
			(mePtr->type == CHECK_BUTTON_ENTRY)
			? mePtr->offValuePtr
			: Tcl_NewObj(),
			TCL_GLOBAL_ONLY);
	    }
	}
	if (mePtr->namePtr != NULL) {
	    name = Tcl_GetStringFromObj(mePtr->namePtr, NULL);
	    Tcl_TraceVar(menuPtr->interp, name,
		    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		    MenuVarProc, (ClientData) mePtr);
	}
    }
    
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureMenuEntry --
 *
 *	This procedure is called to process an argv/argc list in order
 *	to configure (or reconfigure) one entry in a menu.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information such as label and accelerator get
 *	set for mePtr;  old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureMenuEntry(mePtr, objc, objv)
    register TkMenuEntry *mePtr;	/* Information about menu entry;  may
					 * or may not already have values for
					 * some fields. */
    int objc;				/* Number of valid entries in argv. */
    Tcl_Obj *CONST objv[];		/* Arguments. */
{
    TkMenu *menuPtr = mePtr->menuPtr;
    Tk_SavedOptions errorStruct;
    int result;

    /*
     * If this entry is a check button or radio button, then remove
     * its old trace procedure.
     */

    if ((mePtr->namePtr != NULL)
    	    && ((mePtr->type == CHECK_BUTTON_ENTRY)
	    || (mePtr->type == RADIO_BUTTON_ENTRY))) {
	char *name = Tcl_GetStringFromObj(mePtr->namePtr, NULL);
	Tcl_UntraceVar(menuPtr->interp, name,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MenuVarProc, (ClientData) mePtr);
    }

    result = TCL_OK;
    if (menuPtr->tkwin != NULL) {
	if (Tk_SetOptions(menuPtr->interp, (char *) mePtr,
		mePtr->optionTable, objc, objv, menuPtr->tkwin,
		&errorStruct, (int *) NULL) != TCL_OK) {
	    return TCL_ERROR;
	}
	result = PostProcessEntry(mePtr);
	if (result != TCL_OK) {
	    Tk_RestoreSavedOptions(&errorStruct);
	    PostProcessEntry(mePtr);
	}
	Tk_FreeSavedOptions(&errorStruct);
    }

    TkEventuallyRecomputeMenu(menuPtr);
    
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureMenuCloneEntries --
 *
 *	Calls ConfigureMenuEntry for each menu in the clone chain.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information such as label and accelerator get
 *	set for mePtr;  old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureMenuCloneEntries(interp, menuPtr, index, objc, objv)
    Tcl_Interp *interp;			/* Used for error reporting. */
    TkMenu *menuPtr;			/* Information about whole menu. */
    int index;				/* Index of mePtr within menuPtr's
					 * entries. */
    int objc;				/* Number of valid entries in argv. */
    Tcl_Obj *CONST objv[];		/* Arguments. */
{
    TkMenuEntry *mePtr;
    TkMenu *menuListPtr;
    int cascadeEntryChanged = 0;
    TkMenuReferences *oldCascadeMenuRefPtr, *cascadeMenuRefPtr = NULL; 
    Tcl_Obj *oldCascadePtr = NULL;
    char *newCascadeName;

    /*
     * Cascades are kind of tricky here. This is special case #3 in the comment
     * at the top of this file. Basically, if a menu is the master menu of a
     * clone chain, and has an entry with a cascade menu, the clones of
     * the menu will point to clones of the cascade menu. We have
     * to destroy the clones of the cascades, clone the new cascade
     * menu, and configure the entry to point to the new clone.
     */

    mePtr = menuPtr->masterMenuPtr->entries[index];
    if (mePtr->type == CASCADE_ENTRY) {
	oldCascadePtr = mePtr->namePtr;
	if (oldCascadePtr != NULL) {
	    Tcl_IncrRefCount(oldCascadePtr);
	}
    }

    if (ConfigureMenuEntry(mePtr, objc, objv) != TCL_OK) {
	return TCL_ERROR;
    }

    if (mePtr->type == CASCADE_ENTRY) {
	char *oldCascadeName;

	if (mePtr->namePtr != NULL) {
	    newCascadeName = Tcl_GetStringFromObj(mePtr->namePtr, NULL);
	} else {
	    newCascadeName = NULL;
	}
 
	if ((oldCascadePtr == NULL) && (mePtr->namePtr == NULL)) {
	    cascadeEntryChanged = 0;
	} else if (((oldCascadePtr == NULL) && (mePtr->namePtr != NULL))
		|| ((oldCascadePtr != NULL) 
		&& (mePtr->namePtr == NULL))) {
	    cascadeEntryChanged = 1;
	} else {
	    oldCascadeName = Tcl_GetStringFromObj(oldCascadePtr,
		    NULL);
	    cascadeEntryChanged = (strcmp(oldCascadeName, newCascadeName) 
		    == 0);
	}
	if (oldCascadePtr != NULL) {
	    Tcl_DecrRefCount(oldCascadePtr);
	}
    }

    if (cascadeEntryChanged) {
	if (mePtr->namePtr != NULL) {
	    newCascadeName = Tcl_GetStringFromObj(mePtr->namePtr, NULL);
	    cascadeMenuRefPtr = TkFindMenuReferences(menuPtr->interp,
		    newCascadeName);
	}
    }

    for (menuListPtr = menuPtr->masterMenuPtr->nextInstancePtr; 
    	    menuListPtr != NULL;
	    menuListPtr = menuListPtr->nextInstancePtr) {
  	
    	mePtr = menuListPtr->entries[index];

	if (cascadeEntryChanged && (mePtr->namePtr != NULL)) {
	    oldCascadeMenuRefPtr = TkFindMenuReferencesObj(menuPtr->interp, 
		    mePtr->namePtr);

	    if ((oldCascadeMenuRefPtr != NULL)
		    && (oldCascadeMenuRefPtr->menuPtr != NULL)) {
		RecursivelyDeleteMenu(oldCascadeMenuRefPtr->menuPtr);
	    }
	}

    	if (ConfigureMenuEntry(mePtr, objc, objv) != TCL_OK) {
    	    return TCL_ERROR;
    	}
	
	if (cascadeEntryChanged && (mePtr->namePtr != NULL)) {
	    if (cascadeMenuRefPtr->menuPtr != NULL) {
		Tcl_Obj *newObjv[2];
		Tcl_Obj *newCloneNamePtr;
		Tcl_Obj *pathNamePtr = Tcl_NewStringObj(
			Tk_PathName(menuListPtr->tkwin), -1);
		Tcl_Obj *normalPtr = Tcl_NewStringObj("normal", -1);
		Tcl_Obj *menuObjPtr = Tcl_NewStringObj("-menu", -1);

		Tcl_IncrRefCount(pathNamePtr);
		newCloneNamePtr = TkNewMenuName(menuPtr->interp,
			pathNamePtr, 
			cascadeMenuRefPtr->menuPtr);
		Tcl_IncrRefCount(newCloneNamePtr);
		Tcl_IncrRefCount(normalPtr);
		CloneMenu(cascadeMenuRefPtr->menuPtr, newCloneNamePtr,
			normalPtr);

		newObjv[0] = menuObjPtr;
		newObjv[1] = newCloneNamePtr;
		Tcl_IncrRefCount(menuObjPtr);
		ConfigureMenuEntry(mePtr, 2, newObjv);
		Tcl_DecrRefCount(newCloneNamePtr);
		Tcl_DecrRefCount(pathNamePtr);
		Tcl_DecrRefCount(normalPtr);
		Tcl_DecrRefCount(menuObjPtr);
	    }
	}
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkGetMenuIndex --
 *
 *	Parse a textual index into a menu and return the numerical
 *	index of the indicated entry.
 *
 * Results:
 *	A standard Tcl result.  If all went well, then *indexPtr is
 *	filled in with the entry index corresponding to string
 *	(ranges from -1 to the number of entries in the menu minus
 *	one).  Otherwise an error message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkGetMenuIndex(interp, menuPtr, objPtr, lastOK, indexPtr)
    Tcl_Interp *interp;		/* For error messages. */
    TkMenu *menuPtr;		/* Menu for which the index is being
				 * specified. */
    Tcl_Obj *objPtr;		/* Specification of an entry in menu.  See
				 * manual entry for valid .*/
    int lastOK;			/* Non-zero means its OK to return index
				 * just *after* last entry. */
    int *indexPtr;		/* Where to store converted index. */
{
    int i;
    char *string = Tcl_GetStringFromObj(objPtr, NULL);

    if ((string[0] == 'a') && (strcmp(string, "active") == 0)) {
	*indexPtr = menuPtr->active;
	goto success;
    }

    if (((string[0] == 'l') && (strcmp(string, "last") == 0))
	    || ((string[0] == 'e') && (strcmp(string, "end") == 0))) {
	*indexPtr = menuPtr->numEntries - ((lastOK) ? 0 : 1);
	goto success;
    }

    if ((string[0] == 'n') && (strcmp(string, "none") == 0)) {
	*indexPtr = -1;
	goto success;
    }

    if (string[0] == '@') {
	if (GetIndexFromCoords(interp, menuPtr, string, indexPtr)
		== TCL_OK) {
	    goto success;
	}
    }

    if (isdigit(UCHAR(string[0]))) {
	if (Tcl_GetInt(interp, string,  &i) == TCL_OK) {
	    if (i >= menuPtr->numEntries) {
		if (lastOK) {
		    i = menuPtr->numEntries;
		} else {
		    i = menuPtr->numEntries-1;
		}
	    } else if (i < 0) {
		i = -1;
	    }
	    *indexPtr = i;
	    goto success;
	}
	Tcl_SetResult(interp, (char *) NULL, TCL_STATIC);
    }

    for (i = 0; i < menuPtr->numEntries; i++) {
	Tcl_Obj *labelPtr = menuPtr->entries[i]->labelPtr;
	char *label = (labelPtr == NULL) ? NULL
	        : Tcl_GetStringFromObj(labelPtr, NULL);
	
	if ((label != NULL)
		&& (Tcl_StringMatch(label, string))) {
	    *indexPtr = i;
	    goto success;
	}
    }

    Tcl_AppendResult(interp, "bad menu entry index \"",
	    string, "\"", (char *) NULL);
    return TCL_ERROR;

success:
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * MenuCmdDeletedProc --
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
MenuCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    TkMenu *menuPtr = (TkMenu *) clientData;
    Tk_Window tkwin = menuPtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MenuNewEntry --
 *
 *	This procedure allocates and initializes a new menu entry.
 *
 * Results:
 *	The return value is a pointer to a new menu entry structure,
 *	which has been malloc-ed, initialized, and entered into the
 *	entry array for the  menu.
 *
 * Side effects:
 *	Storage gets allocated.
 *
 *----------------------------------------------------------------------
 */

static TkMenuEntry *
MenuNewEntry(menuPtr, index, type)
    TkMenu *menuPtr;		/* Menu that will hold the new entry. */
    int index;			/* Where in the menu the new entry is to
				 * go. */
    int type;			/* The type of the new entry. */
{
    TkMenuEntry *mePtr;
    TkMenuEntry **newEntries;
    int i;

    /*
     * Create a new array of entries with an empty slot for the
     * new entry.
     */

    newEntries = (TkMenuEntry **) ckalloc((unsigned)
	    ((menuPtr->numEntries+1)*sizeof(TkMenuEntry *)));
    for (i = 0; i < index; i++) {
	newEntries[i] = menuPtr->entries[i];
    }
    for (  ; i < menuPtr->numEntries; i++) {
	newEntries[i+1] = menuPtr->entries[i];
	newEntries[i+1]->index = i + 1;
    }
    if (menuPtr->numEntries != 0) {
	ckfree((char *) menuPtr->entries);
    }
    menuPtr->entries = newEntries;
    menuPtr->numEntries++;
    mePtr = (TkMenuEntry *) ckalloc(sizeof(TkMenuEntry));
    menuPtr->entries[index] = mePtr;
    mePtr->type = type;
    mePtr->optionTable = menuPtr->optionTablesPtr->entryOptionTables[type];
    mePtr->menuPtr = menuPtr;
    mePtr->labelPtr = NULL;
    mePtr->labelLength = 0;
    mePtr->underline = -1;
    mePtr->bitmapPtr = NULL;
    mePtr->imagePtr = NULL;
    mePtr->image = NULL;
    mePtr->selectImagePtr = NULL;
    mePtr->selectImage = NULL;
    mePtr->accelPtr = NULL;
    mePtr->accelLength = 0;
    mePtr->state = ENTRY_DISABLED;
    mePtr->borderPtr = NULL;
    mePtr->fgPtr = NULL;
    mePtr->activeBorderPtr = NULL;
    mePtr->activeFgPtr = NULL;
    mePtr->fontPtr = NULL;
    mePtr->indicatorOn = 0;
    mePtr->indicatorFgPtr = NULL;
    mePtr->columnBreak = 0;
    mePtr->hideMargin = 0;
    mePtr->commandPtr = NULL;
    mePtr->namePtr = NULL;
    mePtr->childMenuRefPtr = NULL;
    mePtr->onValuePtr = NULL;
    mePtr->offValuePtr = NULL;
    mePtr->entryFlags = 0;
    mePtr->index = index;
    mePtr->nextCascadePtr = NULL;
    if (Tk_InitOptions(menuPtr->interp, (char *) mePtr,
	    mePtr->optionTable, menuPtr->tkwin) != TCL_OK) {
	ckfree((char *) mePtr);
	return NULL;
    }
    TkMenuInitializeEntryDrawingFields(mePtr);
    if (TkpMenuNewEntry(mePtr) != TCL_OK) {
	Tk_FreeConfigOptions((char *) mePtr, mePtr->optionTable,
		menuPtr->tkwin);
    	ckfree((char *) mePtr);
    	return NULL;
    }

    return mePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * MenuAddOrInsert --
 *
 *	This procedure does all of the work of the "add" and "insert"
 *	widget commands, allowing the code for these to be shared.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	A new menu entry is created in menuPtr.
 *
 *----------------------------------------------------------------------
 */

static int
MenuAddOrInsert(interp, menuPtr, indexPtr, objc, objv)
    Tcl_Interp *interp;			/* Used for error reporting. */
    TkMenu *menuPtr;			/* Widget in which to create new
					 * entry. */
    Tcl_Obj *indexPtr;			/* Object describing index at which
					 * to insert.  NULL means insert at
					 * end. */
    int objc;				/* Number of elements in objv. */
    Tcl_Obj *CONST objv[];		/* Arguments to command:  first arg
					 * is type of entry, others are
					 * config options. */
{
    int type, index;
    TkMenuEntry *mePtr;
    TkMenu *menuListPtr;

    if (indexPtr != NULL) {
	if (TkGetMenuIndex(interp, menuPtr, indexPtr, 1, &index)
		!= TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	index = menuPtr->numEntries;
    }
    if (index < 0) {
	char *indexString = Tcl_GetStringFromObj(indexPtr, NULL);
	Tcl_AppendResult(interp, "bad index \"", indexString, "\"",
		 (char *) NULL);
	return TCL_ERROR;
    }
    if (menuPtr->tearoff && (index == 0)) {
	index = 1;
    }

    /*
     * Figure out the type of the new entry.
     */

    if (Tcl_GetIndexFromObj(interp, objv[0], menuEntryTypeStrings,
	    "menu entry type", 0, &type) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Now we have to add an entry for every instance related to this menu.
     */

    for (menuListPtr = menuPtr->masterMenuPtr; menuListPtr != NULL; 
    	    menuListPtr = menuListPtr->nextInstancePtr) {
    	
    	mePtr = MenuNewEntry(menuListPtr, index, type);
    	if (mePtr == NULL) {
    	    return TCL_ERROR;
    	}
    	if (ConfigureMenuEntry(mePtr, objc - 1, objv + 1) != TCL_OK) {
	    TkMenu *errorMenuPtr;
	    int i;

	    for (errorMenuPtr = menuPtr->masterMenuPtr;
		    errorMenuPtr != NULL;
		    errorMenuPtr = errorMenuPtr->nextInstancePtr) {
    		Tcl_EventuallyFree((ClientData) errorMenuPtr->entries[index],
    	    		DestroyMenuEntry);
		for (i = index; i < errorMenuPtr->numEntries - 1; i++) {
		    errorMenuPtr->entries[i] = errorMenuPtr->entries[i + 1];
		    errorMenuPtr->entries[i]->index = i;
		}
		errorMenuPtr->numEntries--;
		if (errorMenuPtr->numEntries == 0) {
		    ckfree((char *) errorMenuPtr->entries);
		    errorMenuPtr->entries = NULL;
		}
		if (errorMenuPtr == menuListPtr) {
		    break;
		}
	    }
    	    return TCL_ERROR;
    	}
    	
    	/*
    	 * If a menu has cascades, then every instance of the menu has
    	 * to have its own parallel cascade structure. So adding an
	 * entry to a menu with clones means that the menu that the
	 * entry points to has to be cloned for every clone the
	 * master menu has. This is special case #2 in the comment
	 * at the top of this file.
    	 */
 
    	if ((menuPtr != menuListPtr) && (type == CASCADE_ENTRY)) {    	    
    	    if ((mePtr->namePtr != NULL)
		    && (mePtr->childMenuRefPtr != NULL)
    	    	    && (mePtr->childMenuRefPtr->menuPtr != NULL)) {
    	        TkMenu *cascadeMenuPtr =
			mePtr->childMenuRefPtr->menuPtr->masterMenuPtr;
    	        Tcl_Obj *newCascadePtr;
		Tcl_Obj *menuNamePtr = Tcl_NewStringObj("-menu", -1);
		Tcl_Obj *windowNamePtr = 
			Tcl_NewStringObj(Tk_PathName(menuListPtr->tkwin), -1);
		Tcl_Obj *normalPtr = Tcl_NewStringObj("normal", -1);
  		Tcl_Obj *newObjv[2];
		TkMenuReferences *menuRefPtr;
    	          
		Tcl_IncrRefCount(windowNamePtr);
		newCascadePtr = TkNewMenuName(menuListPtr->interp,
			windowNamePtr, cascadeMenuPtr);
		Tcl_IncrRefCount(newCascadePtr);
		Tcl_IncrRefCount(normalPtr);
		CloneMenu(cascadeMenuPtr, newCascadePtr, normalPtr);
		
		menuRefPtr = TkFindMenuReferencesObj(menuListPtr->interp,
			newCascadePtr);
		if (menuRefPtr == NULL) {
		    panic("CloneMenu failed inside of MenuAddOrInsert.");
		}
		newObjv[0] = menuNamePtr;
		newObjv[1] = newCascadePtr;
		Tcl_IncrRefCount(menuNamePtr);
		Tcl_IncrRefCount(newCascadePtr);
    	        ConfigureMenuEntry(mePtr, 2, newObjv);
    	        Tcl_DecrRefCount(newCascadePtr);
		Tcl_DecrRefCount(menuNamePtr);
		Tcl_DecrRefCount(windowNamePtr);
		Tcl_DecrRefCount(normalPtr);
    	    }
    	}
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * MenuVarProc --
 *
 *	This procedure is invoked when someone changes the
 *	state variable associated with a radiobutton or checkbutton
 *	menu entry.  The entry's selected state is set to match
 *	the value of the variable.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The menu entry may become selected or deselected.
 *
 *--------------------------------------------------------------
 */

static char *
MenuVarProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Information about menu entry. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* First part of variable's name. */
    char *name2;		/* Second part of variable's name. */
    int flags;			/* Describes what just happened. */
{
    TkMenuEntry *mePtr = (TkMenuEntry *) clientData;
    TkMenu *menuPtr;
    char *value;
    char *name = Tcl_GetStringFromObj(mePtr->namePtr, NULL);
    char *onValue;

    menuPtr = mePtr->menuPtr;

    /*
     * If the variable is being unset, then re-establish the
     * trace unless the whole interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
	mePtr->entryFlags &= ~ENTRY_SELECTED;
	if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
	    Tcl_TraceVar(interp, name,
		    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		    MenuVarProc, clientData);
	}
	TkpConfigureMenuEntry(mePtr);
	TkEventuallyRedrawMenu(menuPtr, (TkMenuEntry *) NULL);
	return (char *) NULL;
    }

    /*
     * Use the value of the variable to update the selected status of
     * the menu entry.
     */

    value = Tcl_GetVar(interp, name, TCL_GLOBAL_ONLY);
    if (value == NULL) {
	value = "";
    }
    if (mePtr->onValuePtr != NULL) {
	onValue = Tcl_GetStringFromObj(mePtr->onValuePtr, NULL);
	if (strcmp(value, onValue) == 0) {
	    if (mePtr->entryFlags & ENTRY_SELECTED) {
		return (char *) NULL;
	    }
	    mePtr->entryFlags |= ENTRY_SELECTED;
	} else if (mePtr->entryFlags & ENTRY_SELECTED) {
	    mePtr->entryFlags &= ~ENTRY_SELECTED;
	} else {
	    return (char *) NULL;
	}
    } else {
	return (char *) NULL;
    }
    TkpConfigureMenuEntry(mePtr);
    TkEventuallyRedrawMenu(menuPtr, mePtr);
    return (char *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkActivateMenuEntry --
 *
 *	This procedure is invoked to make a particular menu entry
 *	the active one, deactivating any other entry that might
 *	currently be active.
 *
 * Results:
 *	The return value is a standard Tcl result (errors can occur
 *	while posting and unposting submenus).
 *
 * Side effects:
 *	Menu entries get redisplayed, and the active entry changes.
 *	Submenus may get posted and unposted.
 *
 *----------------------------------------------------------------------
 */

int
TkActivateMenuEntry(menuPtr, index)
    register TkMenu *menuPtr;		/* Menu in which to activate. */
    int index;				/* Index of entry to activate, or
					 * -1 to deactivate all entries. */
{
    register TkMenuEntry *mePtr;
    int result = TCL_OK;

    if (menuPtr->active >= 0) {
	mePtr = menuPtr->entries[menuPtr->active];

	/*
	 * Don't change the state unless it's currently active (state
	 * might already have been changed to disabled).
	 */

	if (mePtr->state == ENTRY_ACTIVE) {
	    mePtr->state = ENTRY_NORMAL;
	}
	TkEventuallyRedrawMenu(menuPtr, menuPtr->entries[menuPtr->active]);
    }
    menuPtr->active = index;
    if (index >= 0) {
	mePtr = menuPtr->entries[index];
	mePtr->state = ENTRY_ACTIVE;
	TkEventuallyRedrawMenu(menuPtr, mePtr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkPostCommand --
 *
 *	Execute the postcommand for the given menu.
 *
 * Results:
 *	The return value is a standard Tcl result (errors can occur
 *	while the postcommands are being processed).
 *
 * Side effects:
 *	Since commands can get executed while this routine is being executed,
 *	the entire world can change.
 *
 *----------------------------------------------------------------------
 */
 
int
TkPostCommand(menuPtr)
    TkMenu *menuPtr;
{
    int result;

    /*
     * If there is a command for the menu, execute it.  This
     * may change the size of the menu, so be sure to recompute
     * the menu's geometry if needed.
     */

    if (menuPtr->postCommandPtr != NULL) {
	Tcl_Obj *postCommandPtr = menuPtr->postCommandPtr;

	Tcl_IncrRefCount(postCommandPtr);
	result = Tcl_EvalObjEx(menuPtr->interp, postCommandPtr,
		TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(postCommandPtr);
	if (result != TCL_OK) {
	    return result;
	}
	TkRecomputeMenu(menuPtr);
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * CloneMenu --
 *
 *	Creates a child copy of the menu. It will be inserted into
 *	the menu's instance chain. All attributes and entry
 *	attributes will be duplicated.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Allocates storage. After the menu is created, any 
 *	configuration done with this menu or any related one
 *	will be reflected in all of them.
 *
 *--------------------------------------------------------------
 */

static int
CloneMenu(menuPtr, newMenuNamePtr, newMenuTypePtr)
    TkMenu *menuPtr;		/* The menu we are going to clone */
    Tcl_Obj *newMenuNamePtr;	/* The name to give the new menu */
    Tcl_Obj *newMenuTypePtr;	/* What kind of menu is this, a normal menu
    				 * a menubar, or a tearoff? */
{
    int returnResult;
    int menuType, i;
    TkMenuReferences *menuRefPtr;
    Tcl_Obj *menuDupCommandArray[4];
    
    if (newMenuTypePtr == NULL) {
    	menuType = MASTER_MENU;
    } else {
	if (Tcl_GetIndexFromObj(menuPtr->interp, newMenuTypePtr, 
		menuTypeStrings, "menu type", 0, &menuType) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    menuDupCommandArray[0] = Tcl_NewStringObj("tkMenuDup", -1);
    menuDupCommandArray[1] = Tcl_NewStringObj(Tk_PathName(menuPtr->tkwin), -1);
    menuDupCommandArray[2] = newMenuNamePtr;
    if (newMenuTypePtr == NULL) {
	menuDupCommandArray[3] = Tcl_NewStringObj("normal", -1);
    } else {
	menuDupCommandArray[3] = newMenuTypePtr;
    }
    for (i = 0; i < 4; i++) {
	Tcl_IncrRefCount(menuDupCommandArray[i]);
    }
    Tcl_Preserve((ClientData) menuPtr);
    returnResult = Tcl_EvalObjv(menuPtr->interp, 4, menuDupCommandArray, 0);
    for (i = 0; i < 4; i++) {
	Tcl_DecrRefCount(menuDupCommandArray[i]);
    }

    /*
     * Make sure the tcl command actually created the clone.
     */
    
    if ((returnResult == TCL_OK) &&
    	    ((menuRefPtr = TkFindMenuReferencesObj(menuPtr->interp, 
	    newMenuNamePtr)) != (TkMenuReferences *) NULL)
	    && (menuPtr->numEntries == menuRefPtr->menuPtr->numEntries)) {
    	TkMenu *newMenuPtr = menuRefPtr->menuPtr;
	Tcl_Obj *newObjv[3];
	char *newArgv[3];
	int i, numElements;

	/*
	 * Now put this newly created menu into the parent menu's instance
	 * chain.
	 */

	if (menuPtr->nextInstancePtr == NULL) {
	    menuPtr->nextInstancePtr = newMenuPtr;
	    newMenuPtr->masterMenuPtr = menuPtr->masterMenuPtr;
	} else {
	    TkMenu *masterMenuPtr;
	    
	    masterMenuPtr = menuPtr->masterMenuPtr;
	    newMenuPtr->nextInstancePtr = masterMenuPtr->nextInstancePtr;
	    masterMenuPtr->nextInstancePtr = newMenuPtr;
	    newMenuPtr->masterMenuPtr = masterMenuPtr;
	}
   	
   	/*
   	 * Add the master menu's window to the bind tags for this window
   	 * after this window's tag. This is so the user can bind to either
   	 * this clone (which may not be easy to do) or the entire menu
   	 * clone structure.
   	 */
   	
   	newArgv[0] = "bindtags";
   	newArgv[1] = Tk_PathName(newMenuPtr->tkwin);
   	if (Tk_BindtagsCmd((ClientData)newMenuPtr->tkwin, 
   		newMenuPtr->interp, 2, newArgv) == TCL_OK) {
   	    char *windowName;
   	    Tcl_Obj *bindingsPtr =
		    Tcl_DuplicateObj(Tcl_GetObjResult(newMenuPtr->interp));
   	    Tcl_Obj *elementPtr;
     
   	    Tcl_ListObjLength(newMenuPtr->interp, bindingsPtr, &numElements);
   	    for (i = 0; i < numElements; i++) {
   	    	Tcl_ListObjIndex(newMenuPtr->interp, bindingsPtr, i,
			&elementPtr);
   	    	windowName = Tcl_GetStringFromObj(elementPtr, NULL);
   	    	if (strcmp(windowName, Tk_PathName(newMenuPtr->tkwin))
   	    		== 0) {
   	    	    Tcl_Obj *newElementPtr = Tcl_NewStringObj(
   	    	    	    Tk_PathName(newMenuPtr->masterMenuPtr->tkwin), -1);
		    Tcl_IncrRefCount(newElementPtr);
   	    	    Tcl_ListObjReplace(menuPtr->interp, bindingsPtr,
   	    	    	    i + 1, 0, 1, &newElementPtr);
   	    	    newArgv[2] = Tcl_GetStringFromObj(bindingsPtr, NULL);
		    Tk_BindtagsCmd((ClientData)newMenuPtr->tkwin,
			    menuPtr->interp, 3, newArgv);
   	    	    break;
   	    	}
   	    }
   	    Tcl_DecrRefCount(bindingsPtr);   	    
   	}
   	Tcl_ResetResult(menuPtr->interp);
      	
   	/*
   	 * Clone all of the cascade menus that this menu points to.
   	 */
   	
   	for (i = 0; i < menuPtr->numEntries; i++) {
   	    TkMenuReferences *cascadeRefPtr;
   	    TkMenu *oldCascadePtr;
   	    
   	    if ((menuPtr->entries[i]->type == CASCADE_ENTRY)
		&& (menuPtr->entries[i]->namePtr != NULL)) {
   	    	cascadeRefPtr =
			TkFindMenuReferencesObj(menuPtr->interp,
			menuPtr->entries[i]->namePtr);
   	    	if ((cascadeRefPtr != NULL) && (cascadeRefPtr->menuPtr)) {
		    Tcl_Obj *windowNamePtr = 
			    Tcl_NewStringObj(Tk_PathName(newMenuPtr->tkwin),
			    -1);
		    Tcl_Obj *newCascadePtr;
		    
   	    	    oldCascadePtr = cascadeRefPtr->menuPtr;

		    Tcl_IncrRefCount(windowNamePtr);
   	    	    newCascadePtr = TkNewMenuName(menuPtr->interp,
   	    	     	    windowNamePtr, oldCascadePtr);
		    Tcl_IncrRefCount(newCascadePtr);
		    CloneMenu(oldCascadePtr, newCascadePtr, NULL);

		    newObjv[0] = Tcl_NewStringObj("-menu", -1);
		    newObjv[1] = newCascadePtr;
		    Tcl_IncrRefCount(newObjv[0]);
		    ConfigureMenuEntry(newMenuPtr->entries[i], 2, newObjv);
		    Tcl_DecrRefCount(newObjv[0]);
		    Tcl_DecrRefCount(newCascadePtr);
		    Tcl_DecrRefCount(windowNamePtr);
   	    	}
   	    }
   	}
   	
    	returnResult = TCL_OK;
    } else {
    	returnResult = TCL_ERROR;
    }
    Tcl_Release((ClientData) menuPtr);
    return returnResult;
}

/*
 *----------------------------------------------------------------------
 *
 * MenuDoYPosition --
 *
 *	Given arguments from an option command line, returns the Y position.
 *
 * Results:
 *	Returns TCL_OK or TCL_Error
 *
 * Side effects:
 *	yPosition is set to the Y-position of the menu entry.
 *
 *----------------------------------------------------------------------
 */
    
static int
MenuDoYPosition(interp, menuPtr, objPtr)
    Tcl_Interp *interp;
    TkMenu *menuPtr;
    Tcl_Obj *objPtr;
{
    int index;
    
    TkRecomputeMenu(menuPtr);
    if (TkGetMenuIndex(interp, menuPtr, objPtr, 0, &index) != TCL_OK) {
    	goto error;
    }
    Tcl_ResetResult(interp);
    if (index < 0) {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
    } else {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(menuPtr->entries[index]->y));
    }

    return TCL_OK;
    
error:
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * GetIndexFromCoords --
 *
 *	Given a string of the form "@int", return the menu item corresponding
 *	to int.
 *
 * Results:
 *	If int is a valid number, *indexPtr will be the number of the menuentry
 *	that is the correct height. If int is invaled, *indexPtr will be
 *	unchanged. Returns appropriate Tcl error number.
 *
 * Side effects:
 *	If int is invalid, interp's result will set to NULL.
 *
 *----------------------------------------------------------------------
 */

static int
GetIndexFromCoords(interp, menuPtr, string, indexPtr)
    Tcl_Interp *interp;		/* interp of menu */
    TkMenu *menuPtr;		/* the menu we are searching */
    char *string;		/* The @string we are parsing */
    int *indexPtr;		/* The index of the item that matches */
{
    int x, y, i;
    char *p, *end;
    
    TkRecomputeMenu(menuPtr);
    p = string + 1;
    y = strtol(p, &end, 0);
    if (end == p) {
	goto error;
    }
    if (*end == ',') {
	x = y;
	p = end + 1;
	y = strtol(p, &end, 0);
	if (end == p) {
	    goto error;
	}
    } else {
	Tk_GetPixelsFromObj(interp, menuPtr->tkwin, 
		menuPtr->borderWidthPtr, &x);
    }
    
    for (i = 0; i < menuPtr->numEntries; i++) {
	if ((x >= menuPtr->entries[i]->x) && (y >= menuPtr->entries[i]->y)
		&& (x < (menuPtr->entries[i]->x + menuPtr->entries[i]->width))
		&& (y < (menuPtr->entries[i]->y
		+ menuPtr->entries[i]->height))) {
	    break;
	}
    }
    if (i >= menuPtr->numEntries) {
	/* i = menuPtr->numEntries - 1; */
	i = -1;
    }
    *indexPtr = i;
    return TCL_OK;

    error:
    Tcl_SetResult(interp, (char *) NULL, TCL_STATIC);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * RecursivelyDeleteMenu --
 *
 *	Deletes a menu and any cascades underneath it. Used for deleting
 *	instances when a menu is no longer being used as a menubar,
 *	for instance.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destroys the menu and all cascade menus underneath it.
 *
 *----------------------------------------------------------------------
 */

static void
RecursivelyDeleteMenu(menuPtr)
    TkMenu *menuPtr;		/* The menubar instance we are deleting */
{
    int i;
    TkMenuEntry *mePtr;
    
    for (i = 0; i < menuPtr->numEntries; i++) {
    	mePtr = menuPtr->entries[i];
    	if ((mePtr->type == CASCADE_ENTRY)
    		&& (mePtr->childMenuRefPtr != NULL)
    		&& (mePtr->childMenuRefPtr->menuPtr != NULL)) {
    	    RecursivelyDeleteMenu(mePtr->childMenuRefPtr->menuPtr);
    	}
    }
    Tk_DestroyWindow(menuPtr->tkwin);
}

/*
 *----------------------------------------------------------------------
 *
 * TkNewMenuName --
 *
 *	Makes a new unique name for a cloned menu. Will be a child
 *	of oldName.
 *
 * Results:
 *	Returns a char * which has been allocated; caller must free.
 *
 * Side effects:
 *	Memory is allocated.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TkNewMenuName(interp, parentPtr, menuPtr)
    Tcl_Interp *interp;		/* The interp the new name has to live in.*/
    Tcl_Obj *parentPtr;		/* The prefix path of the new name. */
    TkMenu *menuPtr;		/* The menu we are cloning. */
{
    Tcl_Obj *resultPtr = NULL;	/* Initialization needed only to prevent
				 * compiler warning. */
    Tcl_Obj *childPtr;
    char *destString;
    int i;
    int doDot;
    Tcl_CmdInfo cmdInfo;
    Tcl_HashTable *nameTablePtr = NULL;
    TkWindow *winPtr = (TkWindow *) menuPtr->tkwin;
    char *parentName = Tcl_GetStringFromObj(parentPtr, NULL);

    if (winPtr->mainPtr != NULL) {
	nameTablePtr = &(winPtr->mainPtr->nameTable);
    }

    doDot = parentName[strlen(parentName) - 1] != '.';

    childPtr = Tcl_NewStringObj(Tk_PathName(menuPtr->tkwin), -1);
    for (destString = Tcl_GetStringFromObj(childPtr, NULL);
    	    *destString != '\0'; destString++) {
    	if (*destString == '.') {
    	    *destString = '#';
    	}
    }
    
    for (i = 0; ; i++) {
    	if (i == 0) {
	    resultPtr = Tcl_DuplicateObj(parentPtr);
    	    if (doDot) {
		Tcl_AppendToObj(resultPtr, ".", -1);
    	    }
	    Tcl_AppendObjToObj(resultPtr, childPtr);
    	} else {
	    Tcl_Obj *intPtr;

	    Tcl_DecrRefCount(resultPtr);
	    resultPtr = Tcl_DuplicateObj(parentPtr);
	    if (doDot) {
		Tcl_AppendToObj(resultPtr, ".", -1);
	    }
	    Tcl_AppendObjToObj(resultPtr, childPtr);
	    intPtr = Tcl_NewIntObj(i);
	    Tcl_AppendObjToObj(resultPtr, intPtr);
	    Tcl_DecrRefCount(intPtr);
    	}
	destString = Tcl_GetStringFromObj(resultPtr, NULL);
    	if ((Tcl_GetCommandInfo(interp, destString, &cmdInfo) == 0)
		&& ((nameTablePtr == NULL)
		|| (Tcl_FindHashEntry(nameTablePtr, destString) == NULL))) {
    	    break;
    	}
    }
    Tcl_DecrRefCount(childPtr);
    return resultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetWindowMenuBar --
 *
 *	Associates a menu with a window. Called by ConfigureFrame in
 *	in response to a "-menu .foo" configuration option for a top
 *	level.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The old menu clones for the menubar are thrown away, and a
 *	handler is set up to allocate the new ones.
 *
 *----------------------------------------------------------------------
 */
void
TkSetWindowMenuBar(interp, tkwin, oldMenuName, menuName)
    Tcl_Interp *interp;		/* The interpreter the toplevel lives in. */
    Tk_Window tkwin;		/* The toplevel window */
    char *oldMenuName;		/* The name of the menubar previously set in
    				 * this toplevel. NULL means no menu was
				 * set previously. */
    char *menuName;		/* The name of the new menubar that the
				 * toplevel needs to be set to. NULL means
				 * that their is no menu now. */
{
    TkMenuTopLevelList *topLevelListPtr, *prevTopLevelPtr;
    TkMenu *menuPtr;
    TkMenuReferences *menuRefPtr;
    
    TkMenuInit();

    /*
     * Destroy the menubar instances of the old menu. Take this window
     * out of the old menu's top level reference list.
     */
    
    if (oldMenuName != NULL) {
        menuRefPtr = TkFindMenuReferences(interp, oldMenuName);
    	if (menuRefPtr != NULL) {

	    /*
	     * Find the menubar instance that is to be removed. Destroy
	     * it and all of the cascades underneath it.
	     */

	    if (menuRefPtr->menuPtr != NULL) {    	    
    	    	TkMenu *instancePtr;

    	    	menuPtr = menuRefPtr->menuPtr;
    	        	    
    	    	for (instancePtr = menuPtr->masterMenuPtr;
		        instancePtr != NULL; 
    	    	    	instancePtr = instancePtr->nextInstancePtr) {
    	    	    if (instancePtr->menuType == MENUBAR 
    	    		    && instancePtr->parentTopLevelPtr == tkwin) {
    	    	    	RecursivelyDeleteMenu(instancePtr);
    	    	    	break;
    	    	    }
    	    	}
    	    }
 
 	    /*
 	     * Now we need to remove this toplevel from the list of toplevels
	     * that reference this menu.
 	     */
 
            for (topLevelListPtr = menuRefPtr->topLevelListPtr,
		    prevTopLevelPtr = NULL;
		    (topLevelListPtr != NULL) 
            	    && (topLevelListPtr->tkwin != tkwin);
		    prevTopLevelPtr = topLevelListPtr,
		    topLevelListPtr = topLevelListPtr->nextPtr) {

		/*
		 * Empty loop body.
		 */
		
            }

	    /*
	     * Now we have found the toplevel reference that matches the
	     * tkwin; remove this reference from the list.
	     */

	    if (topLevelListPtr != NULL) {
            	if (prevTopLevelPtr == NULL) {
		    menuRefPtr->topLevelListPtr =
			    menuRefPtr->topLevelListPtr->nextPtr;
		} else {
            	    prevTopLevelPtr->nextPtr = topLevelListPtr->nextPtr;
            	}
            	ckfree((char *) topLevelListPtr);
            	TkFreeMenuReferences(menuRefPtr);
            }
        }
    }

    /*
     * Now, add the clone references for the new menu.
     */
    
    if (menuName != NULL && menuName[0] != 0) {
    	TkMenu *menuBarPtr = NULL;

	menuRefPtr = TkCreateMenuReferences(interp, menuName);    	
    	
    	menuPtr = menuRefPtr->menuPtr;
    	if (menuPtr != NULL) {
   	    Tcl_Obj *cloneMenuPtr;
   	    TkMenuReferences *cloneMenuRefPtr;
	    Tcl_Obj *newObjv[4];
	    Tcl_Obj *windowNamePtr = Tcl_NewStringObj(Tk_PathName(tkwin), 
		    -1);
	    Tcl_Obj *menubarPtr = Tcl_NewStringObj("menubar", -1);
    	
            /*
             * Clone the menu and all of the cascades underneath it.
             */

	    Tcl_IncrRefCount(windowNamePtr);
    	    cloneMenuPtr = TkNewMenuName(interp, windowNamePtr,
    	    	    menuPtr);
	    Tcl_IncrRefCount(cloneMenuPtr);
	    Tcl_IncrRefCount(menubarPtr);
            CloneMenu(menuPtr, cloneMenuPtr, menubarPtr);
	    
            cloneMenuRefPtr = TkFindMenuReferencesObj(interp, cloneMenuPtr);
            if ((cloneMenuRefPtr != NULL)
		    && (cloneMenuRefPtr->menuPtr != NULL)) {
		Tcl_Obj *cursorPtr = Tcl_NewStringObj("-cursor", -1);
		Tcl_Obj *nullPtr = Tcl_NewObj();
            	cloneMenuRefPtr->menuPtr->parentTopLevelPtr = tkwin;
            	menuBarPtr = cloneMenuRefPtr->menuPtr;
		newObjv[0] = cursorPtr;
		newObjv[1] = nullPtr;
		Tcl_IncrRefCount(cursorPtr);
		Tcl_IncrRefCount(nullPtr);
		ConfigureMenu(menuPtr->interp, cloneMenuRefPtr->menuPtr,
			2, newObjv);
		Tcl_DecrRefCount(cursorPtr);
		Tcl_DecrRefCount(nullPtr);
            }

	    TkpSetWindowMenuBar(tkwin, menuBarPtr);
	    Tcl_DecrRefCount(cloneMenuPtr);
	    Tcl_DecrRefCount(menubarPtr);
	    Tcl_DecrRefCount(windowNamePtr);
        } else {
    	    TkpSetWindowMenuBar(tkwin, NULL);
	}

        
        /*
         * Add this window to the menu's list of windows that refer
         * to this menu.
         */

        topLevelListPtr = (TkMenuTopLevelList *)
		ckalloc(sizeof(TkMenuTopLevelList));
        topLevelListPtr->tkwin = tkwin;
        topLevelListPtr->nextPtr = menuRefPtr->topLevelListPtr;
        menuRefPtr->topLevelListPtr = topLevelListPtr;
    } else {
	TkpSetWindowMenuBar(tkwin, NULL);
    }
    TkpSetMainMenubar(interp, tkwin, menuName);
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyMenuHashTable --
 *
 *	Called when an interp is deleted and a menu hash table has
 *	been set in it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The hash table is destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyMenuHashTable(clientData, interp)
    ClientData clientData;	/* The menu hash table we are destroying */
    Tcl_Interp *interp;		/* The interpreter we are destroying */
{
    Tcl_DeleteHashTable((Tcl_HashTable *) clientData);
    ckfree((char *) clientData);
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetMenuHashTable --
 *
 *	For a given interp, give back the menu hash table that goes with
 *	it. If the hash table does not exist, it is created.
 *
 * Results:
 *	Returns a hash table pointer.
 *
 * Side effects:
 *	A new hash table is created if there were no table in the interp
 *	originally.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashTable *
TkGetMenuHashTable(interp)
    Tcl_Interp *interp;		/* The interp we need the hash table in.*/
{
    Tcl_HashTable *menuTablePtr;

    menuTablePtr = (Tcl_HashTable *) Tcl_GetAssocData(interp, MENU_HASH_KEY,
	    NULL);
    if (menuTablePtr == NULL) {
	menuTablePtr = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(menuTablePtr, TCL_STRING_KEYS);
	Tcl_SetAssocData(interp, MENU_HASH_KEY, DestroyMenuHashTable,
		(ClientData) menuTablePtr);
    }
    return menuTablePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkCreateMenuReferences --
 *
 *	Given a pathname, gives back a pointer to a TkMenuReferences structure.
 *	If a reference is not already in the hash table, one is created.
 *
 * Results:
 *	Returns a pointer to a menu reference structure. Should not
 *	be freed by calller; when a field of the reference is cleared,
 *	TkFreeMenuReferences should be called.
 *
 * Side effects:
 *	A new hash table entry is created if there were no references
 *	to the menu originally.
 *
 *----------------------------------------------------------------------
 */

TkMenuReferences *
TkCreateMenuReferences(interp, pathName)
    Tcl_Interp *interp;
    char *pathName;		/* The path of the menu widget */
{
    Tcl_HashEntry *hashEntryPtr;
    TkMenuReferences *menuRefPtr;
    int newEntry;
    Tcl_HashTable *menuTablePtr = TkGetMenuHashTable(interp);

    hashEntryPtr = Tcl_CreateHashEntry(menuTablePtr, pathName, &newEntry);
    if (newEntry) {
    	menuRefPtr = (TkMenuReferences *) ckalloc(sizeof(TkMenuReferences));
    	menuRefPtr->menuPtr = NULL;
    	menuRefPtr->topLevelListPtr = NULL;
    	menuRefPtr->parentEntryPtr = NULL;
    	menuRefPtr->hashEntryPtr = hashEntryPtr;
    	Tcl_SetHashValue(hashEntryPtr, (char *) menuRefPtr);
    } else {
    	menuRefPtr = (TkMenuReferences *) Tcl_GetHashValue(hashEntryPtr);
    }
    return menuRefPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFindMenuReferences --
 *
 *	Given a pathname, gives back a pointer to the TkMenuReferences
 *	structure.
 *
 * Results:
 *	Returns a pointer to a menu reference structure. Should not
 *	be freed by calller; when a field of the reference is cleared,
 *	TkFreeMenuReferences should be called. Returns NULL if no reference
 *	with this pathname exists.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkMenuReferences *
TkFindMenuReferences(interp, pathName)
    Tcl_Interp *interp;		/* The interp the menu is living in. */
    char *pathName;		/* The path of the menu widget */
{
    Tcl_HashEntry *hashEntryPtr;
    TkMenuReferences *menuRefPtr = NULL;
    Tcl_HashTable *menuTablePtr;

    menuTablePtr = TkGetMenuHashTable(interp);
    hashEntryPtr = Tcl_FindHashEntry(menuTablePtr, pathName);
    if (hashEntryPtr != NULL) {
    	menuRefPtr = (TkMenuReferences *) Tcl_GetHashValue(hashEntryPtr);
    }
    return menuRefPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFindMenuReferencesObj --
 *
 *	Given a pathname, gives back a pointer to the TkMenuReferences
 *	structure.
 *
 * Results:
 *	Returns a pointer to a menu reference structure. Should not
 *	be freed by calller; when a field of the reference is cleared,
 *	TkFreeMenuReferences should be called. Returns NULL if no reference
 *	with this pathname exists.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkMenuReferences *
TkFindMenuReferencesObj(interp, objPtr)
    Tcl_Interp *interp;		/* The interp the menu is living in. */
    Tcl_Obj *objPtr;		/* The path of the menu widget */
{
    char *pathName = Tcl_GetStringFromObj(objPtr, NULL);
    return TkFindMenuReferences(interp, pathName);
}

/*
 *----------------------------------------------------------------------
 *
 * TkFreeMenuReferences --
 *
 *	This is called after one of the fields in a menu reference
 *	is cleared. It cleans up the ref if it is now empty.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If this is the last field to be cleared, the menu ref is
 *	taken out of the hash table.
 *
 *----------------------------------------------------------------------
 */

void
TkFreeMenuReferences(menuRefPtr)
    TkMenuReferences *menuRefPtr;		/* The menu reference to
						 * free */
{
    if ((menuRefPtr->menuPtr == NULL) 
    	    && (menuRefPtr->parentEntryPtr == NULL)
    	    && (menuRefPtr->topLevelListPtr == NULL)) {
    	Tcl_DeleteHashEntry(menuRefPtr->hashEntryPtr);
    	ckfree((char *) menuRefPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteMenuCloneEntries --
 *
 *	For every clone in this clone chain, delete the menu entries
 *	given by the parameters.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The appropriate entries are deleted from all clones of this menu.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteMenuCloneEntries(menuPtr, first, last)
    TkMenu *menuPtr;		    /* the menu the command was issued with */
    int	first;			    /* the zero-based first entry in the set
				     * of entries to delete. */
    int last;			    /* the zero-based last entry */
{

    TkMenu *menuListPtr;
    int numDeleted, i;

    numDeleted = last + 1 - first;
    for (menuListPtr = menuPtr->masterMenuPtr; menuListPtr != NULL;
	    menuListPtr = menuListPtr->nextInstancePtr) {
	for (i = last; i >= first; i--) {
	    Tcl_EventuallyFree((ClientData) menuListPtr->entries[i],
		    DestroyMenuEntry);
	}
	for (i = last + 1; i < menuListPtr->numEntries; i++) {
	    menuListPtr->entries[i - numDeleted] = menuListPtr->entries[i];
	    menuListPtr->entries[i - numDeleted]->index = i;
	}
	menuListPtr->numEntries -= numDeleted;
	if (menuListPtr->numEntries == 0) {
	    ckfree((char *) menuListPtr->entries);
	    menuListPtr->entries = NULL;
	}
	if ((menuListPtr->active >= first) 
		&& (menuListPtr->active <= last)) {
	    menuListPtr->active = -1;
	} else if (menuListPtr->active > last) {
	    menuListPtr->active -= numDeleted;
	}
	TkEventuallyRecomputeMenu(menuListPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMenuInit --
 *
 *	Sets up the hash tables and the variables used by the menu package.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	lastMenuID gets initialized, and the parent hash and the command hash
 *	are allocated.
 *
 *----------------------------------------------------------------------
 */

void
TkMenuInit()
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    
    if (!menusInitialized) {
	Tcl_MutexLock(&menuMutex);
	if (!menusInitialized) {
	    TkpMenuInit();
	    menusInitialized = 1;
	}
	Tcl_MutexUnlock(&menuMutex);
    }
    if (!tsdPtr->menusInitialized) {
	TkpMenuThreadInit();
	tsdPtr->menusInitialized = 1;
    }
}
