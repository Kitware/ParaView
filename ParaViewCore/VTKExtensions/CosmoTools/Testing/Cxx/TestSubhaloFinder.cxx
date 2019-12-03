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

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkMPIController.h"
#include "vtkMaskPoints.h"
#include "vtkNew.h"
#include "vtkPANLSubhaloFinder.h"
#include "vtkPGenericIOReader.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkThreshold.h"
#include "vtkUnstructuredGrid.h"

#include <set>

namespace
{

int runSubhaloFinderTest(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Testing/Data/genericio/m000.499.allparticles");

  vtkNew<vtkPGenericIOReader> reader;
  reader->SetFileName(fname);
  reader->UpdateInformation();
  reader->SetXAxisVariableName("x");
  reader->SetYAxisVariableName("y");
  reader->SetZAxisVariableName("z");
  reader->SetPointArrayStatus("vx", 1);
  reader->SetPointArrayStatus("vy", 1);
  reader->SetPointArrayStatus("vz", 1);
  reader->SetPointArrayStatus("id", 1);
  reader->SetPointArrayStatus("fof_halo_tag", 1);
  reader->Update();

  delete[] fname;

  vtkNew<vtkPANLSubhaloFinder> haloFinder;
  haloFinder->SetInputConnection(reader->GetOutputPort());
  haloFinder->SetRL(128);
  haloFinder->SetParticleMass(13070871810);
  haloFinder->SetBB(0.2);
  haloFinder->SetMinCandidateSize(20);
  // running on all halos larger than 1000 particles takes too long for a test
  //  haloFinder->SetMode(vtkPANLSubhaloFinder::HALOS_LARGER_THAN_THRESHOLD);
  //  haloFinder->SetSizeThreshold(1000);
  // run on only the interesting one...
  haloFinder->AddHaloToProcess(1867323);
  haloFinder->SetMode(vtkPANLSubhaloFinder::ONLY_SELECTED_HALOS);
  haloFinder->Update();

  vtkNew<vtkThreshold> threshold;
  threshold->SetInputConnection(haloFinder->GetOutputPort(0));
  threshold->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "subhalo_tag");
  threshold->ThresholdByUpper(0.0);
  threshold->Update();

  vtkNew<vtkMaskPoints> maskPoints;
  maskPoints->SetInputConnection(threshold->GetOutputPort());
  maskPoints->GenerateVerticesOn();
  maskPoints->SetOnRatio(1);
  maskPoints->SetMaximumNumberOfPoints(threshold->GetOutput()->GetNumberOfPoints());
  maskPoints->SingleVertexPerCellOn();
  maskPoints->Update();

  maskPoints->GetOutput()->GetPointData()->SetActiveScalars("subhalo_tag");

  double range[2];
  maskPoints->GetOutput()->GetPointData()->GetArray("subhalo_tag")->GetRange(range);

  vtkNew<vtkColorTransferFunction> lut;
  lut->AddRGBPoint(range[0], 59 / 255.0, 76 / 255.0, 192 / 255.0);
  lut->AddRGBPoint(range[1], 180 / 255.0, 4 / 255.0, 38 / 255.0);
  lut->SetColorSpaceToDiverging();

  vtkNew<vtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(maskPoints->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetLookupTable(lut.GetPointer());
  mapper->InterpolateScalarsBeforeMappingOn();
  mapper->Update();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor.GetPointer());
  vtkCamera* cam = renderer->GetActiveCamera();
  cam->SetPosition(117.52, 123.95, 22.69);
  cam->SetFocalPoint(124.73, -72.54, 416.87);
  cam->SetViewUp(-0.34663, 0.83693, 0.42356);
  renderer->ResetCamera();

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  int retVal = vtkRegressionTestImage(renWin.GetPointer());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
}

int TestSubhaloFinder(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  vtkNew<vtkMPIController> controller;
  controller->Initialize();
  vtkMultiProcessController::SetGlobalController(controller.GetPointer());

  int retVal = runSubhaloFinderTest(argc, argv);

  controller->Finalize();
  return retVal;
}
