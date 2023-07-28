// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMyElevationFilter.h"

#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkMyElevationFilter);

//----------------------------------------------------------------------------
vtkMyElevationFilter::vtkMyElevationFilter() = default;

//----------------------------------------------------------------------------
vtkMyElevationFilter::~vtkMyElevationFilter() = default;

//----------------------------------------------------------------------------
void vtkMyElevationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
