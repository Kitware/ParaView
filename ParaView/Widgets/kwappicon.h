#ifndef __kwappicon_h
#define __kwappicon_h

#ifdef __cplusplus
#define KWAPPICON_EXTERN extern "C"
#else
#define KWAPPICON_EXTERN extern
#endif

#include <tcl.h>

KWAPPICON_EXTERN int ApplicationIcon_DoInit(Tcl_Interp *interp);

#endif // __kwappicon_h
