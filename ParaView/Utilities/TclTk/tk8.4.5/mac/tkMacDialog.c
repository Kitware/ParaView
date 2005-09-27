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
#include <Navigation.h>
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
#define CHOOSE_FOLDER   2

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
static int 		NavGetFileName _ANSI_ARGS_ ((ClientData clientData, 
			    Tcl_Interp *interp, int objc, 
			    Tcl_Obj *CONST objv[], int isOpen));
static Boolean		MatchOneType _ANSI_ARGS_((StringPtr fileNamePtr, OSType fileType,
			    OpenFileData *myofdPtr, FileFilter *filterPtr));
static pascal short 	OpenHookProc _ANSI_ARGS_((short item,
			    DialogPtr theDialog, OpenFileData * myofdPtr));
static int 		ParseFileDlgArgs _ANSI_ARGS_ ((Tcl_Interp * interp,
			    OpenFileData * myofdPtr, int argc, char ** argv,
			    int isOpen));
static pascal Boolean   OpenFileFilterProc(AEDesc* theItem, void* info, 
                            NavCallBackUserData callBackUD,
                            NavFilterModes filterMode );
pascal void             OpenEventProc(NavEventCallbackMessage callBackSelector,
                            NavCBRecPtr callBackParms,
                            NavCallBackUserData callBackUD );
static void             InitFileDialogs();
static int              StdGetFile(Tcl_Interp *interp, OpenFileData *ofd,
                            unsigned char *initialFile, int isOpen);
static int              NavServicesGetFile(Tcl_Interp *interp, OpenFileData *ofd,
                            AEDesc *initialDesc, unsigned char *initialFile,
                            StringPtr title, StringPtr message, int multiple, int isOpen);
static int              HandleInitialDirectory (Tcl_Interp *interp, char *initialDir, FSSpec *dirSpec, 
                            AEDesc *dirDescPtr);                            
/*
 * Filter and hook functions used by the tk_getOpenFile and tk_getSaveFile
 * commands.
 */

