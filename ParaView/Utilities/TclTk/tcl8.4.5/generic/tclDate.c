/* 
 * tclDate.c --
 *
 *	This file is generated from a yacc grammar defined in
 *	the file tclGetDate.y.  It should not be edited directly.
 *
 * Copyright (c) 1992-1995 Karl Lehenbauer and Mark Diekhans.
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"

#if defined(MAC_TCL) && !defined(TCL_MAC_USE_MSL_EPOCH)
#   define EPOCH           1904
#   define START_OF_TIME   1904
#   define END_OF_TIME     2039
#else
#   define EPOCH           1970
#   define START_OF_TIME   1902
#   define END_OF_TIME     2037
#endif

/*
 * The offset of tm_year of struct tm returned by localtime, gmtime, etc.
 * I don't know how universal this is; K&R II, the NetBSD manpages, and
 * ../compat/strftime.c all agree that tm_year is the year-1900.  However,
 * some systems may have a different value.  This #define should be the
 * same as in ../compat/strftime.c.
 */
#define TM_YEAR_BASE 1900

#define HOUR(x)         ((int) (60 * x))
#define SECSPERDAY      (24L * 60L * 60L)
#define IsLeapYear(x)   ((x % 4 == 0) && (x % 100 != 0 || x % 400 == 0))

/*
 *  An entry in the lexical lookup table.
 */
typedef struct _TABLE {
    char        *name;
    int         type;
    time_t      value;
} TABLE;


/*
 *  Daylight-savings mode:  on, off, or not yet known.
 */
typedef enum _DSTMODE {
    DSTon, DSToff, DSTmaybe
} DSTMODE;

/*
 *  Meridian:  am, pm, or 24-hour style.
 */
typedef enum _MERIDIAN {
    MERam, MERpm, MER24
} MERIDIAN;


/*
 *  Global variables.  We could get rid of most of these by using a good
 *  union as the yacc stack.  (This routine was originally written before
 *  yacc had the %union construct.)  Maybe someday; right now we only use
 *  the %union very rarely.
 */
static char     *TclDateInput;
static DSTMODE  TclDateDSTmode;
static time_t   TclDateDayOrdinal;
static time_t   TclDateDayNumber;
static time_t   TclDateMonthOrdinal;
static int      TclDateHaveDate;
static int      TclDateHaveDay;
static int      TclDateHaveOrdinalMonth;
static int      TclDateHaveRel;
static int      TclDateHaveTime;
static int      TclDateHaveZone;
static time_t   TclDateTimezone;
static time_t   TclDateDay;
static time_t   TclDateHour;
static time_t   TclDateMinutes;
static time_t   TclDateMonth;
static time_t   TclDateSeconds;
static time_t   TclDateYear;
static MERIDIAN TclDateMeridian;
static time_t   TclDateRelMonth;
static time_t   TclDateRelDay;
static time_t   TclDateRelSeconds;
static time_t  *TclDateRelPointer;

/*
 * Prototypes of internal functions.
 */
static void	TclDateerror _ANSI_ARGS_((char *s));
static time_t	ToSeconds _ANSI_ARGS_((time_t Hours, time_t Minutes,
		    time_t Seconds, MERIDIAN Meridian));
static int	Convert _ANSI_ARGS_((time_t Month, time_t Day, time_t Year,
		    time_t Hours, time_t Minutes, time_t Seconds,
		    MERIDIAN Meridia, DSTMODE DSTmode, time_t *TimePtr));
static time_t	DSTcorrect _ANSI_ARGS_((time_t Start, time_t Future));
static time_t	NamedDay _ANSI_ARGS_((time_t Start, time_t DayOrdinal,
		    time_t DayNumber));
static time_t   NamedMonth _ANSI_ARGS_((time_t Start, time_t MonthOrdinal,
                    time_t MonthNumber));
static int	RelativeMonth _ANSI_ARGS_((time_t Start, time_t RelMonth,
		    time_t *TimePtr));
static int	RelativeDay _ANSI_ARGS_((time_t Start, time_t RelDay,
		    time_t *TimePtr));
static int	LookupWord _ANSI_ARGS_((char *buff));
static int	TclDatelex _ANSI_ARGS_((void));

int
TclDateparse _ANSI_ARGS_((void));
typedef union
#ifdef __cplusplus
	YYSTYPE
#endif
 {
    time_t              Number;
    enum _MERIDIAN      Meridian;
} YYSTYPE;
# define tAGO 257
# define tDAY 258
# define tDAYZONE 259
# define tID 260
# define tMERIDIAN 261
# define tMINUTE_UNIT 262
# define tMONTH 263
# define tMONTH_UNIT 264
# define tSTARDATE 265
# define tSEC_UNIT 266
# define tSNUMBER 267
# define tUNUMBER 268
# define tZONE 269
# define tEPOCH 270
# define tDST 271
# define tISOBASE 272
# define tDAY_UNIT 273
# define tNEXT 274




#if defined(__cplusplus) || defined(__STDC__)

#if defined(__cplusplus) && defined(__EXTERN_C__)
extern "C" {
#endif
#ifndef TclDateerror
#if defined(__cplusplus)
	void TclDateerror(CONST char *);
#endif
#endif
#ifndef TclDatelex
	int TclDatelex(void);
#endif
	int TclDateparse(void);
#if defined(__cplusplus) && defined(__EXTERN_C__)
}
#endif

#endif

#define TclDateclearin TclDatechar = -1
#define TclDateerrok TclDateerrflag = 0
extern int TclDatechar;
extern int TclDateerrflag;
YYSTYPE TclDatelval;
YYSTYPE TclDateval;
typedef int TclDatetabelem;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#if YYMAXDEPTH > 0
int TclDate_TclDates[YYMAXDEPTH], *TclDates = TclDate_TclDates;
YYSTYPE TclDate_TclDatev[YYMAXDEPTH], *TclDatev = TclDate_TclDatev;
#else	/* user does initial allocation */
int *TclDates;
YYSTYPE *TclDatev;
#endif
static int TclDatemaxdepth = YYMAXDEPTH;
# define YYERRCODE 256


/*
 * Month and day table.
 */
static TABLE    MonthDayTable[] = {
    { "january",        tMONTH,  1 },
    { "february",       tMONTH,  2 },
    { "march",          tMONTH,  3 },
    { "april",          tMONTH,  4 },
    { "may",            tMONTH,  5 },
    { "june",           tMONTH,  6 },
    { "july",           tMONTH,  7 },
    { "august",         tMONTH,  8 },
    { "september",      tMONTH,  9 },
    { "sept",           tMONTH,  9 },
    { "october",        tMONTH, 10 },
    { "november",       tMONTH, 11 },
    { "december",       tMONTH, 12 },
    { "sunday",         tDAY, 0 },
    { "monday",         tDAY, 1 },
    { "tuesday",        tDAY, 2 },
    { "tues",           tDAY, 2 },
    { "wednesday",      tDAY, 3 },
    { "wednes",         tDAY, 3 },
    { "thursday",       tDAY, 4 },
    { "thur",           tDAY, 4 },
    { "thurs",          tDAY, 4 },
    { "friday",         tDAY, 5 },
    { "saturday",       tDAY, 6 },
    { NULL }
};

/*
 * Time units table.
 */
static TABLE    UnitsTable[] = {
    { "year",           tMONTH_UNIT,    12 },
    { "month",          tMONTH_UNIT,     1 },
    { "fortnight",      tDAY_UNIT,      14 },
    { "week",           tDAY_UNIT,       7 },
    { "day",            tDAY_UNIT,       1 },
    { "hour",           tSEC_UNIT, 60 * 60 },
    { "minute",         tSEC_UNIT,      60 },
    { "min",            tSEC_UNIT,      60 },
    { "second",         tSEC_UNIT,       1 },
    { "sec",            tSEC_UNIT,       1 },
    { NULL }
};

/*
 * Assorted relative-time words.
 */
