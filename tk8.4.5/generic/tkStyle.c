/* 
 * tkStyle.c --
 *
 *	This file implements the widget styles and themes support.
 *
 * Copyright (c) 1990-1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"

/*
 * The following structure is used to cache widget option specs matching an 
 * element's required options defined by Tk_ElementOptionSpecs. It also holds
 * information behind Tk_StyledElement opaque tokens.
 */

typedef struct StyledWidgetSpec {
    struct StyledElement *elementPtr;	/* Pointer to the element holding this
					 * structure. */
    Tk_OptionTable optionTable;		/* Option table for the widget class 
					 * using the element. */
    CONST Tk_OptionSpec **optionsPtr;	/* Table of option spec pointers, 
					 * matching the option list provided 
					 * during element registration. 
					 * Malloc'd. */
} StyledWidgetSpec;

/*
 * Elements are declared using static templates. But static
 * information must be completed by dynamic information only
 * accessible at runtime. For each registered element, an instance of
 * the following structure is stored in each style engine and used to
 * cache information about the widget types (identified by their
 * optionTable) that use the given element.
 */

typedef struct StyledElement {
    struct Tk_ElementSpec *specPtr;	
				/* Filled with template provided during 
				 * registration. NULL means no implementation 
				 * is available for the current engine. */ 
    int nbWidgetSpecs;		/* Size of the array below. Number of distinct 
				 * widget classes (actually, distinct option 
				 * tables) that used the element so far. */
    StyledWidgetSpec *widgetSpecs;	
				/* See above for the structure definition.
				 * Table grows dynamically as new widgets
				 * use the element. Malloc'd. */
} StyledElement;

/*
 * The following structure holds information behind Tk_StyleEngine opaque 
 * tokens.
 */

typedef struct StyleEngine {
    CONST char *name;		/* Name of engine. Points to a hash key. */
    StyledElement *elements;	/* Table of widget element descriptors. Each 
				 * element is indexed by a unique system-wide 
				 * ID. Table grows dynamically as new elements 
				 * are registered. Malloc'd*/
    struct StyleEngine *parentPtr;	
				/* Parent engine. Engines may be layered to form
				 * a fallback chain, terminated by the default 
				 * system engine. */
} StyleEngine;

/*
 * Styles are instances of style engines. The following structure holds 
 * information behind Tk_Style opaque tokens.
 */

typedef struct Style {
    int refCount;		/* Number of active uses of this style.
				 * If this count is 0, then this Style
				 * structure is no longer valid. */
    Tcl_HashEntry *hashPtr;	/* Entry in style table for this structure,
				 * used when deleting it. */
    CONST char *name;		/* Name of style. Points to a hash key. */
    StyleEngine *enginePtr;	/* Style engine of which the style is an 
				 * instance. */
    ClientData clientData;	/* Data provided during registration. */
} Style;

/*
 * Each registered element uses an instance of the following structure. 
 */

typedef struct Element {
    CONST char *name;		/* Name of element. Points to a hash key. */
    int id;			/* Id of element. */
    int genericId;		/* Id of generic element. */
    int created;		/* Boolean, whether the element was created 
				 * explicitly (was registered) or implicitly 
				 * (by a derived element). */
} Element;

/*
 * Thread-local data.
 */

typedef struct ThreadSpecificData {
    int nbInit;			/* Number of calls to the init proc. */
    Tcl_HashTable engineTable;	/* Map a name to a style engine. Keys are 
				 * strings, values are Tk_StyleEngine 
				 * pointers. */
    StyleEngine *defaultEnginePtr;	
				/* Default, core-defined style engine. Global 
				 * fallback for all engines. */
    Tcl_HashTable styleTable;	/* Map a name to a style. Keys are strings, 
				 * values are Tk_Style pointers.*/
    int nbElements;		/* Size of the below tables. */
    Tcl_HashTable elementTable;	/* Map a name to an element Id. Keys are 
				 * strings, values are integer element IDs. */
    Element *elements;		/* Array of Elements. */				
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * Forward declarations for procedures defined later in this file:
 */

/* TODO: sort alpha. */
static int		CreateElement _ANSI_ARGS_((CONST char *name,
			    int create));
static void		DupStyleObjProc _ANSI_ARGS_((Tcl_Obj *srcObjPtr,
			    Tcl_Obj *dupObjPtr));
static void		FreeElement _ANSI_ARGS_((Element *elementPtr));
static void		FreeStyle _ANSI_ARGS_((Style *stylePtr));
static void		FreeStyledElement _ANSI_ARGS_((
			    StyledElement *elementPtr));
static void		FreeStyleEngine _ANSI_ARGS_((
			    StyleEngine *enginePtr));
static void		FreeStyleObjProc _ANSI_ARGS_((Tcl_Obj *objPtr));
static void		FreeWidgetSpec _ANSI_ARGS_((
			    StyledWidgetSpec *widgetSpecPtr));
static StyledElement *	GetStyledElement _ANSI_ARGS_((
			    StyleEngine *enginePtr, int elementId));
static StyledWidgetSpec * GetWidgetSpec _ANSI_ARGS_((StyledElement *elementPtr,
			    Tk_OptionTable optionTable));
static void		InitElement _ANSI_ARGS_((Element *elementPtr, 
			    CONST char *name, int id, int genericId, 
			    int created));
static void		InitStyle _ANSI_ARGS_((Style *stylePtr, 
			    Tcl_HashEntry *hashPtr, CONST char *name, 
			    StyleEngine *enginePtr, ClientData clientData));
static void		InitStyledElement _ANSI_ARGS_((
			    StyledElement *elementPtr));
static void		InitStyleEngine _ANSI_ARGS_((StyleEngine *enginePtr,
			    CONST char *name, StyleEngine *parentPtr));
static void		InitWidgetSpec _ANSI_ARGS_((
			    StyledWidgetSpec *widgetSpecPtr, 
			    StyledElement *elementPtr, 
			    Tk_OptionTable optionTable));
static int		SetStyleFromAny _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));

