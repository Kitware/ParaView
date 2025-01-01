// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkElevationFilter.h"
#include "vtkPVExtractHistogram2D.h"
#include "vtkPointDataToCellData.h"
#include "vtkSphereSource.h"

extern int TestPVExtractHistogram2D(int, char*[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(100);
  sphere->SetPhiResolution(100);
  sphere->Update();

  vtkNew<vtkElevationFilter> elevation;
  elevation->SetInputConnection(sphere->GetOutputPort());
  elevation->SetLowPoint(0, 0, -1);
  elevation->SetHighPoint(0, 0, 1);
  elevation->Update();

  vtkNew<vtkPointDataToCellData> pd2cd;
  pd2cd->SetInputConnection(elevation->GetOutputPort());
  pd2cd->Update();

  vtkNew<vtkPVExtractHistogram2D> histogram;
  histogram->SetInputConnection(pd2cd->GetOutputPort());
  histogram->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Elevation");
  histogram->SetNumberOfBins(10, 10);
  histogram->Update();

  return EXIT_SUCCESS;
}
