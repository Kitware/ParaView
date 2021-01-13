/*=========================================================================

  Program:   ParaView
  Module:    TestDataTabulator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataTabulator.h"
#include "vtkFieldData.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPolyData.h"

#include <algorithm>
#include <initializer_list>

#define VERIFY(x, y)                                                                               \
  if (!(x))                                                                                        \
  {                                                                                                \
    vtkLogF(ERROR, y);                                                                             \
    return false;                                                                                  \
  }

namespace
{
template <typename ArrayT, typename ElemT>
vtkSmartPointer<ArrayT> CreateArray(
  const std::string& arrayname, const std::initializer_list<ElemT>& vals)
{
  auto array = vtkSmartPointer<ArrayT>::New();
  array->SetName(arrayname.c_str());
  array->SetNumberOfTuples(vals.size());
  std::copy(vals.begin(), vals.end(), array->GetPointer(0));
  return array;
}

bool TestMultiblockFieldData()
{
  vtkNew<vtkMultiBlockDataSet> mb;
  mb->GetFieldData()->AddArray(CreateArray<vtkIntArray>("Var1", { 1, 2, 3 }));

  vtkNew<vtkPolyData> pd;
  mb->SetBlock(0, pd);
  pd->GetFieldData()->AddArray(CreateArray<vtkIntArray>("Var3", { 4, 5 }));
  pd->GetFieldData()->AddArray(CreateArray<vtkIntArray>("Var2", { 100 }));

  vtkNew<vtkDataTabulator> tabulator;
  tabulator->SetInputData(mb);
  tabulator->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_NONE);
  tabulator->Update();

  auto output = vtkPartitionedDataSet::SafeDownCast(tabulator->GetOutputDataObject(0));
  VERIFY(output != nullptr, "vtkPartitionedDataSet expected.");
  VERIFY(output->GetNumberOfPartitions() == 2, "2 partitions expected.");
  VERIFY(vtkDataTabulator::HasInputCompositeIds(output),
    "HasInputCompositeIds returned incorrect status");
  VERIFY(output->GetPartitionAsDataObject(0)
           ->GetAttributesAsFieldData(vtkDataObject::ROW)
           ->GetNumberOfTuples() == 3,
    "block 0: expecting 3 tuples");
  VERIFY(output->GetPartitionAsDataObject(1)
           ->GetAttributesAsFieldData(vtkDataObject::ROW)
           ->GetNumberOfTuples() == 2,
    "block 1: expecting 2 tuples");
  return true;
}

bool TestNonMultiblockFieldData()
{
  vtkNew<vtkPolyData> pd;
  pd->GetFieldData()->AddArray(CreateArray<vtkIntArray>("Var3", { 4, 5 }));
  pd->GetFieldData()->AddArray(CreateArray<vtkIntArray>("Var2", { 100 }));

  vtkNew<vtkDataTabulator> tabulator;
  tabulator->SetInputData(pd);
  tabulator->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_NONE);
  tabulator->Update();

  auto output = vtkPartitionedDataSet::SafeDownCast(tabulator->GetOutputDataObject(0));
  VERIFY(output != nullptr, "vtkPartitionedDataSet expected.");
  VERIFY(output->GetNumberOfPartitions() == 1, "1 partitions expected.");
  VERIFY(vtkDataTabulator::HasInputCompositeIds(output) == false,
    "HasInputCompositeIds returned incorrect status");
  VERIFY(output->GetPartitionAsDataObject(0)
           ->GetAttributesAsFieldData(vtkDataObject::ROW)
           ->GetNumberOfTuples() == 2,
    "block 0: expecting 2 tuples");
  return true;
}
}

int TestDataTabulator(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  return TestMultiblockFieldData() && TestNonMultiblockFieldData() ? EXIT_SUCCESS : EXIT_FAILURE;
}
