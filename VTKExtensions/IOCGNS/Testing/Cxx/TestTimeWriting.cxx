// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "TestFunctions.h"
#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
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
  w->WriteAllTimeStepsOn();
  w->UseHDF5Off();
  w->SetFileName(filename);
  w->SetInputData(ug);

  double time[1] = { 10.0 };
  double range[2] = { 10.0, 10.0 };
  vtkInformation* inputInformation = w->GetInputInformation();
  vtkLogIfF(ERROR, inputInformation == nullptr, "Information is NULL");

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
  vtkLogIfF(ERROR, outputInformation == nullptr, "Output information is NULL");
  vtkLogIfF(ERROR, !outputInformation->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()),
    "No timesteps found in information");
  vtkLogIfF(ERROR, 1 != outputInformation->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()),
    "Time steps length does not match");

  double* readTime = outputInformation->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  vtkLogIfF(ERROR, readTime == nullptr, "Time array is NULL");
  vtkLogIfF(ERROR, *readTime != 10.0, "Expected time=10.0, got %3.2f", *readTime);

  return EXIT_SUCCESS;
}
