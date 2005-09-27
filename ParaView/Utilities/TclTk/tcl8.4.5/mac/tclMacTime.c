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
#include "tclMacInt.h"
#include <OSUtils.h>
#include <Timer.h>
#include <time.h>

/*
 * Static variables used by the Tcl_GetTime function.
 */
 
static int initalized = false;
static unsigned long baseSeconds;
static UnsignedWide microOffset;

static int gmt_initialized = false;
static long gmt_offset;
static int gmt_isdst;
TCL_DECLARE_MUTEX(gmtMutex)

static int gmt_lastGetDateUseGMT = 0;

typedef struct _TABLE {
    char        *name;
    int         type;
    time_t      value;
} TABLE;


#define HOUR(x)         ((time_t) (3600 * x))

#define tZONE 0
#define tDAYZONE 1


/*
 * inverse timezone table, adapted from tclDate.c by removing duplicates and
 * adding some made up names for unusual daylight savings
 */
static TABLE    invTimezoneTable[] = {
    { "Z",    -1,     HOUR( 36) },      /* Unknown */
    { "GMT",    tZONE,     HOUR( 0) },      /* Greenwich Mean */
    { "BST",    tDAYZONE,  HOUR( 0) },      /* British Summer */
    { "WAT",    tZONE,     HOUR( 1) },      /* West Africa */
    { "WADST",  tDAYZONE,  HOUR( 1) },      /* West Africa Daylight*/
    { "AT",     tZONE,     HOUR( 2) },      /* Azores Daylight*/
    { "ADST",   tDAYZONE,  HOUR( 2) },      /* Azores */
    { "NFT",    tZONE,     HOUR( 7/2) },    /* Newfoundland */
    { "NDT",    tDAYZONE,  HOUR( 7/2) },    /* Newfoundland Daylight */
    { "AST",    tZONE,     HOUR( 4) },      /* Atlantic Standard */
    { "ADT",    tDAYZONE,  HOUR( 4) },      /* Atlantic Daylight */
    { "EST",    tZONE,     HOUR( 5) },      /* Eastern Standard */
    { "EDT",    tDAYZONE,  HOUR( 5) },      /* Eastern Daylight */
    { "CST",    tZONE,     HOUR( 6) },      /* Central Standard */
    { "CDT",    tDAYZONE,  HOUR( 6) },      /* Central Daylight */
    { "MST",    tZONE,     HOUR( 7) },      /* Mountain Standard */
    { "MDT",    tDAYZONE,  HOUR( 7) },      /* Mountain Daylight */
    { "PST",    tZONE,     HOUR( 8) },      /* Pacific Standard */
    { "PDT",    tDAYZONE,  HOUR( 8) },      /* Pacific Daylight */
    { "YST",    tZONE,     HOUR( 9) },      /* Yukon Standard */
    { "YDT",    tDAYZONE,  HOUR( 9) },      /* Yukon Daylight */
    { "HST",    tZONE,     HOUR(10) },      /* Hawaii Standard */
    { "HDT",    tDAYZONE,  HOUR(10) },      /* Hawaii Daylight */
    { "NT",     tZONE,     HOUR(11) },      /* Nome */
    { "NST",    tDAYZONE,  HOUR(11) },      /* Nome Daylight*/
    { "IDLW",   tZONE,     HOUR(12) },      /* International Date Line West */
    { "CET",    tZONE,    -HOUR( 1) },      /* Central European */
    { "CEST",   tDAYZONE, -HOUR( 1) },      /* Central European Summer */
    { "EET",    tZONE,    -HOUR( 2) },      /* Eastern Europe, USSR Zone 1 */
    { "EEST",   tDAYZONE, -HOUR( 2) },      /* Eastern Europe, USSR Zone 1 Daylight*/
    { "BT",     tZONE,    -HOUR( 3) },      /* Baghdad, USSR Zone 2 */
    { "BDST",   tDAYZONE, -HOUR( 3) },      /* Baghdad, USSR Zone 2 Daylight*/
    { "IT",     tZONE,    -HOUR( 7/2) },    /* Iran */
    { "IDST",   tDAYZONE, -HOUR( 7/2) },    /* Iran Daylight*/
    { "ZP4",    tZONE,    -HOUR( 4) },      /* USSR Zone 3 */
    { "ZP4S",   tDAYZONE, -HOUR( 4) },      /* USSR Zone 3 */
    { "ZP5",    tZONE,    -HOUR( 5) },      /* USSR Zone 4 */
    { "ZP5S",   tDAYZONE, -HOUR( 5) },      /* USSR Zone 4 */
    { "IST",    tZONE,    -HOUR(11/2) },    /* Indian Standard */
    { "ISDST",  tDAYZONE, -HOUR(11/2) },    /* Indian Standard */
    { "ZP6",    tZONE,    -HOUR( 6) },      /* USSR Zone 5 */
    { "ZP6S",   tDAYZONE, -HOUR( 6) },      /* USSR Zone 5 */
    { "WAST",   tZONE,    -HOUR( 7) },      /* West Australian Standard */
    { "WADT",   tDAYZONE, -HOUR( 7) },      /* West Australian Daylight */
    { "JT",     tZONE,    -HOUR(15/2) },    /* Java (3pm in Cronusland!) */
    { "JDST",   tDAYZONE, -HOUR(15/2) },    /* Java (3pm in Cronusland!) */
    { "CCT",    tZONE,    -HOUR( 8) },      /* China Coast, USSR Zone 7 */
    { "CCST",   tDAYZONE, -HOUR( 8) },      /* China Coast, USSR Zone 7 */
    { "JST",    tZONE,    -HOUR( 9) },      /* Japan Standard, USSR Zone 8 */
    { "JSDST",  tDAYZONE, -HOUR( 9) },      /* Japan Standard, USSR Zone 8 */
    { "CAST",   tZONE,    -HOUR(19/2) },    /* Central Australian Standard */
    { "CADT",   tDAYZONE, -HOUR(19/2) },    /* Central Australian Daylight */
    { "EAST",   tZONE,    -HOUR(10) },      /* Eastern Australian Standard */
    { "EADT",   tDAYZONE, -HOUR(10) },      /* Eastern Australian Daylight */
    { "NZT",    tZONE,    -HOUR(12) },      /* New Zealand */
    { "NZDT",   tDAYZONE, -HOUR(12) },      /* New Zealand Daylight */
    {  NULL  }
};

