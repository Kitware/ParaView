/* 
 * tkUndo.c --
 *
 *	This module provides the implementation of an undo stack.
 *
 * Copyright (c) 2002 by Ludwig Callewaert.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkUndo.h"


/*
 * TkUndoPushStack
 *    Push elem on the stack identified by stack.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */
 
void TkUndoPushStack ( stack, elem )
    TkUndoAtom ** stack;
    TkUndoAtom *  elem;
{ 
    elem->next = *stack;
    *stack = elem;
}

/*
 * TkUndoPopStack --
 *    Remove and return the top element from the stack identified by 
 *      stack.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */
 
TkUndoAtom * TkUndoPopStack ( stack )
    TkUndoAtom ** stack ;
{ 
    TkUndoAtom * elem = NULL;
    if (*stack != NULL ) {
        elem   = *stack;
        *stack = elem->next;
    }
    return elem;
}

/*
 * TkUndoInsertSeparator --
 *    insert a separator on the stack, indicating a border for
 *      an undo/redo chunk.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */
 
int TkUndoInsertSeparator ( stack )
    TkUndoAtom ** stack;
{
    TkUndoAtom * separator;

    if ( *stack != NULL && (*stack)->type != TK_UNDO_SEPARATOR ) {
        separator = (TkUndoAtom *) ckalloc(sizeof(TkUndoAtom));
        separator->type = TK_UNDO_SEPARATOR;
        TkUndoPushStack(stack,separator);
        return 1;
    } else {
        return 0;
    }
}

/*
 * TkUndoClearStack --
 *    Clear an entire undo or redo stack and destroy all elements in it.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */

void TkUndoClearStack ( stack )
    TkUndoAtom ** stack;      /* An Undo or Redo stack */
{
    TkUndoAtom * elem;

    while ( (elem = TkUndoPopStack(stack)) ) {
        if ( elem->type != TK_UNDO_SEPARATOR ) {
            Tcl_DecrRefCount(elem->apply);
            Tcl_DecrRefCount(elem->revert);
        }
        ckfree((char *)elem);
    }
    *stack = NULL;
}

/*
 * TkUndoPushAction
 *    Push a new elem on the stack identified by stack.
 *    action and revert are given through Tcl_DStrings
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */
 
void TkUndoPushAction ( stack, actionScript, revertScript )
    TkUndoRedoStack * stack;      /* An Undo or Redo stack */
    Tcl_DString * actionScript; /* The script to get the action (redo) */
    Tcl_DString * revertScript; /* The script to revert the action (undo) */
{ 
    TkUndoAtom * atom;

    atom = (TkUndoAtom *) ckalloc(sizeof(TkUndoAtom));
    atom->type = TK_UNDO_ACTION;

    atom->apply = Tcl_NewStringObj(Tcl_DStringValue(actionScript),Tcl_DStringLength(actionScript));
    Tcl_IncrRefCount(atom->apply);

    atom->revert = Tcl_NewStringObj(Tcl_DStringValue(revertScript),Tcl_DStringLength(revertScript));
    Tcl_IncrRefCount(atom->revert);

    TkUndoPushStack(&(stack->undoStack), atom);
    TkUndoClearStack(&(stack->redoStack));
}


/*
 * TkUndoInitStack
 *    Initialize a new undo/redo stack
 *
 * Results:
 *    un Undo/Redo stack pointer
 *
 * Side effects:
 *    None.
 */
 
TkUndoRedoStack * TkUndoInitStack ( interp, maxdepth )
    Tcl_Interp * interp;          /* The interpreter */
    int          maxdepth;        /* The maximum stack depth */
{ 
    TkUndoRedoStack * stack;      /* An Undo/Redo stack */
    stack = (TkUndoRedoStack *) ckalloc(sizeof(TkUndoRedoStack));
    stack->undoStack = NULL;
    stack->redoStack = NULL;
    stack->interp    = interp;
    stack->maxdepth  = maxdepth; 
    stack->depth     = 0;
    return stack;
}


/*
 * TkUndoInitStack
 *    Initialize a new undo/redo stack
 *
 * Results:
 *    un Undo/Redo stack pointer
 *
 * Side effects:
 *    None.
 */
 
void TkUndoSetDepth ( stack, maxdepth )
    TkUndoRedoStack * stack;           /* An Undo/Redo stack */
    int               maxdepth;        /* The maximum stack depth */
{
    TkUndoAtom * elem;
    TkUndoAtom * prevelem;
    int sepNumber = 0;
    
    stack->maxdepth = maxdepth;

    if ((stack->maxdepth > 0) && (stack->depth > stack->maxdepth)) {
        /* Maximum stack depth exceeded. We have to remove the last compound
           elements on the stack */
        elem = stack->undoStack;
        prevelem = NULL;
        while ( sepNumber <= stack->maxdepth ) {
            if (elem != NULL && (elem->type == TK_UNDO_SEPARATOR) ) {
                sepNumber++;
            }
            prevelem = elem;
            elem = elem->next;
        }
        prevelem->next = NULL;
        while ( elem ) {
           prevelem = elem;
           elem = elem->next;
           ckfree((char *) elem);
        }
        stack->depth = stack->maxdepth;
    }
}


