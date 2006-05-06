/*
 * tkMacOSXEntry.c --
 *
 *      This file implements functions that decode & handle keyboard events
 *      on MacOS X.
 *
 *      Copyright 2001, Apple Computer, Inc.
 *
 *      The following terms apply to all files originating from Apple
 *      Computer, Inc. ("Apple") and associated with the software
 *      unless explicitly disclaimed in individual files.
 *
 *
 *      Apple hereby grants permission to use, copy, modify,
 *      distribute, and license this software and its documentation
 *      for any purpose, provided that existing copyright notices are
 *      retained in all copies and that this notice is included
 *      verbatim in any distributions. No written agreement, license,
 *      or royalty fee is required for any of the authorized
 *      uses. Modifications to this software may be copyrighted by
 *      their authors and need not follow the licensing terms
 *      described here, provided that the new terms are clearly
 *      indicated on the first page of each file where they apply.
 *
 *
 *      IN NO EVENT SHALL APPLE, THE AUTHORS OR DISTRIBUTORS OF THE
 *      SOFTWARE BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL,
 *      INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF
 *      THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
 *      EVEN IF APPLE OR THE AUTHORS HAVE BEEN ADVISED OF THE
 *      POSSIBILITY OF SUCH DAMAGE.  APPLE, THE AUTHORS AND
 *      DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
 *      BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 *      FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS
 *      SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND APPLE,THE
 *      AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *      MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *      GOVERNMENT USE: If you are acquiring this software on behalf
 *      of the U.S. government, the Government shall have only
 *      "Restricted Rights" in the software and related documentation
 *      as defined in the Federal Acquisition Regulations (FARs) in
 *      Clause 52.227.19 (c) (2).  If you are acquiring the software
 *      on behalf of the Department of Defense, the software shall be
 *      classified as "Commercial Computer Software" and the
 *      Government shall have only "Restricted Rights" as defined in
 *      Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 *      foregoing, the authors grant the U.S. Government and others
 *      acting in its behalf permission to use and distribute the
 *      software in accordance with the terms specified in this
 *      license.
 *
 * RCS: @(#) Id
 */

#include "tkMacOSXInt.h"
#include "tkMacOSXDefault.h"
#include "tkEntry.h"

static ThemeButtonKind ComputeIncDecParameters (int height, int *width);

/*
 *--------------------------------------------------------------
 *
 * ComputeIncDecParameters --
 *
 *  This procedure figures out which of the kThemeIncDec
 *      buttons to use.  It also sets width to the width of the
 *      IncDec button.
 *
 * Results:
 *  The ThemeButtonKind of the button we should use.
 *
 * Side effects:
 *  May draw the entry border into pixmap.
 *
 *--------------------------------------------------------------
 */
