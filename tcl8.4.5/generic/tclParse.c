/* 
 * tclParse.c --
 *
 *	This file contains procedures that parse Tcl scripts.  They
 *	do so in a general-purpose fashion that can be used for many
 *	different purposes, including compilation, direct execution,
 *	code analysis, etc.  
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 Ajuba Solutions.
 * Contributions from Don Porter, NIST, 2002. (not subject to US copyright)
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"

/*
 * The following table provides parsing information about each possible
 * 8-bit character.  The table is designed to be referenced with either
 * signed or unsigned characters, so it has 384 entries.  The first 128
 * entries correspond to negative character values, the next 256 correspond
 * to positive character values.  The last 128 entries are identical to the
 * first 128.  The table is always indexed with a 128-byte offset (the 128th
 * entry corresponds to a character value of 0).
 *
 * The macro CHAR_TYPE is used to index into the table and return
 * information about its character argument.  The following return
 * values are defined.
 *
 * TYPE_NORMAL -        All characters that don't have special significance
 *                      to the Tcl parser.
 * TYPE_SPACE -         The character is a whitespace character other
 *                      than newline.
 * TYPE_COMMAND_END -   Character is newline or semicolon.
 * TYPE_SUBS -          Character begins a substitution or has other
 *                      special meaning in ParseTokens: backslash, dollar
 *                      sign, or open bracket.
 * TYPE_QUOTE -         Character is a double quote.
 * TYPE_CLOSE_PAREN -   Character is a right parenthesis.
 * TYPE_CLOSE_BRACK -   Character is a right square bracket.
 * TYPE_BRACE -         Character is a curly brace (either left or right).
 */

#define TYPE_NORMAL             0
#define TYPE_SPACE              0x1
#define TYPE_COMMAND_END        0x2
#define TYPE_SUBS               0x4
#define TYPE_QUOTE              0x8
#define TYPE_CLOSE_PAREN        0x10
#define TYPE_CLOSE_BRACK        0x20
#define TYPE_BRACE              0x40

#define CHAR_TYPE(c) (charTypeTable+128)[(int)(c)]

static CONST char charTypeTable[] = {
    /*
     * Negative character values, from -128 to -1:
     */

    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,

    /*
     * Positive character values, from 0-127:
     */

    TYPE_SUBS,        TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_SPACE,       TYPE_COMMAND_END, TYPE_SPACE,
    TYPE_SPACE,       TYPE_SPACE,       TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_SPACE,       TYPE_NORMAL,      TYPE_QUOTE,       TYPE_NORMAL,
    TYPE_SUBS,        TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_CLOSE_PAREN, TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_COMMAND_END,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_SUBS,
    TYPE_SUBS,        TYPE_CLOSE_BRACK, TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_BRACE,
    TYPE_NORMAL,      TYPE_BRACE,       TYPE_NORMAL,      TYPE_NORMAL,

    /*
     * Large unsigned character values, from 128-255:
     */

    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
    TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,      TYPE_NORMAL,
};

/*
 * Prototypes for local procedures defined in this file:
 */

static int		CommandComplete _ANSI_ARGS_((CONST char *script,
			    int numBytes));
static int		ParseComment _ANSI_ARGS_((CONST char *src, int numBytes,
			    Tcl_Parse *parsePtr));
