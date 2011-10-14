
/*
 * Copyright (c) 1997 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to use, copy, modify, and distribute the Software without
 * restriction, provided the Software, including any modified copies made
 * under this license, is not distributed for a fee, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE MASSACHUSETTS INSTITUTE OF TECHNOLOGY BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the Massachusetts
 * Institute of Technology shall not be used in advertising or otherwise
 * to promote the sale, use or other dealings in this Software without
 * prior written authorization from the Massachusetts Institute of
 * Technology.
 *
 */

/* fftw.h -- system-wide definitions */
/* $Id: fftw.h,v 1.1 2004/01/27 23:47:34 ssherw Exp $ */

#ifndef FFTW_H
#define FFTW_H

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif        /* __cplusplus */

/* our real numbers */
typedef double FFTW_REAL;

/*********************************************
 * Complex numbers and operations
 *********************************************/
typedef struct {
     FFTW_REAL re, im;
} FFTW_COMPLEX;

#define c_re(c)  ((c).re)
#define c_im(c)  ((c).im)

typedef enum {
     FFTW_FORWARD = -1, FFTW_BACKWARD = 1
} fftw_direction;

#ifndef FFTW_1_0_COMPATIBILITY
#define FFTW_1_0_COMPATIBILITY 1
#endif

#if FFTW_1_0_COMPATIBILITY
/* backward compatibility with FFTW-1.0 */
#define REAL FFTW_REAL
#define COMPLEX FFTW_COMPLEX
#endif

/*********************************************
 * Success or failure status
 *********************************************/

typedef enum {
     FFTW_SUCCESS = 0, FFTW_FAILURE = -1
} fftw_status;

/*********************************************
 *              Codelets
 *********************************************/
/*
 * There are two kinds of codelets:
 *
 * NO_TWIDDLE    computes the FFT of a certain size, operating
 *               out-of-place (i.e., take an input and produce a
 *               separate output)
 *
 * TWIDDLE       like no_twiddle, but operating in place.  Moreover,
 *               multiplies the input by twiddle factors.
 */

typedef void (notw_codelet) (const FFTW_COMPLEX *, FFTW_COMPLEX *, int, int);
typedef void (twiddle_codelet) (FFTW_COMPLEX *, const FFTW_COMPLEX *, int,
        int, int);
typedef void (generic_codelet) (FFTW_COMPLEX *, const FFTW_COMPLEX *, int,
        int, int, int);

/*********************************************
 *     Configurations
 *********************************************/
/*
 * A configuration is a database of all known codelets
 */

typedef struct {
     int size;      /* size of the problem */
     int signature;    /* unique codelet id */
     notw_codelet *codelet;  /*
         * pointer to the codelet that solves
         * the problem
         */
} config_notw;

extern config_notw fftw_config_notw[];
extern config_notw fftwi_config_notw[];

typedef struct {
     int size;      /* size of the problem */
     int signature;    /* unique codelet id */
     twiddle_codelet *codelet;
} config_twiddle;

extern config_twiddle fftw_config_twiddle[];
extern config_twiddle fftwi_config_twiddle[];

extern generic_codelet fftw_twiddle_generic;
extern generic_codelet fftwi_twiddle_generic;
extern char *fftw_version;

/*****************************
 *        Plans
 *****************************/
/*
 * A plan is a sequence of reductions to compute a FFT of
 * a given size.  At each step, the FFT algorithm can:
 *
 * 1) apply a notw codelet, or
 * 2) recurse and apply a twiddle codelet, or
 * 3) apply the generic codelet.
 */

enum fftw_node_type {
     FFTW_NOTW, FFTW_TWIDDLE, FFTW_GENERIC
};

/* structure that contains twiddle factors */
typedef struct fftw_twiddle_struct {
     int n;
     int r;
     int m;
     FFTW_COMPLEX *twarray;
     struct fftw_twiddle_struct *next;
     int refcnt;
} fftw_twiddle;

/* structure that holds all the data needed for a given step */
typedef struct fftw_plan_node_struct {
     enum fftw_node_type type;

     union {
    /* nodes of type FFTW_NOTW */
    struct {
         int size;
         notw_codelet *codelet;
    } notw;

    /* nodes of type FFTW_TWIDDLE */
    struct {
         int size;
         twiddle_codelet *codelet;
         fftw_twiddle *tw;
         struct fftw_plan_node_struct *recurse;
    } twiddle;

    /* nodes of type FFTW_GENERIC */
    struct {
         int size;
         generic_codelet *codelet;
         fftw_twiddle *tw;
         struct fftw_plan_node_struct *recurse;
    } generic;

     } nodeu;

     int refcnt;
} fftw_plan_node;

