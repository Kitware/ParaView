/*
 * tkFileFilter.h --
 *
 *	Declarations for the file filter processing routines needed by
 *	the file selection dialogs.
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 *
 */

#ifndef _TK_FILE_FILTER
#define _TK_FILE_FILTER

#ifdef MAC_TCL
#include <StandardFile.h>
#else
#define OSType long
#endif

#ifdef BUILD_tk
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

typedef struct GlobPattern {
    struct GlobPattern * next;		/* Chains to the next glob pattern
					 * in a glob pattern list */
    char * pattern;			/* String value of the pattern, such
					 * as "*.txt" or "*.*"
					 */
} GlobPattern;

typedef struct MacFileType {
    struct MacFileType * next;		/* Chains to the next mac file type
					 * in a mac file type list */
    OSType type;			/* Mac file type, such as 'TEXT' or
					 * 'GIFF' */
} MacFileType;

typedef struct FileFilterClause {
    struct FileFilterClause * next;	/* Chains to the next clause in
					 * a clause list */
    GlobPattern * patterns;		/* Head of glob pattern type list */
    GlobPattern * patternsTail;		/* Tail of glob pattern type list */
    MacFileType * macTypes;		/* Head of mac file type list */
    MacFileType * macTypesTail;		/* Tail of mac file type list */
} FileFilterClause;

typedef struct FileFilter {
    struct FileFilter * next;		/* Chains to the next filter
					 * in a filter list */
    char * name;			/* Name of the file filter,
					 * such as "Text Documents" */
    FileFilterClause * clauses;		/* Head of the clauses list */
    FileFilterClause * clausesTail;	/* Tail of the clauses list */
} FileFilter;

/*----------------------------------------------------------------------
 * FileFilterList --
 *
 * The routine TkGetFileFilters() translates the string value of the
 * -filefilters option into a FileFilterList structure, which consists
 * of a list of file filters.
 *
 * Each file filter consists of one or more clauses. Each clause has
 * one or more glob patterns and/or one or more Mac file types
 *----------------------------------------------------------------------
 */

typedef struct FileFilterList {
    FileFilter * filters;		/* Head of the filter list */
    FileFilter * filtersTail;		/* Tail of the filter list */
    int numFilters;			/* number of filters in the list */
} FileFilterList;

EXTERN void		TkFreeFileFilters _ANSI_ARGS_((
			    FileFilterList * flistPtr));
EXTERN void		TkInitFileFilters _ANSI_ARGS_((
			    FileFilterList * flistPtr));
EXTERN int		TkGetFileFilters _ANSI_ARGS_ ((Tcl_Interp *interp,
    			    FileFilterList * flistPtr, char * string,
			    int isWindows));

# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLIMPORT

#endif
