// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSIIntVectorProperty.h"
#include "vtkObjectFactory.h"
#include "vtkSIVectorPropertyTemplate.txx"

vtkStandardNewMacro(vtkSIIntVectorProperty);
//----------------------------------------------------------------------------
vtkSIIntVectorProperty::vtkSIIntVectorProperty() = default;

//----------------------------------------------------------------------------
vtkSIIntVectorProperty::~vtkSIIntVectorProperty() = default;

//----------------------------------------------------------------------------
void vtkSIIntVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
