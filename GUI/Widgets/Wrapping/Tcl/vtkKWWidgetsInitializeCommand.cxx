/*=========================================================================

  Module:    vtkKWWidgetsInitializeCommand.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"

#include "vtkTclUtil.h"

extern "C" {int VTK_TK_EXPORT Vtkkwwidgetsinitializecommand_Init(Tcl_Interp *interp);}

int VTK_TK_EXPORT Vtkkwwidgetsinitializecommand_Init(Tcl_Interp *interp)
{
  if(Tcl_PkgPresent(interp, (char *)"Tcl", (char *)TCL_VERSION, 0))
    {
    ostrstream err;
    Tcl_Interp *res = vtkKWApplication::InitializeTcl(interp, &err);
    err << ends;
    if (!res && *(err.str()))
      {
      vtkGenericWarningMacro(<< " Vtkkwwidgetsinitializecommand_Init: failed to InitializeTcl: " << err.str());
      }
    err.rdbuf()->freeze(0);
    }
  
  return TCL_OK;
}
