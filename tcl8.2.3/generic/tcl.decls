# tcl.decls --
#
#	This file contains the declarations for all supported public
#	functions that are exported by the Tcl library via the stubs table.
#	This file is used to generate the tclDecls.h, tclPlatDecls.h,
#	tclStub.c, and tclPlatStub.c files.
#	
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# 
# RCS: @(#) Id

library tcl

# Define the tcl interface with several sub interfaces:
#     tclPlat	 - platform specific public
#     tclInt	 - generic private
#     tclPlatInt - platform specific private

interface tcl
hooks {tclPlat tclInt tclIntPlat}

# Declare each of the functions in the public Tcl interface.  Note that
# the an index should never be reused for a different function in order
# to preserve backwards compatibility.

declare 0 generic {
    int Tcl_PkgProvideEx(Tcl_Interp *interp, char *name, char *version, \
	    ClientData clientData)
}
declare 1 generic {
    char * Tcl_PkgRequireEx(Tcl_Interp *interp, char *name, char *version, \
	    int exact, ClientData *clientDataPtr)
}
declare 2 generic {
    void Tcl_Panic(char *format, ...)
}
declare 3 generic {
    char * Tcl_Alloc(unsigned int size)
}
declare 4 generic {
    void Tcl_Free(char *ptr)
}
declare 5 generic {
    char * Tcl_Realloc(char *ptr, unsigned int size)
}
declare 6 generic {
    char * Tcl_DbCkalloc(unsigned int size, char *file, int line)
}
declare 7 generic {
    int Tcl_DbCkfree(char *ptr, char *file, int line)
}
declare 8 generic {
    char * Tcl_DbCkrealloc(char *ptr, unsigned int size, char *file, int line)
}

# Tcl_CreateFileHandler and Tcl_DeleteFileHandler are only available on unix,
# but they are part of the old generic interface, so we include them here for
# compatibility reasons.

declare 9 unix {
    void Tcl_CreateFileHandler(int fd, int mask, Tcl_FileProc *proc, \
	    ClientData clientData)
}
declare 10 unix {
    void Tcl_DeleteFileHandler(int fd)
}