struct fftw_plan_struct {
     int n;
     fftw_direction dir;
     fftw_plan_node *root;

     double cost;
     int flags;

     enum fftw_node_type wisdom_type;
     int wisdom_signature;

     struct fftw_plan_struct *next;
     int refcnt;
};

/* a plan is just an array of instructions */
typedef struct fftw_plan_struct *fftw_plan;

/* flags for the planner */
#define  FFTW_ESTIMATE (0)
#define  FFTW_MEASURE  (1)

#define FFTW_IN_PLACE (8)
#define FFTW_USE_WISDOM (16)

extern fftw_plan fftw_create_plan(int n, fftw_direction dir, int flags);
extern fftw_twiddle *fftw_create_twiddle(int n, int r, int m);
extern void fftw_destroy_twiddle(fftw_twiddle * tw);
extern void fftw_print_plan(fftw_plan plan);
extern void fftw_destroy_plan(fftw_plan plan);
extern void fftw_naive(int n, FFTW_COMPLEX *in, FFTW_COMPLEX *out);
extern void fftwi_naive(int n, FFTW_COMPLEX *in, FFTW_COMPLEX *out);
void fftw(fftw_plan plan, int howmany, FFTW_COMPLEX *in, int istride,
    int idist, FFTW_COMPLEX *out, int ostride, int odist);
extern double fftw_measure_runtime(fftw_plan plan);
extern void fftw_die(char *s);
extern void *fftw_malloc(size_t n);
extern void fftw_free(void *p);
extern void fftw_check_memory_leaks(void);
extern void fftw_strided_copy(int, FFTW_COMPLEX *, int, FFTW_COMPLEX *);
extern void fftw_executor_simple(int, const FFTW_COMPLEX *, FFTW_COMPLEX *,
         fftw_plan_node *, int, int);
extern void *(*fftw_malloc_hook) (size_t n);
extern void (*fftw_free_hook) (void *p);

/* Wisdom: */
#define FFTW_HAS_WISDOM /* define this symbol so that we know we are using
         a version of FFTW with wisdom */
extern void fftw_forget_wisdom(void);
extern void fftw_export_wisdom(void (*emitter)(char c, void *), void *data);
extern fftw_status fftw_import_wisdom(int (*g)(void *), void *data);
extern void fftw_export_wisdom_to_file(FILE *output_file);
extern fftw_status fftw_import_wisdom_from_file(FILE *input_file);
extern char *fftw_export_wisdom_to_string(void);
extern fftw_status fftw_import_wisdom_from_string(const char *input_string);

/*
 * define symbol so we know this function is available (it is not in
 * older FFTWs)
 */
#define FFTW_HAS_FPRINT_PLAN
extern void fftw_fprint_plan(FILE * f, fftw_plan plan);

/* Returns 1 if FFTW is working.  Otherwise, its value is undefined: */
#define is_fftw_working() 1

/*****************************
 *    N-dimensional code
 *****************************/
typedef struct {
     int is_in_place;    /* 1 if for in-place FFT's, 0 otherwise */
     int rank;      /*
         * the rank (number of dimensions) of the
         * array to be FFT'ed
         */
     int *n;      /*
         * the dimensions of the array to the
         * FFT'ed
         */
     int *n_before;    /*
         * n_before[i] = product of n[j] for j < i
         */
     int *n_after;    /* n_after[i] = product of n[j] for j > i */
     fftw_plan *plans;    /* fftw plans for each dimension */
     FFTW_COMPLEX *work;  /*
         * work array for FFT when doing
         * "in-place" FFT
         */
} fftwnd_aux_data;

typedef fftwnd_aux_data *fftwnd_plan;

/* Initializing the FFTWND Auxiliary Data */
fftwnd_plan fftw2d_create_plan(int nx, int ny, fftw_direction dir, int flags);
fftwnd_plan fftw3d_create_plan(int nx, int ny, int nz,
             fftw_direction dir, int flags);
fftwnd_plan fftwnd_create_plan(int rank, const int *n, fftw_direction dir,
             int flags);

/* Freeing the FFTWND Auxiliary Data */
void fftwnd_destroy_plan(fftwnd_plan plan);

/* Computing the N-Dimensional FFT */
void fftwnd(fftwnd_plan plan, int howmany,
      FFTW_COMPLEX *in, int istride, int idist,
      FFTW_COMPLEX *out, int ostride, int odist);

/****************************************************************************/
/********************************** Timers **********************************/
/****************************************************************************/

/*
 * Here, you can use all the nice timers available in your machine.
 */

