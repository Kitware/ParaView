/* 
 * tclExecute.c --
 *
 *	This file contains procedures that execute byte-compiled Tcl
 *	commands.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclCompile.h"

#ifdef NO_FLOAT_H
#   include "../compat/float.h"
#else
#   include <float.h>
#endif
#ifndef TCL_NO_MATH
#include "tclMath.h"
#endif

/*
 * The stuff below is a bit of a hack so that this file can be used
 * in environments that include no UNIX, i.e. no errno.  Just define
 * errno here.
 */

#ifndef TCL_GENERIC_ONLY
#include "tclPort.h"
#else
#define NO_ERRNO_H
#endif

#ifdef NO_ERRNO_H
int errno;
#define EDOM 33
#define ERANGE 34
#endif

/*
 * Boolean flag indicating whether the Tcl bytecode interpreter has been
 * initialized.
 */

static int execInitialized = 0;
TCL_DECLARE_MUTEX(execMutex)

/*
 * Variable that controls whether execution tracing is enabled and, if so,
 * what level of tracing is desired:
 *    0: no execution tracing
 *    1: trace invocations of Tcl procs only
 *    2: trace invocations of all (not compiled away) commands
 *    3: display each instruction executed
 * This variable is linked to the Tcl variable "tcl_traceExec".
 */

int tclTraceExec = 0;

typedef struct ThreadSpecificData {
    /*
     * The following global variable is use to signal matherr that Tcl
     * is responsible for the arithmetic, so errors can be handled in a
     * fashion appropriate for Tcl.  Zero means no Tcl math is in
     * progress;  non-zero means Tcl is doing math.
     */
    
    int mathInProgress;

} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * The variable below serves no useful purpose except to generate
 * a reference to matherr, so that the Tcl version of matherr is
 * linked in rather than the system version. Without this reference
 * the need for matherr won't be discovered during linking until after
 * libtcl.a has been processed, so Tcl's version won't be used.
 */

#ifdef NEED_MATHERR
extern int matherr();
int (*tclMatherrPtr)() = matherr;
#endif

/*
 * Mapping from expression instruction opcodes to strings; used for error
 * messages. Note that these entries must match the order and number of the
 * expression opcodes (e.g., INST_LOR) in tclCompile.h.
 */

static char *operatorStrings[] = {
    "||", "&&", "|", "^", "&", "==", "!=", "<", ">", "<=", ">=", "<<", ">>",
    "+", "-", "*", "/", "%", "+", "-", "~", "!",
    "BUILTIN FUNCTION", "FUNCTION"
};
    
/*
 * Mapping from Tcl result codes to strings; used for error and debugging
 * messages. 
 */

#ifdef TCL_COMPILE_DEBUG
static char *resultStrings[] = {
    "TCL_OK", "TCL_ERROR", "TCL_RETURN", "TCL_BREAK", "TCL_CONTINUE"
};
#endif

/*
 * Macros for testing floating-point values for certain special cases. Test
 * for not-a-number by comparing a value against itself; test for infinity
 * by comparing against the largest floating-point value.
 */

#define IS_NAN(v) ((v) != (v))
#ifdef DBL_MAX
#   define IS_INF(v) (((v) > DBL_MAX) || ((v) < -DBL_MAX))
#else
#   define IS_INF(v) 0
#endif

/*
 * Macro to adjust the program counter and restart the instruction execution
 * loop after each instruction is executed.
 */

#define ADJUST_PC(instBytes) \
    pc += (instBytes); \
    continue

/*
 * Macros used to cache often-referenced Tcl evaluation stack information
 * in local variables. Note that a DECACHE_STACK_INFO()-CACHE_STACK_INFO()
 * pair must surround any call inside TclExecuteByteCode (and a few other
 * procedures that use this scheme) that could result in a recursive call
 * to TclExecuteByteCode.
 */

#define CACHE_STACK_INFO() \
    stackPtr = eePtr->stackPtr; \
    stackTop = eePtr->stackTop

#define DECACHE_STACK_INFO() \
    eePtr->stackTop = stackTop

/*
 * Macros used to access items on the Tcl evaluation stack. PUSH_OBJECT
 * increments the object's ref count since it makes the stack have another
 * reference pointing to the object. However, POP_OBJECT does not decrement
 * the ref count. This is because the stack may hold the only reference to
 * the object, so the object would be destroyed if its ref count were
 * decremented before the caller had a chance to, e.g., store it in a
 * variable. It is the caller's responsibility to decrement the ref count
 * when it is finished with an object.
 *
 * WARNING! It is essential that objPtr only appear once in the PUSH_OBJECT
 * macro. The actual parameter might be an expression with side effects,
 * and this ensures that it will be executed only once. 
 */
    
#define PUSH_OBJECT(objPtr) \
    Tcl_IncrRefCount(stackPtr[++stackTop] = (objPtr))
    
#define POP_OBJECT() \
    (stackPtr[stackTop--])

/*
 * Macros used to trace instruction execution. The macros TRACE,
 * TRACE_WITH_OBJ, and O2S are only used inside TclExecuteByteCode.
 * O2S is only used in TRACE* calls to get a string from an object.
 */

#ifdef TCL_COMPILE_DEBUG
#define TRACE(a) \
    if (traceInstructions) { \
        fprintf(stdout, "%2d: %2d (%u) %s ", iPtr->numLevels, stackTop, \
	       (unsigned int)(pc - codePtr->codeStart), \
	       GetOpcodeName(pc)); \
	printf a; \
    }
#define TRACE_WITH_OBJ(a, objPtr) \
    if (traceInstructions) { \
        fprintf(stdout, "%2d: %2d (%u) %s ", iPtr->numLevels, stackTop, \
	       (unsigned int)(pc - codePtr->codeStart), \
	       GetOpcodeName(pc)); \
	printf a; \
        TclPrintObject(stdout, (objPtr), 30); \
        fprintf(stdout, "\n"); \
    }
#define O2S(objPtr) \
    Tcl_GetString(objPtr)
#else
#define TRACE(a)
#define TRACE_WITH_OBJ(a, objPtr)
#define O2S(objPtr)
#endif /* TCL_COMPILE_DEBUG */

/*
 * Declarations for local procedures to this file:
 */

static void		CallTraceProcedure _ANSI_ARGS_((Tcl_Interp *interp,
			    Trace *tracePtr, Command *cmdPtr,
			    char *command, int numChars,
			    int objc, Tcl_Obj *objv[]));
static void		DupCmdNameInternalRep _ANSI_ARGS_((Tcl_Obj *objPtr,
			    Tcl_Obj *copyPtr));
static int		ExprAbsFunc _ANSI_ARGS_((Tcl_Interp *interp,
			    ExecEnv *eePtr, ClientData clientData));
static int		ExprBinaryFunc _ANSI_ARGS_((Tcl_Interp *interp,
			    ExecEnv *eePtr, ClientData clientData));
static int		ExprCallMathFunc _ANSI_ARGS_((Tcl_Interp *interp,
			    ExecEnv *eePtr, int objc, Tcl_Obj **objv));
static int		ExprDoubleFunc _ANSI_ARGS_((Tcl_Interp *interp,
			    ExecEnv *eePtr, ClientData clientData));
static int		ExprIntFunc _ANSI_ARGS_((Tcl_Interp *interp,
			    ExecEnv *eePtr, ClientData clientData));
static int		ExprRandFunc _ANSI_ARGS_((Tcl_Interp *interp,
			    ExecEnv *eePtr, ClientData clientData));
static int		ExprRoundFunc _ANSI_ARGS_((Tcl_Interp *interp,
			    ExecEnv *eePtr, ClientData clientData));
static int		ExprSrandFunc _ANSI_ARGS_((Tcl_Interp *interp,
			    ExecEnv *eePtr, ClientData clientData));
static int		ExprUnaryFunc _ANSI_ARGS_((Tcl_Interp *interp,
			    ExecEnv *eePtr, ClientData clientData));
#ifdef TCL_COMPILE_STATS
static int              EvalStatsCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
#endif
static void		FreeCmdNameInternalRep _ANSI_ARGS_((
    			    Tcl_Obj *objPtr));
#ifdef TCL_COMPILE_DEBUG
static char *		GetOpcodeName _ANSI_ARGS_((unsigned char *pc));
#endif
static ExceptionRange *	GetExceptRangeForPc _ANSI_ARGS_((unsigned char *pc,
			    int catchOnly, ByteCode* codePtr));
static char *		GetSrcInfoForPc _ANSI_ARGS_((unsigned char *pc,
        		    ByteCode* codePtr, int *lengthPtr));
static void		GrowEvaluationStack _ANSI_ARGS_((ExecEnv *eePtr));
static void		IllegalExprOperandType _ANSI_ARGS_((
			    Tcl_Interp *interp, unsigned char *pc,
			    Tcl_Obj *opndPtr));
static void		InitByteCodeExecution _ANSI_ARGS_((
			    Tcl_Interp *interp));
#ifdef TCL_COMPILE_DEBUG
static void		PrintByteCodeInfo _ANSI_ARGS_((ByteCode *codePtr));
#endif
static int		SetCmdNameFromAny _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));
#ifdef TCL_COMPILE_DEBUG
static char *		StringForResultCode _ANSI_ARGS_((int result));
static void		ValidatePcAndStackTop _ANSI_ARGS_((
			    ByteCode *codePtr, unsigned char *pc,
			    int stackTop, int stackLowerBound,
			    int stackUpperBound));
#endif

/*
 * Table describing the built-in math functions. Entries in this table are
 * indexed by the values of the INST_CALL_BUILTIN_FUNC instruction's
 * operand byte.
 */

BuiltinFunc builtinFuncTable[] = {
#ifndef TCL_NO_MATH
    {"acos", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) acos},
    {"asin", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) asin},
    {"atan", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) atan},
    {"atan2", 2, {TCL_DOUBLE, TCL_DOUBLE}, ExprBinaryFunc, (ClientData) atan2},
    {"ceil", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) ceil},
    {"cos", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) cos},
    {"cosh", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) cosh},
    {"exp", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) exp},
    {"floor", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) floor},
    {"fmod", 2, {TCL_DOUBLE, TCL_DOUBLE}, ExprBinaryFunc, (ClientData) fmod},
    {"hypot", 2, {TCL_DOUBLE, TCL_DOUBLE}, ExprBinaryFunc, (ClientData) hypot},
    {"log", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) log},
    {"log10", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) log10},
    {"pow", 2, {TCL_DOUBLE, TCL_DOUBLE}, ExprBinaryFunc, (ClientData) pow},
    {"sin", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) sin},
    {"sinh", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) sinh},
    {"sqrt", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) sqrt},
    {"tan", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) tan},
    {"tanh", 1, {TCL_DOUBLE}, ExprUnaryFunc, (ClientData) tanh},
#endif
    {"abs", 1, {TCL_EITHER}, ExprAbsFunc, 0},
    {"double", 1, {TCL_EITHER}, ExprDoubleFunc, 0},
    {"int", 1, {TCL_EITHER}, ExprIntFunc, 0},
    {"rand", 0, {TCL_EITHER}, ExprRandFunc, 0},	/* NOTE: rand takes no args. */
    {"round", 1, {TCL_EITHER}, ExprRoundFunc, 0},
    {"srand", 1, {TCL_INT}, ExprSrandFunc, 0},
    {0},
};

/*
 * The structure below defines the command name Tcl object type by means of
 * procedures that can be invoked by generic object code. Objects of this
 * type cache the Command pointer that results from looking up command names
 * in the command hashtable. Such objects appear as the zeroth ("command
 * name") argument in a Tcl command.
 */

Tcl_ObjType tclCmdNameType = {
    "cmdName",				/* name */
    FreeCmdNameInternalRep,		/* freeIntRepProc */
    DupCmdNameInternalRep,		/* dupIntRepProc */
    (Tcl_UpdateStringProc *) NULL,	/* updateStringProc */
    SetCmdNameFromAny			/* setFromAnyProc */
};

/*
 *----------------------------------------------------------------------
 *
 * InitByteCodeExecution --
 *
 *	This procedure is called once to initialize the Tcl bytecode
 *	interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This procedure initializes the array of instruction names. If
 *	compiling with the TCL_COMPILE_STATS flag, it initializes the
 *	array that counts the executions of each instruction and it
 *	creates the "evalstats" command. It also registers the command name
 *	Tcl_ObjType. It also establishes the link between the Tcl
 *	"tcl_traceExec" and C "tclTraceExec" variables.
 *
 *----------------------------------------------------------------------
 */

