/*=========================================================================

  Program:   ParaView
  Module:    vtkCPGridBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPGridBuilder.h"

#include "vtkCPBaseFieldBuilder.h"

vtkCxxSetObjectMacro(vtkCPGridBuilder, FieldBuilder, vtkCPBaseFieldBuilder);

//----------------------------------------------------------------------------
vtkCPGridBuilder::vtkCPGridBuilder()
{
  this->FieldBuilder = nullptr;
}

//----------------------------------------------------------------------------
vtkCPGridBuilder::~vtkCPGridBuilder()
{
  this->SetFieldBuilder(nullptr);
}

//----------------------------------------------------------------------------
vtkCPBaseFieldBuilder* vtkCPGridBuilder::GetFieldBuilder()
{
  return this->FieldBuilder;
}

//----------------------------------------------------------------------------
void vtkCPGridBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldBuilder: " << this->FieldBuilder << endl;
}
