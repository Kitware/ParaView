/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NUMBER = 258,
     VAR = 259,
     BLTIN_UNARY = 260,
     BLTIN_BINARY = 261,
     BLTIN_TRINARY = 262,
     BLTIN_QUATERNARY = 263,
     BLTIN_QUINTIC = 264,
     UNDEF = 265,
     DPARAM = 266,
     IPARAM = 267,
     OPTION = 268,
     UNARYMINUS = 269
   };
#endif
/* Tokens.  */
#define NUMBER 258
#define VAR 259
#define BLTIN_UNARY 260
#define BLTIN_BINARY 261
#define BLTIN_TRINARY 262
#define BLTIN_QUATERNARY 263
#define BLTIN_QUINTIC 264
#define UNDEF 265
#define DPARAM 266
#define IPARAM 267
#define OPTION 268
#define UNARYMINUS 269




/* Copy the first part of user declarations.  */
#line 1 "../src/manager.y"

/* --------------------------------------------------------------------- *
 * Manager:  Symbol table manager and parser                             *
 *                                                                       *
 * This file contains functions for managing a set of lookup tables and  *
 * several parsers.  The tables maintained are: (1) a global symbol tab- *
 * le for the parser containing mathematical constants and functions,    *
 * (2) a parameter table used by the spectral element solver, and (3) an *
 * option table used maintaining integer-valued options.  The interface  *
 * routines are as follows:                                              *
 *                                                                       *
 *                                                                       *
 * Internal Symbol Table                                                 *
 * ---------------------                                                 * 
 * Symbol *install(char *name, int type, ...)                            *
 * Symbol *lookup (char *name)                                           *
 *                                                                       *
 * Parameter Symbol Table                                                *
 * ----------------------                                                *
 * int     iparam     (char *name)                                       *
 * int     iparam_set (char *name, int value)                            *
 *                                                                       *
 * double  dparam     (char *name)                                       *
 * double  dparam_set (char *name, double value)                         *
 *                                                                       *
 * Options Table                                                         *
 * -------------                                                         *
 * int     option     (char *name)                                       *
 * int     option_set (char *name, int status)                           *
 *                                                                       *
 *                                                                       *
 * Vector/Scalar Parser                                                  *
 * --------------------                                                  *
 * The parsers provide two types of function-string parsing based on the *
 * type of access involved: a vector parser for forcing functions and    *
 * boundary conditions and a scalar parser for miscellaneous applica-    *   
 * tions.  The interfaces for these routines are:                        *
 *                                                                       *
 * void    vector_def (char *vlist, char *function)                      *
 * void    vector_set (int   vsize, v1, v2, ..., f(v))                   *
 *                                                                       *
 * double  scalar     (char *function)                                   *
 * --------------------------------------------------------------------- */
 
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>

#include "tree.h"
#include "zbesj.h"


#define SIZE   256       /* Maximum number of function string characters */

typedef double (*PFD)(); /* Pointer to a function returning double */

typedef struct Symbol {  /* Symbol table entry */
        char *name   ;   /* Symbol name        */
        short type   ;   /* VAR, BLTIN, UNDEF, DPARAM, IPARAM, OPTION */
	short status ;   /* (see status definitions) */
        union {
		int    num ;
                double val ;
		PFD    ptr ;
	      } u;
      } Symbol;

//extern double doUserdefA(double x, double y, double z);


/* --------------------------------------------------------------------- *
 *                 Function declarations and prototypes                  *
 * --------------------------------------------------------------------- */

/* Internal Prototypes */

static Symbol  *install(char*, int, ...),    /* Table management (LOCAL) */
               *lookup (char*);
static  double 
  Sqrt(double),                         /* Operators (mathematical) */
  Rand(double), Integer(double), 
  Mod(double), 
  Log(double), Log10(double), 
  Exp(double), 
  Radius(double,double),                /* ... binary operators ... */
  Jn(double,double),
  Yn(double,double),
  Angle(double,double),
  Step(double,double),                  /* Step function */
  Step2(double,double),                  /* Step function */
  Pow(double,double), 
  Shock(double,double,double),
  ReJn(double, double, double),
  ImJn(double, double, double),
  Jacobi(double,double,double,double),
  Bump(double), 
  Single(double, double),               /* NUWC new single tile: electrodes  */
  Womsin(double,double,double,double,double),
  Womcos(double,double,double,double,double);

//static double 
//   UserdefA(double,double,double);


extern void show_symbol (Symbol *s);         /* Print symbol's value     */

/* --------------------------------------------------------------------- *
 *                                                                       *
 *          P R O G R A M    D E F A U L T   P A R A M E T E R S         *
 *                                                                       *
 * The following are the default values for options and parameters used  *
 * in the Helmholtz and Navier-Stokes solvers.                           *
 *                                                                       *
 * --------------------------------------------------------------------- */

static struct {
  char   *name;
  int     oval;
} O_default[] = {                 /* Options */
	 "binary",      1,
	 "direct",      1,
	 "core",        1,
	 "ReCalcPrecon",1,
	 "NPVIZ",       2,
	  0, 0
};

