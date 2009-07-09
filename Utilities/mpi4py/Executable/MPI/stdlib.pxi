#---------------------------------------------------------------------

cdef extern from *:
    ctypedef unsigned long int size_t

#---------------------------------------------------------------------

cdef extern from * nogil: # "stdio.h"
    ctypedef struct FILE
    FILE *stdin, *stdout, *stderr
    int fprintf(FILE *, char *, ...)
    int fflush(FILE *)

#---------------------------------------------------------------------
