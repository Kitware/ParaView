/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DistributedData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test focus on noticed buggy behaviour based on the smallest basis

#include "vtkDoubleArray.h"
#include "vtkDummyController.h"
#include "vtkMultiProcessController.h"
#include "vtkSmartPointer.h"
#include "vtkSortedTableStreamer.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"

#include <float.h>
// ----------------------------------------------------------------------------
void fillArray(vtkDoubleArray* array, double* dataPointer, int dataSize, const char* name)
{
  array->SetName(name);
  array->SetNumberOfComponents(1);
  array->Allocate(dataSize);
  for (int i = 0; i < dataSize; i++)
  {
    array->InsertNextTuple1(dataPointer[i]);
  }
}

// ----------------------------------------------------------------------------
// Return true if both array are equals
bool compareArray(
  vtkTable* table, const char* arrayName, double* dataPointer, int dataSize, bool print)
{
  vtkDoubleArray* array = vtkDoubleArray::SafeDownCast(table->GetColumnByName(arrayName));
  if (!array)
    return false;

  if (dataSize != array->GetNumberOfTuples())
    return false;

  for (int i = 0; i < dataSize; i++)
  {
    if (print)
      cout << "Sorted value: " << array->GetValue(i) << " expected " << dataPointer[i] << endl;
    if (array->GetValue(i) != dataPointer[i])
      return false;
  }

  return true;
}

// ----------------------------------------------------------------------------
int sortWithSimilarValues(bool debug)
{
  const int size = 10;
  double dataArray[size] = { 0, 1, 2, 1, 3, 1, 3, 1, 2, 100000 };
  double sortedArray[size] = { 0, 1, 1, 1, 1, 2, 2, 3, 3, 100000 };

  vtkSmartPointer<vtkDoubleArray> dataToSort = vtkSmartPointer<vtkDoubleArray>::New();
  fillArray(dataToSort.GetPointer(), dataArray, size, "data");

  vtkSmartPointer<vtkTable> input = vtkSmartPointer<vtkTable>::New();
  input->AddColumn(dataToSort);
  ;
  vtkSmartPointer<vtkSortedTableStreamer> sortingfilter =
    vtkSmartPointer<vtkSortedTableStreamer>::New();

  sortingfilter->SetInputData(input.GetPointer());
  sortingfilter->SetSelectedComponent(0);
  sortingfilter->SetColumnNameToSort("data");

  sortingfilter->SetBlock(0);
  sortingfilter->SetBlockSize(1024);
  sortingfilter->Update();

  if (!compareArray(sortingfilter->GetOutput(), "data", sortedArray, size, debug))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

// ----------------------------------------------------------------------------
int sortWithEpsilonValues(bool debug)
{
  double epsilon = 1000 * FLT_EPSILON; // FIXME why the delta need to be so big ???
  const int size = 10;
  double dataArray[size];
  double sortedArray[size];
  for (int i = 0; i < size; i++)
  {
    dataArray[size - i - 1] = sortedArray[i] = 10 + epsilon * i;
  }

  vtkSmartPointer<vtkDoubleArray> dataToSort = vtkSmartPointer<vtkDoubleArray>::New();
  fillArray(dataToSort.GetPointer(), dataArray, size, "data");

  vtkSmartPointer<vtkTable> input = vtkSmartPointer<vtkTable>::New();
  input->AddColumn(dataToSort);
  ;
  vtkSmartPointer<vtkSortedTableStreamer> sortingfilter =
    vtkSmartPointer<vtkSortedTableStreamer>::New();

  sortingfilter->SetInputData(input.GetPointer());
  sortingfilter->SetSelectedComponent(0);
  sortingfilter->SetColumnNameToSort("data");

  sortingfilter->SetBlock(0);
  sortingfilter->SetBlockSize(1024);
  sortingfilter->Update();

  if (!compareArray(sortingfilter->GetOutput(), "data", sortedArray, size, debug))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

// ----------------------------------------------------------------------------
int sortMagnitudeOnUnsignedCharVector()
{
  vtkSmartPointer<vtkUnsignedCharArray> dataToSort = vtkSmartPointer<vtkUnsignedCharArray>::New();
  dataToSort->SetNumberOfComponents(3);
  dataToSort->SetName("data");
  dataToSort->Allocate(3 * 128 * 128 * 128);
  for (int r = 0; r < 256; r += 2)
  {
    for (int g = 0; g < 256; g += 2)
    {
      for (int b = 0; b < 256; b += 2)
      {
        dataToSort->InsertNextTuple3(r, g, b);
      }
    }
  }

  vtkSmartPointer<vtkTable> input = vtkSmartPointer<vtkTable>::New();
  input->AddColumn(dataToSort);
  ;
  vtkSmartPointer<vtkSortedTableStreamer> sortingfilter =
    vtkSmartPointer<vtkSortedTableStreamer>::New();

  sortingfilter->SetInputData(input.GetPointer());
  sortingfilter->SetSelectedComponent(-1); // Magnitude
  sortingfilter->SetColumnNameToSort("data");

  sortingfilter->SetBlock(0);
  sortingfilter->SetBlockSize(1024);
  sortingfilter->Update();

  return EXIT_SUCCESS;
}

// ----------------------------------------------------------------------------
int TestSortingTable(int vtkNotUsed(argc), char** vtkNotUsed(argv))
{
  // Create Fake MPI controller
  vtkDummyController* ctrl = vtkDummyController::New();
  vtkMultiProcessController::SetGlobalController(ctrl);
  int result = 0;
  bool debug = false;

  // --------------------------------------------------------------------------
  cout << "Testing sorting with similar values: "
       << ((result += sortWithSimilarValues(debug)) ? "FAILED" : "SUCCESS") << endl;
  // --------------------------------------------------------------------------
  cout << "Testing sorting with epsilon values: "
       << ((result += sortWithEpsilonValues(debug)) ? "FAILED" : "SUCCESS") << endl;
  // --------------------------------------------------------------------------
  cout << "Testing sorting with magnitude on unsigned char: "
       << ((result += sortMagnitudeOnUnsignedCharVector()) ? "FAILED" : "SUCCESS") << endl;
  // --------------------------------------------------------------------------
  // --------------------------------------------------------------------------

  // Delete Fake MPI controller
  vtkMultiProcessController::SetGlobalController(0);
  ctrl->Delete();

  return result;
}
