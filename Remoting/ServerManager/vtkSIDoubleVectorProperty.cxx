// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSIDoubleVectorProperty.h"
#include "vtkObjectFactory.h"
#include "vtkSIVectorPropertyTemplate.txx"

vtkStandardNewMacro(vtkSIDoubleVectorProperty);
//----------------------------------------------------------------------------
vtkSIDoubleVectorProperty::vtkSIDoubleVectorProperty() = default;

//----------------------------------------------------------------------------
vtkSIDoubleVectorProperty::~vtkSIDoubleVectorProperty() = default;

//----------------------------------------------------------------------------
void vtkSIDoubleVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
