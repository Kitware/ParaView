/* 
 * tclMacOSA.c --
 *
 *	This contains the initialization routines, and the implementation of
 *	the OSA and Component commands.  These commands allow you to connect
 *	with the AppleScript or any other OSA component to compile and execute
 *	scripts.
 *
 * Copyright (c) 1996 Lucent Technologies and Jim Ingham
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "License Terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#define MAC_TCL

#include <Aliases.h>
#include <string.h>
#include <AppleEvents.h>
#include <AppleScript.h>
#include <OSA.h>
#include <OSAGeneric.h>
#include <Script.h>

#include <FullPath.h>
#include <components.h>

#include <resources.h>
#include <FSpCompat.h>
/* 
 * The following two Includes are from the More Files package.
 */
#include <MoreFiles.h>
#include <FullPath.h>

#include "tcl.h"
#include "tclInt.h"

/*
 * I need this only for the call to FspGetFullPath,
 * I'm really not poking my nose where it does not belong!
 */
#include "tclMacInt.h"

/*
 * Data structures used by the OSA code.
 */
typedef struct tclOSAScript {
    OSAID scriptID;
    OSType languageID;
    long modeFlags;
} tclOSAScript;

typedef struct tclOSAContext {
	OSAID contextID;
} tclOSAContext;

typedef struct tclOSAComponent {
	char *theName;
	ComponentInstance theComponent; /* The OSA Component represented */
	long componentFlags;
	OSType languageID;
	char *languageName;
	Tcl_HashTable contextTable;    /* Hash Table linking the context names & ID's */
	Tcl_HashTable scriptTable;
	Tcl_Interp *theInterp;
	OSAActiveUPP defActiveProc;
	long defRefCon;
} tclOSAComponent;

/*
 * Prototypes for static procedures. 
 */

static pascal OSErr	TclOSAActiveProc _ANSI_ARGS_((long refCon));
static int		TclOSACompileCmd _ANSI_ARGS_((Tcl_Interp *interp,
		 	    tclOSAComponent *OSAComponent, int argc,
			    char **argv));
static int 		tclOSADecompileCmd _ANSI_ARGS_((Tcl_Interp * Interp,
			    tclOSAComponent *OSAComponent, int argc,
			    char **argv));
static int 		tclOSADeleteCmd _ANSI_ARGS_((Tcl_Interp *interp,
			    tclOSAComponent *OSAComponent, int argc,
			    char **argv));
static int 		tclOSAExecuteCmd _ANSI_ARGS_((Tcl_Interp *interp,
			    tclOSAComponent *OSAComponent, int argc,
			    char **argv));
static int 		tclOSAInfoCmd _ANSI_ARGS_((Tcl_Interp *interp,
			    tclOSAComponent *OSAComponent, int argc,
			    char **argv));
static int 		tclOSALoadCmd _ANSI_ARGS_((Tcl_Interp *interp,
			    tclOSAComponent *OSAComponent, int argc,
			    char **argv));
static int 		tclOSARunCmd _ANSI_ARGS_((Tcl_Interp *interp,
			    tclOSAComponent *OSAComponent, int argc,
			    char **argv));
static int 		tclOSAStoreCmd _ANSI_ARGS_((Tcl_Interp *interp,
			    tclOSAComponent *OSAComponent, int argc, char
			    **argv));
static void		GetRawDataFromDescriptor _ANSI_ARGS_((AEDesc *theDesc,
			    Ptr destPtr, Size destMaxSize, Size *actSize));
static OSErr 		GetCStringFromDescriptor _ANSI_ARGS_((
			    AEDesc *sourceDesc, char *resultStr,
			    Size resultMaxSize,Size *resultSize));
static int 		Tcl_OSAComponentCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv)); 
static void 		getSortedHashKeys _ANSI_ARGS_((Tcl_HashTable *theTable,
			    char *pattern, Tcl_DString *theResult));
static int 		ASCIICompareProc _ANSI_ARGS_((const void *first,
			    const void *second));
static int 		Tcl_OSACmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv)); 
static void 		tclOSAClose _ANSI_ARGS_((ClientData clientData));
static void 		tclOSACloseAll _ANSI_ARGS_((ClientData clientData));
static tclOSAComponent *tclOSAMakeNewComponent _ANSI_ARGS_((Tcl_Interp *interp,
			    char *cmdName, char *languageName,
			    OSType scriptSubtype, long componentFlags));  
static int 		prepareScriptData _ANSI_ARGS_((int argc, char **argv,
			    Tcl_DString *scrptData ,AEDesc *scrptDesc)); 
static void 		tclOSAResultFromID _ANSI_ARGS_((Tcl_Interp *interp,
			    ComponentInstance theComponent, OSAID resultID));
static void 		tclOSAASError _ANSI_ARGS_((Tcl_Interp * interp,
			    ComponentInstance theComponent, char *scriptSource));
static int 		tclOSAGetContextID _ANSI_ARGS_((tclOSAComponent *theComponent, 
			    char *contextName, OSAID *theContext));
static void 		tclOSAAddContext _ANSI_ARGS_((tclOSAComponent *theComponent, 
			    char *contextName, const OSAID theContext));						
static int 		tclOSAMakeContext _ANSI_ARGS_((tclOSAComponent *theComponent, 
			    char *contextName, OSAID *theContext));						
static int 		tclOSADeleteContext _ANSI_ARGS_((tclOSAComponent *theComponent,
			    char *contextName)); 
static int 		tclOSALoad _ANSI_ARGS_((Tcl_Interp *interp, 
			    tclOSAComponent *theComponent, char *resourceName, 
			    int resourceNumber, char *fileName,OSAID *resultID));
static int 		tclOSAStore _ANSI_ARGS_((Tcl_Interp *interp, 
			    tclOSAComponent *theComponent, char *resourceName, 
			    int resourceNumber, char *fileName,char *scriptName));
static int 		tclOSAAddScript _ANSI_ARGS_((tclOSAComponent *theComponent,
			    char *scriptName, long modeFlags, OSAID scriptID)); 		
static int 		tclOSAGetScriptID _ANSI_ARGS_((tclOSAComponent *theComponent,
			    char *scriptName, OSAID *scriptID)); 
static tclOSAScript *	tclOSAGetScript _ANSI_ARGS_((tclOSAComponent *theComponent,
			    char *scriptName)); 
static int 		tclOSADeleteScript _ANSI_ARGS_((tclOSAComponent *theComponent,
			    char *scriptName,char *errMsg));

/*
 * "export" is a MetroWerks specific pragma.  It flags the linker that  
 * any symbols that are defined when this pragma is on will be exported 
 * to shared libraries that link with this library.
 */
 

#pragma export on
int Tclapplescript_Init( Tcl_Interp *interp );
#pragma export reset

/*
 *----------------------------------------------------------------------
 *
 * Tclapplescript_Init --
 *
 *	Initializes the the OSA command which opens connections to
 *	OSA components, creates the AppleScript command, which opens an 
 *	instance of the AppleScript component,and constructs the table of
 *	available languages.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *	Opens one connection to the AppleScript component, if 
 *	available.  Also builds up a table of available OSA languages,
 *	and creates the OSA command.
 *
 *----------------------------------------------------------------------
 */

int 
Tclapplescript_Init(
    Tcl_Interp *interp)		/* Tcl interpreter. */
{
    char *errMsg = NULL;
    OSErr myErr = noErr;
    Boolean gotAppleScript = false;
    Boolean GotOneOSALanguage = false;
    ComponentDescription compDescr = {
	kOSAComponentType,
	(OSType) 0,
	(OSType) 0,
	(long) 0,
	(long) 0
    }, *foundComp;
    Component curComponent = (Component) 0;
    ComponentInstance curOpenComponent;
    Tcl_HashTable *ComponentTable;
    Tcl_HashTable *LanguagesTable;
    Tcl_HashEntry *hashEntry;
    int newPtr;
    AEDesc componentName = { typeNull, NULL };
    char nameStr[32];			
    Size nameLen;
    long appleScriptFlags;
	
    /* 
     * Perform the required stubs magic...
     */
     	
    if (!Tcl_InitStubs(interp, "8.0", 0)) {
	return TCL_ERROR;
    }

    /* 
     * Here We Will Get The Available Osa Languages, Since They Can Only Be 
     * Registered At Startup...  If You Dynamically Load Components, This
     * Will Fail, But This Is Not A Common Thing To Do.
     */
	 
    LanguagesTable = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
	
    if (LanguagesTable == NULL) {
	panic("Memory Error Allocating Languages Hash Table");
    }
	
    Tcl_SetAssocData(interp, "OSAScript_LangTable", NULL, LanguagesTable);
    Tcl_InitHashTable(LanguagesTable, TCL_STRING_KEYS);
	
			
    while ((curComponent = FindNextComponent(curComponent, &compDescr)) != 0) {
	int nbytes = sizeof(ComponentDescription);
	foundComp = (ComponentDescription *)
	    ckalloc(sizeof(ComponentDescription));
	myErr = GetComponentInfo(curComponent, foundComp, NULL, NULL, NULL);
	if (foundComp->componentSubType ==
		kOSAGenericScriptingComponentSubtype) {
	    /* Skip the generic component */
	    ckfree((char *) foundComp);
	} else {
	    GotOneOSALanguage = true;

	    /*
	     * This is gross: looks like I have to open the component just  
	     * to get its name!!! GetComponentInfo is supposed to return
	     * the name, but AppleScript always returns an empty string.
	     */
		 	
	    curOpenComponent = OpenComponent(curComponent);
	    if (curOpenComponent == NULL) {
		Tcl_AppendResult(interp,"Error opening component",
			(char *) NULL);
		return TCL_ERROR;
	    }
			 
	    myErr = OSAScriptingComponentName(curOpenComponent,&componentName);
	    if (myErr == noErr) {
		myErr = GetCStringFromDescriptor(&componentName,
			nameStr, 31, &nameLen);
		AEDisposeDesc(&componentName);
	    }
	    CloseComponent(curOpenComponent);

	    if (myErr == noErr) {
		hashEntry = Tcl_CreateHashEntry(LanguagesTable,
			nameStr, &newPtr);
		Tcl_SetHashValue(hashEntry, (ClientData) foundComp);
	    } else {
		Tcl_AppendResult(interp,"Error getting componentName.",
			(char *) NULL);
		return TCL_ERROR;
	    }
			
	    /*
	     * Make sure AppleScript is loaded, otherwise we will
	     * not bother to make the AppleScript command.
	     */
	    if (foundComp->componentSubType == kAppleScriptSubtype) {
		appleScriptFlags = foundComp->componentFlags;
		gotAppleScript = true;
	    }			
	}
    }				

    /*
     * Create the OSA command.
     */
	
    if (!GotOneOSALanguage) {
	Tcl_AppendResult(interp,"Could not find any OSA languages",
		(char *) NULL);
	return TCL_ERROR;
    }
	
    /*
     * Create the Component Assoc Data & put it in the interpreter.
     */
	
    ComponentTable = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
	
    if (ComponentTable == NULL) {
	panic("Memory Error Allocating Hash Table");
    }
	
    Tcl_SetAssocData(interp, "OSAScript_CompTable", NULL, ComponentTable);
			
    Tcl_InitHashTable(ComponentTable, TCL_STRING_KEYS);

    /*
     * The OSA command is not currently supported.	 
    Tcl_CreateCommand(interp, "OSA", Tcl_OSACmd, (ClientData) NULL,
	    (Tcl_CmdDeleteProc *) NULL);
     */
     
    /* 
     * Open up one AppleScript component, with a default context
     * and tie it to the AppleScript command.
     * If the user just wants single-threaded AppleScript execution
     * this should be enough.
     *
     */
	 
    if (gotAppleScript) {
	if (tclOSAMakeNewComponent(interp, "AppleScript",
		"AppleScript English", kAppleScriptSubtype,
		appleScriptFlags) == NULL ) {
	    return TCL_ERROR;
	}
    }

    return Tcl_PkgProvide(interp, "OSAConnect", "1.0");
}

