/* 
 * tclInterp.c --
 *
 *	This file implements the "interp" command which allows creation
 *	and manipulation of Tcl interpreters from within Tcl scripts.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"
#include <stdio.h>

/*
 * Counter for how many aliases were created (global)
 */

static int aliasCounter = 0;
TCL_DECLARE_MUTEX(cntMutex)

/*
 * struct Alias:
 *
 * Stores information about an alias. Is stored in the slave interpreter
 * and used by the source command to find the target command in the master
 * when the source command is invoked.
 */

typedef struct Alias {
    Tcl_Obj *namePtr;		/* Name of alias command in slave interp. */
    Tcl_Interp *targetInterp;	/* Interp in which target command will be
				 * invoked. */
    Tcl_Command slaveCmd;	/* Source command in slave interpreter,
				 * bound to command that invokes the target
				 * command in the target interpreter. */
    Tcl_HashEntry *aliasEntryPtr;
				/* Entry for the alias hash table in slave.
                                 * This is used by alias deletion to remove
                                 * the alias from the slave interpreter
                                 * alias table. */
    Tcl_HashEntry *targetEntryPtr;
				/* Entry for target command in master.
                                 * This is used in the master interpreter to
                                 * map back from the target command to aliases
                                 * redirecting to it. Random access to this
                                 * hash table is never required - we are using
                                 * a hash table only for convenience. */
    int objc;                   /* Count of Tcl_Obj in the prefix of the
				 * target command to be invoked in the
				 * target interpreter. Additional arguments
				 * specified when calling the alias in the
				 * slave interp will be appended to the prefix
				 * before the command is invoked. */
    Tcl_Obj *objPtr;            /* The first actual prefix object - the target
				 * command name; this has to be at the end of the 
				 * structure, which will be extended to accomodate 
				 * the remaining objects in the prefix. */
} Alias;

/*
 *
 * struct Slave:
 *
 * Used by the "interp" command to record and find information about slave
 * interpreters. Maps from a command name in the master to information about
 * a slave interpreter, e.g. what aliases are defined in it.
 */

typedef struct Slave {
    Tcl_Interp *masterInterp;	/* Master interpreter for this slave. */
    Tcl_HashEntry *slaveEntryPtr;
				/* Hash entry in masters slave table for
                                 * this slave interpreter.  Used to find
                                 * this record, and used when deleting the
                                 * slave interpreter to delete it from the
                                 * master's table. */
    Tcl_Interp	*slaveInterp;	/* The slave interpreter. */
    Tcl_Command interpCmd;	/* Interpreter object command. */
    Tcl_HashTable aliasTable;	/* Table which maps from names of commands
                                 * in slave interpreter to struct Alias
                                 * defined below. */
} Slave;

/*
 * struct Target:
 *
 * Maps from master interpreter commands back to the source commands in slave
 * interpreters. This is needed because aliases can be created between sibling
 * interpreters and must be deleted when the target interpreter is deleted. In
 * case they would not be deleted the source interpreter would be left with a
 * "dangling pointer". One such record is stored in the Master record of the
 * master interpreter (in the targetTable hashtable, see below) with the
 * master for each alias which directs to a command in the master. These
 * records are used to remove the source command for an from a slave if/when
 * the master is deleted.
 */

typedef struct Target {
    Tcl_Command	slaveCmd;	/* Command for alias in slave interp. */
    Tcl_Interp *slaveInterp;	/* Slave Interpreter. */
} Target;

/*
 * struct Master:
 *
 * This record is used for two purposes: First, slaveTable (a hashtable)
 * maps from names of commands to slave interpreters. This hashtable is
 * used to store information about slave interpreters of this interpreter,
 * to map over all slaves, etc. The second purpose is to store information
 * about all aliases in slaves (or siblings) which direct to target commands
 * in this interpreter (using the targetTable hashtable).
 * 
 * NB: the flags field in the interp structure, used with SAFE_INTERP
 * mask denotes whether the interpreter is safe or not. Safe
 * interpreters have restricted functionality, can only create safe slave
 * interpreters and can only load safe extensions.
 */

typedef struct Master {
    Tcl_HashTable slaveTable;	/* Hash table for slave interpreters.
                                 * Maps from command names to Slave records. */
    Tcl_HashTable targetTable;	/* Hash table for Target Records. Contains
                                 * all Target records which denote aliases
                                 * from slaves or sibling interpreters that
                                 * direct to commands in this interpreter. This
                                 * table is used to remove dangling pointers
                                 * from the slave (or sibling) interpreters
                                 * when this interpreter is deleted. */
} Master;

/*
 * The following structure keeps track of all the Master and Slave information
 * on a per-interp basis.
 */

typedef struct InterpInfo {
    Master master;		/* Keeps track of all interps for which this
				 * interp is the Master. */
    Slave slave;		/* Information necessary for this interp to
				 * function as a slave. */
} InterpInfo;

/*
 * Prototypes for local static procedures:
 */

static int		AliasCreate _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, Tcl_Interp *masterInterp,
			    Tcl_Obj *namePtr, Tcl_Obj *targetPtr, int objc,
			    Tcl_Obj *CONST objv[]));
static int		AliasDelete _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, Tcl_Obj *namePtr));
static int		AliasDescribe _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, Tcl_Obj *objPtr));
static int		AliasList _ANSI_ARGS_((Tcl_Interp *interp,
		            Tcl_Interp *slaveInterp));
static int		AliasObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *currentInterp, int objc,
		            Tcl_Obj *CONST objv[]));
static void		AliasObjCmdDeleteProc _ANSI_ARGS_((
			    ClientData clientData));

static Tcl_Interp *	GetInterp _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *pathPtr));
static Tcl_Interp *	GetInterp2 _ANSI_ARGS_((Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static void		InterpInfoDeleteProc _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp));
static Tcl_Interp *	SlaveCreate _ANSI_ARGS_((Tcl_Interp *interp,
		            Tcl_Obj *pathPtr, int safe));
static int		SlaveEval _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, int objc,
			    Tcl_Obj *CONST objv[]));
static int		SlaveExpose _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, int objc,
			    Tcl_Obj *CONST objv[]));
static int		SlaveHide _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, int objc,
			    Tcl_Obj *CONST objv[]));
static int		SlaveHidden _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp));
static int		SlaveInvokeHidden _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, int global, int objc,
			    Tcl_Obj *CONST objv[]));
static int		SlaveMarkTrusted _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp));
static int		SlaveObjCmd _ANSI_ARGS_((ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *CONST objv[]));
static void		SlaveObjCmdDeleteProc _ANSI_ARGS_((
			    ClientData clientData));
static int		SlaveRecursionLimit _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, int objc,
			    Tcl_Obj *CONST objv[]));


/*
 *---------------------------------------------------------------------------
 *
 * TclInterpInit --
 *
 *	Initializes the invoking interpreter for using the master, slave
 *	and safe interp facilities.  This is called from inside
 *	Tcl_CreateInterp().
 *
 * Results:
 *	Always returns TCL_OK for backwards compatibility.
 *
 * Side effects:
 *	Adds the "interp" command to an interpreter and initializes the
 *	interpInfoPtr field of the invoking interpreter.
 *
 *---------------------------------------------------------------------------
 */

