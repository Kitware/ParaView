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

/*
 * This controls the use of the new style tk_chooseDirectory dialog.
 */
#define USE_NEW_CHOOSEDIR 1
#ifdef USE_NEW_CHOOSEDIR
#include <shlobj.h>     /* includes SHBrowseForFolder */

/* These needed for compilation with VC++ 5.2 */
#ifndef BIF_EDITBOX
#define BIF_EDITBOX 0x10
#endif
#ifndef BIF_VALIDATE
#define BIF_VALIDATE 0x0020
#endif
#ifndef BFFM_VALIDATEFAILED
#ifdef UNICODE
#define BFFM_VALIDATEFAILED 4
#else
#define BFFM_VALIDATEFAILED 3
#endif
#endif 

/*
 * The following structure is used by the new Tk_ChooseDirectoryObjCmd
 * to pass data between it and its callback. Unqiue to Winodws platform.
 */
typedef struct ChooseDirData {
   TCHAR utfInitDir[MAX_PATH];       /* Initial folder to use */
   TCHAR utfRetDir[MAX_PATH];        /* Returned folder to use */
   Tcl_Interp *interp;
   int mustExist;                    /* true if file must exist to return from
				      * callback */
} CHOOSEDIRDATA;
#endif

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
 * The value of TK_MULTI_MAX_PATH dictactes how many files can
 * be retrieved with tk_get*File -multiple 1.  It must be allocated
 * on the stack, so make it large enough but not too large.  -- hobbs
 * The data is stored as <dir>\0<file1>\0<file2>\0...<fileN>\0\0.
 * MAX_PATH == 260 on Win2K/NT, so *40 is ~10K.
 */

#define TK_MULTI_MAX_PATH	(MAX_PATH*40)

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
    OPENFILENAME *ofnPtr;	/* pointer to the OFN structure */
} ChooseDir;

/*
 * Definitions of procedures used only in this file.
 */

#ifdef USE_NEW_CHOOSEDIR
static UINT APIENTRY	ChooseDirectoryValidateProc(HWND hdlg, UINT uMsg,
			    LPARAM wParam, LPARAM lParam);
#else
static UINT APIENTRY	ChooseDirectoryHookProc(HWND hdlg, UINT uMsg, 
			    WPARAM wParam, LPARAM lParam);
#endif
static UINT CALLBACK	ColorDlgHookProc(HWND hDlg, UINT uMsg, WPARAM wParam,
			    LPARAM lParam);
static int 		GetFileNameA(ClientData clientData, 
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[], int isOpen);
static int 		GetFileNameW(ClientData clientData, 
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[], int isOpen);
static int 		MakeFilter(Tcl_Interp *interp, char *string, 
			    Tcl_DString *dsPtr);
static UINT APIENTRY	OFNHookProc(HWND hdlg, UINT uMsg, WPARAM wParam, 
			    LPARAM lParam);
static UINT APIENTRY	OFNHookProcW(HWND hdlg, UINT uMsg, WPARAM wParam, 
			    LPARAM lParam);