/*
 * The following structure defines the implementation of the "style" Tcl
 * object, used for drawing. The internalRep.otherValuePtr field of
 * each style object points to the Style structure for the stylefont, or
 * NULL.
 */

static Tcl_ObjType styleObjType = {
    "style",			/* name */
    FreeStyleObjProc,		/* freeIntRepProc */
    DupStyleObjProc,		/* dupIntRepProc */
    NULL,			/* updateStringProc */
    SetStyleFromAny		/* setFromAnyProc */
};

/*
 *---------------------------------------------------------------------------
 *
 * TkStylePkgInit --
 *
 *	This procedure is called when an application is created.  It
 *	initializes all the structures that are used by the style
 *	package on a per application basis.
 *
 * Results:
 *	Stores data in thread-local storage.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

void
TkStylePkgInit(mainPtr)
    TkMainInfo *mainPtr;	/* The application being created. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (tsdPtr->nbInit != 0) return;

    /*
     * Initialize tables.
     */

    Tcl_InitHashTable(&tsdPtr->engineTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&tsdPtr->styleTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&tsdPtr->elementTable, TCL_STRING_KEYS);
    tsdPtr->nbElements = 0;
    tsdPtr->elements = NULL;

    /*
     * Create the default system engine.
     */
    
    tsdPtr->defaultEnginePtr = 
	    (StyleEngine *) Tk_RegisterStyleEngine(NULL, NULL);

    /*
     * Create the default system style.
     */

    Tk_CreateStyle(NULL, (Tk_StyleEngine) tsdPtr->defaultEnginePtr, 
	    (ClientData) 0);

    tsdPtr->nbInit++;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkStylePkgFree --
 *
 *	This procedure is called when an application is deleted.  It
 *	deletes all the structures that were used by the style package
 *	for this application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

void
TkStylePkgFree(mainPtr)
    TkMainInfo *mainPtr;	/* The application being deleted. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;
    StyleEngine *enginePtr;
    int i;

    tsdPtr->nbInit--;
    if (tsdPtr->nbInit != 0) return;

    /*
     * Free styles.
     */

    entryPtr = Tcl_FirstHashEntry(&tsdPtr->styleTable, &search);
    while (entryPtr != NULL) {
	ckfree((char *) Tcl_GetHashValue(entryPtr));
	entryPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&tsdPtr->styleTable);

    /*
     * Free engines.
     */

    entryPtr = Tcl_FirstHashEntry(&tsdPtr->engineTable, &search);
    while (entryPtr != NULL) {
	enginePtr = (StyleEngine *) Tcl_GetHashValue(entryPtr);
	FreeStyleEngine(enginePtr);
	ckfree((char *) enginePtr);
	entryPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&tsdPtr->engineTable);

    /*
     * Free elements.
     */

    for (i = 0; i < tsdPtr->nbElements; i++) {
	FreeElement(tsdPtr->elements+i);
    }
    Tcl_DeleteHashTable(&tsdPtr->elementTable);
    ckfree((char *) tsdPtr->elements);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_RegisterStyleEngine --
 *
 *	This procedure is called to register a new style engine. Style engines
 *	are stored in thread-local space.
 *
 * Results:
 *	The newly allocated engine.
 *
 * Side effects:
 *	Memory allocated. Data added to thread-local table.
 *
 *---------------------------------------------------------------------------
 */

Tk_StyleEngine
Tk_RegisterStyleEngine(name, parent)
    CONST char *name;		/* Name of the engine to create. NULL or empty
				 * means the default system engine. */
    Tk_StyleEngine parent;	/* The engine's parent. NULL means the default 
				 * system engine. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr;
    int newEntry;
    StyleEngine *enginePtr;

    /*
     * Attempt to create a new entry in the engine table. 
     */

    entryPtr = Tcl_CreateHashEntry(&tsdPtr->engineTable, (name?name:""), 
	    &newEntry);
    if (!newEntry) {
	/*
	 * An engine was already registered by that name.
	 */

	return NULL;
    }

    /*
     * Allocate and intitialize a new engine.
     */

    enginePtr = (StyleEngine *) ckalloc(sizeof(StyleEngine));
    InitStyleEngine(enginePtr, Tcl_GetHashKey(&tsdPtr->engineTable, entryPtr),
	    (StyleEngine *) parent);
    Tcl_SetHashValue(entryPtr, (ClientData) enginePtr);

    return (Tk_StyleEngine) enginePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitStyleEngine --
 *
 *	Initialize a newly allocated style engine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

