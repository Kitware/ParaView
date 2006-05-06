/*
 * tkMacOSXKeyEvent.c --
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
#include "tkMacOSXEvent.h"

typedef struct {
    WindowRef   whichWindow;
    Point       global;
    Point       local;
    unsigned int state;
    unsigned char ch;
    UInt32      keyCode;
    UInt32      keyModifiers;
    UInt32      message;  
} KeyEventData;

static Tk_Window gGrabWinPtr = NULL;     /* Current grab window,
                                          * NULL if no grab. */
static Tk_Window gKeyboardWinPtr = NULL; /* Current keyboard grab window. */

static UInt32 deadKeyStateUp = 0;        /* The deadkey state for the current
                                          * sequence of keyup events or 0 if
                                          * not in a deadkey sequence */
static UInt32 deadKeyStateDown = 0;      /* Ditto for keydown */

/*
 * Declarations for functions used only in this file.
 */
 
static int InitKeyData(
        KeyEventData * keyEventDataPtr);

static int InitKeyEvent(
        XEvent * eventPtr,
        KeyEventData * e, 
        UInt32 savedKeyCode,
        UInt32 savedModifiers);

static int GenerateKeyEvent (
        UInt32 eKind, 
        KeyEventData * e, 
        UInt32 savedKeyCode,
        UInt32 savedModifiers,
        const UniChar * chars, int numChars);

static int GetKeyboardLayout (
        Ptr * resourcePtr, TextEncoding * encodingPtr);
static TextEncoding GetKCHREncoding(
        ScriptCode script, SInt32 layoutid);

static int KeycodeToUnicodeViaUnicodeResource(
        UniChar * uniChars, int maxChars,
        Ptr uchr,
        EventKind eKind,
        UInt32 keycode, UInt32 modifiers,
        UInt32 * deadKeyStatePtr);

static int KeycodeToUnicodeViaKCHRResource(
        UniChar * uniChars, int maxChars,
        Ptr kchr, TextEncoding encoding,
        EventKind eKind,
        UInt32 keycode, UInt32 modifiers,
        UInt32 * deadKeyStatePtr);

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXProcessKeyboardEvent --
 *
 *        This routine processes the event in eventPtr, and
 *        generates the appropriate Tk events from it.
 *
 * Results:
 *        True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *        Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

