# tclInt.decls --
#
#	This file contains the declarations for all unsupported
#	functions that are exported by the Tcl library.  This file
#	is used to generate the tclIntDecls.h, tclIntPlatDecls.h,
#	tclIntStub.c, tclPlatStub.c, tclCompileDecls.h and tclCompileStub.c
#	files
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# 
# RCS: @(#) Id

library tcl

# Define the unsupported generic interfaces.

interface tclInt

# Declare each of the functions in the unsupported internal Tcl
# interface.  These interfaces are allowed to changed between versions.
# Use at your own risk.  Note that the position of functions should not
# be changed between versions to avoid gratuitous incompatibilities.

declare 0 generic {
    int TclAccess(CONST char *path, int mode)
}
declare 1 generic {
    int TclAccessDeleteProc(TclAccessProc_ *proc)
}
declare 2 generic {
    int TclAccessInsertProc(TclAccessProc_ *proc)
}
declare 3 generic {
    void TclAllocateFreeObjects(void)
}
# Replaced by TclpChdir in 8.1:
#  declare 4 generic {   
#      int TclChdir(Tcl_Interp *interp, char *dirName)
#  }
declare 5 {unix win} {
    int TclCleanupChildren(Tcl_Interp *interp, int numPids, Tcl_Pid *pidPtr, \
	    Tcl_Channel errorChan)
}
declare 6 generic {
    void TclCleanupCommand(Command *cmdPtr)
}
declare 7 generic {
    int TclCopyAndCollapse(int count, CONST char *src, char *dst)
}
declare 8 generic {
    int TclCopyChannel(Tcl_Interp *interp, Tcl_Channel inChan, \
	    Tcl_Channel outChan, int toRead, Tcl_Obj *cmdPtr)
}

# TclCreatePipeline unofficially exported for use by BLT.

