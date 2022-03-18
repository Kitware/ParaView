/*=========================================================================

  Program:   ParaView
  Module:    TestUnstructuredGrid.cxx

  Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestFunctions.h"
#include "mpi.h"
#include "vtkCGNSReader.h"
#include "vtkCell.h"
#include "vtkLogger.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPCGNSWriter.h"
#include "vtkPVTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtksys/SystemTools.hxx"

int TestUnstructuredGrid(int argc, char* argv[])
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
  writer->SetInputData(unstructuredGrid);
  writer->SetFileName(filename);
  writer->SetController(mpiController);

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

    rc = err == 0 ? 1 : 0;
  }

  delete[] filename;
  return rc == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
