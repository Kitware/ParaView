/*
 * MW_TkHeaderCommon.h --
 *
 * 	Common includes for precompiled headers
 *
 * Copyright (c) 1998 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#pragma once

#include "tclMacCommonPch.h"

#ifdef TCL_DEBUG
    #define TK_TEST
#endif

/*
 * The following defines are for the Xlib.h file to force 
 * it to generate prototypes in the way we need it.  This is
 * defined here in case X.h & company are ever included before
 * tk.h.
 */

#define NeedFunctionPrototypes 1
#define NeedWidePrototypes 0

/*
 * Place any includes below that will are needed by the majority of the
 * and is OK to be in any file in the system.
 */

#include "tcl.h"

#include "tk.h"
#include "tkInt.h"