int
TclInterpInit(interp)
    Tcl_Interp *interp;			/* Interpreter to initialize. */
{
    InterpInfo *interpInfoPtr;
    Master *masterPtr;
    Slave *slavePtr;	

    interpInfoPtr = (InterpInfo *) ckalloc(sizeof(InterpInfo));
    ((Interp *) interp)->interpInfo = (ClientData) interpInfoPtr;

    masterPtr = &interpInfoPtr->master;
    Tcl_InitHashTable(&masterPtr->slaveTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&masterPtr->targetTable, TCL_ONE_WORD_KEYS);

    slavePtr = &interpInfoPtr->slave;
    slavePtr->masterInterp	= NULL;
    slavePtr->slaveEntryPtr	= NULL;
    slavePtr->slaveInterp	= interp;
    slavePtr->interpCmd		= NULL;
    Tcl_InitHashTable(&slavePtr->aliasTable, TCL_STRING_KEYS);

    Tcl_CreateObjCommand(interp, "interp", Tcl_InterpObjCmd, NULL, NULL);

    Tcl_CallWhenDeleted(interp, InterpInfoDeleteProc, NULL);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InterpInfoDeleteProc --
 *
 *	Invoked when an interpreter is being deleted.  It releases all
 *	storage used by the master/slave/safe interpreter facilities.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cleans up storage.  Sets the interpInfoPtr field of the interp
 *	to NULL.
 *
 *---------------------------------------------------------------------------
 */

static void
InterpInfoDeleteProc(clientData, interp)
    ClientData clientData;	/* Ignored. */
    Tcl_Interp *interp;		/* Interp being deleted.  All commands for
				 * slave interps should already be deleted. */
{
    InterpInfo *interpInfoPtr;
    Slave *slavePtr;
    Master *masterPtr;
    Tcl_HashSearch hSearch;
    Tcl_HashEntry *hPtr;
    Target *targetPtr;

    interpInfoPtr = (InterpInfo *) ((Interp *) interp)->interpInfo;

    /*
     * There shouldn't be any commands left.
     */

    masterPtr = &interpInfoPtr->master;
    if (masterPtr->slaveTable.numEntries != 0) {
	panic("InterpInfoDeleteProc: still exist commands");
    }
    Tcl_DeleteHashTable(&masterPtr->slaveTable);

    /*
     * Tell any interps that have aliases to this interp that they should
     * delete those aliases.  If the other interp was already dead, it
     * would have removed the target record already. 
     */

    hPtr = Tcl_FirstHashEntry(&masterPtr->targetTable, &hSearch);
    while (hPtr != NULL) {
	targetPtr = (Target *) Tcl_GetHashValue(hPtr);
	Tcl_DeleteCommandFromToken(targetPtr->slaveInterp,
		targetPtr->slaveCmd);
	hPtr = Tcl_NextHashEntry(&hSearch);
    }
    Tcl_DeleteHashTable(&masterPtr->targetTable);

    slavePtr = &interpInfoPtr->slave;
    if (slavePtr->interpCmd != NULL) {
	/*
	 * Tcl_DeleteInterp() was called on this interpreter, rather
	 * "interp delete" or the equivalent deletion of the command in the
	 * master.  First ensure that the cleanup callback doesn't try to
	 * delete the interp again.
	 */

	slavePtr->slaveInterp = NULL;
        Tcl_DeleteCommandFromToken(slavePtr->masterInterp,
		slavePtr->interpCmd);
    }

    /*
     * There shouldn't be any aliases left.
     */

    if (slavePtr->aliasTable.numEntries != 0) {
	panic("InterpInfoDeleteProc: still exist aliases");
    }
    Tcl_DeleteHashTable(&slavePtr->aliasTable);

    ckfree((char *) interpInfoPtr);    
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InterpObjCmd --
 *
 *	This procedure is invoked to process the "interp" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */
	/* ARGSUSED */
int
Tcl_InterpObjCmd(clientData, interp, objc, objv)
    ClientData clientData;		/* Unused. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    int index;
    static CONST char *options[] = {
        "alias",	"aliases",	"create",	"delete", 
	"eval",		"exists",	"expose",	"hide", 
	"hidden",	"issafe",	"invokehidden",	"marktrusted", 
	"recursionlimit",		"slaves",	"share",
	"target",	"transfer",
        NULL
    };
    enum option {
	OPT_ALIAS,	OPT_ALIASES,	OPT_CREATE,	OPT_DELETE,
	OPT_EVAL,	OPT_EXISTS,	OPT_EXPOSE,	OPT_HIDE,
	OPT_HIDDEN,	OPT_ISSAFE,	OPT_INVOKEHID,	OPT_MARKTRUSTED,
	OPT_RECLIMIT,			OPT_SLAVES,	OPT_SHARE,
	OPT_TARGET,	OPT_TRANSFER
    };


    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "cmd ?arg ...?");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, 
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }
    switch ((enum option) index) {
	case OPT_ALIAS: {
	    Tcl_Interp *slaveInterp, *masterInterp;

	    if (objc < 4) {
		aliasArgs:
		Tcl_WrongNumArgs(interp, 2, objv,
			"slavePath slaveCmd ?masterPath masterCmd? ?args ..?");
		return TCL_ERROR;
	    }
	    slaveInterp = GetInterp(interp, objv[2]);
	    if (slaveInterp == (Tcl_Interp *) NULL) {
		return TCL_ERROR;
	    }
	    if (objc == 4) {
		return AliasDescribe(interp, slaveInterp, objv[3]);
	    }
	    if ((objc == 5) && (Tcl_GetString(objv[4])[0] == '\0')) {
		return AliasDelete(interp, slaveInterp, objv[3]);
	    }
	    if (objc > 5) {
		masterInterp = GetInterp(interp, objv[4]);
		if (masterInterp == (Tcl_Interp *) NULL) {
		    return TCL_ERROR;
		}
		if (Tcl_GetString(objv[5])[0] == '\0') {
		    if (objc == 6) {
			return AliasDelete(interp, slaveInterp, objv[3]);
		    }
		} else {
		    return AliasCreate(interp, slaveInterp, masterInterp,
			    objv[3], objv[5], objc - 6, objv + 6);
		}
	    }
	    goto aliasArgs;
	}
	case OPT_ALIASES: {
	    Tcl_Interp *slaveInterp;

	    slaveInterp = GetInterp2(interp, objc, objv);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }
	    return AliasList(interp, slaveInterp);
	}
	case OPT_CREATE: {
	    int i, last, safe;
	    Tcl_Obj *slavePtr;
	    char buf[16 + TCL_INTEGER_SPACE];
	    static CONST char *options[] = {
		"-safe",	"--",		NULL
	    };
	    enum option {
		OPT_SAFE,	OPT_LAST
	    };

	    safe = Tcl_IsSafe(interp);
	    
	    /*
	     * Weird historical rules: "-safe" is accepted at the end, too.
	     */

	    slavePtr = NULL;
	    last = 0;
	    for (i = 2; i < objc; i++) {
		if ((last == 0) && (Tcl_GetString(objv[i])[0] == '-')) {
		    if (Tcl_GetIndexFromObj(interp, objv[i], options, "option",
			    0, &index) != TCL_OK) {
			return TCL_ERROR;
		    }
		    if (index == OPT_SAFE) {
			safe = 1;
			continue;
		    }
		    i++;
		    last = 1;
		}
		if (slavePtr != NULL) {
		    Tcl_WrongNumArgs(interp, 2, objv, "?-safe? ?--? ?path?");
		    return TCL_ERROR;
		}
		if (i < objc) {
		    slavePtr = objv[i];
		}
	    }
	    buf[0] = '\0';
	    if (slavePtr == NULL) {
		/*
		 * Create an anonymous interpreter -- we choose its name and
		 * the name of the command. We check that the command name
		 * that we use for the interpreter does not collide with an
		 * existing command in the master interpreter.
		 */
		
		for (i = 0; ; i++) {
		    Tcl_CmdInfo cmdInfo;
		    
		    sprintf(buf, "interp%d", i);
		    if (Tcl_GetCommandInfo(interp, buf, &cmdInfo) == 0) {
			break;
		    }
		}
		slavePtr = Tcl_NewStringObj(buf, -1);
	    }
	    if (SlaveCreate(interp, slavePtr, safe) == NULL) {
		if (buf[0] != '\0') {
		    Tcl_DecrRefCount(slavePtr);
		}
		return TCL_ERROR;
	    }
	    Tcl_SetObjResult(interp, slavePtr);
	    return TCL_OK;
	}
	case OPT_DELETE: {
	    int i;
	    InterpInfo *iiPtr;
	    Tcl_Interp *slaveInterp;
	    
	    for (i = 2; i < objc; i++) {
		slaveInterp = GetInterp(interp, objv[i]);
		if (slaveInterp == NULL) {
		    return TCL_ERROR;
		} else if (slaveInterp == interp) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			    "cannot delete the current interpreter",
			    (char *) NULL);
		    return TCL_ERROR;
		}
		iiPtr = (InterpInfo *) ((Interp *) slaveInterp)->interpInfo;
		Tcl_DeleteCommandFromToken(iiPtr->slave.masterInterp,
			iiPtr->slave.interpCmd);
	    }
	    return TCL_OK;
	}
	case OPT_EVAL: {
	    Tcl_Interp *slaveInterp;

	    if (objc < 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "path arg ?arg ...?");
		return TCL_ERROR;
	    }
	    slaveInterp = GetInterp(interp, objv[2]);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }
	    return SlaveEval(interp, slaveInterp, objc - 3, objv + 3);
	}
	case OPT_EXISTS: {
	    int exists;
	    Tcl_Interp *slaveInterp;

	    exists = 1;
	    slaveInterp = GetInterp2(interp, objc, objv);
	    if (slaveInterp == NULL) {
		if (objc > 3) {
		    return TCL_ERROR;
		}
		Tcl_ResetResult(interp);
		exists = 0;
	    }
	    Tcl_SetIntObj(Tcl_GetObjResult(interp), exists);
	    return TCL_OK;
	}
	case OPT_EXPOSE: {
	    Tcl_Interp *slaveInterp;

	    if ((objc < 4) || (objc > 5)) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"path hiddenCmdName ?cmdName?");
		return TCL_ERROR;
	    }
	    slaveInterp = GetInterp(interp, objv[2]);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }
	    return SlaveExpose(interp, slaveInterp, objc - 3, objv + 3);
	}
	case OPT_HIDE: {
	    Tcl_Interp *slaveInterp;		/* A slave. */

	    if ((objc < 4) || (objc > 5)) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"path cmdName ?hiddenCmdName?");
		return TCL_ERROR;
	    }
	    slaveInterp = GetInterp(interp, objv[2]);
	    if (slaveInterp == (Tcl_Interp *) NULL) {
		return TCL_ERROR;
	    }
	    return SlaveHide(interp, slaveInterp, objc - 3, objv + 3);
	}
	case OPT_HIDDEN: {
	    Tcl_Interp *slaveInterp;		/* A slave. */

	    slaveInterp = GetInterp2(interp, objc, objv);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }
	    return SlaveHidden(interp, slaveInterp);
	}
	case OPT_ISSAFE: {
	    Tcl_Interp *slaveInterp;

	    slaveInterp = GetInterp2(interp, objc, objv);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }
	    Tcl_SetIntObj(Tcl_GetObjResult(interp), Tcl_IsSafe(slaveInterp));
	    return TCL_OK;
	}
	case OPT_INVOKEHID: {
	    int i, index, global;
	    Tcl_Interp *slaveInterp;
	    static CONST char *hiddenOptions[] = {
		"-global",	"--",		NULL
	    };
	    enum hiddenOption {
		OPT_GLOBAL,	OPT_LAST
	    };

	    global = 0;
	    for (i = 3; i < objc; i++) {
		if (Tcl_GetString(objv[i])[0] != '-') {
		    break;
		}
		if (Tcl_GetIndexFromObj(interp, objv[i], hiddenOptions,
			"option", 0, &index) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (index == OPT_GLOBAL) {
		    global = 1;
		} else {
		    i++;
		    break;
		}
	    }
	    if (objc - i < 1) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"path ?-global? ?--? cmd ?arg ..?");
		return TCL_ERROR;
	    }
	    slaveInterp = GetInterp(interp, objv[2]);
	    if (slaveInterp == (Tcl_Interp *) NULL) {
		return TCL_ERROR;
	    }
	    return SlaveInvokeHidden(interp, slaveInterp, global, objc - i,
		    objv + i);
	}
	case OPT_MARKTRUSTED: {
	    Tcl_Interp *slaveInterp;

	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "path");
		return TCL_ERROR;
	    }
	    slaveInterp = GetInterp(interp, objv[2]);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }
	    return SlaveMarkTrusted(interp, slaveInterp);
	}
	case OPT_RECLIMIT: {
	    Tcl_Interp *slaveInterp;

	    if (objc != 3 && objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "path ?newlimit?");
		return TCL_ERROR;
	    }
	    slaveInterp = GetInterp(interp, objv[2]);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }
	    return SlaveRecursionLimit(interp, slaveInterp, objc - 3, objv + 3);
	}
	case OPT_SLAVES: {
	    Tcl_Interp *slaveInterp;
	    InterpInfo *iiPtr;
	    Tcl_Obj *resultPtr;
	    Tcl_HashEntry *hPtr;
	    Tcl_HashSearch hashSearch;
	    char *string;
	    
	    slaveInterp = GetInterp2(interp, objc, objv);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }
	    iiPtr = (InterpInfo *) ((Interp *) slaveInterp)->interpInfo;
	    resultPtr = Tcl_GetObjResult(interp);
	    hPtr = Tcl_FirstHashEntry(&iiPtr->master.slaveTable, &hashSearch);
	    for ( ; hPtr != NULL; hPtr = Tcl_NextHashEntry(&hashSearch)) {
		string = Tcl_GetHashKey(&iiPtr->master.slaveTable, hPtr);
		Tcl_ListObjAppendElement(NULL, resultPtr,
			Tcl_NewStringObj(string, -1));
	    }
	    return TCL_OK;
	}
	case OPT_SHARE: {
	    Tcl_Interp *slaveInterp;		/* A slave. */
	    Tcl_Interp *masterInterp;		/* Its master. */
	    Tcl_Channel chan;

	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 2, objv, "srcPath channelId destPath");
		return TCL_ERROR;
	    }
	    masterInterp = GetInterp(interp, objv[2]);
	    if (masterInterp == NULL) {
		return TCL_ERROR;
	    }
	    chan = Tcl_GetChannel(masterInterp, Tcl_GetString(objv[3]),
		    NULL);
	    if (chan == NULL) {
		TclTransferResult(masterInterp, TCL_OK, interp);
		return TCL_ERROR;
	    }
	    slaveInterp = GetInterp(interp, objv[4]);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }
	    Tcl_RegisterChannel(slaveInterp, chan);
	    return TCL_OK;
	}
	case OPT_TARGET: {
	    Tcl_Interp *slaveInterp;
	    InterpInfo *iiPtr;
	    Tcl_HashEntry *hPtr;	
	    Alias *aliasPtr;		
	    char *aliasName;

	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "path alias");
		return TCL_ERROR;
	    }

	    slaveInterp = GetInterp(interp, objv[2]);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }

	    aliasName = Tcl_GetString(objv[3]);

	    iiPtr = (InterpInfo *) ((Interp *) slaveInterp)->interpInfo;
	    hPtr = Tcl_FindHashEntry(&iiPtr->slave.aliasTable, aliasName);
	    if (hPtr == NULL) {
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			"alias \"", aliasName, "\" in path \"",
			Tcl_GetString(objv[2]), "\" not found",
			(char *) NULL);
		return TCL_ERROR;
	    }
	    aliasPtr = (Alias *) Tcl_GetHashValue(hPtr);
	    if (Tcl_GetInterpPath(interp, aliasPtr->targetInterp) != TCL_OK) {
		Tcl_ResetResult(interp);
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			"target interpreter for alias \"", aliasName,
			"\" in path \"", Tcl_GetString(objv[2]),
			"\" is not my descendant", (char *) NULL);
		return TCL_ERROR;
	    }
	    return TCL_OK;
	}
	case OPT_TRANSFER: {
	    Tcl_Interp *slaveInterp;		/* A slave. */
	    Tcl_Interp *masterInterp;		/* Its master. */
	    Tcl_Channel chan;
		    
	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"srcPath channelId destPath");
		return TCL_ERROR;
	    }
	    masterInterp = GetInterp(interp, objv[2]);
	    if (masterInterp == NULL) {
		return TCL_ERROR;
	    }
	    chan = Tcl_GetChannel(masterInterp, Tcl_GetString(objv[3]), NULL);
	    if (chan == NULL) {
		TclTransferResult(masterInterp, TCL_OK, interp);
		return TCL_ERROR;
	    }
	    slaveInterp = GetInterp(interp, objv[4]);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }
	    Tcl_RegisterChannel(slaveInterp, chan);
	    if (Tcl_UnregisterChannel(masterInterp, chan) != TCL_OK) {
		TclTransferResult(masterInterp, TCL_OK, interp);
		return TCL_ERROR;
	    }
	    return TCL_OK;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetInterp2 --
 *
 *	Helper function for Tcl_InterpObjCmd() to convert the interp name
 *	potentially specified on the command line to an Tcl_Interp.
 *
 * Results:
 *	The return value is the interp specified on the command line,
 *	or the interp argument itself if no interp was specified on the
 *	command line.  If the interp could not be found or the wrong
 *	number of arguments was specified on the command line, the return
 *	value is NULL and an error message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
 
