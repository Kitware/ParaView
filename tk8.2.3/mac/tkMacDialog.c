/*
 * tkMacDialog.c --
 *
 *	Contains the Mac implementation of the common dialog boxes.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <Gestalt.h>
#include <Aliases.h>
#include <Errors.h>
#include <Strings.h>
#include <MoreFiles.h>
#include <MoreFilesExtras.h>
#include <StandardFile.h>
#include <ColorPicker.h>
#include <Lowmem.h>
#include "tkPort.h"
#include "tkInt.h"
#include "tclMacInt.h"
#include "tkMacInt.h"
#include "tkFileFilter.h"

#ifndef StrLength
#define StrLength(s) 		(*((unsigned char *) (s)))
#endif
#ifndef StrBody
#define StrBody(s)		((char *) (s) + 1)
#endif

/*
 * The following are ID's for resources that are defined in tkMacResource.r
 */
#define OPEN_BOX        130
#define OPEN_POPUP      131
#define OPEN_MENU       132
#define OPEN_POPUP_ITEM 10

#define SAVE_FILE	0
#define OPEN_FILE	1

#define MATCHED		0
#define UNMATCHED	1

/*
 * The following structure is used in the GetFileName() function. It stored
 * information about the file dialog and the file filters.
 */
typedef struct _OpenFileData {
    FileFilterList fl;			/* List of file filters. */
    SInt16 curType;			/* The filetype currently being
					 * listed. */
    short popupItem;			/* Item number of the popup in the
					 * dialog. */
    int usePopup;			/* True if we show the popup menu (this
    					 * is an open operation and the
					 * -filetypes option is set). */
} OpenFileData;

static pascal Boolean	FileFilterProc _ANSI_ARGS_((CInfoPBPtr pb,
			    void *myData));
static int 		GetFileName _ANSI_ARGS_ ((ClientData clientData, 
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[], int isOpen));
static Boolean		MatchOneType _ANSI_ARGS_((CInfoPBPtr pb,
			    OpenFileData *myofdPtr, FileFilter *filterPtr));
static pascal short 	OpenHookProc _ANSI_ARGS_((short item,
			    DialogPtr theDialog, OpenFileData * myofdPtr));
static int 		ParseFileDlgArgs _ANSI_ARGS_ ((Tcl_Interp * interp,
			    OpenFileData * myofdPtr, int argc, char ** argv,
			    int isOpen));

/*
 * Filter and hook functions used by the tk_getOpenFile and tk_getSaveFile
 * commands.
 */

static FileFilterYDUPP openFilter = NULL;
static DlgHookYDUPP openHook = NULL;
static DlgHookYDUPP saveHook = NULL;
  

