/*
 * tclMacMSLPrefix.h --
 *
 *  A wrapper for the MSL ansi_prefix.mac.h file.  This just turns export on
 *  after including the MSL prefix file, so we can export symbols from the MSL
 *  and through the Tcl shared libraries
 *  
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <ansi_prefix.mac.h>
/*
 * "export" is a MetroWerks specific pragma.  It flags the linker that  
 * any symbols that are defined when this pragma is on will be exported 
 * to shared libraries that link with this library.
 */
 
#pragma export on
