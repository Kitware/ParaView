/* 
 * tkMacOSXKeyboard.c --
 *
 *	Routines to support keyboard events on the Macintosh.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "X11/Xlib.h"
#include "X11/keysym.h"
#include <Carbon/Carbon.h>
#include "tkMacOSXInt.h"
#include "tkMacOSXEvent.h"      /* TkMacOSXKeycodeToUnicode() FIXME: That
                                 * function should probably move here */

typedef struct {
    int keycode;		/* Macintosh keycode */
    KeySym keysym;		/* X windows Keysym */
} KeyInfo;

/*
 * Notes on keyArray:
 *
 * 0x34, XK_Return - Powerbooks use this and some keymaps define it.
 *
 * 0x47, XK_Clear - This key is NumLock when used on PCs, but Mac
 * applications don't use it like that.
 *
 * All other keycodes are taken from the published ADB keyboard layouts.
 */

static KeyInfo keyArray[] = {
    {0x4C,	XK_Return},
    {0x24,	XK_Return},
    {0x34,  XK_Return},
    {0x33,	XK_BackSpace},
    {0x75,	XK_Delete},
    {0x30,	XK_Tab},
    {0x74,	XK_Page_Up},
    {0x79,	XK_Page_Down},
    {0x73,	XK_Home},
    {0x77,	XK_End},
    {0x7B,	XK_Left},
    {0x7C,	XK_Right},
    {0x7E,	XK_Up},
    {0x7D,	XK_Down},
    {0x72,	XK_Help},
    {0x35,	XK_Escape},
    {0x47,	XK_Clear},
    {0,		0}
};

static KeyInfo vituralkeyArray[] = {
    {122,	XK_F1},
    {120,	XK_F2},
    {99,	XK_F3},
    {118,	XK_F4},
    {96,	XK_F5},
    {97,	XK_F6},
    {98,	XK_F7},
    {100,	XK_F8},
    {101,	XK_F9},
    {109,	XK_F10},
    {103,	XK_F11},
    {111,	XK_F12},
    {105,	XK_F13},
    {107,	XK_F14},
    {113,	XK_F15},
    {0,		0}
};

static int initialized = 0;
static Tcl_HashTable keycodeTable;	/* keyArray hashed by keycode value. */
static Tcl_HashTable vkeyTable;		/* vituralkeyArray hashed by virtual
					   keycode value. */

/*
 * Prototypes for static functions used in this file.
 */
static void	InitKeyMaps _ANSI_ARGS_((void));


/*
 *----------------------------------------------------------------------
 *
 * InitKeyMaps --
 *
 *	Creates hash tables used by some of the functions in this file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates memory & creates some hash tables.
 *
 *----------------------------------------------------------------------
 */

