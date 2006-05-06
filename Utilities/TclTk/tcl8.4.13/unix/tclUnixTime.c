/* 
 * tclUnixTime.c --
 *
 *  Contains Unix specific versions of Tcl functions that
 *  obtain time values from the operating system.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"
#include <locale.h>
#define TM_YEAR_BASE 1900
#define IsLeapYear(x)   ((x % 4 == 0) && (x % 100 != 0 || x % 400 == 0))

/*
 * TclpGetDate is coded to return a pointer to a 'struct tm'.  For
 * thread safety, this structure must be in thread-specific data.
 * The 'tmKey' variable is the key to this buffer.
 */

static Tcl_ThreadDataKey tmKey;
typedef struct ThreadSpecificData {
    struct tm gmtime_buf;
    struct tm localtime_buf;
} ThreadSpecificData;

/*
 * If we fall back on the thread-unsafe versions of gmtime and localtime,
 * use this mutex to try to protect them.
 */

TCL_DECLARE_MUTEX(tmMutex)

static char* lastTZ = NULL;  /* Holds the last setting of the
         * TZ environment variable, or an
         * empty string if the variable was
         * not set. */

/* Static functions declared in this file */

static void SetTZIfNecessary _ANSI_ARGS_((void));
static void CleanupMemory _ANSI_ARGS_((ClientData));

/*
 *-----------------------------------------------------------------------------
 *
 * TclpGetSeconds --
 *
 *  This procedure returns the number of seconds from the epoch.  On
 *  most Unix systems the epoch is Midnight Jan 1, 1970 GMT.
 *
 * Results:
 *  Number of seconds from the epoch.
 *
 * Side effects:
 *  None.
 *
 *-----------------------------------------------------------------------------
 */

unsigned long
TclpGetSeconds()
{
    return time((time_t *) NULL);
}

/*
 *-----------------------------------------------------------------------------
 *
 * TclpGetClicks --
 *
 *  This procedure returns a value that represents the highest resolution
 *  clock available on the system.  There are no garantees on what the
 *  resolution will be.  In Tcl we will call this value a "click".  The
 *  start time is also system dependant.
 *
 * Results:
 *  Number of clicks from some start time.
 *
 * Side effects:
 *  None.
 *
 *-----------------------------------------------------------------------------
 */