static TABLE    OtherTable[] = {
    { "tomorrow",       tDAY_UNIT,      1 },
    { "yesterday",      tDAY_UNIT,     -1 },
    { "today",          tDAY_UNIT,      0 },
    { "now",            tSEC_UNIT,      0 },
    { "last",           tUNUMBER,      -1 },
    { "this",           tSEC_UNIT,      0 },
    { "next",           tNEXT,          1 },
#if 0
    { "first",          tUNUMBER,       1 },
    { "second",         tUNUMBER,       2 },
    { "third",          tUNUMBER,       3 },
    { "fourth",         tUNUMBER,       4 },
    { "fifth",          tUNUMBER,       5 },
    { "sixth",          tUNUMBER,       6 },
    { "seventh",        tUNUMBER,       7 },
    { "eighth",         tUNUMBER,       8 },
    { "ninth",          tUNUMBER,       9 },
    { "tenth",          tUNUMBER,       10 },
    { "eleventh",       tUNUMBER,       11 },
    { "twelfth",        tUNUMBER,       12 },
#endif
    { "ago",            tAGO,   1 },
    { "epoch",          tEPOCH,   0 },
    { "stardate",       tSTARDATE, 0},
    { NULL }
};

/*
 * The timezone table.  (Note: This table was modified to not use any floating
 * point constants to work around an SGI compiler bug).
 */
static TABLE    TimezoneTable[] = {
    { "gmt",    tZONE,     HOUR( 0) },      /* Greenwich Mean */
    { "ut",     tZONE,     HOUR( 0) },      /* Universal (Coordinated) */
    { "utc",    tZONE,     HOUR( 0) },
    { "uct",    tZONE,     HOUR( 0) },      /* Universal Coordinated Time */
    { "wet",    tZONE,     HOUR( 0) },      /* Western European */
    { "bst",    tDAYZONE,  HOUR( 0) },      /* British Summer */
    { "wat",    tZONE,     HOUR( 1) },      /* West Africa */
    { "at",     tZONE,     HOUR( 2) },      /* Azores */
#if     0
    /* For completeness.  BST is also British Summer, and GST is
     * also Guam Standard. */
    { "bst",    tZONE,     HOUR( 3) },      /* Brazil Standard */
    { "gst",    tZONE,     HOUR( 3) },      /* Greenland Standard */
#endif
    { "nft",    tZONE,     HOUR( 7/2) },    /* Newfoundland */
    { "nst",    tZONE,     HOUR( 7/2) },    /* Newfoundland Standard */
    { "ndt",    tDAYZONE,  HOUR( 7/2) },    /* Newfoundland Daylight */
    { "ast",    tZONE,     HOUR( 4) },      /* Atlantic Standard */
    { "adt",    tDAYZONE,  HOUR( 4) },      /* Atlantic Daylight */
    { "est",    tZONE,     HOUR( 5) },      /* Eastern Standard */
    { "edt",    tDAYZONE,  HOUR( 5) },      /* Eastern Daylight */
    { "cst",    tZONE,     HOUR( 6) },      /* Central Standard */
    { "cdt",    tDAYZONE,  HOUR( 6) },      /* Central Daylight */
    { "mst",    tZONE,     HOUR( 7) },      /* Mountain Standard */
    { "mdt",    tDAYZONE,  HOUR( 7) },      /* Mountain Daylight */
    { "pst",    tZONE,     HOUR( 8) },      /* Pacific Standard */
    { "pdt",    tDAYZONE,  HOUR( 8) },      /* Pacific Daylight */
    { "yst",    tZONE,     HOUR( 9) },      /* Yukon Standard */
    { "ydt",    tDAYZONE,  HOUR( 9) },      /* Yukon Daylight */
    { "hst",    tZONE,     HOUR(10) },      /* Hawaii Standard */
    { "hdt",    tDAYZONE,  HOUR(10) },      /* Hawaii Daylight */
    { "cat",    tZONE,     HOUR(10) },      /* Central Alaska */
    { "ahst",   tZONE,     HOUR(10) },      /* Alaska-Hawaii Standard */
    { "nt",     tZONE,     HOUR(11) },      /* Nome */
    { "idlw",   tZONE,     HOUR(12) },      /* International Date Line West */
    { "cet",    tZONE,    -HOUR( 1) },      /* Central European */
    { "cest",   tDAYZONE, -HOUR( 1) },      /* Central European Summer */
    { "met",    tZONE,    -HOUR( 1) },      /* Middle European */
    { "mewt",   tZONE,    -HOUR( 1) },      /* Middle European Winter */
    { "mest",   tDAYZONE, -HOUR( 1) },      /* Middle European Summer */
    { "swt",    tZONE,    -HOUR( 1) },      /* Swedish Winter */
    { "sst",    tDAYZONE, -HOUR( 1) },      /* Swedish Summer */
    { "fwt",    tZONE,    -HOUR( 1) },      /* French Winter */
    { "fst",    tDAYZONE, -HOUR( 1) },      /* French Summer */
    { "eet",    tZONE,    -HOUR( 2) },      /* Eastern Europe, USSR Zone 1 */
    { "bt",     tZONE,    -HOUR( 3) },      /* Baghdad, USSR Zone 2 */
    { "it",     tZONE,    -HOUR( 7/2) },    /* Iran */
    { "zp4",    tZONE,    -HOUR( 4) },      /* USSR Zone 3 */
    { "zp5",    tZONE,    -HOUR( 5) },      /* USSR Zone 4 */
    { "ist",    tZONE,    -HOUR(11/2) },    /* Indian Standard */
    { "zp6",    tZONE,    -HOUR( 6) },      /* USSR Zone 5 */
#if     0
    /* For completeness.  NST is also Newfoundland Stanard, nad SST is
     * also Swedish Summer. */
    { "nst",    tZONE,    -HOUR(13/2) },    /* North Sumatra */
    { "sst",    tZONE,    -HOUR( 7) },      /* South Sumatra, USSR Zone 6 */
#endif  /* 0 */
    { "wast",   tZONE,    -HOUR( 7) },      /* West Australian Standard */
    { "wadt",   tDAYZONE, -HOUR( 7) },      /* West Australian Daylight */
    { "jt",     tZONE,    -HOUR(15/2) },    /* Java (3pm in Cronusland!) */
    { "cct",    tZONE,    -HOUR( 8) },      /* China Coast, USSR Zone 7 */
    { "jst",    tZONE,    -HOUR( 9) },      /* Japan Standard, USSR Zone 8 */
    { "cast",   tZONE,    -HOUR(19/2) },    /* Central Australian Standard */
    { "cadt",   tDAYZONE, -HOUR(19/2) },    /* Central Australian Daylight */
    { "east",   tZONE,    -HOUR(10) },      /* Eastern Australian Standard */
    { "eadt",   tDAYZONE, -HOUR(10) },      /* Eastern Australian Daylight */
    { "gst",    tZONE,    -HOUR(10) },      /* Guam Standard, USSR Zone 9 */
    { "nzt",    tZONE,    -HOUR(12) },      /* New Zealand */
    { "nzst",   tZONE,    -HOUR(12) },      /* New Zealand Standard */
    { "nzdt",   tDAYZONE, -HOUR(12) },      /* New Zealand Daylight */
    { "idle",   tZONE,    -HOUR(12) },      /* International Date Line East */
    /* ADDED BY Marco Nijdam */
    { "dst",    tDST,     HOUR( 0) },       /* DST on (hour is ignored) */
    /* End ADDED */
    {  NULL  }
};

/*
 * Military timezone table.
 */
