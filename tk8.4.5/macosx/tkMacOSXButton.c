/* 
 * tkMacOSXButton.c --
 *
 *        This file implements the Macintosh specific portion of the
 *        button widgets.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkButton.h"
#include "tkMacOSXInt.h"
#include "tkMacOSXDebug.h"

#define DEFAULT_USE_TK_TEXT 0

/*
 * Default insets for controls
 */
#define DEF_INSET_LEFT 2
#define DEF_INSET_RIGHT 2
#define DEF_INSET_TOP 2
#define DEF_INSET_BOTTOM 4

#include <Carbon/Carbon.h>

/*
 * Some defines used to control what type of control is drawn.
 */

#define DRAW_LABEL	0	/* Labels are treated genericly. */
#define DRAW_CONTROL	1	/* Draw using the Native control. */
#define DRAW_CUSTOM	2	/* Make our own button drawing. */
#define DRAW_BEVEL	3

/*  
 * Declaration of Mac specific button structure.
 */

typedef struct {
    SInt16 initialValue;
    SInt16 minValue;
    SInt16 maxValue;
    SInt16 procID;
    int	   isBevel;
} MacControlParams;

typedef struct {
    int drawType;
    Tk_3DBorder border;
    int relief;
    int offset;			/* 0 means this is a normal widget.  1 means
				 * it is an image button, so we offset the
				 * image to make the button appear to move
				 * up and down as the relief changes. */
    GC	gc;
    int hasImageOrBitmap;
} DrawParams;


typedef struct {
    TkButton                 info;       /* generic button info */
    int                      id;
    int                      usingControl;
    int                      useTkText;
    int                      flags;     /* initialisation status */
    MacControlParams         params;
    WindowRef                windowRef;
    RGBColor                 userPaneBackground;
    ControlRef               userPane;  /* Carbon control */
    ControlRef               control;   /* Carbon control */
    Str255                   controlTitle;
    ControlFontStyleRec      fontStyle;
    /* 
     * the following are used to store the image content for
     * beveled buttons - i.e. buttons with images.
     */
    CCTabHandle              tabHandle;
    ControlButtonContentInfo bevelButtonContent;
    OpenCPicParams           picParams;
    Pixmap                   picPixmap;
} MacButton;

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

static void ButtonEventProc _ANSI_ARGS_(( ClientData clientData, XEvent *eventPtr));
static int UpdateControlColors _ANSI_ARGS_((MacButton *mbPtr ));
static void TkMacOSXComputeControlParams _ANSI_ARGS_((TkButton * butPtr, MacControlParams * paramsPtr));
static int TkMacOSXComputeDrawParams _ANSI_ARGS_((TkButton * butPtr, DrawParams * dpPtr));
static void TkMacOSXDrawControl _ANSI_ARGS_((MacButton *butPtr,
        GWorldPtr destPort, GC gc, Pixmap pixmap));
static void SetupBevelButton _ANSI_ARGS_((MacButton *butPtr,
        ControlRef controlHandle, 
        GWorldPtr destPort, GC gc, Pixmap pixmap));

extern int TkFontGetFirstTextLayout(Tk_TextLayout layout, Tk_Font * font, char * dst); 
extern void TkMacOSXInitControlFontStyle(Tk_Font tkfont,ControlFontStylePtr fsPtr);

/*
 * The class procedure table for the button widgets.
 */

Tk_ClassProcs tkpButtonProcs = { 
    sizeof(Tk_ClassProcs),        /* size */
    TkButtonWorldChanged,        /* worldChangedProc */
};

static int bCount;

int tkPictureIsOpen;

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateButton --
 *
 *        Allocate a new TkButton structure.
 *
 * Results:
 *        Returns a newly allocated TkButton structure.
 *
 * Side effects:
 *        Registers an event handler for the widget.
 *
 *----------------------------------------------------------------------
 */

