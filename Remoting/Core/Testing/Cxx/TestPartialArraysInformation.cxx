/*=========================================================================

  Program:   ParaView
  Module:    TestPartialArraysInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

vtkSmartPointer<vtkPolyData> GetPolyData(
  const char** parrays = nullptr, const char** carrays = nullptr)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->Update();

  vtkSmartPointer<vtkPolyData> pd = sphere->GetOutput();

  vtkIdType numPts = pd->GetNumberOfPoints();
  for (int cc = 0; parrays != nullptr && parrays[cc] != nullptr; ++cc)
  {
    vtkNew<vtkIntArray> array;
    array->SetName(parrays[cc]);
    array->SetNumberOfComponents(numPts);
    array->FillComponent(0, 0.0);
    pd->GetPointData()->AddArray(array.Get());
  }

  vtkIdType numCells = pd->GetNumberOfCells();
  for (int cc = 0; carrays != nullptr && carrays[cc] != nullptr; ++cc)
  {
    vtkNew<vtkIntArray> array;
    array->SetName(carrays[cc]);
    array->SetNumberOfComponents(numCells);
    array->FillComponent(0, 0.0);
    pd->GetCellData()->AddArray(array.Get());
  }

  return pd;
}

int TestPartialArraysInformation(int, char* [])
{
  vtkNew<vtkMultiBlockDataSet> data;

  const char* p00[] = { "pd0", nullptr };
  const char* c00[] = { "cd0", nullptr };
  data->SetBlock(0, GetPolyData(p00, c00));

  const char* c01[] = { "cd0", "cd1", nullptr };
  data->SetBlock(1, GetPolyData(p00, c01));

  vtkNew<vtkMultiBlockDataSet> base;
  const char* p10[] = { "pd0", nullptr };
  base->SetBlock(0, GetPolyData(p10));
  base->SetBlock(1, data.Get());

  vtkNew<vtkMultiBlockDataSet> root;
  root->SetBlock(0, base.Get());

  vtkNew<vtkPVDataInformation> info;
  info->CopyFromObject(root.Get());

  if (info->GetArrayInformation("pd0", vtkDataObject::POINT) == nullptr)
  {
    cerr << "ERROR: failed to find `pd0`." << endl;
    return EXIT_FAILURE;
  }
  if (info->GetArrayInformation("pd0", vtkDataObject::POINT)->GetIsPartial())
  {
    cerr << "ERROR: 'pd0' should not have been flagged as partial.";
    return EXIT_FAILURE;
  }

  if (info->GetArrayInformation("cd0", vtkDataObject::CELL) == nullptr)
  {
    cerr << "ERROR: failed to find `cd0`." << endl;
    return EXIT_FAILURE;
  }
  if (!info->GetArrayInformation("cd0", vtkDataObject::CELL)->GetIsPartial())
  {
    cerr << "ERROR: 'cd0' should have been flagged as partial.";
    return EXIT_FAILURE;
  }

  if (info->GetArrayInformation("cd1", vtkDataObject::CELL) == nullptr)
  {
    cerr << "ERROR: failed to find `cd1`." << endl;
    return EXIT_FAILURE;
  }
  if (!info->GetArrayInformation("cd1", vtkDataObject::CELL)->GetIsPartial())
  {
    cerr << "ERROR: 'cd1' should have been flagged as partial.";
    return EXIT_FAILURE;
  }

  // Now gather information for a specific block.
  vtkNew<vtkPVDataInformation> b1info;
  b1info->SetSubsetAssemblyNameToHierarchy();
  b1info->SetSubsetSelector("//Block0/Block1");
  b1info->CopyFromObject(root.Get());

  if (b1info->GetArrayInformation("cd0", vtkDataObject::CELL) == nullptr)
  {
    cerr << "ERROR: failed to find `cd0` on block 1." << endl;
    return EXIT_FAILURE;
  }
  if (b1info->GetArrayInformation("cd0", vtkDataObject::CELL)->GetIsPartial())
  {
    cerr << "ERROR: 'cd0' should not have been flagged as partial on block 1";
    return EXIT_FAILURE;
  }

  if (b1info->GetArrayInformation("cd1", vtkDataObject::CELL) == nullptr)
  {
    cerr << "ERROR: failed to find `cd1` on block 1." << endl;
    return EXIT_FAILURE;
  }
  if (!b1info->GetArrayInformation("cd1", vtkDataObject::CELL)->GetIsPartial())
  {
    cerr << "ERROR: 'cd1' should have been flagged as partial on block 1";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
