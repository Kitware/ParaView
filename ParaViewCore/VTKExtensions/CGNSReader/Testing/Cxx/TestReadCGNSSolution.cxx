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
#include "vtkNew.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCell.h"
#include "vtkInformation.h"
#include "vtkTestUtilities.h"
#include "vtkCellData.h"

#define TEST_SUCCESS 0
#define TEST_FAILED 1

#define vtk_assert(x)\
  if (! (x) ) { cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << endl;  return TEST_FAILED;}

int TestOutput(vtkMultiBlockDataSet* mb, int nCells, VTKCellType type);

int TestOutputData(vtkMultiBlockDataSet* mb, int nCells, int nArrays)
{
  int nBlocks = mb->GetNumberOfBlocks();
  vtk_assert(nBlocks > 0);
  for(unsigned int i = 0; i < nBlocks; ++i)
  {    
    vtkMultiBlockDataSet* mb2 = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(i));
    for(unsigned int j = 0; j < mb2->GetNumberOfBlocks(); ++j)
    {
      vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(mb2->GetBlock(j));
      vtkCellData* cd = ug->GetCellData();
      int nArr = cd->GetNumberOfArrays();
      if (nArr != nArrays) return 1;
      for (int k = 0; k < nArr; ++k)
      {
        vtkDataArray* arr = cd->GetArray(k);
        vtkIdType nTpl = arr->GetNumberOfTuples();
        vtk_assert(nTpl == nCells);
      }
    }
  }
  return 0;
}

int TestReadCGNSSolution(int argc, char* argv[])
{
  char* solution = 
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/CGNSReader/channelBump_solution.cgns");
  
  vtkNew<vtkCGNSReader> reader;
  vtkInformation* inf = reader->GetInformation();

  reader->SetFileName(solution);
  reader->Update();

  reader->EnableAllCellArrays();
  reader->EnableAllPointArrays();

  reader->Update();

  vtkMultiBlockDataSet* mb = reader->GetOutput();
  
  if (0 != TestOutput(mb, 19742, VTK_POLYHEDRON))
    return 1;

  if (0 != TestOutputData(mb, 19742, 20))
    return 1;

  delete [] solution;

  cout << __FILE__ << " tests passed." << endl;
  return 0;
}
