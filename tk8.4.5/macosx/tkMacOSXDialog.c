/*
 * tkMacOSXDialog.c --
 *
 *        Contains the Mac implementation of the common dialog boxes.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */
#include <Carbon/Carbon.h>

#include "tkPort.h"
#include "tkInt.h"
#include "tkMacOSXInt.h"
#include "tkFileFilter.h"

#ifndef StrLength
#define StrLength(s)                 (*((unsigned char *) (s)))
#endif
#ifndef StrBody
#define StrBody(s)                ((char *) (s) + 1)
#endif

/*
 * The following are ID's for resources that are defined in tkMacOSXResource.r
 */
#define OPEN_BOX        130
#define OPEN_POPUP      131
#define OPEN_MENU       132
#define OPEN_POPUP_ITEM 10

#define SAVE_FILE        0
#define OPEN_FILE        1
#define CHOOSE_FOLDER    2

#define MATCHED          0
#define UNMATCHED        1

#define TK_DEFAULT_ABOUT 128

/*
 * The following structure is used in the GetFileName() function. It stored
 * information about the file dialog and the file filters.
 */
typedef struct _OpenFileData {
    FileFilterList fl;                        /* List of file filters. */
    SInt16 curType;                        /* The filetype currently being
                                         * listed. */
    short popupItem;                        /* Item number of the popup in the
                                         * dialog. */
    int usePopup;                        /* True if we show the popup menu (this
                                             * is an open operation and the
                                         * -filetypes option is set). */
} OpenFileData;


static Boolean                MatchOneType _ANSI_ARGS_((StringPtr fileNamePtr, OSType fileType,
                            OpenFileData *myofdPtr, FileFilter *filterPtr));
static pascal Boolean   OpenFileFilterProc(AEDesc* theItem, void* info, 
                            NavCallBackUserData callBackUD,
                            NavFilterModes filterMode );
pascal void             OpenEventProc(NavEventCallbackMessage callBackSelector,
                            NavCBRecPtr callBackParms,
                            NavCallBackUserData callBackUD );
static void             InitFileDialogs();
static int              NavServicesGetFile(Tcl_Interp *interp, OpenFileData *ofd,
                            AEDesc *initialDescPtr,
                            unsigned char *initialFile, AEDescList *selectDescPtr,
                            StringPtr title, StringPtr message, int multiple, int isOpen);
static int              HandleInitialDirectory (Tcl_Interp *interp,
                                                char *initialFile, char *initialDir,
                                                FSRef *dirRef,
                                                AEDescList *selectDescPtr,
                                                AEDesc *dirDescPtr);                            

/*
 * Have we initialized the file dialog subsystem
 */

static int fileDlgInited = 0;

/*
 * Filter and hook functions used by the tk_getOpenFile and tk_getSaveFile
 * commands.
 */

NavObjectFilterUPP openFileFilterUPP;
NavEventUPP openFileEventUPP;


