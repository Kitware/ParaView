/* 
 * tkWin32Dll.c --
 *
 *  This file contains a stub dll entry point.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkWinInt.h"
#ifndef STATIC_BUILD

#ifdef HAVE_NO_SEH
/*
 * Unlike Borland and Microsoft, we don't register exception handlers by
 * pushing registration records onto the runtime stack. Instead, we register
 * them by creating an EXCEPTION_REGISTRATION within the activation record.
 */

typedef struct EXCEPTION_REGISTRATION {
    struct EXCEPTION_REGISTRATION *link;
    EXCEPTION_DISPOSITION (*handler)(
      struct _EXCEPTION_RECORD*, void*, struct _CONTEXT*, void*);
    void *ebp;
    void *esp;
    int status;
} EXCEPTION_REGISTRATION;

/* Need to add noinline flag to DllMain declaration so that gcc -O3
 * does not inline asm code into DllEntryPoint and cause a
 * compile time error because of redefined local labels.
 */

BOOL WINAPI    DllMain(HINSTANCE hInst, DWORD reason, 
        LPVOID reserved)
                        __attribute__ ((noinline));

#else

/*
 * The following declaration is for the VC++ DLL entry point.
 */

BOOL WINAPI    DllMain _ANSI_ARGS_((HINSTANCE hInst,
          DWORD reason, LPVOID reserved));
#endif /* HAVE_NO_SEH */

/*
 *----------------------------------------------------------------------
 *
 * DllEntryPoint --
 *
 *  This wrapper function is used by Borland to invoke the
 *  initialization code for Tk.  It simply calls the DllMain
 *  routine.
 *
 * Results:
 *  See DllMain.
 *
 * Side effects:
 *  See DllMain.
 *
 *----------------------------------------------------------------------
 */

BOOL WINAPI
DllEntryPoint(hInst, reason, reserved)
    HINSTANCE hInst;    /* Library instance handle. */
    DWORD reason;    /* Reason this function is being called. */
    LPVOID reserved;    /* Not used. */
{
    return DllMain(hInst, reason, reserved);
}

/*
 *----------------------------------------------------------------------
 *
 * DllMain --
 *
 *  DLL entry point.  It is only necessary to specify our dll here so
 *  that resources are found correctly.  Otherwise Tk will initialize
 *  and clean up after itself through other methods, in order to be
 *  consistent whether the build is static or dynamic.
 *
 * Results:
 *  Always TRUE.
 *
 * Side effects:
 *  This might call some sycronization functions, but MSDN
 *  documentation states: "Waiting on synchronization objects in
 *  DllMain can cause a deadlock."
 *
 *----------------------------------------------------------------------
 */

BOOL WINAPI
DllMain(hInstance, reason, reserved)
    HINSTANCE hInstance;
    DWORD reason;
    LPVOID reserved;
{
#ifdef HAVE_NO_SEH
    EXCEPTION_REGISTRATION registration;
#endif

    /*
     * If we are attaching to the DLL from a new process, tell Tk about
     * the hInstance to use.
     */

    switch (reason) {
    case DLL_PROCESS_ATTACH:
  DisableThreadLibraryCalls(hInstance);
  TkWinSetHINSTANCE(hInstance);
  break;

    case DLL_PROCESS_DETACH:
  /*
   * Protect the call to TkFinalize in an SEH block.  We can't
   * be guarenteed Tk is always being unloaded from a stable
   * condition.
   */

#ifdef HAVE_NO_SEH
  __asm__ __volatile__ (

      /*
       * Construct an EXCEPTION_REGISTRATION to protect the call to
       * TkFinalize
       */

      "leal  %[registration], %%edx"    "\n\t"
      "movl  %%fs:0,    %%eax"    "\n\t"
      "movl  %%eax,    0x0(%%edx)"  "\n\t" /* link */
      "leal  1f,    %%eax"    "\n\t"
      "movl  %%eax,    0x4(%%edx)"  "\n\t" /* handler */
      "movl  %%ebp,    0x8(%%edx)"  "\n\t" /* ebp */
      "movl  %%esp,    0xc(%%edx)"  "\n\t" /* esp */
      "movl  %[error],  0x10(%%edx)"  "\n\t" /* status */

      /*
       * Link the EXCEPTION_REGISTRATION on the chain
       */

      "movl  %%edx,    %%fs:0"    "\n\t"

      /*
       * Call TkFinalize
       */

      "movl  $0x0,    0x0(%%esp)"    "\n\t"
      "call  _TkFinalize"      "\n\t"

      /*
       * Come here on a normal exit. Recover the EXCEPTION_REGISTRATION
       * and store a TCL_OK status
       */

      "movl  %%fs:0,    %%edx"    "\n\t"
      "movl  %[ok],    %%eax"    "\n\t"
      "movl  %%eax,    0x10(%%edx)"  "\n\t"
      "jmp  2f"        "\n"

      /*
       * Come here on an exception. Get the EXCEPTION_REGISTRATION that
       * we previously put on the chain.
       */

      "1:"          "\t"
      "movl  %%fs:0,    %%edx"    "\n\t"
      "movl  0x8(%%edx),  %%edx"    "\n"


      /* 
       * Come here however we exited. Restore context from the
       * EXCEPTION_REGISTRATION in case the stack is unbalanced.
       */

      "2:"          "\t"
      "movl  0xc(%%edx),  %%esp"    "\n\t"
      "movl  0x8(%%edx),  %%ebp"    "\n\t"
      "movl  0x0(%%edx),  %%eax"    "\n\t"
      "movl  %%eax,    %%fs:0"    "\n\t"

      :
      /* No outputs */
      :
      [registration]  "m"  (registration),
      [ok]    "i"  (TCL_OK),
      [error]    "i"  (TCL_ERROR)
      :
      "%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi", "memory"
      );

#else /* HAVE_NO_SEH */
  __try {
      /*
       * Run and remove our exit handlers, if they haven't already
       * been run.  Just in case we are being unloaded prior to
       * Tcl (it can happen), we won't leave any dangling pointers
       * hanging around for when Tcl gets unloaded later.
       */

      TkFinalize(NULL);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
      /* empty handler body. */
  }
#endif

  break;
    }
    return TRUE;
}

#endif /* !STATIC_BUILD */
