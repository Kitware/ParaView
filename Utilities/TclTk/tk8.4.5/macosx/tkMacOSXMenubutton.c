/* 
 * tkMacOSXMenubutton.c --
 *
 *        This file implements the Macintosh specific portion of the
 *        menubutton widget.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <Carbon/Carbon.h>
#include "tkMenu.h"
#include "tkMenubutton.h"
#include "tkMacOSXInt.h"
#include "tkMacOSXDebug.h"

#define kShadowOffset   (3)     /* amount to offset shadow from frame */
#define kTriangleWidth  (11)    /* width of the triangle */
#define kTriangleHeight (6)     /* height of the triangle */
#define kTriangleMargin (5)     /* margin around triangle */

#define TK_POPUP_OFFSET 32      /* size of popup marker */

int TkMacOSXGetNewMenuID _ANSI_ARGS_((Tcl_Interp *interp, TkMenu *menuInstPtr, int cascade, short *menuIDPtr));
void TkMacOSXFreeMenuID _ANSI_ARGS_((short menuID));

typedef struct {
    SInt16 initialValue;
    SInt16 minValue;
    SInt16 maxValue;
    SInt16 procID;
    int    isBevel;
} MenuButtonControlParams;

typedef struct {
    int                     len;
    Str255                  title;
    ControlFontStyleRec     style;
} ControlTitleParams;

/*
 * Declaration of Mac specific button structure.
 */

typedef struct MacMenuButton {
    TkMenuButton info;         /* Generic button info. */
    WindowRef    windowRef;
    ControlRef   userPane;
    ControlRef   control;
    MenuRef      menuRef;
    RGBColor     userPaneBackground;
    MenuButtonControlParams  params;
    ControlTitleParams       titleParams;
    ControlButtonContentInfo bevelButtonContent;
    OpenCPicParams           picParams;
    int                      flags;
} MacMenuButton;

/*
 * Forward declarations for procedures defined later in this file:
 */

static OSErr SetUserPaneDrawProc(ControlRef control,
        ControlUserPaneDrawProcPtr upp);
static OSErr SetUserPaneSetUpSpecialBackgroundProc(ControlRef control,
        ControlUserPaneBackgroundProcPtr upp);
static void UserPaneDraw(ControlRef control, ControlPartCode cpc);
static void UserPaneBackgroundProc(ControlHandle,
        ControlBackgroundPtr info);
static int MenuButtonInitControl ( MacMenuButton *mbPtr, Rect *paneRect, Rect *cntrRect );

static int UpdateControlColors _ANSI_ARGS_((MacMenuButton *mbPtr ));
static void ComputeMenuButtonControlParams _ANSI_ARGS_((TkMenuButton * mbPtr, MenuButtonControlParams * paramsPtr));
static void ComputeControlTitleParams _ANSI_ARGS_((TkMenuButton * mbPtr, ControlTitleParams * paramsPtr));
static void CompareControlTitleParams(
    ControlTitleParams * p1Ptr,
    ControlTitleParams * p2Ptr,
    int * titleChanged,
    int * styleChanged
);

extern int TkFontGetFirstTextLayout(Tk_TextLayout layout, Tk_Font * font, char * dst); 
extern void TkMacOSXInitControlFontStyle(Tk_Font tkfont,ControlFontStylePtr fsPtr);

extern int tkPictureIsOpen;

/*
 * The structure below defines menubutton class behavior by means of
 * procedures that can be invoked from generic window code.
 */

Tk_ClassProcs tkpMenubuttonClass = {
    sizeof(Tk_ClassProcs),        /* size */
    TkMenuButtonWorldChanged,        /* worldChangedProc */
};

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateMenuButton --
 *
 *        Allocate a new TkMenuButton structure.
 *
 * Results:
 *        Returns a newly allocated TkMenuButton structure.
 *
 * Side effects:
 *        Registers an event handler for the widget.
 *
 *----------------------------------------------------------------------
 */