static void
InitByteCodeExecution(interp)
    Tcl_Interp *interp;		/* Interpreter for which the Tcl variable
				 * "tcl_traceExec" is linked to control
				 * instruction tracing. */
{
    Tcl_RegisterObjType(&tclCmdNameType);
    if (Tcl_LinkVar(interp, "tcl_traceExec", (char *) &tclTraceExec,
		    TCL_LINK_INT) != TCL_OK) {
	panic("InitByteCodeExecution: can't create link for tcl_traceExec variable");
    }

#ifdef TCL_COMPILE_STATS    
    Tcl_CreateCommand(interp, "evalstats", EvalStatsCmd,
		      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
#endif /* TCL_COMPILE_STATS */
}

/*
 *----------------------------------------------------------------------
 *
 * TclCreateExecEnv --
 *
 *	This procedure creates a new execution environment for Tcl bytecode
 *	execution. An ExecEnv points to a Tcl evaluation stack. An ExecEnv
 *	is typically created once for each Tcl interpreter (Interp
 *	structure) and recursively passed to TclExecuteByteCode to execute
 *	ByteCode sequences for nested commands.
 *
 * Results:
 *	A newly allocated ExecEnv is returned. This points to an empty
 *	evaluation stack of the standard initial size.
 *
 * Side effects:
 *	The bytecode interpreter is also initialized here, as this
 *	procedure will be called before any call to TclExecuteByteCode.
 *
 *----------------------------------------------------------------------
 */

#define TCL_STACK_INITIAL_SIZE 2000

ExecEnv *
TclCreateExecEnv(interp)
    Tcl_Interp *interp;		/* Interpreter for which the execution
				 * environment is being created. */
{
    ExecEnv *eePtr = (ExecEnv *) ckalloc(sizeof(ExecEnv));

    eePtr->stackPtr = (Tcl_Obj **)
	ckalloc((unsigned) (TCL_STACK_INITIAL_SIZE * sizeof(Tcl_Obj *)));
    eePtr->stackTop = -1;
    eePtr->stackEnd = (TCL_STACK_INITIAL_SIZE - 1);

    Tcl_MutexLock(&execMutex);
    if (!execInitialized) {
	TclInitAuxDataTypeTable();
	InitByteCodeExecution(interp);
	execInitialized = 1;
    }
    Tcl_MutexUnlock(&execMutex);

    return eePtr;
}
#undef TCL_STACK_INITIAL_SIZE

/*
 *----------------------------------------------------------------------
 *
 * TclDeleteExecEnv --
 *
 *	Frees the storage for an ExecEnv.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Storage for an ExecEnv and its contained storage (e.g. the
 *	evaluation stack) is freed.
 *
 *----------------------------------------------------------------------
 */

void
TclDeleteExecEnv(eePtr)
    ExecEnv *eePtr;		/* Execution environment to free. */
{
    ckfree((char *) eePtr->stackPtr);
    ckfree((char *) eePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeExecution --
 *
 *	Finalizes the execution environment setup so that it can be
 *	later reinitialized.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	After this call, the next time TclCreateExecEnv will be called
 *	it will call InitByteCodeExecution.
 *
 *----------------------------------------------------------------------
 */

void
TclFinalizeExecution()
{
    Tcl_MutexLock(&execMutex);
    execInitialized = 0;
    Tcl_MutexUnlock(&execMutex);
    TclFinalizeAuxDataTypeTable();
}

/*
 *----------------------------------------------------------------------
 *
 * GrowEvaluationStack --
 *
 *	This procedure grows a Tcl evaluation stack stored in an ExecEnv.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The size of the evaluation stack is doubled.
 *
 *----------------------------------------------------------------------
 */

static void
GrowEvaluationStack(eePtr)
    register ExecEnv *eePtr; /* Points to the ExecEnv with an evaluation
			      * stack to enlarge. */
{
    /*
     * The current Tcl stack elements are stored from eePtr->stackPtr[0]
     * to eePtr->stackPtr[eePtr->stackEnd] (inclusive).
     */

    int currElems = (eePtr->stackEnd + 1);
    int newElems  = 2*currElems;
    int currBytes = currElems * sizeof(Tcl_Obj *);
    int newBytes  = 2*currBytes;
    Tcl_Obj **newStackPtr = (Tcl_Obj **) ckalloc((unsigned) newBytes);

    /*
     * Copy the existing stack items to the new stack space, free the old
     * storage if appropriate, and mark new space as malloc'ed.
     */
 
    memcpy((VOID *) newStackPtr, (VOID *) eePtr->stackPtr,
	   (size_t) currBytes);
    ckfree((char *) eePtr->stackPtr);
    eePtr->stackPtr = newStackPtr;
    eePtr->stackEnd = (newElems - 1); /* i.e. index of last usable item */
}

/*
 *----------------------------------------------------------------------
 *
 * TclExecuteByteCode --
 *
 *	This procedure executes the instructions of a ByteCode structure.
 *	It returns when a "done" instruction is executed or an error occurs.
 *
 * Results:
 *	The return value is one of the return codes defined in tcl.h
 *	(such as TCL_OK), and interp->objResultPtr refers to a Tcl object
 *	that either contains the result of executing the code or an
 *	error message.
 *
 * Side effects:
 *	Almost certainly, depending on the ByteCode's instructions.
 *
 *----------------------------------------------------------------------
 */

int
TclExecuteByteCode(interp, codePtr)
    Tcl_Interp *interp;		/* Token for command interpreter. */
    ByteCode *codePtr;		/* The bytecode sequence to interpret. */
{
    Interp *iPtr = (Interp *) interp;
    ExecEnv *eePtr = iPtr->execEnvPtr;
    				/* Points to the execution environment. */
    register Tcl_Obj **stackPtr = eePtr->stackPtr;
    				/* Cached evaluation stack base pointer. */
    register int stackTop = eePtr->stackTop;
    				/* Cached top index of evaluation stack. */
    register unsigned char *pc = codePtr->codeStart;
				/* The current program counter. */
    int opnd;			/* Current instruction's operand byte. */
    int pcAdjustment;		/* Hold pc adjustment after instruction. */
    int initStackTop = stackTop;/* Stack top at start of execution. */
    ExceptionRange *rangePtr;	/* Points to closest loop or catch exception
				 * range enclosing the pc. Used by various
				 * instructions and processCatch to
				 * process break, continue, and errors. */
    int result = TCL_OK;	/* Return code returned after execution. */
    int traceInstructions = (tclTraceExec == 3);
    Tcl_Obj *valuePtr, *value2Ptr, *objPtr;
    char *bytes;
    int length;
    long i;

    /*
     * This procedure uses a stack to hold information about catch commands.
     * This information is the current operand stack top when starting to
     * execute the code for each catch command. It starts out with stack-
     * allocated space but uses dynamically-allocated storage if needed.
     */

#define STATIC_CATCH_STACK_SIZE 4
    int (catchStackStorage[STATIC_CATCH_STACK_SIZE]);
    int *catchStackPtr = catchStackStorage;
    int catchTop = -1;

#ifdef TCL_COMPILE_DEBUG
    if (tclTraceExec >= 2) {
	PrintByteCodeInfo(codePtr);
	fprintf(stdout, "  Starting stack top=%d\n", eePtr->stackTop);
	fflush(stdout);
    }
#endif
    
#ifdef TCL_COMPILE_STATS
    iPtr->stats.numExecutions++;
#endif

    /*
     * Make sure the catch stack is large enough to hold the maximum number
     * of catch commands that could ever be executing at the same time. This
     * will be no more than the exception range array's depth.
     */

    if (codePtr->maxExceptDepth > STATIC_CATCH_STACK_SIZE) {
	catchStackPtr = (int *)
	        ckalloc(codePtr->maxExceptDepth * sizeof(int));
    }

    /*
     * Make sure the stack has enough room to execute this ByteCode.
     */

    while ((stackTop + codePtr->maxStackDepth) > eePtr->stackEnd) {
        GrowEvaluationStack(eePtr); 
        stackPtr = eePtr->stackPtr;
    }

    /*
     * Loop executing instructions until a "done" instruction, a TCL_RETURN,
     * or some error.
     */

    for (;;) {
#ifdef TCL_COMPILE_DEBUG
	ValidatePcAndStackTop(codePtr, pc, stackTop, initStackTop,
		eePtr->stackEnd);
#else /* not TCL_COMPILE_DEBUG */
        if (traceInstructions) {
            fprintf(stdout, "%2d: %2d ", iPtr->numLevels, stackTop);
            TclPrintInstruction(codePtr, pc);
            fflush(stdout);
        }
#endif /* TCL_COMPILE_DEBUG */
	
#ifdef TCL_COMPILE_STATS    
	iPtr->stats.instructionCount[*pc]++;
#endif
        switch (*pc) {
	case INST_DONE:
	    /*
	     * Pop the topmost object from the stack, set the interpreter's
	     * object result to point to it, and return.
	     */
	    valuePtr = POP_OBJECT();
	    Tcl_SetObjResult(interp, valuePtr);
	    TclDecrRefCount(valuePtr);
	    if (stackTop != initStackTop) {
		fprintf(stderr, "\nTclExecuteByteCode: done instruction at pc %u: stack top %d != entry stack top %d\n",
			(unsigned int)(pc - codePtr->codeStart),
			(unsigned int) stackTop,
			(unsigned int) initStackTop);
		panic("TclExecuteByteCode execution failure: end stack top != start stack top");
	    }
	    TRACE_WITH_OBJ(("=> return code=%d, result=", result),
		    iPtr->objResultPtr);
#ifdef TCL_COMPILE_DEBUG	    
	    if (traceInstructions) {
		fprintf(stdout, "\n");
	    }
#endif
	    goto done;
	    
	case INST_PUSH1:
#ifdef TCL_COMPILE_DEBUG
	    valuePtr = codePtr->objArrayPtr[TclGetUInt1AtPtr(pc+1)];
	    PUSH_OBJECT(valuePtr);
	    TRACE_WITH_OBJ(("%u => ", TclGetInt1AtPtr(pc+1)), valuePtr);
#else
	    PUSH_OBJECT(codePtr->objArrayPtr[TclGetUInt1AtPtr(pc+1)]);
#endif /* TCL_COMPILE_DEBUG */
	    ADJUST_PC(2);
	    
	case INST_PUSH4:
	    valuePtr = codePtr->objArrayPtr[TclGetUInt4AtPtr(pc+1)];
	    PUSH_OBJECT(valuePtr);
	    TRACE_WITH_OBJ(("%u => ", TclGetUInt4AtPtr(pc+1)), valuePtr);
	    ADJUST_PC(5);
	    
	case INST_POP:
	    valuePtr = POP_OBJECT();
	    TRACE_WITH_OBJ(("=> discarding "), valuePtr);
	    TclDecrRefCount(valuePtr); /* finished with pop'ed object. */
	    ADJUST_PC(1);

	case INST_DUP:
	    valuePtr = stackPtr[stackTop];
	    PUSH_OBJECT(Tcl_DuplicateObj(valuePtr));
	    TRACE_WITH_OBJ(("=> "), valuePtr);
	    ADJUST_PC(1);

	case INST_CONCAT1:
	    opnd = TclGetUInt1AtPtr(pc+1);
	    {
		Tcl_Obj *concatObjPtr;
		int totalLen = 0;

		/*
		 * Concatenate strings (with no separators) from the top
		 * opnd items on the stack starting with the deepest item.
		 * First, determine how many characters are needed.
		 */

		for (i = (stackTop - (opnd-1));  i <= stackTop;  i++) {
		    bytes = Tcl_GetStringFromObj(stackPtr[i], &length);
		    if (bytes != NULL) {
			totalLen += length;
		    }
                }

		/*
		 * Initialize the new append string object by appending the
		 * strings of the opnd stack objects. Also pop the objects. 
		 */

		TclNewObj(concatObjPtr);
		if (totalLen > 0) {
		    char *p = (char *) ckalloc((unsigned) (totalLen + 1));
		    concatObjPtr->bytes = p;
		    concatObjPtr->length = totalLen;
		    for (i = (stackTop - (opnd-1));  i <= stackTop;  i++) {
			valuePtr = stackPtr[i];
			bytes = Tcl_GetStringFromObj(valuePtr, &length);
			if (bytes != NULL) {
			    memcpy((VOID *) p, (VOID *) bytes,
			            (size_t) length);
			    p += length;
			}
			TclDecrRefCount(valuePtr);
		    }
		    *p = '\0';
		} else {
		    for (i = (stackTop - (opnd-1));  i <= stackTop;  i++) {
			Tcl_DecrRefCount(stackPtr[i]);
		    }
		}
		stackTop -= opnd;
		
		PUSH_OBJECT(concatObjPtr);
		TRACE_WITH_OBJ(("%u => ", opnd), concatObjPtr);
		ADJUST_PC(2);
            }
	    
	case INST_INVOKE_STK4:
	    opnd = TclGetUInt4AtPtr(pc+1);
	    pcAdjustment = 5;
	    goto doInvocation;

	case INST_INVOKE_STK1:
	    opnd = TclGetUInt1AtPtr(pc+1);
	    pcAdjustment = 2;
	    
	    doInvocation:
	    {
		int objc = opnd; /* The number of arguments. */
		Tcl_Obj **objv;	 /* The array of argument objects. */
		Command *cmdPtr; /* Points to command's Command struct. */
		int newPcOffset; /* New inst offset for break, continue. */
#ifdef TCL_COMPILE_DEBUG
		int isUnknownCmd = 0;
		char cmdNameBuf[21];
#endif /* TCL_COMPILE_DEBUG */
		
		/*
		 * If the interpreter was deleted, return an error.
		 */
		
		if (iPtr->flags & DELETED) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendToObj(Tcl_GetObjResult(interp),
		            "attempt to call eval in deleted interpreter", -1);
		    Tcl_SetErrorCode(interp, "CORE", "IDELETE",
			    "attempt to call eval in deleted interpreter",
			    (char *) NULL);
		    result = TCL_ERROR;
		    goto checkForCatch;
		}
    
		/*
		 * Find the procedure to execute this command. If the
		 * command is not found, handle it with the "unknown" proc.
		 */

		objv = &(stackPtr[stackTop - (objc-1)]);
		cmdPtr = (Command *) Tcl_GetCommandFromObj(interp, objv[0]);
		if (cmdPtr == NULL) {
		    cmdPtr = (Command *) Tcl_FindCommand(interp, "unknown",
                            (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY);
                    if (cmdPtr == NULL) {
			Tcl_ResetResult(interp);
			Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			        "invalid command name \"",
				Tcl_GetString(objv[0]), "\"",
				(char *) NULL);
			TRACE(("%u => unknown proc not found: ", objc));
			result = TCL_ERROR;
			goto checkForCatch;
		    }
#ifdef TCL_COMPILE_DEBUG
		    isUnknownCmd = 1;
#endif /*TCL_COMPILE_DEBUG*/			
		    stackTop++; /* need room for new inserted objv[0] */
		    for (i = objc-1;  i >= 0;  i--) {
			objv[i+1] = objv[i];
		    }
		    objc++;
		    objv[0] = Tcl_NewStringObj("unknown", -1);
		    Tcl_IncrRefCount(objv[0]);
		}
		
		/*
		 * Call any trace procedures.
		 */

		if (iPtr->tracePtr != NULL) {
		    Trace *tracePtr, *nextTracePtr;

		    for (tracePtr = iPtr->tracePtr;  tracePtr != NULL;
		            tracePtr = nextTracePtr) {
			nextTracePtr = tracePtr->nextPtr;
			if (iPtr->numLevels <= tracePtr->level) {
			    int numChars;
			    char *cmd = GetSrcInfoForPc(pc, codePtr,
				    &numChars);
			    if (cmd != NULL) {
				DECACHE_STACK_INFO();
				CallTraceProcedure(interp, tracePtr, cmdPtr,
				        cmd, numChars, objc, objv);
				CACHE_STACK_INFO();
			    }
			}
		    }
		}
		
		/*
		 * Finally, invoke the command's Tcl_ObjCmdProc. First reset
		 * the interpreter's string and object results to their
		 * default empty values since they could have gotten changed
		 * by earlier invocations.
		 */
		
		Tcl_ResetResult(interp);
		if (tclTraceExec >= 2) {
#ifdef TCL_COMPILE_DEBUG
		    if (traceInstructions) {
			strncpy(cmdNameBuf, Tcl_GetString(objv[0]), 20);
			TRACE(("%u => call ", (isUnknownCmd? objc-1:objc)));
		    } else {
			fprintf(stdout, "%d: (%u) invoking ",
			        iPtr->numLevels,
				(unsigned int)(pc - codePtr->codeStart));
		    }
		    for (i = 0;  i < objc;  i++) {
			TclPrintObject(stdout, objv[i], 15);
			fprintf(stdout, " ");
		    }
		    fprintf(stdout, "\n");
		    fflush(stdout);
#else /* TCL_COMPILE_DEBUG */
		    fprintf(stdout, "%d: (%u) invoking %s\n",
			    iPtr->numLevels,
		            (unsigned int)(pc - codePtr->codeStart),
			    Tcl_GetString(objv[0]));
#endif /*TCL_COMPILE_DEBUG*/
		}

		iPtr->cmdCount++;
		DECACHE_STACK_INFO();
		result = (*cmdPtr->objProc)(cmdPtr->objClientData, interp,
					    objc, objv);
		if (Tcl_AsyncReady()) {
		    result = Tcl_AsyncInvoke(interp, result);
		}
		CACHE_STACK_INFO();

		/*
		 * If the interpreter has a non-empty string result, the
		 * result object is either empty or stale because some
		 * procedure set interp->result directly. If so, move the
		 * string result to the result object, then reset the
		 * string result.
		 */

		if (*(iPtr->result) != 0) {
		    (void) Tcl_GetObjResult(interp);
		}
		
		/*
		 * Pop the objc top stack elements and decrement their ref
		 * counts. 
		 */

		for (i = 0;  i < objc;  i++) {
		    valuePtr = stackPtr[stackTop];
		    TclDecrRefCount(valuePtr);
		    stackTop--;
		}

		/*
		 * Process the result of the Tcl_ObjCmdProc call.
		 */
		
		switch (result) {
		case TCL_OK:
		    /*
		     * Push the call's object result and continue execution
		     * with the next instruction.
		     */
		    PUSH_OBJECT(Tcl_GetObjResult(interp));
		    TRACE_WITH_OBJ(("%u => ...after \"%.20s\", result=",
		            objc, cmdNameBuf), Tcl_GetObjResult(interp));
		    ADJUST_PC(pcAdjustment);
		    
		case TCL_BREAK:
		case TCL_CONTINUE:
		    /*
		     * The invoked command requested a break or continue.
		     * Find the closest enclosing loop or catch exception
		     * range, if any. If a loop is found, terminate its
		     * execution or skip to its next iteration. If the
		     * closest is a catch exception range, jump to its
		     * catchOffset. If no enclosing range is found, stop
		     * execution and return the TCL_BREAK or TCL_CONTINUE.
		     */
		    rangePtr = GetExceptRangeForPc(pc, /*catchOnly*/ 0,
			    codePtr);
		    if (rangePtr == NULL) {
		        TRACE(("%u => ... after \"%.20s\", no encl. loop or catch, returning %s\n",
		                objc, cmdNameBuf,
			        StringForResultCode(result)));
			goto abnormalReturn; /* no catch exists to check */
		    }
		    newPcOffset = 0;
		    switch (rangePtr->type) {
		    case LOOP_EXCEPTION_RANGE:
			if (result == TCL_BREAK) {
			    newPcOffset = rangePtr->breakOffset;
			} else if (rangePtr->continueOffset == -1) {
			    TRACE(("%u => ... after \"%.20s\", %s, loop w/o continue, checking for catch\n",
				   objc, cmdNameBuf,
				   StringForResultCode(result)));
			    goto checkForCatch;
			} else {
			    newPcOffset = rangePtr->continueOffset;
			}
			TRACE(("%u => ... after \"%.20s\", %s, range at %d, new pc %d\n",
			       objc, cmdNameBuf,
			       StringForResultCode(result),
			       rangePtr->codeOffset, newPcOffset));
			break;
		    case CATCH_EXCEPTION_RANGE:
			TRACE(("%u => ... after \"%.20s\", %s...\n",
			       objc, cmdNameBuf,
			       StringForResultCode(result)));
			goto processCatch; /* it will use rangePtr */
		    default:
			panic("TclExecuteByteCode: bad ExceptionRange type\n");
		    }
		    result = TCL_OK;
		    pc = (codePtr->codeStart + newPcOffset);
		    continue;	/* restart outer instruction loop at pc */
		    
		case TCL_ERROR:
		    /*
		     * The invoked command returned an error. Look for an
		     * enclosing catch exception range, if any.
		     */
		    TRACE_WITH_OBJ(("%u => ... after \"%.20s\", TCL_ERROR ",
		            objc, cmdNameBuf), Tcl_GetObjResult(interp));
		    goto checkForCatch;

		case TCL_RETURN:
		    /*
		     * The invoked command requested that the current
		     * procedure stop execution and return. First check
		     * for an enclosing catch exception range, if any.
		     */
		    TRACE(("%u => ... after \"%.20s\", TCL_RETURN\n",
		            objc, cmdNameBuf));
		    goto checkForCatch;

		default:
		    TRACE_WITH_OBJ(("%u => ... after \"%.20s\", OTHER RETURN CODE %d ",
		            objc, cmdNameBuf, result),
			    Tcl_GetObjResult(interp));
		    goto checkForCatch;
		}
	    }
	    
	case INST_EVAL_STK:
	    objPtr = POP_OBJECT();
	    DECACHE_STACK_INFO();
	    result = Tcl_EvalObjEx(interp, objPtr, 0);
	    CACHE_STACK_INFO();
	    if (result == TCL_OK) {
		/*
		 * Normal return; push the eval's object result.
		 */
		PUSH_OBJECT(Tcl_GetObjResult(interp));
		TRACE_WITH_OBJ(("\"%.30s\" => ", O2S(objPtr)),
			Tcl_GetObjResult(interp));
		TclDecrRefCount(objPtr);
		ADJUST_PC(1);
	    } else if ((result == TCL_BREAK) || (result == TCL_CONTINUE)) {
		/*
		 * Find the closest enclosing loop or catch exception range,
		 * if any. If a loop is found, terminate its execution or
		 * skip to its next iteration. If the closest is a catch
		 * exception range, jump to its catchOffset. If no enclosing
		 * range is found, stop execution and return that same
		 * TCL_BREAK or TCL_CONTINUE.
		 */

		int newPcOffset = 0; /* Pc offset computed during break,
				      * continue, error processing. Init.
				      * to avoid compiler warning. */

		rangePtr = GetExceptRangeForPc(pc, /*catchOnly*/ 0,
			codePtr);
		if (rangePtr == NULL) {
		    TRACE(("\"%.30s\" => no encl. loop or catch, returning %s\n",
			    O2S(objPtr), StringForResultCode(result)));
		    Tcl_DecrRefCount(objPtr);
		    goto abnormalReturn;    /* no catch exists to check */
		}
		switch (rangePtr->type) {
		case LOOP_EXCEPTION_RANGE:
		    if (result == TCL_BREAK) {
			newPcOffset = rangePtr->breakOffset;
		    } else if (rangePtr->continueOffset == -1) {
			TRACE(("\"%.30s\" => %s, loop w/o continue, checking for catch\n",
			       O2S(objPtr), StringForResultCode(result)));
			Tcl_DecrRefCount(objPtr);
			goto checkForCatch;
		    } else {
			newPcOffset = rangePtr->continueOffset;
		    }
		    result = TCL_OK;
		    TRACE_WITH_OBJ(("\"%.30s\" => %s, range at %d, new pc %d ",
			    O2S(objPtr), StringForResultCode(result),
			    rangePtr->codeOffset, newPcOffset), valuePtr);
		    break;
		case CATCH_EXCEPTION_RANGE:
		    TRACE_WITH_OBJ(("\"%.30s\" => %s ",
			    O2S(objPtr), StringForResultCode(result)),
			    valuePtr);
		    Tcl_DecrRefCount(objPtr);
		    goto processCatch;  /* it will use rangePtr */
		default:
		    panic("TclExecuteByteCode: unrecognized ExceptionRange type %d\n", rangePtr->type);
		}
		Tcl_DecrRefCount(objPtr);
		pc = (codePtr->codeStart + newPcOffset);
		continue;	/* restart outer instruction loop at pc */
	    } else { /* eval returned TCL_ERROR, TCL_RETURN, unknown code */
		TRACE_WITH_OBJ(("\"%.30s\" => ERROR: ", O2S(objPtr)),
		        Tcl_GetObjResult(interp));
		Tcl_DecrRefCount(objPtr);
		goto checkForCatch;
	    }

	case INST_EXPR_STK:
	    objPtr = POP_OBJECT();
	    Tcl_ResetResult(interp);
	    DECACHE_STACK_INFO();
	    result = Tcl_ExprObj(interp, objPtr, &valuePtr);
	    CACHE_STACK_INFO();
	    if (result != TCL_OK) {
		TRACE_WITH_OBJ(("\"%.30s\" => ERROR: ", 
		        O2S(objPtr)), Tcl_GetObjResult(interp));
		Tcl_DecrRefCount(objPtr);
		goto checkForCatch;
	    }
	    stackPtr[++stackTop] = valuePtr; /* already has right refct */
	    TRACE_WITH_OBJ(("\"%.30s\" => ", O2S(objPtr)), valuePtr);
	    TclDecrRefCount(objPtr);
	    ADJUST_PC(1);

	case INST_LOAD_SCALAR1:
#ifdef TCL_COMPILE_DEBUG
	    opnd = TclGetUInt1AtPtr(pc+1);
	    DECACHE_STACK_INFO();
	    valuePtr = TclGetIndexedScalar(interp, opnd,
		    /*leaveErrorMsg*/ 1);
	    CACHE_STACK_INFO();
	    if (valuePtr == NULL) {
		TRACE_WITH_OBJ(("%u => ERROR: ", opnd),
		        Tcl_GetObjResult(interp));
		result = TCL_ERROR;
		goto checkForCatch;
            }
	    PUSH_OBJECT(valuePtr);
	    TRACE_WITH_OBJ(("%u => ", opnd), valuePtr);
#else /* TCL_COMPILE_DEBUG */
	    DECACHE_STACK_INFO();
	    opnd = TclGetUInt1AtPtr(pc+1);
	    valuePtr = TclGetIndexedScalar(interp, opnd, /*leaveErrorMsg*/ 1);
	    CACHE_STACK_INFO();
	    if (valuePtr == NULL) {
		result = TCL_ERROR;
		goto checkForCatch;
            }
	    PUSH_OBJECT(valuePtr);
#endif /* TCL_COMPILE_DEBUG */
	    ADJUST_PC(2);

	case INST_LOAD_SCALAR4:
	    opnd = TclGetUInt4AtPtr(pc+1);
	    DECACHE_STACK_INFO();
	    valuePtr = TclGetIndexedScalar(interp, opnd,
					   /*leaveErrorMsg*/ 1);
	    CACHE_STACK_INFO();
	    if (valuePtr == NULL) {
		TRACE_WITH_OBJ(("%u => ERROR: ", opnd),
		        Tcl_GetObjResult(interp));
		result = TCL_ERROR;
		goto checkForCatch;
            }
	    PUSH_OBJECT(valuePtr);
	    TRACE_WITH_OBJ(("%u => ", opnd), valuePtr);
	    ADJUST_PC(5);

	case INST_LOAD_SCALAR_STK:
	    objPtr = POP_OBJECT(); /* scalar name */
	    DECACHE_STACK_INFO();
	    valuePtr = Tcl_ObjGetVar2(interp, objPtr, NULL, TCL_LEAVE_ERR_MSG);
	    CACHE_STACK_INFO();
	    if (valuePtr == NULL) {
		TRACE_WITH_OBJ(("\"%.30s\" => ERROR: ", O2S(objPtr)),
		        Tcl_GetObjResult(interp));
		Tcl_DecrRefCount(objPtr);
		result = TCL_ERROR;
		goto checkForCatch;
            }
	    PUSH_OBJECT(valuePtr);
	    TRACE_WITH_OBJ(("\"%.30s\" => ", O2S(objPtr)), valuePtr);
	    TclDecrRefCount(objPtr);
	    ADJUST_PC(1);

	case INST_LOAD_ARRAY4:
	    opnd = TclGetUInt4AtPtr(pc+1);
	    pcAdjustment = 5;
	    goto doLoadArray;

	case INST_LOAD_ARRAY1:
	    opnd = TclGetUInt1AtPtr(pc+1);
	    pcAdjustment = 2;
	    
	    doLoadArray:
	    {
		Tcl_Obj *elemPtr = POP_OBJECT();
		
		DECACHE_STACK_INFO();
		valuePtr = TclGetElementOfIndexedArray(interp, opnd,
	                elemPtr, /*leaveErrorMsg*/ 1);
		CACHE_STACK_INFO();
		if (valuePtr == NULL) {
		    TRACE_WITH_OBJ(("%u \"%.30s\" => ERROR: ",
			    opnd, O2S(elemPtr)), Tcl_GetObjResult(interp));
		    Tcl_DecrRefCount(elemPtr);
		    result = TCL_ERROR;
		    goto checkForCatch;
		}
		PUSH_OBJECT(valuePtr);
		TRACE_WITH_OBJ(("%u \"%.30s\" => ",
		        opnd, O2S(elemPtr)),valuePtr);
		TclDecrRefCount(elemPtr);
	    }
	    ADJUST_PC(pcAdjustment);

	case INST_LOAD_ARRAY_STK:
	    {
		Tcl_Obj *elemPtr = POP_OBJECT();
		
		objPtr = POP_OBJECT();	/* array name */
		DECACHE_STACK_INFO();
		valuePtr = Tcl_ObjGetVar2(interp, objPtr, elemPtr,
		        TCL_LEAVE_ERR_MSG);
		CACHE_STACK_INFO();
		if (valuePtr == NULL) {
		    TRACE_WITH_OBJ(("\"%.30s(%.30s)\" => ERROR: ",
		            O2S(objPtr), O2S(elemPtr)),
			    Tcl_GetObjResult(interp));
		    Tcl_DecrRefCount(objPtr);
		    Tcl_DecrRefCount(elemPtr);
		    result = TCL_ERROR;
		    goto checkForCatch;
		}
		PUSH_OBJECT(valuePtr);
		TRACE_WITH_OBJ(("\"%.30s(%.30s)\" => ",
		        O2S(objPtr), O2S(elemPtr)), valuePtr);
		TclDecrRefCount(objPtr);
		TclDecrRefCount(elemPtr);
	    }
	    ADJUST_PC(1);

	case INST_LOAD_STK:
	    objPtr = POP_OBJECT(); /* variable name */
	    DECACHE_STACK_INFO();
	    valuePtr = Tcl_ObjGetVar2(interp, objPtr, NULL, TCL_LEAVE_ERR_MSG);
	    CACHE_STACK_INFO();
	    if (valuePtr == NULL) {
		TRACE_WITH_OBJ(("\"%.30s\" => ERROR: ",
		        O2S(objPtr)), Tcl_GetObjResult(interp));
		Tcl_DecrRefCount(objPtr);
		result = TCL_ERROR;
		goto checkForCatch;
	    }
	    PUSH_OBJECT(valuePtr);
	    TRACE_WITH_OBJ(("\"%.30s\" => ", O2S(objPtr)), valuePtr);
	    TclDecrRefCount(objPtr);
	    ADJUST_PC(1);
	    
	case INST_STORE_SCALAR4:
	    opnd = TclGetUInt4AtPtr(pc+1);
	    pcAdjustment = 5;
	    goto doStoreScalar;

	case INST_STORE_SCALAR1:
	    opnd = TclGetUInt1AtPtr(pc+1);
	    pcAdjustment = 2;
	    
	  doStoreScalar:
	    valuePtr = POP_OBJECT();
	    DECACHE_STACK_INFO();
	    value2Ptr = TclSetIndexedScalar(interp, opnd, valuePtr,
	            /*leaveErrorMsg*/ 1);
	    CACHE_STACK_INFO();
	    if (value2Ptr == NULL) {
		TRACE_WITH_OBJ(("%u <- \"%.30s\" => ERROR: ",
			opnd, O2S(valuePtr)), Tcl_GetObjResult(interp));
		Tcl_DecrRefCount(valuePtr);
		result = TCL_ERROR;
		goto checkForCatch;
	    }
	    PUSH_OBJECT(value2Ptr);
	    TRACE_WITH_OBJ(("%u <- \"%.30s\" => ",
		    opnd, O2S(valuePtr)), value2Ptr);
	    TclDecrRefCount(valuePtr);
	    ADJUST_PC(pcAdjustment);

	case INST_STORE_SCALAR_STK:
	    valuePtr = POP_OBJECT();
	    objPtr = POP_OBJECT(); /* scalar name */
	    DECACHE_STACK_INFO();
	    value2Ptr = Tcl_ObjSetVar2(interp, objPtr, NULL, valuePtr,
		    TCL_LEAVE_ERR_MSG);
	    CACHE_STACK_INFO();
	    if (value2Ptr == NULL) {
		TRACE_WITH_OBJ(("\"%.30s\" <- \"%.30s\" => ERROR: ",
		        O2S(objPtr), O2S(valuePtr)),
			Tcl_GetObjResult(interp));
		Tcl_DecrRefCount(objPtr);
		Tcl_DecrRefCount(valuePtr);
		result = TCL_ERROR;
		goto checkForCatch;
	    }
	    PUSH_OBJECT(value2Ptr);
	    TRACE_WITH_OBJ(("\"%.30s\" <- \"%.30s\" => ",
		    O2S(objPtr), O2S(valuePtr)), value2Ptr);
	    TclDecrRefCount(objPtr);
	    TclDecrRefCount(valuePtr);
	    ADJUST_PC(1);

	case INST_STORE_ARRAY4:
	    opnd = TclGetUInt4AtPtr(pc+1);
	    pcAdjustment = 5;
	    goto doStoreArray;

	case INST_STORE_ARRAY1:
	    opnd = TclGetUInt1AtPtr(pc+1);
	    pcAdjustment = 2;
	    
	    doStoreArray:
	    {
		Tcl_Obj *elemPtr;

		valuePtr = POP_OBJECT();
		elemPtr = POP_OBJECT();
		DECACHE_STACK_INFO();
		value2Ptr = TclSetElementOfIndexedArray(interp, opnd,
		        elemPtr, valuePtr, TCL_LEAVE_ERR_MSG);
		CACHE_STACK_INFO();
		if (value2Ptr == NULL) {
		    TRACE_WITH_OBJ(("%u \"%.30s\" <- \"%.30s\" => ERROR: ",
			    opnd, O2S(elemPtr), O2S(valuePtr)),
			    Tcl_GetObjResult(interp));
		    Tcl_DecrRefCount(elemPtr);
		    Tcl_DecrRefCount(valuePtr);
		    result = TCL_ERROR;
		    goto checkForCatch;
		}
		PUSH_OBJECT(value2Ptr);
		TRACE_WITH_OBJ(("%u \"%.30s\" <- \"%.30s\" => ",
		        opnd, O2S(elemPtr), O2S(valuePtr)), value2Ptr);
		TclDecrRefCount(elemPtr);
		TclDecrRefCount(valuePtr);
	    }
	    ADJUST_PC(pcAdjustment);

	case INST_STORE_ARRAY_STK:
	    {
		Tcl_Obj *elemPtr;

		valuePtr = POP_OBJECT();
		elemPtr = POP_OBJECT();
		objPtr = POP_OBJECT();	/* array name */
		DECACHE_STACK_INFO();
		value2Ptr = Tcl_ObjSetVar2(interp, objPtr, elemPtr, valuePtr,
			TCL_LEAVE_ERR_MSG);
		CACHE_STACK_INFO();
		if (value2Ptr == NULL) {
		    TRACE_WITH_OBJ(("\"%.30s(%.30s)\" <- \"%.30s\" => ERROR: ",
		            O2S(objPtr), O2S(elemPtr), O2S(valuePtr)),
			    Tcl_GetObjResult(interp));
		    Tcl_DecrRefCount(objPtr);
		    Tcl_DecrRefCount(elemPtr);
		    Tcl_DecrRefCount(valuePtr);
		    result = TCL_ERROR;
		    goto checkForCatch;
		}
		PUSH_OBJECT(value2Ptr);
		TRACE_WITH_OBJ(("\"%.30s(%.30s)\" <- \"%.30s\" => ",
		        O2S(objPtr), O2S(elemPtr), O2S(valuePtr)),
			value2Ptr);
		TclDecrRefCount(objPtr);
		TclDecrRefCount(elemPtr);
		TclDecrRefCount(valuePtr);
	    }
	    ADJUST_PC(1);

	case INST_STORE_STK:
	    valuePtr = POP_OBJECT();
	    objPtr = POP_OBJECT(); /* variable name */
	    DECACHE_STACK_INFO();
	    value2Ptr = Tcl_ObjSetVar2(interp, objPtr, NULL, valuePtr,
		    TCL_LEAVE_ERR_MSG);
	    CACHE_STACK_INFO();
	    if (value2Ptr == NULL) {
		TRACE_WITH_OBJ(("\"%.30s\" <- \"%.30s\" => ERROR: ",
		        O2S(objPtr), O2S(valuePtr)),
			Tcl_GetObjResult(interp));
		Tcl_DecrRefCount(objPtr);
		Tcl_DecrRefCount(valuePtr);
		result = TCL_ERROR;
		goto checkForCatch;
	    }
	    PUSH_OBJECT(value2Ptr);
	    TRACE_WITH_OBJ(("\"%.30s\" <- \"%.30s\" => ",
		    O2S(objPtr), O2S(valuePtr)), value2Ptr);
	    TclDecrRefCount(objPtr);
	    TclDecrRefCount(valuePtr);
	    ADJUST_PC(1);

	case INST_INCR_SCALAR1:
	    opnd = TclGetUInt1AtPtr(pc+1);
	    valuePtr = POP_OBJECT(); 
	    if (valuePtr->typePtr != &tclIntType) {
		result = tclIntType.setFromAnyProc(interp, valuePtr);
		if (result != TCL_OK) {
		    TRACE_WITH_OBJ(("%u (by %s) => ERROR converting increment amount to int: ",
		            opnd, O2S(valuePtr)), Tcl_GetObjResult(interp));
		    Tcl_DecrRefCount(valuePtr);
		    goto checkForCatch;
		}
	    }
	    i = valuePtr->internalRep.longValue;
	    DECACHE_STACK_INFO();
	    value2Ptr = TclIncrIndexedScalar(interp, opnd, i);
	    CACHE_STACK_INFO();
	    if (value2Ptr == NULL) {
		TRACE_WITH_OBJ(("%u (by %ld) => ERROR: ", opnd, i),
			Tcl_GetObjResult(interp));
		Tcl_DecrRefCount(valuePtr);
		result = TCL_ERROR;
		goto checkForCatch;
	    }
	    PUSH_OBJECT(value2Ptr);
	    TRACE_WITH_OBJ(("%u (by %ld) => ", opnd, i), value2Ptr);
	    TclDecrRefCount(valuePtr);
	    ADJUST_PC(2);

	case INST_INCR_SCALAR_STK:
	case INST_INCR_STK:
	    valuePtr = POP_OBJECT();
	    objPtr = POP_OBJECT(); /* scalar name */
	    if (valuePtr->typePtr != &tclIntType) {
		result = tclIntType.setFromAnyProc(interp, valuePtr);
		if (result != TCL_OK) {
		    TRACE_WITH_OBJ(("\"%.30s\" (by %s) => ERROR converting increment amount to int: ",
		            O2S(objPtr), O2S(valuePtr)),
			    Tcl_GetObjResult(interp));
		    Tcl_DecrRefCount(objPtr);
		    Tcl_DecrRefCount(valuePtr);
		    goto checkForCatch;
		}
	    }
	    i = valuePtr->internalRep.longValue;
	    DECACHE_STACK_INFO();
	    value2Ptr = TclIncrVar2(interp, objPtr, (Tcl_Obj *) NULL, i,
		    TCL_LEAVE_ERR_MSG);
	    CACHE_STACK_INFO();
	    if (value2Ptr == NULL) {
		TRACE_WITH_OBJ(("\"%.30s\" (by %ld) => ERROR: ",
		        O2S(objPtr), i), Tcl_GetObjResult(interp));
		Tcl_DecrRefCount(objPtr);
		Tcl_DecrRefCount(valuePtr);
		result = TCL_ERROR;
		goto checkForCatch;
	    }
	    PUSH_OBJECT(value2Ptr);
	    TRACE_WITH_OBJ(("\"%.30s\" (by %ld) => ", O2S(objPtr), i),
		    value2Ptr);
	    Tcl_DecrRefCount(objPtr);
	    Tcl_DecrRefCount(valuePtr);
	    ADJUST_PC(1);

	case INST_INCR_ARRAY1:
	    {
		Tcl_Obj *elemPtr;

		opnd = TclGetUInt1AtPtr(pc+1);
		valuePtr = POP_OBJECT();
		elemPtr = POP_OBJECT();
		if (valuePtr->typePtr != &tclIntType) {
		    result = tclIntType.setFromAnyProc(interp, valuePtr);
		    if (result != TCL_OK) {
			TRACE_WITH_OBJ(("%u \"%.30s\" (by %s) => ERROR converting increment amount to int: ",
		                opnd, O2S(elemPtr), O2S(valuePtr)),
			        Tcl_GetObjResult(interp));
			Tcl_DecrRefCount(elemPtr);
			Tcl_DecrRefCount(valuePtr);
			goto checkForCatch;
		    }
		}
		i = valuePtr->internalRep.longValue;
		DECACHE_STACK_INFO();
		value2Ptr = TclIncrElementOfIndexedArray(interp, opnd,
		        elemPtr, i);
		CACHE_STACK_INFO();
		if (value2Ptr == NULL) {
		    TRACE_WITH_OBJ(("%u \"%.30s\" (by %ld) => ERROR: ",
		            opnd, O2S(elemPtr), i),
			    Tcl_GetObjResult(interp));
		    Tcl_DecrRefCount(elemPtr);
		    Tcl_DecrRefCount(valuePtr);
		    result = TCL_ERROR;
		    goto checkForCatch;
		}
		PUSH_OBJECT(value2Ptr);
		TRACE_WITH_OBJ(("%u \"%.30s\" (by %ld) => ",
	                opnd, O2S(elemPtr), i), value2Ptr);
		Tcl_DecrRefCount(elemPtr);
		Tcl_DecrRefCount(valuePtr);
	    }
	    ADJUST_PC(2);
	    
	case INST_INCR_ARRAY_STK:
	    {
		Tcl_Obj *elemPtr;

		valuePtr = POP_OBJECT();
		elemPtr = POP_OBJECT();
		objPtr = POP_OBJECT();	/* array name */
		if (valuePtr->typePtr != &tclIntType) {
		    result = tclIntType.setFromAnyProc(interp, valuePtr);
		    if (result != TCL_OK) {
		        TRACE_WITH_OBJ(("\"%.30s(%.30s)\" (by %s) => ERROR converting increment amount to int: ",
		                O2S(objPtr), O2S(elemPtr), O2S(valuePtr)),
			        Tcl_GetObjResult(interp));
			Tcl_DecrRefCount(objPtr);
			Tcl_DecrRefCount(elemPtr);
			Tcl_DecrRefCount(valuePtr);
			goto checkForCatch;
		    }
		}
		i = valuePtr->internalRep.longValue;
		DECACHE_STACK_INFO();
		value2Ptr = TclIncrVar2(interp, objPtr, elemPtr, i,
			TCL_LEAVE_ERR_MSG);
		CACHE_STACK_INFO();
		if (value2Ptr == NULL) {
		    TRACE_WITH_OBJ(("\"%.30s(%.30s)\" (by %ld) => ERROR: ",
		            O2S(objPtr), O2S(elemPtr), i),
			    Tcl_GetObjResult(interp));
		    Tcl_DecrRefCount(objPtr);
		    Tcl_DecrRefCount(elemPtr);
		    Tcl_DecrRefCount(valuePtr);
		    result = TCL_ERROR;
		    goto checkForCatch;
		}
		PUSH_OBJECT(value2Ptr);
		TRACE_WITH_OBJ(("\"%.30s(%.30s)\" (by %ld) => ",
	                O2S(objPtr), O2S(elemPtr), i), value2Ptr);
		Tcl_DecrRefCount(objPtr);
		Tcl_DecrRefCount(elemPtr);
		Tcl_DecrRefCount(valuePtr);
	    }
	    ADJUST_PC(1);
	    
	case INST_INCR_SCALAR1_IMM:
	    opnd = TclGetUInt1AtPtr(pc+1);
	    i = TclGetInt1AtPtr(pc+2);
	    DECACHE_STACK_INFO();
	    value2Ptr = TclIncrIndexedScalar(interp, opnd, i);
	    CACHE_STACK_INFO();
	    if (value2Ptr == NULL) {
		TRACE_WITH_OBJ(("%u %ld => ERROR: ", opnd, i),
			Tcl_GetObjResult(interp));
		result = TCL_ERROR;
		goto checkForCatch;
	    }
	    PUSH_OBJECT(value2Ptr);
	    TRACE_WITH_OBJ(("%u %ld => ", opnd, i), value2Ptr);
	    ADJUST_PC(3);

	case INST_INCR_SCALAR_STK_IMM:
	case INST_INCR_STK_IMM:
	    objPtr = POP_OBJECT(); /* variable name */
	    i = TclGetInt1AtPtr(pc+1);
	    DECACHE_STACK_INFO();
	    value2Ptr = TclIncrVar2(interp, objPtr, (Tcl_Obj *) NULL, i,
		    TCL_LEAVE_ERR_MSG);
	    CACHE_STACK_INFO();
	    if (value2Ptr == NULL) {
		TRACE_WITH_OBJ(("\"%.30s\" %ld => ERROR: ",
		        O2S(objPtr), i), Tcl_GetObjResult(interp));
		result = TCL_ERROR;
		Tcl_DecrRefCount(objPtr);
		goto checkForCatch;
	    }
	    PUSH_OBJECT(value2Ptr);
	    TRACE_WITH_OBJ(("\"%.30s\" %ld => ", O2S(objPtr), i),
		    value2Ptr);
	    TclDecrRefCount(objPtr);
	    ADJUST_PC(2);

	case INST_INCR_ARRAY1_IMM:
	    {
		Tcl_Obj *elemPtr;

		opnd = TclGetUInt1AtPtr(pc+1);
		i = TclGetInt1AtPtr(pc+2);
		elemPtr = POP_OBJECT();
		DECACHE_STACK_INFO();
		value2Ptr = TclIncrElementOfIndexedArray(interp, opnd,
		        elemPtr, i);
		CACHE_STACK_INFO();
		if (value2Ptr == NULL) {
		    TRACE_WITH_OBJ(("%u \"%.30s\" (by %ld) => ERROR: ",
		            opnd, O2S(elemPtr), i),
			    Tcl_GetObjResult(interp));
		    Tcl_DecrRefCount(elemPtr);
		    result = TCL_ERROR;
		    goto checkForCatch;
		}
		PUSH_OBJECT(value2Ptr);
		TRACE_WITH_OBJ(("%u \"%.30s\" (by %ld) => ",
	                opnd, O2S(elemPtr), i), value2Ptr);
		Tcl_DecrRefCount(elemPtr);
	    }
	    ADJUST_PC(3);
	    
	case INST_INCR_ARRAY_STK_IMM:
	    {
		Tcl_Obj *elemPtr;

		i = TclGetInt1AtPtr(pc+1);
		elemPtr = POP_OBJECT();
		objPtr = POP_OBJECT();	/* array name */
		DECACHE_STACK_INFO();
		value2Ptr = TclIncrVar2(interp, objPtr, elemPtr, i,
			TCL_LEAVE_ERR_MSG);
		CACHE_STACK_INFO();
		if (value2Ptr == NULL) {
		    TRACE_WITH_OBJ(("\"%.30s(%.30s)\" (by %ld) => ERROR: ",
		            O2S(objPtr), O2S(elemPtr), i),
			    Tcl_GetObjResult(interp));
		    Tcl_DecrRefCount(objPtr);
		    Tcl_DecrRefCount(elemPtr);
		    result = TCL_ERROR;
		    goto checkForCatch;
		}
		PUSH_OBJECT(value2Ptr);
		TRACE_WITH_OBJ(("\"%.30s(%.30s)\" (by %ld) => ",
	                O2S(objPtr), O2S(elemPtr), i), value2Ptr);
		Tcl_DecrRefCount(objPtr);
		Tcl_DecrRefCount(elemPtr);
	    }
	    ADJUST_PC(2);

	case INST_JUMP1:
#ifdef TCL_COMPILE_DEBUG
	    opnd = TclGetInt1AtPtr(pc+1);
	    TRACE(("%d => new pc %u\n", opnd,
		   (unsigned int)(pc + opnd - codePtr->codeStart)));
	    pc += opnd;
#else
	    pc += TclGetInt1AtPtr(pc+1);
#endif /* TCL_COMPILE_DEBUG */
	    continue;

	case INST_JUMP4:
	    opnd = TclGetInt4AtPtr(pc+1);
	    TRACE(("%d => new pc %u\n", opnd,
		   (unsigned int)(pc + opnd - codePtr->codeStart)));
	    ADJUST_PC(opnd);

	case INST_JUMP_TRUE4:
	    opnd = TclGetInt4AtPtr(pc+1);
	    pcAdjustment = 5;
	    goto doJumpTrue;

	case INST_JUMP_TRUE1:
	    opnd = TclGetInt1AtPtr(pc+1);
	    pcAdjustment = 2;
	    
	    doJumpTrue:
	    {
		int b;
		
		valuePtr = POP_OBJECT();
		if (valuePtr->typePtr == &tclIntType) {
		    b = (valuePtr->internalRep.longValue != 0);
		} else if (valuePtr->typePtr == &tclDoubleType) {
		    b = (valuePtr->internalRep.doubleValue != 0.0);
		} else {
		    result = Tcl_GetBooleanFromObj(interp, valuePtr, &b);
		    if (result != TCL_OK) {
			TRACE_WITH_OBJ(("%d => ERROR: ", opnd),
				Tcl_GetObjResult(interp));
			Tcl_DecrRefCount(valuePtr);
			goto checkForCatch;
		    }
		}
		if (b) {
		    TRACE(("%d => %.20s true, new pc %u\n",
			    opnd, O2S(valuePtr),
		            (unsigned int)(pc+opnd - codePtr->codeStart)));
		    TclDecrRefCount(valuePtr);
		    ADJUST_PC(opnd);
		} else {
		    TRACE(("%d => %.20s false\n", opnd, O2S(valuePtr)));
		    TclDecrRefCount(valuePtr);
		    ADJUST_PC(pcAdjustment);
		}
	    }
	    
	case INST_JUMP_FALSE4:
	    opnd = TclGetInt4AtPtr(pc+1);
	    pcAdjustment = 5;
	    goto doJumpFalse;

	case INST_JUMP_FALSE1:
	    opnd = TclGetInt1AtPtr(pc+1);
	    pcAdjustment = 2;
	    
	    doJumpFalse:
	    {
		int b;
		
		valuePtr = POP_OBJECT();
		if (valuePtr->typePtr == &tclIntType) {
		    b = (valuePtr->internalRep.longValue != 0);
		} else if (valuePtr->typePtr == &tclDoubleType) {
		    b = (valuePtr->internalRep.doubleValue != 0.0);
		} else {
		    result = Tcl_GetBooleanFromObj(interp, valuePtr, &b);
		    if (result != TCL_OK) {
			TRACE_WITH_OBJ(("%d => ERROR: ", opnd),
				Tcl_GetObjResult(interp));
			Tcl_DecrRefCount(valuePtr);
			goto checkForCatch;
		    }
		}
		if (b) {
		    TRACE(("%d => %.20s true\n", opnd, O2S(valuePtr)));
		    TclDecrRefCount(valuePtr);
		    ADJUST_PC(pcAdjustment);
		} else {
		    TRACE(("%d => %.20s false, new pc %u\n",
			   opnd, O2S(valuePtr),
			   (unsigned int)(pc + opnd - codePtr->codeStart)));
		    TclDecrRefCount(valuePtr);
		    ADJUST_PC(opnd);
		}
	    }
	    
	case INST_LOR:
	case INST_LAND:
	    {
		/*
		 * Operands must be boolean or numeric. No int->double
		 * conversions are performed.
		 */
		
		int i1, i2;
		int iResult;
		char *s;
		Tcl_ObjType *t1Ptr, *t2Ptr;
		
		value2Ptr = POP_OBJECT();
		valuePtr  = POP_OBJECT();
		t1Ptr = valuePtr->typePtr;
		t2Ptr = value2Ptr->typePtr;
		
		if ((t1Ptr == &tclIntType) || (t1Ptr == &tclBooleanType)) {
		    i1 = (valuePtr->internalRep.longValue != 0);
		} else if (t1Ptr == &tclDoubleType) {
		    i1 = (valuePtr->internalRep.doubleValue != 0.0);
		} else {
		    s = Tcl_GetStringFromObj(valuePtr, &length);
		    if (TclLooksLikeInt(s, length)) {
			result = Tcl_GetLongFromObj((Tcl_Interp *) NULL,
				valuePtr, &i);
			i1 = (i != 0);
		    } else {
			result = Tcl_GetBooleanFromObj((Tcl_Interp *) NULL,
				valuePtr, &i1);
			i1 = (i1 != 0);
		    }
		    if (result != TCL_OK) {
			TRACE(("\"%.20s\" => ILLEGAL TYPE %s \n",
			        O2S(valuePtr),
			        (t1Ptr? t1Ptr->name : "null")));
			IllegalExprOperandType(interp, pc, valuePtr);
			Tcl_DecrRefCount(valuePtr);
			Tcl_DecrRefCount(value2Ptr);
			goto checkForCatch;
		    }
		}
		
		if ((t2Ptr == &tclIntType) || (t2Ptr == &tclBooleanType)) {
		    i2 = (value2Ptr->internalRep.longValue != 0);
		} else if (t2Ptr == &tclDoubleType) {
		    i2 = (value2Ptr->internalRep.doubleValue != 0.0);
		} else {
		    s = Tcl_GetStringFromObj(value2Ptr, &length);
		    if (TclLooksLikeInt(s, length)) {
			result = Tcl_GetLongFromObj((Tcl_Interp *) NULL,
				value2Ptr, &i);
			i2 = (i != 0);
		    } else {
			result = Tcl_GetBooleanFromObj((Tcl_Interp *) NULL,
				value2Ptr, &i2);
		    }
		    if (result != TCL_OK) {
			TRACE(("\"%.20s\" => ILLEGAL TYPE %s \n",
			        O2S(value2Ptr),
			        (t2Ptr? t2Ptr->name : "null")));
			IllegalExprOperandType(interp, pc, value2Ptr);
			Tcl_DecrRefCount(valuePtr);
			Tcl_DecrRefCount(value2Ptr);
			goto checkForCatch;
		    }
		}
		
		/*
		 * Reuse the valuePtr object already on stack if possible.
		 */

		if (*pc == INST_LOR) {
		    iResult = (i1 || i2);
		} else {
		    iResult = (i1 && i2);
		}
		if (Tcl_IsShared(valuePtr)) {
		    PUSH_OBJECT(Tcl_NewLongObj(iResult));
		    TRACE(("%.20s %.20s => %d\n",
			   O2S(valuePtr), O2S(value2Ptr), iResult));
		    TclDecrRefCount(valuePtr);
		} else {	/* reuse the valuePtr object */
		    TRACE(("%.20s %.20s => %d\n", 
			   O2S(valuePtr), O2S(value2Ptr), iResult));
		    Tcl_SetLongObj(valuePtr, iResult);
		    ++stackTop; /* valuePtr now on stk top has right r.c. */
		}
		TclDecrRefCount(value2Ptr);
	    }
	    ADJUST_PC(1);

	case INST_EQ:
	case INST_NEQ:
	case INST_LT:
	case INST_GT:
	case INST_LE:
	case INST_GE:
	    {
		/*
		 * Any type is allowed but the two operands must have the
	         * same type. We will compute value op value2.
		 */

		Tcl_ObjType *t1Ptr, *t2Ptr;
		char *s1 = NULL;   /* Init. avoids compiler warning. */
		char *s2 = NULL;   /* Init. avoids compiler warning. */
		long i2 = 0;	   /* Init. avoids compiler warning. */
		double d1 = 0.0;   /* Init. avoids compiler warning. */
		double d2 = 0.0;   /* Init. avoids compiler warning. */
		long iResult = 0;  /* Init. avoids compiler warning. */

		value2Ptr = POP_OBJECT();
		valuePtr  = POP_OBJECT();
		t1Ptr = valuePtr->typePtr;
		t2Ptr = value2Ptr->typePtr;
		
		if ((t1Ptr != &tclIntType) && (t1Ptr != &tclDoubleType)) {
		    s1 = Tcl_GetStringFromObj(valuePtr, &length);
		    if (TclLooksLikeInt(s1, length)) {
			(void) Tcl_GetLongFromObj((Tcl_Interp *) NULL,
				valuePtr, &i);
		    } else {
			(void) Tcl_GetDoubleFromObj((Tcl_Interp *) NULL,
				valuePtr, &d1);
		    }
		    t1Ptr = valuePtr->typePtr;
		}
		if ((t2Ptr != &tclIntType) && (t2Ptr != &tclDoubleType)) {
		    s2 = Tcl_GetStringFromObj(value2Ptr, &length);
		    if (TclLooksLikeInt(s2, length)) {
			(void) Tcl_GetLongFromObj((Tcl_Interp *) NULL,
				value2Ptr, &i2);
		    } else {
			(void) Tcl_GetDoubleFromObj((Tcl_Interp *) NULL,
				value2Ptr, &d2);
		    }
		    t2Ptr = value2Ptr->typePtr;
		}

		if (((t1Ptr != &tclIntType) && (t1Ptr != &tclDoubleType))
		        || ((t2Ptr != &tclIntType) && (t2Ptr != &tclDoubleType))) {
		    /*
		     * One operand is not numeric. Compare as strings.
		     */
		    int cmpValue;
		    s1 = Tcl_GetString(valuePtr);
		    s2 = Tcl_GetString(value2Ptr);
		    cmpValue = strcmp(s1, s2);
		    switch (*pc) {
		    case INST_EQ:
			iResult = (cmpValue == 0);
			break;
		    case INST_NEQ:
			iResult = (cmpValue != 0);
			break;
		    case INST_LT:
			iResult = (cmpValue < 0);
			break;
		    case INST_GT:
			iResult = (cmpValue > 0);
			break;
		    case INST_LE:
			iResult = (cmpValue <= 0);
			break;
		    case INST_GE:
			iResult = (cmpValue >= 0);
			break;
		    }
		} else if ((t1Ptr == &tclDoubleType)
		        || (t2Ptr == &tclDoubleType)) {
		    /*
		     * Compare as doubles.
		     */
		    if (t1Ptr == &tclDoubleType) {
			d1 = valuePtr->internalRep.doubleValue;
			if (t2Ptr == &tclIntType) {
			    d2 = value2Ptr->internalRep.longValue;
			} else {
			    d2 = value2Ptr->internalRep.doubleValue;
			}
		    } else {	/* t1Ptr is int, t2Ptr is double */
			d1 = valuePtr->internalRep.longValue;
			d2 = value2Ptr->internalRep.doubleValue;
		    }
		    switch (*pc) {
		    case INST_EQ:
			iResult = d1 == d2;
			break;
		    case INST_NEQ:
			iResult = d1 != d2;
			break;
		    case INST_LT:
			iResult = d1 < d2;
			break;
		    case INST_GT:
			iResult = d1 > d2;
			break;
		    case INST_LE:
			iResult = d1 <= d2;
			break;
		    case INST_GE:
			iResult = d1 >= d2;
			break;
		    }
		} else {
		    /*
		     * Compare as ints.
		     */
		    i  = valuePtr->internalRep.longValue;
		    i2 = value2Ptr->internalRep.longValue;
		    switch (*pc) {
		    case INST_EQ:
			iResult = i == i2;
			break;
		    case INST_NEQ:
			iResult = i != i2;
			break;
		    case INST_LT:
			iResult = i < i2;
			break;
		    case INST_GT:
			iResult = i > i2;
			break;
		    case INST_LE:
			iResult = i <= i2;
			break;
		    case INST_GE:
			iResult = i >= i2;
			break;
		    }
		}

		/*
		 * Reuse the valuePtr object already on stack if possible.
		 */
		
		if (Tcl_IsShared(valuePtr)) {
		    PUSH_OBJECT(Tcl_NewLongObj(iResult));
		    TRACE(("%.20s %.20s => %ld\n",
			   O2S(valuePtr), O2S(value2Ptr), iResult));
		    TclDecrRefCount(valuePtr);
		} else {	/* reuse the valuePtr object */
		    TRACE(("%.20s %.20s => %ld\n",
			    O2S(valuePtr), O2S(value2Ptr), iResult));
		    Tcl_SetLongObj(valuePtr, iResult);
		    ++stackTop; /* valuePtr now on stk top has right r.c. */
		}
		TclDecrRefCount(value2Ptr);
	    }
	    ADJUST_PC(1);
	    
	case INST_MOD:
	case INST_LSHIFT:
	case INST_RSHIFT:
	case INST_BITOR:
	case INST_BITXOR:
	case INST_BITAND:
	    {
		/*
		 * Only integers are allowed. We compute value op value2.
		 */

		long i2, rem, negative;
		long iResult = 0; /* Init. avoids compiler warning. */
		
		value2Ptr = POP_OBJECT();
		valuePtr  = POP_OBJECT(); 
		if (valuePtr->typePtr == &tclIntType) {
		    i = valuePtr->internalRep.longValue;
		} else {	/* try to convert to int */
		    result = Tcl_GetLongFromObj((Tcl_Interp *) NULL,
			    valuePtr, &i);
		    if (result != TCL_OK) {
			TRACE(("%.20s %.20s => ILLEGAL 1st TYPE %s\n",
			      O2S(valuePtr), O2S(value2Ptr),
			      (valuePtr->typePtr?
				   valuePtr->typePtr->name : "null")));
			IllegalExprOperandType(interp, pc, valuePtr);
			Tcl_DecrRefCount(valuePtr);
			Tcl_DecrRefCount(value2Ptr);
			goto checkForCatch;
		    }
		}
		if (value2Ptr->typePtr == &tclIntType) {
		    i2 = value2Ptr->internalRep.longValue;
		} else {
		    result = Tcl_GetLongFromObj((Tcl_Interp *) NULL,
			    value2Ptr, &i2);
		    if (result != TCL_OK) {
			TRACE(("%.20s %.20s => ILLEGAL 2nd TYPE %s\n",
			      O2S(valuePtr), O2S(value2Ptr),
			      (value2Ptr->typePtr?
				   value2Ptr->typePtr->name : "null")));
			IllegalExprOperandType(interp, pc, value2Ptr);
			Tcl_DecrRefCount(valuePtr);
			Tcl_DecrRefCount(value2Ptr);
			goto checkForCatch;
		    }
		}

		switch (*pc) {
		case INST_MOD:
		    /*
		     * This code is tricky: C doesn't guarantee much about
		     * the quotient or remainder, but Tcl does. The
		     * remainder always has the same sign as the divisor and
		     * a smaller absolute value.
		     */
		    if (i2 == 0) {
			TRACE(("%ld %ld => DIVIDE BY ZERO\n", i, i2));
			Tcl_DecrRefCount(valuePtr);
			Tcl_DecrRefCount(value2Ptr);
			goto divideByZero;
		    }
		    negative = 0;
		    if (i2 < 0) {
			i2 = -i2;
			i = -i;
			negative = 1;
		    }
		    rem  = i % i2;
		    if (rem < 0) {
			rem += i2;
		    }
		    if (negative) {
			rem = -rem;
		    }
		    iResult = rem;
		    break;
		case INST_LSHIFT:
		    iResult = i << i2;
		    break;
		case INST_RSHIFT:
		    /*
		     * The following code is a bit tricky: it ensures that
		     * right shifts propagate the sign bit even on machines
		     * where ">>" won't do it by default.
		     */
		    if (i < 0) {
			iResult = ~((~i) >> i2);
		    } else {
			iResult = i >> i2;
		    }
		    break;
		case INST_BITOR:
		    iResult = i | i2;
		    break;
		case INST_BITXOR:
		    iResult = i ^ i2;
		    break;
		case INST_BITAND:
		    iResult = i & i2;
		    break;
		}

		/*
		 * Reuse the valuePtr object already on stack if possible.
		 */
		
		if (Tcl_IsShared(valuePtr)) {
		    PUSH_OBJECT(Tcl_NewLongObj(iResult));
		    TRACE(("%ld %ld => %ld\n", i, i2, iResult));
		    TclDecrRefCount(valuePtr);
		} else {	/* reuse the valuePtr object */
		    TRACE(("%ld %ld => %ld\n", i, i2, iResult));
		    Tcl_SetLongObj(valuePtr, iResult);
		    ++stackTop; /* valuePtr now on stk top has right r.c. */
		}
		TclDecrRefCount(value2Ptr);
	    }
	    ADJUST_PC(1);
	    
	case INST_ADD:
	case INST_SUB:
	case INST_MULT:
	case INST_DIV:
	    {
		/*
		 * Operands must be numeric and ints get converted to floats
		 * if necessary. We compute value op value2.
		 */

		Tcl_ObjType *t1Ptr, *t2Ptr;
		long i2, quot, rem;
		double d1, d2;
		long iResult = 0;     /* Init. avoids compiler warning. */
		double dResult = 0.0; /* Init. avoids compiler warning. */
		int doDouble = 0;     /* 1 if doing floating arithmetic */
		
		value2Ptr = POP_OBJECT();
		valuePtr  = POP_OBJECT();
		t1Ptr = valuePtr->typePtr;
		t2Ptr = value2Ptr->typePtr;
		
		if (t1Ptr == &tclIntType) {
		    i  = valuePtr->internalRep.longValue;
		} else if ((t1Ptr == &tclDoubleType)
			&& (valuePtr->bytes == NULL)) {
		    /*
		     * We can only use the internal rep directly if there is
		     * no string rep.  Otherwise the string rep might actually
		     * look like an integer, which is preferred.
		     */

		    d1 = valuePtr->internalRep.doubleValue;
		} else {
		    char *s = Tcl_GetStringFromObj(valuePtr, &length);
		    if (TclLooksLikeInt(s, length)) {
			result = Tcl_GetLongFromObj((Tcl_Interp *) NULL,
				valuePtr, &i);
		    } else {
			result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL,
				valuePtr, &d1);
		    }
		    if (result != TCL_OK) {
			TRACE(("%.20s %.20s => ILLEGAL 1st TYPE %s\n",
			       s, O2S(valuePtr),
			       (valuePtr->typePtr?
				    valuePtr->typePtr->name : "null")));
			IllegalExprOperandType(interp, pc, valuePtr);
			Tcl_DecrRefCount(valuePtr);
			Tcl_DecrRefCount(value2Ptr);
			goto checkForCatch;
		    }
		    t1Ptr = valuePtr->typePtr;
		}
		
		if (t2Ptr == &tclIntType) {
		    i2 = value2Ptr->internalRep.longValue;
		} else if ((t2Ptr == &tclDoubleType)
			&& (value2Ptr->bytes == NULL)) {
		    /*
		     * We can only use the internal rep directly if there is
		     * no string rep.  Otherwise the string rep might actually
		     * look like an integer, which is preferred.
		     */

		    d2 = value2Ptr->internalRep.doubleValue;
		} else {
		    char *s = Tcl_GetStringFromObj(value2Ptr, &length);
		    if (TclLooksLikeInt(s, length)) {
			result = Tcl_GetLongFromObj((Tcl_Interp *) NULL,
				value2Ptr, &i2);
		    } else {
			result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL,
				value2Ptr, &d2);
		    }
		    if (result != TCL_OK) {
			TRACE(("%.20s %.20s => ILLEGAL 2nd TYPE %s\n",
			       O2S(value2Ptr), s,
			       (value2Ptr->typePtr?
				    value2Ptr->typePtr->name : "null")));
			IllegalExprOperandType(interp, pc, value2Ptr);
			Tcl_DecrRefCount(valuePtr);
			Tcl_DecrRefCount(value2Ptr);
			goto checkForCatch;
		    }
		    t2Ptr = value2Ptr->typePtr;
		}

		if ((t1Ptr == &tclDoubleType) || (t2Ptr == &tclDoubleType)) {
		    /*
		     * Do double arithmetic.
		     */
		    doDouble = 1;
		    if (t1Ptr == &tclIntType) {
			d1 = i;       /* promote value 1 to double */
		    } else if (t2Ptr == &tclIntType) {
			d2 = i2;      /* promote value 2 to double */
		    }
		    switch (*pc) {
		    case INST_ADD:
			dResult = d1 + d2;
			break;
		    case INST_SUB:
			dResult = d1 - d2;
			break;
		    case INST_MULT:
			dResult = d1 * d2;
			break;
		    case INST_DIV:
			if (d2 == 0.0) {
			    TRACE(("%.6g %.6g => DIVIDE BY ZERO\n", d1, d2));
			    Tcl_DecrRefCount(valuePtr);
			    Tcl_DecrRefCount(value2Ptr);
			    goto divideByZero;
			}
			dResult = d1 / d2;
			break;
		    }
		    
		    /*
		     * Check now for IEEE floating-point error.
		     */
		    
		    if (IS_NAN(dResult) || IS_INF(dResult)) {
			TRACE(("%.20s %.20s => IEEE FLOATING PT ERROR\n",
			       O2S(valuePtr), O2S(value2Ptr)));
			TclExprFloatError(interp, dResult);
			result = TCL_ERROR;
			Tcl_DecrRefCount(valuePtr);
			Tcl_DecrRefCount(value2Ptr);
			goto checkForCatch;
		    }
		} else {
		    /*
		     * Do integer arithmetic.
		     */
		    switch (*pc) {
		    case INST_ADD:
			iResult = i + i2;
			break;
		    case INST_SUB:
			iResult = i - i2;
			break;
		    case INST_MULT:
			iResult = i * i2;
			break;
		    case INST_DIV:
			/*
			 * This code is tricky: C doesn't guarantee much
			 * about the quotient or remainder, but Tcl does.
			 * The remainder always has the same sign as the
			 * divisor and a smaller absolute value.
			 */
			if (i2 == 0) {
			    TRACE(("%ld %ld => DIVIDE BY ZERO\n", i, i2));
			    Tcl_DecrRefCount(valuePtr);
			    Tcl_DecrRefCount(value2Ptr);
			    goto divideByZero;
			}
			if (i2 < 0) {
			    i2 = -i2;
			    i = -i;
			}
			quot = i / i2;
			rem  = i % i2;
			if (rem < 0) {
			    quot -= 1;
			}
			iResult = quot;
			break;
		    }
		}

		/*
		 * Reuse the valuePtr object already on stack if possible.
		 */
		
		if (Tcl_IsShared(valuePtr)) {
		    if (doDouble) {
			PUSH_OBJECT(Tcl_NewDoubleObj(dResult));
			TRACE(("%.6g %.6g => %.6g\n", d1, d2, dResult));
		    } else {
			PUSH_OBJECT(Tcl_NewLongObj(iResult));
			TRACE(("%ld %ld => %ld\n", i, i2, iResult));
		    } 
		    TclDecrRefCount(valuePtr);
		} else {	    /* reuse the valuePtr object */
		    if (doDouble) { /* NB: stack top is off by 1 */
			TRACE(("%.6g %.6g => %.6g\n", d1, d2, dResult));
			Tcl_SetDoubleObj(valuePtr, dResult);
		    } else {
			TRACE(("%ld %ld => %ld\n", i, i2, iResult));
			Tcl_SetLongObj(valuePtr, iResult);
		    }
		    ++stackTop; /* valuePtr now on stk top has right r.c. */
		}
		TclDecrRefCount(value2Ptr);
	    }
	    ADJUST_PC(1);
	    
	case INST_UPLUS:
	    {
	        /*
	         * Operand must be numeric.
	         */

		double d;
		Tcl_ObjType *tPtr;
		
		valuePtr = stackPtr[stackTop];
		tPtr = valuePtr->typePtr;
		if ((tPtr != &tclIntType) && ((tPtr != &tclDoubleType)
			|| (valuePtr->bytes != NULL))) {
		    char *s = Tcl_GetStringFromObj(valuePtr, &length);
		    if (TclLooksLikeInt(s, length)) {
			result = Tcl_GetLongFromObj((Tcl_Interp *) NULL,
				valuePtr, &i);
		    } else {
			result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL,
				valuePtr, &d);
		    }
		    if (result != TCL_OK) { 
			TRACE(("\"%.20s\" => ILLEGAL TYPE %s \n",
			        s, (tPtr? tPtr->name : "null")));
			IllegalExprOperandType(interp, pc, valuePtr);
			goto checkForCatch;
		    }
		    tPtr = valuePtr->typePtr;
		}

		/*
		 * Ensure that the operand's string rep is the same as the
		 * formatted version of its internal rep. This makes sure
		 * that "expr +000123" yields "83", not "000123". We
		 * implement this by _discarding_ the string rep since we
		 * know it will be regenerated, if needed later, by
		 * formatting the internal rep's value.
		 */

		if (Tcl_IsShared(valuePtr)) {
		    if (tPtr == &tclIntType) {
			i = valuePtr->internalRep.longValue;
			objPtr = Tcl_NewLongObj(i);
		    } else {
			d = valuePtr->internalRep.doubleValue;
			objPtr = Tcl_NewDoubleObj(d);
		    }
		    Tcl_IncrRefCount(objPtr);
		    Tcl_DecrRefCount(valuePtr);
		    valuePtr = objPtr;
		    stackPtr[stackTop] = valuePtr;
		} else {
		    Tcl_InvalidateStringRep(valuePtr);
		}
		TRACE_WITH_OBJ(("%s => ", O2S(valuePtr)), valuePtr);
	    }
	    ADJUST_PC(1);
	    
	case INST_UMINUS:
	case INST_LNOT:
	    {
		/*
		 * The operand must be numeric. If the operand object is
		 * unshared modify it directly, otherwise create a copy to
		 * modify: this is "copy on write". free any old string
		 * representation since it is now invalid.
		 */
		
		double d;
		Tcl_ObjType *tPtr;
		
		valuePtr = POP_OBJECT();
		tPtr = valuePtr->typePtr;
		if ((tPtr != &tclIntType) && ((tPtr != &tclDoubleType)
			|| (valuePtr->bytes != NULL))) {
		    char *s = Tcl_GetStringFromObj(valuePtr, &length);
		    if (TclLooksLikeInt(s, length)) {
			result = Tcl_GetLongFromObj((Tcl_Interp *) NULL,
				valuePtr, &i);
		    } else {
			result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL,
				valuePtr, &d);
		    }
		    if (result != TCL_OK) {
			TRACE(("\"%.20s\" => ILLEGAL TYPE %s\n",
			        s, (tPtr? tPtr->name : "null")));
			IllegalExprOperandType(interp, pc, valuePtr);
			Tcl_DecrRefCount(valuePtr);
			goto checkForCatch;
		    }
		    tPtr = valuePtr->typePtr;
		}
		
		if (Tcl_IsShared(valuePtr)) {
		    /*
		     * Create a new object.
		     */
		    if (tPtr == &tclIntType) {
			i = valuePtr->internalRep.longValue;
			objPtr = Tcl_NewLongObj(
			        (*pc == INST_UMINUS)? -i : !i);
			TRACE_WITH_OBJ(("%ld => ", i), objPtr);
		    } else {
			d = valuePtr->internalRep.doubleValue;
			if (*pc == INST_UMINUS) {
			    objPtr = Tcl_NewDoubleObj(-d);
			} else {
			    /*
			     * Should be able to use "!d", but apparently
			     * some compilers can't handle it.
			     */
			    objPtr = Tcl_NewLongObj((d==0.0)? 1 : 0);
			}
			TRACE_WITH_OBJ(("%.6g => ", d), objPtr);
		    }
		    PUSH_OBJECT(objPtr);
		    TclDecrRefCount(valuePtr);
		} else {
		    /*
		     * valuePtr is unshared. Modify it directly.
		     */
		    if (tPtr == &tclIntType) {
			i = valuePtr->internalRep.longValue;
			Tcl_SetLongObj(valuePtr,
			        (*pc == INST_UMINUS)? -i : !i);
			TRACE_WITH_OBJ(("%ld => ", i), valuePtr);
		    } else {
			d = valuePtr->internalRep.doubleValue;
			if (*pc == INST_UMINUS) {
			    Tcl_SetDoubleObj(valuePtr, -d);
			} else {
			    /*
			     * Should be able to use "!d", but apparently
			     * some compilers can't handle it.
			     */
			    Tcl_SetLongObj(valuePtr, (d==0.0)? 1 : 0);
			}
			TRACE_WITH_OBJ(("%.6g => ", d), valuePtr);
		    }
		    ++stackTop; /* valuePtr now on stk top has right r.c. */
		}
	    }
	    ADJUST_PC(1);
	    
	case INST_BITNOT:
	    {
		/*
		 * The operand must be an integer. If the operand object is
		 * unshared modify it directly, otherwise modify a copy. 
		 * Free any old string representation since it is now
		 * invalid.
		 */
		
		Tcl_ObjType *tPtr;
		
		valuePtr = POP_OBJECT();
		tPtr = valuePtr->typePtr;
		if (tPtr != &tclIntType) {
		    result = Tcl_GetLongFromObj((Tcl_Interp *) NULL,
			    valuePtr, &i);
		    if (result != TCL_OK) {   /* try to convert to double */
			TRACE(("\"%.20s\" => ILLEGAL TYPE %s\n",
			       O2S(valuePtr), (tPtr? tPtr->name : "null")));
			IllegalExprOperandType(interp, pc, valuePtr);
			Tcl_DecrRefCount(valuePtr);
			goto checkForCatch;
		    }
		}
		
		i = valuePtr->internalRep.longValue;
		if (Tcl_IsShared(valuePtr)) {
		    PUSH_OBJECT(Tcl_NewLongObj(~i));
		    TRACE(("0x%lx => (%lu)\n", i, ~i));
		    TclDecrRefCount(valuePtr);
		} else {
		    /*
		     * valuePtr is unshared. Modify it directly.
		     */
		    Tcl_SetLongObj(valuePtr, ~i);
		    ++stackTop; /* valuePtr now on stk top has right r.c. */
		    TRACE(("0x%lx => (%lu)\n", i, ~i));
		}
	    }
	    ADJUST_PC(1);
	    
	case INST_CALL_BUILTIN_FUNC1:
	    opnd = TclGetUInt1AtPtr(pc+1);
	    {
		/*
		 * Call one of the built-in Tcl math functions.
		 */

		BuiltinFunc *mathFuncPtr;
		ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

		if ((opnd < 0) || (opnd > LAST_BUILTIN_FUNC)) {
		    TRACE(("UNRECOGNIZED BUILTIN FUNC CODE %d\n", opnd));
		    panic("TclExecuteByteCode: unrecognized builtin function code %d", opnd);
		}
		mathFuncPtr = &(builtinFuncTable[opnd]);
		DECACHE_STACK_INFO();
		tsdPtr->mathInProgress++;
		result = (*mathFuncPtr->proc)(interp, eePtr,
		        mathFuncPtr->clientData);
		tsdPtr->mathInProgress--;
		CACHE_STACK_INFO();
		if (result != TCL_OK) {
		    goto checkForCatch;
		}
		TRACE_WITH_OBJ(("%d => ", opnd), stackPtr[stackTop]);
	    }
	    ADJUST_PC(2);
		    
	case INST_CALL_FUNC1:
	    opnd = TclGetUInt1AtPtr(pc+1);
	    {
		/*
		 * Call a non-builtin Tcl math function previously
		 * registered by a call to Tcl_CreateMathFunc.
		 */
		
		int objc = opnd;   /* Number of arguments. The function name
				    * is the 0-th argument. */
		Tcl_Obj **objv;	   /* The array of arguments. The function
				    * name is objv[0]. */
		ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

		objv = &(stackPtr[stackTop - (objc-1)]); /* "objv[0]" */
		DECACHE_STACK_INFO();
		tsdPtr->mathInProgress++;
		result = ExprCallMathFunc(interp, eePtr, objc, objv);
		tsdPtr->mathInProgress--;
		CACHE_STACK_INFO();
		if (result != TCL_OK) {
		    goto checkForCatch;
		}
		TRACE_WITH_OBJ(("%d => ", objc), stackPtr[stackTop]);
		ADJUST_PC(2);
	    }

	case INST_TRY_CVT_TO_NUMERIC:
	    {
		/*
		 * Try to convert the topmost stack object to an int or
		 * double object. This is done in order to support Tcl's
		 * policy of interpreting operands if at all possible as
		 * first integers, else floating-point numbers.
		 */
		
		double d;
		char *s;
		Tcl_ObjType *tPtr;
		int converted, shared;

		valuePtr = stackPtr[stackTop];
		tPtr = valuePtr->typePtr;
		converted = 0;
		if ((tPtr != &tclIntType) && ((tPtr != &tclDoubleType)
			|| (valuePtr->bytes != NULL))) {
		    s = Tcl_GetStringFromObj(valuePtr, &length);
		    if (TclLooksLikeInt(s, length)) {
			result = Tcl_GetLongFromObj((Tcl_Interp *) NULL,
				valuePtr, &i);
		    } else {
			result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL,
				valuePtr, &d);
		    }
		    if (result == TCL_OK) {
			converted = 1;
		    }
		    result = TCL_OK; /* reset the result variable */
		    tPtr = valuePtr->typePtr;
		}

		/*
		 * Ensure that the topmost stack object, if numeric, has a
		 * string rep the same as the formatted version of its
		 * internal rep. This is used, e.g., to make sure that "expr
		 * {0001}" yields "1", not "0001". We implement this by
		 * _discarding_ the string rep since we know it will be
		 * regenerated, if needed later, by formatting the internal
		 * rep's value. Also check if there has been an IEEE
		 * floating point error.
		 */

		if ((tPtr == &tclIntType) || (tPtr == &tclDoubleType)) {
		    shared = 0;
		    if (Tcl_IsShared(valuePtr)) {
			shared = 1;
			if (tPtr == &tclIntType) {
			    i = valuePtr->internalRep.longValue;
			    objPtr = Tcl_NewLongObj(i);
			} else {
			    d = valuePtr->internalRep.doubleValue;
			    objPtr = Tcl_NewDoubleObj(d);
			}
			Tcl_IncrRefCount(objPtr);
			TclDecrRefCount(valuePtr);
			valuePtr = objPtr;
			stackPtr[stackTop] = valuePtr;
			tPtr = valuePtr->typePtr;
		    } else {
			Tcl_InvalidateStringRep(valuePtr);
		    }
		
		    if (tPtr == &tclDoubleType) {
			d = valuePtr->internalRep.doubleValue;
			if (IS_NAN(d) || IS_INF(d)) {
			    TRACE(("\"%.20s\" => IEEE FLOATING PT ERROR\n",
			           O2S(valuePtr)));
			    TclExprFloatError(interp, d);
			    result = TCL_ERROR;
			    goto checkForCatch;
			}
		    }
		    shared = shared;        /* lint, shared not used. */
		    converted = converted;  /* lint, converted not used. */
		    TRACE(("\"%.20s\" => numeric, %s, %s\n", O2S(valuePtr),
			   (converted? "converted" : "not converted"),
			   (shared? "shared" : "not shared")));
		} else {
		    TRACE(("\"%.20s\" => not numeric\n", O2S(valuePtr)));
		}
	    }
	    ADJUST_PC(1);

	case INST_BREAK:
	    /*
	     * First reset the interpreter's result. Then find the closest
	     * enclosing loop or catch exception range, if any. If a loop is
	     * found, terminate its execution. If the closest is a catch
	     * exception range, jump to its catchOffset. If no enclosing
	     * range is found, stop execution and return TCL_BREAK.
	     */

	    Tcl_ResetResult(interp);
	    rangePtr = GetExceptRangeForPc(pc, /*catchOnly*/ 0, codePtr);
	    if (rangePtr == NULL) {
		TRACE(("=> no encl. loop or catch, returning TCL_BREAK\n"));
		result = TCL_BREAK;
		goto abnormalReturn; /* no catch exists to check */
	    }
	    switch (rangePtr->type) {
	    case LOOP_EXCEPTION_RANGE:
		result = TCL_OK;
		TRACE(("=> range at %d, new pc %d\n",
		       rangePtr->codeOffset, rangePtr->breakOffset));
		break;
	    case CATCH_EXCEPTION_RANGE:
		result = TCL_BREAK;
		TRACE(("=> ...\n"));
		goto processCatch; /* it will use rangePtr */
	    default:
		panic("TclExecuteByteCode: unrecognized ExceptionRange type %d\n", rangePtr->type);
	    }
	    pc = (codePtr->codeStart + rangePtr->breakOffset);
	    continue;	/* restart outer instruction loop at pc */

	case INST_CONTINUE:
            /*
	     * Find the closest enclosing loop or catch exception range,
	     * if any. If a loop is found, skip to its next iteration.
	     * If the closest is a catch exception range, jump to its
	     * catchOffset. If no enclosing range is found, stop
	     * execution and return TCL_CONTINUE.
	     */

	    Tcl_ResetResult(interp);
	    rangePtr = GetExceptRangeForPc(pc, /*catchOnly*/ 0, codePtr);
	    if (rangePtr == NULL) {
		TRACE(("=> no encl. loop or catch, returning TCL_CONTINUE\n"));
		result = TCL_CONTINUE;
		goto abnormalReturn;
	    }
	    switch (rangePtr->type) {
	    case LOOP_EXCEPTION_RANGE:
		if (rangePtr->continueOffset == -1) {
		    TRACE(("=> loop w/o continue, checking for catch\n"));
		    goto checkForCatch;
		} else {
		    result = TCL_OK;
		    TRACE(("=> range at %d, new pc %d\n",
			   rangePtr->codeOffset, rangePtr->continueOffset));
		}
		break;
	    case CATCH_EXCEPTION_RANGE:
		result = TCL_CONTINUE;
		TRACE(("=> ...\n"));
		goto processCatch; /* it will use rangePtr */
	    default:
		panic("TclExecuteByteCode: unrecognized ExceptionRange type %d\n", rangePtr->type);
	    }
	    pc = (codePtr->codeStart + rangePtr->continueOffset);
	    continue;	/* restart outer instruction loop at pc */

	case INST_FOREACH_START4:
	    opnd = TclGetUInt4AtPtr(pc+1);
	    {
	        /*
		 * Initialize the temporary local var that holds the count
		 * of the number of iterations of the loop body to -1.
		 */

		ForeachInfo *infoPtr = (ForeachInfo *)
		    codePtr->auxDataArrayPtr[opnd].clientData;
		int iterTmpIndex = infoPtr->loopCtTemp;
		Var *compiledLocals = iPtr->varFramePtr->compiledLocals;
		Var *iterVarPtr = &(compiledLocals[iterTmpIndex]);
		Tcl_Obj *oldValuePtr = iterVarPtr->value.objPtr;

		if (oldValuePtr == NULL) {
		    iterVarPtr->value.objPtr = Tcl_NewLongObj(-1);
		    Tcl_IncrRefCount(iterVarPtr->value.objPtr);
		} else {
		    Tcl_SetLongObj(oldValuePtr, -1);
		}
		TclSetVarScalar(iterVarPtr);
		TclClearVarUndefined(iterVarPtr);
		TRACE(("%u => loop iter count temp %d\n", 
		        opnd, iterTmpIndex));
	    }
	    ADJUST_PC(5);
	
	case INST_FOREACH_STEP4:
	    opnd = TclGetUInt4AtPtr(pc+1);
	    {
	        /*
		 * "Step" a foreach loop (i.e., begin its next iteration) by
		 * assigning the next value list element to each loop var.
		 */

		ForeachInfo *infoPtr = (ForeachInfo *)
		        codePtr->auxDataArrayPtr[opnd].clientData;
		ForeachVarList *varListPtr;
		int numLists = infoPtr->numLists;
		Var *compiledLocals = iPtr->varFramePtr->compiledLocals;
		Tcl_Obj *listPtr;
		List *listRepPtr;
		Var *iterVarPtr, *listVarPtr;
		int iterNum, listTmpIndex, listLen, numVars;
		int varIndex, valIndex, continueLoop, j;

		/*
		 * Increment the temp holding the loop iteration number.
		 */

		iterVarPtr = &(compiledLocals[infoPtr->loopCtTemp]);
		valuePtr = iterVarPtr->value.objPtr;
		iterNum = (valuePtr->internalRep.longValue + 1);
		Tcl_SetLongObj(valuePtr, iterNum);
		
		/*
		 * Check whether all value lists are exhausted and we should
		 * stop the loop.
		 */

		continueLoop = 0;
		listTmpIndex = infoPtr->firstValueTemp;
		for (i = 0;  i < numLists;  i++) {
		    varListPtr = infoPtr->varLists[i];
		    numVars = varListPtr->numVars;
		    
		    listVarPtr = &(compiledLocals[listTmpIndex]);
		    listPtr = listVarPtr->value.objPtr;
		    result = Tcl_ListObjLength(interp, listPtr, &listLen);
		    if (result != TCL_OK) {
			TRACE_WITH_OBJ(("%u => ERROR converting list %ld, \"%s\": ",
			        opnd, i, O2S(listPtr)),
				Tcl_GetObjResult(interp));
			goto checkForCatch;
		    }
		    if (listLen > (iterNum * numVars)) {
			continueLoop = 1;
		    }
		    listTmpIndex++;
		}

		/*
		 * If some var in some var list still has a remaining list
		 * element iterate one more time. Assign to var the next
		 * element from its value list. We already checked above
		 * that each list temp holds a valid list object.
		 */
		
		if (continueLoop) {
		    listTmpIndex = infoPtr->firstValueTemp;
		    for (i = 0;  i < numLists;  i++) {
			varListPtr = infoPtr->varLists[i];
			numVars = varListPtr->numVars;

			listVarPtr = &(compiledLocals[listTmpIndex]);
			listPtr = listVarPtr->value.objPtr;
			listRepPtr = (List *) listPtr->internalRep.otherValuePtr;
			listLen = listRepPtr->elemCount;
			
			valIndex = (iterNum * numVars);
			for (j = 0;  j < numVars;  j++) {
			    int setEmptyStr = 0;
			    if (valIndex >= listLen) {
				setEmptyStr = 1;
				valuePtr = Tcl_NewObj();
			    } else {
				valuePtr = listRepPtr->elements[valIndex];
			    }
			    
			    varIndex = varListPtr->varIndexes[j];
			    DECACHE_STACK_INFO();
			    value2Ptr = TclSetIndexedScalar(interp,
			           varIndex, valuePtr, /*leaveErrorMsg*/ 1);
			    CACHE_STACK_INFO();
			    if (value2Ptr == NULL) {
				TRACE_WITH_OBJ(("%u => ERROR init. index temp %d: ",
				       opnd, varIndex),
				       Tcl_GetObjResult(interp));
				if (setEmptyStr) {
				    Tcl_DecrRefCount(valuePtr);
				}
				result = TCL_ERROR;
				goto checkForCatch;
			    }
			    valIndex++;
			}
			listTmpIndex++;
		    }
		}
		
		/*
		 * Push 1 if at least one value list had a remaining element
		 * and the loop should continue. Otherwise push 0.
		 */

		PUSH_OBJECT(Tcl_NewLongObj(continueLoop));
		TRACE(("%u => %d lists, iter %d, %s loop\n", 
		        opnd, numLists, iterNum,
		        (continueLoop? "continue" : "exit")));
	    }
	    ADJUST_PC(5);

	case INST_BEGIN_CATCH4:
	    /*
	     * Record start of the catch command with exception range index
	     * equal to the operand. Push the current stack depth onto the
	     * special catch stack.
	     */
	    catchStackPtr[++catchTop] = stackTop;
	    TRACE(("%u => catchTop=%d, stackTop=%d\n",
		    TclGetUInt4AtPtr(pc+1), catchTop, stackTop));
	    ADJUST_PC(5);

	case INST_END_CATCH:
	    catchTop--;
	    result = TCL_OK;
	    TRACE(("=> catchTop=%d\n", catchTop));
	    ADJUST_PC(1);

	case INST_PUSH_RESULT:
	    PUSH_OBJECT(Tcl_GetObjResult(interp));
	    TRACE_WITH_OBJ(("=> "), Tcl_GetObjResult(interp));
	    ADJUST_PC(1);

	case INST_PUSH_RETURN_CODE:
	    PUSH_OBJECT(Tcl_NewLongObj(result));
	    TRACE(("=> %u\n", result));
	    ADJUST_PC(1);

	default:
	    panic("TclExecuteByteCode: unrecognized opCode %u", *pc);
	} /* end of switch on opCode */

	/*
	 * Division by zero in an expression. Control only reaches this
	 * point by "goto divideByZero".
	 */
	
        divideByZero:
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp), "divide by zero", -1);
	Tcl_SetErrorCode(interp, "ARITH", "DIVZERO", "divide by zero",
			 (char *) NULL);
	result = TCL_ERROR;
	
	/*
	 * Execution has generated an "exception" such as TCL_ERROR. If the
	 * exception is an error, record information about what was being
	 * executed when the error occurred. Find the closest enclosing
	 * catch range, if any. If no enclosing catch range is found, stop
	 * execution and return the "exception" code.
	 */
	
        checkForCatch:
	if ((result == TCL_ERROR) && !(iPtr->flags & ERR_ALREADY_LOGGED)) {
	    bytes = GetSrcInfoForPc(pc, codePtr, &length);
	    if (bytes != NULL) {
		Tcl_LogCommandInfo(interp, codePtr->source, bytes, length);
		iPtr->flags |= ERR_ALREADY_LOGGED;
	    }
        }
	rangePtr = GetExceptRangeForPc(pc, /*catchOnly*/ 1, codePtr);
	if (rangePtr == NULL) {
#ifdef TCL_COMPILE_DEBUG
	    if (traceInstructions) {
		fprintf(stdout, "   ... no enclosing catch, returning %s\n",
		        StringForResultCode(result));
	    }
#endif
	    goto abnormalReturn;
	}

	/*
	 * A catch exception range (rangePtr) was found to handle an
	 * "exception". It was found either by checkForCatch just above or
	 * by an instruction during break, continue, or error processing.
	 * Jump to its catchOffset after unwinding the operand stack to
	 * the depth it had when starting to execute the range's catch
	 * command.
	 */

        processCatch:
	while (stackTop > catchStackPtr[catchTop]) {
	    valuePtr = POP_OBJECT();
	    TclDecrRefCount(valuePtr);
	}
