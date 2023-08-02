// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVNullSource.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVNullSource);
//----------------------------------------------------------------------------
vtkPVNullSource::vtkPVNullSource()
{
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkPVNullSource::~vtkPVNullSource() = default;

//----------------------------------------------------------------------------
void vtkPVNullSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
