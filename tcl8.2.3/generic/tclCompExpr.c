/* 
 * tclCompExpr.c --
 *
 *	This file contains the code to compile Tcl expressions.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclCompile.h"

/*
 * The stuff below is a bit of a hack so that this file can be used in
 * environments that include no UNIX, i.e. no errno: just arrange to use
 * the errno from tclExecute.c here.
 */

#ifndef TCL_GENERIC_ONLY
#include "tclPort.h"
#else
#define NO_ERRNO_H
#endif

#ifdef NO_ERRNO_H
extern int errno;			/* Use errno from tclExecute.c. */
#define ERANGE 34
#endif

/*
 * Boolean variable that controls whether expression compilation tracing
 * is enabled.
 */

#ifdef TCL_COMPILE_DEBUG
static int traceExprComp = 0;
#endif /* TCL_COMPILE_DEBUG */

/*
 * The ExprInfo structure describes the state of compiling an expression.
 * A pointer to an ExprInfo record is passed among the routines in
 * this module.
 */

typedef struct ExprInfo {
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Parse *parsePtr;	/* Structure filled with information about
				 * the parsed expression. */
    char *expr;			/* The expression that was originally passed
				 * to TclCompileExpr. */
    char *lastChar;		/* Points just after last byte of expr. */
    int hasOperators;		/* Set 1 if the expr has operators; 0 if
				 * expr is only a primary. If 1 after
				 * compiling an expr, a tryCvtToNumeric
				 * instruction is emitted to convert the
				 * primary to a number if possible. */
    int exprIsJustVarRef;	/* Set 1 if the expr consists of just a
				 * variable reference as in the expression
				 * of "if $b then...". Otherwise 0. If 1 the
				 * expr is compiled out-of-line in order to
				 * implement expr's 2 level substitution
				 * semantics properly. */
    int exprIsComparison;	/* Set 1 if the top-level operator in the
				 * expr is a comparison. Otherwise 0. If 1,
				 * because the operands might be strings,
				 * the expr is compiled out-of-line in order
				 * to implement expr's 2 level substitution
				 * semantics properly. */
} ExprInfo;

/*
 * Definitions of numeric codes representing each expression operator.
 * The order of these must match the entries in the operatorTable below.
 * Also the codes for the relational operators (OP_LESS, OP_GREATER, 
 * OP_LE, OP_GE, OP_EQ, and OP_NE) must be consecutive and in that order.
 * Note that OP_PLUS and OP_MINUS represent both unary and binary operators.
 */

#define OP_MULT		0
#define OP_DIVIDE	1
#define OP_MOD		2
#define OP_PLUS		3
#define OP_MINUS	4
#define OP_LSHIFT	5
#define OP_RSHIFT	6
#define OP_LESS		7
#define OP_GREATER	8
#define OP_LE		9
#define OP_GE		10
#define OP_EQ		11
#define OP_NEQ		12
#define OP_BITAND	13
#define OP_BITXOR	14
#define OP_BITOR	15
#define OP_LAND		16
#define OP_LOR		17
#define OP_QUESTY	18
#define OP_LNOT		19
#define OP_BITNOT	20

/*
 * Table describing the expression operators. Entries in this table must
 * correspond to the definitions of numeric codes for operators just above.
 */

static int opTableInitialized = 0; /* 0 means not yet initialized. */

TCL_DECLARE_MUTEX(opMutex)

typedef struct OperatorDesc {
    char *name;			/* Name of the operator. */
    int numOperands;		/* Number of operands. 0 if the operator
				 * requires special handling. */
    int instruction;		/* Instruction opcode for the operator.
				 * Ignored if numOperands is 0. */
} OperatorDesc;

OperatorDesc operatorTable[] = {
    {"*",   2,  INST_MULT},
    {"/",   2,  INST_DIV},
    {"%",   2,  INST_MOD},
    {"+",   0}, 
    {"-",   0},
    {"<<",  2,  INST_LSHIFT},
    {">>",  2,  INST_RSHIFT},
    {"<",   2,  INST_LT},
    {">",   2,  INST_GT},
    {"<=",  2,  INST_LE},
    {">=",  2,  INST_GE},
    {"==",  2,  INST_EQ},
    {"!=",  2,  INST_NEQ},
    {"&",   2,  INST_BITAND},
    {"^",   2,  INST_BITXOR},
    {"|",   2,  INST_BITOR},
    {"&&",  0},
    {"||",  0},
    {"?",   0},
    {"!",   1,  INST_LNOT},
    {"~",   1,  INST_BITNOT},
    {NULL}
};

