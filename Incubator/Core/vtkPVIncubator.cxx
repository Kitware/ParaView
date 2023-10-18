// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVIncubator.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVIncubator);

//----------------------------------------------------------------------------
vtkPVIncubator::vtkPVIncubator() = default;
vtkPVIncubator::~vtkPVIncubator() = default;

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkPVIncubator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