static Tcl_Interp *
GetInterp2(interp, objc, objv)
    Tcl_Interp *interp;		/* Default interp if no interp was specified
				 * on the command line. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    if (objc == 2) {
	return interp;
    } else if (objc == 3) {
	return GetInterp(interp, objv[2]);
    } else {
	Tcl_WrongNumArgs(interp, 2, objv, "?path?");
	return NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateAlias --
 *
 *	Creates an alias between two interpreters.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates a new alias, manipulates the result field of slaveInterp.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_CreateAlias(slaveInterp, slaveCmd, targetInterp, targetCmd, argc, argv)
    Tcl_Interp *slaveInterp;	/* Interpreter for source command. */
    CONST char *slaveCmd;	/* Command to install in slave. */
    Tcl_Interp *targetInterp;	/* Interpreter for target command. */
    CONST char *targetCmd;	/* Name of target command. */
    int argc;			/* How many additional arguments? */
    CONST char * CONST *argv;	/* These are the additional args. */
{
    Tcl_Obj *slaveObjPtr, *targetObjPtr;
    Tcl_Obj **objv;
    int i;
    int result;
    
    objv = (Tcl_Obj **) ckalloc((unsigned) sizeof(Tcl_Obj *) * argc);
    for (i = 0; i < argc; i++) {
        objv[i] = Tcl_NewStringObj(argv[i], -1);
        Tcl_IncrRefCount(objv[i]);
    }
    
    slaveObjPtr = Tcl_NewStringObj(slaveCmd, -1);
    Tcl_IncrRefCount(slaveObjPtr);

    targetObjPtr = Tcl_NewStringObj(targetCmd, -1);
    Tcl_IncrRefCount(targetObjPtr);

    result = AliasCreate(slaveInterp, slaveInterp, targetInterp, slaveObjPtr,
	    targetObjPtr, argc, objv);

    for (i = 0; i < argc; i++) {
	Tcl_DecrRefCount(objv[i]);
    }
    ckfree((char *) objv);
    Tcl_DecrRefCount(targetObjPtr);
    Tcl_DecrRefCount(slaveObjPtr);

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateAliasObj --
 *
 *	Object version: Creates an alias between two interpreters.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates a new alias.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_CreateAliasObj(slaveInterp, slaveCmd, targetInterp, targetCmd, objc, objv)
    Tcl_Interp *slaveInterp;	/* Interpreter for source command. */
    CONST char *slaveCmd;	/* Command to install in slave. */
    Tcl_Interp *targetInterp;	/* Interpreter for target command. */
    CONST char *targetCmd;	/* Name of target command. */
    int objc;			/* How many additional arguments? */
    Tcl_Obj *CONST objv[];	/* Argument vector. */
{
    Tcl_Obj *slaveObjPtr, *targetObjPtr;
    int result;

    slaveObjPtr = Tcl_NewStringObj(slaveCmd, -1);
    Tcl_IncrRefCount(slaveObjPtr);

    targetObjPtr = Tcl_NewStringObj(targetCmd, -1);
    Tcl_IncrRefCount(targetObjPtr);

    result = AliasCreate(slaveInterp, slaveInterp, targetInterp, slaveObjPtr,
	    targetObjPtr, objc, objv);

    Tcl_DecrRefCount(slaveObjPtr);
    Tcl_DecrRefCount(targetObjPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetAlias --
 *
 *	Gets information about an alias.
 *
 * Results:
 *	A standard Tcl result. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetAlias(interp, aliasName, targetInterpPtr, targetNamePtr, argcPtr,
        argvPtr)
    Tcl_Interp *interp;			/* Interp to start search from. */
    CONST char *aliasName;			/* Name of alias to find. */
    Tcl_Interp **targetInterpPtr;	/* (Return) target interpreter. */
    CONST char **targetNamePtr;		/* (Return) name of target command. */
    int *argcPtr;			/* (Return) count of addnl args. */
    CONST char ***argvPtr;		/* (Return) additional arguments. */
{
    InterpInfo *iiPtr;
    Tcl_HashEntry *hPtr;
    Alias *aliasPtr;
    int i, objc;
    Tcl_Obj **objv;
    
    iiPtr = (InterpInfo *) ((Interp *) interp)->interpInfo;
    hPtr = Tcl_FindHashEntry(&iiPtr->slave.aliasTable, aliasName);
    if (hPtr == NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "alias \"", aliasName, "\" not found", (char *) NULL);
	return TCL_ERROR;
    }
    aliasPtr = (Alias *) Tcl_GetHashValue(hPtr);
    objc = aliasPtr->objc;
    objv = &aliasPtr->objPtr;

    if (targetInterpPtr != NULL) {
	*targetInterpPtr = aliasPtr->targetInterp;
    }
    if (targetNamePtr != NULL) {
	*targetNamePtr = Tcl_GetString(objv[0]);
    }
    if (argcPtr != NULL) {
	*argcPtr = objc - 1;
    }
    if (argvPtr != NULL) {
        *argvPtr = (CONST char **) 
		ckalloc((unsigned) sizeof(CONST char *) * (objc - 1));
        for (i = 1; i < objc; i++) {
            *argvPtr[i - 1] = Tcl_GetString(objv[i]);
        }
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetAliasObj --
 *
 *	Object version: Gets information about an alias.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetAliasObj(interp, aliasName, targetInterpPtr, targetNamePtr, objcPtr,
        objvPtr)
    Tcl_Interp *interp;			/* Interp to start search from. */
    CONST char *aliasName;		/* Name of alias to find. */
    Tcl_Interp **targetInterpPtr;	/* (Return) target interpreter. */
    CONST char **targetNamePtr;		/* (Return) name of target command. */
    int *objcPtr;			/* (Return) count of addnl args. */
    Tcl_Obj ***objvPtr;			/* (Return) additional args. */
{
    InterpInfo *iiPtr;
    Tcl_HashEntry *hPtr;
    Alias *aliasPtr;	
    int objc;
    Tcl_Obj **objv;

    iiPtr = (InterpInfo *) ((Interp *) interp)->interpInfo;
    hPtr = Tcl_FindHashEntry(&iiPtr->slave.aliasTable, aliasName);
    if (hPtr == (Tcl_HashEntry *) NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "alias \"", aliasName, "\" not found", (char *) NULL);
        return TCL_ERROR;
    }
    aliasPtr = (Alias *) Tcl_GetHashValue(hPtr);
    objc = aliasPtr->objc;
    objv = &aliasPtr->objPtr;

    if (targetInterpPtr != (Tcl_Interp **) NULL) {
        *targetInterpPtr = aliasPtr->targetInterp;
    }
    if (targetNamePtr != (CONST char **) NULL) {
        *targetNamePtr = Tcl_GetString(objv[0]);
    }
    if (objcPtr != (int *) NULL) {
        *objcPtr = objc - 1;
    }
    if (objvPtr != (Tcl_Obj ***) NULL) {
        *objvPtr = objv + 1;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclPreventAliasLoop --
 *
 *	When defining an alias or renaming a command, prevent an alias
 *	loop from being formed.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	If TCL_ERROR is returned, the function also stores an error message
 *	in the interpreter's result object.
 *
 * NOTE:
 *	This function is public internal (instead of being static to
 *	this file) because it is also used from TclRenameCommand.
 *
 *----------------------------------------------------------------------
 */

int
TclPreventAliasLoop(interp, cmdInterp, cmd)
    Tcl_Interp *interp;			/* Interp in which to report errors. */
    Tcl_Interp *cmdInterp;		/* Interp in which the command is
                                         * being defined. */
    Tcl_Command cmd;                    /* Tcl command we are attempting
                                         * to define. */
{
    Command *cmdPtr = (Command *) cmd;
    Alias *aliasPtr, *nextAliasPtr;
    Tcl_Command aliasCmd;
    Command *aliasCmdPtr;

    /*
     * If we are not creating or renaming an alias, then it is
     * always OK to create or rename the command.
     */
    
    if (cmdPtr->objProc != AliasObjCmd) {
        return TCL_OK;
    }

    /*
     * OK, we are dealing with an alias, so traverse the chain of aliases.
     * If we encounter the alias we are defining (or renaming to) any in
     * the chain then we have a loop.
     */

    aliasPtr = (Alias *) cmdPtr->objClientData;
    nextAliasPtr = aliasPtr;
    while (1) {
	Tcl_Obj *cmdNamePtr;

        /*
         * If the target of the next alias in the chain is the same as
         * the source alias, we have a loop.
	 */

	if (Tcl_InterpDeleted(nextAliasPtr->targetInterp)) {
	    /*
	     * The slave interpreter can be deleted while creating the alias.
	     * [Bug #641195]
	     */

	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "cannot define or rename alias \"",
		    Tcl_GetString(aliasPtr->namePtr),
		    "\": interpreter deleted", (char *) NULL);
	    return TCL_ERROR;
	}
	cmdNamePtr = nextAliasPtr->objPtr;
	aliasCmd = Tcl_FindCommand(nextAliasPtr->targetInterp,
                Tcl_GetString(cmdNamePtr),
		Tcl_GetGlobalNamespace(nextAliasPtr->targetInterp),
		/*flags*/ 0);
        if (aliasCmd == (Tcl_Command) NULL) {
            return TCL_OK;
        }
	aliasCmdPtr = (Command *) aliasCmd;
        if (aliasCmdPtr == cmdPtr) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "cannot define or rename alias \"",
		    Tcl_GetString(aliasPtr->namePtr),
		    "\": would create a loop", (char *) NULL);
            return TCL_ERROR;
        }

        /*
	 * Otherwise, follow the chain one step further. See if the target
         * command is an alias - if so, follow the loop to its target
         * command. Otherwise we do not have a loop.
	 */

        if (aliasCmdPtr->objProc != AliasObjCmd) {
            return TCL_OK;
        }
        nextAliasPtr = (Alias *) aliasCmdPtr->objClientData;
    }

    /* NOTREACHED */
}

/*
 *----------------------------------------------------------------------
 *
 * AliasCreate --
 *
 *	Helper function to do the work to actually create an alias.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	An alias command is created and entered into the alias table
 *	for the slave interpreter.
 *
 *----------------------------------------------------------------------
 */

static int
AliasCreate(interp, slaveInterp, masterInterp, namePtr, targetNamePtr,
	objc, objv)
    Tcl_Interp *interp;		/* Interp for error reporting. */
    Tcl_Interp *slaveInterp;	/* Interp where alias cmd will live or from
				 * which alias will be deleted. */
    Tcl_Interp *masterInterp;	/* Interp in which target command will be
				 * invoked. */
    Tcl_Obj *namePtr;		/* Name of alias cmd. */
    Tcl_Obj *targetNamePtr;	/* Name of target cmd. */
    int objc;			/* Additional arguments to store */
    Tcl_Obj *CONST objv[];	/* with alias. */
{
    Alias *aliasPtr;
    Tcl_HashEntry *hPtr;
    Target *targetPtr;
    Slave *slavePtr;
    Master *masterPtr;
    Tcl_Obj **prefv;
    int new, i;

    aliasPtr = (Alias *) ckalloc((unsigned) (sizeof(Alias) 
            + objc * sizeof(Tcl_Obj *)));
    aliasPtr->namePtr		= namePtr;
    Tcl_IncrRefCount(aliasPtr->namePtr);
    aliasPtr->targetInterp	= masterInterp;

    aliasPtr->objc = objc + 1;
    prefv = &aliasPtr->objPtr;

    *prefv = targetNamePtr;
    Tcl_IncrRefCount(targetNamePtr);
    for (i = 0; i < objc; i++) {
	*(++prefv) = objv[i];
	Tcl_IncrRefCount(objv[i]);
    }

    Tcl_Preserve(slaveInterp);
    Tcl_Preserve(masterInterp);

    aliasPtr->slaveCmd = Tcl_CreateObjCommand(slaveInterp,
	    Tcl_GetString(namePtr), AliasObjCmd, (ClientData) aliasPtr,
	    AliasObjCmdDeleteProc);

    if (TclPreventAliasLoop(interp, slaveInterp,
	    aliasPtr->slaveCmd) != TCL_OK) {
	/*
	 * Found an alias loop!	 The last call to Tcl_CreateObjCommand made
	 * the alias point to itself.  Delete the command and its alias
	 * record.  Be careful to wipe out its client data first, so the
	 * command doesn't try to delete itself.
	 */

	Command *cmdPtr;
	
	Tcl_DecrRefCount(aliasPtr->namePtr);
	Tcl_DecrRefCount(targetNamePtr);
	for (i = 0; i < objc; i++) {
	    Tcl_DecrRefCount(objv[i]);
	}
	
	cmdPtr = (Command *) aliasPtr->slaveCmd;
	cmdPtr->clientData = NULL;
	cmdPtr->deleteProc = NULL;
	cmdPtr->deleteData = NULL;
	Tcl_DeleteCommandFromToken(slaveInterp, aliasPtr->slaveCmd);

	ckfree((char *) aliasPtr);

	/*
	 * The result was already set by TclPreventAliasLoop.
	 */

	Tcl_Release(slaveInterp);
	Tcl_Release(masterInterp);
	return TCL_ERROR;
    }

    /*
     * Make an entry in the alias table. If it already exists delete
     * the alias command. Then retry.
     */

    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;
    while (1) {
	Alias *oldAliasPtr;
	char *string;
	
	string = Tcl_GetString(namePtr);
	hPtr = Tcl_CreateHashEntry(&slavePtr->aliasTable, string, &new);
	if (new != 0) {
	    break;
	}

	oldAliasPtr = (Alias *) Tcl_GetHashValue(hPtr);
	Tcl_DeleteCommandFromToken(slaveInterp, oldAliasPtr->slaveCmd);
    }

    aliasPtr->aliasEntryPtr = hPtr;
    Tcl_SetHashValue(hPtr, (ClientData) aliasPtr);
    
    /*
     * Create the new command. We must do it after deleting any old command,
     * because the alias may be pointing at a renamed alias, as in:
     *
     * interp alias {} foo {} bar		# Create an alias "foo"
     * rename foo zop				# Now rename the alias
     * interp alias {} foo {} zop		# Now recreate "foo"...
     */

    targetPtr = (Target *) ckalloc((unsigned) sizeof(Target));
    targetPtr->slaveCmd = aliasPtr->slaveCmd;
    targetPtr->slaveInterp = slaveInterp;

    Tcl_MutexLock(&cntMutex);
    masterPtr = &((InterpInfo *) ((Interp *) masterInterp)->interpInfo)->master;
    do {
        hPtr = Tcl_CreateHashEntry(&masterPtr->targetTable,
                (char *) aliasCounter, &new);
	aliasCounter++;
    } while (new == 0);
    Tcl_MutexUnlock(&cntMutex);

    Tcl_SetHashValue(hPtr, (ClientData) targetPtr);
    aliasPtr->targetEntryPtr = hPtr;

    Tcl_SetObjResult(interp, namePtr);

    Tcl_Release(slaveInterp);
    Tcl_Release(masterInterp);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AliasDelete --
 *
 *	Deletes the given alias from the slave interpreter given.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Deletes the alias from the slave interpreter.
 *
 *----------------------------------------------------------------------
 */

static int
AliasDelete(interp, slaveInterp, namePtr)
    Tcl_Interp *interp;		/* Interpreter for result & errors. */
    Tcl_Interp *slaveInterp;	/* Interpreter containing alias. */
    Tcl_Obj *namePtr;		/* Name of alias to delete. */
{
    Slave *slavePtr;
    Alias *aliasPtr;
    Tcl_HashEntry *hPtr;

    /*
     * If the alias has been renamed in the slave, the master can still use
     * the original name (with which it was created) to find the alias to
     * delete it.
     */

    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;
    hPtr = Tcl_FindHashEntry(&slavePtr->aliasTable, Tcl_GetString(namePtr));
    if (hPtr == NULL) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), "alias \"",
		Tcl_GetString(namePtr), "\" not found", NULL);
        return TCL_ERROR;
    }
    aliasPtr = (Alias *) Tcl_GetHashValue(hPtr);
    Tcl_DeleteCommandFromToken(slaveInterp, aliasPtr->slaveCmd);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AliasDescribe --
 *
 *	Sets the interpreter's result object to a Tcl list describing
 *	the given alias in the given interpreter: its target command
 *	and the additional arguments to prepend to any invocation
 *	of the alias.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
AliasDescribe(interp, slaveInterp, namePtr)
    Tcl_Interp *interp;		/* Interpreter for result & errors. */
    Tcl_Interp *slaveInterp;	/* Interpreter containing alias. */
    Tcl_Obj *namePtr;		/* Name of alias to describe. */
{
    Slave *slavePtr;
    Tcl_HashEntry *hPtr;
    Alias *aliasPtr;	
    Tcl_Obj *prefixPtr;

    /*
     * If the alias has been renamed in the slave, the master can still use
     * the original name (with which it was created) to find the alias to
     * describe it.
     */

    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;
    hPtr = Tcl_FindHashEntry(&slavePtr->aliasTable, Tcl_GetString(namePtr));
    if (hPtr == NULL) {
        return TCL_OK;
    }
    aliasPtr = (Alias *) Tcl_GetHashValue(hPtr);
    prefixPtr = Tcl_NewListObj(aliasPtr->objc, &aliasPtr->objPtr);
    Tcl_SetObjResult(interp, prefixPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AliasList --
 *
 *	Computes a list of aliases defined in a slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
AliasList(interp, slaveInterp)
    Tcl_Interp *interp;		/* Interp for data return. */
    Tcl_Interp *slaveInterp;	/* Interp whose aliases to compute. */
{
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch hashSearch;
    Tcl_Obj *resultPtr;	
    Alias *aliasPtr;
    Slave *slavePtr;

    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;
    resultPtr = Tcl_GetObjResult(interp);

    entryPtr = Tcl_FirstHashEntry(&slavePtr->aliasTable, &hashSearch);
    for ( ; entryPtr != NULL; entryPtr = Tcl_NextHashEntry(&hashSearch)) {
        aliasPtr = (Alias *) Tcl_GetHashValue(entryPtr);
        Tcl_ListObjAppendElement(NULL, resultPtr, aliasPtr->namePtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AliasObjCmd --
 *
 *	This is the procedure that services invocations of aliases in a
 *	slave interpreter. One such command exists for each alias. When
 *	invoked, this procedure redirects the invocation to the target
 *	command in the master interpreter as designated by the Alias
 *	record associated with this command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Causes forwarding of the invocation; all possible side effects
 *	may occur as a result of invoking the command to which the
 *	invocation is forwarded.
 *
 *----------------------------------------------------------------------
 */

static int
AliasObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Alias record. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument vector. */	
{
#define ALIAS_CMDV_PREALLOC 10
    Tcl_Interp *targetInterp;	
    Alias *aliasPtr;		
    int result, prefc, cmdc, i;
    Tcl_Obj **prefv, **cmdv;
    Tcl_Obj *cmdArr[ALIAS_CMDV_PREALLOC];
    aliasPtr = (Alias *) clientData;
    targetInterp = aliasPtr->targetInterp;

    /*
     * Append the arguments to the command prefix and invoke the command
     * in the target interp's global namespace.
     */
     
    prefc = aliasPtr->objc;
    prefv = &aliasPtr->objPtr;
    cmdc = prefc + objc - 1;
    if (cmdc <= ALIAS_CMDV_PREALLOC) {
	cmdv = cmdArr;
    } else {
	cmdv = (Tcl_Obj **) ckalloc((unsigned) (cmdc * sizeof(Tcl_Obj *)));
    }

    prefv = &aliasPtr->objPtr;
    memcpy((VOID *) cmdv, (VOID *) prefv, 
            (size_t) (prefc * sizeof(Tcl_Obj *)));
    memcpy((VOID *) (cmdv+prefc), (VOID *) (objv+1), 
	    (size_t) ((objc-1) * sizeof(Tcl_Obj *)));

    Tcl_ResetResult(targetInterp);

    for (i=0; i<cmdc; i++) {
	Tcl_IncrRefCount(cmdv[i]);
    }
    if (targetInterp != interp) {
	Tcl_Preserve((ClientData) targetInterp);
	result = Tcl_EvalObjv(targetInterp, cmdc, cmdv, TCL_EVAL_INVOKE);
	TclTransferResult(targetInterp, result, interp);	
	Tcl_Release((ClientData) targetInterp);
    } else {
	result = Tcl_EvalObjv(targetInterp, cmdc, cmdv, TCL_EVAL_INVOKE);
    }
    for (i=0; i<cmdc; i++) {
	Tcl_DecrRefCount(cmdv[i]);
    }

    if (cmdv != cmdArr) {
	ckfree((char *) cmdv);
    }
    return result;        
#undef ALIAS_CMDV_PREALLOC
}

/*
 *----------------------------------------------------------------------
 *
 * AliasObjCmdDeleteProc --
 *
 *	Is invoked when an alias command is deleted in a slave. Cleans up
 *	all storage associated with this alias.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes the alias record and its entry in the alias table for
 *	the interpreter.
 *
 *----------------------------------------------------------------------
 */

static void
AliasObjCmdDeleteProc(clientData)
    ClientData clientData;	/* The alias record for this alias. */
{
    Alias *aliasPtr;		
    Target *targetPtr;		
    int i;
    Tcl_Obj **objv;

    aliasPtr = (Alias *) clientData;
    
    Tcl_DecrRefCount(aliasPtr->namePtr);
    objv = &aliasPtr->objPtr;
    for (i = 0; i < aliasPtr->objc; i++) {
	Tcl_DecrRefCount(objv[i]);
    }
    Tcl_DeleteHashEntry(aliasPtr->aliasEntryPtr);

    targetPtr = (Target *) Tcl_GetHashValue(aliasPtr->targetEntryPtr);
    ckfree((char *) targetPtr);
    Tcl_DeleteHashEntry(aliasPtr->targetEntryPtr);

    ckfree((char *) aliasPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateSlave --
 *
 *	Creates a slave interpreter. The slavePath argument denotes the
 *	name of the new slave relative to the current interpreter; the
 *	slave is a direct descendant of the one-before-last component of
 *	the path, e.g. it is a descendant of the current interpreter if
 *	the slavePath argument contains only one component. Optionally makes
 *	the slave interpreter safe.
 *
 * Results:
 *	Returns the interpreter structure created, or NULL if an error
 *	occurred.
 *
 * Side effects:
 *	Creates a new interpreter and a new interpreter object command in
 *	the interpreter indicated by the slavePath argument.
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *
Tcl_CreateSlave(interp, slavePath, isSafe)
    Tcl_Interp *interp;		/* Interpreter to start search at. */
    CONST char *slavePath;	/* Name of slave to create. */
    int isSafe;			/* Should new slave be "safe" ? */
{
    Tcl_Obj *pathPtr;
    Tcl_Interp *slaveInterp;

    pathPtr = Tcl_NewStringObj(slavePath, -1);
    slaveInterp = SlaveCreate(interp, pathPtr, isSafe);
    Tcl_DecrRefCount(pathPtr);

    return slaveInterp;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetSlave --
 *
 *	Finds a slave interpreter by its path name.
 *
 * Results:
 *	Returns a Tcl_Interp * for the named interpreter or NULL if not
 *	found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *
Tcl_GetSlave(interp, slavePath)
    Tcl_Interp *interp;		/* Interpreter to start search from. */
    CONST char *slavePath;	/* Path of slave to find. */
{
    Tcl_Obj *pathPtr;
    Tcl_Interp *slaveInterp;

    pathPtr = Tcl_NewStringObj(slavePath, -1);
    slaveInterp = GetInterp(interp, pathPtr);
    Tcl_DecrRefCount(pathPtr);

    return slaveInterp;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetMaster --
 *
 *	Finds the master interpreter of a slave interpreter.
 *
 * Results:
 *	Returns a Tcl_Interp * for the master interpreter or NULL if none.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *
Tcl_GetMaster(interp)
    Tcl_Interp *interp;		/* Get the master of this interpreter. */
{
    Slave *slavePtr;		/* Slave record of this interpreter. */

    if (interp == (Tcl_Interp *) NULL) {
        return NULL;
    }
    slavePtr = &((InterpInfo *) ((Interp *) interp)->interpInfo)->slave;
    return slavePtr->masterInterp;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetInterpPath --
 *
 *	Sets the result of the asking interpreter to a proper Tcl list
 *	containing the names of interpreters between the asking and
 *	target interpreters. The target interpreter must be either the
 *	same as the asking interpreter or one of its slaves (including
 *	recursively).
 *
 * Results:
 *	TCL_OK if the target interpreter is the same as, or a descendant
 *	of, the asking interpreter; TCL_ERROR else. This way one can
 *	distinguish between the case where the asking and target interps
 *	are the same (an empty list is the result, and TCL_OK is returned)
 *	and when the target is not a descendant of the asking interpreter
 *	(in which case the Tcl result is an error message and the function
 *	returns TCL_ERROR).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetInterpPath(askingInterp, targetInterp)
    Tcl_Interp *askingInterp;	/* Interpreter to start search from. */
    Tcl_Interp *targetInterp;	/* Interpreter to find. */
{
    InterpInfo *iiPtr;
    
    if (targetInterp == askingInterp) {
        return TCL_OK;
    }
    if (targetInterp == NULL) {
	return TCL_ERROR;
    }
    iiPtr = (InterpInfo *) ((Interp *) targetInterp)->interpInfo;
    if (Tcl_GetInterpPath(askingInterp, iiPtr->slave.masterInterp) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_AppendElement(askingInterp,
	    Tcl_GetHashKey(&iiPtr->master.slaveTable,
		    iiPtr->slave.slaveEntryPtr));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetInterp --
 *
 *	Helper function to find a slave interpreter given a pathname.
 *
 * Results:
 *	Returns the slave interpreter known by that name in the calling
 *	interpreter, or NULL if no interpreter known by that name exists. 
 *
 * Side effects:
 *	Assigns to the pointer variable passed in, if not NULL.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Interp *
GetInterp(interp, pathPtr)
    Tcl_Interp *interp;		/* Interp. to start search from. */
    Tcl_Obj *pathPtr;		/* List object containing name of interp. to 
				 * be found. */
{
    Tcl_HashEntry *hPtr;	/* Search element. */
    Slave *slavePtr;		/* Interim slave record. */
    Tcl_Obj **objv;
    int objc, i;	
    Tcl_Interp *searchInterp;	/* Interim storage for interp. to find. */
    InterpInfo *masterInfoPtr;

    if (Tcl_ListObjGetElements(interp, pathPtr, &objc, &objv) != TCL_OK) {
	return NULL;
    }

    searchInterp = interp;
    for (i = 0; i < objc; i++) {
	masterInfoPtr = (InterpInfo *) ((Interp *) searchInterp)->interpInfo;
        hPtr = Tcl_FindHashEntry(&masterInfoPtr->master.slaveTable,
		Tcl_GetString(objv[i]));
        if (hPtr == NULL) {
	    searchInterp = NULL;
	    break;
	}
        slavePtr = (Slave *) Tcl_GetHashValue(hPtr);
        searchInterp = slavePtr->slaveInterp;
        if (searchInterp == NULL) {
	    break;
	}
    }
    if (searchInterp == NULL) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"could not find interpreter \"",
                Tcl_GetString(pathPtr), "\"", (char *) NULL);
    }
    return searchInterp;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveCreate --
 *
 *	Helper function to do the actual work of creating a slave interp
 *	and new object command. Also optionally makes the new slave
 *	interpreter "safe".
 *
 * Results:
 *	Returns the new Tcl_Interp * if successful or NULL if not. If failed,
 *	the result of the invoking interpreter contains an error message.
 *
 * Side effects:
 *	Creates a new slave interpreter and a new object command.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Interp *
SlaveCreate(interp, pathPtr, safe)
    Tcl_Interp *interp;		/* Interp. to start search from. */
    Tcl_Obj *pathPtr;		/* Path (name) of slave to create. */
    int safe;			/* Should we make it "safe"? */
{
    Tcl_Interp *masterInterp, *slaveInterp;
    Slave *slavePtr;
    InterpInfo *masterInfoPtr;
    Tcl_HashEntry *hPtr;
    char *path;
    int new, objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, pathPtr, &objc, &objv) != TCL_OK) {
	return NULL;
    }
    if (objc < 2) {
	masterInterp = interp;
	path = Tcl_GetString(pathPtr);
    } else {
	Tcl_Obj *objPtr;
	
	objPtr = Tcl_NewListObj(objc - 1, objv);
	masterInterp = GetInterp(interp, objPtr);
	Tcl_DecrRefCount(objPtr);
	if (masterInterp == NULL) {
	    return NULL;
	}
	path = Tcl_GetString(objv[objc - 1]);
    }
    if (safe == 0) {
	safe = Tcl_IsSafe(masterInterp);
    }

    masterInfoPtr = (InterpInfo *) ((Interp *) masterInterp)->interpInfo;
    hPtr = Tcl_CreateHashEntry(&masterInfoPtr->master.slaveTable, path, &new);
    if (new == 0) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "interpreter named \"", path,
		"\" already exists, cannot create", (char *) NULL);
        return NULL;
    }

    slaveInterp = Tcl_CreateInterp();
    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;
    slavePtr->masterInterp = masterInterp;
    slavePtr->slaveEntryPtr = hPtr;
    slavePtr->slaveInterp = slaveInterp;
    slavePtr->interpCmd = Tcl_CreateObjCommand(masterInterp, path,
            SlaveObjCmd, (ClientData) slaveInterp, SlaveObjCmdDeleteProc);
    Tcl_InitHashTable(&slavePtr->aliasTable, TCL_STRING_KEYS);
    Tcl_SetHashValue(hPtr, (ClientData) slavePtr);
    Tcl_SetVar(slaveInterp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);
    
    /*
     * Inherit the recursion limit.
     */
    ((Interp *) slaveInterp)->maxNestingDepth =
	((Interp *) masterInterp)->maxNestingDepth ;

    if (safe) {
        if (Tcl_MakeSafe(slaveInterp) == TCL_ERROR) {
            goto error;
        }
    } else {
        if (Tcl_Init(slaveInterp) == TCL_ERROR) {
            goto error;
        }
	/*
	 * This will create the "memory" command in slave interpreters
	 * if we compiled with TCL_MEM_DEBUG, otherwise it does nothing.
	 */
	Tcl_InitMemory(slaveInterp);
    }
    return slaveInterp;

    error:
    TclTransferResult(slaveInterp, TCL_ERROR, interp);
    Tcl_DeleteInterp(slaveInterp);

    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveObjCmd --
 *
 *	Command to manipulate an interpreter, e.g. to send commands to it
 *	to be evaluated. One such command exists for each slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See user documentation for details.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* Slave interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Tcl_Interp *slaveInterp;
    int index;
    static CONST char *options[] = {
        "alias",	"aliases",	"eval",		"expose",
        "hide",		"hidden",	"issafe",	"invokehidden",
        "marktrusted",	"recursionlimit", NULL
    };
    enum options {
	OPT_ALIAS,	OPT_ALIASES,	OPT_EVAL,	OPT_EXPOSE,
	OPT_HIDE,	OPT_HIDDEN,	OPT_ISSAFE,	OPT_INVOKEHIDDEN,
	OPT_MARKTRUSTED, OPT_RECLIMIT
    };
    
    slaveInterp = (Tcl_Interp *) clientData;
    if (slaveInterp == NULL) {
	panic("SlaveObjCmd: interpreter has been deleted");
    }

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "cmd ?arg ...?");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
	case OPT_ALIAS: {
	    if (objc > 2) {
		if (objc == 3) {
		    return AliasDescribe(interp, slaveInterp, objv[2]);
		}
		if (Tcl_GetString(objv[3])[0] == '\0') {
		    if (objc == 4) {
			return AliasDelete(interp, slaveInterp, objv[2]);
		    }
		} else {
		    return AliasCreate(interp, slaveInterp, interp, objv[2],
			    objv[3], objc - 4, objv + 4);
		}
	    }
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "aliasName ?targetName? ?args..?");
            return TCL_ERROR;
	}
	case OPT_ALIASES: {
	    if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, (char *) NULL);
		return TCL_ERROR;
	    }
	    return AliasList(interp, slaveInterp);
	}
	case OPT_EVAL: {
	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "arg ?arg ...?");
		return TCL_ERROR;
	    }
	    return SlaveEval(interp, slaveInterp, objc - 2, objv + 2);
	}
        case OPT_EXPOSE: {
	    if ((objc < 3) || (objc > 4)) {
		Tcl_WrongNumArgs(interp, 2, objv, "hiddenCmdName ?cmdName?");
		return TCL_ERROR;
	    }
            return SlaveExpose(interp, slaveInterp, objc - 2, objv + 2);
	}
	case OPT_HIDE: {
	    if ((objc < 3) || (objc > 4)) {
		Tcl_WrongNumArgs(interp, 2, objv, "cmdName ?hiddenCmdName?");
		return TCL_ERROR;
	    }
            return SlaveHide(interp, slaveInterp, objc - 2, objv + 2);
	}
        case OPT_HIDDEN: {
	    if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	    }
            return SlaveHidden(interp, slaveInterp);
	}
        case OPT_ISSAFE: {
	    if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, (char *) NULL);
		return TCL_ERROR;
	    }
	    Tcl_SetIntObj(Tcl_GetObjResult(interp), Tcl_IsSafe(slaveInterp));
	    return TCL_OK;
	}
        case OPT_INVOKEHIDDEN: {
	    int global, i, index;
	    static CONST char *hiddenOptions[] = {
		"-global",	"--",		NULL
	    };
	    enum hiddenOption {
		OPT_GLOBAL,	OPT_LAST
	    };
	    global = 0;
	    for (i = 2; i < objc; i++) {
		if (Tcl_GetString(objv[i])[0] != '-') {
		    break;
		}
		if (Tcl_GetIndexFromObj(interp, objv[i], hiddenOptions,
			"option", 0, &index) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (index == OPT_GLOBAL) {
		    global = 1;
		} else {
		    i++;
		    break;
		}
	    }
	    if (objc - i < 1) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"?-global? ?--? cmd ?arg ..?");
		return TCL_ERROR;
	    }
	    return SlaveInvokeHidden(interp, slaveInterp, global, objc - i,
		    objv + i);
	}
	case OPT_MARKTRUSTED: {
	    if (objc != 2) {
		Tcl_WrongNumArgs(interp, 2, objv, NULL);
		return TCL_ERROR;
	    }
            return SlaveMarkTrusted(interp, slaveInterp);
	}
	case OPT_RECLIMIT: {
	    if (objc != 2 && objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "?newlimit?");
		return TCL_ERROR;
	    }
	    return SlaveRecursionLimit(interp, slaveInterp, objc - 2, objv + 2);
	}
    }

    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveObjCmdDeleteProc --
 *
 *	Invoked when an object command for a slave interpreter is deleted;
 *	cleans up all state associated with the slave interpreter and destroys
 *	the slave interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cleans up all state associated with the slave interpreter and
 *	destroys the slave interpreter.
 *
 *----------------------------------------------------------------------
 */

