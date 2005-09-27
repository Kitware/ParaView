/* 
 * tclParseExpr.c --
 *
 *	This file contains procedures that parse Tcl expressions. They
 *	do so in a general-purpose fashion that can be used for many
 *	different purposes, including compilation, direct execution,
 *	code analysis, etc.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 by Scriptics Corporation.
 * Contributions from Don Porter, NIST, 2002.  (not subject to US copyright)
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"

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
 * Boolean variable that controls whether expression parse tracing
 * is enabled.
 */

#ifdef TCL_COMPILE_DEBUG
static int traceParseExpr = 0;
#endif /* TCL_COMPILE_DEBUG */

/*
 * The ParseInfo structure holds state while parsing an expression.
 * A pointer to an ParseInfo record is passed among the routines in
 * this module.
 */

typedef struct ParseInfo {
    Tcl_Parse *parsePtr;	/* Points to structure to fill in with
				 * information about the expression. */
    int lexeme;			/* Type of last lexeme scanned in expr.
				 * See below for definitions. Corresponds to
				 * size characters beginning at start. */
    CONST char *start;		/* First character in lexeme. */
    int size;			/* Number of bytes in lexeme. */
    CONST char *next;		/* Position of the next character to be
				 * scanned in the expression string. */
    CONST char *prevEnd;	/* Points to the character just after the
				 * last one in the previous lexeme. Used to
				 * compute size of subexpression tokens. */
    CONST char *originalExpr;	/* Points to the start of the expression
				 * originally passed to Tcl_ParseExpr. */
    CONST char *lastChar;	/* Points just after last byte of expr. */
} ParseInfo;

/*
 * Definitions of the different lexemes that appear in expressions. The
 * order of these must match the corresponding entries in the
 * operatorStrings array below.
 *
 * Basic lexemes:
 */

#define LITERAL		0
#define FUNC_NAME	1
#define OPEN_BRACKET	2
#define OPEN_BRACE	3
#define OPEN_PAREN	4
#define CLOSE_PAREN	5
#define DOLLAR		6
#define QUOTE		7
#define COMMA		8
#define END		9
#define UNKNOWN		10
#define UNKNOWN_CHAR	11

/*
 * Binary numeric operators:
 */

#define MULT		12
#define DIVIDE		13
#define MOD		14
#define PLUS		15
#define MINUS		16
#define LEFT_SHIFT	17
#define RIGHT_SHIFT	18
#define LESS		19
#define GREATER		20
#define LEQ		21
#define GEQ		22
#define EQUAL		23
#define NEQ		24
#define BIT_AND		25
#define BIT_XOR		26
#define BIT_OR		27
#define AND		28
#define OR		29
#define QUESTY		30
#define COLON		31

/*
 * Unary operators. Unary minus and plus are represented by the (binary)
 * lexemes MINUS and PLUS.
 */

#define NOT		32
#define BIT_NOT		33

/*
 * Binary string operators:
 */

#define STREQ		34
#define STRNEQ		35

/*
 * Mapping from lexemes to strings; used for debugging messages. These
 * entries must match the order and number of the lexeme definitions above.
 */

static char *lexemeStrings[] = {
    "LITERAL", "FUNCNAME",
    "[", "{", "(", ")", "$", "\"", ",", "END", "UNKNOWN", "UNKNOWN_CHAR",
    "*", "/", "%", "+", "-",
    "<<", ">>", "<", ">", "<=", ">=", "==", "!=",
    "&", "^", "|", "&&", "||", "?", ":",
    "!", "~", "eq", "ne",
};

/*
 * Declarations for local procedures to this file:
 */

static int		GetLexeme _ANSI_ARGS_((ParseInfo *infoPtr));
static void		LogSyntaxError _ANSI_ARGS_((ParseInfo *infoPtr,
				CONST char *extraInfo));
static int		ParseAddExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParseBitAndExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParseBitOrExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParseBitXorExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParseCondExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParseEqualityExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParseLandExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParseLorExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParseMaxDoubleLength _ANSI_ARGS_((CONST char *string,
				CONST char *end));
static int		ParseMultiplyExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParsePrimaryExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParseRelationalExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParseShiftExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static int		ParseUnaryExpr _ANSI_ARGS_((ParseInfo *infoPtr));
static void		PrependSubExprTokens _ANSI_ARGS_((CONST char *op,
				int opBytes, CONST char *src, int srcBytes,
				int firstIndex, ParseInfo *infoPtr));

/*
 * Macro used to debug the execution of the recursive descent parser used
 * to parse expressions.
 */

#ifdef TCL_COMPILE_DEBUG
#define HERE(production, level) \
    if (traceParseExpr) { \
	fprintf(stderr, "%*s%s: lexeme=%s, next=\"%.20s\"\n", \
		(level), " ", (production), \
		lexemeStrings[infoPtr->lexeme], infoPtr->next); \
    }
