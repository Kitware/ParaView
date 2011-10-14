#ifndef  H_TRANSF77
#define  H_TRANSF77

#if defined (__hpux) || defined (_CRAY)
/* Fortran routines need no underscore */
#define F77NAME(x) x
#else
/* Fortran routines need an underscore */
#define F77NAME(x) x##_
#endif

#endif