static void
InitStyleEngine(enginePtr, name, parentPtr)
    StyleEngine *enginePtr;	/* Points to an uninitialized engine. */
    CONST char *name;		/* Name of the registered engine. NULL or empty
				 * means the default system engine. Usually
				 * points to the hash key. */
    StyleEngine *parentPtr;	/* The engine's parent. NULL means the default 
				 * system engine. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    int elementId;

    if (name == NULL || *name == '\0') {
	/*
	 * This is the default style engine.
	 */

	enginePtr->parentPtr = NULL;

    } else if (parentPtr == NULL) {
	/*
	 * The default style engine is the parent.
	 */

	enginePtr->parentPtr = tsdPtr->defaultEnginePtr;

    } else {
	enginePtr->parentPtr = parentPtr;
    }

    /* 
     * Allocate and initialize elements array. 
     */

    if (tsdPtr->nbElements > 0) {
	enginePtr->elements = (StyledElement *) ckalloc(
		sizeof(StyledElement) * tsdPtr->nbElements);
	for (elementId = 0; elementId < tsdPtr->nbElements; elementId++) {
	    InitStyledElement(enginePtr->elements+elementId);
	}
    } else {
	enginePtr->elements = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeStyleEngine --
 *
 *	Free an engine and its associated data.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeStyleEngine(enginePtr)
    StyleEngine *enginePtr;	/* The style engine to free. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    int elementId;

    /*
     * Free allocated elements.
     */

    for (elementId = 0; elementId < tsdPtr->nbElements; elementId++) {
	FreeStyledElement(enginePtr->elements+elementId);
    }
    ckfree((char *) enginePtr->elements);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetStyleEngine --
 *
 *	Retrieve a registered style engine by its name.
 *
 * Results:
 *	A pointer to the style engine, or NULL if none found.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tk_StyleEngine
Tk_GetStyleEngine(name)
    CONST char *name;		/* Name of the engine to retrieve. NULL or
				 * empty means the default system engine. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr;

    if (name == NULL) {
	return (Tk_StyleEngine) tsdPtr->defaultEnginePtr;
    }

    entryPtr = Tcl_FindHashEntry(&tsdPtr->engineTable, (name?name:""));
    if (!entryPtr) {
	return NULL;
    }

    return (Tk_StyleEngine) Tcl_GetHashValue(entryPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * InitElement --
 *
 *	Initialize a newly allocated element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
InitElement(elementPtr, name, id, genericId, created)
    Element *elementPtr;	/* Points to an uninitialized element.*/
    CONST char *name;		/* Name of the registered element. Usually
				 * points to the hash key. */
    int id;			/* Unique element ID. */
    int genericId;		/* ID of generic element. -1 means none. */
    int created;		/* Boolean, whether the element was created 
				 * explicitly (was registered) or implicitly 
				 * (by a derived element). */
{
    elementPtr->name = name;
    elementPtr->id = id;
    elementPtr->genericId = genericId;
    elementPtr->created = (created?1:0);
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeElement --
 *
 *	Free an element and its associated data.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeElement(elementPtr)
    Element *elementPtr;	/* The element to free. */
{
    /* Nothing to do. */
}

/*
 *---------------------------------------------------------------------------
 *
 * InitStyledElement --
 *
 *	Initialize a newly allocated styled element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
InitStyledElement(elementPtr)
    StyledElement *elementPtr;	/* Points to an uninitialized element.*/
{
    memset(elementPtr, 0, sizeof(StyledElement));
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeStyledElement --
 *
 *	Free a styled element and its associated data.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeStyledElement(elementPtr)
    StyledElement *elementPtr;	/* The styled element to free. */
{
    int i;

    /*
     * Free allocated widget specs.
     */

    for (i = 0; i < elementPtr->nbWidgetSpecs; i++) {
	FreeWidgetSpec(elementPtr->widgetSpecs+i);
    }
    ckfree((char *) elementPtr->widgetSpecs);
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateElement --
 *
 *	Find an existing or create a new element.
 *
 * Results:
 *	The unique ID for the created or found element.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

static int
CreateElement(name, create)
    CONST char *name;	/* Name of the element. */
    int create;		/* Boolean, whether the element is being created 
			 * explicitly (being registered) or implicitly (by a 
			 * derived element). */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr, *engineEntryPtr;
    Tcl_HashSearch search;
    int newEntry;
    int elementId, genericId = -1;
    char *dot;
    StyleEngine *enginePtr;

    /*
     * Find or create the element.
     */

    entryPtr = Tcl_CreateHashEntry(&tsdPtr->elementTable, name, &newEntry);
    if (!newEntry) {
	elementId = (int) Tcl_GetHashValue(entryPtr);
	if (create) {
	    tsdPtr->elements[elementId].created = 1;
	}
	return elementId;
    }

    /*
     * The element didn't exist. If it's a derived element, find or
     * create its generic element ID.
     */

    dot = strchr(name, '.');
    if (dot) {
	genericId = CreateElement(dot+1, 0);
    }

    elementId = tsdPtr->nbElements++;
    Tcl_SetHashValue(entryPtr, (ClientData) elementId);

    /*
     * Reallocate element table.
     */

    tsdPtr->elements = (Element *) ckrealloc((char *) tsdPtr->elements, 
	    sizeof(Element) * tsdPtr->nbElements);
    InitElement(tsdPtr->elements+elementId, 
	    Tcl_GetHashKey(&tsdPtr->elementTable, entryPtr), elementId,
	    genericId, create);

    /*
     * Reallocate style engines' element table.
     */

    engineEntryPtr = Tcl_FirstHashEntry(&tsdPtr->engineTable, &search);
    while (engineEntryPtr != NULL) {
	enginePtr = (StyleEngine *) Tcl_GetHashValue(engineEntryPtr);

	enginePtr->elements = (StyledElement *) ckrealloc(
		(char *) enginePtr->elements, 
		sizeof(StyledElement) * tsdPtr->nbElements);
	InitStyledElement(enginePtr->elements+elementId);

	engineEntryPtr = Tcl_NextHashEntry(&search);
    }

    return elementId;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetElementId --
 *
 *	Find an existing element.
 *
 * Results:
 *	The unique ID for the found element, or -1 if not found.
 *
 * Side effects:
 *	Generic elements may be created.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_GetElementId(name)
    CONST char *name;		/* Name of the element. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr;
    int genericId = -1;
    char *dot;

    /*
     * Find the element Id.
     */

    entryPtr = Tcl_FindHashEntry(&tsdPtr->elementTable, name);
    if (entryPtr) {
	return (int) Tcl_GetHashValue(entryPtr);
    }

    /*
     * Element not found. If the given name was derived, then first search for 
     * the generic element. If found, create the new derived element.
     */

    dot = strchr(name, '.');
    if (!dot) {
	return -1;
    }
    genericId = Tk_GetElementId(dot+1);
    if (genericId == -1) {
	return -1;
    }
    if (!tsdPtr->elements[genericId].created) {
	/* 
	 * The generic element was created implicitly and thus has no real
	 * existence.
	 */

	return -1;
    } else {
	/*
	 * The generic element was created explicitly. Create the derived
	 * element.
	 */

	return CreateElement(name, 1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_RegisterStyledElement --
 *
 *	Register an implementation of a new or existing element for the
 *	given style engine.
 *
 * Results:
 *	The unique ID for the created or found element.
 *
 * Side effects:
 *	Elements may be created. Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_RegisterStyledElement(engine, templatePtr)
    Tk_StyleEngine engine;		/* Style engine providing the
					 * implementation. */
    Tk_ElementSpec *templatePtr;	/* Static template information about
					 * the element. */
{
    int elementId;
    StyledElement *elementPtr;
    Tk_ElementSpec *specPtr;
    int nbOptions;
    register Tk_ElementOptionSpec *srcOptions, *dstOptions;

    if (templatePtr->version != TK_STYLE_VERSION_1) {
	/*
	 * Version mismatch. Do nothing.
	 */

	return -1;
    }

    if (engine == NULL) {
	engine = Tk_GetStyleEngine(NULL);
    }

    /*
     * Register the element, allocating storage in the various engines if 
     * necessary.
     */

    elementId = CreateElement(templatePtr->name, 1);

    /*
     * Initialize the styled element.
     */

    elementPtr = ((StyleEngine *) engine)->elements+elementId;

    specPtr = (Tk_ElementSpec *) ckalloc(sizeof(Tk_ElementSpec));
    specPtr->version = templatePtr->version;
    specPtr->name = ckalloc(strlen(templatePtr->name)+1);
    strcpy(specPtr->name, templatePtr->name);
    nbOptions = 0;
    for (nbOptions = 0, srcOptions = templatePtr->options;
	 srcOptions->name != NULL;
	 nbOptions++, srcOptions++);
    specPtr->options = (Tk_ElementOptionSpec *) ckalloc(
	    sizeof(Tk_ElementOptionSpec) * (nbOptions+1));
    for (srcOptions = templatePtr->options, dstOptions = specPtr->options;
	 /* End condition within loop */;
	 srcOptions++, dstOptions++) {
	if (srcOptions->name == NULL) {
	    dstOptions->name = NULL;
	    break;
	}

	dstOptions->name = ckalloc(strlen(srcOptions->name)+1);
	strcpy(dstOptions->name, srcOptions->name);
	dstOptions->type = srcOptions->type;
    }
    specPtr->getSize = templatePtr->getSize;
    specPtr->getBox = templatePtr->getBox;
    specPtr->getBorderWidth = templatePtr->getBorderWidth;
    specPtr->draw = templatePtr->draw;

    elementPtr->specPtr = specPtr;
    elementPtr->nbWidgetSpecs = 0;
    elementPtr->widgetSpecs = NULL;

    return elementId;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetStyledElement --
 *
 *	Get a registered implementation of an existing element for the
 *	given style engine.
 *
 * Results:
 *	The styled element descriptor, or NULL if not found.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static StyledElement *
GetStyledElement(enginePtr, elementId)
    StyleEngine *enginePtr;	/* Style engine providing the implementation. 
				 * NULL means the default system engine. */
    int elementId;		/* Unique element ID */{
    StyledElement *elementPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    StyleEngine *enginePtr2;

    if (enginePtr == NULL) {
	enginePtr = tsdPtr->defaultEnginePtr;
    }

    while (elementId >= 0 && elementId < tsdPtr->nbElements) {
	/*
	 * Look for an implemented element through the engine chain.
	 */

	enginePtr2 = enginePtr;
	do {
	    elementPtr = enginePtr2->elements+elementId;
	    if (elementPtr->specPtr != NULL) {
		return elementPtr;
	    }
	    enginePtr2 = enginePtr2->parentPtr;
	} while (enginePtr2 != NULL);

	/*
	 * None found, try with the generic element.
	 */

	elementId = tsdPtr->elements[elementId].genericId;
    }

    /*
     * No matching element found.
     */

    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitWidgetSpec --
 *
 *	Initialize a newly allocated widget spec.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

static void
InitWidgetSpec(widgetSpecPtr, elementPtr, optionTable)
    StyledWidgetSpec *widgetSpecPtr;	/* Points to an uninitialized widget
					 * spec. */
    StyledElement *elementPtr;		/* Styled element descriptor. */
    Tk_OptionTable optionTable;		/* The widget's option table. */
{
    int i, nbOptions;
    Tk_ElementOptionSpec *elementOptionPtr;
    CONST Tk_OptionSpec *widgetOptionPtr;

    widgetSpecPtr->elementPtr = elementPtr;
    widgetSpecPtr->optionTable = optionTable;
    
    /*
     * Count the number of options.
     */

    for (nbOptions = 0, elementOptionPtr = elementPtr->specPtr->options; 
	    elementOptionPtr->name != NULL;
	    nbOptions++, elementOptionPtr++) {
    }

    /*
     * Build the widget option list.
     */

    widgetSpecPtr->optionsPtr = (CONST Tk_OptionSpec **) ckalloc(
	    sizeof(Tk_OptionSpec *) * nbOptions);
    for (i = 0, elementOptionPtr = elementPtr->specPtr->options; 
	    i < nbOptions;
	    i++, elementOptionPtr++) {
	widgetOptionPtr = TkGetOptionSpec(elementOptionPtr->name, optionTable);

	/*
	 * Check that the widget option type is compatible with one of the 
	 * element's required types.
	 */

	if (   elementOptionPtr->type == TK_OPTION_END
	    || elementOptionPtr->type == widgetOptionPtr->type) {
	    widgetSpecPtr->optionsPtr[i] = widgetOptionPtr;
	} else {
	    widgetSpecPtr->optionsPtr[i] = NULL;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeWidgetSpec --
 *
 *	Free a widget spec and its associated data.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeWidgetSpec(widgetSpecPtr)
    StyledWidgetSpec *widgetSpecPtr;	/* The widget spec to free. */
{
    ckfree((char *) widgetSpecPtr->optionsPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetWidgetSpec --
 *
 *	Return a new or existing widget spec for the given element and
 *	widget type (identified by its option table).
 *
 * Results:
 *	A pointer to the matching widget spec.
 *
 * Side effects:
 *	Memory may be allocated.
 *
 *---------------------------------------------------------------------------
 */

static StyledWidgetSpec *
GetWidgetSpec(elementPtr, optionTable)
    StyledElement *elementPtr;		/* Styled element descriptor. */
    Tk_OptionTable optionTable;		/* The widget's option table. */
{
    StyledWidgetSpec *widgetSpecPtr;
    int i;

    /*
     * Try to find an existing widget spec.
     */

    for (i = 0; i < elementPtr->nbWidgetSpecs; i++) {
	widgetSpecPtr = elementPtr->widgetSpecs+i;
	if (widgetSpecPtr->optionTable == optionTable) {
	    return widgetSpecPtr;
	}
    }

    /*
     * Create and initialize a new widget spec.
     */

    i = elementPtr->nbWidgetSpecs++;
    elementPtr->widgetSpecs = (StyledWidgetSpec *) ckrealloc(
	    (char *) elementPtr->widgetSpecs, 
	    sizeof(StyledWidgetSpec) * elementPtr->nbWidgetSpecs);
    widgetSpecPtr = elementPtr->widgetSpecs+i;
    InitWidgetSpec(widgetSpecPtr, elementPtr, optionTable);

    return widgetSpecPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetStyledElement --
 *
 *	This procedure returns a styled instance of the given element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cached data may be allocated or updated.
 *
 *---------------------------------------------------------------------------
 */

Tk_StyledElement
Tk_GetStyledElement(style, elementId, optionTable)
    Tk_Style style;		/* The widget style. */
    int elementId;		/* Unique element ID. */
    Tk_OptionTable optionTable;	/* Option table for the widget. */
{
    Style *stylePtr = (Style *) style;
    StyledElement *elementPtr;

    /*
     * Get an element implementation and call corresponding hook.
     */

    elementPtr = GetStyledElement((stylePtr?stylePtr->enginePtr:NULL), 
	    elementId);
    if (!elementPtr) {
	return NULL;
    }

    return (Tk_StyledElement) GetWidgetSpec(elementPtr, optionTable);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetElementSize --
 *
 *	This procedure computes the size of the given widget element according
 *	to its style.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cached data may be allocated or updated.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_GetElementSize(style, element, recordPtr, tkwin, width, height, inner, widthPtr, 
	heightPtr)
    Tk_Style style;			/* The widget style. */
    Tk_StyledElement element;		/* The styled element, previously
					 * returned by Tk_GetStyledElement. */
    char *recordPtr;			/* The widget record. */
    Tk_Window tkwin;			/* The widget window. */
    int width, height;			/* Requested size. */
    int inner;				/* Boolean. If TRUE, compute the outer
					 * size according to the requested
					 * minimum inner size. If FALSE, compute
					 * the inner size according to the 
					 * requested maximum outer size. */
    int *widthPtr, *heightPtr;		/* Returned size. */
{
    Style *stylePtr = (Style *) style;
    StyledWidgetSpec *widgetSpecPtr = (StyledWidgetSpec *) element;

    widgetSpecPtr->elementPtr->specPtr->getSize(stylePtr->clientData, 
	    recordPtr, widgetSpecPtr->optionsPtr, tkwin, width, height, inner, 
	    widthPtr, heightPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetElementBox --
 *
 *	This procedure computes the bounding or inscribed box coordinates
 *	of the given widget element according to its style and within the
 *	given limits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cached data may be allocated or updated.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_GetElementBox(style, element, recordPtr, tkwin, x, y, width, height, inner, 
	xPtr, yPtr, widthPtr, heightPtr)
    Tk_Style style;			/* The widget style. */
    Tk_StyledElement element;		/* The styled element, previously
					 * returned by Tk_GetStyledElement. */
    char *recordPtr;			/* The widget record. */
    Tk_Window tkwin;			/* The widget window. */
    int x, y;				/* Top left corner of available area. */
    int width, height;			/* Size of available area. */
    int inner;				/* Boolean. If TRUE, compute the 
					 * bounding box according to the 
					 * requested inscribed box size. If 
					 * FALSE, compute the inscribed box 
					 * according to the requested bounding
					 * box. */
    int *xPtr, *yPtr;			/* Returned top left corner. */
    int *widthPtr, *heightPtr;		/* Returned size. */
{
    Style *stylePtr = (Style *) style;
    StyledWidgetSpec *widgetSpecPtr = (StyledWidgetSpec *) element;

    widgetSpecPtr->elementPtr->specPtr->getBox(stylePtr->clientData, 
	    recordPtr, widgetSpecPtr->optionsPtr, tkwin, x, y, width, height, 
	    inner, xPtr, yPtr, widthPtr, heightPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetElementBorderWidth --
 *
 *	This procedure computes the border widthof the given widget element 
 *	according to its style and within the given limits.
 *
 * Results:
 *	Border width in pixels. This value is uniform for all four sides.
 *
 * Side effects:
 *	Cached data may be allocated or updated.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_GetElementBorderWidth(style, element, recordPtr, tkwin)
    Tk_Style style;			/* The widget style. */
    Tk_StyledElement element;		/* The styled element, previously
					 * returned by Tk_GetStyledElement. */
    char *recordPtr;			/* The widget record. */
    Tk_Window tkwin;			/* The widget window. */
{
    Style *stylePtr = (Style *) style;
    StyledWidgetSpec *widgetSpecPtr = (StyledWidgetSpec *) element;

    return widgetSpecPtr->elementPtr->specPtr->getBorderWidth(
	    stylePtr->clientData, recordPtr, widgetSpecPtr->optionsPtr, tkwin);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_DrawElement --
 *
 *	This procedure draw the given widget element in a given drawable area.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Cached data may be allocated or updated.
 *
 *---------------------------------------------------------------------------
 */

void
Tk_DrawElement(style, element, recordPtr, tkwin, d, x, y, width, height, state)
    Tk_Style style;			/* The widget style. */
    Tk_StyledElement element;		/* The styled element, previously
					 * returned by Tk_GetStyledElement. */
    char *recordPtr;			/* The widget record. */
    Tk_Window tkwin;			/* The widget window. */
    Drawable d;				/* Where to draw element. */
    int x, y;				/* Top left corner of element. */
    int width, height;			/* Size of element. */
    int state;				/* Drawing state flags. */
{
    Style *stylePtr = (Style *) style;
    StyledWidgetSpec *widgetSpecPtr = (StyledWidgetSpec *) element;

    widgetSpecPtr->elementPtr->specPtr->draw(stylePtr->clientData, 
	    recordPtr, widgetSpecPtr->optionsPtr, tkwin, d, x, y, width, 
	    height, state);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_CreateStyle --
 *
 *	This procedure is called to create a new style as an instance of the
 *	given engine. Styles are stored in thread-local space.
 *
 * Results:
 *	The newly allocated style.
 *
 * Side effects:
 *	Memory allocated. Data added to thread-local table. The style's
 *	refCount is incremented.
 *
 *---------------------------------------------------------------------------
 */

Tk_Style
Tk_CreateStyle(name, engine, clientData)
    CONST char *name;		/* Name of the style to create. NULL or empty
				 * means the default system style. */
    Tk_StyleEngine engine;	/* The style engine. */
    ClientData clientData;	/* Private data passed as is to engine code. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr;
    int newEntry;
    Style *stylePtr;

    /*
     * Attempt to create a new entry in the style table. 
     */

    entryPtr = Tcl_CreateHashEntry(&tsdPtr->styleTable, (name?name:""), 
	    &newEntry);
    if (!newEntry) {
	/*
	 * A style was already registered by that name.
	 */

	return NULL;
    }

    /*
     * Allocate and intitialize a new style.
     */

    stylePtr = (Style *) ckalloc(sizeof(Style));
    InitStyle(stylePtr, entryPtr, Tcl_GetHashKey(&tsdPtr->styleTable, entryPtr),
	    (engine?(StyleEngine *) engine:tsdPtr->defaultEnginePtr), clientData);
    Tcl_SetHashValue(entryPtr, (ClientData) stylePtr);
    stylePtr->refCount++;

    return (Tk_Style) stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_NameOfStyle --
 *
 *	Given a style, return its registered name.
 *
 * Results:
 *	The return value is the name that was passed to Tk_CreateStyle() to 
 *	create the style.  The storage for the returned string is private
 *	(it points to the corresponding hash key) The caller should not modify 
 *	this string.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

CONST char *
Tk_NameOfStyle(style)
    Tk_Style style;		/* Style whose name is desired. */
{
    Style *stylePtr = (Style *) style;

    return stylePtr->name;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitStyle --
 *
 *	Initialize a newly allocated style.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
InitStyle(stylePtr, hashPtr, name, enginePtr, clientData)
    Style *stylePtr;		/* Points to an uninitialized style. */
    Tcl_HashEntry *hashPtr;	/* Hash entry for the registered style. */
    CONST char *name;		/* Name of the registered style. NULL or empty
				 * means the default system style. Usually
				 * points to the hash key. */
    StyleEngine *enginePtr;	/* The style engine. */
    ClientData clientData;	/* Private data passed as is to engine code. */
{
    stylePtr->refCount = 0;
    stylePtr->hashPtr = hashPtr;
    stylePtr->name = name;
    stylePtr->enginePtr = enginePtr;
    stylePtr->clientData = clientData;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeStyle --
 *
 *	Free a style and its associated data.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeStyle(stylePtr)
    Style *stylePtr;		/* The style to free. */
{
    /* Nothing to do. */
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_GetStyle --
 *
 *	Retrieve a registered style by its name.
 *
 * Results:
 *	A pointer to the style engine, or NULL if none found.  In the latter
 *	case and if the interp is not NULL, an error message is left in the 
 *	interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tk_Style
Tk_GetStyle(interp, name)
    Tcl_Interp *interp;		/* Interp for error return. */
    CONST char *name;		/* Name of the style to retrieve. NULL or empty
				 * means the default system style. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_HashEntry *entryPtr;
    Style *stylePtr;

    /*
     * Search for a corresponding entry in the style table. 
     */

    entryPtr = Tcl_FindHashEntry(&tsdPtr->styleTable, (name?name:""));
    if (entryPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "style \"", name, "\" doesn't exist", NULL);
	}
	return (Tk_Style) NULL;
    }
    stylePtr = (Style *) Tcl_GetHashValue(entryPtr);
    stylePtr->refCount++;

    return (Tk_Style) stylePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_FreeStyle --
 *
 *	Free a style previously created by Tk_CreateStyle.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	The style's refCount is decremented. If it reaches zero, the style
 *	is freed.
 *
 *---------------------------------------------------------------------------
 */

void 
Tk_FreeStyle(style)
    Tk_Style style;		/* The style to free. */
{
    Style *stylePtr = (Style *) style;

    if (stylePtr == NULL) {
	return;
    }
    stylePtr->refCount--;
    if (stylePtr->refCount > 0) {
	return;
    }
    
    /*
     * Keep the default style alive.
     */

    if (*stylePtr->name == '\0') {
	stylePtr->refCount = 1;
	return;
    }

    Tcl_DeleteHashEntry(stylePtr->hashPtr);
    FreeStyle(stylePtr);
    ckfree((char *) stylePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_AllocStyleFromObj -- 
 *
 *	Map the string name of a style to a corresponding Tk_Style. The style 
 *	must have already been created by Tk_CreateStyle.
 *
 * Results:
 *	The return value is a token for the style that matches objPtr, or 
 *	NULL if none found.  If NULL is returned, an error message will be 
 *	left in interp's result object.
 *
 * Side effects:
 * 	The style's reference count is incremented. For each call to this 
 *	procedure, there should eventually be a call to Tk_FreeStyle() or 
 *	Tk_FreeStyleFromObj() so that the database is cleaned up when styles
 *	aren't in use anymore.
 *
 *---------------------------------------------------------------------------
 */

Tk_Style
Tk_AllocStyleFromObj(interp, objPtr)
    Tcl_Interp *interp;		/* Interp for error return. */
    Tcl_Obj *objPtr;		/* Object containing name of the style to
				 * retrieve. */
{
    Style *stylePtr;

    if (objPtr->typePtr != &styleObjType) {
	SetStyleFromAny(interp, objPtr);
	stylePtr = (Style *) objPtr->internalRep.otherValuePtr;
    } else {
	stylePtr = (Style *) objPtr->internalRep.otherValuePtr;
	stylePtr->refCount++;
    }

    return (Tk_Style) stylePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetStyleFromObj --
 *
 *	Find the style that corresponds to a given object.  The style must
 *	have already been created by Tk_CreateStyle.
 *
 * Results:
 *	The return value is a token for the style that matches objPtr, or 
 *	NULL if none found.
 *
 * Side effects:
 *	If the object is not already a style ref, the conversion will free
 *	any old internal representation. 
 *
 *----------------------------------------------------------------------
 */

Tk_Style
Tk_GetStyleFromObj(objPtr)
    Tcl_Obj *objPtr;		/* The object from which to get the style. */
{
    if (objPtr->typePtr != &styleObjType) {
	SetStyleFromAny((Tcl_Interp *) NULL, objPtr);
    }

    return (Tk_Style) objPtr->internalRep.otherValuePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_FreeStyleFromObj -- 
 *
 *	Called to release a style inside a Tcl_Obj *.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the object is a style ref, the conversion will free its 
 *	internal representation. 
 *
 *---------------------------------------------------------------------------
 */

void
Tk_FreeStyleFromObj(objPtr)
    Tcl_Obj *objPtr;		/* The Tcl_Obj * to be freed. */
{
    if (objPtr->typePtr == &styleObjType) {
	FreeStyleObjProc(objPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetStyleFromAny --
 *
 *	Convert the internal representation of a Tcl object to the
 *	style internal form.
 *
 * Results:
 *	Always returns TCL_OK.  If an error occurs is returned (e.g. the
 *	style doesn't exist), an error message will be left in interp's 
 *	result.
 *
 * Side effects:
 *	The object is left with its typePtr pointing to styleObjType.
 *	The reference count is incremented (in Tk_GetStyle()).
 *
 *----------------------------------------------------------------------
 */

static int
SetStyleFromAny(interp, objPtr)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr;		/* The object to convert. */
{
    Tcl_ObjType *typePtr;
    char *name;

    /*
     * Free the old internalRep before setting the new one. 
     */

    name = Tcl_GetString(objPtr);
    typePtr = objPtr->typePtr;
    if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
	(*typePtr->freeIntRepProc)(objPtr);
    }

    objPtr->typePtr = &styleObjType;
    objPtr->internalRep.otherValuePtr = (VOID *) Tk_GetStyle(interp, name);

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeStyleObjProc -- 
 *
 *	This proc is called to release an object reference to a style.
 *	Called when the object's internal rep is released.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count is decremented (in Tk_FreeStyle()).
 *
 *---------------------------------------------------------------------------
 */

static void
FreeStyleObjProc(objPtr)
    Tcl_Obj *objPtr;		/* The object we are releasing. */
{
    Style *stylePtr = (Style *) objPtr->internalRep.otherValuePtr;

    if (stylePtr != NULL) {
	Tk_FreeStyle((Tk_Style) stylePtr);
	objPtr->internalRep.otherValuePtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DupStyleObjProc -- 
 *
 *	When a cached style object is duplicated, this is called to
 *	update the internal reps.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The style's refCount is incremented and the internal rep of the copy 
 *	is set to point to it.
 *
 *---------------------------------------------------------------------------
 */

static void
DupStyleObjProc(srcObjPtr, dupObjPtr)
    Tcl_Obj *srcObjPtr;		/* The object we are copying from. */
    Tcl_Obj *dupObjPtr;		/* The object we are copying to. */
{
    Style *stylePtr = (Style *) srcObjPtr->internalRep.otherValuePtr;
    
    dupObjPtr->typePtr = srcObjPtr->typePtr;
    dupObjPtr->internalRep.otherValuePtr = (VOID *) stylePtr;

    if (stylePtr != NULL) {
	stylePtr->refCount++;
    }
}
