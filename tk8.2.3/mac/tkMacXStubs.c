/* 
 * tkMacXStubs.c --
 *
 *	This file contains most of the X calls called by Tk.  Many of
 * these calls are just stubs and either don't make sense on the
 * Macintosh or thier implamentation just doesn't do anything.  Other
 * calls will eventually be moved into other files.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include <X.h>
#include <Xlib.h>
#include <stdio.h>
#include <tcl.h>

#include <Xatom.h>

#include <Windows.h>
#include <Fonts.h>
#include <QDOffscreen.h>
#include <ToolUtils.h>
#include <Sound.h>
#include "tkMacInt.h"
#include "tkPort.h"

/*
 * Because this file is still under major development Debugger statements are
 * used through out this file.  The define TCL_DEBUG will decide whether
 * the debugger statements actually call the debugger or not.
 */

#ifndef TCL_DEBUG
#   define Debugger()
#endif
 
#define ROOT_ID 10

/*
 * Declarations of static variables used in this file.
 */

static TkDisplay *gMacDisplay = NULL; /* Macintosh display. */
static char *macScreenName = ":0";
				/* Default name of macintosh display. */

/*
 * Forward declarations of procedures used in this file.
 */

static XID MacXIdAlloc _ANSI_ARGS_((Display *display));
static int DefaultErrorHandler _ANSI_ARGS_((Display* display,
	XErrorEvent* err_evt));

/*
 * Other declrations
 */

int TkMacXDestroyImage _ANSI_ARGS_((XImage *image));
unsigned long TkMacXGetPixel _ANSI_ARGS_((XImage *image, int x, int y));
int TkMacXPutPixel _ANSI_ARGS_((XImage *image, int x, int y,
	unsigned long pixel));
XImage *TkMacXSubImage _ANSI_ARGS_((XImage *image, int x, int y, 
	unsigned int width, unsigned int height));
int TkMacXAddPixel _ANSI_ARGS_((XImage *image, long value));
int _XInitImageFuncPtrs _ANSI_ARGS_((XImage *image));

/*
 *----------------------------------------------------------------------
 *
 * TkpOpenDisplay --
 *
 *	Create the Display structure and fill it with device
 *	specific information.
 *
 * Results:
 *	Returns a Display structure on success or NULL on failure.
 *
 * Side effects:
 *	Allocates a new Display structure.
 *
 *----------------------------------------------------------------------
 */

