// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCPPipeline.h"

//----------------------------------------------------------------------------
vtkCPPipeline::vtkCPPipeline() = default;

//----------------------------------------------------------------------------
vtkCPPipeline::~vtkCPPipeline() = default;

//----------------------------------------------------------------------------
int vtkCPPipeline::Finalize()
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkCPPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
