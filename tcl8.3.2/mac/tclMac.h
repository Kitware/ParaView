/*
 * tclMac.h --
 *
 *	Declarations of Macintosh specific public variables and procedures.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TCLMAC
#define _TCLMAC

#ifndef _TCL
#   include "tcl.h"
#endif
#include <Types.h>
#include <Files.h>
#include <Events.h>

/*
 * "export" is a MetroWerks specific pragma.  It flags the linker that  
 * any symbols that are defined when this pragma is on will be exported 
 * to shared libraries that link with this library.
 */
 
#pragma export on

typedef int (*Tcl_MacConvertEventPtr) _ANSI_ARGS_((EventRecord *eventPtr));

#include "tclPlatDecls.h"

#pragma export reset

#endif /* _TCLMAC */
