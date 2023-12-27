// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInSituPipeline.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkInSituPipeline::vtkInSituPipeline()
{
  this->Name = nullptr;
}

//----------------------------------------------------------------------------
vtkInSituPipeline::~vtkInSituPipeline()
{
  delete[] this->Name;
}

//----------------------------------------------------------------------------
void vtkInSituPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
