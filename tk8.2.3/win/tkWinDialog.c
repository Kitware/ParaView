
/*
 * tkWinDialog.c --
 *
 *	Contains the Windows implementation of the common dialog boxes.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 *
 */

#include "tkWinInt.h"
#include "tkFileFilter.h"

#include <commdlg.h>    /* includes common dialog functionality */
#include <dlgs.h>       /* includes common dialog template defines */
#include <cderr.h>      /* includes the common dialog error codes */

typedef struct ThreadSpecificData { 
    int debugFlag;            /* Flags whether we should output debugging 
			       * information while displaying a builtin 
			       * dialog. */
    Tcl_Interp *debugInterp;  /* Interpreter to used for debugging. */
    UINT WM_LBSELCHANGED;     /* Holds a registered windows event used for
			       * communicating between the Directory
			       * Chooser dialog and its hook proc. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * The following structures are used by Tk_MessageBoxCmd() to parse 
 * arguments and return results.
 */

static const TkStateMap iconMap[] = {
    {MB_ICONERROR,		"error"},
    {MB_ICONINFORMATION,	"info"},
    {MB_ICONQUESTION,		"question"},
    {MB_ICONWARNING,		"warning"},
    {-1,			NULL}
};
	  
static const TkStateMap typeMap[] = {
    {MB_ABORTRETRYIGNORE,	"abortretryignore"},
    {MB_OK, 			"ok"},
    {MB_OKCANCEL,		"okcancel"},
    {MB_RETRYCANCEL,		"retrycancel"},
    {MB_YESNO,			"yesno"},
    {MB_YESNOCANCEL,		"yesnocancel"},
    {-1,			NULL}
};

static const TkStateMap buttonMap[] = {
    {IDABORT,			"abort"},
    {IDRETRY,			"retry"},
    {IDIGNORE,			"ignore"},
    {IDOK,			"ok"},
    {IDCANCEL,			"cancel"},
    {IDNO,			"no"},
    {IDYES,			"yes"},
    {-1,			NULL}
};

static const int buttonFlagMap[] = {
    MB_DEFBUTTON1, MB_DEFBUTTON2, MB_DEFBUTTON3, MB_DEFBUTTON4
};

static const struct {int type; int btnIds[3];} allowedTypes[] = {
    {MB_ABORTRETRYIGNORE,	{IDABORT, IDRETRY,  IDIGNORE}},
    {MB_OK, 			{IDOK,    -1,       -1      }},
    {MB_OKCANCEL,		{IDOK,    IDCANCEL, -1      }},
    {MB_RETRYCANCEL,		{IDRETRY, IDCANCEL, -1      }},
    {MB_YESNO,			{IDYES,   IDNO,     -1      }},
    {MB_YESNOCANCEL,		{IDYES,   IDNO,     IDCANCEL}}
};

#define NUM_TYPES (sizeof(allowedTypes) / sizeof(allowedTypes[0]))

/*
 * The following structure is used to pass information between the directory
 * chooser procedure, Tk_ChooseDirectoryObjCmd(), and its dialog hook proc.
 */

typedef struct ChooseDir {
    Tcl_Interp *interp;		/* Interp, used only if debug is turned on, 
				 * for setting the "tk_dialog" variable. */
    int lastCtrl;		/* Used by hook proc to keep track of last
				 * control that had input focus, so when OK
				 * is pressed we know whether to browse a
				 * new directory or return. */
    int lastIdx;		/* Last item that was selected in directory 
				 * browser listbox. */
    TCHAR path[MAX_PATH];	/* On return from choose directory dialog, 
				 * holds the selected path.  Cannot return 
				 * selected path in ofnPtr->lpstrFile because
				 * the default dialog proc stores a '\0' in 
				 * it, since, of course, no _file_ was 
				 * selected. */
} ChooseDir;

/*
 * Definitions of procedures used only in this file.
 */

static UINT APIENTRY	ChooseDirectoryHookProc(HWND hdlg, UINT uMsg, 
			    WPARAM wParam, LPARAM lParam);
static UINT CALLBACK	ColorDlgHookProc(HWND hDlg, UINT uMsg, WPARAM wParam,
			    LPARAM lParam);
static int 		GetFileName(ClientData clientData, 
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[], int isOpen);
static int 		MakeFilter(Tcl_Interp *interp, char *string, 
			    Tcl_DString *dsPtr);
static UINT APIENTRY	OFNHookProc(HWND hdlg, UINT uMsg, WPARAM wParam, 
			    LPARAM lParam);
static void		SetTkDialog(ClientData clientData);
static int		TrySetDirectory(HWND hwnd, const TCHAR *dir);

/*
 *-------------------------------------------------------------------------
 *
 * TkWinDialogDebug --
 *
 *	Function to turn on/off debugging support for common dialogs under
 *	windows.  The variable "tk_debug" is set to the identifier of the
 *	dialog window when the modal dialog window pops up and it is safe to 
 *	send messages to the dialog.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This variable only makes sense if just one dialog is up at a time.
 *
 *-------------------------------------------------------------------------
 */

void	    	
TkWinDialogDebug(
    int debug)
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    tsdPtr->debugFlag = debug;
}