/*
 * Hashtable used to map the names of expression operators to the index
 * of their OperatorDesc description.
 */

static Tcl_HashTable opHashTable;

/*
 * Declarations for local procedures to this file:
 */

static int		CompileCondExpr _ANSI_ARGS_((
			    Tcl_Token *exprTokenPtr, ExprInfo *infoPtr,
			    CompileEnv *envPtr, Tcl_Token **endPtrPtr));
static int		CompileLandOrLorExpr _ANSI_ARGS_((
			    Tcl_Token *exprTokenPtr, int opIndex,
			    ExprInfo *infoPtr, CompileEnv *envPtr,
			    Tcl_Token **endPtrPtr));
static int		CompileMathFuncCall _ANSI_ARGS_((
			    Tcl_Token *exprTokenPtr, char *funcName,
			    ExprInfo *infoPtr, CompileEnv *envPtr,
			    Tcl_Token **endPtrPtr));
static int		CompileSubExpr _ANSI_ARGS_((
			    Tcl_Token *exprTokenPtr, ExprInfo *infoPtr,
			    CompileEnv *envPtr));
static void		LogSyntaxError _ANSI_ARGS_((ExprInfo *infoPtr));

/*
 * Macro used to debug the execution of the expression compiler.
 */

#ifdef TCL_COMPILE_DEBUG
#define TRACE(exprBytes, exprLength, tokenBytes, tokenLength) \
    if (traceExprComp) { \
	fprintf(stderr, "CompileSubExpr: \"%.*s\", token \"%.*s\"\n", \
	        (exprLength), (exprBytes), (tokenLength), (tokenBytes)); \
    }
#else
#define TRACE(exprBytes, exprLength, tokenBytes, tokenLength)
#endif /* TCL_COMPILE_DEBUG */

/*
 *----------------------------------------------------------------------
 *
 * TclCompileExpr --
 *
 *	This procedure compiles a string containing a Tcl expression into
 *	Tcl bytecodes. This procedure is the top-level interface to the
 *	the expression compilation module, and is used by such public
 *	procedures as Tcl_ExprString, Tcl_ExprStringObj, Tcl_ExprLong,
 *	Tcl_ExprDouble, Tcl_ExprBoolean, and Tcl_ExprBooleanObj.
 *
 * Results:
 *	The return value is TCL_OK on a successful compilation and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the expression.
 *
 *	envPtr->exprIsJustVarRef is set 1 if the expression consisted of
 *	a single variable reference as in the expression of "if $b then...".
 *	Otherwise it is set 0. This is used to implement Tcl's two level
 *	expression substitution semantics properly.
 *
 *	envPtr->exprIsComparison is set 1 if the top-level operator in the
 *	expr is a comparison. Otherwise it is set 0. If 1, because the
 *	operands might be strings, the expr is compiled out-of-line in order
 *	to implement expr's 2 level substitution semantics properly.
 *
 * Side effects:
 *	Adds instructions to envPtr to evaluate the expression at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileExpr(interp, script, numBytes, envPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    char *script;		/* The source script to compile. */
    int numBytes;		/* Number of bytes in script. If < 0, the
				 * string consists of all bytes up to the
				 * first null character. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    ExprInfo info;
    Tcl_Parse parse;
    Tcl_HashEntry *hPtr;
    int maxDepth, new, i, code;

    /*
     * If this is the first time we've been called, initialize the table
     * of expression operators.
     */

    if (numBytes < 0) {
	numBytes = (script? strlen(script) : 0);
    }
    if (!opTableInitialized) {
	Tcl_MutexLock(&opMutex);
	if (!opTableInitialized) {
	    Tcl_InitHashTable(&opHashTable, TCL_STRING_KEYS);
	    for (i = 0;  operatorTable[i].name != NULL;  i++) {
		hPtr = Tcl_CreateHashEntry(&opHashTable,
			operatorTable[i].name, &new);
		if (new) {
		    Tcl_SetHashValue(hPtr, (ClientData) i);
		}
	    }
	    opTableInitialized = 1;
	}
	Tcl_MutexUnlock(&opMutex);
    }

    /*
     * Initialize the structure containing information abvout this
     * expression compilation.
     */

    info.interp = interp;
    info.parsePtr = &parse;
    info.expr = script;
    info.lastChar = (script + numBytes); 
    info.hasOperators = 0;
    info.exprIsJustVarRef = 1;	/* will be set 0 if anything else is seen */
    info.exprIsComparison = 0;

    /*
     * Parse the expression then compile it.
     */

    maxDepth = 0;
    code = Tcl_ParseExpr(interp, script, numBytes, &parse);
    if (code != TCL_OK) {
	goto done;
    }

    code = CompileSubExpr(parse.tokenPtr, &info, envPtr);
    if (code != TCL_OK) {
	Tcl_FreeParse(&parse);
	goto done;
    }
    maxDepth = envPtr->maxStackDepth;
    
    if (!info.hasOperators) {
	/*
	 * Attempt to convert the primary's object to an int or double.
	 * This is done in order to support Tcl's policy of interpreting
	 * operands if at all possible as first integers, else
	 * floating-point numbers.
	 */
	
	TclEmitOpcode(INST_TRY_CVT_TO_NUMERIC, envPtr);
    }
    Tcl_FreeParse(&parse);

    done:
    envPtr->maxStackDepth = maxDepth;
    envPtr->exprIsJustVarRef = info.exprIsJustVarRef;
    envPtr->exprIsComparison = info.exprIsComparison;
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeCompilation --
 *
 *	Clean up the compilation environment so it can later be
 *	properly reinitialized. This procedure is called by
 *	TclFinalizeCompExecEnv() in tclObj.c, which in turn is called
 *	by Tcl_Finalize().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cleans up the compilation environment. At the moment, just the
 *	table of expression operators is freed.
 *
 *----------------------------------------------------------------------
 */

