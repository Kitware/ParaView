#ifndef __vtkKWSetApplicationIconTclCommand_h
#define __vtkKWSetApplicationIconTclCommand_h

#include "vtkTcl.h"
#include "vtkKWWidgets.h"

KWWidgets_EXTERN int vtkKWSetApplicationIconTclCommand_DoInit(Tcl_Interp *interp);

KWWidgets_EXTERN int vtkKWSetApplicationIcon(Tcl_Interp *interp, const char *app_name, int icon_res_id);

KWWidgets_EXTERN int vtkKWSetApplicationSmallIcon(Tcl_Interp *interp, const char *app_name, int icon_res_id);

#endif // __vtkKWSetApplicationIconTclCommand_h