declare 9 {unix win} {
    int TclCreatePipeline(Tcl_Interp *interp, int argc, char **argv, \
	    Tcl_Pid **pidArrayPtr, TclFile *inPipePtr, TclFile *outPipePtr, \
	    TclFile *errFilePtr)
}
declare 10 generic {
    int TclCreateProc(Tcl_Interp *interp, Namespace *nsPtr, char *procName, \
	    Tcl_Obj *argsPtr, Tcl_Obj *bodyPtr, Proc **procPtrPtr)
}
declare 11 generic {
    void TclDeleteCompiledLocalVars(Interp *iPtr, CallFrame *framePtr)
}
declare 12 generic {
    void TclDeleteVars(Interp *iPtr, Tcl_HashTable *tablePtr)
}
declare 13 generic {
    int TclDoGlob(Tcl_Interp *interp, char *separators, \
	    Tcl_DString *headPtr, char *tail)
}
declare 14 generic {
    void TclDumpMemoryInfo(FILE *outFile)
}
# Removed in 8.1:
#  declare 15 generic {
#      void TclExpandParseValue(ParseValue *pvPtr, int needed)
#  }
declare 16 generic {
    void TclExprFloatError(Tcl_Interp *interp, double value)
}
declare 17 generic {
    int TclFileAttrsCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
}
declare 18 generic {
    int TclFileCopyCmd(Tcl_Interp *interp, int argc, char **argv)
}
declare 19 generic {
    int TclFileDeleteCmd(Tcl_Interp *interp, int argc, char **argv)
}
declare 20 generic {
    int TclFileMakeDirsCmd(Tcl_Interp *interp, int argc, char **argv)
}
declare 21 generic {
    int TclFileRenameCmd(Tcl_Interp *interp, int argc, char **argv)
}
declare 22 generic {
    int TclFindElement(Tcl_Interp *interp, CONST char *listStr, \
	    int listLength, CONST char **elementPtr, CONST char **nextPtr, \
	    int *sizePtr, int *bracePtr)
}
declare 23 generic {
    Proc * TclFindProc(Interp *iPtr, char *procName)
}
declare 24 generic {
    int TclFormatInt(char *buffer, long n)
}
declare 25 generic {
    void TclFreePackageInfo(Interp *iPtr)
}
# Removed in 8.1:
#  declare 26 generic {	
#      char * TclGetCwd(Tcl_Interp *interp)
#  }
declare 27 generic {
    int TclGetDate(char *p, unsigned long now, long zone, \
	    unsigned long *timePtr)
}
declare 28 generic {
    Tcl_Channel TclpGetDefaultStdChannel(int type)
}
declare 29 generic {
    Tcl_Obj * TclGetElementOfIndexedArray(Tcl_Interp *interp, \
	    int localIndex, Tcl_Obj *elemPtr, int leaveErrorMsg)
}
# Replaced by char * TclGetEnv(CONST char *name, Tcl_DString *valuePtr) in 8.1:
#  declare 30 generic {
#      char * TclGetEnv(CONST char *name)
#  }
declare 31 generic {
    char * TclGetExtension(char *name)
}
declare 32 generic {
    int TclGetFrame(Tcl_Interp *interp, char *str, CallFrame **framePtrPtr)
}
declare 33 generic {
    TclCmdProcType TclGetInterpProc(void)
}
declare 34 generic {
    int TclGetIntForIndex(Tcl_Interp *interp, Tcl_Obj *objPtr, \
	    int endValue, int *indexPtr)
}
declare 35 generic {
    Tcl_Obj * TclGetIndexedScalar(Tcl_Interp *interp, int localIndex, \
	    int leaveErrorMsg)
}
declare 36 generic {
    int TclGetLong(Tcl_Interp *interp, char *str, long *longPtr)
}
declare 37 generic {
    int TclGetLoadedPackages(Tcl_Interp *interp, char *targetName)
}
declare 38 generic {
    int TclGetNamespaceForQualName(Tcl_Interp *interp, char *qualName, \
	    Namespace *cxtNsPtr, int flags, Namespace **nsPtrPtr, \
	    Namespace **altNsPtrPtr, Namespace **actualCxtPtrPtr, \
	    char **simpleNamePtr)
}
declare 39 generic {
    TclObjCmdProcType TclGetObjInterpProc(void)
}
declare 40 generic {
    int TclGetOpenMode(Tcl_Interp *interp, char *str, int *seekFlagPtr)
}
declare 41 generic {
    Tcl_Command TclGetOriginalCommand(Tcl_Command command)
}
declare 42 generic {
    char * TclpGetUserHome(CONST char *name, Tcl_DString *bufferPtr)
}
declare 43 generic {
    int TclGlobalInvoke(Tcl_Interp *interp, int argc, char **argv, int flags)
}
declare 44 generic {
    int TclGuessPackageName(char *fileName, Tcl_DString *bufPtr)
}
declare 45 generic {
    int TclHideUnsafeCommands(Tcl_Interp *interp)
}
declare 46 generic {
    int TclInExit(void)
}
declare 47 generic {
    Tcl_Obj * TclIncrElementOfIndexedArray(Tcl_Interp *interp, \
	    int localIndex, Tcl_Obj *elemPtr, long incrAmount)
}
declare 48 generic {
    Tcl_Obj * TclIncrIndexedScalar(Tcl_Interp *interp, int localIndex, \
	    long incrAmount)
}
declare 49 generic {
    Tcl_Obj * TclIncrVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr, \
	    Tcl_Obj *part2Ptr, long incrAmount, int part1NotParsed)
}
declare 50 generic {
    void TclInitCompiledLocals(Tcl_Interp *interp, CallFrame *framePtr, \
	    Namespace *nsPtr)
}
declare 51 generic {
    int TclInterpInit(Tcl_Interp *interp)
}
declare 52 generic {
    int TclInvoke(Tcl_Interp *interp, int argc, char **argv, int flags)
}
declare 53 generic {
    int TclInvokeObjectCommand(ClientData clientData, Tcl_Interp *interp, \
	    int argc, char **argv)
}
declare 54 generic {
    int TclInvokeStringCommand(ClientData clientData, Tcl_Interp *interp, \
	    int objc, Tcl_Obj *CONST objv[])
}
declare 55 generic {
    Proc * TclIsProc(Command *cmdPtr)
}
# Replaced with TclpLoadFile in 8.1:
#  declare 56 generic {
#      int TclLoadFile(Tcl_Interp *interp, char *fileName, char *sym1, \
#  	    char *sym2, Tcl_PackageInitProc **proc1Ptr, \
#  	    Tcl_PackageInitProc **proc2Ptr)
#  }
# Signature changed to take a length in 8.1:
#  declare 57 generic {
#      int TclLooksLikeInt(char *p)
#  }
declare 58 generic {
    Var * TclLookupVar(Tcl_Interp *interp, char *part1, char *part2, \
	    int flags, char *msg, int createPart1, int createPart2, \
	    Var **arrayPtrPtr)
}
declare 59 generic {
    int TclpMatchFiles(Tcl_Interp *interp, char *separators, \
	    Tcl_DString *dirPtr, char *pattern, char *tail)
}
declare 60 generic {
    int TclNeedSpace(char *start, char *end)
}
declare 61 generic {
    Tcl_Obj * TclNewProcBodyObj(Proc *procPtr)
}
declare 62 generic {
    int TclObjCommandComplete(Tcl_Obj *cmdPtr)
}
declare 63 generic {
    int TclObjInterpProc(ClientData clientData, Tcl_Interp *interp, \
	    int objc, Tcl_Obj *CONST objv[])
}
declare 64 generic {
    int TclObjInvoke(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], \
	    int flags)
}
declare 65 generic {
    int TclObjInvokeGlobal(Tcl_Interp *interp, int objc, \
	    Tcl_Obj *CONST objv[], int flags)
}
declare 66 generic {
    int TclOpenFileChannelDeleteProc(TclOpenFileChannelProc_ *proc)
}
declare 67 generic {
    int TclOpenFileChannelInsertProc(TclOpenFileChannelProc_ *proc)
}
declare 68 generic {
    int TclpAccess(CONST char *path, int mode)
}
declare 69 generic {
    char * TclpAlloc(unsigned int size)
}
declare 70 generic {
    int TclpCopyFile(CONST char *source, CONST char *dest)
}
declare 71 generic {
    int TclpCopyDirectory(CONST char *source, CONST char *dest, \
	    Tcl_DString *errorPtr)
}
declare 72 generic {
    int TclpCreateDirectory(CONST char *path)
}
declare 73 generic {
    int TclpDeleteFile(CONST char *path)
}
declare 74 generic {
    void TclpFree(char *ptr)
}
declare 75 generic {
    unsigned long TclpGetClicks(void)
}
declare 76 generic {
    unsigned long TclpGetSeconds(void)
}
declare 77 generic {
    void TclpGetTime(Tcl_Time *time)
}
declare 78 generic {
    int TclpGetTimeZone(unsigned long time)
}
declare 79 generic {
    int TclpListVolumes(Tcl_Interp *interp)
}
declare 80 generic {
    Tcl_Channel TclpOpenFileChannel(Tcl_Interp *interp, char *fileName, \
	    char *modeString, int permissions)
}
declare 81 generic {
    char * TclpRealloc(char *ptr, unsigned int size)
}
declare 82 generic {
    int TclpRemoveDirectory(CONST char *path, int recursive, \
	    Tcl_DString *errorPtr)
}
declare 83 generic {
    int TclpRenameFile(CONST char *source, CONST char *dest)
}
# Removed in 8.1:
#  declare 84 generic {
#      int TclParseBraces(Tcl_Interp *interp, char *str, char **termPtr, \
#  	    ParseValue *pvPtr)
#  }
#  declare 85 generic {
#      int TclParseNestedCmd(Tcl_Interp *interp, char *str, int flags, \
#  	    char **termPtr, ParseValue *pvPtr)
#  }
#  declare 86 generic {
#      int TclParseQuotes(Tcl_Interp *interp, char *str, int termChar, \
#  	    int flags, char **termPtr, ParseValue *pvPtr)
#  }
#  declare 87 generic {
#      void TclPlatformInit(Tcl_Interp *interp)
#  }
declare 88 generic {
    char * TclPrecTraceProc(ClientData clientData, Tcl_Interp *interp, \
	    char *name1, char *name2, int flags)
}
declare 89 generic {
    int TclPreventAliasLoop(Tcl_Interp *interp, Tcl_Interp *cmdInterp, \
	    Tcl_Command cmd)
}
# Removed in 8.1 (only available if compiled with TCL_COMPILE_DEBUG):
#  declare 90 generic {
#      void TclPrintByteCodeObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
#  }
declare 91 generic {
    void TclProcCleanupProc(Proc *procPtr)
}
declare 92 generic {
    int TclProcCompileProc(Tcl_Interp *interp, Proc *procPtr, \
	    Tcl_Obj *bodyPtr, Namespace *nsPtr, CONST char *description, \
	    CONST char *procName)
}
declare 93 generic {
    void TclProcDeleteProc(ClientData clientData)
}
declare 94 generic {
    int TclProcInterpProc(ClientData clientData, Tcl_Interp *interp, \
	    int argc, char **argv)
}
declare 95 generic {
    int TclpStat(CONST char *path, struct stat *buf)
}
declare 96 generic {
    int TclRenameCommand(Tcl_Interp *interp, char *oldName, char *newName)
}
declare 97 generic {
    void TclResetShadowedCmdRefs(Tcl_Interp *interp, Command *newCmdPtr)
}
declare 98 generic {
    int TclServiceIdle(void)
}
declare 99 generic {
    Tcl_Obj * TclSetElementOfIndexedArray(Tcl_Interp *interp, \
	    int localIndex, Tcl_Obj *elemPtr, Tcl_Obj *objPtr, int leaveErrorMsg)
}
declare 100 generic {
    Tcl_Obj * TclSetIndexedScalar(Tcl_Interp *interp, int localIndex, \
	    Tcl_Obj *objPtr, int leaveErrorMsg)
}
declare 101 {unix win} {
    char * TclSetPreInitScript(char *string)
}
declare 102 generic {
    void TclSetupEnv(Tcl_Interp *interp)
}
declare 103 generic {
    int TclSockGetPort(Tcl_Interp *interp, char *str, char *proto, \
	    int *portPtr)
}
declare 104 {unix win} {
    int TclSockMinimumBuffers(int sock, int size)
}
declare 105 generic {
    int TclStat(CONST char *path, struct stat *buf)
}
declare 106 generic {
    int TclStatDeleteProc(TclStatProc_ *proc)
}
declare 107 generic {
    int TclStatInsertProc(TclStatProc_ *proc)
}
declare 108 generic {
    void TclTeardownNamespace(Namespace *nsPtr)
}
declare 109 generic {
    int TclUpdateReturnInfo(Interp *iPtr)
}
# Removed in 8.1:
#  declare 110 generic {
#      char * TclWordEnd(char *start, char *lastChar, int nested, int *semiPtr)
#  }

