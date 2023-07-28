// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "TestFunctions.h"
#include "vtkCGNSReader.h"
#include "vtkCGNSWriter.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
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
  // update information first to get all bases in the information
  r->UpdateInformation();
  // then enable all bases get both bases (volume, surface) into the output
  r->EnableAllBases();
  r->Update();

  delete[] filename;
  vtkMultiBlockDataSet* read = r->GetOutput();
  vtkLogIfF(ERROR, 2 != read->GetNumberOfBlocks(), "Expected to read 2 blocks");
  return MultiBlockTest(read);
}

int MultiBlockTest(vtkMultiBlockDataSet* read)
{
  vtkLogIfF(ERROR, !read->HasMetaData(0u), "Expected metadata for Base_Volume_Elements ");
  vtkLogIfF(ERROR, !read->HasMetaData(1u), "Expected metadata for Base_Surface_Elements");

  vtkLogIfF(ERROR, !read->GetMetaData(0u)->Has(vtkCompositeDataSet::NAME()),
    "Expected name for Base_Volume_Elements ");
  vtkLogIfF(ERROR, !read->GetMetaData(1u)->Has(vtkCompositeDataSet::NAME()),
    "Expected name for Base_Surface_Elements");

  const char* name = read->GetMetaData(0u)->Get(vtkCompositeDataSet::NAME());
  vtkLogIfF(ERROR, 0 != strncmp("Base_Volume_Elements", name, 12),
    "Name '%s' is not Base_Volume_Elements", name);
  name = read->GetMetaData(1u)->Get(vtkCompositeDataSet::NAME());
  vtkLogIfF(ERROR, 0 != strncmp("Base_Surface_Elements", name, 12),
    "Name '%s' is not Base_Surface_Elements", name);

  int rc0 = UnstructuredGridTest(read, 0, 0, 10, "UNSTRUCTURED");
  vtkLogIfF(ERROR, EXIT_SUCCESS != rc0, "Failed unstructured grid test.");

  int rc1 = PolydataTest(read, 1, 0, "POLYDATA");
  vtkLogIfF(ERROR, EXIT_SUCCESS != rc1, "Failed polydata test.");

  return EXIT_SUCCESS;
}
