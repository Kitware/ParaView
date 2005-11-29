#ifndef __vtkKWSetApplicationIconTclCommand_h
#define __vtkKWSetApplicationIconTclCommand_h

#ifdef __cplusplus
#define VTKKWSETAPPLICATIONICONTCLCOMMAND_EXTERN extern "C"
#else
#define VTKKWSETAPPLICATIONICONTCLCOMMAND_EXTERN extern
#endif

#include "vtkTcl.h"

VTKKWSETAPPLICATIONICONTCLCOMMAND_EXTERN int vtkKWSetApplicationIconTclCommand_DoInit(Tcl_Interp *interp);

VTKKWSETAPPLICATIONICONTCLCOMMAND_EXTERN int vtkKWSetApplicationIcon(Tcl_Interp *interp, const char *app_name, int icon_res_id);

VTKKWSETAPPLICATIONICONTCLCOMMAND_EXTERN int vtkKWSetApplicationSmallIcon(Tcl_Interp *interp, const char *app_name, int icon_res_id);

#endif // __vtkKWSetApplicationIconTclCommand_h