# Procedures used in conjunction with Tcl namespaces. They are
# defined here instead of in tcl.decls since they are not stable yet.

declare 111 generic {
    void Tcl_AddInterpResolvers(Tcl_Interp *interp, char *name, \
	    Tcl_ResolveCmdProc *cmdProc, Tcl_ResolveVarProc *varProc, \
	    Tcl_ResolveCompiledVarProc *compiledVarProc)
}
declare 112 generic {
    int Tcl_AppendExportList(Tcl_Interp *interp, Tcl_Namespace *nsPtr, \
	    Tcl_Obj *objPtr)
}
declare 113 generic {
    Tcl_Namespace * Tcl_CreateNamespace(Tcl_Interp *interp, char *name, \
	    ClientData clientData, Tcl_NamespaceDeleteProc *deleteProc)
}
declare 114 generic {
    void Tcl_DeleteNamespace(Tcl_Namespace *nsPtr)
}
declare 115 generic {
    int Tcl_Export(Tcl_Interp *interp, Tcl_Namespace *nsPtr, char *pattern, \
	    int resetListFirst)
}
declare 116 generic {
    Tcl_Command Tcl_FindCommand(Tcl_Interp *interp, char *name, \
	    Tcl_Namespace *contextNsPtr, int flags)
}
declare 117 generic {
    Tcl_Namespace * Tcl_FindNamespace(Tcl_Interp *interp, char *name, \
	    Tcl_Namespace *contextNsPtr, int flags)
}
declare 118 generic {
    int Tcl_GetInterpResolvers(Tcl_Interp *interp, char *name, \
	    Tcl_ResolverInfo *resInfo)
}
declare 119 generic {
    int Tcl_GetNamespaceResolvers(Tcl_Namespace *namespacePtr, \
	    Tcl_ResolverInfo *resInfo)
}
declare 120 generic {
    Tcl_Var Tcl_FindNamespaceVar(Tcl_Interp *interp, char *name, \
	    Tcl_Namespace *contextNsPtr, int flags)
}
declare 121 generic {
    int Tcl_ForgetImport(Tcl_Interp *interp, Tcl_Namespace *nsPtr, \
	    char *pattern)
}
declare 122 generic {
    Tcl_Command Tcl_GetCommandFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 123 generic {
    void Tcl_GetCommandFullName(Tcl_Interp *interp, Tcl_Command command, \
	    Tcl_Obj *objPtr)
}
declare 124 generic {
    Tcl_Namespace * Tcl_GetCurrentNamespace(Tcl_Interp *interp)
}
declare 125 generic {
    Tcl_Namespace * Tcl_GetGlobalNamespace(Tcl_Interp *interp)
}
declare 126 generic {
    void Tcl_GetVariableFullName(Tcl_Interp *interp, Tcl_Var variable, \
	    Tcl_Obj *objPtr)
}
declare 127 generic {
    int Tcl_Import(Tcl_Interp *interp, Tcl_Namespace *nsPtr, \
	    char *pattern, int allowOverwrite)
}
declare 128 generic {
    void Tcl_PopCallFrame(Tcl_Interp* interp)
}
declare 129 generic {
    int Tcl_PushCallFrame(Tcl_Interp* interp, Tcl_CallFrame *framePtr, \
	    Tcl_Namespace *nsPtr, int isProcCallFrame)
} 
declare 130 generic {
    int Tcl_RemoveInterpResolvers(Tcl_Interp *interp, char *name)
}
declare 131 generic {
    void Tcl_SetNamespaceResolvers(Tcl_Namespace *namespacePtr, \
	    Tcl_ResolveCmdProc *cmdProc, Tcl_ResolveVarProc *varProc, \
	    Tcl_ResolveCompiledVarProc *compiledVarProc)
}
declare 132 generic {
    int TclpHasSockets(Tcl_Interp *interp)
}
declare 133 generic {
    struct tm *	TclpGetDate(TclpTime_t time, int useGMT)
}
declare 134 generic {
    size_t TclpStrftime(char *s, size_t maxsize, CONST char *format, \
	    CONST struct tm *t)
}
declare 135 generic {
    int TclpCheckStackSpace(void)
}

# Added in 8.1:

declare 137 generic {
   int TclpChdir(CONST char *dirName)
}
declare 138 generic {
    char * TclGetEnv(CONST char *name, Tcl_DString *valuePtr)
}
declare 139 generic {
    int TclpLoadFile(Tcl_Interp *interp, char *fileName, char *sym1, \
	    char *sym2, Tcl_PackageInitProc **proc1Ptr, \
	    Tcl_PackageInitProc **proc2Ptr, ClientData *clientDataPtr)
}
declare 140 generic {
    int TclLooksLikeInt(char *bytes, int length)
}
declare 141 generic {
    char *TclpGetCwd(Tcl_Interp *interp, Tcl_DString *cwdPtr)
}
declare 142 generic {
    int TclSetByteCodeFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr, \
	    CompileHookProc *hookProc, ClientData clientData)
}
declare 143 generic {
    int TclAddLiteralObj(struct CompileEnv *envPtr, Tcl_Obj *objPtr, \
	    LiteralEntry **litPtrPtr)
}
declare 144 generic {
    void TclHideLiteral(Tcl_Interp *interp, struct CompileEnv *envPtr, \
	    int index)
}
declare 145 generic {
    struct AuxDataType *TclGetAuxDataType(char *typeName)
}

