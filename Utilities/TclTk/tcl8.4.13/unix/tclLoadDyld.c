/*
 * tclLoadDyld.c --
 *
 *  This procedure provides a version of the TclLoadFile that works with
 *  Apple's dyld dynamic loading.
 *  Original version of his file (now superseded long ago) provided by
 *  Wilfredo Sanchez (wsanchez@apple.com).
 *
 * Copyright (c) 1995 Apple Computer, Inc.
 * Copyright (c) 2005 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"
#include <mach-o/dyld.h>
#include <mach-o/fat.h>
#include <mach-o/swap.h>
#include <mach-o/arch.h>
#include <libkern/OSByteOrder.h>
#undef panic
#include <mach/mach.h>

typedef struct Tcl_DyldModuleHandle {
    struct Tcl_DyldModuleHandle *nextPtr;
    NSModule module;
} Tcl_DyldModuleHandle;

typedef struct Tcl_DyldLoadHandle {
    CONST struct mach_header *dyldLibHeader;
    Tcl_DyldModuleHandle *modulePtr;
} Tcl_DyldLoadHandle;

#ifdef TCL_LOAD_FROM_MEMORY
typedef struct ThreadSpecificData {
    int haveLoadMemory;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;
#endif

/*
 *----------------------------------------------------------------------
 *
 * DyldOFIErrorMsg --
 *
 *  Converts a numerical NSObjectFileImage error into an error message
 *  string.
 *
 * Results:
 *  Error message string.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static CONST char*
DyldOFIErrorMsg(int err) {
    switch(err) {
    case NSObjectFileImageSuccess:
  return NULL;
    case NSObjectFileImageFailure:
  return "object file setup failure";
    case NSObjectFileImageInappropriateFile:
  return "not a Mach-O MH_BUNDLE file";
    case NSObjectFileImageArch:
  return "no object for this architecture";
    case NSObjectFileImageFormat:
  return "bad object file format";
    case NSObjectFileImageAccess:
  return "can't read object file";
    default:
  return "unknown error";
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpDlopen --
 *
 *  Dynamically loads a binary code file into memory and returns a handle
 *  to the new code.
 *
 * Results:
 *  A standard Tcl completion code. If an error occurs, an error message
 *  is left in the interpreter's result.
 *
 * Side effects:
 *  New code suddenly appears in memory.
 *
 *----------------------------------------------------------------------
 */