/*
 *-------------------------------------------------------------------------
 *
 * Tk_ChooseColorObjCmd --
 *
 *	This procedure implements the color dialog box for the Windows
 *	platform. See the user documentation for details on what it
 *	does.
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	A dialog window is created the first time this procedure is called.
 *	This window is not destroyed and will be reused the next time the
 *	application invokes the "tk_chooseColor" command.
 *
 *-------------------------------------------------------------------------
 */

int
Tk_ChooseColorObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Tk_Window tkwin, parent;
    int i, oldMode, winCode;
    CHOOSECOLOR chooseColor;
    static inited = 0;
    static COLORREF dwCustColors[16];
    static long oldColor;		/* the color selected last time */
    static char *optionStrings[] = {
	"-initialcolor",    "-parent",	    "-title",	    NULL
    };
    enum options {
	COLOR_INITIAL,	    COLOR_PARENT,   COLOR_TITLE
    };

    if (inited == 0) {
	/*
	 * dwCustColors stores the custom color which the user can
	 * modify. We store these colors in a static array so that the next
	 * time the color dialog pops up, the same set of custom colors
	 * remain in the dialog.
	 */
	for (i = 0; i < 16; i++) {
	    dwCustColors[i] = RGB(255-i * 10, i, i * 10);
	}
	oldColor = RGB(0xa0, 0xa0, 0xa0);
	inited = 1;
    }

    tkwin = (Tk_Window) clientData;

    parent			= tkwin;
    chooseColor.lStructSize	= sizeof(CHOOSECOLOR) ;
    chooseColor.hwndOwner	= 0;			
    chooseColor.hInstance	= 0;
    chooseColor.rgbResult	= oldColor;
    chooseColor.lpCustColors	= dwCustColors ;
    chooseColor.Flags		= CC_RGBINIT | CC_FULLOPEN | CC_ENABLEHOOK;
    chooseColor.lCustData	= (LPARAM) NULL;
    chooseColor.lpfnHook	= ColorDlgHookProc;
    chooseColor.lpTemplateName	= (LPTSTR) interp;

    for (i = 1; i < objc; i += 2) {
	int index;
	char *string;
	Tcl_Obj *optionPtr, *valuePtr;

	optionPtr = objv[i];
	valuePtr = objv[i + 1];

	if (Tcl_GetIndexFromObj(interp, optionPtr, optionStrings, "option",
		TCL_EXACT, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (i + 1 == objc) {
	    string = Tcl_GetStringFromObj(optionPtr, NULL);
	    Tcl_AppendResult(interp, "value for \"", string, "\" missing", 
		    (char *) NULL);
	    return TCL_ERROR;
	}

	string = Tcl_GetStringFromObj(valuePtr, NULL);
	switch ((enum options) index) {
	    case COLOR_INITIAL: {
		XColor *colorPtr;

		colorPtr = Tk_GetColor(interp, tkwin, string);
		if (colorPtr == NULL) {
		    return TCL_ERROR;
		}
		chooseColor.rgbResult = RGB(colorPtr->red / 0x100, 
			colorPtr->green / 0x100, colorPtr->blue / 0x100);
		break;
	    }
	    case COLOR_PARENT: {
		parent = Tk_NameToWindow(interp, string, tkwin);
		if (parent == NULL) {
		    return TCL_ERROR;
		}
		break;
	    }
	    case COLOR_TITLE: {
		chooseColor.lCustData = (LPARAM) string;
		break;
	    }
	}
    }

    Tk_MakeWindowExist(parent);
    chooseColor.hwndOwner = Tk_GetHWND(Tk_WindowId(parent));

    oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
    winCode = ChooseColor(&chooseColor);
    (void) Tcl_SetServiceMode(oldMode);

    /*
     * Clear the interp result since anything may have happened during the
     * modal loop.
     */

    Tcl_ResetResult(interp);

    /*
     * 3. Process the result of the dialog
     */

    if (winCode) {
	/*
	 * User has selected a color
	 */
	char result[100];

	sprintf(result, "#%02x%02x%02x",
	GetRValue(chooseColor.rgbResult), 
	        GetGValue(chooseColor.rgbResult), 
		GetBValue(chooseColor.rgbResult));
        Tcl_AppendResult(interp, result, NULL);
	oldColor = chooseColor.rgbResult;
    }
    return TCL_OK;
}

/*
 *-------------------------------------------------------------------------
 *
 * ColorDlgHookProc --
 *
 *	Provides special handling of messages for the Color common dialog
 *	box.  Used to set the title when the dialog first appears.
 *
 * Results:
 *	The return value is 0 if the default dialog box procedure should
 *	handle the message, non-zero otherwise. 
 *
 * Side effects:
 *	Changes the title of the dialog window.
 *
 *----------------------------------------------------------------------
 */

static UINT CALLBACK 
ColorDlgHookProc(hDlg, uMsg, wParam, lParam)
    HWND hDlg;			/* Handle to the color dialog. */
    UINT uMsg;			/* Type of message. */
    WPARAM wParam;		/* First message parameter. */
    LPARAM lParam;		/* Second message parameter. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    switch (uMsg) {
	case WM_INITDIALOG: {
	    const char *title;
	    CHOOSECOLOR *ccPtr;
	    Tcl_DString ds;

	    /* 
	     * Set the title string of the dialog.
	     */

	    ccPtr = (CHOOSECOLOR *) lParam;
	    title = (const char *) ccPtr->lCustData;
	    if ((title != NULL) && (title[0] != '\0')) {
		Tcl_UtfToExternalDString(NULL, title, -1, &ds);
		SetWindowText(hDlg, (TCHAR *) Tcl_DStringValue(&ds));
		Tcl_DStringFree(&ds);
	    }
	    if (tsdPtr->debugFlag) {
		tsdPtr->debugInterp = (Tcl_Interp *) ccPtr->lpTemplateName;
		Tcl_DoWhenIdle(SetTkDialog, (ClientData) hDlg);
	    }
	    return TRUE;
	}
    }
    return FALSE;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetOpenFileCmd --
 *
 *	This procedure implements the "open file" dialog box for the
 *	Windows platform. See the user documentation for details on what
 *	it does.
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	A dialog window is created the first this procedure is called.
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetOpenFileObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    return GetFileName(clientData, interp, objc, objv, 1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetSaveFileCmd --
 *
 *	Same as Tk_GetOpenFileCmd but opens a "save file" dialog box
 *	instead
 *
 * Results:
 *	Same as Tk_GetOpenFileCmd.
 *
 * Side effects:
 *	Same as Tk_GetOpenFileCmd.
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetSaveFileObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    return GetFileName(clientData, interp, objc, objv, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * GetFileName --
 *
 *	Calls GetOpenFileName() or GetSaveFileName().
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	See user documentation.
 *
 *----------------------------------------------------------------------
 */

static int 
GetFileName(clientData, interp, objc, objv, open)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
    int open;			/* 1 to call GetOpenFileName(), 0 to 
				 * call GetSaveFileName(). */
{
    OPENFILENAME ofn;
    TCHAR file[MAX_PATH], savePath[MAX_PATH];
    int result, winCode, oldMode, i;
    char *extension, *filter, *title;
    Tk_Window tkwin;
    Tcl_DString utfFilterString, utfDirString;
    Tcl_DString extString, filterString, dirString, titleString;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    static char *optionStrings[] = {
	"-defaultextension", "-filetypes", "-initialdir", "-initialfile",
	"-parent",	"-title",	NULL
    };
    enum options {
	FILE_DEFAULT,	FILE_TYPES,	FILE_INITDIR,	FILE_INITFILE,
	FILE_PARENT,	FILE_TITLE
    };

    result = TCL_ERROR;
    file[0] = '\0';

    /*
     * Parse the arguments.
     */

    extension = NULL;
    filter = NULL;
    Tcl_DStringInit(&utfFilterString);
    Tcl_DStringInit(&utfDirString);
    tkwin = (Tk_Window) clientData;
    title = NULL;

    for (i = 1; i < objc; i += 2) {
	int index;
	char *string;
	Tcl_Obj *optionPtr, *valuePtr;

	optionPtr = objv[i];
	valuePtr = objv[i + 1];

	if (Tcl_GetIndexFromObj(interp, optionPtr, optionStrings, "option", 
		0, &index) != TCL_OK) {
	    goto end;
	}
	if (i + 1 == objc) {
	    string = Tcl_GetStringFromObj(optionPtr, NULL);
	    Tcl_AppendResult(interp, "value for \"", string, "\" missing", 
		    (char *) NULL);
	    goto end;
	}

	string = Tcl_GetStringFromObj(valuePtr, NULL);
	switch ((enum options) index) {
	    case FILE_DEFAULT: {
		if (string[0] == '.') {
		    string++;
		}
		extension = string;
		break;
	    }
	    case FILE_TYPES: {
		Tcl_DStringFree(&utfFilterString);
		if (MakeFilter(interp, string, &utfFilterString) != TCL_OK) {
		    goto end;
		}
		filter = Tcl_DStringValue(&utfFilterString);
		break;
	    }
	    case FILE_INITDIR: {
		Tcl_DStringFree(&utfDirString);
		if (Tcl_TranslateFileName(interp, string, 
			&utfDirString) == NULL) {
		    goto end;
		}
		break;
	    }
	    case FILE_INITFILE: {
		Tcl_DString ds;

		if (Tcl_TranslateFileName(interp, string, &ds) == NULL) {
		    goto end;
		}
		Tcl_UtfToExternal(NULL, NULL, Tcl_DStringValue(&ds), 
			Tcl_DStringLength(&ds), 0, NULL, (char *) file, 
			sizeof(file), NULL, NULL, NULL);
		break;
	    }
	    case FILE_PARENT: {
		tkwin = Tk_NameToWindow(interp, string, tkwin);
		if (tkwin == NULL) {
		    goto end;
		}
		break;
	    }
	    case FILE_TITLE: {
		title = string;
		break;
	    }
	}
    }

    if (filter == NULL) {
	if (MakeFilter(interp, "", &utfFilterString) != TCL_OK) {
	    goto end;
	}
    }

    Tk_MakeWindowExist(tkwin);

    ofn.lStructSize		= sizeof(ofn);
    ofn.hwndOwner		= Tk_GetHWND(Tk_WindowId(tkwin));
    ofn.hInstance		= (HINSTANCE) GetWindowLong(ofn.hwndOwner, 
					GWL_HINSTANCE);
    ofn.lpstrFilter		= NULL;
    ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 0;
    ofn.lpstrFile		= (LPTSTR) file;
    ofn.nMaxFile		= MAX_PATH;
    ofn.lpstrFileTitle		= NULL;
    ofn.nMaxFileTitle		= 0;
    ofn.lpstrInitialDir		= NULL;
    ofn.lpstrTitle		= NULL;
    ofn.Flags			= OFN_HIDEREADONLY | OFN_PATHMUSTEXIST 
				  | OFN_NOCHANGEDIR;
    ofn.nFileOffset		= 0;
    ofn.nFileExtension		= 0;
    ofn.lpstrDefExt		= NULL;
    ofn.lpfnHook		= OFNHookProc;
    ofn.lCustData		= (LPARAM) interp;
    ofn.lpTemplateName		= NULL;

    if (LOBYTE(LOWORD(GetVersion())) >= 4) {
	/*
	 * Use the "explorer" style file selection box on platforms that
	 * support it (Win95 and NT4.0 both have a major version number
	 * of 4).
	 */

	ofn.Flags |= OFN_EXPLORER;
    }

    if (open != 0) {
	ofn.Flags |= OFN_FILEMUSTEXIST;
    } else {
	ofn.Flags |= OFN_OVERWRITEPROMPT;
    }

    if (tsdPtr->debugFlag != 0) {
	ofn.Flags |= OFN_ENABLEHOOK;
    }

    if (extension != NULL) {
	Tcl_UtfToExternalDString(NULL, extension, -1, &extString);
	ofn.lpstrDefExt = (LPTSTR) Tcl_DStringValue(&extString);
    }
    Tcl_UtfToExternalDString(NULL, Tcl_DStringValue(&utfFilterString),
	    Tcl_DStringLength(&utfFilterString), &filterString);
    ofn.lpstrFilter = (LPTSTR) Tcl_DStringValue(&filterString);

    if (Tcl_DStringValue(&utfDirString)[0] != '\0') {
	Tcl_UtfToExternalDString(NULL, Tcl_DStringValue(&utfDirString),
		Tcl_DStringLength(&utfDirString), &dirString);
        ofn.lpstrInitialDir = (LPTSTR) Tcl_DStringValue(&dirString);
    }
    if (title != NULL) {
	Tcl_UtfToExternalDString(NULL, title, -1, &titleString);
	ofn.lpstrTitle = (LPTSTR) Tcl_DStringValue(&titleString);
    }

    /*
     * Popup the dialog.  
     */

    GetCurrentDirectory(MAX_PATH, savePath);
    oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
    if (open != 0) {
	winCode = GetOpenFileName(&ofn);
    } else {
	winCode = GetSaveFileName(&ofn);
    }
    Tcl_SetServiceMode(oldMode);
    SetCurrentDirectory(savePath);

    /*
     * Clear the interp result since anything may have happened during the
     * modal loop.
     */

    Tcl_ResetResult(interp);

    /*
     * Process the results.
     */

    if (winCode != 0) {
	char *p;
	Tcl_DString ds;

	Tcl_ExternalToUtfDString(NULL, (char *) ofn.lpstrFile, -1, &ds);
	for (p = Tcl_DStringValue(&ds); *p != '\0'; p++) {
	    /*
	     * Change the pathname to the Tcl "normalized" pathname, where
	     * back slashes are used instead of forward slashes
	     */
	    if (*p == '\\') {
		*p = '/';
	    }
	}
	Tcl_AppendResult(interp, Tcl_DStringValue(&ds), NULL);
	Tcl_DStringFree(&ds);
    }

    if (ofn.lpstrTitle != NULL) {
	Tcl_DStringFree(&titleString);
    }
    if (ofn.lpstrInitialDir != NULL) {
	Tcl_DStringFree(&dirString);
    }
    Tcl_DStringFree(&filterString);
    if (ofn.lpstrDefExt != NULL) {
	Tcl_DStringFree(&extString);
    }
    result = TCL_OK;

    end:
    Tcl_DStringFree(&utfDirString);
    Tcl_DStringFree(&utfFilterString);

    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * OFNHookProc --
 *
 *	Hook procedure called only if debugging is turned on.  Sets
 *	the "tk_dialog" variable when the dialog is ready to receive
 *	messages.
 *
 * Results:
 *	Returns 0 to allow default processing of messages to occur.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static UINT APIENTRY 
OFNHookProc(
    HWND hdlg,		// handle to child dialog window
    UINT uMsg,		// message identifier
    WPARAM wParam,	// message parameter
    LPARAM lParam) 	// message parameter
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    OPENFILENAME *ofnPtr;

    if (uMsg == WM_INITDIALOG) {
	SetWindowLong(hdlg, GWL_USERDATA, lParam);
    } else if (uMsg == WM_WINDOWPOSCHANGED) {
	/*
	 * This message is delivered at the right time to both 
	 * old-style and explorer-style hook procs to enable Tk
	 * to set the debug information.  Unhooks itself so it 
	 * won't set the debug information every time it gets a 
	 * WM_WINDOWPOSCHANGED message.
	 */

        ofnPtr = (OPENFILENAME *) GetWindowLong(hdlg, GWL_USERDATA);
	if (ofnPtr != NULL) {
	    if (ofnPtr->Flags & OFN_EXPLORER) {
		hdlg = GetParent(hdlg);
	    }
	    tsdPtr->debugInterp = (Tcl_Interp *) ofnPtr->lCustData;
	    Tcl_DoWhenIdle(SetTkDialog, (ClientData) hdlg);
	    SetWindowLong(hdlg, GWL_USERDATA, (LPARAM) NULL);
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * MakeFilter --
 *
 *	Allocate a buffer to store the filters in a format understood by
 *	Windows
 *
 * Results:
 *	A standard TCL return value.
 *
 * Side effects:
 *	ofnPtr->lpstrFilter is modified.
 *
 *----------------------------------------------------------------------
 */
static int 
MakeFilter(interp, string, dsPtr) 
    Tcl_Interp *interp;		/* Current interpreter. */
    char *string;		/* String value of the -filetypes option */
    Tcl_DString *dsPtr;		/* Filled with windows filter string. */
{
    char *filterStr;
    char *p;
    int pass;
    FileFilterList flist;
    FileFilter *filterPtr;

    TkInitFileFilters(&flist);
    if (TkGetFileFilters(interp, &flist, string, 1) != TCL_OK) {
	return TCL_ERROR;
    }

    if (flist.filters == NULL) {
	/*
	 * Use "All Files (*.*) as the default filter if none is specified
	 */
	char *defaultFilter = "All Files (*.*)";

	p = filterStr = (char*)ckalloc(30 * sizeof(char));

	strcpy(p, defaultFilter);
	p+= strlen(defaultFilter);

	*p++ = '\0';
	*p++ = '*';
	*p++ = '.';
	*p++ = '*';
	*p++ = '\0';
	*p++ = '\0';
	*p = '\0';

    } else {
	/* We format the filetype into a string understood by Windows:
	 * {"Text Documents" {.doc .txt} {TEXT}} becomes
	 * "Text Documents (*.doc,*.txt)\0*.doc;*.txt\0"
	 *
	 * See the Windows OPENFILENAME manual page for details on the filter
	 * string format.
	 */

	/*
	 * Since we may only add asterisks (*) to the filter, we need at most
	 * twice the size of the string to format the filter
	 */
	filterStr = ckalloc(strlen(string) * 3);

	for (filterPtr = flist.filters, p = filterStr; filterPtr;
	        filterPtr = filterPtr->next) {
	    char *sep;
	    FileFilterClause *clausePtr;

	    /*
	     *  First, put in the name of the file type
	     */
	    strcpy(p, filterPtr->name);
	    p+= strlen(filterPtr->name);
	    *p++ = ' ';
	    *p++ = '(';

	    for (pass = 1; pass <= 2; pass++) {
		/*
		 * In the first pass, we format the extensions in the 
		 * name field. In the second pass, we format the extensions in
		 * the filter pattern field
		 */
		sep = "";
		for (clausePtr=filterPtr->clauses;clausePtr;
		         clausePtr=clausePtr->next) {
		    GlobPattern *globPtr;
		

		    for (globPtr=clausePtr->patterns; globPtr;
			    globPtr=globPtr->next) {
			strcpy(p, sep);
			p+= strlen(sep);
			strcpy(p, globPtr->pattern);
			p+= strlen(globPtr->pattern);

			if (pass==1) {
			    sep = ",";
			} else {
			    sep = ";";
			}
		    }
		}
		if (pass == 1) {
		    if (pass == 1) {
			*p ++ = ')';
		    }
		}
		*p ++ = '\0';
	    }
	}

	/*
	 * Windows requires the filter string to be ended by two NULL
	 * characters.
	 */
	*p++ = '\0';
	*p = '\0';
    }

    Tcl_DStringAppend(dsPtr, filterStr, p - filterStr);
    ckfree((char *) filterStr);

    TkFreeFileFilters(&flist);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ChooseDirectoryObjCmd --
 *
 *	This procedure implements the "tk_chooseDirectory" dialog box 
 *	for the Windows platform. See the user documentation for details 
 *	on what it does.
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	A modal dialog window is created.  Tcl_SetServiceMode() is
 *	called to allow background events to be processed
 *
 *----------------------------------------------------------------------
 */

int
Tk_ChooseDirectoryObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    OPENFILENAME ofn;
    TCHAR path[MAX_PATH], savePath[MAX_PATH];
    ChooseDir cd;
    int result, mustExist, code, mode, i;
    Tk_Window tkwin;
    char *utfTitle;
    Tcl_DString utfDirString;
    Tcl_DString titleString, dirString;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    static char *optionStrings[] = {
	"-initialdir",	"-mustexist",	"-parent",	"-title",
	NULL
    };
    enum options {
	DIR_INITIAL,	DIR_EXIST,	DIR_PARENT,	FILE_TITLE
    };

    if (tsdPtr->WM_LBSELCHANGED == 0) {
        tsdPtr->WM_LBSELCHANGED = RegisterWindowMessage(LBSELCHSTRING);
    }
   
    result = TCL_ERROR;
    path[0] = '\0';

    Tcl_DStringInit(&utfDirString);
    mustExist = 0;
    tkwin = (Tk_Window) clientData;
    utfTitle = NULL;

    for (i = 1; i < objc; i += 2) {
	int index;
	char *string;
	Tcl_Obj *optionPtr, *valuePtr;

	optionPtr = objv[i];
	valuePtr = objv[i + 1];

	if (Tcl_GetIndexFromObj(interp, optionPtr, optionStrings, "option",
		0, &index) != TCL_OK) {
	    goto cleanup;
	}
	if (i + 1 == objc) {
	    string = Tcl_GetStringFromObj(optionPtr, NULL);
	    Tcl_AppendResult(interp, "value for \"", string, "\" missing", 
		    (char *) NULL);
	    goto cleanup;
	}

	string = Tcl_GetStringFromObj(valuePtr, NULL);
	switch ((enum options) index) {
	    case DIR_INITIAL: {
		Tcl_DStringFree(&utfDirString);
		if (Tcl_TranslateFileName(interp, string, 
			&utfDirString) == NULL) {
		    goto cleanup;
		}
		break;
	    }
	    case DIR_EXIST: {
		if (Tcl_GetBooleanFromObj(interp, valuePtr, &mustExist) != TCL_OK) {
		    goto cleanup;
		}
		break;
	    }
	    case DIR_PARENT: {
		tkwin = Tk_NameToWindow(interp, string, tkwin);
		if (tkwin == NULL) {
		    goto cleanup;
		}
		break;
	    }
	    case FILE_TITLE: {
		utfTitle = string;
		break;
	    }
	}
    }

    Tk_MakeWindowExist(tkwin);

    cd.interp = interp;

    ofn.lStructSize		= sizeof(ofn);
    ofn.hwndOwner		= Tk_GetHWND(Tk_WindowId(tkwin));
    ofn.hInstance		= (HINSTANCE) GetWindowLong(ofn.hwndOwner, 
					GWL_HINSTANCE);
    ofn.lpstrFilter		= NULL;
    ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 0;
    ofn.lpstrFile		= NULL; //(TCHAR *) path;
    ofn.nMaxFile		= MAX_PATH;
    ofn.lpstrFileTitle		= NULL;
    ofn.nMaxFileTitle		= 0;
    ofn.lpstrInitialDir		= NULL;
    ofn.lpstrTitle		= NULL;
    ofn.Flags			= OFN_HIDEREADONLY  
				  | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
    ofn.nFileOffset		= 0;
    ofn.nFileExtension		= 0;
    ofn.lpstrDefExt		= NULL;
    ofn.lCustData		= (LPARAM) &cd;
    ofn.lpfnHook		= ChooseDirectoryHookProc;
    ofn.lpTemplateName		= MAKEINTRESOURCE(FILEOPENORD);

    if (Tcl_DStringValue(&utfDirString)[0] != '\0') {
	Tcl_UtfToExternalDString(NULL, Tcl_DStringValue(&utfDirString), 
		Tcl_DStringLength(&utfDirString), &dirString);
	ofn.lpstrInitialDir = (LPTSTR) Tcl_DStringValue(&dirString);
    }
    if (mustExist) {
	ofn.Flags |= OFN_PATHMUSTEXIST;
    }
    if (utfTitle != NULL) {
	Tcl_UtfToExternalDString(NULL, utfTitle, -1, &titleString);
	ofn.lpstrTitle = (LPTSTR) Tcl_DStringValue(&titleString);
    }

    /*
     * Display dialog.  The choose directory dialog doesn't preserve the
     * current directory, so it must be saved and restored here.
     */
    
    GetCurrentDirectory(MAX_PATH, savePath);
    mode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
    code = GetOpenFileName(&ofn);
    Tcl_SetServiceMode(mode);
    SetCurrentDirectory(savePath);

    Tcl_ResetResult(interp);
    if (code != 0) {
	/*
	 * Change the pathname to the Tcl "normalized" pathname, where
	 * back slashes are used instead of forward slashes
	 */

	char *p;
	Tcl_DString ds;

	Tcl_ExternalToUtfDString(NULL, (char *) cd.path, -1, &ds);
	for (p = Tcl_DStringValue(&ds); *p != '\0'; p++) {
	    if (*p == '\\') {
		*p = '/';
	    }
	}
	Tcl_AppendResult(interp, Tcl_DStringValue(&ds), NULL);
	Tcl_DStringFree(&ds);
    }

    if (ofn.lpstrTitle != NULL) {
	Tcl_DStringFree(&titleString);
    }
    if (ofn.lpstrInitialDir != NULL) {
	Tcl_DStringFree(&dirString);
    }
    result = TCL_OK;

    cleanup:
    Tcl_DStringFree(&utfDirString);

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ChooseDirectoryHookProc --
 *
 *	Hook procedure called by the ChooseDirectory dialog to modify
 *	its default behavior.  The ChooseDirectory dialog is really an
 *	OpenFile dialog with certain controls rearranged and certain
 *	behaviors changed.  For instance, typing a name in the 
 *	ChooseDirectory dialog selects a directory, rather than 
 *	selecting a file.
 *
 * Results:
 *	Returns 0 to allow default processing of message, or 1 to 
 *	tell default dialog procedure not to process the message.
 *
 * Side effects:
 *	A dialog window is created the first this procedure is called.
 *	This window is not destroyed and will be reused the next time
 *	the application invokes the "tk_getOpenFile" or
 *	"tk_getSaveFile" command.
 *
 *----------------------------------------------------------------------
 */

static UINT APIENTRY 
ChooseDirectoryHookProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    OPENFILENAME *ofnPtr;

    /*
     * GWL_USERDATA keeps track of ofnPtr.
     */
    
    ofnPtr = (OPENFILENAME *) GetWindowLong(hwnd, GWL_USERDATA);

    if (message == WM_INITDIALOG) {
        ChooseDir *cdPtr;

	SetWindowLong(hwnd, GWL_USERDATA, lParam);
	ofnPtr = (OPENFILENAME *) lParam;
	cdPtr = (ChooseDir *) ofnPtr->lCustData;
	cdPtr->lastCtrl = 0;
	cdPtr->lastIdx = 1000;
	cdPtr->path[0] = '\0';

	if (ofnPtr->lpstrInitialDir == NULL) {
	    GetCurrentDirectory(MAX_PATH, cdPtr->path);
	} else {
	    lstrcpy(cdPtr->path, ofnPtr->lpstrInitialDir);
	}
	SetDlgItemText(hwnd, edt10, cdPtr->path);
	SendDlgItemMessage(hwnd, edt10, EM_SETSEL, 0, -1);
	if (tsdPtr->debugFlag) {
	    tsdPtr->debugInterp = cdPtr->interp;
	    Tcl_DoWhenIdle(SetTkDialog, (ClientData) hwnd);
	}
	return 0;
    }
    if (ofnPtr == NULL) {
	return 0;
    }

    if (message == tsdPtr->WM_LBSELCHANGED) {
	/*
	 * Called when double-clicking on directory.
	 * If directory wasn't already open, browse that directory.
	 * If directory was already open, return selected directory.
	 */

        ChooseDir *cdPtr;
	int idCtrl, thisItem;

	idCtrl = (int) wParam;
        thisItem = LOWORD(lParam);
	cdPtr = (ChooseDir *) ofnPtr->lCustData;

	GetCurrentDirectory(MAX_PATH, cdPtr->path);
	if (idCtrl == lst2) {
	    if ((cdPtr->lastIdx < 0) || (cdPtr->lastIdx == thisItem)) {
		EndDialog(hwnd, IDOK);
		return 1;
	    }
	    cdPtr->lastIdx = thisItem;
	}
	SetDlgItemText(hwnd, edt10, cdPtr->path);
	SendDlgItemMessage(hwnd, edt10, EM_SETSEL, 0, -1);
    } else if (message == WM_COMMAND) {
        ChooseDir *cdPtr;
	int idCtrl, notifyCode;

	idCtrl = LOWORD(wParam);
	notifyCode = HIWORD(wParam);
	cdPtr = (ChooseDir *) ofnPtr->lCustData;

	if ((idCtrl != IDOK) || (notifyCode != BN_CLICKED)) {
	    /*
	     * OK Button wasn't clicked.  Do the default.
	     */

	    if ((idCtrl == lst2) || (idCtrl == edt10)) {
		cdPtr->lastCtrl = idCtrl;
	    }
	    return 0;
	}

	/*
	 * Dialogs also get the message that OK was clicked when Enter 
	 * is pressed in some other control.  Find out what window
	 * we were really in when we got the supposed "OK", because the 
	 * behavior is different.
	 */

	if (cdPtr->lastCtrl == edt10) {
	    /*
	     * Hit Enter or clicked OK while typing a directory name in the 
	     * edit control.
	     * If it's a new name, try to go to that directory.
	     * If the name hasn't changed since last time, return selected 
	     * directory.
	     */

	    int changed;
	    TCHAR tmp[MAX_PATH];

	    if (GetDlgItemText(hwnd, edt10, tmp, MAX_PATH) == 0) {
		return 0;
	    }

	    changed = lstrcmp(cdPtr->path, tmp);
	    lstrcpy(cdPtr->path, tmp);

	    if (SetCurrentDirectory(cdPtr->path) == 0) {
		/*
		 * Non-existent directory.
		 */

		if (ofnPtr->Flags & OFN_PATHMUSTEXIST) {
		    /*
		     * Directory must exist.  Complain, then rehighlight text.
		     */

		    wsprintf(tmp, _T("Cannot change directory to \"%.200s\"."), 
			    cdPtr->path);
		    MessageBox(hwnd, tmp, NULL, MB_OK);
		    SendDlgItemMessage(hwnd, edt10, EM_SETSEL, 0, -1);
		    return 0;
		} 
		if (changed) {
		    /*
		     * Directory was invalid, but we want to keep displaying
		     * this name.  Don't update the listbox that displays the 
		     * current directory heirarchy, or it'll erase the name.
		     */
		    
		    SendDlgItemMessage(hwnd, edt10, EM_SETSEL, 0, -1);
		    return 0;
		}
	    }
	    if (changed == 0) {
		/*
		 * Name hasn't changed since the last time we hit return
		 * or double-clicked on a directory, so return this.
		 */

		EndDialog(hwnd, IDOK);
		return 1;
	    }
	    
	    cdPtr->lastCtrl = IDOK;

	    /*
	     * The following is the magic code, determined by running 
	     * Spy++ on some other directory chooser, that it takes to 
	     * get this dialog to update the listbox to display the 
	     * current directory.
	     */

	    SetDlgItemText(hwnd, edt1, cdPtr->path);
	    SendMessage(hwnd, WM_COMMAND, (WPARAM) MAKELONG(cmb2, 0x8003), 
		    (LPARAM) GetDlgItem(hwnd, cmb2));
	    return 0;
	} else if (idCtrl == lst2) {
	    /*
	     * Enter key was pressed while in listbox.  
	     * If it's a new directory, allow default behavior to open dir.
	     * If the directory hasn't changed, return selected directory.
	     */

	    int thisItem;

	    thisItem = (int) SendDlgItemMessage(hwnd, lst2, LB_GETCURSEL, 0, 0);
	    if (cdPtr->lastIdx == thisItem) {
		GetCurrentDirectory(MAX_PATH, cdPtr->path);
		EndDialog(hwnd, IDOK);
		return 1;
	    }
	} else if (idCtrl == IDOK) {
	    /* 
	     * The OK button was clicked.  Return the path currently specified
	     * in the listbox.  
	     *
	     * The directory has not yet been changed to the one specified in
	     * the listbox.  Returning 0 allows the default dialog proc to 
	     * change the directory to the one specified in the listbox and 
	     * then causes it to send a WM_LBSELCHANGED back to the hook proc.  
	     * When we get that message, we will record the current directory
	     * and then quit.
	     */

	    cdPtr->lastIdx = -1;
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MessageBoxObjCmd --
 *
 *	This procedure implements the MessageBox window for the
 *	Windows platform. See the user documentation for details on what
 *	it does.
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	None. The MessageBox window will be destroy before this procedure
 *	returns.
 *
 *----------------------------------------------------------------------
 */

int
Tk_MessageBoxObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Tk_Window tkwin, parent;
    HWND hWnd;
    char *message, *title;
    int defaultBtn, icon, type;
    int i, oldMode, flags, winCode;
    Tcl_DString messageString, titleString;
    static char *optionStrings[] = {
	"-default",	"-icon",	"-message",	"-parent",
	"-title",	"-type",	NULL
    };
    enum options {
	MSG_DEFAULT,	MSG_ICON,	MSG_MESSAGE,	MSG_PARENT,
	MSG_TITLE,	MSG_TYPE
    };

    tkwin = (Tk_Window) clientData;

    defaultBtn	= -1;
    icon	= MB_ICONINFORMATION;
    message	= NULL;
    parent	= tkwin;
    title	= NULL;
    type	= MB_OK;

    for (i = 1; i < objc; i += 2) {
	int index;
	char *string;
	Tcl_Obj *optionPtr, *valuePtr;

	optionPtr = objv[i];
	valuePtr = objv[i + 1];

	if (Tcl_GetIndexFromObj(interp, optionPtr, optionStrings, "option",
		TCL_EXACT, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (i + 1 == objc) {
	    string = Tcl_GetStringFromObj(optionPtr, NULL);
	    Tcl_AppendResult(interp, "value for \"", string, "\" missing", 
		    (char *) NULL);
	    return TCL_ERROR;
	}

        string = Tcl_GetStringFromObj(valuePtr, NULL);
	switch ((enum options) index) {
        case MSG_DEFAULT:
	    defaultBtn = TkFindStateNumObj(interp, optionPtr, buttonMap, 
		    valuePtr);
	    if (defaultBtn < 0) {
		return TCL_ERROR;
	    }
	    break;

	case MSG_ICON:
	    icon = TkFindStateNumObj(interp, optionPtr, iconMap, valuePtr);
	    if (icon < 0) {
		return TCL_ERROR;
	    }
	    break;

	case MSG_MESSAGE:
	    message = string;
	    break;

	case MSG_PARENT: 
	    parent = Tk_NameToWindow(interp, string, tkwin);
	    if (parent == NULL) {
		return TCL_ERROR;
	    }
	    break;

	case MSG_TITLE:
	    title = string;
	    break;

	case MSG_TYPE:
	    type = TkFindStateNumObj(interp, optionPtr, typeMap, valuePtr);
	    if (type < 0) {
		return TCL_ERROR;
	    }
	    break;

	}
    }

    Tk_MakeWindowExist(parent);
    hWnd = Tk_GetHWND(Tk_WindowId(parent));

    flags = 0;
    if (defaultBtn >= 0) {
	int defaultBtnIdx;

	defaultBtnIdx = -1;
	for (i = 0; i < NUM_TYPES; i++) {
	    if (type == allowedTypes[i].type) {
		int j;

		for (j = 0; j < 3; j++) {
		    if (allowedTypes[i].btnIds[j] == defaultBtn) {
			defaultBtnIdx = j;
			break;
		    }
		}
		if (defaultBtnIdx < 0) {
		    Tcl_AppendResult(interp, "invalid default button \"",
			    TkFindStateString(buttonMap, defaultBtn), 
			    "\"", NULL);
		    return TCL_ERROR;
		}
		break;
	    }
	}
	flags = buttonFlagMap[defaultBtnIdx];
    }
    
    flags |= icon | type | MB_SYSTEMMODAL;

    Tcl_UtfToExternalDString(NULL, message, -1, &messageString);
    Tcl_UtfToExternalDString(NULL, title, -1, &titleString);

    oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
    winCode = MessageBox(hWnd, Tcl_DStringValue(&messageString),
		Tcl_DStringValue(&titleString), flags);
    (void) Tcl_SetServiceMode(oldMode);

    Tcl_DStringFree(&messageString);
    Tcl_DStringFree(&titleString);

    Tcl_SetResult(interp, TkFindStateString(buttonMap, winCode), TCL_STATIC);
    return TCL_OK;
}

static void 
SetTkDialog(ClientData clientData)
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    char buf[32];
    HWND hwnd;

    hwnd = (HWND) clientData;

    sprintf(buf, "0x%08x", hwnd);
    Tcl_SetVar(tsdPtr->debugInterp, "tk_dialog", buf, TCL_GLOBAL_ONLY);
}