declare 146 generic {
    TclHandle TclHandleCreate(VOID *ptr)
}

declare 147 generic {
    void TclHandleFree(TclHandle handle)
}

declare 148 generic {
    TclHandle TclHandlePreserve(TclHandle handle)
}

declare 149 generic {
    void TclHandleRelease(TclHandle handle)
}

# Added for Tcl 8.2

declare 150 generic {
    int TclRegAbout(Tcl_Interp *interp, Tcl_RegExp re)
}
declare 151 generic {
    void TclRegExpRangeUniChar(Tcl_RegExp re, int index, int *startPtr, \
	    int *endPtr)
}

declare 152 generic {
    void TclSetLibraryPath(Tcl_Obj *pathPtr)
}
declare 153 generic {
    Tcl_Obj *TclGetLibraryPath(void)
}

declare 154 generic {
    int TclTestChannelCmd(ClientData clientData,
    Tcl_Interp *interp, int argc, char **argv)
}
declare 155 generic {
    int TclTestChannelEventCmd(ClientData clientData, \
	    Tcl_Interp *interp, int argc, char **argv)
}
declare 156 generic {
    void TclRegError (Tcl_Interp *interp, char *msg, \
	    int status)
}
declare 157 generic {
    Var * TclVarTraceExists (Tcl_Interp *interp, char *varName)
}

