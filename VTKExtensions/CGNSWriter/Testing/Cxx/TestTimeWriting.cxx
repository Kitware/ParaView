/*=========================================================================

  Program:   ParaView
  Module:    TestTimeWriting.cxx

  Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestFunctions.h"
#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

int TestTimeWriting(int argc, char* argv[])
{
  vtkNew<vtkUnstructuredGrid> ug;
  Create(ug.GetPointer(), 10);

  vtkNew<vtkPVTestUtilities> u;
  u->Initialize(argc, argv);
  const char* filename = u->GetTempFilePath("unstructured_grid_time.cgns");

  vtkNew<vtkCGNSWriter> w;
  w->UseHDF5Off();
  w->SetFileName(filename);
  w->SetInputData(ug);

  double time[1] = { 10.0 };
  double range[2] = { 10.0, 10.0 };
  vtkInformation* inputInformation = w->GetInputInformation();
  vtk_assert(inputInformation != nullptr);

  inputInformation->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &time[0], 1);
  inputInformation->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);

  int rc = w->Write();
  if (rc != 1)
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkCGNSReader> r;
  r->SetFileName(filename);
  r->EnableAllBases();
  r->Update();

  delete[] filename;
  vtkInformation* outputInformation = r->GetOutputInformation(0);
  vtk_assert(outputInformation != nullptr);
  vtk_assert(outputInformation->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()));
  vtk_assert(1 == outputInformation->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()));

  double* readTime = outputInformation->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  vtk_assert(readTime != nullptr);
  vtk_assert(*readTime == 10.0);

  return EXIT_SUCCESS;
}
