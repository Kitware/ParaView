/* 
 * tclTestProcBodyObj.c --
 *
 *	Implements the "procbodytest" package, which contains commands
 *	to test creation of Tcl procedures whose body argument is a
 *	Tcl_Obj of type "procbody" rather than a string.
 *
 * Copyright (c) 1998 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"

/*
 * name and version of this package
 */

static char packageName[] = "procbodytest";
static char packageVersion[] = "1.0";

/*
 * Name of the commands exported by this package
 */

static char procCommand[] = "proc";

/*
 * this struct describes an entry in the table of command names and command
 * procs
 */

typedef struct CmdTable
{
    char *cmdName;		/* command name */
    Tcl_ObjCmdProc *proc;	/* command proc */
    int exportIt;		/* if 1, export the command */
} CmdTable;

/*
 * Declarations for functions defined in this file.
 */

static int	ProcBodyTestProcObjCmd _ANSI_ARGS_((ClientData dummy,
			Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]));
static int	ProcBodyTestInitInternal _ANSI_ARGS_((Tcl_Interp *interp,
			int isSafe));
static int	RegisterCommand _ANSI_ARGS_((Tcl_Interp* interp,
			char *namespace, CONST CmdTable *cmdTablePtr));
int             Procbodytest_Init _ANSI_ARGS_((Tcl_Interp * interp));
int             Procbodytest_SafeInit _ANSI_ARGS_((Tcl_Interp * interp));

/*
 * List of commands to create when the package is loaded; must go after the
 * declarations of the enable command procedure.
 */

static CONST CmdTable commands[] =
{
    { procCommand,	ProcBodyTestProcObjCmd,	1 },

    { 0, 0, 0 }
};

static CONST CmdTable safeCommands[] =
{
    { procCommand,	ProcBodyTestProcObjCmd,	1 },

    { 0, 0, 0 }
};

