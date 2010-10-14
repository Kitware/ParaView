/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPID.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPID.h"

#include "vtkObjectFactory.h"
#include <sys/types.h>
#include <unistd.h>

vtkStandardNewMacro(vtkPID);

//----------------------------------------------------------------------------
vtkPID::vtkPID()
{
}

//----------------------------------------------------------------------------
vtkPID::~vtkPID()
{
}

//----------------------------------------------------------------------------
int vtkPID::GetPID()
{
  cout << "PID: " << getpid() << endl;
  return getpid();
}
//----------------------------------------------------------------------------
int vtkPID::GetParentPID()
{
  return getppid();
}

//----------------------------------------------------------------------------
void vtkPID::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ProcessId: " << this->GetPID() << endl;
  os << indent << "Parent ProcessId: " << this->GetParentPID() << endl;
}
