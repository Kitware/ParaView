/* 
 * tkWinKey.c --
 *
 *	This file contains X emulation routines for keyboard related
 *	functions.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkWinInt.h"

typedef struct {
    unsigned int keycode;
    KeySym keysym;
} Keys;

static Keys keymap[] = {
    VK_CANCEL, XK_Cancel,
    VK_BACK, XK_BackSpace,
    VK_TAB, XK_Tab,
    VK_CLEAR, XK_Clear,
    VK_RETURN, XK_Return,
    VK_SHIFT, XK_Shift_L,
    VK_CONTROL, XK_Control_L,
    VK_MENU, XK_Alt_L,
    VK_PAUSE, XK_Pause,
    VK_CAPITAL, XK_Caps_Lock,
    VK_ESCAPE, XK_Escape,
    VK_SPACE, XK_space,
    VK_PRIOR, XK_Prior,
    VK_NEXT, XK_Next,
    VK_END, XK_End,
    VK_HOME, XK_Home,
    VK_LEFT, XK_Left,
    VK_UP, XK_Up,
    VK_RIGHT, XK_Right,
    VK_DOWN, XK_Down,
    VK_SELECT, XK_Select,
    VK_PRINT, XK_Print,
    VK_EXECUTE, XK_Execute,
    VK_INSERT, XK_Insert,
    VK_DELETE, XK_Delete,
    VK_HELP, XK_Help,
    VK_F1, XK_F1,
    VK_F2, XK_F2,
    VK_F3, XK_F3,
    VK_F4, XK_F4,
    VK_F5, XK_F5,
    VK_F6, XK_F6,
    VK_F7, XK_F7,
    VK_F8, XK_F8,
    VK_F9, XK_F9,
    VK_F10, XK_F10,
    VK_F11, XK_F11,
    VK_F12, XK_F12,
    VK_F13, XK_F13,
    VK_F14, XK_F14,
    VK_F15, XK_F15,
    VK_F16, XK_F16,
    VK_F17, XK_F17,
    VK_F18, XK_F18,
    VK_F19, XK_F19,
    VK_F20, XK_F20,
    VK_F21, XK_F21,
    VK_F22, XK_F22,
    VK_F23, XK_F23,
    VK_F24, XK_F24,
    VK_NUMLOCK, XK_Num_Lock, 
    VK_SCROLL, XK_Scroll_Lock,

    /*
     * The following support the new keys in the Microsoft keyboard.
     * Win_L and Win_R have the windows logo.  App has the menu.
     */

    VK_LWIN, XK_Win_L,
    VK_RWIN, XK_Win_R,
    VK_APPS, XK_App,

    0, NoSymbol
};


