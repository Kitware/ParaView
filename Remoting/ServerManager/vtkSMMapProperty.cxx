// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

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