#else
#define HERE(production, level)
#endif /* TCL_COMPILE_DEBUG */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseExpr --
 *
 *	Given a string, this procedure parses the first Tcl expression
 *	in the string and returns information about the structure of
 *	the expression. This procedure is the top-level interface to the
 *	the expression parsing module.  No more that numBytes bytes will
 *	be scanned.
 *
 * Results:
 *	The return value is TCL_OK if the command was parsed successfully
 *	and TCL_ERROR otherwise. If an error occurs and interp isn't NULL
 *	then an error message is left in its result. On a successful return,
 *	parsePtr is filled in with information about the expression that 
 *	was parsed.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the expression, then additional space is
 *	malloc-ed. If the procedure returns TCL_OK then the caller must
 *	eventually invoke Tcl_FreeParse to release any additional space
 *	that was allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ParseExpr(interp, string, numBytes, parsePtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    CONST char *string;		/* The source string to parse. */
    int numBytes;		/* Number of bytes in string. If < 0, the
				 * string consists of all bytes up to the
				 * first null character. */
    Tcl_Parse *parsePtr;	/* Structure to fill with information about
				 * the parsed expression; any previous
				 * information in the structure is
				 * ignored. */
{
    ParseInfo info;
    int code;

    if (numBytes < 0) {
	numBytes = (string? strlen(string) : 0);
    }
#ifdef TCL_COMPILE_DEBUG
    if (traceParseExpr) {
	fprintf(stderr, "Tcl_ParseExpr: string=\"%.*s\"\n",
	        numBytes, string);
    }
#endif /* TCL_COMPILE_DEBUG */
    
    parsePtr->commentStart = NULL;
    parsePtr->commentSize = 0;
    parsePtr->commandStart = NULL;
    parsePtr->commandSize = 0;
    parsePtr->numWords = 0;
    parsePtr->tokenPtr = parsePtr->staticTokens;
    parsePtr->numTokens = 0;
    parsePtr->tokensAvailable = NUM_STATIC_TOKENS;
    parsePtr->string = string;
    parsePtr->end = (string + numBytes);
    parsePtr->interp = interp;
    parsePtr->term = string;
    parsePtr->incomplete = 0;

    /*
     * Initialize the ParseInfo structure that holds state while parsing
     * the expression.
     */

    info.parsePtr = parsePtr;
    info.lexeme = UNKNOWN;
    info.start = NULL;
    info.size = 0;
    info.next = string;
    info.prevEnd = string;
    info.originalExpr = string;
    info.lastChar = (string + numBytes); /* just after last char of expr */

    /*
     * Get the first lexeme then parse the expression.
     */

    code = GetLexeme(&info);
    if (code != TCL_OK) {
	goto error;
    }
    code = ParseCondExpr(&info);
    if (code != TCL_OK) {
	goto error;
    }
    if (info.lexeme != END) {
	LogSyntaxError(&info, "extra tokens at end of expression");
	goto error;
    }
    return TCL_OK;
    
    error:
    if (parsePtr->tokenPtr != parsePtr->staticTokens) {
	ckfree((char *) parsePtr->tokenPtr);
    }
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseCondExpr --
 *
 *	This procedure parses a Tcl conditional expression:
 *	condExpr ::= lorExpr ['?' condExpr ':' condExpr]
 *
 *	Note that this is the topmost recursive-descent parsing routine used
 *	by Tcl_ParseExpr to parse expressions. This avoids an extra procedure
 *	call since such a procedure would only return the result of calling
 *	ParseCondExpr. Other recursive-descent procedures that need to parse
 *	complete expressions also call ParseCondExpr.
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseCondExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    Tcl_Token *tokenPtr, *firstTokenPtr, *condTokenPtr;
    int firstIndex, numToMove, code;
    CONST char *srcStart;
    
    HERE("condExpr", 1);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    code = ParseLorExpr(infoPtr);
    if (code != TCL_OK) {
	return code;
    }
    
    if (infoPtr->lexeme == QUESTY) {
	/*
	 * Emit two tokens: one TCL_TOKEN_SUB_EXPR token for the entire
	 * conditional expression, and a TCL_TOKEN_OPERATOR token for 
	 * the "?" operator. Note that these two tokens must be inserted
	 * before the LOR operand tokens generated above.
	 */

	if ((parsePtr->numTokens + 1) >= parsePtr->tokensAvailable) {
	    TclExpandTokenArray(parsePtr);
	}
	firstTokenPtr = &parsePtr->tokenPtr[firstIndex];
	tokenPtr = (firstTokenPtr + 2);
	numToMove = (parsePtr->numTokens - firstIndex);
	memmove((VOID *) tokenPtr, (VOID *) firstTokenPtr,
	        (size_t) (numToMove * sizeof(Tcl_Token)));
	parsePtr->numTokens += 2;
	
	tokenPtr = firstTokenPtr;
	tokenPtr->type = TCL_TOKEN_SUB_EXPR;
	tokenPtr->start = srcStart;
	
	tokenPtr++;
	tokenPtr->type = TCL_TOKEN_OPERATOR;
	tokenPtr->start = infoPtr->start;
	tokenPtr->size = 1;
	tokenPtr->numComponents = 0;
    
	/*
	 * Skip over the '?'.
	 */
	
	code = GetLexeme(infoPtr); 
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Parse the "then" expression.
	 */

	code = ParseCondExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}
	if (infoPtr->lexeme != COLON) {
	    LogSyntaxError(infoPtr, "missing colon from ternary conditional");
	    return TCL_ERROR;
	}
	code = GetLexeme(infoPtr); /* skip over the ':' */
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Parse the "else" expression.
	 */

	code = ParseCondExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Now set the size-related fields in the '?' subexpression token.
	 */

	condTokenPtr = &parsePtr->tokenPtr[firstIndex];
	condTokenPtr->size = (infoPtr->prevEnd - srcStart);
	condTokenPtr->numComponents = parsePtr->numTokens - (firstIndex+1);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseLorExpr --
 *
 *	This procedure parses a Tcl logical or expression:
 *	lorExpr ::= landExpr {'||' landExpr}
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseLorExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    int firstIndex, code;
    CONST char *srcStart, *operator;
    
    HERE("lorExpr", 2);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    code = ParseLandExpr(infoPtr);
    if (code != TCL_OK) {
	return code;
    }

    while (infoPtr->lexeme == OR) {
	operator = infoPtr->start;
	code = GetLexeme(infoPtr); /* skip over the '||' */
	if (code != TCL_OK) {
	    return code;
	}
	code = ParseLandExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Generate tokens for the LOR subexpression and the '||' operator.
	 */

	PrependSubExprTokens(operator, 2, srcStart,
	        (infoPtr->prevEnd - srcStart), firstIndex, infoPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseLandExpr --
 *
 *	This procedure parses a Tcl logical and expression:
 *	landExpr ::= bitOrExpr {'&&' bitOrExpr}
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseLandExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    int firstIndex, code;
    CONST char *srcStart, *operator;

    HERE("landExpr", 3);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    code = ParseBitOrExpr(infoPtr);
    if (code != TCL_OK) {
	return code;
    }

    while (infoPtr->lexeme == AND) {
	operator = infoPtr->start;
	code = GetLexeme(infoPtr); /* skip over the '&&' */
	if (code != TCL_OK) {
	    return code;
	}
	code = ParseBitOrExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Generate tokens for the LAND subexpression and the '&&' operator.
	 */

	PrependSubExprTokens(operator, 2, srcStart,
	        (infoPtr->prevEnd - srcStart), firstIndex, infoPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseBitOrExpr --
 *
 *	This procedure parses a Tcl bitwise or expression:
 *	bitOrExpr ::= bitXorExpr {'|' bitXorExpr}
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseBitOrExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    int firstIndex, code;
    CONST char *srcStart, *operator;

    HERE("bitOrExpr", 4);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    code = ParseBitXorExpr(infoPtr);
    if (code != TCL_OK) {
	return code;
    }
    
    while (infoPtr->lexeme == BIT_OR) {
	operator = infoPtr->start;
	code = GetLexeme(infoPtr); /* skip over the '|' */
	if (code != TCL_OK) {
	    return code;
	}

	code = ParseBitXorExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}
	
	/*
	 * Generate tokens for the BITOR subexpression and the '|' operator.
	 */

	PrependSubExprTokens(operator, 1, srcStart,
	        (infoPtr->prevEnd - srcStart), firstIndex, infoPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseBitXorExpr --
 *
 *	This procedure parses a Tcl bitwise exclusive or expression:
 *	bitXorExpr ::= bitAndExpr {'^' bitAndExpr}
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseBitXorExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    int firstIndex, code;
    CONST char *srcStart, *operator;

    HERE("bitXorExpr", 5);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    code = ParseBitAndExpr(infoPtr);
    if (code != TCL_OK) {
	return code;
    }
    
    while (infoPtr->lexeme == BIT_XOR) {
	operator = infoPtr->start;
	code = GetLexeme(infoPtr); /* skip over the '^' */
	if (code != TCL_OK) {
	    return code;
	}

	code = ParseBitAndExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}
	
	/*
	 * Generate tokens for the XOR subexpression and the '^' operator.
	 */

	PrependSubExprTokens(operator, 1, srcStart,
	        (infoPtr->prevEnd - srcStart), firstIndex, infoPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseBitAndExpr --
 *
 *	This procedure parses a Tcl bitwise and expression:
 *	bitAndExpr ::= equalityExpr {'&' equalityExpr}
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseBitAndExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    int firstIndex, code;
    CONST char *srcStart, *operator;

    HERE("bitAndExpr", 6);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    code = ParseEqualityExpr(infoPtr);
    if (code != TCL_OK) {
	return code;
    }
    
    while (infoPtr->lexeme == BIT_AND) {
	operator = infoPtr->start;
	code = GetLexeme(infoPtr); /* skip over the '&' */
	if (code != TCL_OK) {
	    return code;
	}
	code = ParseEqualityExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}
	
	/*
	 * Generate tokens for the BITAND subexpression and '&' operator.
	 */

	PrependSubExprTokens(operator, 1, srcStart,
	        (infoPtr->prevEnd - srcStart), firstIndex, infoPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseEqualityExpr --
 *
 *	This procedure parses a Tcl equality (inequality) expression:
 *	equalityExpr ::= relationalExpr
 *		{('==' | '!=' | 'ne' | 'eq') relationalExpr}
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseEqualityExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    int firstIndex, lexeme, code;
    CONST char *srcStart, *operator;

    HERE("equalityExpr", 7);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    code = ParseRelationalExpr(infoPtr);
    if (code != TCL_OK) {
	return code;
    }

    lexeme = infoPtr->lexeme;
    while ((lexeme == EQUAL) || (lexeme == NEQ)
	    || (lexeme == STREQ) || (lexeme == STRNEQ)) {
	operator = infoPtr->start;
	code = GetLexeme(infoPtr); /* skip over ==, !=, 'eq' or 'ne'  */
	if (code != TCL_OK) {
	    return code;
	}
	code = ParseRelationalExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Generate tokens for the subexpression and '==', '!=', 'eq' or 'ne'
	 * operator.
	 */

	PrependSubExprTokens(operator, 2, srcStart,
	        (infoPtr->prevEnd - srcStart), firstIndex, infoPtr);
	lexeme = infoPtr->lexeme;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseRelationalExpr --
 *
 *	This procedure parses a Tcl relational expression:
 *	relationalExpr ::= shiftExpr {('<' | '>' | '<=' | '>=') shiftExpr}
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseRelationalExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    int firstIndex, lexeme, operatorSize, code;
    CONST char *srcStart, *operator;

    HERE("relationalExpr", 8);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    code = ParseShiftExpr(infoPtr);
    if (code != TCL_OK) {
	return code;
    }

    lexeme = infoPtr->lexeme;
    while ((lexeme == LESS) || (lexeme == GREATER) || (lexeme == LEQ)
            || (lexeme == GEQ)) {
	operator = infoPtr->start;
	if ((lexeme == LEQ) || (lexeme == GEQ)) {
	    operatorSize = 2;
	} else {
	    operatorSize = 1;
	}
	code = GetLexeme(infoPtr); /* skip over the operator */
	if (code != TCL_OK) {
	    return code;
	}
	code = ParseShiftExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Generate tokens for the subexpression and the operator.
	 */

	PrependSubExprTokens(operator, operatorSize, srcStart,
	        (infoPtr->prevEnd - srcStart), firstIndex, infoPtr);
	lexeme = infoPtr->lexeme;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseShiftExpr --
 *
 *	This procedure parses a Tcl shift expression:
 *	shiftExpr ::= addExpr {('<<' | '>>') addExpr}
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseShiftExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    int firstIndex, lexeme, code;
    CONST char *srcStart, *operator;

    HERE("shiftExpr", 9);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    code = ParseAddExpr(infoPtr);
    if (code != TCL_OK) {
	return code;
    }

    lexeme = infoPtr->lexeme;
    while ((lexeme == LEFT_SHIFT) || (lexeme == RIGHT_SHIFT)) {
	operator = infoPtr->start;
	code = GetLexeme(infoPtr); /* skip over << or >> */
	if (code != TCL_OK) {
	    return code;
	}
	code = ParseAddExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Generate tokens for the subexpression and '<<' or '>>' operator.
	 */

	PrependSubExprTokens(operator, 2, srcStart,
	        (infoPtr->prevEnd - srcStart), firstIndex, infoPtr);
	lexeme = infoPtr->lexeme;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseAddExpr --
 *
 *	This procedure parses a Tcl addition expression:
 *	addExpr ::= multiplyExpr {('+' | '-') multiplyExpr}
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseAddExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    int firstIndex, lexeme, code;
    CONST char *srcStart, *operator;

    HERE("addExpr", 10);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    code = ParseMultiplyExpr(infoPtr);
    if (code != TCL_OK) {
	return code;
    }

    lexeme = infoPtr->lexeme;
    while ((lexeme == PLUS) || (lexeme == MINUS)) {
	operator = infoPtr->start;
	code = GetLexeme(infoPtr); /* skip over + or - */
	if (code != TCL_OK) {
	    return code;
	}
	code = ParseMultiplyExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Generate tokens for the subexpression and '+' or '-' operator.
	 */

	PrependSubExprTokens(operator, 1, srcStart,
	        (infoPtr->prevEnd - srcStart), firstIndex, infoPtr);
	lexeme = infoPtr->lexeme;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseMultiplyExpr --
 *
 *	This procedure parses a Tcl multiply expression:
 *	multiplyExpr ::= unaryExpr {('*' | '/' | '%') unaryExpr}
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseMultiplyExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    int firstIndex, lexeme, code;
    CONST char *srcStart, *operator;

    HERE("multiplyExpr", 11);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    code = ParseUnaryExpr(infoPtr);
    if (code != TCL_OK) {
	return code;
    }

    lexeme = infoPtr->lexeme;
    while ((lexeme == MULT) || (lexeme == DIVIDE) || (lexeme == MOD)) {
	operator = infoPtr->start;
	code = GetLexeme(infoPtr); /* skip over * or / or % */
	if (code != TCL_OK) {
	    return code;
	}
	code = ParseUnaryExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Generate tokens for the subexpression and * or / or % operator.
	 */

	PrependSubExprTokens(operator, 1, srcStart,
	        (infoPtr->prevEnd - srcStart), firstIndex, infoPtr);
	lexeme = infoPtr->lexeme;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseUnaryExpr --
 *
 *	This procedure parses a Tcl unary expression:
 *	unaryExpr ::= ('+' | '-' | '~' | '!') unaryExpr | primaryExpr
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParseUnaryExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    int firstIndex, lexeme, code;
    CONST char *srcStart, *operator;

    HERE("unaryExpr", 12);
    srcStart = infoPtr->start;
    firstIndex = parsePtr->numTokens;
    
    lexeme = infoPtr->lexeme;
    if ((lexeme == PLUS) || (lexeme == MINUS) || (lexeme == BIT_NOT)
            || (lexeme == NOT)) {
	operator = infoPtr->start;
	code = GetLexeme(infoPtr); /* skip over the unary operator */
	if (code != TCL_OK) {
	    return code;
	}
	code = ParseUnaryExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}

	/*
	 * Generate tokens for the subexpression and the operator.
	 */

	PrependSubExprTokens(operator, 1, srcStart,
	        (infoPtr->prevEnd - srcStart), firstIndex, infoPtr);
    } else {			/* must be a primaryExpr */
	code = ParsePrimaryExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ParsePrimaryExpr --
 *
 *	This procedure parses a Tcl primary expression:
 *	primaryExpr ::= literal | varReference | quotedString |
 *			'[' command ']' | mathFuncCall | '(' condExpr ')'
 *
 * Results:
 *	The return value is TCL_OK on a successful parse and TCL_ERROR
 *	on failure. If TCL_ERROR is returned, then the interpreter's result
 *	contains an error message.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static int
ParsePrimaryExpr(infoPtr)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    Tcl_Interp *interp = parsePtr->interp;
    Tcl_Token *tokenPtr, *exprTokenPtr;
    Tcl_Parse nested;
    CONST char *dollarPtr, *stringStart, *termPtr, *src;
    int lexeme, exprIndex, firstIndex, numToMove, code;

    /*
     * We simply recurse on parenthesized subexpressions.
     */

    HERE("primaryExpr", 13);
    lexeme = infoPtr->lexeme;
    if (lexeme == OPEN_PAREN) {
	code = GetLexeme(infoPtr); /* skip over the '(' */
	if (code != TCL_OK) {
	    return code;
	}
	code = ParseCondExpr(infoPtr);
	if (code != TCL_OK) {
	    return code;
	}
	if (infoPtr->lexeme != CLOSE_PAREN) {
	    LogSyntaxError(infoPtr, "looking for close parenthesis");
	    return TCL_ERROR;
	}
	code = GetLexeme(infoPtr); /* skip over the ')' */
	if (code != TCL_OK) {
	    return code;
	}
	return TCL_OK;
    }

    /*
     * Start a TCL_TOKEN_SUB_EXPR token for the primary.
     */

    if (parsePtr->numTokens == parsePtr->tokensAvailable) {
	TclExpandTokenArray(parsePtr);
    }
    exprIndex = parsePtr->numTokens;
    exprTokenPtr = &parsePtr->tokenPtr[exprIndex];
    exprTokenPtr->type = TCL_TOKEN_SUB_EXPR;
    exprTokenPtr->start = infoPtr->start;
    parsePtr->numTokens++;

    /*
     * Process the primary then finish setting the fields of the
     * TCL_TOKEN_SUB_EXPR token. Note that we can't use the pointer now
     * stored in "exprTokenPtr" in the code below since the token array
     * might be reallocated.
     */

    firstIndex = parsePtr->numTokens;
    switch (lexeme) {
    case LITERAL:
	/*
	 * Int or double number.
	 */
	
	if (parsePtr->numTokens == parsePtr->tokensAvailable) {
	    TclExpandTokenArray(parsePtr);
	}
	tokenPtr = &parsePtr->tokenPtr[parsePtr->numTokens];
	tokenPtr->type = TCL_TOKEN_TEXT;
	tokenPtr->start = infoPtr->start;
	tokenPtr->size = infoPtr->size;
	tokenPtr->numComponents = 0;
	parsePtr->numTokens++;

	exprTokenPtr = &parsePtr->tokenPtr[exprIndex];
	exprTokenPtr->size = infoPtr->size;
	exprTokenPtr->numComponents = 1;
	break;

    case DOLLAR:
	/*
	 * $var variable reference.
	 */
	
	dollarPtr = (infoPtr->next - 1);
	code = Tcl_ParseVarName(interp, dollarPtr,
	        (infoPtr->lastChar - dollarPtr), parsePtr, 1);
	if (code != TCL_OK) {
	    return code;
	}
	infoPtr->next = dollarPtr + parsePtr->tokenPtr[firstIndex].size;

	exprTokenPtr = &parsePtr->tokenPtr[exprIndex];
	exprTokenPtr->size = parsePtr->tokenPtr[firstIndex].size;
	exprTokenPtr->numComponents =
	        (parsePtr->tokenPtr[firstIndex].numComponents + 1);
	break;
	
    case QUOTE:
	/*
	 * '"' string '"'
	 */
	
	stringStart = infoPtr->next;
	code = Tcl_ParseQuotedString(interp, infoPtr->start,
	        (infoPtr->lastChar - stringStart), parsePtr, 1, &termPtr);
	if (code != TCL_OK) {
	    return code;
	}
	infoPtr->next = termPtr;

	exprTokenPtr = &parsePtr->tokenPtr[exprIndex];
	exprTokenPtr->size = (termPtr - exprTokenPtr->start);
	exprTokenPtr->numComponents = parsePtr->numTokens - firstIndex;

	/*
	 * If parsing the quoted string resulted in more than one token,
	 * insert a TCL_TOKEN_WORD token before them. This indicates that
	 * the quoted string represents a concatenation of multiple tokens.
	 */

	if (exprTokenPtr->numComponents > 1) {
	    if (parsePtr->numTokens >= parsePtr->tokensAvailable) {
		TclExpandTokenArray(parsePtr);
	    }
	    tokenPtr = &parsePtr->tokenPtr[firstIndex];
	    numToMove = (parsePtr->numTokens - firstIndex);
	    memmove((VOID *) (tokenPtr + 1), (VOID *) tokenPtr,
	            (size_t) (numToMove * sizeof(Tcl_Token)));
	    parsePtr->numTokens++;

	    exprTokenPtr = &parsePtr->tokenPtr[exprIndex];
	    exprTokenPtr->numComponents++;

	    tokenPtr->type = TCL_TOKEN_WORD;
	    tokenPtr->start = exprTokenPtr->start;
	    tokenPtr->size = exprTokenPtr->size;
	    tokenPtr->numComponents = (exprTokenPtr->numComponents - 1);
	}
	break;
	
    case OPEN_BRACKET:
	/*
	 * '[' command {command} ']'
	 */

	if (parsePtr->numTokens == parsePtr->tokensAvailable) {
	    TclExpandTokenArray(parsePtr);
	}
	tokenPtr = &parsePtr->tokenPtr[parsePtr->numTokens];
	tokenPtr->type = TCL_TOKEN_COMMAND;
	tokenPtr->start = infoPtr->start;
	tokenPtr->numComponents = 0;
	parsePtr->numTokens++;

	/*
	 * Call Tcl_ParseCommand repeatedly to parse the nested command(s)
	 * to find their end, then throw away that parse information.
	 */
	
	src = infoPtr->next;
	while (1) {
	    if (Tcl_ParseCommand(interp, src, (parsePtr->end - src), 1,
		    &nested) != TCL_OK) {
		parsePtr->term = nested.term;
		parsePtr->errorType = nested.errorType;
		parsePtr->incomplete = nested.incomplete;
		return TCL_ERROR;
	    }
	    src = (nested.commandStart + nested.commandSize);

	    /*
	     * This is equivalent to Tcl_FreeParse(&nested), but
	     * presumably inlined here for sake of runtime optimization
	     */

	    if (nested.tokenPtr != nested.staticTokens) {
		ckfree((char *) nested.tokenPtr);
	    }

	    /*
	     * Check for the closing ']' that ends the command substitution.
	     * It must have been the last character of the parsed command.
	     */

	    if ((nested.term < parsePtr->end) && (*nested.term == ']') 
		    && !nested.incomplete) {
		break;
	    }
	    if (src == parsePtr->end) {
		if (parsePtr->interp != NULL) {
		    Tcl_SetResult(interp, "missing close-bracket",
			    TCL_STATIC);
		}
		parsePtr->term = tokenPtr->start;
		parsePtr->errorType = TCL_PARSE_MISSING_BRACKET;
		parsePtr->incomplete = 1;
		return TCL_ERROR;
	    }
	}
	tokenPtr->size = (src - tokenPtr->start);
	infoPtr->next = src;

	exprTokenPtr = &parsePtr->tokenPtr[exprIndex];
	exprTokenPtr->size = (src - tokenPtr->start);
	exprTokenPtr->numComponents = 1;
	break;

    case OPEN_BRACE:
	/*
	 * '{' string '}'
	 */

	code = Tcl_ParseBraces(interp, infoPtr->start,
	        (infoPtr->lastChar - infoPtr->start), parsePtr, 1,
		&termPtr);
	if (code != TCL_OK) {
	    return code;
	}
	infoPtr->next = termPtr;

	exprTokenPtr = &parsePtr->tokenPtr[exprIndex];
	exprTokenPtr->size = (termPtr - infoPtr->start);
	exprTokenPtr->numComponents = parsePtr->numTokens - firstIndex;

	/*
	 * If parsing the braced string resulted in more than one token,
	 * insert a TCL_TOKEN_WORD token before them. This indicates that
	 * the braced string represents a concatenation of multiple tokens.
	 */

	if (exprTokenPtr->numComponents > 1) {
	    if (parsePtr->numTokens >= parsePtr->tokensAvailable) {
		TclExpandTokenArray(parsePtr);
	    }
	    tokenPtr = &parsePtr->tokenPtr[firstIndex];
	    numToMove = (parsePtr->numTokens - firstIndex);
	    memmove((VOID *) (tokenPtr + 1), (VOID *) tokenPtr,
	            (size_t) (numToMove * sizeof(Tcl_Token)));
	    parsePtr->numTokens++;

	    exprTokenPtr = &parsePtr->tokenPtr[exprIndex];
	    exprTokenPtr->numComponents++;
	    
	    tokenPtr->type = TCL_TOKEN_WORD;
	    tokenPtr->start = exprTokenPtr->start;
	    tokenPtr->size = exprTokenPtr->size;
	    tokenPtr->numComponents = exprTokenPtr->numComponents-1;
	}
	break;
	
    case FUNC_NAME:
	/*
	 * math_func '(' expr {',' expr} ')'
	 */
	
	if (parsePtr->numTokens == parsePtr->tokensAvailable) {
	    TclExpandTokenArray(parsePtr);
	}
	tokenPtr = &parsePtr->tokenPtr[parsePtr->numTokens];
	tokenPtr->type = TCL_TOKEN_OPERATOR;
	tokenPtr->start = infoPtr->start;
	tokenPtr->size = infoPtr->size;
	tokenPtr->numComponents = 0;
	parsePtr->numTokens++;
	
	code = GetLexeme(infoPtr); /* skip over function name */
	if (code != TCL_OK) {
	    return code;
	}
	if (infoPtr->lexeme != OPEN_PAREN) {
	    /*
	     * Guess what kind of error we have by trying to tell
	     * whether we have a function or variable name here.
	     * Alas, this makes the parser more tightly bound with the
	     * rest of the interpreter, but that is the only way to
	     * give a sensible message here.  Still, it is not too
	     * serious as this is only done when generating an error.
	     */
	    Interp *iPtr = (Interp *) infoPtr->parsePtr->interp;
	    Tcl_DString functionName;
	    Tcl_HashEntry *hPtr;

	    /*
	     * Look up the name as a function name.  We need a writable
	     * copy (DString) so we can terminate it with a NULL for
	     * the benefit of Tcl_FindHashEntry which operates on
	     * NULL-terminated string keys.
	     */
	    Tcl_DStringInit(&functionName);
	    hPtr = Tcl_FindHashEntry(&iPtr->mathFuncTable, 
	    	Tcl_DStringAppend(&functionName, tokenPtr->start,
		tokenPtr->size));
	    Tcl_DStringFree(&functionName);

	    /*
	     * Assume that we have an attempted variable reference
	     * unless we've got a function name, as the set of
	     * potential function names is typically much smaller.
	     */
	    if (hPtr != NULL) {
		LogSyntaxError(infoPtr,
			"expected parenthesis enclosing function arguments");
	    } else {
		LogSyntaxError(infoPtr,
			"variable references require preceding $");
	    }
	    return TCL_ERROR;
	}
	code = GetLexeme(infoPtr); /* skip over '(' */
	if (code != TCL_OK) {
	    return code;
	}

	while (infoPtr->lexeme != CLOSE_PAREN) {
	    code = ParseCondExpr(infoPtr);
	    if (code != TCL_OK) {
		return code;
	    }
	    
	    if (infoPtr->lexeme == COMMA) {
		code = GetLexeme(infoPtr); /* skip over , */
		if (code != TCL_OK) {
		    return code;
		}
	    } else if (infoPtr->lexeme != CLOSE_PAREN) {
		LogSyntaxError(infoPtr,
			"missing close parenthesis at end of function call");
		return TCL_ERROR;
	    }
	}

	exprTokenPtr = &parsePtr->tokenPtr[exprIndex];
	exprTokenPtr->size = (infoPtr->next - exprTokenPtr->start);
	exprTokenPtr->numComponents = parsePtr->numTokens - firstIndex;
	break;

    case COMMA:
	LogSyntaxError(infoPtr,
		"commas can only separate function arguments");
	return TCL_ERROR;
    case END:
	LogSyntaxError(infoPtr, "premature end of expression");
	return TCL_ERROR;
    case UNKNOWN:
	LogSyntaxError(infoPtr, "single equality character not legal in expressions");
	return TCL_ERROR;
    case UNKNOWN_CHAR:
	LogSyntaxError(infoPtr, "character not legal in expressions");
	return TCL_ERROR;
    case QUESTY:
	LogSyntaxError(infoPtr, "unexpected ternary 'then' separator");
	return TCL_ERROR;
    case COLON:
	LogSyntaxError(infoPtr, "unexpected ternary 'else' separator");
	return TCL_ERROR;
    case CLOSE_PAREN:
	LogSyntaxError(infoPtr, "unexpected close parenthesis");
	return TCL_ERROR;

    default: {
	char buf[64];

	sprintf(buf, "unexpected operator %s", lexemeStrings[lexeme]);
	LogSyntaxError(infoPtr, buf);
	return TCL_ERROR;
	}
    }

    /*
     * Advance to the next lexeme before returning.
     */
    
    code = GetLexeme(infoPtr);
    if (code != TCL_OK) {
	return code;
    }
    parsePtr->term = infoPtr->next;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetLexeme --
 *
 *	Lexical scanner for Tcl expressions: scans a single operator or
 *	other syntactic element from an expression string.
 *
 * Results:
 *	TCL_OK is returned unless an error occurred. In that case a standard
 *	Tcl error code is returned and, if infoPtr->parsePtr->interp is
 *	non-NULL, the interpreter's result is set to hold an error
 *	message. TCL_ERROR is returned if an integer overflow, or a
 *	floating-point overflow or underflow occurred while reading in a
 *	number. If the lexical analysis is successful, infoPtr->lexeme
 *	refers to the next symbol in the expression string, and
 *	infoPtr->next is advanced past the lexeme. Also, if the lexeme is a
 *	LITERAL or FUNC_NAME, then infoPtr->start is set to the first
 *	character of the lexeme; otherwise it is set NULL.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the subexpression, then additional space is
 *	malloc-ed..
 *
 *----------------------------------------------------------------------
 */

static int
GetLexeme(infoPtr)
    ParseInfo *infoPtr;		/* Holds state needed to parse the expr,
				 * including the resulting lexeme. */
{
    register CONST char *src;	/* Points to current source char. */
    char c;
    int offset, length, numBytes;
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    Tcl_Interp *interp = parsePtr->interp;
    Tcl_UniChar ch;

    /*
     * Record where the previous lexeme ended. Since we always read one
     * lexeme ahead during parsing, this helps us know the source length of
     * subexpression tokens.
     */

    infoPtr->prevEnd = infoPtr->next;

    /*
     * Scan over leading white space at the start of a lexeme. 
     */

    src = infoPtr->next;
    numBytes = parsePtr->end - src;
    do {
	char type;
	int scanned = TclParseWhiteSpace(src, numBytes, parsePtr, &type);
	src += scanned; numBytes -= scanned;
    } while  (numBytes && (*src == '\n') && (src++,numBytes--));
    parsePtr->term = src;
    if (numBytes == 0) {
	infoPtr->lexeme = END;
	infoPtr->next = src;
	return TCL_OK;
    }

    /*
     * Try to parse the lexeme first as an integer or floating-point
     * number. Don't check for a number if the first character c is
     * "+" or "-". If we did, we might treat a binary operator as unary
     * by mistake, which would eventually cause a syntax error.
     */

    c = *src;
    if ((c != '+') && (c != '-')) {
	CONST char *end = infoPtr->lastChar;
	if ((length = TclParseInteger(src, (end - src)))) {
	    /*
	     * First length bytes look like an integer.  Verify by
	     * attempting the conversion to the largest integer we have.
	     */
	    int code;
	    Tcl_WideInt wide;
	    Tcl_Obj *value = Tcl_NewStringObj(src, length);

	    Tcl_IncrRefCount(value);
	    code = Tcl_GetWideIntFromObj(interp, value, &wide);
	    Tcl_DecrRefCount(value);
	    if (code == TCL_ERROR) {
		parsePtr->errorType = TCL_PARSE_BAD_NUMBER;
		return TCL_ERROR;
	    }
            infoPtr->lexeme = LITERAL;
	    infoPtr->start = src;
	    infoPtr->size = length;
            infoPtr->next = (src + length);
	    parsePtr->term = infoPtr->next;
            return TCL_OK;
	} else if ((length = ParseMaxDoubleLength(src, end))) {
	    /*
	     * There are length characters that could be a double.
	     * Let strtod() tells us for sure.  Need a writable copy
	     * so we can set an terminating NULL to keep strtod from
	     * scanning too far.
	     */
	    char *startPtr, *termPtr;
	    double doubleValue;
	    Tcl_DString toParse;

	    errno = 0;
	    Tcl_DStringInit(&toParse);
	    startPtr = Tcl_DStringAppend(&toParse, src, length);
	    doubleValue = strtod(startPtr, &termPtr);
	    Tcl_DStringFree(&toParse);
	    if (termPtr != startPtr) {
		if (errno != 0) {
		    if (interp != NULL) {
			TclExprFloatError(interp, doubleValue);
		    }
		    parsePtr->errorType = TCL_PARSE_BAD_NUMBER;
		    return TCL_ERROR;
		}
		
		/*
                 * startPtr was the start of a valid double, copied
		 * from src.
                 */
		
		infoPtr->lexeme = LITERAL;
		infoPtr->start = src;
		if ((termPtr - startPtr) > length) {
		    infoPtr->size = length;
		} else {
		    infoPtr->size = (termPtr - startPtr);
		}
		infoPtr->next = src + infoPtr->size;
		parsePtr->term = infoPtr->next;
		return TCL_OK;
	    }
	}
    }

    /*
     * Not an integer or double literal. Initialize the lexeme's fields
     * assuming the common case of a single character lexeme.
     */

    infoPtr->start = src;
    infoPtr->size = 1;
    infoPtr->next = src+1;
    parsePtr->term = infoPtr->next;
    
    switch (*src) {
	case '[':
	    infoPtr->lexeme = OPEN_BRACKET;
	    return TCL_OK;

        case '{':
	    infoPtr->lexeme = OPEN_BRACE;
	    return TCL_OK;

	case '(':
	    infoPtr->lexeme = OPEN_PAREN;
	    return TCL_OK;

	case ')':
	    infoPtr->lexeme = CLOSE_PAREN;
	    return TCL_OK;

	case '$':
	    infoPtr->lexeme = DOLLAR;
	    return TCL_OK;

	case '\"':
	    infoPtr->lexeme = QUOTE;
	    return TCL_OK;

	case ',':
	    infoPtr->lexeme = COMMA;
	    return TCL_OK;

	case '*':
	    infoPtr->lexeme = MULT;
	    return TCL_OK;

	case '/':
	    infoPtr->lexeme = DIVIDE;
	    return TCL_OK;

	case '%':
	    infoPtr->lexeme = MOD;
	    return TCL_OK;

	case '+':
	    infoPtr->lexeme = PLUS;
	    return TCL_OK;

	case '-':
	    infoPtr->lexeme = MINUS;
	    return TCL_OK;

	case '?':
	    infoPtr->lexeme = QUESTY;
	    return TCL_OK;

	case ':':
	    infoPtr->lexeme = COLON;
	    return TCL_OK;

	case '<':
	    infoPtr->lexeme = LESS;
	    if ((infoPtr->lastChar - src) > 1) {
		switch (src[1]) {
		    case '<':
			infoPtr->lexeme = LEFT_SHIFT;
			infoPtr->size = 2;
			infoPtr->next = src+2;
			break;
		    case '=':
			infoPtr->lexeme = LEQ;
			infoPtr->size = 2;
			infoPtr->next = src+2;
			break;
		}
	    }
	    parsePtr->term = infoPtr->next;
	    return TCL_OK;

	case '>':
	    infoPtr->lexeme = GREATER;
	    if ((infoPtr->lastChar - src) > 1) {
		switch (src[1]) {
		    case '>':
			infoPtr->lexeme = RIGHT_SHIFT;
			infoPtr->size = 2;
			infoPtr->next = src+2;
			break;
		    case '=':
			infoPtr->lexeme = GEQ;
			infoPtr->size = 2;
			infoPtr->next = src+2;
			break;
		}
	    }
	    parsePtr->term = infoPtr->next;
	    return TCL_OK;

	case '=':
	    infoPtr->lexeme = UNKNOWN;
	    if ((src[1] == '=') && ((infoPtr->lastChar - src) > 1)) {
		infoPtr->lexeme = EQUAL;
		infoPtr->size = 2;
		infoPtr->next = src+2;
	    }
	    parsePtr->term = infoPtr->next;
	    return TCL_OK;

	case '!':
	    infoPtr->lexeme = NOT;
	    if ((src[1] == '=') && ((infoPtr->lastChar - src) > 1)) {
		infoPtr->lexeme = NEQ;
		infoPtr->size = 2;
		infoPtr->next = src+2;
	    }
	    parsePtr->term = infoPtr->next;
	    return TCL_OK;

	case '&':
	    infoPtr->lexeme = BIT_AND;
	    if ((src[1] == '&') && ((infoPtr->lastChar - src) > 1)) {
		infoPtr->lexeme = AND;
		infoPtr->size = 2;
		infoPtr->next = src+2;
	    }
	    parsePtr->term = infoPtr->next;
	    return TCL_OK;

	case '^':
	    infoPtr->lexeme = BIT_XOR;
	    return TCL_OK;

	case '|':
	    infoPtr->lexeme = BIT_OR;
	    if ((src[1] == '|') && ((infoPtr->lastChar - src) > 1)) {
		infoPtr->lexeme = OR;
		infoPtr->size = 2;
		infoPtr->next = src+2;
	    }
	    parsePtr->term = infoPtr->next;
	    return TCL_OK;

	case '~':
	    infoPtr->lexeme = BIT_NOT;
	    return TCL_OK;

	case 'e':
	    if ((src[1] == 'q') && ((infoPtr->lastChar - src) > 1)) {
		infoPtr->lexeme = STREQ;
		infoPtr->size = 2;
		infoPtr->next = src+2;
		parsePtr->term = infoPtr->next;
		return TCL_OK;
	    } else {
		goto checkFuncName;
	    }

	case 'n':
	    if ((src[1] == 'e') && ((infoPtr->lastChar - src) > 1)) {
		infoPtr->lexeme = STRNEQ;
		infoPtr->size = 2;
		infoPtr->next = src+2;
		parsePtr->term = infoPtr->next;
		return TCL_OK;
	    } else {
		goto checkFuncName;
	    }

	default:
	checkFuncName:
	    length = (infoPtr->lastChar - src);
	    if (Tcl_UtfCharComplete(src, length)) {
		offset = Tcl_UtfToUniChar(src, &ch);
	    } else {
		char utfBytes[TCL_UTF_MAX];
		memcpy(utfBytes, src, (size_t) length);
		utfBytes[length] = '\0';
		offset = Tcl_UtfToUniChar(utfBytes, &ch);
	    }
	    c = UCHAR(ch);
	    if (isalpha(UCHAR(c))) {	/* INTL: ISO only. */
		infoPtr->lexeme = FUNC_NAME;
		while (isalnum(UCHAR(c)) || (c == '_')) { /* INTL: ISO only. */
		    src += offset; length -= offset;
		    if (Tcl_UtfCharComplete(src, length)) {
			offset = Tcl_UtfToUniChar(src, &ch);
		    } else {
			char utfBytes[TCL_UTF_MAX];
			memcpy(utfBytes, src, (size_t) length);
			utfBytes[length] = '\0';
			offset = Tcl_UtfToUniChar(utfBytes, &ch);
		    }
		    c = UCHAR(ch);
		}
		infoPtr->size = (src - infoPtr->start);
		infoPtr->next = src;
		parsePtr->term = infoPtr->next;
		/*
		 * Check for boolean literals (true, false, yes, no, on, off)
		 */
		switch (infoPtr->start[0]) {
		case 'f':
		    if (infoPtr->size == 5 &&
			strncmp("false", infoPtr->start, 5) == 0) {
			infoPtr->lexeme = LITERAL;
			return TCL_OK;
		    }
		    break;
		case 'n':
		    if (infoPtr->size == 2 &&
			strncmp("no", infoPtr->start, 2) == 0) {
			infoPtr->lexeme = LITERAL;
			return TCL_OK;
		    }
		    break;
		case 'o':
		    if (infoPtr->size == 3 &&
			strncmp("off", infoPtr->start, 3) == 0) {
			infoPtr->lexeme = LITERAL;
			return TCL_OK;
		    } else if (infoPtr->size == 2 &&
			strncmp("on", infoPtr->start, 2) == 0) {
			infoPtr->lexeme = LITERAL;
			return TCL_OK;
		    }
		    break;
		case 't':
		    if (infoPtr->size == 4 &&
			strncmp("true", infoPtr->start, 4) == 0) {
			infoPtr->lexeme = LITERAL;
			return TCL_OK;
		    }
		    break;
		case 'y':
		    if (infoPtr->size == 3 &&
			strncmp("yes", infoPtr->start, 3) == 0) {
			infoPtr->lexeme = LITERAL;
			return TCL_OK;
		    }
		    break;
		}
		return TCL_OK;
	    }
	    infoPtr->lexeme = UNKNOWN_CHAR;
	    return TCL_OK;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclParseInteger --
 *
 *	Scans up to numBytes bytes starting at src, and checks whether
 *	the leading bytes look like an integer's string representation.
 *
 * Results:
 *	Returns 0 if the leading bytes do not look like an integer.
 *	Otherwise, returns the number of bytes examined that look
 *	like an integer.  This may be less than numBytes if the integer
 *	is only the leading part of the string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclParseInteger(string, numBytes)
    register CONST char *string;/* The string to examine. */
    register int numBytes;	/* Max number of bytes to scan. */
{
    register CONST char *p = string;

    /* Take care of introductory "0x" */
    if ((numBytes > 1) && (p[0] == '0') && ((p[1] == 'x') || (p[1] == 'X'))) {
	int scanned;
	Tcl_UniChar ch;
	p+=2; numBytes -= 2;
 	scanned = TclParseHex(p, numBytes, &ch);
	if (scanned) {
	    return scanned + 2;
	}

	/* Recognize the 0 as valid integer, but x is left behind */
	return 1;
    }
    while (numBytes && isdigit(UCHAR(*p))) {	/* INTL: digit */
	numBytes--; p++;
    }
    if (numBytes == 0) {
        return (p - string);
    }
    if ((*p != '.') && (*p != 'e') && (*p != 'E')) {
        return (p - string);
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseMaxDoubleLength --
 *
 *      Scans a sequence of bytes checking that the characters could
 *	be in a string rep of a double.
 *
 * Results:
 *	Returns the number of bytes starting with string, runing to, but
 *	not including end, all of which could be part of a string rep.
 *	of a double.  Only character identity is used, no actual
 *	parsing is done.
 *
 *	The legal bytes are '0' - '9', 'A' - 'F', 'a' - 'f', 
 *	'.', '+', '-', 'i', 'I', 'n', 'N', 'p', 'P', 'x',  and 'X'.
 *	This covers the values "Inf" and "Nan" as well as the
 *	decimal and hexadecimal representations recognized by a
 *	C99-compliant strtod().
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ParseMaxDoubleLength(string, end)
    register CONST char *string;/* The string to examine. */
    CONST char *end;		/* Point to the first character past the end
				 * of the string we are examining. */
{
    CONST char *p = string;
    while (p < end) {
	switch (*p) {
	    case '0': case '1': case '2': case '3': case '4': case '5':
	    case '6': case '7': case '8': case '9': case 'A': case 'B':
	    case 'C': case 'D': case 'E': case 'F': case 'I': case 'N':
	    case 'P': case 'X': case 'a': case 'b': case 'c': case 'd':
	    case 'e': case 'f': case 'i': case 'n': case 'p': case 'x':
	    case '.': case '+': case '-':
		p++;
		break;
	    default:
		goto done;
	}
    }
    done:
    return (p - string);
}

/*
 *----------------------------------------------------------------------
 *
 * PrependSubExprTokens --
 *
 *	This procedure is called after the operands of an subexpression have
 *	been parsed. It generates two tokens: a TCL_TOKEN_SUB_EXPR token for
 *	the subexpression, and a TCL_TOKEN_OPERATOR token for its operator.
 *	These two tokens are inserted before the operand tokens.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold the new tokens,
 *	additional space is malloc-ed.
 *
 *----------------------------------------------------------------------
 */

static void
PrependSubExprTokens(op, opBytes, src, srcBytes, firstIndex, infoPtr)
    CONST char *op;		/* Points to first byte of the operator
				 * in the source script. */
    int opBytes;		/* Number of bytes in the operator. */
    CONST char *src;		/* Points to first byte of the subexpression
				 * in the source script. */
    int srcBytes;		/* Number of bytes in subexpression's
				 * source. */
    int firstIndex;		/* Index of first token already emitted for
				 * operator's first (or only) operand. */
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
{
    Tcl_Parse *parsePtr = infoPtr->parsePtr;
    Tcl_Token *tokenPtr, *firstTokenPtr;
    int numToMove;

    if ((parsePtr->numTokens + 1) >= parsePtr->tokensAvailable) {
	TclExpandTokenArray(parsePtr);
    }
    firstTokenPtr = &parsePtr->tokenPtr[firstIndex];
    tokenPtr = (firstTokenPtr + 2);
    numToMove = (parsePtr->numTokens - firstIndex);
    memmove((VOID *) tokenPtr, (VOID *) firstTokenPtr,
            (size_t) (numToMove * sizeof(Tcl_Token)));
    parsePtr->numTokens += 2;
    
    tokenPtr = firstTokenPtr;
    tokenPtr->type = TCL_TOKEN_SUB_EXPR;
    tokenPtr->start = src;
    tokenPtr->size = srcBytes;
    tokenPtr->numComponents = parsePtr->numTokens - (firstIndex + 1);
    
    tokenPtr++;
    tokenPtr->type = TCL_TOKEN_OPERATOR;
    tokenPtr->start = op;
    tokenPtr->size = opBytes;
    tokenPtr->numComponents = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * LogSyntaxError --
 *
 *	This procedure is invoked after an error occurs when parsing an
 *	expression. It sets the interpreter result to an error message
 *	describing the error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the interpreter result to an error message describing the
 *	expression that was being parsed when the error occurred, and why
 *	the parser considers that to be a syntax error at all.
 *
 *----------------------------------------------------------------------
 */

static void
LogSyntaxError(infoPtr, extraInfo)
    ParseInfo *infoPtr;		/* Holds the parse state for the
				 * expression being parsed. */
    CONST char *extraInfo;	/* String to provide extra information
				 * about the syntax error. */
{
    int numBytes = (infoPtr->lastChar - infoPtr->originalExpr);
    char buffer[100];

    if (numBytes > 60) {
	sprintf(buffer, "syntax error in expression \"%.60s...\"",
		infoPtr->originalExpr);
    } else {
	sprintf(buffer, "syntax error in expression \"%.*s\"",
		numBytes, infoPtr->originalExpr);
    }
    Tcl_ResetResult(infoPtr->parsePtr->interp);
    Tcl_AppendStringsToObj(Tcl_GetObjResult(infoPtr->parsePtr->interp),
	    buffer, ": ", extraInfo, (char *) NULL);
    infoPtr->parsePtr->errorType = TCL_PARSE_SYNTAX;
    infoPtr->parsePtr->term = infoPtr->start;
}