##############################################################################

# Define the platform specific internal Tcl interface. These functions are
# only available on the designated platform.

interface tclIntPlat

########################
# Mac specific internals

declare 0 mac {
    VOID * TclpSysAlloc(long size, int isBin)
}
declare 1 mac {
    void TclpSysFree(VOID *ptr)
}
declare 2 mac {
    VOID * TclpSysRealloc(VOID *cp, unsigned int size)
}
declare 3 mac {
    void TclpExit(int status)
}

# Prototypes for functions found in the tclMacUtil.c compatability library.

declare 4 mac {
    int FSpGetDefaultDir(FSSpecPtr theSpec)
}
declare 5 mac {
    int FSpSetDefaultDir(FSSpecPtr theSpec)
}
declare 6 mac {
    OSErr FSpFindFolder(short vRefNum, OSType folderType, \
	    Boolean createFolder, FSSpec *spec)
}
declare 7 mac {
    void GetGlobalMouse(Point *mouse)
}

# The following routines are utility functions in Tcl.  They are exported
# here because they are needed in Tk.  They are not officially supported,
# however.  The first set are from the MoreFiles package.

declare 8 mac {
    pascal OSErr FSpGetDirectoryID(CONST FSSpec *spec, long *theDirID, \
	    Boolean *isDirectory)
}
declare 9 mac {
    pascal short FSpOpenResFileCompat(CONST FSSpec *spec, \
	    SignedByte permission)
}
declare 10 mac {
    pascal void FSpCreateResFileCompat(CONST FSSpec *spec, OSType creator, \
	    OSType fileType, ScriptCode scriptTag)
}

