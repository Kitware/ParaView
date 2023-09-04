// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDSPIterator.h"

#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkHDFReader.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPExtractDataArraysOverTime.h"
#include "vtkTable.h"
#include "vtkTemporalMultiplexing.h"
#include "vtkTestUtilities.h"

namespace
{

//-----------------------------------------------------------------------------
bool CheckMDArrayInCorrectState(vtkTable* table)
{
  if (!table)
  {
    std::cerr << "Unable to retrieve the table corresponding to the first point!" << std::endl;
    return false;
  }

  // Check array
  vtkDataArray* array = vtkDataArray::SafeDownCast(table->GetColumnByName("Modulator"));

  if (!array)
  {
    std::cerr << "Missing 'Modulator' array." << std::endl;
    return false;
  }

  // Check number of tuples (number of timesteps)
  if (array->GetNumberOfTuples() != 10)
  {
    std::cerr << "'Modulator' multidimensional array should have 10 tuples but has "
              << array->GetNumberOfTuples() << "." << std::endl;
    return false;
  }

  // Check timestep 4 for point 0
  if (!vtkMathUtilities::FuzzyCompare(array->GetComponent(4, 0), 0.951057, 0.0001))
  {
    std::cerr << "Expected 0.951057 but got " << array->GetComponent(4, 0) << "." << std::endl;
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool CheckDSPIteratorWithObject(vtkDataObject* object)
{
  auto dspIterator = vtkDSPIterator::GetInstance(object);

  if (!dspIterator)
  {
    std::cerr << "Failed to generate iterator!" << std::endl;
    return false;
  }

  // Check first item
  dspIterator->GoToFirstItem();
  vtkTable* table = dspIterator->GetCurrentTable();

  if (!CheckMDArrayInCorrectState(table))
  {
    std::cerr << "MDArray not in correct state at first iteration" << std::endl;
    return false;
  }

  // Check total number of iterations and tables
  int nbIter = 1;
  dspIterator->GoToNextItem();

  while (!dspIterator->IsDoneWithTraversal())
  {
    if (!dspIterator->GetCurrentTable())
    {
      std::cerr << "Unable to retrieve the table corresponding to point " << nbIter << "!"
                << std::endl;
      return false;
    }

    nbIter++;
    dspIterator->GoToNextItem();
  }

  if (nbIter != 724)
  {
    std::cerr << "Number of iterations should be 724. Got " << nbIter << " instead." << std::endl;
    return false;
  }

  return true;
}
}

//-----------------------------------------------------------------------------
int TestDSPIterator(int argc, char* argv[])
{
  // Read temporal dataset
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/transient_sphere.hdf");
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  // Apply Plot Data Over Time to obtain a multiblock of tables (one block per point)
  vtkNew<vtkPExtractDataArraysOverTime> plotDataOverTime;
  plotDataOverTime->SetInputConnection(reader->GetOutputPort());
  plotDataOverTime->Update();

  // Check output
  vtkMultiBlockDataSet* outputMB =
    vtkMultiBlockDataSet::SafeDownCast(plotDataOverTime->GetOutputDataObject(0));

  if (!outputMB)
  {
    std::cerr << "Missing output of vtkPExtractDataArraysOverTime filter." << std::endl;
    return EXIT_FAILURE;
  }

  if (!::CheckDSPIteratorWithObject(outputMB))
  {
    std::cerr << "Failed test for multiblock output." << std::endl;
    return EXIT_FAILURE;
  }

  // Create multiplexing filter
  vtkNew<vtkTemporalMultiplexing> multiplexer;
  multiplexer->SetInputConnection(reader->GetOutputPort());
  multiplexer->EnableAttributeArray("Modulator");
  multiplexer->Update();

  // Check output
  vtkTable* output = vtkTable::SafeDownCast(multiplexer->GetOutputDataObject(0));

  if (!output)
  {
    std::cerr << "Missing output of vtkTemporalMultiplexing filter." << std::endl;
    return EXIT_FAILURE;
  }

  if (!::CheckDSPIteratorWithObject(output))
  {
    std::cerr << "Failed test for table output." << std::endl;
    return EXIT_FAILURE;
  }

  // run this check to make sure the iterator didn't break its input
  if (!::CheckMDArrayInCorrectState(output))
  {
    std::cerr << "Failed test after iteration." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