TkButton *
TkpCreateButton(
    Tk_Window tkwin)
{
    MacButton *macButtonPtr;
    macButtonPtr = (MacButton *) ckalloc(sizeof(MacButton));
    Tk_CreateEventHandler(tkwin, ActivateMask,
            ButtonEventProc, (ClientData) macButtonPtr);
    macButtonPtr->id=bCount++;
    macButtonPtr->usingControl=0;
    macButtonPtr->flags=0;
    macButtonPtr->userPaneBackground.red=0;
    macButtonPtr->userPaneBackground.green=0;
    macButtonPtr->userPaneBackground.blue=~0;
    macButtonPtr->userPane=NULL;
    macButtonPtr->control=NULL;
    macButtonPtr->controlTitle[0]=
    macButtonPtr->controlTitle[1]=0;
    macButtonPtr->picParams.version = -2;
    macButtonPtr->picParams.hRes = 0x00480000;
    macButtonPtr->picParams.vRes = 0x00480000;
    macButtonPtr->picParams.srcRect.top = 0;
    macButtonPtr->picParams.srcRect.left = 0;
    macButtonPtr->picParams.reserved1 = 0;
    macButtonPtr->picParams.reserved2 = 0;
    macButtonPtr->bevelButtonContent.contentType = kControlContentPictHandle;
    bzero(&macButtonPtr->params, sizeof(macButtonPtr->params));
    bzero(&macButtonPtr->fontStyle,sizeof(macButtonPtr->fontStyle));
    return (TkButton *)macButtonPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayButton --
 *
 *        This procedure is invoked to display a button widget.  It is
 *        normally invoked as an idle handler.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Commands are output to X to display the button in its
 *        current mode.  The REDRAW_PENDING flag is cleared.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayButton(
    ClientData clientData)        /* Information about widget. */
{
    MacButton *macButtonPtr = (MacButton *)clientData;
    TkButton  *butPtr       = (TkButton *) clientData;
    Tk_Window tkwin         = butPtr->tkwin;
    int x = 0;                  /* Initialization only needed to stop
                                 * compiler warning. */
    int y;
    int width, height, fullWidth, fullHeight;
    int textXOffset, textYOffset;
    int haveImage = 0, haveText = 0;
    GWorldPtr destPort;
    int borderWidth;
    Pixmap pixmap;
    int wasUsingControl;
    int imageWidth, imageHeight;
    int imageXOffset = 0, imageYOffset = 0; /* image information that will
					     * be used to restrict disabled
					     * pixmap as well */
    DrawParams drawParams, * dpPtr = &drawParams;

    butPtr->flags &= ~REDRAW_PENDING;
    if ((butPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
        return;
    }

    pixmap = (Pixmap) Tk_WindowId(tkwin);
    wasUsingControl = macButtonPtr->usingControl;

    if (TkMacOSXComputeDrawParams(butPtr, &drawParams) ) {
        macButtonPtr->usingControl=1;
        macButtonPtr->useTkText=DEFAULT_USE_TK_TEXT;
    } else {
        macButtonPtr->usingControl=0;
        macButtonPtr->useTkText=1;
    }
   
    /* 
     * set up clipping region 
     */
    
    TkMacOSXSetUpClippingRgn(pixmap);

    /*
     * See the comment in UpdateControlColors as to why we use the 
     * highlightbackground for the border of Macintosh buttons.
     */

    if (macButtonPtr->useTkText) {
        if (butPtr->type == TYPE_BUTTON) {
            Tk_Fill3DRectangle(tkwin, pixmap, butPtr->highlightBorder, 0, 0,
                Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);
        } else {
            Tk_Fill3DRectangle(tkwin, pixmap, butPtr->normalBorder, 0, 0,
                Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);
        }
    }

    /*
     * Draw the native portion of the buttons. Start by creating the control
     * if it doesn't already exist.  Then configure the Macintosh control from
     * the Tk info.  Finally, we call Draw1Control to draw to the screen.
     */

    if (macButtonPtr->usingControl) {
        borderWidth = 0;
        /*
         * This part uses Macintosh rather than Tk calls to draw
         * to the screen.  Make sure the ports etc. are set correctly.
         */
         
        destPort = TkMacOSXGetDrawablePort(pixmap);
        SetGWorld(destPort, NULL);
        TkMacOSXDrawControl(macButtonPtr, destPort, dpPtr->gc, pixmap);
    } else {
       if (wasUsingControl && macButtonPtr->userPane) {
           DisposeControl(macButtonPtr->userPane);
           macButtonPtr->userPane = NULL;
           macButtonPtr->control  = NULL;
           macButtonPtr->flags = 0;
       }
    }

    if ((dpPtr->drawType == DRAW_CUSTOM) || (dpPtr->drawType == DRAW_LABEL)) {
        borderWidth = butPtr->borderWidth;
    }

    /*
     * Display image or bitmap or text for button.  This has
     * already been done under Appearance with the Bevel
     * button types.
     */

    if (dpPtr->drawType == DRAW_BEVEL) {
        /* Empty Body */
    } else {
        if (butPtr->image != None) {
            Tk_SizeOfImage(butPtr->image, &width, &height);
            haveImage = 1;
        } else if (butPtr->bitmap != None) {
            Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
            haveImage = 1;
        }
	imageWidth  = width;
	imageHeight = height;

        haveText = (butPtr->textWidth != 0 && butPtr->textHeight != 0);
        if (butPtr->compound != COMPOUND_NONE && haveImage && haveText) {
            textXOffset = 0;
            textYOffset = 0;
            fullWidth = 0;
            fullHeight = 0;
            
            switch ((enum compound) butPtr->compound) {
                case COMPOUND_TOP: 
                case COMPOUND_BOTTOM: {
                    /* Image is above or below text */
                    if (butPtr->compound == COMPOUND_TOP) {
                        textYOffset = height + butPtr->padY;
                    } else {
                        imageYOffset = butPtr->textHeight + butPtr->padY;
                    }
                    fullHeight = height + butPtr->textHeight + butPtr->padY;
                    fullWidth = (width > butPtr->textWidth ? width :
                            butPtr->textWidth);
                    textXOffset = (fullWidth - butPtr->textWidth)/2;
                    imageXOffset = (fullWidth - width)/2;
                    break;
                }
                case COMPOUND_LEFT:
                case COMPOUND_RIGHT: {
                    /* 
                     * Image is left or right of text 
                     */
                     
                    if (butPtr->compound == COMPOUND_LEFT) {
                        textXOffset = width + butPtr->padX;
                    } else {
                        imageXOffset = butPtr->textWidth + butPtr->padX;
                    }
                    fullWidth = butPtr->textWidth + butPtr->padX + width;
                    fullHeight = (height > butPtr->textHeight ? height :
                            butPtr->textHeight);
                    textYOffset = (fullHeight - butPtr->textHeight)/2;
                    imageYOffset = (fullHeight - height)/2;
                    break;
                }
                case COMPOUND_CENTER: {
                    /* 
                     * Image and text are superimposed 
                     */
                     
                    fullWidth = (width > butPtr->textWidth ? width :
                            butPtr->textWidth);
                    fullHeight = (height > butPtr->textHeight ? height :
                            butPtr->textHeight);
                    textXOffset = (fullWidth - butPtr->textWidth)/2;
                    imageXOffset = (fullWidth - width)/2;
                    textYOffset = (fullHeight - butPtr->textHeight)/2;
                    imageYOffset = (fullHeight - height)/2;
                    break;
                }
                case COMPOUND_NONE: {break;}
            }
            
            TkComputeAnchor(butPtr->anchor, tkwin, butPtr->padX, butPtr->padY,
                    butPtr->indicatorSpace + fullWidth, fullHeight, &x, &y);
            
            x += butPtr->indicatorSpace;
            
            x += dpPtr->offset;
            y += dpPtr->offset;
            if (dpPtr->relief == TK_RELIEF_RAISED) {
                x -= dpPtr->offset;
                y -= dpPtr->offset;
            } else if (dpPtr->relief == TK_RELIEF_SUNKEN) {
                x += dpPtr->offset;
                y += dpPtr->offset;
            }
	    imageXOffset += x;
	    imageYOffset += y;
            if (butPtr->image != NULL) {
                if ((butPtr->selectImage != NULL) &&
                        (butPtr->flags & SELECTED)) {
                    Tk_RedrawImage(butPtr->selectImage, 0, 0,
                            width, height, pixmap, imageXOffset, imageYOffset);
                } else {
                    Tk_RedrawImage(butPtr->image, 0, 0, width,
                            height, pixmap, imageXOffset, imageYOffset);
                }
            } else {
                XSetClipOrigin(butPtr->display, dpPtr->gc,
			imageXOffset, imageYOffset);
                XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, dpPtr->gc,
                        0, 0, (unsigned int) width, (unsigned int) height,
			imageXOffset, imageYOffset, 1);
                XSetClipOrigin(butPtr->display, dpPtr->gc, 0, 0);
            }

            if (macButtonPtr->useTkText) {
                Tk_DrawTextLayout(butPtr->display, pixmap, 
                        dpPtr->gc, butPtr->textLayout,
                        x + textXOffset, y + textYOffset, 0, -1);
                Tk_UnderlineTextLayout(butPtr->display, pixmap, dpPtr->gc,
                        butPtr->textLayout, 
                        x + textXOffset, y + textYOffset,
                        butPtr->underline);
            }
            y += fullHeight/2;
        } else {
            if (haveImage) {
                TkComputeAnchor(butPtr->anchor, tkwin, 0, 0,
                        butPtr->indicatorSpace + width, height, &x, &y);
                x += butPtr->indicatorSpace;

                x += dpPtr->offset;
                y += dpPtr->offset;
                if (dpPtr->relief == TK_RELIEF_RAISED) {
                    x -= dpPtr->offset;
                    y -= dpPtr->offset;
                } else if (dpPtr->relief == TK_RELIEF_SUNKEN) {
                    x += dpPtr->offset;
                    y += dpPtr->offset;
                }
		imageXOffset += x;
		imageYOffset += y;
                if (butPtr->image != NULL) {
                    if ((butPtr->selectImage != NULL) &&
                            (butPtr->flags & SELECTED)) {
                        Tk_RedrawImage(butPtr->selectImage, 0, 0, width,
                                height, pixmap, imageXOffset, imageYOffset);
                    } else {
                        Tk_RedrawImage(butPtr->image, 0, 0, width, height,
                                pixmap, imageXOffset, imageYOffset);
                    }
                } else {
                    XSetClipOrigin(butPtr->display, dpPtr->gc, x, y);
                    XCopyPlane(butPtr->display, butPtr->bitmap, 
                            pixmap, dpPtr->gc,
                            0, 0, (unsigned int) width,
                            (unsigned int) height, x, y, 1);
                    XSetClipOrigin(butPtr->display, dpPtr->gc, 0, 0);
                }
                y += height/2;
            } else {
                TkComputeAnchor(butPtr->anchor, tkwin, butPtr->padX,
                        butPtr->padY,
                        butPtr->indicatorSpace + butPtr->textWidth,
                        butPtr->textHeight, &x, &y);
                
                x += butPtr->indicatorSpace;
                
                if (macButtonPtr->useTkText) {
                    Tk_DrawTextLayout(butPtr->display, pixmap, dpPtr->gc,
                        butPtr->textLayout, x, y, 0, -1);
                }
                y += butPtr->textHeight/2;
            }
        }
    }

    /*
     * If the button is disabled with a stipple rather than a special
     * foreground color, generate the stippled effect.  If the widget
     * is selected and we use a different background color when selected,
     * must temporarily modify the GC so the stippling is the right color.
     */

    if (macButtonPtr->useTkText) {
        if ((butPtr->state == STATE_DISABLED)
                && ((butPtr->disabledFg == NULL) || (butPtr->image != NULL))) {
            if ((butPtr->flags & SELECTED) && !butPtr->indicatorOn
		    && (butPtr->selectBorder != NULL)) {
                XSetForeground(butPtr->display, butPtr->stippleGC,
			Tk_3DBorderColor(butPtr->selectBorder)->pixel);
            }
	    /*
	     * Stipple the whole button if no disabledFg was specified,
	     * otherwise restrict stippling only to displayed image
	     */
	    if (butPtr->disabledFg == NULL) {
		XFillRectangle(butPtr->display, pixmap, butPtr->stippleGC,
			0, 0, (unsigned) Tk_Width(tkwin),
			(unsigned) Tk_Height(tkwin));
	    } else {
		XFillRectangle(butPtr->display, pixmap, butPtr->stippleGC,
			imageXOffset, imageYOffset,
			(unsigned) imageWidth, (unsigned) imageHeight);
	    }
            if ((butPtr->flags & SELECTED) && !butPtr->indicatorOn
		    && (butPtr->selectBorder != NULL)) {
		XSetForeground(butPtr->display, butPtr->stippleGC,
			Tk_3DBorderColor(butPtr->normalBorder)->pixel);
	    }
        }

        /*
         * Draw the border and traversal highlight last.  This way, if the
         * button's contents overflow they'll be covered up by the border.
         */
         
        if (dpPtr->relief != TK_RELIEF_FLAT) {
            int inset = butPtr->highlightWidth;
            Tk_Draw3DRectangle(tkwin, pixmap, dpPtr->border, inset, inset,
                Tk_Width(tkwin) - 2*inset, Tk_Height(tkwin) - 2*inset,
                butPtr->borderWidth, dpPtr->relief);
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpComputeButtonGeometry --
 *
 *        After changes in a button's text or bitmap, this procedure
 *        recomputes the button's geometry and passes this information
 *        along to the geometry manager for the window.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The button's window may change size.
 *
 *----------------------------------------------------------------------
 */

void
TkpComputeButtonGeometry(
    TkButton *butPtr)        /* Button whose geometry may have changed. */
{
    int width, height, avgWidth, haveImage = 0, haveText = 0;
    int xInset, yInset;
    int txtWidth, txtHeight;
    Tk_FontMetrics fm;
    DrawParams drawParams;

    /*
     * First figure out the size of the contents of the button.
     */
     
    width = 0;
    height = 0;
    txtWidth = 0;
    txtHeight = 0;
    avgWidth = 0;

     
    butPtr->indicatorSpace = 0;
    if (butPtr->image != NULL) {
        Tk_SizeOfImage(butPtr->image, &width, &height);
        haveImage = 1;
    } else if (butPtr->bitmap != None) {
        Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, &width, &height);
        haveImage = 1;
    }

    if (haveImage == 0 || butPtr->compound != COMPOUND_NONE) {
        Tk_FreeTextLayout(butPtr->textLayout);
        butPtr->textLayout = Tk_ComputeTextLayout(butPtr->tkfont,
                Tcl_GetString(butPtr->textPtr), -1, butPtr->wrapLength,
                butPtr->justify, 0, &butPtr->textWidth, &butPtr->textHeight);

        txtWidth = butPtr->textWidth;
        txtHeight = butPtr->textHeight;
        avgWidth = Tk_TextWidth(butPtr->tkfont, "0", 1);
        Tk_GetFontMetrics(butPtr->tkfont, &fm);
        haveText = (txtWidth != 0 && txtHeight != 0);
    }

    /*
     * If the button is compound (ie, it shows both an image and text),
     * the new geometry is a combination of the image and text geometry.
     * We only honor the compound bit if the button has both text and an
     * image, because otherwise it is not really a compound button.
     */

    if (butPtr->compound != COMPOUND_NONE && haveImage && haveText) {
        switch ((enum compound) butPtr->compound) {
            case COMPOUND_TOP:
            case COMPOUND_BOTTOM: {
                /* 
                 * Image is above or below text 
                 */
                 
                height += txtHeight + butPtr->padY;
                width = (width > txtWidth ? width : txtWidth);
                break;
            }
            case COMPOUND_LEFT:
            case COMPOUND_RIGHT: {
                /* 
                 * Image is left or right of text 
                 */
                  
                width += txtWidth + butPtr->padX;
                height = (height > txtHeight ? height : txtHeight);
                break;
            }
            case COMPOUND_CENTER: {
                /* 
                 * Image and text are superimposed 
                 */
                 
                width = (width > txtWidth ? width : txtWidth);
                height = (height > txtHeight ? height : txtHeight);
                break;
            }
            case COMPOUND_NONE: {break;}
        }
        if (butPtr->width > 0) {
            width = butPtr->width;
        }
        if (butPtr->height > 0) {
            height = butPtr->height;
        }

        if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
            butPtr->indicatorSpace = height;
            if (butPtr->type == TYPE_CHECK_BUTTON) {
                butPtr->indicatorDiameter = (65 * height)/100;
            } else {
                butPtr->indicatorDiameter = (75 * height)/100;
            }
        }

        width += 2 * butPtr->padX;
        height += 2 * butPtr->padY;

    } else {
        if (haveImage) {
            if (butPtr->width > 0) {
                width = butPtr->width;
            }
            if (butPtr->height > 0) {
                height = butPtr->height;
            }
            if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
                butPtr->indicatorSpace = height;
                if (butPtr->type == TYPE_CHECK_BUTTON) {
                    butPtr->indicatorDiameter = (65 * height)/100;
                } else {
                    butPtr->indicatorDiameter = (75 * height)/100;
                }
            }
        } else {
            width = txtWidth;
            height = txtHeight;
            if (butPtr->width > 0) {
                width = butPtr->width * avgWidth;
            }
            if (butPtr->height > 0) {
                height = butPtr->height * fm.linespace;
            }
            if ((butPtr->type >= TYPE_CHECK_BUTTON) && butPtr->indicatorOn) {
                butPtr->indicatorDiameter = fm.linespace;
                if (butPtr->type == TYPE_CHECK_BUTTON) {
                    butPtr->indicatorDiameter =
                        (80 * butPtr->indicatorDiameter)/100;
                }
                butPtr->indicatorSpace = butPtr->indicatorDiameter + avgWidth;
            }
        }
    }

    /*
     * Now figure out the size of the border decorations for the button.
     */
     
    if (butPtr->highlightWidth < 0) {
        butPtr->highlightWidth = 0;
    }
    
    /*
     * The width and height calculation for Appearance buttons with images & 
     * non-Appearance buttons with images is different.         In the latter case, 
     * we add the borderwidth to the inset, since we are going to stamp a
     * 3-D border over the image.  In the former, we add it to the height,
     * directly, since Appearance will draw the border as part of our control.
     *
     * When issuing the geometry request, add extra space for the indicator,
     * if any, and for the border and padding, plus if this is an image two 
     * extra pixels so the display can be offset by 1 pixel in either
     * direction for the raised or lowered effect.
     *
     * The highlight width corresponds to the default ring on the Macintosh.
     * As such, the highlight width is only added if the button is the default
     * button.        The actual width of the default ring is one less than the
     * highlight width as there is also one pixel of spacing.
     * Appearance buttons with images do not have a highlight ring, because the 
     * Bevel button type does not support one.
     */

    if ((butPtr->image == None) && (butPtr->bitmap == None)) {
        width += 2*butPtr->padX;
        height += 2*butPtr->padY;
    }
    
    if ((butPtr->type == TYPE_BUTTON)) {
        if ((butPtr->image == None) && (butPtr->bitmap == None)) {
            butPtr->inset = 0;
            if (butPtr->defaultState != STATE_DISABLED) {
                butPtr->inset += butPtr->highlightWidth;
            }
        } else  {
            butPtr->inset = 0;
            width += (2 * butPtr->borderWidth + 4);
            height += (2 * butPtr->borderWidth + 4);
        }
    } else if ((butPtr->type != TYPE_LABEL)) {
        if (butPtr->indicatorOn) {
            butPtr->inset = 0;
        } else {
            /*
             * Under Appearance, the Checkbutton or radiobutton with an image
             * is represented by a BevelButton with the Sticky defProc...  
             * So we must set its height in the same way as the Button 
             * with an image or bitmap.
             */
             
            if ( (butPtr->image != None) || (butPtr->bitmap != None)) {
                int border;
                butPtr->inset = 0;
                if ( butPtr->borderWidth <= 2 ) {
                    border = 6;
                }  else {
                    border = 2 * butPtr->borderWidth + 2;
                }               
                width += border;
                height += border;
            } else {
                butPtr->inset = butPtr->borderWidth;
            }        
        }        
    } else {
        butPtr->inset = butPtr->borderWidth;
    }

    if (TkMacOSXComputeDrawParams(butPtr,&drawParams)) {
        xInset = butPtr->indicatorSpace + DEF_INSET_LEFT + DEF_INSET_RIGHT;
        yInset = DEF_INSET_TOP + DEF_INSET_BOTTOM;
    } else {
        xInset = butPtr->indicatorSpace+butPtr->inset*2;
        yInset = butPtr->inset*2;
    }
    Tk_GeometryRequest(butPtr->tkwin, width + xInset, height + yInset);
    Tk_SetInternalBorder(butPtr->tkwin, butPtr->inset);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyButton --
 *
 *        Free data structures associated with the button control.
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
TkpDestroyButton(
    TkButton *butPtr)
{
    MacButton *mbPtr = ( MacButton *) butPtr; /* Mac button. */
    if (mbPtr->userPane) {
        DisposeControl(mbPtr->userPane);
        mbPtr->userPane = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXInitControl --
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
TkMacOSXInitControl (
    MacButton *mbPtr,                /* Mac button. */
    GWorldPtr  destPort,
    GC         gc,
    Pixmap     pixmap,
    Rect      *paneRect,
    Rect      *cntrRect
)
{
    OSErr      status;
    TkButton * butPtr = ( TkButton * )mbPtr;
    ControlRef rootControl;
    SInt16     procID;
    Boolean    initiallyVisible;
    SInt16     initialValue;
    SInt16     minValue;
    SInt16     maxValue;
    SInt32     controlReference;

    rootControl = TkMacOSXGetRootControl(Tk_WindowId(butPtr->tkwin));
    mbPtr->windowRef
            = GetWindowFromPort(TkMacOSXGetDrawablePort(Tk_WindowId(butPtr->tkwin)));

    /* 
     * Set up the user pane
     */

    initiallyVisible=false;
    initialValue=kControlSupportsEmbedding|   
        kControlHasSpecialBackground;
    minValue=0;
    maxValue=1;
    procID=kControlUserPaneProc;
    controlReference=(SInt32)mbPtr;
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
    initiallyVisible=false;
    TkMacOSXComputeControlParams(butPtr,&mbPtr->params);
    mbPtr->control=NewControl(mbPtr->windowRef,
        cntrRect, "\p",
        initiallyVisible,
        mbPtr->params.initialValue,
        mbPtr->params.minValue,
        mbPtr->params.maxValue,
        mbPtr->params.procID,
        controlReference );
        
    if (!mbPtr->control) {
        fprintf(stderr,"failed to create control of type %d\n",procID);
        return 1;
    }
    
    if (EmbedControl(mbPtr->control,mbPtr->userPane) != noErr ) {
        fprintf(stderr,"failed to embed control of type %d\n",procID);
        return 1;
    }
    
    mbPtr->flags|=(1 + 2);
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * TkMacOSXDrawControl --
 *
 *        This function draws the tk button using Mac controls
 *        In addition, this code may apply custom colors passed 
 *        in the TkButton.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *      The control is created, or reinitialised as needed
 *
 *
 *--------------------------------------------------------------
 */

static void
TkMacOSXDrawControl(
    MacButton *mbPtr,    /* Mac button. */
    GWorldPtr destPort,  /* Off screen GWorld. */
    GC gc,               /* The GC we are drawing into - needed for
                          * the bevel button */
    Pixmap pixmap)       /* The pixmap we are drawing into - needed
                          * for the bevel button */
                
{
    TkButton * butPtr = ( TkButton *)mbPtr;
    int        err;
    TkWindow * winPtr;
    Rect       paneRect;
    Rect       cntrRect;
    int        hilitePart = -1;


    winPtr=(TkWindow *)butPtr->tkwin;
   
    paneRect.left = winPtr->privatePtr->xOff;
    paneRect.top = winPtr->privatePtr->yOff;
    paneRect.right = paneRect.left + Tk_Width(butPtr->tkwin);
    paneRect.bottom = paneRect.top + Tk_Height(butPtr->tkwin);

    cntrRect=paneRect;

/*
    cntrRect.left+=butPtr->inset;
    cntrRect.top+=butPtr->inset;
    cntrRect.right-=butPtr->inset;
    cntrRect.bottom-=butPtr->inset;
*/
    cntrRect.left+=DEF_INSET_LEFT;
    cntrRect.top+=DEF_INSET_TOP;
    cntrRect.right-=DEF_INSET_RIGHT;
    cntrRect.bottom-=DEF_INSET_BOTTOM;

    /* 
     * The control has been previously initialised
     * It may need to be re-initialised 
     */
     
    if (mbPtr->flags) {
        MacControlParams params;
        TkMacOSXComputeControlParams(butPtr, &params);
        if (bcmp(&params, &mbPtr->params, sizeof(params))) {
            /* 
             * the type of control has changed
             * Clean it up and clear the flag 
             */
             
            if (mbPtr->userPane) {
                DisposeControl(mbPtr->userPane);
                mbPtr->userPane = NULL;
                mbPtr->control = NULL;
            }
            mbPtr->flags = 0;
        }
    }
    if (!(mbPtr->flags & 1)) {
        if (TkMacOSXInitControl(mbPtr, destPort, gc, 
                pixmap, &paneRect, &cntrRect) ) {
            return;
        }
    }
    SetControlBounds(mbPtr->userPane, &paneRect);
    SetControlBounds(mbPtr->control, &cntrRect);

    if (!mbPtr->useTkText) {
        Str255              controlTitle;
        ControlFontStyleRec fontStyle;
        Tk_Font    font;
        int        len;
        
        len = TkFontGetFirstTextLayout(butPtr->textLayout, 
                &font, controlTitle);
        controlTitle[len] = 0;
        if (bcmp(mbPtr->controlTitle, controlTitle, len+1)) {
            CFStringRef cf;    	    
            cf = CFStringCreateWithCString(NULL,
                  controlTitle, kCFStringEncodingUTF8);
            if (cf != NULL) {
            SetControlTitleWithCFString(mbPtr->control, cf);
            CFRelease(cf);
            }
            bcopy(controlTitle, mbPtr->controlTitle, len+1);
        }
        if (len) {
            TkMacOSXInitControlFontStyle(font, &fontStyle);
            if (bcmp(&mbPtr->fontStyle, &fontStyle, sizeof(fontStyle)) ) {
                if (SetControlFontStyle(mbPtr->control, &fontStyle) != noErr) {
                    fprintf(stderr,"SetControlFontStyle failed\n");
                }
                bcopy(&fontStyle, &mbPtr->fontStyle, 
                        sizeof(fontStyle));
            }
        }
    }
    if (mbPtr->params.isBevel) {
        /* Initialiase the image/button parameters */
        SetupBevelButton(mbPtr, mbPtr->control, destPort, 
                      gc, pixmap);
    }

    if (butPtr->flags & SELECTED) {
     SetControlValue(mbPtr->control, 1);
    } else {
     SetControlValue(mbPtr->control, 0);
    }
    
    if (!Tk_MacOSXIsAppInFront() || butPtr->state == STATE_DISABLED) {
        HiliteControl(mbPtr->control, kControlInactivePart);
    } else if (butPtr->state == STATE_ACTIVE) {
        if (mbPtr->params.isBevel) {
           HiliteControl(mbPtr->control, kControlButtonPart);
        } else {
            switch (butPtr->type) {
                case TYPE_BUTTON:
                    HiliteControl(mbPtr->control,  kControlButtonPart);
                    break;
                case TYPE_RADIO_BUTTON:
                    HiliteControl(mbPtr->control, kControlRadioButtonPart);
                    break;
                case TYPE_CHECK_BUTTON:
                    HiliteControl(mbPtr->control, kControlCheckBoxPart);
                    break;
            }
        }
    } else {
        HiliteControl(mbPtr->control, kControlNoPart);
    }
    UpdateControlColors(mbPtr);
        
    if ((butPtr->type == TYPE_BUTTON) ) {
        Boolean isDefault;
        
        if (butPtr->defaultState == STATE_ACTIVE) {
            isDefault = true;
        } else {
            isDefault = false;
        }
        if ((err=SetControlData(mbPtr->control, kControlNoPart, 
                kControlPushButtonDefaultTag,
                sizeof(isDefault), (Ptr) &isDefault)) != noErr ) {
        }
    }

    if (mbPtr->flags&2) {
        ShowControl(mbPtr->control);
        ShowControl(mbPtr->userPane);
        mbPtr->flags ^= 2;
    } else {
        Draw1Control(mbPtr->userPane);
        SetControlVisibility(mbPtr->control, true, true);
    }
 
    if (mbPtr->params.isBevel) {
        KillPicture(mbPtr->bevelButtonContent.u.picture);
    }         
}

/*
 *--------------------------------------------------------------
 *
 * SetupBevelButton --
 *
 *        Sets up the Bevel Button with image by copying the
 *        source image onto the PicHandle for the button.
 *
 * Results:
 *        None
 *
 * Side effects:
 *        The image or bitmap for the button is copied over to a picture.
 *
 *--------------------------------------------------------------
 */
void
SetupBevelButton(
    MacButton *mbPtr,                /* Mac button. */
    ControlRef controlHandle,         /* The control to set this picture to */
    GWorldPtr destPort,                /* Off screen GWorld. */
    GC gc,                        /* The GC we are drawing into - needed for
                                 * the bevel button */
    Pixmap pixmap                /* The pixmap we are drawing into - needed
                                   for the bevel button */
    )
{
    int       err;
    TkButton *butPtr = ( TkButton *)mbPtr;
    int height, width;
    ControlButtonGraphicAlignment theAlignment;
    
    SetPort(destPort);

    if (butPtr->image != None) {
        Tk_SizeOfImage(butPtr->image, 
                &width, &height);
    } else {
        Tk_SizeOfBitmap(butPtr->display, butPtr->bitmap, 
                &width, &height);
    }
            
    if ((butPtr->width > 0) && (butPtr->width < width)) {
        width = butPtr->width;
    }
    if ((butPtr->height > 0) && (butPtr->height < height)) {
        height = butPtr->height;
    }

    mbPtr->picParams.srcRect.right = width;
    mbPtr->picParams.srcRect.bottom = height;

    /* 
     * Set the flag to circumvent clipping and bounds problems with OS 10.0.4 
     */
     
    if (!(mbPtr->bevelButtonContent.u.picture 
            = OpenCPicture(&mbPtr->picParams)) ) {
        fprintf(stderr,"OpenCPicture failed\n");
    }
    tkPictureIsOpen = 1;
    
    /*
     * TO DO - There is one case where XCopyPlane calls CopyDeepMask,
     * which does not get recorded in the picture.  So the bitmap code
     * will fail in that case.
     */
     
    if ((butPtr->selectImage != NULL) && (butPtr->flags & SELECTED)) {
        Tk_RedrawImage(butPtr->selectImage, 0, 0, width, height,
                pixmap, 0, 0);
    } else if (butPtr->image != NULL) {
        Tk_RedrawImage(butPtr->image, 0, 0, width, 
                height, pixmap, 0, 0);
    } else {                        
        XSetClipOrigin(butPtr->display, gc, 0, 0);
        XCopyPlane(butPtr->display, butPtr->bitmap, pixmap, gc, 0, 0,
                (unsigned int) width, (unsigned int) height, 0, 0, 1);
    }
    
    ClosePicture();
    tkPictureIsOpen = 0;
    
    if ( (err=SetControlData(controlHandle, kControlButtonPart,
            kControlBevelButtonContentTag,
            sizeof(ControlButtonContentInfo),
            (char *) &mbPtr->bevelButtonContent)) != noErr ) {
        fprintf(stderr,
                "SetControlData BevelButtonContent failed, %d\n", err );
    }
            
    if (butPtr->anchor == TK_ANCHOR_N) {
        theAlignment = kControlBevelButtonAlignTop;
    } else if (butPtr->anchor == TK_ANCHOR_NE) { 
        theAlignment = kControlBevelButtonAlignTopRight;
    } else if (butPtr->anchor == TK_ANCHOR_E) { 
        theAlignment = kControlBevelButtonAlignRight;
    } else if (butPtr->anchor == TK_ANCHOR_SE) {
        theAlignment = kControlBevelButtonAlignBottomRight;
    } else if (butPtr->anchor == TK_ANCHOR_S) {
        theAlignment = kControlBevelButtonAlignBottom;
    } else if (butPtr->anchor == TK_ANCHOR_SW) {
        theAlignment = kControlBevelButtonAlignBottomLeft;
    } else if (butPtr->anchor == TK_ANCHOR_W) {
        theAlignment = kControlBevelButtonAlignLeft;
    } else if (butPtr->anchor == TK_ANCHOR_NW) {
        theAlignment = kControlBevelButtonAlignTopLeft;
    } else if (butPtr->anchor == TK_ANCHOR_CENTER) {
        theAlignment = kControlBevelButtonAlignCenter;
    }

    if ((err=SetControlData(controlHandle, kControlButtonPart,
            kControlBevelButtonGraphicAlignTag,
            sizeof(ControlButtonGraphicAlignment),
            (char *) &theAlignment)) != noErr ) {
        fprintf(stderr,
                "SetControlData BevelButtonGraphicAlign failed, %d\n", err );
    }

    if (butPtr->compound != COMPOUND_NONE) {
        ControlButtonTextPlacement thePlacement = \
                kControlBevelButtonPlaceNormally;
        if (butPtr->compound == COMPOUND_TOP) {
            thePlacement = kControlBevelButtonPlaceBelowGraphic;
        } else if (butPtr->compound == COMPOUND_BOTTOM) {
            thePlacement = kControlBevelButtonPlaceAboveGraphic;
        } else if (butPtr->compound == COMPOUND_LEFT) {
            thePlacement = kControlBevelButtonPlaceToRightOfGraphic;
        } else if (butPtr->compound == COMPOUND_RIGHT) {
            thePlacement = kControlBevelButtonPlaceToLeftOfGraphic;
        }
        if ((err=SetControlData(controlHandle, kControlButtonPart,
                kControlBevelButtonTextPlaceTag,
                sizeof(ControlButtonTextPlacement),
                (char *) &thePlacement)) != noErr ) {
            fprintf(stderr,
                    "SetControlData BevelButtonTextPlace failed, %d\n", err );
        }
    }
}

/*
 *--------------------------------------------------------------
 *
 * SetUserPaneDrawProc --
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
    MacButton * mbPtr;
    mbPtr = ( MacButton *)GetControlReference(control);
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
    MacButton * mbPtr;
    mbPtr = ( MacButton *)GetControlReference(control);
    if (info->colorDevice) {
        RGBBackColor (&mbPtr->userPaneBackground);
    }
}

/*
 *--------------------------------------------------------------
 *
 * UpdateControlColors --
 *
 *        This function will review the colors used to display
 *        a Macintosh button.  If any non-standard colors are
 *        used we create a custom palette for the button, populate
 *        with the colors for the button and install the palette.
 *
 *        Under Appearance, we just set the pointer that will be
 *        used by the UserPaneDrawProc.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        The Macintosh control may get a custom palette installed.
 *
 *--------------------------------------------------------------
 */

static int
UpdateControlColors(MacButton * mbPtr)
{
    XColor *xcolor;
    TkButton * butPtr = ( TkButton * )mbPtr;
    
    /*
     * Under Appearance we cannot change the background of the
     * button itself.  However, the color we are setting is the color
     * of the containing userPane.  This will be the color that peeks 
     * around the rounded corners of the button.  
     * We make this the highlightbackground rather than the background,
     * because if you color the background of a frame containing a
     * button, you usually also color the highlightbackground as well,
     * or you will get a thin grey ring around the button.
     */
      
    if (butPtr->type == TYPE_BUTTON) {
        xcolor = Tk_3DBorderColor(butPtr->highlightBorder);
    } else {
        xcolor = Tk_3DBorderColor(butPtr->normalBorder);
    }
    TkSetMacColor(xcolor->pixel, &mbPtr->userPaneBackground);
    
    return false;
}
/*
 *--------------------------------------------------------------
 *
 * ButtonEventProc --
 *
 *        This procedure is invoked by the Tk dispatcher for various
 *        events on buttons.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *      When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
ButtonEventProc(
    ClientData clientData,        /* Information about window. */
    XEvent *eventPtr)                /* Information about event. */
{
    TkButton *buttonPtr = (TkButton *) clientData;
    if (eventPtr->type == ActivateNotify
            || eventPtr->type == DeactivateNotify) {
        if ((buttonPtr->tkwin == NULL) 
                || (!Tk_IsMapped(buttonPtr->tkwin))) {
            return;
        }
        if ((buttonPtr->flags & REDRAW_PENDING) == 0) {
            Tcl_DoWhenIdle(TkpDisplayButton, (ClientData) buttonPtr);
            buttonPtr->flags |= REDRAW_PENDING;
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXComputeControlParams --
 *
 *        This procedure computes the various parameters used
 *        when creating a Carbon control (NewControl)
 *      These are determined by the various tk button parameters
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
TkMacOSXComputeControlParams(TkButton * butPtr, MacControlParams * paramsPtr )
{
    paramsPtr->isBevel = 0;
    
    /* 
     * Determine ProcID based on button type and dimensions 
     */
     
    switch (butPtr->type) {
        case TYPE_BUTTON:
            if ((butPtr->image == None) && (butPtr->bitmap == None)) {
                paramsPtr->initialValue = 1;
                paramsPtr->minValue = 0;
                paramsPtr->maxValue = 1;
                paramsPtr->procID = kControlPushButtonProc;
            } else {
                paramsPtr->initialValue = 0;
                paramsPtr->minValue = kControlBehaviorOffsetContents
                    | kControlContentPictHandle;
                paramsPtr->maxValue = 1;
                if (butPtr->borderWidth <= 2) {
                    paramsPtr->procID = kControlBevelButtonSmallBevelProc;
                } else if (butPtr->borderWidth == 3) {
                    paramsPtr->procID = kControlBevelButtonNormalBevelProc;
                } else {
                    paramsPtr->procID = kControlBevelButtonLargeBevelProc;
                }
                paramsPtr->isBevel = 1;                
            }
            break;
        case TYPE_RADIO_BUTTON:
            if (((butPtr->image == None) && (butPtr->bitmap == None))
                || (butPtr->indicatorOn)) {
                paramsPtr->initialValue = 1;
                paramsPtr->minValue = 0;
                paramsPtr->maxValue = 1;
                paramsPtr->procID = kControlRadioButtonProc;
            } else {
                paramsPtr->initialValue = 0;
                paramsPtr->minValue = kControlBehaviorOffsetContents|
                    kControlBehaviorSticky|
                    kControlContentPictHandle;
                paramsPtr->maxValue = 1;
                if (butPtr->borderWidth <= 2) {
                    paramsPtr->procID = kControlBevelButtonSmallBevelProc;
                } else if (butPtr->borderWidth == 3) {
                    paramsPtr->procID = kControlBevelButtonNormalBevelProc;
                } else {
                    paramsPtr->procID = kControlBevelButtonLargeBevelProc;
                }
                paramsPtr->isBevel = 1;                
            }
            break;
        case TYPE_CHECK_BUTTON:
            if (((butPtr->image == None) 
                    && (butPtr->bitmap == None))
                    || (butPtr->indicatorOn)) {
                paramsPtr->initialValue = 1;
                paramsPtr->minValue = 0;
                paramsPtr->maxValue = 1;
                paramsPtr->procID = kControlCheckBoxProc;
            } else {
                paramsPtr->initialValue = 0;
                paramsPtr->minValue = kControlBehaviorOffsetContents
                    | kControlBehaviorSticky
                    | kControlContentPictHandle;
                paramsPtr->maxValue = 1;
                if (butPtr->borderWidth <= 2) {
                    paramsPtr->procID = kControlBevelButtonSmallBevelProc;
                } else if (butPtr->borderWidth == 3) {
                    paramsPtr->procID = kControlBevelButtonNormalBevelProc;
                } else {
                    paramsPtr->procID = kControlBevelButtonLargeBevelProc;
                }
                paramsPtr->isBevel = 1;                
            }   
            break;
    }
}
/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXComputeDrawParams --
 *
 *        This procedure computes the various parameters used
 *        when drawing a button
 *      These are determined by the various tk button parameters
 *
 * Results:
 *        1 if control will be used, 0 otherwise.
 *
 * Side effects:
 *        Sets the button draw parameters
 *
 *----------------------------------------------------------------------
 */

static int
TkMacOSXComputeDrawParams(TkButton * butPtr, DrawParams * dpPtr)
{
    dpPtr->hasImageOrBitmap = ((butPtr->image != NULL) 
            || (butPtr->bitmap != None));
    dpPtr->offset = (butPtr->type == TYPE_BUTTON) 
            && dpPtr->hasImageOrBitmap;
    dpPtr->border = butPtr->normalBorder;
    if ((butPtr->state == STATE_DISABLED) 
            && (butPtr->disabledFg != NULL)) {
        dpPtr->gc = butPtr->disabledGC;
    } else if ((butPtr->type == TYPE_BUTTON)
            && (butPtr->state == STATE_ACTIVE)) {
        dpPtr->gc = butPtr->activeTextGC;
        dpPtr->border = butPtr->activeBorder;
    } else {
        dpPtr->gc = butPtr->normalTextGC;
    }

    if ((butPtr->flags & SELECTED) 
            && (butPtr->state != STATE_ACTIVE)
            && (butPtr->selectBorder != NULL) 
            && !butPtr->indicatorOn) {
       dpPtr->border = butPtr->selectBorder;
    }
    
    /*
     * Override the relief specified for the button if this is a
     * checkbutton or radiobutton and there's no indicator.
     * However, don't do this in the presence of Appearance, since
     * then the bevel button will take care of the relief.
     */

    dpPtr->relief = butPtr->relief;

    if ((butPtr->type >= TYPE_CHECK_BUTTON) && !butPtr->indicatorOn) {
        if (!dpPtr->hasImageOrBitmap) {
            dpPtr->relief = (butPtr->flags & SELECTED) ? TK_RELIEF_SUNKEN
                : TK_RELIEF_RAISED;
        }
    }

    /*
     * Determine the draw type
     */
    if (butPtr->type == TYPE_LABEL) {
        dpPtr->drawType = DRAW_LABEL;
    } else if (butPtr->type == TYPE_BUTTON) {
        if (!dpPtr->hasImageOrBitmap) {
            dpPtr->drawType = DRAW_CONTROL;
        } else if (butPtr->image != None) {
            dpPtr->drawType = DRAW_BEVEL;
        } else {
            /*
             * TO DO - The current way the we draw bitmaps (XCopyPlane)
             * uses CopyDeepMask in this one case.  The Picture recording 
             * does not record this call, and so we can't use the
             * Appearance bevel button here.  The only case that would
             * exercise this is if you use a bitmap, with
             * -data & -mask specified.         We should probably draw the 
             * appearance button and overprint the image in this case.
             * This just punts and draws the old-style, ugly, button.
             */
             
            if (dpPtr->gc->clip_mask == 0) {
                dpPtr->drawType = DRAW_BEVEL;
            } else {
                TkpClipMask *clipPtr = (TkpClipMask*) dpPtr->gc->clip_mask;
                if ((clipPtr->type == TKP_CLIP_PIXMAP) &&
                        (clipPtr->value.pixmap != butPtr->bitmap)) {
                    dpPtr->drawType = DRAW_CUSTOM;
                } else {
                    dpPtr->drawType = DRAW_BEVEL;
                }
            }
        }
    } else {
        if (butPtr->indicatorOn) {
            dpPtr->drawType = DRAW_CONTROL;
        } else if (dpPtr->hasImageOrBitmap) {
            if (dpPtr->gc->clip_mask == 0) {
                dpPtr->drawType = DRAW_BEVEL;
            } else {
                TkpClipMask *clipPtr = (TkpClipMask*) dpPtr->gc->clip_mask;
                if ((clipPtr->type == TKP_CLIP_PIXMAP) &&
                        (clipPtr->value.pixmap != butPtr->bitmap)) {
                    dpPtr->drawType = DRAW_CUSTOM;
                } else {
                    dpPtr->drawType = DRAW_BEVEL;
                }
            }
        } else {
            dpPtr->drawType = DRAW_CUSTOM;
        }
    }

    if ((dpPtr->drawType == DRAW_CONTROL) || (dpPtr->drawType == DRAW_BEVEL)) {
        return 1;
    } else {
        return 0;
    }
}
