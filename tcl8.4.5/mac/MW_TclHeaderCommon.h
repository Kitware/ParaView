/*
 * MW_TclHeaderCommon.h --
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

/*
 * Place any includes below that will are needed by the majority of the
 * and is OK to be in any file in the system.
 */

#include "tcl.h"

#ifdef BUILD_tcl
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif
#include "tclMac.h"
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

#include "tclInt.h"


#if PRAGMA_IMPORT
#pragma import on
#endif

#include <MoreFiles.h>
#include <MoreFilesExtras.h>
#include <FSpCompat.h>
#include <FileCopy.h>
#include <FullPath.h>
#include <IterateDirectory.h>
#include <MoreDesktopMgr.h>
#include <DirectoryCopy.h>
#include <Search.h>

#ifdef PRAGMA_IMPORT_OFF
#pragma import off
#elif PRAGMA_IMPORT
#pragma import reset
#endif
