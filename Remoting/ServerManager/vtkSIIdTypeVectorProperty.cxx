// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSIIdTypeVectorProperty.h"
#include "vtkObjectFactory.h"
#include "vtkSIVectorPropertyTemplate.txx"

vtkStandardNewMacro(vtkSIIdTypeVectorProperty);
//----------------------------------------------------------------------------
vtkSIIdTypeVectorProperty::vtkSIIdTypeVectorProperty() = default;

//----------------------------------------------------------------------------
vtkSIIdTypeVectorProperty::~vtkSIIdTypeVectorProperty() = default;

//----------------------------------------------------------------------------
void vtkSIIdTypeVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
