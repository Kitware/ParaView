/*=========================================================================

  Program:   ParaView
  Module:    vtkSIIdTypeVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIIdTypeVectorProperty.h"
#include "vtkObjectFactory.h"
#include "vtkSIVectorPropertyTemplate.txx"

vtkStandardNewMacro(vtkSIIdTypeVectorProperty);
//----------------------------------------------------------------------------
vtkSIIdTypeVectorProperty::vtkSIIdTypeVectorProperty()
{
}

//----------------------------------------------------------------------------
vtkSIIdTypeVectorProperty::~vtkSIIdTypeVectorProperty()
{
}

//----------------------------------------------------------------------------
void vtkSIIdTypeVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