/*
 *----------------------------------------------------------------------
 *
 * Tk_ChooseColorObjCmd --
 *
 *	This procedure implements the color dialog box for the Mac
 *	platform. See the user documentation for details on what it
 *	does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_ChooseColorObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST objv[])	/* Argument objects. */
{
    Tk_Window parent;
    char *title;
    int i, picked, srcRead, dstWrote;
    long response;
    OSErr err;
    static inited = 0;
    static RGBColor in;
    static char *optionStrings[] = {
	"-initialcolor",    "-parent",	    "-title",	    NULL
    };
    enum options {
	COLOR_INITIAL,	    COLOR_PARENT,   COLOR_TITLE
    };

    if (inited == 0) {
    	/*
    	 * 'in' stores the last color picked.  The next time the color dialog
    	 * pops up, the last color will remain in the dialog.
    	 */
    	 
        in.red = 0xffff;
        in.green = 0xffff;
        in.blue = 0xffff;
        inited = 1;
    }
    
    parent = (Tk_Window) clientData;
    title = "Choose a color:";
    picked = 0;
        
    for (i = 1; i < objc; i += 2) {
    	int index;
    	char *option, *value;
    	
        if (Tcl_GetIndexFromObj(interp, objv[i], optionStrings, "option",
		TCL_EXACT, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (i + 1 == objc) {
	    option = Tcl_GetStringFromObj(objv[i], NULL);
	    Tcl_AppendResult(interp, "value for \"", option, "\" missing", 
		    (char *) NULL);
	    return TCL_ERROR;
	}
	value = Tcl_GetStringFromObj(objv[i + 1], NULL);
	
	switch ((enum options) index) {
	    case COLOR_INITIAL: {
		XColor *colorPtr;

		colorPtr = Tk_GetColor(interp, parent, value);
		if (colorPtr == NULL) {
		    return TCL_ERROR;
		}
		in.red   = colorPtr->red;
		in.green = colorPtr->green;
                in.blue  = colorPtr->blue;
                Tk_FreeColor(colorPtr);
		break;
	    }
	    case COLOR_PARENT: {
		parent = Tk_NameToWindow(interp, value, parent);
		if (parent == NULL) {
		    return TCL_ERROR;
		}
		break;
	    }
	    case COLOR_TITLE: {
	        title = value;
		break;
	    }
	}
    }
        
    /*
     * Use the gestalt manager to determine how to bring
     * up the color picker.  If versin 2.0 isn't available
     * we can assume version 1.0 is available as it comes with
     * Color Quickdraw which Tk requires to run at all.
     */
     
    err = Gestalt(gestaltColorPicker, &response); 
    if ((err == noErr) && (response == 0x0200L)) {
	ColorPickerInfo cpinfo;

        /*
         * Version 2.0 of the color picker is available. Let's use it
         */

    	cpinfo.theColor.profile = 0L;
    	cpinfo.theColor.color.rgb.red   = in.red;
    	cpinfo.theColor.color.rgb.green = in.green;
    	cpinfo.theColor.color.rgb.blue  = in.blue;
    	cpinfo.dstProfile = 0L;
    	cpinfo.flags = CanModifyPalette | CanAnimatePalette;
    	cpinfo.placeWhere = kDeepestColorScreen;
    	cpinfo.pickerType = 0L;
    	cpinfo.eventProc = NULL;
    	cpinfo.colorProc = NULL;
    	cpinfo.colorProcData = NULL;
    	
    	Tcl_UtfToExternal(NULL, NULL, title, -1, 0, NULL, 
		StrBody(cpinfo.prompt), 255, &srcRead, &dstWrote, NULL);
    	StrLength(cpinfo.prompt) = (unsigned char) dstWrote;

        if ((PickColor(&cpinfo) == noErr) && (cpinfo.newColorChosen != 0)) {
            in.red 	= cpinfo.theColor.color.rgb.red;
            in.green 	= cpinfo.theColor.color.rgb.green;
            in.blue 	= cpinfo.theColor.color.rgb.blue;
            picked = 1;
        }
    } else {
    	RGBColor out;
    	Str255 prompt;
    	Point point = {-1, -1};
    	
        /*
         * Use version 1.0 of the color picker
         */
    	
    	Tcl_UtfToExternal(NULL, NULL, title, -1, 0, NULL, StrBody(prompt), 
		255, &srcRead, &dstWrote, NULL);
    	StrLength(prompt) = (unsigned char) dstWrote;

        if (GetColor(point, prompt, &in, &out)) {
            in = out;
            picked = 1;
        }
    } 
    
    if (picked != 0) {
        char result[32];

        sprintf(result, "#%02x%02x%02x", in.red >> 8, in.green >> 8, 
        	in.blue >> 8);
	Tcl_AppendResult(interp, result, NULL);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetOpenFileObjCmd --
 *
 *	This procedure implements the "open file" dialog box for the
 *	Mac platform. See the user documentation for details on what
 *	it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *	See user documentation.
 *----------------------------------------------------------------------
 */

int
Tk_GetOpenFileObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST objv[])	/* Argument objects. */
{
    return GetFileName(clientData, interp, objc, objv, OPEN_FILE);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetSaveFileObjCmd --
 *
 *	Same as Tk_GetOpenFileCmd but opens a "save file" dialog box
 *	instead
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *	See user documentation.
 *----------------------------------------------------------------------
 */

int
Tk_GetSaveFileObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST objv[])	/* Argument objects. */
{
    return GetFileName(clientData, interp, objc, objv, SAVE_FILE);
}

/*
 *----------------------------------------------------------------------
 *
 * GetFileName --
 *
 *	Calls the Mac file dialog functions for the user to choose a
 *	file to or save.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	If the user selects a file, the native pathname of the file
 *	is returned in the interp's result. Otherwise an empty string
 *	is returned in the interp's result.
 *
 *----------------------------------------------------------------------
 */

static int
GetFileName(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST objv[],	/* Argument objects. */
    int isOpen)			/* true if we should call GetOpenFileName(),
				 * false if we should call GetSaveFileName() */
{
    int i, result;
    OpenFileData ofd;
    StandardFileReply reply;
    Point mypoint;
    MenuHandle menu;
    Str255 initialFile;
    char *choice[6];
    Tk_Window parent;
    static char *optionStrings[] = {
	"-defaultextension", "-filetypes", "-initialdir", "-initialfile",
	"-parent",	"-title",	NULL
    };
    enum options {
	FILE_DEFAULT,	FILE_TYPES,	FILE_INITDIR,	FILE_INITFILE,
	FILE_PARENT,	FILE_TITLE
    };

    if (openFilter == NULL) {
	openFilter = NewFileFilterYDProc(FileFilterProc);
	openHook = NewDlgHookYDProc(OpenHookProc);
	saveHook = NewDlgHookYDProc(OpenHookProc);
    }
    
    result = TCL_ERROR;    
    parent = (Tk_Window) clientData;    
    memset(choice, 0, sizeof(choice));

    for (i = 1; i < objc; i += 2) {
	int index;
	char *string;

	if (Tcl_GetIndexFromObj(interp, objv[i], optionStrings, "option",
		TCL_EXACT, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (i + 1 == objc) {
	    string = Tcl_GetStringFromObj(objv[i], NULL);
	    Tcl_AppendResult(interp, "value for \"", string, "\" missing", 
		    (char *) NULL);
	    return TCL_ERROR;
	}
	choice[index] = Tcl_GetStringFromObj(objv[i + 1], NULL);
    }
    
    StrLength(initialFile) = 0;
    menu = NULL;
    
    TkInitFileFilters(&ofd.fl);
    ofd.curType		= 0;
    ofd.popupItem	= OPEN_POPUP_ITEM;
    ofd.usePopup 	= isOpen;
    
    if (choice[FILE_TYPES] != NULL) {
        if (TkGetFileFilters(interp, &ofd.fl, choice[FILE_TYPES], 0) != TCL_OK) {
            goto end;
        }
    }
    if (choice[FILE_INITDIR] != NULL) {
        FSSpec dirSpec;
	Tcl_DString ds;
	long dirID;
	OSErr err;
	Boolean isDirectory;
	char *string;
	Str255 dir;
	int srcRead, dstWrote;
	
	string = choice[FILE_INITDIR];
	if (Tcl_TranslateFileName(interp, string, &ds) == NULL) {
	    goto end;
	}
	Tcl_UtfToExternal(NULL, NULL, Tcl_DStringValue(&ds), 
		Tcl_DStringLength(&ds), 0, NULL, StrBody(dir), 255, 
		&srcRead, &dstWrote, NULL);
        StrLength(dir) = (unsigned char) dstWrote;
	Tcl_DStringFree(&ds);
          
	err = FSpLocationFromPath(StrLength(dir), StrBody(dir), &dirSpec);
	if (err != noErr) {
	    Tcl_AppendResult(interp, "bad directory \"", string, "\"", NULL);
	    goto end;
	}
	err = FSpGetDirectoryID(&dirSpec, &dirID, &isDirectory);
	if ((err != noErr) || !isDirectory) {
	    Tcl_AppendResult(interp, "bad directory \"", string, "\"", NULL);
	    goto end;
	}
	/*
	 * Make sure you negate -dirSpec.vRefNum because the 
	 * standard file package wants it that way !
	 */
	
	LMSetSFSaveDisk(-dirSpec.vRefNum);
	LMSetCurDirStore(dirID);
    }
    if (choice[FILE_INITFILE] != NULL) {
        Tcl_DString ds;
        int srcRead, dstWrote;
        
        if (Tcl_TranslateFileName(interp, choice[FILE_INITFILE], &ds) == NULL) {
            goto end;
        }
        Tcl_UtfToExternal(NULL, NULL, Tcl_DStringValue(&ds), 
        	Tcl_DStringLength(&ds), 0, NULL, 
		StrBody(initialFile), 255, &srcRead, &dstWrote, NULL);
        StrLength(initialFile) = (unsigned char) dstWrote;
        Tcl_DStringFree(&ds);
    }
    if (choice[FILE_PARENT] != NULL) {
        parent = Tk_NameToWindow(interp, choice[FILE_PARENT], parent);
	if (parent == NULL) {
	    return TCL_ERROR;
	}
    }

    /*
     * 2. Set the items in the file types popup.
     */

    /*
     * Delete all the entries inside the popup menu, in case there's any
     * left overs from previous invocation of this command
     */

    if (ofd.usePopup) {
	FileFilter *filterPtr;
	
	menu = GetMenu(OPEN_MENU);
        for (i = CountMItems(menu); i > 0; i--) {
            /*
             * The item indices are one based. Also, if we delete from
             * the beginning, the items may be re-numbered. So we
             * delete from the end
    	     */
    	     
    	     DeleteMenuItem(menu, i);
        }

	filterPtr = ofd.fl.filters;
	if (filterPtr == NULL) {
	    ofd.usePopup = 0;
	} else {
	    for ( ; filterPtr != NULL; filterPtr = filterPtr->next) {
	        Str255 str;
	        
	    	StrLength(str) = (unsigned char) strlen(filterPtr->name);
	    	strcpy(StrBody(str), filterPtr->name);
		AppendMenu(menu, str);
	    }
	}
    }

    /*
     * 3. Call the toolbox file dialog function.
     */
     
    SetPt(&mypoint, -1, -1);
    TkpSetCursor(NULL);
    if (isOpen) {
        if (ofd.usePopup) {
	    CustomGetFile(openFilter, (short) -1, NULL, &reply, OPEN_BOX,
	    	    mypoint, openHook, NULL, NULL, NULL, (void*) &ofd);
	} else {
	    StandardGetFile(NULL, -1, NULL, &reply);
	}
    } else {
	static Str255 prompt = "\pSave as";
	
   	if (ofd.usePopup) {
   	    /*
   	     * Currently this never gets called because we don't use
   	     * popup for the save dialog.
   	     */
	    CustomPutFile(prompt, initialFile, &reply, OPEN_BOX, 
	    	    mypoint, saveHook, NULL, NULL, NULL, (void *) &ofd);
	} else {
	    StandardPutFile(prompt, initialFile, &reply);
	}
    }

    if (reply.sfGood) {
        int length;
    	Handle pathHandle;
    	
    	pathHandle = NULL;
    	FSpPathFromLocation(&reply.sfFile, &length, &pathHandle);
	if (pathHandle != NULL) {
	    Tcl_DString ds;
	    
	    HLock(pathHandle);
	    Tcl_ExternalToUtfDString(NULL, (char *) *pathHandle, -1, &ds);
	    Tcl_AppendResult(interp, Tcl_DStringValue(&ds), NULL);
	    Tcl_DStringFree(&ds);
	    HUnlock(pathHandle);
	    DisposeHandle(pathHandle);
	}
    }
    
    result = TCL_OK;

    end:
    TkFreeFileFilters(&ofd.fl);
    if (menu != NULL) {
    	DisposeMenu(menu);
    }
    return result;
}
/*
 *----------------------------------------------------------------------
 *
 * OpenHookProc --
 *
 *	Gets called for various events that occur in the file dialog box.
 *	Initializes the popup menu or rebuild the file list depending on
 *	the type of the event.
 *
 * Results:
 *	A standard result understood by the Mac file dialog event dispatcher.
 *
 * Side effects:
 *	The contents in the file dialog may be changed depending on
 *	the type of the event.
 *----------------------------------------------------------------------
 */

static pascal short
OpenHookProc(
    short item,			/* Event description. */
    DialogPtr theDialog,	/* The dialog where the event occurs. */
    OpenFileData *ofdPtr)	/* Information about the file dialog. */
{
    short ignore;
    Rect rect;
    Handle handle;
    int newType;

    switch (item) {
	case sfHookFirstCall:
	    if (ofdPtr->usePopup) {
		/*
		 * Set the popup list to display the selected type.
		 */
		GetDialogItem(theDialog, ofdPtr->popupItem, &ignore, &handle, 
			&rect);
		SetControlValue((ControlRef) handle, ofdPtr->curType + 1);
	    }
	    return sfHookNullEvent;
      
	case OPEN_POPUP_ITEM:
	    if (ofdPtr->usePopup) {
		GetDialogItem(theDialog, ofdPtr->popupItem,
			&ignore, &handle, &rect);
		newType = GetCtlValue((ControlRef) handle) - 1;
		if (ofdPtr->curType != newType) {
		    if (newType<0 || newType>ofdPtr->fl.numFilters) {
			/*
			 * Sanity check. Looks like the user selected an
			 * non-existent menu item?? Don't do anything.
			 */
		    } else {
			ofdPtr->curType = newType;
		    }
		    return sfHookRebuildList;
		}
	    }  
	    break;
    }

    return item;
}

/*
 *----------------------------------------------------------------------
 *
 * FileFilterProc --
 *
 *	Filters files according to file types. Get called whenever the
 *	file list needs to be updated inside the dialog box.
 *
 * Results:
 *	Returns MATCHED if the file should be shown in the listbox, returns
 *	UNMATCHED otherwise.
 *
 * Side effects:
 *	If MATCHED is returned, the file is shown in the listbox.
 *
 *----------------------------------------------------------------------
 */

static pascal Boolean
FileFilterProc(
    CInfoPBPtr pb,		/* Information about the file */
    void *myData)		/* Client data for this file dialog */
{
    int i;
    OpenFileData * ofdPtr = (OpenFileData*)myData;
    FileFilter * filterPtr;

    if (ofdPtr->fl.numFilters == 0) {
	/*
	 * No types have been specified. List all files by default
	 */
	return MATCHED;
    }

    if (pb->dirInfo.ioFlAttrib & 0x10) {
    	/*
    	 * This is a directory: always show it
    	 */
    	return MATCHED;
    }

    if (ofdPtr->usePopup) {
        i = ofdPtr->curType;
	for (filterPtr=ofdPtr->fl.filters; filterPtr && i>0; i--) {
	    filterPtr = filterPtr->next;
	}
	if (filterPtr) {
	    return MatchOneType(pb, ofdPtr, filterPtr);
	} else {
	    return UNMATCHED;
        }
    } else {
	/*
	 * We are not using the popup menu. In this case, the file is
	 * considered matched if it matches any of the file filters.
	 */

	for (filterPtr=ofdPtr->fl.filters; filterPtr;
		filterPtr=filterPtr->next) {
	    if (MatchOneType(pb, ofdPtr, filterPtr) == MATCHED) {
	        return MATCHED;
	    }
	}
	return UNMATCHED;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MatchOneType --
 *
 *	Match a file with one file type in the list of file types.
 *
 * Results:
 *	Returns MATCHED if the file matches with the file type; returns
 *	UNMATCHED otherwise.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static Boolean
MatchOneType(
    CInfoPBPtr pb,		/* Information about the file */
    OpenFileData * ofdPtr,	/* Information about this file dialog */
    FileFilter * filterPtr)	/* Match the file described by pb against
				 * this filter */
{
    FileFilterClause * clausePtr;

    /*
     * A file matches with a file type if it matches with at least one
     * clause of the type.
     *
     * If the clause has both glob patterns and ostypes, the file must
     * match with at least one pattern AND at least one ostype.
     *
     * If the clause has glob patterns only, the file must match with at least
     * one pattern.
     *
     * If the clause has mac types only, the file must match with at least
     * one mac type.
     *
     * If the clause has neither glob patterns nor mac types, it's
     * considered an error.
     */

    for (clausePtr=filterPtr->clauses; clausePtr; clausePtr=clausePtr->next) {
	int macMatched  = 0;
	int globMatched = 0;
	GlobPattern * globPtr;
	MacFileType * mfPtr;

	if (clausePtr->patterns == NULL) {
	    globMatched = 1;
	}
	if (clausePtr->macTypes == NULL) {
	    macMatched = 1;
	}

	for (globPtr=clausePtr->patterns; globPtr; globPtr=globPtr->next) {
	    char filename[256];
	    int len;
	    char * p, *q, *ext;
        
	    if (pb->hFileInfo.ioNamePtr == NULL) {
		continue;
	    }
	    p = (char*)(pb->hFileInfo.ioNamePtr);
	    len = p[0];
	    strncpy(filename, p+1, len);
	    filename[len] = '\0';
	    ext = globPtr->pattern;

	    if (ext[0] == '\0') {
		/*
		 * We don't want any extensions: OK if the filename doesn't
		 * have "." in it
		 */
		for (q=filename; *q; q++) {
		    if (*q == '.') {
			goto glob_unmatched;
		    }
		}
		goto glob_matched;
	    }
        
	    if (Tcl_StringMatch(filename, ext)) {
		goto glob_matched;
	    } else {
		goto glob_unmatched;
	    }

	  glob_unmatched:
	    continue;

	  glob_matched:
	    globMatched = 1;
	    break;
	}

	for (mfPtr=clausePtr->macTypes; mfPtr; mfPtr=mfPtr->next) {
	    if (pb->hFileInfo.ioFlFndrInfo.fdType == mfPtr->type) {
		macMatched = 1;
		break;
	    }
        }

	if (globMatched && macMatched) {
	    return MATCHED;
	}
    }

    return UNMATCHED;
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
    return TCL_ERROR;
}