unsigned long
TclpGetClicks()
{
    unsigned long now;
#ifdef NO_GETTOD
    struct tms dummy;
#else
    struct timeval date;
    struct timezone tz;
#endif

#ifdef NO_GETTOD
    now = (unsigned long) times(&dummy);
#else
    gettimeofday(&date, &tz);
    now = date.tv_sec*1000000 + date.tv_usec;
#endif

    return now;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetTimeZone --
 *
 *  Determines the current timezone.  The method varies wildly
 *  between different platform implementations, so its hidden in
 *  this function.
 *
 * Results:
 *  The return value is the local time zone, measured in
 *  minutes away from GMT (-ve for east, +ve for west).
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

int
TclpGetTimeZone (currentTime)
    Tcl_WideInt  currentTime;
{
    /*
     * We prefer first to use the time zone in "struct tm" if the
     * structure contains such a member.  Following that, we try
     * to locate the external 'timezone' variable and use its value.
     * If both of those methods fail, we attempt to convert a known
     * time to local time and use the difference from UTC as the local
     * time zone.  In all cases, we need to undo any Daylight Saving Time
     * adjustment.
     */
    
#if defined(HAVE_TM_TZADJ)
#   define TCL_GOT_TIMEZONE

    /* Struct tm contains tm_tzadj - that value may be used. */

    time_t      curTime = (time_t) currentTime;
    struct tm  *timeDataPtr = TclpLocaltime((TclpTime_t) &curTime);
    int         timeZone;

    timeZone = timeDataPtr->tm_tzadj  / 60;
    if (timeDataPtr->tm_isdst) {
        timeZone += 60;
    }
    
    return timeZone;

#endif

#if defined(HAVE_TM_GMTOFF) && !defined (TCL_GOT_TIMEZONE)
#   define TCL_GOT_TIMEZONE

    /* Struct tm contains tm_gmtoff - that value may be used. */

    time_t     curTime = (time_t) currentTime;
    struct tm *timeDataPtr = TclpLocaltime((TclpTime_t) &curTime);
    int        timeZone;

    timeZone = -(timeDataPtr->tm_gmtoff / 60);
    if (timeDataPtr->tm_isdst) {
        timeZone += 60;
    }
    
    return timeZone;

#endif

#if defined(HAVE_TIMEZONE_VAR) && !defined(TCL_GOT_TIMEZONE) && !defined(USE_DELTA_FOR_TZ)
#   define TCL_GOT_TIMEZONE

    int        timeZone;

    /* The 'timezone' external var is present and may be used. */

    SetTZIfNecessary();

    /*
     * Note: this is not a typo in "timezone" below!  See tzset
     * documentation for details.
     */

    timeZone = timezone / 60;
    return timeZone;

#endif

#if !defined(TCL_GOT_TIMEZONE) 
#define TCL_GOT_TIMEZONE 1
    /*
     * Fallback - determine time zone with a known reference time.
     */

    int timeZone;
    time_t tt;
    struct tm *stm;
    tt = 849268800L;      /*    1996-11-29 12:00:00  GMT */
    stm = TclpLocaltime((TclpTime_t) &tt); /* eg 1996-11-29  6:00:00  CST6CDT */
    /* The calculation below assumes a max of +12 or -12 hours from GMT */
    timeZone = (12 - stm->tm_hour)*60 + (0 - stm->tm_min);
    if ( stm -> tm_isdst ) {
        timeZone += 60;
    }
    return timeZone;  /* eg +360 for CST6CDT */
#endif

#ifndef TCL_GOT_TIMEZONE
    /*
     * Cause compile error, we don't know how to get timezone.
     */

#error autoconf did not figure out how to determine the timezone. 

#endif

}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetTime --
 *
 *  Gets the current system time in seconds and microseconds
 *  since the beginning of the epoch: 00:00 UCT, January 1, 1970.
 *
 * Results:
 *  Returns the current time in timePtr.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_GetTime(timePtr)
    Tcl_Time *timePtr;    /* Location to store time information. */
{
    struct timeval tv;
    struct timezone tz;
    
    (void) gettimeofday(&tv, &tz);
    timePtr->sec = tv.tv_sec;
    timePtr->usec = tv.tv_usec;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetDate --
 *
 *  This function converts between seconds and struct tm.  If
 *  useGMT is true, then the returned date will be in Greenwich
 *  Mean Time (GMT).  Otherwise, it will be in the local time zone.
 *
 * Results:
 *  Returns a static tm structure.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

struct tm *
TclpGetDate(time, useGMT)
    TclpTime_t time;
    int useGMT;
{
    if (useGMT) {
  return TclpGmtime(time);
    } else {
  return TclpLocaltime(time);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclpStrftime --
 *
 *  On Unix, we can safely call the native strftime implementation,
 *  and also ignore the useGMT parameter.
 *
 * Results:
 *  The normal strftime result.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

size_t
TclpStrftime(s, maxsize, format, t, useGMT)
    char *s;
    size_t maxsize;
    CONST char *format;
    CONST struct tm *t;
    int useGMT;
{
    if (format[0] == '%' && format[1] == 'Q') {
  /* Format as a stardate */
  sprintf(s, "Stardate %2d%03d.%01d",
    (((t->tm_year + TM_YEAR_BASE) + 377) - 2323),
    (((t->tm_yday + 1) * 1000) /
      (365 + IsLeapYear((t->tm_year + TM_YEAR_BASE)))),
    (((t->tm_hour * 60) + t->tm_min)/144));
  return(strlen(s));
    }
    setlocale(LC_TIME, "");
    return strftime(s, maxsize, format, t);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGmtime --
 *
 *  Wrapper around the 'gmtime' library function to make it thread
 *  safe.
 *
 * Results:
 *  Returns a pointer to a 'struct tm' in thread-specific data.
 *
 * Side effects:
 *  Invokes gmtime or gmtime_r as appropriate.
 *
 *----------------------------------------------------------------------
 */

struct tm *
TclpGmtime( tt )
    CONST TclpTime_t tt;
{
    CONST time_t *timePtr = (CONST time_t *) tt;
        /* Pointer to the number of seconds
         * since the local system's epoch */

    /*
     * Get a thread-local buffer to hold the returned time.
     */

    ThreadSpecificData *tsdPtr = TCL_TSD_INIT( &tmKey );
#ifdef HAVE_GMTIME_R
    gmtime_r(timePtr, &( tsdPtr->gmtime_buf ));
#else
    Tcl_MutexLock( &tmMutex );
    memcpy( (VOID *) &( tsdPtr->gmtime_buf ),
      (VOID *) gmtime( timePtr ),
      sizeof( struct tm ) );
    Tcl_MutexUnlock( &tmMutex );
#endif    
    return &( tsdPtr->gmtime_buf );
}
/*
 * Forwarder for obsolete item in Stubs
 */
struct tm*
TclpGmtime_unix( timePtr )
    CONST TclpTime_t timePtr;
{
    return TclpGmtime( timePtr );
}

/*
 *----------------------------------------------------------------------
 *
 * TclpLocaltime --
 *
 *  Wrapper around the 'localtime' library function to make it thread
 *  safe.
 *
 * Results:
 *  Returns a pointer to a 'struct tm' in thread-specific data.
 *
 * Side effects:
 *  Invokes localtime or localtime_r as appropriate.
 *
 *----------------------------------------------------------------------
 */

struct tm *
TclpLocaltime( tt )
    CONST TclpTime_t tt;
{
    CONST time_t *timePtr = (CONST time_t *) tt;
        /* Pointer to the number of seconds
         * since the local system's epoch */
    /*
     * Get a thread-local buffer to hold the returned time.
     */

    ThreadSpecificData *tsdPtr = TCL_TSD_INIT( &tmKey );
    SetTZIfNecessary();
#ifdef HAVE_LOCALTIME_R
    localtime_r( timePtr, &( tsdPtr->localtime_buf ) );
#else
    Tcl_MutexLock( &tmMutex );
    memcpy( (VOID *) &( tsdPtr -> localtime_buf ),
      (VOID *) localtime( timePtr ),
      sizeof( struct tm ) );
    Tcl_MutexUnlock( &tmMutex );
#endif    
    return &( tsdPtr->localtime_buf );
}
/*
 * Forwarder for obsolete item in Stubs
 */
struct tm*
TclpLocaltime_unix( timePtr )
    CONST TclpTime_t timePtr;
{
    return TclpLocaltime( timePtr );
}

/*
 *----------------------------------------------------------------------
 *
 * SetTZIfNecessary --
 *
 *  Determines whether a call to 'tzset' is needed prior to the
 *  next call to 'localtime' or examination of the 'timezone' variable.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  If 'tzset' has never been called in the current process, or if
 *  the value of the environment variable TZ has changed since the
 *  last call to 'tzset', then 'tzset' is called again.
 *
 *----------------------------------------------------------------------
 */

static void
SetTZIfNecessary() {

    CONST char* newTZ = getenv( "TZ" );
    Tcl_MutexLock(&tmMutex);
    if ( newTZ == NULL ) {
  newTZ = "";
    }
    if ( lastTZ == NULL || strcmp( lastTZ, newTZ ) ) {
        tzset();
  if ( lastTZ == NULL ) {
      Tcl_CreateExitHandler( CleanupMemory, (ClientData) NULL );
  } else {
      Tcl_Free( lastTZ );
  }
  lastTZ = Tcl_Alloc( strlen( newTZ ) + 1 );
  strcpy( lastTZ, newTZ );
    }
    Tcl_MutexUnlock(&tmMutex);

}

/*
 *----------------------------------------------------------------------
 *
 * CleanupMemory --
 *
 *  Releases the private copy of the TZ environment variable
 *  upon exit from Tcl.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Frees allocated memory.
 *
 *----------------------------------------------------------------------
 */

static void
CleanupMemory( ClientData ignored )
{
    Tcl_Free( lastTZ );
}
