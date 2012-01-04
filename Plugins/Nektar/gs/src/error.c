/**********************************error.c*************************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
6.21.97
***********************************error.c************************************/

/**********************************error.c*************************************
File Description:
-----------------

***********************************error.c************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if   defined NXSRC
#ifndef DELTA
#include <nx.h>
#endif

#elif defined MPISRC
#include <mpi.h>

#endif

#if   defined NXSRC
#include "const.h"
#include "types.h"
#include "error.h"
#include "comm.h"


#elif defined MPISRC
#include <mpi.h>
#include "const.h"
#include "types.h"
#include "error.h"
#include "comm.h"

#else
#include "const.h"
#include "error.h"

static int my_id=0;

#endif


void
error_msg_fatal_ (char *msg)
{
  error_msg_fatal(msg);
}



/**********************************error.c*************************************
Function error_msg_fatal()

Input : pointer to formatted error message.
Output: prints message to stdout.
Return: na.
Description: prints error message and terminates program.
***********************************error.c************************************/
void
error_msg_fatal(char *msg, ...)
{
  va_list ap;
  char *p, *sval, cval;
  int ival;
  REAL dval;


  /* print error message along w/node identifier */
  va_start(ap,msg);
  printf("%d :: FATAL :: ", my_id);
  for (p=msg; *p; p++)
    {
      if (*p != '%')
  {
    putchar(*p);
    continue;
  }
      switch (*++p) {
      case 'c':
  cval = va_arg(ap,int);
    putchar(cval);
  break;
      case 'd':
  ival = va_arg(ap,int);
  printf("%d",ival);
  break;
      case 'e':
  dval = va_arg(ap,REAL);
  printf("%e",dval);
  break;
      case 'f':
  dval = va_arg(ap,REAL);
  printf("%f",dval);
  break;
      case 'g':
  dval = va_arg(ap,REAL);
  printf("%g",dval);
  break;
      case 's':
  for (sval=va_arg(ap,char *); *sval; sval++)
    {putchar(*sval);}
  break;
      default:
  putchar(*p);
  break;
      }
    }
  /* printf("\n"); */
  va_end(ap);

#ifdef DELTA
  fflush(stdout);
#else
  fflush(stdout);
  /*  fflush(NULL); */
#endif


  /* exit program */
#if   defined NXSRC
  abort();


#elif defined MPISRC
  /* Try with MPI_Finalize() as well _only_ if all procs call this routine */
  /* Choose a more meaningful error code than -12 */
  MPI_Abort(MPI_COMM_WORLD, -12);

#else
  exit(1);


#endif
}



/**********************************error.c*************************************
Function error_msg_warning()

Input : formatted string and arguments.
Output: conversion printed to stdout.
Return: na.
Description: prints error message.
***********************************error.c************************************/
void
error_msg_warning(char *msg, ...)
{
  va_list ap;
  char *p, *sval, cval;
  int ival;
  REAL dval;


  /* print error message along w/node identifier */
#if   defined V
  va_start(ap,msg);
  if (!my_id)
    {
      printf("%d :: WARNING :: ", my_id);
      for (p=msg; *p; p++)
  {
    if (*p != '%')
      {
        putchar(*p);
        continue;
      }
    switch (*++p) {
    case 'c':
      cval = va_arg(ap,char);
      putchar(cval);
      break;
    case 'd':
      ival = va_arg(ap,int);
      printf("%d",ival);
      break;
    case 'e':
      dval = va_arg(ap,REAL);
      printf("%e",dval);
      break;
    case 'f':
      dval = va_arg(ap,REAL);
      printf("%f",dval);
      break;
    case 'g':
      dval = va_arg(ap,REAL);
      printf("%g",dval);
      break;
    case 's':
      for (sval=va_arg(ap,char *); *sval; sval++)
        {putchar(*sval);}
      break;
    default:
      putchar(*p);
      break;
    }
  }
      /*      printf("\n"); */
    }
  va_end(ap);


#elif defined VV
  va_start(ap,msg);
  if (my_id>=0)
    {
      printf("%d :: WARNING :: ", my_id);
      for (p=msg; *p; p++)
  {
    if (*p != '%')
      {
        putchar(*p);
        continue;
      }
    switch (*++p) {
    case 'c':
      cval = va_arg(ap,char);
      putchar(cval);
      break;
    case 'd':
      ival = va_arg(ap,int);
      printf("%d",ival);
      break;
    case 'e':
      dval = va_arg(ap,REAL);
      printf("%e",dval);
      break;
    case 'f':
      dval = va_arg(ap,REAL);
      printf("%f",dval);
      break;
    case 'g':
      dval = va_arg(ap,REAL);
      printf("%g",dval);
      break;
    case 's':
      for (sval=va_arg(ap,char *); *sval; sval++)
        {putchar(*sval);}
      break;
    default:
      putchar(*p);
      break;
    }
  }
      /* printf("\n"); */
    }
  va_end(ap);
#endif

#ifdef DELTA
  fflush(stdout);
#else
  fflush(stdout);
  /*  fflush(NULL); */
#endif

}