void
TclFinalizeCompilation()
{
    Tcl_MutexLock(&opMutex);
    if (opTableInitialized) {
        Tcl_DeleteHashTable(&opHashTable);
        opTableInitialized = 0;
    }
    Tcl_MutexUnlock(&opMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * CompileSubExpr --
 *
 *	Given a pointer to a TCL_TOKEN_SUB_EXPR token describing a
 *	subexpression, this procedure emits instructions to evaluate the
 *	subexpression at runtime.
 *
 * Results:
 *	The return value is TCL_OK on a successful compilation and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the subexpression.
 *
 *	envPtr->exprIsJustVarRef is set 1 if the subexpression consisted of
 *	a single variable reference as in the expression of "if $b then...".
 *	Otherwise it is set 0. This is used to implement Tcl's two level
 *	expression substitution semantics properly.
 *
 *	envPtr->exprIsComparison is set 1 if the top-level operator in the
 *	subexpression is a comparison. Otherwise it is set 0. If 1, because
 *	the operands might be strings, the expr is compiled out-of-line in
 *	order to implement expr's 2 level substitution semantics properly.
 *
 * Side effects:
 *	Adds instructions to envPtr to evaluate the subexpression.
 *
 *----------------------------------------------------------------------
 */

static int
CompileSubExpr(exprTokenPtr, infoPtr, envPtr)
    Tcl_Token *exprTokenPtr;	/* Points to TCL_TOKEN_SUB_EXPR token
				 * to compile. */
    ExprInfo *infoPtr;		/* Describes the compilation state for the
				 * expression being compiled. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    Tcl_Interp *interp = infoPtr->interp;
    Tcl_Token *tokenPtr, *endPtr, *afterSubexprPtr;
    OperatorDesc *opDescPtr;
    Tcl_HashEntry *hPtr;
    char *operator;
    char savedChar;
    int maxDepth, objIndex, opIndex, length, code;
    char buffer[TCL_UTF_MAX];

    if (exprTokenPtr->type != TCL_TOKEN_SUB_EXPR) {
	panic("CompileSubExpr: token type %d not TCL_TOKEN_SUB_EXPR\n",
	        exprTokenPtr->type);
    }
    maxDepth = 0;
    code = TCL_OK;

    /*
     * Switch on the type of the first token after the subexpression token.
     * After processing it, advance tokenPtr to point just after the
     * subexpression's last token.
     */
    
    tokenPtr = exprTokenPtr+1;
    TRACE(exprTokenPtr->start, exprTokenPtr->size,
	    tokenPtr->start, tokenPtr->size);
    switch (tokenPtr->type) {
        case TCL_TOKEN_WORD:
	    code = TclCompileTokens(interp, tokenPtr+1,
	            tokenPtr->numComponents, envPtr);
	    if (code != TCL_OK) {
		goto done;
	    }
	    maxDepth = envPtr->maxStackDepth;
	    tokenPtr += (tokenPtr->numComponents + 1);
	    infoPtr->exprIsJustVarRef = 0;
	    break;
	    
        case TCL_TOKEN_TEXT:
	    if (tokenPtr->size > 0) {
		objIndex = TclRegisterLiteral(envPtr, tokenPtr->start,
	                tokenPtr->size, /*onHeap*/ 0);
	    } else {
		objIndex = TclRegisterLiteral(envPtr, "", 0, /*onHeap*/ 0);
	    }
	    TclEmitPush(objIndex, envPtr);
	    maxDepth = 1;
	    tokenPtr += 1;
	    infoPtr->exprIsJustVarRef = 0;
	    break;
	    
        case TCL_TOKEN_BS:
	    length = Tcl_UtfBackslash(tokenPtr->start, (int *) NULL,
		    buffer);
	    if (length > 0) {
		objIndex = TclRegisterLiteral(envPtr, buffer, length,
	                /*onHeap*/ 0);
	    } else {
		objIndex = TclRegisterLiteral(envPtr, "", 0, /*onHeap*/ 0);
	    }
	    TclEmitPush(objIndex, envPtr);
	    maxDepth = 1;
	    tokenPtr += 1;
	    infoPtr->exprIsJustVarRef = 0;
	    break;
	    
        case TCL_TOKEN_COMMAND:
	    code = TclCompileScript(interp, tokenPtr->start+1,
		    tokenPtr->size-2, /*nested*/ 1, envPtr);
	    if (code != TCL_OK) {
		goto done;
	    }
	    maxDepth = envPtr->maxStackDepth;
	    tokenPtr += 1;
	    infoPtr->exprIsJustVarRef = 0;
	    break;
	    
        case TCL_TOKEN_VARIABLE:
	    code = TclCompileTokens(interp, tokenPtr, 1, envPtr);
	    if (code != TCL_OK) {
		goto done;
	    }
	    maxDepth = envPtr->maxStackDepth;
	    tokenPtr += (tokenPtr->numComponents + 1);
	    break;
	    
        case TCL_TOKEN_SUB_EXPR:
	    infoPtr->exprIsComparison = 0;
	    code = CompileSubExpr(tokenPtr, infoPtr, envPtr);
	    if (code != TCL_OK) {
		goto done;
	    }
	    maxDepth = envPtr->maxStackDepth;
	    tokenPtr += (tokenPtr->numComponents + 1);
	    break;
	    
        case TCL_TOKEN_OPERATOR:
	    /*
	     * Look up the operator. Temporarily overwrite the character
	     * just after the end of the operator with a 0 byte. If the
	     * operator isn't found, treat it as a math function.
	     */

	    /*
	     * TODO: Note that the string is modified in place.  This is unsafe
	     * and will break if any of the routines called while the string is
	     * modified have side effects that depend on the original string
	     * being unmodified (e.g. adding an entry to the literal table).
	     */

	    operator = tokenPtr->start;
	    savedChar = operator[tokenPtr->size];
	    operator[tokenPtr->size] = 0;
	    hPtr = Tcl_FindHashEntry(&opHashTable, operator);
	    if (hPtr == NULL) {
		code = CompileMathFuncCall(exprTokenPtr, operator, infoPtr,
		        envPtr, &endPtr);
		operator[tokenPtr->size] = (char) savedChar;
		if (code != TCL_OK) {
		    goto done;
		}
		maxDepth = envPtr->maxStackDepth;
		tokenPtr = endPtr;
		infoPtr->exprIsJustVarRef = 0;
		infoPtr->exprIsComparison = 0;
		break;
	    }
	    operator[tokenPtr->size] = (char) savedChar;
	    opIndex = (int) Tcl_GetHashValue(hPtr);
	    opDescPtr = &(operatorTable[opIndex]);

	    /*
	     * If the operator is "normal", compile it using information
	     * from the operator table.
	     */

	    if (opDescPtr->numOperands > 0) {
		tokenPtr++;
		code = CompileSubExpr(tokenPtr, infoPtr, envPtr);
		if (code != TCL_OK) {
		    goto done;
		}
		maxDepth = envPtr->maxStackDepth;
		tokenPtr += (tokenPtr->numComponents + 1);

		if (opDescPtr->numOperands == 2) {
		    code = CompileSubExpr(tokenPtr, infoPtr, envPtr);
		    if (code != TCL_OK) {
			goto done;
		    }
		    maxDepth = TclMax((envPtr->maxStackDepth + 1),
		            maxDepth);
		    tokenPtr += (tokenPtr->numComponents + 1);
		}
		TclEmitOpcode(opDescPtr->instruction, envPtr);
		infoPtr->hasOperators = 1;
		infoPtr->exprIsJustVarRef = 0;
		infoPtr->exprIsComparison =
		        ((opIndex >= OP_LESS) && (opIndex <= OP_NEQ));
		break;
	    }
	    
	    /*
	     * The operator requires special treatment, and is either
	     * "+" or "-", or one of "&&", "||" or "?".
	     */
	    
	    switch (opIndex) {
	        case OP_PLUS:
	        case OP_MINUS:
		    tokenPtr++;
		    code = CompileSubExpr(tokenPtr, infoPtr, envPtr);
		    if (code != TCL_OK) {
			goto done;
		    }
		    maxDepth = envPtr->maxStackDepth;
		    tokenPtr += (tokenPtr->numComponents + 1);
		    
		    /*
		     * Check whether the "+" or "-" is unary.
		     */
		    
		    afterSubexprPtr = exprTokenPtr
			    + exprTokenPtr->numComponents+1;
		    if (tokenPtr == afterSubexprPtr) {
			TclEmitOpcode(((opIndex==OP_PLUS)?
			        INST_UPLUS : INST_UMINUS),
			        envPtr);
			break;
		    }
		    
		    /*
		     * The "+" or "-" is binary.
		     */
		    
		    code = CompileSubExpr(tokenPtr, infoPtr, envPtr);
		    if (code != TCL_OK) {
			goto done;
		    }
		    maxDepth = TclMax((envPtr->maxStackDepth + 1),
			    maxDepth);
		    tokenPtr += (tokenPtr->numComponents + 1);
		    TclEmitOpcode(((opIndex==OP_PLUS)? INST_ADD : INST_SUB),
			    envPtr);
		    break;

	        case OP_LAND:
	        case OP_LOR:
		    code = CompileLandOrLorExpr(exprTokenPtr, opIndex,
			    infoPtr, envPtr, &endPtr);
		    if (code != TCL_OK) {
			goto done;
		    }
		    maxDepth = envPtr->maxStackDepth;
		    tokenPtr = endPtr;
		    break;
			
	        case OP_QUESTY:
		    code = CompileCondExpr(exprTokenPtr, infoPtr,
			    envPtr, &endPtr);
		    if (code != TCL_OK) {
			goto done;
		    }
		    maxDepth = envPtr->maxStackDepth;
		    tokenPtr = endPtr;
		    break;
		    
		default:
		    panic("CompileSubExpr: unexpected operator %d requiring special treatment\n",
		        opIndex);
	    } /* end switch on operator requiring special treatment */
	    infoPtr->hasOperators = 1;
	    infoPtr->exprIsJustVarRef = 0;
	    infoPtr->exprIsComparison = 0;
	    break;

        default:
	    panic("CompileSubExpr: unexpected token type %d\n",
	            tokenPtr->type);
    }

    /*
     * Verify that the subexpression token had the required number of
     * subtokens: that we've advanced tokenPtr just beyond the
     * subexpression's last token. For example, a "*" subexpression must
     * contain the tokens for exactly two operands.
     */
    
    if (tokenPtr != (exprTokenPtr + exprTokenPtr->numComponents+1)) {
	LogSyntaxError(infoPtr);
	code = TCL_ERROR;
    }
    
    done:
    envPtr->maxStackDepth = maxDepth;
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * CompileLandOrLorExpr --
 *
 *	This procedure compiles a Tcl logical and ("&&") or logical or
 *	("||") subexpression.
 *
 * Results:
 *	The return value is TCL_OK on a successful compilation and TCL_ERROR
 *	on failure. If TCL_OK is returned, a pointer to the token just after
 *	the last one in the subexpression is stored at the address in
 *	endPtrPtr. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the expression.
 *
 * Side effects:
 *	Adds instructions to envPtr to evaluate the expression at runtime.
 *
 *----------------------------------------------------------------------
 */

static int
CompileLandOrLorExpr(exprTokenPtr, opIndex, infoPtr, envPtr, endPtrPtr)
    Tcl_Token *exprTokenPtr;	 /* Points to TCL_TOKEN_SUB_EXPR token
				  * containing the "&&" or "||" operator. */
    int opIndex;		 /* A code describing the expression
				  * operator: either OP_LAND or OP_LOR. */
    ExprInfo *infoPtr;		 /* Describes the compilation state for the
				  * expression being compiled. */
    CompileEnv *envPtr;		 /* Holds resulting instructions. */
    Tcl_Token **endPtrPtr;	 /* If successful, a pointer to the token
				  * just after the last token in the
				  * subexpression is stored here. */
{
    JumpFixup shortCircuitFixup; /* Used to fix up the short circuit jump
				  * after the first subexpression. */
    JumpFixup lhsTrueFixup, lhsEndFixup;
    				 /* Used to fix up jumps used to convert the
				  * first operand to 0 or 1. */
    Tcl_Token *tokenPtr;
    int dist, maxDepth, code;

    /*
     * Emit code for the first operand.
     */

    maxDepth = 0;
    tokenPtr = exprTokenPtr+2;
    code = CompileSubExpr(tokenPtr, infoPtr, envPtr);
    if (code != TCL_OK) {
	goto done;
    }
    maxDepth = envPtr->maxStackDepth;
    tokenPtr += (tokenPtr->numComponents + 1);

    /*
     * Convert the first operand to the result that Tcl requires:
     * "0" or "1". Eventually we'll use a new instruction for this.
     */
    
    TclEmitForwardJump(envPtr, TCL_TRUE_JUMP, &lhsTrueFixup);
    TclEmitPush(TclRegisterLiteral(envPtr, "0", 1, /*onHeap*/ 0), envPtr);
    TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &lhsEndFixup);
    dist = (envPtr->codeNext - envPtr->codeStart) - lhsTrueFixup.codeOffset;
    if (TclFixupForwardJump(envPtr, &lhsTrueFixup, dist, 127)) {
        badDist:
	panic("CompileLandOrLorExpr: bad jump distance %d\n", dist);
    }
    TclEmitPush(TclRegisterLiteral(envPtr, "1", 1, /*onHeap*/ 0), envPtr);
    dist = (envPtr->codeNext - envPtr->codeStart) - lhsEndFixup.codeOffset;
    if (TclFixupForwardJump(envPtr, &lhsEndFixup, dist, 127)) {
	goto badDist;
    }

    /*
     * Emit the "short circuit" jump around the rest of the expression.
     * Duplicate the "0" or "1" on top of the stack first to keep the
     * jump from consuming it.
     */

    TclEmitOpcode(INST_DUP, envPtr);
    TclEmitForwardJump(envPtr,
	    ((opIndex==OP_LAND)? TCL_FALSE_JUMP : TCL_TRUE_JUMP),
	    &shortCircuitFixup);

    /*
     * Emit code for the second operand.
     */

    code = CompileSubExpr(tokenPtr, infoPtr, envPtr);
    if (code != TCL_OK) {
	goto done;
    }
    maxDepth = TclMax((envPtr->maxStackDepth + 1), maxDepth);
    tokenPtr += (tokenPtr->numComponents + 1);

    /*
     * Emit a "logical and" or "logical or" instruction. This does not try
     * to "short- circuit" the evaluation of both operands, but instead
     * ensures that we either have a "1" or a "0" result.
     */

    TclEmitOpcode(((opIndex==OP_LAND)? INST_LAND : INST_LOR), envPtr);

    /*
     * Now that we know the target of the forward jump, update it with the
     * correct distance.
     */

    dist = (envPtr->codeNext - envPtr->codeStart)
	    - shortCircuitFixup.codeOffset;
    TclFixupForwardJump(envPtr, &shortCircuitFixup, dist, 127);
    *endPtrPtr = tokenPtr;

    done:
    envPtr->maxStackDepth = maxDepth;
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * CompileCondExpr --
 *
 *	This procedure compiles a Tcl conditional expression:
 *	condExpr ::= lorExpr ['?' condExpr ':' condExpr]
 *
 * Results:
 *	The return value is TCL_OK on a successful compilation and TCL_ERROR
 *	on failure. If TCL_OK is returned, a pointer to the token just after
 *	the last one in the subexpression is stored at the address in
 *	endPtrPtr. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the expression.
 *
 * Side effects:
 *	Adds instructions to envPtr to evaluate the expression at runtime.
 *
 *----------------------------------------------------------------------
 */

static int
CompileCondExpr(exprTokenPtr, infoPtr, envPtr, endPtrPtr)
    Tcl_Token *exprTokenPtr;	/* Points to TCL_TOKEN_SUB_EXPR token
				 * containing the "?" operator. */
    ExprInfo *infoPtr;		/* Describes the compilation state for the
				 * expression being compiled. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
    Tcl_Token **endPtrPtr;	/* If successful, a pointer to the token
				 * just after the last token in the
				 * subexpression is stored here. */
{
    JumpFixup jumpAroundThenFixup, jumpAroundElseFixup;
				/* Used to update or replace one-byte jumps
				 * around the then and else expressions when
				 * their target PCs are determined. */
    Tcl_Token *tokenPtr;
    int elseCodeOffset, dist, maxDepth, code;

    /*
     * Emit code for the test.
     */

    maxDepth = 0;
    tokenPtr = exprTokenPtr+2;
    code = CompileSubExpr(tokenPtr, infoPtr, envPtr);
    if (code != TCL_OK) {
	goto done;
    }
    maxDepth = envPtr->maxStackDepth;
    tokenPtr += (tokenPtr->numComponents + 1);
    
    /*
     * Emit the jump to the "else" expression if the test was false.
     */
    
    TclEmitForwardJump(envPtr, TCL_FALSE_JUMP, &jumpAroundThenFixup);

    /*
     * Compile the "then" expression. Note that if a subexpression is only
     * a primary, we need to try to convert it to numeric. We do this to
     * support Tcl's policy of interpreting operands if at all possible as
     * first integers, else floating-point numbers.
     */

    infoPtr->hasOperators = 0;
    code = CompileSubExpr(tokenPtr, infoPtr, envPtr);
    if (code != TCL_OK) {
	goto done;
    }
    maxDepth = TclMax(envPtr->maxStackDepth, maxDepth);
    tokenPtr += (tokenPtr->numComponents + 1);
    if (!infoPtr->hasOperators) {
	TclEmitOpcode(INST_TRY_CVT_TO_NUMERIC, envPtr);
    }

    /*
     * Emit an unconditional jump around the "else" condExpr.
     */
    
    TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP,
	    &jumpAroundElseFixup);

    /*
     * Compile the "else" expression.
     */

    elseCodeOffset = (envPtr->codeNext - envPtr->codeStart);
    infoPtr->hasOperators = 0;
    code = CompileSubExpr(tokenPtr, infoPtr, envPtr);
    if (code != TCL_OK) {
	goto done;
    }
    maxDepth = TclMax(envPtr->maxStackDepth, maxDepth);
    tokenPtr += (tokenPtr->numComponents + 1);
    if (!infoPtr->hasOperators) {
	TclEmitOpcode(INST_TRY_CVT_TO_NUMERIC, envPtr);
    }

    /*
     * Fix up the second jump around the "else" expression.
     */

    dist = (envPtr->codeNext - envPtr->codeStart)
	    - jumpAroundElseFixup.codeOffset;
    if (TclFixupForwardJump(envPtr, &jumpAroundElseFixup, dist, 127)) {
	/*
	 * Update the else expression's starting code offset since it
	 * moved down 3 bytes too.
	 */
	
	elseCodeOffset += 3;
    }
	
    /*
     * Fix up the first jump to the "else" expression if the test was false.
     */
    
    dist = (elseCodeOffset - jumpAroundThenFixup.codeOffset);
    TclFixupForwardJump(envPtr, &jumpAroundThenFixup, dist, 127);
    *endPtrPtr = tokenPtr;

    done:
    envPtr->maxStackDepth = maxDepth;
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * CompileMathFuncCall --
 *
 *	This procedure compiles a call on a math function in an expression:
 *	mathFuncCall ::= funcName '(' [condExpr {',' condExpr}] ')'
 *
 * Results:
 *	The return value is TCL_OK on a successful compilation and TCL_ERROR
 *	on failure. If TCL_OK is returned, a pointer to the token just after
 *	the last one in the subexpression is stored at the address in
 *	endPtrPtr. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the function.
 *
 * Side effects:
 *	Adds instructions to envPtr to evaluate the math function at
 *	runtime.
 *
 *----------------------------------------------------------------------
 */

static int
CompileMathFuncCall(exprTokenPtr, funcName, infoPtr, envPtr, endPtrPtr)
    Tcl_Token *exprTokenPtr;	/* Points to TCL_TOKEN_SUB_EXPR token
				 * containing the math function call. */
    char *funcName;		/* Name of the math function. */
    ExprInfo *infoPtr;		/* Describes the compilation state for the
				 * expression being compiled. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
    Tcl_Token **endPtrPtr;	/* If successful, a pointer to the token
				 * just after the last token in the
				 * subexpression is stored here. */
{
    Tcl_Interp *interp = infoPtr->interp;
    Interp *iPtr = (Interp *) interp;
    MathFunc *mathFuncPtr;
    Tcl_HashEntry *hPtr;
    Tcl_Token *tokenPtr, *afterSubexprPtr;
    int maxDepth, code, i;

    /*
     * Look up the MathFunc record for the function.
     */

    code = TCL_OK;
    maxDepth = 0;
    hPtr = Tcl_FindHashEntry(&iPtr->mathFuncTable, funcName);
    if (hPtr == NULL) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"unknown math function \"", funcName, "\"", (char *) NULL);
	code = TCL_ERROR;
	goto done;
    }
    mathFuncPtr = (MathFunc *) Tcl_GetHashValue(hPtr);

    /*
     * If not a builtin function, push an object with the function's name.
     */

    if (mathFuncPtr->builtinFuncIndex < 0) {
	TclEmitPush(TclRegisterLiteral(envPtr, funcName, -1, /*onHeap*/ 0),
	        envPtr);
	maxDepth = 1;
    }

    /*
     * Compile any arguments for the function.
     */

    tokenPtr = exprTokenPtr+2;
    afterSubexprPtr = exprTokenPtr + (exprTokenPtr->numComponents + 1);
    if (mathFuncPtr->numArgs > 0) {
	for (i = 0;  i < mathFuncPtr->numArgs;  i++) {
	    if (tokenPtr == afterSubexprPtr) {
		Tcl_ResetResult(interp);
		Tcl_AppendToObj(Tcl_GetObjResult(interp),
		        "too few arguments for math function", -1);
		code = TCL_ERROR;
		goto done;
	    }
	    infoPtr->exprIsComparison = 0;
	    code = CompileSubExpr(tokenPtr, infoPtr, envPtr);
	    if (code != TCL_OK) {
		goto done;
	    }
	    tokenPtr += (tokenPtr->numComponents + 1);
	    maxDepth++;
	}
	if (tokenPtr != afterSubexprPtr) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp),
		    "too many arguments for math function", -1);
	    code = TCL_ERROR;
	    goto done;
	} 
    } else if (tokenPtr != afterSubexprPtr) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
		"too many arguments for math function", -1);
	code = TCL_ERROR;
	goto done;
    }
    
    /*
     * Compile the call on the math function. Note that the "objc" argument
     * count for non-builtin functions is incremented by 1 to include the
     * function name itself.
     */

    if (mathFuncPtr->builtinFuncIndex >= 0) { /* a builtin function */
	TclEmitInstInt1(INST_CALL_BUILTIN_FUNC1, 
	        mathFuncPtr->builtinFuncIndex, envPtr);
    } else {
	TclEmitInstInt1(INST_CALL_FUNC1, (mathFuncPtr->numArgs+1), envPtr);
    }
    *endPtrPtr = afterSubexprPtr;

    done:
    envPtr->maxStackDepth = maxDepth;
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * LogSyntaxError --
 *
 *	This procedure is invoked after an error occurs when compiling an
 *	expression. It sets the interpreter result to an error message
 *	describing the error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the interpreter result to an error message describing the
 *	expression that was being compiled when the error occurred.
 *
 *----------------------------------------------------------------------
 */

static void
LogSyntaxError(infoPtr)
    ExprInfo *infoPtr;		/* Describes the compilation state for the
				 * expression being compiled. */
{
    int numBytes = (infoPtr->lastChar - infoPtr->expr);
    char buffer[100];

    sprintf(buffer, "syntax error in expression \"%.*s\"",
	    ((numBytes > 60)? 60 : numBytes), infoPtr->expr);
    Tcl_AppendStringsToObj(Tcl_GetObjResult(infoPtr->interp),
	    buffer, (char *) NULL);
}
