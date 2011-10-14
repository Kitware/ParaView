/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/include/memcheck.h,v $
 * $Revision: 1.3 $
 * $Date: 2006/05/13 09:15:23 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#ifndef MEMCHECK_H
#define MEMCHECK_H

#include <sys/types.h>
#if 0
#include <malloc.h>
#endif

#define memcheck { char *t = (char*) malloc(1); free(t); fprintf(stderr,\
 "Made it passed %d in file %s \n",__LINE__,__FILE__); fflush(stderr);}
#ifdef NOT
#define memspace { struct mallinfo minfo =  mallinfo();fprintf(stderr,\
 "In %s at line %d, arena : %d, small space in use: %d \n", \
  __FILE__,__LINE__,minfo.arena,minfo.usmblks);}
#endif
#endif
