/* 
 * tclMacTime.c --
 *
 *	Contains Macintosh specific versions of Tcl functions that
 *	obtain time values from the operating system.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"
#include <OSUtils.h>
#include <Timer.h>
#include <time.h>

/*
 * Static variables used by the TclpGetTime function.
 */
 
static int initalized = false;
static unsigned long baseSeconds;
static UnsignedWide microOffset;

/*
 * Prototypes for procedures that are private to this file:
 */

static void SubtractUnsignedWide _ANSI_ARGS_((UnsignedWide *x,
	UnsignedWide *y, UnsignedWide *result));

/*
 *-----------------------------------------------------------------------------
 *
 * TclpGetSeconds --
 *
 *	This procedure returns the number of seconds from the epoch.  On
 *	the Macintosh the epoch is Midnight Jan 1, 1904.  Unfortunatly,
 *	the Macintosh doesn't tie the epoch to a particular time zone.  For
 *	Tcl we tie the epoch to GMT.  This makes the time zone date parsing
 *	code work.  The epoch for Mac-Tcl is: Midnight Jan 1, 1904 GMT.
 *
 * Results:
 *	Number of seconds from the epoch in GMT.
 *
 * Side effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

unsigned long
TclpGetSeconds()
{
    unsigned long seconds;
    MachineLocation loc;
    long int offset;
    
    ReadLocation(&loc);
    offset = loc.u.gmtDelta & 0x00ffffff;
    if (offset & 0x00800000) {
	offset = offset | 0xff000000;
    }

    if (ReadDateTime(&seconds) == noErr) {
	return (seconds - offset);
    } else {
	panic("Can't get time.");
	return 0;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * TclpGetClicks --
 *
 *	This procedure returns a value that represents the highest resolution
 *	clock available on the system.  There are no garantees on what the
 *	resolution will be.  In Tcl we will call this value a "click".  The
 *	start time is also system dependant.
 *
 * Results:
 *	Number of clicks from some start time.
 *
 * Side effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

unsigned long
TclpGetClicks()
{
    UnsignedWide micros;

    Microseconds(&micros);
    return micros.lo;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetTimeZone --
 *
 *	Get the current time zone.
 *
 * Results:
 *	The return value is the local time zone, measured in
 *	minutes away from GMT (-ve for east, +ve for west).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclpGetTimeZone (
    unsigned long  currentTime)		/* Ignored on Mac. */
{
    MachineLocation loc;
    long int offset;

    ReadLocation(&loc);
    offset = loc.u.gmtDelta & 0x00ffffff;
    if (offset & 0x00700000) {
	offset |= 0xff000000;
    }

    /*
     * Convert the Mac offset from seconds to minutes and
     * add an hour if we have daylight savings time.
     */
    offset = -offset;
    offset /= 60;
    if (loc.u.dlsDelta < 0) {
	offset += 60;
    }
    
    return offset;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetTime --
 *
 *	Gets the current system time in seconds and microseconds
 *	since the beginning of the epoch: 00:00 UCT, January 1, 1970.
 *
 * Results:
 *	Returns the current time (in the local timezone) in timePtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TclpGetTime(
    Tcl_Time *timePtr)		/* Location to store time information. */
{
    UnsignedWide micro;
#ifndef NO_LONG_LONG
    long long *microPtr;
#endif
	
    if (initalized == false) {
        MachineLocation loc;
        long int offset;
    
        ReadLocation(&loc);
        offset = loc.u.gmtDelta & 0x00ffffff;
        if (offset & 0x00800000) {
            offset = offset | 0xff000000;
	}
	if (ReadDateTime(&baseSeconds) != noErr) {
	    /*
	     * This should never happen!
	     */
	    return;
	}
	/*
	 * Remove the local offset that ReadDateTime() adds.
	 */
	baseSeconds -= offset;
	Microseconds(&microOffset);
	initalized = true;
    }

    Microseconds(&micro);

#ifndef NO_LONG_LONG
    microPtr = (long long *) &micro;
    *microPtr -= *((long long *) &microOffset);
    timePtr->sec = baseSeconds + (*microPtr / 1000000);
    timePtr->usec = *microPtr % 1000000;
#else
    SubtractUnsignedWide(&micro, &microOffset, &micro);

    /*
     * This lovely computation is equal to: base + (micro / 1000000)
     * For the .hi part the ratio of 0x100000000 / 1000000 has been
     * reduced to avoid overflow.  This computation certainly has 
     * problems as the .hi part gets large.  However, your application
     * would have to run for a long time to make that happen.
     */

    timePtr->sec = baseSeconds + (micro.lo / 1000000) + 
    	(long) (micro.hi * ((double) 33554432.0 / 15625.0));
    timePtr->usec = micro.lo % 1000000;
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetDate --
 *
 *	Converts raw seconds to a struct tm data structure.  The
 *	returned time will be for Greenwich Mean Time if the useGMT flag 
 *	is set.  Otherwise, the returned time will be for the local
 *	time zone.  This function is meant to be used as a replacement
 *	for localtime and gmtime which is broken on most ANSI libs
 *	on the Macintosh.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *  	The passed in struct tm data structure is modified.
 *
 *----------------------------------------------------------------------
 */

struct tm *
TclpGetDate(
    TclpTime_t time,	/* Time struct to fill. */
    int useGMT)		/* True if date should reflect GNT time. */
{
    const time_t *tp = (const time_t *)time;
    DateTimeRec dtr;
    MachineLocation loc;
    long int offset;
    static struct tm statictime;
    static const short monthday[12] =
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    ReadLocation(&loc);
	
    if (useGMT) {
	SecondsToDate(*tp, &dtr);
    } else {
	offset = loc.u.gmtDelta & 0x00ffffff;
	if (offset & 0x00700000) {
	    offset |= 0xff000000;
	}
	
	SecondsToDate(*tp + offset, &dtr);
    }

    statictime.tm_sec = dtr.second;
    statictime.tm_min = dtr.minute;
    statictime.tm_hour = dtr.hour;
    statictime.tm_mday = dtr.day;
    statictime.tm_mon = dtr.month - 1;
    statictime.tm_year = dtr.year - 1900;
    statictime.tm_wday = dtr.dayOfWeek - 1;
    statictime.tm_yday = monthday[statictime.tm_mon]
	+ statictime.tm_mday - 1;
    if (1 < statictime.tm_mon && !(statictime.tm_year & 3)) {
	++statictime.tm_yday;
    }
    statictime.tm_isdst = loc.u.dlsDelta;
    return(&statictime);
}

#ifdef NO_LONG_LONG
/*
 *----------------------------------------------------------------------
 *
 * SubtractUnsignedWide --
 *
 *	Subtracts one UnsignedWide value from another.
 *
 * Results:
 *  	The subtracted value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
SubtractUnsignedWide(
    UnsignedWide *x,		/* Ptr to wide int. */
    UnsignedWide *y,		/* Ptr to wide int. */
    UnsignedWide *result)	/* Ptr to result. */
{
    result->hi = x->hi - y->hi;
    if (x->lo < y->lo) {
	result->hi--;
    }
    result->lo = x->lo - y->lo;
}
#endif
