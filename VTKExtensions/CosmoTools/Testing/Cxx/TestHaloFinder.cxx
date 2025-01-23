// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtk_mpi.h>

#include "HaloFinderTestHelpers.h"

#include "vtkMPIController.h"
#include "vtkRegressionTestImage.h"

namespace
{
int runHaloFinderTest1(int argc, char* argv[])
{
  HaloFinderTestHelpers::HaloFinderTestVTKObjects to =
    HaloFinderTestHelpers::SetupHaloFinderTest(argc, argv);

  vtkUnstructuredGrid* allParticles = to.haloFinder->GetOutput(0);
  if (!HaloFinderTestHelpers::pointDataHasTheseArrays(
        allParticles->GetPointData(), HaloFinderTestHelpers::getFirstOutputArrays()))
  {
    std::cerr << "Error at line: " << __LINE__ << std::endl;
    return 0;
  }
  vtkUnstructuredGrid* haloSummaries = to.haloFinder->GetOutput(1);
  if (!HaloFinderTestHelpers::pointDataHasTheseArrays(
        haloSummaries->GetPointData(), HaloFinderTestHelpers::getHaloSummaryArrays()))
  {
    std::cerr << "Error at line: " << __LINE__ << std::endl;
    return 0;
  }

  int retVal = vtkRegressionTestImage(to.renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    to.iren->Start();
  }

  return retVal;
}
}

extern int TestHaloFinder(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  vtkNew<vtkMPIController> controller;
  controller->Initialize();
  vtkMultiProcessController::SetGlobalController(controller.GetPointer());

  int retVal = runHaloFinderTest1(argc, argv);

  controller->Finalize();
  return !retVal;
}
