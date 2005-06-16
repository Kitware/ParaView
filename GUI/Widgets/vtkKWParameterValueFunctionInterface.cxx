/*=========================================================================

  Module:    vtkKWParameterValueFunctionInterface.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWParameterValueFunctionInterface.h"

#include "vtkCallbackCommand.h"
#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkKWParameterValueFunctionInterface, "1.3");

int vtkKWParameterValueFunctionInterfaceCommand(ClientData cd, Tcl_Interp *interp, int argc, char *argv[]);

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionInterface::Create(vtkKWApplication *app)
{
  this->Superclass::Create(app);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionInterface::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
