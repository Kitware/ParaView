// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "TestFunctions.h"
#include "mpi.h"
#include "vtkCGNSReader.h"
#include "vtkCell.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPCGNSWriter.h"
#include "vtkPVTestUtilities.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtksys/SystemTools.hxx"

extern int TestUnstructuredGrid(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  vtkObject::GlobalWarningDisplayOff();
  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 1);

  vtkMultiProcessController::SetGlobalController(mpiController);

  int rank = mpiController->GetCommunicator()->GetLocalProcessId();
  int size = mpiController->GetCommunicator()->GetNumberOfProcesses();

  vtkNew<vtkUnstructuredGrid> unstructuredGrid;
  Create(unstructuredGrid, rank, size);

  vtkNew<vtkPVTestUtilities> utilities;
  utilities->Initialize(argc, argv);
  const char* filename = utilities->GetTempFilePath("unstructured-mpi.cgns");
  if (vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  vtkNew<vtkPCGNSWriter> writer;
  writer->WriteAllTimeStepsOn();
  writer->SetInputData(unstructuredGrid);
  writer->SetFileName(filename);
  writer->SetController(mpiController);

  double time[1] = { 20.0 };
  double range[2] = { 20.0, 20.0 };
  vtkInformation* inputInformation = writer->GetInputInformation();
  vtkLogIfF(ERROR, inputInformation == nullptr, "Information is NULL");

  inputInformation->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &time[0], 1);
  inputInformation->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range, 2);

  int rc = writer->Write();
  mpiController->Finalize();

  if (rc == 1 && rank == 0)
  {
    vtkLogIfF(ERROR, !vtksys::SystemTools::FileExists(filename), "File '%s' not found", filename);

    vtkNew<vtkCGNSReader> reader;
    reader->SetFileName(filename);
    reader->Update();

    unsigned long err = reader->GetErrorCode();
    vtkLogIfF(ERROR, err != 0, "Reading CGNS file failed.");

    vtkMultiBlockDataSet* output = reader->GetOutput();
    vtkLogIfF(ERROR, nullptr == output, "No CGNS reader output.");
    vtkLogIfF(ERROR, 1 != output->GetNumberOfBlocks(), "Expected 1 base block.");

    vtkMultiBlockDataSet* firstBlock = vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0));
    vtkLogIfF(ERROR, nullptr == firstBlock, "First block is NULL");
    vtkLogIfF(ERROR, 1 != firstBlock->GetNumberOfBlocks(), "Expected 1 zone block.");

    vtkUnstructuredGrid* outputGrid = vtkUnstructuredGrid::SafeDownCast(firstBlock->GetBlock(0));
    vtkLogIfF(ERROR, nullptr == outputGrid, "Read grid is NULL");
    vtkLogIfF(ERROR, std::max(2, size) != outputGrid->GetNumberOfCells(),
      "Expected %d cells, got %lld.", std::max(2, size), outputGrid->GetNumberOfCells());

    vtkInformation* outputInformation = reader->GetOutputInformation(0);
    vtkLogIfF(ERROR, outputInformation == nullptr, "Output information is NULL");
    vtkLogIfF(ERROR, !outputInformation->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()),
      "No timesteps found in information");
    vtkLogIfF(ERROR, 1 != outputInformation->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()),
      "Time steps length does not match");

    double* readTime = outputInformation->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    vtkLogIfF(ERROR, readTime == nullptr, "Time array is NULL");
    vtkLogIfF(ERROR, *readTime != time[0], "Expected time=%3.2f, got %3.2f", time[0], *readTime);

    rc = err == 0 ? 1 : 0;
  }

  delete[] filename;
  return rc == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
