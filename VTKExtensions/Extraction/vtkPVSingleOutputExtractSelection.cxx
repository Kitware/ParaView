// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVSingleOutputExtractSelection.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVSingleOutputExtractSelection);
//----------------------------------------------------------------------------
vtkPVSingleOutputExtractSelection::vtkPVSingleOutputExtractSelection()
{
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVSingleOutputExtractSelection::~vtkPVSingleOutputExtractSelection() = default;

//----------------------------------------------------------------------------
void vtkPVSingleOutputExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
