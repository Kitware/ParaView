/* 
 * tkTextIndex.c --
 *
 *	This module provides procedures that manipulate indices for
 *	text widgets.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "default.h"
#include "tkPort.h"
#include "tkInt.h"
#include "tkText.h"

/*
 * Index to use to select last character in line (very large integer):
 */

#define LAST_CHAR 1000000

/*
 * Forward declarations for procedures defined later in this file:
 */

static char *		ForwBack _ANSI_ARGS_((char *string,
			    TkTextIndex *indexPtr));
static char *		StartEnd _ANSI_ARGS_(( char *string,
			    TkTextIndex *indexPtr));

/*
 *---------------------------------------------------------------------------
 *
 * TkTextMakeByteIndex --
 *
 *	Given a line index and a byte index, look things up in the B-tree
 *	and fill in a TkTextIndex structure.
 *
 * Results:
 *	The structure at *indexPtr is filled in with information about the
 *	character at lineIndex and byteIndex (or the closest existing
 *	character, if the specified one doesn't exist), and indexPtr is
 *	returned as result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

TkTextIndex *
TkTextMakeByteIndex(tree, lineIndex, byteIndex, indexPtr)
    TkTextBTree tree;		/* Tree that lineIndex and charIndex refer
				 * to. */
    int lineIndex;		/* Index of desired line (0 means first
				 * line of text). */
    int byteIndex;		/* Byte index of desired character. */
    TkTextIndex *indexPtr;	/* Structure to fill in. */
{
    TkTextSegment *segPtr;
    int index;
    char *p, *start;
    Tcl_UniChar ch;

    indexPtr->tree = tree;
    if (lineIndex < 0) {
	lineIndex = 0;
	byteIndex = 0;
    }
    if (byteIndex < 0) {
	byteIndex = 0;
    }
    indexPtr->linePtr = TkBTreeFindLine(tree, lineIndex);
    if (indexPtr->linePtr == NULL) {
	indexPtr->linePtr = TkBTreeFindLine(tree, TkBTreeNumLines(tree));
	byteIndex = 0;
    }
    if (byteIndex == 0) {
	indexPtr->byteIndex = byteIndex;
	return indexPtr;
    }

    /*
     * Verify that the index is within the range of the line and points
     * to a valid character boundary.  
     */

    index = 0;
    for (segPtr = indexPtr->linePtr->segPtr; ; segPtr = segPtr->nextPtr) {
	if (segPtr == NULL) {
	    /*
	     * Use the index of the last character in the line.  Since
	     * the last character on the line is guaranteed to be a '\n',
	     * we can back up a constant sizeof(char) bytes.
	     */
	     
	    indexPtr->byteIndex = index - sizeof(char);
	    break;
	}
	if (index + segPtr->size > byteIndex) {
	    indexPtr->byteIndex = byteIndex;
	    if ((byteIndex > index) && (segPtr->typePtr == &tkTextCharType)) {
		/*
		 * Prevent UTF-8 character from being split up by ensuring
		 * that byteIndex falls on a character boundary.  If index
		 * falls in the middle of a UTF-8 character, it will be
		 * adjusted to the end of that UTF-8 character.
		 */

		start = segPtr->body.chars + (byteIndex - index);
		p = Tcl_UtfPrev(start, segPtr->body.chars);
		p += Tcl_UtfToUniChar(p, &ch);
		indexPtr->byteIndex += p - start;
	    }
	    break;
	}
	index += segPtr->size;
    }
    return indexPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextMakeCharIndex --
 *
 *	Given a line index and a character index, look things up in the
 *	B-tree and fill in a TkTextIndex structure.
 *
 * Results:
 *	The structure at *indexPtr is filled in with information about the
 *	character at lineIndex and charIndex (or the closest existing
 *	character, if the specified one doesn't exist), and indexPtr is
 *	returned as result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

TkTextIndex *
TkTextMakeCharIndex(tree, lineIndex, charIndex, indexPtr)
    TkTextBTree tree;		/* Tree that lineIndex and charIndex refer
				 * to. */
    int lineIndex;		/* Index of desired line (0 means first
				 * line of text). */
    int charIndex;		/* Index of desired character. */
    TkTextIndex *indexPtr;	/* Structure to fill in. */
{
    register TkTextSegment *segPtr;
    char *p, *start, *end;
    int index, offset;
    Tcl_UniChar ch;

    indexPtr->tree = tree;
    if (lineIndex < 0) {
	lineIndex = 0;
	charIndex = 0;
    }
    if (charIndex < 0) {
	charIndex = 0;
    }
    indexPtr->linePtr = TkBTreeFindLine(tree, lineIndex);
    if (indexPtr->linePtr == NULL) {
	indexPtr->linePtr = TkBTreeFindLine(tree, TkBTreeNumLines(tree));
	charIndex = 0;
    }

    /*
     * Verify that the index is within the range of the line.
     * If not, just use the index of the last character in the line.
     */

    index = 0;
    for (segPtr = indexPtr->linePtr->segPtr; ; segPtr = segPtr->nextPtr) {
	if (segPtr == NULL) {
	    /*
	     * Use the index of the last character in the line.  Since
	     * the last character on the line is guaranteed to be a '\n',
	     * we can back up a constant sizeof(char) bytes.
	     */
	     
	    indexPtr->byteIndex = index - sizeof(char);
	    break;
	}
	if (segPtr->typePtr == &tkTextCharType) {
	    /*
	     * Turn character offset into a byte offset.
	     */

	    start = segPtr->body.chars;
	    end = start + segPtr->size;
	    for (p = start; p < end; p += offset) {
		if (charIndex == 0) {
		    indexPtr->byteIndex = index;
		    return indexPtr;
		}
		charIndex--;
		offset = Tcl_UtfToUniChar(p, &ch);
		index += offset;
	    }
	} else {
	    if (charIndex < segPtr->size) {
		indexPtr->byteIndex = index;
		break;
	    }
	    charIndex -= segPtr->size;
	    index += segPtr->size;
	}
    }
    return indexPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexToSeg --
 *
 *	Given an index, this procedure returns the segment and offset
 *	within segment for the index.
 *
 * Results:
 *	The return value is a pointer to the segment referred to by
 *	indexPtr; this will always be a segment with non-zero size.  The
 *	variable at *offsetPtr is set to hold the integer offset within
 *	the segment of the character given by indexPtr.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

TkTextSegment *
TkTextIndexToSeg(indexPtr, offsetPtr)
    CONST TkTextIndex *indexPtr;/* Text index. */
    int *offsetPtr;		/* Where to store offset within segment, or
				 * NULL if offset isn't wanted. */
{
    TkTextSegment *segPtr;
    int offset;

    for (offset = indexPtr->byteIndex, segPtr = indexPtr->linePtr->segPtr;
	    offset >= segPtr->size;
	    offset -= segPtr->size, segPtr = segPtr->nextPtr) {
	/* Empty loop body. */
    }
    if (offsetPtr != NULL) {
	*offsetPtr = offset;
    }
    return segPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextSegToOffset --
 *
 *	Given a segment pointer and the line containing it, this procedure
 *	returns the offset of the segment within its line.
 *
 * Results:
 *	The return value is the offset (within its line) of the first
 *	character in segPtr.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextSegToOffset(segPtr, linePtr)
    CONST TkTextSegment *segPtr;/* Segment whose offset is desired. */
    CONST TkTextLine *linePtr;	/* Line containing segPtr. */
{
    CONST TkTextSegment *segPtr2;
    int offset;

    offset = 0;
    for (segPtr2 = linePtr->segPtr; segPtr2 != segPtr;
	    segPtr2 = segPtr2->nextPtr) {
	offset += segPtr2->size;
    }
    return offset;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextGetIndex --
 *
 *	Given a string, return the index that is described.
 *
 * Results:
 *	The return value is a standard Tcl return result.  If TCL_OK is
 *	returned, then everything went well and the index at *indexPtr is
 *	filled in; otherwise TCL_ERROR is returned and an error message
 *	is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextGetIndex(interp, textPtr, string, indexPtr)
    Tcl_Interp *interp;		/* Use this for error reporting. */
    TkText *textPtr;		/* Information about text widget. */
    char *string;		/* Textual description of position. */
    TkTextIndex *indexPtr;	/* Index structure to fill in. */
{
    char *p, *end, *endOfBase;
    Tcl_HashEntry *hPtr;
    TkTextTag *tagPtr;
    TkTextSearch search;
    TkTextIndex first, last;
    int wantLast, result;
    char c;

    /*
     *---------------------------------------------------------------------
     * Stage 1: check to see if the index consists of nothing but a mark
     * name.  We do this check now even though it's also done later, in
     * order to allow mark names that include funny characters such as
     * spaces or "+1c".
     *---------------------------------------------------------------------
     */

    if (TkTextMarkNameToIndex(textPtr, string, indexPtr) == TCL_OK) {
	return TCL_OK;
    }

    /*
     *------------------------------------------------
     * Stage 2: start again by parsing the base index.
     *------------------------------------------------
     */

    indexPtr->tree = textPtr->tree;

    /*
     * First look for the form "tag.first" or "tag.last" where "tag"
     * is the name of a valid tag.  Try to use up as much as possible
     * of the string in this check (strrchr instead of strchr below).
     * Doing the check now, and in this way, allows tag names to include
     * funny characters like "@" or "+1c".
     */

    p = strrchr(string, '.');
    if (p != NULL) {
	if ((p[1] == 'f') && (strncmp(p+1, "first", 5) == 0)) {
	    wantLast = 0;
	    endOfBase = p+6;
	} else if ((p[1] == 'l') && (strncmp(p+1, "last", 4) == 0)) {
	    wantLast = 1;
	    endOfBase = p+5;
	} else {
	    goto tryxy;
	}
	*p = 0;
	hPtr = Tcl_FindHashEntry(&textPtr->tagTable, string);
	*p = '.';
	if (hPtr == NULL) {
	    goto tryxy;
	}
	tagPtr = (TkTextTag *) Tcl_GetHashValue(hPtr);
	TkTextMakeByteIndex(textPtr->tree, 0, 0, &first);
	TkTextMakeByteIndex(textPtr->tree, TkBTreeNumLines(textPtr->tree), 0,
		&last);
	TkBTreeStartSearch(&first, &last, tagPtr, &search);
	if (!TkBTreeCharTagged(&first, tagPtr) && !TkBTreeNextTag(&search)) {
	    Tcl_AppendResult(interp,
		    "text doesn't contain any characters tagged with \"",
		    Tcl_GetHashKey(&textPtr->tagTable, hPtr), "\"",
			    (char *) NULL);
	    return TCL_ERROR;
	}
	*indexPtr = search.curIndex;
	if (wantLast) {
	    while (TkBTreeNextTag(&search)) {
		*indexPtr = search.curIndex;
	    }
	}
	goto gotBase;
    }

    tryxy:
    if (string[0] == '@') {
	/*
	 * Find character at a given x,y location in the window.
	 */

	int x, y;

	p = string+1;
	x = strtol(p, &end, 0);
	if ((end == p) || (*end != ',')) {
	    goto error;
	}
	p = end+1;
	y = strtol(p, &end, 0);
	if (end == p) {
	    goto error;
	}
	TkTextPixelIndex(textPtr, x, y, indexPtr);
	endOfBase = end;
	goto gotBase; 
    }

    if (isdigit(UCHAR(string[0])) || (string[0] == '-')) {
	int lineIndex, charIndex;

	/*
	 * Base is identified with line and character indices.
	 */

	lineIndex = strtol(string, &end, 0) - 1;
	if ((end == string) || (*end != '.')) {
	    goto error;
	}
	p = end+1;
	if ((*p == 'e') && (strncmp(p, "end", 3) == 0)) {
	    charIndex = LAST_CHAR;
	    endOfBase = p+3;
	} else {
	    charIndex = strtol(p, &end, 0);
	    if (end == p) {
		goto error;
	    }
	    endOfBase = end;
	}
	TkTextMakeCharIndex(textPtr->tree, lineIndex, charIndex, indexPtr);
	goto gotBase;
    }

    for (p = string; *p != 0; p++) {
	if (isspace(UCHAR(*p)) || (*p == '+') || (*p == '-')) {
	    break;
	}
    }
    endOfBase = p;
    if (string[0] == '.') {
	/*
	 * See if the base position is the name of an embedded window.
	 */

	c = *endOfBase;
	*endOfBase = 0;
	result = TkTextWindowIndex(textPtr, string, indexPtr);
	*endOfBase = c;
	if (result != 0) {
	    goto gotBase;
	}
    }
    if ((string[0] == 'e')
	    && (strncmp(string, "end", (size_t) (endOfBase-string)) == 0)) {
	/*
	 * Base position is end of text.
	 */

	TkTextMakeByteIndex(textPtr->tree, TkBTreeNumLines(textPtr->tree),
		0, indexPtr);
	goto gotBase;
    } else {
	/*
	 * See if the base position is the name of a mark.
	 */

	c = *endOfBase;
	*endOfBase = 0;
	result = TkTextMarkNameToIndex(textPtr, string, indexPtr);
	*endOfBase = c;
	if (result == TCL_OK) {
	    goto gotBase;
	}

	/*
	 * See if the base position is the name of an embedded image
	 */

	c = *endOfBase;
	*endOfBase = 0;
	result = TkTextImageIndex(textPtr, string, indexPtr);
	*endOfBase = c;
	if (result != 0) {
	    goto gotBase;
	}
    }
    goto error;

    /*
     *-------------------------------------------------------------------
     * Stage 3: process zero or more modifiers.  Each modifier is either
     * a keyword like "wordend" or "linestart", or it has the form
     * "op count units" where op is + or -, count is a number, and units
     * is "chars" or "lines".
     *-------------------------------------------------------------------
     */

    gotBase:
    p = endOfBase;
    while (1) {
	while (isspace(UCHAR(*p))) {
	    p++;
	}
	if (*p == 0) {
	    break;
	}
    
	if ((*p == '+') || (*p == '-')) {
	    p = ForwBack(p, indexPtr);
	} else {
	    p = StartEnd(p, indexPtr);
	}
	if (p == NULL) {
	    goto error;
	}
    }
    return TCL_OK;

    error:
    Tcl_AppendResult(interp, "bad text index \"", string, "\"",
	    (char *) NULL);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextPrintIndex --
 *	
 *	This procedure generates a string description of an index, suitable
 *	for reading in again later.
 *
 * Results:
 *	The characters pointed to by string are modified.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TkTextPrintIndex(indexPtr, string)
    CONST TkTextIndex *indexPtr;/* Pointer to index. */
    char *string;		/* Place to store the position.  Must have
				 * at least TK_POS_CHARS characters. */
{
    TkTextSegment *segPtr;
    int numBytes, charIndex;

    numBytes = indexPtr->byteIndex;
    charIndex = 0;
    for (segPtr = indexPtr->linePtr->segPtr; ; segPtr = segPtr->nextPtr) {
	if (numBytes <= segPtr->size) {
	    break;
	}
	if (segPtr->typePtr == &tkTextCharType) {
	    charIndex += Tcl_NumUtfChars(segPtr->body.chars, segPtr->size);
	} else {
	    charIndex += segPtr->size;
	}
	numBytes -= segPtr->size;
    }
    if (segPtr->typePtr == &tkTextCharType) {
	charIndex += Tcl_NumUtfChars(segPtr->body.chars, numBytes);
    } else {
	charIndex += numBytes;
    }
    sprintf(string, "%d.%d", TkBTreeLineIndex(indexPtr->linePtr) + 1,
	    charIndex);
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexCmp --
 *
 *	Compare two indices to see which one is earlier in the text.
 *
 * Results:
 *	The return value is 0 if index1Ptr and index2Ptr refer to the same
 *	position in the file, -1 if index1Ptr refers to an earlier position
 *	than index2Ptr, and 1 otherwise.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkTextIndexCmp(index1Ptr, index2Ptr)
    CONST TkTextIndex *index1Ptr;		/* First index. */
    CONST TkTextIndex *index2Ptr;		/* Second index. */
{
    int line1, line2;

    if (index1Ptr->linePtr == index2Ptr->linePtr) {
	if (index1Ptr->byteIndex < index2Ptr->byteIndex) {
	    return -1;
	} else if (index1Ptr->byteIndex > index2Ptr->byteIndex) {
	    return 1;
	} else {
	    return 0;
	}
    }
    line1 = TkBTreeLineIndex(index1Ptr->linePtr);
    line2 = TkBTreeLineIndex(index2Ptr->linePtr);
    if (line1 < line2) {
	return -1;
    }
    if (line1 > line2) {
	return 1;
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * ForwBack --
 *
 *	This procedure handles +/- modifiers for indices to adjust the
 *	index forwards or backwards.
 *
 * Results:
 *	If the modifier in string is successfully parsed then the return
 *	value is the address of the first character after the modifier,
 *	and *indexPtr is updated to reflect the modifier.  If there is a
 *	syntax error in the modifier then NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static char *
ForwBack(string, indexPtr)
    char *string;		/* String to parse for additional info
				 * about modifier (count and units). 
				 * Points to "+" or "-" that starts
				 * modifier. */
    TkTextIndex *indexPtr;	/* Index to update as specified in string. */
{
    register char *p;
    char *end, *units;
    int count, lineIndex;
    size_t length;

    /*
     * Get the count (how many units forward or backward).
     */

    p = string+1;
    while (isspace(UCHAR(*p))) {
	p++;
    }
    count = strtol(p, &end, 0);
    if (end == p) {
	return NULL;
    }
    p = end;
    while (isspace(UCHAR(*p))) {
	p++;
    }

    /*
     * Find the end of this modifier (next space or + or - character),
     * then parse the unit specifier and update the position
     * accordingly.
     */

    units = p; 
    while ((*p != '\0') && !isspace(UCHAR(*p)) && (*p != '+') && (*p != '-')) {
	p++;
    }
    length = p - units;
    if ((*units == 'c') && (strncmp(units, "chars", length) == 0)) {
	if (*string == '+') {
	    TkTextIndexForwChars(indexPtr, count, indexPtr);
	} else {
	    TkTextIndexBackChars(indexPtr, count, indexPtr);
	}
    } else if ((*units == 'l') && (strncmp(units, "lines", length) == 0)) {
	lineIndex = TkBTreeLineIndex(indexPtr->linePtr);
	if (*string == '+') {
	    lineIndex += count;
	} else {
	    lineIndex -= count;

	    /*
	     * The check below retains the character position, even
	     * if the line runs off the start of the file.  Without
	     * it, the character position will get reset to 0 by
	     * TkTextMakeIndex.
	     */

	    if (lineIndex < 0) {
		lineIndex = 0;
	    }
	}
	/*
	 * This doesn't work quite right if using a proportional font or
	 * UTF-8 characters with varying numbers of bytes.  The cursor will
	 * bop around, keeping a constant number of bytes (not characters)
	 * from the left edge (but making sure not to split any UTF-8
	 * characters), regardless of the x-position the index corresponds
	 * to.  The proper way to do this is to get the x-position of the
	 * index and then pick the character at the same x-position in the
	 * new line.
	 */

	TkTextMakeByteIndex(indexPtr->tree, lineIndex, indexPtr->byteIndex,
		indexPtr);
    } else {
	return NULL;
    }
    return p;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexForwBytes --
 *
 *	Given an index for a text widget, this procedure creates a new
 *	index that points "count" bytes ahead of the source index.
 *
 * Results:
 *	*dstPtr is modified to refer to the character "count" bytes after
 *	srcPtr, or to the last character in the TkText if there aren't
 *	"count" bytes left.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TkTextIndexForwBytes(srcPtr, byteCount, dstPtr)
    CONST TkTextIndex *srcPtr;	/* Source index. */
    int byteCount;		/* How many bytes forward to move.  May be
				 * negative. */
    TkTextIndex *dstPtr;	/* Destination index: gets modified. */
{
    TkTextLine *linePtr;
    TkTextSegment *segPtr;
    int lineLength;

    if (byteCount < 0) {
	TkTextIndexBackBytes(srcPtr, -byteCount, dstPtr);
	return;
    }

    *dstPtr = *srcPtr;
    dstPtr->byteIndex += byteCount;
    while (1) {
	/*
	 * Compute the length of the current line.
	 */

	lineLength = 0;
	for (segPtr = dstPtr->linePtr->segPtr; segPtr != NULL;
		segPtr = segPtr->nextPtr) {
	    lineLength += segPtr->size;
	}

	/*
	 * If the new index is in the same line then we're done.
	 * Otherwise go on to the next line.
	 */

	if (dstPtr->byteIndex < lineLength) {
	    return;
	}
	dstPtr->byteIndex -= lineLength;
	linePtr = TkBTreeNextLine(dstPtr->linePtr);
	if (linePtr == NULL) {
	    dstPtr->byteIndex = lineLength - 1;
	    return;
	}
	dstPtr->linePtr = linePtr;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexForwChars --
 *
 *	Given an index for a text widget, this procedure creates a new
 *	index that points "count" characters ahead of the source index.
 *
 * Results:
 *	*dstPtr is modified to refer to the character "count" characters
 *	after srcPtr, or to the last character in the TkText if there
 *	aren't "count" characters left in the file.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TkTextIndexForwChars(srcPtr, charCount, dstPtr)
    CONST TkTextIndex *srcPtr;	/* Source index. */
    int charCount;		/* How many characters forward to move.
				 * May be negative. */
    TkTextIndex *dstPtr;	/* Destination index: gets modified. */
{
    TkTextLine *linePtr;
    TkTextSegment *segPtr;
    int byteOffset;
    char *start, *end, *p;
    Tcl_UniChar ch;

    if (charCount < 0) {
	TkTextIndexBackChars(srcPtr, -charCount, dstPtr);
	return;
    }

    *dstPtr = *srcPtr;

    /*
     * Find seg that contains src byteIndex.
     * Move forward specified number of chars.
     */

    segPtr = TkTextIndexToSeg(dstPtr, &byteOffset);
    while (1) {
	/*
	 * Go through each segment in line looking for specified character
	 * index.
	 */

	for ( ; segPtr != NULL; segPtr = segPtr->nextPtr) {
	    if (segPtr->typePtr == &tkTextCharType) {
		start = segPtr->body.chars + byteOffset;
		end = segPtr->body.chars + segPtr->size;
		for (p = start; p < end; p += Tcl_UtfToUniChar(p, &ch)) {
		    if (charCount == 0) {
			dstPtr->byteIndex += (p - start);
			return;
		    }
		    charCount--;
		}
	    } else {
		if (charCount < segPtr->size - byteOffset) {
		    dstPtr->byteIndex += charCount;
		    return;
		}
		charCount -= segPtr->size - byteOffset;
	    }
	    dstPtr->byteIndex += segPtr->size - byteOffset;
	    byteOffset = 0;
	}

	/*
	 * Go to the next line.  If we are at the end of the text item,
	 * back up one byte (for the terminal '\n' character) and return
	 * that index.
	 */
	 
	linePtr = TkBTreeNextLine(dstPtr->linePtr);
	if (linePtr == NULL) {
	    dstPtr->byteIndex -= sizeof(char);
	    return;
	}
	dstPtr->linePtr = linePtr;
	dstPtr->byteIndex = 0;
	segPtr = dstPtr->linePtr->segPtr;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexBackBytes --
 *
 *	Given an index for a text widget, this procedure creates a new
 *	index that points "count" bytes earlier than the source index.
 *
 * Results:
 *	*dstPtr is modified to refer to the character "count" bytes before
 *	srcPtr, or to the first character in the TkText if there aren't
 *	"count" bytes earlier than srcPtr.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TkTextIndexBackBytes(srcPtr, byteCount, dstPtr)
    CONST TkTextIndex *srcPtr;	/* Source index. */
    int byteCount;		/* How many bytes backward to move.  May be
				 * negative. */
    TkTextIndex *dstPtr;	/* Destination index: gets modified. */
{
    TkTextSegment *segPtr;
    int lineIndex;

    if (byteCount < 0) {
	TkTextIndexForwBytes(srcPtr, -byteCount, dstPtr);
	return;
    }

    *dstPtr = *srcPtr;
    dstPtr->byteIndex -= byteCount;
    lineIndex = -1;
    while (dstPtr->byteIndex < 0) {
	/*
	 * Move back one line in the text.  If we run off the beginning
	 * of the file then just return the first character in the text.
	 */

	if (lineIndex < 0) {
	    lineIndex = TkBTreeLineIndex(dstPtr->linePtr);
	}
	if (lineIndex == 0) {
	    dstPtr->byteIndex = 0;
	    return;
	}
	lineIndex--;
	dstPtr->linePtr = TkBTreeFindLine(dstPtr->tree, lineIndex);

	/*
	 * Compute the length of the line and add that to dstPtr->charIndex.
	 */

	for (segPtr = dstPtr->linePtr->segPtr; segPtr != NULL;
		segPtr = segPtr->nextPtr) {
	    dstPtr->byteIndex += segPtr->size;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TkTextIndexBackChars --
 *
 *	Given an index for a text widget, this procedure creates a new
 *	index that points "count" characters earlier than the source index.
 *
 * Results:
 *	*dstPtr is modified to refer to the character "count" characters
 *	before srcPtr, or to the first character in the file if there
 *	aren't "count" characters earlier than srcPtr.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TkTextIndexBackChars(srcPtr, charCount, dstPtr)
    CONST TkTextIndex *srcPtr;	/* Source index. */
    int charCount;		/* How many characters backward to move.
				 * May be negative. */
    TkTextIndex *dstPtr;	/* Destination index: gets modified. */
{
    TkTextSegment *segPtr, *oldPtr;
    int lineIndex, segSize;
    char *p, *start, *end;

    if (charCount <= 0) {
	TkTextIndexForwChars(srcPtr, -charCount, dstPtr);
	return;
    }

    *dstPtr = *srcPtr;

    /*
     * Find offset within seg that contains byteIndex.
     * Move backward specified number of chars.
     */

    lineIndex = -1;
    
    segSize = dstPtr->byteIndex;
    for (segPtr = dstPtr->linePtr->segPtr; ; segPtr = segPtr->nextPtr) {
	if (segSize <= segPtr->size) {
	    break;
	}
	segSize -= segPtr->size;
    }
    while (1) {
	if (segPtr->typePtr == &tkTextCharType) {
	    start = segPtr->body.chars;
	    end = segPtr->body.chars + segSize;
	    for (p = end; ; p = Tcl_UtfPrev(p, start)) {
		if (charCount == 0) {
		    dstPtr->byteIndex -= (end - p);
		    return;
		}
		if (p == start) {
		    break;
		}
		charCount--;
	    }
	} else {
	    if (charCount <= segSize) {
		dstPtr->byteIndex -= charCount;
		return;
	    }
	    charCount -= segSize;
	}
	dstPtr->byteIndex -= segSize;

	/*
	 * Move back into previous segment.
	 */

	oldPtr = segPtr;
	segPtr = dstPtr->linePtr->segPtr;
	if (segPtr != oldPtr) {
	    for ( ; segPtr->nextPtr != oldPtr; segPtr = segPtr->nextPtr) {
		/* Empty body. */
	    }
	    segSize = segPtr->size;
	    continue;
	}

	/*
	 * Move back to previous line.
	 */

	if (lineIndex < 0) {
	    lineIndex = TkBTreeLineIndex(dstPtr->linePtr);
	}
	if (lineIndex == 0) {
	    dstPtr->byteIndex = 0;
	    return;
	}
	lineIndex--;
	dstPtr->linePtr = TkBTreeFindLine(dstPtr->tree, lineIndex);

	/*
	 * Compute the length of the line and add that to dstPtr->byteIndex.
	 */

	oldPtr = dstPtr->linePtr->segPtr;
	for (segPtr = oldPtr; segPtr != NULL; segPtr = segPtr->nextPtr) {
	    dstPtr->byteIndex += segPtr->size;
	    oldPtr = segPtr;
	}
	segPtr = oldPtr;
	segSize = segPtr->size;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * StartEnd --
 *
 *	This procedure handles modifiers like "wordstart" and "lineend"
 *	to adjust indices forwards or backwards.
 *
 * Results:
 *	If the modifier is successfully parsed then the return value
 *	is the address of the first character after the modifier, and
 *	*indexPtr is updated to reflect the modifier. If there is a
 *	syntax error in the modifier then NULL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
StartEnd(string, indexPtr)
    char *string;		/* String to parse for additional info
				 * about modifier (count and units). 
				 * Points to first character of modifer
				 * word. */
    TkTextIndex *indexPtr;	/* Index to mdoify based on string. */
{
    char *p;
    int c, offset;
    size_t length;
    register TkTextSegment *segPtr;

    /*
     * Find the end of the modifier word.
     */

    for (p = string; isalnum(UCHAR(*p)); p++) {
	/* Empty loop body. */
    }
    length = p-string;
    if ((*string == 'l') && (strncmp(string, "lineend", length) == 0)
	    && (length >= 5)) {
	indexPtr->byteIndex = 0;
	for (segPtr = indexPtr->linePtr->segPtr; segPtr != NULL;
		segPtr = segPtr->nextPtr) {
	    indexPtr->byteIndex += segPtr->size;
	}
	indexPtr->byteIndex -= sizeof(char);
    } else if ((*string == 'l') && (strncmp(string, "linestart", length) == 0)
	    && (length >= 5)) {
	indexPtr->byteIndex = 0;
    } else if ((*string == 'w') && (strncmp(string, "wordend", length) == 0)
	    && (length >= 5)) {
	int firstChar = 1;

	/*
	 * If the current character isn't part of a word then just move
	 * forward one character.  Otherwise move forward until finding
	 * a character that isn't part of a word and stop there.
	 */

	segPtr = TkTextIndexToSeg(indexPtr, &offset);
	while (1) {
	    if (segPtr->typePtr == &tkTextCharType) {
		c = segPtr->body.chars[offset];
		if (!isalnum(UCHAR(c)) && (c != '_')) {
		    break;
		}
		firstChar = 0;
	    }
	    offset += 1;
	    indexPtr->byteIndex += sizeof(char);
	    if (offset >= segPtr->size) {
		segPtr = TkTextIndexToSeg(indexPtr, &offset);
	    }
	}
	if (firstChar) {
	    TkTextIndexForwChars(indexPtr, 1, indexPtr);
	}
    } else if ((*string == 'w') && (strncmp(string, "wordstart", length) == 0)
	    && (length >= 5)) {
	int firstChar = 1;

	/*
	 * Starting with the current character, look for one that's not
	 * part of a word and keep moving backward until you find one.
	 * Then if the character found wasn't the first one, move forward
	 * again one position.
	 */

	segPtr = TkTextIndexToSeg(indexPtr, &offset);
	while (1) {
	    if (segPtr->typePtr == &tkTextCharType) {
		c = segPtr->body.chars[offset];
		if (!isalnum(UCHAR(c)) && (c != '_')) {
		    break;
		}
		firstChar = 0;
	    }
	    offset -= 1;
	    indexPtr->byteIndex -= sizeof(char);
	    if (offset < 0) {
		if (indexPtr->byteIndex < 0) {
		    indexPtr->byteIndex = 0;
		    goto done;
		}
		segPtr = TkTextIndexToSeg(indexPtr, &offset);
	    }
	}
	if (!firstChar) {
	    TkTextIndexForwChars(indexPtr, 1, indexPtr);
	}
    } else {
	return NULL;
    }
    done:
    return p;
}