declare 11 generic {
    void Tcl_SetTimer(Tcl_Time *timePtr)
}
declare 12 generic {
    void Tcl_Sleep(int ms)
}
declare 13 generic {
    int Tcl_WaitForEvent(Tcl_Time *timePtr)
}
declare 14 generic {
    int Tcl_AppendAllObjTypes(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 15 generic {
    void Tcl_AppendStringsToObj(Tcl_Obj *objPtr, ...)
}
declare 16 generic {
    void Tcl_AppendToObj(Tcl_Obj *objPtr, char *bytes, int length)
}
declare 17 generic {
    Tcl_Obj * Tcl_ConcatObj(int objc, Tcl_Obj *CONST objv[])
}
declare 18 generic {
    int Tcl_ConvertToType(Tcl_Interp *interp, Tcl_Obj *objPtr, \
	    Tcl_ObjType *typePtr)
}
declare 19 generic {
    void Tcl_DbDecrRefCount(Tcl_Obj *objPtr, char *file, int line)
}
declare 20 generic {
    void Tcl_DbIncrRefCount(Tcl_Obj *objPtr, char *file, int line)
}
declare 21 generic {
    int Tcl_DbIsShared(Tcl_Obj *objPtr, char *file, int line)
}
declare 22 generic {
    Tcl_Obj * Tcl_DbNewBooleanObj(int boolValue, char *file, int line)
}
declare 23 generic {
    Tcl_Obj * Tcl_DbNewByteArrayObj(unsigned char *bytes, int length, \
	    char *file, int line)
}
declare 24 generic {
    Tcl_Obj * Tcl_DbNewDoubleObj(double doubleValue, char *file, int line)
}
declare 25 generic {
    Tcl_Obj * Tcl_DbNewListObj(int objc, Tcl_Obj *CONST objv[], char *file, \
	    int line)
}
declare 26 generic {
    Tcl_Obj * Tcl_DbNewLongObj(long longValue, char *file, int line)
}
declare 27 generic {
    Tcl_Obj * Tcl_DbNewObj(char *file, int line)
}
declare 28 generic {
    Tcl_Obj * Tcl_DbNewStringObj(CONST char *bytes, int length, \
	    char *file, int line)
}
declare 29 generic {
    Tcl_Obj * Tcl_DuplicateObj(Tcl_Obj *objPtr)
}
declare 30 generic {
    void TclFreeObj(Tcl_Obj *objPtr)
}
declare 31 generic {
    int Tcl_GetBoolean(Tcl_Interp *interp, char *str, int *boolPtr)
}
declare 32 generic {
    int Tcl_GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, \
	    int *boolPtr)
}
declare 33 generic {
    unsigned char * Tcl_GetByteArrayFromObj(Tcl_Obj *objPtr, int *lengthPtr)
}
declare 34 generic {
    int Tcl_GetDouble(Tcl_Interp *interp, char *str, double *doublePtr)
}
declare 35 generic {
    int Tcl_GetDoubleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, \
	    double *doublePtr)
}
declare 36 generic {
    int Tcl_GetIndexFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, \
	    char **tablePtr, char *msg, int flags, int *indexPtr)
}
declare 37 generic {
    int Tcl_GetInt(Tcl_Interp *interp, char *str, int *intPtr)
}
declare 38 generic {
    int Tcl_GetIntFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *intPtr)
}
declare 39 generic {
    int Tcl_GetLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, long *longPtr)
}
declare 40 generic {
    Tcl_ObjType * Tcl_GetObjType(char *typeName)
}
declare 41 generic {
    char * Tcl_GetStringFromObj(Tcl_Obj *objPtr, int *lengthPtr)
}
declare 42 generic {
    void Tcl_InvalidateStringRep(Tcl_Obj *objPtr)
}
declare 43 generic {
    int Tcl_ListObjAppendList(Tcl_Interp *interp, Tcl_Obj *listPtr, \
	    Tcl_Obj *elemListPtr)
}
declare 44 generic {
    int Tcl_ListObjAppendElement(Tcl_Interp *interp, Tcl_Obj *listPtr, \
	    Tcl_Obj *objPtr)
}
declare 45 generic {
    int Tcl_ListObjGetElements(Tcl_Interp *interp, Tcl_Obj *listPtr, \
	    int *objcPtr, Tcl_Obj ***objvPtr)
}
declare 46 generic {
    int Tcl_ListObjIndex(Tcl_Interp *interp, Tcl_Obj *listPtr, int index, \
	    Tcl_Obj **objPtrPtr)
}
declare 47 generic {
    int Tcl_ListObjLength(Tcl_Interp *interp, Tcl_Obj *listPtr, int *intPtr)
}
declare 48 generic {
    int Tcl_ListObjReplace(Tcl_Interp *interp, Tcl_Obj *listPtr, int first, \
	    int count, int objc, Tcl_Obj *CONST objv[])
}
declare 49 generic {
    Tcl_Obj * Tcl_NewBooleanObj(int boolValue)
}
declare 50 generic {
    Tcl_Obj * Tcl_NewByteArrayObj(unsigned char *bytes, int length)
}
declare 51 generic {
    Tcl_Obj * Tcl_NewDoubleObj(double doubleValue)
}
declare 52 generic {
    Tcl_Obj * Tcl_NewIntObj(int intValue)
}
declare 53 generic {
    Tcl_Obj * Tcl_NewListObj(int objc, Tcl_Obj *CONST objv[])
}
declare 54 generic {
    Tcl_Obj * Tcl_NewLongObj(long longValue)
}
declare 55 generic {
    Tcl_Obj * Tcl_NewObj(void)
}
declare 56 generic {
    Tcl_Obj *Tcl_NewStringObj(CONST char *bytes, int length)
}
declare 57 generic {
    void Tcl_SetBooleanObj(Tcl_Obj *objPtr, int boolValue)
}
declare 58 generic {
    unsigned char * Tcl_SetByteArrayLength(Tcl_Obj *objPtr, int length)
}
declare 59 generic {
    void Tcl_SetByteArrayObj(Tcl_Obj *objPtr, unsigned char *bytes, int length)
}
declare 60 generic {
    void Tcl_SetDoubleObj(Tcl_Obj *objPtr, double doubleValue)
}
declare 61 generic {
    void Tcl_SetIntObj(Tcl_Obj *objPtr, int intValue)
}
declare 62 generic {
    void Tcl_SetListObj(Tcl_Obj *objPtr, int objc, Tcl_Obj *CONST objv[])
}
declare 63 generic {
    void Tcl_SetLongObj(Tcl_Obj *objPtr, long longValue)
}
declare 64 generic {
    void Tcl_SetObjLength(Tcl_Obj *objPtr, int length)
}
declare 65 generic {
    void Tcl_SetStringObj(Tcl_Obj *objPtr, char *bytes, int length)
}
declare 66 generic {
    void Tcl_AddErrorInfo(Tcl_Interp *interp, CONST char *message)
}
declare 67 generic {
    void Tcl_AddObjErrorInfo(Tcl_Interp *interp, CONST char *message, \
	    int length)
}
declare 68 generic {
    void Tcl_AllowExceptions(Tcl_Interp *interp)
}
declare 69 generic {
    void Tcl_AppendElement(Tcl_Interp *interp, CONST char *string)
}
declare 70 generic {
    void Tcl_AppendResult(Tcl_Interp *interp, ...)
}
declare 71 generic {
    Tcl_AsyncHandler Tcl_AsyncCreate(Tcl_AsyncProc *proc, \
	    ClientData clientData)
}
declare 72 generic {
    void Tcl_AsyncDelete(Tcl_AsyncHandler async)
}
declare 73 generic {
    int Tcl_AsyncInvoke(Tcl_Interp *interp, int code)
}
declare 74 generic {
    void Tcl_AsyncMark(Tcl_AsyncHandler async)
}
declare 75 generic {
    int Tcl_AsyncReady(void)
}
declare 76 generic {
    void Tcl_BackgroundError(Tcl_Interp *interp)
}
declare 77 generic {
    char Tcl_Backslash(CONST char *src, int *readPtr)
}
declare 78 generic {
    int Tcl_BadChannelOption(Tcl_Interp *interp, char *optionName, \
	    char *optionList)
}
declare 79 generic {
    void Tcl_CallWhenDeleted(Tcl_Interp *interp, Tcl_InterpDeleteProc *proc, \
	    ClientData clientData)
}
declare 80 generic {
    void Tcl_CancelIdleCall(Tcl_IdleProc *idleProc, ClientData clientData)
}
declare 81 generic {
    int Tcl_Close(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 82 generic {
    int Tcl_CommandComplete(char *cmd)
}
declare 83 generic {
    char * Tcl_Concat(int argc, char **argv)
}
declare 84 generic {
    int Tcl_ConvertElement(CONST char *src, char *dst, int flags)
}
declare 85 generic {
    int Tcl_ConvertCountedElement(CONST char *src, int length, char *dst, \
	    int flags)
}
declare 86 generic {
    int Tcl_CreateAlias(Tcl_Interp *slave, char *slaveCmd, \
	    Tcl_Interp *target, char *targetCmd, int argc, char **argv)
}
declare 87 generic {
    int Tcl_CreateAliasObj(Tcl_Interp *slave, char *slaveCmd, \
	    Tcl_Interp *target, char *targetCmd, int objc, \
	    Tcl_Obj *CONST objv[])
}
declare 88 generic {
    Tcl_Channel Tcl_CreateChannel(Tcl_ChannelType *typePtr, char *chanName, \
	    ClientData instanceData, int mask)
}
declare 89 generic {
    void Tcl_CreateChannelHandler(Tcl_Channel chan, int mask, \
	    Tcl_ChannelProc *proc, ClientData clientData)
}
declare 90 generic {
    void Tcl_CreateCloseHandler(Tcl_Channel chan, Tcl_CloseProc *proc, \
	    ClientData clientData)
}
declare 91 generic {
    Tcl_Command Tcl_CreateCommand(Tcl_Interp *interp, char *cmdName, \
	    Tcl_CmdProc *proc, ClientData clientData, \
	    Tcl_CmdDeleteProc *deleteProc)
}
declare 92 generic {
    void Tcl_CreateEventSource(Tcl_EventSetupProc *setupProc, \
	    Tcl_EventCheckProc *checkProc, ClientData clientData)
}
declare 93 generic {
    void Tcl_CreateExitHandler(Tcl_ExitProc *proc, ClientData clientData)
}
declare 94 generic {
    Tcl_Interp * Tcl_CreateInterp(void)
}
declare 95 generic {
    void Tcl_CreateMathFunc(Tcl_Interp *interp, char *name, int numArgs, \
	    Tcl_ValueType *argTypes, Tcl_MathProc *proc, ClientData clientData)
}
declare 96 generic {
    Tcl_Command Tcl_CreateObjCommand(Tcl_Interp *interp, char *cmdName, \
	    Tcl_ObjCmdProc *proc, ClientData clientData, \
	    Tcl_CmdDeleteProc *deleteProc)
}
declare 97 generic {
    Tcl_Interp * Tcl_CreateSlave(Tcl_Interp *interp, char *slaveName, \
	    int isSafe)
}
declare 98 generic {
    Tcl_TimerToken Tcl_CreateTimerHandler(int milliseconds, \
	    Tcl_TimerProc *proc, ClientData clientData)
}
declare 99 generic {
    Tcl_Trace Tcl_CreateTrace(Tcl_Interp *interp, int level, \
	    Tcl_CmdTraceProc *proc, ClientData clientData)
}
declare 100 generic {
    void Tcl_DeleteAssocData(Tcl_Interp *interp, char *name)
}
declare 101 generic {
    void Tcl_DeleteChannelHandler(Tcl_Channel chan, Tcl_ChannelProc *proc, \
	    ClientData clientData)
}
declare 102 generic {
    void Tcl_DeleteCloseHandler(Tcl_Channel chan, Tcl_CloseProc *proc, \
	    ClientData clientData)
}
declare 103 generic {
    int Tcl_DeleteCommand(Tcl_Interp *interp, char *cmdName)
}
declare 104 generic {
    int Tcl_DeleteCommandFromToken(Tcl_Interp *interp, Tcl_Command command)
}
declare 105 generic {
    void Tcl_DeleteEvents(Tcl_EventDeleteProc *proc, ClientData clientData)
}
declare 106 generic {
    void Tcl_DeleteEventSource(Tcl_EventSetupProc *setupProc, \
	    Tcl_EventCheckProc *checkProc, ClientData clientData)
}
declare 107 generic {
    void Tcl_DeleteExitHandler(Tcl_ExitProc *proc, ClientData clientData)
}
declare 108 generic {
    void Tcl_DeleteHashEntry(Tcl_HashEntry *entryPtr)
}
declare 109 generic {
    void Tcl_DeleteHashTable(Tcl_HashTable *tablePtr)
}
declare 110 generic {
    void Tcl_DeleteInterp(Tcl_Interp *interp)
}
declare 111 {unix win} {
    void Tcl_DetachPids(int numPids, Tcl_Pid *pidPtr)
}
declare 112 generic {
    void Tcl_DeleteTimerHandler(Tcl_TimerToken token)
}
declare 113 generic {
    void Tcl_DeleteTrace(Tcl_Interp *interp, Tcl_Trace trace)
}
declare 114 generic {
    void Tcl_DontCallWhenDeleted(Tcl_Interp *interp, \
	    Tcl_InterpDeleteProc *proc, ClientData clientData)
}
declare 115 generic {
    int Tcl_DoOneEvent(int flags)
}
declare 116 generic {
    void Tcl_DoWhenIdle(Tcl_IdleProc *proc, ClientData clientData)
}
declare 117 generic {
    char * Tcl_DStringAppend(Tcl_DString *dsPtr, CONST char *str, int length)
}
declare 118 generic {
    char * Tcl_DStringAppendElement(Tcl_DString *dsPtr, CONST char *string)
}
declare 119 generic {
    void Tcl_DStringEndSublist(Tcl_DString *dsPtr)
}
declare 120 generic {
    void Tcl_DStringFree(Tcl_DString *dsPtr)
}
declare 121 generic {
    void Tcl_DStringGetResult(Tcl_Interp *interp, Tcl_DString *dsPtr)
}
declare 122 generic {
    void Tcl_DStringInit(Tcl_DString *dsPtr)
}
declare 123 generic {
    void Tcl_DStringResult(Tcl_Interp *interp, Tcl_DString *dsPtr)
}
declare 124 generic {
    void Tcl_DStringSetLength(Tcl_DString *dsPtr, int length)
}
declare 125 generic {
    void Tcl_DStringStartSublist(Tcl_DString *dsPtr)
}
declare 126 generic {
    int Tcl_Eof(Tcl_Channel chan)
}
declare 127 generic {
    char * Tcl_ErrnoId(void)
}
declare 128 generic {
    char * Tcl_ErrnoMsg(int err)
}
declare 129 generic {
    int Tcl_Eval(Tcl_Interp *interp, char *string)
}
declare 130 generic {
    int Tcl_EvalFile(Tcl_Interp *interp, char *fileName)
}
declare 131 generic {
    int Tcl_EvalObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 132 generic {
    void Tcl_EventuallyFree(ClientData clientData, Tcl_FreeProc *freeProc)
}
declare 133 generic {
    void Tcl_Exit(int status)
}
declare 134 generic {
    int Tcl_ExposeCommand(Tcl_Interp *interp, char *hiddenCmdToken, \
	    char *cmdName)
}
declare 135 generic {
    int Tcl_ExprBoolean(Tcl_Interp *interp, char *str, int *ptr)
}
declare 136 generic {
    int Tcl_ExprBooleanObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *ptr)
}
declare 137 generic {
    int Tcl_ExprDouble(Tcl_Interp *interp, char *str, double *ptr)
}
declare 138 generic {
    int Tcl_ExprDoubleObj(Tcl_Interp *interp, Tcl_Obj *objPtr, double *ptr)
}
declare 139 generic {
    int Tcl_ExprLong(Tcl_Interp *interp, char *str, long *ptr)
}
declare 140 generic {
    int Tcl_ExprLongObj(Tcl_Interp *interp, Tcl_Obj *objPtr, long *ptr)
}
declare 141 generic {
    int Tcl_ExprObj(Tcl_Interp *interp, Tcl_Obj *objPtr, \
	    Tcl_Obj **resultPtrPtr)
}
declare 142 generic {
    int Tcl_ExprString(Tcl_Interp *interp, char *string)
}
declare 143 generic {
    void Tcl_Finalize(void)
}
declare 144 generic {
    void Tcl_FindExecutable(CONST char *argv0)
}
declare 145 generic {
    Tcl_HashEntry * Tcl_FirstHashEntry(Tcl_HashTable *tablePtr, \
	    Tcl_HashSearch *searchPtr)
}
declare 146 generic {
    int Tcl_Flush(Tcl_Channel chan)
}
declare 147 generic {
    void Tcl_FreeResult(Tcl_Interp *interp)
}
declare 148 generic {
    int Tcl_GetAlias(Tcl_Interp *interp, char *slaveCmd, \
	    Tcl_Interp **targetInterpPtr, char **targetCmdPtr, int *argcPtr, \
	    char ***argvPtr)
}
declare 149 generic {
    int Tcl_GetAliasObj(Tcl_Interp *interp, char *slaveCmd, \
	    Tcl_Interp **targetInterpPtr, char **targetCmdPtr, int *objcPtr, \
	    Tcl_Obj ***objv)
}
declare 150 generic {
    ClientData Tcl_GetAssocData(Tcl_Interp *interp, char *name, \
	    Tcl_InterpDeleteProc **procPtr)
}
declare 151 generic {
    Tcl_Channel Tcl_GetChannel(Tcl_Interp *interp, char *chanName, \
	    int *modePtr)
}
declare 152 generic {
    int Tcl_GetChannelBufferSize(Tcl_Channel chan)
}
declare 153 generic {
    int Tcl_GetChannelHandle(Tcl_Channel chan, int direction, \
	    ClientData *handlePtr)
}
declare 154 generic {
    ClientData Tcl_GetChannelInstanceData(Tcl_Channel chan)
}
declare 155 generic {
    int Tcl_GetChannelMode(Tcl_Channel chan)
}
declare 156 generic {
    char * Tcl_GetChannelName(Tcl_Channel chan)
}
declare 157 generic {
    int Tcl_GetChannelOption(Tcl_Interp *interp, Tcl_Channel chan, \
	    char *optionName, Tcl_DString *dsPtr)
}
declare 158 generic {
    Tcl_ChannelType * Tcl_GetChannelType(Tcl_Channel chan)
}
declare 159 generic {
    int Tcl_GetCommandInfo(Tcl_Interp *interp, char *cmdName, \
	    Tcl_CmdInfo *infoPtr)
}
declare 160 generic {
    char * Tcl_GetCommandName(Tcl_Interp *interp, Tcl_Command command)
}
declare 161 generic {
    int Tcl_GetErrno(void)
}
declare 162 generic {
    char * Tcl_GetHostName(void)
}
declare 163 generic {
    int Tcl_GetInterpPath(Tcl_Interp *askInterp, Tcl_Interp *slaveInterp)
}
declare 164 generic {
    Tcl_Interp * Tcl_GetMaster(Tcl_Interp *interp)
}
declare 165 generic {
    CONST char * Tcl_GetNameOfExecutable(void)
}
declare 166 generic {
    Tcl_Obj * Tcl_GetObjResult(Tcl_Interp *interp)
}

# Tcl_GetOpenFile is only available on unix, but it is a part of the old
# generic interface, so we inlcude it here for compatibility reasons.

declare 167 unix {
    int Tcl_GetOpenFile(Tcl_Interp *interp, char *str, int write, \
	    int checkUsage, ClientData *filePtr)
}

declare 168 generic {
    Tcl_PathType Tcl_GetPathType(char *path)
}
declare 169 generic {
    int Tcl_Gets(Tcl_Channel chan, Tcl_DString *dsPtr)
}
declare 170 generic {
    int Tcl_GetsObj(Tcl_Channel chan, Tcl_Obj *objPtr)
}
declare 171 generic {
    int Tcl_GetServiceMode(void)
}
declare 172 generic {
    Tcl_Interp * Tcl_GetSlave(Tcl_Interp *interp, char *slaveName)
}
declare 173 generic {
    Tcl_Channel Tcl_GetStdChannel(int type)
}
declare 174 generic {
    char * Tcl_GetStringResult(Tcl_Interp *interp)
}
declare 175 generic {
    char * Tcl_GetVar(Tcl_Interp *interp, char *varName, int flags)
}
declare 176 generic {
    char * Tcl_GetVar2(Tcl_Interp *interp, char *part1, char *part2, int flags)
}
declare 177 generic {
    int Tcl_GlobalEval(Tcl_Interp *interp, char *command)
}
declare 178 generic {
    int Tcl_GlobalEvalObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 179 generic {
    int Tcl_HideCommand(Tcl_Interp *interp, char *cmdName, \
	    char *hiddenCmdToken)
}
declare 180 generic {
    int Tcl_Init(Tcl_Interp *interp)
}
declare 181 generic {
    void Tcl_InitHashTable(Tcl_HashTable *tablePtr, int keyType)
}
declare 182 generic {
    int Tcl_InputBlocked(Tcl_Channel chan)
}
declare 183 generic {
    int Tcl_InputBuffered(Tcl_Channel chan)
}
declare 184 generic {
    int Tcl_InterpDeleted(Tcl_Interp *interp)
}
declare 185 generic {
    int Tcl_IsSafe(Tcl_Interp *interp)
}
declare 186 generic {
    char * Tcl_JoinPath(int argc, char **argv, Tcl_DString *resultPtr)
}
declare 187 generic {
    int Tcl_LinkVar(Tcl_Interp *interp, char *varName, char *addr, int type)
}

# This slot is reserved for use by the plus patch:
#  declare 188 generic {
#      Tcl_MainLoop
#  }

declare 189 generic {
    Tcl_Channel Tcl_MakeFileChannel(ClientData handle, int mode)
}
declare 190 generic {
    int Tcl_MakeSafe(Tcl_Interp *interp)
}
declare 191 generic {
    Tcl_Channel Tcl_MakeTcpClientChannel(ClientData tcpSocket)
}
declare 192 generic {
    char * Tcl_Merge(int argc, char **argv)
}
declare 193 generic {
    Tcl_HashEntry * Tcl_NextHashEntry(Tcl_HashSearch *searchPtr)
}
declare 194 generic {
    void Tcl_NotifyChannel(Tcl_Channel channel, int mask)
}
declare 195 generic {
    Tcl_Obj * Tcl_ObjGetVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr, \
	    Tcl_Obj *part2Ptr, int flags)
}
declare 196 generic {
    Tcl_Obj * Tcl_ObjSetVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr, \
	    Tcl_Obj *part2Ptr, Tcl_Obj *newValuePtr, int flags)
}
declare 197 {unix win} {
    Tcl_Channel Tcl_OpenCommandChannel(Tcl_Interp *interp, int argc, \
	    char **argv, int flags)
}
declare 198 generic {
    Tcl_Channel Tcl_OpenFileChannel(Tcl_Interp *interp, char *fileName, \
	    char *modeString, int permissions)
}
declare 199 generic {
    Tcl_Channel Tcl_OpenTcpClient(Tcl_Interp *interp, int port, \
	    char *address, char *myaddr, int myport, int async)
}
declare 200 generic {
    Tcl_Channel Tcl_OpenTcpServer(Tcl_Interp *interp, int port, char *host, \
	    Tcl_TcpAcceptProc *acceptProc, ClientData callbackData)
}
declare 201 generic {
    void Tcl_Preserve(ClientData data)
}
declare 202 generic {
    void Tcl_PrintDouble(Tcl_Interp *interp, double value, char *dst)
}
declare 203 generic {
    int Tcl_PutEnv(CONST char *string)
}
declare 204 generic {
    char * Tcl_PosixError(Tcl_Interp *interp)
}
declare 205 generic {
    void Tcl_QueueEvent(Tcl_Event *evPtr, Tcl_QueuePosition position)
}
declare 206 generic {
    int Tcl_Read(Tcl_Channel chan, char *bufPtr, int toRead)
}
declare 207 {unix win} {
    void Tcl_ReapDetachedProcs(void)
}
declare 208 generic {
    int Tcl_RecordAndEval(Tcl_Interp *interp, char *cmd, int flags)
}
declare 209 generic {
    int Tcl_RecordAndEvalObj(Tcl_Interp *interp, Tcl_Obj *cmdPtr, int flags)
}
declare 210 generic {
    void Tcl_RegisterChannel(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 211 generic {
    void Tcl_RegisterObjType(Tcl_ObjType *typePtr)
}
declare 212 generic {
    Tcl_RegExp Tcl_RegExpCompile(Tcl_Interp *interp, char *string)
}
declare 213 generic {
    int Tcl_RegExpExec(Tcl_Interp *interp, Tcl_RegExp regexp, \
	    CONST char *str, CONST char *start)
}
declare 214 generic {
    int Tcl_RegExpMatch(Tcl_Interp *interp, char *str, char *pattern)
}
declare 215 generic {
    void Tcl_RegExpRange(Tcl_RegExp regexp, int index, char **startPtr, \
	    char **endPtr)
}
declare 216 generic {
    void Tcl_Release(ClientData clientData)
}
declare 217 generic {
    void Tcl_ResetResult(Tcl_Interp *interp)
}
declare 218 generic {
    int Tcl_ScanElement(CONST char *str, int *flagPtr)
}
declare 219 generic {
    int Tcl_ScanCountedElement(CONST char *str, int length, int *flagPtr)
}
declare 220 generic {
    int Tcl_Seek(Tcl_Channel chan, int offset, int mode)
}
declare 221 generic {
    int Tcl_ServiceAll(void)
}
declare 222 generic {
    int Tcl_ServiceEvent(int flags)
}
declare 223 generic {
    void Tcl_SetAssocData(Tcl_Interp *interp, char *name, \
	    Tcl_InterpDeleteProc *proc, ClientData clientData)
}
declare 224 generic {
    void Tcl_SetChannelBufferSize(Tcl_Channel chan, int sz)
}
declare 225 generic {
    int Tcl_SetChannelOption(Tcl_Interp *interp, Tcl_Channel chan, \
	    char *optionName, char *newValue)
}
declare 226 generic {
    int Tcl_SetCommandInfo(Tcl_Interp *interp, char *cmdName, \
	    Tcl_CmdInfo *infoPtr)
}
declare 227 generic {
    void Tcl_SetErrno(int err)
}
declare 228 generic {
    void Tcl_SetErrorCode(Tcl_Interp *interp, ...)
}
declare 229 generic {
    void Tcl_SetMaxBlockTime(Tcl_Time *timePtr)
}
declare 230 generic {
    void Tcl_SetPanicProc(Tcl_PanicProc *panicProc)
}
declare 231 generic {
    int Tcl_SetRecursionLimit(Tcl_Interp *interp, int depth)
}
declare 232 generic {
    void Tcl_SetResult(Tcl_Interp *interp, char *str, \
	    Tcl_FreeProc *freeProc)
}
declare 233 generic {
    int Tcl_SetServiceMode(int mode)
}
declare 234 generic {
    void Tcl_SetObjErrorCode(Tcl_Interp *interp, Tcl_Obj *errorObjPtr)
}
declare 235 generic {
    void Tcl_SetObjResult(Tcl_Interp *interp, Tcl_Obj *resultObjPtr)
}
declare 236 generic {
    void Tcl_SetStdChannel(Tcl_Channel channel, int type)
}
declare 237 generic {
    char * Tcl_SetVar(Tcl_Interp *interp, char *varName, char *newValue, \
	    int flags)
}
declare 238 generic {
    char * Tcl_SetVar2(Tcl_Interp *interp, char *part1, char *part2, \
	    char *newValue, int flags)
}
declare 239 generic {
    char * Tcl_SignalId(int sig)
}
declare 240 generic {
    char * Tcl_SignalMsg(int sig)
}
declare 241 generic {
    void Tcl_SourceRCFile(Tcl_Interp *interp)
}
declare 242 generic {
    int Tcl_SplitList(Tcl_Interp *interp, CONST char *listStr, int *argcPtr, \
	    char ***argvPtr)
}
declare 243 generic {
    void Tcl_SplitPath(CONST char *path, int *argcPtr, char ***argvPtr)
}
declare 244 generic {
    void Tcl_StaticPackage(Tcl_Interp *interp, char *pkgName, \
	    Tcl_PackageInitProc *initProc, Tcl_PackageInitProc *safeInitProc)
}
declare 245 generic {
    int Tcl_StringMatch(CONST char *str, CONST char *pattern)
}
declare 246 generic {
    int Tcl_Tell(Tcl_Channel chan)
}
declare 247 generic {
    int Tcl_TraceVar(Tcl_Interp *interp, char *varName, int flags, \
	    Tcl_VarTraceProc *proc, ClientData clientData)
}
declare 248 generic {
    int Tcl_TraceVar2(Tcl_Interp *interp, char *part1, char *part2, \
	    int flags, Tcl_VarTraceProc *proc, ClientData clientData)
}
declare 249 generic {
    char * Tcl_TranslateFileName(Tcl_Interp *interp, char *name, \
	    Tcl_DString *bufferPtr)
}
declare 250 generic {
    int Tcl_Ungets(Tcl_Channel chan, char *str, int len, int atHead)
}
declare 251 generic {
    void Tcl_UnlinkVar(Tcl_Interp *interp, char *varName)
}
declare 252 generic {
    int Tcl_UnregisterChannel(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 253 generic {
    int Tcl_UnsetVar(Tcl_Interp *interp, char *varName, int flags)
}
declare 254 generic {
    int Tcl_UnsetVar2(Tcl_Interp *interp, char *part1, char *part2, int flags)
}
declare 255 generic {
    void Tcl_UntraceVar(Tcl_Interp *interp, char *varName, int flags, \
	    Tcl_VarTraceProc *proc, ClientData clientData)
}
declare 256 generic {
    void Tcl_UntraceVar2(Tcl_Interp *interp, char *part1, char *part2, \
	    int flags, Tcl_VarTraceProc *proc, ClientData clientData)
}
declare 257 generic {
    void Tcl_UpdateLinkedVar(Tcl_Interp *interp, char *varName)
}
declare 258 generic {
    int Tcl_UpVar(Tcl_Interp *interp, char *frameName, char *varName, \
	    char *localName, int flags)
}
declare 259 generic {
    int Tcl_UpVar2(Tcl_Interp *interp, char *frameName, char *part1, \
	    char *part2, char *localName, int flags)
}
declare 260 generic {
    int Tcl_VarEval(Tcl_Interp *interp, ...)
}
declare 261 generic {
    ClientData Tcl_VarTraceInfo(Tcl_Interp *interp, char *varName, \
	    int flags, Tcl_VarTraceProc *procPtr, ClientData prevClientData)
}
declare 262 generic {
    ClientData Tcl_VarTraceInfo2(Tcl_Interp *interp, char *part1, \
	    char *part2, int flags, Tcl_VarTraceProc *procPtr, \
	    ClientData prevClientData)
}
declare 263 generic {
    int Tcl_Write(Tcl_Channel chan, char *s, int slen)
}
declare 264 generic {
    void Tcl_WrongNumArgs(Tcl_Interp *interp, int objc, \
	    Tcl_Obj *CONST objv[], char *message)
}
declare 265 generic {
    int Tcl_DumpActiveMemory(char *fileName)
}
declare 266 generic {
    void Tcl_ValidateAllMemory(char *file, int line)
}
declare 267 generic {
    void Tcl_AppendResultVA(Tcl_Interp *interp, va_list argList)
}
declare 268 generic {
    void Tcl_AppendStringsToObjVA(Tcl_Obj *objPtr, va_list argList)
}
declare 269 generic {
    char * Tcl_HashStats(Tcl_HashTable *tablePtr)
}
declare 270 generic {
    char * Tcl_ParseVar(Tcl_Interp *interp, char *str, char **termPtr)
}
declare 271 generic {
    char * Tcl_PkgPresent(Tcl_Interp *interp, char *name, char *version, \
	    int exact)
}
declare 272 generic {
    char * Tcl_PkgPresentEx(Tcl_Interp *interp, char *name, char *version, \
	    int exact, ClientData *clientDataPtr)
}
declare 273 generic {
    int Tcl_PkgProvide(Tcl_Interp *interp, char *name, char *version)
}
declare 274 generic {
    char * Tcl_PkgRequire(Tcl_Interp *interp, char *name, char *version, \
	    int exact)
}
declare 275 generic {
    void Tcl_SetErrorCodeVA(Tcl_Interp *interp, va_list argList)
}
declare 276 generic {
    int  Tcl_VarEvalVA(Tcl_Interp *interp, va_list argList)
}
declare 277 generic {
    Tcl_Pid Tcl_WaitPid(Tcl_Pid pid, int *statPtr, int options)
}
declare 278 {unix win} {
    void Tcl_PanicVA(char *format, va_list argList)
}
declare 279 generic {
    void Tcl_GetVersion(int *major, int *minor, int *patchLevel, int *type)
}
declare 280 generic {
    void Tcl_InitMemory(Tcl_Interp *interp)
}

# Andreas Kupries <a.kupries@westend.com>, 03/21/1999
# "Trf-Patch for filtering channels"
#
# C-Level API for (un)stacking of channels. This allows the introduction
# of filtering channels with relatively little changes to the core.
# This patch was created in cooperation with Jan Nijtmans <nijtmans@wxs.nl>
# and is therefore part of his plus-patches too.
#
# It would have been possible to place the following definitions according
# to the alphabetical order used elsewhere in this file, but I decided
# against that to ease the maintenance of the patch across new tcl versions
# (patch usually has no problems to integrate the patch file for the last
# version into the new one).

declare 281 generic {
    Tcl_Channel Tcl_StackChannel(Tcl_Interp *interp, \
	    Tcl_ChannelType *typePtr, ClientData instanceData, \
	    int mask, Tcl_Channel prevChan)
}
declare 282 generic {
    void Tcl_UnstackChannel(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 283 generic {
    Tcl_Channel Tcl_GetStackedChannel(Tcl_Channel chan)
}
# Reserved for future use (8.0.x vs. 8.1)
#  declare 284 generic {
#  }
#  declare 285 generic {
#  }


# Added in 8.1:

declare 286 generic {
    void Tcl_AppendObjToObj(Tcl_Obj *objPtr, Tcl_Obj *appendObjPtr)
}
declare 287 generic {
    Tcl_Encoding Tcl_CreateEncoding(Tcl_EncodingType *typePtr)
}
declare 288 generic {
    void Tcl_CreateThreadExitHandler(Tcl_ExitProc *proc, ClientData clientData)
}
declare 289 generic {
    void Tcl_DeleteThreadExitHandler(Tcl_ExitProc *proc, ClientData clientData)
}
declare 290 generic {
    void Tcl_DiscardResult(Tcl_SavedResult *statePtr)
}
declare 291 generic {
    int Tcl_EvalEx(Tcl_Interp *interp, char *script, int numBytes, int flags)
}
declare 292 generic {
    int Tcl_EvalObjv(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], \
	    int flags)
}
declare 293 generic {
    int Tcl_EvalObjEx(Tcl_Interp *interp, Tcl_Obj *objPtr, int flags)
}
declare 294 generic {
    void Tcl_ExitThread(int status)
}
declare 295 generic {
    int Tcl_ExternalToUtf(Tcl_Interp *interp, Tcl_Encoding encoding, \
	    CONST char *src, int srcLen, int flags, \
	    Tcl_EncodingState *statePtr, char *dst, int dstLen, \
	    int *srcReadPtr, int *dstWrotePtr, int *dstCharsPtr)
}
declare 296 generic {
    char * Tcl_ExternalToUtfDString(Tcl_Encoding encoding, CONST char *src, \
	    int srcLen, Tcl_DString *dsPtr)
}
declare 297 generic {
    void Tcl_FinalizeThread(void)
}
declare 298 generic {
    void Tcl_FinalizeNotifier(ClientData clientData)
}
declare 299 generic {
    void Tcl_FreeEncoding(Tcl_Encoding encoding)
}
declare 300 generic {
    Tcl_ThreadId Tcl_GetCurrentThread(void)
}
declare 301 generic {
    Tcl_Encoding Tcl_GetEncoding(Tcl_Interp *interp, CONST char *name)
}
declare 302 generic {
    char * Tcl_GetEncodingName(Tcl_Encoding encoding)
}
declare 303 generic {
    void Tcl_GetEncodingNames(Tcl_Interp *interp)
}
declare 304 generic {
    int Tcl_GetIndexFromObjStruct(Tcl_Interp *interp, Tcl_Obj *objPtr, \
	    char **tablePtr, int offset, char *msg, int flags, int *indexPtr)
}
declare 305 generic {
    VOID * Tcl_GetThreadData(Tcl_ThreadDataKey *keyPtr, int size)
}
declare 306 generic {
    Tcl_Obj * Tcl_GetVar2Ex(Tcl_Interp *interp, char *part1, char *part2, \
	    int flags)
}
declare 307 generic {
    ClientData Tcl_InitNotifier(void)
}
declare 308 generic {
    void Tcl_MutexLock(Tcl_Mutex *mutexPtr)
}
declare 309 generic {
    void Tcl_MutexUnlock(Tcl_Mutex *mutexPtr)
}
declare 310 generic {
    void Tcl_ConditionNotify(Tcl_Condition *condPtr)
}
declare 311 generic {
    void Tcl_ConditionWait(Tcl_Condition *condPtr, Tcl_Mutex *mutexPtr, \
	    Tcl_Time *timePtr)
}
declare 312 generic {
    int Tcl_NumUtfChars(CONST char *src, int len)
}
declare 313 generic {
    int Tcl_ReadChars(Tcl_Channel channel, Tcl_Obj *objPtr, int charsToRead, \
	    int appendFlag)
}
declare 314 generic {
    void Tcl_RestoreResult(Tcl_Interp *interp, Tcl_SavedResult *statePtr)
}
declare 315 generic {
    void Tcl_SaveResult(Tcl_Interp *interp, Tcl_SavedResult *statePtr)
}
declare 316 generic {
    int Tcl_SetSystemEncoding(Tcl_Interp *interp, CONST char *name)
}
declare 317 generic {
    Tcl_Obj * Tcl_SetVar2Ex(Tcl_Interp *interp, char *part1, char *part2, \
	    Tcl_Obj *newValuePtr, int flags)
}
declare 318 generic {
    void Tcl_ThreadAlert(Tcl_ThreadId threadId)
}
declare 319 generic {
    void Tcl_ThreadQueueEvent(Tcl_ThreadId threadId, Tcl_Event* evPtr, \
	    Tcl_QueuePosition position)
}
declare 320 generic {
    Tcl_UniChar Tcl_UniCharAtIndex(CONST char *src, int index)
}
declare 321 generic {
    Tcl_UniChar Tcl_UniCharToLower(int ch)
}
declare 322 generic {
    Tcl_UniChar Tcl_UniCharToTitle(int ch)
}
declare 323 generic {
    Tcl_UniChar Tcl_UniCharToUpper(int ch)
}
declare 324 generic {
    int Tcl_UniCharToUtf(int ch, char *buf)
}
declare 325 generic {
    char * Tcl_UtfAtIndex(CONST char *src, int index)
}
declare 326 generic {
    int Tcl_UtfCharComplete(CONST char *src, int len)
}
declare 327 generic {
    int Tcl_UtfBackslash(CONST char *src, int *readPtr, char *dst)
}
declare 328 generic {
    char * Tcl_UtfFindFirst(CONST char *src, int ch)
}
declare 329 generic {
    char * Tcl_UtfFindLast(CONST char *src, int ch)
}
declare 330 generic {
    char * Tcl_UtfNext(CONST char *src)
}
declare 331 generic {
    char * Tcl_UtfPrev(CONST char *src, CONST char *start)
}
declare 332 generic {
    int Tcl_UtfToExternal(Tcl_Interp *interp, Tcl_Encoding encoding, \
	    CONST char *src, int srcLen, int flags, \
	    Tcl_EncodingState *statePtr, char *dst, int dstLen, \
	    int *srcReadPtr, int *dstWrotePtr, int *dstCharsPtr)
}
declare 333 generic {
    char * Tcl_UtfToExternalDString(Tcl_Encoding encoding, CONST char *src, \
	    int srcLen, Tcl_DString *dsPtr)
}
declare 334 generic {
    int Tcl_UtfToLower(char *src)
}
declare 335 generic {
    int Tcl_UtfToTitle(char *src)
}
declare 336 generic {
    int Tcl_UtfToUniChar(CONST char *src, Tcl_UniChar *chPtr)
}
declare 337 generic {
    int Tcl_UtfToUpper(char *src)
}
declare 338 generic {
    int Tcl_WriteChars(Tcl_Channel chan, CONST char *src, int srcLen)
}
declare 339 generic {
    int Tcl_WriteObj(Tcl_Channel chan, Tcl_Obj *objPtr)
}
declare 340 generic {
    char * Tcl_GetString(Tcl_Obj *objPtr)
}
declare 341 generic {
    char * Tcl_GetDefaultEncodingDir(void)
}
declare 342 generic {
    void Tcl_SetDefaultEncodingDir(char *path)
}
declare 343 generic {
    void Tcl_AlertNotifier(ClientData clientData)
}
declare 344 generic {
    void Tcl_ServiceModeHook(int mode)
}
declare 345 generic {
    int Tcl_UniCharIsAlnum(int ch)
}
declare 346 generic {
    int Tcl_UniCharIsAlpha(int ch)
}
declare 347 generic {
    int Tcl_UniCharIsDigit(int ch)
}
declare 348 generic {
    int Tcl_UniCharIsLower(int ch)
}
declare 349 generic {
    int Tcl_UniCharIsSpace(int ch)
}
declare 350 generic {
    int Tcl_UniCharIsUpper(int ch)
}
declare 351 generic {
    int Tcl_UniCharIsWordChar(int ch)
}
declare 352 generic {
    int Tcl_UniCharLen(Tcl_UniChar *str)
}
declare 353 generic {
    int Tcl_UniCharNcmp(CONST Tcl_UniChar *cs, CONST Tcl_UniChar *ct,\
    unsigned long n)
}
declare 354 generic {
    char * Tcl_UniCharToUtfDString(CONST Tcl_UniChar *string, int numChars, \
 	    Tcl_DString *dsPtr)
}
declare 355 generic {
    Tcl_UniChar * Tcl_UtfToUniCharDString(CONST char *string, int length, \
	    Tcl_DString *dsPtr)
}
declare 356 generic {
    Tcl_RegExp	Tcl_GetRegExpFromObj(Tcl_Interp *interp, Tcl_Obj *patObj, int flags)
}

declare 357 generic {
    Tcl_Obj *Tcl_EvalTokens (Tcl_Interp *interp, Tcl_Token *tokenPtr, \
	    int count)
}
declare 358 generic {
    void Tcl_FreeParse (Tcl_Parse *parsePtr)
}
declare 359 generic {
    void Tcl_LogCommandInfo (Tcl_Interp *interp, char *script, \
	    char *command, int length)
}
declare 360 generic {
    int Tcl_ParseBraces (Tcl_Interp *interp, char *string, \
	    int numBytes, Tcl_Parse *parsePtr,int append, char **termPtr)
}
declare 361 generic {
    int Tcl_ParseCommand (Tcl_Interp *interp, char *string, int numBytes, \
	    int nested, Tcl_Parse *parsePtr)
}
declare 362 generic {
    int Tcl_ParseExpr(Tcl_Interp *interp, char *string, int numBytes, \
	    Tcl_Parse *parsePtr)	 
}
declare 363 generic {
    int Tcl_ParseQuotedString(Tcl_Interp *interp, char *string, int numBytes, \
	    Tcl_Parse *parsePtr, int append, char **termPtr)
}
declare 364 generic {
    int Tcl_ParseVarName (Tcl_Interp *interp, char *string, \
	    int numBytes, Tcl_Parse *parsePtr, int append)
}
declare 365 generic {
    char *Tcl_GetCwd(Tcl_Interp *interp, Tcl_DString *cwdPtr)
}
declare 366 generic {
   int Tcl_Chdir(CONST char *dirName)
}
declare 367 generic {
   int Tcl_Access(CONST char *path, int mode)
}
declare 368 generic {
    int Tcl_Stat(CONST char *path, struct stat *bufPtr)
}
declare 369 generic {
    int Tcl_UtfNcmp(CONST char *s1, CONST char *s2, unsigned long n)
}
declare 370 generic {
    int Tcl_UtfNcasecmp(CONST char *s1, CONST char *s2, unsigned long n)
}
declare 371 generic {
    int Tcl_StringCaseMatch(CONST char *str, CONST char *pattern, int nocase)
}
declare 372 generic {
    int Tcl_UniCharIsControl(int ch)
}
declare 373 generic {
    int Tcl_UniCharIsGraph(int ch)
}
declare 374 generic {
    int Tcl_UniCharIsPrint(int ch)
}
declare 375 generic {
    int Tcl_UniCharIsPunct(int ch)
}
declare 376 generic {
    int Tcl_RegExpExecObj(Tcl_Interp *interp, Tcl_RegExp regexp, \
	    Tcl_Obj *objPtr, int offset, int nmatches, int flags)
}
declare 377 generic {
    void Tcl_RegExpGetInfo(Tcl_RegExp regexp, Tcl_RegExpInfo *infoPtr)
}
declare 378 generic {
    Tcl_Obj * Tcl_NewUnicodeObj(Tcl_UniChar *unicode, int numChars)
}
declare 379 generic {
    void Tcl_SetUnicodeObj(Tcl_Obj *objPtr, Tcl_UniChar *unicode, \
	    int numChars)
}
declare 380 generic {
    int Tcl_GetCharLength (Tcl_Obj *objPtr)
}
declare 381 generic {
    Tcl_UniChar Tcl_GetUniChar (Tcl_Obj *objPtr, int index)
}
declare 382 generic {
    Tcl_UniChar * Tcl_GetUnicode (Tcl_Obj *objPtr)
}
declare 383 generic {
    Tcl_Obj * Tcl_GetRange (Tcl_Obj *objPtr, int first, int last)
}
declare 384 generic {
    void Tcl_AppendUnicodeToObj (register Tcl_Obj *objPtr, \
	    Tcl_UniChar *unicode, int length)
}
declare 385 generic {
    int Tcl_RegExpMatchObj(Tcl_Interp *interp, Tcl_Obj *stringObj, \
	    Tcl_Obj *patternObj)
}
declare 386 generic {
    void Tcl_SetNotifier(Tcl_NotifierProcs *notifierProcPtr)
}
declare 387 generic {
    Tcl_Mutex * Tcl_GetAllocMutex(void)
}
declare 388 generic {
    int Tcl_GetChannelNames(Tcl_Interp *interp)
}



##############################################################################

# Define the platform specific public Tcl interface.  These functions are
# only available on the designated platform.

interface tclPlat

######################
# Windows declarations

# Added in Tcl 8.1

declare 0 win {
    TCHAR * Tcl_WinUtfToTChar(CONST char *str, int len, Tcl_DString *dsPtr)
}
declare 1 win {
    char * Tcl_WinTCharToUtf(CONST TCHAR *str, int len, Tcl_DString *dsPtr)
}

##################
# Mac declarations

# This is needed by the shells to handle Macintosh events.
 
declare 0 mac {
    void Tcl_MacSetEventProc(Tcl_MacConvertEventPtr procPtr)
}

# These routines are useful for handling using scripts from resources 
# in the application shell

declare 1 mac {
    char * Tcl_MacConvertTextResource(Handle resource)
}
declare 2 mac {
    int Tcl_MacEvalResource(Tcl_Interp *interp, char *resourceName, \
	    int resourceNumber, char *fileName)
}
declare 3 mac {
    Handle Tcl_MacFindResource(Tcl_Interp *interp, long resourceType, \
	    char *resourceName, int resourceNumber, char *resFileRef, \
	    int * releaseIt)
}

# These routines support the new OSType object type (i.e. the packed 4
# character type and creator codes).

declare 4 mac {
    int Tcl_GetOSTypeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, \
	    OSType *osTypePtr)
}
declare 5 mac {
    void Tcl_SetOSTypeObj(Tcl_Obj *objPtr, OSType osType)
}
declare 6 mac {
    Tcl_Obj * Tcl_NewOSTypeObj(OSType osType)
}

# These are not in MSL 2.1.2, so we need to export them from the
# Tcl shared library.  They are found in the compat directory
# except the panic routine which is found in tclMacPanic.h.
 
declare 7 mac {
    int strncasecmp(CONST char *s1, CONST char *s2, size_t n)
}
declare 8 mac {
    int strcasecmp(CONST char *s1, CONST char *s2)
}