static TABLE    MilitaryTable[] = {
    { "a",      tZONE,  HOUR(  1) },
    { "b",      tZONE,  HOUR(  2) },
    { "c",      tZONE,  HOUR(  3) },
    { "d",      tZONE,  HOUR(  4) },
    { "e",      tZONE,  HOUR(  5) },
    { "f",      tZONE,  HOUR(  6) },
    { "g",      tZONE,  HOUR(  7) },
    { "h",      tZONE,  HOUR(  8) },
    { "i",      tZONE,  HOUR(  9) },
    { "k",      tZONE,  HOUR( 10) },
    { "l",      tZONE,  HOUR( 11) },
    { "m",      tZONE,  HOUR( 12) },
    { "n",      tZONE,  HOUR(- 1) },
    { "o",      tZONE,  HOUR(- 2) },
    { "p",      tZONE,  HOUR(- 3) },
    { "q",      tZONE,  HOUR(- 4) },
    { "r",      tZONE,  HOUR(- 5) },
    { "s",      tZONE,  HOUR(- 6) },
    { "t",      tZONE,  HOUR(- 7) },
    { "u",      tZONE,  HOUR(- 8) },
    { "v",      tZONE,  HOUR(- 9) },
    { "w",      tZONE,  HOUR(-10) },
    { "x",      tZONE,  HOUR(-11) },
    { "y",      tZONE,  HOUR(-12) },
    { "z",      tZONE,  HOUR(  0) },
    { NULL }
};


/*
 * Dump error messages in the bit bucket.
 */
static void
TclDateerror(s)
    char  *s;
{
}


static time_t
ToSeconds(Hours, Minutes, Seconds, Meridian)
    time_t      Hours;
    time_t      Minutes;
    time_t      Seconds;
    MERIDIAN    Meridian;
{
    if (Minutes < 0 || Minutes > 59 || Seconds < 0 || Seconds > 59)
        return -1;
    switch (Meridian) {
    case MER24:
        if (Hours < 0 || Hours > 23)
            return -1;
        return (Hours * 60L + Minutes) * 60L + Seconds;
    case MERam:
        if (Hours < 1 || Hours > 12)
            return -1;
        return ((Hours % 12) * 60L + Minutes) * 60L + Seconds;
    case MERpm:
        if (Hours < 1 || Hours > 12)
            return -1;
        return (((Hours % 12) + 12) * 60L + Minutes) * 60L + Seconds;
    }
    return -1;  /* Should never be reached */
}

/*
 *-----------------------------------------------------------------------------
 *
 * Convert --
 *
 *      Convert a {month, day, year, hours, minutes, seconds, meridian, dst}
 *      tuple into a clock seconds value.
 *
 * Results:
 *      0 or -1 indicating success or failure.
 *
 * Side effects:
 *      Fills TimePtr with the computed value.
 *
 *-----------------------------------------------------------------------------
 */
