/* 
 * tclCompCmds.c --
 *
 *	This file contains compilation procedures that compile various
 *	Tcl commands into a sequence of instructions ("bytecodes"). 
 *
 * Copyright (c) 1997-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclCompile.h"

/*
 * Prototypes for procedures defined later in this file:
 */

static ClientData	DupForeachInfo _ANSI_ARGS_((ClientData clientData));
static void		FreeForeachInfo _ANSI_ARGS_((
			    ClientData clientData));

/*
 * The structures below define the AuxData types defined in this file.
 */

AuxDataType tclForeachInfoType = {
    "ForeachInfo",				/* name */
    DupForeachInfo,				/* dupProc */
    FreeForeachInfo				/* freeProc */
};

/*
 *----------------------------------------------------------------------
 *
 * TclCompileBreakCmd --
 *
 *	Procedure called to compile the "break" command.
 *
 * Results:
 *	The return value is a standard Tcl result, which is TCL_OK unless
 *	there was an error during compilation. If an error occurs then
 *	the interpreter's result contains a standard error message.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the command.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "break" command
 *	at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileBreakCmd(interp, parsePtr, envPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Parse *parsePtr;	/* Points to a parse structure for the
				 * command created by Tcl_ParseCommand. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    if (parsePtr->numWords != 1) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
	        "wrong # args: should be \"break\"", -1);
	envPtr->maxStackDepth = 0;
	return TCL_ERROR;
    }

    /*
     * Emit a break instruction.
     */

    TclEmitOpcode(INST_BREAK, envPtr);
    envPtr->maxStackDepth = 0;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileCatchCmd --
 *
 *	Procedure called to compile the "catch" command.
 *
 * Results:
 *	The return value is a standard Tcl result, which is TCL_OK if
 *	compilation was successful. If an error occurs then the
 *	interpreter's result contains a standard error message and TCL_ERROR
 *	is returned. If the command is too complex for TclCompileCatchCmd,
 *	TCL_OUT_LINE_COMPILE is returned indicating that the catch command
 *	should be compiled "out of line" by emitting code to invoke its
 *	command procedure at runtime.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the command.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "catch" command
 *	at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileCatchCmd(interp, parsePtr, envPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Parse *parsePtr;	/* Points to a parse structure for the
				 * command created by Tcl_ParseCommand. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    JumpFixup jumpFixup;
    Tcl_Token *cmdTokenPtr, *nameTokenPtr;
    char *name;
    int localIndex, nameChars, range, maxDepth, startOffset, jumpDist;
    int code;
    char buffer[32 + TCL_INTEGER_SPACE];

    envPtr->maxStackDepth = 0;
    if ((parsePtr->numWords != 2) && (parsePtr->numWords != 3)) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
	        "wrong # args: should be \"catch command ?varName?\"", -1);
	return TCL_ERROR;
    }

    /*
     * If a variable was specified and the catch command is at global level
     * (not in a procedure), don't compile it inline: the payoff is
     * too small.
     */

    if ((parsePtr->numWords == 3) && (envPtr->procPtr == NULL)) {
	return TCL_OUT_LINE_COMPILE;
    }

    /*
     * Make sure the variable name, if any, has no substitutions and just
     * refers to a local scaler.
     */

    localIndex = -1;
    cmdTokenPtr = parsePtr->tokenPtr
	    + (parsePtr->tokenPtr->numComponents + 1);
    if (parsePtr->numWords == 3) {
	nameTokenPtr = cmdTokenPtr + (cmdTokenPtr->numComponents + 1);
	if (nameTokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	    name = nameTokenPtr[1].start;
	    nameChars = nameTokenPtr[1].size;
	    if (!TclIsLocalScalar(name, nameChars)) {
		return TCL_OUT_LINE_COMPILE;
	    }
	    localIndex = TclFindCompiledLocal(nameTokenPtr[1].start,
		    nameTokenPtr[1].size, /*create*/ 1, 
		    /*flags*/ VAR_SCALAR, envPtr->procPtr);
	} else {
	   return TCL_OUT_LINE_COMPILE;
	}
    }

    /*
     * We will compile the catch command. Emit a beginCatch instruction at
     * the start of the catch body: the subcommand it controls.
     */

    maxDepth = 0;
    
    envPtr->exceptDepth++;
    envPtr->maxExceptDepth =
	TclMax(envPtr->exceptDepth, envPtr->maxExceptDepth);
    range = TclCreateExceptRange(CATCH_EXCEPTION_RANGE, envPtr);
    TclEmitInstInt4(INST_BEGIN_CATCH4, range, envPtr);

    startOffset = (envPtr->codeNext - envPtr->codeStart);
    envPtr->exceptArrayPtr[range].codeOffset = startOffset;
    code = TclCompileCmdWord(interp, cmdTokenPtr+1,
	    cmdTokenPtr->numComponents, envPtr);
    if (code != TCL_OK) {
	if (code == TCL_ERROR) {
	    sprintf(buffer, "\n    (\"catch\" body line %d)",
		    interp->errorLine);
            Tcl_AddObjErrorInfo(interp, buffer, -1);
        }
	goto done;
    }
    maxDepth = envPtr->maxStackDepth;
    envPtr->exceptArrayPtr[range].numCodeBytes =
	    (envPtr->codeNext - envPtr->codeStart) - startOffset;
		    
    /*
     * The "no errors" epilogue code: store the body's result into the
     * variable (if any), push "0" (TCL_OK) as the catch's "no error"
     * result, and jump around the "error case" code.
     */

    if (localIndex != -1) {
	if (localIndex <= 255) {
	    TclEmitInstInt1(INST_STORE_SCALAR1, localIndex, envPtr);
	} else {
	    TclEmitInstInt4(INST_STORE_SCALAR4, localIndex, envPtr);
	}
    }
    TclEmitOpcode(INST_POP, envPtr);
    TclEmitPush(TclRegisterLiteral(envPtr, "0", 1, /*onHeap*/ 0),
	    envPtr);
    if (maxDepth == 0) {
	maxDepth = 1;
    }
    TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP, &jumpFixup);

    /*
     * The "error case" code: store the body's result into the variable (if
     * any), then push the error result code. The initial PC offset here is
     * the catch's error target.
     */

    envPtr->exceptArrayPtr[range].catchOffset =
	    (envPtr->codeNext - envPtr->codeStart);
    if (localIndex != -1) {
	TclEmitOpcode(INST_PUSH_RESULT, envPtr);
	if (localIndex <= 255) {
	    TclEmitInstInt1(INST_STORE_SCALAR1, localIndex, envPtr);
	} else {
	    TclEmitInstInt4(INST_STORE_SCALAR4, localIndex, envPtr);
	}
	TclEmitOpcode(INST_POP, envPtr);
    }
    TclEmitOpcode(INST_PUSH_RETURN_CODE, envPtr);

    /*
     * Update the target of the jump after the "no errors" code, then emit
     * an endCatch instruction at the end of the catch command.
     */

    jumpDist = (envPtr->codeNext - envPtr->codeStart)
	    - jumpFixup.codeOffset;
    if (TclFixupForwardJump(envPtr, &jumpFixup, jumpDist, 127)) {
	panic("TclCompileCatchCmd: bad jump distance %d\n", jumpDist);
    }
    TclEmitOpcode(INST_END_CATCH, envPtr);

    done:
    envPtr->exceptDepth--;
    envPtr->maxStackDepth = maxDepth;
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileContinueCmd --
 *
 *	Procedure called to compile the "continue" command.
 *
 * Results:
 *	The return value is a standard Tcl result, which is TCL_OK unless
 *	there was an error while parsing string. If an error occurs then
 *	the interpreter's result contains a standard error message.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the command.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "continue" command
 *	at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileContinueCmd(interp, parsePtr, envPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Parse *parsePtr;	/* Points to a parse structure for the
				 * command created by Tcl_ParseCommand. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    /*
     * There should be no argument after the "continue".
     */

    if (parsePtr->numWords != 1) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
	        "wrong # args: should be \"continue\"", -1);
	envPtr->maxStackDepth = 0;
	return TCL_ERROR;
    }

    /*
     * Emit a continue instruction.
     */

    TclEmitOpcode(INST_CONTINUE, envPtr);
    envPtr->maxStackDepth = 0;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileExprCmd --
 *
 *	Procedure called to compile the "expr" command.
 *
 * Results:
 *	The return value is a standard Tcl result, which is TCL_OK
 *	unless there was an error while parsing string. If an error occurs
 *	then the interpreter's result contains a standard error message.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the "expr" command.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "expr" command
 *	at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileExprCmd(interp, parsePtr, envPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Parse *parsePtr;	/* Points to a parse structure for the
				 * command created by Tcl_ParseCommand. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    Tcl_Token *firstWordPtr;

    envPtr->maxStackDepth = 0;
    if (parsePtr->numWords == 1) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
	        "wrong # args: should be \"expr arg ?arg ...?\"", -1);
        return TCL_ERROR;
    }

    firstWordPtr = parsePtr->tokenPtr
	    + (parsePtr->tokenPtr->numComponents + 1);
    return TclCompileExprWords(interp, firstWordPtr, (parsePtr->numWords-1),
	    envPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileForCmd --
 *
 *	Procedure called to compile the "for" command.
 *
 * Results:
 *	The return value is a standard Tcl result, which is TCL_OK unless
 *	there was an error while parsing string. If an error occurs then
 *	the interpreter's result contains a standard error message.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the command.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "for" command
 *	at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileForCmd(interp, parsePtr, envPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Parse *parsePtr;	/* Points to a parse structure for the
				 * command created by Tcl_ParseCommand. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    Tcl_Token *startTokenPtr, *testTokenPtr, *nextTokenPtr, *bodyTokenPtr;
    JumpFixup jumpFalseFixup;
    int maxDepth, jumpBackDist, jumpBackOffset, testCodeOffset, jumpDist;
    int bodyRange, nextRange, code;
    unsigned char *jumpPc;
    char buffer[32 + TCL_INTEGER_SPACE];

    envPtr->maxStackDepth = 0;
    if (parsePtr->numWords != 5) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
	        "wrong # args: should be \"for start test next command\"", -1);
	return TCL_ERROR;
    }

    /*
     * If the test expression requires substitutions, don't compile the for
     * command inline. E.g., the expression might cause the loop to never
     * execute or execute forever, as in "for {} "$x > 5" {incr x} {}".
     */

    startTokenPtr = parsePtr->tokenPtr
	    + (parsePtr->tokenPtr->numComponents + 1);
    testTokenPtr = startTokenPtr + (startTokenPtr->numComponents + 1);
    if (testTokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	return TCL_OUT_LINE_COMPILE;
    }

    /*
     * Create ExceptionRange records for the body and the "next" command.
     * The "next" command's ExceptionRange supports break but not continue
     * (and has a -1 continueOffset).
     */

    envPtr->exceptDepth++;
    envPtr->maxExceptDepth =
	    TclMax(envPtr->exceptDepth, envPtr->maxExceptDepth);
    bodyRange = TclCreateExceptRange(LOOP_EXCEPTION_RANGE, envPtr);
    nextRange = TclCreateExceptRange(LOOP_EXCEPTION_RANGE, envPtr);

    /*
     * Inline compile the initial command.
     */

    maxDepth = 0;
    code = TclCompileCmdWord(interp, startTokenPtr+1,
	    startTokenPtr->numComponents, envPtr);
    if (code != TCL_OK) {
	if (code == TCL_ERROR) {
            Tcl_AddObjErrorInfo(interp,
	            "\n    (\"for\" initial command)", -1);
        }
	goto done;
    }
    maxDepth = envPtr->maxStackDepth;
    TclEmitOpcode(INST_POP, envPtr);
    
    /*
     * Compile the test then emit the conditional jump that exits the for.
     */

    testCodeOffset = (envPtr->codeNext - envPtr->codeStart);
    code = TclCompileExprWords(interp, testTokenPtr, 1, envPtr);
    if (code != TCL_OK) {
	if (code == TCL_ERROR) {
            Tcl_AddObjErrorInfo(interp,
		    "\n    (\"for\" test expression)", -1);
        }
	goto done;
    }
    maxDepth = TclMax(envPtr->maxStackDepth, maxDepth);
    TclEmitForwardJump(envPtr, TCL_FALSE_JUMP, &jumpFalseFixup);

    /*
     * Compile the loop body.
     */

    nextTokenPtr = testTokenPtr + (testTokenPtr->numComponents + 1);
    bodyTokenPtr = nextTokenPtr + (nextTokenPtr->numComponents + 1);
    envPtr->exceptArrayPtr[bodyRange].codeOffset =
	    (envPtr->codeNext - envPtr->codeStart);
    code = TclCompileCmdWord(interp, bodyTokenPtr+1,
	    bodyTokenPtr->numComponents, envPtr);
    if (code != TCL_OK) {
	if (code == TCL_ERROR) {
	    sprintf(buffer, "\n    (\"for\" body line %d)",
		    interp->errorLine);
            Tcl_AddObjErrorInfo(interp, buffer, -1);
        }
	goto done;
    }
    maxDepth = TclMax(envPtr->maxStackDepth, maxDepth);
    envPtr->exceptArrayPtr[bodyRange].numCodeBytes =
	    (envPtr->codeNext - envPtr->codeStart)
	    - envPtr->exceptArrayPtr[bodyRange].codeOffset;
    TclEmitOpcode(INST_POP, envPtr);

    /*
     * Compile the "next" subcommand.
     */

    envPtr->exceptArrayPtr[bodyRange].continueOffset =
	    (envPtr->codeNext - envPtr->codeStart);
    envPtr->exceptArrayPtr[nextRange].codeOffset =
	    (envPtr->codeNext - envPtr->codeStart);
    code = TclCompileCmdWord(interp, nextTokenPtr+1,
	    nextTokenPtr->numComponents, envPtr);
    if (code != TCL_OK) {
	if (code == TCL_ERROR) {
	    Tcl_AddObjErrorInfo(interp,
	            "\n    (\"for\" loop-end command)", -1);
	}
	goto done;
    }
    maxDepth = TclMax(envPtr->maxStackDepth, maxDepth);
    envPtr->exceptArrayPtr[nextRange].numCodeBytes =
	    (envPtr->codeNext - envPtr->codeStart)
	    - envPtr->exceptArrayPtr[nextRange].codeOffset;
    TclEmitOpcode(INST_POP, envPtr);
	
    /*
     * Jump back to the test at the top of the loop. Generate a 4 byte jump
     * if the distance to the test is > 120 bytes. This is conservative and
     * ensures that we won't have to replace this jump if we later need to
     * replace the ifFalse jump with a 4 byte jump.
     */

    jumpBackOffset = (envPtr->codeNext - envPtr->codeStart);
    jumpBackDist = (jumpBackOffset - testCodeOffset);
    if (jumpBackDist > 120) {
	TclEmitInstInt4(INST_JUMP4, -jumpBackDist, envPtr);
    } else {
	TclEmitInstInt1(INST_JUMP1, -jumpBackDist, envPtr);
    }

    /*
     * Fix the target of the jumpFalse after the test.
     */

    jumpDist = (envPtr->codeNext - envPtr->codeStart)
	    - jumpFalseFixup.codeOffset;
    if (TclFixupForwardJump(envPtr, &jumpFalseFixup, jumpDist, 127)) {
	/*
	 * Update the loop body and "next" command ExceptionRanges since
	 * they moved down.
	 */

	envPtr->exceptArrayPtr[bodyRange].codeOffset += 3;
	envPtr->exceptArrayPtr[bodyRange].continueOffset += 3;
	envPtr->exceptArrayPtr[nextRange].codeOffset += 3;

	/*
	 * Update the jump back to the test at the top of the loop since it
	 * also moved down 3 bytes.
	 */

	jumpBackOffset += 3;
	jumpPc = (envPtr->codeStart + jumpBackOffset);
	jumpBackDist += 3;
	if (jumpBackDist > 120) {
	    TclUpdateInstInt4AtPc(INST_JUMP4, -jumpBackDist, jumpPc);
	} else {
	    TclUpdateInstInt1AtPc(INST_JUMP1, -jumpBackDist, jumpPc);
	}
    }
    
    /*
     * Set the loop's break target.
     */

    envPtr->exceptArrayPtr[bodyRange].breakOffset =
            envPtr->exceptArrayPtr[nextRange].breakOffset =
	    (envPtr->codeNext - envPtr->codeStart);
    
    /*
     * The for command's result is an empty string.
     */

    TclEmitPush(TclRegisterLiteral(envPtr, "", 0, /*onHeap*/ 0), envPtr);
    if (maxDepth == 0) {
	maxDepth = 1;
    }
    code = TCL_OK;

    done:
    envPtr->maxStackDepth = maxDepth;
    envPtr->exceptDepth--;
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileForeachCmd --
 *
 *	Procedure called to compile the "foreach" command.
 *
 * Results:
 *	The return value is a standard Tcl result, which is TCL_OK if
 *	compilation was successful. If an error occurs then the
 *	interpreter's result contains a standard error message and TCL_ERROR
 *	is returned. If the command is too complex for TclCompileForeachCmd,
 *	TCL_OUT_LINE_COMPILE is returned indicating that the foreach command
 *	should be compiled "out of line" by emitting code to invoke its
 *	command procedure at runtime.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the "while" command.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "foreach" command
 *	at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileForeachCmd(interp, parsePtr, envPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Parse *parsePtr;	/* Points to a parse structure for the
				 * command created by Tcl_ParseCommand. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    Proc *procPtr = envPtr->procPtr;
    ForeachInfo *infoPtr;	/* Points to the structure describing this
				 * foreach command. Stored in a AuxData
				 * record in the ByteCode. */
    int firstValueTemp;		/* Index of the first temp var in the frame
				 * used to point to a value list. */
    int loopCtTemp;		/* Index of temp var holding the loop's
				 * iteration count. */
    Tcl_Token *tokenPtr, *bodyTokenPtr;
    char *varList;
    unsigned char *jumpPc;
    JumpFixup jumpFalseFixup;
    int jumpDist, jumpBackDist, jumpBackOffset, maxDepth, infoIndex, range;
    int numWords, numLists, numVars, loopIndex, tempVar, i, j, code;
    char savedChar;
    char buffer[32 + TCL_INTEGER_SPACE];

    /*
     * We parse the variable list argument words and create two arrays:
     *    varcList[i] is number of variables in i-th var list
     *    varvList[i] points to array of var names in i-th var list
     */

#define STATIC_VAR_LIST_SIZE 5
    int varcListStaticSpace[STATIC_VAR_LIST_SIZE];
    char **varvListStaticSpace[STATIC_VAR_LIST_SIZE];
    int *varcList = varcListStaticSpace;
    char ***varvList = varvListStaticSpace;

    /*
     * If the foreach command isn't in a procedure, don't compile it inline:
     * the payoff is too small.
     */

    envPtr->maxStackDepth = 0;
    if (procPtr == NULL) {
	return TCL_OUT_LINE_COMPILE;
    }

    maxDepth = 0;
    
    numWords = parsePtr->numWords;
    if ((numWords < 4) || (numWords%2 != 0)) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
	        "wrong # args: should be \"foreach varList list ?varList list ...? command\"", -1);
        return TCL_ERROR;
    }

    /*
     * Allocate storage for the varcList and varvList arrays if necessary.
     */

    numLists = (numWords - 2)/2;
    if (numLists > STATIC_VAR_LIST_SIZE) {
        varcList = (int *) ckalloc(numLists * sizeof(int));
        varvList = (char ***) ckalloc(numLists * sizeof(char **));
    }
    for (loopIndex = 0;  loopIndex < numLists;  loopIndex++) {
        varcList[loopIndex] = 0;
        varvList[loopIndex] = (char **) NULL;
    }
    
    /*
     * Set the exception stack depth.
     */ 

    envPtr->exceptDepth++;
    envPtr->maxExceptDepth =
	TclMax(envPtr->exceptDepth, envPtr->maxExceptDepth);

    /*
     * Break up each var list and set the varcList and varvList arrays.
     * Don't compile the foreach inline if any var name needs substitutions
     * or isn't a scalar, or if any var list needs substitutions.
     */

    loopIndex = 0;
    for (i = 0, tokenPtr = parsePtr->tokenPtr;
	    i < numWords-1;
	    i++, tokenPtr += (tokenPtr->numComponents + 1)) {
	if (i%2 == 1) {
	    if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
		code = TCL_OUT_LINE_COMPILE;
		goto done;
	    }
	    varList = tokenPtr[1].start;
	    savedChar = varList[tokenPtr[1].size];

	    /*
	     * Note there is a danger that modifying the string could have
	     * undesirable side effects.  In this case, Tcl_SplitList does
	     * not have any dependencies on shared strings so we should be
	     * safe.
	     */

	    varList[tokenPtr[1].size] = '\0';
	    code = Tcl_SplitList(interp, varList,
		    &varcList[loopIndex], &varvList[loopIndex]);
	    varList[tokenPtr[1].size] = savedChar;
	    if (code != TCL_OK) {
		goto done;
	    }

	    numVars = varcList[loopIndex];
	    for (j = 0;  j < numVars;  j++) {
		char *varName = varvList[loopIndex][j];
		if (!TclIsLocalScalar(varName, (int) strlen(varName))) {
		    code = TCL_OUT_LINE_COMPILE;
		    goto done;
		}
	    }
	    loopIndex++;
	}
    }

    /*
     * We will compile the foreach command.
     * Reserve (numLists + 1) temporary variables:
     *    - numLists temps to hold each value list
     *    - 1 temp for the loop counter (index of next element in each list)
     * At this time we don't try to reuse temporaries; if there are two
     * nonoverlapping foreach loops, they don't share any temps.
     */

    firstValueTemp = -1;
    for (loopIndex = 0;  loopIndex < numLists;  loopIndex++) {
	tempVar = TclFindCompiledLocal(NULL, /*nameChars*/ 0,
		/*create*/ 1, /*flags*/ VAR_SCALAR, procPtr);
	if (loopIndex == 0) {
	    firstValueTemp = tempVar;
	}
    }
    loopCtTemp = TclFindCompiledLocal(NULL, /*nameChars*/ 0,
	    /*create*/ 1, /*flags*/ VAR_SCALAR, procPtr);
    
    /*
     * Create and initialize the ForeachInfo and ForeachVarList data
     * structures describing this command. Then create a AuxData record
     * pointing to the ForeachInfo structure.
     */

    infoPtr = (ForeachInfo *) ckalloc((unsigned)
	    (sizeof(ForeachInfo) + (numLists * sizeof(ForeachVarList *))));
    infoPtr->numLists = numLists;
    infoPtr->firstValueTemp = firstValueTemp;
    infoPtr->loopCtTemp = loopCtTemp;
    for (loopIndex = 0;  loopIndex < numLists;  loopIndex++) {
	ForeachVarList *varListPtr;
	numVars = varcList[loopIndex];
	varListPtr = (ForeachVarList *) ckalloc((unsigned)
	        sizeof(ForeachVarList) + (numVars * sizeof(int)));
	varListPtr->numVars = numVars;
	for (j = 0;  j < numVars;  j++) {
	    char *varName = varvList[loopIndex][j];
	    int nameChars = strlen(varName);
	    varListPtr->varIndexes[j] = TclFindCompiledLocal(varName,
		    nameChars, /*create*/ 1, /*flags*/ VAR_SCALAR, procPtr);
	}
	infoPtr->varLists[loopIndex] = varListPtr;
    }
    infoIndex = TclCreateAuxData((ClientData) infoPtr, &tclForeachInfoType, envPtr);

    /*
     * Evaluate then store each value list in the associated temporary.
     */

    range = TclCreateExceptRange(LOOP_EXCEPTION_RANGE, envPtr);
    
    loopIndex = 0;
    for (i = 0, tokenPtr = parsePtr->tokenPtr;
	    i < numWords-1;
	    i++, tokenPtr += (tokenPtr->numComponents + 1)) {
	if ((i%2 == 0) && (i > 0)) {
	    code = TclCompileTokens(interp, tokenPtr+1,
		    tokenPtr->numComponents, envPtr);
	    if (code != TCL_OK) {
		goto done;
	    }
	    maxDepth = TclMax(envPtr->maxStackDepth, maxDepth);

	    tempVar = (firstValueTemp + loopIndex);
	    if (tempVar <= 255) {
		TclEmitInstInt1(INST_STORE_SCALAR1, tempVar, envPtr);
	    } else {
		TclEmitInstInt4(INST_STORE_SCALAR4, tempVar, envPtr);
	    }
	    TclEmitOpcode(INST_POP, envPtr);
	    loopIndex++;
	}
    }
    bodyTokenPtr = tokenPtr;

    /*
     * Initialize the temporary var that holds the count of loop iterations.
     */

    TclEmitInstInt4(INST_FOREACH_START4, infoIndex, envPtr);
    
    /*
     * Top of loop code: assign each loop variable and check whether
     * to terminate the loop.
     */

    envPtr->exceptArrayPtr[range].continueOffset =
	    (envPtr->codeNext - envPtr->codeStart);
    TclEmitInstInt4(INST_FOREACH_STEP4, infoIndex, envPtr);
    TclEmitForwardJump(envPtr, TCL_FALSE_JUMP, &jumpFalseFixup);
    
    /*
     * Inline compile the loop body.
     */

    envPtr->exceptArrayPtr[range].codeOffset =
	    (envPtr->codeNext - envPtr->codeStart);
    code = TclCompileCmdWord(interp, bodyTokenPtr+1,
	    bodyTokenPtr->numComponents, envPtr);
    if (code != TCL_OK) {
	if (code == TCL_ERROR) {
	    sprintf(buffer, "\n    (\"foreach\" body line %d)",
		    interp->errorLine);
            Tcl_AddObjErrorInfo(interp, buffer, -1);
        }
	goto done;
    }
    maxDepth = TclMax(envPtr->maxStackDepth, maxDepth);
    envPtr->exceptArrayPtr[range].numCodeBytes =
	    (envPtr->codeNext - envPtr->codeStart)
	    - envPtr->exceptArrayPtr[range].codeOffset;
    TclEmitOpcode(INST_POP, envPtr);
	
    /*
     * Jump back to the test at the top of the loop. Generate a 4 byte jump
     * if the distance to the test is > 120 bytes. This is conservative and
     * ensures that we won't have to replace this jump if we later need to
     * replace the ifFalse jump with a 4 byte jump.
     */

    jumpBackOffset = (envPtr->codeNext - envPtr->codeStart);
    jumpBackDist =
	(jumpBackOffset - envPtr->exceptArrayPtr[range].continueOffset);
    if (jumpBackDist > 120) {
	TclEmitInstInt4(INST_JUMP4, -jumpBackDist, envPtr);
    } else {
	TclEmitInstInt1(INST_JUMP1, -jumpBackDist, envPtr);
    }

    /*
     * Fix the target of the jump after the foreach_step test.
     */

    jumpDist = (envPtr->codeNext - envPtr->codeStart)
	    - jumpFalseFixup.codeOffset;
    if (TclFixupForwardJump(envPtr, &jumpFalseFixup, jumpDist, 127)) {
	/*
	 * Update the loop body's starting PC offset since it moved down.
	 */

	envPtr->exceptArrayPtr[range].codeOffset += 3;

	/*
	 * Update the jump back to the test at the top of the loop since it
	 * also moved down 3 bytes.
	 */

	jumpBackOffset += 3;
	jumpPc = (envPtr->codeStart + jumpBackOffset);
	jumpBackDist += 3;
	if (jumpBackDist > 120) {
	    TclUpdateInstInt4AtPc(INST_JUMP4, -jumpBackDist, jumpPc);
	} else {
	    TclUpdateInstInt1AtPc(INST_JUMP1, -jumpBackDist, jumpPc);
	}
    }

    /*
     * Set the loop's break target.
     */

    envPtr->exceptArrayPtr[range].breakOffset =
	    (envPtr->codeNext - envPtr->codeStart);
    
    /*
     * The foreach command's result is an empty string.
     */

    TclEmitPush(TclRegisterLiteral(envPtr, "", 0, /*onHeap*/ 0), envPtr);
    if (maxDepth == 0) {
	maxDepth = 1;
    }

    done:
    for (loopIndex = 0;  loopIndex < numLists;  loopIndex++) {
        if (varvList[loopIndex] != (char **) NULL) {
            ckfree((char *) varvList[loopIndex]);
        }
    }
    if (varcList != varcListStaticSpace) {
	ckfree((char *) varcList);
        ckfree((char *) varvList);
    }
    envPtr->maxStackDepth = maxDepth;
    envPtr->exceptDepth--;
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * DupForeachInfo --
 *
 *	This procedure duplicates a ForeachInfo structure created as
 *	auxiliary data during the compilation of a foreach command.
 *
 * Results:
 *	A pointer to a newly allocated copy of the existing ForeachInfo
 *	structure is returned.
 *
 * Side effects:
 *	Storage for the copied ForeachInfo record is allocated. If the
 *	original ForeachInfo structure pointed to any ForeachVarList
 *	records, these structures are also copied and pointers to them
 *	are stored in the new ForeachInfo record.
 *
 *----------------------------------------------------------------------
 */

