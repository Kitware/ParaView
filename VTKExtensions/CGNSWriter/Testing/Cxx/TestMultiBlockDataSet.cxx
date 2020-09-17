/*=========================================================================

  Program:   ParaView
  Module:    TestMultiBlockDataSet.cxx

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
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

int MultiBlockTest(vtkMultiBlockDataSet*);
int TestMultiBlockDataSet(int argc, char* argv[])
{
  vtkNew<vtkUnstructuredGrid> ug;
  Create(ug, 10);
  vtkNew<vtkPolyData> pd;
  Create(pd);

  vtkNew<vtkMultiBlockDataSet> mb;
  mb->SetBlock(0, ug);
  mb->SetBlock(1, pd);

  mb->GetMetaData(0u)->Set(vtkCompositeDataSet::NAME(), "UNSTRUCTURED");
  mb->GetMetaData(1u)->Set(vtkCompositeDataSet::NAME(), "POLYDATA");

  vtkNew<vtkPVTestUtilities> u;
  u->Initialize(argc, argv);
  const char* filename = u->GetTempFilePath("multiblock.cgns");

  vtkNew<vtkCGNSWriter> w;
  w->UseHDF5Off();
  w->SetFileName(filename);
  w->SetInputData(mb);
  int rc = w->Write();
  if (rc != 1)
  {
    delete[] filename;
    return EXIT_FAILURE;
  }

  vtkNew<vtkCGNSReader> r;
  r->SetFileName(filename);
  r->UpdateInformation();
  r->EnableAllBases();
  r->Update();

  delete[] filename;

  return MultiBlockTest(r->GetOutput());
}

int MultiBlockTest(vtkMultiBlockDataSet* read)
{
  int rc0 = UnstructuredGridTest(read, 0, 0, 10);
  vtk_assert(EXIT_SUCCESS == rc0);

  int rc1 = PolydataTest(read, 1, 0);
  vtk_assert(EXIT_SUCCESS == rc1);

  return EXIT_SUCCESS;
}