static int
Convert(Month, Day, Year, Hours, Minutes, Seconds, Meridian, DSTmode, TimePtr)
    time_t      Month;
    time_t      Day;
    time_t      Year;
    time_t      Hours;
    time_t      Minutes;
    time_t      Seconds;
    MERIDIAN    Meridian;
    DSTMODE     DSTmode;
    time_t     *TimePtr;
{
    static int  DaysInMonth[12] = {
        31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    time_t tod;
    time_t Julian;
    int i;

    /* Figure out how many days are in February for the given year.
     * Every year divisible by 4 is a leap year.
     * But, every year divisible by 100 is not a leap year.
     * But, every year divisible by 400 is a leap year after all.
     */
    DaysInMonth[1] = IsLeapYear(Year) ? 29 : 28;

    /* Check the inputs for validity */
    if (Month < 1 || Month > 12
	    || Year < START_OF_TIME || Year > END_OF_TIME
	    || Day < 1 || Day > DaysInMonth[(int)--Month])
        return -1;

    /* Start computing the value.  First determine the number of days
     * represented by the date, then multiply by the number of seconds/day.
     */
    for (Julian = Day - 1, i = 0; i < Month; i++)
        Julian += DaysInMonth[i];
    if (Year >= EPOCH) {
        for (i = EPOCH; i < Year; i++)
            Julian += 365 + IsLeapYear(i);
    } else {
        for (i = Year; i < EPOCH; i++)
            Julian -= 365 + IsLeapYear(i);
    }
    Julian *= SECSPERDAY;

    /* Add the timezone offset ?? */
    Julian += TclDateTimezone * 60L;

    /* Add the number of seconds represented by the time component */
    if ((tod = ToSeconds(Hours, Minutes, Seconds, Meridian)) < 0)
        return -1;
    Julian += tod;

    /* Perform a preliminary DST compensation ?? */
    if (DSTmode == DSTon
     || (DSTmode == DSTmaybe && TclpGetDate((TclpTime_t)&Julian, 0)->tm_isdst))
        Julian -= 60 * 60;
    *TimePtr = Julian;
    return 0;
}


static time_t
DSTcorrect(Start, Future)
    time_t      Start;
    time_t      Future;
{
    time_t      StartDay;
    time_t      FutureDay;
    StartDay = (TclpGetDate((TclpTime_t)&Start, 0)->tm_hour + 1) % 24;
    FutureDay = (TclpGetDate((TclpTime_t)&Future, 0)->tm_hour + 1) % 24;
    return (Future - Start) + (StartDay - FutureDay) * 60L * 60L;
}


static time_t
NamedDay(Start, DayOrdinal, DayNumber)
    time_t      Start;
    time_t      DayOrdinal;
    time_t      DayNumber;
{
    struct tm   *tm;
    time_t      now;

    now = Start;
    tm = TclpGetDate((TclpTime_t)&now, 0);
    now += SECSPERDAY * ((DayNumber - tm->tm_wday + 7) % 7);
    now += 7 * SECSPERDAY * (DayOrdinal <= 0 ? DayOrdinal : DayOrdinal - 1);
    return DSTcorrect(Start, now);
}

static time_t
NamedMonth(Start, MonthOrdinal, MonthNumber)
    time_t Start;
    time_t MonthOrdinal;
    time_t MonthNumber;
{
    struct tm *tm;
    time_t now;
    int result;
    
    now = Start;
    tm = TclpGetDate((TclpTime_t)&now, 0);
    /* To compute the next n'th month, we use this alg:
     * add n to year value
     * if currentMonth < requestedMonth decrement year value by 1 (so that
     *  doing next february from january gives us february of the current year)
     * set day to 1, time to 0
     */
    tm->tm_year += MonthOrdinal;
    if (tm->tm_mon < MonthNumber - 1) {
	tm->tm_year--;
    }
    result = Convert(MonthNumber, (time_t) 1, tm->tm_year + TM_YEAR_BASE,
	    (time_t) 0, (time_t) 0, (time_t) 0, MER24, DSTmaybe, &now);
    if (result < 0) {
	return 0;
    }
    return DSTcorrect(Start, now);
}

static int
RelativeMonth(Start, RelMonth, TimePtr)
    time_t Start;
    time_t RelMonth;
    time_t *TimePtr;
{
    struct tm *tm;
    time_t Month;
    time_t Year;
    time_t Julian;
    int result;

    if (RelMonth == 0) {
        *TimePtr = 0;
        return 0;
    }
    tm = TclpGetDate((TclpTime_t)&Start, 0);
    Month = 12 * (tm->tm_year + TM_YEAR_BASE) + tm->tm_mon + RelMonth;
    Year = Month / 12;
    Month = Month % 12 + 1;
    result = Convert(Month, (time_t) tm->tm_mday, Year,
	    (time_t) tm->tm_hour, (time_t) tm->tm_min, (time_t) tm->tm_sec,
	    MER24, DSTmaybe, &Julian);

    /*
     * The Julian time returned above is behind by one day, if "month" 
     * or "year" is used to specify relative time and the GMT flag is true.
     * This problem occurs only when the current time is closer to
     * midnight, the difference being not more than its time difference
     * with GMT. For example, in US/Pacific time zone, the problem occurs
     * whenever the current time is between midnight to 8:00am or 7:00amDST.
     * See Bug# 413397 for more details and sample script.
     * To resolve this bug, we simply add the number of seconds corresponding
     * to timezone difference with GMT to Julian time, if GMT flag is true.
     */

    if (TclDateTimezone == 0) {
        Julian += TclpGetTimeZone((unsigned long) Start) * 60L;
    }

    /*
     * The following iteration takes into account the case were we jump
     * into a "short month".  Far example, "one month from Jan 31" will
     * fail because there is no Feb 31.  The code below will reduce the
     * day and try converting the date until we succed or the date equals
     * 28 (which always works unless the date is bad in another way).
     */

    while ((result != 0) && (tm->tm_mday > 28)) {
	tm->tm_mday--;
	result = Convert(Month, (time_t) tm->tm_mday, Year,
		(time_t) tm->tm_hour, (time_t) tm->tm_min, (time_t) tm->tm_sec,
		MER24, DSTmaybe, &Julian);
    }
    if (result != 0) {
	return -1;
    }
    *TimePtr = DSTcorrect(Start, Julian);
    return 0;
}


/*
 *-----------------------------------------------------------------------------
 *
 * RelativeDay --
 *
 *      Given a starting time and a number of days before or after, compute the
 *      DST corrected difference between those dates.
 *
 * Results:
 *     1 or -1 indicating success or failure.
 *
 * Side effects:
 *      Fills TimePtr with the computed value.
 *
 *-----------------------------------------------------------------------------
 */

static int
RelativeDay(Start, RelDay, TimePtr)
    time_t Start;
    time_t RelDay;
    time_t *TimePtr;
{
    time_t new;

    new = Start + (RelDay * 60 * 60 * 24);
    *TimePtr = DSTcorrect(Start, new);
    return 1;
}

static int
LookupWord(buff)
    char                *buff;
{
    register char *p;
    register char *q;
    register TABLE *tp;
    int i;
    int abbrev;

    /*
     * Make it lowercase.
     */

    Tcl_UtfToLower(buff);

    if (strcmp(buff, "am") == 0 || strcmp(buff, "a.m.") == 0) {
        TclDatelval.Meridian = MERam;
        return tMERIDIAN;
    }
    if (strcmp(buff, "pm") == 0 || strcmp(buff, "p.m.") == 0) {
        TclDatelval.Meridian = MERpm;
        return tMERIDIAN;
    }

    /*
     * See if we have an abbreviation for a month.
     */
    if (strlen(buff) == 3) {
        abbrev = 1;
    } else if (strlen(buff) == 4 && buff[3] == '.') {
        abbrev = 1;
        buff[3] = '\0';
    } else {
        abbrev = 0;
    }

    for (tp = MonthDayTable; tp->name; tp++) {
        if (abbrev) {
            if (strncmp(buff, tp->name, 3) == 0) {
                TclDatelval.Number = tp->value;
                return tp->type;
            }
        } else if (strcmp(buff, tp->name) == 0) {
            TclDatelval.Number = tp->value;
            return tp->type;
        }
    }

    for (tp = TimezoneTable; tp->name; tp++) {
        if (strcmp(buff, tp->name) == 0) {
            TclDatelval.Number = tp->value;
            return tp->type;
        }
    }

    for (tp = UnitsTable; tp->name; tp++) {
        if (strcmp(buff, tp->name) == 0) {
            TclDatelval.Number = tp->value;
            return tp->type;
        }
    }

    /*
     * Strip off any plural and try the units table again.
     */
    i = strlen(buff) - 1;
    if (buff[i] == 's') {
        buff[i] = '\0';
        for (tp = UnitsTable; tp->name; tp++) {
            if (strcmp(buff, tp->name) == 0) {
                TclDatelval.Number = tp->value;
                return tp->type;
            }
	}
    }

    for (tp = OtherTable; tp->name; tp++) {
        if (strcmp(buff, tp->name) == 0) {
            TclDatelval.Number = tp->value;
            return tp->type;
        }
    }

    /*
     * Military timezones.
     */
    if (buff[1] == '\0' && !(*buff & 0x80)
	    && isalpha(UCHAR(*buff))) {	/* INTL: ISO only */
        for (tp = MilitaryTable; tp->name; tp++) {
            if (strcmp(buff, tp->name) == 0) {
                TclDatelval.Number = tp->value;
                return tp->type;
            }
	}
    }

    /*
     * Drop out any periods and try the timezone table again.
     */
    for (i = 0, p = q = buff; *q; q++)
        if (*q != '.') {
            *p++ = *q;
        } else {
            i++;
	}
    *p = '\0';
    if (i) {
        for (tp = TimezoneTable; tp->name; tp++) {
            if (strcmp(buff, tp->name) == 0) {
                TclDatelval.Number = tp->value;
                return tp->type;
            }
	}
    }
    
    return tID;
}


static int
TclDatelex()
{
    register char       c;
    register char       *p;
    char                buff[20];
    int                 Count;

    for ( ; ; ) {
        while (isspace(UCHAR(*TclDateInput))) {
            TclDateInput++;
	}

        if (isdigit(UCHAR(c = *TclDateInput))) { /* INTL: digit */
	    /* convert the string into a number; count the number of digits */
	    Count = 0;
            for (TclDatelval.Number = 0;
		    isdigit(UCHAR(c = *TclDateInput++)); ) { /* INTL: digit */
                TclDatelval.Number = 10 * TclDatelval.Number + c - '0';
		Count++;
	    }
            TclDateInput--;
	    /* A number with 6 or more digits is considered an ISO 8601 base */
	    if (Count >= 6) {
		return tISOBASE;
	    } else {
		return tUNUMBER;
	    }
        }
        if (!(c & 0x80) && isalpha(UCHAR(c))) {	/* INTL: ISO only. */
            for (p = buff; isalpha(UCHAR(c = *TclDateInput++)) /* INTL: ISO only. */
		     || c == '.'; ) {
                if (p < &buff[sizeof buff - 1]) {
                    *p++ = c;
		}
	    }
            *p = '\0';
            TclDateInput--;
            return LookupWord(buff);
        }
        if (c != '(') {
            return *TclDateInput++;
	}
        Count = 0;
        do {
            c = *TclDateInput++;
            if (c == '\0') {
                return c;
	    } else if (c == '(') {
                Count++;
	    } else if (c == ')') {
                Count--;
	    }
        } while (Count > 0);
    }
}

/*
 * Specify zone is of -50000 to force GMT.  (This allows BST to work).
 */

int
TclGetDate(p, now, zone, timePtr)
    char *p;
    unsigned long now;
    long zone;
    unsigned long *timePtr;
{
    struct tm *tm;
    time_t Start;
    time_t Time;
    time_t tod;
    int thisyear;

    TclDateInput = p;
    /* now has to be cast to a time_t for 64bit compliance */
    Start = now;
    tm = TclpGetDate((TclpTime_t) &Start, 0);
    thisyear = tm->tm_year + TM_YEAR_BASE;
    TclDateYear = thisyear;
    TclDateMonth = tm->tm_mon + 1;
    TclDateDay = tm->tm_mday;
    TclDateTimezone = zone;
    if (zone == -50000) {
        TclDateDSTmode = DSToff;  /* assume GMT */
        TclDateTimezone = 0;
    } else {
        TclDateDSTmode = DSTmaybe;
    }
    TclDateHour = 0;
    TclDateMinutes = 0;
    TclDateSeconds = 0;
    TclDateMeridian = MER24;
    TclDateRelSeconds = 0;
    TclDateRelMonth = 0;
    TclDateRelDay = 0;
    TclDateRelPointer = NULL;

    TclDateHaveDate = 0;
    TclDateHaveDay = 0;
    TclDateHaveOrdinalMonth = 0;
    TclDateHaveRel = 0;
    TclDateHaveTime = 0;
    TclDateHaveZone = 0;

    if (TclDateparse() || TclDateHaveTime > 1 || TclDateHaveZone > 1 || TclDateHaveDate > 1 ||
	    TclDateHaveDay > 1 || TclDateHaveOrdinalMonth > 1) {
        return -1;
    }
    
    if (TclDateHaveDate || TclDateHaveTime || TclDateHaveDay) {
	if (TclDateYear < 0) {
	    TclDateYear = -TclDateYear;
	}
	/*
	 * The following line handles years that are specified using
	 * only two digits.  The line of code below implements a policy
	 * defined by the X/Open workgroup on the millinium rollover.
	 * Note: some of those dates may not actually be valid on some
	 * platforms.  The POSIX standard startes that the dates 70-99
	 * shall refer to 1970-1999 and 00-38 shall refer to 2000-2038.
	 * This later definition should work on all platforms.
	 */

	if (TclDateYear < 100) {
	    if (TclDateYear >= 69) {
		TclDateYear += 1900;
	    } else {
		TclDateYear += 2000;
	    }
	}
	if (Convert(TclDateMonth, TclDateDay, TclDateYear, TclDateHour, TclDateMinutes, TclDateSeconds,
		TclDateMeridian, TclDateDSTmode, &Start) < 0) {
            return -1;
	}
    } else {
        Start = now;
        if (!TclDateHaveRel) {
            Start -= ((tm->tm_hour * 60L * 60L) +
		    tm->tm_min * 60L) +	tm->tm_sec;
	}
    }

    Start += TclDateRelSeconds;
    if (RelativeMonth(Start, TclDateRelMonth, &Time) < 0) {
        return -1;
    }
    Start += Time;

    if (RelativeDay(Start, TclDateRelDay, &Time) < 0) {
	return -1;
    }
    Start += Time;
    
    if (TclDateHaveDay && !TclDateHaveDate) {
        tod = NamedDay(Start, TclDateDayOrdinal, TclDateDayNumber);
        Start += tod;
    }

    if (TclDateHaveOrdinalMonth) {
	tod = NamedMonth(Start, TclDateMonthOrdinal, TclDateMonth);
	Start += tod;
    }
    
    *timePtr = Start;
    return 0;
}
static CONST TclDatetabelem TclDateexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 56
# define YYLAST 261
static CONST TclDatetabelem TclDateact[]={

    24,    40,    23,    36,    54,    81,    41,    28,    53,    26,
    37,    42,    58,    38,    56,    28,    27,    26,    28,    33,
    26,    32,    61,    50,    27,    80,    76,    27,    51,    75,
    74,    73,    30,    72,    71,    70,    69,    52,    49,    48,
    47,    45,    39,    62,    78,    46,    79,    68,    25,    65,
    60,    67,    66,    55,    44,    21,    63,    11,    10,     9,
     8,    35,     7,     6,     5,     4,     3,    43,     2,     1,
    20,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    57,     0,     0,    59,    77,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    19,    14,     0,     0,     0,
    16,    28,    22,    26,     0,    12,    13,    17,     0,    15,
    27,    18,    31,     0,     0,    29,     0,    34,    28,     0,
    26,     0,     0,     0,     0,     0,     0,    27,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    64,
    64 };
static CONST TclDatetabelem TclDatepact[]={

-10000000,   -43,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,   -26,  -268,-10000000,  -259,  -226,-10000000,  -257,    10,
  -227,  -212,  -228,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
  -229,-10000000,  -230,  -240,  -231,-10000000,-10000000,  -264,-10000000,     9,
-10000000,-10000000,  -249,-10000000,-10000000,  -246,-10000000,     4,    -2,     2,
     7,     6,-10000000,-10000000,   -11,  -232,-10000000,-10000000,-10000000,-10000000,
  -233,-10000000,  -234,  -235,-10000000,  -237,  -238,  -239,  -242,-10000000,
-10000000,-10000000,    -1,-10000000,-10000000,-10000000,   -12,-10000000,  -243,  -263,
-10000000,-10000000 };
static CONST TclDatetabelem TclDatepgo[]={

     0,    48,    70,    22,    69,    68,    66,    65,    64,    63,
    62,    60,    59,    58,    57,    55 };
static CONST TclDatetabelem TclDater1[]={

     0,     4,     4,     5,     5,     5,     5,     5,     5,     5,
     5,     5,     6,     6,     6,     6,     6,     7,     7,     7,
    10,    10,    10,    10,    10,     8,     8,     8,     8,     8,
     8,     8,     8,     8,     8,     9,     9,    12,    12,    12,
    13,    11,    11,    15,    15,    15,    15,    15,     2,     2,
     1,     1,     1,    14,     3,     3 };
static CONST TclDatetabelem TclDater2[]={

     0,     0,     4,     3,     3,     3,     3,     3,     3,     3,
     3,     2,     5,     9,    11,    13,    15,     5,     3,     3,
     3,     5,     5,     7,     5,     7,    11,     3,    11,    11,
     5,     9,     5,     3,     7,     5,     7,     7,    15,     5,
     9,     5,     2,     7,     5,     5,     7,     3,     3,     3,
     3,     3,     3,     3,     1,     3 };
static CONST TclDatetabelem TclDatechk[]={

-10000000,    -4,    -5,    -6,    -7,    -8,    -9,   -10,   -11,   -12,
   -13,   -14,   268,   269,   259,   272,   263,   270,   274,   258,
    -2,   -15,   265,    45,    43,    -1,   266,   273,   264,   261,
    58,   258,    47,    45,   263,    -1,   271,   269,   272,   268,
   258,   263,   268,    -1,    44,   268,   257,   268,   268,   268,
   263,   268,   268,   272,   268,    44,   263,    -1,   258,    -1,
    46,    -3,    45,    58,   261,    47,    45,    45,    58,   268,
   268,   268,   268,   268,   268,   268,   268,    -3,    45,    58,
   268,   268 };
static CONST TclDatetabelem TclDatedef[]={

     1,    -2,     2,     3,     4,     5,     6,     7,     8,     9,
    10,    11,    53,    18,    19,    27,     0,    33,     0,    20,
     0,    42,     0,    48,    49,    47,    50,    51,    52,    12,
     0,    22,     0,     0,    32,    44,    17,     0,    39,    30,
    24,    35,     0,    45,    21,     0,    41,     0,    54,    25,
     0,     0,    34,    37,     0,     0,    36,    46,    23,    43,
     0,    13,     0,     0,    55,     0,     0,     0,     0,    31,
    40,    14,    54,    26,    28,    29,     0,    15,     0,     0,
    16,    38 };
typedef struct
#ifdef __cplusplus
	TclDatetoktype
#endif
{ char *t_name; int t_val; } TclDatetoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

TclDatetoktype TclDatetoks[] =
{
	"tAGO",	257,
	"tDAY",	258,
	"tDAYZONE",	259,
	"tID",	260,
	"tMERIDIAN",	261,
	"tMINUTE_UNIT",	262,
	"tMONTH",	263,
	"tMONTH_UNIT",	264,
	"tSTARDATE",	265,
	"tSEC_UNIT",	266,
	"tSNUMBER",	267,
	"tUNUMBER",	268,
	"tZONE",	269,
	"tEPOCH",	270,
	"tDST",	271,
	"tISOBASE",	272,
	"tDAY_UNIT",	273,
	"tNEXT",	274,
	"-unknown-",	-1	/* ends search */
};

char * TclDatereds[] =
{
	"-no such reduction-",
	"spec : /* empty */",
	"spec : spec item",
	"item : time",
	"item : zone",
	"item : date",
	"item : ordMonth",
	"item : day",
	"item : relspec",
	"item : iso",
	"item : trek",
	"item : number",
	"time : tUNUMBER tMERIDIAN",
	"time : tUNUMBER ':' tUNUMBER o_merid",
	"time : tUNUMBER ':' tUNUMBER '-' tUNUMBER",
	"time : tUNUMBER ':' tUNUMBER ':' tUNUMBER o_merid",
	"time : tUNUMBER ':' tUNUMBER ':' tUNUMBER '-' tUNUMBER",
	"zone : tZONE tDST",
	"zone : tZONE",
	"zone : tDAYZONE",
	"day : tDAY",
	"day : tDAY ','",
	"day : tUNUMBER tDAY",
	"day : sign tUNUMBER tDAY",
	"day : tNEXT tDAY",
	"date : tUNUMBER '/' tUNUMBER",
	"date : tUNUMBER '/' tUNUMBER '/' tUNUMBER",
	"date : tISOBASE",
	"date : tUNUMBER '-' tMONTH '-' tUNUMBER",
	"date : tUNUMBER '-' tUNUMBER '-' tUNUMBER",
	"date : tMONTH tUNUMBER",
	"date : tMONTH tUNUMBER ',' tUNUMBER",
	"date : tUNUMBER tMONTH",
	"date : tEPOCH",
	"date : tUNUMBER tMONTH tUNUMBER",
	"ordMonth : tNEXT tMONTH",
	"ordMonth : tNEXT tUNUMBER tMONTH",
	"iso : tISOBASE tZONE tISOBASE",
	"iso : tISOBASE tZONE tUNUMBER ':' tUNUMBER ':' tUNUMBER",
	"iso : tISOBASE tISOBASE",
	"trek : tSTARDATE tUNUMBER '.' tUNUMBER",
	"relspec : relunits tAGO",
	"relspec : relunits",
	"relunits : sign tUNUMBER unit",
	"relunits : tUNUMBER unit",
	"relunits : tNEXT unit",
	"relunits : tNEXT tUNUMBER unit",
	"relunits : unit",
	"sign : '-'",
	"sign : '+'",
	"unit : tSEC_UNIT",
	"unit : tDAY_UNIT",
	"unit : tMONTH_UNIT",
	"number : tUNUMBER",
	"o_merid : /* empty */",
	"o_merid : tMERIDIAN",
};
#endif /* YYDEBUG */
/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */


/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#define YYERROR		goto TclDateerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( TclDatechar >= 0 || ( TclDater2[ TclDatetmp ] >> 1 ) != 1 )\
	{\
		TclDateerror( "syntax error - cannot backup" );\
		goto TclDateerrlab;\
	}\
	TclDatechar = newtoken;\
	TclDatestate = *TclDateps;\
	TclDatelval = newvalue;\
	goto TclDatenewstate;\
}
#define YYRECOVERING()	(!!TclDateerrflag)
#define YYNEW(type)	malloc(sizeof(type) * TclDatenewmax)
#define YYCOPY(to, from, type) \
	(type *) memcpy(to, (char *) from, TclDatemaxdepth * sizeof (type))