#ifdef TCL_COMPILE_DEBUG
	if (traceInstructions) {
	    fprintf(stdout, "  ... found catch at %d, catchTop=%d, unwound to %d, new pc %u\n",
	        rangePtr->codeOffset, catchTop, catchStackPtr[catchTop],
	        (unsigned int)(rangePtr->catchOffset));
	}
#endif	
	pc = (codePtr->codeStart + rangePtr->catchOffset);
	continue;		/* restart the execution loop at pc */
    } /* end of infinite loop dispatching on instructions */

    /*
     * Abnormal return code. Restore the stack to state it had when starting
     * to execute the ByteCode.
     */

    abnormalReturn:
    while (stackTop > initStackTop) {
	valuePtr = POP_OBJECT();
	Tcl_DecrRefCount(valuePtr);
    }

    /*
     * Free the catch stack array if malloc'ed storage was used.
     */

    done:
    if (catchStackPtr != catchStackStorage) {
	ckfree((char *) catchStackPtr);
    }
    eePtr->stackTop = initStackTop;
    return result;
#undef STATIC_CATCH_STACK_SIZE
}

#ifdef TCL_COMPILE_DEBUG
/*
 *----------------------------------------------------------------------
 *
 * PrintByteCodeInfo --
 *
 *	This procedure prints a summary about a bytecode object to stdout.
 *	It is called by TclExecuteByteCode when starting to execute the
 *	bytecode object if tclTraceExec has the value 2 or more.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
PrintByteCodeInfo(codePtr)
    register ByteCode *codePtr;	/* The bytecode whose summary is printed
				 * to stdout. */
{
    Proc *procPtr = codePtr->procPtr;
    Interp *iPtr = (Interp *) *codePtr->interpHandle;

    fprintf(stdout, "\nExecuting ByteCode 0x%x, refCt %u, epoch %u, interp 0x%x (epoch %u)\n",
	    (unsigned int) codePtr, codePtr->refCount,
	    codePtr->compileEpoch, (unsigned int) iPtr,
	    iPtr->compileEpoch);
    
    fprintf(stdout, "  Source: ");
    TclPrintSource(stdout, codePtr->source, 60);

    fprintf(stdout, "\n  Cmds %d, src %d, inst %u, litObjs %u, aux %d, stkDepth %u, code/src %.2f\n",
            codePtr->numCommands, codePtr->numSrcBytes,
	    codePtr->numCodeBytes, codePtr->numLitObjects,
	    codePtr->numAuxDataItems, codePtr->maxStackDepth,
#ifdef TCL_COMPILE_STATS
	    (codePtr->numSrcBytes?
	            ((float)codePtr->structureSize)/((float)codePtr->numSrcBytes) : 0.0));
#else
	    0.0);