# Like the MoreFiles routines these fix problems in the standard
# Mac calls.  These routines are from tclMacUtils.h.

declare 11 mac {
    int FSpLocationFromPath(int length, CONST char *path, FSSpecPtr theSpec)
}
declare 12 mac {
    OSErr FSpPathFromLocation(FSSpecPtr theSpec, int *length, \
	    Handle *fullPath)
}

# Prototypes of Mac only internal functions.

declare 13 mac {
    void TclMacExitHandler(void)
}
declare 14 mac {
    void TclMacInitExitToShell(int usePatch)
}
declare 15 mac {
    OSErr TclMacInstallExitToShellPatch(ExitToShellProcPtr newProc)
}
declare 16 mac {
    int TclMacOSErrorToPosixError(int error)
}
declare 17 mac {
    void TclMacRemoveTimer(void *timerToken)
}
declare 18 mac {
    void * TclMacStartTimer(long ms)
}
declare 19 mac {
    int TclMacTimerExpired(void *timerToken)
}
declare 20 mac {
    int TclMacRegisterResourceFork(short fileRef, Tcl_Obj *tokenPtr, \
	    int insert)
}	
declare 21 mac {
    short TclMacUnRegisterResourceFork(char *tokenPtr, Tcl_Obj *resultPtr)
}	
declare 22 mac {
    int TclMacCreateEnv(void)
}
declare 23 mac {
    FILE * TclMacFOpenHack(CONST char *path, CONST char *mode)
}
# Replaced in 8.1 by TclpReadLink:
#  declare 24 mac {
#      int TclMacReadlink(char *path, char *buf, int size)
#  }
declare 25 mac {
    int TclMacChmod(char *path, int mode)
}

############################
# Windows specific internals

