// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInSituPipeline.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkInSituPipeline::vtkInSituPipeline() = default;

//----------------------------------------------------------------------------
vtkInSituPipeline::~vtkInSituPipeline() = default;

//----------------------------------------------------------------------------
void vtkInSituPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
