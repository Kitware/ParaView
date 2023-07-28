// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMBooleanDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMBooleanDomain);

//---------------------------------------------------------------------------
vtkSMBooleanDomain::vtkSMBooleanDomain() = default;

//---------------------------------------------------------------------------
vtkSMBooleanDomain::~vtkSMBooleanDomain() = default;

//---------------------------------------------------------------------------
int vtkSMBooleanDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
  {
    return 1;
  }

  if (!property)
  {
    return 0;
  }

  int value = vtkSMPropertyHelper(property).GetAsInt();
  return (value == 0 || value == 1) ? 1 : 0;
}

//---------------------------------------------------------------------------
void vtkSMBooleanDomain::SetAnimationValue(vtkSMProperty* property, int idx, double value)
{
  if (!property)
  {
    return;
  }
  vtkSMPropertyHelper(property).Set(idx, static_cast<int>(value));
}

//---------------------------------------------------------------------------
void vtkSMBooleanDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
