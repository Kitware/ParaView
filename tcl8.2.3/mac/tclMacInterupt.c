/* 
 * tclMacInterupt.c --
 *
 *	This file contains routines that deal with the Macintosh's low level
 *	time manager.  This code provides a better resolution timer than what
 *	can be provided by WaitNextEvent.  
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclMacInt.h"
#include <LowMem.h>
#include <Processes.h>
#include <Timer.h>

/*
 * Data structure for timer tasks.
 */
typedef struct TMInfo {
    TMTask		tmTask;
    ProcessSerialNumber	psn;
    Point 		lastPoint;
    Point 		newPoint;
    long 		currentA5;
    long 		ourA5;
    int			installed;
} TMInfo;

/*
 * Globals used within this file.
 */
 
static TimerUPP sleepTimerProc = NULL;
static int interuptsInited = false;
static ProcessSerialNumber applicationPSN;
#define MAX_TIMER_ARRAY_SIZE 16
static TMInfo timerInfoArray[MAX_TIMER_ARRAY_SIZE];
static int topTimerElement = 0;

/*
 * Prototypes for procedures that are referenced only in this file:
 */

#if !GENERATINGCFM
static TMInfo * 	GetTMInfo(void) ONEWORDINLINE(0x2E89); /* MOVE.L A1,(SP) */
#endif
static void		SleepTimerProc _ANSI_ARGS_((void));
static pascal void	CleanUpExitProc _ANSI_ARGS_((void));
static void		InitInteruptSystem _ANSI_ARGS_((void));

/*
 *----------------------------------------------------------------------
 *
 * InitInteruptSystem --
 *
 *	Does various initialization for the functions used in this 
 *	file.  Sets up Universial Pricedure Pointers, installs a trap
 *	patch for ExitToShell, etc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Various initialization.
 *
 *----------------------------------------------------------------------
 */

void
InitInteruptSystem()
{
    int i;
    
    sleepTimerProc = NewTimerProc(SleepTimerProc);
    GetCurrentProcess(&applicationPSN);
    for (i = 0; i < MAX_TIMER_ARRAY_SIZE; i++) {
	timerInfoArray[i].installed = false;
    }
    
    /*
     * Install the ExitToShell patch.  We use this patch instead
     * of the Tcl exit mechanism because we need to ensure that
     * these routines are cleaned up even if we crash or are forced
     * to quit.  There are some circumstances when the Tcl exit
     * handlers may not fire.
     */
     
    TclMacInstallExitToShellPatch(CleanUpExitProc);
    interuptsInited = true;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMacStartTimer --
 *
 *	Install a Time Manager task to wake our process up in the
 *	future.  The process should get a NULL event after ms 
 *	milliseconds.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Schedules our process to wake up.
 *
 *----------------------------------------------------------------------
 */

void *
TclMacStartTimer(
    long ms)		/* Milliseconds. */
{
    TMInfo *timerInfoPtr;
    
    if (!interuptsInited) {
	InitInteruptSystem();
    }
    
    /*
     * Obtain a pointer for the timer.  We only allocate up
     * to MAX_TIMER_ARRAY_SIZE timers.  If we are past that
     * max we return NULL.
     */
    if (topTimerElement < MAX_TIMER_ARRAY_SIZE) {
	timerInfoPtr = &timerInfoArray[topTimerElement];
	topTimerElement++;
    } else {
	return NULL;
    }
    
    /*
     * Install timer to wake process in ms milliseconds.
     */
    timerInfoPtr->tmTask.tmAddr = sleepTimerProc;
    timerInfoPtr->tmTask.tmWakeUp = 0;
    timerInfoPtr->tmTask.tmReserved = 0;
    timerInfoPtr->psn = applicationPSN;
    timerInfoPtr->installed = true;

    InsTime((QElemPtr) timerInfoPtr);
    PrimeTime((QElemPtr) timerInfoPtr, (long) ms);

    return (void *) timerInfoPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMacRemoveTimer --
 *
 *	Remove the timer event from the Time Manager.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A scheduled timer would be removed.
 *
 *----------------------------------------------------------------------
 */

void
TclMacRemoveTimer(
    void * timerToken)		/* Token got from start timer. */
{
    TMInfo *timerInfoPtr = (TMInfo *) timerToken;
    
    if (timerInfoPtr == NULL) {
	return;
    }
    
    RmvTime((QElemPtr) timerInfoPtr);
    timerInfoPtr->installed = false;
    topTimerElement--;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMacTimerExpired --
 *
 *	Check to see if the installed timer has expired.
 *
 * Results:
 *	True if timer has expired, false otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclMacTimerExpired(
    void * timerToken)		/* Our token again. */
{
    TMInfo *timerInfoPtr = (TMInfo *) timerToken;
    
    if ((timerInfoPtr == NULL) || 
	!(timerInfoPtr->tmTask.qType & kTMTaskActive)) {
	return true;
    } else {
	return false;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SleepTimerProc --
 *
 *	Time proc is called by the is a callback routine placed in the 
 *	system by Tcl_Sleep.  The routine is called at interupt time
 *	and threrfor can not move or allocate memory.  This call will
 *	schedule our process to wake up the next time the process gets
 *	around to consider running it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Schedules our process to wake up.
 *
 *----------------------------------------------------------------------
 */

static void
SleepTimerProc()
{
    /*
     * In CFM code we can access our code directly.  In 68k code that
     * isn't based on CFM we must do a glorious hack.  The function 
     * GetTMInfo is an inline assembler call that moves the pointer 
     * at A1 to the top of the stack.  The Time Manager keeps the TMTask
     * info record there before calling this call back.  In order for
     * this to work the infoPtr argument must be the *last* item on the
     * stack.  If we "piggyback" our data to the TMTask info record we 
     * can get access to the information we need.  While this is really 
     * ugly - it's the way Apple recomends it be done - go figure...
     */
    
#if GENERATINGCFM
    WakeUpProcess(&applicationPSN);
#else
    TMInfo * infoPtr;
    
    infoPtr = GetTMInfo();
    WakeUpProcess(&infoPtr->psn);
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * CleanUpExitProc --
 *
 *	This procedure is invoked as an exit handler when ExitToShell
 *	is called.  It removes the system level timer handler if it 
 *	is installed.  This must be called or the Mac OS will more than 
 *	likely crash.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static pascal void
CleanUpExitProc()
{
    int i;
    
    for (i = 0; i < MAX_TIMER_ARRAY_SIZE; i++) {
	if (timerInfoArray[i].installed) {
	    RmvTime((QElemPtr) &timerInfoArray[i]);
	    timerInfoArray[i].installed = false;
	}
    }
}
