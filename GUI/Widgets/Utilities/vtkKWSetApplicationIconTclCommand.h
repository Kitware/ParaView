#ifndef __vtkKWSetApplicationIconTclCommand_h
#define __vtkKWSetApplicationIconTclCommand_h

#ifdef __cplusplus
#define VTKKWSETAPPLICATIONICONTCLCOMMAND_EXTERN extern "C"
#else
#define VTKKWSETAPPLICATIONICONTCLCOMMAND_EXTERN extern
#endif

#include "vtkTcl.h"

VTKKWSETAPPLICATIONICONTCLCOMMAND_EXTERN int vtkKWSetApplicationIconTclCommand_DoInit(Tcl_Interp *interp);

#endif // __vtkKWSetApplicationIconTclCommand_h
