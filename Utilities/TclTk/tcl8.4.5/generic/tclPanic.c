/* 
 * tclPanic.c --
 *
 *	Source code for the "Tcl_Panic" library procedure for Tcl;
 *	individual applications will probably call Tcl_SetPanicProc()
 *	to set an application-specific panic procedure.
 *
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"

/*
 * The panicProc variable contains a pointer to an application
 * specific panic procedure.
 */

static Tcl_PanicProc *panicProc = NULL;

/*
 * The platformPanicProc variable contains a pointer to a platform
 * specific panic procedure, if any.  ( TclpPanic may be NULL via
 * a macro. )
 */

static Tcl_PanicProc * CONST platformPanicProc = TclpPanic;


/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetPanicProc --
 *
 *	Replace the default panic behavior with the specified functiion.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the panicProc variable.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetPanicProc(proc)
    Tcl_PanicProc *proc;
{
    panicProc = proc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_PanicVA --
 *
 *	Print an error message and kill the process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process dies, entering the debugger if possible.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_PanicVA (format, argList)
    CONST char *format;		/* Format string, suitable for passing to
				 * fprintf. */
    va_list argList;		/* Variable argument list. */
{
    char *arg1, *arg2, *arg3, *arg4;	/* Additional arguments (variable in
					 * number) to pass to fprintf. */
    char *arg5, *arg6, *arg7, *arg8;

    arg1 = va_arg(argList, char *);
    arg2 = va_arg(argList, char *);
    arg3 = va_arg(argList, char *);
    arg4 = va_arg(argList, char *);
    arg5 = va_arg(argList, char *);
    arg6 = va_arg(argList, char *);
    arg7 = va_arg(argList, char *);
    arg8 = va_arg(argList, char *);
    
    if (panicProc != NULL) {
	(void) (*panicProc)(format, arg1, arg2, arg3, arg4,
		arg5, arg6, arg7, arg8);
    } else if (platformPanicProc != NULL) {
	(void) (*platformPanicProc)(format, arg1, arg2, arg3, arg4,
		arg5, arg6, arg7, arg8);
    } else {
	(void) fprintf(stderr, format, arg1, arg2, arg3, arg4, arg5, arg6,
		arg7, arg8);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	abort();
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Panic --
 *
 *	Print an error message and kill the process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process dies, entering the debugger if possible.
 *
 *----------------------------------------------------------------------
 */

	/* VARARGS ARGSUSED */
void
Tcl_Panic TCL_VARARGS_DEF(CONST char *,arg1)
{
    va_list argList;
    CONST char *format;

    format = TCL_VARARGS_START(CONST char *,arg1,argList);
    Tcl_PanicVA(format, argList);
    va_end (argList);
}
