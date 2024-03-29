// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <iostream>

#include "vtkDummyController.h"
#include "vtkMultiProcessController.h"
#include "vtkPVAMRDualContour.h"
#include "vtkSmartPointer.h"
#include "vtkSpyPlotReader.h"
#include "vtkTestUtilities.h"

typedef vtkSmartPointer<vtkDummyController> vtkDummyControllerRefPtr;
typedef vtkSmartPointer<vtkSpyPlotReader> vtkSpyPlotReaderRefPtr;
typedef vtkSmartPointer<vtkPVAMRDualContour> vtkPVAMRDualContourRefPtr;

int TestPVAMRDualContour(int argc, char* argv[])
{
  vtkDummyControllerRefPtr controller(vtkDummyControllerRefPtr::New());
  vtkMultiProcessController::SetGlobalController(controller);

  int rc = 0;
  //  const char *fname =
  //      "/media/shared/Data/ParaViewData/Data/SPCTH/Dave_Karelitz_Small/spcth.0";

  const char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/Dave_Karelitz_Small/spcth.0");

  vtkSpyPlotReaderRefPtr reader = vtkSpyPlotReaderRefPtr::New();
  reader->SetFileName(fname);
  reader->SetGlobalController(controller);
  reader->MergeXYZComponentsOn();
  reader->DownConvertVolumeFractionOn();
  reader->DistributeFilesOn();
  reader->SetCellArrayStatus("Material volume fraction - 2", 1);
  reader->Update();

  vtkPVAMRDualContourRefPtr contour = vtkPVAMRDualContourRefPtr::New();
  contour->SetInputData(reader->GetOutputDataObject(0));
  contour->SetVolumeFractionSurfaceValue(0.1);
  contour->SetEnableMergePoints(1);
  contour->SetEnableDegenerateCells(1);
  contour->SetEnableMultiProcessCommunication(1);
  contour->AddInputCellArrayToProcess("Material volume fraction - 2");
  contour->Update();

  return (rc);
}