int
TkMacOSXProcessKeyboardEvent(
        TkMacOSXEvent * eventPtr, 
        MacEventStatus * statusPtr)
{
    static UInt32 savedKeyCode = 0;
    static UInt32 savedModifiers = 0;
    static UniChar savedChar = 0;
    OSStatus     status;
    KeyEventData keyEventData;
    MenuRef   menuRef;
    MenuItemIndex menuItemIndex;
    int eventGenerated;
    UniChar uniChars[5]; /* make this larger, if needed */
    UInt32 uniCharsLen = 0;

    if (!InitKeyData(&keyEventData)) {
        statusPtr->err = 1;
        return false;
    }
    
    /*
     * Because of the way that Tk operates, we can't in general funnel menu
     * accelerators through IsMenuKeyEvent.  Tk treats accelerators as mere
     * decoration, and the user has to install bindings to get them to fire.
     *
     * However, the only way to trigger the Hide & Hide Others functions
     * is by invoking the Menu command for Hide.  So there is no nice way to
     * provide a Tk command to hide the app which would be available for a
     * binding.  So I am going to hijack Command-H and Command-Shift-H 
     * here, and run the menu commands.  Since the HI Guidelines explicitly
     * reserve these for Hide, this isn't such a bad thing.  Also, if you do
     * rebind Command-H to another menu item, Hide will lose its binding.
     * 
     * Note that I don't really do anything at this point, 
     * I just mark stopProcessing as 0 and return, and then the
     * RecieveAndProcessEvent code will dispatch the event to the default
     * handler.
     */

    if ((eventPtr->eKind == kEventRawKeyDown 
            || eventPtr->eKind == kEventRawKeyRepeat) 
            && IsMenuKeyEvent(NULL, eventPtr->eventRef, 
            kMenuEventQueryOnly, &menuRef, &menuItemIndex)) {
        MenuCommand menuCmd;
        
        GetMenuItemCommandID (menuRef, menuItemIndex, &menuCmd);
        
        switch (menuCmd) {
            case kHICommandHide:
            case kHICommandHideOthers:
            case kHICommandShowAll:
            case kHICommandPreferences:
                statusPtr->stopProcessing = 0;
                return 0; /* TODO: may not be on event on queue. */
                break;
            default:
                break;
        }
    }

    status = GetEventParameter(eventPtr->eventRef, 
            kEventParamKeyMacCharCodes,
            typeChar, NULL,
            sizeof(keyEventData.ch), NULL,
            &keyEventData.ch);
    if (status != noErr) {
#ifdef TK_MAC_DEBUG
        fprintf (stderr, "Failed to retrieve KeyMacCharCodes\n");
#endif
        statusPtr->err = 1;
        return false;
    } 
    status = GetEventParameter(eventPtr->eventRef, 
            kEventParamKeyCode,
            typeUInt32, NULL,
            sizeof(keyEventData.keyCode), NULL,
            &keyEventData.keyCode);
    if (status != noErr) {
#ifdef TK_MAC_DEBUG
        fprintf (stderr, "Failed to retrieve KeyCode\n");
#endif
        statusPtr->err = 1;
        return false;
    }
    status = GetEventParameter(eventPtr->eventRef, 
            kEventParamKeyModifiers,
            typeUInt32, NULL,
            sizeof(keyEventData.keyModifiers), NULL,
            &keyEventData.keyModifiers);
    if (status != noErr) {
#ifdef TK_MAC_DEBUG
        fprintf (stderr, "Failed to retrieve KeyModifiers\n");
#endif
        statusPtr->err = 1;
        return false;
    }

    switch (eventPtr->eKind) {
        case kEventRawKeyUp:
        case kEventRawKeyDown:
        case kEventRawKeyRepeat:
            {
                UInt32 *deadKeyStatePtr;

                if (kEventRawKeyDown == eventPtr->eKind) {
                    deadKeyStatePtr = &deadKeyStateDown;
                } else {
                    deadKeyStatePtr = &deadKeyStateUp;
                }

                uniCharsLen = TkMacOSXKeycodeToUnicode(
                        uniChars, sizeof(uniChars)/sizeof(*uniChars),
                        eventPtr->eKind,
                        keyEventData.keyCode, keyEventData.keyModifiers,
                        deadKeyStatePtr);
            }
    }

    if (kEventRawKeyUp == eventPtr->eKind) {
        /*
         * For some reason the deadkey processing for KeyUp doesn't work
         * sometimes, so we fudge and use the last detected KeyDown.
         */

        if((0 == uniCharsLen) && (0 != savedChar)) {
            uniChars[0] = savedChar;
            uniCharsLen = 1;
        }

        /*
         * Suppress keyup events while we have a deadkey sequence on keydown.
         * We still *do* want to collect deadkey state in this situation if
         * the system provides it, that's why we do this only after
         * TkMacOSXKeycodeToUnicode().
         */

        if (0 != deadKeyStateDown) {
            uniCharsLen = 0;
        }
    }

    keyEventData.message = keyEventData.ch|(keyEventData.keyCode << 8);

    eventGenerated = GenerateKeyEvent(
            eventPtr->eKind, &keyEventData,
            savedKeyCode, savedModifiers,
            uniChars, uniCharsLen);

    savedModifiers = keyEventData.keyModifiers;

    if ((kEventRawKeyDown == eventPtr->eKind) && (uniCharsLen > 0)) {
        savedChar = uniChars[0];
    } else {
        savedChar = 0;
    }
    
    statusPtr->stopProcessing = 1;

    if (eventGenerated == 0) {
        savedKeyCode = keyEventData.message;
        return false;
    } else if (eventGenerated == -1) {
        savedKeyCode = 0;
        statusPtr->stopProcessing = 0;
        return false;
    } else {
        savedKeyCode = 0;
        return true;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateKeyEvent --
 *
 *        Given Macintosh keyUp, keyDown & autoKey events (in their "raw"
 *        form) and a list of unicode characters this function generates the
 *        appropriate X key events.
 *
 *        Parameter eKind is a raw keyboard event.  e contains the data sent
 *        with the event. savedKeyCode and savedModifiers contain the values
 *        from the last event that came before (see
 *        TkMacOSXProcessKeyboardEvent()).  chars/numChars has the Unicode
 *        characters for which we want to create events.
 *
 * Results:
 *        1 if an event was generated, -1 for any error.
 *
 * Side effects:
 *        Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

static int
GenerateKeyEvent(
        UInt32 eKind, 
        KeyEventData * e, 
        UInt32 savedKeyCode,
        UInt32 savedModifiers,
        const UniChar * chars, int numChars)
{
    XEvent event;
    int i;
    
    if (-1 == InitKeyEvent(&event, e, savedKeyCode, savedModifiers)) {
        return -1;
    }

    if (kEventRawKeyModifiersChanged == eKind) {

        if (savedModifiers > e->keyModifiers) {
            event.xany.type = KeyRelease;
        } else {
            event.xany.type = KeyPress;
        }
        
        /* 
         * Use special '-1' to signify a special keycode to our
         * platform specific code in tkMacOSXKeyboard.c.  This is
         * rather like what happens on Windows.
         */
        
        event.xany.send_event = -1;

        /*
         * Set keycode (which was zero) to the changed modifier
         */
        
        event.xkey.keycode = (e->keyModifiers ^ savedModifiers);
        Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);

    } else {

        for (i = 0; i < numChars; ++i) {

            /*
             * Encode one char in the trans_chars array that was already
             * introduced for MS Windows.  Don't encode the string, if it is
             * a control character but was not generated with a real control
             * modifier.  Such control characters get generated by KeyTrans()
             * for special keys, but we rather want to identify those by
             * their KeySyms.
             */

            event.xkey.trans_chars[0] = 0;
            if ((controlKey & e->keyModifiers) || (chars[i] >= ' ')) {
                int done;
                done = Tcl_UniCharToUtf(chars[i],event.xkey.trans_chars);
                event.xkey.trans_chars[done] = 0;
            }

            switch(eKind) {
                case kEventRawKeyDown:
                    event.xany.type = KeyPress;
                    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
                    break;
                case kEventRawKeyUp:
                    event.xany.type = KeyRelease;
                    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
                    break;
                case kEventRawKeyRepeat:
                    event.xany.type = KeyRelease;
                    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
                    event.xany.type = KeyPress;
                    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
                    break;
                default:
#ifdef TK_MAC_DEBUG
                    fprintf (stderr,
                            "GenerateKeyEvent(): Invalid parameter eKind %d\n",
                            (int) eKind);
#endif
                    return -1;
            } 
        }
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * InitKeyData --
 *
 *        This routine initializes a KeyEventData structure by asking the OS
 *        and Tk for all the global information needed here.
 *
 * Results:
 *        True if the current front window can be found in Tk data structures
 *        - false otherwise.
 *
 * Side Effects:
 *        None
 *
 *----------------------------------------------------------------------
 */
static int 
InitKeyData(KeyEventData * keyEventDataPtr)
{
    int x, y;
    memset (keyEventDataPtr, 0, sizeof(*keyEventDataPtr));

    keyEventDataPtr->whichWindow = ActiveNonFloatingWindow();
    if (keyEventDataPtr->whichWindow == NULL) {
        return false;
    }
    XQueryPointer(NULL, None, NULL, NULL, &x, &y, 
            NULL, NULL, &keyEventDataPtr->state);
    keyEventDataPtr->global.v = x;
    keyEventDataPtr->global.v = y;
    keyEventDataPtr->local = keyEventDataPtr->global;
    GlobalToLocal(&keyEventDataPtr->local);

    return true;
}

/*
 *----------------------------------------------------------------------
 *
 * InitKeyEvent --
 *
 *        Initialize an XEvent structure by asking Tk for global information.
 *        Also uses a KeyEventData structure and other current state.
 *
 * Results:
 *        1 on success, -1 for any error.
 *
 * Side effects:
 *        Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

/* 
 * We have a general problem here.  How do we handle 'Option-char'
 * keypresses?  The problem is that we might want to bind to some of these
 * (e.g. Cmd-Opt-d is 'uncomment' in Alpha).  OTOH Option-d actually produces
 * a real character on MacOS, namely a mathematical delta.
 *
 * The current behaviour is that a binding goes by the combinations of
 * modifiers and base keysym, that is Option-d.  The string value of the
 * event is the mathematical delta character, so if no binding calls
 * [break], the text widget will insert that character.
 *
 * Note that this is similar to control combinations on all platforms.  They
 * also generate events that have the base character as keysym and a real
 * control character as character value.  So Ctrl+C gets us the keysym XK_C,
 * the modifier Control (so you can bind <Control-C>) and a string value as
 * "\u0003".
 * 
 * For a different solutions we may want for the event to contain keysyms for
 * *both* the 'Opt-d' side of things and the mathematical delta.  Then a
 * binding on Opt-d will trigger, but a binding on mathematical delta would
 * also trigger.  This would require changes in the core, though.
 */

static int
InitKeyEvent(
        XEvent * eventPtr,
        KeyEventData * e, 
        UInt32 savedKeyCode,
        UInt32 savedModifiers)
{
    Window window;
    Tk_Window tkwin;
    TkDisplay *dispPtr;
    
    /*
     * The focus must be in the FrontWindow on the Macintosh.
     * We then query Tk to determine the exact Tk window
     * that owns the focus.
     */

    window = TkMacOSXGetXWindow(e->whichWindow);
    dispPtr = TkGetDisplayList();
    tkwin = Tk_IdToWindow(dispPtr->display, window);
    
    if (tkwin == NULL) {
#ifdef TK_MAC_DEBUG
        fprintf(stderr,"tkwin == NULL, %d\n", __LINE__);
#endif
        return -1;
    }
    
    tkwin = (Tk_Window) ((TkWindow *) tkwin)->dispPtr->focusPtr;
    if (tkwin == NULL) {
#ifdef TK_MAC_DEBUG
        fprintf(stderr,"tkwin == NULL, %d\n", __LINE__);
#endif
        return -1;
    }

    eventPtr->xany.send_event = false;
    eventPtr->xany.serial = Tk_Display(tkwin)->request;

    eventPtr->xkey.same_screen = true;
    eventPtr->xkey.subwindow = None;
    eventPtr->xkey.time = TkpGetMS();
    eventPtr->xkey.x_root = e->global.h;
    eventPtr->xkey.y_root = e->global.v;
    eventPtr->xkey.window = Tk_WindowId(tkwin);
    eventPtr->xkey.display = Tk_Display(tkwin);
    eventPtr->xkey.root = XRootWindow(Tk_Display(tkwin), 0);
    eventPtr->xkey.state =  e->state;
    eventPtr->xkey.trans_chars[0] = 0;

    Tk_TopCoordsToWindow(
            tkwin, e->local.h, e->local.v, 
            &eventPtr->xkey.x, &eventPtr->xkey.y);

    eventPtr->xkey.keycode = e->ch |
        ((savedKeyCode & charCodeMask) << 8) |
        ((e->message&keyCodeMask) << 8);

    return 1;
}


/*
 * If we have old headers, we need to define these types and constants
 * ourself.  We use preprocessor macros instead of enums and typedefs,
 * because macros work even in case of version misunderstandings, while
 * duplicate enums and typedefs would give errrors.
 */

#if !defined(MAC_OS_X_VERSION_10_2) || \
    (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_2)
#define KeyboardLayoutRef Ptr
#define KeyboardLayoutPropertyTag UInt32
#define kKLKCHRData 0
#define kKLuchrData 1
#define kKLIdentifier 2
#endif

/*
 *----------------------------------------------------------------------
 *
 * GetKeyboardLayout --
 *
 *      Queries the OS for a pointer to a keyboard resource.
 *
 *      This function works with the keyboard layout switch menu.  It uses
 *      Keyboard Layout Services, where available.
 *
 * Results:
 *      1 if there is returned a Unicode 'uchr' resource in *resourcePtr, 0
 *      if it is a classic 'KCHR' resource.  A pointer to the actual resource
 *      data goes into *resourcePtr.  If the resource is a 'KCHR' resource,
 *      the corresponding Mac encoding goes into *encodingPtr.
 *
 * Side effects:
 *      Sets some internal static variables.
 *
 *----------------------------------------------------------------------
 */

static int
GetKeyboardLayout (Ptr * resourcePtr, TextEncoding * encodingPtr)
{
    static KeyboardLayoutRef lastLayout = NULL;
    static SInt32 lastLayoutId;
    static TextEncoding lastEncoding = kTextEncodingMacRoman;
    static Ptr uchr = NULL;
    static Ptr KCHR = NULL;
    static Handle handle = NULL;

    int hasLayoutChanged = false;
    KeyboardLayoutRef currentLayout = NULL;
    SInt32 currentLayoutId = 0;
    ScriptCode currentKeyScript;

    /*
     * Several code branches need this information.
     */

    currentKeyScript = GetScriptManagerVariable(smKeyScript);

    TkMacOSXInitNamedSymbol(HIToolbox, OSStatus, KLGetCurrentKeyboardLayout,
            KeyboardLayoutRef*);
    TkMacOSXInitNamedSymbol(HIToolbox, OSStatus, KLGetKeyboardLayoutProperty,
            KeyboardLayoutRef, KeyboardLayoutPropertyTag, const void**);
    if (KLGetCurrentKeyboardLayout && KLGetKeyboardLayoutProperty) {

        /*
         * Use the Keyboard Layout Services (these functions only exist since
         * 10.2).
         */

        KLGetCurrentKeyboardLayout(&currentLayout);

        if (currentLayout != NULL) {

            /*
             * The layout pointer could in theory be the same for different
             * layouts, only the id gives us the information that the
             * keyboard has actually changed.  OTOH the layout object can
             * also change and it could still be the same layoutid.
             */

            KLGetKeyboardLayoutProperty(currentLayout, kKLIdentifier,
                    (const void**)&currentLayoutId);

            if ((lastLayout != currentLayout)
                    || (lastLayoutId != currentLayoutId)) {

#ifdef TK_MAC_DEBUG
                fprintf (stderr, "GetKeyboardLayout(): Use KLS\n");
#endif

                hasLayoutChanged = true;

                /*
                 * Reinitialize all relevant variables.
                 */

                lastLayout = currentLayout;
                lastLayoutId = currentLayoutId;
                uchr = NULL;
                KCHR = NULL;

                if ((KLGetKeyboardLayoutProperty(currentLayout,
                                        kKLuchrData, (const void**)&uchr)
                                == noErr)
                        && (uchr != NULL)) {
                    /* done */
                } else if ((KLGetKeyboardLayoutProperty(currentLayout,
                                        kKLKCHRData, (const void**)&KCHR)
                                == noErr)
                        && (KCHR != NULL)) {
                    /* done */
                }
            }
        }

    } else {

        /*
         * Use the classic approach as shown in Apple code samples, loading
         * the keyboard resources directly.  This is broken for 10.3 and
         * possibly already in 10.2.
         */

        currentLayoutId = GetScriptVariable(currentKeyScript,smScriptKeys);

        if ((lastLayout == NULL) || (lastLayoutId != currentLayoutId)) {

#ifdef TK_MAC_DEBUG
            fprintf (stderr, "GetKeyboardLayout(): Use GetResource()\n");
#endif

            hasLayoutChanged = true;

            /*
             * Reinitialize all relevant variables.
             */

            lastLayout = (KeyboardLayoutRef)-1; 
            lastLayoutId = currentLayoutId;
            uchr = NULL;
            KCHR = NULL;

            /*
             * Get the new layout resource in the classic way.
             */

            if (handle != NULL) {
                HUnlock(handle);
            }
            
            if ((handle = GetResource('uchr',currentLayoutId)) != NULL) {
                HLock(handle);
                uchr = *handle;
            } else if ((handle = GetResource('KCHR',currentLayoutId)) != NULL) {
                HLock(handle);
                KCHR = *handle;
            }
        }
    }

    if (hasLayoutChanged) {

#ifdef TK_MAC_DEBUG
        if (KCHR != NULL) {
            fprintf (stderr, "GetKeyboardLayout(): New 'KCHR' layout %d\n",
                    (int) (short) currentLayoutId);
        } else if (uchr != NULL) {
            fprintf (stderr, "GetKeyboardLayout(): New 'uchr' layout %d\n",
                    (int) (short) currentLayoutId);
        } else {
            fprintf (stderr, "GetKeyboardLayout(): Use cached layout "
                    "(should have been %d)\n",
                    (int) (short) currentLayoutId);
        }
#endif

        deadKeyStateUp = deadKeyStateDown = 0;

        /*
         * If we did get a new 'KCHR', compute its encoding and put it into
         * lastEncoding.
         *
         * If we didn't get a new 'KCHR' and if we have no 'uchr' either, get
         * some 'KCHR' from the OS cache and leave lastEncoding at its
         * current value.  This should better not happen, it doesn't really
         * work.
         */

        if (KCHR != NULL) {
            lastEncoding = GetKCHREncoding(currentKeyScript, currentLayoutId);
#ifdef TK_MAC_DEBUG
            fprintf (stderr, "GetKeyboardLayout(): New 'KCHR' encoding %lu "
                    "(%lu + 0x%lX)\n",
                    lastEncoding, lastEncoding & 0xFFFFL,
                    lastEncoding & ~0xFFFFL);
#endif
        } else if (uchr == NULL) {
            KCHR = (Ptr) GetScriptManagerVariable(smKCHRCache);
        }
    }

    if (uchr != NULL) {
        *resourcePtr = uchr;
        return 1;
    } else {
        *resourcePtr = KCHR;
        *encodingPtr = lastEncoding;
        return 0;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * GetKCHREncoding --
 *
 *      Upgrade a WorldScript code to a TEC encoding based on the keyboard
 *      layout id.
 *
 * Results:
 *      The TEC code that corresponds best to the combination of WorldScript
 *      code and 'KCHR' id.
 *
 * Side effects:
 *      None.
 *
 * Rationale and Notes:
 *      WorldScript codes are sometimes not unique encodings.  E.g. Icelandic
 *      uses script smRoman (0), but the actual encoding is
 *      kTextEncodingMacIcelandic (37).  ftp://ftp.unicode.org/Public
 *      /MAPPINGS/VENDORS/APPLE/README.TXT has a good summary of these
 *      variants.  So we need to upgrade the script to an encoding with
 *      GetTextEncodingFromScriptInfo().
 *
 *      'KCHR' ids are usually region codes (see the comments in Script.h).
 *      Where they are not, we get a paramErr from the OS function and have
 *      appropriate fallbacks.
 *
 *----------------------------------------------------------------------
 */

static TextEncoding
GetKCHREncoding(ScriptCode script, SInt32 layoutid)
{
    RegionCode region = layoutid;
    TextEncoding encoding = script;

    if (GetTextEncodingFromScriptInfo(script, kTextLanguageDontCare, region,
                    &encoding) == noErr) {
        return encoding;
    }

    /*
     * GetTextEncodingFromScriptInfo() doesn't know about more exotic
     * layouts.  This provides a fallback for good measure.  In an ideal
     * world, exotic layouts would always provide a 'uchr' resource anyway,
     * so we wouldn't need this.
     *
     * We can add more keyboard layouts, if we get actual complaints.  Farsi
     * or other Celtic/Gaelic layouts would be candidates.
     */

    switch (layoutid) {

        /*
         * Icelandic and Faroese (planned).  These layouts are sold by Apple
         * Iceland for legacy applications.
         */

        case 1800: case 1821:
            return kTextEncodingMacIcelandic;

        /*
         * Irish and Welsh.  These layouts are mentioned in <Script.h>.
         *
         * FIXME: This may have to be kTextEncodingMacGaelic instead, but I
         * can't locate layouts of this type for testing.
         */

        case 581: case 779:
            return kTextEncodingMacCeltic;
    }
    
    /*
     * The valid script codes are also the valid default encoding codes, so
     * if nothing else helps, fall back on those.
     */

    return script;
}


/*
 *----------------------------------------------------------------------
 *
 * KeycodeToUnicodeViaUnicodeResource --
 *
 *        Given MacOS key event data this function generates the Unicode
 *        characters.  It does this using a 'uchr' and the UCKeyTranslate
 *        API.
 *
 *        The parameter deadKeyStatePtr can be NULL, if no deadkey handling
 *        is needed.
 *
 *        Tested and known to work with US, Hebrew, Greek and Russian layouts
 *        as well as "Unicode Hex Input".
 *
 * Results:
 *        The number of characters generated if any, 0 if we are waiting for
 *        another byte of a dead-key sequence. Fills in the uniChars array
 *        with a Unicode string.
 *
 * Side Effects:
 *        None
 *
 *----------------------------------------------------------------------
 */

static int
KeycodeToUnicodeViaUnicodeResource(
        UniChar * uniChars, int maxChars,
        Ptr uchr,
        EventKind eKind,
        UInt32 keycode, UInt32 modifiers,
        UInt32 * deadKeyStatePtr)
{
    int action;
    unsigned long keyboardType;
    OptionBits options = 0;
    UInt32 dummy_state;
    UniCharCount actuallength; 
    OSStatus status;

    keycode &= 0xFF;
    modifiers = (modifiers >> 8) & 0xFF;
    keyboardType = LMGetKbdType();

    if (NULL==deadKeyStatePtr) {
        options = kUCKeyTranslateNoDeadKeysMask;
        dummy_state = 0;
        deadKeyStatePtr = &dummy_state;
    }

    switch(eKind) {     
        case kEventRawKeyDown:
            action = kUCKeyActionDown;
            break;
        case kEventRawKeyUp:
            action = kUCKeyActionUp;
            break;
        case kEventRawKeyRepeat:
            action = kUCKeyActionAutoKey;
            break;
        default:
#ifdef TK_MAC_DEBUG
            fprintf (stderr,
                    "KeycodeToUnicodeViaUnicodeResource(): "
                    "Invalid parameter eKind %d\n",
                    (int) eKind);
#endif
            return 0;
    }

    status = UCKeyTranslate(
            (const UCKeyboardLayout *) uchr,
            keycode, action, modifiers, keyboardType,
            options, deadKeyStatePtr,
            maxChars, &actuallength, uniChars);

    if ((0 == actuallength) && (0 != *deadKeyStatePtr)) {
        /*
         * More data later
         */
        
        return 0; 
    }
    
    /*
     * some IMEs leave residue :-(
     */
    
    *deadKeyStatePtr = 0; 

    if (noErr != status) {
#ifdef TK_MAC_DEBUG
        fprintf(stderr,"UCKeyTranslate failed: %d", (int) status);
#endif
        actuallength = 0;
    }

    return actuallength;
}


/*
 *----------------------------------------------------------------------
 *
 * KeycodeToUnicodeViaKCHRResource --
 *
 *        Given MacOS key event data this function generates the Unicode
 *        characters.  It does this using a 'KCHR' and the KeyTranslate API.
 *
 *        The parameter deadKeyStatePtr can be NULL, if no deadkey handling
 *        is needed.
 *
 * Results:
 *        The number of characters generated if any, 0 if we are waiting for
 *        another byte of a dead-key sequence. Fills in the uniChars array
 *        with a Unicode string.
 *
 * Side Effects:
 *        None
 *
 *----------------------------------------------------------------------
 */

static int
KeycodeToUnicodeViaKCHRResource(
        UniChar * uniChars, int maxChars,
        Ptr kchr, TextEncoding encoding,
        EventKind eKind,
        UInt32 keycode, UInt32 modifiers,
        UInt32 * deadKeyStatePtr)
{
    UInt32 result;
    char macBuff[3];
    char * macStr;
    int macStrLen;
    UInt32 dummy_state = 0;


    if (NULL == deadKeyStatePtr) {
        deadKeyStatePtr = &dummy_state;
    }

    keycode |= modifiers;
    result = KeyTranslate(kchr, keycode, deadKeyStatePtr);

    if ((0 == result) && (0 != dummy_state)) {
        /*
         * 'dummy_state' gets only filled if the caller did not want deadkey
         * processing (deadKeyStatePtr was NULL originally), but we still
         * have a deadkey.  We just push the keycode for the space bar to get
         * the real key value.
         */

        result = KeyTranslate(kchr, 0x31, deadKeyStatePtr);
        *deadKeyStatePtr = 0;
    }

    if ((0 == result) && (0 != *deadKeyStatePtr)) {
        /*
         * More data later
         */
        
        return 0; 
    }

    macBuff[0] = (char) (result >> 16);
    macBuff[1] = (char)  result;
    macBuff[2] = 0;

    if (0 != macBuff[0]) {
        /*
         * If the first byte is valid, the second is too
         */
        
        macStr = macBuff;
        macStrLen = 2;
    } else if (0 != macBuff[1]) {
        /*
         * Only the second is valid
         */
        
        macStr = macBuff+1;
        macStrLen = 1;
    } else {
        /*
         * No valid bytes at all -- shouldn't happen
         */
        
        macStr = NULL;
        macStrLen = 0;
    }

    if (macStrLen <= 0) {
        return 0;
    } else {

        /*
         * Use the CFString conversion routines.  This is the easiest and
         * most compatible way to get from an 8-bit string and a MacOS script
         * code to a Unicode string.
         *
         * FIXME: The system ships with an Irish 'KCHR' but without the
         * corresponding macCeltic encoding, which triggers the error below.
         * Tcl doesn't have the macCeltic encoding either right now, so until
         * we get that, we can just as well stick to this code.  The right
         * fix would be to use the Tcl encodings and add macCeltic and
         * probably others there.  Suitable Unicode data files for the
         * missing encodings are available from www.evertype.com.
         */
        
        CFStringRef cfString;
        int uniStrLen;

        cfString = CFStringCreateWithCStringNoCopy(
            NULL, macStr, encoding, kCFAllocatorNull);
        if (cfString == NULL) {
#ifdef TK_MAC_DEBUG
            fprintf(stderr, "CFString: Can't convert with encoding %d\n",
                    (int) encoding);
#endif
            return 0;
        }

        uniStrLen = CFStringGetLength(cfString);
        if (uniStrLen > maxChars) {
            uniStrLen = maxChars;
        }
        CFStringGetCharacters(cfString, CFRangeMake(0,uniStrLen), uniChars);
        CFRelease(cfString);

        return uniStrLen;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXKeycodeToUnicode --
 *
 *        Given MacOS key event data this function generates the Unicode
 *        characters.  It does this using OS resources and APIs.
 *
 *        The parameter deadKeyStatePtr can be NULL, if no deadkey handling
 *        is needed.
 *
 *        This function is called from XKeycodeToKeysym() in
 *        tkMacOSKeyboard.c.
 *
 * Results:
 *        The number of characters generated if any, 0 if we are waiting for
 *        another byte of a dead-key sequence. Fills in the uniChars array
 *        with a Unicode string.
 *
 * Side Effects:
 *        None
 *
 *----------------------------------------------------------------------
 */

int
TkMacOSXKeycodeToUnicode(
        UniChar * uniChars, int maxChars,
        EventKind eKind,
        UInt32 keycode, UInt32 modifiers,
        UInt32 * deadKeyStatePtr)
{
    Ptr resource = NULL;
    TextEncoding encoding;
    int len;


    if (GetKeyboardLayout(&resource,&encoding)) {
        len = KeycodeToUnicodeViaUnicodeResource(
            uniChars, maxChars, resource, eKind,
            keycode, modifiers, deadKeyStatePtr);
    } else {
        len = KeycodeToUnicodeViaKCHRResource(
            uniChars, maxChars, resource, encoding, eKind,
            keycode, modifiers, deadKeyStatePtr);
    }

    return len;
}



/*
 *----------------------------------------------------------------------
 *
 * XGrabKeyboard --
 *
 *        Simulates a keyboard grab by setting the focus.
 *
 * Results:
 *        Always returns GrabSuccess.
 *
 * Side effects:
 *        Sets the keyboard focus to the specified window.
 *
 *----------------------------------------------------------------------
 */

int
XGrabKeyboard(
    Display* display,
    Window grab_window,
    Bool owner_events,
    int pointer_mode,
    int keyboard_mode,
    Time time)
{
    gKeyboardWinPtr = Tk_IdToWindow(display, grab_window);
    return GrabSuccess;
}

/*
 *----------------------------------------------------------------------
 *
 * XUngrabKeyboard --
 *
 *        Releases the simulated keyboard grab.
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Sets the keyboard focus back to the value before the grab.
 *
 *----------------------------------------------------------------------
 */

void
XUngrabKeyboard(
    Display* display,
    Time time)
{
    gKeyboardWinPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetCapture --
 *
 * Results:
 *      Returns the current grab window
 * Side effects:
 *        None.
 *
 */
Tk_Window
TkMacOSXGetCapture()
{
    return gGrabWinPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSetCapture --
 *
 *        This function captures the mouse so that all future events
 *        will be reported to this window, even if the mouse is outside
 *        the window.  If the specified window is NULL, then the mouse
 *        is released. 
 *
 * Results:
 *        None.
 *
 * Side effects:
 *        Sets the capture flag and captures the mouse.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetCapture(
    TkWindow *winPtr)                        /* Capture window, or NULL. */
{
    while ((winPtr != NULL) && !Tk_IsTopLevel(winPtr)) {
        winPtr = winPtr->parentPtr;
    }
    gGrabWinPtr = (Tk_Window) winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetCaretPos --
 *
 *      This enables correct placement of the XIM caret.  This is called
 *      by widgets to indicate their cursor placement, and the caret
 *      location is used by TkpGetString to place the XIM caret.
 *
 * Results:
 *      None
 *
 * Side effects:
 *      None
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetCaretPos(tkwin, x, y, height)
    Tk_Window tkwin;
    int       x;
    int       y;
    int       height;
{
}