static void
SlaveObjCmdDeleteProc(clientData)
    ClientData clientData;		/* The SlaveRecord for the command. */
{
    Slave *slavePtr;			/* Interim storage for Slave record. */
    Tcl_Interp *slaveInterp;		/* And for a slave interp. */

    slaveInterp = (Tcl_Interp *) clientData;
    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;

    /*
     * Unlink the slave from its master interpreter.
     */

    Tcl_DeleteHashEntry(slavePtr->slaveEntryPtr);

    /*
     * Set to NULL so that when the InterpInfo is cleaned up in the slave
     * it does not try to delete the command causing all sorts of grief.
     * See SlaveRecordDeleteProc().
     */

    slavePtr->interpCmd = NULL;

    if (slavePtr->slaveInterp != NULL) {
	Tcl_DeleteInterp(slavePtr->slaveInterp);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveEval --
 *
 *	Helper function to evaluate a command in a slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Whatever the command does.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveEval(interp, slaveInterp, objc, objv)
    Tcl_Interp *interp;		/* Interp for error return. */
    Tcl_Interp *slaveInterp;	/* The slave interpreter in which command
				 * will be evaluated. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int result;
    Tcl_Obj *objPtr;
    
    Tcl_Preserve((ClientData) slaveInterp);
    Tcl_AllowExceptions(slaveInterp);

    if (objc == 1) {
	result = Tcl_EvalObjEx(slaveInterp, objv[0], 0);
    } else {
	objPtr = Tcl_ConcatObj(objc, objv);
	Tcl_IncrRefCount(objPtr);
	result = Tcl_EvalObjEx(slaveInterp, objPtr, 0);
	Tcl_DecrRefCount(objPtr);
    }
    TclTransferResult(slaveInterp, result, interp);

    Tcl_Release((ClientData) slaveInterp);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveExpose --
 *
 *	Helper function to expose a command in a slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	After this call scripts in the slave will be able to invoke
 *	the newly exposed command.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveExpose(interp, slaveInterp, objc, objv)
    Tcl_Interp *interp;		/* Interp for error return. */
    Tcl_Interp	*slaveInterp;	/* Interp in which command will be exposed. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument strings. */
{
    char *name;
    
    if (Tcl_IsSafe(interp)) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"permission denied: safe interpreter cannot expose commands",
		(char *) NULL);
	return TCL_ERROR;
    }

    name = Tcl_GetString(objv[(objc == 1) ? 0 : 1]);
    if (Tcl_ExposeCommand(slaveInterp, Tcl_GetString(objv[0]),
	    name) != TCL_OK) {
	TclTransferResult(slaveInterp, TCL_ERROR, interp);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveRecursionLimit --
 *
 *	Helper function to set/query the Recursion limit of an interp
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *      When (objc == 1), slaveInterp will be set to a new recursion
 *	limit of objv[0].
 *
 *----------------------------------------------------------------------
 */

static int
SlaveRecursionLimit(interp, slaveInterp, objc, objv)
    Tcl_Interp *interp;		/* Interp for error return. */
    Tcl_Interp	*slaveInterp;	/* Interp in which limit is set/queried. */
    int objc;			/* Set or Query. */
    Tcl_Obj *CONST objv[];	/* Argument strings. */
{
    Interp *iPtr;
    int limit;

    if (objc) {
	if (Tcl_IsSafe(interp)) {
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "permission denied: ",
		    "safe interpreters cannot change recursion limit",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[0], &limit) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	if (limit <= 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "recursion limit must be > 0", -1));
	    return TCL_ERROR;
	}
	Tcl_SetRecursionLimit(slaveInterp, limit);
	iPtr = (Interp *) slaveInterp;
	if (interp == slaveInterp && iPtr->numLevels > limit) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "falling back due to new recursion limit", -1));
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, objv[0]);
        return TCL_OK;
    } else {
	limit = Tcl_SetRecursionLimit(slaveInterp, 0);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(limit));
        return TCL_OK;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveHide --
 *
 *	Helper function to hide a command in a slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	After this call scripts in the slave will no longer be able
 *	to invoke the named command.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveHide(interp, slaveInterp, objc, objv)
    Tcl_Interp *interp;		/* Interp for error return. */
    Tcl_Interp	*slaveInterp;	/* Interp in which command will be exposed. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument strings. */
{
    char *name;
    
    if (Tcl_IsSafe(interp)) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"permission denied: safe interpreter cannot hide commands",
		(char *) NULL);
	return TCL_ERROR;
    }

    name = Tcl_GetString(objv[(objc == 1) ? 0 : 1]);
    if (Tcl_HideCommand(slaveInterp, Tcl_GetString(objv[0]),
	    name) != TCL_OK) {
	TclTransferResult(slaveInterp, TCL_ERROR, interp);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveHidden --
 *
 *	Helper function to compute list of hidden commands in a slave
 *	interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveHidden(interp, slaveInterp)
    Tcl_Interp *interp;		/* Interp for data return. */
    Tcl_Interp *slaveInterp;	/* Interp whose hidden commands to query. */
{
    Tcl_Obj *listObjPtr;		/* Local object pointer. */
    Tcl_HashTable *hTblPtr;		/* For local searches. */
    Tcl_HashEntry *hPtr;		/* For local searches. */
    Tcl_HashSearch hSearch;		/* For local searches. */
    
    listObjPtr = Tcl_GetObjResult(interp);
    hTblPtr = ((Interp *) slaveInterp)->hiddenCmdTablePtr;
    if (hTblPtr != (Tcl_HashTable *) NULL) {
	for (hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch);
	     hPtr != (Tcl_HashEntry *) NULL;
	     hPtr = Tcl_NextHashEntry(&hSearch)) {

	    Tcl_ListObjAppendElement(NULL, listObjPtr,
		    Tcl_NewStringObj(Tcl_GetHashKey(hTblPtr, hPtr), -1));
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveInvokeHidden --
 *
 *	Helper function to invoke a hidden command in a slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Whatever the hidden command does.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveInvokeHidden(interp, slaveInterp, global, objc, objv)
    Tcl_Interp *interp;		/* Interp for error return. */
    Tcl_Interp *slaveInterp;	/* The slave interpreter in which command
				 * will be invoked. */
    int global;			/* Non-zero to invoke in global namespace. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int result;
    
    if (Tcl_IsSafe(interp)) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp),
		"not allowed to invoke hidden commands from safe interpreter",
		-1);
	return TCL_ERROR;
    }

    Tcl_Preserve((ClientData) slaveInterp);
    Tcl_AllowExceptions(slaveInterp);
    
    if (global) {
        result = TclObjInvokeGlobal(slaveInterp, objc, objv,
                TCL_INVOKE_HIDDEN);
    } else {
        result = TclObjInvoke(slaveInterp, objc, objv, TCL_INVOKE_HIDDEN);
    }

    TclTransferResult(slaveInterp, result, interp);

    Tcl_Release((ClientData) slaveInterp);
    return result;        
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveMarkTrusted --
 *
 *	Helper function to mark a slave interpreter as trusted (unsafe).
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	After this call the hard-wired security checks in the core no
 *	longer prevent the slave from performing certain operations.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveMarkTrusted(interp, slaveInterp)
    Tcl_Interp *interp;		/* Interp for error return. */
    Tcl_Interp *slaveInterp;	/* The slave interpreter which will be
				 * marked trusted. */
{
    if (Tcl_IsSafe(interp)) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"permission denied: safe interpreter cannot mark trusted",
		(char *) NULL);
	return TCL_ERROR;
    }
    ((Interp *) slaveInterp)->flags &= ~SAFE_INTERP;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_IsSafe --
 *
 *	Determines whether an interpreter is safe
 *
 * Results:
 *	1 if it is safe, 0 if it is not.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_IsSafe(interp)
    Tcl_Interp *interp;		/* Is this interpreter "safe" ? */
{
    Interp *iPtr;

    if (interp == (Tcl_Interp *) NULL) {
        return 0;
    }
    iPtr = (Interp *) interp;

    return ( (iPtr->flags) & SAFE_INTERP ) ? 1 : 0 ;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MakeSafe --
 *
 *	Makes its argument interpreter contain only functionality that is
 *	defined to be part of Safe Tcl. Unsafe commands are hidden, the
 *	env array is unset, and the standard channels are removed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Hides commands in its argument interpreter, and removes settings
 *	and channels.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_MakeSafe(interp)
    Tcl_Interp *interp;		/* Interpreter to be made safe. */
{
    Tcl_Channel chan;				/* Channel to remove from
                                                 * safe interpreter. */
    Interp *iPtr = (Interp *) interp;

    TclHideUnsafeCommands(interp);
    
    iPtr->flags |= SAFE_INTERP;

    /*
     *  Unsetting variables : (which should not have been set 
     *  in the first place, but...)
     */

    /*
     * No env array in a safe slave.
     */

    Tcl_UnsetVar(interp, "env", TCL_GLOBAL_ONLY);

    /* 
     * Remove unsafe parts of tcl_platform
     */

    Tcl_UnsetVar2(interp, "tcl_platform", "os", TCL_GLOBAL_ONLY);
    Tcl_UnsetVar2(interp, "tcl_platform", "osVersion", TCL_GLOBAL_ONLY);
    Tcl_UnsetVar2(interp, "tcl_platform", "machine", TCL_GLOBAL_ONLY);
    Tcl_UnsetVar2(interp, "tcl_platform", "user", TCL_GLOBAL_ONLY);

    /*
     * Unset path informations variables
     * (the only one remaining is [info nameofexecutable])
     */

    Tcl_UnsetVar(interp, "tclDefaultLibrary", TCL_GLOBAL_ONLY);
    Tcl_UnsetVar(interp, "tcl_library", TCL_GLOBAL_ONLY);
    Tcl_UnsetVar(interp, "tcl_pkgPath", TCL_GLOBAL_ONLY);
    
    /*
     * Remove the standard channels from the interpreter; safe interpreters
     * do not ordinarily have access to stdin, stdout and stderr.
     *
     * NOTE: These channels are not added to the interpreter by the
     * Tcl_CreateInterp call, but may be added later, by another I/O
     * operation. We want to ensure that the interpreter does not have
     * these channels even if it is being made safe after being used for
     * some time..
     */

    chan = Tcl_GetStdChannel(TCL_STDIN);
    if (chan != (Tcl_Channel) NULL) {
        Tcl_UnregisterChannel(interp, chan);
    }
    chan = Tcl_GetStdChannel(TCL_STDOUT);
    if (chan != (Tcl_Channel) NULL) {
        Tcl_UnregisterChannel(interp, chan);
    }
    chan = Tcl_GetStdChannel(TCL_STDERR);
    if (chan != (Tcl_Channel) NULL) {
        Tcl_UnregisterChannel(interp, chan);
    }

    return TCL_OK;
}