TkDisplay *
TkpOpenDisplay(
    char *display_name)
{
    Display *display;
    Screen *screen;
    GDHandle graphicsDevice;

    if (gMacDisplay != NULL) {
	if (strcmp(gMacDisplay->display->display_name, display_name) == 0) {
	    return gMacDisplay;
	} else {
	    return NULL;
	}
    }

    graphicsDevice = GetMainDevice();
    display = (Display *) ckalloc(sizeof(Display));
    display->resource_alloc = MacXIdAlloc;
    screen = (Screen *) ckalloc(sizeof(Screen) * 2);
    display->default_screen = 0;
    display->request = 0;
    display->nscreens = 1;
    display->screens = screen;
    display->display_name = macScreenName;
    display->qlen = 0;
    
    screen->root = ROOT_ID;
    screen->display = display;
    screen->root_depth = (*(*graphicsDevice)->gdPMap)->cmpSize *
                               (*(*graphicsDevice)->gdPMap)->cmpCount;	
    screen->height = (*graphicsDevice)->gdRect.bottom -
	(*graphicsDevice)->gdRect.top;
    screen->width = (*graphicsDevice)->gdRect.right -
	(*graphicsDevice)->gdRect.left;
    
    screen->mwidth = (screen->width * 254 + 360) / 720;
    screen->mheight = (screen->height * 254 + 360) / 720;
    screen->black_pixel = 0x00000000;
    screen->white_pixel = 0x00FFFFFF;
    screen->root_visual = (Visual *) ckalloc(sizeof(Visual));
    screen->root_visual->visualid = 0;
    screen->root_visual->class = TrueColor;
    screen->root_visual->red_mask = 0x00FF0000;
    screen->root_visual->green_mask = 0x0000FF00;
    screen->root_visual->blue_mask = 0x000000FF;
    screen->root_visual->bits_per_rgb = 24;
    screen->root_visual->map_entries = 2 ^ 8;

    gMacDisplay = (TkDisplay *) ckalloc(sizeof(TkDisplay));
    gMacDisplay->display = display;
    return gMacDisplay;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpCloseDisplay --
 *
 *	Deallocates a display structure created by TkpOpenDisplay.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory.
 *
 *----------------------------------------------------------------------
 */

void
TkpCloseDisplay(
    TkDisplay *displayPtr)
{
    Display *display = displayPtr->display;
    if (gMacDisplay != displayPtr) {
        panic("TkpCloseDisplay: tried to call TkpCloseDisplay on bad display");
    }

    /*
     * Make sure that the local scrap is transfered to the global
     * scrap if needed.
     */

    TkSuspendClipboard();

    gMacDisplay = NULL;
    if (display->screens != (Screen *) NULL) {
        if (display->screens->root_visual != (Visual *) NULL) {
            ckfree((char *) display->screens->root_visual);
        }
        ckfree((char *) display->screens);
    }
    ckfree((char *) display);
    ckfree((char *) displayPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * MacXIdAlloc --
 *
 *	This procedure is invoked by Xlib as the resource allocator
 *	for a display.
 *
 * Results:
 *	The return value is an X resource identifier that isn't currently
 *	in use.
 *
 * Side effects:
 *	The identifier is removed from the stack of free identifiers,
 *	if it was previously on the stack.
 *
 *----------------------------------------------------------------------
 */

static XID
MacXIdAlloc(
    Display *display)			/* Display for which to allocate. */
{
	static long int cur_id = 100;
	/*
	 * Some special XIds are reserved
	 *   - this is why we start at 100
	 */

	return ++cur_id;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpWindowWasRecentlyDeleted --
 *
 *	Tries to determine whether the given window was recently deleted.
 *	Called from the generic code error handler to attempt to deal with
 *	async BadWindow errors under some circumstances.
 *
 * Results:
 *	Always 0, we do not keep this information on the Mac, so we do not
 *	know whether the window was destroyed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkpWindowWasRecentlyDeleted(
    Window win,
    TkDisplay *dispPtr)
{
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * DefaultErrorHandler --
 *
 *	This procedure is the default X error handler.  Tk uses it's
 *	own error handler so this call should never be called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This function will call panic and exit.
 *
 *----------------------------------------------------------------------
 */

static int
DefaultErrorHandler(
    Display* display,
    XErrorEvent* err_evt)
{
    /*
     * This call should never be called.  Tk replaces
     * it with its own error handler.
     */
    panic("Warning hit bogus error handler!");
    return 0;
}


char *
XGetAtomName(
    Display * display,
    Atom atom)
{
    display->request++;
    return NULL;
}

int
_XInitImageFuncPtrs(XImage *image)
{
    return 0;
}

XErrorHandler
XSetErrorHandler(
    XErrorHandler handler)
{
    return DefaultErrorHandler;
}

Window
XRootWindow(Display *display, int screen_number)
{
    display->request++;
    return ROOT_ID;
}

XImage *
XGetImage(display, d, x, y, width, height, plane_mask, format)
    Display *display;
    Drawable d;
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    unsigned long plane_mask;
    int format;
{
    Debugger();
    return NULL;
}

int
XGetGeometry(display, d, root_return, x_return, y_return, width_return,
	height_return, border_width_return, depth_return)
    Display* display;
    Drawable d;
    Window* root_return;
    int* x_return;
    int* y_return;
    unsigned int* width_return;
    unsigned int* height_return;
    unsigned int* border_width_return;
    unsigned int* depth_return;
{
    /* Used in tkCanvPs.c & wm code */
    Debugger();
    return 0;
}

void
XChangeProperty(
    Display* display,
    Window w,
    Atom property,
    Atom type,
    int format,
    int mode,
    _Xconst unsigned char* data,
    int nelements)
{
    Debugger();
}

void
XSelectInput(
    Display* display,
    Window w,
    long event_mask)
{
    Debugger();
}

void
XBell(
    Display* display,
    int percent)
{
    SysBeep(percent);
}

void
XSetWMNormalHints(
    Display* display,
    Window w,
    XSizeHints* hints)
{
    /*
     * Do nothing.  Shouldn't even be called.
     */
}

XSizeHints *
XAllocSizeHints()
{
    /*
     * Always return NULL.  Tk code checks to see if NULL
     * is returned & does nothing if it is.
     */
    
    return NULL;
}

XImage * 
XCreateImage(
    Display* display,
    Visual* visual,
    unsigned int depth,
    int format,
    int offset,
    char* data,
    unsigned int width,
    unsigned int height,
    int bitmap_pad,
    int bytes_per_line)
{ 
    XImage *ximage;

    display->request++;
    ximage = (XImage *) ckalloc(sizeof(XImage));

    ximage->height = height;
    ximage->width = width;
    ximage->depth = depth;
    ximage->xoffset = offset;
    ximage->format = format;
    ximage->data = data;
    ximage->bitmap_pad = bitmap_pad;
    if (bytes_per_line == 0) {
	ximage->bytes_per_line = width * 4;  /* assuming 32 bits per pixel */
    } else {
	ximage->bytes_per_line = bytes_per_line;
    }

    if (format == ZPixmap) {
	ximage->bits_per_pixel = 32;
	ximage->bitmap_unit = 32;
    } else {
	ximage->bits_per_pixel = 1;
	ximage->bitmap_unit = 8;
    }
    ximage->byte_order = LSBFirst;
    ximage->bitmap_bit_order = LSBFirst;
    ximage->red_mask = 0x00FF0000;
    ximage->green_mask = 0x0000FF00;
    ximage->blue_mask = 0x000000FF;

    ximage->f.destroy_image = TkMacXDestroyImage;
    ximage->f.get_pixel = TkMacXGetPixel;
    ximage->f.put_pixel = TkMacXPutPixel;
    ximage->f.sub_image = TkMacXSubImage;
    ximage->f.add_pixel = TkMacXAddPixel;

    return ximage;
}

GContext
XGContextFromGC(
    GC gc)
{
    /* TODO - currently a no-op */
    return 0;
}

Status
XSendEvent(
    Display* display,
    Window w,
    Bool propagate,
    long event_mask,
    XEvent* event_send)
{
    Debugger();
    return 0;
}

int
XGetWindowProperty(
    Display *display,
    Window w,
    Atom property,
    long long_offset,
    long long_length,
    Bool delete,
    Atom req_type,
    Atom *actual_type_return,
    int *actual_format_return,
    unsigned long *nitems_return,
    unsigned long *bytes_after_return,
    unsigned char ** prop_return)
{
    display->request++;
    *actual_type_return = None;
    *actual_format_return = *bytes_after_return = 0;
    *nitems_return = 0;
    return 0;
}

void
XRefreshKeyboardMapping()
{
    /* used by tkXEvent.c */
    Debugger();
}

void 
XSetIconName(
    Display* display,
    Window w,
    const char *icon_name)
{
    /*
     * This is a no-op, no icon name for Macs.
     */
    display->request++;
}

void 
XForceScreenSaver(
    Display* display,
    int mode)
{
    /* 
     * This function is just a no-op.  It is defined to 
     * reset the screen saver.  However, there is no real
     * way to do this on a Mac.  Let me know if there is!
     */
    display->request++;
}

void
Tk_FreeXId (
    Display *display,
    XID xid)
{
    /* no-op function needed for stubs implementation. */
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetServerInfo --
 *
 *	Given a window, this procedure returns information about
 *	the window server for that window.  This procedure provides
 *	the guts of the "winfo server" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkGetServerInfo(
    Tcl_Interp *interp,		/* The server information is returned in
				 * this interpreter's result. */
    Tk_Window tkwin)		/* Token for window;  this selects a
				 * particular display and server. */
{
    char buffer[8 + TCL_INTEGER_SPACE * 2];
    char buffer2[TCL_INTEGER_SPACE];

    sprintf(buffer, "X%dR%d ", ProtocolVersion(Tk_Display(tkwin)),
	    ProtocolRevision(Tk_Display(tkwin)));
    sprintf(buffer2, " %d", VendorRelease(Tk_Display(tkwin)));
    Tcl_AppendResult(interp, buffer, ServerVendor(Tk_Display(tkwin)),
	    buffer2, (char *) NULL);
}
/*
 * Image stuff 
 */

int 
TkMacXDestroyImage(
    XImage *image)
{
    Debugger();
    return 0;
}

unsigned long 
TkMacXGetPixel(
    XImage *image,
    int x,
    int y)
{
    Debugger();
    return 0;
}

int 
TkMacXPutPixel(
    XImage *image,
    int x,
    int y,
    unsigned long pixel)
{
    /* Debugger(); */
    return 0;
}

XImage *
TkMacXSubImage(
    XImage *image,
    int x,
    int y,
    unsigned int width,
    unsigned int height)
{
    Debugger();
    return NULL;
}

int 
TkMacXAddPixel(
    XImage *image,
    long value)
{
    Debugger();
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * XChangeWindowAttributes, XSetWindowBackground,
 * XSetWindowBackgroundPixmap, XSetWindowBorder, XSetWindowBorderPixmap,
 * XSetWindowBorderWidth, XSetWindowColormap
 *
 *	These functions are all no-ops.  They all have equivilent
 *	Tk calls that should always be used instead.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
XChangeWindowAttributes(
    Display* display,
    Window w,
    unsigned long value_mask,
    XSetWindowAttributes* attributes)
{
}

void 
XSetWindowBackground(
	Display *display,
	Window window,
	unsigned long value)
{
}

void
XSetWindowBackgroundPixmap(
    Display* display,
    Window w,
    Pixmap background_pixmap)
{
}

void
XSetWindowBorder(
    Display* display,
    Window w,
    unsigned long border_pixel)
{
}

void
XSetWindowBorderPixmap(
    Display* display,
    Window w,
    Pixmap border_pixmap)
{
}

void
XSetWindowBorderWidth(
    Display* display,
    Window w,
    unsigned int width)
{
}

void
XSetWindowColormap(
    Display* display,
    Window w,
    Colormap colormap)
{
    Debugger();
}

Status		
XStringListToTextProperty(
    char** list, 
    int count, 
    XTextProperty* text_prop_return)
{
    Debugger();
    return (Status) 0;
}
void		
XSetWMClientMachine(
    Display* display, 
    Window w, 
    XTextProperty* text_prop)
{
    Debugger();
}
XIC		
XCreateIC(
    void)
{
    Debugger();
    return (XIC) 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetDefaultScreenName --
 *
 *	Returns the name of the screen that Tk should use during
 *	initialization.
 *
 * Results:
 *	Returns a statically allocated string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
TkGetDefaultScreenName(
    Tcl_Interp *interp,		/* Not used. */
    char *screenName)		/* If NULL, use default string. */
{
    if ((screenName == NULL) || (screenName[0] == '\0')) {
	screenName = macScreenName;
    }
    return screenName;
}
