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

vtkCxxRevisionMacro(vtkKWParameterValueFunctionInterface, "1.5");

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionInterface::Create(vtkKWApplication *app)
{
  this->Superclass::Create(app);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionInterface::FunctionLineIsSampledBetweenPoints(
  int vtkNotUsed(id1), int vtkNotUsed(id2))
{
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionInterface::GetFunctionPointId(
  double parameter, int *id)
{
  int size = this->GetFunctionSize();
  double p;
  for (int i = 0; i < size; i++)
    {
    if (this->GetFunctionPointParameter(i, &p) && p == parameter)
      {
      *id = i;
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionInterface::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
