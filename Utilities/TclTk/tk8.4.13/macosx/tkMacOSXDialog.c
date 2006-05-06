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

#include "tkMacOSXInt.h"
#include "tkFileFilter.h"

#ifndef StrLength
#define StrLength(s)  (*((unsigned char *) (s)))
#endif
#ifndef StrBody
#define StrBody(s)  ((char *) (s) + 1)
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
    FileFilterList fl;    /* List of file filters. */
    SInt16 curType;    /* The filetype currently being listed. */
    short popupItem;    /* Item number of the popup in the dialog. */
    int usePopup;    /* True if we show the popup menu (this is
         * an open operation and the -filetypes
         * option is set). */
} OpenFileData;


/*
 * The following structure is used in the tk_messageBox
 * implementation.
 */
typedef struct {
    WindowRef  windowRef;
    int    buttonIndex;
} CallbackUserData;


static OSStatus    AlertHandler(EventHandlerCallRef callRef,
          EventRef eventRef, void *userData);
static Boolean                MatchOneType(StringPtr fileNamePtr, OSType fileType,
          OpenFileData *myofdPtr, FileFilter *filterPtr);
static pascal Boolean   OpenFileFilterProc(AEDesc* theItem, void* info,
          NavCallBackUserData callBackUD,
          NavFilterModes filterMode);
static pascal void      OpenEventProc(NavEventCallbackMessage callBackSelector,
          NavCBRecPtr callBackParms,
          NavCallBackUserData callBackUD);
static void             InitFileDialogs();
static int              NavServicesGetFile(Tcl_Interp *interp, OpenFileData *ofd,
          AEDesc *initialDescPtr,
          char *initialFile, AEDescList *selectDescPtr,
          CFStringRef title, CFStringRef message, int multiple, int isOpen);
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