/*
 * TkUndoClearStacks
 *    Clear both the undo and redo stack
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */
 
void TkUndoClearStacks ( stack )
    TkUndoRedoStack * stack;      /* An Undo/Redo stack */
{ 
    TkUndoClearStack(&(stack->undoStack));
    TkUndoClearStack(&(stack->redoStack));
    stack->depth = 0;
}


/*
 * TkUndoFreeStack
 *    Clear both the undo and redo stack
 *    also free the memory allocated to the u/r stack pointer
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */
 
void TkUndoFreeStack ( stack )
    TkUndoRedoStack * stack;      /* An Undo/Redo stack */
{ 
   TkUndoClearStacks(stack);
/*   ckfree((TkUndoRedoStack *) stack); */
   ckfree((char *) stack);
}


/*
 * TkUndoInsertUndoSeparator --
 *    insert a separator on the undo stack, indicating a border for
 *      an undo/redo chunk.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    None.
 */
 
void TkUndoInsertUndoSeparator ( stack )
    TkUndoRedoStack * stack;
{
/*    TkUndoAtom * elem;
    TkUndoAtom * prevelem;
    int sepNumber = 0;
*/
    
    if ( TkUndoInsertSeparator(&(stack->undoStack)) ) {
        ++(stack->depth);
        TkUndoSetDepth(stack,stack->maxdepth);
/*        if ((stack->maxdepth > 0) && (stack->depth > stack->maxdepth)) {
            elem = stack->undoStack;
            prevelem = NULL;
            while ( sepNumber < stack->depth ) {
                if (elem != NULL && (elem->type == TK_UNDO_SEPARATOR) ) {
                    sepNumber++;
                }
                prevelem = elem;
                elem = elem->next;
            }
            prevelem->next = NULL;
            while ( elem ) {
               prevelem = elem;
               elem = elem->next;
               ckfree((char *) elem);
            }
            stack->depth;
        } */
    }
}


/*
 * TkUndoRevert --
 *    Undo a compound action on the stack.
 *
 * Results:
 *    A TCL status code
 *
 * Side effects:
 *    None.
 */
 
int TkUndoRevert ( stack )
    TkUndoRedoStack * stack;
{
    TkUndoAtom * elem;

    /* insert a separator on the undo and the redo stack */

    TkUndoInsertUndoSeparator(stack);
    TkUndoInsertSeparator(&(stack->redoStack));

    /* Pop and skip the first separator if there is one*/

    elem = TkUndoPopStack(&(stack->undoStack));

    if ( elem == NULL ) {
        return TCL_ERROR;
    }

    if ( ( elem != NULL ) && ( elem->type == TK_UNDO_SEPARATOR ) ) {
        ckfree((char *) elem);
        elem = TkUndoPopStack(&(stack->undoStack));
    }
    
    while ( elem && (elem->type != TK_UNDO_SEPARATOR) ) {
        Tcl_EvalObjEx(stack->interp,elem->revert,TCL_EVAL_GLOBAL);
        
        TkUndoPushStack(&(stack->redoStack),elem);
        elem = TkUndoPopStack(&(stack->undoStack));
    }
    
    /* insert a separator on the redo stack */
    
    TkUndoInsertSeparator(&(stack->redoStack));
    
    --(stack->depth);
    
    return TCL_OK;
}


/*
 * TkUndoApply --
 *    Redo a compound action on the stack.
 *
 * Results:
 *    A TCL status code
 *
 * Side effects:
 *    None.
 */
 
int TkUndoApply ( stack )
    TkUndoRedoStack * stack;
{
    TkUndoAtom *elem;

    /* insert a separator on the undo stack */

    TkUndoInsertSeparator(&(stack->undoStack));

    /* Pop and skip the first separator if there is one*/

    elem = TkUndoPopStack(&(stack->redoStack));

    if ( elem == NULL ) {
       return TCL_ERROR;
    }

    if ( ( elem != NULL ) && ( elem->type == TK_UNDO_SEPARATOR ) ) {
        ckfree((char *) elem);
        elem = TkUndoPopStack(&(stack->redoStack));
    }

    while ( elem && (elem->type != TK_UNDO_SEPARATOR) ) {
        Tcl_EvalObjEx(stack->interp,elem->apply,TCL_EVAL_GLOBAL);
        
        TkUndoPushStack(&(stack->undoStack), elem);
        elem = TkUndoPopStack(&(stack->redoStack));
    }

    /* insert a separator on the undo stack */
    
    TkUndoInsertSeparator(&(stack->undoStack));

    ++(stack->depth);
    
    return TCL_OK;
}