#endif
#ifdef TCL_COMPILE_STATS
    fprintf(stdout, "  Code %d = header %d+inst %d+litObj %d+exc %d+aux %d+cmdMap %d\n",
	    codePtr->structureSize,
	    (sizeof(ByteCode) - (sizeof(size_t) + sizeof(Tcl_Time))),
	    codePtr->numCodeBytes,
	    (codePtr->numLitObjects * sizeof(Tcl_Obj *)),
	    (codePtr->numExceptRanges * sizeof(ExceptionRange)),
	    (codePtr->numAuxDataItems * sizeof(AuxData)),
	    codePtr->numCmdLocBytes);
#endif /* TCL_COMPILE_STATS */
    if (procPtr != NULL) {
	fprintf(stdout,
		"  Proc 0x%x, refCt %d, args %d, compiled locals %d\n",
		(unsigned int) procPtr, procPtr->refCount,
		procPtr->numArgs, procPtr->numCompiledLocals);
    }
}
#endif /* TCL_COMPILE_DEBUG */

/*
 *----------------------------------------------------------------------
 *
 * ValidatePcAndStackTop --
 *
 *	This procedure is called by TclExecuteByteCode when debugging to
 *	verify that the program counter and stack top are valid during
 *	execution.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints a message to stderr and panics if either the pc or stack
 *	top are invalid.
 *
 *----------------------------------------------------------------------
 */

