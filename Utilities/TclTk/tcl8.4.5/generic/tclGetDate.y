/* 
 * tclGetDate.y --
 *
 *	Contains yacc grammar for parsing date and time strings.
 *	The output of this file should be the file tclDate.c which
 *	is used directly in the Tcl sources.
 *
 * Copyright (c) 1992-1995 Karl Lehenbauer and Mark Diekhans.
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

%{
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
 * SCCSID
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
static char     *yyInput;
static DSTMODE  yyDSTmode;
static time_t   yyDayOrdinal;
static time_t   yyDayNumber;
static time_t   yyMonthOrdinal;
static int      yyHaveDate;
static int      yyHaveDay;
static int      yyHaveOrdinalMonth;
static int      yyHaveRel;
static int      yyHaveTime;
static int      yyHaveZone;
static time_t   yyTimezone;
static time_t   yyDay;
static time_t   yyHour;
static time_t   yyMinutes;
static time_t   yyMonth;
static time_t   yySeconds;
static time_t   yyYear;
static MERIDIAN yyMeridian;
static time_t   yyRelMonth;
static time_t   yyRelDay;
static time_t   yyRelSeconds;
static time_t  *yyRelPointer;

/*
 * Prototypes of internal functions.
 */
static void	yyerror _ANSI_ARGS_((char *s));
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
static int	yylex _ANSI_ARGS_((void));

int
yyparse _ANSI_ARGS_((void));
%}

%union {
    time_t              Number;
    enum _MERIDIAN      Meridian;
}

%token  tAGO tDAY tDAYZONE tID tMERIDIAN tMINUTE_UNIT tMONTH tMONTH_UNIT
%token  tSTARDATE tSEC_UNIT tSNUMBER tUNUMBER tZONE tEPOCH tDST tISOBASE
%token  tDAY_UNIT tNEXT

%type   <Number>        tDAY tDAYZONE tMINUTE_UNIT tMONTH tMONTH_UNIT tDST
%type   <Number>        tSEC_UNIT tSNUMBER tUNUMBER tZONE tISOBASE tDAY_UNIT
%type   <Number>        unit sign tNEXT tSTARDATE
%type   <Meridian>      tMERIDIAN o_merid

%%

spec    : /* NULL */
        | spec item
        ;

item    : time {
            yyHaveTime++;
        }
        | zone {
            yyHaveZone++;
        }
        | date {
            yyHaveDate++;
        }
        | ordMonth {
            yyHaveOrdinalMonth++;
        }
        | day {
            yyHaveDay++;
        }
        | relspec {
            yyHaveRel++;
        }
        | iso {
	    yyHaveTime++;
	    yyHaveDate++;
	}
        | trek {
	    yyHaveTime++;
	    yyHaveDate++;
	    yyHaveRel++;
        }
        | number
        ;

time    : tUNUMBER tMERIDIAN {
            yyHour = $1;
            yyMinutes = 0;
            yySeconds = 0;
            yyMeridian = $2;
        }
        | tUNUMBER ':' tUNUMBER o_merid {
            yyHour = $1;
            yyMinutes = $3;
            yySeconds = 0;
            yyMeridian = $4;
        }
        | tUNUMBER ':' tUNUMBER '-' tUNUMBER {
            yyHour = $1;
            yyMinutes = $3;
            yyMeridian = MER24;
            yyDSTmode = DSToff;
            yyTimezone = ($5 % 100 + ($5 / 100) * 60);
        }
        | tUNUMBER ':' tUNUMBER ':' tUNUMBER o_merid {
            yyHour = $1;
            yyMinutes = $3;
            yySeconds = $5;
            yyMeridian = $6;
        }
        | tUNUMBER ':' tUNUMBER ':' tUNUMBER '-' tUNUMBER {
            yyHour = $1;
            yyMinutes = $3;
            yySeconds = $5;
            yyMeridian = MER24;
            yyDSTmode = DSToff;
            yyTimezone = ($7 % 100 + ($7 / 100) * 60);
        }
        ;

zone    : tZONE tDST {
            yyTimezone = $1;
            yyDSTmode = DSTon;
        }
        | tZONE {
            yyTimezone = $1;
            yyDSTmode = DSToff;
        }
        | tDAYZONE {
            yyTimezone = $1;
            yyDSTmode = DSTon;
        }
        ;

day     : tDAY {
            yyDayOrdinal = 1;
            yyDayNumber = $1;
        }
        | tDAY ',' {
            yyDayOrdinal = 1;
            yyDayNumber = $1;
        }
        | tUNUMBER tDAY {
            yyDayOrdinal = $1;
            yyDayNumber = $2;
        }
        | sign tUNUMBER tDAY {
            yyDayOrdinal = $1 * $2;
            yyDayNumber = $3;
        }
        | tNEXT tDAY {
            yyDayOrdinal = 2;
            yyDayNumber = $2;
        }
        ;