static NavObjectFilterUPP openFileFilterUPP;
static NavEventUPP openFileEventUPP;


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
    cpinfo.colorProcData = 0;

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
    CFStringRef message, title;
    AEDesc initialDesc = {typeNull, NULL};
    FSRef dirRef;
    AEDesc *initialPtr = NULL;
    AEDescList selectDesc = {typeNull, NULL};
    char *initialFile = NULL, *initialDir = NULL;
    static CONST char *openOptionStrings[] = {
      "-defaultextension", "-filetypes",
      "-initialdir", "-initialfile",
      "-message", "-multiple",
      "-parent", "-title", NULL
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
    title = NULL;
    message = NULL;

    TkInitFileFilters(&ofd.fl);

    ofd.curType                = 0;
    ofd.popupItem        = OPEN_POPUP_ITEM;
    ofd.usePopup         = 1;

    for (i = 1; i < objc; i += 2) {
  char *choice;
  int index, choiceLen;
  char *string;

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
    if (TkGetFileFilters(interp, &ofd.fl, choice, 0) != TCL_OK) {
        result = TCL_ERROR;
        goto end;
    }
    break;
      case OPEN_INITDIR:
    initialDir = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
    /* empty strings should be like no selection given */
    if (choiceLen == 0) { initialDir = NULL; }
    break;
      case OPEN_INITFILE:
    initialFile = Tcl_GetStringFromObj(objv[i + 1], NULL);
    /* empty strings should be like no selection given */
    if (choiceLen == 0) { initialFile = NULL; }
    break;
      case OPEN_MESSAGE:
    choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
    message = CFStringCreateWithBytes(NULL, (unsigned char*) choice, choiceLen,
      kCFStringEncodingUTF8, false);
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
    title = CFStringCreateWithBytes(NULL, (unsigned char*) choice, choiceLen,
      kCFStringEncodingUTF8, false);
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
      NULL, &selectDesc, title, message, multiple, OPEN_FILE);

    end:
    TkFreeFileFilters(&ofd.fl);
    AEDisposeDesc(&initialDesc);
    AEDisposeDesc(&selectDesc);
    if (title != NULL) {
  CFRelease(title);
    }
    if (message != NULL) {
  CFRelease(message);
    }

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
    char *initialFile = NULL;
    Tk_Window parent;
    AEDesc initialDesc = {typeNull, NULL};
    AEDesc *initialPtr = NULL;
    FSRef dirRef;
    CFStringRef title, message;
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
    title = NULL;
    message = NULL;

    for (i = 1; i < objc; i += 2) {
  char *choice;
  int index, choiceLen;
  char *string;

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
    choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
    /* empty strings should be like no selection given */
    if (choiceLen &&
      HandleInitialDirectory(interp, NULL, choice, &dirRef,
        NULL, &initialDesc) != TCL_OK) {
        result = TCL_ERROR;
        goto end;
    }
    break;
      case SAVE_INITFILE:
    initialFile = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
    /* empty strings should be like no selection given */
    if (choiceLen == 0) { initialFile = NULL; }
    break;
      case SAVE_MESSAGE:
    choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
    message = CFStringCreateWithBytes(NULL, (unsigned char*) choice, choiceLen,
      kCFStringEncodingUTF8, false);
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
    title = CFStringCreateWithBytes(NULL, (unsigned char*) choice, choiceLen,
      kCFStringEncodingUTF8, false);
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
    if (title != NULL) {
  CFRelease(title);
    }
    if (message != NULL) {
  CFRelease(message);
    }

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
    CFStringRef message, title;
    OpenFileData ofd;
    static CONST char *chooseOptionStrings[] = {
  "-initialdir", "-message", "-mustexist", "-parent", "-title", NULL
    };
    enum chooseOptions {
  CHOOSE_INITDIR, CHOOSE_MESSAGE, CHOOSE_MUSTEXIST,
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
    title = NULL;
    message = NULL;

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
    choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
    if (choiceLen &&
      HandleInitialDirectory(interp, NULL, choice, &dirRef,
        NULL, &initialDesc) != TCL_OK) {
        result = TCL_ERROR;
        goto end;
    }
    break;
      case CHOOSE_MESSAGE:
    choice = Tcl_GetStringFromObj(objv[i + 1], &choiceLen);
    message = CFStringCreateWithBytes(NULL, (unsigned char*) choice, choiceLen,
      kCFStringEncodingUTF8, false);
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
    title = CFStringCreateWithBytes(NULL, (unsigned char*) choice, choiceLen,
      kCFStringEncodingUTF8, false);
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
    if (title != NULL) {
  CFRelease(title);
    }
    if (message != NULL) {
  CFRelease(message);
    }

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

  err = FSPathMakeRef((unsigned char*) dirName,
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

  err = FSPathMakeRef((unsigned char*) namePtr, &fileRef, &isDirectory);
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
    char *initialFile,
    AEDescList *selectDescPtr,
    CFStringRef title,
    CFStringRef message,
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
    diagOptions.modality = kWindowModalityAppModal;

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
        filterPtr->name, kCFStringEncodingUTF8);
  }
  diagOptions.popupExtension = CFArrayCreate(NULL,
    (const void **) menuItemNames, ofdPtr->fl.numFilters, NULL);
    } else {
  diagOptions.optionFlags += kNavNoTypePopup;
  diagOptions.popupExtension = NULL;
    }

    /*
     * This is required to allow App packages to be selectable in the
     * file dialogs...
     */

    diagOptions.optionFlags += kNavSupportPackages;

    diagOptions.clientName = CFStringCreateWithCString(NULL, "Wish", kCFStringEncodingUTF8);
    diagOptions.message = message;
    diagOptions.windowTitle = title;
    if (initialFile) {
  diagOptions.saveFileName = CFStringCreateWithCString(NULL,
    initialFile, kCFStringEncodingUTF8);
    } else {
  diagOptions.saveFileName = NULL;
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
  if (err != noErr) {
#ifdef TK_MAC_DEBUG
      fprintf(stderr,"NavCreateGetFileDialog failed, %d\n", err);
#endif
      dialogRef = NULL;
  }
    } else if (isOpen == SAVE_FILE) {
  err = NavCreatePutFileDialog(&diagOptions, 'TEXT', 'WIsH',
    openFileEventUPP, NULL, &dialogRef);
  if (err!=noErr){
#ifdef TK_MAC_DEBUG
      fprintf(stderr,"NavCreatePutFileDialog failed, %d\n", err);
#endif
      dialogRef = NULL;
  }
    } else if (isOpen == CHOOSE_FOLDER) {
  err = NavCreateChooseFolderDialog(&diagOptions, openFileEventUPP,
    openFileFilterUPP, NULL, &dialogRef);
  if (err!=noErr){
#ifdef TK_MAC_DEBUG
      fprintf(stderr,"NavCreateChooseFolderDialog failed, %d\n", err);
#endif
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

  if ((err = NavDialogRun(dialogRef)) != noErr){
#ifdef TK_MAC_DEBUG
      fprintf(stderr,"NavDialogRun failed, %d\n", err);
#endif
  } else {
      if ((err = NavDialogGetReply(dialogRef, &theReply)) != noErr) {
#ifdef TK_MAC_DEBUG
    fprintf(stderr,"NavGetReply failed, %d\n", err);
#endif
      }
  }
    }

    /*
     * Most commands assume that the file dialogs return a single
     * item, not a list.  So only build a list if multiple is true...
     */
    if (err == noErr) {
  if (multiple) {
      theResult = Tcl_NewListObj(0, NULL);
  } else {
      theResult = Tcl_NewObj();
  }
  if (!theResult) {
      err = memFullErr;
  }
    }
    if (theReply.validRecord && err == noErr) {
  AEDesc resultDesc;
  long count;
  FSRef  fsRef;
  char   pathPtr[1024];
  int    pathValid = 0;
  err = AECountItems(&theReply.selection, &count);
  if (err == noErr) {
      long i;
      for (i = 1; i <= count; i++) {
    err = AEGetNthDesc(&theReply.selection,
        i, typeFSRef, NULL, &resultDesc);
    pathValid = 0;
    if (err == noErr) {
        if ((err = AEGetDescData(&resultDesc, &fsRef, sizeof(fsRef)))
          != noErr) {
#ifdef TK_MAC_DEBUG
      fprintf(stderr,"AEGetDescData failed %d\n", err);
#endif
        } else {
      if ((err = FSRefMakePath(&fsRef, (unsigned char*) pathPtr, 1024))) {
#ifdef TK_MAC_DEBUG
          fprintf(stderr,"FSRefMakePath failed, %d\n", err);
#endif
      } else {
          if (isOpen == SAVE_FILE) {
        CFStringRef saveNameRef;
        char saveName [1024];
        if ((saveNameRef = NavDialogGetSaveFileName(dialogRef))) {
            if (CFStringGetCString(saveNameRef, saveName,
              1024, kCFStringEncodingUTF8)) {
          if (strlen(pathPtr) + strlen(saveName) < 1023) {
              strcat(pathPtr, "/");
              strcat(pathPtr, saveName);
              pathValid = 1;
          } else {
#ifdef TK_MAC_DEBUG
              fprintf(stderr, "Path name too long\n");
#endif
          }
            } else {
#ifdef TK_MAC_DEBUG
          fprintf(stderr, "CFStringGetCString failed\n");
#endif
            }
        } else {
#ifdef TK_MAC_DEBUG
            fprintf(stderr, "NavDialogGetSaveFileName failed\n");
#endif
        }
          } else {
        pathValid = 1;
          }
          if (pathValid) {
        if (multiple) {
            Tcl_ListObjAppendElement(interp, theResult,
          Tcl_NewStringObj(pathPtr, -1));
        } else {
            Tcl_SetStringObj(theResult, pathPtr, -1);
        }
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
  for (i = 0;i < ofdPtr->fl.numFilters; i++) {
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
    NavFilterModes filterMode)
{
    OpenFileData *ofdPtr = (OpenFileData *) callBackUD;

    if (!ofdPtr || !ofdPtr->usePopup) {
  return true;
    } else {
  if (ofdPtr->fl.numFilters == 0) {
      return true;
  } else {
      if ((theItem->descriptorType == typeFSS)
        || (theItem->descriptorType = typeFSRef)) {
    NavFileOrFolderInfo* theInfo = (NavFileOrFolderInfo *) info;
    char fileName[256];
    int result;

    if (!theInfo->isFolder) {
        OSType fileType;
        StringPtr fileNamePtr;
        Tcl_DString fileNameDString;
        int i;
        FileFilter *filterPtr;

        fileType =
          theInfo->fileAndFolder.fileInfo.finderInfo.fdType;
        Tcl_DStringInit (&fileNameDString);

        if (theItem->descriptorType == typeFSS) {
      int len;
      fileNamePtr = (((FSSpec *) *theItem->dataHandle)->name);
      len = fileNamePtr[0];
      strncpy(fileName, (char*) fileNamePtr + 1, len);
      fileName[len] = '\0';
      fileNamePtr = (unsigned char*) fileName;

        } else if ((theItem->descriptorType = typeFSRef)) {
      OSStatus err;
      FSRef *theRef = (FSRef *) *theItem->dataHandle;
      HFSUniStr255 uniFileName;
      err = FSGetCatalogInfo (theRef, kFSCatInfoNone, NULL,
        &uniFileName, NULL, NULL);

      if (err == noErr) {
          Tcl_UniCharToUtfDString (
            (Tcl_UniChar *) uniFileName.unicode,
            uniFileName.length,
            &fileNameDString);
          fileNamePtr = (unsigned char*) Tcl_DStringValue(&fileNameDString);
      } else {
          fileNamePtr = NULL;
      }
        }
        if (ofdPtr->usePopup) {
      i = ofdPtr->curType;
      for (filterPtr = ofdPtr->fl.filters;
        filterPtr && i > 0; i--) {
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
       * We are not using the popup menu. In this case, the
       * file is considered matched if it matches any of
       * the file filters.
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

        Tcl_DStringFree (&fileNameDString);
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
    NavCallBackUserData callBackUD)
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
      || (callBackSelector & otherEvent) != 0) {
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
    OSType    fileType,         /* Type of the file, 0 means there was no specified type.  */
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

    for (clausePtr = filterPtr->clauses; clausePtr;
      clausePtr = clausePtr->next) {
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

  for (globPtr = clausePtr->patterns; globPtr;
    globPtr = globPtr->next) {
      char *q, *ext;

      if (fileNamePtr == NULL) {
    continue;
      }
      ext = globPtr->pattern;

      if (ext[0] == '\0') {
    /*
     * We don't want any extensions: OK if the filename doesn't
     * have "." in it
     */

    for (q = (char*) fileNamePtr; *q; q++) {
        if (*q == '.') {
      goto glob_unmatched;
        }
    }
    goto glob_matched;
      }

      if (Tcl_StringMatch((char*) fileNamePtr, ext)) {
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

  for (mfPtr = clausePtr->macTypes; mfPtr; mfPtr = mfPtr->next) {
      if (fileType == mfPtr->type) {
    macMatched = 1;
    break;
      }
  }

  /*
   * On Mac OS X, it is not uncommon for files to have NO
   * file type.  But folks with Tcl code on Classic MacOS pretty
   * much assume that a generic file will have type TEXT.  So
   * if we were strict about matching types when the source file
   * had NO type set, they would have to add another rule always
   * with no fileType.  To avoid that, we pass the macMatch side
   * of the test if no fileType is set.
   */

  if (globMatched && (macMatched || (fileType == 0))) {
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

    windowRef = GetDialogWindow(aboutDlog);
    SelectWindow(windowRef);

    while (itemHit != 1) {
  ModalDialog(NULL, &itemHit);
    }
    DisposeDialog(aboutDlog);
    aboutDlog = NULL;

    SelectWindow(ActiveNonFloatingWindow());

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MessageBoxObjCmd --
 *
 *    Implements the tk_messageBox in native Mac OS X style.
 *
 * Results:
 *    A standard Tcl result.
 *
 * Side effects:
 *    none
 *
 *----------------------------------------------------------------------
 */

int
Tk_MessageBoxObjCmd(
    ClientData  clientData,      /* Main window associated with interpreter. */
    Tcl_Interp  *interp,    /* Current interpreter. */
    int         objc,        /* Number of arguments. */
    Tcl_Obj     *CONST objv[])  /* Argument objects. */
{
    Tk_Window         tkwin = (Tk_Window) clientData;
    AlertStdCFStringAlertParamRec paramCFStringRec;
    AlertType               alertType;
    DialogRef        dialogRef;
    CFStringRef       messageTextCF = NULL;
    CFStringRef       finemessageTextCF = NULL;
    OSErr                   osError;
    SInt16                  itemHit;
    Boolean                 haveDefaultOption = false;
    Boolean                 haveParentOption = false;
    char                    *str;
    int                     index;
    int                     defaultButtonIndex;
    int                     defaultNativeButtonIndex;    /* 1, 2, 3: right to left. */
    int                     typeIndex;
    int                     i;
    int                     indexDefaultOption = 0;
    int                     result = TCL_OK;

    static CONST char *movableAlertStrings[] = {
  "-default",/* "-detail",*/ "-icon",
  "-message", "-parent",
  "-title", "-type",
  (char *)NULL
    };
    static CONST char *movableTypeStrings[] = {
  "abortretryignore", "ok",
  "okcancel", "retrycancel",
  "yesno", "yesnocancel",
  (char *)NULL
    };
    static CONST char *movableButtonStrings[] = {
  "abort", "retry", "ignore",
  "ok", "cancel", "yes", "no",
  (char *)NULL
    };
    static CONST char *movableIconStrings[] = {
  "error", "info", "question", "warning",
  (char *)NULL
    };
    enum movableAlertOptions {
  ALERT_DEFAULT,/* ALERT_DETAIL,*/ ALERT_ICON,
  ALERT_MESSAGE, ALERT_PARENT,
  ALERT_TITLE, ALERT_TYPE
    };
    enum movableTypeOptions {
  TYPE_ABORTRETRYIGNORE, TYPE_OK,
  TYPE_OKCANCEL, TYPE_RETRYCANCEL,
  TYPE_YESNO, TYPE_YESNOCANCEL
    };
    enum movableButtonOptions {
  TEXT_ABORT, TEXT_RETRY, TEXT_IGNORE,
  TEXT_OK, TEXT_CANCEL, TEXT_YES, TEXT_NO
    };
    enum movableIconOptions {
  ICON_ERROR, ICON_INFO, ICON_QUESTION, ICON_WARNING
    };

    /*
     * Need to map from 'movableButtonStrings' and its corresponding integer index,
     * to the native button index, which is 1, 2, 3, from right to left.
     * This is necessary to do for each separate '-type' of button sets.
     */

    short   buttonIndexAndTypeToNativeButtonIndex[][7] = {
  /*  abort retry ignore ok   cancel yes   no    */
  {1,    2,    3,    0,    0,    0,    0},        /*  abortretryignore */
  {0,    0,    0,    1,    0,    0,    0},        /*  ok */
  {0,    0,    0,    1,    2,    0,    0},        /*  okcancel */
  {0,    1,    0,    0,    2,    0,    0},        /*  retrycancel */
  {0,    0,    0,    0,    0,    1,    2},        /*  yesno */
  {0,    0,    0,    0,    3,    1,    2},        /*  yesnocancel */
    };

    /*
     * Need also the inverse mapping, from native button (1, 2, 3) to the
     * descriptive button text string index.
     */

    short   nativeButtonIndexAndTypeToButtonIndex[][4] = {
  {-1, 0, 1, 2},        /*  abortretryignore */
  {-1, 3, 0, 0},        /*  ok */
  {-1, 3, 4, 0},        /*  okcancel */
  {-1, 1, 4, 0},        /*  retrycancel */
  {-1, 5, 6, 0},        /*  yesno */
  {-1, 5, 6, 4},        /*  yesnocancel */
    };

    alertType = kAlertPlainAlert;
    typeIndex = TYPE_OK;

    GetStandardAlertDefaultParams(&paramCFStringRec, kStdCFStringAlertVersionOne);
    paramCFStringRec.movable = true;
    paramCFStringRec.helpButton = false;
    paramCFStringRec.defaultButton = kAlertStdAlertOKButton;
    paramCFStringRec.cancelButton = kAlertStdAlertCancelButton;

    for (i = 1; i < objc; i += 2) {
  int     iconIndex;
  char    *string;

  if (Tcl_GetIndexFromObj(interp, objv[i], movableAlertStrings, "option",
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

      case ALERT_DEFAULT:

      /*
       * Need to postpone processing of this option until we are
       * sure to know the '-type' as well.
       */

      haveDefaultOption = true;
      indexDefaultOption = i;
      break;

/*
      case ALERT_DETAIL:
      str = Tcl_GetStringFromObj(objv[i + 1], NULL);
      finemessageTextCF = CFStringCreateWithCString(NULL, str, kCFStringEncodingUTF8);
      break;
*/

      case ALERT_ICON:
      /*  not sure about UTF translation here... */
      if (Tcl_GetIndexFromObj(interp, objv[i + 1], movableIconStrings,
            "value", TCL_EXACT, &iconIndex) != TCL_OK) {
    result = TCL_ERROR;
    goto end;
      }
      switch (iconIndex) {
    case ICON_ERROR:
    alertType = kAlertStopAlert;
    break;
    case ICON_INFO:
    alertType = kAlertNoteAlert;
    break;
    case ICON_QUESTION:
    alertType = kAlertCautionAlert;
    break;
    case ICON_WARNING:
    alertType = kAlertCautionAlert;
    break;
      }
      break;

      case ALERT_MESSAGE:
      str = Tcl_GetStringFromObj(objv[i + 1], NULL);
      messageTextCF = CFStringCreateWithCString(NULL, str, kCFStringEncodingUTF8);
      break;

      case ALERT_PARENT:
      str = Tcl_GetStringFromObj(objv[i + 1], NULL);
      tkwin = Tk_NameToWindow(interp, str, tkwin);
      if (tkwin == NULL) {
    result = TCL_ERROR;
    goto end;
      }
      haveParentOption = true;
      break;

      case ALERT_TITLE:
      break;

      case ALERT_TYPE:
      /*  not sure about UTF translation here... */
      if (Tcl_GetIndexFromObj(interp, objv[i + 1], movableTypeStrings,
            "value", TCL_EXACT, &typeIndex) != TCL_OK) {
    result = TCL_ERROR;
    goto end;
      }
      switch (typeIndex) {
    case TYPE_ABORTRETRYIGNORE:
    paramCFStringRec.defaultText = CFSTR("Abort");
    paramCFStringRec.cancelText = CFSTR("Retry");
    paramCFStringRec.otherText = CFSTR("Ignore");
    paramCFStringRec.cancelButton = kAlertStdAlertOtherButton;
    break;
    case TYPE_OK:
    paramCFStringRec.defaultText = CFSTR("OK");
    break;
    case TYPE_OKCANCEL:
    paramCFStringRec.defaultText = CFSTR("OK");
    paramCFStringRec.cancelText = CFSTR("Cancel");
    break;
    case TYPE_RETRYCANCEL:
    paramCFStringRec.defaultText = CFSTR("Retry");
    paramCFStringRec.cancelText = CFSTR("Cancel");
    break;
    case TYPE_YESNO:
    paramCFStringRec.defaultText = CFSTR("Yes");
    paramCFStringRec.cancelText = CFSTR("No");
    break;
    case TYPE_YESNOCANCEL:
    paramCFStringRec.defaultText = CFSTR("Yes");
    paramCFStringRec.cancelText = CFSTR("No");
    paramCFStringRec.otherText = CFSTR("Cancel");
    paramCFStringRec.cancelButton = kAlertStdAlertOtherButton;
    break;
      }
      break;
  }
    }

    if (haveDefaultOption) {

  /*
   * Any '-default' option needs to know the '-type' option, which is why
   * we do this here.
   */

  str = Tcl_GetStringFromObj(objv[indexDefaultOption + 1], NULL);
  if (Tcl_GetIndexFromObj(interp, objv[indexDefaultOption + 1],
        movableButtonStrings, "value", TCL_EXACT,
        &defaultButtonIndex) != TCL_OK) {
      result = TCL_ERROR;
      goto end;
  }

  /* Need to map from "ok" etc. to 1, 2, 3, right to left. */

  defaultNativeButtonIndex =
  buttonIndexAndTypeToNativeButtonIndex[typeIndex][defaultButtonIndex];
  if (defaultNativeButtonIndex == 0) {
      Tcl_SetObjResult(interp,
           Tcl_NewStringObj("Illegal default option", -1));
      result = TCL_ERROR;
      goto end;
  }
  paramCFStringRec.defaultButton = defaultNativeButtonIndex;
    }
    SetThemeCursor(kThemeArrowCursor);

    if (haveParentOption) {
  TkWindow       *winPtr;
  WindowRef      windowRef;
  EventTargetRef     notifyTarget;
  EventHandlerUPP    handler;
  CallbackUserData  data;
  const EventTypeSpec kEvents[] = {
      {kEventClassCommand, kEventProcessCommand}
  };

  winPtr = (TkWindow *) tkwin;

  /*
   * Create the underlying Mac window for this Tk window.
   */

  windowRef = GetWindowFromPort(
      TkMacOSXGetDrawablePort(Tk_WindowId(tkwin)));
  notifyTarget = GetWindowEventTarget(windowRef);
  osError = CreateStandardSheet(alertType, messageTextCF,
              finemessageTextCF, &paramCFStringRec,
              notifyTarget, &dialogRef);
  if(osError != noErr) {
      result = TCL_ERROR;
      goto end;
  }
  data.windowRef = windowRef;
  data.buttonIndex = 1;
  handler = NewEventHandlerUPP(AlertHandler);
  InstallEventHandler(notifyTarget, handler,
    GetEventTypeCount(kEvents),
    kEvents, &data, NULL);
  osError = ShowSheetWindow(GetDialogWindow(dialogRef), windowRef);
  if(osError != noErr) {
      result = TCL_ERROR;
      goto end;
  }
  osError = RunAppModalLoopForWindow(windowRef);
  if (osError != noErr) {
      result = TCL_ERROR;
      goto end;
  }
  itemHit = data.buttonIndex;
  DisposeEventHandlerUPP(handler);
    } else {
  osError = CreateStandardAlert(alertType, messageTextCF,
              finemessageTextCF, &paramCFStringRec, &dialogRef);
  if(osError != noErr) {
      result = TCL_ERROR;
      goto end;
  }
  osError = RunStandardAlert(dialogRef, NULL, &itemHit);
  if (osError != noErr) {
      result = TCL_ERROR;
      goto end;
  }
    }
    if(osError == noErr) {
  int     ind;

  /*
   * Map 'itemHit' (1, 2, 3) to descriptive text string.
   */

  ind = nativeButtonIndexAndTypeToButtonIndex[typeIndex][itemHit];
  Tcl_SetObjResult(interp,
       Tcl_NewStringObj(movableButtonStrings[ind], -1));
    } else {
  result = TCL_ERROR;
    }

    end:
    if (finemessageTextCF != NULL) {
  CFRelease(finemessageTextCF);
    }
    if (messageTextCF != NULL) {
  CFRelease(messageTextCF);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * AlertHandler --
 *
 *  Carbon event handler for the Standard Sheet dialog.
 *
 * Results:
 *  OSStatus if event handled or not.
 *
 * Side effects:
 *  May set userData.
 *
 *----------------------------------------------------------------------
 */

static OSStatus
AlertHandler(EventHandlerCallRef callRef, EventRef eventRef, void *userData)
{
    OSStatus    result = eventNotHandledErr;
    HICommand    cmd;
    CallbackUserData  *dataPtr = (CallbackUserData *) userData;

    GetEventParameter(eventRef, kEventParamDirectObject, typeHICommand,
          NULL, sizeof(cmd), NULL, &cmd);
    switch (cmd.commandID) {
  case kHICommandOK:
  dataPtr->buttonIndex = 1;
  result = noErr;
  break;
  case kHICommandCancel:
  dataPtr->buttonIndex = 2;
  result = noErr;
  break;
  case kHICommandOther:
  dataPtr->buttonIndex = 3;
  result = noErr;
  break;
    }
    if (result == noErr) {
  result = QuitAppModalLoopForWindow(dataPtr->windowRef);
    }
    return result;
}
