/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMapProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMMapProperty.h"

//---------------------------------------------------------------------------
vtkSMMapProperty::vtkSMMapProperty() = default;

//---------------------------------------------------------------------------
vtkSMMapProperty::~vtkSMMapProperty() = default;

//---------------------------------------------------------------------------
vtkIdType vtkSMMapProperty::GetNumberOfElements()
{
  return 0;
}

//---------------------------------------------------------------------------
bool vtkSMMapProperty::IsValueDefault()
{
  return this->GetNumberOfElements() == 0;
}

//---------------------------------------------------------------------------
int vtkSMMapProperty::LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader)
{
  this->Superclass::LoadState(element, loader);

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMMapProperty::ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(parent, element);

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMMapProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSMMapProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);
}
