/*
 * tclMacMath.h --
 *
 *	This file is necessary because of Metrowerks CodeWarrior Pro 1
 *	on the Macintosh. With 8-byte doubles turned on, the definitions of
 *	sin, cos, acos, etc., are screwed up.  They are fine as long as
 *	they are used as function calls, but if the function pointers
 *	are passed around and used, they will crash hard on the 68K.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TCLMACMATH
#define _TCLMACMATH

#include <math.h>

#if defined(__MWERKS__) && !defined(__POWERPC__)
#if __option(IEEEdoubles)

#   ifdef cos
#	undef cos
#	define cos cosd
#   endif

#   ifdef sin
#	undef sin
#	define sin sind
#   endif

#   ifdef tan
#	undef tan
#	define tan tand
#   endif

#   ifdef acos
#	undef acos
#	define acos acosd
#   endif

#   ifdef asin
#	undef asin
#	define asin asind
#   endif

#   ifdef atan
#	undef atan
#	define atan atand
#   endif

#   ifdef cosh
#	undef cosh
#	define cosh coshd
#   endif

#   ifdef sinh
#	undef sinh
#	define sinh sinhd
#   endif

#   ifdef tanh
#	undef tanh
#	define tanh tanhd
#   endif

#   ifdef exp
#	undef exp
#	define exp expd
#   endif

#   ifdef ldexp
#	undef ldexp
#	define ldexp ldexpd
#   endif

#   ifdef log
#	undef log
#	define log logd
#   endif

#   ifdef log10
#	undef log10
#	define log10 log10d
#   endif

#   ifdef fabs
#	undef fabs
#	define fabs fabsd
#   endif

#   ifdef sqrt
#	undef sqrt
#	define sqrt sqrtd
#   endif

#   ifdef fmod
#	undef fmod
#	define fmod fmodd
#   endif

#   ifdef atan2
#	undef atan2
#	define atan2 atan2d
#   endif

#   ifdef frexp
#	undef frexp
#	define frexp frexpd
#   endif

#   ifdef modf
#	undef modf
#	define modf modfd
#   endif

#   ifdef pow
#	undef pow
#	define pow powd
#   endif

#   ifdef ceil
#	undef ceil
#	define ceil ceild
#   endif

#   ifdef floor
#	undef floor
#	define floor floord
#   endif
#endif
#endif

#if (defined(THINK_C) || defined(__MWERKS__))
#pragma export on
double		hypotd(double x, double y);
#define hypot hypotd
#pragma export reset
#endif

#endif /* _TCLMACMATH */