static int		ParseTokens _ANSI_ARGS_((CONST char *src, int numBytes,
			    int mask, Tcl_Parse *parsePtr));

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseCommand --
 *
 *	Given a string, this procedure parses the first Tcl command
 *	in the string and returns information about the structure of
 *	the command.
 *
 * Results:
 *	The return value is TCL_OK if the command was parsed
 *	successfully and TCL_ERROR otherwise.  If an error occurs
 *	and interp isn't NULL then an error message is left in
 *	its result.  On a successful return, parsePtr is filled in
 *	with information about the command that was parsed.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the command, then additional space is
 *	malloc-ed.  If the procedure returns TCL_OK then the caller must
 *	eventually invoke Tcl_FreeParse to release any additional space
 *	that was allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ParseCommand(interp, string, numBytes, nested, parsePtr)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting;
				 * if NULL, then no error message is
				 * provided. */
    CONST char *string;		/* First character of string containing
				 * one or more Tcl commands. */
    register int numBytes;	/* Total number of bytes in string.  If < 0,
				 * the script consists of all bytes up to 
				 * the first null character. */
    int nested;			/* Non-zero means this is a nested command:
				 * close bracket should be considered
				 * a command terminator. If zero, then close
				 * bracket has no special meaning. */
    register Tcl_Parse *parsePtr;
    				/* Structure to fill in with information
				 * about the parsed command; any previous
				 * information in the structure is
				 * ignored. */
{
    register CONST char *src;	/* Points to current character
				 * in the command. */
    char type;			/* Result returned by CHAR_TYPE(*src). */
    Tcl_Token *tokenPtr;	/* Pointer to token being filled in. */
    int wordIndex;		/* Index of word token for current word. */
    int terminators;		/* CHAR_TYPE bits that indicate the end
				 * of a command. */
    CONST char *termPtr;	/* Set by Tcl_ParseBraces/QuotedString to
				 * point to char after terminating one. */
    int scanned;
    
    if ((string == NULL) && (numBytes>0)) {
	if (interp != NULL) {
	    Tcl_SetResult(interp, "can't parse a NULL pointer", TCL_STATIC);
	}
	return TCL_ERROR;
    }
    if (numBytes < 0) {
	numBytes = strlen(string);
    }
    parsePtr->commentStart = NULL;
    parsePtr->commentSize = 0;
    parsePtr->commandStart = NULL;
    parsePtr->commandSize = 0;
    parsePtr->numWords = 0;
    parsePtr->tokenPtr = parsePtr->staticTokens;
    parsePtr->numTokens = 0;
    parsePtr->tokensAvailable = NUM_STATIC_TOKENS;
    parsePtr->string = string;
    parsePtr->end = string + numBytes;
    parsePtr->term = parsePtr->end;
    parsePtr->interp = interp;
    parsePtr->incomplete = 0;
    parsePtr->errorType = TCL_PARSE_SUCCESS;
    if (nested != 0) {
	terminators = TYPE_COMMAND_END | TYPE_CLOSE_BRACK;
    } else {
	terminators = TYPE_COMMAND_END;
    }

    /*
     * Parse any leading space and comments before the first word of the
     * command.
     */

    scanned = ParseComment(string, numBytes, parsePtr);
    src = (string + scanned); numBytes -= scanned;
    if (numBytes == 0) {
	if (nested) {
	    parsePtr->incomplete = nested;
	}
    }

    /*
     * The following loop parses the words of the command, one word
     * in each iteration through the loop.
     */

    parsePtr->commandStart = src;
    while (1) {
	/*
	 * Create the token for the word.
	 */

	if (parsePtr->numTokens == parsePtr->tokensAvailable) {
	    TclExpandTokenArray(parsePtr);
	}
	wordIndex = parsePtr->numTokens;
	tokenPtr = &parsePtr->tokenPtr[wordIndex];
	tokenPtr->type = TCL_TOKEN_WORD;

	/*
	 * Skip white space before the word. Also skip a backslash-newline
	 * sequence: it should be treated just like white space.
	 */

	scanned = TclParseWhiteSpace(src, numBytes, parsePtr, &type);
	src += scanned; numBytes -= scanned;
	if (numBytes == 0) {
	    parsePtr->term = src;
	    break;
	}
	if ((type & terminators) != 0) {
	    parsePtr->term = src;
	    src++;
	    break;
	}
	tokenPtr->start = src;
	parsePtr->numTokens++;
	parsePtr->numWords++;

	/*
	 * At this point the word can have one of three forms: something
	 * enclosed in quotes, something enclosed in braces, or an
	 * unquoted word (anything else).
	 */

	if (*src == '"') {
	    if (Tcl_ParseQuotedString(interp, src, numBytes,
		    parsePtr, 1, &termPtr) != TCL_OK) {
		goto error;
	    }
	    src = termPtr; numBytes = parsePtr->end - src;
	} else if (*src == '{') {
	    if (Tcl_ParseBraces(interp, src, numBytes,
		    parsePtr, 1, &termPtr) != TCL_OK) {
		goto error;
	    }
	    src = termPtr; numBytes = parsePtr->end - src;
	} else {
	    /*
	     * This is an unquoted word.  Call ParseTokens and let it do
	     * all of the work.
	     */

	    if (ParseTokens(src, numBytes, TYPE_SPACE|terminators,
		    parsePtr) != TCL_OK) {
		goto error;
	    }
	    src = parsePtr->term; numBytes = parsePtr->end - src;
	}

	/*
	 * Finish filling in the token for the word and check for the
	 * special case of a word consisting of a single range of
	 * literal text.
	 */

	tokenPtr = &parsePtr->tokenPtr[wordIndex];
	tokenPtr->size = src - tokenPtr->start;
	tokenPtr->numComponents = parsePtr->numTokens - (wordIndex + 1);
	if ((tokenPtr->numComponents == 1)
		&& (tokenPtr[1].type == TCL_TOKEN_TEXT)) {
	    tokenPtr->type = TCL_TOKEN_SIMPLE_WORD;
	}

	/*
	 * Do two additional checks: (a) make sure we're really at the
	 * end of a word (there might have been garbage left after a
	 * quoted or braced word), and (b) check for the end of the
	 * command.
	 */

	scanned = TclParseWhiteSpace(src, numBytes, parsePtr, &type);
	if (scanned) {
	    src += scanned; numBytes -= scanned;
	    continue;
	}

	if (numBytes == 0) {
	    parsePtr->term = src;
	    break;
	}
	if ((type & terminators) != 0) {
	    parsePtr->term = src;
	    src++; 
	    break;
	}
	if (src[-1] == '"') { 
	    if (interp != NULL) {
		Tcl_SetResult(interp, "extra characters after close-quote",
			TCL_STATIC);
	    }
	    parsePtr->errorType = TCL_PARSE_QUOTE_EXTRA;
	} else {
	    if (interp != NULL) {
		Tcl_SetResult(interp, "extra characters after close-brace",
			TCL_STATIC);
	    }
	    parsePtr->errorType = TCL_PARSE_BRACE_EXTRA;
	}
	parsePtr->term = src;
	goto error;
    }

    parsePtr->commandSize = src - parsePtr->commandStart;
    return TCL_OK;

    error:
    Tcl_FreeParse(parsePtr);
    if (parsePtr->commandStart == NULL) {
	parsePtr->commandStart = string;
    }
    parsePtr->commandSize = parsePtr->end - parsePtr->commandStart;
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TclParseWhiteSpace --
 *
 *	Scans up to numBytes bytes starting at src, consuming white
 *	space as defined by Tcl's parsing rules.  
 *
 * Results:
 *	Returns the number of bytes recognized as white space.  Records
 *	at parsePtr, information about the parse.  Records at typePtr
 *	the character type of the non-whitespace character that terminated
 *	the scan.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
TclParseWhiteSpace(src, numBytes, parsePtr, typePtr)
    CONST char *src;		/* First character to parse. */
    register int numBytes;	/* Max number of bytes to scan. */
    Tcl_Parse *parsePtr;	/* Information about parse in progress.
				 * Updated if parsing indicates
				 * an incomplete command. */
    char *typePtr;		/* Points to location to store character
				 * type of character that ends run
				 * of whitespace */
{
    register char type = TYPE_NORMAL;
    register CONST char *p = src;

    while (1) {
	while (numBytes && ((type = CHAR_TYPE(*p)) & TYPE_SPACE)) {
	    numBytes--; p++;
	}
	if (numBytes && (type & TYPE_SUBS)) {
	    if (*p != '\\') {
		break;
	    }
	    if (--numBytes == 0) {
		break;
	    }
	    if (p[1] != '\n') {
		break;
	    }
	    p+=2;
	    if (--numBytes == 0) {
		parsePtr->incomplete = 1;
		break;
	    }
	    continue;
	}
	break;
    }
    *typePtr = type;
    return (p - src);
}

/*
 *----------------------------------------------------------------------
 *
 * TclParseHex --
 *
 *	Scans a hexadecimal number as a Tcl_UniChar value.
 *	(e.g., for parsing \x and \u escape sequences).
 *	At most numBytes bytes are scanned.
 *
 * Results:
 *	The numeric value is stored in *resultPtr.
 *	Returns the number of bytes consumed.
 *
 * Notes:
 *	Relies on the following properties of the ASCII
 *	character set, with which UTF-8 is compatible:
 *
 *	The digits '0' .. '9' and the letters 'A' .. 'Z' and 'a' .. 'z' 
 *	occupy consecutive code points, and '0' < 'A' < 'a'.
 *
 *----------------------------------------------------------------------
 */
int
TclParseHex(src, numBytes, resultPtr)
    CONST char *src;		/* First character to parse. */
    int numBytes;		/* Max number of byes to scan */
    Tcl_UniChar *resultPtr;	/* Points to storage provided by
				 * caller where the Tcl_UniChar
				 * resulting from the conversion is
				 * to be written. */
{
    Tcl_UniChar result = 0;
    register CONST char *p = src;

    while (numBytes--) {
	unsigned char digit = UCHAR(*p);

	if (!isxdigit(digit))
	    break;

	++p;
	result <<= 4;

	if (digit >= 'a') {
	    result |= (10 + digit - 'a');
	} else if (digit >= 'A') {
	    result |= (10 + digit - 'A');
	} else {
	    result |= (digit - '0');
	}
    }

    *resultPtr = result;
    return (p - src);
}

/*
 *----------------------------------------------------------------------
 *
 * TclParseBackslash --
 *
 *	Scans up to numBytes bytes starting at src, consuming a
 *	backslash sequence as defined by Tcl's parsing rules.  
 *
 * Results:
 * 	Records at readPtr the number of bytes making up the backslash
 * 	sequence.  Records at dst the UTF-8 encoded equivalent of
 * 	that backslash sequence.  Returns the number of bytes written
 * 	to dst, at most TCL_UTF_MAX.  Either readPtr or dst may be
 * 	NULL, if the results are not needed, but the return value is
 * 	the same either way.
 *
 * Side effects:
 * 	None.
 *
 *----------------------------------------------------------------------
 */
int
TclParseBackslash(src, numBytes, readPtr, dst)
    CONST char * src;	/* Points to the backslash character of a
			 * a backslash sequence */
    int numBytes;	/* Max number of bytes to scan */
    int *readPtr;	/* NULL, or points to storage where the
			 * number of bytes scanned should be written. */
    char *dst;		/* NULL, or points to buffer where the UTF-8
			 * encoding of the backslash sequence is to be
			 * written.  At most TCL_UTF_MAX bytes will be
			 * written there. */
{
    register CONST char *p = src+1;
    Tcl_UniChar result;
    int count;
    char buf[TCL_UTF_MAX];

    if (numBytes == 0) {
	if (readPtr != NULL) {
	    *readPtr = 0;
	}
	return 0;
    }

    if (dst == NULL) {
        dst = buf;
    }

    if (numBytes == 1) {
	/* Can only scan the backslash.  Return it. */
	result = '\\';
	count = 1;
	goto done;
    }

    count = 2;
    switch (*p) {
        /*
         * Note: in the conversions below, use absolute values (e.g.,
         * 0xa) rather than symbolic values (e.g. \n) that get converted
         * by the compiler.  It's possible that compilers on some
         * platforms will do the symbolic conversions differently, which
         * could result in non-portable Tcl scripts.
         */

        case 'a':
            result = 0x7;
            break;
        case 'b':
            result = 0x8;
            break;
        case 'f':
            result = 0xc;
            break;
        case 'n':
            result = 0xa;
            break;
        case 'r':
            result = 0xd;
            break;
        case 't':
            result = 0x9;
            break;
        case 'v':
            result = 0xb;
            break;
        case 'x':
	    count += TclParseHex(p+1, numBytes-1, &result);
	    if (count == 2) {
		/* No hexadigits -> This is just "x". */
		result = 'x';
	    } else {
		/* Keep only the last byte (2 hex digits) */
		result = (unsigned char) result;
	    }
            break;
        case 'u':
	    count += TclParseHex(p+1, (numBytes > 5) ? 4 : numBytes-1, &result);
	    if (count == 2) {
		/* No hexadigits -> This is just "u". */
		result = 'u';
	    }
            break;
        case '\n':
            count--;
            do {
                p++; count++;
            } while ((count < numBytes) && ((*p == ' ') || (*p == '\t')));
            result = ' ';
            break;
        case 0:
            result = '\\';
            count = 1;
            break;
        default:
            /*
             * Check for an octal number \oo?o?
             */
            if (isdigit(UCHAR(*p)) && (UCHAR(*p) < '8')) { /* INTL: digit */
                result = (unsigned char)(*p - '0');
                p++;
                if ((numBytes == 2) || !isdigit(UCHAR(*p)) /* INTL: digit */
			|| (UCHAR(*p) >= '8')) { 
                    break;
                }
                count = 3;
                result = (unsigned char)((result << 3) + (*p - '0'));
                p++;
                if ((numBytes == 3) || !isdigit(UCHAR(*p)) /* INTL: digit */
			|| (UCHAR(*p) >= '8')) {
                    break;
                }
                count = 4;
                result = (unsigned char)((result << 3) + (*p - '0'));
                break;
            }
            /*
             * We have to convert here in case the user has put a
             * backslash in front of a multi-byte utf-8 character.
             * While this means nothing special, we shouldn't break up
             * a correct utf-8 character. [Bug #217987] test subst-3.2
             */
	    if (Tcl_UtfCharComplete(p, numBytes - 1)) {
	        count = Tcl_UtfToUniChar(p, &result) + 1; /* +1 for '\' */
	    } else {
		char utfBytes[TCL_UTF_MAX];
		memcpy(utfBytes, p, (size_t) (numBytes - 1));
		utfBytes[numBytes - 1] = '\0';
	        count = Tcl_UtfToUniChar(utfBytes, &result) + 1;
	    }
            break;
    }

    done:
    if (readPtr != NULL) {
        *readPtr = count;
    }
    return Tcl_UniCharToUtf((int) result, dst);
}

/*
 *----------------------------------------------------------------------
 *
 * ParseComment --
 *
 *	Scans up to numBytes bytes starting at src, consuming a
 *	Tcl comment as defined by Tcl's parsing rules.  
 *
 * Results:
 * 	Records in parsePtr information about the parse.  Returns the
 * 	number of bytes consumed.
 *
 * Side effects:
 * 	None.
 *
 *----------------------------------------------------------------------
 */
static int
ParseComment(src, numBytes, parsePtr)
    CONST char *src;		/* First character to parse. */
    register int numBytes;	/* Max number of bytes to scan. */
    Tcl_Parse *parsePtr;	/* Information about parse in progress.
				 * Updated if parsing indicates
				 * an incomplete command. */
{
    register CONST char *p = src;
    while (numBytes) {
	char type;
	int scanned;
	do {
	    scanned = TclParseWhiteSpace(p, numBytes, parsePtr, &type);
	    p += scanned; numBytes -= scanned;
	} while (numBytes && (*p == '\n') && (p++,numBytes--));
	if ((numBytes == 0) || (*p != '#')) {
	    break;
	}
	if (parsePtr->commentStart == NULL) {
	    parsePtr->commentStart = p;
	}
	while (numBytes) {
	    if (*p == '\\') {
		scanned = TclParseWhiteSpace(p, numBytes, parsePtr, &type);
		if (scanned) {
		    p += scanned; numBytes -= scanned;
		} else {
		    /*
		     * General backslash substitution in comments isn't
		     * part of the formal spec, but test parse-15.47
		     * and history indicate that it has been the de facto
		     * rule.  Don't change it now.
		     */
		    TclParseBackslash(p, numBytes, &scanned, NULL);
		    p += scanned; numBytes -= scanned;
		}
	    } else {
		p++; numBytes--;
		if (p[-1] == '\n') {
		    break;
		}
	    }
	}
	parsePtr->commentSize = p - parsePtr->commentStart;
    }
    return (p - src);
}

/*
 *----------------------------------------------------------------------
 *
 * ParseTokens --
 *
 *	This procedure forms the heart of the Tcl parser.  It parses one
 *	or more tokens from a string, up to a termination point
 *	specified by the caller.  This procedure is used to parse
 *	unquoted command words (those not in quotes or braces), words in
 *	quotes, and array indices for variables.  No more than numBytes
 *	bytes will be scanned.
 *
 * Results:
 *	Tokens are added to parsePtr and parsePtr->term is filled in
 *	with the address of the character that terminated the parse (the
 *	first one whose CHAR_TYPE matched mask or the character at
 *	parsePtr->end).  The return value is TCL_OK if the parse
 *	completed successfully and TCL_ERROR otherwise.  If a parse
 *	error occurs and parsePtr->interp isn't NULL, then an error
 *	message is left in the interpreter's result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ParseTokens(src, numBytes, mask, parsePtr)
    register CONST char *src;	/* First character to parse. */
    register int numBytes;	/* Max number of bytes to scan. */
    int mask;			/* Specifies when to stop parsing.  The
				 * parse stops at the first unquoted
				 * character whose CHAR_TYPE contains
				 * any of the bits in mask. */
    Tcl_Parse *parsePtr;	/* Information about parse in progress.
				 * Updated with additional tokens and
				 * termination information. */
{
    char type; 
    int originalTokens, varToken;
    Tcl_Token *tokenPtr;
    Tcl_Parse nested;

    /*
     * Each iteration through the following loop adds one token of
     * type TCL_TOKEN_TEXT, TCL_TOKEN_BS, TCL_TOKEN_COMMAND, or
     * TCL_TOKEN_VARIABLE to parsePtr.  For TCL_TOKEN_VARIABLE tokens,
     * additional tokens are added for the parsed variable name.
     */

    originalTokens = parsePtr->numTokens;
    while (numBytes && !((type = CHAR_TYPE(*src)) & mask)) {
	if (parsePtr->numTokens == parsePtr->tokensAvailable) {
	    TclExpandTokenArray(parsePtr);
	}
	tokenPtr = &parsePtr->tokenPtr[parsePtr->numTokens];
	tokenPtr->start = src;
	tokenPtr->numComponents = 0;

	if ((type & TYPE_SUBS) == 0) {
	    /*
	     * This is a simple range of characters.  Scan to find the end
	     * of the range.
	     */

	    while ((++src, --numBytes) 
		    && !(CHAR_TYPE(*src) & (mask | TYPE_SUBS))) {
		/* empty loop */
	    }
	    tokenPtr->type = TCL_TOKEN_TEXT;
	    tokenPtr->size = src - tokenPtr->start;
	    parsePtr->numTokens++;
	} else if (*src == '$') {
	    /*
	     * This is a variable reference.  Call Tcl_ParseVarName to do
	     * all the dirty work of parsing the name.
	     */

	    varToken = parsePtr->numTokens;
	    if (Tcl_ParseVarName(parsePtr->interp, src, numBytes,
		    parsePtr, 1) != TCL_OK) {
		return TCL_ERROR;
	    }
	    src += parsePtr->tokenPtr[varToken].size;
	    numBytes -= parsePtr->tokenPtr[varToken].size;
	} else if (*src == '[') {
	    /*
	     * Command substitution.  Call Tcl_ParseCommand recursively
	     * (and repeatedly) to parse the nested command(s), then
	     * throw away the parse information.
	     */

	    src++; numBytes--;
	    while (1) {
		if (Tcl_ParseCommand(parsePtr->interp, src,
			numBytes, 1, &nested) != TCL_OK) {
		    parsePtr->errorType = nested.errorType;
		    parsePtr->term = nested.term;
		    parsePtr->incomplete = nested.incomplete;
		    return TCL_ERROR;
		}
		src = nested.commandStart + nested.commandSize;
		numBytes = parsePtr->end - src;

		/*
		 * This is equivalent to Tcl_FreeParse(&nested), but
		 * presumably inlined here for sake of runtime optimization
		 */

		if (nested.tokenPtr != nested.staticTokens) {
		    ckfree((char *) nested.tokenPtr);
		}

		/*
		 * Check for the closing ']' that ends the command
		 * substitution.  It must have been the last character of
		 * the parsed command.
		 */

		if ((nested.term < parsePtr->end) && (*nested.term == ']')
			&& !nested.incomplete) {
		    break;
		}
		if (numBytes == 0) {
		    if (parsePtr->interp != NULL) {
			Tcl_SetResult(parsePtr->interp,
			    "missing close-bracket", TCL_STATIC);
		    }
		    parsePtr->errorType = TCL_PARSE_MISSING_BRACKET;
		    parsePtr->term = tokenPtr->start;
		    parsePtr->incomplete = 1;
		    return TCL_ERROR;
		}
	    }
	    tokenPtr->type = TCL_TOKEN_COMMAND;
	    tokenPtr->size = src - tokenPtr->start;
	    parsePtr->numTokens++;
	} else if (*src == '\\') {
	    /*
	     * Backslash substitution.
	     */
	    TclParseBackslash(src, numBytes, &tokenPtr->size, NULL);

	    if (tokenPtr->size == 1) {
		/* Just a backslash, due to end of string */
		tokenPtr->type = TCL_TOKEN_TEXT;
		parsePtr->numTokens++;
		src++; numBytes--;
		continue;
	    }

	    if (src[1] == '\n') {
		if (numBytes == 2) {
		    parsePtr->incomplete = 1;
		}

		/*
		 * Note: backslash-newline is special in that it is
		 * treated the same as a space character would be.  This
		 * means that it could terminate the token.
		 */

		if (mask & TYPE_SPACE) {
		    if (parsePtr->numTokens == originalTokens) {
			goto finishToken;
		    }
		    break;
		}
	    }

	    tokenPtr->type = TCL_TOKEN_BS;
	    parsePtr->numTokens++;
	    src += tokenPtr->size;
	    numBytes -= tokenPtr->size;
	} else if (*src == 0) {
	    tokenPtr->type = TCL_TOKEN_TEXT;
	    tokenPtr->size = 1;
	    parsePtr->numTokens++;
	    src++; numBytes--;
	} else {
	    panic("ParseTokens encountered unknown character");
	}
    }
    if (parsePtr->numTokens == originalTokens) {
	/*
	 * There was nothing in this range of text.  Add an empty token
	 * for the empty range, so that there is always at least one
	 * token added.
	 */
	if (parsePtr->numTokens == parsePtr->tokensAvailable) {
	    TclExpandTokenArray(parsePtr);
	}
	tokenPtr = &parsePtr->tokenPtr[parsePtr->numTokens];
	tokenPtr->start = src;
	tokenPtr->numComponents = 0;

	finishToken:
	tokenPtr->type = TCL_TOKEN_TEXT;
	tokenPtr->size = 0;
	parsePtr->numTokens++;
    }
    parsePtr->term = src;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_FreeParse --
 *
 *	This procedure is invoked to free any dynamic storage that may
 *	have been allocated by a previous call to Tcl_ParseCommand.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is any dynamically allocated memory in *parsePtr,
 *	it is freed.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_FreeParse(parsePtr)
    Tcl_Parse *parsePtr;	/* Structure that was filled in by a
				 * previous call to Tcl_ParseCommand. */
{
    if (parsePtr->tokenPtr != parsePtr->staticTokens) {
	ckfree((char *) parsePtr->tokenPtr);
	parsePtr->tokenPtr = parsePtr->staticTokens;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclExpandTokenArray --
 *
 *	This procedure is invoked when the current space for tokens in
 *	a Tcl_Parse structure fills up; it allocates memory to grow the
 *	token array
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is allocated for a new larger token array; the memory
 *	for the old array is freed, if it had been dynamically allocated.
 *
 *----------------------------------------------------------------------
 */

void
TclExpandTokenArray(parsePtr)
    Tcl_Parse *parsePtr;	/* Parse structure whose token space
				 * has overflowed. */
{
    int newCount;
    Tcl_Token *newPtr;

    newCount = parsePtr->tokensAvailable*2;
    newPtr = (Tcl_Token *) ckalloc((unsigned) (newCount * sizeof(Tcl_Token)));
    memcpy((VOID *) newPtr, (VOID *) parsePtr->tokenPtr,
	    (size_t) (parsePtr->tokensAvailable * sizeof(Tcl_Token)));
    if (parsePtr->tokenPtr != parsePtr->staticTokens) {
	ckfree((char *) parsePtr->tokenPtr);
    }
    parsePtr->tokenPtr = newPtr;
    parsePtr->tokensAvailable = newCount;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseVarName --
 *
 *	Given a string starting with a $ sign, parse off a variable
 *	name and return information about the parse.  No more than
 *	numBytes bytes will be scanned.
 *
 * Results:
 *	The return value is TCL_OK if the command was parsed
 *	successfully and TCL_ERROR otherwise.  If an error occurs and
 *	interp isn't NULL then an error message is left in its result. 
 *	On a successful return, tokenPtr and numTokens fields of
 *	parsePtr are filled in with information about the variable name
 *	that was parsed.  The "size" field of the first new token gives
 *	the total number of bytes in the variable name.  Other fields in
 *	parsePtr are undefined.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the command, then additional space is
 *	malloc-ed.  If the procedure returns TCL_OK then the caller must
 *	eventually invoke Tcl_FreeParse to release any additional space
 *	that was allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ParseVarName(interp, string, numBytes, parsePtr, append)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting;
				 * if NULL, then no error message is
				 * provided. */
    CONST char *string;		/* String containing variable name.  First
				 * character must be "$". */
    register int numBytes;	/* Total number of bytes in string.  If < 0,
				 * the string consists of all bytes up to the
				 * first null character. */
    Tcl_Parse *parsePtr;	/* Structure to fill in with information
				 * about the variable name. */
    int append;			/* Non-zero means append tokens to existing
				 * information in parsePtr; zero means ignore
				 * existing tokens in parsePtr and reinitialize
				 * it. */
{
    Tcl_Token *tokenPtr;
    register CONST char *src;
    unsigned char c;
    int varIndex, offset;
    Tcl_UniChar ch;
    unsigned array;

    if ((numBytes == 0) || (string == NULL)) {
	return TCL_ERROR;
    }
    if (numBytes < 0) {
	numBytes = strlen(string);
    }

    if (!append) {
	parsePtr->numWords = 0;
	parsePtr->tokenPtr = parsePtr->staticTokens;
	parsePtr->numTokens = 0;
	parsePtr->tokensAvailable = NUM_STATIC_TOKENS;
	parsePtr->string = string;
	parsePtr->end = (string + numBytes);
	parsePtr->interp = interp;
	parsePtr->errorType = TCL_PARSE_SUCCESS;
	parsePtr->incomplete = 0;
    }

    /*
     * Generate one token for the variable, an additional token for the
     * name, plus any number of additional tokens for the index, if
     * there is one.
     */

    src = string;
    if ((parsePtr->numTokens + 2) > parsePtr->tokensAvailable) {
	TclExpandTokenArray(parsePtr);
    }
    tokenPtr = &parsePtr->tokenPtr[parsePtr->numTokens];
    tokenPtr->type = TCL_TOKEN_VARIABLE;
    tokenPtr->start = src;
    varIndex = parsePtr->numTokens;
    parsePtr->numTokens++;
    tokenPtr++;
    src++; numBytes--;
    if (numBytes == 0) {
	goto justADollarSign;
    }
    tokenPtr->type = TCL_TOKEN_TEXT;
    tokenPtr->start = src;
    tokenPtr->numComponents = 0;

    /*
     * The name of the variable can have three forms:
     * 1. The $ sign is followed by an open curly brace.  Then 
     *    the variable name is everything up to the next close
     *    curly brace, and the variable is a scalar variable.
     * 2. The $ sign is not followed by an open curly brace.  Then
     *    the variable name is everything up to the next
     *    character that isn't a letter, digit, or underscore.
     *    :: sequences are also considered part of the variable
     *    name, in order to support namespaces. If the following
     *    character is an open parenthesis, then the information
     *    between parentheses is the array element name.
     * 3. The $ sign is followed by something that isn't a letter,
     *    digit, or underscore:  in this case, there is no variable
     *    name and the token is just "$".
     */

    if (*src == '{') {
	src++; numBytes--;
	tokenPtr->type = TCL_TOKEN_TEXT;
	tokenPtr->start = src;
	tokenPtr->numComponents = 0;

	while (numBytes && (*src != '}')) {
	    numBytes--; src++;
	}
	if (numBytes == 0) {
	    if (interp != NULL) {
		Tcl_SetResult(interp, "missing close-brace for variable name",
			TCL_STATIC);
	    }
	    parsePtr->errorType = TCL_PARSE_MISSING_VAR_BRACE;
	    parsePtr->term = tokenPtr->start-1;
	    parsePtr->incomplete = 1;
	    goto error;
	}
	tokenPtr->size = src - tokenPtr->start;
	tokenPtr[-1].size = src - tokenPtr[-1].start;
	parsePtr->numTokens++;
	src++;
    } else {
	tokenPtr->type = TCL_TOKEN_TEXT;
	tokenPtr->start = src;
	tokenPtr->numComponents = 0;
	while (numBytes) {
	    if (Tcl_UtfCharComplete(src, numBytes)) {
	        offset = Tcl_UtfToUniChar(src, &ch);
	    } else {
		char utfBytes[TCL_UTF_MAX];
		memcpy(utfBytes, src, (size_t) numBytes);
		utfBytes[numBytes] = '\0';
	        offset = Tcl_UtfToUniChar(utfBytes, &ch);
	    }
	    c = UCHAR(ch);
	    if (isalnum(c) || (c == '_')) { /* INTL: ISO only, UCHAR. */
		src += offset;  numBytes -= offset;
		continue;
	    }
	    if ((c == ':') && (numBytes != 1) && (src[1] == ':')) {
		src += 2; numBytes -= 2;
		while (numBytes && (*src == ':')) {
		    src++; numBytes--; 
		}
		continue;
	    }
	    break;
	}

	/*
	 * Support for empty array names here.
	 */
	array = (numBytes && (*src == '('));
	tokenPtr->size = src - tokenPtr->start;
	if ((tokenPtr->size == 0) && !array) {
	    goto justADollarSign;
	}
	parsePtr->numTokens++;
	if (array) {
	    /*
	     * This is a reference to an array element.  Call
	     * ParseTokens recursively to parse the element name,
	     * since it could contain any number of substitutions.
	     */

	    if (ParseTokens(src+1, numBytes-1, TYPE_CLOSE_PAREN, parsePtr)
		    != TCL_OK) {
		goto error;
	    }
	    if ((parsePtr->term == (src + numBytes)) 
		    || (*parsePtr->term != ')')) { 
		if (parsePtr->interp != NULL) {
		    Tcl_SetResult(parsePtr->interp, "missing )",
			    TCL_STATIC);
		}
		parsePtr->errorType = TCL_PARSE_MISSING_PAREN;
		parsePtr->term = src;
		parsePtr->incomplete = 1;
		goto error;
	    }
	    src = parsePtr->term + 1;
	}
    }
    tokenPtr = &parsePtr->tokenPtr[varIndex];
    tokenPtr->size = src - tokenPtr->start;
    tokenPtr->numComponents = parsePtr->numTokens - (varIndex + 1);
    return TCL_OK;

    /*
     * The dollar sign isn't followed by a variable name.
     * replace the TCL_TOKEN_VARIABLE token with a
     * TCL_TOKEN_TEXT token for the dollar sign.
     */

    justADollarSign:
    tokenPtr = &parsePtr->tokenPtr[varIndex];
    tokenPtr->type = TCL_TOKEN_TEXT;
    tokenPtr->size = 1;
    tokenPtr->numComponents = 0;
    return TCL_OK;

    error:
    Tcl_FreeParse(parsePtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseVar --
 *
 *	Given a string starting with a $ sign, parse off a variable
 *	name and return its value.
 *
 * Results:
 *	The return value is the contents of the variable given by
 *	the leading characters of string.  If termPtr isn't NULL,
 *	*termPtr gets filled in with the address of the character
 *	just after the last one in the variable specifier.  If the
 *	variable doesn't exist, then the return value is NULL and
 *	an error message will be left in interp's result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

CONST char *
Tcl_ParseVar(interp, string, termPtr)
    Tcl_Interp *interp;			/* Context for looking up variable. */
    register CONST char *string;	/* String containing variable name.
					 * First character must be "$". */
    CONST char **termPtr;		/* If non-NULL, points to word to fill
					 * in with character just after last
					 * one in the variable specifier. */

{
    Tcl_Parse parse;
    register Tcl_Obj *objPtr;
    int code;

    if (Tcl_ParseVarName(interp, string, -1, &parse, 0) != TCL_OK) {
	return NULL;
    }

    if (termPtr != NULL) {
	*termPtr = string + parse.tokenPtr->size;
    }
    if (parse.numTokens == 1) {
	/*
	 * There isn't a variable name after all: the $ is just a $.
	 */

	return "$";
    }

    code = Tcl_EvalTokensStandard(interp, parse.tokenPtr, parse.numTokens);
    if (code != TCL_OK) {
	return NULL;
    }
    objPtr = Tcl_GetObjResult(interp);

    /*
     * At this point we should have an object containing the value of
     * a variable.  Just return the string from that object.
     *
     * This should have returned the object for the user to manage, but
     * instead we have some weak reference to the string value in the
     * object, which is why we make sure the object exists after resetting
     * the result.  This isn't ideal, but it's the best we can do with the
     * current documented interface. -- hobbs
     */

    if (!Tcl_IsShared(objPtr)) {
	Tcl_IncrRefCount(objPtr);
    }
    Tcl_ResetResult(interp);
    return TclGetString(objPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseBraces --
 *
 *	Given a string in braces such as a Tcl command argument or a string
 *	value in a Tcl expression, this procedure parses the string and
 *	returns information about the parse.  No more than numBytes bytes
 *	will be scanned.
 *
 * Results:
 *	The return value is TCL_OK if the string was parsed successfully and
 *	TCL_ERROR otherwise. If an error occurs and interp isn't NULL then
 *	an error message is left in its result. On a successful return,
 *	tokenPtr and numTokens fields of parsePtr are filled in with
 *	information about the string that was parsed. Other fields in
 *	parsePtr are undefined. termPtr is set to point to the character
 *	just after the last one in the braced string.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the command, then additional space is
 *	malloc-ed. If the procedure returns TCL_OK then the caller must
 *	eventually invoke Tcl_FreeParse to release any additional space
 *	that was allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ParseBraces(interp, string, numBytes, parsePtr, append, termPtr)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting;
				 * if NULL, then no error message is
				 * provided. */
    CONST char *string;		/* String containing the string in braces.
				 * The first character must be '{'. */
    register int numBytes;	/* Total number of bytes in string. If < 0,
				 * the string consists of all bytes up to
				 * the first null character. */
    register Tcl_Parse *parsePtr;
    				/* Structure to fill in with information
				 * about the string. */
    int append;			/* Non-zero means append tokens to existing
				 * information in parsePtr; zero means
				 * ignore existing tokens in parsePtr and
				 * reinitialize it. */
    CONST char **termPtr;	/* If non-NULL, points to word in which to
				 * store a pointer to the character just
				 * after the terminating '}' if the parse
				 * was successful. */

{
    Tcl_Token *tokenPtr;
    register CONST char *src;
    int startIndex, level, length;

    if ((numBytes == 0) || (string == NULL)) {
	return TCL_ERROR;
    }
    if (numBytes < 0) {
	numBytes = strlen(string);
    }

    if (!append) {
	parsePtr->numWords = 0;
	parsePtr->tokenPtr = parsePtr->staticTokens;
	parsePtr->numTokens = 0;
	parsePtr->tokensAvailable = NUM_STATIC_TOKENS;
	parsePtr->string = string;
	parsePtr->end = (string + numBytes);
	parsePtr->interp = interp;
	parsePtr->errorType = TCL_PARSE_SUCCESS;
    }

    src = string;
    startIndex = parsePtr->numTokens;

    if (parsePtr->numTokens == parsePtr->tokensAvailable) {
	TclExpandTokenArray(parsePtr);
    }
    tokenPtr = &parsePtr->tokenPtr[startIndex];
    tokenPtr->type = TCL_TOKEN_TEXT;
    tokenPtr->start = src+1;
    tokenPtr->numComponents = 0;
    level = 1;
    while (1) {
	while (++src, --numBytes) {
	    if (CHAR_TYPE(*src) != TYPE_NORMAL) {
		break;
	    }
	}
	if (numBytes == 0) {
	    register int openBrace = 0;

	    parsePtr->errorType = TCL_PARSE_MISSING_BRACE;
	    parsePtr->term = string;
	    parsePtr->incomplete = 1;
	    if (interp == NULL) {
		/*
		 * Skip straight to the exit code since we have no
		 * interpreter to put error message in.
		 */
		goto error;
	    }

	    Tcl_SetResult(interp, "missing close-brace", TCL_STATIC);

	    /*
	     *  Guess if the problem is due to comments by searching
	     *  the source string for a possible open brace within the
	     *  context of a comment.  Since we aren't performing a
	     *  full Tcl parse, just look for an open brace preceded
	     *  by a '<whitespace>#' on the same line.
	     */

	    for (; src > string; src--) {
		switch (*src) {
		    case '{':
			openBrace = 1;
			break;
		    case '\n':
			openBrace = 0;
			break;
		    case '#' :
			if (openBrace && (isspace(UCHAR(src[-1])))) {
			    Tcl_AppendResult(interp,
				    ": possible unbalanced brace in comment",
				    (char *) NULL);
			    goto error;
			}
			break;
		}
	    }

	    error:
	    Tcl_FreeParse(parsePtr);
	    return TCL_ERROR;
	}
	switch (*src) {
	    case '{':
		level++;
		break;
	    case '}':
		if (--level == 0) {

		    /*
		     * Decide if we need to finish emitting a
		     * partially-finished token.  There are 3 cases:
		     *     {abc \newline xyz} or {xyz}
		     *		- finish emitting "xyz" token
		     *     {abc \newline}
		     *		- don't emit token after \newline
		     *     {}	- finish emitting zero-sized token
		     *
		     * The last case ensures that there is a token
		     * (even if empty) that describes the braced string.
		     */
    
		    if ((src != tokenPtr->start)
			    || (parsePtr->numTokens == startIndex)) {
			tokenPtr->size = (src - tokenPtr->start);
			parsePtr->numTokens++;
		    }
		    if (termPtr != NULL) {
			*termPtr = src+1;
		    }
		    return TCL_OK;
		}
		break;
	    case '\\':
		TclParseBackslash(src, numBytes, &length, NULL);
		if ((length > 1) && (src[1] == '\n')) {
		    /*
		     * A backslash-newline sequence must be collapsed, even
		     * inside braces, so we have to split the word into
		     * multiple tokens so that the backslash-newline can be
		     * represented explicitly.
		     */
		
		    if (numBytes == 2) {
			parsePtr->incomplete = 1;
		    }
		    tokenPtr->size = (src - tokenPtr->start);
		    if (tokenPtr->size != 0) {
			parsePtr->numTokens++;
		    }
		    if ((parsePtr->numTokens+1) >= parsePtr->tokensAvailable) {
			TclExpandTokenArray(parsePtr);
		    }
		    tokenPtr = &parsePtr->tokenPtr[parsePtr->numTokens];
		    tokenPtr->type = TCL_TOKEN_BS;
		    tokenPtr->start = src;
		    tokenPtr->size = length;
		    tokenPtr->numComponents = 0;
		    parsePtr->numTokens++;
		
		    src += length - 1;
		    numBytes -= length - 1;
		    tokenPtr++;
		    tokenPtr->type = TCL_TOKEN_TEXT;
		    tokenPtr->start = src + 1;
		    tokenPtr->numComponents = 0;
		} else {
		    src += length - 1;
		    numBytes -= length - 1;
		}
		break;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ParseQuotedString --
 *
 *	Given a double-quoted string such as a quoted Tcl command argument
 *	or a quoted value in a Tcl expression, this procedure parses the
 *	string and returns information about the parse.  No more than
 *	numBytes bytes will be scanned.
 *
 * Results:
 *	The return value is TCL_OK if the string was parsed successfully and
 *	TCL_ERROR otherwise. If an error occurs and interp isn't NULL then
 *	an error message is left in its result. On a successful return,
 *	tokenPtr and numTokens fields of parsePtr are filled in with
 *	information about the string that was parsed. Other fields in
 *	parsePtr are undefined. termPtr is set to point to the character
 *	just after the quoted string's terminating close-quote.
 *
 * Side effects:
 *	If there is insufficient space in parsePtr to hold all the
 *	information about the command, then additional space is
 *	malloc-ed. If the procedure returns TCL_OK then the caller must
 *	eventually invoke Tcl_FreeParse to release any additional space
 *	that was allocated.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_ParseQuotedString(interp, string, numBytes, parsePtr, append, termPtr)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting;
				 * if NULL, then no error message is
				 * provided. */
    CONST char *string;		/* String containing the quoted string. 
				 * The first character must be '"'. */
    register int numBytes;	/* Total number of bytes in string. If < 0,
				 * the string consists of all bytes up to
				 * the first null character. */
    register Tcl_Parse *parsePtr;
    				/* Structure to fill in with information
				 * about the string. */
    int append;			/* Non-zero means append tokens to existing
				 * information in parsePtr; zero means
				 * ignore existing tokens in parsePtr and
				 * reinitialize it. */
    CONST char **termPtr;	/* If non-NULL, points to word in which to
				 * store a pointer to the character just
				 * after the quoted string's terminating
				 * close-quote if the parse succeeds. */
{
    if ((numBytes == 0) || (string == NULL)) {
	return TCL_ERROR;
    }
    if (numBytes < 0) {
	numBytes = strlen(string);
    }

    if (!append) {
	parsePtr->numWords = 0;
	parsePtr->tokenPtr = parsePtr->staticTokens;
	parsePtr->numTokens = 0;
	parsePtr->tokensAvailable = NUM_STATIC_TOKENS;
	parsePtr->string = string;
	parsePtr->end = (string + numBytes);
	parsePtr->interp = interp;
	parsePtr->errorType = TCL_PARSE_SUCCESS;
    }
    
    if (ParseTokens(string+1, numBytes-1, TYPE_QUOTE, parsePtr) != TCL_OK) {
	goto error;
    }
    if (*parsePtr->term != '"') {
	if (interp != NULL) {
	    Tcl_SetResult(parsePtr->interp, "missing \"", TCL_STATIC);
	}
	parsePtr->errorType = TCL_PARSE_MISSING_QUOTE;
	parsePtr->term = string;
	parsePtr->incomplete = 1;
	goto error;
    }
    if (termPtr != NULL) {
	*termPtr = (parsePtr->term + 1);
    }
    return TCL_OK;

    error:
    Tcl_FreeParse(parsePtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * CommandComplete --
 *
 *	This procedure is shared by TclCommandComplete and
 *	Tcl_ObjCommandcoComplete; it does all the real work of seeing
 *	whether a script is complete
 *
 * Results:
 *	1 is returned if the script is complete, 0 if there are open
 *	delimiters such as " or (. 1 is also returned if there is a
 *	parse error in the script other than unmatched delimiters.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CommandComplete(script, numBytes)
    CONST char *script;			/* Script to check. */
    int numBytes;			/* Number of bytes in script. */
{
    Tcl_Parse parse;
    CONST char *p, *end;
    int result;

    p = script;
    end = p + numBytes;
    while (Tcl_ParseCommand((Tcl_Interp *) NULL, p, end - p, 0, &parse)
	    == TCL_OK) {
	p = parse.commandStart + parse.commandSize;
	if (p >= end) {
	    break;
	}
	Tcl_FreeParse(&parse);
    }
    if (parse.incomplete) {
	result = 0;
    } else {
	result = 1;
    }
    Tcl_FreeParse(&parse);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CommandComplete --
 *
 *	Given a partial or complete Tcl script, this procedure
 *	determines whether the script is complete in the sense
 *	of having matched braces and quotes and brackets.
 *
 * Results:
 *	1 is returned if the script is complete, 0 otherwise.
 *	1 is also returned if there is a parse error in the script
 *	other than unmatched delimiters.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_CommandComplete(script)
    CONST char *script;			/* Script to check. */
{
    return CommandComplete(script, (int) strlen(script));
}

/*
 *----------------------------------------------------------------------
 *
 * TclObjCommandComplete --
 *
 *	Given a partial or complete Tcl command in a Tcl object, this
 *	procedure determines whether the command is complete in the sense of
 *	having matched braces and quotes and brackets.
 *
 * Results:
 *	1 is returned if the command is complete, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclObjCommandComplete(objPtr)
    Tcl_Obj *objPtr;			/* Points to object holding script
					 * to check. */
{
    CONST char *script;
    int length;

    script = Tcl_GetStringFromObj(objPtr, &length);
    return CommandComplete(script, length);
}

/*
 *----------------------------------------------------------------------
 *
 * TclIsLocalScalar --
 *
 *	Check to see if a given string is a legal scalar variable
 *	name with no namespace qualifiers or substitutions.
 *
 * Results:
 *	Returns 1 if the variable is a local scalar.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclIsLocalScalar(src, len)
    CONST char *src;
    int len;
{
    CONST char *p;
    CONST char *lastChar = src + (len - 1);

    for (p = src; p <= lastChar; p++) {
	if ((CHAR_TYPE(*p) != TYPE_NORMAL) &&
		(CHAR_TYPE(*p) != TYPE_COMMAND_END)) {
	    /*
	     * TCL_COMMAND_END is returned for the last character
	     * of the string.  By this point we know it isn't
	     * an array or namespace reference.
	     */

	    return 0;
	}
	if  (*p == '(') {
	    if (*lastChar == ')') { /* we have an array element */
		return 0;
	    }
	} else if (*p == ':') {
	    if ((p != lastChar) && *(p+1) == ':') { /* qualified name */
		return 0;
	    }
	}
    }
	
    return 1;
}