#define YYENLARGE( from, type) \
	(type *) realloc((char *) from, TclDatenewmax * sizeof(type))
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int TclDatedebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-10000000)

/*
** global variables used by the parser
*/
YYSTYPE *TclDatepv;			/* top of value stack */
int *TclDateps;			/* top of state stack */

int TclDatestate;			/* current state */
int TclDatetmp;			/* extra var (lasts between blocks) */

int TclDatenerrs;			/* number of errors */
int TclDateerrflag;			/* error recovery flag */
int TclDatechar;			/* current input token number */



#ifdef YYNMBCHARS
#define YYLEX()		TclDatecvtok(TclDatelex())
/*
** TclDatecvtok - return a token if i is a wchar_t value that exceeds 255.
**	If i<255, i itself is the token.  If i>255 but the neither 
**	of the 30th or 31st bit is on, i is already a token.
*/
#if defined(__STDC__) || defined(__cplusplus)
int TclDatecvtok(int i)
#else
int TclDatecvtok(i) int i;
#endif
{
	int first = 0;
	int last = YYNMBCHARS - 1;
	int mid;
	wchar_t j;

	if(i&0x60000000){/*Must convert to a token. */
		if( TclDatembchars[last].character < i ){
			return i;/*Giving up*/
		}
		while ((last>=first)&&(first>=0)) {/*Binary search loop*/
			mid = (first+last)/2;
			j = TclDatembchars[mid].character;
			if( j==i ){/*Found*/ 
				return TclDatembchars[mid].tvalue;
			}else if( j<i ){
				first = mid + 1;
			}else{
				last = mid -1;
			}
		}
		/*No entry in the table.*/
		return i;/* Giving up.*/
	}else{/* i is already a token. */
		return i;
	}
}
#else/*!YYNMBCHARS*/
#define YYLEX()		TclDatelex()
#endif/*!YYNMBCHARS*/