static ClientData
DupForeachInfo(clientData)
    ClientData clientData;	/* The foreach command's compilation
				 * auxiliary data to duplicate. */
{
    register ForeachInfo *srcPtr = (ForeachInfo *) clientData;
    ForeachInfo *dupPtr;
    register ForeachVarList *srcListPtr, *dupListPtr;
    int numLists = srcPtr->numLists;
    int numVars, i, j;
    
    dupPtr = (ForeachInfo *) ckalloc((unsigned)
	    (sizeof(ForeachInfo) + (numLists * sizeof(ForeachVarList *))));
    dupPtr->numLists = numLists;
    dupPtr->firstValueTemp = srcPtr->firstValueTemp;
    dupPtr->loopCtTemp = srcPtr->loopCtTemp;
    
    for (i = 0;  i < numLists;  i++) {
	srcListPtr = srcPtr->varLists[i];
	numVars = srcListPtr->numVars;
	dupListPtr = (ForeachVarList *) ckalloc((unsigned)
	        sizeof(ForeachVarList) + numVars*sizeof(int));
	dupListPtr->numVars = numVars;
	for (j = 0;  j < numVars;  j++) {
	    dupListPtr->varIndexes[j] =	srcListPtr->varIndexes[j];
	}
	dupPtr->varLists[i] = dupListPtr;
    }
    return (ClientData) dupPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeForeachInfo --
 *
 *	Procedure to free a ForeachInfo structure created as auxiliary data
 *	during the compilation of a foreach command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Storage for the ForeachInfo structure pointed to by the ClientData
 *	argument is freed as is any ForeachVarList record pointed to by the
 *	ForeachInfo structure.
 *
 *----------------------------------------------------------------------
 */

static void
FreeForeachInfo(clientData)
    ClientData clientData;	/* The foreach command's compilation
				 * auxiliary data to free. */
{
    register ForeachInfo *infoPtr = (ForeachInfo *) clientData;
    register ForeachVarList *listPtr;
    int numLists = infoPtr->numLists;
    register int i;

    for (i = 0;  i < numLists;  i++) {
	listPtr = infoPtr->varLists[i];
	ckfree((char *) listPtr);
    }
    ckfree((char *) infoPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileIfCmd --
 *
 *	Procedure called to compile the "if" command.
 *
 * Results:
 *	The return value is a standard Tcl result, which is TCL_OK if
 *	compilation was successful. If an error occurs then the
 *	interpreter's result contains a standard error message and TCL_ERROR
 *	is returned. If the command is too complex for TclCompileIfCmd,
 *	TCL_OUT_LINE_COMPILE is returned indicating that the if command
 *	should be compiled "out of line" by emitting code to invoke its
 *	command procedure at runtime.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the command.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "if" command
 *	at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileIfCmd(interp, parsePtr, envPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Parse *parsePtr;	/* Points to a parse structure for the
				 * command created by Tcl_ParseCommand. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    JumpFixupArray jumpFalseFixupArray;
    				/* Used to fix the ifFalse jump after each
				 * test when its target PC is determined. */
    JumpFixupArray jumpEndFixupArray;
				/* Used to fix the jump after each "then"
				 * body to the end of the "if" when that PC
				 * is determined. */
    Tcl_Token *tokenPtr, *testTokenPtr;
    int jumpDist, jumpFalseDist, jumpIndex;
    int numWords, wordIdx, numBytes, maxDepth, j, code;
    char *word;
    char buffer[100];

    TclInitJumpFixupArray(&jumpFalseFixupArray);
    TclInitJumpFixupArray(&jumpEndFixupArray);
    maxDepth = 0;
    code = TCL_OK;

    /*
     * Each iteration of this loop compiles one "if expr ?then? body"
     * or "elseif expr ?then? body" clause. 
     */

    tokenPtr = parsePtr->tokenPtr;
    wordIdx = 0;
    numWords = parsePtr->numWords;
    while (wordIdx < numWords) {
	/*
	 * Stop looping if the token isn't "if" or "elseif".
	 */

	if (tokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	    break;
	}
	word = tokenPtr[1].start;
	numBytes = tokenPtr[1].size;
	if ((tokenPtr == parsePtr->tokenPtr)
	        || ((numBytes == 6) && (strncmp(word, "elseif", 6) == 0))) {
	    tokenPtr += (tokenPtr->numComponents + 1);
	    wordIdx++;
	} else {
	    break;
	}
	if (wordIdx >= numWords) {
	    sprintf(buffer,
	            "wrong # args: no expression after \"%.30s\" argument",
		    word);
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp), buffer, -1);
	    code = TCL_ERROR;
	    goto done;
	}

	/*
	 * Compile the test expression then emit the conditional jump
	 * around the "then" part. If the expression word isn't simple,
	 * we back off and compile the if command out-of-line.
	 */
	
	testTokenPtr = tokenPtr;
	code = TclCompileExprWords(interp, testTokenPtr, 1, envPtr);
	if (code != TCL_OK) {
	    if (code == TCL_ERROR) {
		Tcl_AddObjErrorInfo(interp,
		        "\n    (\"if\" test expression)", -1);
	    }
	    goto done;
	}
	maxDepth = TclMax(envPtr->maxStackDepth, maxDepth);
	if (jumpFalseFixupArray.next >= jumpFalseFixupArray.end) {
	    TclExpandJumpFixupArray(&jumpFalseFixupArray);
	}
	jumpIndex = jumpFalseFixupArray.next;
	jumpFalseFixupArray.next++;
	TclEmitForwardJump(envPtr, TCL_FALSE_JUMP,
		&(jumpFalseFixupArray.fixup[jumpIndex]));
	
	/*
	 * Skip over the optional "then" before the then clause.
	 */

	tokenPtr = testTokenPtr + (testTokenPtr->numComponents + 1);
	wordIdx++;
	if (wordIdx >= numWords) {
	    sprintf(buffer, "wrong # args: no script following \"%.20s\" argument", testTokenPtr->start);
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp), buffer, -1);
	    code = TCL_ERROR;
	    goto done;
	}
	if (tokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	    word = tokenPtr[1].start;
	    numBytes = tokenPtr[1].size;
	    if ((numBytes == 4) && (strncmp(word, "then", 4) == 0)) {
		tokenPtr += (tokenPtr->numComponents + 1);
		wordIdx++;
		if (wordIdx >= numWords) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendToObj(Tcl_GetObjResult(interp),
		            "wrong # args: no script following \"then\" argument", -1);
		    code = TCL_ERROR;
		    goto done;
		}
	    }
	}

	/*
	 * Compile the "then" command body.
	 */

	code = TclCompileCmdWord(interp, tokenPtr+1,
		tokenPtr->numComponents, envPtr);
	if (code != TCL_OK) {
	    if (code == TCL_ERROR) {
		sprintf(buffer, "\n    (\"if\" then script line %d)",
		        interp->errorLine);
		Tcl_AddObjErrorInfo(interp, buffer, -1);
	    }
	    goto done;
	}
	maxDepth = TclMax(envPtr->maxStackDepth, maxDepth);

	/*
	 * Jump to the end of the "if" command. Both jumpFalseFixupArray and
	 * jumpEndFixupArray are indexed by "jumpIndex".
	 */

	if (jumpEndFixupArray.next >= jumpEndFixupArray.end) {
	    TclExpandJumpFixupArray(&jumpEndFixupArray);
	}
	jumpEndFixupArray.next++;
	TclEmitForwardJump(envPtr, TCL_UNCONDITIONAL_JUMP,
		&(jumpEndFixupArray.fixup[jumpIndex]));

 	/*
	 * Fix the target of the jumpFalse after the test. Generate a 4 byte
	 * jump if the distance is > 120 bytes. This is conservative, and
	 * ensures that we won't have to replace this jump if we later also
	 * need to replace the proceeding jump to the end of the "if" with a
	 * 4 byte jump.
	 */

	jumpDist = (envPtr->codeNext - envPtr->codeStart)
	        - jumpFalseFixupArray.fixup[jumpIndex].codeOffset;
	if (TclFixupForwardJump(envPtr,
	        &(jumpFalseFixupArray.fixup[jumpIndex]), jumpDist, 120)) {
	    /*
	     * Adjust the code offset for the proceeding jump to the end
	     * of the "if" command.
	     */

	    jumpEndFixupArray.fixup[jumpIndex].codeOffset += 3;
	}

	tokenPtr += (tokenPtr->numComponents + 1);
	wordIdx++;
    }

    /*
     * Check for the optional else clause.
     */

    if ((wordIdx < numWords)
	    && (tokenPtr->type == TCL_TOKEN_SIMPLE_WORD)) {
	/*
	 * There is an else clause. Skip over the optional "else" word.
	 */
	
	word = tokenPtr[1].start;
	numBytes = tokenPtr[1].size;
	if ((numBytes == 4) && (strncmp(word, "else", 4) == 0)) {
	    tokenPtr += (tokenPtr->numComponents + 1);
	    wordIdx++;
	    if (wordIdx >= numWords) {
		Tcl_ResetResult(interp);
		Tcl_AppendToObj(Tcl_GetObjResult(interp),
		        "wrong # args: no script following \"else\" argument", -1);
		code = TCL_ERROR;
		goto done;
	    }
	}

	/*
	 * Compile the else command body.
	 */
	
	code = TclCompileCmdWord(interp, tokenPtr+1,
		tokenPtr->numComponents, envPtr);
	if (code != TCL_OK) {
	    if (code == TCL_ERROR) {
		sprintf(buffer, "\n    (\"if\" else script line %d)",
			interp->errorLine);
		Tcl_AddObjErrorInfo(interp, buffer, -1);
	    }
	    goto done;
	}
	maxDepth = TclMax(envPtr->maxStackDepth, maxDepth);

	/*
	 * Make sure there are no words after the else clause.
	 */
	
	wordIdx++;
	if (wordIdx < numWords) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp),
		    "wrong # args: extra words after \"else\" clause in \"if\" command", -1);
	    code = TCL_ERROR;
	    goto done;
	}
    } else {
	/*
	 * No else clause: the "if" command's result is an empty string.
	 */

	TclEmitPush(TclRegisterLiteral(envPtr, "", 0,/*onHeap*/ 0), envPtr);
	maxDepth = TclMax(1, maxDepth);
    }

    /*
     * Fix the unconditional jumps to the end of the "if" command.
     */
    
    for (j = jumpEndFixupArray.next;  j > 0;  j--) {
	jumpIndex = (j - 1);	/* i.e. process the closest jump first */
	jumpDist = (envPtr->codeNext - envPtr->codeStart)
	        - jumpEndFixupArray.fixup[jumpIndex].codeOffset;
	if (TclFixupForwardJump(envPtr,
	        &(jumpEndFixupArray.fixup[jumpIndex]), jumpDist, 127)) {
	    /*
	     * Adjust the immediately preceeding "ifFalse" jump. We moved
	     * it's target (just after this jump) down three bytes.
	     */

	    unsigned char *ifFalsePc = envPtr->codeStart
	            + jumpFalseFixupArray.fixup[jumpIndex].codeOffset;
	    unsigned char opCode = *ifFalsePc;
	    if (opCode == INST_JUMP_FALSE1) {
		jumpFalseDist = TclGetInt1AtPtr(ifFalsePc + 1);
		jumpFalseDist += 3;
		TclStoreInt1AtPtr(jumpFalseDist, (ifFalsePc + 1));
	    } else if (opCode == INST_JUMP_FALSE4) {
		jumpFalseDist = TclGetInt4AtPtr(ifFalsePc + 1);
		jumpFalseDist += 3;
		TclStoreInt4AtPtr(jumpFalseDist, (ifFalsePc + 1));
	    } else {
		panic("TclCompileIfCmd: unexpected opcode updating ifFalse jump");
	    }
	}
    }
	
    /*
     * Free the jumpFixupArray array if malloc'ed storage was used.
     */

    done:
    TclFreeJumpFixupArray(&jumpFalseFixupArray);
    TclFreeJumpFixupArray(&jumpEndFixupArray);
    envPtr->maxStackDepth = maxDepth;
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileIncrCmd --
 *
 *	Procedure called to compile the "incr" command.
 *
 * Results:
 *	The return value is a standard Tcl result, which is TCL_OK if
 *	compilation was successful. If an error occurs then the
 *	interpreter's result contains a standard error message and TCL_ERROR
 *	is returned. If the command is too complex for TclCompileIncrCmd,
 *	TCL_OUT_LINE_COMPILE is returned indicating that the incr command
 *	should be compiled "out of line" by emitting code to invoke its
 *	command procedure at runtime.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the "incr" command.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "incr" command
 *	at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileIncrCmd(interp, parsePtr, envPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Parse *parsePtr;	/* Points to a parse structure for the
				 * command created by Tcl_ParseCommand. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    Tcl_Token *varTokenPtr, *incrTokenPtr;
    Tcl_Parse elemParse;
    int gotElemParse = 0;
    char *name, *elName, *p;
    int nameChars, elNameChars, haveImmValue, immValue, localIndex, i, code;
    int maxDepth = 0;
    char buffer[160];

    envPtr->maxStackDepth = 0;
    if ((parsePtr->numWords != 2) && (parsePtr->numWords != 3)) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
	        "wrong # args: should be \"incr varName ?increment?\"", -1);
	return TCL_ERROR;
    }
    
    name = NULL;
    elName = NULL;
    elNameChars = 0;
    localIndex = -1;
    code = TCL_OK;

    varTokenPtr = parsePtr->tokenPtr
	    + (parsePtr->tokenPtr->numComponents + 1);
    /*
     * Check not only that the type is TCL_TOKEN_SIMPLE_WORD, but whether
     * curly braces surround the variable name.
     * This really matters for array elements to handle things like
     *    set {x($foo)} 5
     * which raises an undefined var error if we are not careful here.
     * This goes with the hack in TclCompileSetCmd.
     */
    if ((varTokenPtr->type == TCL_TOKEN_SIMPLE_WORD) &&
	    (varTokenPtr->start[0] != '{')) {
	/*
	 * A simple variable name. Divide it up into "name" and "elName"
	 * strings. If it is not a local variable, look it up at runtime.
	 */
	
	name = varTokenPtr[1].start;
	nameChars = varTokenPtr[1].size;
	for (i = 0, p = name;  i < nameChars;  i++, p++) {
	    if (*p == '(') {
		char *openParen = p;
		p = (name + nameChars-1);	
		if (*p == ')') { /* last char is ')' => array reference */
		    nameChars = (openParen - name);
		    elName = openParen+1;
		    elNameChars = (p - elName);
		}
		break;
	    }
	}
	if (envPtr->procPtr != NULL) {
	    localIndex = TclFindCompiledLocal(name, nameChars,
	            /*create*/ 0, /*flags*/ 0, envPtr->procPtr);
	    if (localIndex > 255) {	      /* we'll push the name */
		localIndex = -1;
	    }
	}
	if (localIndex < 0) {
	    TclEmitPush(TclRegisterLiteral(envPtr, name, nameChars,
			/*onHeap*/ 0), envPtr);
	    maxDepth = 1;
	}

	/*
	 * Compile the element script, if any.
	 */
	
	if (elName != NULL) {
	    /*
	     * Temporarily replace the '(' and ')' by '"'s.
	     */
	    
	    *(elName-1) = '"';
	    *(elName+elNameChars) = '"';
	    code = Tcl_ParseCommand(interp, elName-1, elNameChars+2,
                    /*nested*/ 0, &elemParse);
	    *(elName-1) = '(';
	    *(elName+elNameChars) = ')';
	    gotElemParse = 1;
	    if ((code != TCL_OK) || (elemParse.numWords > 1)) {
		sprintf(buffer, "\n    (parsing index for array \"%.*s\")",
			TclMin(nameChars, 100), name);
		Tcl_AddObjErrorInfo(interp, buffer, -1);
		code = TCL_ERROR;
		goto done;
	    } else if (elemParse.numWords == 1) {
		code = TclCompileTokens(interp, elemParse.tokenPtr+1,
                        elemParse.tokenPtr->numComponents, envPtr);
		if (code != TCL_OK) {
		    goto done;
		}
		maxDepth += envPtr->maxStackDepth;
	    } else {
		TclEmitPush(TclRegisterLiteral(envPtr, "", 0,
                        /*alreadyAlloced*/ 0), envPtr);
		maxDepth += 1;
	    }
	}
    } else {
	/*
	 * Not a simple variable name. Look it up at runtime.
	 */
	
	code = TclCompileTokens(interp, varTokenPtr+1,
	        varTokenPtr->numComponents, envPtr);
	if (code != TCL_OK) {
	    goto done;
	}
	maxDepth = envPtr->maxStackDepth;
    }
    
    /*
     * If an increment is given, push it, but see first if it's a small
     * integer.
     */

    haveImmValue = 0;
    immValue = 0;
    if (parsePtr->numWords == 3) {
	incrTokenPtr = varTokenPtr + (varTokenPtr->numComponents + 1);
	if (incrTokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	    char *word = incrTokenPtr[1].start;
	    int numBytes = incrTokenPtr[1].size;
	    char savedChar = word[numBytes];
	    long n;
	
	    /*
	     * Note there is a danger that modifying the string could have
	     * undesirable side effects.  In this case, TclLooksLikeInt and
	     * TclGetLong do not have any dependencies on shared strings so we
	     * should be safe.
	     */

	    word[numBytes] = '\0';
	    if (TclLooksLikeInt(word, numBytes)
		    && (TclGetLong((Tcl_Interp *) NULL, word, &n) == TCL_OK)) {
		if ((-127 <= n) && (n <= 127)) {
		    haveImmValue = 1;
		    immValue = n;
		}
	    }
	    word[numBytes] = savedChar;
	    if (!haveImmValue) {
		TclEmitPush(TclRegisterLiteral(envPtr, word, numBytes,
	               /*onHeap*/ 0), envPtr);
		maxDepth += 1;
	    }
	} else {
	    code = TclCompileTokens(interp, incrTokenPtr+1, 
	            incrTokenPtr->numComponents, envPtr);
	    if (code != TCL_OK) {
		if (code == TCL_ERROR) {
		    Tcl_AddObjErrorInfo(interp,
	                    "\n    (increment expression)", -1);
		}
		goto done;
	    }
	    maxDepth += envPtr->maxStackDepth;
	}
    } else {			/* no incr amount given so use 1 */
	haveImmValue = 1;
	immValue = 1;
    }
    
    /*
     * Emit the instruction to increment the variable.
     */

    if (name != NULL) {
	if (elName == NULL) {
	    if (localIndex >= 0) {
		if (haveImmValue) {
		    TclEmitInstInt1(INST_INCR_SCALAR1_IMM, localIndex,
				    envPtr);
		    TclEmitInt1(immValue, envPtr);
		} else {
		    TclEmitInstInt1(INST_INCR_SCALAR1, localIndex, envPtr);
		}
	    } else {
		if (haveImmValue) {
		    TclEmitInstInt1(INST_INCR_SCALAR_STK_IMM, immValue,
				   envPtr);
		} else {
		    TclEmitOpcode(INST_INCR_SCALAR_STK, envPtr);
		}
	    }
	} else {
	    if (localIndex >= 0) {
		if (haveImmValue) {
		    TclEmitInstInt1(INST_INCR_ARRAY1_IMM, localIndex,
				    envPtr);
		    TclEmitInt1(immValue, envPtr);
		} else {
		    TclEmitInstInt1(INST_INCR_ARRAY1, localIndex, envPtr);
		}
	    } else {
		if (haveImmValue) {
		    TclEmitInstInt1(INST_INCR_ARRAY_STK_IMM, immValue,
				   envPtr);
		} else {
		    TclEmitOpcode(INST_INCR_ARRAY_STK, envPtr);
		}
	    }
	}
    } else {			/* non-simple variable name */
	if (haveImmValue) {
	    TclEmitInstInt1(INST_INCR_STK_IMM, immValue, envPtr);
	} else {
	    TclEmitOpcode(INST_INCR_STK, envPtr);
	}
    }
	
    done:
    if (gotElemParse) {
        Tcl_FreeParse(&elemParse);
    }
    envPtr->maxStackDepth = maxDepth;
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileSetCmd --
 *
 *	Procedure called to compile the "set" command.
 *
 * Results:
 *	The return value is a standard Tcl result, which is normally TCL_OK
 *	unless there was an error while parsing string. If an error occurs
 *	then the interpreter's result contains a standard error message. If
 *	complation fails because the set command requires a second level of
 *	substitutions, TCL_OUT_LINE_COMPILE is returned indicating that the
 *	set command should be compiled "out of line" by emitting code to
 *	invoke its command procedure (Tcl_SetCmd) at runtime.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the incr command.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "set" command
 *	at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileSetCmd(interp, parsePtr, envPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Parse *parsePtr;	/* Points to a parse structure for the
				 * command created by Tcl_ParseCommand. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    Tcl_Token *varTokenPtr, *valueTokenPtr;
    Tcl_Parse elemParse;
    int gotElemParse = 0;
    register char *p;
    char *name, *elName;
    int nameChars, elNameChars;
    register int i;
    int isAssignment, simpleVarName, localIndex, numWords;
    int maxDepth = 0;
    int code = TCL_OK;

    envPtr->maxStackDepth = 0;
    numWords = parsePtr->numWords;
    if ((numWords != 2) && (numWords != 3)) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
	        "wrong # args: should be \"set varName ?newValue?\"", -1);
        return TCL_ERROR;
    }
    isAssignment = (numWords == 3);

    /*
     * Decide if we can use a frame slot for the var/array name or if we
     * need to emit code to compute and push the name at runtime. We use a
     * frame slot (entry in the array of local vars) if we are compiling a
     * procedure body and if the name is simple text that does not include
     * namespace qualifiers. 
     */

    simpleVarName = 0;
    name = elName = NULL;
    nameChars = elNameChars = 0;
    localIndex = -1;

    varTokenPtr = parsePtr->tokenPtr
	    + (parsePtr->tokenPtr->numComponents + 1);
    /*
     * Check not only that the type is TCL_TOKEN_SIMPLE_WORD, but whether
     * curly braces surround the variable name.
     * This really matters for array elements to handle things like
     *    set {x($foo)} 5
     * which raises an undefined var error if we are not careful here.
     * This goes with the hack in TclCompileIncrCmd.
     */
    if ((varTokenPtr->type == TCL_TOKEN_SIMPLE_WORD) &&
	    (varTokenPtr->start[0] != '{')) {
	simpleVarName = 1;

	name = varTokenPtr[1].start;
	nameChars = varTokenPtr[1].size;
	/* last char is ')' => potential array reference */
	if ( *(name + nameChars - 1) == ')') {
	    for (i = 0, p = name;  i < nameChars;  i++, p++) {
		if (*p == '(') {
		    elName = p + 1;
		    elNameChars = nameChars - i - 2;
		    nameChars = i ;
		    break;
		}
	    }
	}

	/*
	 * If elName contains any double quotes ("), we can't inline
	 * compile the element script using the replace '()' by '"'
	 * technique below.
	 */

	for (i = 0, p = elName;  i < elNameChars;  i++, p++) {
	    if (*p == '"') {
		simpleVarName = 0;
		break;
	    }
	}
    } else if ((varTokenPtr->numComponents == 4)
	    && (varTokenPtr[1].type == TCL_TOKEN_TEXT)
	    && (varTokenPtr[1].start[varTokenPtr[1].size-1] == '(')
	    && (varTokenPtr[4].type == TCL_TOKEN_TEXT)
	    && (varTokenPtr[4].size == 1)
	    && (varTokenPtr[4].start[0] == ')')) {
	simpleVarName = 1;
	name = varTokenPtr[1].start;
	nameChars = varTokenPtr[1].size - 1;
	elName = varTokenPtr[2].start;
	elNameChars = varTokenPtr[2].size;
    }

    if (simpleVarName) {
	/*
	 * See whether name has any namespace separators (::'s).
	 */

	int hasNsQualifiers = 0;
	for (i = 0, p = name;  i < nameChars;  i++, p++) {
	    if ((*p == ':') && ((i+1) < nameChars) && (*(p+1) == ':')) {
		hasNsQualifiers = 1;
		break;
	    }
	}
	
	/*
	 * Look up the var name's index in the array of local vars in the
	 * proc frame. If retrieving the var's value and it doesn't already
	 * exist, push its name and look it up at runtime.
	 */

	if ((envPtr->procPtr != NULL) && !hasNsQualifiers) {
	    localIndex = TclFindCompiledLocal(name, nameChars,
		    /*create*/ isAssignment,
                    /*flags*/ ((elName==NULL)? VAR_SCALAR : VAR_ARRAY),
		    envPtr->procPtr);
	}
	if (localIndex >= 0) {
	    maxDepth = 0;
	} else {
	    TclEmitPush(TclRegisterLiteral(envPtr, name, nameChars,
		    /*onHeap*/ 0), envPtr);
	    maxDepth = 1;
	}

	/*
	 * Compile the element script, if any.
	 */
	
	if (elName != NULL) {
	    /*
	     * Temporarily replace the '(' and ')' by '"'s.
	     */

	    *(elName-1) = '"';
	    *(elName+elNameChars) = '"';
	    code = Tcl_ParseCommand(interp, elName-1, elNameChars+2,
                    /*nested*/ 0, &elemParse);
	    *(elName-1) = '(';
	    *(elName+elNameChars) = ')';
	    gotElemParse = 1;
	    if ((code != TCL_OK) || (elemParse.numWords > 1)) {
		char buffer[160];
		sprintf(buffer, "\n    (parsing index for array \"%.*s\")",
		        TclMin(nameChars, 100), name);
		Tcl_AddObjErrorInfo(interp, buffer, -1);
		code = TCL_ERROR;
		goto done;
	    } else if (elemParse.numWords == 1) {
		code = TclCompileTokens(interp, elemParse.tokenPtr+1,
                        elemParse.tokenPtr->numComponents, envPtr);
		if (code != TCL_OK) {
		    goto done;
		}
		maxDepth += envPtr->maxStackDepth;
	    } else {
		TclEmitPush(TclRegisterLiteral(envPtr, "", 0,
                        /*alreadyAlloced*/ 0), envPtr);
		maxDepth += 1;
	    }
	}
    } else {
	/*
	 * The var name isn't simple: compile and push it.
	 */

	code = TclCompileTokens(interp, varTokenPtr+1,
		varTokenPtr->numComponents, envPtr);
	if (code != TCL_OK) {
	    goto done;
	}
	maxDepth += envPtr->maxStackDepth;
    }
	
    /*
     * If we are doing an assignment, push the new value.
     */
    
    if (isAssignment) {
	valueTokenPtr = varTokenPtr + (varTokenPtr->numComponents + 1);
	if (valueTokenPtr->type == TCL_TOKEN_SIMPLE_WORD) {
	    TclEmitPush(TclRegisterLiteral(envPtr, valueTokenPtr[1].start,
		    valueTokenPtr[1].size, /*onHeap*/ 0), envPtr);
	    maxDepth += 1;
	} else {
	    code = TclCompileTokens(interp, valueTokenPtr+1,
	            valueTokenPtr->numComponents, envPtr);
	    if (code != TCL_OK) {
		goto done;
	    }
	    maxDepth += envPtr->maxStackDepth;
	}
    }
	
    /*
     * Emit instructions to set/get the variable.
     */

    if (simpleVarName) {
	if (elName == NULL) {
	    if (localIndex >= 0) {
		if (localIndex <= 255) {
		    TclEmitInstInt1((isAssignment?
		            INST_STORE_SCALAR1 : INST_LOAD_SCALAR1),
			    localIndex, envPtr);
		} else {
		    TclEmitInstInt4((isAssignment?
			    INST_STORE_SCALAR4 : INST_LOAD_SCALAR4),
			    localIndex, envPtr);
		}
	    } else {
		TclEmitOpcode((isAssignment?
		        INST_STORE_SCALAR_STK : INST_LOAD_SCALAR_STK),
			envPtr);
	    }
	} else {
	    if (localIndex >= 0) {
		if (localIndex <= 255) {
		    TclEmitInstInt1((isAssignment?
		            INST_STORE_ARRAY1 : INST_LOAD_ARRAY1),
			    localIndex, envPtr);
		} else {
		    TclEmitInstInt4((isAssignment?
			    INST_STORE_ARRAY4 : INST_LOAD_ARRAY4),
			    localIndex, envPtr);
		}
	    } else {
		TclEmitOpcode((isAssignment?
		        INST_STORE_ARRAY_STK : INST_LOAD_ARRAY_STK),
			envPtr);
	    }
	}
    } else {
	TclEmitOpcode((isAssignment? INST_STORE_STK : INST_LOAD_STK),
	        envPtr);
    }
	
    done:
    if (gotElemParse) {
        Tcl_FreeParse(&elemParse);
    }
    envPtr->maxStackDepth = maxDepth;
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TclCompileWhileCmd --
 *
 *	Procedure called to compile the "while" command.
 *
 * Results:
 *	The return value is a standard Tcl result, which is TCL_OK if
 *	compilation was successful. If an error occurs then the
 *	interpreter's result contains a standard error message and TCL_ERROR
 *	is returned. If compilation failed because the command is too
 *	complex for TclCompileWhileCmd, TCL_OUT_LINE_COMPILE is returned
 *	indicating that the while command should be compiled "out of line"
 *	by emitting code to invoke its command procedure at runtime.
 *
 *	envPtr->maxStackDepth is updated with the maximum number of stack
 *	elements needed to execute the "while" command.
 *
 * Side effects:
 *	Instructions are added to envPtr to execute the "while" command
 *	at runtime.
 *
 *----------------------------------------------------------------------
 */

int
TclCompileWhileCmd(interp, parsePtr, envPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Parse *parsePtr;	/* Points to a parse structure for the
				 * command created by Tcl_ParseCommand. */
    CompileEnv *envPtr;		/* Holds resulting instructions. */
{
    Tcl_Token *testTokenPtr, *bodyTokenPtr;
    JumpFixup jumpFalseFixup;
    unsigned char *jumpPc;
    int testCodeOffset, jumpDist, jumpBackDist, jumpBackOffset;
    int range, maxDepth, code;
    char buffer[32 + TCL_INTEGER_SPACE];

    envPtr->maxStackDepth = 0;
    maxDepth = 0;
    if (parsePtr->numWords != 3) {
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp),
	        "wrong # args: should be \"while test command\"", -1);
	return TCL_ERROR;
    }

    /*
     * If the test expression requires substitutions, don't compile the
     * while command inline. E.g., the expression might cause the loop to
     * never execute or execute forever, as in "while "$x < 5" {}".
     */

    testTokenPtr = parsePtr->tokenPtr
	    + (parsePtr->tokenPtr->numComponents + 1);
    if (testTokenPtr->type != TCL_TOKEN_SIMPLE_WORD) {
	return TCL_OUT_LINE_COMPILE;
    }

    /*
     * Create a ExceptionRange record for the loop body. This is used to
     * implement break and continue.
     */

    envPtr->exceptDepth++;
    envPtr->maxExceptDepth =
	TclMax(envPtr->exceptDepth, envPtr->maxExceptDepth);
    range = TclCreateExceptRange(LOOP_EXCEPTION_RANGE, envPtr);
    envPtr->exceptArrayPtr[range].continueOffset =
	    (envPtr->codeNext - envPtr->codeStart);

    /*
     * Compile the test expression then emit the conditional jump that
     * terminates the while. We already know it's a simple word.
     */

    testCodeOffset = (envPtr->codeNext - envPtr->codeStart);
    envPtr->exceptArrayPtr[range].continueOffset = testCodeOffset;
    code = TclCompileExprWords(interp, testTokenPtr, 1, envPtr);
    if (code != TCL_OK) {
	if (code == TCL_ERROR) {
            Tcl_AddObjErrorInfo(interp,
		    "\n    (\"while\" test expression)", -1);
        }
	goto error;
    }
    maxDepth = envPtr->maxStackDepth;
    TclEmitForwardJump(envPtr, TCL_FALSE_JUMP, &jumpFalseFixup);
    
    /*
     * Compile the loop body.
     */

    bodyTokenPtr = testTokenPtr + (testTokenPtr->numComponents + 1);
    envPtr->exceptArrayPtr[range].codeOffset =
	    (envPtr->codeNext - envPtr->codeStart);
    code = TclCompileCmdWord(interp, bodyTokenPtr+1,
	    bodyTokenPtr->numComponents, envPtr);
    if (code != TCL_OK) {
	if (code == TCL_ERROR) {
	    sprintf(buffer, "\n    (\"while\" body line %d)",
		    interp->errorLine);
            Tcl_AddObjErrorInfo(interp, buffer, -1);
        }
	goto error;
    }
    maxDepth = TclMax(envPtr->maxStackDepth, maxDepth);
    envPtr->exceptArrayPtr[range].numCodeBytes =
	    (envPtr->codeNext - envPtr->codeStart)
	    - envPtr->exceptArrayPtr[range].codeOffset;
    TclEmitOpcode(INST_POP, envPtr);
	
    /*
     * Jump back to the test at the top of the loop. Generate a 4 byte jump
     * if the distance to the test is > 120 bytes. This is conservative and
     * ensures that we won't have to replace this jump if we later need to
     * replace the ifFalse jump with a 4 byte jump.
     */

    jumpBackOffset = (envPtr->codeNext - envPtr->codeStart);
    jumpBackDist = (jumpBackOffset - testCodeOffset);
    if (jumpBackDist > 120) {
	TclEmitInstInt4(INST_JUMP4, -jumpBackDist, envPtr);
    } else {
	TclEmitInstInt1(INST_JUMP1, -jumpBackDist, envPtr);
    }

    /*
     * Fix the target of the jumpFalse after the test. 
     */

    jumpDist = (envPtr->codeNext - envPtr->codeStart)
	    - jumpFalseFixup.codeOffset;
    if (TclFixupForwardJump(envPtr, &jumpFalseFixup, jumpDist, 127)) {
	/*
	 * Update the loop body's starting PC offset since it moved down.
	 */

	envPtr->exceptArrayPtr[range].codeOffset += 3;

	/*
	 * Update the jump back to the test at the top of the loop since it
	 * also moved down 3 bytes.
	 */

	jumpBackOffset += 3;
	jumpPc = (envPtr->codeStart + jumpBackOffset);
	jumpBackDist += 3;
	if (jumpBackDist > 120) {
	    TclUpdateInstInt4AtPc(INST_JUMP4, -jumpBackDist, jumpPc);
	} else {
	    TclUpdateInstInt1AtPc(INST_JUMP1, -jumpBackDist, jumpPc);
	}
    }

    /*
     * Set the loop's break target.
     */

    envPtr->exceptArrayPtr[range].breakOffset =
	    (envPtr->codeNext - envPtr->codeStart);
    
    /*
     * The while command's result is an empty string.
     */

    TclEmitPush(TclRegisterLiteral(envPtr, "", 0, /*onHeap*/ 0), envPtr);
    if (maxDepth == 0) {
	maxDepth = 1;
    }
    envPtr->maxStackDepth = maxDepth;
    envPtr->exceptDepth--;
    return TCL_OK;

    error:
    envPtr->maxStackDepth = maxDepth;
    envPtr->exceptDepth--;
    return code;
}



