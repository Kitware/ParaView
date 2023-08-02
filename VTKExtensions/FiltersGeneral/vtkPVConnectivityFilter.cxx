// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVConnectivityFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkPVConnectivityFilter);

vtkPVConnectivityFilter::vtkPVConnectivityFilter()
{
  this->ExtractionMode = VTK_EXTRACT_ALL_REGIONS;
  this->ColorRegions = 1;
}

void vtkPVConnectivityFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