int
TclpDlopen(interp, pathPtr, loadHandle, unloadProcPtr)
    Tcl_Interp *interp;    /* Used for error reporting. */
    Tcl_Obj *pathPtr;    /* Name of the file containing the desired
         * code (UTF-8). */
    Tcl_LoadHandle *loadHandle;  /* Filled with token for dynamically loaded
         * file which will be passed back to
         * (*unloadProcPtr)() to unload the file. */
    Tcl_FSUnloadFileProc **unloadProcPtr;
        /* Filled with address of Tcl_FSUnloadFileProc
         * function which should be used for this
         * file. */
{
    Tcl_DyldLoadHandle *dyldLoadHandle;
    CONST struct mach_header *dyldLibHeader;
    NSObjectFileImage dyldObjFileImage = NULL;
    Tcl_DyldModuleHandle *modulePtr = NULL;
    CONST char *native;

    /*
     * First try the full path the user gave us. This is particularly
     * important if the cwd is inside a vfs, and we are trying to load using a
     * relative path.
     */

    native = Tcl_FSGetNativePath(pathPtr);
    dyldLibHeader = NSAddImage(native, NSADDIMAGE_OPTION_RETURN_ON_ERROR);

    if (!dyldLibHeader) {
  NSLinkEditErrors editError;
  int errorNumber;
  CONST char *name, *msg, *objFileImageErrMsg = NULL;

  NSLinkEditError(&editError, &errorNumber, &name, &msg);

  if (editError == NSLinkEditFileAccessError) {
      /*
       * The requested file was not found. Let the OS loader examine the
       * binary search path for whatever string the user gave us which
       * hopefully refers to a file on the binary path.
       */

      Tcl_DString ds;
      char *fileName = Tcl_GetString(pathPtr);
      CONST char *native =
        Tcl_UtfToExternalDString(NULL, fileName, -1, &ds);

      dyldLibHeader = NSAddImage(native, NSADDIMAGE_OPTION_WITH_SEARCHING
        | NSADDIMAGE_OPTION_RETURN_ON_ERROR);
      Tcl_DStringFree(&ds);
      if (!dyldLibHeader) {
    NSLinkEditError(&editError, &errorNumber, &name, &msg);
      }
  } else if ((editError == NSLinkEditFileFormatError
    && errorNumber == EBADMACHO)
    || editError == NSLinkEditOtherError){
      /*
       * The requested file was found but was not of type MH_DYLIB,
       * attempt to load it as a MH_BUNDLE.
       */

      NSObjectFileImageReturnCode err =
        NSCreateObjectFileImageFromFile(native, &dyldObjFileImage);
      objFileImageErrMsg = DyldOFIErrorMsg(err);
  }

  if (!dyldLibHeader && !dyldObjFileImage) {
      Tcl_AppendResult(interp, msg, (char *) NULL);
      if (msg && *msg) {
    Tcl_AppendResult(interp, "\n", (char *) NULL);
      }
      if (objFileImageErrMsg) {
    Tcl_AppendResult(interp,
      "NSCreateObjectFileImageFromFile() error: ",
      objFileImageErrMsg, (char *) NULL);
      }
      return TCL_ERROR;
  }
    }

    if (dyldObjFileImage) {
  NSModule module;

  module = NSLinkModule(dyldObjFileImage, native,
    NSLINKMODULE_OPTION_BINDNOW
    | NSLINKMODULE_OPTION_RETURN_ON_ERROR);
  NSDestroyObjectFileImage(dyldObjFileImage);

  if (!module) {
      NSLinkEditErrors editError;
      int errorNumber;
      CONST char *name, *msg;

      NSLinkEditError(&editError, &errorNumber, &name, &msg);
      Tcl_AppendResult(interp, msg, (char *) NULL);
      return TCL_ERROR;
  }

  modulePtr = (Tcl_DyldModuleHandle *)
    ckalloc(sizeof(Tcl_DyldModuleHandle));
  modulePtr->module = module;
  modulePtr->nextPtr = NULL;
    }

    dyldLoadHandle = (Tcl_DyldLoadHandle *)
      ckalloc(sizeof(Tcl_DyldLoadHandle));
    dyldLoadHandle->dyldLibHeader = dyldLibHeader;
    dyldLoadHandle->modulePtr = modulePtr;
    *loadHandle = (Tcl_LoadHandle) dyldLoadHandle;
    *unloadProcPtr = &TclpUnloadFile;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFindSymbol --
 *
 *  Looks up a symbol, by name, through a handle associated with a
 *  previously loaded piece of code (shared library).
 *
 * Results:
 *  Returns a pointer to the function associated with 'symbol' if it is
 *  found. Otherwise returns NULL and may leave an error message in the
 *  interp's result.
 *
 *----------------------------------------------------------------------
 */

Tcl_PackageInitProc*
TclpFindSymbol(interp, loadHandle, symbol)
    Tcl_Interp *interp;    /* For error reporting. */
    Tcl_LoadHandle loadHandle;  /* Handle from TclpDlopen. */
    CONST char *symbol;    /* Symbol name to look up. */
{
    NSSymbol nsSymbol;
    CONST char *native;
    Tcl_DString newName, ds;
    Tcl_PackageInitProc *proc = NULL;
    Tcl_DyldLoadHandle *dyldLoadHandle = (Tcl_DyldLoadHandle *) loadHandle;

    /*
     * dyld adds an underscore to the beginning of symbol names.
     */

    native = Tcl_UtfToExternalDString(NULL, symbol, -1, &ds);
    Tcl_DStringInit(&newName);
    Tcl_DStringAppend(&newName, "_", 1);
    native = Tcl_DStringAppend(&newName, native, -1);

    if (dyldLoadHandle->dyldLibHeader) {
  nsSymbol = NSLookupSymbolInImage(dyldLoadHandle->dyldLibHeader, native,
    NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW |
    NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
  if (nsSymbol) {
      /*
       * Until dyld supports unloading of MY_DYLIB binaries, the
       * following is not needed.
       */

#ifdef DYLD_SUPPORTS_DYLIB_UNLOADING
      NSModule module = NSModuleForSymbol(nsSymbol);
      Tcl_DyldModuleHandle *modulePtr = dyldLoadHandle->modulePtr;

      while (modulePtr != NULL) {
    if (module == modulePtr->module) {
        break;
    }
    modulePtr = modulePtr->nextPtr;
      }
      if (modulePtr == NULL) {
    modulePtr = (Tcl_DyldModuleHandle *)
      ckalloc(sizeof(Tcl_DyldModuleHandle));
    modulePtr->module = module;
    modulePtr->nextPtr = dyldLoadHandle->modulePtr;
    dyldLoadHandle->modulePtr = modulePtr;
      }
#endif /* DYLD_SUPPORTS_DYLIB_UNLOADING */

  } else {
      NSLinkEditErrors editError;
      int errorNumber;
      CONST char *name, *msg;

      NSLinkEditError(&editError, &errorNumber, &name, &msg);
      Tcl_AppendResult(interp, msg, (char *) NULL);
  }
    } else {
  nsSymbol = NSLookupSymbolInModule(dyldLoadHandle->modulePtr->module,
    native);
    }
    if (nsSymbol) {
  proc = NSAddressOfSymbol(nsSymbol);
    }
    Tcl_DStringFree(&newName);
    Tcl_DStringFree(&ds);

    return proc;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpUnloadFile --
 *
 *  Unloads a dynamically loaded binary code file from memory. Code
 *  pointers in the formerly loaded file are no longer valid after calling
 *  this function.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Code dissapears from memory. Note that dyld currently only supports
 *  unloading of binaries of type MH_BUNDLE loaded with NSLinkModule() in
 *  TclpDlopen() above.
 *
 *----------------------------------------------------------------------
 */

void
TclpUnloadFile(loadHandle)
    Tcl_LoadHandle loadHandle;  /* loadHandle returned by a previous call to
         * TclpDlopen(). The loadHandle is a token
         * that represents the loaded file. */
{
    Tcl_DyldLoadHandle *dyldLoadHandle = (Tcl_DyldLoadHandle *) loadHandle;
    Tcl_DyldModuleHandle *modulePtr = dyldLoadHandle->modulePtr;

    while (modulePtr != NULL) {
  void *ptr;

  NSUnLinkModule(modulePtr->module,
    NSUNLINKMODULE_OPTION_RESET_LAZY_REFERENCES);
  ptr = modulePtr;
  modulePtr = modulePtr->nextPtr;
  ckfree(ptr);
    }
    ckfree((char*) dyldLoadHandle);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGuessPackageName --
 *
 *  If the "load" command is invoked without providing a package name,
 *  this procedure is invoked to try to figure it out.
 *
 * Results:
 *  Always returns 0 to indicate that we couldn't figure out a package
 *  name; generic code will then try to guess the package from the file
 *  name. A return value of 1 would have meant that we figured out the
 *  package name and put it in bufPtr.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

int
TclGuessPackageName(fileName, bufPtr)
    CONST char *fileName;  /* Name of file containing package (already
         * translated to local form if needed). */
    Tcl_DString *bufPtr;  /* Initialized empty dstring. Append package
         * name to this if possible. */
{
    return 0;
}

#ifdef TCL_LOAD_FROM_MEMORY
/*
 *----------------------------------------------------------------------
 *
 * TclpLoadMemoryGetBuffer --
 *
 *  Allocate a buffer that can be used with TclpLoadMemory() below.
 *
 * Results:
 *  Pointer to allocated buffer or NULL if an error occurs.
 *
 * Side effects:
 *  Buffer is allocated.
 *
 *----------------------------------------------------------------------
 */

void*
TclpLoadMemoryGetBuffer(interp, size)
    Tcl_Interp *interp;    /* Used for error reporting. */
    int size;      /* Size of desired buffer. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    void *buffer = NULL;

    if (!tsdPtr->haveLoadMemory) {
  /*
   * NSCreateObjectFileImageFromMemory is available but always fails
   * prior to Darwin 7.
   */

  struct utsname name;

  if (!uname(&name)) {
      long release = strtol(name.release, NULL, 10);
      tsdPtr->haveLoadMemory = (release >= 7) ? 1 : -1;
  }
    }
    if (tsdPtr->haveLoadMemory > 0) {
  /*
   * We must allocate the buffer using vm_allocate, because
   * NSCreateObjectFileImageFromMemory will dispose of it using
   * vm_deallocate.
   */

  if (vm_allocate(mach_task_self(), (vm_address_t *) &buffer, size, 1)) {
      buffer = NULL;
  }
    }
    return buffer;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpLoadMemory --
 *
 *  Dynamically loads binary code file from memory and returns a handle to
 *  the new code.
 *
 * Results:
 *  A standard Tcl completion code. If an error occurs, an error message
 *  is left in the interpreter's result.
 *
 * Side effects:
 *  New code is loaded from memory.
 *
 *----------------------------------------------------------------------
 */

int
TclpLoadMemory(interp, buffer, size, codeSize, loadHandle, unloadProcPtr)
    Tcl_Interp *interp;    /* Used for error reporting. */
    void *buffer;    /* Buffer containing the desired code
         * (allocated with TclpLoadMemoryGetBuffer). */
    int size;      /* Allocation size of buffer. */
    int codeSize;    /* Size of code data read into buffer or -1 if
         * an error occurred and the buffer should
         * just be freed. */
    Tcl_LoadHandle *loadHandle;  /* Filled with token for dynamically loaded
         * file which will be passed back to
         * (*unloadProcPtr)() to unload the file. */
    Tcl_FSUnloadFileProc **unloadProcPtr;
        /* Filled with address of Tcl_FSUnloadFileProc
         * function which should be used for this
         * file. */
{
    Tcl_DyldLoadHandle *dyldLoadHandle;
    NSObjectFileImage dyldObjFileImage = NULL;
    Tcl_DyldModuleHandle *modulePtr;
    NSModule module;
    CONST char *objFileImageErrMsg = NULL;

    /*
     * Try to create an object file image that we can load from.
     */

    if (codeSize >= 0) {
  NSObjectFileImageReturnCode err = NSObjectFileImageSuccess;
  CONST struct fat_header *fh = buffer;
  uint32_t ms = 0;
#ifndef __LP64__
  CONST struct mach_header *mh = NULL;
  #define mh_magic OSSwapHostToBigInt32(MH_MAGIC)
  #define mh_size  sizeof(struct mach_header)
#else
  CONST struct mach_header_64 *mh = NULL;
  #define mh_magic OSSwapHostToBigInt32(MH_MAGIC_64)
  #define mh_size  sizeof(struct mach_header_64)
#endif
  
  if (codeSize >= sizeof(struct fat_header)
    && fh->magic == OSSwapHostToBigInt32(FAT_MAGIC)) {
      /*
       * Fat binary, try to find mach_header for our architecture
       */
      uint32_t fh_nfat_arch = OSSwapBigToHostInt32(fh->nfat_arch);
      
      if (codeSize >= sizeof(struct fat_header) + 
        fh_nfat_arch * sizeof(struct fat_arch)) {
    void *fatarchs = buffer + sizeof(struct fat_header);
    CONST NXArchInfo *arch = NXGetLocalArchInfo();
    struct fat_arch *fa;
    
    if (fh->magic != FAT_MAGIC) {
        swap_fat_arch(fatarchs, fh_nfat_arch, arch->byteorder);
    }
    fa = NXFindBestFatArch(arch->cputype, arch->cpusubtype,
      fatarchs, fh_nfat_arch);
    if (fa) {
        mh = buffer + fa->offset;
        ms = fa->size;
    } else {
        err = NSObjectFileImageInappropriateFile;
    }
    if (fh->magic != FAT_MAGIC) {
        swap_fat_arch(fatarchs, fh_nfat_arch, arch->byteorder);
    }
      } else {
    err = NSObjectFileImageInappropriateFile;
      }
  } else {
      /*
       * Thin binary
       */
      mh = buffer;
      ms = codeSize;
  }
  if (ms && !(ms >= mh_size && mh->magic == mh_magic &&
     mh->filetype == OSSwapHostToBigInt32(MH_BUNDLE))) {
      err = NSObjectFileImageInappropriateFile;
  }
  if (err == NSObjectFileImageSuccess) {
      err = NSCreateObjectFileImageFromMemory(buffer, codeSize,
        &dyldObjFileImage);
  }
  objFileImageErrMsg = DyldOFIErrorMsg(err);
    }

    /*
     * If it went wrong (or we were asked to just deallocate), get rid of the
     * memory block and create an error message.
     */

    if (dyldObjFileImage == NULL) {
  vm_deallocate(mach_task_self(), (vm_address_t) buffer, size);
  if (objFileImageErrMsg != NULL) {
      Tcl_AppendResult(interp,
        "NSCreateObjectFileImageFromMemory() error: ",
        objFileImageErrMsg, (char *) NULL);
  }
  return TCL_ERROR;
    }

    /*
     * Extract the module we want from the image of the object file.
     */

    module = NSLinkModule(dyldObjFileImage, "[Memory Based Bundle]",
      NSLINKMODULE_OPTION_BINDNOW | NSLINKMODULE_OPTION_RETURN_ON_ERROR);
    NSDestroyObjectFileImage(dyldObjFileImage);

    if (!module) {
  NSLinkEditErrors editError;
  int errorNumber;
  CONST char *name, *msg;

  NSLinkEditError(&editError, &errorNumber, &name, &msg);
  Tcl_AppendResult(interp, msg, (char *) NULL);
  return TCL_ERROR;
    }

    /*
     * Stash the module reference within the load handle we create and return.
     */

    modulePtr = (Tcl_DyldModuleHandle *) ckalloc(sizeof(Tcl_DyldModuleHandle));
    modulePtr->module = module;
    modulePtr->nextPtr = NULL;

    dyldLoadHandle = (Tcl_DyldLoadHandle *)
      ckalloc(sizeof(Tcl_DyldLoadHandle));
    dyldLoadHandle->dyldLibHeader = NULL;
    dyldLoadHandle->modulePtr = modulePtr;
    *loadHandle = (Tcl_LoadHandle) dyldLoadHandle;
    *unloadProcPtr = &TclpUnloadFile;
    return TCL_OK;
}
#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
