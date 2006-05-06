/*
 * tkMacOSXFont.h --
 *
 *        Contains the Macintosh implementation of the platform-independant
 *        font package interface.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef  TKMACOSXFONT_H
#define  TKMACOSXFONT_H  1

#include "tkFont.h"

#include <Carbon/Carbon.h>

/*
 * Function prototypes
 */

extern void  TkMacOSXInitControlFontStyle(Tk_Font tkfont,
          ControlFontStylePtr fsPtr);

#endif  /*TKMACOSXFONT_H*/