declare 0 win {
    void TclWinConvertError(DWORD errCode)
}
declare 1 win {
    void TclWinConvertWSAError(DWORD errCode)
}
declare 2 win {
    struct servent * TclWinGetServByName(CONST char *nm, \
	    CONST char *proto)
}
declare 3 win {
    int TclWinGetSockOpt(SOCKET s, int level, int optname, \
	    char FAR * optval, int FAR *optlen)
}
declare 4 win {
    HINSTANCE TclWinGetTclInstance(void)
}
# Removed in 8.1:
#  declare 5 win {
#      HINSTANCE TclWinLoadLibrary(char *name)
#  }
declare 6 win {
    u_short TclWinNToHS(u_short ns)
}
declare 7 win {
    int TclWinSetSockOpt(SOCKET s, int level, int optname, \
	    CONST char FAR * optval, int optlen)
}
declare 8 win {
    unsigned long TclpGetPid(Tcl_Pid pid)
}
declare 9 win {
    int TclWinGetPlatformId(void)
}
declare 10 win {
    int TclWinSynchSpawn(void *args, int type, void **trans, Tcl_Pid *pidPtr)
}

# Pipe channel functions

declare 11 win {
    void TclGetAndDetachPids(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 12 win {
    int TclpCloseFile(TclFile file)
}
declare 13 win {
    Tcl_Channel TclpCreateCommandChannel(TclFile readFile, \
	    TclFile writeFile, TclFile errorFile, int numPids, Tcl_Pid *pidPtr)
}
declare 14 win {
    int TclpCreatePipe(TclFile *readPipe, TclFile *writePipe)
}
declare 15 win {
    int TclpCreateProcess(Tcl_Interp *interp, int argc, char **argv, \
	    TclFile inputFile, TclFile outputFile, TclFile errorFile, \
	    Tcl_Pid *pidPtr)
}
# Signature changed in 8.1:
#  declare 16 win {
#      TclFile TclpCreateTempFile(char *contents, Tcl_DString *namePtr)
#  }
#  declare 17 win {
#      char * TclpGetTZName(void)
#  }
declare 18 win {
    TclFile TclpMakeFile(Tcl_Channel channel, int direction)
}
declare 19 win {
    TclFile TclpOpenFile(CONST char *fname, int mode)
}
declare 20 win {
    void TclWinAddProcess(HANDLE hProcess, DWORD id)
}
declare 21 win {
    void TclpAsyncMark(Tcl_AsyncHandler async)
}

# Added in 8.1:
declare 22 win {
    TclFile TclpCreateTempFile(CONST char *contents)
}
declare 23 win {
    char * TclpGetTZName(int isdst)
}
declare 24 win {
    char * TclWinNoBackslash(char *path)
}
declare 25 win {
    TclPlatformType *TclWinGetPlatform(void)
}
declare 26 win {
    void TclWinSetInterfaces(int wide)
}

#########################
# Unix specific internals

# Pipe channel functions

declare 0 unix {
    void TclGetAndDetachPids(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 1 unix {
    int TclpCloseFile(TclFile file)
}
declare 2 unix {
    Tcl_Channel TclpCreateCommandChannel(TclFile readFile, \
	    TclFile writeFile, TclFile errorFile, int numPids, Tcl_Pid *pidPtr)
}
declare 3 unix {
    int TclpCreatePipe(TclFile *readPipe, TclFile *writePipe)
}
declare 4 unix {
    int TclpCreateProcess(Tcl_Interp *interp, int argc, char **argv, \
	    TclFile inputFile, TclFile outputFile, TclFile errorFile, \
	    Tcl_Pid *pidPtr)
}
# Signature changed in 8.1:
#  declare 5 unix {
#      TclFile TclpCreateTempFile(char *contents, 
#      Tcl_DString *namePtr)
#  }
declare 6 unix {
    TclFile TclpMakeFile(Tcl_Channel channel, int direction)
}
declare 7 unix {
    TclFile TclpOpenFile(CONST char *fname, int mode)
}
declare 8 unix {
    int TclUnixWaitForFile(int fd, int mask, int timeout)
}

# Added in 8.1:

declare 9 unix {
    TclFile TclpCreateTempFile(CONST char *contents)
}