/*
 *----------------------------------------------------------------------
 *
 * TkpGetString --
 *
 *	Retrieve the UTF string equivalent for the given keyboard event.
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
TkpGetString(winPtr, eventPtr, dsPtr)
    TkWindow *winPtr;		/* Window where event occurred:  needed to
				 * get input context. */
    XEvent *eventPtr;		/* X keyboard event. */
    Tcl_DString *dsPtr;		/* Uninitialized or empty string to hold
				 * result. */
{
    int index;
    KeySym keysym;
    XKeyEvent* keyEv = &eventPtr->xkey;

    Tcl_DStringInit(dsPtr);
    if (eventPtr->xkey.send_event != -1) {
	/*
	 * This is an event generated from generic code.  It has no
	 * nchars or trans_chars members. 
	 */

	index = 0;
	if (eventPtr->xkey.state & ShiftMask) {
	    index |= 1;
	}
	if (eventPtr->xkey.state & Mod1Mask) {
	    index |= 2;
	}
	keysym = XKeycodeToKeysym(eventPtr->xkey.display, 
		eventPtr->xkey.keycode, index);
	if (((keysym != NoSymbol) && (keysym > 0) && (keysym < 256)) 
		|| (keysym == XK_Return)
		|| (keysym == XK_Tab)) {
	    char buf[TCL_UTF_MAX];
	    int len = Tcl_UniCharToUtf((Tcl_UniChar) keysym, buf);
	    Tcl_DStringAppend(dsPtr, buf, len);
	}
    } else if (eventPtr->xkey.nbytes > 0) {
	Tcl_ExternalToUtfDString(NULL, eventPtr->xkey.trans_chars,
		eventPtr->xkey.nbytes, dsPtr);
    }
    return Tcl_DStringValue(dsPtr);
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
XKeycodeToKeysym(display, keycode, index)
    Display* display;
    unsigned int keycode;
    int index;
{
    Keys* key;
    BYTE keys[256];
    int result;
    char buf[4];
    unsigned int scancode = MapVirtualKey(keycode, 0);

    memset(keys, 0, 256);
    if (index & 0x02) {
	keys[VK_NUMLOCK] = 1;
    }
    if (index & 0x01) {
	keys[VK_SHIFT] = 0x80;
    }
    result = ToAscii(keycode, scancode, keys, (LPWORD) buf, 0);

    /*
     * Keycode mapped to a valid Latin-1 character.  Since the keysyms
     * for alphanumeric characters map onto Latin-1, we just return it.
     */

    if (result == 1 && UCHAR(buf[0]) >= 0x20) {
	return (KeySym) UCHAR(buf[0]);
    }

    /*
     * Keycode is a non-alphanumeric key, so we have to do the lookup.
     */

    for (key = keymap; key->keycode != 0; key++) {
	if (key->keycode == keycode) {
	    return key->keysym;
	}
    }

    return NoSymbol;
}

/*
 *----------------------------------------------------------------------
 *
 * XKeysymToKeycode --
 *
 *	Translate a keysym back into a keycode.
 *
 * Results:
 *	Returns the keycode that would generate the specified keysym.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

KeyCode
XKeysymToKeycode(display, keysym)
    Display* display;
    KeySym keysym;
{
    Keys* key;
    SHORT result;

    /*
     * We check our private map first for a virtual keycode,
     * as VkKeyScan will return values that don't map to X
     * for the "extended" Syms.  This may be due to just casting
     * problems below, but this works.
     */

    for (key = keymap; key->keycode != 0; key++) {
	if (key->keysym == keysym) {
	    return key->keycode;
	}
    }

    if (keysym >= 0x20) {
	result = VkKeyScan((char) keysym);
	if (result != -1) {
	    return (KeyCode) (result & 0xff);
	}
    }

    return 0;
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

XModifierKeymap	*
XGetModifierMapping(display)
    Display* display;
{
    XModifierKeymap *map = (XModifierKeymap *)ckalloc(sizeof(XModifierKeymap));

    map->max_keypermod = 1;
    map->modifiermap = (KeyCode *) ckalloc(sizeof(KeyCode)*8);
    map->modifiermap[ShiftMapIndex] = VK_SHIFT;
    map->modifiermap[LockMapIndex] = VK_CAPITAL;
    map->modifiermap[ControlMapIndex] = VK_CONTROL;
    map->modifiermap[Mod1MapIndex] = VK_NUMLOCK;
    map->modifiermap[Mod2MapIndex] = VK_MENU;
    map->modifiermap[Mod3MapIndex] = VK_SCROLL;
    map->modifiermap[Mod4MapIndex] = 0;
    map->modifiermap[Mod5MapIndex] = 0;
    return map;
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
XFreeModifiermap(modmap)
    XModifierKeymap* modmap;
{
    ckfree((char *) modmap->modifiermap);
    ckfree((char *) modmap);
}

/*
 *----------------------------------------------------------------------
 *
 * XStringToKeysym --
 *
 *	Translate a keysym name to the matching keysym. 
 *
 * Results:
 *	Returns the keysym.  Since this is already handled by
 *	Tk's StringToKeysym function, we just return NoSymbol.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

KeySym
XStringToKeysym(string)
    _Xconst char *string;
{
    return NoSymbol;
}

/*
 *----------------------------------------------------------------------
 *
 * XKeysymToString --
 *
 *	Convert a keysym to character form.
 *
 * Results:
 *	Returns NULL, since Tk will have handled this already.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
XKeysymToString(keysym)
    KeySym keysym;
{
    return NULL;
}