date    : tUNUMBER '/' tUNUMBER {
            yyMonth = $1;
            yyDay = $3;
        }
        | tUNUMBER '/' tUNUMBER '/' tUNUMBER {
            yyMonth = $1;
            yyDay = $3;
            yyYear = $5;
        }
        | tISOBASE {
	    yyYear = $1 / 10000;
	    yyMonth = ($1 % 10000)/100;
	    yyDay = $1 % 100;
	}
        | tUNUMBER '-' tMONTH '-' tUNUMBER {
	    yyDay = $1;
	    yyMonth = $3;
	    yyYear = $5;
	}
        | tUNUMBER '-' tUNUMBER '-' tUNUMBER {
            yyMonth = $3;
            yyDay = $5;
            yyYear = $1;
        }
        | tMONTH tUNUMBER {
            yyMonth = $1;
            yyDay = $2;
        }
        | tMONTH tUNUMBER ',' tUNUMBER {
            yyMonth = $1;
            yyDay = $2;
            yyYear = $4;
        }
        | tUNUMBER tMONTH {
            yyMonth = $2;
            yyDay = $1;
        }
        | tEPOCH {
	    yyMonth = 1;
	    yyDay = 1;
	    yyYear = EPOCH;
	}
        | tUNUMBER tMONTH tUNUMBER {
            yyMonth = $2;
            yyDay = $1;
            yyYear = $3;
        }
        ;

ordMonth: tNEXT tMONTH {
	    yyMonthOrdinal = 1;
	    yyMonth = $2;
	}
        | tNEXT tUNUMBER tMONTH {
	    yyMonthOrdinal = $2;
	    yyMonth = $3;
	}
        ;

iso     : tISOBASE tZONE tISOBASE {
            if ($2 != HOUR(- 7)) YYABORT;
	    yyYear = $1 / 10000;
	    yyMonth = ($1 % 10000)/100;
	    yyDay = $1 % 100;
	    yyHour = $3 / 10000;
	    yyMinutes = ($3 % 10000)/100;
	    yySeconds = $3 % 100;
        }
        | tISOBASE tZONE tUNUMBER ':' tUNUMBER ':' tUNUMBER {
            if ($2 != HOUR(- 7)) YYABORT;
	    yyYear = $1 / 10000;
	    yyMonth = ($1 % 10000)/100;
	    yyDay = $1 % 100;
	    yyHour = $3;
	    yyMinutes = $5;
	    yySeconds = $7;
        }
	| tISOBASE tISOBASE {
	    yyYear = $1 / 10000;
	    yyMonth = ($1 % 10000)/100;
	    yyDay = $1 % 100;
	    yyHour = $2 / 10000;
	    yyMinutes = ($2 % 10000)/100;
	    yySeconds = $2 % 100;
        }
        ;

trek    : tSTARDATE tUNUMBER '.' tUNUMBER {
            /*
	     * Offset computed year by -377 so that the returned years will
	     * be in a range accessible with a 32 bit clock seconds value
	     */
            yyYear = $2/1000 + 2323 - 377;
            yyDay  = 1;
	    yyMonth = 1;
	    yyRelDay += (($2%1000)*(365 + IsLeapYear(yyYear)))/1000;
	    yyRelSeconds += $4 * 144 * 60;
        }
        ;

relspec : relunits tAGO {
	    yyRelSeconds *= -1;
	    yyRelMonth *= -1;
	    yyRelDay *= -1;
	}
	| relunits
	;
relunits : sign tUNUMBER unit  { *yyRelPointer += $1 * $2 * $3; }
        | tUNUMBER unit        { *yyRelPointer += $1 * $2; }
        | tNEXT unit           { *yyRelPointer += $2; }
        | tNEXT tUNUMBER unit  { *yyRelPointer += $2 * $3; }
        | unit                 { *yyRelPointer += $1; }
        ;
sign    : '-'            { $$ = -1; }
        | '+'            { $$ =  1; }
        ;
unit    : tSEC_UNIT      { $$ = $1; yyRelPointer = &yyRelSeconds; }
        | tDAY_UNIT      { $$ = $1; yyRelPointer = &yyRelDay; }
        | tMONTH_UNIT    { $$ = $1; yyRelPointer = &yyRelMonth; }
        ;

number  : tUNUMBER
    {
	if (yyHaveTime && yyHaveDate && !yyHaveRel) {
	    yyYear = $1;
	} else {
	    yyHaveTime++;
	    if ($1 < 100) {
		yyHour = $1;
		yyMinutes = 0;
	    } else {
		yyHour = $1 / 100;
		yyMinutes = $1 % 100;
	    }
	    yySeconds = 0;
	    yyMeridian = MER24;
	}
    }
;

o_merid : /* NULL */ {
            $$ = MER24;
        }
        | tMERIDIAN {
            $$ = $1;
        }
        ;

