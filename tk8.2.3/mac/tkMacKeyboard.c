/* 
 * tkMacKeyboard.c --
 *
 *	Routines to support keyboard events on the Macintosh.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "Xlib.h"
#include "keysym.h"

#include <Events.h>
#include <Script.h>

typedef struct {
    short keycode;		/* Macintosh keycode */
    KeySym keysym;		/* X windows Keysym */
} KeyInfo;

static KeyInfo keyArray[] = {
    {0x4C,	XK_Return},
    {0x24,	XK_Return},
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
static Ptr KCHRPtr;			/* Pointer to 'KCHR' resource. */

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
    register Tcl_HashEntry *hPtr;
    register KeyInfo *kPtr;
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
    KCHRPtr = (Ptr) GetScriptManagerVariable(smKCHRCache);
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
    char virtualKey;
    int newKeycode;
    unsigned long dummy, newChar;

    if (!initialized) {
	InitKeyMaps();
    }
	
    virtualKey = (char) (keycode >> 16);    
    c = (keycode) & 0xffff;
    if (c > 255) {
        return NoSymbol;
    }

    /*
     * When determining what keysym to produce we firt check to see if
     * the key is a function key.  We then check to see if the character
     * is another non-printing key.  Finally, we return the key syms
     * for all ASCI chars.
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
    dummy = 0;
    newChar = KeyTranslate(KCHRPtr, (short) newKeycode, &dummy);
    c = newChar & charCodeMask;
    
    if (c >= XK_space && c < XK_asciitilde) {
	return c;
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
    register Tcl_HashEntry *hPtr;
    char string[3];
    char virtualKey;
    int c, len;

    if (!initialized) {
	InitKeyMaps();
    }
    
    Tcl_DStringInit(dsPtr);
    
    virtualKey = (char) (eventPtr->xkey.keycode >> 16);    
    c = (eventPtr->xkey.keycode) & 0xffff;
    
    if (c < 256) {
        string[0] = (char) c;
        len = 1;
    } else {
        string[0] = (char) (c >> 8);
        string[1] = (char) c;
        len = 2;
    }
    
    /*
     * Just return NULL if the character is a function key or another
     * non-printing key.
     */
    if (c == 0x10) {
	len = 0;
    } else {
	hPtr = Tcl_FindHashEntry(&keycodeTable, (char *) virtualKey);
	if (hPtr != NULL) {
	    len = 0;
	}
    }
    return Tcl_ExternalToUtfDString(NULL, string, len, dsPtr);
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