static ThemeButtonKind
ComputeIncDecParameters (int height, int *width)
{
    static int version = 0;
    
    if (version == 0) {
        Gestalt(gestaltSystemVersion, (long *) &version);
    }
    
    /* 
     * The small and mini incDec buttons were introduced in 10.3.
     */
    #ifndef kThemeIncDecButtonSmall
    #define kThemeIncDecButtonSmall 21
    #endif
    #ifndef kThemeIncDecButtonMini
    #define kThemeIncDecButtonMini 22
    #endif
     
    if (version >= 0x1030) {
        if (height < 11 || height > 28) {
            *width = 0;
            return (ThemeButtonKind) 0;
        }
        
        if (height >= 21) {
            *width = 13;
            return kThemeIncDecButton;
        } else if (height >= 18) {
            *width = 12;
            return kThemeIncDecButtonSmall;
        } else {
            *width = 11;
            return kThemeIncDecButtonMini;
        }
    } else {
        if (height < 21 || height > 28) {
            *width = 0;
            return (ThemeButtonKind) 0;
        }
        *width = 13;
        return kThemeIncDecButton;
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkpDrawEntryBorderAndFocus --
 *
 *  This procedure redraws the border of an entry window.
 *      It overrides the generic border drawing code if the 
 *      entry widget parameters are such that the native widget
 *      drawing is a good fit.
 *      This version just returns 1, so platforms that don't
 *      do special native drawing don't have to implement it.
 *
 * Results:
 *  1 if it has drawn the border, 0 if not.
 *
 * Side effects:
 *  May draw the entry border into pixmap.
 *
 *--------------------------------------------------------------
 */
int
TkpDrawEntryBorderAndFocus(Entry *entryPtr, Drawable d, int isSpinbox)
{
    Rect bounds;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    GC bgGC;
    Tk_Window tkwin = entryPtr->tkwin;
    ThemeDrawState drawState;
    int oldWidth = 0;

    /* 
     * I use 6 as the borderwidth.  2 of the 5 go into the actual frame the
     * 3 are because the Mac OS Entry widgets leave more space around the
     * Text than Tk does on X11.
     */
     
    if (entryPtr->borderWidth != MAC_OSX_ENTRY_BORDER 
            || entryPtr->highlightWidth != MAC_OSX_FOCUS_WIDTH
            ||entryPtr->relief != MAC_OSX_ENTRY_RELEIF) {
        return 0;
    }
    
    destPort = TkMacOSXGetDrawablePort(d);

    /*
     * For the spinbox, we have to make the entry part smaller by the size
     * of the buttons.  We also leave 2 pixels to the left (as per the HIG)
     * and space for one pixel to the right, 'cause it makes the buttons look
     * nicer. 
     */
     
    if (isSpinbox) {
        ThemeButtonKind buttonKind;
        int incDecWidth;
        
        oldWidth = Tk_Width(tkwin);
        
        buttonKind = ComputeIncDecParameters(Tk_Height(tkwin) 
                - 2 * MAC_OSX_FOCUS_WIDTH, &incDecWidth);
        Tk_Width(tkwin) -= incDecWidth + 1;
    }

   /*
    * The focus ring is drawn with an Alpha at the outside
    * part of the ring, so we have to draw over the edges of the
    * ring before drawing the focus or the text will peep through.
    */
    
    bgGC = Tk_GCForColor(entryPtr->highlightBgColorPtr, d);
    TkDrawInsetFocusHighlight(entryPtr->tkwin, bgGC, MAC_OSX_FOCUS_WIDTH, d, 0);
    
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    
    /*
     * Inset the entry Frame by the maximum width of the focus rect, 
     * which is 3 according to the Carbon docs.
     */
     
    bounds.top = MAC_OSX_FOCUS_WIDTH;
    bounds.left = MAC_OSX_FOCUS_WIDTH;
    bounds.right = Tk_Width(tkwin) - MAC_OSX_FOCUS_WIDTH;
    bounds.bottom = Tk_Height(tkwin) - MAC_OSX_FOCUS_WIDTH;
    if (entryPtr->state == STATE_DISABLED) {
        drawState = kThemeStateInactive;
    } else {
        drawState = kThemeStateActive;
    }
    DrawThemeEditTextFrame(&bounds, drawState);
    if (entryPtr->flags & GOT_FOCUS) {
        /* 
         * Don't call this if we don't have the focus, because then it
         * erases the focus rect to white, but we've already drawn the
         * highlightbackground above.  
         */

        DrawThemeFocusRect(&bounds, (entryPtr->flags & GOT_FOCUS) != 0);
    }
    SetGWorld(saveWorld, saveDevice);
    
    if (isSpinbox) {
        Tk_Width(tkwin) = oldWidth;
    }
    return 1;
}
/*
 *--------------------------------------------------------------
 *
 * TkpDrawSpinboxButtons --
 *
 *  This procedure redraws the buttons of an spinbox widget.
 *      It overrides the generic button drawing code if the 
 *      spinbox widget parameters are such that the native widget
 *      drawing is a good fit.
 *      This version just returns 0, so platforms that don't
 *      do special native drawing don't have to implement it.
 *
 * Results:
 *  1 if it has drawn the border, 0 if not.
 *
 * Side effects:
 *  May draw the entry border into pixmap.
 *
 *--------------------------------------------------------------
 */

int
TkpDrawSpinboxButtons(Spinbox *sbPtr, Drawable d)
{
    OSStatus err;
    Rect inBounds;
    ThemeButtonKind inKind;
    ThemeButtonDrawInfo inNewInfo;
    ThemeButtonDrawInfo * inPrevInfo = NULL;
    ThemeEraseUPP inEraseProc = NULL;
    ThemeButtonDrawUPP inLabelProc = NULL;
    UInt32 inUserData = 0;
    Tk_Window tkwin = sbPtr->entry.tkwin;
    int height = Tk_Height(tkwin);
    int buttonHeight = height - 2 * MAC_OSX_FOCUS_WIDTH;
    int incDecWidth;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    XRectangle rects[1];
    GC bgGC;

    /* FIXME RAISED really makes more sense */
    if (sbPtr->buRelief != TK_RELIEF_FLAT) {
        return 0;
    }
    
    /* 
     * The actual sizes of the IncDec button are 21 for the normal,
     * 18 for the small and 15 for the mini.  But the spinbox still
     * looks okay if the entry is a little bigger than this, so we
     * give it a little slop.
     */
     
    inKind = ComputeIncDecParameters(buttonHeight, &incDecWidth);
    if (inKind == (ThemeButtonKind) 0) {
        return 0;
    }
    
    destPort = TkMacOSXGetDrawablePort(d);
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    if (sbPtr->entry.state == STATE_DISABLED) {
        inNewInfo.state = kThemeStateInactive;
        inNewInfo.value = kThemeButtonOff;
    } else if (sbPtr->selElement == SEL_BUTTONUP) {
        inNewInfo.state = kThemeStatePressedUp;
        inNewInfo.value = kThemeButtonOn;
    } else if (sbPtr->selElement == SEL_BUTTONDOWN) {
        inNewInfo.state = kThemeStatePressedDown;
        inNewInfo.value = kThemeButtonOn;
    } else {
        inNewInfo.state = kThemeStateActive;
        inNewInfo.value = kThemeButtonOff;
    }
    
    inNewInfo.adornment = kThemeAdornmentNone;

    inBounds.left = Tk_Width(tkwin) - incDecWidth - 1;
    inBounds.right = Tk_Width(tkwin) - 1;
    inBounds.top = MAC_OSX_FOCUS_WIDTH;
    inBounds.bottom = Tk_Height(tkwin) - MAC_OSX_FOCUS_WIDTH;
    
    /* We had to make the entry part of the window smaller so that we
     * wouldn't overdraw the spin buttons with the focus highlight.  SO
     * now we have to draw the highlightbackground.
     */
     
    bgGC = Tk_GCForColor(sbPtr->entry.highlightBgColorPtr, d);
    rects[0].x = inBounds.left;
    rects[0].y = 0;
    rects[0].width = Tk_Width(tkwin);
    rects[0].height = Tk_Height(tkwin);
    XFillRectangles(Tk_Display(tkwin), d, bgGC, rects, 1);
    
    err =  DrawThemeButton (&inBounds, inKind, &inNewInfo, inPrevInfo,
            inEraseProc, inLabelProc, inUserData);

    SetGWorld(saveWorld, saveDevice);

    return 1;
}

