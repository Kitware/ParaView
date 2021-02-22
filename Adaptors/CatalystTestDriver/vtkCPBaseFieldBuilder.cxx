/*=========================================================================

  Program:   ParaView
  Module:    vtkCPBaseFieldBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPBaseFieldBuilder.h"

//----------------------------------------------------------------------------
vtkCPBaseFieldBuilder::vtkCPBaseFieldBuilder() = default;

//----------------------------------------------------------------------------
vtkCPBaseFieldBuilder::~vtkCPBaseFieldBuilder() = default;

//----------------------------------------------------------------------------
void vtkCPBaseFieldBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