TkMenuButton *
TkpCreateMenuButton(
    Tk_Window tkwin)
{
    MacMenuButton *mbPtr = (MacMenuButton *) ckalloc(sizeof(MacMenuButton));
    mbPtr->userPaneBackground.red = 0;
    mbPtr->userPaneBackground.green = 0;
    mbPtr->userPaneBackground.blue = ~0;
    mbPtr->flags = 0;
    mbPtr->userPane = NULL;
    mbPtr->control = NULL;
    mbPtr->picParams.version = -2;
    mbPtr->picParams.hRes = 0x00480000;
    mbPtr->picParams.vRes = 0x00480000;
    mbPtr->picParams.srcRect.top = 0;
    mbPtr->picParams.srcRect.left = 0;
    mbPtr->picParams.reserved1 = 0;
    mbPtr->picParams.reserved2 = 0;
    mbPtr->bevelButtonContent.contentType = kControlContentPictHandle;
    mbPtr->menuRef = NULL;

    bzero(&mbPtr->params, sizeof(mbPtr->params));
    bzero(&mbPtr->titleParams,sizeof(mbPtr->titleParams));
    return (TkMenuButton *) mbPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayMenuButton --
 *
 *        This procedure is invoked to display a menubutton widget.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Commands are output to X to display the menubutton in its
 *        current mode.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayMenuButton(
    ClientData clientData)        /* Information about widget. */
{
    TkMenuButton *butPtr = (TkMenuButton *) clientData;
    Tk_Window tkwin = butPtr->tkwin;
    TkWindow *  winPtr;
    Pixmap      pixmap;
    MacMenuButton * mbPtr = (MacMenuButton *) butPtr;
    GWorldPtr dstPort;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    int      hasImageOrBitmap = 0;
    int      width, height;
    int      err;
    ControlButtonGraphicAlignment theAlignment;

    Rect paneRect, cntrRect;

    butPtr->flags &= ~REDRAW_PENDING;
    if ((butPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
        return;
    }
    pixmap = ( Pixmap )Tk_WindowId(tkwin);
    GetGWorld(&saveWorld, &saveDevice);
    dstPort = TkMacOSXGetDrawablePort(Tk_WindowId(tkwin));
    SetGWorld(dstPort, NULL);
    TkMacOSXSetUpClippingRgn(Tk_WindowId(tkwin));

    winPtr=(TkWindow *)butPtr->tkwin;
    paneRect.left=winPtr->privatePtr->xOff;
    paneRect.top=winPtr->privatePtr->yOff;
    paneRect.right=paneRect.left+Tk_Width(butPtr->tkwin)-1;
    paneRect.bottom=paneRect.top+Tk_Height(butPtr->tkwin)-1;
    
    cntrRect=paneRect;
        
    cntrRect.left+=butPtr->inset;
    cntrRect.top+=butPtr->inset;
    cntrRect.right-=butPtr->inset;
    cntrRect.bottom-=butPtr->inset;

    if (mbPtr->userPane) {
        MenuButtonControlParams params;
        bzero(&params, sizeof(params));
        ComputeMenuButtonControlParams(butPtr, &params );
        if (bcmp(&params,&mbPtr->params,sizeof(params))) {
            if (mbPtr->userPane) {
                DisposeControl(mbPtr->userPane);
                mbPtr->userPane = NULL;
                mbPtr->control = NULL;
            }
        }
     }
     if (!mbPtr->userPane) {
         if (MenuButtonInitControl(mbPtr,&paneRect,&cntrRect ) ) {
             fprintf(stderr,"Init Control failed\n" );
             return;
         }
     }
    SetControlBounds(mbPtr->userPane,&paneRect);
    SetControlBounds(mbPtr->control,&cntrRect); 

    /*
     * We need to cache the title and its style
     */
    if (!(mbPtr->flags&2)) {
        ControlTitleParams titleParams;
        int                titleChanged;
        int                styleChanged;
        ComputeControlTitleParams(butPtr,&titleParams);
        CompareControlTitleParams(&titleParams,&mbPtr->titleParams,
            &titleChanged,&styleChanged);
        if (titleChanged) {
            CFStringRef cf;    	    
            cf = CFStringCreateWithCString(NULL,
                  titleParams.title, kCFStringEncodingUTF8);
            if (hasImageOrBitmap) {
                SetControlTitleWithCFString(mbPtr->control, cf);
            } else {
                SetMenuItemTextWithCFString(mbPtr->menuRef, 1, cf);
            }
            CFRelease(cf);
            bcopy(titleParams.title,mbPtr->titleParams.title,titleParams.len+1);
            mbPtr->titleParams.len = titleParams.len;
        }
        if ((titleChanged||styleChanged) && titleParams .len) {
            if (hasImageOrBitmap) {
                if ((err=SetControlFontStyle(mbPtr->control,&titleParams.style))!=noErr) {
                    fprintf(stderr,"SetControlFontStyle failed %d\n", err);
                    return;
                }
            }
            bcopy(&titleParams.style,&mbPtr->titleParams.style,sizeof(titleParams.style));
        }
    }
    if (butPtr->image != None) {
        Tk_SizeOfImage(butPtr->image, &width, &height);
        hasImageOrBitmap = 1;
    } else if (butPtr->bitmap != None) {
        Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
        hasImageOrBitmap = 1;
    }
    if (hasImageOrBitmap) {
        mbPtr->picParams.srcRect.right = width;
        mbPtr->picParams.srcRect.bottom = height; 
        /* Set the flag to circumvent clipping and bounds problems with OS 10.0.4 */
        tkPictureIsOpen = 1;
        if (!(mbPtr->bevelButtonContent.u.picture = OpenCPicture(&mbPtr->picParams)) ) {
            fprintf(stderr,"OpenCPicture failed\n");
        }
        /*
         * TO DO - There is one case where XCopyPlane calls CopyDeepMask,
         * which does not get recorded in the picture.  So the bitmap code
         * will fail in that case.
         */
        if (butPtr->image != NULL) {
            Tk_RedrawImage(butPtr->image, 0, 0, width,
                height, pixmap, 0, 0);
        } else {   
            XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, NULL, 0, 0,
                (unsigned int) width, (unsigned int) height, 0, 0, 1);
        }
        ClosePicture();
        
        tkPictureIsOpen = 0;
        if ( (err=SetControlData(mbPtr->control, kControlButtonPart,
                    kControlBevelButtonContentTag,
                    sizeof(ControlButtonContentInfo),
                    (char *) &mbPtr->bevelButtonContent)) != noErr ) {
                fprintf(stderr,"SetControlData BevelButtonContent failed, %d\n", err );
        }
        switch (butPtr->anchor) {
            case TK_ANCHOR_N:
                theAlignment = kControlBevelButtonAlignTop;
                break;
            case TK_ANCHOR_NE:
                theAlignment = kControlBevelButtonAlignTopRight;
                break;
            case TK_ANCHOR_E:
                theAlignment = kControlBevelButtonAlignRight;
                break;
            case TK_ANCHOR_SE:
                theAlignment = kControlBevelButtonAlignBottomRight;
                break;
            case TK_ANCHOR_S:
                theAlignment = kControlBevelButtonAlignBottom;
                break;
            case TK_ANCHOR_SW:
                theAlignment = kControlBevelButtonAlignBottomLeft;
                break;
            case TK_ANCHOR_W:
                theAlignment = kControlBevelButtonAlignLeft;
                break;
            case TK_ANCHOR_NW:
                theAlignment = kControlBevelButtonAlignTopLeft;
                break;
            case TK_ANCHOR_CENTER:
                theAlignment = kControlBevelButtonAlignCenter;
                break;
        }
    
        if ((err=SetControlData(mbPtr->control, kControlButtonPart,
                kControlBevelButtonGraphicAlignTag,
                sizeof(ControlButtonGraphicAlignment),
                (char *) &theAlignment)) != noErr ) {
            fprintf(stderr,"SetControlData BevelButtonGraphicAlign failed, %d\n", err );
        }
    }
    if (butPtr->flags & GOT_FOCUS) {
        HiliteControl(mbPtr->control,kControlButtonPart);
    } else {
        HiliteControl(mbPtr->control,kControlNoPart);
    }
    UpdateControlColors(mbPtr);
    if (mbPtr->flags&2) {
        ShowControl(mbPtr->control);
        ShowControl(mbPtr->userPane);
        mbPtr->flags ^= 2;
    } else {
        Draw1Control(mbPtr->userPane);
        SetControlVisibility(mbPtr->control, true, true);
    }
    if (hasImageOrBitmap) {
        KillPicture(mbPtr->bevelButtonContent.u.picture);
    }
    SetGWorld(saveWorld, saveDevice);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyMenuButton --
 *
 *        Free data structures associated with the menubutton control.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Restores the default control state.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyMenuButton(
    TkMenuButton *mbPtr)
{
    MacMenuButton * macMbPtr = (MacMenuButton *)mbPtr;
    if (macMbPtr->userPane) {
        DisposeControl(macMbPtr->userPane);
        macMbPtr->userPane = NULL;
    }
    if (macMbPtr->menuRef) {
        short menuID;
        menuID = GetMenuID(macMbPtr->menuRef);
        TkMacOSXFreeMenuID(menuID);
        DisposeMenu(macMbPtr->menuRef);
        macMbPtr->menuRef = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpComputeMenuButtonGeometry --
 *
 *        After changes in a menu button's text or bitmap, this procedure
 *        recomputes the menu button's geometry and passes this information
 *        along to the geometry manager for the window.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The menu button's window may change size.
 *
 *----------------------------------------------------------------------
 */

void
TkpComputeMenuButtonGeometry(mbPtr)
    register TkMenuButton *mbPtr;                /* Widget record for menu button. */
{
    int width, height, mm, pixels;
    int hasImageOrBitmap = 0;

    mbPtr->inset = mbPtr->highlightWidth + mbPtr->borderWidth;
    if (mbPtr->image != None) {
        Tk_SizeOfImage(mbPtr->image, &width, &height);
        if (mbPtr->width > 0) {
            width = mbPtr->width;
        }
        if (mbPtr->height > 0) {
            height = mbPtr->height;
        }
        hasImageOrBitmap = 1;
    } else if (mbPtr->bitmap != None) {
        Tk_SizeOfBitmap(mbPtr->display, mbPtr->bitmap, &width, &height);
        if (mbPtr->width > 0) {
            width = mbPtr->width;
        }
        if (mbPtr->height > 0) {
            height = mbPtr->height;
        }
        hasImageOrBitmap = 1;
    } else {
        hasImageOrBitmap = 0;
        Tk_FreeTextLayout(mbPtr->textLayout);
        mbPtr->textLayout = Tk_ComputeTextLayout(mbPtr->tkfont, mbPtr->text,
                -1, mbPtr->wrapLength, mbPtr->justify, 0, &mbPtr->textWidth,
                &mbPtr->textHeight);
        width = mbPtr->textWidth;
        height = mbPtr->textHeight;
        if (mbPtr->width > 0) {
            width = mbPtr->width * Tk_TextWidth(mbPtr->tkfont, "0", 1);
        }
        if (mbPtr->height > 0) {
            Tk_FontMetrics fm;

            Tk_GetFontMetrics(mbPtr->tkfont, &fm);
            height = mbPtr->height * fm.linespace;
        }
        width += 2*mbPtr->padX;
        height += 2*mbPtr->padY;
    }

    if (mbPtr->indicatorOn) {
        mm = WidthMMOfScreen(Tk_Screen(mbPtr->tkwin));
        pixels = WidthOfScreen(Tk_Screen(mbPtr->tkwin));
        mbPtr->indicatorHeight= kTriangleHeight;
        mbPtr->indicatorWidth = kTriangleWidth + kTriangleMargin;
        width += mbPtr->indicatorWidth;
    } else {
        mbPtr->indicatorHeight = 0;
        mbPtr->indicatorWidth = 0;
    }
    if (!hasImageOrBitmap) {
        width += TK_POPUP_OFFSET;
    }

    Tk_GeometryRequest(mbPtr->tkwin, (int) (width + 2*mbPtr->inset),
            (int) (height + 2*mbPtr->inset));
    Tk_SetInternalBorder(mbPtr->tkwin, mbPtr->inset);
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeMenuButtonControlParams --
 *
 *        This procedure computes the various parameters used
 *        when creating a Carbon control (NewControl)
 *      These are determined by the various tk menu button parameters
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Sets the control initialisation parameters
 *
 *----------------------------------------------------------------------
 */

static void
ComputeMenuButtonControlParams(TkMenuButton * mbPtr, 
        MenuButtonControlParams * paramsPtr )
{
    int fakeMenuID = 256;

    /* 
     * Determine ProcID based on button type and dimensions
     *
     * We need to set minValue to some non-zero value,
     * Otherwise, the markers do not show up
     */

    paramsPtr->minValue = kControlBehaviorMultiValueMenu;
    paramsPtr->maxValue = 0;
    if (mbPtr->image || mbPtr->bitmap) {
        paramsPtr->isBevel = 1;
        if (mbPtr->borderWidth <= 2) {
            paramsPtr->procID = kControlBevelButtonSmallBevelProc;
        } else if (mbPtr->borderWidth == 3) {
            paramsPtr->procID = kControlBevelButtonNormalBevelProc;
        } else {
            paramsPtr->procID = kControlBevelButtonLargeBevelProc;
        }
        if (mbPtr->indicatorOn) {
            paramsPtr->initialValue = fakeMenuID;
        } else {
            paramsPtr->initialValue = 0;
        }
    } else {
        paramsPtr->isBevel = 0;
        paramsPtr->procID = kControlPopupButtonProc
                + kControlPopupVariableWidthVariant;
        paramsPtr->minValue = -12345;
        paramsPtr->maxValue = -1;
        paramsPtr->initialValue = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * returns 0 if same, 1 otherwise
 *----------------------------------------------------------------------
 */
static void
CompareControlTitleParams( 
    ControlTitleParams * p1Ptr,
    ControlTitleParams * p2Ptr,
    int * titleChanged,
    int * styleChanged
)
{
    if (p1Ptr->len != p2Ptr->len) {
         *titleChanged = 1;
    } else {
        if (bcmp(p1Ptr->title,p2Ptr->title,p1Ptr->len)) {
            *titleChanged = 1;
        } else {
            *titleChanged = 0;
        }
    }
    if (p1Ptr->len && p2Ptr->len) {
        *styleChanged = bcmp(&p1Ptr->style, &p2Ptr->style, sizeof(p2Ptr->style));
    } else {
        *styleChanged = p1Ptr->len||p2Ptr->len;
    }
}

static void
ComputeControlTitleParams(TkMenuButton * butPtr, ControlTitleParams * paramsPtr )
{
    Tk_Font font;
    paramsPtr->len =TkFontGetFirstTextLayout(butPtr->textLayout,&font, paramsPtr->title);
    paramsPtr->title [paramsPtr->len] = 0;
    if (paramsPtr->len) {
        TkMacOSXInitControlFontStyle(font,&paramsPtr->style);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * MenuButtonInitControl --
 *
 *        This procedure initialises a Carbon control
 *
 * Results:
 *        0 on success, 1 on failure.
 *
 * Side effects:
 *        A background pane control and the control itself is created
 *      The contol is embedded in the background control
 *      The background control is embedded in the root control
 *      of the containing window
 *      The creation parameters for the control are also computed
 *
 *----------------------------------------------------------------------
 */
int
MenuButtonInitControl (
    MacMenuButton *mbPtr,                /* Mac button. */
    Rect      *paneRect,
    Rect      *cntrRect
)
{
    OSErr      status;
    TkMenuButton * butPtr = ( TkMenuButton * )mbPtr;
    ControlRef rootControl;
    SInt16     procID;
    Boolean    initiallyVisible;
    SInt16     initialValue;
    SInt16     minValue;
    SInt16     maxValue;
    SInt32     controlReference;
    int        err;
    short      menuID;
    int        length;
    Str255     itemText;

    rootControl=TkMacOSXGetRootControl(Tk_WindowId(butPtr->tkwin));
    mbPtr->windowRef=GetWindowFromPort(TkMacOSXGetDrawablePort(Tk_WindowId(butPtr->tkwin)));
    /* 
     * Set up the user pane
     */
    initiallyVisible = false;
    initialValue = kControlSupportsEmbedding|
        kControlHasSpecialBackground;
    minValue = 0;
    maxValue = 1;
    procID = kControlUserPaneProc;
    controlReference = (SInt32)mbPtr;
    mbPtr->userPane=NewControl(mbPtr->windowRef,
        paneRect, "\p",
        initiallyVisible,
        initialValue,
        minValue,
        maxValue,
        procID,
        controlReference );
    if (!mbPtr->userPane) {
        fprintf(stderr,"Failed to create user pane control\n");
        return 1;
    }
    if ((status=EmbedControl(mbPtr->userPane,rootControl))!=noErr) {
        fprintf(stderr,"Failed to embed user pane control %d\n", status);
        return 1;
    }
    SetUserPaneSetUpSpecialBackgroundProc(mbPtr->userPane,
        UserPaneBackgroundProc);
    SetUserPaneDrawProc(mbPtr->userPane,UserPaneDraw);
    initiallyVisible = false;
    ComputeMenuButtonControlParams(butPtr,&mbPtr->params);
    /* Do this only if we are using bevel buttons */
    ComputeControlTitleParams(butPtr,&mbPtr->titleParams);
    mbPtr->control = NewControl(mbPtr->windowRef,
        cntrRect, "\p", //mbPtr->titleParams.title,
        initiallyVisible,
        mbPtr->params.initialValue,
        mbPtr->params.minValue,
        mbPtr->params.maxValue,
        mbPtr->params.procID,
        controlReference );
    if (!mbPtr->control) {
        fprintf(stderr,"failed to create control of type %d : line %d\n",mbPtr->params.procID, __LINE__);
        return 1;
    }
    if ((err=EmbedControl(mbPtr->control,mbPtr->userPane)) != noErr ) {
        fprintf(stderr,"failed to embed control of type %d,%d\n",procID, err);
        return 1;
    }
    if (mbPtr->params.isBevel) {
            CFStringRef cf;    	    
            cf = CFStringCreateWithCString(NULL,
                  mbPtr->titleParams.title, kCFStringEncodingUTF8);
        SetControlTitleWithCFString(mbPtr->control, cf);
        CFRelease(cf);
        if (mbPtr->titleParams.len) {
            if ((err=SetControlFontStyle(mbPtr->control,&mbPtr->titleParams.style))!=noErr) {
                fprintf(stderr,"SetControlFontStyle failed %d\n", err);
                return 1;
             }
        }
    } else {
            CFStringRef cf;    	    
        err = TkMacOSXGetNewMenuID(mbPtr->info.interp, (TkMenu *)mbPtr, 0, &menuID);
        if (err != TCL_OK) {
            return err;
        }       
        length = strlen(Tk_PathName(mbPtr->info.tkwin));
        memmove(&itemText[1], Tk_PathName(mbPtr->info.tkwin),
            (length > 230) ? 230 : length);
        itemText[0] = (length > 230) ? 230 : length;
        if (!(mbPtr->menuRef = NewMenu(menuID,itemText))) {
            return 1;
        }
            cf = CFStringCreateWithCString(NULL,
                  mbPtr->titleParams.title, kCFStringEncodingUTF8);
        AppendMenuItemText(mbPtr->menuRef, "\px");
        if (cf != NULL) {
        SetMenuItemTextWithCFString(mbPtr->menuRef, 1, cf);
        CFRelease(cf);
        }
        err = SetControlData(mbPtr->control,
            kControlNoPart,
            kControlPopupButtonMenuRefTag,
            sizeof(mbPtr->menuRef), &mbPtr->menuRef);
        SetControlMinimum(mbPtr->control, 1);
        SetControlMaximum(mbPtr->control, 1);
        SetControlValue(mbPtr->control, 1);
    }
    mbPtr->flags |= 2;
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * SetUserPane
 *
 *        Utility function to add a UserPaneDrawProc
 *        to a userPane control.        From MoreControls code
 *        from Apple DTS.
 *
 * Results:
 *        MacOS system error.
 *
 * Side effects:
 *        The user pane gets a new UserPaneDrawProc.
 *
 *--------------------------------------------------------------
 */
OSErr SetUserPaneDrawProc (
    ControlRef control,
    ControlUserPaneDrawProcPtr upp)
{
    ControlUserPaneDrawUPP myControlUserPaneDrawUPP;
    myControlUserPaneDrawUPP = NewControlUserPaneDrawUPP(upp);        
    return SetControlData (control, 
        kControlNoPart, kControlUserPaneDrawProcTag, 
        sizeof(myControlUserPaneDrawUPP), 
        (Ptr) &myControlUserPaneDrawUPP);
}

/*
 *--------------------------------------------------------------
 *
 * SetUserPaneSetUpSpecialBackgroundProc --
 *
 *        Utility function to add a UserPaneBackgroundProc
 *        to a userPane control
 *
 * Results:
 *        MacOS system error.
 *
 * Side effects:
 *        The user pane gets a new UserPaneBackgroundProc.
 *
 *--------------------------------------------------------------
 */
OSErr
SetUserPaneSetUpSpecialBackgroundProc(
    ControlRef control, 
    ControlUserPaneBackgroundProcPtr upp)
{
    ControlUserPaneBackgroundUPP myControlUserPaneBackgroundUPP;
    myControlUserPaneBackgroundUPP = NewControlUserPaneBackgroundUPP(upp);
    return SetControlData (control, kControlNoPart, 
        kControlUserPaneBackgroundProcTag, 
        sizeof(myControlUserPaneBackgroundUPP), 
        (Ptr) &myControlUserPaneBackgroundUPP);
}

/*
 *--------------------------------------------------------------
 *
 * UserPaneDraw --
 *
 *        This function draws the background of the user pane that will 
 *        lie under checkboxes and radiobuttons.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The user pane gets updated to the current color.
 *
 *--------------------------------------------------------------
 */
void
UserPaneDraw(
    ControlRef control,
    ControlPartCode cpc)
{
    Rect contrlRect;
    MacMenuButton * mbPtr;
    mbPtr = ( MacMenuButton *)GetControlReference(control);
    GetControlBounds(control,&contrlRect);
    RGBBackColor (&mbPtr->userPaneBackground);
    EraseRect (&contrlRect);
}

/*
 *--------------------------------------------------------------
 *
 * UserPaneBackgroundProc --
 *
 *        This function sets up the background of the user pane that will 
 *        lie under checkboxes and radiobuttons.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The user pane background gets set to the current color.
 *
 *--------------------------------------------------------------
 */

void
UserPaneBackgroundProc(
    ControlHandle control,
    ControlBackgroundPtr info)
{
    MacMenuButton * mbPtr;
    mbPtr = (MacMenuButton *)GetControlReference(control);
    if (info->colorDevice) {
        RGBBackColor (&mbPtr->userPaneBackground);
    }
}

/*   
 *--------------------------------------------------------------
 *  
 * UpdateControlColors --
 *      
 *      This function will review the colors used to display
 *      a Macintosh button.  If any non-standard colors are
 *      used we create a custom palette for the button, populate
 *      with the colors for the button and install the palette.
 *   
 *      Under Appearance, we just set the pointer that will be
 *      used by the UserPaneDrawProc.
 *      
 * Results:
 *      None.
 *  
 * Side effects:
 *      The Macintosh control may get a custom palette installed.
 *
 *--------------------------------------------------------------
 */

static int
UpdateControlColors(MacMenuButton * mbPtr)
{
    XColor *xcolor;
    TkMenuButton * butPtr = ( TkMenuButton * )mbPtr;
   
    /*
     * Under Appearance we cannot change the background of the
     * button itself.  However, the color we are setting is the color
     *  of the containing userPane.  This will be the color that peeks
     * around the rounded corners of the button.
     * We make this the highlightbackground rather than the background,
     * because if you color the background of a frame containing a
     * button, you usually also color the highlightbackground as well,
     * or you will get a thin grey ring around the button.
     */

    xcolor = Tk_3DBorderColor(butPtr->normalBorder);
    TkSetMacColor(xcolor->pixel, &mbPtr->userPaneBackground);
   
    return false;
}