#ifdef TCL_COMPILE_DEBUG
static void
ValidatePcAndStackTop(codePtr, pc, stackTop, stackLowerBound,
        stackUpperBound)
    register ByteCode *codePtr; /* The bytecode whose summary is printed
				 * to stdout. */
    unsigned char *pc;		/* Points to first byte of a bytecode
				 * instruction. The program counter. */
    int stackTop;		/* Current stack top. Must be between
				 * stackLowerBound and stackUpperBound
				 * (inclusive). */
    int stackLowerBound;	/* Smallest legal value for stackTop. */
    int stackUpperBound;	/* Greatest legal value for stackTop. */
{
    unsigned int relativePc = (unsigned int) (pc - codePtr->codeStart);
    unsigned int codeStart = (unsigned int) codePtr->codeStart;
    unsigned int codeEnd = (unsigned int)
	    (codePtr->codeStart + codePtr->numCodeBytes);
    unsigned char opCode = *pc;

    if (((unsigned int) pc < codeStart) || ((unsigned int) pc > codeEnd)) {
	fprintf(stderr, "\nBad instruction pc 0x%x in TclExecuteByteCode\n",
		(unsigned int) pc);
	panic("TclExecuteByteCode execution failure: bad pc");
    }
    if ((unsigned int) opCode > LAST_INST_OPCODE) {
	fprintf(stderr, "\nBad opcode %d at pc %u in TclExecuteByteCode\n",
		(unsigned int) opCode, relativePc);
	panic("TclExecuteByteCode execution failure: bad opcode");
    }
    if ((stackTop < stackLowerBound) || (stackTop > stackUpperBound)) {
	int numChars;
	char *cmd = GetSrcInfoForPc(pc, codePtr, &numChars);
	char *ellipsis = "";
	
	fprintf(stderr, "\nBad stack top %d at pc %u in TclExecuteByteCode",
		stackTop, relativePc);
	if (cmd != NULL) {
	    if (numChars > 100) {
		numChars = 100;
		ellipsis = "...";
	    }
	    fprintf(stderr, "\n executing %.*s%s\n", numChars, cmd,
		    ellipsis);
	} else {
	    fprintf(stderr, "\n");
	}
	panic("TclExecuteByteCode execution failure: bad stack top");
    }
}
#endif /* TCL_COMPILE_DEBUG */