/*
 *----------------------------------------------------------------------
 *
 * Procbodytest_Init --
 *
 *  This procedure initializes the "procbodytest" package.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

int
Procbodytest_Init(interp)
    Tcl_Interp *interp;		/* the Tcl interpreter for which the package
                                 * is initialized */
{
    return ProcBodyTestInitInternal(interp, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * Procbodytest_SafeInit --
 *
 *  This procedure initializes the "procbodytest" package.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

int
Procbodytest_SafeInit(interp)
    Tcl_Interp *interp;		/* the Tcl interpreter for which the package
                                 * is initialized */
{
    return ProcBodyTestInitInternal(interp, 1);
}

/*
 *----------------------------------------------------------------------
 *
 * RegisterCommand --
 *
 *  This procedure registers a command in the context of the given namespace.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static int RegisterCommand(interp, namespace, cmdTablePtr)
    Tcl_Interp* interp;			/* the Tcl interpreter for which the
                                         * operation is performed */
    char *namespace;			/* the namespace in which the command
                                         * is registered */
    CONST CmdTable *cmdTablePtr;	/* the command to register */
{
    char buf[128];

    if (cmdTablePtr->exportIt) {
        sprintf(buf, "namespace eval %s { namespace export %s }",
                namespace, cmdTablePtr->cmdName);
        if (Tcl_Eval(interp, buf) != TCL_OK)
            return TCL_ERROR;
    }
    
    sprintf(buf, "%s::%s", namespace, cmdTablePtr->cmdName);
    Tcl_CreateObjCommand(interp, buf, cmdTablePtr->proc, 0, 0);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ProcBodyTestInitInternal --
 *
 *  This procedure initializes the Loader package.
 *  The isSafe flag is 1 if the interpreter is safe, 0 otherwise.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static int
ProcBodyTestInitInternal(interp, isSafe)
    Tcl_Interp *interp;		/* the Tcl interpreter for which the package
                                 * is initialized */
    int isSafe;			/* 1 if this is a safe interpreter */
{
    CONST CmdTable *cmdTablePtr;

    cmdTablePtr = (isSafe) ? &safeCommands[0] : &commands[0];
    for ( ; cmdTablePtr->cmdName ; cmdTablePtr++) {
        if (RegisterCommand(interp, packageName, cmdTablePtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    
    return Tcl_PkgProvide(interp, packageName, packageVersion);
}

/*
 *----------------------------------------------------------------------
 *
 * ProcBodyTestProcObjCmd --
 *
 *  Implements the "procbodytest::proc" command. Here is the command
 *  description:
 *	procbodytest::proc newName argList bodyName
 *  Looks up a procedure called $bodyName and, if the procedure exists,
 *  constructs a Tcl_Obj of type "procbody" and calls Tcl_ProcObjCmd.
 *  Arguments:
 *    newName		the name of the procedure to be created
 *    argList		the argument list for the procedure
 *    bodyName		the name of an existing procedure from which the
 *			body is to be copied.
 *  This command can be used to trigger the branches in Tcl_ProcObjCmd that
 *  construct a proc from a "procbody", for example:
 *	proc a {x} {return $x}
 *	a 123
 *	procbodytest::proc b {x} a
 *  Note the call to "a 123", which is necessary so that the Proc pointer
 *  for "a" is filled in by the internal compiler; this is a hack.
 *
 * Results:
 *  Returns a standard Tcl code.
 *
 * Side effects:
 *  A new procedure is created.
 *  Leaves an error message in the interp's result on error.
 *
 *----------------------------------------------------------------------
 */

static int
ProcBodyTestProcObjCmd (dummy, interp, objc, objv)
    ClientData dummy;		/* context; not used */
    Tcl_Interp *interp;		/* the current interpreter */
    int objc;			/* argument count */
    Tcl_Obj *CONST objv[];	/* arguments */
{
    char *fullName;
    Tcl_Command procCmd;
    Command *cmdPtr;
    Proc *procPtr = (Proc *) NULL;
    Tcl_Obj *bodyObjPtr;
    Tcl_Obj *myobjv[5];
    int result;
    
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "newName argsList bodyName");
	return TCL_ERROR;
    }

    /*
     * Find the Command pointer to this procedure
     */
    
    fullName = Tcl_GetStringFromObj(objv[3], (int *) NULL);
    procCmd = Tcl_FindCommand(interp, fullName, (Tcl_Namespace *) NULL,
            TCL_LEAVE_ERR_MSG);
    if (procCmd == NULL) {
        return TCL_ERROR;
    }

    cmdPtr = (Command *) procCmd;

    /*
     * check that this is a procedure and not a builtin command:
     * If a procedure, cmdPtr->objProc is either 0 or TclObjInterpProc,
     * and cmdPtr->proc is either 0 or TclProcInterpProc.
     * Also, the compile proc should be 0, but we don't check for that.
     */

    if (((cmdPtr->objProc != NULL)
            && (cmdPtr->objProc != TclGetObjInterpProc()))
            || ((cmdPtr->proc != NULL)
                    && (cmdPtr->proc != TclGetInterpProc()))) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"command \"", fullName,
		"\" is not a Tcl procedure", (char *) NULL);
        return TCL_ERROR;
    }

    /*
     * it is a Tcl procedure: the client data is the Proc structure
     */
    
    if (cmdPtr->objProc != NULL) {
        procPtr = (Proc *) cmdPtr->objClientData;
    } else if (cmdPtr->proc != NULL) {
        procPtr = (Proc *) cmdPtr->clientData;
    }

    if (procPtr == NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"procedure \"", fullName,
		"\" does not have a Proc struct!", (char *) NULL);
        return TCL_ERROR;
    }
        
    /*
     * create a new object, initialize our argument vector, call into Tcl
     */

    bodyObjPtr = TclNewProcBodyObj(procPtr);
    if (bodyObjPtr == NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"failed to create a procbody object for procedure \"",
                fullName, "\"", (char *) NULL);
        return TCL_ERROR;
    }
    Tcl_IncrRefCount(bodyObjPtr);

    myobjv[0] = objv[0];
    myobjv[1] = objv[1];
    myobjv[2] = objv[2];
    myobjv[3] = bodyObjPtr;
    myobjv[4] = (Tcl_Obj *) NULL;

    result = Tcl_ProcObjCmd((ClientData) NULL, interp, objc, myobjv);
    Tcl_DecrRefCount(bodyObjPtr);

    return result;
}
