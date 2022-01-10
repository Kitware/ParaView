/*=========================================================================

  Program:   ParaView
  Module:    TestMultiBlockData.cxx

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
#include "vtkInformation.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPCGNSWriter.h"
#include "vtkPVTestUtilities.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/SystemTools.hxx"

int TestMultiBlockData(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  vtkObject::GlobalWarningDisplayOff();
  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 1);

  vtkMultiProcessController::SetGlobalController(mpiController);

  int rank = mpiController->GetCommunicator()->GetLocalProcessId();
  int size = mpiController->GetCommunicator()->GetNumberOfProcesses();

  int rc(0);

  vtkNew<vtkMultiBlockDataSet> mb;
  {
    vtkNew<vtkUnstructuredGrid> ug;
    Create(ug, rank, size);

    vtkNew<vtkPolyData> pd;
    Create(pd, rank, size);

    mb->SetBlock(0u, ug);
    mb->SetBlock(1u, pd.GetPointer());
    mb->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "UNSTRUCTURED");
    mb->GetMetaData(1u)->Set(vtkCompositeDataSet::NAME(), "POLYDATA");
  }

  vtkNew<vtkPVTestUtilities> utilities;
  utilities->Initialize(argc, argv);
  const char* filename = utilities->GetTempFilePath("multiblock-mpi.cgns");
  if (vtksys::SystemTools::FileExists(filename))
  {
    vtksys::SystemTools::RemoveFile(filename);
  }

  vtkNew<vtkPCGNSWriter> writer;
  writer->SetInputData(mb);
  writer->SetFileName(filename);
  writer->SetController(mpiController);

  rc = writer->Write();

  mpiController->Finalize();
  if (rc == 1 && rank == 0)
  {
    vtk_assert(vtksys::SystemTools::FileExists(filename));
    vtkNew<vtkCGNSReader> reader;
    reader->SetFileName(filename);
    // update information first to get all bases in the information
    reader->UpdateInformation();
    // then enable all bases get both bases (volume, surface) into the output
    reader->EnableAllBases();
    reader->Update();

    unsigned long err = reader->GetErrorCode();
    vtk_assert(err == 0);

    vtkMultiBlockDataSet* output = reader->GetOutput();
    vtk_assert(nullptr != output);
    vtk_assert2(2, output->GetNumberOfBlocks());
    {
      vtkMultiBlockDataSet* firstBlock = vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0));
      vtk_assert(nullptr != firstBlock);

      vtk_assert(1 == firstBlock->GetNumberOfBlocks());

      vtkUnstructuredGrid* outputGrid = vtkUnstructuredGrid::SafeDownCast(firstBlock->GetBlock(0));
      vtk_assert(nullptr != outputGrid);

      vtk_assert2(std::max(2, size), outputGrid->GetNumberOfCells());
    }
    {
      vtkMultiBlockDataSet* secondBlock = vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(1));
      vtk_assert(nullptr != secondBlock);

      vtk_assert(1 == secondBlock->GetNumberOfBlocks());

      vtkUnstructuredGrid* outputGrid = vtkUnstructuredGrid::SafeDownCast(secondBlock->GetBlock(0));
      vtk_assert(nullptr != outputGrid);

      vtk_assert(std::max(2, size) == outputGrid->GetNumberOfCells());
    }
    rc = err == 0 ? 1 : 0;
  }

  delete[] filename;
  return rc == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}
