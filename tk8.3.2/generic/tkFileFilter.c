/*
 * tkFileFilter.c --
 *
 *	Process the -filetypes option for the file dialogs on Windows and the
 *	Mac.
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkFileFilter.h"

static int		AddClause _ANSI_ARGS_((
			    Tcl_Interp * interp, FileFilter * filterPtr,
			    char * patternsStr, char * ostypesStr,
			    int isWindows));
static void		FreeClauses _ANSI_ARGS_((FileFilter * filterPtr));
static void		FreeGlobPatterns _ANSI_ARGS_((
			    FileFilterClause * clausePtr));
static void		FreeMacFileTypes _ANSI_ARGS_((
			    FileFilterClause * clausePtr));
static FileFilter *	GetFilter _ANSI_ARGS_((FileFilterList * flistPtr,
			    char * name));

/*
 *----------------------------------------------------------------------
 *
 * TkInitFileFilters --
 *
 *	Initializes a FileFilterList data structure. A FileFilterList
 *	must be initialized EXACTLY ONCE before any calls to
 *	TkGetFileFilters() is made. The usual flow of control is:
 *		TkInitFileFilters(&flist);
 *		    TkGetFileFilters(&flist, ...);
 *		    TkGetFileFilters(&flist, ...);
 *		    ...
 *		TkFreeFileFilters(&flist);
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields in flistPtr are initialized.
 *----------------------------------------------------------------------
 */

