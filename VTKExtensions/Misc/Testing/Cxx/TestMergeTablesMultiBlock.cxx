/*=========================================================================

  Program:   ParaView
  Module:    TestMergeTablesMultiBlock.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPVMergeTablesMultiBlock.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkVariant.h"
#include "vtkXMLTableReader.h"

#define expect(x, msg)                                                                             \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << __LINE__ << ": " msg << endl;                                                          \
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
}

int TestMergeTablesMultiBlock(int argc, char* argv[])
{
  char* fname0 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/table_0.vtt");
  auto data0 = ReadAsTable(fname0);
  delete[] fname0;

  char* fname1 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/table_1.vtt");
  auto data1 = ReadAsTable(fname1);
  delete[] fname1;

  // First, test when merging should merge tables.
  vtkNew<vtkMultiBlockDataSet> mb0;
  mb0->SetBlock(0, data0);

  vtkNew<vtkMultiBlockDataSet> mb1;
  mb1->SetBlock(0, data1);

  vtkNew<vtkPVMergeTablesMultiBlock> merger;
  merger->SetInputDataObject(0, mb0);
  merger->AddInputDataObject(0, mb1);
  merger->Update();

  if (true)
  {
    vtkTable* result = vtkTable::SafeDownCast(merger->GetOutput()->GetBlock(0));
    expect(result->GetNumberOfRows() == data0->GetNumberOfRows() + data1->GetNumberOfRows(),
      "mismatched rows");
    expect(result->GetNumberOfColumns() == data0->GetNumberOfColumns(), "columns mismatched.");
    expect(result->GetNumberOfColumns() == data1->GetNumberOfColumns(), "columns mismatched.");
    for (vtkIdType rr = 0; rr < data0->GetNumberOfRows(); rr++)
    {
      for (vtkIdType cc = 0; cc < data0->GetNumberOfColumns(); cc++)
      {
        expect(result->GetValue(rr, cc) == data0->GetValue(rr, cc), "data0 value mismatched at ("
            << rr << "," << cc << ")");
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

  if (true)
  {
    vtkTable* result0 = vtkTable::SafeDownCast(merger->GetOutput()->GetBlock(0));
    expect(result0->GetNumberOfRows() == data0->GetNumberOfRows(), "mismatched rows");
    expect(result0->GetNumberOfColumns() == data0->GetNumberOfColumns(), "columns mismatched.");
    for (vtkIdType rr = 0; rr < data0->GetNumberOfRows(); rr++)
    {
      for (vtkIdType cc = 0; cc < data0->GetNumberOfColumns(); cc++)
      {
        expect(result0->GetValue(rr, cc) == data0->GetValue(rr, cc), "data0 value mismatched at ("
            << rr << "," << cc << ")");
      }
    }
    vtkTable* result1 = vtkTable::SafeDownCast(merger->GetOutput()->GetBlock(1));
    expect(result1->GetNumberOfRows() == data1->GetNumberOfRows(), "mismatched rows");
    expect(result1->GetNumberOfColumns() == data1->GetNumberOfColumns(), "columns mismatched.");
    for (vtkIdType rr = 0; rr < data1->GetNumberOfRows(); rr++)
    {
      for (vtkIdType cc = 0; cc < data1->GetNumberOfColumns(); cc++)
      {
        expect(result1->GetValue(rr, cc) == data1->GetValue(rr, cc), "data1 value mismatched at ("
            << rr << "," << cc << ")");
      }
    }
  }

  return EXIT_SUCCESS;
}
