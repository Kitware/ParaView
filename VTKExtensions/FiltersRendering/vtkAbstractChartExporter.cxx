// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAbstractChartExporter.h"
#include "vtkObjectFactory.h"

vtkAbstractObjectFactoryNewMacro(vtkAbstractChartExporter);

//----------------------------------------------------------------------------
vtkAbstractChartExporter::vtkAbstractChartExporter() = default;

//----------------------------------------------------------------------------
vtkAbstractChartExporter::~vtkAbstractChartExporter() = default;

//----------------------------------------------------------------------------
void vtkAbstractChartExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
