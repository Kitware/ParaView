/*=========================================================================

  Program:   ParaView
  Module:    vtkCPIntHolder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPIntHolder.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCPIntHolder);

vtkCPIntHolder::vtkCPIntHolder()
{
  this->Value = 0;
}

vtkCPIntHolder::~vtkCPIntHolder()
{
}

int vtkCPIntHolder::GetInt() 
{
  return this->Value;
}

void vtkCPIntHolder::SetInt(int V)
{
  this->Value = V;
}

//----------------------------------------------------------------------------
void vtkCPIntHolder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Value: " << this->Value << "\n";
}
