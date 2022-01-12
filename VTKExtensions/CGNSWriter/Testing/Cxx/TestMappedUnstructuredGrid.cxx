/*=========================================================================

  Program:   ParaView
  Module:    TestMappedUnstructuredGrid.cxx

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
#include "vtkMappedUnstructuredGridGenerator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVTestUtilities.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

int TestMappedUnstructuredGrid(int argc, char* argv[])
{
  vtkUnstructuredGridBase* ug;
  vtkMappedUnstructuredGridGenerator::GenerateMappedUnstructuredGrid(&ug);

  vtkNew<vtkPVTestUtilities> u;
  u->Initialize(argc, argv);
  const char* filename = u->GetTempFilePath("mapped_unstructured_grid.cgns");

  vtkNew<vtkCGNSWriter> w;
  w->UseHDF5Off();
  w->SetFileName(filename);
  w->SetInputData(ug);
  int rc = w->Write();
  if (rc != 1)
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkCGNSReader> r;
  r->SetFileName(filename);
  r->EnableAllBases();
  r->Update();

  ug->Delete();
  delete[] filename;

  return MappedUnstructuredGridTest(r->GetOutput(), 0, 0, 3);
}

int MappedUnstructuredGridTest(vtkMultiBlockDataSet* read, unsigned int b0, unsigned int b1, int N)
{
  vtk_assert(nullptr != read);
  vtk_assert(b0 < read->GetNumberOfBlocks());

  vtkMultiBlockDataSet* block0 = vtkMultiBlockDataSet::SafeDownCast(read->GetBlock(b0));
  vtk_assert(nullptr != block0);
  vtk_assert(b1 < block0->GetNumberOfBlocks());

  vtkUnstructuredGrid* target = vtkUnstructuredGrid::SafeDownCast(block0->GetBlock(b1));
  vtk_assert(nullptr != target);
  vtk_assert(N == target->GetNumberOfCells());
  return EXIT_SUCCESS;
}