/*
 *----------------------------------------------------------------------
 *
 * IllegalExprOperandType --
 *
 *	Used by TclExecuteByteCode to add an error message to errorInfo
 *	when an illegal operand type is detected by an expression
 *	instruction. The argument opndPtr holds the operand object in error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An error message is appended to errorInfo.
 *
 *----------------------------------------------------------------------
 */

static void
IllegalExprOperandType(interp, pc, opndPtr)
    Tcl_Interp *interp;		/* Interpreter to which error information
				 * pertains. */
    unsigned char *pc;		/* Points to the instruction being executed
				 * when the illegal type was found. */
    Tcl_Obj *opndPtr;		/* Points to the operand holding the value
				 * with the illegal type. */
{
    unsigned char opCode = *pc;
    int isDouble;
    
    Tcl_ResetResult(interp);
    if ((opndPtr->bytes == NULL) || (opndPtr->length == 0)) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"can't use empty string as operand of \"",
		operatorStrings[opCode - INST_LOR], "\"", (char *) NULL);
    } else {
	isDouble = 1;
	if (opndPtr->typePtr != &tclDoubleType) {
	    /*
	     * See if the operand can be interpreted as a double in order to
	     * improve the error message.
	     */

	    char *s = Tcl_GetString(opndPtr);
	    double d;

	    if (Tcl_GetDouble((Tcl_Interp *) NULL, s, &d) != TCL_OK) {
		isDouble = 0;
	    }
	}
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), "can't use ",
		(isDouble? "floating-point value" : "non-numeric string"),
		" as operand of \"", operatorStrings[opCode - INST_LOR],
		"\"", (char *) NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CallTraceProcedure --
 *
 *	Invokes a trace procedure registered with an interpreter. These
 *	procedures trace command execution. Currently this trace procedure
 *	is called with the address of the string-based Tcl_CmdProc for the
 *	command, not the Tcl_ObjCmdProc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Those side effects made by the trace procedure.
 *
 *----------------------------------------------------------------------
 */

static void
CallTraceProcedure(interp, tracePtr, cmdPtr, command, numChars, objc, objv)
    Tcl_Interp *interp;		/* The current interpreter. */
    register Trace *tracePtr;	/* Describes the trace procedure to call. */
    Command *cmdPtr;		/* Points to command's Command struct. */
    char *command;		/* Points to the first character of the
				 * command's source before substitutions. */
    int numChars;		/* The number of characters in the
				 * command's source. */
    register int objc;		/* Number of arguments for the command. */
    Tcl_Obj *objv[];		/* Pointers to Tcl_Obj of each argument. */
{
    Interp *iPtr = (Interp *) interp;
    register char **argv;
    register int i;
    int length;
    char *p;

    /*
     * Get the string rep from the objv argument objects and place their
     * pointers in argv. First make sure argv is large enough to hold the
     * objc args plus 1 extra word for the zero end-of-argv word.
     */
    
    argv = (char **) ckalloc((unsigned)(objc + 1) * sizeof(char *));
    for (i = 0;  i < objc;  i++) {
	argv[i] = Tcl_GetStringFromObj(objv[i], &length);
    }
    argv[objc] = 0;

    /*
     * Copy the command characters into a new string.
     */

    p = (char *) ckalloc((unsigned) (numChars + 1));
    memcpy((VOID *) p, (VOID *) command, (size_t) numChars);
    p[numChars] = '\0';
    
    /*
     * Call the trace procedure then free allocated storage.
     */
    
    (*tracePtr->proc)(tracePtr->clientData, interp, iPtr->numLevels,
                      p, cmdPtr->proc, cmdPtr->clientData, objc, argv);

    ckfree((char *) argv);
    ckfree((char *) p);
}

/*
 *----------------------------------------------------------------------
 *
 * GetSrcInfoForPc --
 *
 *	Given a program counter value, finds the closest command in the
 *	bytecode code unit's CmdLocation array and returns information about
 *	that command's source: a pointer to its first byte and the number of
 *	characters.
 *
 * Results:
 *	If a command is found that encloses the program counter value, a
 *	pointer to the command's source is returned and the length of the
 *	source is stored at *lengthPtr. If multiple commands resulted in
 *	code at pc, information about the closest enclosing command is
 *	returned. If no matching command is found, NULL is returned and
 *	*lengthPtr is unchanged.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
GetSrcInfoForPc(pc, codePtr, lengthPtr)
    unsigned char *pc;		/* The program counter value for which to
				 * return the closest command's source info.
				 * This points to a bytecode instruction
				 * in codePtr's code. */
    ByteCode *codePtr;		/* The bytecode sequence in which to look
				 * up the command source for the pc. */
    int *lengthPtr;		/* If non-NULL, the location where the
				 * length of the command's source should be
				 * stored. If NULL, no length is stored. */
{
    register int pcOffset = (pc - codePtr->codeStart);
    int numCmds = codePtr->numCommands;
    unsigned char *codeDeltaNext, *codeLengthNext;
    unsigned char *srcDeltaNext, *srcLengthNext;
    int codeOffset, codeLen, codeEnd, srcOffset, srcLen, delta, i;
    int bestDist = INT_MAX;	/* Distance of pc to best cmd's start pc. */
    int bestSrcOffset = -1;	/* Initialized to avoid compiler warning. */
    int bestSrcLength = -1;	/* Initialized to avoid compiler warning. */

    if ((pcOffset < 0) || (pcOffset >= codePtr->numCodeBytes)) {
	return NULL;
    }

    /*
     * Decode the code and source offset and length for each command. The
     * closest enclosing command is the last one whose code started before
     * pcOffset.
     */

    codeDeltaNext = codePtr->codeDeltaStart;
    codeLengthNext = codePtr->codeLengthStart;
    srcDeltaNext  = codePtr->srcDeltaStart;
    srcLengthNext = codePtr->srcLengthStart;
    codeOffset = srcOffset = 0;
    for (i = 0;  i < numCmds;  i++) {
	if ((unsigned int) (*codeDeltaNext) == (unsigned int) 0xFF) {
	    codeDeltaNext++;
	    delta = TclGetInt4AtPtr(codeDeltaNext);
	    codeDeltaNext += 4;
	} else {
	    delta = TclGetInt1AtPtr(codeDeltaNext);
	    codeDeltaNext++;
	}
	codeOffset += delta;

	if ((unsigned int) (*codeLengthNext) == (unsigned int) 0xFF) {
	    codeLengthNext++;
	    codeLen = TclGetInt4AtPtr(codeLengthNext);
	    codeLengthNext += 4;
	} else {
	    codeLen = TclGetInt1AtPtr(codeLengthNext);
	    codeLengthNext++;
	}
	codeEnd = (codeOffset + codeLen - 1);

	if ((unsigned int) (*srcDeltaNext) == (unsigned int) 0xFF) {
	    srcDeltaNext++;
	    delta = TclGetInt4AtPtr(srcDeltaNext);
	    srcDeltaNext += 4;
	} else {
	    delta = TclGetInt1AtPtr(srcDeltaNext);
	    srcDeltaNext++;
	}
	srcOffset += delta;

	if ((unsigned int) (*srcLengthNext) == (unsigned int) 0xFF) {
	    srcLengthNext++;
	    srcLen = TclGetInt4AtPtr(srcLengthNext);
	    srcLengthNext += 4;
	} else {
	    srcLen = TclGetInt1AtPtr(srcLengthNext);
	    srcLengthNext++;
	}
	
	if (codeOffset > pcOffset) {      /* best cmd already found */
	    break;
	} else if (pcOffset <= codeEnd) { /* this cmd's code encloses pc */
	    int dist = (pcOffset - codeOffset);
	    if (dist <= bestDist) {
		bestDist = dist;
		bestSrcOffset = srcOffset;
		bestSrcLength = srcLen;
	    }
	}
    }

    if (bestDist == INT_MAX) {
	return NULL;
    }
    
    if (lengthPtr != NULL) {
	*lengthPtr = bestSrcLength;
    }
    return (codePtr->source + bestSrcOffset);
}

/*
 *----------------------------------------------------------------------
 *
 * GetExceptRangeForPc --
 *
 *	Given a program counter value, return the closest enclosing
 *	ExceptionRange.
 *
 * Results:
 *	In the normal case, catchOnly is 0 (false) and this procedure
 *	returns a pointer to the most closely enclosing ExceptionRange
 *	structure regardless of whether it is a loop or catch exception
 *	range. This is appropriate when processing a TCL_BREAK or
 *	TCL_CONTINUE, which will be "handled" either by a loop exception
 *	range or a closer catch range. If catchOnly is nonzero, this
 *	procedure ignores loop exception ranges and returns a pointer to the
 *	closest catch range. If no matching ExceptionRange is found that
 *	encloses pc, a NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ExceptionRange *
GetExceptRangeForPc(pc, catchOnly, codePtr)
    unsigned char *pc;		/* The program counter value for which to
				 * search for a closest enclosing exception
				 * range. This points to a bytecode
				 * instruction in codePtr's code. */
    int catchOnly;		/* If 0, consider either loop or catch
				 * ExceptionRanges in search. If nonzero
				 * consider only catch ranges (and ignore
				 * any closer loop ranges). */
    ByteCode* codePtr;		/* Points to the ByteCode in which to search
				 * for the enclosing ExceptionRange. */
{
    ExceptionRange *rangeArrayPtr;
    int numRanges = codePtr->numExceptRanges;
    register ExceptionRange *rangePtr;
    int pcOffset = (pc - codePtr->codeStart);
    register int i, level;

    if (numRanges == 0) {
	return NULL;
    }
    rangeArrayPtr = codePtr->exceptArrayPtr;

    for (level = codePtr->maxExceptDepth;  level >= 0;  level--) {
	for (i = 0;  i < numRanges;  i++) {
	    rangePtr = &(rangeArrayPtr[i]);
	    if (rangePtr->nestingLevel == level) {
		int start = rangePtr->codeOffset;
		int end   = (start + rangePtr->numCodeBytes);
		if ((start <= pcOffset) && (pcOffset < end)) {
		    if ((!catchOnly)
			    || (rangePtr->type == CATCH_EXCEPTION_RANGE)) {
			return rangePtr;
		    }
		}
	    }
	}
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * GetOpcodeName --
 *
 *	This procedure is called by the TRACE and TRACE_WITH_OBJ macros
 *	used in TclExecuteByteCode when debugging. It returns the name of
 *	the bytecode instruction at a specified instruction pc.
 *
 * Results:
 *	A character string for the instruction.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef TCL_COMPILE_DEBUG
static char *
GetOpcodeName(pc)
    unsigned char *pc;		/* Points to the instruction whose name
				 * should be returned. */
{
    unsigned char opCode = *pc;
    
    return instructionTable[opCode].name;
}
#endif /* TCL_COMPILE_DEBUG */

/*
 *----------------------------------------------------------------------
 *
 * Math Functions --
 *
 *	This page contains the procedures that implement all of the
 *	built-in math functions for expressions.
 *
 * Results:
 *	Each procedure returns TCL_OK if it succeeds and pushes an
 *	Tcl object holding the result. If it fails it returns TCL_ERROR
 *	and leaves an error message in the interpreter's result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ExprUnaryFunc(interp, eePtr, clientData)
    Tcl_Interp *interp;		/* The interpreter in which to execute the
				 * function. */
    ExecEnv *eePtr;		/* Points to the environment for executing
				 * the function. */
    ClientData clientData;	/* Contains the address of a procedure that
				 * takes one double argument and returns a
				 * double result. */
{
    Tcl_Obj **stackPtr;		/* Cached evaluation stack base pointer. */
    register int stackTop;	/* Cached top index of evaluation stack. */
    register Tcl_Obj *valuePtr;
    Tcl_ObjType *tPtr;
    double d, dResult;
    long i;
    int length, result;
    
    double (*func) _ANSI_ARGS_((double)) =
	(double (*)_ANSI_ARGS_((double))) clientData;

    /*
     * Set stackPtr and stackTop from eePtr.
     */

    result = TCL_OK;
    CACHE_STACK_INFO();

    /*
     * Pop the function's argument from the evaluation stack. Convert it
     * to a double if necessary.
     */

    valuePtr = POP_OBJECT();
    tPtr = valuePtr->typePtr;
    
    if (tPtr == &tclIntType) {
	d = (double) valuePtr->internalRep.longValue;
    } else if (tPtr == &tclDoubleType) {
	d = valuePtr->internalRep.doubleValue;
    } else {
	char *s = Tcl_GetStringFromObj(valuePtr, &length);
	
	if (TclLooksLikeInt(s, length)) {
	    result = Tcl_GetLongFromObj((Tcl_Interp *) NULL, valuePtr, &i);
	    d = (double) valuePtr->internalRep.longValue;
	} else {
	    result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL, valuePtr, &d);
	}
	if (result != TCL_OK) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp),
	            "argument to math function didn't have numeric value", -1);
	    goto done;
	}
    }

    errno = 0;
    dResult = (*func)(d);
    if ((errno != 0) || IS_NAN(dResult) || IS_INF(dResult)) {
	TclExprFloatError(interp, dResult);
	result = TCL_ERROR;
	goto done;
    }
    
    /*
     * Push a Tcl object holding the result.
     */

    PUSH_OBJECT(Tcl_NewDoubleObj(dResult));
    
    /*
     * Reflect the change to stackTop back in eePtr.
     */

    done:
    Tcl_DecrRefCount(valuePtr);
    DECACHE_STACK_INFO();
    return result;
}

static int
ExprBinaryFunc(interp, eePtr, clientData)
    Tcl_Interp *interp;		/* The interpreter in which to execute the
				 * function. */
    ExecEnv *eePtr;		/* Points to the environment for executing
				 * the function. */
    ClientData clientData;	/* Contains the address of a procedure that
				 * takes two double arguments and
				 * returns a double result. */
{
    Tcl_Obj **stackPtr;        /* Cached evaluation stack base pointer. */
    register int stackTop;	/* Cached top index of evaluation stack. */
    register Tcl_Obj *valuePtr, *value2Ptr;
    Tcl_ObjType *tPtr;
    double d1, d2, dResult;
    long i;
    char *s;
    int length, result;
    
    double (*func) _ANSI_ARGS_((double, double))
	= (double (*)_ANSI_ARGS_((double, double))) clientData;

    /*
     * Set stackPtr and stackTop from eePtr.
     */

    result = TCL_OK;
    CACHE_STACK_INFO();

    /*
     * Pop the function's two arguments from the evaluation stack. Convert
     * them to doubles if necessary.
     */

    value2Ptr = POP_OBJECT();
    valuePtr  = POP_OBJECT();

    tPtr = valuePtr->typePtr;
    if (tPtr == &tclIntType) {
	d1 = (double) valuePtr->internalRep.longValue;
    } else if (tPtr == &tclDoubleType) {
	d1 = valuePtr->internalRep.doubleValue;
    } else {
	s = Tcl_GetStringFromObj(valuePtr, &length);
	if (TclLooksLikeInt(s, length)) {
	    result = Tcl_GetLongFromObj((Tcl_Interp *) NULL, valuePtr, &i);
	    d1 = (double) valuePtr->internalRep.longValue;
	} else {
	    result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL, valuePtr, &d1);
	}
	if (result != TCL_OK) {
            badArg:
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp),
	            "argument to math function didn't have numeric value", -1);
	    goto done;
	}
    }

    tPtr = value2Ptr->typePtr;
    if (tPtr == &tclIntType) {
	d2 = value2Ptr->internalRep.longValue;
    } else if (tPtr == &tclDoubleType) {
	d2 = value2Ptr->internalRep.doubleValue;
    } else {
	s = Tcl_GetStringFromObj(value2Ptr, &length);
	if (TclLooksLikeInt(s, length)) {
	    result = Tcl_GetLongFromObj((Tcl_Interp *) NULL, value2Ptr, &i);
	    d2 = (double) value2Ptr->internalRep.longValue;
	} else {
	    result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL, value2Ptr, &d2);
	}
	if (result != TCL_OK) {
	    goto badArg;
	}
    }

    errno = 0;
    dResult = (*func)(d1, d2);
    if ((errno != 0) || IS_NAN(dResult) || IS_INF(dResult)) {
	TclExprFloatError(interp, dResult);
	result = TCL_ERROR;
	goto done;
    }

    /*
     * Push a Tcl object holding the result.
     */

    PUSH_OBJECT(Tcl_NewDoubleObj(dResult));
    
    /*
     * Reflect the change to stackTop back in eePtr.
     */

    done:
    Tcl_DecrRefCount(valuePtr);
    Tcl_DecrRefCount(value2Ptr);
    DECACHE_STACK_INFO();
    return result;
}

static int
ExprAbsFunc(interp, eePtr, clientData)
    Tcl_Interp *interp;		/* The interpreter in which to execute the
				 * function. */
    ExecEnv *eePtr;		/* Points to the environment for executing
				 * the function. */
    ClientData clientData;	/* Ignored. */
{
    Tcl_Obj **stackPtr;        /* Cached evaluation stack base pointer. */
    register int stackTop;	/* Cached top index of evaluation stack. */
    register Tcl_Obj *valuePtr;
    Tcl_ObjType *tPtr;
    long i, iResult;
    double d, dResult;
    int length, result;

    /*
     * Set stackPtr and stackTop from eePtr.
     */

    result = TCL_OK;
    CACHE_STACK_INFO();

    /*
     * Pop the argument from the evaluation stack.
     */

    valuePtr = POP_OBJECT();
    tPtr = valuePtr->typePtr;
    
    if (tPtr == &tclIntType) {
	i = valuePtr->internalRep.longValue;
    } else if (tPtr == &tclDoubleType) {
	d = valuePtr->internalRep.doubleValue;
    } else {
	char *s = Tcl_GetStringFromObj(valuePtr, &length);
	
	if (TclLooksLikeInt(s, length)) {
	    result = Tcl_GetLongFromObj((Tcl_Interp *) NULL, valuePtr, &i);
	} else {
	    result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL, valuePtr, &d);
	}
	if (result != TCL_OK) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp),
	            "argument to math function didn't have numeric value", -1);
	    goto done;
	}
	tPtr = valuePtr->typePtr;
    }

    /*
     * Push a Tcl object with the result.
     */
    
    if (tPtr == &tclIntType) {
	if (i < 0) {
	    iResult = -i;
	    if (iResult < 0) {
		Tcl_ResetResult(interp);
		Tcl_AppendToObj(Tcl_GetObjResult(interp),
		        "integer value too large to represent", -1);
		Tcl_SetErrorCode(interp, "ARITH", "IOVERFLOW",
			"integer value too large to represent", (char *) NULL);
		result = TCL_ERROR;
		goto done;
	    }
	} else {
	    iResult = i;
	}	    
	PUSH_OBJECT(Tcl_NewLongObj(iResult));
    } else {
	if (d < 0.0) {
	    dResult = -d;
	} else {
	    dResult = d;
	}
	if (IS_NAN(dResult) || IS_INF(dResult)) {
	    TclExprFloatError(interp, dResult);
	    result = TCL_ERROR;
	    goto done;
	}
	PUSH_OBJECT(Tcl_NewDoubleObj(dResult));
    }
    
    /*
     * Reflect the change to stackTop back in eePtr.
     */

    done:
    Tcl_DecrRefCount(valuePtr);
    DECACHE_STACK_INFO();
    return result;
}

static int
ExprDoubleFunc(interp, eePtr, clientData)
    Tcl_Interp *interp;		/* The interpreter in which to execute the
				 * function. */
    ExecEnv *eePtr;		/* Points to the environment for executing
				 * the function. */
    ClientData clientData;	/* Ignored. */
{
    Tcl_Obj **stackPtr;        /* Cached evaluation stack base pointer. */
    register int stackTop;	/* Cached top index of evaluation stack. */
    register Tcl_Obj *valuePtr;
    double dResult;
    long i;
    int length, result;

    /*
     * Set stackPtr and stackTop from eePtr.
     */

    result = TCL_OK;
    CACHE_STACK_INFO();

    /*
     * Pop the argument from the evaluation stack.
     */

    valuePtr = POP_OBJECT();
    if (valuePtr->typePtr == &tclIntType) {
	dResult = (double) valuePtr->internalRep.longValue;
    } else if (valuePtr->typePtr == &tclDoubleType) {
	dResult = valuePtr->internalRep.doubleValue;
    } else {
	char *s = Tcl_GetStringFromObj(valuePtr, &length);
	
	if (TclLooksLikeInt(s, length)) {
	    result = Tcl_GetLongFromObj((Tcl_Interp *) NULL, valuePtr, &i);
	    dResult = (double) valuePtr->internalRep.longValue;
	} else {
	    result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL, valuePtr,
		    &dResult);
	}
	if (result != TCL_OK) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp),
		    "argument to math function didn't have numeric value", -1);
	    goto done;
	}
    }

    /*
     * Push a Tcl object with the result.
     */

    PUSH_OBJECT(Tcl_NewDoubleObj(dResult));

    /*
     * Reflect the change to stackTop back in eePtr.
     */

    done:
    Tcl_DecrRefCount(valuePtr);
    DECACHE_STACK_INFO();
    return result;
}

static int
ExprIntFunc(interp, eePtr, clientData)
    Tcl_Interp *interp;		/* The interpreter in which to execute the
				 * function. */
    ExecEnv *eePtr;		/* Points to the environment for executing
				 * the function. */
    ClientData clientData;	/* Ignored. */
{
    Tcl_Obj **stackPtr;        /* Cached evaluation stack base pointer. */
    register int stackTop;	/* Cached top index of evaluation stack. */
    register Tcl_Obj *valuePtr;
    Tcl_ObjType *tPtr;
    long i = 0;			/* Initialized to avoid compiler warning. */
    long iResult;
    double d;
    int length, result;

    /*
     * Set stackPtr and stackTop from eePtr.
     */

    result = TCL_OK;
    CACHE_STACK_INFO();

    /*
     * Pop the argument from the evaluation stack.
     */

    valuePtr = POP_OBJECT();
    tPtr = valuePtr->typePtr;
    
    if (tPtr == &tclIntType) {
	i = valuePtr->internalRep.longValue;
    } else if (tPtr == &tclDoubleType) {
	d = valuePtr->internalRep.doubleValue;
    } else {
	char *s = Tcl_GetStringFromObj(valuePtr, &length);
	
	if (TclLooksLikeInt(s, length)) {
	    result = Tcl_GetLongFromObj((Tcl_Interp *) NULL, valuePtr, &i);
	} else {
	    result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL, valuePtr, &d);
	}
	if (result != TCL_OK) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp),
		    "argument to math function didn't have numeric value", -1);
	    goto done;
	}
	tPtr = valuePtr->typePtr;
    }

    /*
     * Push a Tcl object with the result.
     */
    
    if (tPtr == &tclIntType) {
	iResult = i;
    } else {
	if (d < 0.0) {
	    if (d < (double) (long) LONG_MIN) {
		tooLarge:
		Tcl_ResetResult(interp);
		Tcl_AppendToObj(Tcl_GetObjResult(interp),
		        "integer value too large to represent", -1);
		Tcl_SetErrorCode(interp, "ARITH", "IOVERFLOW",
			"integer value too large to represent", (char *) NULL);
		result = TCL_ERROR;
		goto done;
	    }
	} else {
	    if (d > (double) LONG_MAX) {
		goto tooLarge;
	    }
	}
	if (IS_NAN(d) || IS_INF(d)) {
	    TclExprFloatError(interp, d);
	    result = TCL_ERROR;
	    goto done;
	}
	iResult = (long) d;
    }
    PUSH_OBJECT(Tcl_NewLongObj(iResult));

    /*
     * Reflect the change to stackTop back in eePtr.
     */

    done:
    Tcl_DecrRefCount(valuePtr);
    DECACHE_STACK_INFO();
    return result;
}

