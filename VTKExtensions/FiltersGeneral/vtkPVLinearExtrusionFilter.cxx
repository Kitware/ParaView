// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVLinearExtrusionFilter.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVLinearExtrusionFilter);

vtkPVLinearExtrusionFilter::vtkPVLinearExtrusionFilter()
{
  this->ExtrusionType = VTK_VECTOR_EXTRUSION;
}

void vtkPVLinearExtrusionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