void
TkInitFileFilters(flistPtr)
    FileFilterList * flistPtr;	/* The structure to be initialized. */
{
    flistPtr->filters = NULL;
    flistPtr->filtersTail = NULL;
    flistPtr->numFilters = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetFileFilters --
 *
 *	This function is called by the Mac and Windows implementation
 *	of tk_getOpenFile and tk_getSaveFile to translate the string
 *	value of the -filetypes option of into an easy-to-parse C
 *	structure (flistPtr). The caller of this function will then use
 *	flistPtr to perform filetype matching in a platform specific way.
 *
 *	flistPtr must be initialized (See comments in TkInitFileFilters).
 *
 * Results:
 *	A standard TCL return value.
 *
 * Side effects:
 *	The fields in flistPtr are changed according to string.
 *----------------------------------------------------------------------
 */
int
TkGetFileFilters(interp, flistPtr, string, isWindows)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting. */
    FileFilterList * flistPtr;	/* Stores the list of file filters. */
    char * string;		/* Value of the -filetypes option. */
    int isWindows;		/* True if we are running on Windows. */
{
    int listArgc;
    char ** listArgv = NULL;
    char ** typeInfo = NULL;
    int code = TCL_OK;
    int i;

    if (Tcl_SplitList(interp, string, &listArgc, &listArgv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (listArgc == 0) {
	goto done;
    }

    /*
     * Free the filter information that have been allocated the previous
     * time -- the -filefilters option may have been used more than once in
     * the command line.
     */
    TkFreeFileFilters(flistPtr);

    for (i = 0; i<listArgc; i++) {
	/*
	 * Each file type should have two or three elements: the first one
	 * is the name of the type and the second is the filter of the type.
	 * The third is the Mac OSType ID, but we don't care about them here.
	 */
	int count;
	FileFilter * filterPtr;

	if (Tcl_SplitList(interp, listArgv[i], &count, &typeInfo) != TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
	
	if (count != 2 && count != 3) {
	    Tcl_AppendResult(interp, "bad file type \"", listArgv[i], "\", ",
		"should be \"typeName {extension ?extensions ...?} ",
		"?{macType ?macTypes ...?}?\"",	NULL);
	    code = TCL_ERROR;
	    goto done;
	}

	filterPtr = GetFilter(flistPtr, typeInfo[0]);

	if (count == 2) {
	    code = AddClause(interp, filterPtr, typeInfo[1], NULL,
		isWindows);
	} else {
	    code = AddClause(interp, filterPtr, typeInfo[1], typeInfo[2],
		isWindows);
	}
	if (code != TCL_OK) {
	    goto done;
	}

        if (typeInfo) {
	    ckfree((char*)typeInfo);
	}
	typeInfo = NULL;
    }

  done:
    if (typeInfo) {
       ckfree((char*)typeInfo);
    }
    if (listArgv) {
	ckfree((char*)listArgv);
    }
    return code;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFreeFileFilters --
 *
 *	Frees the malloc'ed file filter information.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields allocated by TkGetFileFilters() are freed.
 *----------------------------------------------------------------------
 */

void
TkFreeFileFilters(flistPtr)
    FileFilterList * flistPtr;	/* List of file filters to free */
{
    FileFilter * filterPtr, *toFree;

    filterPtr=flistPtr->filters;
    while (filterPtr) {
	toFree = filterPtr;
	filterPtr=filterPtr->next;
	FreeClauses(toFree);
	ckfree((char*)toFree->name);
	ckfree((char*)toFree);
    }
    flistPtr->filters = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * AddClause --
 *
 *	Add one FileFilterClause to filterPtr.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	The list of filter clauses are updated in filterPtr.
 *----------------------------------------------------------------------
 */

static int AddClause(interp, filterPtr, patternsStr, ostypesStr, isWindows)
    Tcl_Interp * interp;	/* Interpreter to use for error reporting. */
    FileFilter * filterPtr;	/* Stores the new filter clause */
    char * patternsStr;		/* A TCL list of glob patterns. */
    char * ostypesStr;		/* A TCL list of Mac OSType strings. */
    int isWindows;		/* True if we are running on Windows; False
				 * if we are running on the Mac; Glob
				 * patterns need to be processed differently
				 * on these two platforms */
{
    char ** globList = NULL;
    int globCount;
    char ** ostypeList = NULL;
    int ostypeCount;
    FileFilterClause * clausePtr;
    int i;
    int code = TCL_OK;

    if (Tcl_SplitList(interp, patternsStr, &globCount, &globList)!= TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }
    if (ostypesStr != NULL) {
	if (Tcl_SplitList(interp, ostypesStr, &ostypeCount, &ostypeList)
		!= TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
	for (i=0; i<ostypeCount; i++) {
	    if (strlen(ostypeList[i]) != 4) {
		Tcl_AppendResult(interp, "bad Macintosh file type \"",
		    ostypeList[i], "\"", NULL);
		code = TCL_ERROR;
		goto done;
	    }
	}
    }

    /*
     * Add the clause into the list of clauses 
     */

    clausePtr = (FileFilterClause*)ckalloc(sizeof(FileFilterClause));
    clausePtr->patterns     = NULL;
    clausePtr->patternsTail = NULL;
    clausePtr->macTypes     = NULL;
    clausePtr->macTypesTail = NULL;

    if (filterPtr->clauses == NULL) {
	filterPtr->clauses = filterPtr->clausesTail = clausePtr;
    } else {
	filterPtr->clausesTail->next = clausePtr;
	filterPtr->clausesTail = clausePtr;
    }
    clausePtr->next = NULL;

    if (globCount > 0 && globList != NULL) {
	for (i=0; i<globCount; i++) {
	    GlobPattern * globPtr = (GlobPattern*)ckalloc(sizeof(GlobPattern));
	    int len;
	    
	    len = (strlen(globList[i]) + 1) * sizeof(char);

	    if (globList[i][0] && globList[i][0] != '*') {
		/*
		 * Prepend a "*" to patterns that do not have a leading "*"
		 */
		globPtr->pattern = (char*)ckalloc(len+1);
		globPtr->pattern[0] = '*';
		strcpy(globPtr->pattern+1, globList[i]);
	    }
	    else if (isWindows) {
		if (strcmp(globList[i], "*") == 0) {
		    globPtr->pattern = (char*)ckalloc(4*sizeof(char));
		    strcpy(globPtr->pattern, "*.*");
		}
		else if (strcmp(globList[i], "") == 0) {
		    /*
		     * An empty string means "match all files with no
		     * extensions"
		     * BUG: "*." actually matches with all files on Win95
		     */
		    globPtr->pattern = (char*)ckalloc(3*sizeof(char));
		    strcpy(globPtr->pattern, "*.");
		}
		else {
		    globPtr->pattern = (char*)ckalloc(len);
		    strcpy(globPtr->pattern, globList[i]);
		}
	    } else {
		globPtr->pattern = (char*)ckalloc(len);
		strcpy(globPtr->pattern, globList[i]);
	    }

	    /*
	     * Add the glob pattern into the list of patterns.
	     */

	    if (clausePtr->patterns == NULL) {
		clausePtr->patterns = clausePtr->patternsTail = globPtr;
	    } else {
		clausePtr->patternsTail->next = globPtr;
		clausePtr->patternsTail = globPtr;
	    }
	    globPtr->next = NULL;
	}
    }
    if (ostypeCount > 0 && ostypeList != NULL) {
	for (i=0; i<ostypeCount; i++) {
	    MacFileType * mfPtr = (MacFileType*)ckalloc(sizeof(MacFileType));

	    memcpy(&mfPtr->type, ostypeList[i], sizeof(OSType));

	    /*
	     * Add the Mac type pattern into the list of Mac types
	     */
	    if (clausePtr->macTypes == NULL) {
		clausePtr->macTypes = clausePtr->macTypesTail = mfPtr;
	    } else {
		clausePtr->macTypesTail->next = mfPtr;
		clausePtr->macTypesTail = mfPtr;
	    }
	    mfPtr->next = NULL;
	}
    }

  done:
    if (globList) {
	ckfree((char*)globList);
    }
    if (ostypeList) {
	ckfree((char*)ostypeList);
    }

    return code;
}	

/*
 *----------------------------------------------------------------------
 *
 * GetFilter --
 *
 *	Add one FileFilter to flistPtr.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	The list of filters are updated in flistPtr.
 *----------------------------------------------------------------------
 */

static FileFilter * GetFilter(flistPtr, name)
    FileFilterList * flistPtr;	/* The FileFilterList that contains the
				 * newly created filter */
    char * name;		/* Name of the filter. It is usually displayed
				 * in the "File Types" listbox in the file
				 * dialogs. */
{
    FileFilter * filterPtr;

    for (filterPtr=flistPtr->filters; filterPtr; filterPtr=filterPtr->next) {
	if (strcmp(filterPtr->name, name)==0) {
	    return filterPtr;
	}
    }

    filterPtr = (FileFilter*)ckalloc(sizeof(FileFilter));
    filterPtr->clauses = NULL;
    filterPtr->clausesTail = NULL;
    filterPtr->name = (char*)ckalloc((strlen(name)+1) * sizeof(char));
    strcpy(filterPtr->name, name);

    if (flistPtr->filters == NULL) {
	flistPtr->filters = flistPtr->filtersTail = filterPtr;
    } else {
	flistPtr->filtersTail->next = filterPtr;
	flistPtr->filtersTail = filterPtr;
    }
    filterPtr->next = NULL;

    ++flistPtr->numFilters;
    return filterPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeClauses --
 *
 *	Frees the malloc'ed file type clause
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The list of clauses in filterPtr->clauses are freed.
 *----------------------------------------------------------------------
 */

static void
FreeClauses(filterPtr)
    FileFilter * filterPtr;	/* FileFilter whose clauses are to be freed */
{
    FileFilterClause * clausePtr, * toFree;

    clausePtr = filterPtr->clauses;
    while (clausePtr) {
	toFree = clausePtr;
	clausePtr=clausePtr->next;
	FreeGlobPatterns(toFree);
	FreeMacFileTypes(toFree);
	ckfree((char*)toFree);
    }
    filterPtr->clauses = NULL;
    filterPtr->clausesTail = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeGlobPatterns --
 *
 *	Frees the malloc'ed glob patterns in a clause
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The list of glob patterns in clausePtr->patterns are freed.
 *----------------------------------------------------------------------
 */

static void
FreeGlobPatterns(clausePtr)
    FileFilterClause * clausePtr;/* The clause whose patterns are to be freed*/
{
    GlobPattern * globPtr, * toFree;

    globPtr = clausePtr->patterns;
    while (globPtr) {
	toFree = globPtr;
	globPtr=globPtr->next;

	ckfree((char*)toFree->pattern);
	ckfree((char*)toFree);
    }
    clausePtr->patterns = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeMacFileTypes --
 *
 *	Frees the malloc'ed Mac file types in a clause
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The list of Mac file types in clausePtr->macTypes are freed.
 *----------------------------------------------------------------------
 */

static void
FreeMacFileTypes(clausePtr)
    FileFilterClause * clausePtr;  /* The clause whose mac types are to be
				    * freed */
{
    MacFileType * mfPtr, * toFree;

    mfPtr = clausePtr->macTypes;
    while (mfPtr) {
	toFree = mfPtr;
	mfPtr=mfPtr->next;
	ckfree((char*)toFree);
    }
    clausePtr->macTypes = NULL;
}