static void		SetTkDialog(ClientData clientData);

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
    HWND hWnd;
    int i, oldMode, winCode, result;
    CHOOSECOLOR chooseColor;
    static int inited = 0;
    static COLORREF dwCustColors[16];
    static long oldColor;		/* the color selected last time */
    static CONST char *optionStrings[] = {
	"-initialcolor", "-parent", "-title", NULL
    };
    enum options {
	COLOR_INITIAL, COLOR_PARENT, COLOR_TITLE
    };

    result = TCL_OK;
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
    chooseColor.lStructSize	= sizeof(CHOOSECOLOR);
    chooseColor.hwndOwner	= NULL;			
    chooseColor.hInstance	= NULL;
    chooseColor.rgbResult	= oldColor;
    chooseColor.lpCustColors	= dwCustColors;
    chooseColor.Flags		= CC_RGBINIT | CC_FULLOPEN | CC_ENABLEHOOK;
    chooseColor.lCustData	= (LPARAM) NULL;
    chooseColor.lpfnHook	= (LPOFNHOOKPROC) ColorDlgHookProc;
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
    chooseColor.hwndOwner = NULL;
    hWnd = Tk_GetHWND(Tk_WindowId(parent));
    chooseColor.hwndOwner = hWnd;
    
    oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
    winCode = ChooseColor(&chooseColor);
    (void) Tcl_SetServiceMode(oldMode);

    /*
     * Ensure that hWnd is enabled, because it can happen that we
     * have updated the wrapper of the parent, which causes us to
     * leave this child disabled (Windows loses sync).
     */
    EnableWindow(hWnd, 1);

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
	char color[100];

	sprintf(color, "#%02x%02x%02x",
		GetRValue(chooseColor.rgbResult), 
	        GetGValue(chooseColor.rgbResult), 
		GetBValue(chooseColor.rgbResult));
        Tcl_AppendResult(interp, color, NULL);
	oldColor = chooseColor.rgbResult;
	result = TCL_OK;
    }

    return result;
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
		(*tkWinProcs->setWindowText)(hDlg,
			Tcl_WinUtfToTChar(title, -1, &ds));
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
    if (TkWinGetPlatformId() == VER_PLATFORM_WIN32_NT) {
	return GetFileNameW(clientData, interp, objc, objv, 1);
    } else {
	return GetFileNameA(clientData, interp, objc, objv, 1);
    }
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
    if (TkWinGetPlatformId() == VER_PLATFORM_WIN32_NT) {
	return GetFileNameW(clientData, interp, objc, objv, 0);
    } else {
	return GetFileNameA(clientData, interp, objc, objv, 0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetFileNameW --
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
GetFileNameW(clientData, interp, objc, objv, open)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
    int open;			/* 1 to call GetOpenFileName(), 0 to 
				 * call GetSaveFileName(). */
{
    OPENFILENAMEW ofn;
    WCHAR file[TK_MULTI_MAX_PATH];
    int result, winCode, oldMode, i, multi = 0;
    char *extension, *filter, *title;
    Tk_Window tkwin;
    HWND hWnd;
    Tcl_DString utfFilterString, utfDirString;
    Tcl_DString extString, filterString, dirString, titleString;
    Tcl_Encoding unicodeEncoding = TkWinGetUnicodeEncoding();
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    static CONST char *saveOptionStrings[] = {
	"-defaultextension", "-filetypes", "-initialdir", "-initialfile",
	"-parent", "-title", NULL
    };
    static CONST char *openOptionStrings[] = {
	"-defaultextension", "-filetypes", "-initialdir", "-initialfile",
	"-multiple", "-parent", "-title", NULL
    };
    CONST char **optionStrings;

    enum options {
	FILE_DEFAULT,	FILE_TYPES,	FILE_INITDIR,	FILE_INITFILE,
	FILE_MULTIPLE,	FILE_PARENT,	FILE_TITLE
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

    if (open) {
	optionStrings = openOptionStrings;
    } else {
	optionStrings = saveOptionStrings;
    }

    for (i = 1; i < objc; i += 2) {
	int index;
	char *string;
	Tcl_Obj *optionPtr, *valuePtr;

	optionPtr = objv[i];
	valuePtr = objv[i + 1];

	if (Tcl_GetIndexFromObj(interp, optionPtr, optionStrings,
		"option", 0, &index) != TCL_OK) {
	    goto end;
	}
	/*
	 * We want to maximize code sharing between the open and save file
	 * dialog implementations; in particular, the switch statement below.
	 * We use different sets of option strings from the GetIndexFromObj
	 * call above, but a single enumeration for both.  The save file
	 * dialog doesn't support -multiple, but it falls in the middle of
	 * the enumeration.  Ultimately, this means that when the index found
	 * by GetIndexFromObj is >= FILE_MULTIPLE, when doing a save file
	 * dialog, we have to increment the index, so that it matches the
	 * open file dialog enumeration.
	 */
	if (!open && index >= FILE_MULTIPLE) {
	    index++;
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
		Tcl_UtfToExternal(NULL, unicodeEncoding, Tcl_DStringValue(&ds),
			Tcl_DStringLength(&ds), 0, NULL, (char *) file,
			sizeof(file), NULL, NULL, NULL);
		break;
	    }
	    case FILE_MULTIPLE: {
		if (Tcl_GetBooleanFromObj(interp, valuePtr,
			&multi) != TCL_OK) {
		    return TCL_ERROR;
		}
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
    hWnd = Tk_GetHWND(Tk_WindowId(tkwin));

    ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
    ofn.lStructSize		= sizeof(OPENFILENAMEW);
    ofn.hwndOwner		= hWnd;
#ifdef _WIN64
    ofn.hInstance		= (HINSTANCE) GetWindowLongPtr(ofn.hwndOwner, 
					GWLP_HINSTANCE);
#else
    ofn.hInstance		= (HINSTANCE) GetWindowLong(ofn.hwndOwner, 
					GWL_HINSTANCE);
#endif
    ofn.lpstrFile		= (WCHAR *) file;
    ofn.nMaxFile		= TK_MULTI_MAX_PATH;
    ofn.Flags			= OFN_HIDEREADONLY | OFN_PATHMUSTEXIST 
				  | OFN_NOCHANGEDIR | OFN_EXPLORER;
    ofn.lpfnHook		= (LPOFNHOOKPROC) OFNHookProcW;
    ofn.lCustData		= (LPARAM) interp;

    if (open != 0) {
	ofn.Flags |= OFN_FILEMUSTEXIST;
    } else {
	ofn.Flags |= OFN_OVERWRITEPROMPT;
    }

    if (tsdPtr->debugFlag != 0) {
	ofn.Flags |= OFN_ENABLEHOOK;
    }

    if (multi != 0) {
	ofn.Flags |= OFN_ALLOWMULTISELECT;
    }

    if (extension != NULL) {
	Tcl_UtfToExternalDString(unicodeEncoding, extension, -1, &extString);
	ofn.lpstrDefExt = (WCHAR *) Tcl_DStringValue(&extString);
    }

    Tcl_UtfToExternalDString(unicodeEncoding,
	    Tcl_DStringValue(&utfFilterString),
	    Tcl_DStringLength(&utfFilterString), &filterString);
    ofn.lpstrFilter = (WCHAR *) Tcl_DStringValue(&filterString);

    if (Tcl_DStringValue(&utfDirString)[0] != '\0') {
	Tcl_UtfToExternalDString(unicodeEncoding,
		Tcl_DStringValue(&utfDirString),
		Tcl_DStringLength(&utfDirString), &dirString);
    } else {
	/*
	 * NT 5.0 changed the meaning of lpstrInitialDir, so we have
	 * to ensure that we set the [pwd] if the user didn't specify
	 * anything else.
	 */
	Tcl_DString cwd;

	Tcl_DStringFree(&utfDirString);
	if ((Tcl_GetCwd(interp, &utfDirString) == (char *) NULL) ||
		(Tcl_TranslateFileName(interp,
			Tcl_DStringValue(&utfDirString), &cwd) == NULL)) {
	    Tcl_ResetResult(interp);
	} else {
	    Tcl_UtfToExternalDString(unicodeEncoding, Tcl_DStringValue(&cwd),
		    Tcl_DStringLength(&cwd), &dirString);
	}
	Tcl_DStringFree(&cwd);
    }
    ofn.lpstrInitialDir = (WCHAR *) Tcl_DStringValue(&dirString);

    if (title != NULL) {
	Tcl_UtfToExternalDString(unicodeEncoding, title, -1, &titleString);
	ofn.lpstrTitle = (WCHAR *) Tcl_DStringValue(&titleString);
    }

    /*
     * Popup the dialog.
     */

    oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
    if (open != 0) {
	winCode = GetOpenFileNameW(&ofn);
    } else {
	winCode = GetSaveFileNameW(&ofn);
    }
    Tcl_SetServiceMode(oldMode);

    /*
     * Ensure that hWnd is enabled, because it can happen that we
     * have updated the wrapper of the parent, which causes us to
     * leave this child disabled (Windows loses sync).
     */
    EnableWindow(hWnd, 1);

    /*
     * Clear the interp result since anything may have happened during the
     * modal loop.
     */

    Tcl_ResetResult(interp);

    /*
     * Process the results.
     */

    if (winCode != 0) {
	if (ofn.Flags & OFN_ALLOWMULTISELECT) {
            /*
	     * The result in custData->szFile contains many items,
	     * separated with null characters.  It is terminated with
	     * two nulls in a row.  The first element is the directory
	     * path.
	     */
	    char *dir;
	    char *p;
	    char *file;
	    WCHAR *files;
	    Tcl_DString ds;
	    Tcl_DString fullname, filename;
	    Tcl_Obj *returnList;
	    int count = 0;

	    returnList = Tcl_NewObj();
	    Tcl_IncrRefCount(returnList);

	    files = ofn.lpstrFile;
	    Tcl_ExternalToUtfDString(unicodeEncoding, (char *) files, -1, &ds);

	    /* Get directory */
	    dir = Tcl_DStringValue(&ds);
	    for (p = dir; p && *p; p++) {
		/*
		 * Change the pathname to the Tcl "normalized" pathname, where
		 * back slashes are used instead of forward slashes
		 */
		if (*p == '\\') {
		    *p = '/';
		}
	    }

	    while (*files != '\0') {
		while (*files != '\0') {
		    files++;
		}
		files++;
		if (*files != '\0') {
		    count++;
		    Tcl_ExternalToUtfDString(unicodeEncoding,
			    (char *)files, -1, &filename);
		    file = Tcl_DStringValue(&filename);
		    for (p = file; *p != '\0'; p++) {
			if (*p == '\\') {
			    *p = '/';
			}
		    }
		    Tcl_DStringInit(&fullname);
		    Tcl_DStringAppend(&fullname, dir, -1);
		    Tcl_DStringAppend(&fullname, "/", -1);
		    Tcl_DStringAppend(&fullname, file, -1);
		    Tcl_ListObjAppendElement(interp, returnList,
			    Tcl_NewStringObj(Tcl_DStringValue(&fullname), -1));
		    Tcl_DStringFree(&fullname);
		    Tcl_DStringFree(&filename);
		}
	    }
	    if (count == 0) {
		/*
		 * Only one file was returned.
		 */
		Tcl_ListObjAppendElement(interp, returnList,
			Tcl_NewStringObj(dir, -1));
	    }
	    Tcl_SetObjResult(interp, returnList);
	    Tcl_DecrRefCount(returnList);
	    Tcl_DStringFree(&ds);
	} else {
	    char *p;
	    Tcl_DString ds;
	    
	    Tcl_ExternalToUtfDString(unicodeEncoding,
		    (char *) ofn.lpstrFile, -1, &ds);
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
	result = TCL_OK;
    } else {
	/*
	 * Use the CommDlgExtendedError() function to retrieve the error code.
	 * This function can return one of about two dozen codes; most of
	 * these indicate some sort of gross system failure (insufficient
	 * memory, bad window handles, etc.).  Most of the error codes will be
	 * ignored; as we find we want more specific error messages for
	 * particular errors, we can extend the code as needed.
	 *
	 * We could also check for FNERR_BUFFERTOOSMALL, but we can't
	 * really do anything about it when it happens.
	 */

	if (CommDlgExtendedError() == FNERR_INVALIDFILENAME) {
	    char *p;
	    Tcl_DString ds;
	    
	    Tcl_ExternalToUtfDString(unicodeEncoding,
		    (char *) ofn.lpstrFile, -1, &ds);
	    for (p = Tcl_DStringValue(&ds); *p != '\0'; p++) {
		/*
		 * Change the pathname to the Tcl "normalized" pathname,
		 * where back slashes are used instead of forward slashes
		 */
		if (*p == '\\') {
		    *p = '/';
		}
	    }
	    Tcl_SetResult(interp, "invalid filename \"", TCL_STATIC);
	    Tcl_AppendResult(interp, Tcl_DStringValue(&ds), "\"", NULL);
	    Tcl_DStringFree(&ds);
	} else {
	    result = TCL_OK;
	}
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

    end:
    Tcl_DStringFree(&utfDirString);
    Tcl_DStringFree(&utfFilterString);

    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * OFNHookProcW --
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
OFNHookProcW(
    HWND hdlg,		// handle to child dialog window
    UINT uMsg,		// message identifier
    WPARAM wParam,	// message parameter
    LPARAM lParam) 	// message parameter
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    OPENFILENAMEW *ofnPtr;

    if (uMsg == WM_INITDIALOG) {
#ifdef _WIN64
	SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
#else
	SetWindowLong(hdlg, GWL_USERDATA, lParam);
#endif
    } else if (uMsg == WM_WINDOWPOSCHANGED) {
	/*
	 * This message is delivered at the right time to enable Tk
	 * to set the debug information.  Unhooks itself so it 
	 * won't set the debug information every time it gets a 
	 * WM_WINDOWPOSCHANGED message.
	 */

#ifdef _WIN64
        ofnPtr = (OPENFILENAMEW *) GetWindowLongPtr(hdlg, GWLP_USERDATA);
#else
        ofnPtr = (OPENFILENAMEW *) GetWindowLong(hdlg, GWL_USERDATA);
#endif
	if (ofnPtr != NULL) {
	    hdlg = GetParent(hdlg);
	    tsdPtr->debugInterp = (Tcl_Interp *) ofnPtr->lCustData;
	    Tcl_DoWhenIdle(SetTkDialog, (ClientData) hdlg);
#ifdef _WIN64
	    SetWindowLongPtr(hdlg, GWLP_USERDATA, (LPARAM) NULL);
#else
	    SetWindowLong(hdlg, GWL_USERDATA, (LPARAM) NULL);
#endif
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * GetFileNameA --
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
GetFileNameA(clientData, interp, objc, objv, open)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
    int open;			/* 1 to call GetOpenFileName(), 0 to 
				 * call GetSaveFileName(). */
{
    OPENFILENAME ofn;
    TCHAR file[TK_MULTI_MAX_PATH], savePath[MAX_PATH];
    int result, winCode, oldMode, i, multi = 0;
    char *extension, *filter, *title;
    Tk_Window tkwin;
    HWND hWnd;
    Tcl_DString utfFilterString, utfDirString;
    Tcl_DString extString, filterString, dirString, titleString;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    static CONST char *saveOptionStrings[] = {
	"-defaultextension", "-filetypes", "-initialdir", "-initialfile",
	"-parent", "-title", NULL
    };
    static CONST char *openOptionStrings[] = {
	"-defaultextension", "-filetypes", "-initialdir", "-initialfile",
	"-multiple", "-parent", "-title", NULL
    };
    CONST char **optionStrings;

    enum options {
	FILE_DEFAULT,	FILE_TYPES,	FILE_INITDIR,	FILE_INITFILE,
	FILE_MULTIPLE,	FILE_PARENT,	FILE_TITLE
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

    if (open) {
	optionStrings = openOptionStrings;
    } else {
	optionStrings = saveOptionStrings;
    }

    for (i = 1; i < objc; i += 2) {
	int index;
	char *string;
	Tcl_Obj *optionPtr, *valuePtr;

	optionPtr = objv[i];
	valuePtr = objv[i + 1];

	if (Tcl_GetIndexFromObj(interp, optionPtr, optionStrings,
		"option", 0, &index) != TCL_OK) {
	    goto end;
	}
	/*
	 * We want to maximize code sharing between the open and save file
	 * dialog implementations; in particular, the switch statement below.
	 * We use different sets of option strings from the GetIndexFromObj
	 * call above, but a single enumeration for both.  The save file
	 * dialog doesn't support -multiple, but it falls in the middle of
	 * the enumeration.  Ultimately, this means that when the index found
	 * by GetIndexFromObj is >= FILE_MULTIPLE, when doing a save file
	 * dialog, we have to increment the index, so that it matches the
	 * open file dialog enumeration.
	 */
	if (!open && index >= FILE_MULTIPLE) {
	    index++;
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
	    case FILE_MULTIPLE: {
		if (Tcl_GetBooleanFromObj(interp, valuePtr,
			&multi) != TCL_OK) {
		    return TCL_ERROR;
		}
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
    hWnd = Tk_GetHWND(Tk_WindowId(tkwin));

    ofn.lStructSize		= sizeof(ofn);
    ofn.hwndOwner		= hWnd;
#ifdef _WIN64
    ofn.hInstance		= (HINSTANCE) GetWindowLongPtr(ofn.hwndOwner, 
					GWLP_HINSTANCE);
#else
    ofn.hInstance		= (HINSTANCE) GetWindowLong(ofn.hwndOwner, 
					GWL_HINSTANCE);
#endif
    ofn.lpstrFilter		= NULL;
    ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 0;
    ofn.lpstrFile		= (LPTSTR) file;
    ofn.nMaxFile		= TK_MULTI_MAX_PATH;
    ofn.lpstrFileTitle		= NULL;
    ofn.nMaxFileTitle		= 0;
    ofn.lpstrInitialDir		= NULL;
    ofn.lpstrTitle		= NULL;
    ofn.Flags			= OFN_HIDEREADONLY | OFN_PATHMUSTEXIST 
				  | OFN_NOCHANGEDIR | OFN_EXPLORER;
    ofn.nFileOffset		= 0;
    ofn.nFileExtension		= 0;
    ofn.lpstrDefExt		= NULL;
    ofn.lpfnHook		= (LPOFNHOOKPROC) OFNHookProc;
    ofn.lCustData		= (LPARAM) interp;
    ofn.lpTemplateName		= NULL;

    if (open != 0) {
	ofn.Flags |= OFN_FILEMUSTEXIST;
    } else {
	ofn.Flags |= OFN_OVERWRITEPROMPT;
    }

    if (tsdPtr->debugFlag != 0) {
	ofn.Flags |= OFN_ENABLEHOOK;
    }

    if (multi != 0) {
	ofn.Flags |= OFN_ALLOWMULTISELECT;
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
    } else {
	/*
	 * NT 5.0 changed the meaning of lpstrInitialDir, so we have
	 * to ensure that we set the [pwd] if the user didn't specify
	 * anything else.
	 */
	Tcl_DString cwd;

	Tcl_DStringFree(&utfDirString);
	if ((Tcl_GetCwd(interp, &utfDirString) == (char *) NULL) ||
		(Tcl_TranslateFileName(interp,
			Tcl_DStringValue(&utfDirString), &cwd) == NULL)) {
	    Tcl_ResetResult(interp);
	} else {
	    Tcl_UtfToExternalDString(NULL, Tcl_DStringValue(&cwd),
		    Tcl_DStringLength(&cwd), &dirString);
	}
	Tcl_DStringFree(&cwd);
    }
    ofn.lpstrInitialDir = (LPTSTR) Tcl_DStringValue(&dirString);

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
     * Ensure that hWnd is enabled, because it can happen that we
     * have updated the wrapper of the parent, which causes us to
     * leave this child disabled (Windows loses sync).
     */
    EnableWindow(hWnd, 1);

    /*
     * Clear the interp result since anything may have happened during the
     * modal loop.
     */

    Tcl_ResetResult(interp);

    /*
     * Process the results.
     */

    if (winCode != 0) {
	if (ofn.Flags & OFN_ALLOWMULTISELECT) {
            /*
	     * The result in custData->szFile contains many items,
	     * separated with null characters.  It is terminated with
	     * two nulls in a row.  The first element is the directory
	     * path.
	     */
	    char *dir;
	    char *p;
	    char *file;
	    char *files;
	    Tcl_DString ds;
	    Tcl_DString fullname, filename;
	    Tcl_Obj *returnList;
	    int count = 0;

	    returnList = Tcl_NewObj();
	    Tcl_IncrRefCount(returnList);

	    files = ofn.lpstrFile;
	    Tcl_ExternalToUtfDString(NULL, (char *) files, -1, &ds);

	    /* Get directory */
	    dir = Tcl_DStringValue(&ds);
	    for (p = dir; p && *p; p++) {
		/*
		 * Change the pathname to the Tcl "normalized" pathname, where
		 * back slashes are used instead of forward slashes
		 */
		if (*p == '\\') {
		    *p = '/';
		}
	    }

	    while (*files != '\0') {
		while (*files != '\0') {
		    files++;
		}
		files++;
		if (*files != '\0') {
		    count++;
		    Tcl_ExternalToUtfDString(NULL,
			    (char *)files, -1, &filename);
		    file = Tcl_DStringValue(&filename);
		    for (p = file; *p != '\0'; p++) {
			if (*p == '\\') {
			    *p = '/';
			}
		    }
		    Tcl_DStringInit(&fullname);
		    Tcl_DStringAppend(&fullname, dir, -1);
		    Tcl_DStringAppend(&fullname, "/", -1);
		    Tcl_DStringAppend(&fullname, file, -1);
		    Tcl_ListObjAppendElement(interp, returnList,
			    Tcl_NewStringObj(Tcl_DStringValue(&fullname), -1));
		    Tcl_DStringFree(&fullname);
		    Tcl_DStringFree(&filename);
		}
	    }
	    if (count == 0) {
		/*
		 * Only one file was returned.
		 */
		Tcl_ListObjAppendElement(interp, returnList,
			Tcl_NewStringObj(dir, -1));
	    }
	    Tcl_SetObjResult(interp, returnList);
	    Tcl_DecrRefCount(returnList);
	    Tcl_DStringFree(&ds);
	} else {
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
	result = TCL_OK;
    } else {
	/*
	 * Use the CommDlgExtendedError() function to retrieve the error code.
	 * This function can return one of about two dozen codes; most of
	 * these indicate some sort of gross system failure (insufficient
	 * memory, bad window handles, etc.).  Most of the error codes will be
	 * ignored;; as we find we want specific error messages for particular
	 * errors, we can extend the code as needed.
	 *
	 * We could also check for FNERR_BUFFERTOOSMALL, but we can't
	 * really do anything about it when it happens.
	 */
	if (CommDlgExtendedError() == FNERR_INVALIDFILENAME) {
	    char *p;
	    Tcl_DString ds;

	    Tcl_ExternalToUtfDString(NULL, (char *) ofn.lpstrFile, -1, &ds);
	    for (p = Tcl_DStringValue(&ds); *p != '\0'; p++) {
		/*
		 * Change the pathname to the Tcl "normalized" pathname,
		 * where back slashes are used instead of forward slashes
		 */
		if (*p == '\\') {
		    *p = '/';
		}
	    }
	    Tcl_SetResult(interp, "invalid filename \"", TCL_STATIC);
	    Tcl_AppendResult(interp, Tcl_DStringValue(&ds), "\"", NULL);
	    Tcl_DStringFree(&ds);
	} else {
	    result = TCL_OK;
	}
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
#ifdef _WIN64
	SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
#else
	SetWindowLong(hdlg, GWL_USERDATA, lParam);
#endif
    } else if (uMsg == WM_WINDOWPOSCHANGED) {
	/*
	 * This message is delivered at the right time to both 
	 * old-style and explorer-style hook procs to enable Tk
	 * to set the debug information.  Unhooks itself so it 
	 * won't set the debug information every time it gets a 
	 * WM_WINDOWPOSCHANGED message.
	 */

#ifdef _WIN64
        ofnPtr = (OPENFILENAME *) GetWindowLongPtr(hdlg, GWLP_USERDATA);
#else
        ofnPtr = (OPENFILENAME *) GetWindowLong(hdlg, GWL_USERDATA);
#endif
	if (ofnPtr != NULL) {
	    if (ofnPtr->Flags & OFN_EXPLORER) {
		hdlg = GetParent(hdlg);
	    }
	    tsdPtr->debugInterp = (Tcl_Interp *) ofnPtr->lCustData;
	    Tcl_DoWhenIdle(SetTkDialog, (ClientData) hdlg);
#ifdef _WIN64
	    SetWindowLongPtr(hdlg, GWLP_USERDATA, (LPARAM) NULL);
#else
	    SetWindowLong(hdlg, GWL_USERDATA, (LPARAM) NULL);
#endif
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
	filterStr = ckalloc((unsigned int) strlen(string) * 3);

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

    Tcl_DStringAppend(dsPtr, filterStr, (int) (p - filterStr));
    ckfree((char *) filterStr);

    TkFreeFileFilters(&flist);
    return TCL_OK;
}

#ifdef USE_NEW_CHOOSEDIR
/*
 *----------------------------------------------------------------------
 *
 * Tk_ChooseDirectoryObjCmd --
 *
 * This procedure implements the "tk_chooseDirectory" dialog box
 * for the Windows platform. See the user documentation for details
 * on what it does. Uses the newer SHBrowseForFolder explorer type
 * interface.
 *
 * Results:
 * See user documentation.
 *
 * Side effects:
 * A modal dialog window is created.  Tcl_SetServiceMode() is
 * called to allow background events to be processed
 *
 *----------------------------------------------------------------------

The procedure tk_chooseDirectory pops up a dialog box for the user to
select a directory.  The following option-value pairs are possible as
command line arguments:

-initialdir dirname

Specifies that the directories in directory should be displayed when the
dialog pops up.  If this parameter is not specified, then the directories
in the current working directory are displayed.  If the parameter specifies
a relative path, the return value will convert the relative path to an
absolute path.  This option may not always work on the Macintosh.  This is
not a bug.  Rather, the General Controls control panel on the Mac allows
the end user to override the application default directory.

-parent window

Makes window the logical parent of the dialog.  The dialog is displayed on
top of its parent window.

-title titleString

Specifies a string to display as the title of the dialog box.  If this
option is not specified, then a default title will be displayed.

-mustexist boolean

Specifies whether the user may specify non-existant directories.  If this
parameter is true, then the user may only select directories that already
exist.  The default value is false.

New Behaviour:

- If mustexist = 0 and a user entered folder does not exist, a prompt will
  pop-up asking if the user wants another chance to change it. The old
  dialog just returned the bogus entry. On mustexist = 1, the entries MUST
  exist before exiting the box with OK.

  Bugs:

- If valid abs directory name is entered into the entry box and Enter
  pressed, the box will close returning the name. This is inconsistent when
  entering relative names or names with forward slashes, which are
  invalidated then corrected in the callback. After correction, the box is
  held open to allow further modification by the user.

- Not sure how to implement localization of message prompts.

- -title is really -message.
ToDo:
- Fix bugs.
- test to see what platforms this really works on.  May require v4.71
  of shell32.dll everywhere (what is standard?).
 *
 */
int
Tk_ChooseDirectoryObjCmd(clientData, interp, objc, objv)
    ClientData clientData;      /* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    char path[MAX_PATH];
    int oldMode, result, i;
    LPCITEMIDLIST pidl;		/* Returned by browser */
    BROWSEINFO bInfo;		/* Used by browser */
    CHOOSEDIRDATA cdCBData;	/* Structure to pass back and forth */
    LPMALLOC pMalloc;		/* Used by shell */

    Tk_Window tkwin;
    HWND hWnd;
    char *utfTitle;		/* Title for window */
    TCHAR saveDir[MAX_PATH];
    Tcl_DString titleString;	/* UTF Title */
    Tcl_DString initDirString;	/* Initial directory */
    static CONST char *optionStrings[] = {
        "-initialdir", "-mustexist",  "-parent",  "-title", (char *) NULL
    };
    enum options {
        DIR_INITIAL,   DIR_EXIST,  DIR_PARENT, FILE_TITLE
    };

    /*
     * Initialize
     */
    result		= TCL_ERROR;
    path[0]		= '\0';
    utfTitle		= NULL;

    ZeroMemory(&cdCBData, sizeof(CHOOSEDIRDATA));
    cdCBData.interp	= interp;

    tkwin = (Tk_Window) clientData;
    /*
     * Process the command line options
     */
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

	string = Tcl_GetString(valuePtr);
        switch ((enum options) index) {
            case DIR_INITIAL: {
                if (Tcl_TranslateFileName(interp, string,
			&initDirString) == NULL) {
		    goto cleanup;
		}
                string = Tcl_DStringValue(&initDirString);
                /*
                 * Convert possible relative path to full path to keep
                 * dialog happy
                 */
                GetFullPathName(string, MAX_PATH, saveDir, NULL);
                lstrcpyn(cdCBData.utfInitDir, saveDir, MAX_PATH);
                Tcl_DStringFree(&initDirString);
                break;
            }
            case DIR_EXIST: {
                if (Tcl_GetBooleanFromObj(interp, valuePtr,
                        &cdCBData.mustExist) != TCL_OK) {
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

    /*
     * Get ready to call the browser
     */

    Tk_MakeWindowExist(tkwin);
    hWnd = Tk_GetHWND(Tk_WindowId(tkwin));

    /*
     * Setup the parameters used by SHBrowseForFolder
     */

    bInfo.hwndOwner      = hWnd;
    bInfo.pszDisplayName = path;
    bInfo.pidlRoot       = NULL;
    if (lstrlen(cdCBData.utfInitDir) == 0) {
        GetCurrentDirectory(MAX_PATH, cdCBData.utfInitDir);
    }
    bInfo.lParam = (LPARAM) &cdCBData;

    if (utfTitle != NULL) {
        Tcl_UtfToExternalDString(NULL, utfTitle, -1, &titleString);
        bInfo.lpszTitle = (LPTSTR) Tcl_DStringValue(&titleString);
    } else {
        bInfo.lpszTitle = "Please choose a directory, then select OK.";
    }

    /*
     * Set flags to add edit box (needs 4.71 Shell DLLs), status text line,
     * validate edit box and
     */
    bInfo.ulFlags  =  BIF_EDITBOX | BIF_STATUSTEXT | BIF_RETURNFSANCESTORS
        | BIF_VALIDATE;

    /*
     * Callback to handle events
     */
    bInfo.lpfn     = (BFFCALLBACK) ChooseDirectoryValidateProc;

    /*
     * Display dialog in background and process result.
     * We look to give the user a chance to change their mind
     * on an invalid folder if mustexist is 0;
     */

    oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
    GetCurrentDirectory(MAX_PATH, saveDir);
    if (SHGetMalloc(&pMalloc) == NOERROR) {
	pidl = SHBrowseForFolder(&bInfo);
	/* Null for cancel button or invalid dir, otherwise valid*/
	if (pidl != NULL) {
	    if (!SHGetPathFromIDList(pidl, path)) {
		Tcl_SetResult(interp, "Error: Not a file system folder\n",
			TCL_VOLATILE);
	    };
	    pMalloc->lpVtbl->Free(pMalloc, (void *) pidl);
	} else if (lstrlen(cdCBData.utfRetDir) > 0) {
	    lstrcpy(path, cdCBData.utfRetDir);
	}
	pMalloc->lpVtbl->Release(pMalloc);
    }
    SetCurrentDirectory(saveDir);
    Tcl_SetServiceMode(oldMode);

    /*
     * Ensure that hWnd is enabled, because it can happen that we
     * have updated the wrapper of the parent, which causes us to
     * leave this child disabled (Windows loses sync).
     */
    EnableWindow(hWnd, 1);

    /*
     * Change the pathname to the Tcl "normalized" pathname, where
     * back slashes are used instead of forward slashes
     */
    Tcl_ResetResult(interp);
    if (*path) {
        char *p;
        Tcl_DString ds;

        Tcl_ExternalToUtfDString(NULL, (char *) path, -1, &ds);
        for (p = Tcl_DStringValue(&ds); *p != '\0'; p++) {
            if (*p == '\\') {
                *p = '/';
            }
        }
        Tcl_AppendResult(interp, Tcl_DStringValue(&ds), NULL);
        Tcl_DStringFree(&ds);
    }

    result = TCL_OK;

    if (utfTitle != NULL) {
        Tcl_DStringFree(&titleString);
    }

    cleanup:
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ChooseDirectoryValidateProc --
 *
 * Hook procedure called by the explorer ChooseDirectory dialog when events
 * occur.  It is used to validate the text entry the user may have entered.
 *
 * Results:
 * Returns 0 to allow default processing of message, or 1 to
 * tell default dialog procedure not to close.
 *
 *----------------------------------------------------------------------
 */
static UINT APIENTRY
ChooseDirectoryValidateProc (
    HWND hwnd,
    UINT message,
    LPARAM lParam,
    LPARAM lpData)
{
    TCHAR selDir[MAX_PATH];
    CHOOSEDIRDATA *chooseDirSharedData;
    Tcl_DString initDirString;
    char string[MAX_PATH];
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
        Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    chooseDirSharedData = (CHOOSEDIRDATA *)lpData;

#ifdef _WIN64
    SetWindowLongPtr(hwnd, GWLP_USERDATA, lpData);
#else
    SetWindowLong(hwnd, GWL_USERDATA, lpData);
#endif

    if (tsdPtr->debugFlag) {
        tsdPtr->debugInterp = (Tcl_Interp *) chooseDirSharedData->interp;
        Tcl_DoWhenIdle(SetTkDialog, (ClientData) hwnd);
    }
    chooseDirSharedData->utfRetDir[0] = '\0';
    switch (message) {
        case BFFM_VALIDATEFAILED:
            /*
             * First save and check to see if it is a valid path name, if
             * so then make that path the one shown in the
             * window. Otherwise, it failed the check and should be treated
             * as such. Use Set/GetCurrentDirectory which allows relative
             * path names and names with forward slashes. Use
             * Tcl_TranslateFileName to make sure names like ~ are
             * converted correctly.
             */
            Tcl_TranslateFileName(chooseDirSharedData->interp,
                    (char *)lParam, &initDirString);
            lstrcpyn (string, Tcl_DStringValue(&initDirString), MAX_PATH);
            Tcl_DStringFree(&initDirString);

            if (SetCurrentDirectory((char *)string) == 0) {
                LPTSTR lpFilePart[MAX_PATH];
                /*
                 * Get the full path name to the user entry,
                 * at this point it doesn't exist so see if
                 * it is supposed to. Otherwise just return it.
                 */
                GetFullPathName(string, MAX_PATH,
			chooseDirSharedData->utfRetDir, /*unused*/ lpFilePart);
                if (chooseDirSharedData->mustExist) {
                    /*
                     * User HAS to select a valid directory.
                     */
                    wsprintf(selDir, TEXT("Directory '%.200s' does not exist,\nplease select or enter an existing directory."), chooseDirSharedData->utfRetDir);
                    MessageBox(NULL, selDir, NULL, MB_ICONEXCLAMATION|MB_OK);
                    return 1;
                }
            } else {
                /*
                 * Changed to new folder OK, return immediatly with the
                 * current directory in utfRetDir.
                 */
                GetCurrentDirectory(MAX_PATH, chooseDirSharedData->utfRetDir);
                return 0;
            }
            return 0;

        case BFFM_SELCHANGED:
            /*
             * Set the status window to the currently selected path.
             * And enable the OK button if a file system folder, otherwise
             * disable the OK button for things like server names.
             * perhaps a new switch -enablenonfolders can be used to allow
             * non folders to be selected.
             *
             * Not called when user changes edit box directly.
             */

            if (SHGetPathFromIDList((LPITEMIDLIST) lParam, selDir)) {
                SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM) selDir);
                // enable the OK button
                SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM) 1);
                //EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
                SetCurrentDirectory(selDir);
            } else {
                // disable the OK button
                SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM) 0);
                //EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
            }
            UpdateWindow(hwnd);
            return 1;

        case BFFM_INITIALIZED:
            /*
             * Directory browser intializing - tell it where to start from,
             * user specified parameter.
             */
            SetCurrentDirectory((char *) lpData);
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpData);
            SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM) 1);
            break;

    }
    return 0;
}
#else
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
    HWND hWnd;
    char *utfTitle;
    Tcl_DString utfDirString;
    Tcl_DString titleString, dirString;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    static CONST char *optionStrings[] = {
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
    hWnd = Tk_GetHWND(Tk_WindowId(tkwin));

    cd.interp = interp;
    cd.ofnPtr = &ofn;

    ofn.lStructSize		= sizeof(ofn);
    ofn.hwndOwner		= hWnd;
#ifdef _WIN64
    ofn.hInstance		= (HINSTANCE) GetWindowLongPtr(ofn.hwndOwner, 
					GWLP_HINSTANCE);
#else
    ofn.hInstance		= (HINSTANCE) GetWindowLong(ofn.hwndOwner, 
					GWL_HINSTANCE);
#endif
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
    ofn.lpfnHook		= (LPOFNHOOKPROC) ChooseDirectoryHookProc;
    ofn.lpTemplateName		= MAKEINTRESOURCE(FILEOPENORD);

    if (Tcl_DStringValue(&utfDirString)[0] != '\0') {
	Tcl_UtfToExternalDString(NULL, Tcl_DStringValue(&utfDirString), 
		Tcl_DStringLength(&utfDirString), &dirString);
    } else {
	/*
	 * NT 5.0 changed the meaning of lpstrInitialDir, so we have
	 * to ensure that we set the [pwd] if the user didn't specify
	 * anything else.
	 */
	Tcl_DString cwd;

	Tcl_DStringFree(&utfDirString);
	if ((Tcl_GetCwd(interp, &utfDirString) == (char *) NULL) ||
		(Tcl_TranslateFileName(interp,
			Tcl_DStringValue(&utfDirString), &cwd) == NULL)) {
	    Tcl_ResetResult(interp);
	} else {
	    Tcl_UtfToExternalDString(NULL, Tcl_DStringValue(&cwd),
		    Tcl_DStringLength(&cwd), &dirString);
	}
	Tcl_DStringFree(&cwd);
    }
    ofn.lpstrInitialDir = (LPTSTR) Tcl_DStringValue(&dirString);

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

    /*
     * Ensure that hWnd is enabled, because it can happen that we
     * have updated the wrapper of the parent, which causes us to
     * leave this child disabled (Windows loses sync).
     */
    EnableWindow(hWnd, 1);

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
    ChooseDir *cdPtr;

    if (message == WM_INITDIALOG) {
	ofnPtr = (OPENFILENAME *) lParam;
	cdPtr = (ChooseDir *) ofnPtr->lCustData;
	cdPtr->lastCtrl = 0;
	cdPtr->lastIdx = 1000;
	cdPtr->path[0] = '\0';
#ifdef _WIN64
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) cdPtr);
#else
	SetWindowLong(hwnd, GWL_USERDATA, (LONG) cdPtr);
#endif

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

    /*
     * GWL_USERDATA keeps track of cdPtr.
     */
    
#ifdef _WIN64
    cdPtr = (ChooseDir *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
    cdPtr = (ChooseDir *) GetWindowLong(hwnd, GWL_USERDATA);
#endif
    if (cdPtr == NULL) {
	return 0;
    }
    ofnPtr = cdPtr->ofnPtr;

    if (message == tsdPtr->WM_LBSELCHANGED) {
	/*
	 * Called when double-clicking on directory.
	 * If directory wasn't already open, browse that directory.
	 * If directory was already open, return selected directory.
	 */

	int idCtrl, thisItem;

	idCtrl = (int) wParam;
        thisItem = LOWORD(lParam);

	GetCurrentDirectory(MAX_PATH, cdPtr->path);
	if (idCtrl == lst2) {
	    if (cdPtr->lastIdx == thisItem) {
		EndDialog(hwnd, IDOK);
		return 1;
	    }
	    cdPtr->lastIdx = thisItem;
	}
	SetDlgItemText(hwnd, edt10, cdPtr->path);
	SendDlgItemMessage(hwnd, edt10, EM_SETSEL, 0, -1);
    } else if (message == WM_COMMAND) {
	int idCtrl, notifyCode;

	idCtrl = LOWORD(wParam);
	notifyCode = HIWORD(wParam);

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
	     * The OK button was clicked. Return the value currently selected
             * in the entry.
	     */

	    GetCurrentDirectory(MAX_PATH, cdPtr->path);
	    EndDialog(hwnd, IDOK);
	    return 1;
	}
    }
    return 0;
}
#endif

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
    int i, oldMode, winCode;
    UINT flags;
    Tcl_DString messageString, titleString;
    Tcl_Encoding unicodeEncoding = TkWinGetUnicodeEncoding();
    static CONST char *optionStrings[] = {
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

    Tcl_UtfToExternalDString(unicodeEncoding, message, -1, &messageString);
    Tcl_UtfToExternalDString(unicodeEncoding, title, -1, &titleString);

    oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
    /*
     * MessageBoxW exists for all platforms.  Use it to allow unicode
     * error message to be displayed correctly where possible by the OS.
     */
    winCode = MessageBoxW(hWnd, (WCHAR *) Tcl_DStringValue(&messageString),
		(WCHAR *) Tcl_DStringValue(&titleString), flags);
    (void) Tcl_SetServiceMode(oldMode);

    /*
     * Ensure that hWnd is enabled, because it can happen that we
     * have updated the wrapper of the parent, which causes us to
     * leave this child disabled (Windows loses sync).
     */
    EnableWindow(hWnd, 1);

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

    sprintf(buf, "0x%p", (HWND) clientData);
    Tcl_SetVar(tsdPtr->debugInterp, "tk_dialog", buf, TCL_GLOBAL_ONLY);
}
