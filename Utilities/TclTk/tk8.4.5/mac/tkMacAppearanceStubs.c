/* 
 * tkMacAppearanceStubs.c --
 *
 *	This file contains stubs for some MacOS8.6+ Toolbox calls that
 *      are not contained in any of the CFM68K stubs libraries.  Their
 *      use must be conditionalized by checks (usually for Appearance version
 *      greater than 1.1), so they will never get called on a CFM68k system.
 *      Putting in the stubs means I don't have to clutter the code BOTH
 *      with appearance version checks & #ifdef GENERATING_CFM68K...
 *
 * Copyright (c) 1999 Scriptics Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * 
 */

#include <MacWindows.h>
#include <Appearance.h>

/* Export these calls from the Tk library, since we may need to use
 * them in shell calls.
 */
  
pascal OSStatus
MoveWindowStructure(
    WindowPtr window, 
    short hGlobal, 
    short vGlobal)
{
    panic("Error: Running stub for PPC-Only routine");
    return noErr;

}

pascal OSStatus
CreateNewWindow(
    WindowClass windowClass, 
    WindowAttributes attributes, 
    const Rect *bounds, 
    WindowPtr *outWindow)
{
    panic("Error: Running stub for PPC-Only routine");
    return noErr;

}

pascal WindowPtr
FrontNonFloatingWindow()
{
    panic("Error: Running stub for PPC-Only routine");
    return NULL;
}

pascal OSStatus
GetWindowClass(
    WindowPtr window,
    WindowClass *outClass)
{
    panic("Error: Running stub for PPC-Only routine");
    return noErr;
}

pascal OSStatus
ApplyThemeBackground(
    ThemeBackgroundKind inKind,
    const Rect* bounds,
    ThemeDrawState inState,
    SInt16 inDepth,
    Boolean inColorDev)
{
    panic("Error: Running stub for PPC-Only routine");
    return noErr;
}

pascal OSStatus
InitFloatingWindows(void)
{
    panic("Error: Running stub for PPC-Only routine");
    return noErr;
}

pascal OSStatus
ShowFloatingWindows(void)
{
    panic("Error: Running stub for PPC-Only routine");
    return noErr;
}

pascal OSStatus
HideFloatingWindows(void)
{
    panic("Error: Running stub for PPC-Only routine");
    return noErr;
}

pascal Boolean
IsValidWindowPtr(GrafPtr grafPort)
{
    panic("Error: Running stub for PPC-Only routine");
    return true;
}