/*
** TclDateparse - return 0 if worked, 1 if syntax error not recovered from
*/
#if defined(__STDC__) || defined(__cplusplus)
int TclDateparse(void)
#else
int TclDateparse()
#endif
{
	register YYSTYPE *TclDatepvt = 0;	/* top of value stack for $vars */

#if defined(__cplusplus) || defined(lint)
/*
	hacks to please C++ and lint - goto's inside
	switch should never be executed
*/
	static int __yaccpar_lint_hack__ = 0;
	switch (__yaccpar_lint_hack__)
	{
		case 1: goto TclDateerrlab;
		case 2: goto TclDatenewstate;
	}
#endif

	/*
	** Initialize externals - TclDateparse may be called more than once
	*/
	TclDatepv = &TclDatev[-1];
	TclDateps = &TclDates[-1];
	TclDatestate = 0;
	TclDatetmp = 0;
	TclDatenerrs = 0;
	TclDateerrflag = 0;
	TclDatechar = -1;

#if YYMAXDEPTH <= 0
	if (TclDatemaxdepth <= 0)
	{
		if ((TclDatemaxdepth = YYEXPAND(0)) <= 0)
		{
			TclDateerror("yacc initialization error");
			YYABORT;
		}
	}
#endif

	{
		register YYSTYPE *TclDate_pv;	/* top of value stack */
		register int *TclDate_ps;		/* top of state stack */
		register int TclDate_state;		/* current state */
		register int  TclDate_n;		/* internal state number info */
	goto TclDatestack;	/* moved from 6 lines above to here to please C++ */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
		TclDate_pv = TclDatepv;
		TclDate_ps = TclDateps;
		TclDate_state = TclDatestate;
		goto TclDate_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	TclDatestack:
		TclDate_pv = TclDatepv;
		TclDate_ps = TclDateps;
		TclDate_state = TclDatestate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	TclDate_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( TclDatedebug )
		{
			register int TclDate_i;

			printf( "State %d, token ", TclDate_state );
			if ( TclDatechar == 0 )
				printf( "end-of-file\n" );
			else if ( TclDatechar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( TclDate_i = 0; TclDatetoks[TclDate_i].t_val >= 0;
					TclDate_i++ )
				{
					if ( TclDatetoks[TclDate_i].t_val == TclDatechar )
						break;
				}
				printf( "%s\n", TclDatetoks[TclDate_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++TclDate_ps >= &TclDates[ TclDatemaxdepth ] )	/* room on stack? */
		{
			/*
			** reallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			long TclDateps_index = (TclDate_ps - TclDates);
			long TclDatepv_index = (TclDate_pv - TclDatev);
			long TclDatepvt_index = (TclDatepvt - TclDatev);
			int TclDatenewmax;
#ifdef YYEXPAND
			TclDatenewmax = YYEXPAND(TclDatemaxdepth);
#else
			TclDatenewmax = 2 * TclDatemaxdepth;	/* double table size */
			if (TclDatemaxdepth == YYMAXDEPTH)	/* first time growth */
			{
				char *newTclDates = (char *)YYNEW(int);
				char *newTclDatev = (char *)YYNEW(YYSTYPE);
				if (newTclDates != 0 && newTclDatev != 0)
				{
					TclDates = YYCOPY(newTclDates, TclDates, int);
					TclDatev = YYCOPY(newTclDatev, TclDatev, YYSTYPE);
				}
				else
					TclDatenewmax = 0;	/* failed */
			}
			else				/* not first time */
			{
				TclDates = YYENLARGE(TclDates, int);
				TclDatev = YYENLARGE(TclDatev, YYSTYPE);
				if (TclDates == 0 || TclDatev == 0)
					TclDatenewmax = 0;	/* failed */
			}
#endif
			if (TclDatenewmax <= TclDatemaxdepth)	/* tables not expanded */
			{
				TclDateerror( "yacc stack overflow" );
				YYABORT;
			}
			TclDatemaxdepth = TclDatenewmax;

			TclDate_ps = TclDates + TclDateps_index;
			TclDate_pv = TclDatev + TclDatepv_index;
			TclDatepvt = TclDatev + TclDatepvt_index;
		}
		*TclDate_ps = TclDate_state;
		*++TclDate_pv = TclDateval;

		/*
		** we have a new state - find out what to do
		*/
	TclDate_newstate:
		if ( ( TclDate_n = TclDatepact[ TclDate_state ] ) <= YYFLAG )
			goto TclDatedefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		TclDatetmp = TclDatechar < 0;
#endif
		if ( ( TclDatechar < 0 ) && ( ( TclDatechar = YYLEX() ) < 0 ) )
			TclDatechar = 0;		/* reached EOF */
#if YYDEBUG
		if ( TclDatedebug && TclDatetmp )
		{
			register int TclDate_i;

			printf( "Received token " );
			if ( TclDatechar == 0 )
				printf( "end-of-file\n" );
			else if ( TclDatechar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( TclDate_i = 0; TclDatetoks[TclDate_i].t_val >= 0;
					TclDate_i++ )
				{
					if ( TclDatetoks[TclDate_i].t_val == TclDatechar )
						break;
				}
				printf( "%s\n", TclDatetoks[TclDate_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( TclDate_n += TclDatechar ) < 0 ) || ( TclDate_n >= YYLAST ) )
			goto TclDatedefault;
		if ( TclDatechk[ TclDate_n = TclDateact[ TclDate_n ] ] == TclDatechar )	/*valid shift*/
		{
			TclDatechar = -1;
			TclDateval = TclDatelval;
			TclDate_state = TclDate_n;
			if ( TclDateerrflag > 0 )
				TclDateerrflag--;
			goto TclDate_stack;
		}

	TclDatedefault:
		if ( ( TclDate_n = TclDatedef[ TclDate_state ] ) == -2 )
		{
#if YYDEBUG
			TclDatetmp = TclDatechar < 0;
#endif
			if ( ( TclDatechar < 0 ) && ( ( TclDatechar = YYLEX() ) < 0 ) )
				TclDatechar = 0;		/* reached EOF */
#if YYDEBUG
			if ( TclDatedebug && TclDatetmp )
			{
				register int TclDate_i;

				printf( "Received token " );
				if ( TclDatechar == 0 )
					printf( "end-of-file\n" );
				else if ( TclDatechar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( TclDate_i = 0;
						TclDatetoks[TclDate_i].t_val >= 0;
						TclDate_i++ )
					{
						if ( TclDatetoks[TclDate_i].t_val
							== TclDatechar )
						{
							break;
						}
					}
					printf( "%s\n", TclDatetoks[TclDate_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register CONST int *TclDatexi = TclDateexca;

				while ( ( *TclDatexi != -1 ) ||
					( TclDatexi[1] != TclDate_state ) )
				{
					TclDatexi += 2;
				}
				while ( ( *(TclDatexi += 2) >= 0 ) &&
					( *TclDatexi != TclDatechar ) )
					;
				if ( ( TclDate_n = TclDatexi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( TclDate_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( TclDateerrflag )
			{
			case 0:		/* new error */
				TclDateerror( "syntax error" );
				goto skip_init;
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				TclDate_pv = TclDatepv;
				TclDate_ps = TclDateps;
				TclDate_state = TclDatestate;
			skip_init:
				TclDatenerrs++;
				/* FALLTHRU */
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				TclDateerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( TclDate_ps >= TclDates )
				{
					TclDate_n = TclDatepact[ *TclDate_ps ] + YYERRCODE;
					if ( TclDate_n >= 0 && TclDate_n < YYLAST &&
						TclDatechk[TclDateact[TclDate_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						TclDate_state = TclDateact[ TclDate_n ];
						goto TclDate_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( TclDatedebug )
						printf( _POP_, *TclDate_ps,
							TclDate_ps[-1] );
#	undef _POP_
#endif
					TclDate_ps--;
					TclDate_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( TclDatedebug )
				{
					register int TclDate_i;

					printf( "Error recovery discards " );
					if ( TclDatechar == 0 )
						printf( "token end-of-file\n" );
					else if ( TclDatechar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( TclDate_i = 0;
							TclDatetoks[TclDate_i].t_val >= 0;
							TclDate_i++ )
						{
							if ( TclDatetoks[TclDate_i].t_val
								== TclDatechar )
							{
								break;
							}
						}
						printf( "token %s\n",
							TclDatetoks[TclDate_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( TclDatechar == 0 )	/* reached EOF. quit */
					YYABORT;
				TclDatechar = -1;
				goto TclDate_newstate;
			}
		}/* end if ( TclDate_n == 0 ) */
		/*
		** reduction by production TclDate_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( TclDatedebug )
			printf( "Reduce by (%d) \"%s\"\n",
				TclDate_n, TclDatereds[ TclDate_n ] );
#endif
		TclDatetmp = TclDate_n;			/* value to switch over */
		TclDatepvt = TclDate_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using TclDate_state here as temporary
		** register variable, but why not, if it works...
		** If TclDater2[ TclDate_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto TclDate_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int TclDate_len = TclDater2[ TclDate_n ];

			if ( !( TclDate_len & 01 ) )
			{
				TclDate_len >>= 1;
				TclDateval = ( TclDate_pv -= TclDate_len )[1];	/* $$ = $1 */
				TclDate_state = TclDatepgo[ TclDate_n = TclDater1[ TclDate_n ] ] +
					*( TclDate_ps -= TclDate_len ) + 1;
				if ( TclDate_state >= YYLAST ||
					TclDatechk[ TclDate_state =
					TclDateact[ TclDate_state ] ] != -TclDate_n )
				{
					TclDate_state = TclDateact[ TclDatepgo[ TclDate_n ] ];
				}
				goto TclDate_stack;
			}
			TclDate_len >>= 1;
			TclDateval = ( TclDate_pv -= TclDate_len )[1];	/* $$ = $1 */
			TclDate_state = TclDatepgo[ TclDate_n = TclDater1[ TclDate_n ] ] +
				*( TclDate_ps -= TclDate_len ) + 1;
			if ( TclDate_state >= YYLAST ||
				TclDatechk[ TclDate_state = TclDateact[ TclDate_state ] ] != -TclDate_n )
			{
				TclDate_state = TclDateact[ TclDatepgo[ TclDate_n ] ];
			}
		}
					/* save until reenter driver code */
		TclDatestate = TclDate_state;
		TclDateps = TclDate_ps;
		TclDatepv = TclDate_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( TclDatetmp )
	{
		
case 3:{
            TclDateHaveTime++;
        } break;
case 4:{
            TclDateHaveZone++;
        } break;
case 5:{
            TclDateHaveDate++;
        } break;
case 6:{
            TclDateHaveOrdinalMonth++;
        } break;
case 7:{
            TclDateHaveDay++;
        } break;
case 8:{
            TclDateHaveRel++;
        } break;
case 9:{
	    TclDateHaveTime++;
	    TclDateHaveDate++;
	} break;
case 10:{
	    TclDateHaveTime++;
	    TclDateHaveDate++;
	    TclDateHaveRel++;
        } break;
case 12:{
            TclDateHour = TclDatepvt[-1].Number;
            TclDateMinutes = 0;
            TclDateSeconds = 0;
            TclDateMeridian = TclDatepvt[-0].Meridian;
        } break;
case 13:{
            TclDateHour = TclDatepvt[-3].Number;
            TclDateMinutes = TclDatepvt[-1].Number;
            TclDateSeconds = 0;
            TclDateMeridian = TclDatepvt[-0].Meridian;
        } break;
case 14:{
            TclDateHour = TclDatepvt[-4].Number;
            TclDateMinutes = TclDatepvt[-2].Number;
            TclDateMeridian = MER24;
            TclDateDSTmode = DSToff;
            TclDateTimezone = (TclDatepvt[-0].Number % 100 + (TclDatepvt[-0].Number / 100) * 60);
        } break;
case 15:{
            TclDateHour = TclDatepvt[-5].Number;
            TclDateMinutes = TclDatepvt[-3].Number;
            TclDateSeconds = TclDatepvt[-1].Number;
            TclDateMeridian = TclDatepvt[-0].Meridian;
        } break;
case 16:{
            TclDateHour = TclDatepvt[-6].Number;
            TclDateMinutes = TclDatepvt[-4].Number;
            TclDateSeconds = TclDatepvt[-2].Number;
            TclDateMeridian = MER24;
            TclDateDSTmode = DSToff;
            TclDateTimezone = (TclDatepvt[-0].Number % 100 + (TclDatepvt[-0].Number / 100) * 60);
        } break;
case 17:{
            TclDateTimezone = TclDatepvt[-1].Number;
            TclDateDSTmode = DSTon;
        } break;
case 18:{
            TclDateTimezone = TclDatepvt[-0].Number;
            TclDateDSTmode = DSToff;
        } break;
case 19:{
            TclDateTimezone = TclDatepvt[-0].Number;
            TclDateDSTmode = DSTon;
        } break;
case 20:{
            TclDateDayOrdinal = 1;
            TclDateDayNumber = TclDatepvt[-0].Number;
        } break;
case 21:{
            TclDateDayOrdinal = 1;
            TclDateDayNumber = TclDatepvt[-1].Number;
        } break;
case 22:{
            TclDateDayOrdinal = TclDatepvt[-1].Number;
            TclDateDayNumber = TclDatepvt[-0].Number;
        } break;
case 23:{
            TclDateDayOrdinal = TclDatepvt[-2].Number * TclDatepvt[-1].Number;
            TclDateDayNumber = TclDatepvt[-0].Number;
        } break;
case 24:{
            TclDateDayOrdinal = 2;
            TclDateDayNumber = TclDatepvt[-0].Number;
        } break;
case 25:{
            TclDateMonth = TclDatepvt[-2].Number;
            TclDateDay = TclDatepvt[-0].Number;
        } break;
case 26:{
            TclDateMonth = TclDatepvt[-4].Number;
            TclDateDay = TclDatepvt[-2].Number;
            TclDateYear = TclDatepvt[-0].Number;
        } break;
case 27:{
	    TclDateYear = TclDatepvt[-0].Number / 10000;
	    TclDateMonth = (TclDatepvt[-0].Number % 10000)/100;
	    TclDateDay = TclDatepvt[-0].Number % 100;
	} break;
case 28:{
	    TclDateDay = TclDatepvt[-4].Number;
	    TclDateMonth = TclDatepvt[-2].Number;
	    TclDateYear = TclDatepvt[-0].Number;
	} break;
case 29:{
            TclDateMonth = TclDatepvt[-2].Number;
            TclDateDay = TclDatepvt[-0].Number;
            TclDateYear = TclDatepvt[-4].Number;
        } break;
case 30:{
            TclDateMonth = TclDatepvt[-1].Number;
            TclDateDay = TclDatepvt[-0].Number;
        } break;
case 31:{
            TclDateMonth = TclDatepvt[-3].Number;
            TclDateDay = TclDatepvt[-2].Number;
            TclDateYear = TclDatepvt[-0].Number;
        } break;
case 32:{
            TclDateMonth = TclDatepvt[-0].Number;
            TclDateDay = TclDatepvt[-1].Number;
        } break;
case 33:{
	    TclDateMonth = 1;
	    TclDateDay = 1;
	    TclDateYear = EPOCH;
	} break;
case 34:{
            TclDateMonth = TclDatepvt[-1].Number;
            TclDateDay = TclDatepvt[-2].Number;
            TclDateYear = TclDatepvt[-0].Number;
        } break;
case 35:{
	    TclDateMonthOrdinal = 1;
	    TclDateMonth = TclDatepvt[-0].Number;
	} break;
case 36:{
	    TclDateMonthOrdinal = TclDatepvt[-1].Number;
	    TclDateMonth = TclDatepvt[-0].Number;
	} break;
case 37:{
            if (TclDatepvt[-1].Number != HOUR(- 7)) YYABORT;
	    TclDateYear = TclDatepvt[-2].Number / 10000;
	    TclDateMonth = (TclDatepvt[-2].Number % 10000)/100;
	    TclDateDay = TclDatepvt[-2].Number % 100;
	    TclDateHour = TclDatepvt[-0].Number / 10000;
	    TclDateMinutes = (TclDatepvt[-0].Number % 10000)/100;
	    TclDateSeconds = TclDatepvt[-0].Number % 100;
        } break;
case 38:{
            if (TclDatepvt[-5].Number != HOUR(- 7)) YYABORT;
	    TclDateYear = TclDatepvt[-6].Number / 10000;
	    TclDateMonth = (TclDatepvt[-6].Number % 10000)/100;
	    TclDateDay = TclDatepvt[-6].Number % 100;
	    TclDateHour = TclDatepvt[-4].Number;
	    TclDateMinutes = TclDatepvt[-2].Number;
	    TclDateSeconds = TclDatepvt[-0].Number;
        } break;
case 39:{
	    TclDateYear = TclDatepvt[-1].Number / 10000;
	    TclDateMonth = (TclDatepvt[-1].Number % 10000)/100;
	    TclDateDay = TclDatepvt[-1].Number % 100;
	    TclDateHour = TclDatepvt[-0].Number / 10000;
	    TclDateMinutes = (TclDatepvt[-0].Number % 10000)/100;
	    TclDateSeconds = TclDatepvt[-0].Number % 100;
        } break;
case 40:{
            /*
	     * Offset computed year by -377 so that the returned years will
	     * be in a range accessible with a 32 bit clock seconds value
	     */
            TclDateYear = TclDatepvt[-2].Number/1000 + 2323 - 377;
            TclDateDay  = 1;
	    TclDateMonth = 1;
	    TclDateRelDay += ((TclDatepvt[-2].Number%1000)*(365 + IsLeapYear(TclDateYear)))/1000;
	    TclDateRelSeconds += TclDatepvt[-0].Number * 144 * 60;
        } break;
case 41:{
	    TclDateRelSeconds *= -1;
	    TclDateRelMonth *= -1;
	    TclDateRelDay *= -1;
	} break;
case 43:{ *TclDateRelPointer += TclDatepvt[-2].Number * TclDatepvt[-1].Number * TclDatepvt[-0].Number; } break;
case 44:{ *TclDateRelPointer += TclDatepvt[-1].Number * TclDatepvt[-0].Number; } break;
case 45:{ *TclDateRelPointer += TclDatepvt[-0].Number; } break;
case 46:{ *TclDateRelPointer += TclDatepvt[-1].Number * TclDatepvt[-0].Number; } break;
case 47:{ *TclDateRelPointer += TclDatepvt[-0].Number; } break;
case 48:{ TclDateval.Number = -1; } break;
case 49:{ TclDateval.Number =  1; } break;
case 50:{ TclDateval.Number = TclDatepvt[-0].Number; TclDateRelPointer = &TclDateRelSeconds; } break;
case 51:{ TclDateval.Number = TclDatepvt[-0].Number; TclDateRelPointer = &TclDateRelDay; } break;
case 52:{ TclDateval.Number = TclDatepvt[-0].Number; TclDateRelPointer = &TclDateRelMonth; } break;
case 53:{
	if (TclDateHaveTime && TclDateHaveDate && !TclDateHaveRel) {
	    TclDateYear = TclDatepvt[-0].Number;
	} else {
	    TclDateHaveTime++;
	    if (TclDatepvt[-0].Number < 100) {
		TclDateHour = TclDatepvt[-0].Number;
		TclDateMinutes = 0;
	    } else {
		TclDateHour = TclDatepvt[-0].Number / 100;
		TclDateMinutes = TclDatepvt[-0].Number % 100;
	    }
	    TclDateSeconds = 0;
	    TclDateMeridian = MER24;
	}
    } break;
case 54:{
            TclDateval.Meridian = MER24;
        } break;
case 55:{
            TclDateval.Meridian = TclDatepvt[-0].Meridian;
        } break;
	}
	goto TclDatestack;		/* reset registers in driver code */
}