/*
 * Prototypes for procedures that are private to this file:
 */

static void SubtractUnsignedWide _ANSI_ARGS_((UnsignedWide *x,
	UnsignedWide *y, UnsignedWide *result));

/*
 *-----------------------------------------------------------------------------
 *
 * TclpGetGMTOffset --
 *
 *	This procedure gets the offset seconds that needs to be _added_ to tcl time
 *  in seconds (i.e. GMT time) to get local time needed as input to various
 *  Mac OS APIs, to convert Mac OS API output to tcl time, _subtract_ this value.
 *
 * Results:
 *	Number of seconds separating GMT time and mac.
 *
 * Side effects:
 *	None.
 *
 *-----------------------------------------------------------------------------
 */

long
TclpGetGMTOffset()
{
    if (gmt_initialized == false) {
	MachineLocation loc;
	
    Tcl_MutexLock(&gmtMutex);
	ReadLocation(&loc);
	gmt_offset = loc.u.gmtDelta & 0x00ffffff;
	if (gmt_offset & 0x00800000) {
	    gmt_offset = gmt_offset | 0xff000000;
	}
	gmt_isdst=(loc.u.dlsDelta < 0);
	gmt_initialized = true;
    Tcl_MutexUnlock(&gmtMutex);
    }
	return (gmt_offset);
}

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

    GetDateTime(&seconds);
	return (seconds - TclpGetGMTOffset() + tcl_mac_epoch_offset);
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
    long offset;

    /*
     * Convert the Mac offset from seconds to minutes and
     * add an hour if we have daylight savings time.
     */
    offset = -TclpGetGMTOffset();
    offset /= 60;
    if (gmt_isdst) {
	offset += 60;
    }
    
    return offset;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetTime --
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
Tcl_GetTime(
    Tcl_Time *timePtr)		/* Location to store time information. */
{
    UnsignedWide micro;
#ifndef NO_LONG_LONG
    long long *microPtr;
#endif
	
    if (initalized == false) {
	GetDateTime(&baseSeconds);
	/*
	 * Remove the local offset that ReadDateTime() adds.
	 */
	baseSeconds -= TclpGetGMTOffset() - tcl_mac_epoch_offset;
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
    unsigned long offset=0L;
    static struct tm statictime;
    static const short monthday[12] =
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
	    
	if(useGMT)
		SecondsToDate(*tp - tcl_mac_epoch_offset, &dtr);
	else
		SecondsToDate(*tp + TclpGetGMTOffset() - tcl_mac_epoch_offset, &dtr);
	
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
    if(useGMT)
    	statictime.tm_isdst = 0;
    else
    	statictime.tm_isdst = gmt_isdst;
    gmt_lastGetDateUseGMT=useGMT; /* hack to make TclpGetTZName below work */
    return(&statictime);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetTZName --
 *
 *	Gets the current timezone string.
 *
 * Results:
 *	Returns a pointer to a static string, or NULL on failure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
TclpGetTZName(int dst)
{
    register TABLE *tp;
	long zonevalue=-TclpGetGMTOffset();
		
    if (gmt_isdst)
        zonevalue += HOUR(1);

	if(gmt_lastGetDateUseGMT) /* hack: if last TclpGetDate was called */
		zonevalue=0;          /* with useGMT==1 then we're using GMT  */

    for (tp = invTimezoneTable; tp->name; tp++) {
        if ((tp->value == zonevalue) && (tp->type == dst)) break;
    }
	if(!tp->name)
		tp = invTimezoneTable; /* default to unknown */

    return tp->name;
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