static void
InitKeyMaps()
{
    Tcl_HashEntry *hPtr;
    KeyInfo *kPtr;
    int dummy;
		
    Tcl_InitHashTable(&keycodeTable, TCL_ONE_WORD_KEYS);
    for (kPtr = keyArray; kPtr->keycode != 0; kPtr++) {
	hPtr = Tcl_CreateHashEntry(&keycodeTable, (char *) kPtr->keycode,
		&dummy);
	Tcl_SetHashValue(hPtr, kPtr->keysym);
    }
    Tcl_InitHashTable(&vkeyTable, TCL_ONE_WORD_KEYS);
    for (kPtr = vituralkeyArray; kPtr->keycode != 0; kPtr++) {
	hPtr = Tcl_CreateHashEntry(&vkeyTable, (char *) kPtr->keycode,
		&dummy);
	Tcl_SetHashValue(hPtr, kPtr->keysym);
    }
    initialized = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * XKeycodeToKeysym --
 *
 *	Translate from a system-dependent keycode to a
 *	system-independent keysym.
 *
 * Results:
 *	Returns the translated keysym, or NoSymbol on failure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

KeySym 
XKeycodeToKeysym(
    Display* display,
    KeyCode keycode,
    int	index)
{
    register Tcl_HashEntry *hPtr;
    int c;
    int virtualKey;
    int newKeycode;
    UniChar newChar;

    if (!initialized) {
	InitKeyMaps();
    }
    if (keycode == 0) {
        /* 
         * This means we had a pure modifier keypress or
         * something similar which is a TO DO.
         */
        return NoSymbol;
    }
    
    virtualKey = keycode >> 16;
    c = (keycode) & 0xffff;
    if (c > 255) {
        return NoSymbol;
    }

    /*
     * When determining what keysym to produce we first check to see if
     * the key is a function key.  We then check to see if the character
     * is another non-printing key.  Finally, we return the key syms
     * for all ASCII chars.
     */
    if (c == 0x10) {
	hPtr = Tcl_FindHashEntry(&vkeyTable, (char *) virtualKey);
	if (hPtr != NULL) {
            return (KeySym) Tcl_GetHashValue(hPtr);
	}
    }
    hPtr = Tcl_FindHashEntry(&keycodeTable, (char *) virtualKey);
    if (hPtr != NULL) {
 	return (KeySym) Tcl_GetHashValue(hPtr);
    }

    /* 
     * Recompute the character based on the Shift key only.
     * TODO: The index may also specify the NUM_LOCK.
     */
    newKeycode = virtualKey;
    if (index & 0x01) {
	newKeycode += 0x0200;
    }

    newChar = 0;
    TkMacOSXKeycodeToUnicode(
            &newChar, 1, kEventRawKeyDown,
            newKeycode & 0x00FF, newKeycode & 0xFF00, NULL);

    /*
     * X11 keysyms are identical to Unicode for ASCII and Latin-1.  Give up
     * for other characters for now.
     */
    if (newChar >= XK_space && newChar <= 0x255) {
  	return newChar;
    }

    return NoSymbol; 
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetString --
 *
 *	Retrieve the string equivalent for the given keyboard event.
 *
 * Results:
 *	Returns the UTF string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
TkpGetString(
    TkWindow *winPtr,		/* Window where event occurred:  needed to
				 * get input context. */
    XEvent *eventPtr,		/* X keyboard event. */
    Tcl_DString *dsPtr)		/* Uninitialized or empty string to hold
				 * result. */
{
    (void) winPtr; /*unused*/
    Tcl_DStringInit(dsPtr);
    return Tcl_DStringAppend(dsPtr, eventPtr->xkey.trans_chars, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * XGetModifierMapping --
 *
 *	Fetch the current keycodes used as modifiers.
 *
 * Results:
 *	Returns a new modifier map.
 *
 * Side effects:
 *	Allocates a new modifier map data structure.
 *
 *----------------------------------------------------------------------
 */

XModifierKeymap * 
XGetModifierMapping(
    Display* display)
{ 
    XModifierKeymap * modmap;

    modmap = (XModifierKeymap *) ckalloc(sizeof(XModifierKeymap));
    modmap->max_keypermod = 0;
    modmap->modifiermap = NULL;
    return modmap;
}

/*
 *----------------------------------------------------------------------
 *
 * XFreeModifiermap --
 *
 *	Deallocate a modifier map that was created by
 *	XGetModifierMapping.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees the datastructure referenced by modmap.
 *
 *----------------------------------------------------------------------
 */

void 
XFreeModifiermap(
    XModifierKeymap *modmap)
{
    if (modmap->modifiermap != NULL) {
	ckfree((char *) modmap->modifiermap);
    }
    ckfree((char *) modmap);
}

/*
 *----------------------------------------------------------------------
 *
 * XKeysymToString, XStringToKeysym --
 *
 *	These X window functions map Keysyms to strings & strings to 
 * 	keysyms.  However, Tk already does this for the most common keysyms.  
 *  	Therefor, these functions only need to support keysyms that will be 
 *  	specific to the Macintosh.  Currently, there are none.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char * 
XKeysymToString(
    KeySym keysym)
{
    return NULL; 
}

KeySym 
XStringToKeysym(
    const char*	string)
{ 
    return NoSymbol;
}

/*
 *----------------------------------------------------------------------
 *
 * XKeysymToKeycode --
 *
 *	The function XKeysymToKeycode is only used by tkTest.c and
 *	currently only implementes the support for keys used in the
 *	Tk test suite.
 *      FIXME - This is no longer true.  This function is now used in
 *      "event generate" so we really should make it work.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

KeyCode
XKeysymToKeycode(
    Display* display,
    KeySym keysym)
{
    KeyCode keycode = 0;
    char virtualKeyCode = 0;
    
    if ((keysym >= XK_space) && (XK_asciitilde)) {
        if (keysym == 'a') {
            virtualKeyCode = 0x00;
        } else if (keysym == 'b' || keysym == 'B') {
            virtualKeyCode = 0x0B;
        } else if (keysym == 'c') {
            virtualKeyCode = 0x08;
        } else if (keysym == 'x' || keysym == 'X') {
            virtualKeyCode = 0x07;
        } else if (keysym == 'z') {
            virtualKeyCode = 0x06;
        } else if (keysym == ' ') {
            virtualKeyCode = 0x31;
        } else if (keysym == XK_Return) {
            virtualKeyCode = 0x24;
            keysym = '\r';
        }
	keycode = keysym + (virtualKeyCode <<16);
    }

    return keycode;
}

/*
 * When mapping from a keysym to a keycode, need
 * information about the modifier state that should be used
 * so that when they call XKeycodeToKeysym taking into
 * account the xkey.state, they will get back the original
 * keysym.
 */

void
TkpSetKeycodeAndState(tkwin, keySym, eventPtr)
    Tk_Window tkwin;
    KeySym keySym;
    XEvent *eventPtr;
{
    Display *display;
    int state;
    KeyCode keycode;
    
    display = Tk_Display(tkwin);
    
    if (keySym == NoSymbol) {
	keycode = 0;
    } else {
	keycode = XKeysymToKeycode(display, keySym);
    }
    if (keycode != 0) {
	for (state = 0; state < 4; state++) {
	    if (XKeycodeToKeysym(display, keycode, state) == keySym) {
		if (state & 1) {
		    eventPtr->xkey.state |= ShiftMask;
		}
		if (state & 2) {
		    TkDisplay *dispPtr;

		    dispPtr = ((TkWindow *) tkwin)->dispPtr; 
		    eventPtr->xkey.state |= dispPtr->modeModMask;
		}
		break;
	    }
	}
    }
    eventPtr->xkey.keycode = keycode;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetKeySym --
 *
 *	Given an X KeyPress or KeyRelease event, map the
 *	keycode in the event into a KeySym.
 *
 * Results:
 *	The return value is the KeySym corresponding to
 *	eventPtr, or NoSymbol if no matching Keysym could be
 *	found.
 *
 * Side effects:
 *	In the first call for a given display, keycode-to-
 *	KeySym maps get loaded.
 *
 *----------------------------------------------------------------------
 */

KeySym
TkpGetKeySym(dispPtr, eventPtr)
    TkDisplay *dispPtr;	/* Display in which to
					 * map keycode. */
    XEvent *eventPtr;		/* Description of X event. */
{
    KeySym sym;
    int index;

    /*
     * Refresh the mapping information if it's stale
     */

    if (dispPtr->bindInfoStale) {
	TkpInitKeymapInfo(dispPtr);
    }

    /*
     * Figure out which of the four slots in the keymap vector to
     * use for this key.  Refer to Xlib documentation for more info
     * on how this computation works.
     */

    index = 0;
    if (eventPtr->xkey.state & dispPtr->modeModMask) {
	index = 2;
    }
    if ((eventPtr->xkey.state & ShiftMask)
	    || ((dispPtr->lockUsage != LU_IGNORE)
	    && (eventPtr->xkey.state & LockMask))) {
	index += 1;
    }
    if (eventPtr->xany.send_event == -1) {
	/* We use -1 as a special signal for a pure modifier */
	int modifier = eventPtr->xkey.keycode;
	if (modifier == cmdKey) {
	    return XK_Alt_L;
	} else if (modifier == shiftKey) {
	    return XK_Shift_L;
	} else if (modifier == alphaLock) {
	    return XK_Caps_Lock;
	} else if (modifier == optionKey) {
	    return XK_Meta_L;
	} else if (modifier == controlKey) {
	    return XK_Control_L;
	} else if (modifier == rightShiftKey) {
	    return XK_Shift_R;
	} else if (modifier == rightOptionKey) {
	    return XK_Meta_R;
	} else if (modifier == rightControlKey) {
	    return XK_Control_R;
	} else {
	    /* If we get here, we probably need to implement something new */
	    return NoSymbol;
	} 
    }
    sym = XKeycodeToKeysym(dispPtr->display, eventPtr->xkey.keycode, index);

    /*
     * Special handling:  if the key was shifted because of Lock, but
     * lock is only caps lock, not shift lock, and the shifted keysym
     * isn't upper-case alphabetic, then switch back to the unshifted
     * keysym.
     */

    if ((index & 1) && !(eventPtr->xkey.state & ShiftMask)
	    && (dispPtr->lockUsage == LU_CAPS)) {
	if (!(((sym >= XK_A) && (sym <= XK_Z))
		|| ((sym >= XK_Agrave) && (sym <= XK_Odiaeresis))
		|| ((sym >= XK_Ooblique) && (sym <= XK_Thorn)))) {
	    index &= ~1;
	    sym = XKeycodeToKeysym(dispPtr->display, eventPtr->xkey.keycode,
		    index);
	}
    }

    /*
     * Another bit of special handling:  if this is a shifted key and there
     * is no keysym defined, then use the keysym for the unshifted key.
     */

    if ((index & 1) && (sym == NoSymbol)) {
	sym = XKeycodeToKeysym(dispPtr->display, eventPtr->xkey.keycode,
		index & ~1);
    }
    return sym;
}

/*
 *--------------------------------------------------------------
 *
 * TkpInitKeymapInfo --
 *
 *	This procedure is invoked to scan keymap information
 *	to recompute stuff that's important for binding, such
 *	as the modifier key (if any) that corresponds to "mode
 *	switch".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Keymap-related information in dispPtr is updated.
 *
 *--------------------------------------------------------------
 */

void
TkpInitKeymapInfo(dispPtr)
    TkDisplay *dispPtr;		/* Display for which to recompute keymap
				 * information. */
{
    XModifierKeymap *modMapPtr;
    KeyCode *codePtr;
    KeySym keysym;
    int count, i, j, max, arraySize;
#define KEYCODE_ARRAY_SIZE 20

    dispPtr->bindInfoStale = 0;
    modMapPtr = XGetModifierMapping(dispPtr->display);

    /*
     * Check the keycodes associated with the Lock modifier.  If
     * any of them is associated with the XK_Shift_Lock modifier,
     * then Lock has to be interpreted as Shift Lock, not Caps Lock.
     */

    dispPtr->lockUsage = LU_IGNORE;
    codePtr = modMapPtr->modifiermap + modMapPtr->max_keypermod*LockMapIndex;
    for (count = modMapPtr->max_keypermod; count > 0; count--, codePtr++) {
	if (*codePtr == 0) {
	    continue;
	}
	keysym = XKeycodeToKeysym(dispPtr->display, *codePtr, 0);
	if (keysym == XK_Shift_Lock) {
	    dispPtr->lockUsage = LU_SHIFT;
	    break;
	}
	if (keysym == XK_Caps_Lock) {
	    dispPtr->lockUsage = LU_CAPS;
	    break;
	}
    }

    /*
     * Look through the keycodes associated with modifiers to see if
     * the the "mode switch", "meta", or "alt" keysyms are associated
     * with any modifiers.  If so, remember their modifier mask bits.
     */

    dispPtr->modeModMask = 0;
    dispPtr->metaModMask = 0;
    dispPtr->altModMask = 0;
    codePtr = modMapPtr->modifiermap;
    max = 8*modMapPtr->max_keypermod;
    for (i = 0; i < max; i++, codePtr++) {
	if (*codePtr == 0) {
	    continue;
	}
	keysym = XKeycodeToKeysym(dispPtr->display, *codePtr, 0);
	if (keysym == XK_Mode_switch) {
	    dispPtr->modeModMask |= ShiftMask << (i/modMapPtr->max_keypermod);
	}
	if ((keysym == XK_Meta_L) || (keysym == XK_Meta_R)) {
	    dispPtr->metaModMask |= ShiftMask << (i/modMapPtr->max_keypermod);
	}
	if ((keysym == XK_Alt_L) || (keysym == XK_Alt_R)) {
	    dispPtr->altModMask |= ShiftMask << (i/modMapPtr->max_keypermod);
	}
    }

    /*
     * Create an array of the keycodes for all modifier keys.
     */

    if (dispPtr->modKeyCodes != NULL) {
	ckfree((char *) dispPtr->modKeyCodes);
    }
    dispPtr->numModKeyCodes = 0;
    arraySize = KEYCODE_ARRAY_SIZE;
    dispPtr->modKeyCodes = (KeyCode *) ckalloc((unsigned)
	    (KEYCODE_ARRAY_SIZE * sizeof(KeyCode)));
    for (i = 0, codePtr = modMapPtr->modifiermap; i < max; i++, codePtr++) {
	if (*codePtr == 0) {
	    continue;
	}

	/*
	 * Make sure that the keycode isn't already in the array.
	 */

	for (j = 0; j < dispPtr->numModKeyCodes; j++) {
	    if (dispPtr->modKeyCodes[j] == *codePtr) {
		goto nextModCode;
	    }
	}
	if (dispPtr->numModKeyCodes >= arraySize) {
	    KeyCode *new;

	    /*
	     * Ran out of space in the array;  grow it.
	     */

	    arraySize *= 2;
	    new = (KeyCode *) ckalloc((unsigned)
		    (arraySize * sizeof(KeyCode)));
	    memcpy((VOID *) new, (VOID *) dispPtr->modKeyCodes,
		    (dispPtr->numModKeyCodes * sizeof(KeyCode)));
	    ckfree((char *) dispPtr->modKeyCodes);
	    dispPtr->modKeyCodes = new;
	}
	dispPtr->modKeyCodes[dispPtr->numModKeyCodes] = *codePtr;
	dispPtr->numModKeyCodes++;
	nextModCode: continue;
    }
    XFreeModifiermap(modMapPtr);
}