%%

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
yyerror(s)
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
    Julian += yyTimezone * 60L;

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
        yylval.Meridian = MERam;
        return tMERIDIAN;
    }
    if (strcmp(buff, "pm") == 0 || strcmp(buff, "p.m.") == 0) {
        yylval.Meridian = MERpm;
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
                yylval.Number = tp->value;
                return tp->type;
            }
        } else if (strcmp(buff, tp->name) == 0) {
            yylval.Number = tp->value;
            return tp->type;
        }
    }

    for (tp = TimezoneTable; tp->name; tp++) {
        if (strcmp(buff, tp->name) == 0) {
            yylval.Number = tp->value;
            return tp->type;
        }
    }

    for (tp = UnitsTable; tp->name; tp++) {
        if (strcmp(buff, tp->name) == 0) {
            yylval.Number = tp->value;
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
                yylval.Number = tp->value;
                return tp->type;
            }
	}
    }

    for (tp = OtherTable; tp->name; tp++) {
        if (strcmp(buff, tp->name) == 0) {
            yylval.Number = tp->value;
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
                yylval.Number = tp->value;
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
                yylval.Number = tp->value;
                return tp->type;
            }
	}
    }
    
    return tID;
}


static int
yylex()
{
    register char       c;
    register char       *p;
    char                buff[20];
    int                 Count;

    for ( ; ; ) {
        while (isspace(UCHAR(*yyInput))) {
            yyInput++;
	}

        if (isdigit(UCHAR(c = *yyInput))) { /* INTL: digit */
	    /* convert the string into a number; count the number of digits */
	    Count = 0;
            for (yylval.Number = 0;
		    isdigit(UCHAR(c = *yyInput++)); ) { /* INTL: digit */
                yylval.Number = 10 * yylval.Number + c - '0';
		Count++;
	    }
            yyInput--;
	    /* A number with 6 or more digits is considered an ISO 8601 base */
	    if (Count >= 6) {
		return tISOBASE;
	    } else {
		return tUNUMBER;
	    }
        }
        if (!(c & 0x80) && isalpha(UCHAR(c))) {	/* INTL: ISO only. */
            for (p = buff; isalpha(UCHAR(c = *yyInput++)) /* INTL: ISO only. */
		     || c == '.'; ) {
                if (p < &buff[sizeof buff - 1]) {
                    *p++ = c;
		}
	    }
            *p = '\0';
            yyInput--;
            return LookupWord(buff);
        }
        if (c != '(') {
            return *yyInput++;
	}
        Count = 0;
        do {
            c = *yyInput++;
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

    yyInput = p;
    /* now has to be cast to a time_t for 64bit compliance */
    Start = now;
    tm = TclpGetDate((TclpTime_t) &Start, 0);
    thisyear = tm->tm_year + TM_YEAR_BASE;
    yyYear = thisyear;
    yyMonth = tm->tm_mon + 1;
    yyDay = tm->tm_mday;
    yyTimezone = zone;
    if (zone == -50000) {
        yyDSTmode = DSToff;  /* assume GMT */
        yyTimezone = 0;
    } else {
        yyDSTmode = DSTmaybe;
    }
    yyHour = 0;
    yyMinutes = 0;
    yySeconds = 0;
    yyMeridian = MER24;
    yyRelSeconds = 0;
    yyRelMonth = 0;
    yyRelDay = 0;
    yyRelPointer = NULL;

    yyHaveDate = 0;
    yyHaveDay = 0;
    yyHaveOrdinalMonth = 0;
    yyHaveRel = 0;
    yyHaveTime = 0;
    yyHaveZone = 0;

    if (yyparse() || yyHaveTime > 1 || yyHaveZone > 1 || yyHaveDate > 1 ||
	    yyHaveDay > 1 || yyHaveOrdinalMonth > 1) {
        return -1;
    }
    
    if (yyHaveDate || yyHaveTime || yyHaveDay) {
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
	if (Convert(yyMonth, yyDay, yyYear, yyHour, yyMinutes, yySeconds,
		yyMeridian, yyDSTmode, &Start) < 0) {
            return -1;
	}
    } else {
        Start = now;
        if (!yyHaveRel) {
            Start -= ((tm->tm_hour * 60L * 60L) +
		    tm->tm_min * 60L) +	tm->tm_sec;
	}
    }

    Start += yyRelSeconds;
    if (RelativeMonth(Start, yyRelMonth, &Time) < 0) {
        return -1;
    }
    Start += Time;

    if (RelativeDay(Start, yyRelDay, &Time) < 0) {
	return -1;
    }
    Start += Time;
    
    if (yyHaveDay && !yyHaveDate) {
        tod = NamedDay(Start, yyDayOrdinal, yyDayNumber);
        Start += tod;
    }

    if (yyHaveOrdinalMonth) {
	tod = NamedMonth(Start, yyMonthOrdinal, yyMonth);
	Start += tod;
    }
    
    *timePtr = Start;
    return 0;
}