static struct {                /* Parameters (integer) */
  char   *name;
  int     pval;
} I_default[] = {
         "P_HYBRID_STATUS",       1,
         "P_MAX_NMB_OF_STEPS",    0,
         "P_TIME_PERIODIC_DUMPS", 0,
         "P_TIME_DUMPS_NMB",      6,
         "P_TIME_INTERP_ORD",     5,
         "P_N_BUNCHS",            1,
         "P_REVERSE_DBG",         0,
         "P_COLOR_ID",            1,
         "P_RK_scheme_id",        7,
         "P_RK_IOsteps",          0,
         "HISSTEP",               0,
	 "CFLSTEP",               0,
         "DIM",                   2,
	 "NSTEPS",                1,
	 "NRCSTEP",               0, /* needed for steering of computation. resets RC B.C. values */
	 "IOSTEP",                0,
         "ELEMENTS",              0,
	 "NORDER",                5,
	 "MODES",                 0,
	 "EQTYPE",                0,
	 "INTYPE",                2,
         "VTKSTEPS",              0,
         "RMTHST",                0, /* 1 if remote host is spesified 0 if not */
	 "NPODORDER",		  0, /* number of fields for accelerator */
         "NPPODORDER",            0, /* number of fields for accelerator */	
         "NPORT",              5001, /* default port number */
	 "LQUAD",                 0, /* quadrature points in 'a' direction  */
         "MQUAD",                 0, /* quadrature points in 'b' direction  */
#if DIM == 3
	 "NQUAD",                 0, /* quadrature points in 'c' direction  */
#endif
	 "IDpatch",		  0, /* handle, index of domain currently being processed*/
	  0,                      0
};

static struct {                  /* Parameters (double) */
  char   *name;
  double  pval;
} D_default[] = {
         "P_COOR_TOL",      1.E-14,
         "P_TIME_STEP_TOL", 1.E-6,
         "P_BUNCH_TIME",    0.,
         "P_MAX_TIME",      0.,
         "P_RK_Kfixed",     0.,
         "P_DUMP_TIME",     0.,
         "DT",              0.001,    /* Time step (also below)    */
	 "DELT",            0.,
	 "STARTIME",        0.,
	 "XSCALE",          1.,
	 "YSCALE",          1.,
	 "TOL",             1.e-8,    /* Last-resort tolerance       */
	 "TOLCG",           1.e-8,    /* Conjugate Gradient Solver   */
	 "TOLCGP",          1.e-8,    /* Pressure Conjugate Gradient */
	 "TOLABS",          1.e-8,    /* Default PCG tolerance       */
	 "TOLREL",          1.e-6,    /* Default for ?               */
	 "IOTIME",          0.,
	 "FLOWRATE",        0.,
	 "PGRADX",          0.,
	 "FFZ",             0.,       /* Applied force for N-S       */
	 "FFY",             0.,
	 "FFX",             0.,
	 "LAMBDA",          1.e30,    /* Helmholtz Constant          */
	 "KINVIS",          1.,       /* 1/Re for N-S                */
	 "THETA",           0.0,      /* theta scheme variable       */
	 "BNDTIMEFCE",      1.0,      /* time dependent boundary fce */
	 "LZ",              1.0,      /* default Z direction length  */
	 "Re_Uinf",         1.0,      /* default velocity in Re      */
	 "Re_Len",          1.0,      /* default length in Re        */
	 "PSfactor",        1.0,      /* default scaling width of prism */
         "DMAXANGLESUR",    0.95,     /* maximum angle for Recon surfaces */ 
	  0,                0.
};


static struct {                  /* Constants */
  char    *name;
  double   cval;
} consts[] = {
        "PI",     3.14159265358979323846,   /* Pi */
	"E",      2.71828182845904523536,   /* Natural logarithm */
	"GAMMA",  0.57721566490153286060,   /* Euler */
	"DEG",   57.29577951308232087680,   /* deg/radian */
	"PHI",    1.61803398874989484820,   /* golden ratio */
	 0,       0
};

static struct {                /* Built-ins */
  char    *name;               /* Function name */
  short    args;               /* # of arguments */
  PFD      func;               /* Pointer to the function */
} builtins[] = {
         "sin",   1,  sin,
	 "cos",   1,  cos,
	 "cosh",  1,  cosh,
	 "sinh",  1,  sinh,
         "tanh",  1,  tanh,
	 "atan",  1,  atan,
	 "abs",   1,  fabs,
	 "int",   1,  Integer,     /* .... Argument Checking .... */
	 "log",   1,  Log,         
	 "log10", 1,  Log10,       
	 "exp",   1,  Exp,         
	 "sqrt",  1,  Sqrt,        
	 "rand",  1,  Rand,        /* random number (input the magnitude) */
	 "mod",   1,  Mod,   /* remainder */
	 "bump",  1,  Bump, 
	 "single", 2, Single,
	 "jn",    2,  Jn,      /* Bessel function J */
	 "yn",    2,  Yn,      /* Bessel function Y */
	 "rad",   2,  Radius,  /* rad = sqrt(x^2 + y^2) */
	 "ang",   2,  Angle,   /* ang = atan2(x,y)      */
	 "step",  2,  Step,    /* step(x,a) = 0  (if x < a) else 1 */
	 "step2", 2,  Step2,   /* step(x,a) = 0 (if x <= a) else 1 */
	 "rejn",  3,  ReJn,    /* Real part of complex Bessel function Jn(z) */
	 "imjn",  3,  ImJn,    /* Imag part of complex Bessel function Jn(z) */
         "shock", 3,  Shock,   /* shock(x,a,b) = a (if x < 0), (a+b)/2 (if x==0) or b (if x > 0) */
         "jacobi", 4,  Jacobi,
	 "womsin", 5,  Womsin,     /* Womersley solution due to sin component of u_avg  */
	 "womcos", 5,  Womcos,     /* Womersley solution due to cos component of u_avg  */
   /*      "userdefA", 3, UserdefA, */
	 0,       0
};

/* External variables */

