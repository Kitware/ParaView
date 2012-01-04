/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/include/dbutils.h,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/13 09:15:23 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
/*
 * General debugging routines
 */

#ifndef DBUTILS_H
#define DBUTILS_H
#include <time.h>

typedef struct telapse{
  double init;   /* intialalisation call to clock()           */
  double diff;   /* difference (sec) of last stop/start call  */
  double accum;  /* accumalative time of all calls (sec)      */
} Telapse;
#define _MAX_TIMECALL 10

/* functions in Dbutils.c*/
void  init_debug   (void);
void  plotdvector  (double *, int , int);
void  plotdmatrix  (double **, int , int , int , int );
void  showdmatrix  (double **, int , int , int , int);
void  showdvector  (double *,  int , int );
void  showivector  (int    *,  int , int );
void  showfield    (Element *);
void  showcoord    (Cmodes  *,  int  );
void  change_state (Element *, char );
void  pmem         (void);
void  plotsystem   (double *a, int nsolve, int bwidth);

void  dump_sc       (int , int , double **, double **, double **);
void  shownum(Element_List *EL);
void  showbcs(Bndry *Ubc);
void showbsys(Bsystem *B, Element_List *EL);
void Free_dbx(void *pter);


void shownormals(Element_List *EL);
/* time test calls */


#define startclock(a) timetest[a].init = (double) clock();
#define stopclock(a) {timetest[a].diff = (clock()-timetest[a].init)/  \
(double)CLOCKS_PER_SEC; timetest[a].accum += timetest[a].diff;}
#define printclock(a) fprintf(stderr,"In %s at line %d:\n"  \
"\t last   call to %d (sec) : %lf\n""\t accum. call to %d (sec) : %lf\n", \
__FILE__,__LINE__,a,timetest[a].diff,a,timetest[a].accum);

#endif
