/* 
 * tclWinTime.c --
 *
 *	Contains Windows specific versions of Tcl functions that
 *	obtain time values from the operating system.
 *
 * Copyright 1995-1998 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclWinInt.h"

#define SECSPERDAY (60L * 60L * 24L)
#define SECSPERYEAR (SECSPERDAY * 365L)
#define SECSPER4YEAR (SECSPERYEAR * 4L + SECSPERDAY)

/*
 * The following arrays contain the day of year for the last day of
 * each month, where index 1 is January.
 */

static int normalDays[] = {
    -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364
};

static int leapDays[] = {
    -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

typedef struct ThreadSpecificData {
    char tzName[64];		/* Time zone name */
    struct tm tm;		/* time information */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Declarations for functions defined later in this file.
 */

static struct tm *	ComputeGMT _ANSI_ARGS_((const time_t *tp));

/*
 *----------------------------------------------------------------------
 *
 * TclpGetSeconds --
 *
 *	This procedure returns the number of seconds from the epoch.
 *	On most Unix systems the epoch is Midnight Jan 1, 1970 GMT.
 *
 * Results:
 *	Number of seconds from the epoch.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned long
TclpGetSeconds()
{
    return (unsigned long) time((time_t *) NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetClicks --
 *
 *	This procedure returns a value that represents the highest
 *	resolution clock available on the system.  There are no
 *	guarantees on what the resolution will be.  In Tcl we will
 *	call this value a "click".  The start time is also system
 *	dependant.
 *
 * Results:
 *	Number of clicks from some start time.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned long
TclpGetClicks()
{
    return GetTickCount();
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetTimeZone --
 *
 *	Determines the current timezone.  The method varies wildly
 *	between different Platform implementations, so its hidden in
 *	this function.
 *
 * Results:
 *	Minutes west of GMT.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclpGetTimeZone (currentTime)
    unsigned long  currentTime;
{
    int timeZone;

    tzset();
    timeZone = _timezone / 60;

    return timeZone;
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
 *	Returns the current time in timePtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TclpGetTime(timePtr)
    Tcl_Time *timePtr;		/* Location to store time information. */
{
    struct timeb t;

    ftime(&t);
    timePtr->sec = t.time;
    timePtr->usec = t.millitm * 1000;
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
    int len;
    char *zone, *p;
    TIME_ZONE_INFORMATION tz;
    Tcl_Encoding encoding;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    char *name = tsdPtr->tzName;

    /*
     * tzset() under Borland doesn't seem to set up tzname[] at all.
     * tzset() under MSVC has the following weird observed behavior:
     *	 First time we call "clock format [clock seconds] -format %Z -gmt 1"
     *	 we get "GMT", but on all subsequent calls we get the current time 
     *	 zone string, even though env(TZ) is GMT and the variable _timezone 
     *   is 0.
     */

    name[0] = '\0';

    zone = getenv("TZ");
    if (zone != NULL) {
	/*
	 * TZ is of form "NST-4:30NDT", where "NST" would be the
	 * name of the standard time zone for this area, "-4:30" is
	 * the offset from GMT in hours, and "NDT is the name of 
	 * the daylight savings time zone in this area.  The offset 
	 * and DST strings are optional.
	 */

	len = strlen(zone);
	if (len > 3) {
	    len = 3;
	}
	if (dst != 0) {
	    /*
	     * Skip the offset string and get the DST string.
	     */

	    p = zone + len;
	    p += strspn(p, "+-:0123456789");
	    if (*p != '\0') {
		zone = p;
		len = strlen(zone);
		if (len > 3) {
		    len = 3;
		}
	    }
	}
	Tcl_ExternalToUtf(NULL, NULL, zone, len, 0, NULL, name,
		sizeof(tsdPtr->tzName), NULL, NULL, NULL);
    }
    if ((name[0] == '\0') 
	    && (GetTimeZoneInformation(&tz) != TIME_ZONE_ID_UNKNOWN)) {
	encoding = Tcl_GetEncoding(NULL, "unicode");
	Tcl_ExternalToUtf(NULL, encoding, 
		(char *) ((dst) ? tz.DaylightName : tz.StandardName), -1, 
		0, NULL, name, sizeof(tsdPtr->tzName), NULL, NULL, NULL);
	Tcl_FreeEncoding(encoding);
    } 
    if (name[0] == '\0') {
	return "%Z";
    }
    return name;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetDate --
 *
 *	This function converts between seconds and struct tm.  If
 *	useGMT is true, then the returned date will be in Greenwich
 *	Mean Time (GMT).  Otherwise, it will be in the local time zone.
 *
 * Results:
 *	Returns a static tm structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

struct tm *
TclpGetDate(t, useGMT)
    TclpTime_t t;
    int useGMT;
{
    const time_t *tp = (const time_t *) t;
    struct tm *tmPtr;
    long time;

    if (!useGMT) {
	tzset();

	/*
	 * If we are in the valid range, let the C run-time library
	 * handle it.  Otherwise we need to fake it.  Note that this
	 * algorithm ignores daylight savings time before the epoch.
	 */

	if (*tp >= 0) {
	    return localtime(tp);
	}

	time = *tp - _timezone;
	
	/*
	 * If we aren't near to overflowing the long, just add the bias and
	 * use the normal calculation.  Otherwise we will need to adjust
	 * the result at the end.
	 */

	if (*tp < (LONG_MAX - 2 * SECSPERDAY)
		&& *tp > (LONG_MIN + 2 * SECSPERDAY)) {
	    tmPtr = ComputeGMT(&time);
	} else {
	    tmPtr = ComputeGMT(tp);

	    tzset();

	    /*
	     * Add the bias directly to the tm structure to avoid overflow.
	     * Propagate seconds overflow into minutes, hours and days.
	     */

	    time = tmPtr->tm_sec - _timezone;
	    tmPtr->tm_sec = (int)(time % 60);
	    if (tmPtr->tm_sec < 0) {
		tmPtr->tm_sec += 60;
		time -= 60;
	    }
    
	    time = tmPtr->tm_min + time/60;
	    tmPtr->tm_min = (int)(time % 60);
	    if (tmPtr->tm_min < 0) {
		tmPtr->tm_min += 60;
		time -= 60;
	    }

	    time = tmPtr->tm_hour + time/60;
	    tmPtr->tm_hour = (int)(time % 24);
	    if (tmPtr->tm_hour < 0) {
		tmPtr->tm_hour += 24;
		time -= 24;
	    }

	    time /= 24;
	    tmPtr->tm_mday += time;
	    tmPtr->tm_yday += time;
	    tmPtr->tm_wday = (tmPtr->tm_wday + time) % 7;
	}
    } else {
	tmPtr = ComputeGMT(tp);
    }
    return tmPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeGMT --
 *
 *	This function computes GMT given the number of seconds since
 *	the epoch (midnight Jan 1 1970).
 *
 * Results:
 *	Returns a (per thread) statically allocated struct tm.
 *
 * Side effects:
 *	Updates the values of the static struct tm.
 *
 *----------------------------------------------------------------------
 */

static struct tm *
ComputeGMT(tp)
    const time_t *tp;
{
    struct tm *tmPtr;
    long tmp, rem;
    int isLeap;
    int *days;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

    tmPtr = &tsdPtr->tm;

    /*
     * Compute the 4 year span containing the specified time.
     */

    tmp = *tp / SECSPER4YEAR;
    rem = *tp % SECSPER4YEAR;

    /*
     * Correct for weird mod semantics so the remainder is always positive.
     */

    if (rem < 0) {
	tmp--;
	rem += SECSPER4YEAR;
    }

    /*
     * Compute the year after 1900 by taking the 4 year span and adjusting
     * for the remainder.  This works because 2000 is a leap year, and
     * 1900/2100 are out of the range.
     */

    tmp = (tmp * 4) + 70;
    isLeap = 0;
    if (rem >= SECSPERYEAR) {			  /* 1971, etc. */
	tmp++;
	rem -= SECSPERYEAR;
	if (rem >= SECSPERYEAR) {		  /* 1972, etc. */
	    tmp++;
	    rem -= SECSPERYEAR;
	    if (rem >= SECSPERYEAR + SECSPERDAY) { /* 1973, etc. */
		tmp++;
		rem -= SECSPERYEAR + SECSPERDAY;
	    } else {
		isLeap = 1;
	    }
	}
    }
    tmPtr->tm_year = tmp;

    /*
     * Compute the day of year and leave the seconds in the current day in
     * the remainder.
     */

    tmPtr->tm_yday = rem / SECSPERDAY;
    rem %= SECSPERDAY;
    
    /*
     * Compute the time of day.
     */

    tmPtr->tm_hour = rem / 3600;
    rem %= 3600;
    tmPtr->tm_min = rem / 60;
    tmPtr->tm_sec = rem % 60;

    /*
     * Compute the month and day of month.
     */

    days = (isLeap) ? leapDays : normalDays;
    for (tmp = 1; days[tmp] < tmPtr->tm_yday; tmp++) {
    }
    tmPtr->tm_mon = --tmp;
    tmPtr->tm_mday = tmPtr->tm_yday - days[tmp];

    /*
     * Compute day of week.  Epoch started on a Thursday.
     */

    tmPtr->tm_wday = (*tp / SECSPERDAY) + 4;
    if ((*tp % SECSPERDAY) < 0) {
	tmPtr->tm_wday--;
    }
    tmPtr->tm_wday %= 7;
    if (tmPtr->tm_wday < 0) {
	tmPtr->tm_wday += 7;
    }

    return tmPtr;
}