/*
 *---------------------------------------------------------------------- 
 *
 * Tcl_OSACmd --
 *
 *	This is the command that provides the interface to the OSA
 *	component manager.  The subcommands are: close: close a component, 
 *	info: get info on components open, and open: get a new connection
 *	with the Scripting Component
 *
 * Results:
 *  	A standard Tcl result.
 *
 * Side effects:
 *  	Depends on the subcommand, see the user documentation
 *	for more details.
 *
 *----------------------------------------------------------------------
 */
 
int 
Tcl_OSACmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char **argv)
{
    static unsigned short componentCmdIndex = 0;
    char autoName[32];
    char c;
    int length;
    Tcl_HashTable *ComponentTable = NULL;
	

    if (argc == 1) {
	Tcl_AppendResult(interp, "Wrong # of arguments, should be \"",
		argv[0], " option\"", (char *) NULL);
	return TCL_ERROR;
    }
	
    c = *argv[1];
    length = strlen(argv[1]);
	
    /*
     * Query out the Component Table, since most of these commands use it...
     */
	
    ComponentTable = (Tcl_HashTable *) Tcl_GetAssocData(interp,
	    "OSAScript_CompTable", (Tcl_InterpDeleteProc **) NULL);
	
    if (ComponentTable == NULL) {
	Tcl_AppendResult(interp, "Error, could not get the Component Table",
		" from the Associated data.", (char *) NULL);
	return TCL_ERROR;
    }
	
    if (c == 'c' && strncmp(argv[1],"close",length) == 0) {
	Tcl_HashEntry *hashEntry;
	if (argc != 3) {
	    Tcl_AppendResult(interp, "Wrong # of arguments, should be \"",
		    argv[0], " ",argv[1], " componentName\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
		
	if ((hashEntry = Tcl_FindHashEntry(ComponentTable,argv[2])) == NULL) {
	    Tcl_AppendResult(interp, "Component \"", argv[2], "\" not found",
		    (char *) NULL);
	    return TCL_ERROR;
	} else {
	    Tcl_DeleteCommand(interp,argv[2]);
	    return TCL_OK;
	}
    } else if (c == 'o' && strncmp(argv[1],"open",length) == 0) {
	/*
	 * Default language is AppleScript.
	 */
	OSType scriptSubtype = kAppleScriptSubtype;
	char *languageName = "AppleScript English";
	char *errMsg = NULL;
	ComponentDescription *theCD;

	argv += 2;
	argc -= 2;
		 
	while (argc > 0 ) {
	    if (*argv[0] == '-') {
		c = *(argv[0] + 1);
		if (c == 'l' && strcmp(argv[0] + 1, "language") == 0) {
		    if (argc == 1) {
			Tcl_AppendResult(interp,
				"Error - no language provided for the -language switch",
				(char *) NULL);
			return TCL_ERROR;
		    } else {
			Tcl_HashEntry *hashEntry;
			Tcl_HashSearch search;
			Boolean gotIt = false;
			Tcl_HashTable *LanguagesTable;
						
			/*
			 * Look up the language in the languages table
			 * Do a simple strstr match, so AppleScript
			 * will match "AppleScript English"...
			 */
						
			LanguagesTable = Tcl_GetAssocData(interp,
				"OSAScript_LangTable",
				(Tcl_InterpDeleteProc **) NULL);
							
			for (hashEntry =
				 Tcl_FirstHashEntry(LanguagesTable, &search);
			     hashEntry != NULL;
			     hashEntry = Tcl_NextHashEntry(&search)) {
			    languageName = Tcl_GetHashKey(LanguagesTable,
				    hashEntry);
			    if (strstr(languageName,argv[1]) != NULL) {
				theCD = (ComponentDescription *)
				    Tcl_GetHashValue(hashEntry);
				gotIt = true;
				break;
			    }
			}
			if (!gotIt) {
			    Tcl_AppendResult(interp,
				    "Error, could not find the language \"",
				    argv[1],
				    "\" in the list of known languages.",
				    (char *) NULL);
			    return TCL_ERROR;
			}
		    }
		}
		argc -= 2;
		argv += 2;				
	    } else {
		Tcl_AppendResult(interp, "Expected a flag, but got ",
			argv[0], (char *) NULL);
		return TCL_ERROR;
	    }
	}
			
	sprintf(autoName, "OSAComponent%-d", componentCmdIndex++);
	if (tclOSAMakeNewComponent(interp, autoName, languageName,
		theCD->componentSubType, theCD->componentFlags) == NULL ) {
	    return TCL_ERROR;
	} else {
	    Tcl_SetResult(interp,autoName,TCL_VOLATILE);
	    return TCL_OK;	
	}
		
    } else if (c == 'i' && strncmp(argv[1],"info",length) == 0) {
	if (argc == 2) {
	    Tcl_AppendResult(interp, "Wrong # of arguments, should be \"",
		    argv[0], " ", argv[1], " what\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
		 	
	c = *argv[2];
	length = strlen(argv[2]);
		
	if (c == 'c' && strncmp(argv[2], "components", length) == 0) {
	    Tcl_DString theResult;
			
	    Tcl_DStringInit(&theResult);
			
	    if (argc == 3) {
		getSortedHashKeys(ComponentTable,(char *) NULL, &theResult);
	    } else if (argc == 4) {
		getSortedHashKeys(ComponentTable, argv[3], &theResult);
	    } else {
		Tcl_AppendResult(interp, "Error: wrong # of arguments",
			", should be \"", argv[0], " ", argv[1], " ",
			argv[2], " ?pattern?\".", (char *) NULL);
		return TCL_ERROR;
	    }
	    Tcl_DStringResult(interp, &theResult);
	    return TCL_OK;			
	} else if (c == 'l' && strncmp(argv[2],"languages",length) == 0) {
	    Tcl_DString theResult;
	    Tcl_HashTable *LanguagesTable;
			
	    Tcl_DStringInit(&theResult);
	    LanguagesTable = Tcl_GetAssocData(interp,
		    "OSAScript_LangTable", (Tcl_InterpDeleteProc **) NULL);
							
	    if (argc == 3) {
		getSortedHashKeys(LanguagesTable, (char *) NULL, &theResult);
	    } else if (argc == 4) {
		getSortedHashKeys(LanguagesTable, argv[3], &theResult);
	    } else {
		Tcl_AppendResult(interp, "Error: wrong # of arguments",
			", should be \"", argv[0], " ", argv[1], " ",
			argv[2], " ?pattern?\".", (char *) NULL);
		return TCL_ERROR;
	    }
	    Tcl_DStringResult(interp,&theResult);
	    return TCL_OK;			
	} else {
	    Tcl_AppendResult(interp, "Unknown option: ", argv[2],
		    " for OSA info, should be one of",
		    " \"components\" or \"languages\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
    } else {
	Tcl_AppendResult(interp, "Unknown option: ", argv[1],
		", should be one of \"open\", \"close\" or \"info\".",
		(char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/* 
 *----------------------------------------------------------------------
 *
 * Tcl_OSAComponentCmd --
 *
 *	This is the command that provides the interface with an OSA
 *	component.  The sub commands are:
 *	- compile ? -context context? scriptData
 *		compiles the script data, returns the ScriptID
 *	- decompile ? -context context? scriptData
 *		decompiles the script data, source code
 *	- execute ?-context context? scriptData
 *		compiles and runs script data
 *	- info what: get component info
 *	- load ?-flags values? fileName
 *		loads & compiles script data from fileName
 *	- run scriptId ?options?
 *		executes the compiled script 
 *
 * Results:
 *	A standard Tcl result
 *
 * Side Effects:
 *	Depends on the subcommand, see the user documentation
 *	for more details.
 *
 *----------------------------------------------------------------------
 */
 
int 
Tcl_OSAComponentCmd(
    ClientData clientData,
    Tcl_Interp *interp, 
    int argc,
    char **argv)
{
    int length;
    char c;
	
    tclOSAComponent *OSAComponent = (tclOSAComponent *) clientData;
	
    if (argc == 1) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg ...?\"",
		(char *) NULL);
	return TCL_ERROR;
    }
	
    c = *argv[1];
    length = strlen(argv[1]);
    if (c == 'c' && strncmp(argv[1], "compile", length) == 0) {
	return TclOSACompileCmd(interp, OSAComponent, argc, argv);
    } else if (c == 'l' && strncmp(argv[1], "load", length) == 0) {
	return tclOSALoadCmd(interp, OSAComponent, argc, argv);
    } else if (c == 'e' && strncmp(argv[1], "execute", length) == 0) {
	return tclOSAExecuteCmd(interp, OSAComponent, argc, argv);
    } else if (c == 'i' && strncmp(argv[1], "info", length) == 0) {
	return tclOSAInfoCmd(interp, OSAComponent, argc, argv);
    } else if (c == 'd' && strncmp(argv[1], "decompile", length) == 0) {
	return tclOSADecompileCmd(interp, OSAComponent, argc, argv);
    } else if (c == 'd' && strncmp(argv[1], "delete", length) == 0) {
	return tclOSADeleteCmd(interp, OSAComponent, argc, argv);
    } else if (c == 'r' && strncmp(argv[1], "run", length) == 0) {
	return tclOSARunCmd(interp, OSAComponent, argc, argv);
    } else if (c == 's' && strncmp(argv[1], "store", length) == 0) {
	return tclOSAStoreCmd(interp, OSAComponent, argc, argv);
    } else {
	Tcl_AppendResult(interp,"bad option \"", argv[1],
		"\": should be compile, decompile, delete, ",
		 "execute, info, load, run or store",
		 (char *) NULL);
	return TCL_ERROR;
    }

    return TCL_OK;
}
 
/*
 *----------------------------------------------------------------------
 *
 * TclOSACompileCmd --
 *
 *	This is the compile subcommand for the component command.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side Effects:
 *  	Compiles the script data either into a script or a script
 *	context.  Adds the script to the component's script or context
 *	table.  Sets interp's result to the name of the new script or
 *	context.
 *
 *----------------------------------------------------------------------
 */
 
static int 
TclOSACompileCmd(
    Tcl_Interp *interp,
    tclOSAComponent *OSAComponent,
    int argc,
    char **argv)
{
    int  tclError = TCL_OK;
    int augment = 1;
    int makeContext = 0;
    char c;
    char autoName[16];
    char buffer[32];
    char *resultName;
    Boolean makeNewContext = false;
    Tcl_DString scrptData;
    AEDesc scrptDesc = { typeNull, NULL };
    long modeFlags = kOSAModeCanInteract;
    OSAID resultID = kOSANullScript;
    OSAID contextID = kOSANullScript;
    OSAID parentID = kOSANullScript;
    OSAError osaErr = noErr;
	
    if (!(OSAComponent->componentFlags && kOSASupportsCompiling)) {
	Tcl_AppendResult(interp,
		"OSA component does not support compiling",
		(char *) NULL);
	return TCL_ERROR;
    }

    /* 
     * This signals that we should make up a name, which is the
     * default behavior:
     */
	 
    autoName[0] = '\0';
    resultName = NULL;
	
    if (argc == 2) {
	numArgs:
	Tcl_AppendResult(interp,
		"wrong # args: should be \"", argv[0], " ", argv[1],
		" ?options? code\"",(char *) NULL);
	return TCL_ERROR;
    } 

    argv += 2;
    argc -= 2;

    /*
     * Do the argument parsing.
     */
	
    while (argc > 0) {
		
	if (*argv[0] == '-') {
	    c = *(argv[0] + 1);
			
	    /*
	     * "--" is the only switch that has no value, stops processing
	     */
			
	    if (c == '-' && *(argv[0] + 2) == '\0') {
		argv += 1;
		argc--;
		break;
	    }
			
	    /*
	     * So we can check here a switch with no value.
	     */
			
	    if (argc == 1)  {
		Tcl_AppendResult(interp,
			"no value given for switch: ",
			argv[0], (char *) NULL);
		return TCL_ERROR;
	    }
			
	    if (c == 'c' && strcmp(argv[0] + 1, "context") == 0) {
		if (Tcl_GetBoolean(interp, argv[1], &makeContext) != TCL_OK) {
		    return TCL_ERROR;
		}
	    } else if (c == 'a' && strcmp(argv[0] + 1, "augment") == 0) {
		/*
		 * Augment the current context which implies making a context.
		 */

		if (Tcl_GetBoolean(interp, argv[1], &augment) != TCL_OK) {
		    return TCL_ERROR;
		}
		makeContext = 1;
	    } else if (c == 'n' && strcmp(argv[0] + 1, "name") == 0) {
		resultName = argv[1];
	    } else if (c == 'p' && strcmp(argv[0] + 1,"parent") == 0) {
		/*
		 * Since this implies we are compiling into a context, 
		 * set makeContext here
		 */
		if (tclOSAGetContextID(OSAComponent,
			argv[1], &parentID) != TCL_OK) {
		    Tcl_AppendResult(interp, "context not found \"",
			    argv[1], "\"", (char *) NULL);
		    return TCL_ERROR;
		}
		makeContext = 1;
	    } else {
		Tcl_AppendResult(interp, "bad option \"", argv[0],
			"\": should be -augment, -context, -name or -parent",
			 (char *) NULL);
		return TCL_ERROR;
	    }
	    argv += 2;
	    argc -= 2;
			
	} else {
	    break;
	}
    }

    /*
     * Make sure we have some data left...
     */
    if (argc == 0) {
	goto numArgs;
    }
	
    /* 
     * Now if we are making a context, see if it is a new one... 
     * There are three options here:
     * 1) There was no name provided, so we autoName it
     * 2) There was a name, then check and see if it already exists
     *  a) If yes, then makeNewContext is false
     *  b) Otherwise we are making a new context
     */

    if (makeContext) {
	modeFlags |= kOSAModeCompileIntoContext;
	if (resultName == NULL) {
	    /*
	     * Auto name the new context.
	     */
	    resultName = autoName;
	    resultID = kOSANullScript;
	    makeNewContext = true;
	} else if (tclOSAGetContextID(OSAComponent,
		resultName, &resultID) == TCL_OK) {
	    makeNewContext = false;
	} else { 
	    makeNewContext = true;
	    resultID = kOSANullScript;
	}
		
	/*
	 * Deal with the augment now...
	 */
	if (augment && !makeNewContext) {
	    modeFlags |= kOSAModeAugmentContext;
	}
    }
	
    /*
     * Ok, now we have the options, so we can compile the script data.
     */
			
    if (prepareScriptData(argc, argv, &scrptData, &scrptDesc) == TCL_ERROR) {
	Tcl_DStringResult(interp, &scrptData);
	AEDisposeDesc(&scrptDesc);
	return TCL_ERROR;
    }

    /* 
     * If we want to use a parent context, we have to make the context 
     * by hand. Note, parentID is only specified when you make a new context. 
     */
	
    if (parentID != kOSANullScript && makeNewContext) {
	AEDesc contextDesc = { typeNull, NULL };

	osaErr = OSAMakeContext(OSAComponent->theComponent,
		&contextDesc, parentID, &resultID);
	modeFlags |= kOSAModeAugmentContext;
    }
	
    osaErr = OSACompile(OSAComponent->theComponent, &scrptDesc,
	    modeFlags, &resultID);								
    if (osaErr == noErr) {
	 
	if (makeContext) {
	    /* 
	     * For the compiled context to be active, you need to run 
	     * the code that is in the context.
	     */
	    OSAID activateID;

	    osaErr = OSAExecute(OSAComponent->theComponent, resultID,
		    resultID, kOSAModeCanInteract, &activateID);
	    OSADispose(OSAComponent->theComponent, activateID);

	    if (osaErr == noErr) {
		if (makeNewContext) {
		    /*
		     * If we have compiled into a context, 
		     * this is added to the context table 
		     */
					 
		    tclOSAAddContext(OSAComponent, resultName, resultID);
		}
				
		Tcl_SetResult(interp, resultName, TCL_VOLATILE);
		tclError = TCL_OK;
	    }
	} else {
	    /*
	     * For a script, we return the script name.
	     */
	    tclOSAAddScript(OSAComponent, resultName, modeFlags, resultID);
	    Tcl_SetResult(interp, resultName, TCL_VOLATILE);
	    tclError = TCL_OK;	
	}
    }
	
    /* 
     * This catches the error either from the original compile, 
     * or from the execute in case makeContext == true
     */
	 						
    if (osaErr == errOSAScriptError) {
	OSADispose(OSAComponent->theComponent, resultID);
	tclOSAASError(interp, OSAComponent->theComponent,
		Tcl_DStringValue(&scrptData));
	tclError = TCL_ERROR;
    } else if (osaErr != noErr)  {
	sprintf(buffer, "Error #%-6d compiling script", osaErr);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
	tclError = TCL_ERROR;		
    } 

    Tcl_DStringFree(&scrptData);
    AEDisposeDesc(&scrptDesc);
	
    return tclError;
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSADecompileCmd --
 *
 * 	This implements the Decompile subcommand of the component command
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *  	Decompiles the script, and sets interp's result to the
 *	decompiled script data.
 *
 *----------------------------------------------------------------------
 */
 		
static int 
tclOSADecompileCmd(
    Tcl_Interp * interp,
    tclOSAComponent *OSAComponent,
    int argc, 
    char **argv)
{
    AEDesc resultingSourceData = { typeChar, NULL };
    OSAID scriptID;
    Boolean isContext;
    long result;
    OSErr sysErr = noErr;
 		
    if (argc == 2) {
	Tcl_AppendResult(interp, "Wrong # of arguments, should be \"",
		argv[0], " ",argv[1], " scriptName \"", (char *) NULL );
	return TCL_ERROR;
    }
 	
    if (!(OSAComponent->componentFlags && kOSASupportsGetSource)) {
	Tcl_AppendResult(interp,
		"Error, this component does not support get source",
		(char *) NULL);
	return TCL_ERROR;
    }
 	
    if (tclOSAGetScriptID(OSAComponent, argv[2], &scriptID) == TCL_OK) {
	isContext = false;
    } else if (tclOSAGetContextID(OSAComponent, argv[2], &scriptID)
	    == TCL_OK ) {
	isContext = true;
    } else { 
	Tcl_AppendResult(interp, "Could not find script \"",
		argv[2], "\"", (char *) NULL);
	return TCL_ERROR;
    }
	
    OSAGetScriptInfo(OSAComponent->theComponent, scriptID,
	    kOSACanGetSource, &result);
						
    sysErr = OSAGetSource(OSAComponent->theComponent, 
	    scriptID, typeChar, &resultingSourceData);
	
    if (sysErr == noErr) {
	Tcl_DString theResult;
	Tcl_DStringInit(&theResult);

	Tcl_DStringAppend(&theResult, *resultingSourceData.dataHandle,
		GetHandleSize(resultingSourceData.dataHandle));
	Tcl_DStringResult(interp, &theResult);
	AEDisposeDesc(&resultingSourceData);
	return TCL_OK;
    } else {
	Tcl_AppendResult(interp, "Error getting source data", (char *) NULL);
	AEDisposeDesc(&resultingSourceData);
	return TCL_ERROR;
    }
}			
	 	
/*
 *----------------------------------------------------------------------
 *
 * tclOSADeleteCmd --
 *
 *	This implements the Delete subcommand of the Component command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *  	Deletes a script from the script list of the given component.
 *	Removes all references to the script, and frees the memory
 *	associated with it.
 *
 *----------------------------------------------------------------------
 */
 
static int 
tclOSADeleteCmd(
    Tcl_Interp *interp,
    tclOSAComponent *OSAComponent,
    int argc,
    char **argv)
{
    char c,*errMsg = NULL;
    int length;
 	
    if (argc < 4) {
	Tcl_AppendResult(interp, "Wrong # of arguments, should be \"",
		argv[0], " ", argv[1], " what scriptName", (char *) NULL);
	return TCL_ERROR;
    }
 	
    c = *argv[2];
    length = strlen(argv[2]);
    if (c == 'c' && strncmp(argv[2], "context", length) == 0) {
	if (strcmp(argv[3], "global") == 0) {
	    Tcl_AppendResult(interp, "You cannot delete the global context",
		    (char *) NULL);
	    return TCL_ERROR;
	} else if (tclOSADeleteContext(OSAComponent, argv[3]) != TCL_OK) {
	    Tcl_AppendResult(interp, "Error deleting script \"", argv[2],
		    "\": ", errMsg, (char *) NULL);
	    ckfree(errMsg);
	    return TCL_ERROR;
	}
    } else if (c == 's' && strncmp(argv[2], "script", length) == 0) {
	if (tclOSADeleteScript(OSAComponent, argv[3], errMsg) != TCL_OK) {
	    Tcl_AppendResult(interp, "Error deleting script \"", argv[3],
		    "\": ", errMsg, (char *) NULL);
	    ckfree(errMsg);
	    return TCL_ERROR;
	}
    } else {
	Tcl_AppendResult(interp,"Unknown value ", argv[2],
		" should be one of ",
		"\"context\" or \"script\".",
		(char *) NULL );
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------- 
 *
 * tclOSAExecuteCmd --
 *
 *	This implements the execute subcommand of the component command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Executes the given script data, and sets interp's result to
 *	the OSA component's return value.
 *
 *---------------------------------------------------------------------- 
 */
 
static int 
tclOSAExecuteCmd(
    Tcl_Interp *interp,
    tclOSAComponent *OSAComponent,
    int argc,
    char **argv)
{
    int tclError = TCL_OK, resID = 128;
    char c,buffer[32],
	*contextName = NULL,*scriptName = NULL, *resName = NULL;
    Boolean makeNewContext = false,makeContext = false;
    AEDesc scrptDesc = { typeNull, NULL };
    long modeFlags = kOSAModeCanInteract;
    OSAID resultID = kOSANullScript,
	contextID = kOSANullScript,
	parentID = kOSANullScript;
    Tcl_DString scrptData;
    OSAError osaErr = noErr;
    OSErr  sysErr = noErr;

    if (argc == 2) {
	Tcl_AppendResult(interp,
		"Error, no script data for \"", argv[0],
		" run\"", (char *) NULL);
	return TCL_ERROR;
    } 

    argv += 2;
    argc -= 2;

    /*
     * Set the context to the global context by default.
     * Then parse the argument list for switches
     */
    tclOSAGetContextID(OSAComponent, "global", &contextID);
	
    while (argc > 0) {
		
	if (*argv[0] == '-') {
	    c = *(argv[0] + 1);

	    /*
	     * "--" is the only switch that has no value.
	     */
			
	    if (c == '-' && *(argv[0] + 2) == '\0') {
		argv += 1;
		argc--;
		break;
	    }
			
	    /*
	     * So we can check here for a switch with no value.
	     */
			
	    if (argc == 1)  {
		Tcl_AppendResult(interp,
			"Error, no value given for switch ",
			argv[0], (char *) NULL);
		return TCL_ERROR;
	    }
			
	    if (c == 'c' && strcmp(argv[0] + 1, "context") == 0) {
		if (tclOSAGetContextID(OSAComponent,
			argv[1], &contextID) == TCL_OK) {
		} else {
		    Tcl_AppendResult(interp, "Script context \"",
			    argv[1], "\" not found", (char *) NULL);
		    return TCL_ERROR;
		}
	    } else { 
		Tcl_AppendResult(interp, "Error, invalid switch ", argv[0],
			" should be \"-context\"", (char *) NULL);
		return TCL_ERROR;
	    }
			
	    argv += 2;
	    argc -= 2;
	} else {
	    break;
	}
    }
	
    if (argc == 0) {
	Tcl_AppendResult(interp, "Error, no script data", (char *) NULL);
	return TCL_ERROR;
    }
		
    if (prepareScriptData(argc, argv, &scrptData, &scrptDesc) == TCL_ERROR) {
	Tcl_DStringResult(interp, &scrptData);
	AEDisposeDesc(&scrptDesc);
	return TCL_ERROR;
    }
    /*
     * Now try to compile and run, but check to make sure the
     * component supports the one shot deal
     */
    if (OSAComponent->componentFlags && kOSASupportsConvenience) {
	osaErr = OSACompileExecute(OSAComponent->theComponent,
		&scrptDesc, contextID, modeFlags, &resultID);
    } else {
	/*
	 * If not, we have to do this ourselves
	 */
	if (OSAComponent->componentFlags && kOSASupportsCompiling) {
	    OSAID compiledID = kOSANullScript;
	    osaErr = OSACompile(OSAComponent->theComponent, &scrptDesc,
		    modeFlags, &compiledID);
	    if (osaErr == noErr) {
		osaErr = OSAExecute(OSAComponent->theComponent, compiledID,
			contextID, modeFlags, &resultID);
	    }
	    OSADispose(OSAComponent->theComponent, compiledID);
	} else {
	    /*
	     * The scripting component had better be able to load text data...
	     */
	    OSAID loadedID = kOSANullScript;
			
	    scrptDesc.descriptorType = OSAComponent->languageID;
	    osaErr = OSALoad(OSAComponent->theComponent, &scrptDesc,
		    modeFlags, &loadedID);
	    if (osaErr == noErr) {
		OSAExecute(OSAComponent->theComponent, loadedID,
			contextID, modeFlags, &resultID);
	    }
	    OSADispose(OSAComponent->theComponent, loadedID);
	}
    }
    if (osaErr == errOSAScriptError) {
	tclOSAASError(interp, OSAComponent->theComponent,
		Tcl_DStringValue(&scrptData));
	tclError = TCL_ERROR;
    } else if (osaErr != noErr) {
	sprintf(buffer, "Error #%-6d compiling script", osaErr);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
	tclError = TCL_ERROR;		
    } else  {
	tclOSAResultFromID(interp, OSAComponent->theComponent, resultID);
	osaErr = OSADispose(OSAComponent->theComponent, resultID);
	tclError = TCL_OK;
    } 

    Tcl_DStringFree(&scrptData);
    AEDisposeDesc(&scrptDesc);	

    return tclError;	
} 

/*
 *----------------------------------------------------------------------
 *
 * tclOSAInfoCmd --
 *
 * This implements the Info subcommand of the component command
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Info on scripts and contexts.  See the user documentation for details.
 *
 *----------------------------------------------------------------------
 */
static int 
tclOSAInfoCmd(
    Tcl_Interp *interp,
    tclOSAComponent *OSAComponent,
    int argc, 
    char **argv)
{
    char c;
    int length;
    Tcl_DString theResult;
	
    if (argc == 2) {
	Tcl_AppendResult(interp, "Wrong # of arguments, should be \"",
		argv[0], " ", argv[1], " what \"", (char *) NULL );
	return TCL_ERROR;
    }
 	
    c = *argv[2];
    length = strlen(argv[2]);
    if (c == 's' && strncmp(argv[2], "scripts", length) == 0) {
	Tcl_DStringInit(&theResult);
	if (argc == 3) {
	    getSortedHashKeys(&OSAComponent->scriptTable, (char *) NULL,
		    &theResult);
	} else if (argc == 4) {
	    getSortedHashKeys(&OSAComponent->scriptTable, argv[3], &theResult);
	} else {
	    Tcl_AppendResult(interp, "Error: wrong # of arguments,",
		    " should be \"", argv[0], " ", argv[1], " ",
		    argv[2], " ?pattern?", (char *) NULL);
	    return TCL_ERROR;
	}
	Tcl_DStringResult(interp, &theResult);
	return TCL_OK;			
    } else if (c == 'c' && strncmp(argv[2], "contexts", length) == 0) {
	Tcl_DStringInit(&theResult);		
	if (argc == 3) {
	    getSortedHashKeys(&OSAComponent->contextTable, (char *) NULL,
		   &theResult);
	} else if (argc == 4) {
	    getSortedHashKeys(&OSAComponent->contextTable,
		    argv[3], &theResult);
	} else {
	    Tcl_AppendResult(interp, "Error: wrong # of arguments for ,",
		    " should be \"", argv[0], " ", argv[1], " ",
		    argv[2], " ?pattern?", (char *) NULL);
	    return TCL_ERROR;
	}
	Tcl_DStringResult(interp, &theResult);
	return TCL_OK;			
    } else if (c == 'l' && strncmp(argv[2], "language", length) == 0) {
	Tcl_SetResult(interp, OSAComponent->languageName, TCL_STATIC);
	return TCL_OK;
    } else {
	Tcl_AppendResult(interp, "Unknown argument \"", argv[2],
		"\" for \"", argv[0], " info \", should be one of ",
		"\"scripts\" \"language\", or \"contexts\"",
		(char *) NULL);
	return TCL_ERROR;
    } 
}
		
/*
 *----------------------------------------------------------------------
 *
 * tclOSALoadCmd --
 *
 *	This is the load subcommand for the Component Command
 *
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Loads script data from the given file, creates a new context
 *	for it, and sets interp's result to the name of the new context.
 *
 *----------------------------------------------------------------------
 */
 
static int 
tclOSALoadCmd(
    Tcl_Interp *interp,
    tclOSAComponent *OSAComponent,
    int argc,
    char **argv)
{
    int tclError = TCL_OK, resID = 128;
    char c, autoName[24],
	*contextName = NULL, *scriptName = NULL, *resName = NULL;
    Boolean makeNewContext = false, makeContext = false;
    AEDesc scrptDesc = { typeNull, NULL };
    long modeFlags = kOSAModeCanInteract;
    OSAID resultID = kOSANullScript,
	contextID = kOSANullScript,
	parentID = kOSANullScript;
    OSAError osaErr = noErr;
    OSErr  sysErr = noErr;
    long scptInfo;
	
    autoName[0] = '\0';
    scriptName = autoName;
    contextName = autoName;
	
    if (argc == 2) {
	Tcl_AppendResult(interp,
		"Error, no data for \"", argv[0], " ", argv[1],
		"\"", (char *) NULL);
	return TCL_ERROR;
    } 

    argv += 2;
    argc -= 2;

    /*
     * Do the argument parsing.
     */
	
    while (argc > 0) {
		
	if (*argv[0] == '-') {
	    c = *(argv[0] + 1);
			
	    /*
	     * "--" is the only switch that has no value.
	     */
			
	    if (c == '-' && *(argv[0] + 2) == '\0') {
		argv += 1;
		argc--;
		break;
	    }
			
	    /*
	     * So we can check here a switch with no value.
	     */
			
	    if (argc == 1)  {
		Tcl_AppendResult(interp, "Error, no value given for switch ",
			argv[0], (char *) NULL);
		return TCL_ERROR;
	    }
			
	    if (c == 'r' && strcmp(argv[0] + 1, "rsrcname") == 0) {
		resName = argv[1];
	    } else if (c == 'r' && strcmp(argv[0] + 1, "rsrcid") == 0) {
		if (Tcl_GetInt(interp, argv[1], &resID) != TCL_OK) {
		    Tcl_AppendResult(interp,
			    "Error getting resource ID", (char *) NULL);
		    return TCL_ERROR;
		}
	    } else {
		Tcl_AppendResult(interp, "Error, invalid switch ", argv[0],
			" should be \"--\", \"-rsrcname\" or \"-rsrcid\"",
			(char *) NULL);
		return TCL_ERROR;
	    }
			
	    argv += 2;
	    argc -= 2;
	} else {
	    break;
	}
    }
    /*
     * Ok, now we have the options, so we can load the resource,
     */
    if (argc == 0) {
	Tcl_AppendResult(interp, "Error, no filename given", (char *) NULL);
	return TCL_ERROR;
    }
	
    if (tclOSALoad(interp, OSAComponent, resName, resID,
	    argv[0], &resultID) != TCL_OK) {
	Tcl_AppendResult(interp, "Error in load command", (char *) NULL);
	return TCL_ERROR;
    }
	 
    /*
     *  Now find out whether we have a script, or a script context.
     */
	 
    OSAGetScriptInfo(OSAComponent->theComponent, resultID,
	    kOSAScriptIsTypeScriptContext, &scptInfo);
    
    if (scptInfo) {
	autoName[0] = '\0';
	tclOSAAddContext(OSAComponent, autoName, resultID);
		
	Tcl_SetResult(interp, autoName, TCL_VOLATILE);
    } else {
	/*
	 * For a script, we return the script name
	 */
	autoName[0] = '\0';
	tclOSAAddScript(OSAComponent, autoName, kOSAModeCanInteract, resultID);
	Tcl_SetResult(interp, autoName, TCL_VOLATILE);
    }		 	
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSARunCmd --
 *
 *	This implements the run subcommand of the component command
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Runs the given compiled script, and returns the OSA
 *	component's result.
 *
 *----------------------------------------------------------------------
 */
 
static int 
tclOSARunCmd(
    Tcl_Interp *interp,
    tclOSAComponent *OSAComponent,
    int argc,
    char **argv)
{
    int tclError = TCL_OK,
	resID = 128;
    char c, *contextName = NULL,
	*scriptName = NULL, 
	*resName = NULL;
    AEDesc scrptDesc = { typeNull, NULL };
    long modeFlags = kOSAModeCanInteract;
    OSAID resultID = kOSANullScript,
	contextID = kOSANullScript,
	parentID = kOSANullScript;
    OSAError osaErr = noErr;
    OSErr sysErr = noErr;
    char *componentName = argv[0];
    OSAID scriptID;
	
    if (argc == 2) {
	Tcl_AppendResult(interp, "Wrong # of arguments, should be \"",
		argv[0], " ", argv[1], " scriptName", (char *) NULL);
	return TCL_ERROR;
    }
	
    /*
     * Set the context to the global context for this component,
     * as a default
     */
    if (tclOSAGetContextID(OSAComponent, "global", &contextID) != TCL_OK) {
	Tcl_AppendResult(interp,
		"Could not find the global context for component ",
		OSAComponent->theName, (char *) NULL );
	return TCL_ERROR;
    }

    /*
     * Now parse the argument list for switches
     */
    argv += 2;
    argc -= 2;
	
    while (argc > 0) {
	if (*argv[0] == '-') {
	    c = *(argv[0] + 1);
	    /*
	     * "--" is the only switch that has no value
	     */
	    if (c == '-' && *(argv[0] + 2) == '\0') {
		argv += 1;
		argc--;
		break;
	    }
			
	    /*
	     * So we can check here for a switch with no value.
	     */
	    if (argc == 1)  {
		Tcl_AppendResult(interp, "Error, no value given for switch ",
			argv[0], (char *) NULL);
		return TCL_ERROR;
	    }
			
	    if (c == 'c' && strcmp(argv[0] + 1, "context") == 0) {
		if (argc == 1) {
		    Tcl_AppendResult(interp,
			    "Error - no context provided for the -context switch",
			    (char *) NULL);
		    return TCL_ERROR;
		} else if (tclOSAGetContextID(OSAComponent,
			argv[1], &contextID) == TCL_OK) {
		} else {
		    Tcl_AppendResult(interp, "Script context \"", argv[1],
			    "\" not found", (char *) NULL);
		    return TCL_ERROR;
		} 
	    } else {
		Tcl_AppendResult(interp, "Error, invalid switch ", argv[0],
			" for ", componentName,
			" should be \"-context\"", (char *) NULL);
		return TCL_ERROR;
	    }
	    argv += 2;
	    argc -= 2;
	} else {
	    break;
	}
    }
	
    if (tclOSAGetScriptID(OSAComponent, argv[0], &scriptID) != TCL_OK) {
	if (tclOSAGetContextID(OSAComponent, argv[0], &scriptID) != TCL_OK) {
	    Tcl_AppendResult(interp, "Could not find script \"",
		    argv[2], "\"", (char *) NULL);
	    return TCL_ERROR;
	}
    }
	
    sysErr = OSAExecute(OSAComponent->theComponent,
	    scriptID, contextID, modeFlags, &resultID);
							
    if (sysErr == errOSAScriptError) {
	tclOSAASError(interp, OSAComponent->theComponent, (char *) NULL);
	tclError = TCL_ERROR;
    } else if (sysErr != noErr) {
	char buffer[32];
	sprintf(buffer, "Error #%6.6d encountered in run", sysErr);
	Tcl_SetResult(interp, buffer, TCL_VOLATILE);
	tclError = TCL_ERROR;
    } else {
	tclOSAResultFromID(interp, OSAComponent->theComponent, resultID );
    }
    OSADispose(OSAComponent->theComponent, resultID);

    return tclError;		
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAStoreCmd --
 *
 *	This implements the store subcommand of the component command
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Runs the given compiled script, and returns the OSA
 *	component's result.
 *
 *----------------------------------------------------------------------
 */
 
static int 
tclOSAStoreCmd(
    Tcl_Interp *interp,
    tclOSAComponent *OSAComponent,
    int argc,
    char **argv)
{
    int tclError = TCL_OK, resID = 128;
    char c, *contextName = NULL, *scriptName = NULL, *resName = NULL;
    Boolean makeNewContext = false, makeContext = false;
    AEDesc scrptDesc = { typeNull, NULL };
    long modeFlags = kOSAModeCanInteract;
    OSAID resultID = kOSANullScript,
	contextID = kOSANullScript,
	parentID = kOSANullScript;
    OSAError osaErr = noErr;
    OSErr  sysErr = noErr;
		
    if (argc == 2) {
	Tcl_AppendResult(interp, "Error, no data for \"", argv[0],
		" ",argv[1], "\"", (char *) NULL);
	return TCL_ERROR;
    } 

    argv += 2;
    argc -= 2;

    /*
     * Do the argument parsing
     */
	
    while (argc > 0) {
	if (*argv[0] == '-') {
	    c = *(argv[0] + 1);
			
	    /*
	     * "--" is the only switch that has no value
	     */
	    if (c == '-' && *(argv[0] + 2) == '\0') {
		argv += 1;
		argc--;
		break;
	    }
			
	    /*
	     * So we can check here a switch with no value.
	     */
	    if (argc == 1)  {
		Tcl_AppendResult(interp,
			"Error, no value given for switch ",
			argv[0], (char *) NULL);
		return TCL_ERROR;
	    }
			
	    if (c == 'r' && strcmp(argv[0] + 1, "rsrcname") == 0) {
		resName = argv[1];
	    } else if (c == 'r' && strcmp(argv[0] + 1, "rsrcid") == 0) {
		if (Tcl_GetInt(interp, argv[1], &resID) != TCL_OK) {
		    Tcl_AppendResult(interp,
			    "Error getting resource ID", (char *) NULL);
		    return TCL_ERROR;
		}
	    } else {
		Tcl_AppendResult(interp, "Error, invalid switch ", argv[0],
			" should be \"--\", \"-rsrcname\" or \"-rsrcid\"",
			(char *) NULL);
		return TCL_ERROR;
	    }
			
	    argv += 2;
	    argc -= 2;
	} else {
	    break;
	}
    }
    /*
     * Ok, now we have the options, so we can load the resource,
     */
    if (argc != 2) {
	Tcl_AppendResult(interp, "Error, wrong # of arguments, should be ",
		argv[0], " ", argv[1], "?option flag? scriptName fileName",
		(char *) NULL);
	return TCL_ERROR;
    }
	
    if (tclOSAStore(interp, OSAComponent, resName, resID,
	    argv[0], argv[1]) != TCL_OK) {
	Tcl_AppendResult(interp, "Error in load command", (char *) NULL);
	return TCL_ERROR;
    } else {
	Tcl_ResetResult(interp);
	tclError = TCL_OK;
    }
    
    return tclError;
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAMakeNewComponent --
 *
 *	Makes a command cmdName to represent a new connection to the
 *	OSA component with componentSubType scriptSubtype.
 *
 * Results: 
 *	Returns the tclOSAComponent structure for the connection.
 *
 * Side Effects: 
 *	Adds a new element to the component table.  If there is an
 *	error, then the result of the Tcl interpreter interp is set
 *	to an appropriate error message.
 *
 *----------------------------------------------------------------------
 */
 
tclOSAComponent *
tclOSAMakeNewComponent(
    Tcl_Interp *interp,
    char *cmdName,
    char *languageName, 
    OSType scriptSubtype,
    long componentFlags) 
{
    char buffer[32];
    AEDesc resultingName = {typeNull, NULL};
    AEDesc nullDesc = {typeNull, NULL };
    OSAID globalContext;
    char global[] = "global";
    int nbytes;
    ComponentDescription requestedComponent = {
	kOSAComponentType,
	(OSType) 0,
	(OSType) 0,
	(long int) 0,
	(long int) 0
    };
    Tcl_HashTable *ComponentTable;
    Component foundComponent = NULL;
    OSAActiveUPP myActiveProcUPP;
			
    tclOSAComponent *newComponent;
    Tcl_HashEntry *hashEntry;
    int newPtr;
	
    requestedComponent.componentSubType = scriptSubtype;
    nbytes = sizeof(tclOSAComponent);
    newComponent = (tclOSAComponent *) ckalloc(sizeof(tclOSAComponent));
    if (newComponent == NULL) {
	goto CleanUp;
    }
	
    foundComponent = FindNextComponent(0, &requestedComponent);
    if (foundComponent == 0) {
	Tcl_AppendResult(interp,
		"Could not find component of requested type", (char *) NULL);
	goto CleanUp;
    } 
	
    newComponent->theComponent = OpenComponent(foundComponent); 
	
    if (newComponent->theComponent == NULL) {
	Tcl_AppendResult(interp,
		"Could not open component of the requested type",
		(char *) NULL);
	goto CleanUp;
    }
							
    newComponent->languageName = (char *) ckalloc(strlen(languageName) + 1);
    strcpy(newComponent->languageName,languageName);
	
    newComponent->componentFlags = componentFlags;
	
    newComponent->theInterp = interp;
	
    Tcl_InitHashTable(&newComponent->contextTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&newComponent->scriptTable, TCL_STRING_KEYS);
		
    if (tclOSAMakeContext(newComponent, global, &globalContext) != TCL_OK) {
	sprintf(buffer, "%-6.6d", globalContext);
	Tcl_AppendResult(interp, "Error ", buffer, " making ", global,
		" context.", (char *) NULL);
	goto CleanUp;
    }
    
    newComponent->languageID = scriptSubtype;
	
    newComponent->theName = (char *) ckalloc(strlen(cmdName) + 1 );
    strcpy(newComponent->theName, cmdName);

    Tcl_CreateCommand(interp, newComponent->theName, Tcl_OSAComponentCmd,
	    (ClientData) newComponent, tclOSAClose);
					
    /*
     * Register the new component with the component table
     */ 

    ComponentTable = (Tcl_HashTable *) Tcl_GetAssocData(interp,
	    "OSAScript_CompTable", (Tcl_InterpDeleteProc **) NULL);
	
    if (ComponentTable == NULL) {
	Tcl_AppendResult(interp, "Error, could not get the Component Table",
		" from the Associated data.", (char *) NULL);
	return (tclOSAComponent *) NULL;
    }
	
    hashEntry = Tcl_CreateHashEntry(ComponentTable,
	    newComponent->theName, &newPtr);	
    Tcl_SetHashValue(hashEntry, (ClientData) newComponent);

    /*
     * Set the active proc to call Tcl_DoOneEvent() while idle
     */
    if (OSAGetActiveProc(newComponent->theComponent,
	    &newComponent->defActiveProc, &newComponent->defRefCon) != noErr ) {
    	/* TODO -- clean up here... */
    }

    myActiveProcUPP = NewOSAActiveProc(TclOSAActiveProc);
    OSASetActiveProc(newComponent->theComponent,
	    myActiveProcUPP, (long) newComponent);
    return newComponent;
	
    CleanUp:
	
    ckfree((char *) newComponent);
    return (tclOSAComponent *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAClose --
 *
 *	This procedure closes the connection to an OSA component, and 
 *	deletes all the script and context data associated with it.
 *	It is the command deletion callback for the component's command.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Closes the connection, and releases all the script data.
 *
 *----------------------------------------------------------------------
 */

void 
tclOSAClose(
    ClientData clientData) 
{
    tclOSAComponent *theComponent = (tclOSAComponent *) clientData;
    Tcl_HashEntry *hashEntry;
    Tcl_HashSearch search;
    tclOSAScript *theScript;
    Tcl_HashTable *ComponentTable;
	
    /* 
     * Delete the context and script tables 
     * the memory for the language name, and
     * the hash entry.
     */
	
    for (hashEntry = Tcl_FirstHashEntry(&theComponent->scriptTable, &search);
	 hashEntry != NULL;
	 hashEntry = Tcl_NextHashEntry(&search)) {

	theScript = (tclOSAScript *) Tcl_GetHashValue(hashEntry);
	OSADispose(theComponent->theComponent, theScript->scriptID);	
	ckfree((char *) theScript);
	Tcl_DeleteHashEntry(hashEntry);
    }
	
    for (hashEntry = Tcl_FirstHashEntry(&theComponent->contextTable, &search);
	 hashEntry != NULL;
	 hashEntry = Tcl_NextHashEntry(&search)) {

	Tcl_DeleteHashEntry(hashEntry);
    }
	
    ckfree(theComponent->languageName);
    ckfree(theComponent->theName);
	
    /*
     * Finally close the component
     */
	
    CloseComponent(theComponent->theComponent);
	
    ComponentTable = (Tcl_HashTable *)
	Tcl_GetAssocData(theComponent->theInterp,
		"OSAScript_CompTable", (Tcl_InterpDeleteProc **) NULL);
	
    if (ComponentTable == NULL) {
	panic("Error, could not get the Component Table from the Associated data.");
    }
	
    hashEntry = Tcl_FindHashEntry(ComponentTable, theComponent->theName);
    if (hashEntry != NULL) {
	Tcl_DeleteHashEntry(hashEntry);
    }
    
    ckfree((char *) theComponent);
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAGetContextID  --
 *
 *	This returns the context ID, given the component name.
 *
 * Results:
 *	A context ID
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

static int 
tclOSAGetContextID(
    tclOSAComponent *theComponent, 
    char *contextName, 
    OSAID *theContext)
{
    Tcl_HashEntry *hashEntry;
    tclOSAContext *contextStruct;
	
    if ((hashEntry = Tcl_FindHashEntry(&theComponent->contextTable,
	    contextName)) == NULL ) {			
	return TCL_ERROR;
    } else {
	contextStruct = (tclOSAContext *) Tcl_GetHashValue(hashEntry);
	*theContext = contextStruct->contextID;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAAddContext  --
 *
 *	This adds the context ID, with the name contextName.  If the
 *	name is passed in as a NULL string, space is malloc'ed for the
 *	string and a new name is made up, if the string is empty, you
 *	must have allocated enough space ( 24 characters is fine) for
 *	the name, which is made up and passed out.
 *
 * Results:
 *	Nothing
 *
 * Side effects:
 *	Adds the script context to the component's context table.
 *
 *----------------------------------------------------------------------
 */

static void 
tclOSAAddContext(
    tclOSAComponent *theComponent, 
    char *contextName,
    const OSAID theContext)
{
    static unsigned short contextIndex = 0;
    tclOSAContext *contextStruct;
    Tcl_HashEntry *hashEntry;
    int newPtr;

    if (contextName == NULL) {
	contextName = ckalloc(16 + TCL_INTEGER_SPACE);
	sprintf(contextName, "OSAContext%d", contextIndex++);
    } else if (*contextName == '\0') {
	sprintf(contextName, "OSAContext%d", contextIndex++);
    }
	
    hashEntry = Tcl_CreateHashEntry(&theComponent->contextTable,
	    contextName, &newPtr);	

    contextStruct = (tclOSAContext *) ckalloc(sizeof(tclOSAContext));
    contextStruct->contextID = theContext;
    Tcl_SetHashValue(hashEntry,(ClientData) contextStruct);
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSADeleteContext  --
 *
 *	This deletes the context struct, with the name contextName.  
 *
 * Results:
 *	A normal Tcl result
 *
 * Side effects:
 *	Removes the script context to the component's context table,
 *	and deletes the data associated with it.
 *
 *----------------------------------------------------------------------
 */

static int 
tclOSADeleteContext(
    tclOSAComponent *theComponent,
    char *contextName) 
{
    Tcl_HashEntry *hashEntry;
    tclOSAContext *contextStruct;
	
    hashEntry = Tcl_FindHashEntry(&theComponent->contextTable, contextName);
    if (hashEntry == NULL) {
	return TCL_ERROR;
    }	
    /*
     * Dispose of the script context data
     */
    contextStruct = (tclOSAContext *) Tcl_GetHashValue(hashEntry);
    OSADispose(theComponent->theComponent,contextStruct->contextID);
    /*
     * Then the hash entry
     */
    ckfree((char *) contextStruct);
    Tcl_DeleteHashEntry(hashEntry);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAMakeContext  --
 *
 *	This makes the context with name contextName, and returns the ID.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	Makes a new context, adds it to the context table, and returns 
 *	the new contextID in the variable theContext.
 *
 *----------------------------------------------------------------------
 */

static int 
tclOSAMakeContext(
    tclOSAComponent *theComponent, 
    char *contextName,
    OSAID *theContext)
{
    AEDesc contextNameDesc = {typeNull, NULL};
    OSAError osaErr = noErr;

    AECreateDesc(typeChar, contextName, strlen(contextName), &contextNameDesc);
    osaErr = OSAMakeContext(theComponent->theComponent, &contextNameDesc,
	    kOSANullScript, theContext);
								
    AEDisposeDesc(&contextNameDesc);
	
    if (osaErr == noErr) {
	tclOSAAddContext(theComponent, contextName, *theContext);
    } else {
	*theContext = (OSAID) osaErr;
	return TCL_ERROR;
    }
	
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAStore --
 *
 *	This stores a script resource from the file named in fileName.
 *
 *	Most of this routine is caged from the Tcl Source, from the
 *	Tcl_MacSourceCmd routine.  This is good, since it ensures this
 *	follows the same convention for looking up files as Tcl.
 *
 * Returns
 *	A standard Tcl result.
 *
 * Side Effects:
 *	The given script data is stored in the file fileName.
 *
 *----------------------------------------------------------------------
 */
 
int
tclOSAStore(
    Tcl_Interp *interp,
    tclOSAComponent *theComponent,
    char *resourceName,
    int resourceNumber, 
    char *scriptName,
    char *fileName)
{
    Handle resHandle;
    Str255 rezName;
    int result = TCL_OK;
    short saveRef, fileRef = -1;
    char idStr[16 + TCL_INTEGER_SPACE];
    FSSpec fileSpec;
    Tcl_DString buffer;
    char *nativeName;
    OSErr myErr = noErr;
    OSAID scriptID;
    Size scriptSize;
    AEDesc scriptData;

    /*
     * First extract the script data
     */
	
    if (tclOSAGetScriptID(theComponent, scriptName, &scriptID) != TCL_OK ) {
	if (tclOSAGetContextID(theComponent, scriptName, &scriptID)
		!= TCL_OK) {
	    Tcl_AppendResult(interp, "Error getting script ",
		    scriptName, (char *) NULL);
	    return TCL_ERROR;
	}
    }
	
    myErr = OSAStore(theComponent->theComponent, scriptID,
	    typeOSAGenericStorage, kOSAModeNull, &scriptData);
    if (myErr != noErr) {
	sprintf(idStr, "%d", myErr);
	Tcl_AppendResult(interp, "Error #", idStr,
		" storing script ", scriptName, (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Now try to open the output file
     */
	
    saveRef = CurResFile();
	
    if (fileName != NULL) {
	OSErr err;
		
	Tcl_DStringInit(&buffer);	
	nativeName = Tcl_TranslateFileName(interp, fileName, &buffer);
	if (nativeName == NULL) {
	    return TCL_ERROR;
	}
	err = FSpLocationFromPath(strlen(nativeName), nativeName, &fileSpec);
		
	Tcl_DStringFree(&buffer);
	if ((err != noErr) && (err != fnfErr)) {
	    Tcl_AppendResult(interp,
		    "Error getting a location for the file: \"", 
		    fileName, "\".", NULL);
	    return TCL_ERROR;
	}
		
	FSpCreateResFileCompat(&fileSpec,
		'WiSH', 'osas', smSystemScript);	
	myErr = ResError();
	
	if ((myErr != noErr) && (myErr != dupFNErr)) {
	    sprintf(idStr, "%d", myErr);
	    Tcl_AppendResult(interp, "Error #", idStr,
		    " creating new resource file ", fileName, (char *) NULL);
	    result = TCL_ERROR;
	    goto rezEvalCleanUp;
	}
		
	fileRef = FSpOpenResFileCompat(&fileSpec, fsRdWrPerm);
	if (fileRef == -1) {
	    Tcl_AppendResult(interp, "Error reading the file: \"", 
		    fileName, "\".", NULL);
	    result = TCL_ERROR;
	    goto rezEvalCleanUp;
	}
	UseResFile(fileRef);
    } else {
	/*
	 * The default behavior will search through all open resource files.
	 * This may not be the behavior you desire.  If you want the behavior
	 * of this call to *only* search the application resource fork, you
	 * must call UseResFile at this point to set it to the application
	 * file.  This means you must have already obtained the application's 
	 * fileRef when the application started up.
	 */
    }
	
    /*
     * Load the resource by name 
     */
    if (resourceName != NULL) {
	strcpy((char *) rezName + 1, resourceName);
	rezName[0] = strlen(resourceName);
	resHandle = Get1NamedResource('scpt', rezName);
	myErr = ResError();
	if (resHandle == NULL) {
	    /*
	     * These signify either the resource or the resource
	     * type were not found
	     */
	    if (myErr == resNotFound || myErr == noErr) {
		short uniqueID;
		while ((uniqueID = Unique1ID('scpt') ) < 128) {}
		AddResource(scriptData.dataHandle, 'scpt', uniqueID, rezName);
		WriteResource(resHandle);
		result = TCL_OK;
		goto rezEvalCleanUp;
	    } else {
		/*
		 * This means there was some other error, for now
		 * I just bag out.
		 */
		sprintf(idStr, "%d", myErr);
		Tcl_AppendResult(interp, "Error #", idStr,
			" opening scpt resource named ", resourceName,
			" in file ", fileName, (char *) NULL);
		result = TCL_ERROR;
		goto rezEvalCleanUp;
	    }
	}
	/*
	 * Or ID
	 */ 
    } else {
	resHandle = Get1Resource('scpt', resourceNumber);
	rezName[0] = 0;
	rezName[1] = '\0';
	myErr = ResError();
	if (resHandle == NULL) {
	    /*
	     * These signify either the resource or the resource
	     * type were not found
	     */
	    if (myErr == resNotFound || myErr == noErr) {
		AddResource(scriptData.dataHandle, 'scpt',
			resourceNumber, rezName);
		WriteResource(resHandle);
		result = TCL_OK;
		goto rezEvalCleanUp;
	    } else {
		/*
		 * This means there was some other error, for now
		 * I just bag out */
		sprintf(idStr, "%d", myErr);
		Tcl_AppendResult(interp, "Error #", idStr,
			" opening scpt resource named ", resourceName,
			" in file ", fileName,(char *) NULL);
		result = TCL_ERROR;
		goto rezEvalCleanUp;
	    }
	} 
    }
	
    /* 
     * We get to here if the resource exists 
     * we just copy into it... 
     */
	 
    scriptSize = GetHandleSize(scriptData.dataHandle);
    SetHandleSize(resHandle, scriptSize);
    HLock(scriptData.dataHandle);
    HLock(resHandle);
    BlockMove(*scriptData.dataHandle, *resHandle,scriptSize);
    HUnlock(scriptData.dataHandle);
    HUnlock(resHandle);
    ChangedResource(resHandle);
    WriteResource(resHandle);
    result = TCL_OK;
    goto rezEvalCleanUp;
			
    rezEvalError:
    sprintf(idStr, "ID=%d", resourceNumber);
    Tcl_AppendResult(interp, "The resource \"",
	    (resourceName != NULL ? resourceName : idStr),
	    "\" could not be loaded from ",
	    (fileName != NULL ? fileName : "application"),
	    ".", NULL);

    rezEvalCleanUp:
    if (fileRef != -1) {
	CloseResFile(fileRef);
    }

    UseResFile(saveRef);
	
    return result;
}

/*----------------------------------------------------------------------
 *
 * tclOSALoad --
 *
 *	This loads a script resource from the file named in fileName.
 *	Most of this routine is caged from the Tcl Source, from the
 *	Tcl_MacSourceCmd routine.  This is good, since it ensures this
 *	follows the same convention for looking up files as Tcl.
 *
 * Returns
 *	A standard Tcl result.
 *
 * Side Effects:
 *	A new script element is created from the data in the file.
 *	The script ID is passed out in the variable resultID.
 *
 *----------------------------------------------------------------------
 */
 
int
tclOSALoad(
    Tcl_Interp *interp,
    tclOSAComponent *theComponent,
    char *resourceName,
    int resourceNumber, 
    char *fileName,
    OSAID *resultID)
{
    Handle sourceData;
    Str255 rezName;
    int result = TCL_OK;
    short saveRef, fileRef = -1;
    char idStr[16 + TCL_INTEGER_SPACE];
    FSSpec fileSpec;
    Tcl_DString buffer;
    char *nativeName;

    saveRef = CurResFile();
	
    if (fileName != NULL) {
	OSErr err;
		
	Tcl_DStringInit(&buffer);	
	nativeName = Tcl_TranslateFileName(interp, fileName, &buffer);
	if (nativeName == NULL) {
	    return TCL_ERROR;
	}
	err = FSpLocationFromPath(strlen(nativeName), nativeName, &fileSpec);
	Tcl_DStringFree(&buffer);
	if (err != noErr) {
	    Tcl_AppendResult(interp, "Error finding the file: \"", 
		    fileName, "\".", NULL);
	    return TCL_ERROR;
	}
			
	fileRef = FSpOpenResFileCompat(&fileSpec, fsRdPerm);
	if (fileRef == -1) {
	    Tcl_AppendResult(interp, "Error reading the file: \"", 
		    fileName, "\".", NULL);
	    return TCL_ERROR;
	}
	UseResFile(fileRef);
    } else {
	/*
	 * The default behavior will search through all open resource files.
	 * This may not be the behavior you desire.  If you want the behavior
	 * of this call to *only* search the application resource fork, you
	 * must call UseResFile at this point to set it to the application
	 * file.  This means you must have already obtained the application's 
	 * fileRef when the application started up.
	 */
    }
	
    /*
     * Load the resource by name or ID
     */
    if (resourceName != NULL) {
	strcpy((char *) rezName + 1, resourceName);
	rezName[0] = strlen(resourceName);
	sourceData = GetNamedResource('scpt', rezName);
    } else {
	sourceData = GetResource('scpt', (short) resourceNumber);
    }
	
    if (sourceData == NULL) {
	result = TCL_ERROR;
    } else {
	AEDesc scriptDesc;
	OSAError osaErr;
		
	scriptDesc.descriptorType = typeOSAGenericStorage;
	scriptDesc.dataHandle = sourceData;
		
	osaErr = OSALoad(theComponent->theComponent, &scriptDesc,
		kOSAModeNull, resultID);
		
	ReleaseResource(sourceData);
		
	if (osaErr != noErr) {
	    result = TCL_ERROR;
	    goto rezEvalError;
	}
			
	goto rezEvalCleanUp;
    }
	
    rezEvalError:
    sprintf(idStr, "ID=%d", resourceNumber);
    Tcl_AppendResult(interp, "The resource \"",
	    (resourceName != NULL ? resourceName : idStr),
	    "\" could not be loaded from ",
	    (fileName != NULL ? fileName : "application"),
	    ".", NULL);

    rezEvalCleanUp:
    if (fileRef != -1) {
	CloseResFile(fileRef);
    }

    UseResFile(saveRef);
	
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAGetScriptID  --
 *
 *	This returns the context ID, gibven the component name.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	Passes out the script ID in the variable scriptID.
 *
 *----------------------------------------------------------------------
 */

static int 
tclOSAGetScriptID(
    tclOSAComponent *theComponent,
    char *scriptName,
    OSAID *scriptID) 
{
    tclOSAScript *theScript;
	
    theScript = tclOSAGetScript(theComponent, scriptName);
    if (theScript == NULL) {
	return TCL_ERROR;
    }
	
    *scriptID = theScript->scriptID;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAAddScript  --
 *
 *	This adds a script to theComponent's script table, with the
 *	given name & ID.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	Adds an element to the component's script table.
 *
 *----------------------------------------------------------------------
 */

static int 
tclOSAAddScript(
    tclOSAComponent *theComponent,
    char *scriptName,
    long modeFlags,
    OSAID scriptID) 
{
    Tcl_HashEntry *hashEntry;
    int newPtr;
    static int scriptIndex = 0;
    tclOSAScript *theScript;
	
    if (*scriptName == '\0') {
	sprintf(scriptName, "OSAScript%d", scriptIndex++);
    }
	
    hashEntry = Tcl_CreateHashEntry(&theComponent->scriptTable,
	    scriptName, &newPtr);
    if (newPtr == 0) {
	theScript = (tclOSAScript *) Tcl_GetHashValue(hashEntry);
	OSADispose(theComponent->theComponent, theScript->scriptID);
    } else {		
	theScript = (tclOSAScript *) ckalloc(sizeof(tclOSAScript));
	if (theScript == NULL) {
	    return TCL_ERROR;
	}
    }
		
    theScript->scriptID = scriptID;
    theScript->languageID = theComponent->languageID;
    theScript->modeFlags = modeFlags;
	
    Tcl_SetHashValue(hashEntry,(ClientData) theScript);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAGetScriptID  --
 *
 *	This returns the script structure, given the component and script name.
 *
 * Results:
 *	A pointer to the script structure.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */
 
static tclOSAScript *
tclOSAGetScript(
    tclOSAComponent *theComponent,
    char *scriptName)
{
    Tcl_HashEntry *hashEntry;
	
    hashEntry = Tcl_FindHashEntry(&theComponent->scriptTable, scriptName);
    if (hashEntry == NULL) {
	return NULL;
    }
	
    return (tclOSAScript *) Tcl_GetHashValue(hashEntry);
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSADeleteScript  --
 *
 *	This deletes the script given by scriptName.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	Deletes the script from the script table, and frees up the
 *	resources associated with it.  If there is an error, then
 *	space for the error message is malloc'ed, and passed out in
 *	the variable errMsg.
 *
 *----------------------------------------------------------------------
 */

static int
tclOSADeleteScript(
    tclOSAComponent *theComponent,
    char *scriptName,
    char *errMsg) 
{
    Tcl_HashEntry *hashEntry;
    tclOSAScript *scriptPtr;

    hashEntry = Tcl_FindHashEntry(&theComponent->scriptTable, scriptName);
    if (hashEntry == NULL) {
	errMsg = ckalloc(17);
	strcpy(errMsg,"Script not found");
	return TCL_ERROR;
    }
	
    scriptPtr = (tclOSAScript *) Tcl_GetHashValue(hashEntry);
    OSADispose(theComponent->theComponent, scriptPtr->scriptID);
    ckfree((char *) scriptPtr);
    Tcl_DeleteHashEntry(hashEntry);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclOSAActiveProc --
 *
 *	This is passed to each component.  It is run periodically
 *	during script compilation and script execution.  It in turn
 *	calls Tcl_DoOneEvent to process the event queue.  We also call
 *	the default Active proc which will let the user cancel the script
 *	by hitting Command-.
 * 
 * Results:
 *	A standard MacOS system error
 *
 * Side effects:
 *	Any Tcl code may run while calling Tcl_DoOneEvent.
 *
 *----------------------------------------------------------------------
 */
 
static pascal OSErr 
TclOSAActiveProc(
    long refCon)
{
    tclOSAComponent *theComponent = (tclOSAComponent *) refCon;
	
    Tcl_DoOneEvent(TCL_DONT_WAIT);
    CallOSAActiveProc(theComponent->defActiveProc, theComponent->defRefCon);
	
    return noErr;
}

/*
 *----------------------------------------------------------------------
 *
 * ASCIICompareProc --
 *
 *	Trivial ascii compare for use with qsort.	
 *
 * Results:
 *	strcmp of the two input strings
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */
static int 
ASCIICompareProc(const void *first,const void *second)
{
    int order;
    
    char *firstString = *((char **) first);
    char *secondString = *((char **) second);

    order = strcmp(firstString, secondString);
	
    return order;
}

#define REALLOC_INCR 30
/*
 *----------------------------------------------------------------------
 *
 * getSortedHashKeys --
 *
 *	returns an alphabetically sorted list of the keys of the hash
 *	theTable which match the string "pattern" in the DString
 *	theResult. pattern == NULL matches all.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	ReInitializes the DString theResult, then copies the names of
 *	the matching keys into the string as list elements.
 *
 *----------------------------------------------------------------------
 */
 
static void 
getSortedHashKeys(
    Tcl_HashTable *theTable,
    char *pattern,
    Tcl_DString *theResult)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    Boolean compare = true;
    char *keyPtr;
    static char **resultArgv = NULL;
    static int totSize = 0;
    int totElem = 0, i;
	
    if (pattern == NULL || *pattern == '\0' || 
	    (*pattern == '*' && *(pattern + 1) == '\0')) {
	compare = false;
    }
	
    for (hPtr = Tcl_FirstHashEntry(theTable,&search), totElem = 0;
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&search)) {
			
	keyPtr = (char *) Tcl_GetHashKey(theTable, hPtr);
	if (!compare || Tcl_StringMatch(keyPtr, pattern)) {
	    totElem++;		
	    if (totElem >= totSize) {
		totSize += REALLOC_INCR;
		resultArgv = (char **) ckrealloc((char *) resultArgv,
			totSize * sizeof(char *));
	    }
	    resultArgv[totElem - 1] = keyPtr;
	} 
    }
		
    Tcl_DStringInit(theResult);
    if (totElem == 1) {
	Tcl_DStringAppendElement(theResult, resultArgv[0]);
    } else if (totElem > 1) {
	qsort((VOID *) resultArgv, (size_t) totElem, sizeof (char *),
		ASCIICompareProc);

	for (i = 0; i < totElem; i++) {
	    Tcl_DStringAppendElement(theResult, resultArgv[i]);
	}
    }	
}

/*
 *----------------------------------------------------------------------
 *
 * prepareScriptData --
 *
 *	Massages the input data in the argv array, concating the 
 *	elements, with a " " between each, and replacing \n with \r,
 *	and \\n with "  ".  Puts the result in the the DString scrptData,
 *	and copies the result to the AEdesc scrptDesc.
 *
 * Results:
 *	Standard Tcl result
 *
 * Side effects:
 *	Creates a new Handle (with AECreateDesc) for the script data.
 *	Stores the script in scrptData, or the error message if there
 *	is an error creating the descriptor.
 *
 *----------------------------------------------------------------------
 */
 
static int
prepareScriptData(
    int argc,
    char **argv,
    Tcl_DString *scrptData,
    AEDesc *scrptDesc) 
{
    char * ptr;
    int i;
    char buffer[7];
    OSErr sysErr = noErr;
		
    Tcl_DStringInit(scrptData);
	
    for (i = 0; i < argc; i++) {
	Tcl_DStringAppend(scrptData, argv[i], -1);
	Tcl_DStringAppend(scrptData, " ", 1);
    }

    /*
     * First replace the \n's with \r's in the script argument
     * Also replace "\\n" with "  ".
     */
	 
    for (ptr = scrptData->string; *ptr != '\0'; ptr++) {
	if (*ptr == '\n') {
	    *ptr = '\r';
	} else if (*ptr == '\\') {
	    if (*(ptr + 1) == '\n') {
		*ptr = ' ';
		*(ptr + 1) = ' ';
	    }
	}
    }
 	
    sysErr = AECreateDesc(typeChar, Tcl_DStringValue(scrptData),
	    Tcl_DStringLength(scrptData), scrptDesc);
						
    if (sysErr != noErr) {
	sprintf(buffer, "%6d", sysErr);
	Tcl_DStringFree(scrptData);
	Tcl_DStringAppend(scrptData, "Error #", 7);
	Tcl_DStringAppend(scrptData, buffer, -1);
	Tcl_DStringAppend(scrptData, " creating Script Data Descriptor.", 33);
	return TCL_ERROR;					
    }
	
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAResultFromID --
 *
 *	Gets a human readable version of the result from the script ID
 *	and returns it in the result of the interpreter interp
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Sets the result of interp to the human readable version of resultID.
 *  
 *
 *----------------------------------------------------------------------
 */
 
void 
tclOSAResultFromID(
    Tcl_Interp *interp,
    ComponentInstance theComponent,
    OSAID resultID )
{
    OSErr myErr = noErr;
    AEDesc resultDesc;
    Tcl_DString resultStr;
	
    Tcl_DStringInit(&resultStr);
	
    myErr = OSADisplay(theComponent, resultID, typeChar,
	    kOSAModeNull, &resultDesc);
    Tcl_DStringAppend(&resultStr, (char *) *resultDesc.dataHandle,
	    GetHandleSize(resultDesc.dataHandle));
    Tcl_DStringResult(interp,&resultStr);
}

/*
 *----------------------------------------------------------------------
 *
 * tclOSAASError --
 *
 *	Gets the error message from the AppleScript component, and adds
 *	it to interp's result. If the script data is known, will point
 *	out the offending bit of code.  This MUST BE A NULL TERMINATED
 *	C-STRING, not a typeChar.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Sets the result of interp to error, plus the relevant portion
 *	of the script.
 *
 *----------------------------------------------------------------------
 */
 
void 
tclOSAASError(
    Tcl_Interp * interp,
    ComponentInstance theComponent,
    char *scriptData )
{
    OSErr myErr = noErr;
    AEDesc errResult,errLimits;
    Tcl_DString errStr;
    DescType returnType;
    Size returnSize;
    short srcStart,srcEnd;
    char buffer[16];
	
    Tcl_DStringInit(&errStr);
    Tcl_DStringAppend(&errStr, "An AppleScript error was encountered.\n", -1); 
	
    OSAScriptError(theComponent, kOSAErrorNumber,
	    typeShortInteger, &errResult);
	
    sprintf(buffer, "Error #%-6.6d\n", (short int) **errResult.dataHandle);

    AEDisposeDesc(&errResult);
	
    Tcl_DStringAppend(&errStr,buffer, 15);
	
    OSAScriptError(theComponent, kOSAErrorMessage, typeChar, &errResult);
    Tcl_DStringAppend(&errStr, (char *) *errResult.dataHandle,
	    GetHandleSize(errResult.dataHandle));
    AEDisposeDesc(&errResult);
	
    if (scriptData != NULL) {
	int lowerB, upperB;
		
	myErr = OSAScriptError(theComponent, kOSAErrorRange,
		typeOSAErrorRange, &errResult);
		
	myErr = AECoerceDesc(&errResult, typeAERecord, &errLimits);
	myErr = AEGetKeyPtr(&errLimits, keyOSASourceStart,
		typeShortInteger, &returnType, &srcStart,
		sizeof(short int), &returnSize);
	myErr = AEGetKeyPtr(&errLimits, keyOSASourceEnd, typeShortInteger,
		&returnType, &srcEnd, sizeof(short int), &returnSize);
	AEDisposeDesc(&errResult);
	AEDisposeDesc(&errLimits);

	Tcl_DStringAppend(&errStr, "\nThe offending bit of code was:\n\t", -1);
	/*
	 * Get the full line on which the error occured:
	 */
	for (lowerB = srcStart; lowerB > 0; lowerB--) {
	    if (*(scriptData + lowerB ) == '\r') {
		lowerB++;
		break;
	    }
	}
		
	for (upperB = srcEnd; *(scriptData + upperB) != '\0'; upperB++) {
	    if (*(scriptData + upperB) == '\r') {
		break;
	    }
	}

	Tcl_DStringAppend(&errStr, scriptData+lowerB, srcStart - lowerB);
	Tcl_DStringAppend(&errStr, "_", 1);
	Tcl_DStringAppend(&errStr, scriptData+srcStart, upperB - srcStart);
    }
	
    Tcl_DStringResult(interp,&errStr);
}

/*
 *----------------------------------------------------------------------
 *
 * GetRawDataFromDescriptor --
 *
 *	Get the data from a descriptor.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
 
static void
GetRawDataFromDescriptor(
    AEDesc *theDesc,
    Ptr destPtr,
    Size destMaxSize,
    Size *actSize)
  {
      Size copySize;

      if (theDesc->dataHandle) {
	  HLock((Handle)theDesc->dataHandle);
	  *actSize = GetHandleSize((Handle)theDesc->dataHandle);
	  copySize = *actSize < destMaxSize ? *actSize : destMaxSize;
	  BlockMove(*theDesc->dataHandle, destPtr, copySize);
	  HUnlock((Handle)theDesc->dataHandle);
      } else {
	  *actSize = 0;
      }
      
  }

/*
 *----------------------------------------------------------------------
 *
 * GetRawDataFromDescriptor --
 *
 *	Get the data from a descriptor.  Assume it's a C string.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
 
static OSErr
GetCStringFromDescriptor(
    AEDesc *sourceDesc,
    char *resultStr,
    Size resultMaxSize,
    Size *resultSize)
{
    OSErr err;
    AEDesc resultDesc;

    resultDesc.dataHandle = nil;
				
    err = AECoerceDesc(sourceDesc, typeChar, &resultDesc);
		
    if (!err) {
	GetRawDataFromDescriptor(&resultDesc, (Ptr) resultStr,
		resultMaxSize - 1, resultSize);
	resultStr[*resultSize] = 0;
    } else {
	err = errAECoercionFail;
    }
			
    if (resultDesc.dataHandle) {
	AEDisposeDesc(&resultDesc);
    }
    
    return err;
}
