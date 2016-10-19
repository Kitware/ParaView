/*=========================================================================

  Program:   ParaView
  Module:    TestReadCGNSFiles.cxx

  Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCGNSReader.h"
#include "vtkCell.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#define TEST_SUCCESS 0
#define TEST_FAILED 1

#define vtk_assert(x)                                                                              \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << endl;               \
    return TEST_FAILED;                                                                            \
  }

int TestOutput(vtkMultiBlockDataSet* mb, int nCells, VTKCellType type)
{
  int nBlocks = mb->GetNumberOfBlocks();
  vtk_assert(nBlocks > 0);
  for (unsigned int i = 0; i < nBlocks; ++i)
  {
    vtkMultiBlockDataSet* mb2 = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(i));
    for (unsigned int j = 0; j < mb2->GetNumberOfBlocks(); ++j)
    {
      vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(mb2->GetBlock(j));
      int nc = ug->GetNumberOfCells();
      vtk_assert(nc == nCells);
      for (vtkIdType k = 0; k < ug->GetNumberOfCells(); ++k)
      {
        vtkCell* cell = ug->GetCell(k);
        vtk_assert(cell->GetCellType() == type);
      }
    }
  }
  return 0;
}

int TestCGNSReader(int argc, char* argv[])
{
  if (argc < 4)
    return 0; // for some reason two tests are run, one without data data on the cmdline

  const char* mixed = argv[2];
  const char* nfacen = argv[3];

  cout << "Opening " << mixed << endl;
  cout << "Opening " << nfacen << endl;

  vtkNew<vtkCGNSReader> mixedReader;

  mixedReader->SetFileName(mixed);
  mixedReader->Update();

  vtkMultiBlockDataSet* mb = mixedReader->GetOutput();

  if (0 != TestOutput(mb, 7, VTK_HEXAHEDRON))
    return 1;

  vtkNew<vtkCGNSReader> nfacenReader;
  nfacenReader->SetFileName(nfacen);
  nfacenReader->Update();
  mb = nfacenReader->GetOutput();

  if (0 != TestOutput(mb, 7, VTK_POLYHEDRON))
    return 1;

  cout << __FILE__ << " tests passed." << endl;
  return 0;
}
