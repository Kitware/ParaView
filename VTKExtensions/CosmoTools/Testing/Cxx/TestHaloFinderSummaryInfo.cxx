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

#include "vtkArrayCalculator.h"
#include "vtkColorTransferFunction.h"
#include "vtkMPIController.h"
#include "vtkRegressionTestImage.h"

namespace
{
int runHaloFinderTest(int argc, char* argv[])
{
  HaloFinderTestHelpers::HaloFinderTestVTKObjects to =
    HaloFinderTestHelpers::SetupHaloFinderTest(argc, argv, vtkPANLHaloFinder::MOST_BOUND_PARTICLE);

  vtkUnstructuredGrid* allParticles = to.haloFinder->GetOutput(0);
  if (!HaloFinderTestHelpers::pointDataHasTheseArrays(
        allParticles->GetPointData(), HaloFinderTestHelpers::getFirstOutputArrays()))
  {
    std::cerr << "Error at line: " << __LINE__ << std::endl;
    return 0;
  }
  vtkUnstructuredGrid* haloSummaries = to.haloFinder->GetOutput(1);
  if (!HaloFinderTestHelpers::pointDataHasTheseArrays(
        haloSummaries->GetPointData(), HaloFinderTestHelpers::getHaloSummaryWithCenterInfoArrays()))
  {
    std::cerr << "Error at line: " << __LINE__ << std::endl;
    return 0;
  }

  vtkNew<vtkArrayCalculator> calc;
  calc->SetInputConnection(to.haloFinder->GetOutputPort(1));
  calc->SetResultArrayName("Result");
  calc->AddCoordinateVectorVariable("coords");
  calc->AddVectorArrayName("fof_center");
  calc->SetFunction("mag(fof_center - coords)");
  calc->Update();

  double range[2];
  calc->GetDataSetOutput()->GetPointData()->GetArray("Result")->GetRange(range);

  to.maskPoints->SetInputConnection(calc->GetOutputPort());
  to.maskPoints->Update();

  vtkNew<vtkColorTransferFunction> lut;
  lut->AddRGBPoint(range[0], 59 / 255.0, 76 / 255.0, 192 / 255.0);
  lut->AddRGBPoint(range[1], 180 / 255.0, 4 / 255.0, 38 / 255.0);
  lut->SetColorSpaceToDiverging();
  lut->SetVectorModeToMagnitude();

  to.mapper->SetLookupTable(lut.GetPointer());
  to.mapper->ScalarVisibilityOn();
  to.mapper->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Result");
  to.mapper->InterpolateScalarsBeforeMappingOn();

  int retVal = vtkRegressionTestImage(to.renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    to.iren->Start();
  }

  return retVal;
}
}

int TestHaloFinderSummaryInfo(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  vtkNew<vtkMPIController> controller;
  controller->Initialize();
  vtkMultiProcessController::SetGlobalController(controller.GetPointer());

  int retVal = runHaloFinderTest(argc, argv);

  controller->Finalize();
  return !retVal;
}
