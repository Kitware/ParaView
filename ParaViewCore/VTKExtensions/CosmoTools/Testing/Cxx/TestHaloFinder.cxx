/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHaloFinder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

int TestHaloFinder(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  vtkNew<vtkMPIController> controller;
  controller->Initialize();
  vtkMultiProcessController::SetGlobalController(controller.GetPointer());

  int retVal = runHaloFinderTest1(argc, argv);

  controller->Finalize();
  return !retVal;
}
