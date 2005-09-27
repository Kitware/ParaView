/* Id
 *
 * stm_types.h - include file for spy_file.h
 *
 * David Crawford
 * Computational Physics
 * Sandia National Laboratories
 * Albuquerque, New Mexico 87185
 *
 */

#ifndef __stm_types_h__
#define __stm_types_h__

#ifdef DBL
typedef double Real;
#else
typedef float Real;
#endif

typedef unsigned int    UInt;
typedef int             Int;
typedef short           Short;
typedef int             Boolean;

#define TRUE 1
#define FALSE 0

/* Raster output modes */
#define GIF_MODE 10
#define PPM_MODE 20
#define RGB_MODE 30
#define PS_MODE  40
#define ZB_MODE  50
#define JPG_MODE 60

#endif /* __stm_types_h__ */