Tree*    Symbols  = 0;     /* Symbol table     */
Tree*    Options  = 0;     /* Option table     */
Tree*    Params   = 0;     /* Parameters table */
jmp_buf  begin;            

static char     func_string[SIZE], 
                *cur_string;
static double   stack_value;

extern int errno;



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 275 "../src/manager.y"
typedef union YYSTYPE {                /* stack type */
	double  val;    /* actual value */
	Symbol *sym;    /* symbol table pointer */
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 393 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 405 "y.tab.c"

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYINCLUDED_STDLIB_H
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   243

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  25
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  4
/* YYNRULES -- Number of rules. */
#define YYNRULES  24
/* YYNRULES -- Number of states. */
#define YYNSTATES  72

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   269

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      21,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      22,    23,    17,    15,    24,    16,     2,    18,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    14,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    20,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    19
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     4,     7,    11,    15,    19,    23,    25,
      27,    29,    31,    33,    38,    45,    54,    65,    78,    82,
      86,    90,    94,    98,   102
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      26,     0,    -1,    -1,    26,    21,    -1,    26,    27,    21,
      -1,    26,    28,    21,    -1,    26,     1,    21,    -1,     4,
      14,    28,    -1,     3,    -1,     4,    -1,    12,    -1,    11,
      -1,    27,    -1,     5,    22,    28,    23,    -1,     6,    22,
      28,    24,    28,    23,    -1,     7,    22,    28,    24,    28,
      24,    28,    23,    -1,     8,    22,    28,    24,    28,    24,
      28,    24,    28,    23,    -1,     9,    22,    28,    24,    28,
      24,    28,    24,    28,    24,    28,    23,    -1,    28,    15,
      28,    -1,    28,    16,    28,    -1,    28,    17,    28,    -1,
      28,    18,    28,    -1,    28,    20,    28,    -1,    22,    28,
      23,    -1,    16,    28,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   288,   288,   289,   290,   291,   292,   294,   296,   297,
     300,   301,   302,   303,   305,   307,   309,   311,   313,   314,
     315,   316,   329,   330,   331
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NUMBER", "VAR", "BLTIN_UNARY",
  "BLTIN_BINARY", "BLTIN_TRINARY", "BLTIN_QUATERNARY", "BLTIN_QUINTIC",
  "UNDEF", "DPARAM", "IPARAM", "OPTION", "'='", "'+'", "'-'", "'*'", "'/'",
  "UNARYMINUS", "'^'", "'\\n'", "'('", "')'", "','", "$accept", "list",
  "asgn", "expr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,    61,    43,    45,    42,    47,   269,
      94,    10,    40,    41,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    25,    26,    26,    26,    26,    26,    27,    28,    28,
      28,    28,    28,    28,    28,    28,    28,    28,    28,    28,
      28,    28,    28,    28,    28
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     2,     3,     3,     3,     3,     1,     1,
       1,     1,     1,     4,     6,     8,    10,    12,     3,     3,
       3,     3,     3,     3,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       2,     0,     1,     0,     8,     9,     0,     0,     0,     0,
       0,    11,    10,     0,     3,     0,    12,     0,     6,     0,
       0,     0,     0,     0,     0,    12,    24,     0,     4,     0,
       0,     0,     0,     0,     5,     7,     0,     0,     0,     0,
       0,    23,    18,    19,    20,    21,    22,    13,     0,     0,
       0,     0,     0,     0,     0,     0,    14,     0,     0,     0,
       0,     0,     0,    15,     0,     0,     0,     0,    16,     0,
       0,    17
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     1,    25,    17
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -21
static const short int yypact[] =
{
     -21,    54,   -21,   -20,   -21,   -11,   -18,   -17,    -8,     6,
       9,   -21,   -21,    18,   -21,    18,    11,   216,   -21,    18,
      18,    18,    18,    18,    18,   -21,    13,   162,   -21,    18,
      18,    18,    18,    18,   -21,   223,   171,    62,    72,    82,
      92,   -21,    -5,    -5,    13,    13,    13,   -21,    18,    18,
      18,    18,   180,   102,   112,   122,   -21,    18,    18,    18,
     189,   132,   142,   -21,    18,    18,   198,   152,   -21,    18,
     207,   -21
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -21,   -21,    38,   -13
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      26,    18,    27,    19,    20,    21,    35,    36,    37,    38,
      39,    40,    31,    32,    22,    33,    42,    43,    44,    45,
      46,     4,     5,     6,     7,     8,     9,    10,    23,    11,
      12,    24,    28,    33,    13,    52,    53,    54,    55,    16,
      15,     0,     0,     0,    60,    61,    62,     0,     0,     0,
       0,    66,    67,     0,     2,     3,    70,     4,     5,     6,
       7,     8,     9,    10,     0,    11,    12,     0,     0,     0,
      13,     0,     0,     0,     0,    14,    15,    29,    30,    31,
      32,     0,    33,     0,     0,     0,    48,    29,    30,    31,
      32,     0,    33,     0,     0,     0,    49,    29,    30,    31,
      32,     0,    33,     0,     0,     0,    50,    29,    30,    31,
      32,     0,    33,     0,     0,     0,    51,    29,    30,    31,
      32,     0,    33,     0,     0,     0,    57,    29,    30,    31,
      32,     0,    33,     0,     0,     0,    58,    29,    30,    31,
      32,     0,    33,     0,     0,     0,    59,    29,    30,    31,
      32,     0,    33,     0,     0,     0,    64,    29,    30,    31,
      32,     0,    33,     0,     0,     0,    65,    29,    30,    31,
      32,     0,    33,     0,     0,     0,    69,    29,    30,    31,
      32,     0,    33,     0,     0,    41,    29,    30,    31,    32,
       0,    33,     0,     0,    47,    29,    30,    31,    32,     0,
      33,     0,     0,    56,    29,    30,    31,    32,     0,    33,
       0,     0,    63,    29,    30,    31,    32,     0,    33,     0,
       0,    68,    29,    30,    31,    32,     0,    33,     0,     0,
      71,    29,    30,    31,    32,     0,    33,    34,    29,    30,
      31,    32,     0,    33
};

static const yysigned_char yycheck[] =
{
      13,    21,    15,    14,    22,    22,    19,    20,    21,    22,
      23,    24,    17,    18,    22,    20,    29,    30,    31,    32,
      33,     3,     4,     5,     6,     7,     8,     9,    22,    11,
      12,    22,    21,    20,    16,    48,    49,    50,    51,     1,
      22,    -1,    -1,    -1,    57,    58,    59,    -1,    -1,    -1,
      -1,    64,    65,    -1,     0,     1,    69,     3,     4,     5,
       6,     7,     8,     9,    -1,    11,    12,    -1,    -1,    -1,
      16,    -1,    -1,    -1,    -1,    21,    22,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    -1,    24,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    -1,    24,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    -1,    24,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    -1,    24,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    -1,    24,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    -1,    24,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    -1,    24,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    -1,    24,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    -1,    24,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    -1,    24,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    23,    15,    16,    17,    18,
      -1,    20,    -1,    -1,    23,    15,    16,    17,    18,    -1,
      20,    -1,    -1,    23,    15,    16,    17,    18,    -1,    20,
      -1,    -1,    23,    15,    16,    17,    18,    -1,    20,    -1,
      -1,    23,    15,    16,    17,    18,    -1,    20,    -1,    -1,
      23,    15,    16,    17,    18,    -1,    20,    21,    15,    16,
      17,    18,    -1,    20
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    26,     0,     1,     3,     4,     5,     6,     7,     8,
       9,    11,    12,    16,    21,    22,    27,    28,    21,    14,
      22,    22,    22,    22,    22,    27,    28,    28,    21,    15,
      16,    17,    18,    20,    21,    28,    28,    28,    28,    28,
      28,    23,    28,    28,    28,    28,    28,    23,    24,    24,
      24,    24,    28,    28,    28,    28,    23,    24,    24,    24,
      28,    28,    28,    23,    24,    24,    28,    28,    23,    24,
      28,    23
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()
    ;
#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 5:
#line 291 "../src/manager.y"
    { stack_value = (yyvsp[-1].val); }
    break;

  case 6:
#line 292 "../src/manager.y"
    { yyerrok; }
    break;

  case 7:
#line 294 "../src/manager.y"
    { (yyval.val)=(yyvsp[-2].sym)->u.val=(yyvsp[0].val); (yyvsp[-2].sym)->type = VAR; }
    break;

  case 8:
#line 296 "../src/manager.y"
    { (yyval.val) = (yyvsp[0].val); }
    break;

  case 9:
#line 297 "../src/manager.y"
    { if ((yyvsp[0].sym)->type == UNDEF)
		      execerrnr("undefined variable",(yyvsp[0].sym)->name);
		    (yyval.val) = (yyvsp[0].sym)->u.val; }
    break;

  case 10:
#line 300 "../src/manager.y"
    { (yyval.val) = (double) (yyvsp[0].sym)->u.num; }
    break;

  case 11:
#line 301 "../src/manager.y"
    { (yyval.val) = (yyvsp[0].sym)->u.val; }
    break;

  case 13:
#line 304 "../src/manager.y"
    { (yyval.val) = (*((yyvsp[-3].sym)->u.ptr))((yyvsp[-1].val)); }
    break;

  case 14:
#line 306 "../src/manager.y"
    { (yyval.val) = (*((yyvsp[-5].sym)->u.ptr))((yyvsp[-3].val),(yyvsp[-1].val)); }
    break;

  case 15:
#line 308 "../src/manager.y"
    { (yyval.val) = (*((yyvsp[-7].sym)->u.ptr))((yyvsp[-5].val),(yyvsp[-3].val),(yyvsp[-1].val)); }
    break;

  case 16:
#line 310 "../src/manager.y"
    { (yyval.val) = (*((yyvsp[-9].sym)->u.ptr))((yyvsp[-7].val),(yyvsp[-5].val),(yyvsp[-3].val),(yyvsp[-1].val)); }
    break;

  case 17:
#line 312 "../src/manager.y"
    { (yyval.val) = (*((yyvsp[-11].sym)->u.ptr))((yyvsp[-9].val),(yyvsp[-7].val),(yyvsp[-5].val),(yyvsp[-3].val),(yyvsp[-1].val)); }
    break;

  case 18:
#line 313 "../src/manager.y"
    { (yyval.val) = (yyvsp[-2].val) + (yyvsp[0].val); }
    break;

  case 19:
#line 314 "../src/manager.y"
    { (yyval.val) = (yyvsp[-2].val) - (yyvsp[0].val); }
    break;

  case 20:
#line 315 "../src/manager.y"
    { (yyval.val) = (yyvsp[-2].val) * (yyvsp[0].val); }
    break;

  case 21:
#line 316 "../src/manager.y"
    {
	  if ((yyvsp[0].val) == 0.0){
#if ZERONULLDIV
	    (yyval.val) = 0.0;
#else
	    execerror("division by zero","");
	    (yyval.val) = (yyvsp[-2].val) / (yyvsp[0].val); 
#endif
	  }
	  else{
	    (yyval.val) = (yyvsp[-2].val) / (yyvsp[0].val); 
	  }
	}
    break;

  case 22:
#line 329 "../src/manager.y"
    { (yyval.val) = Pow((yyvsp[-2].val),(yyvsp[0].val)); }
    break;

  case 23:
#line 330 "../src/manager.y"
    { (yyval.val) = (yyvsp[-1].val); }
    break;

  case 24:
#line 331 "../src/manager.y"
    { (yyval.val) = -(yyvsp[0].val); }
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 1580 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
	  char *yyfmt;
	  char const *yyf;
	  static char const yyunexpected[] = "syntax error, unexpected %s";
	  static char const yyexpecting[] = ", expecting %s";
	  static char const yyor[] = " or %s";
	  char yyformat[sizeof yyunexpected
			+ sizeof yyexpecting - 1
			+ ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
			   * (sizeof yyor - 1))];
	  char const *yyprefix = yyexpecting;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 1;

	  yyarg[0] = yytname[yytype];
	  yyfmt = yystpcpy (yyformat, yyunexpected);

	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
		  {
		    yycount = 1;
		    yysize = yysize0;
		    yyformat[sizeof yyunexpected - 1] = '\0';
		    break;
		  }
		yyarg[yycount++] = yytname[yyx];
		yysize1 = yysize + yytnamerr (0, yytname[yyx]);
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
		{
		  if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		    {
		      yyp += yytnamerr (yyp, yyarg[yyi++]);
		      yyf += 2;
		    }
		  else
		    {
		      yyp++;
		      yyf++;
		    }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 333 "../src/manager.y"

	/* end of grammer */

/* --------------------------------------------------------------------- *
 *                                                                       *
 *                              P A R S E R                              *
 *                                                                       *
 * --------------------------------------------------------------------- */

yylex()
{
	int c;

	while((c = *cur_string++) == ' ' || c == '\t');

	if(c == EOF)
		return 0;
	if(c == '.' || isdigit(c)) {                      /* number */
	        char *p;
	        yylval.val = strtod(--cur_string, &p);
		cur_string = p;
		return NUMBER;
	}
	if(isalpha(c)) {                                  /* symbol */
		Symbol *s;
		char sbuf[100], *p = sbuf;

		do
		  *p++ = c;
		while
		  ((c = *cur_string++) != EOF && (isalnum(c) || c == '_')); 

		cur_string--;
		*p = '\0';
		if(!(s=lookup(sbuf))) 
		  s = install(sbuf, UNDEF, 0.);
		yylval.sym = s;
		return (s->type == UNDEF) ? VAR : s->type;
	}

	return c;
}

warning(char *s, char *t)    /* print warning message */
{
  fprintf(stderr,"parser: %s",s);
  if (t)
    fprintf(stderr," %s\n",t);
  else
    fprintf(stderr," in function string %s\n",func_string);
}

yyerror(char *s)      /* called for yacc syntax error */
{
  warning (s, (char *) 0);
}

execerror(char *s, char *t)    /* recover from run-time error */
{
  warning (s,t);
  longjmp (begin,0);
}

execerrnr(char *s, char *t)   /* run-time error, no recovery */
{
  warning(s,t);
  fprintf(stderr,"exiting to system...\n");
  exit(-1);
}

fpecatch()	 /* catch floating point exceptions */
{
  fputs ("speclib: floating point exception\n"
	 "exiting to system...\n", stderr);
  exit  (-1);
}

/* --------------------------------------------------------------------- *
 * Vector/Scalar parser                                                  *
 *                                                                       *
 * The scalar and vector parsers are the interfaces to the arithmetic    *
 * routines.  The scalar parser evaluates a single expression using      *
 * variables that have been defined as PARAM's or VAR's.                 *
 *                                                                       *
 * The vector parser is just a faster way to call the scalar parser.     *
 * Using the vector parser involves two steps: a call to vector_def() to *
 * declare the names of the vectors and the function, and a call to      *
 * vector_set() to evaluate it.                                          *
 *                                                                       *
 * Example:    vector_def ("x y z", "sin(x)*cos(y)*exp(z)");             *
 *             vector_set (100, x, y, z, u);                             *
 *                                                                       *
 * In this example, "x y z" is the space-separated list of vector names  *
 * referenced in the function string "sin(x)...".  The number 100 is the *
 * length of the vectors to be processed.  The function is evaluted as:  *
 *                                                                       *
 *             u[i] = sin(x[i])*cos(y[i])*exp(z[i])                      *
 *                                                                       *
 * --------------------------------------------------------------------- */

#define  VMAX     10    /* maximum number of vectors in a single call */
#define  VLEN   SIZE    /* maximum vector name string length          */

double scalar (char *function)
{
  if (strlen(function) > SIZE-1)
    execerrnr ("Too many characters in function:\n", function);
  
  sprintf (cur_string = func_string, "%s\n", function);
  yyparse ();
  
  return stack_value;
}

double scalar_set (char *name, double val)
{
  Node   *np;
  Symbol *sp;

  if (np = tree_search (Symbols->root, name)) {
    if ((sp = (Symbol*) np->other)->type == VAR)
      sp->u.val = val;
    else
      warning (name, "has a type other than VAR.  Not set.");
  } else
    install (name, VAR, val);

  return val;
}

static int     nvec;
static Symbol *vs[VMAX];
static double *vv[VMAX];

#ifndef VELINTERP
void vector_def (char *vlist, char *function)
{
  Symbol  *s;
  char    *name, buf[VLEN];

  if (strlen(vlist) > SIZE)
    execerrnr("name string is too long:\n", vlist);
  else
    strcpy(buf, vlist);

  /* install the vector names in the symbol table */

  name = strtok(buf, " ");
  nvec = 0;
  while (name && nvec < VMAX) {
    if (!(s=lookup(name))) 
      s = install (name, VAR, 0.);
    vs[nvec++] = s;
    name  = strtok((char*) NULL, " ");
  }

  if (strlen(function) > SIZE-1)
    execerrnr("too many characters in function:\n", function);

  sprintf (func_string, "%s\n", function);

  return;
}

void vector_set (int n, ...)
{
  va_list  ap;
  double   *fv;
  register int i;

  /* initialize the vectors */

  va_start(ap, n);
  for (i = 0; i < nvec; i++) vv[i] = va_arg(ap, double*);
  fv = va_arg(ap, double*);
  va_end(ap);

  /* evaluate the function */

  while (n--) {
    for (i = 0; i < nvec; i++) vs[i]->u.val = *(vv[i]++);    
    cur_string = func_string; 
    yyparse();
    *(fv++)    = stack_value;
  }

  return;
}
#endif
#undef VMAX
#undef VLEN

/* --------------------------------------------------------------------- *
 * Parameters and Options                                                *
 *                                                                       *
 * The following functions simply set and lookup values from the tables  *
 * of variables.   If a symbol isn't found, they silently return zero.   *
 * --------------------------------------------------------------------- */

int iparam (char *name)
{
  Node   *np;
  Symbol *sp;
  int    num = 0;

  if ((np = tree_search (Params->root, name)) &&
      (sp = (Symbol*) np->other)->type == IPARAM)
    num = sp->u.num;

  return num;
}

int iparam_set (char *name, int num)
{
  Node   *np;
  Symbol *sp;

  if (np = tree_search (Params->root, name)) {
    if ((sp = (Symbol*) np->other)->type == IPARAM)
      sp->u.num = num;
    else
      warning (name, "has a type other than IPARAM.  Not set.");
  } else
    install (name, IPARAM, num);

  return num;
}

double dparam (char *name)
{
  Node   *np;
  Symbol *sp;
  double val = 0.;

  if ((np = tree_search (Params->root, name)) &&
      (sp = (Symbol*) np->other)->type == DPARAM)
    val = sp->u.val;

  return val;
}

double dparam_set (char *name, double val)
{
  Node   *np;
  Symbol *sp;

  if (np = tree_search (Params->root, name)) {
    if ((sp = (Symbol*) np->other)->type == DPARAM)
      sp->u.val = val;
    else
      warning (name, "has a type other than DPARAM.  Not set.");
  } else
    install (name, DPARAM, val);

  return val;
}

int option (char *name)
{
  Node *np;
  int   status = 0;
  
  if (np = tree_search (Options->root, name))
    status = ((Symbol*) np->other)->u.num;

  return status;
}

int option_set (char *name, int status)
{
  Node *np;

  if (np = tree_search (Options->root, name))
    ((Symbol*) np->other)->u.num = status;
  else
    install (name, OPTION, status);
  
  return status;
}

/* --------------------------------------------------------------------- *
 * manager_init() -- Initialize the parser                               *
 *                                                                       *
 * The following function must be called before any other parser func-   *
 * tions to install the symbol tables and builtin functions.             *
 * --------------------------------------------------------------------- */

void manager_init (void)
{
  register int i;

  /* initialize the trees */

  Symbols = create_tree (show_symbol, free);
  Options = create_tree (show_symbol, free);
  Params  = create_tree (show_symbol, free);

  /* initialize the signal manager */

  setjmp(begin);
  signal(SIGFPE, (void(*)()) fpecatch);

  /* options and parameters */

  for(i = 0; O_default[i].name; i++)
     install(O_default[i].name,OPTION,O_default[i].oval);
  for(i = 0; I_default[i].name; i++) 
     install(I_default[i].name,IPARAM,I_default[i].pval);
  for(i = 0; D_default[i].name; i++)
     install(D_default[i].name,DPARAM,D_default[i].pval);

  /* constants and built-ins */

  for(i = 0; consts[i].name; i++)
    install (consts[i].name,VAR,consts[i].cval);
  for(i = 0; builtins[i].name; i++) {
    switch  (builtins[i].args) {
    case 1:
      install (builtins[i].name, BLTIN_UNARY, builtins[i].func);
      break;
    case 2:
      install (builtins[i].name, BLTIN_BINARY, builtins[i].func);
      break;
    case 3:
      install (builtins[i].name, BLTIN_TRINARY, builtins[i].func);
      break;
    case 4:
      install (builtins[i].name, BLTIN_QUATERNARY, builtins[i].func);
      break;
    case 5:
      install (builtins[i].name, BLTIN_QUINTIC, builtins[i].func);
      break;
    default:
      execerrnr ("too many arguments for builtin:", builtins[i].name);
      break;
    }
  }
  
  return;
}

/* Print parameter, option, and symbol tables */

void show_symbols(void) { puts ("\nSymbol table:"); tree_walk (Symbols); }
void show_options(void) { puts ("\nOptions:")     ; tree_walk (Options); }
void show_params (void) { puts ("\nParameters:")  ; tree_walk (Params);  }

/* Print a Symbol */

void show_symbol (Symbol *s)
{
  printf ("%-15s -- ", s->name);
  switch (s->type) {
  case OPTION:
  case IPARAM:
    printf ("%d\n", s->u.num);
    break;
  case DPARAM:
  case VAR:
    printf ("%g\n", s->u.val);
    break;
  default:
    puts   ("unprintable");
    break;
  }
  return;
}

/* ..........  Symbol Table Functions  .......... */

static Symbol *lookup (char *key)
{
  Node *np;

  if (np = tree_search (Symbols->root, key))
    return (Symbol*) np->other;

  if (np = tree_search (Params ->root, key))
    return (Symbol*) np->other;

  if (np = tree_search (Options->root, key))
    return (Symbol*) np->other;

  return (Symbol*) NULL;     /* not found */
}                  

/* 
 * install "key" in a symbol table 
 */

static Symbol *install (char *key, int type, ...)     
{
  Node   *np;
  Symbol *sp;
  Tree   *tp;
  va_list ap;

  va_start (ap, type);
  
  /* Get a node for this key and create a new symbol */

  np       = create_node (key);
  sp       = (Symbol *) malloc(sizeof(Symbol));
  sp->name = np->name;

  switch (sp->type = type) {
  case OPTION:
    tp        = Options;
    sp->u.num = va_arg(ap, int);
    break;
  case IPARAM:
    tp        = Params;
    sp->u.num = va_arg(ap, int);
    break;
  case DPARAM:
    tp        = Params;
    sp->u.val = va_arg(ap, double);
    break;
  case VAR: 
  case UNDEF:  
    tp        = Symbols;
    sp->u.val = va_arg(ap, double);
    break;
  case BLTIN_UNARY:
  case BLTIN_BINARY:
  case BLTIN_TRINARY:
  case BLTIN_QUATERNARY:
  case BLTIN_QUINTIC:
    tp        = Symbols;
    sp->u.ptr = va_arg(ap, PFD);
    break;
  default:
    tp        = Symbols;
    sp->u.val = va_arg(ap, double);
    sp->type  = UNDEF;
    break;
  }

  va_end (ap);

  np->other = (void *) sp;     /* Save the symbol */
  tree_insert (tp, np);        /* Insert the node */

  return sp;
}

/*
 *  Math Functions
 *  --------------  */

static double errcheck (double d, char *s)
{
  if (errno == EDOM) {
    errno = 0                              ;
    execerror(s, "argument out of domain") ;
  }
  else if (errno == ERANGE) {
    errno = 0                           ;
    execerror(s, "result out of range") ;
  }
  return d;
}

static double Log (double x)
{
  return errcheck(log(x), "log") ;
}

static double Log10 (double x) 
{
  return errcheck(log10(x), "log10") ;
}

static double Exp (double x)
{
  if(x<-28.)
    return 0.;
  
  return errcheck(exp(x), "exp") ;
}

static double Sqrt (double x)
{
  return errcheck(sqrt(x), "sqrt") ;
}

static double Pow (double x, double y)
{
  const
  double yn = floor(y + .5);
  double px = 1.;

  if (yn >= 0 && yn == y) {     /* Do it inline if y is an integer power */
      register int n = yn;
      while (n--) 
         px *= x;
  } else  
      px = errcheck (pow(x,y), "exponentiation");

  return px;
}

static double Integer (double x)
{
  return (double) (long) x;
} 

static double Mod (double x)
{
  double tmp;
  return (double) modf(x,&tmp);
} 

static double Rand (double x)
{
  return x * drand();
}


static double Bump (double x)
{
  if(x >= 0. && x < .125)
    return -1;
  if(x >= 0.125 && x < .25)
    return 0.;
  if(x >= 0.25 && x < .375)
    return 1.;
  if(x >= 0.375 && x <= .5)
    return 0.;

  return -9999.;
}



static double Radius (double x, double y)
{
  if (x != 0. || y != 0.)
    return sqrt (x*x + y*y);
  else
    return 0.;
}

static double Jn (double i, double x)
{
    return jn((int)i, x);
}

static double Yn (double i, double x)
{
    return yn((int)i, x);
}


static double ReJn (double n, double x,  double y)
{
  double rej, imj;
  int nz,ierr;
  
  zbesj(&x,&y,n,1,1,&rej,&imj,&nz,&ierr);
  return rej;
}

static double ImJn (double n, double x, double y)
{
  double rej, imj;
  int nz,ierr;
  
  zbesj(&x,&y,n,1,1,&rej,&imj,&nz,&ierr);
  return imj;
}

/* Calcualte the Womersley solution at r for a pipe of radius R and
   wave number wnum.  The solution is assumed to be set so that the
   spatail mean fo the flow satisfies u_avg(r) = A cos (wnum t) + B
   sin(wnum t) 
*/

static double Womersley(double A,double B,double r,double R,double mu, 
			double wnum,double t){
  
  double x,y;

  if(r > R) fprintf(stderr,"Error in manager.y: Womersley - r > R\n");

  if(wnum == 0) /* return poseuille flow  with mean of 1.*/
    return 2*(1-r*r/R/R);
  else{
    int    ierr,nz;
    double cr,ci,J0r,J0i,rej,imj,re,im,fac;
    double isqrt2 = 1.0/sqrt(2.0);
    static double R_str, wnum_str,mu_str;
    static double Jr,Ji,alpha,j0r,j0i, isqrt;
    

    /* for case of repeated calls to with same parameters look to store 
       parameters independent of r. */
    if((R != R_str)||(wnum != wnum_str)||(mu != mu_str)){
      double retmp[2],imtmp[2];
      alpha = R*sqrt(wnum/mu);

      re  = -alpha*isqrt2;
      im  =  alpha*isqrt2;
      zbesj(&re,&im,0,1,2,retmp,imtmp,&nz,&ierr);
      j0r = retmp[0]; j0i = imtmp[0];
      rej = retmp[1]; imj = imtmp[1];

      fac = 1/(j0r*j0r+j0i*j0i);
      Jr  = 1 + 2*fac/alpha*((rej*j0r+imj*j0i)*isqrt2 - (imj*j0r - rej*j0i)*isqrt2);
      Ji  = 2*fac/alpha*((rej*j0r+imj*j0i)*isqrt2 + (imj*j0r - rej*j0i)*isqrt2);

      R_str = R; wnum_str = wnum; mu_str = mu;
    }

    /* setup cr, ci from pre-stored value of Jr & Ji */
    fac = 1/(Jr*Jr + Ji*Ji);
    cr  =  (A*Jr - B*Ji)*fac;
    ci  = -(A*Ji + B*Jr)*fac;
    
    /* setup J0r, J0i */
    re  = -alpha*isqrt2*r/R;
    im  =  alpha*isqrt2*r/R;
    zbesj(&re,&im,0,1,1,&rej,&imj,&nz,&ierr);
    fac = 1/(j0r*j0r+j0i*j0i);
    J0r = 1-fac*(rej*j0r+imj*j0i);
    J0i = -fac*(imj*j0r-rej*j0i); 

    /* return solution */
    return (cr*J0r - ci*J0i)*cos(wnum*t) - (ci*J0r + cr*J0i)*sin(wnum*t);
  }
}

static double Womsin(double r, double R, double mu, double wnum, double t){
  if(wnum == 0) /* no sin term for zeroth mode */
    return 0;
  else
    return Womersley(0,1,r,R,mu,wnum,t);
}

static double Womcos(double r, double R, double mu, double wnum, double t){
  return Womersley(1,0,r,R,mu, wnum,t);
}

#ifndef M_PI
#define M_PI  consts[0].cval
#endif

static double Angle (double x, double y)
{
  double theta = 0.;

  if ((x != 0.)||(y != 0.))
    theta =  atan2 (y,x);

  return theta;
}


/* Heaviside step function H(x-a) =1 if x >= a else =0 */
static double Step (double x, double a)
{
  double H = 1.0;
  if (x < a)
    H = 0.0;

  return H;
}

/* Heaviside step function H(x-a) =1 if x > a else =0 */ 
static double Step2 (double x, double a)
{
  double H = 1.0;
  if (x <= a)
    H = 0.0;

  return H;
} 

static double Shock(double x, double a, double b)
{
  if(x==0)
    return 0.5*(a+b);
  if(x>0)
    return b;
  if(x<0)
    return a;
  return 0;
}


/* -----------------------------------------------------------------
   jacobi() - jacobi polynomials 
   
   Get a vector 'poly' of values of the n_th order Jacobi polynomial
   P^(alpha,beta)_n(z) alpha > -1, beta > -1 at the z
   ----------------------------------------------------------------- */

static double Jacobi(double z, double n, double alpha, double beta){

  register int i,k;
  double  one = 1.0;
  double   a1,a2,a3,a4;
  double   two = 2.0, apb = alpha + beta;
  double   poly, polyn1,polyn2;
  
  polyn2 = one;
  polyn1 = 0.5*(alpha - beta + (alpha + beta + 2)*z);
  
  for(k = 2; k <= n; ++k){
    a1 =  two*k*(k + apb)*(two*k + apb - two);
    a2 = (two*k + apb - one)*(alpha*alpha - beta*beta);
    a3 = (two*k + apb - two)*(two*k + apb - one)*(two*k + apb);
    a4 =  two*(k + alpha - one)*(k + beta - one)*(two*k + apb);
    
    a2 /= a1;
    a3 /= a1;
    a4 /= a1;
    
    poly   = (a2 + a3*z)*polyn1 - a4*polyn2;
    polyn2 = polyn1;
    polyn1 = poly  ;
  }

  return poly;
}


#if 1
static double Single(double x, double y)
{
#if 1
  double gamma = 64.0*64.0;
  double tmp;

  if (y>=3.0 && y<=4.0)
    {
      tmp = (y-3.)*(y-4.)*(y-3.)*(y-4.);
      if (x>=1.0 && x<=2.)
	return gamma*(x-1.0)*(x-2.)*(x-1.0)*(x-2.);
      if(x>=3.0 && x<=4.0)
	return gamma*(x-3.0)*(x-4.0)*(x-3.0)*(x-4.0);
      if(x>=5. && x<=6.) 
	return gamma*(x-5.0)*(x-6.)*(x-5.0)*(x-6.);
    }
#else
  double gamma = 64.0*64.0;
  double xa,xb,ya,yb;

  if(x>1. && x<2. && y>1. && y<2.){
    xa = 1.;    xb = 2.;
    ya = 1.;    yb = 2.;
  }
  else if(x>3. && x<4. && y>3. && y<4.){
    xa = 3.;    xb = 4.;
    ya = 3.;    yb = 4.;
  }
  else if(x>5. && x<6. && y>5. && y<6.){
    xa = 5.;    xb = 6.;
    ya = 5.;    yb = 6.;
  }
  else 
    return 0.;

  return gamma*(x-xa)*(x-xa)*(x-xb)*(x-xb)*(y-ya)*(y-ya)*(y-yb)*(y-yb);
#endif
}
#endif

/*
static double UserdefA(double x, double y, double z){
  return doUserdefA(x,y,z);
}
*/
  
#undef M_PI