/*
 *----------------------------------------------------------------------
 *
 * Tk_ChooseColorObjCmd --
 *
 *        This procedure implements the color dialog box for the Mac
 *        platform. See the user documentation for details on what it
 *        does.
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
    ClientData clientData,        /* Main window associated with interpreter. */
    Tcl_Interp *interp,                /* Current interpreter. */
    int objc,                        /* Number of arguments. */
    Tcl_Obj *CONST objv[])        /* Argument objects. */
{
    Tk_Window parent;
    char *title;
    int i, picked, srcRead, dstWrote;
    ColorPickerInfo cpinfo;
    static int inited = 0;
    static RGBColor in;
    static CONST char *optionStrings[] = {
        "-initialcolor",    "-parent",            "-title",            NULL
    };
    enum options {
        COLOR_INITIAL,            COLOR_PARENT,   COLOR_TITLE
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
        

    cpinfo.theColor.profile = 0L;
    cpinfo.theColor.color.rgb.red   = in.red;
    cpinfo.theColor.color.rgb.green = in.green;
    cpinfo.theColor.color.rgb.blue  = in.blue;
    cpinfo.dstProfile = 0L;
    cpinfo.flags = kColorPickerCanModifyPalette 
            |  kColorPickerCanAnimatePalette;
    cpinfo.placeWhere = kDeepestColorScreen;
    cpinfo.pickerType = 0L;
    cpinfo.eventProc = NULL;
    cpinfo.colorProc = NULL;
    cpinfo.colorProcData = NULL;
    
    /* This doesn't seem to actually set the title! */
    Tcl_UtfToExternal(NULL, NULL, title, -1, 0, NULL, 
        StrBody(cpinfo.prompt), 255, &srcRead, &dstWrote, NULL);
    StrLength(cpinfo.prompt) = (unsigned char) dstWrote;

    if ((PickColor(&cpinfo) == noErr) && (cpinfo.newColorChosen != 0)) {
        in.red    = cpinfo.theColor.color.rgb.red;
        in.green  = cpinfo.theColor.color.rgb.green;
        in.blue   = cpinfo.theColor.color.rgb.blue;
        picked = 1;
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
 *        This procedure implements the "open file" dialog box for the
 *        Mac platform. See the user documentation for details on what
 *        it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *        See user documentation.
 *----------------------------------------------------------------------
 */

int
Tk_GetOpenFileObjCmd(
    ClientData clientData,        /* Main window associated with interpreter. */
    Tcl_Interp *interp,                /* Current interpreter. */
    int objc,                        /* Number of arguments. */
    Tcl_Obj *CONST objv[])        /* Argument objects. */
{
    int i, result, multiple;
    OpenFileData ofd;
    Tk_Window parent;
    Str255 message, title;
    AEDesc initialDesc = {typeNull, NULL};
    FSRef dirRef;
    AEDesc *initialPtr = NULL;
    AEDescList selectDesc = {typeNull, NULL};
    char *initialFile = NULL, *initialDir = NULL;
    static CONST char *openOptionStrings[] = {
            "-defaultextension", "-filetypes", 
            "-initialdir", "-initialfile", 
            "-message", "-multiple",
            "-parent",        "-title",         NULL
    };
    enum openOptions {
            OPEN_DEFAULT, OPEN_FILETYPES,        
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
    
    ofd.curType                = 0;
    ofd.popupItem        = OPEN_POPUP_ITEM;
    ofd.usePopup         = 1;

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
            case OPEN_FILETYPES:
                choice = Tcl_GetStringFromObj(objv[i + 1], NULL);
                if (TkGetFileFilters(interp, &ofd.fl, choice, 0) 
                        != TCL_OK) {
                    result = TCL_ERROR;
                    goto end;
                }
                break;
            case OPEN_INITDIR:
                initialDir = Tcl_GetStringFromObj(objv[i + 1], NULL);
                break;
            case OPEN_INITFILE:
                initialFile = Tcl_GetStringFromObj(objv[i + 1], NULL);
                break;
            case OPEN_MESSAGE:
                choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
                Tcl_UtfToExternal(NULL, TkMacOSXCarbonEncoding, choice, choiceLen, 
                        0, NULL, StrBody(message), 255, 
                        &srcRead, &dstWrote, NULL);
                message[0] = dstWrote;
                break;
            case OPEN_MULTIPLE:
                if (Tcl_GetBooleanFromObj(interp, objv[i + 1], &multiple) 
                        != TCL_OK) {
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
                Tcl_UtfToExternal(NULL, TkMacOSXCarbonEncoding, choice, choiceLen, 
                        0, NULL, StrBody(title), 255, 
                        &srcRead, &dstWrote, NULL);
                title[0] = dstWrote;
                break;
        }
    }

    if (HandleInitialDirectory(interp, initialFile, initialDir, &dirRef,
                               &selectDesc, &initialDesc) != TCL_OK) {
        result = TCL_ERROR;
        goto end;
    }
    
    if (initialDesc.descriptorType == typeFSRef) {
        initialPtr = &initialDesc;
    }
    result = NavServicesGetFile(interp, &ofd, initialPtr,
            NULL, &selectDesc, 
            title, message, multiple, OPEN_FILE);

    end:
    TkFreeFileFilters(&ofd.fl);
    AEDisposeDesc(&initialDesc);
    AEDisposeDesc(&selectDesc);
    
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetSaveFileObjCmd --
 *
 *        Same as Tk_GetOpenFileCmd but opens a "save file" dialog box
 *        instead
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *        See user documentation.
 *----------------------------------------------------------------------
 */

int
Tk_GetSaveFileObjCmd(
    ClientData clientData,     /* Main window associated with interpreter. */
    Tcl_Interp *interp,        /* Current interpreter. */
    int objc,                  /* Number of arguments. */
    Tcl_Obj *CONST objv[])     /* Argument objects. */
{
    int i, result;
    Str255 initialFile;
    Tk_Window parent;
    AEDesc initialDesc = {typeNull, NULL};
    AEDesc *initialPtr = NULL;
    FSRef dirRef;
    Str255 title, message;
    OpenFileData ofd;
    static CONST char *saveOptionStrings[] = {
            "-defaultextension", "-filetypes", "-initialdir", "-initialfile", 
            "-message", "-parent",        "-title",         NULL
    };
    enum saveOptions {
            SAVE_DEFAULT, SAVE_FILETYPES, SAVE_INITDIR, SAVE_INITFILE,
            SAVE_MESSAGE, SAVE_PARENT, SAVE_TITLE
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
            case SAVE_FILETYPES:
                /* Currently unimplemented - what would we do here anyway? */
                break;
            case SAVE_INITDIR:
                choice = Tcl_GetStringFromObj(objv[i + 1], NULL);
                if (HandleInitialDirectory(interp, NULL, choice, &dirRef, 
                        NULL, &initialDesc) != TCL_OK) {
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
                Tcl_UtfToExternal(NULL, TkMacOSXCarbonEncoding, Tcl_DStringValue(&ds), 
                        Tcl_DStringLength(&ds), 0, NULL, 
                        StrBody(initialFile), 255, &srcRead, &dstWrote, NULL);
                StrLength(initialFile) = (unsigned char) dstWrote;
                Tcl_DStringFree(&ds);            
                break;
            case SAVE_MESSAGE:
                choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
                Tcl_UtfToExternal(NULL, TkMacOSXCarbonEncoding, choice, choiceLen, 
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
                Tcl_UtfToExternal(NULL, TkMacOSXCarbonEncoding, choice, choiceLen, 
                        0, NULL, StrBody(title), 255, 
                        &srcRead, &dstWrote, NULL);
                StrLength(title) = (unsigned char) dstWrote;
                break;
        }
    }
         
    TkInitFileFilters(&ofd.fl);
    ofd.usePopup = 0;

    if (initialDesc.descriptorType == typeFSRef) {
        initialPtr = &initialDesc;
    }
    result = NavServicesGetFile(interp, &ofd, initialPtr, initialFile, NULL,
        title, message, false, SAVE_FILE);

    end:
    
    AEDisposeDesc(&initialDesc);
    
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ChooseDirectoryObjCmd --
 *
 *        This procedure implements the "tk_chooseDirectory" dialog box 
 *        for the Windows platform. See the user documentation for details 
 *        on what it does.
 *
 * Results:
 *        See user documentation.
 *
 * Side effects:
 *        A modal dialog window is created.  Tcl_SetServiceMode() is
 *        called to allow background events to be processed
 *
 *----------------------------------------------------------------------
 */

int
Tk_ChooseDirectoryObjCmd(clientData, interp, objc, objv)
    ClientData clientData;        /* Main window associated with interpreter. */
    Tcl_Interp *interp;                /* Current interpreter. */
    int objc;                        /* Number of arguments. */
    Tcl_Obj *CONST objv[];        /* Argument objects. */
{
    int i, result;
    Tk_Window parent;
    AEDesc initialDesc = {typeNull, NULL};
    AEDesc *initialPtr = NULL;
    FSRef dirRef;
    Str255 message, title;
    int srcRead, dstWrote;
    OpenFileData ofd;
    static CONST char *chooseOptionStrings[] = {
            "-initialdir", "-message", "-mustexist", "-parent", "-title", NULL
    };
    enum chooseOptions {
            CHOOSE_INITDIR,        CHOOSE_MESSAGE, CHOOSE_MUSTEXIST, 
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
                if (HandleInitialDirectory(interp, NULL, choice, &dirRef,  
                        NULL, &initialDesc) != TCL_OK) {
                    result = TCL_ERROR;
                    goto end;
                }
                break;
            case CHOOSE_MESSAGE:
                choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
                Tcl_UtfToExternal(NULL, TkMacOSXCarbonEncoding, choice, choiceLen, 
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
                Tcl_UtfToExternal(NULL, TkMacOSXCarbonEncoding, choice, choiceLen, 
                        0, NULL, StrBody(title), 255, 
                        &srcRead, &dstWrote, NULL);
                StrLength(title) = (unsigned char) dstWrote;
                break;
        }
    }
             
    TkInitFileFilters(&ofd.fl);
    ofd.usePopup = 0;

        
    if (initialDesc.descriptorType == typeFSRef) {
        initialPtr = &initialDesc;
    }
    result = NavServicesGetFile(interp, &ofd, initialPtr, NULL, NULL,
        title, message, false, CHOOSE_FOLDER);

    end:
    AEDisposeDesc(&initialDesc);
    
    return result;
}

int
HandleInitialDirectory (
    Tcl_Interp *interp,
    char *initialFile,
    char *initialDir,
    FSRef *dirRef, 
    AEDescList *selectDescPtr,
    AEDesc *dirDescPtr)
{
    Tcl_DString ds;
    OSErr err;
    Boolean isDirectory;
    char *dirName = NULL;
    int result = TCL_OK;

    if (initialDir != NULL) {
        dirName = Tcl_TranslateFileName(interp, initialDir, &ds);
        if (dirName == NULL) {
            return TCL_ERROR;
        }

        err = FSPathMakeRef(dirName,
                dirRef, &isDirectory);     

        if (err != noErr) {
            Tcl_AppendResult(interp, "bad directory \"",
                             initialDir, "\"", NULL);
            result = TCL_ERROR;
            goto end;
        }
        if (!isDirectory) {
            Tcl_AppendResult(interp, "-intialdir \"",
                    initialDir, " is a file, not a directory.\"", NULL);
            result = TCL_ERROR;
            goto end;
        }

        AECreateDesc(typeFSRef, dirRef, sizeof(*dirRef), dirDescPtr);
    }

    if (initialFile != NULL && selectDescPtr != NULL) {
        FSRef fileRef;
        AEDesc fileDesc;
        char *namePtr;

        if (initialDir != NULL) {
            Tcl_DStringAppend(&ds, "/", 1);
            Tcl_DStringAppend(&ds, initialFile, -1);
            namePtr = Tcl_DStringValue(&ds);
        } else {
            namePtr = initialFile;
        }

        AECreateList(NULL, 0, false, selectDescPtr);

        err = FSPathMakeRef(namePtr, &fileRef, &isDirectory);
        if (err != noErr) {
            Tcl_AppendResult(interp, "bad initialfile \"", initialFile,
                    "\" file does not exist.", NULL);
            return TCL_ERROR;
        }
        AECreateDesc(typeFSRef, &fileRef, sizeof(fileRef), &fileDesc);
        AEPutDesc(selectDescPtr, 1, &fileDesc);
        AEDisposeDesc(&fileDesc);
    }

end:
    if (dirName != NULL) {
        Tcl_DStringFree(&ds);
    }
    return result;
}

static void
InitFileDialogs()
{
    fileDlgInited = 1;
    openFileFilterUPP = NewNavObjectFilterUPP(OpenFileFilterProc);
    openFileEventUPP = NewNavEventUPP(OpenEventProc);
}

static int
NavServicesGetFile(
    Tcl_Interp *interp,
    OpenFileData *ofdPtr,
    AEDesc *initialDescPtr,
    unsigned char *initialFile,
    AEDescList *selectDescPtr,
    StringPtr title,
    StringPtr message,
    int multiple,
    int isOpen)
{
    NavReplyRecord theReply;
    NavDialogCreationOptions diagOptions;
    NavDialogRef dialogRef = NULL;
    CFStringRef * menuItemNames = NULL;
    OSErr err;
    Tcl_Obj *theResult = NULL;
    int result;
    TextEncoding encoding;

    encoding = GetApplicationTextEncoding();
    err = NavGetDefaultDialogCreationOptions(&diagOptions);
    if (err!=noErr) {
        return TCL_ERROR;
    }
    diagOptions.location.h = -1;
    diagOptions.location.v = -1;
    diagOptions.optionFlags = kNavDontAutoTranslate 
        + kNavDontAddTranslateItems;
            
    if (multiple) {
        diagOptions.optionFlags += kNavAllowMultipleFiles;
    }
    
    if (ofdPtr != NULL && ofdPtr->usePopup) {
        FileFilter *filterPtr;
        
        filterPtr = ofdPtr->fl.filters;
        if (filterPtr == NULL) {
            ofdPtr->usePopup = 0;
        }
    }
    
    if (ofdPtr != NULL && ofdPtr->usePopup) {    
        FileFilter *filterPtr;
        int index = 0;
        ofdPtr->curType = 0;
        
        menuItemNames = (CFStringRef *)ckalloc(ofdPtr->fl.numFilters 
            * sizeof(CFStringRef));
        
        for (filterPtr = ofdPtr->fl.filters; filterPtr != NULL; 
                filterPtr = filterPtr->next, index++) {
            menuItemNames[index] = CFStringCreateWithCString(NULL, 
                    filterPtr->name, encoding);
        }
        diagOptions.popupExtension = CFArrayCreate(NULL, 
                (const void **)menuItemNames, ofdPtr->fl.numFilters, NULL);;
    } else {        
        diagOptions.optionFlags += kNavNoTypePopup; 
        diagOptions.popupExtension = NULL;
    }

    /*
     * This is required to allow App packages to be selectable in the
     * file dialogs...
     */
    
    diagOptions.optionFlags += kNavSupportPackages;
    
    diagOptions.clientName = CFStringCreateWithCString(NULL, "Wish", encoding);
    if (message == NULL) {
        diagOptions.message = NULL;
    } else {
        diagOptions.message = CFStringCreateWithPascalString(NULL, message, encoding);
    }
    if ((initialFile != NULL) && (initialFile[0] != 0)) {
        diagOptions.saveFileName = CFStringCreateWithPascalString(NULL,
                initialFile, encoding);
    } else {
        diagOptions.saveFileName = NULL;
    }
    if (title == NULL) {
        diagOptions.windowTitle = NULL;
    } else {
        diagOptions.windowTitle = CFStringCreateWithPascalString(NULL, title, encoding);
    }
    
    diagOptions.actionButtonLabel = NULL;
    diagOptions.cancelButtonLabel = NULL;
    diagOptions.preferenceKey = 0;
    
    /* 
     * Now process the selection list.  We have to use the popupExtension
     * to fill the menu.
     */
    
    if (isOpen == OPEN_FILE) {
        err = NavCreateGetFileDialog(&diagOptions,
            NULL,
            openFileEventUPP,
            NULL,
            openFileFilterUPP,
            ofdPtr,
            &dialogRef);
        if (err!=noErr){
            fprintf(stderr,"NavCreateGetFileDialog failed, %d\n", err );
            dialogRef = NULL;
        }
    } else if (isOpen == SAVE_FILE) {
        err = NavCreatePutFileDialog(&diagOptions, 'TEXT', 'WIsH', 
                openFileEventUPP, NULL, &dialogRef);
        if (err!=noErr){
            fprintf(stderr,"NavCreatePutFileDialog failed, %d\n", err );
            dialogRef = NULL;
        }
    } else if (isOpen == CHOOSE_FOLDER) {
        err = NavCreateChooseFolderDialog(&diagOptions, openFileEventUPP, 
                openFileFilterUPP, NULL, &dialogRef);
        if (err!=noErr){
            fprintf(stderr,"NavCreateChooseFolderDialog failed, %d\n", err );
            dialogRef = NULL;
        }
    }

    if (dialogRef) {
        if (initialDescPtr != NULL) {
            NavCustomControl (dialogRef, kNavCtlSetLocation, initialDescPtr);
        }
        if ((selectDescPtr != NULL)
                && (selectDescPtr->descriptorType != typeNull)) {
            NavCustomControl(dialogRef, kNavCtlSetSelection, selectDescPtr);
        }
        
        if ((err = NavDialogRun(dialogRef)) != noErr ){
            fprintf(stderr,"NavDialogRun failed, %d\n", err );
        } else {
            if ((err = NavDialogGetReply(dialogRef, &theReply)) != noErr) {
                fprintf(stderr,"NavGetReply failed, %d\n", err );
            }
        }
    }

    /*
     * Most commands assume that the file dialogs return a single
     * item, not a list.  So only build a list if multiple is true...
     */
    if (err==noErr) {
        if (multiple) {
            theResult = Tcl_NewListObj(0, NULL);
        } else {
            theResult = Tcl_NewObj();
        }
        if (!theResult) {
            err = memFullErr;
        }
    }
    if (theReply.validRecord && err==noErr) {
        AEDesc resultDesc;
        long count;
        Tcl_DString fileName;
        FSRef  fsRef;
        char   pathPtr[1024];
        int    pathValid = 0;
        err = AECountItems(&theReply.selection, &count);
        if (err == noErr) {
            long i;
            for (i = 1; i <= count; i++ ) {
                err = AEGetNthDesc(&theReply.selection,
                    i, typeFSRef, NULL, &resultDesc);
                pathValid = 0;
                if (err == noErr) {
                    if ((err = AEGetDescData(&resultDesc, &fsRef, sizeof(fsRef))) 
                            != noErr ) {
                        fprintf(stderr,"AEGetDescData failed %d\n", err );
                    } else {
                        if (err = FSRefMakePath(&fsRef, pathPtr, 1024) ) {
                            fprintf(stderr,"FSRefMakePath failed, %d\n", err );
                        } else {
                            if (isOpen == SAVE_FILE) {
                                CFStringRef saveNameRef;
                                char saveName [1024];
                                if (saveNameRef = NavDialogGetSaveFileName(dialogRef)) {
                                    if (CFStringGetCString(saveNameRef, saveName, 
                                            1024, encoding)) {
                                        strcat(pathPtr, "/");
                                        strcat(pathPtr, saveName);
                                        pathValid = 1;
                                    } else {
                                        fprintf(stderr, "CFStringGetCString failed\n");
                                    }
                                } else {
                                    fprintf(stderr, "NavDialogGetSaveFileName failed\n");
                                }
                            } else {
                                pathValid = 1;
                            }
                            if (pathValid) {
                                /* 
                                 * Tested this and NULL=utf-8 encoding is
                                 * good here
                                 */
                                Tcl_ExternalToUtfDString(NULL, pathPtr, -1, 
							 &fileName);
                                if (multiple) {
                                    Tcl_ListObjAppendElement(interp, theResult, 
                                        Tcl_NewStringObj(Tcl_DStringValue(&fileName), 
                                        Tcl_DStringLength(&fileName)));
                                } else {
                                    Tcl_SetStringObj(theResult, Tcl_DStringValue(&fileName), 
                                        Tcl_DStringLength(&fileName));
                                }
                                Tcl_DStringFree(&fileName);
                            }
                        }
                    }
                    AEDisposeDesc(&resultDesc);
                }
            }
         }
         err = NavDisposeReply(&theReply);
         Tcl_SetObjResult(interp, theResult);
         result = TCL_OK;
    } else if (err == userCanceledErr) {
        result = TCL_OK;
    } else {
        result = TCL_ERROR;
    }
    
    /* 
     * Clean up any allocated strings
     * dispose of things in reverse order of creation 
     */
     
    if (diagOptions.windowTitle) {
        CFRelease(diagOptions.windowTitle);
    }
    if (diagOptions.saveFileName) {
        CFRelease(diagOptions.saveFileName);
    }
    if (diagOptions.message) {
        CFRelease(diagOptions.message);
    }
    if (diagOptions.clientName) {
        CFRelease(diagOptions.clientName);
    }
    /* 
     * dispose of the CFArray diagOptions.popupExtension 
     */
     
    if (menuItemNames) {
        int i;
        for (i=0;i < ofdPtr->fl.numFilters;i++) {
            CFRelease(menuItemNames[i]);
        }
        ckfree((void *)menuItemNames);
    }
    if (diagOptions.popupExtension != NULL) {
        CFRelease(diagOptions.popupExtension);
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
    if (!ofdPtr || !ofdPtr->usePopup) {
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
                    HLock((Handle)theItem->dataHandle);
                    fileNamePtr = (((FSSpec *) *theItem->dataHandle)->name);
                    
                    if (ofdPtr->usePopup) {
                        i = ofdPtr->curType;
                        for (filterPtr = ofdPtr->fl.filters; filterPtr && i > 0; i--) {
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
                        for (filterPtr = ofdPtr->fl.filters; filterPtr;
                                filterPtr = filterPtr->next) {
                            if (MatchOneType(fileNamePtr, fileType,
                                    ofdPtr, filterPtr) == MATCHED) {
                                result = MATCHED;
                                break;
                            }
                        }
                    }
                    
                    HUnlock((Handle)theItem->dataHandle);
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
    static SInt32 otherEvent = ~(kNavCBCustomize|kNavCBStart|kNavCBTerminate
            |kNavCBNewLocation|kNavCBShowDesktop|kNavCBSelectEntry|kNavCBAccept
            |kNavCBCancel|kNavCBAdjustPreview);
        
    if (callBackSelector ==  kNavCBPopupMenuSelect) {
        chosenItem = (NavMenuItemSpec *) callBackParams->eventData.eventDataParms.param;
        ofd->curType = chosenItem->menuType;
    } else if (callBackSelector == kNavCBAdjustRect 
            || callBackSelector & otherEvent != 0) { 
        while (Tcl_DoOneEvent(TCL_IDLE_EVENTS
                | TCL_DONT_WAIT 
                | TCL_WINDOW_EVENTS)) {
            /* Empty Body */
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MatchOneType --
 *
 *        Match a file with one file type in the list of file types.
 *
 * Results:
 *        Returns MATCHED if the file matches with the file type; returns
 *        UNMATCHED otherwise.
 *
 * Side effects:
 *        None
 *
 *----------------------------------------------------------------------
 */

static Boolean
MatchOneType(
    StringPtr fileNamePtr,        /* Name of the file */
    OSType    fileType,         /* Type of the file */ 
    OpenFileData * ofdPtr,        /* Information about this file dialog */
    FileFilter * filterPtr)        /* Match the file described by pb against
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

/*
 *----------------------------------------------------------------------
 *
 * TkAboutDlg --
 *
 *        Displays the default Tk About box.  This code uses Macintosh
 *        resources to define the content of the About Box.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        None.
 *
 *----------------------------------------------------------------------
 */

void 
TkAboutDlg()
{
    DialogPtr aboutDlog;
    WindowRef windowRef;
    short itemHit = -9;
        
    aboutDlog = GetNewDialog(128, NULL, (void *) (-1));
        
    if (!aboutDlog) {
        return;
    }
        
    windowRef=GetDialogWindow(aboutDlog);
    SelectWindow(windowRef);
        
    while (itemHit != 1) {
        ModalDialog( NULL, &itemHit);
    }
    DisposeDialog(aboutDlog);
    aboutDlog = NULL;
        
    SelectWindow(FrontNonFloatingWindow());

    return;
}
