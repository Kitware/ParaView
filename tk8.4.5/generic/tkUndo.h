/*
 * tkUndo.h --
 *
 *	Declarations shared among the files that implement an undo
 *	stack.
 *
 * Copyright (c) 2002 Ludwig Callewaert.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TKUNDO
#define _TKUNDO

#ifndef _TK
#include "tk.h"
#endif

#ifdef BUILD_tk
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

/* enum definining the types used in an undo stack */

typedef enum {
    TK_UNDO_SEPARATOR,			/* Marker */
    TK_UNDO_ACTION			   /* Command */
} TkUndoAtomType;

/* struct defining the basic undo/redo stack element */

typedef struct TkUndoAtom {
    TkUndoAtomType type;		 /* The type that will trigger the
					 * required action*/
    Tcl_Obj * apply;			   /* Command to apply the action that was taken */
    Tcl_Obj * revert;			/* The command to undo the action */
    struct TkUndoAtom * next;	/* Pointer to the next element in the
					 * stack */
} TkUndoAtom;

/* struct defining the basic undo/redo stack element */

typedef struct TkUndoRedoStack {
    TkUndoAtom * undoStack;		 /* The undo stack */
    TkUndoAtom * redoStack;		 /* The redo stack */
    Tcl_Interp * interp   ;       /* The interpreter in which to execute the revert and apply scripts */
    int          maxdepth;
    int          depth;
} TkUndoRedoStack;

/* basic functions */

EXTERN void TkUndoPushStack  _ANSI_ARGS_((TkUndoAtom ** stack,
    TkUndoAtom *  elem));

EXTERN TkUndoAtom * TkUndoPopStack _ANSI_ARGS_((TkUndoAtom ** stack));
 
EXTERN int TkUndoInsertSeparator _ANSI_ARGS_((TkUndoAtom ** stack));

EXTERN void TkUndoClearStack _ANSI_ARGS_((TkUndoAtom ** stack));

/* functions working on an undo/redo stack */

EXTERN TkUndoRedoStack * TkUndoInitStack _ANSI_ARGS_((Tcl_Interp * interp,
    int maxdepth));

EXTERN void TkUndoSetDepth _ANSI_ARGS_((TkUndoRedoStack * stack,
    int maxdepth));

EXTERN void TkUndoClearStacks _ANSI_ARGS_((TkUndoRedoStack * stack));

EXTERN void TkUndoFreeStack _ANSI_ARGS_((TkUndoRedoStack * stack));

EXTERN void TkUndoInsertUndoSeparator _ANSI_ARGS_((TkUndoRedoStack * stack));

EXTERN void TkUndoPushAction _ANSI_ARGS_((TkUndoRedoStack * stack,
    Tcl_DString * actionScript, Tcl_DString * revertScript));

EXTERN int TkUndoRevert _ANSI_ARGS_((TkUndoRedoStack *  stack));
 
EXTERN int TkUndoApply _ANSI_ARGS_((TkUndoRedoStack *  stack));

# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _TKUNDO */