/*
 *
   Things you should define to include your own clock:

   fftw_time -- the data type used to store a time

   extern fftw_time fftw_get_time(void);
         -- a function returning the current time.  (We have
            implemented this as a macro in most cases.)

   extern fftw_time fftw_time_diff(fftw_time t1, fftw_time t2);
         -- returns the time difference (t1 - t2).
      If t1 < t2, it may simply return zero (although this
            is not required).  (We have implemented this as a macro
      in most cases.)

   extern double fftw_time_to_sec(fftw_time t);
         -- returns the time t expressed in seconds, as a double.
      (Implemented as a macro in most cases.)

   FFTW_TIME_MIN -- a double-precision macro holding the minimum
         time interval (in seconds) for accurate time measurements.
         This should probably be at least 100 times the precision of
         your clock (we use even longer intervals, to be conservative).
   This will determine how long the planner takes to measure
   the speeds of different possible plans.

   Bracket all of your definitions with an appropriate #ifdef so that
   they will be enabled on your machine.  If you do add your own
   high-precision timer code, let us know (at fftw@theory.lcs.mit.edu).

   Only declarations should go in this file.  Any function definitions
   that you need should go into timer.c.
*/

/* define a symbol so that we know that we have the fftw_time_diff
   function/macro (it did not exist prior to FFTW 1.2) */
#define FFTW_HAS_TIME_DIFF

#ifdef SOLARIS

/* we use the nanosecond virtual timer */
#include <sys/time.h>

typedef hrtime_t fftw_time;

#define fftw_get_time() gethrtime()
#define fftw_time_diff(t1,t2) ((t1) - (t2))
#define fftw_time_to_sec(t) ((double) t / 1.0e9)

/*
 * a measurement is valid if it runs for at least
 * FFTW_TIME_MIN seconds.
 */
#define FFTW_TIME_MIN (1.0e-4)   /* for Solaris nanosecond timer */

#endif /* SOLARIS */

#if defined(MAC) || defined(macintosh)

/* Use Macintosh Time Manager routines (maximum resolution is about 20
   microseconds). */

typedef struct fftw_time_struct {
     unsigned long hi,lo;
} fftw_time;

extern fftw_time get_Mac_microseconds(void);

#define fftw_get_time() get_Mac_microseconds()

/* define as a function instead of a macro: */
extern fftw_time fftw_time_diff(fftw_time t1, fftw_time t2);

#define fftw_time_to_sec(t) ((t).lo * 1.0e-6 + 4294967295.0e-6 * (t).hi)

/* very conservative, since timer should be accurate to 20e-6: */
/* (although this seems not to be the case in practice) */
#define FFTW_TIME_MIN (5.0e-2)   /* for MacOS Time Manager timer */

#endif /* Macintosh */

#ifdef __WIN32__

#include <time.h>

typedef unsigned long fftw_time;
extern unsigned long GetPerfTime(void);
extern double GetPerfSec(double ticks);

#define fftw_get_time() GetPerfTime()
#define fftw_time_diff(t1,t2) ((t1) - (t2))
#define fftw_time_to_sec(t) GetPerfSec(t)

#define FFTW_TIME_MIN (5.0e-2)   /* for Win32 timer */
#endif /* __WIN32__ */

#if defined(_CRAYMPP)    /* Cray MPP system */

double SECONDR(void);    /*
         * I think you have to link with -lsci to
         * get this
         */

typedef double fftw_time;
#define fftw_get_time() SECONDR()
#define fftw_time_diff(t1,t2) ((t1) - (t2))
#define fftw_time_to_sec(t) (t)

#define FFTW_TIME_MIN (1.0e-1)   /* for Cray MPP SECONDR timer */

#endif /* _CRAYMPP */

/***********************************************
 * last resort: good old Unix clock()
 ***********************************************/
#ifndef FFTW_TIME_MIN
#include <time.h>

typedef clock_t fftw_time;

#ifndef CLOCKS_PER_SEC
#ifdef sun
     /* stupid sunos4 prototypes */
#define CLOCKS_PER_SEC 1000000
extern long clock(void);
#else        /* not sun, we don't know CLOCKS_PER_SEC */
#error Please define CLOCKS_PER_SEC
#endif
#endif

#define fftw_get_time() clock()
#define fftw_time_diff(t1,t2) ((t1) - (t2))
#define fftw_time_to_sec(t) (((double) (t)) / CLOCKS_PER_SEC)

/*
 * ***VERY*** conservative constant: this says that a
 * measurement must run for 200ms in order to be valid.
 * You had better check the manual of your machine
 * to discover if it can do better than this
 */
#define FFTW_TIME_MIN (2.0e-1)   /* for default clock() timer */

#endif /* UNIX clock() */

/****************************************************************************/

#ifdef __cplusplus
}                               /* extern "C" */
#endif        /* __cplusplus */

#endif        /* FFTW_H */
