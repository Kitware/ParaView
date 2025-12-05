// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVMergeTablesComposite.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkVariant.h"
#include "vtkXMLTableReader.h"

#include <algorithm>
#include <iostream>

#define expect(x, msg)                                                                             \
  if (!(x))                                                                                        \
  {                                                                                                \
    std::cerr << __LINE__ << ": " msg << endl;                                                     \
    return EXIT_FAILURE;                                                                           \
  }

namespace
{
vtkSmartPointer<vtkTable> ReadAsTable(const char* fname)
{
  vtkNew<vtkXMLTableReader> reader0;
  reader0->SetFileName(fname);
  reader0->Update();
  return reader0->GetOutput();
}

void SortTableColumnsByName(vtkTable* table)
{
  if (!table || table->GetNumberOfColumns() == 0)
  {
    return;
  }

  auto rowData = table->GetRowData();
  const int numColumns = rowData->GetNumberOfArrays();

  // Create a vector of (column_name, array_pointer) pairs
  std::vector<std::pair<std::string, vtkSmartPointer<vtkAbstractArray>>> columnPairs;
  columnPairs.reserve(numColumns);

  // Collect all columns with their names
  for (int i = 0; i < numColumns; ++i)
  {
    auto array = rowData->GetAbstractArray(i);
    std::string name = array->GetName() ? array->GetName() : "";
    columnPairs.emplace_back(name, array);
  }

  // Sort by column name
  std::sort(columnPairs.begin(), columnPairs.end(),
    [](const auto& a, const auto& b) { return a.first < b.first; });

  // Clear the existing row data
  rowData->Initialize();

  // Add columns back in sorted order
  for (const auto& pair : columnPairs)
  {
    rowData->AddArray(pair.second);
  }
}
}

extern int TestMergeTablesComposite(int argc, char* argv[])
{
  char* fname0 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/table_0.vtt");
  auto data0 = ReadAsTable(fname0);
  SortTableColumnsByName(data0);
  delete[] fname0;

  char* fname1 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/table_1.vtt");
  auto data1 = ReadAsTable(fname1);
  SortTableColumnsByName(data1);
  delete[] fname1;

  // First, test when merging should merge tables.
  vtkNew<vtkMultiBlockDataSet> mb0;
  mb0->SetBlock(0, data0);

  vtkNew<vtkMultiBlockDataSet> mb1;
  mb1->SetBlock(0, data1);

  vtkNew<vtkPVMergeTablesComposite> merger;
  merger->SetInputDataObject(0, mb0);
  merger->AddInputDataObject(0, mb1);
  merger->Update();

  auto mergerOutput = vtkMultiBlockDataSet::SafeDownCast(merger->GetOutput());
  if (true)
  {
    vtkTable* result = vtkTable::SafeDownCast(mergerOutput->GetBlock(0));
    expect(result->GetNumberOfRows() == data0->GetNumberOfRows() + data1->GetNumberOfRows(),
      "mismatched rows");
    expect(result->GetNumberOfColumns() == data0->GetNumberOfColumns(), "columns mismatched.");
    expect(result->GetNumberOfColumns() == data1->GetNumberOfColumns(), "columns mismatched.");
    for (vtkIdType rr = 0; rr < data0->GetNumberOfRows(); rr++)
    {
      for (vtkIdType cc = 0; cc < data0->GetNumberOfColumns(); cc++)
      {
        expect(result->GetValue(rr, cc) == data0->GetValue(rr, cc),
          "data0 value mismatched at (" << rr << "," << cc << ")");
      }
    }
    for (vtkIdType rr = 0; rr < data1->GetNumberOfRows(); rr++)
    {
      for (vtkIdType cc = 0; cc < data1->GetNumberOfColumns(); cc++)
      {
        expect(result->GetValue(rr + data0->GetNumberOfRows(), cc) == data1->GetValue(rr, cc),
          "data1 value mismatched at (" << rr << "," << cc << ")");
      }
    }
  }

  // Second, test if merging shouldn't merge tables.
  // First, test when merging should merge tables.
  mb0->SetBlock(0, data0);
  mb0->SetBlock(1, nullptr);
  mb1->SetBlock(0, nullptr);
  mb1->SetBlock(1, data1);
  merger->Update();
  mergerOutput = vtkMultiBlockDataSet::SafeDownCast(merger->GetOutput());

  if (true)
  {
    vtkTable* result0 = vtkTable::SafeDownCast(mergerOutput->GetBlock(0));
    expect(result0->GetNumberOfRows() == data0->GetNumberOfRows(), "mismatched rows");
    expect(result0->GetNumberOfColumns() == data0->GetNumberOfColumns(), "columns mismatched.");
    for (vtkIdType rr = 0; rr < data0->GetNumberOfRows(); rr++)
    {
      for (vtkIdType cc = 0; cc < data0->GetNumberOfColumns(); cc++)
      {
        expect(result0->GetValue(rr, cc) == data0->GetValue(rr, cc),
          "data0 value mismatched at (" << rr << "," << cc << ")");
      }
    }
    vtkTable* result1 = vtkTable::SafeDownCast(mergerOutput->GetBlock(1));
    expect(result1->GetNumberOfRows() == data1->GetNumberOfRows(), "mismatched rows");
    expect(result1->GetNumberOfColumns() == data1->GetNumberOfColumns(), "columns mismatched.");
    for (vtkIdType rr = 0; rr < data1->GetNumberOfRows(); rr++)
    {
      for (vtkIdType cc = 0; cc < data1->GetNumberOfColumns(); cc++)
      {
        expect(result1->GetValue(rr, cc) == data1->GetValue(rr, cc),
          "data1 value mismatched at (" << rr << "," << cc << ")");
      }
    }
  }

  return EXIT_SUCCESS;
}