static int
ExprRandFunc(interp, eePtr, clientData)
    Tcl_Interp *interp;		/* The interpreter in which to execute the
				 * function. */
    ExecEnv *eePtr;		/* Points to the environment for executing
				 * the function. */
    ClientData clientData;	/* Ignored. */
{
    Tcl_Obj **stackPtr;        /* Cached evaluation stack base pointer. */
    register int stackTop;	/* Cached top index of evaluation stack. */
    Interp *iPtr = (Interp *) interp;
    double dResult;
    int tmp;

    if (!(iPtr->flags & RAND_SEED_INITIALIZED)) {
	iPtr->flags |= RAND_SEED_INITIALIZED;
	iPtr->randSeed = TclpGetClicks();
    }
    
    /*
     * Set stackPtr and stackTop from eePtr.
     */
    
    CACHE_STACK_INFO();

    /*
     * Generate the random number using the linear congruential
     * generator defined by the following recurrence:
     *		seed = ( IA * seed ) mod IM
     * where IA is 16807 and IM is (2^31) - 1.  In order to avoid
     * potential problems with integer overflow, the  code uses
     * additional constants IQ and IR such that
     *		IM = IA*IQ + IR
     * For details on how this algorithm works, refer to the following
     * papers: 
     *
     *	S.K. Park & K.W. Miller, "Random number generators: good ones
     *	are hard to find," Comm ACM 31(10):1192-1201, Oct 1988
     *
     *	W.H. Press & S.A. Teukolsky, "Portable random number
     *	generators," Computers in Physics 6(5):522-524, Sep/Oct 1992.
     */

#define RAND_IA		16807
#define RAND_IM		2147483647
#define RAND_IQ		127773
#define RAND_IR		2836
#define RAND_MASK	123459876

    if (iPtr->randSeed == 0) {
	/*
	 * Don't allow a 0 seed, since it breaks the generator.  Shift
	 * it to some other value.
	 */

	iPtr->randSeed = 123459876;
    }
    tmp = iPtr->randSeed/RAND_IQ;
    iPtr->randSeed = RAND_IA*(iPtr->randSeed - tmp*RAND_IQ) - RAND_IR*tmp;
    if (iPtr->randSeed < 0) {
	iPtr->randSeed += RAND_IM;
    }

    /*
     * On 64-bit architectures we need to mask off the upper bits to
     * ensure we only have a 32-bit range.  The constant has the
     * bizarre form below in order to make sure that it doesn't
     * get sign-extended (the rules for sign extension are very
     * concat, particularly on 64-bit machines).
     */

    iPtr->randSeed &= ((((unsigned long) 0xfffffff) << 4) | 0xf);
    dResult = iPtr->randSeed * (1.0/RAND_IM);

    /*
     * Push a Tcl object with the result.
     */

    PUSH_OBJECT(Tcl_NewDoubleObj(dResult));
    
    /*
     * Reflect the change to stackTop back in eePtr.
     */

    DECACHE_STACK_INFO();
    return TCL_OK;
}

static int
ExprRoundFunc(interp, eePtr, clientData)
    Tcl_Interp *interp;		/* The interpreter in which to execute the
				 * function. */
    ExecEnv *eePtr;		/* Points to the environment for executing
				 * the function. */
    ClientData clientData;	/* Ignored. */
{
    Tcl_Obj **stackPtr;        /* Cached evaluation stack base pointer. */
    register int stackTop;	/* Cached top index of evaluation stack. */
    Tcl_Obj *valuePtr;
    Tcl_ObjType *tPtr;
    long i = 0;			/* Initialized to avoid compiler warning. */
    long iResult;
    double d, temp;
    int length, result;

    /*
     * Set stackPtr and stackTop from eePtr.
     */

    result = TCL_OK;
    CACHE_STACK_INFO();

    /*
     * Pop the argument from the evaluation stack.
     */

    valuePtr = POP_OBJECT();
    tPtr = valuePtr->typePtr;
    
    if (tPtr == &tclIntType) {
	i = valuePtr->internalRep.longValue;
    } else if (tPtr == &tclDoubleType) {
	d = valuePtr->internalRep.doubleValue;
    } else {
	char *s = Tcl_GetStringFromObj(valuePtr, &length);
	
	if (TclLooksLikeInt(s, length)) {
	    result = Tcl_GetLongFromObj((Tcl_Interp *) NULL, valuePtr, &i);
	} else {
	    result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL, valuePtr, &d);
	}
	if (result != TCL_OK) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp),
		    "argument to math function didn't have numeric value", -1);
	    goto done;
	}
	tPtr = valuePtr->typePtr;
    }

    /*
     * Push a Tcl object with the result.
     */
    
    if (tPtr == &tclIntType) {
	iResult = i;
    } else {
	if (d < 0.0) {
	    if (d <= (((double) (long) LONG_MIN) - 0.5)) {
		tooLarge:
		Tcl_ResetResult(interp);
		Tcl_AppendToObj(Tcl_GetObjResult(interp),
		        "integer value too large to represent", -1);
		Tcl_SetErrorCode(interp, "ARITH", "IOVERFLOW",
			"integer value too large to represent",
			(char *) NULL);
		result = TCL_ERROR;
		goto done;
	    }
	    temp = (long) (d - 0.5);
	} else {
	    if (d >= (((double) LONG_MAX + 0.5))) {
		goto tooLarge;
	    }
	    temp = (long) (d + 0.5);
	}
	if (IS_NAN(temp) || IS_INF(temp)) {
	    TclExprFloatError(interp, temp);
	    result = TCL_ERROR;
	    goto done;
	}
	iResult = (long) temp;
    }
    PUSH_OBJECT(Tcl_NewLongObj(iResult));

    /*
     * Reflect the change to stackTop back in eePtr.
     */

    done:
    Tcl_DecrRefCount(valuePtr);
    DECACHE_STACK_INFO();
    return result;
}

static int
ExprSrandFunc(interp, eePtr, clientData)
    Tcl_Interp *interp;		/* The interpreter in which to execute the
				 * function. */
    ExecEnv *eePtr;		/* Points to the environment for executing
				 * the function. */
    ClientData clientData;	/* Ignored. */
{
    Tcl_Obj **stackPtr;        /* Cached evaluation stack base pointer. */
    register int stackTop;	/* Cached top index of evaluation stack. */
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj *valuePtr;
    Tcl_ObjType *tPtr;
    long i = 0;			/* Initialized to avoid compiler warning. */
    int isDouble, result;

    /*
     * Set stackPtr and stackTop from eePtr.
     */
    
    CACHE_STACK_INFO();

    /*
     * Pop the argument from the evaluation stack.  Use the value
     * to reset the random number seed.
     */

    valuePtr = POP_OBJECT();
    tPtr = valuePtr->typePtr;
    
    if (tPtr == &tclIntType) {
	i = valuePtr->internalRep.longValue;
    } else {
	result = Tcl_GetLongFromObj((Tcl_Interp *) NULL, valuePtr, &i);
	if (result != TCL_OK) {
	    /*
	     * See if the operand can be interpreted as a double in order to
	     * improve the error message.
	     */

	    isDouble = 1;
	    if (valuePtr->typePtr != &tclDoubleType) {
		char *s = Tcl_GetString(valuePtr);
		double d;

		if (Tcl_GetDouble((Tcl_Interp *) NULL, s, &d) != TCL_OK) {
		    isDouble = 0;
		}
	    }

	    Tcl_ResetResult(interp);
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), "can't use ",
		    (isDouble? "floating-point value":"non-numeric string"),
		    " as argument to srand", (char *) NULL);
	    Tcl_DecrRefCount(valuePtr);
	    DECACHE_STACK_INFO();
	    return result;
	}
    }
    
    /*
     * Reset the seed.
     */

    iPtr->flags |= RAND_SEED_INITIALIZED;
    iPtr->randSeed = i;

    /*
     * To avoid duplicating the random number generation code we simply
     * clean up our state and call the real random number function. That
     * function will always succeed.
     */
    
    Tcl_DecrRefCount(valuePtr);
    DECACHE_STACK_INFO();

    ExprRandFunc(interp, eePtr, clientData);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ExprCallMathFunc --
 *
 *	This procedure is invoked to call a non-builtin math function
 *	during the execution of an expression. 
 *
 * Results:
 *	TCL_OK is returned if all went well and the function's value
 *	was computed successfully. If an error occurred, TCL_ERROR
 *	is returned and an error message is left in the interpreter's
 *	result.	After a successful return this procedure pushes a Tcl object
 *	holding the result. 
 *
 * Side effects:
 *	None, unless the called math function has side effects.
 *
 *----------------------------------------------------------------------
 */

