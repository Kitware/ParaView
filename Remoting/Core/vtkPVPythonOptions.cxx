/*=========================================================================

  Module:    vtkPVPythonOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPythonOptions.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVPythonOptions);
//----------------------------------------------------------------------------
vtkPVPythonOptions::vtkPVPythonOptions() = default;

//----------------------------------------------------------------------------
vtkPVPythonOptions::~vtkPVPythonOptions() = default;

//----------------------------------------------------------------------------
int vtkPVPythonOptions::WrongArgument(const char* argument)
{
  this->SetUnknownArgument(argument);
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPythonOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
