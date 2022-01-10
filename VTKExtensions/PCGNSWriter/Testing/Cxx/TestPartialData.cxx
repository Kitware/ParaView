/*=========================================================================

  Program:   ParaView
  Module:    TestPolygonalData.cxx

  Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCGNSReader.h"
#include "vtkCell.h"
#include "vtkClipDataSet.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPCGNSWriter.h"
#include "vtkPVTestUtilities.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#include "TestFunctions.h"
#include "mpi.h"

#include "vtksys/SystemTools.hxx"

int TestPartialData(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 1);
  vtkObject::GlobalWarningDisplayOff();
  vtkMultiProcessController::SetGlobalController(mpiController);

  int rank = mpiController->GetCommunicator()->GetLocalProcessId();
  int size = mpiController->GetCommunicator()->GetNumberOfProcesses();

  vtkNew<vtkUnstructuredGrid> partialData;
  CreatePartial(partialData, rank);

  vtkNew<vtkPVTestUtilities> utilities;
  utilities->Initialize(argc, argv);
  const char* filename = utilities->GetTempFilePath("partial-mpi.cgns");
  if (vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  const char* filenameBefore = utilities->GetTempFilePath("partial_before_clip-mpi.cgns");
  if (vtksys::SystemTools::FileExists(filenameBefore))
  {
    vtksys::SystemTools::RemoveFile(filenameBefore);
  }
  int rc = 0;
  {
    // clip the dataset in such a way that some process
    // have zero cells and others have non-zero cells
    vtkNew<vtkClipDataSet> clip;
    clip->SetInputData(partialData);
    vtkNew<vtkPlane> plane;
    plane->SetOrigin(0, 0, size - 0.5);
    plane->SetNormal(0, 0, 1);
    clip->SetClipFunction(plane);

    clip->Update();

    vtkNew<vtkPCGNSWriter> writer;
    writer->SetController(mpiController);
    writer->SetInputData(partialData);
    writer->SetFileName(filenameBefore);
    writer->Write(); // this is tested in other tests

    writer->SetInputData(clip->GetOutput());
    writer->SetFileName(filename);
    rc = writer->Write();
  }
  mpiController->Finalize();

  if (rc == 1 && rank == 0)
  {
    vtk_assert(vtksys::SystemTools::FileExists(filename));

    vtkNew<vtkCGNSReader> reader;
    reader->SetFileName(filename);
    reader->Update();
    unsigned long err = reader->GetErrorCode();
    vtk_assert(err == 0);

    vtkMultiBlockDataSet* output = reader->GetOutput();
    vtk_assert(1 == output->GetNumberOfBlocks());

    vtkMultiBlockDataSet* firstBlock = vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0));
    vtk_assert(nullptr != firstBlock);
    vtk_assert(1 == firstBlock->GetNumberOfBlocks());

    vtkUnstructuredGrid* outputGrid = vtkUnstructuredGrid::SafeDownCast(firstBlock->GetBlock(0));
    vtk_assert(nullptr != outputGrid);

    vtk_assert(size + (size % 2 == 0 ? 0 : 1) == outputGrid->GetNumberOfCells());

    rc = err == 0 ? 1 : 0;
  }

  delete[] filename;
  delete[] filenameBefore;
  return rc == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
