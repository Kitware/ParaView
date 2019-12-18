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

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkMPIController.h"
#include "vtkRegressionTestImage.h"
#include "vtkRendererCollection.h"

namespace
{
std::set<std::string> getSubhaloSummaryArrays()
{
  std::set<std::string> result;
  result.insert("fof_halo_count");
  result.insert("fof_halo_tag");
  result.insert("subhalo_com");
  result.insert("subhalo_count");
  result.insert("subhalo_mass");
  result.insert("subhalo_mean_velocity");
  result.insert("subhalo_tag");
  result.insert("subhalo_velocity_dispersion");
  return result;
}

int runHaloFinderTest(int argc, char* argv[])
{
  HaloFinderTestHelpers::HaloFinderTestVTKObjects to =
    HaloFinderTestHelpers::SetupHaloFinderTest(argc, argv, vtkPANLHaloFinder::NONE, true);

  vtkUnstructuredGrid* allParticles = to.haloFinder->GetOutput(0);
  std::set<std::string> firstOutputArrays = HaloFinderTestHelpers::getFirstOutputArrays();
  firstOutputArrays.insert("subhalo_tag");
  if (!HaloFinderTestHelpers::pointDataHasTheseArrays(
        allParticles->GetPointData(), firstOutputArrays))
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
  vtkUnstructuredGrid* subhaloSummaries = to.haloFinder->GetOutput(2);
  if (!HaloFinderTestHelpers::pointDataHasTheseArrays(
        subhaloSummaries->GetPointData(), getSubhaloSummaryArrays()))
  {
    std::cerr << "Error at line: " << __LINE__ << std::endl;
    return 0;
  }

  to.onlyPointsInHalos->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "subhalo_tag");
  to.onlyPointsInHalos->ThresholdByUpper(0.0);
  to.onlyPointsInHalos->Update();

  double range[2];
  to.onlyPointsInHalos->GetOutput()->GetPointData()->GetArray("subhalo_tag")->GetRange(range);
  to.onlyPointsInHalos->GetOutput()->GetPointData()->SetActiveScalars("subhalo_tag");

  vtkNew<vtkColorTransferFunction> lut;
  lut->AddRGBPoint(range[0], 59 / 255.0, 76 / 255.0, 192 / 255.0);
  lut->AddRGBPoint(range[1], 180 / 255.0, 4 / 255.0, 38 / 255.0);
  lut->SetColorSpaceToDiverging();

  to.mapper->SetLookupTable(lut.GetPointer());
  to.mapper->ScalarVisibilityOn();
  to.mapper->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "subhalo_tag");
  to.mapper->SelectColorArray("subhalo_tag");
  to.mapper->InterpolateScalarsBeforeMappingOn();

  vtkRenderer* ren = to.renWin->GetRenderers()->GetFirstRenderer();
  vtkCamera* cam = ren->GetActiveCamera();
  cam->SetPosition(39.8465, 33.3915, 99.8347);
  cam->SetFocalPoint(25.1646, 47.1462, 107.878);
  cam->SetViewUp(-0.526454, -0.77122, 0.357864);
  ren->ResetCamera();

  int retVal = vtkRegressionTestImage(to.renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    to.iren->Start();
  }

  return retVal;
}
}

int TestHaloFinderSubhaloFinding(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  vtkNew<vtkMPIController> controller;
  controller->Initialize();
  vtkMultiProcessController::SetGlobalController(controller.GetPointer());

  int retVal = runHaloFinderTest(argc, argv);

  controller->Finalize();
  return !retVal;
}