int fileDlgInited = 0;
int useNavServices = 0;
NavObjectFilterUPP openFileFilterUPP;
NavEventUPP openFileEventUPP;

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
    static CONST char *optionStrings[] = {
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
    	cpinfo.flags = kColorPickerCanModifyPalette | kColorPickerCanAnimatePalette;
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
    int i, result, multiple;
    OpenFileData ofd;
    Tk_Window parent;
    Str255 message, title;
    AEDesc initialDesc = {typeNull, NULL};
    FSSpec dirSpec;
    static CONST char *openOptionStrings[] = {
	    "-defaultextension", "-filetypes", 
	    "-initialdir", "-initialfile", 
	    "-message", "-multiple",
	    "-parent",	"-title", 	NULL
    };
    enum openOptions {
	    OPEN_DEFAULT, OPEN_TYPES,	
	    OPEN_INITDIR, OPEN_INITFILE,
	    OPEN_MESSAGE, OPEN_MULTIPLE, 
	    OPEN_PARENT, OPEN_TITLE
    };
    
    if (!fileDlgInited) {
	InitFileDialogs();
    }
    
    result = TCL_ERROR;    
    parent = (Tk_Window) clientData; 
    multiple = false;
    title[0] = 0;
    message[0] = 0;   

    TkInitFileFilters(&ofd.fl);
    
    ofd.curType		= 0;
    ofd.popupItem	= OPEN_POPUP_ITEM;
    ofd.usePopup 	= 1;

    for (i = 1; i < objc; i += 2) {
        char *choice;
	int index, choiceLen;
	char *string;
	int srcRead, dstWrote;

	if (Tcl_GetIndexFromObj(interp, objv[i], openOptionStrings, "option",
		TCL_EXACT, &index) != TCL_OK) {
	    result = TCL_ERROR;
	    goto end;
	}
	if (i + 1 == objc) {
	    string = Tcl_GetStringFromObj(objv[i], NULL);
	    Tcl_AppendResult(interp, "value for \"", string, "\" missing", 
		    (char *) NULL);
	    result = TCL_ERROR;
	    goto end;
	}
	
	switch (index) {
	    case OPEN_DEFAULT:
	        break;
	    case OPEN_TYPES:
	        choice = Tcl_GetStringFromObj(objv[i + 1], NULL);
                if (TkGetFileFilters(interp, &ofd.fl, choice, 0) 
                        != TCL_OK) {
                    result = TCL_ERROR;
                    goto end;
                }
	        break;
	    case OPEN_INITDIR:
	        choice = Tcl_GetStringFromObj(objv[i + 1], NULL);
                if (HandleInitialDirectory(interp, choice, &dirSpec, 
                        &initialDesc) != TCL_OK) {
                    result = TCL_ERROR;
                    goto end;
                }
	        break;
	    case OPEN_INITFILE:
	        break;
	    case OPEN_MESSAGE:
	        choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
	        Tcl_UtfToExternal(NULL, NULL, choice, choiceLen, 
		        0, NULL, StrBody(message), 255, 
		        &srcRead, &dstWrote, NULL);
                message[0] = dstWrote;
	        break;
	    case OPEN_MULTIPLE:
	        if (Tcl_GetBooleanFromObj(interp, objv[i + 1], &multiple) != TCL_OK) {
	            result = TCL_ERROR;
	            goto end;
	        }
	        break;
	    case OPEN_PARENT:
	        choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
                parent = Tk_NameToWindow(interp, choice, parent);
	        if (parent == NULL) {
	            result = TCL_ERROR;
	            goto end;
	        }
	        break;
	    case OPEN_TITLE:
	        choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
	        Tcl_UtfToExternal(NULL, NULL, choice, choiceLen, 
		        0, NULL, StrBody(title), 255, 
		        &srcRead, &dstWrote, NULL);
                title[0] = dstWrote;
	        break;
	}
    }
             
    if (useNavServices) {
        AEDesc *initialPtr = NULL;
        
        if (initialDesc.descriptorType == typeFSS) {
            initialPtr = &initialDesc;
        }
        result = NavServicesGetFile(interp, &ofd, initialPtr, NULL, 
                title, message, multiple, OPEN_FILE);
    } else {
        result = StdGetFile(interp, &ofd, NULL, OPEN_FILE);
    }

    end:
    TkFreeFileFilters(&ofd.fl);
    AEDisposeDesc(&initialDesc);
    
    return result;
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
    int i, result;
    Str255 initialFile;
    Tk_Window parent;
    AEDesc initialDesc = {typeNull, NULL};
    FSSpec dirSpec;
    Str255 title, message;
    OpenFileData ofd;
    static CONST char *saveOptionStrings[] = {
	    "-defaultextension", "-filetypes", "-initialdir", "-initialfile", 
	    "-message", "-parent",	"-title", 	NULL
    };
    enum saveOptions {
	    SAVE_DEFAULT,	SAVE_TYPES,	SAVE_INITDIR,	SAVE_INITFILE,
	    SAVE_MESSAGE,	SAVE_PARENT,	SAVE_TITLE
    };

    if (!fileDlgInited) {
	InitFileDialogs();
    }
    
    result = TCL_ERROR;    
    parent = (Tk_Window) clientData;    
    StrLength(initialFile) = 0;
    title[0] = 0;
    message[0] = 0;   
    

    for (i = 1; i < objc; i += 2) {
        char *choice;
	int index, choiceLen;
	char *string;
        Tcl_DString ds;
        int srcRead, dstWrote;

	if (Tcl_GetIndexFromObj(interp, objv[i], saveOptionStrings, "option",
		TCL_EXACT, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (i + 1 == objc) {
	    string = Tcl_GetStringFromObj(objv[i], NULL);
	    Tcl_AppendResult(interp, "value for \"", string, "\" missing", 
		    (char *) NULL);
	    return TCL_ERROR;
	}
	switch (index) {
	    case SAVE_DEFAULT:
	        break;
	    case SAVE_TYPES:
	        break;
	    case SAVE_INITDIR:
	        choice = Tcl_GetStringFromObj(objv[i + 1], NULL);
                if (HandleInitialDirectory(interp, choice, &dirSpec, 
                        &initialDesc) != TCL_OK) {
                    result = TCL_ERROR;
                    goto end;
                }
	        break;
	    case SAVE_INITFILE:
	        choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
                if (Tcl_TranslateFileName(interp, choice, &ds) == NULL) {
                    result = TCL_ERROR;
                    goto end;
                }
                Tcl_UtfToExternal(NULL, NULL, Tcl_DStringValue(&ds), 
        	        Tcl_DStringLength(&ds), 0, NULL, 
		        StrBody(initialFile), 255, &srcRead, &dstWrote, NULL);
                StrLength(initialFile) = (unsigned char) dstWrote;
                Tcl_DStringFree(&ds);            
	        break;
	    case SAVE_MESSAGE:
	        choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
	        Tcl_UtfToExternal(NULL, NULL, choice, choiceLen, 
		        0, NULL, StrBody(message), 255, 
		        &srcRead, &dstWrote, NULL);
                StrLength(message) = (unsigned char) dstWrote;
	        break;
	    case SAVE_PARENT:
	        choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
                parent = Tk_NameToWindow(interp, choice, parent);
	        if (parent == NULL) {
	            result = TCL_ERROR;
	            goto end;
	        }
	        break;
	    case SAVE_TITLE:
	        choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
	        Tcl_UtfToExternal(NULL, NULL, choice, choiceLen, 
		        0, NULL, StrBody(title), 255, 
		        &srcRead, &dstWrote, NULL);
                StrLength(title) = (unsigned char) dstWrote;
	        break;
	}
    }
         
    TkInitFileFilters(&ofd.fl);
    ofd.usePopup = 0;

    if (useNavServices) {
        AEDesc *initialPtr = NULL;
        
        if (initialDesc.descriptorType == typeFSS) {
            initialPtr = &initialDesc;
        }
        result = NavServicesGetFile(interp, &ofd, initialPtr, initialFile, 
                title, message, false, SAVE_FILE);
    } else {
        result = StdGetFile(interp, NULL, initialFile, SAVE_FILE);
    }

    end:
    
    AEDisposeDesc(&initialDesc);
    
    return result;
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
    int i, result;
    Tk_Window parent;
    AEDesc initialDesc = {typeNull, NULL};
    FSSpec dirSpec;
    Str255 message, title;
    int srcRead, dstWrote;
    OpenFileData ofd;
    static CONST char *chooseOptionStrings[] = {
	    "-initialdir", "-message", "-mustexist", "-parent", "-title", NULL
    };
    enum chooseOptions {
	    CHOOSE_INITDIR,	CHOOSE_MESSAGE, CHOOSE_MUSTEXIST, 
	    CHOOSE_PARENT, CHOOSE_TITLE
    };
  
    
    if (!NavServicesAvailable()) {
        return TCL_ERROR;
    }

    if (!fileDlgInited) {
	InitFileDialogs();
    }
    result = TCL_ERROR;    
    parent = (Tk_Window) clientData;    
    title[0] = 0;
    message[0] = 0;   

    for (i = 1; i < objc; i += 2) {
        char *choice;
	int index, choiceLen;
	char *string;

	if (Tcl_GetIndexFromObj(interp, objv[i], chooseOptionStrings, "option",
		TCL_EXACT, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (i + 1 == objc) {
	    string = Tcl_GetStringFromObj(objv[i], NULL);
	    Tcl_AppendResult(interp, "value for \"", string, "\" missing", 
		    (char *) NULL);
	    return TCL_ERROR;
	}
	switch (index) {
	    case CHOOSE_INITDIR:
	        choice = Tcl_GetStringFromObj(objv[i + 1], NULL);
                if (HandleInitialDirectory(interp, choice, &dirSpec, 
                        &initialDesc) != TCL_OK) {
                    result = TCL_ERROR;
                    goto end;
                }
	        break;
	    case CHOOSE_MESSAGE:
	        choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
	        Tcl_UtfToExternal(NULL, NULL, choice, choiceLen, 
		        0, NULL, StrBody(message), 255, 
		        &srcRead, &dstWrote, NULL);
                StrLength(message) = (unsigned char) dstWrote;
	        break;
	    case CHOOSE_PARENT:
	        choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
                parent = Tk_NameToWindow(interp, choice, parent);
	        if (parent == NULL) {
	            result = TCL_ERROR;
	            goto end;
	        }
	        break;
	    case CHOOSE_TITLE:
	        choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
	        Tcl_UtfToExternal(NULL, NULL, choice, choiceLen, 
		        0, NULL, StrBody(title), 255, 
		        &srcRead, &dstWrote, NULL);
                StrLength(title) = (unsigned char) dstWrote;
	        break;
	}
    }
             
    TkInitFileFilters(&ofd.fl);
    ofd.usePopup = 0;

    if (useNavServices) {
        AEDesc *initialPtr = NULL;
        
        if (initialDesc.descriptorType == typeFSS) {
            initialPtr = &initialDesc;
        }
        result = NavServicesGetFile(interp, &ofd, initialPtr, NULL, 
                title, message, false, CHOOSE_FOLDER);
    } else {
        result = TCL_ERROR;
    }

    end:
    AEDisposeDesc(&initialDesc);
    
    return result;
}

int
HandleInitialDirectory (
    Tcl_Interp *interp,
    char *initialDir, 
    FSSpec *dirSpec, 
    AEDesc *dirDescPtr)
{
	Tcl_DString ds;
	long dirID;
	OSErr err;
	Boolean isDirectory;
	Str255 dir;
	int srcRead, dstWrote;
	
	if (Tcl_TranslateFileName(interp, initialDir, &ds) == NULL) {
	    return TCL_ERROR;
	}
	Tcl_UtfToExternal(NULL, NULL, Tcl_DStringValue(&ds), 
		Tcl_DStringLength(&ds), 0, NULL, StrBody(dir), 255, 
		&srcRead, &dstWrote, NULL);
        StrLength(dir) = (unsigned char) dstWrote;
	Tcl_DStringFree(&ds);
          
	err = FSpLocationFromPath(StrLength(dir), StrBody(dir), dirSpec);
	if (err != noErr) {
	    Tcl_AppendResult(interp, "bad directory \"", initialDir, "\"", NULL);
	    return TCL_ERROR;
	}
	err = FSpGetDirectoryIDTcl(dirSpec, &dirID, &isDirectory);
	if ((err != noErr) || !isDirectory) {
	    Tcl_AppendResult(interp, "bad directory \"", initialDir, "\"", NULL);
	    return TCL_ERROR;
	}

        if (useNavServices) {
            AECreateDesc( typeFSS, dirSpec, sizeof(*dirSpec), dirDescPtr);        
        } else {
	    /*
	     * Make sure you negate -dirSpec.vRefNum because the 
	     * standard file package wants it that way !
	     */
	
	    LMSetSFSaveDisk(-dirSpec->vRefNum);
	    LMSetCurDirStore(dirID);
	}
        return TCL_OK;
}

static void
InitFileDialogs()
{
    fileDlgInited = 1;
    
    if (NavServicesAvailable()) {
        openFileFilterUPP = NewNavObjectFilterProc(OpenFileFilterProc);
        openFileEventUPP = NewNavEventProc(OpenEventProc);
        useNavServices = 1;
    } else {
	openFilter = NewFileFilterYDProc(FileFilterProc);
	openHook = NewDlgHookYDProc(OpenHookProc);
	saveHook = NewDlgHookYDProc(OpenHookProc);
	useNavServices = 0;
    }
    
        
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
    return TCL_OK;
}

static int
NavServicesGetFile(
    Tcl_Interp *interp,
    OpenFileData *ofdPtr,
    AEDesc *initialDesc,
    unsigned char *initialFile,
    StringPtr title,
    StringPtr message,
    int multiple,
    int isOpen)
{
    NavReplyRecord theReply;
    NavDialogOptions diagOptions;
    OSErr err;
    Tcl_Obj *theResult;
    int result;

    
    diagOptions.location.h = -1;
    diagOptions.location.v = -1;
    diagOptions.dialogOptionFlags = kNavDontAutoTranslate 
            + kNavDontAddTranslateItems;
            
    if (multiple) {
        diagOptions.dialogOptionFlags += kNavAllowMultipleFiles;
    }
    
    if (ofdPtr != NULL && ofdPtr->usePopup) {
        FileFilter *filterPtr;
        
	filterPtr = ofdPtr->fl.filters;
	if (filterPtr == NULL) {
	    ofdPtr->usePopup = 0;
	}
    }
    
    if (ofdPtr != NULL && ofdPtr->usePopup) {    
        NavMenuItemSpecHandle popupExtensionHandle = NULL;
        NavMenuItemSpec *popupItems;
        FileFilter *filterPtr;
        short index = 0;
	
	ofdPtr->curType = 0;
	
        popupExtensionHandle = (NavMenuItemSpecHandle) NewHandle(ofdPtr->fl.numFilters 
                * sizeof(NavMenuItemSpec));
        HLock((Handle) popupExtensionHandle);
        popupItems = *popupExtensionHandle;
        
        for (filterPtr = ofdPtr->fl.filters; filterPtr != NULL; 
                filterPtr = filterPtr->next, popupItems++, index++) {
            int len;
            
            len = strlen(filterPtr->name);
            BlockMove(filterPtr->name, popupItems->menuItemName + 1, len);
            popupItems->menuItemName[0] = len;
            popupItems->menuCreator = 'WIsH';
            popupItems->menuType = index;
        }
        HUnlock((Handle) popupExtensionHandle);
        diagOptions.popupExtension = popupExtensionHandle;
    } else {        
        diagOptions.dialogOptionFlags += kNavNoTypePopup; 
        diagOptions.popupExtension = NULL;
    }
        
    if ((initialFile != NULL) && (initialFile[0] != 0)) {
        char *lastColon;
        int len;
        
        len = initialFile[0];
        
        p2cstr(initialFile);        
        lastColon = strrchr((char *)initialFile, ':');
        if (lastColon != NULL) {
            len -= lastColon - ((char *) (initialFile + 1));
            BlockMove(lastColon + 1, diagOptions.savedFileName + 1, len);
            diagOptions.savedFileName[0] = len;
        } else {  
            BlockMove(initialFile, diagOptions.savedFileName + 1, len);
            diagOptions.savedFileName[0] = len;
        }
    } else {
        diagOptions.savedFileName[0] = 0;
    }
    
    strcpy((char *) (diagOptions.clientName + 1),"Wish");
    diagOptions.clientName[0] = strlen("Wish");
    
    if (title == NULL) {
        diagOptions.windowTitle[0] = 0;
    } else {
        BlockMove(title, diagOptions.windowTitle, title[0] + 1);
        diagOptions.windowTitle[0] = title[0];
    }
    
    if (message == NULL) {
        diagOptions.message[0] = 0;
    } else {
        BlockMove(message, diagOptions.message, message[0] + 1);
        diagOptions.message[0] = message[0];
    }
    
    diagOptions.actionButtonLabel[0] = 0;
    diagOptions.cancelButtonLabel[0] = 0;
    diagOptions.preferenceKey = 0;
    
    /* Now process the selection list.  We have to use the popupExtension
     * to fill the menu.
     */
    
    
    if (isOpen == OPEN_FILE) {
        err = NavGetFile(initialDesc, &theReply, &diagOptions, openFileEventUPP,  
                NULL, openFileFilterUPP, NULL, ofdPtr);    
    } else if (isOpen == SAVE_FILE) {
        err = NavPutFile (initialDesc, &theReply, &diagOptions, openFileEventUPP, 
                'TEXT', 'WIsH', NULL);
    } else if (isOpen == CHOOSE_FOLDER) {
        err = NavChooseFolder (initialDesc, &theReply, &diagOptions,
                openFileEventUPP, NULL, NULL);
    }
    
                        
    /*
     * Most commands assume that the file dialogs return a single
     * item, not a list.  So only build a list if multiple is true...
     */
                         
    if (multiple) {
        theResult = Tcl_NewListObj(0, NULL);
    } else {
        theResult = Tcl_NewObj();
    }
           
    if ( theReply.validRecord && err == noErr ) {
        AEDesc resultDesc;
        long count;
        Tcl_DString fileName;
        Handle pathHandle;
        int length;
        
        if ( err == noErr ) {
            err = AECountItems(&(theReply.selection), &count);
            if (err == noErr) {
                long i;
                for (i = 1; i <= count; i++ ) {
                    err = AEGetNthDesc(&(theReply.selection),
                            i, typeFSS, NULL, &resultDesc);
                    if (err == noErr) {
                        HLock(resultDesc.dataHandle);
                        pathHandle = NULL;
                        FSpPathFromLocation((FSSpec *) *resultDesc.dataHandle, 
                                &length, &pathHandle);
                        HLock(pathHandle);
                        Tcl_ExternalToUtfDString(NULL, (char *) *pathHandle, -1, &fileName);
                        if (multiple) {
                            Tcl_ListObjAppendElement(interp, theResult, 
                                    Tcl_NewStringObj(Tcl_DStringValue(&fileName), 
                                    Tcl_DStringLength(&fileName)));
                        } else {
                            Tcl_SetStringObj(theResult, Tcl_DStringValue(&fileName), 
                                    Tcl_DStringLength(&fileName));
                        }
                        
                        Tcl_DStringFree(&fileName);
                        HUnlock(pathHandle);
                        DisposeHandle(pathHandle);
                        HUnlock(resultDesc.dataHandle);
                        AEDisposeDesc( &resultDesc );
                    }
                }
            }
         }
         err = NavDisposeReply( &theReply );
         Tcl_SetObjResult(interp, theResult);
         result = TCL_OK;
    } else if (err == userCanceledErr) {
        result = TCL_OK;
    } else {
        result = TCL_ERROR;
    }
    
    if (diagOptions.popupExtension != NULL) {
        DisposeHandle((Handle) diagOptions.popupExtension);
    }
    
    return result;
}

static pascal Boolean 
OpenFileFilterProc( 
    AEDesc* theItem, void* info,
    NavCallBackUserData callBackUD,
    NavFilterModes filterMode )
{
    OpenFileData *ofdPtr = (OpenFileData *) callBackUD;
    if (!ofdPtr->usePopup) {
        return true;
    } else {
        if (ofdPtr->fl.numFilters == 0) {
            return true;
        } else {
            
            if ( theItem->descriptorType == typeFSS ) {
                NavFileOrFolderInfo* theInfo = (NavFileOrFolderInfo*)info;
                int result;
                
                if ( !theInfo->isFolder ) {
                    OSType fileType;
                    StringPtr fileNamePtr;
                    int i;
                    FileFilter *filterPtr;
               
                    fileType = theInfo->fileAndFolder.fileInfo.finderInfo.fdType;
                    HLock(theItem->dataHandle);
                    fileNamePtr = (((FSSpec *) *theItem->dataHandle)->name);
                    
                    if (ofdPtr->usePopup) {
                        i = ofdPtr->curType;
	                for (filterPtr=ofdPtr->fl.filters; filterPtr && i>0; i--) {
	                    filterPtr = filterPtr->next;
	                }
	                if (filterPtr) {
	                    result = MatchOneType(fileNamePtr, fileType,
	                            ofdPtr, filterPtr);
	                } else {
	                    result = false;
                        }
                    } else {
	                /*
	                 * We are not using the popup menu. In this case, the file is
	                 * considered matched if it matches any of the file filters.
	                 */
			result = UNMATCHED;
	                for (filterPtr=ofdPtr->fl.filters; filterPtr;
		                filterPtr=filterPtr->next) {
	                    if (MatchOneType(fileNamePtr, fileType,
	                            ofdPtr, filterPtr) == MATCHED) {
	                        result = MATCHED;
	                        break;
	                    }
	                }
                    }
                    
                    HUnlock(theItem->dataHandle);
                    return (result == MATCHED);
                } else {
                    return true;
                }
            }
        }
        
        return true;
    }
}

pascal void 
OpenEventProc(
    NavEventCallbackMessage callBackSelector,
    NavCBRecPtr callBackParams,
    NavCallBackUserData callBackUD )
{
    NavMenuItemSpec *chosenItem;
    OpenFileData *ofd = (OpenFileData *) callBackUD;
        
    if (callBackSelector ==  kNavCBPopupMenuSelect) {
        chosenItem = (NavMenuItemSpec *) callBackParams->eventData.eventDataParms.param;
        ofd->curType = chosenItem->menuType;
    } else if (callBackSelector == kNavCBEvent) {
    	if (callBackParams->eventData.eventDataParms.event->what == updateEvt) {
    		if (TkMacConvertEvent( callBackParams->eventData.eventDataParms.event)) {
        		while (Tcl_DoOneEvent(TCL_IDLE_EVENTS|TCL_DONT_WAIT|TCL_WINDOW_EVENTS)) {
           			/* Empty Body */
        		}
        	}
        }
    }
}

static int
StdGetFile(
    Tcl_Interp *interp,
    OpenFileData *ofd,
    unsigned char *initialFile,
    int isOpen)
{
    int i;
    StandardFileReply reply;
    Point mypoint;
    MenuHandle menu = NULL;


    /*
     * Set the items in the file types popup.
     */

    /*
     * Delete all the entries inside the popup menu, in case there's any
     * left overs from previous invocation of this command
     */

    if (ofd != NULL && ofd->usePopup) {
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

	filterPtr = ofd->fl.filters;
	if (filterPtr == NULL) {
	    ofd->usePopup = 0;
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
     * Call the toolbox file dialog function.
     */
     
    SetPt(&mypoint, -1, -1);
    TkpSetCursor(NULL);
    if (isOpen == OPEN_FILE) {
        if (ofd != NULL && ofd->usePopup) {
	    CustomGetFile(openFilter, (short) -1, NULL, &reply, OPEN_BOX,
	    	    mypoint, openHook, NULL, NULL, NULL, (void*) ofd);
	} else {
	    StandardGetFile(NULL, -1, NULL, &reply);
	}
    } else if (isOpen == SAVE_FILE) {
	static Str255 prompt = "\pSave as";
	
   	if (ofd != NULL && ofd->usePopup) {
   	    /*
   	     * Currently this never gets called because we don't use
   	     * popup for the save dialog.
   	     */
	    CustomPutFile(prompt, initialFile, &reply, OPEN_BOX, 
		    mypoint, saveHook, NULL, NULL, NULL, (void *) ofd);
	} else {
	    StandardPutFile(prompt, initialFile, &reply);
	}
    }

    /*
     * Now parse the reply, and populate the Tcl result.
     */
     
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
    
    if (menu != NULL) {
    	DisposeMenu(menu);
    }

    return TCL_OK;
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
		newType = GetControlValue((ControlRef) handle) - 1;
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
	    return MatchOneType(pb->hFileInfo.ioNamePtr, pb->hFileInfo.ioFlFndrInfo.fdType,
	            ofdPtr, filterPtr);
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
	    if (MatchOneType(pb->hFileInfo.ioNamePtr, pb->hFileInfo.ioFlFndrInfo.fdType,
	            ofdPtr, filterPtr) == MATCHED) {
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
    StringPtr fileNamePtr,	/* Name of the file */
    OSType    fileType,         /* Type of the file */ 
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
        
	    if (fileNamePtr == NULL) {
		continue;
	    }
	    p = (char*)(fileNamePtr);
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
	    if (fileType == mfPtr->type) {
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