static int
ExprCallMathFunc(interp, eePtr, objc, objv)
    Tcl_Interp *interp;		/* The interpreter in which to execute the
				 * function. */
    ExecEnv *eePtr;		/* Points to the environment for executing
				 * the function. */
    int objc;			/* Number of arguments. The function name is
				 * the 0-th argument. */
    Tcl_Obj **objv;		/* The array of arguments. The function name
				 * is objv[0]. */
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Obj **stackPtr;		/* Cached evaluation stack base pointer. */
    register int stackTop;	/* Cached top index of evaluation stack. */
    char *funcName;
    Tcl_HashEntry *hPtr;
    MathFunc *mathFuncPtr;	/* Information about math function. */
    Tcl_Value args[MAX_MATH_ARGS]; /* Arguments for function call. */
    Tcl_Value funcResult;	/* Result of function call as Tcl_Value. */
    register Tcl_Obj *valuePtr;
    Tcl_ObjType *tPtr;
    long i;
    double d;
    int j, k, length, result;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    Tcl_ResetResult(interp);

    /*
     * Set stackPtr and stackTop from eePtr.
     */
    
    CACHE_STACK_INFO();

    /*
     * Look up the MathFunc record for the function.
     */

    funcName = Tcl_GetString(objv[0]);
    hPtr = Tcl_FindHashEntry(&iPtr->mathFuncTable, funcName);
    if (hPtr == NULL) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"unknown math function \"", funcName, "\"", (char *) NULL);
	result = TCL_ERROR;
	goto done;
    }
    mathFuncPtr = (MathFunc *) Tcl_GetHashValue(hPtr);
    if (mathFuncPtr->numArgs != (objc-1)) {
	panic("ExprCallMathFunc: expected number of args %d != actual number %d",
	        mathFuncPtr->numArgs, objc);
	result = TCL_ERROR;
	goto done;
    }

    /*
     * Collect the arguments for the function, if there are any, into the
     * array "args". Note that args[0] will have the Tcl_Value that
     * corresponds to objv[1].
     */

    for (j = 1, k = 0;  j < objc;  j++, k++) {
	valuePtr = objv[j];
	tPtr = valuePtr->typePtr;
	
	if (tPtr == &tclIntType) {
	    i = valuePtr->internalRep.longValue;
	} else if (tPtr == &tclDoubleType) {
	    d = valuePtr->internalRep.doubleValue;
	} else {
	    /*
	     * Try to convert to int first then double.
	     */
	    
	    char *s = Tcl_GetStringFromObj(valuePtr, &length);
	    
	    if (TclLooksLikeInt(s, length)) {
		result = Tcl_GetLongFromObj((Tcl_Interp *) NULL, valuePtr, &i);
	    } else {
		result = Tcl_GetDoubleFromObj((Tcl_Interp *) NULL,
			valuePtr, &d);
	    }
	    if (result != TCL_OK) {
		Tcl_AppendToObj(Tcl_GetObjResult(interp),
			"argument to math function didn't have numeric value", -1);
		goto done;
	    }
	    tPtr = valuePtr->typePtr;
	}

	/*
	 * Copy the object's numeric value to the argument record,
	 * converting it if necessary. 
	 */
	
	if (tPtr == &tclIntType) {
	    if (mathFuncPtr->argTypes[k] == TCL_DOUBLE) {
		args[k].type = TCL_DOUBLE;
		args[k].doubleValue = i;
	    } else {
		args[k].type = TCL_INT;
		args[k].intValue = i;
	    }
	} else {
	    if (mathFuncPtr->argTypes[k] == TCL_INT) {
		args[k].type = TCL_INT;
		args[k].intValue = (long) d;
	    } else {
		args[k].type = TCL_DOUBLE;
		args[k].doubleValue = d;
	    }
	}
    }

    /*
     * Invoke the function and copy its result back into valuePtr.
     */

    tsdPtr->mathInProgress++;
    result = (*mathFuncPtr->proc)(mathFuncPtr->clientData, interp, args,
	    &funcResult);
    tsdPtr->mathInProgress--;
    if (result != TCL_OK) {
	goto done;
    }

    /*
     * Pop the objc top stack elements and decrement their ref counts.
     */
		
    i = (stackTop - (objc-1));
    while (i <= stackTop) {
	valuePtr = stackPtr[i];
	Tcl_DecrRefCount(valuePtr);
	i++;
    }
    stackTop -= objc;
    
    /*
     * Push the call's object result.
     */
    
    if (funcResult.type == TCL_INT) {
	PUSH_OBJECT(Tcl_NewLongObj(funcResult.intValue));
    } else {
	d = funcResult.doubleValue;
	if (IS_NAN(d) || IS_INF(d)) {
	    TclExprFloatError(interp, d);
	    result = TCL_ERROR;
	    goto done;
	}
	PUSH_OBJECT(Tcl_NewDoubleObj(d));
    }

    /*
     * Reflect the change to stackTop back in eePtr.
     */

    done:
    DECACHE_STACK_INFO();
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclExprFloatError --
 *
 *	This procedure is called when an error occurs during a
 *	floating-point operation. It reads errno and sets
 *	interp->objResultPtr accordingly.
 *
 * Results:
 *	interp->objResultPtr is set to hold an error message.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TclExprFloatError(interp, value)
    Tcl_Interp *interp;		/* Where to store error message. */
    double value;		/* Value returned after error;  used to
				 * distinguish underflows from overflows. */
{
    char *s;

    Tcl_ResetResult(interp);
    if ((errno == EDOM) || (value != value)) {
	s = "domain error: argument not in valid range";
	Tcl_AppendToObj(Tcl_GetObjResult(interp), s, -1);
	Tcl_SetErrorCode(interp, "ARITH", "DOMAIN", s, (char *) NULL);
    } else if ((errno == ERANGE) || IS_INF(value)) {
	if (value == 0.0) {
	    s = "floating-point value too small to represent";
	    Tcl_AppendToObj(Tcl_GetObjResult(interp), s, -1);
	    Tcl_SetErrorCode(interp, "ARITH", "UNDERFLOW", s, (char *) NULL);
	} else {
	    s = "floating-point value too large to represent";
	    Tcl_AppendToObj(Tcl_GetObjResult(interp), s, -1);
	    Tcl_SetErrorCode(interp, "ARITH", "OVERFLOW", s, (char *) NULL);
	}
    } else {
	char msg[64 + TCL_INTEGER_SPACE];
	
	sprintf(msg, "unknown floating-point error, errno = %d", errno);
	Tcl_AppendToObj(Tcl_GetObjResult(interp), msg, -1);
	Tcl_SetErrorCode(interp, "ARITH", "UNKNOWN", msg, (char *) NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclMathInProgress --
 *
 *	This procedure is called to find out if Tcl is doing math
 *	in this thread.
 *
 * Results:
 *	0 or 1.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclMathInProgress()
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    return tsdPtr->mathInProgress;
}

#ifdef TCL_COMPILE_STATS
/*
 *----------------------------------------------------------------------
 *
 * TclLog2 --
 *
 *	Procedure used while collecting compilation statistics to determine
 *	the log base 2 of an integer.
 *
 * Results:
 *	Returns the log base 2 of the operand. If the argument is less
 *	than or equal to zero, a zero is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclLog2(value)
    register int value;		/* The integer for which to compute the
				 * log base 2. */
{
    register int n = value;
    register int result = 0;

    while (n > 1) {
	n = n >> 1;
	result++;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * EvalStatsCmd --
 *
 *	Implements the "evalstats" command that prints instruction execution
 *	counts to stdout.
 *
 * Results:
 *	Standard Tcl results.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
EvalStatsCmd(unused, interp, argc, argv)
    ClientData unused;		/* Unused. */
    Tcl_Interp *interp;		/* The current interpreter. */
    int argc;			/* The number of arguments. */
    char **argv;		/* The argument strings. */
{
    Interp *iPtr = (Interp *) interp;
    LiteralTable *globalTablePtr = &(iPtr->literalTable);
    ByteCodeStats *statsPtr = &(iPtr->stats);
    double totalCodeBytes, currentCodeBytes;
    double totalLiteralBytes, currentLiteralBytes;
    double objBytesIfUnshared, strBytesIfUnshared, sharingBytesSaved;
    double strBytesSharedMultX, strBytesSharedOnce;
    double numInstructions, currentHeaderBytes;
    long numCurrentByteCodes, numByteCodeLits;
    long refCountSum, literalMgmtBytes, sum;
    int numSharedMultX, numSharedOnce;
    int decadeHigh, minSizeDecade, maxSizeDecade, length, i;
    char *litTableStats;
    LiteralEntry *entryPtr;

    numInstructions = 0.0;
    for (i = 0;  i < 256;  i++) {
        if (statsPtr->instructionCount[i] != 0) {
            numInstructions += statsPtr->instructionCount[i];
        }
    }

    totalLiteralBytes = sizeof(LiteralTable)
	    + iPtr->literalTable.numBuckets * sizeof(LiteralEntry *)
	    + (statsPtr->numLiteralsCreated * sizeof(LiteralEntry))
	    + (statsPtr->numLiteralsCreated * sizeof(Tcl_Obj))
	    + statsPtr->totalLitStringBytes;
    totalCodeBytes = statsPtr->totalByteCodeBytes + totalLiteralBytes;

    numCurrentByteCodes =
	    statsPtr->numCompilations - statsPtr->numByteCodesFreed;
    currentHeaderBytes = numCurrentByteCodes
	    * (sizeof(ByteCode) - (sizeof(size_t) + sizeof(Tcl_Time)));
    literalMgmtBytes = sizeof(LiteralTable)
	    + (iPtr->literalTable.numBuckets * sizeof(LiteralEntry *))
	    + (iPtr->literalTable.numEntries * sizeof(LiteralEntry));
    currentLiteralBytes = literalMgmtBytes
	    + iPtr->literalTable.numEntries * sizeof(Tcl_Obj)
	    + statsPtr->currentLitStringBytes;
    currentCodeBytes = statsPtr->currentByteCodeBytes + currentLiteralBytes;
    
    /*
     * Summary statistics, total and current source and ByteCode sizes.
     */

    fprintf(stdout, "\n----------------------------------------------------------------\n");
    fprintf(stdout,
	    "Compilation and execution statistics for interpreter 0x%x\n",
	    (unsigned int) iPtr);

    fprintf(stdout, "\nNumber ByteCodes executed	%ld\n",
	    statsPtr->numExecutions);
    fprintf(stdout, "Number ByteCodes compiled	%ld\n",
	    statsPtr->numCompilations);
    fprintf(stdout, "  Mean executions/compile	%.1f\n",
	    ((float)statsPtr->numExecutions) / ((float)statsPtr->numCompilations));
    
    fprintf(stdout, "\nInstructions executed		%.0f\n",
	    numInstructions);
    fprintf(stdout, "  Mean inst/compile		%.0f\n",
	    numInstructions / statsPtr->numCompilations);
    fprintf(stdout, "  Mean inst/execution		%.0f\n",
	    numInstructions / statsPtr->numExecutions);

    fprintf(stdout, "\nTotal ByteCodes			%ld\n",
	    statsPtr->numCompilations);
    fprintf(stdout, "  Source bytes			%.6g\n",
	    statsPtr->totalSrcBytes);
    fprintf(stdout, "  Code bytes			%.6g\n",
	    totalCodeBytes);
    fprintf(stdout, "    ByteCode bytes		%.6g\n",
	    statsPtr->totalByteCodeBytes);
    fprintf(stdout, "    Literal bytes		%.6g\n",
	    totalLiteralBytes);
    fprintf(stdout, "      table %d + bkts %d + entries %ld + objects %ld + strings %.6g\n",
	    sizeof(LiteralTable),
	    iPtr->literalTable.numBuckets * sizeof(LiteralEntry *),
	    statsPtr->numLiteralsCreated * sizeof(LiteralEntry),
	    statsPtr->numLiteralsCreated * sizeof(Tcl_Obj),
	    statsPtr->totalLitStringBytes);
    fprintf(stdout, "  Mean code/compile		%.1f\n",
	    totalCodeBytes / statsPtr->numCompilations);
    fprintf(stdout, "  Mean code/source		%.1f\n",
	    totalCodeBytes / statsPtr->totalSrcBytes);

    fprintf(stdout, "\nCurrent ByteCodes		%ld\n",
	    numCurrentByteCodes);
    fprintf(stdout, "  Source bytes			%.6g\n",
	    statsPtr->currentSrcBytes);
    fprintf(stdout, "  Code bytes			%.6g\n",
	    currentCodeBytes);
    fprintf(stdout, "    ByteCode bytes		%.6g\n",
	    statsPtr->currentByteCodeBytes);
    fprintf(stdout, "    Literal bytes		%.6g\n",
	    currentLiteralBytes);
    fprintf(stdout, "      table %d + bkts %d + entries %d + objects %d + strings %.6g\n",
	    sizeof(LiteralTable),
	    iPtr->literalTable.numBuckets * sizeof(LiteralEntry *),
	    iPtr->literalTable.numEntries * sizeof(LiteralEntry),
	    iPtr->literalTable.numEntries * sizeof(Tcl_Obj),
	    statsPtr->currentLitStringBytes);
    fprintf(stdout, "  Mean code/source		%.1f\n",
	    currentCodeBytes / statsPtr->currentSrcBytes);
    fprintf(stdout, "  Code + source bytes		%.6g (%0.1f mean code/src)\n",
	    (currentCodeBytes + statsPtr->currentSrcBytes),
	    (currentCodeBytes / statsPtr->currentSrcBytes) + 1.0);

    /*
     * Literal table statistics.
     */

    numByteCodeLits = 0;
    refCountSum = 0;
    numSharedMultX = 0;
    numSharedOnce  = 0;
    objBytesIfUnshared  = 0.0;
    strBytesIfUnshared  = 0.0;
    strBytesSharedMultX = 0.0;
    strBytesSharedOnce  = 0.0;
    for (i = 0;  i < globalTablePtr->numBuckets;  i++) {
	for (entryPtr = globalTablePtr->buckets[i];  entryPtr != NULL;
	        entryPtr = entryPtr->nextPtr) {
	    if (entryPtr->objPtr->typePtr == &tclByteCodeType) {
		numByteCodeLits++;
	    }
	    (void) Tcl_GetStringFromObj(entryPtr->objPtr, &length);
	    refCountSum += entryPtr->refCount;
	    objBytesIfUnshared += (entryPtr->refCount * sizeof(Tcl_Obj));
	    strBytesIfUnshared += (entryPtr->refCount * (length+1));
	    if (entryPtr->refCount > 1) {
		numSharedMultX++;
		strBytesSharedMultX += (length+1);
	    } else {
		numSharedOnce++;
		strBytesSharedOnce += (length+1);
	    }
	}
    }
    sharingBytesSaved = (objBytesIfUnshared + strBytesIfUnshared)
	    - currentLiteralBytes;

    fprintf(stdout, "\nTotal objects (all interps)	%ld\n",
	    tclObjsAlloced);
    fprintf(stdout, "Current objects			%ld\n",
	    (tclObjsAlloced - tclObjsFreed));
    fprintf(stdout, "Total literal objects		%ld\n",
	    statsPtr->numLiteralsCreated);
    
    fprintf(stdout, "\nCurrent literal objects		%d (%0.1f%% of current objects)\n",
	    globalTablePtr->numEntries,
	    (globalTablePtr->numEntries * 100.0) / (tclObjsAlloced-tclObjsFreed));
    fprintf(stdout, "  ByteCode literals	 	%ld (%0.1f%% of current literals)\n",
	    numByteCodeLits,
	    (numByteCodeLits * 100.0) / globalTablePtr->numEntries);
    fprintf(stdout, "  Literals reused > 1x	 	%d\n",
	    numSharedMultX);
    fprintf(stdout, "  Mean reference count	 	%.2f\n",
	    ((double) refCountSum) / globalTablePtr->numEntries);
    fprintf(stdout, "  Mean len, str reused >1x 	%.2f\n",
	    (numSharedMultX? (strBytesSharedMultX/numSharedMultX) : 0.0));
    fprintf(stdout, "  Mean len, str used 1x	 	%.2f\n",
	    (numSharedOnce? (strBytesSharedOnce/numSharedOnce) : 0.0));
    fprintf(stdout, "  Total sharing savings	 	%.6g (%0.1f%% of bytes if no sharing)\n",
	    sharingBytesSaved,
	    (sharingBytesSaved * 100.0) / (objBytesIfUnshared + strBytesIfUnshared));
    fprintf(stdout, "    Bytes with sharing		%.6g\n",
	    currentLiteralBytes);
    fprintf(stdout, "      table %d + bkts %d + entries %d + objects %d + strings %.6g\n",
	    sizeof(LiteralTable),
	    iPtr->literalTable.numBuckets * sizeof(LiteralEntry *),
	    iPtr->literalTable.numEntries * sizeof(LiteralEntry),
	    iPtr->literalTable.numEntries * sizeof(Tcl_Obj),
	    statsPtr->currentLitStringBytes);
    fprintf(stdout, "    Bytes if no sharing		%.6g = objects %.6g + strings %.6g\n",
	    (objBytesIfUnshared + strBytesIfUnshared),
	    objBytesIfUnshared, strBytesIfUnshared);
    fprintf(stdout, "  String sharing savings 	%.6g = unshared %.6g - shared %.6g\n",
	    (strBytesIfUnshared - statsPtr->currentLitStringBytes),
	    strBytesIfUnshared, statsPtr->currentLitStringBytes);
    fprintf(stdout, "  Literal mgmt overhead	 	%ld (%0.1f%% of bytes with sharing)\n",
	    literalMgmtBytes,
	    (literalMgmtBytes * 100.0) / currentLiteralBytes);
    fprintf(stdout, "    table %d + buckets %d + entries %d\n",
	    sizeof(LiteralTable),
	    iPtr->literalTable.numBuckets * sizeof(LiteralEntry *),
	    iPtr->literalTable.numEntries * sizeof(LiteralEntry));

    /*
     * Breakdown of current ByteCode space requirements.
     */
    
    fprintf(stdout, "\nBreakdown of current ByteCode requirements:\n");
    fprintf(stdout, "                         Bytes      Pct of    Avg per\n");
    fprintf(stdout, "                                     total    ByteCode\n");
    fprintf(stdout, "Total             %12.6g     100.00%%   %8.1f\n",
	    statsPtr->currentByteCodeBytes,
	    statsPtr->currentByteCodeBytes / numCurrentByteCodes);
    fprintf(stdout, "Header            %12.6g   %8.1f%%   %8.1f\n",
	    currentHeaderBytes,
	    ((currentHeaderBytes * 100.0) / statsPtr->currentByteCodeBytes),
	    currentHeaderBytes / numCurrentByteCodes);
    fprintf(stdout, "Instructions      %12.6g   %8.1f%%   %8.1f\n",
	    statsPtr->currentInstBytes,
	    ((statsPtr->currentInstBytes * 100.0) / statsPtr->currentByteCodeBytes),
	    statsPtr->currentInstBytes / numCurrentByteCodes);
    fprintf(stdout, "Literal ptr array %12.6g   %8.1f%%   %8.1f\n",
	    statsPtr->currentLitBytes,
	    ((statsPtr->currentLitBytes * 100.0) / statsPtr->currentByteCodeBytes),
	    statsPtr->currentLitBytes / numCurrentByteCodes);
    fprintf(stdout, "Exception table   %12.6g   %8.1f%%   %8.1f\n",
	    statsPtr->currentExceptBytes,
	    ((statsPtr->currentExceptBytes * 100.0) / statsPtr->currentByteCodeBytes),
	    statsPtr->currentExceptBytes / numCurrentByteCodes);
    fprintf(stdout, "Auxiliary data    %12.6g   %8.1f%%   %8.1f\n",
	    statsPtr->currentAuxBytes,
	    ((statsPtr->currentAuxBytes * 100.0) / statsPtr->currentByteCodeBytes),
	    statsPtr->currentAuxBytes / numCurrentByteCodes);
    fprintf(stdout, "Command map       %12.6g   %8.1f%%   %8.1f\n",
	    statsPtr->currentCmdMapBytes,
	    ((statsPtr->currentCmdMapBytes * 100.0) / statsPtr->currentByteCodeBytes),
	    statsPtr->currentCmdMapBytes / numCurrentByteCodes);

    /*
     * Detailed literal statistics.
     */
    
    fprintf(stdout, "\nLiteral string sizes:\n");
    fprintf(stdout, "	 Up to length		Percentage\n");
    maxSizeDecade = 0;
    for (i = 31;  i >= 0;  i--) {
        if (statsPtr->literalCount[i] > 0) {
            maxSizeDecade = i;
	    break;
        }
    }
    sum = 0;
    for (i = 0;  i <= maxSizeDecade;  i++) {
	decadeHigh = (1 << (i+1)) - 1;
	sum += statsPtr->literalCount[i];
        fprintf(stdout,	"	%10d		%8.0f%%\n",
		decadeHigh, (sum * 100.0) / statsPtr->numLiteralsCreated);
    }

    litTableStats = TclLiteralStats(globalTablePtr);
    fprintf(stdout, "\nCurrent literal table statistics:\n%s\n",
            litTableStats);
    ckfree((char *) litTableStats);

    /*
     * Source and ByteCode size distributions.
     */

    fprintf(stdout, "\nSource sizes:\n");
    fprintf(stdout, "	 Up to size		Percentage\n");
    minSizeDecade = maxSizeDecade = 0;
    for (i = 0;  i < 31;  i++) {
        if (statsPtr->srcCount[i] > 0) {
	    minSizeDecade = i;
	    break;
        }
    }
    for (i = 31;  i >= 0;  i--) {
        if (statsPtr->srcCount[i] > 0) {
            maxSizeDecade = i;
	    break;
        }
    }
    sum = 0;
    for (i = minSizeDecade;  i <= maxSizeDecade;  i++) {
	decadeHigh = (1 << (i+1)) - 1;
	sum += statsPtr->srcCount[i];
        fprintf(stdout,	"	%10d		%8.0f%%\n",
		decadeHigh, (sum * 100.0) / statsPtr->numCompilations);
    }

    fprintf(stdout, "\nByteCode sizes:\n");
    fprintf(stdout, "	 Up to size		Percentage\n");
    minSizeDecade = maxSizeDecade = 0;
    for (i = 0;  i < 31;  i++) {
        if (statsPtr->byteCodeCount[i] > 0) {
	    minSizeDecade = i;
	    break;
        }
    }
    for (i = 31;  i >= 0;  i--) {
        if (statsPtr->byteCodeCount[i] > 0) {
            maxSizeDecade = i;
	    break;
        }
    }
    sum = 0;
    for (i = minSizeDecade;  i <= maxSizeDecade;  i++) {
	decadeHigh = (1 << (i+1)) - 1;
	sum += statsPtr->byteCodeCount[i];
        fprintf(stdout,	"	%10d		%8.0f%%\n",
		decadeHigh, (sum * 100.0) / statsPtr->numCompilations);
    }

    fprintf(stdout, "\nByteCode longevity (excludes current ByteCodes):\n");
    fprintf(stdout, "	       Up to ms		Percentage\n");
    minSizeDecade = maxSizeDecade = 0;
    for (i = 0;  i < 31;  i++) {
        if (statsPtr->lifetimeCount[i] > 0) {
	    minSizeDecade = i;
	    break;
        }
    }
    for (i = 31;  i >= 0;  i--) {
        if (statsPtr->lifetimeCount[i] > 0) {
            maxSizeDecade = i;
	    break;
        }
    }
    sum = 0;
    for (i = minSizeDecade;  i <= maxSizeDecade;  i++) {
	decadeHigh = (1 << (i+1)) - 1;
	sum += statsPtr->lifetimeCount[i];
        fprintf(stdout,	"	%12.3f		%8.0f%%\n",
		decadeHigh / 1000.0,
		(sum * 100.0) / statsPtr->numByteCodesFreed);
    }

    /*
     * Instruction counts.
     */

    fprintf(stdout, "\nInstruction counts:\n");
    for (i = 0;  i <= LAST_INST_OPCODE;  i++) {
        if (statsPtr->instructionCount[i]) {
            fprintf(stdout, "%20s %8ld %6.1f%%\n",
		    instructionTable[i].name,
		    statsPtr->instructionCount[i],
		    (statsPtr->instructionCount[i]*100.0) / numInstructions);
        }
    }

    fprintf(stdout, "\nInstructions NEVER executed:\n");
    for (i = 0;  i <= LAST_INST_OPCODE;  i++) {
        if (statsPtr->instructionCount[i] == 0) {
            fprintf(stdout, "%20s\n",
		    instructionTable[i].name);
        }
    }

#ifdef TCL_MEM_DEBUG
    fprintf(stdout, "\nHeap Statistics:\n");
    TclDumpMemoryInfo(stdout);
#endif
    fprintf(stdout, "\n----------------------------------------------------------------\n");
    return TCL_OK;
}
#endif /* TCL_COMPILE_STATS */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetCommandFromObj --
 *
 *      Returns the command specified by the name in a Tcl_Obj.
 *
 * Results:
 *	Returns a token for the command if it is found. Otherwise, if it
 *	can't be found or there is an error, returns NULL.
 *
 * Side effects:
 *      May update the internal representation for the object, caching
 *      the command reference so that the next time this procedure is
 *	called with the same object, the command can be found quickly.
 *
 *----------------------------------------------------------------------
 */

Tcl_Command
Tcl_GetCommandFromObj(interp, objPtr)
    Tcl_Interp *interp;		/* The interpreter in which to resolve the
				 * command and to report errors. */
    register Tcl_Obj *objPtr;	/* The object containing the command's
				 * name. If the name starts with "::", will
				 * be looked up in global namespace. Else,
				 * looked up first in the current namespace
				 * if contextNsPtr is NULL, then in global
				 * namespace. */
{
    Interp *iPtr = (Interp *) interp;
    register ResolvedCmdName *resPtr;
    register Command *cmdPtr;
    Namespace *currNsPtr;
    int result;

    /*
     * Get the internal representation, converting to a command type if
     * needed. The internal representation is a ResolvedCmdName that points
     * to the actual command.
     */
    
    if (objPtr->typePtr != &tclCmdNameType) {
        result = tclCmdNameType.setFromAnyProc(interp, objPtr);
        if (result != TCL_OK) {
            return (Tcl_Command) NULL;
        }
    }
    resPtr = (ResolvedCmdName *) objPtr->internalRep.otherValuePtr;

    /*
     * Get the current namespace.
     */
    
    if (iPtr->varFramePtr != NULL) {
	currNsPtr = iPtr->varFramePtr->nsPtr;
    } else {
	currNsPtr = iPtr->globalNsPtr;
    }

    /*
     * Check the context namespace and the namespace epoch of the resolved
     * symbol to make sure that it is fresh. If not, then force another
     * conversion to the command type, to discard the old rep and create a
     * new one. Note that we verify that the namespace id of the context
     * namespace is the same as the one we cached; this insures that the
     * namespace wasn't deleted and a new one created at the same address
     * with the same command epoch.
     */
    
    cmdPtr = NULL;
    if ((resPtr != NULL)
	    && (resPtr->refNsPtr == currNsPtr)
	    && (resPtr->refNsId == currNsPtr->nsId)
	    && (resPtr->refNsCmdEpoch == currNsPtr->cmdRefEpoch)) {
        cmdPtr = resPtr->cmdPtr;
        if (cmdPtr->cmdEpoch != resPtr->cmdEpoch) {
            cmdPtr = NULL;
        }
    }

    if (cmdPtr == NULL) {
        result = tclCmdNameType.setFromAnyProc(interp, objPtr);
        if (result != TCL_OK) {
            return (Tcl_Command) NULL;
        }
        resPtr = (ResolvedCmdName *) objPtr->internalRep.otherValuePtr;
        if (resPtr != NULL) {
            cmdPtr = resPtr->cmdPtr;
        }
    }
    return (Tcl_Command) cmdPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclSetCmdNameObj --
 *
 *	Modify an object to be an CmdName object that refers to the argument
 *	Command structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object's old internal rep is freed. It's string rep is not
 *	changed. The refcount in the Command structure is incremented to
 *	keep it from being freed if the command is later deleted until
 *	TclExecuteByteCode has a chance to recognize that it was deleted.
 *
 *----------------------------------------------------------------------
 */

void
TclSetCmdNameObj(interp, objPtr, cmdPtr)
    Tcl_Interp *interp;		/* Points to interpreter containing command
				 * that should be cached in objPtr. */
    register Tcl_Obj *objPtr;	/* Points to Tcl object to be changed to
				 * a CmdName object. */
    Command *cmdPtr;		/* Points to Command structure that the
				 * CmdName object should refer to. */
{
    Interp *iPtr = (Interp *) interp;
    register ResolvedCmdName *resPtr;
    Tcl_ObjType *oldTypePtr = objPtr->typePtr;
    register Namespace *currNsPtr;

    if (oldTypePtr == &tclCmdNameType) {
	return;
    }
    
    /*
     * Get the current namespace.
     */
    
    if (iPtr->varFramePtr != NULL) {
	currNsPtr = iPtr->varFramePtr->nsPtr;
    } else {
	currNsPtr = iPtr->globalNsPtr;
    }
    
    cmdPtr->refCount++;
    resPtr = (ResolvedCmdName *) ckalloc(sizeof(ResolvedCmdName));
    resPtr->cmdPtr = cmdPtr;
    resPtr->refNsPtr = currNsPtr;
    resPtr->refNsId  = currNsPtr->nsId;
    resPtr->refNsCmdEpoch = currNsPtr->cmdRefEpoch;
    resPtr->cmdEpoch = cmdPtr->cmdEpoch;
    resPtr->refCount = 1;
    
    if ((oldTypePtr != NULL) && (oldTypePtr->freeIntRepProc != NULL)) {
	oldTypePtr->freeIntRepProc(objPtr);
    }
    objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) resPtr;
    objPtr->internalRep.twoPtrValue.ptr2 = NULL;
    objPtr->typePtr = &tclCmdNameType;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeCmdNameInternalRep --
 *
 *	Frees the resources associated with a cmdName object's internal
 *	representation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Decrements the ref count of any cached ResolvedCmdName structure
 *	pointed to by the cmdName's internal representation. If this is 
 *	the last use of the ResolvedCmdName, it is freed. This in turn
 *	decrements the ref count of the Command structure pointed to by 
 *	the ResolvedSymbol, which may free the Command structure.
 *
 *----------------------------------------------------------------------
 */

static void
FreeCmdNameInternalRep(objPtr)
    register Tcl_Obj *objPtr;	/* CmdName object with internal
				 * representation to free. */
{
    register ResolvedCmdName *resPtr =
	(ResolvedCmdName *) objPtr->internalRep.otherValuePtr;

    if (resPtr != NULL) {
	/*
	 * Decrement the reference count of the ResolvedCmdName structure.
	 * If there are no more uses, free the ResolvedCmdName structure.
	 */
    
        resPtr->refCount--;
        if (resPtr->refCount == 0) {
            /*
	     * Now free the cached command, unless it is still in its
             * hash table or if there are other references to it
             * from other cmdName objects.
	     */
	    
            Command *cmdPtr = resPtr->cmdPtr;
            TclCleanupCommand(cmdPtr);
            ckfree((char *) resPtr);
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DupCmdNameInternalRep --
 *
 *	Initialize the internal representation of an cmdName Tcl_Obj to a
 *	copy of the internal representation of an existing cmdName object. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	"copyPtr"s internal rep is set to point to the ResolvedCmdName
 *	structure corresponding to "srcPtr"s internal rep. Increments the
 *	ref count of the ResolvedCmdName structure pointed to by the
 *	cmdName's internal representation.
 *
 *----------------------------------------------------------------------
 */

static void
DupCmdNameInternalRep(srcPtr, copyPtr)
    Tcl_Obj *srcPtr;		/* Object with internal rep to copy. */
    register Tcl_Obj *copyPtr;	/* Object with internal rep to set. */
{
    register ResolvedCmdName *resPtr =
        (ResolvedCmdName *) srcPtr->internalRep.otherValuePtr;

    copyPtr->internalRep.twoPtrValue.ptr1 = (VOID *) resPtr;
    copyPtr->internalRep.twoPtrValue.ptr2 = NULL;
    if (resPtr != NULL) {
        resPtr->refCount++;
    }
    copyPtr->typePtr = &tclCmdNameType;
}

/*
 *----------------------------------------------------------------------
 *
 * SetCmdNameFromAny --
 *
 *	Generate an cmdName internal form for the Tcl object "objPtr".
 *
 * Results:
 *	The return value is a standard Tcl result. The conversion always
 *	succeeds and TCL_OK is returned.
 *
 * Side effects:
 *	A pointer to a ResolvedCmdName structure that holds a cached pointer
 *	to the command with a name that matches objPtr's string rep is
 *	stored as objPtr's internal representation. This ResolvedCmdName
 *	pointer will be NULL if no matching command was found. The ref count
 *	of the cached Command's structure (if any) is also incremented.
 *
 *----------------------------------------------------------------------
 */

static int
SetCmdNameFromAny(interp, objPtr)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    register Tcl_Obj *objPtr;	/* The object to convert. */
{
    Interp *iPtr = (Interp *) interp;
    char *name;
    Tcl_Command cmd;
    register Command *cmdPtr;
    Namespace *currNsPtr;
    register ResolvedCmdName *resPtr;

    /*
     * Get "objPtr"s string representation. Make it up-to-date if necessary.
     */

    name = objPtr->bytes;
    if (name == NULL) {
	name = Tcl_GetString(objPtr);
    }

    /*
     * Find the Command structure, if any, that describes the command called
     * "name". Build a ResolvedCmdName that holds a cached pointer to this
     * Command, and bump the reference count in the referenced Command
     * structure. A Command structure will not be deleted as long as it is
     * referenced from a CmdName object.
     */

    cmd = Tcl_FindCommand(interp, name, (Tcl_Namespace *) NULL,
	    /*flags*/ 0);
    cmdPtr = (Command *) cmd;
    if (cmdPtr != NULL) {
	/*
	 * Get the current namespace.
	 */
	
	if (iPtr->varFramePtr != NULL) {
	    currNsPtr = iPtr->varFramePtr->nsPtr;
	} else {
	    currNsPtr = iPtr->globalNsPtr;
	}
	
	cmdPtr->refCount++;
        resPtr = (ResolvedCmdName *) ckalloc(sizeof(ResolvedCmdName));
        resPtr->cmdPtr        = cmdPtr;
        resPtr->refNsPtr      = currNsPtr;
        resPtr->refNsId       = currNsPtr->nsId;
        resPtr->refNsCmdEpoch = currNsPtr->cmdRefEpoch;
        resPtr->cmdEpoch      = cmdPtr->cmdEpoch;
        resPtr->refCount      = 1;
    } else {
	resPtr = NULL;	/* no command named "name" was found */
    }

    /*
     * Free the old internalRep before setting the new one. We do this as
     * late as possible to allow the conversion code, in particular
     * GetStringFromObj, to use that old internalRep. If no Command
     * structure was found, leave NULL as the cached value.
     */

    if ((objPtr->typePtr != NULL)
	    && (objPtr->typePtr->freeIntRepProc != NULL)) {
	objPtr->typePtr->freeIntRepProc(objPtr);
    }
    
    objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) resPtr;
    objPtr->internalRep.twoPtrValue.ptr2 = NULL;
    objPtr->typePtr = &tclCmdNameType;
    return TCL_OK;
}

#ifdef TCL_COMPILE_DEBUG
/*
 *----------------------------------------------------------------------
 *
 * StringForResultCode --
 *
 *	Procedure that returns a human-readable string representing a
 *	Tcl result code such as TCL_ERROR. 
 *
 * Results:
 *	If the result code is one of the standard Tcl return codes, the
 *	result is a string representing that code such as "TCL_ERROR".
 *	Otherwise, the result string is that code formatted as a
 *	sequence of decimal digit characters. Note that the resulting
 *	string must not be modified by the caller.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
StringForResultCode(result)
    int result;			/* The Tcl result code for which to
				 * generate a string. */
{
    static char buf[TCL_INTEGER_SPACE];
    
    if ((result >= TCL_OK) && (result <= TCL_CONTINUE)) {
	return resultStrings[result];
    }
    TclFormatInt(buf, result);
    return buf;
}
#endif /* TCL_COMPILE_DEBUG */
