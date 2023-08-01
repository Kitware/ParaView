// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTemporalMultiplexing.h"

#include "vtkHDFReader.h"
#include "vtkMathUtilities.h"
#include "vtkMultiDimensionalArray.h"
#include "vtkNew.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"

#include "vtkDataArraySelection.h"

//-----------------------------------------------------------------------------
int TestTemporalMultiplexing(int argc, char* argv[])
{
  // Read temporal dataset
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/transient_sphere.hdf");
  vtkNew<vtkHDFReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

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

  // Check array
  vtkMultiDimensionalArray<float>* array =
    vtkMultiDimensionalArray<float>::SafeDownCast(output->GetColumnByName("Modulator"));

  if (!array)
  {
    std::cerr << "Missing 'Modulator' multidimensional array." << std::endl;
    return EXIT_FAILURE;
  }

  // Check number of tuples (number of timesteps)
  if (array->GetNumberOfTuples() != 10)
  {
    std::cerr << "'Modulator' multidimensional array should have 10 tuples but has "
              << array->GetNumberOfTuples() << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Check number of arrays (number of mesh points)
  if (array->GetNumberOfArrays() != 724)
  {
    std::cerr << "'Modulator' multidimensional array should have 724 arrays but has "
              << array->GetNumberOfArrays() << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Check timestep 6 for point 0
  if (!vtkMathUtilities::FuzzyCompare(array->GetValue(6), 0.951057f, 0.0001f))
  {
    std::cerr << "Expected 0.951057 but got " << array->GetValue(6) << "." << std::endl;
    return EXIT_FAILURE;
  }

  // Change index = change point
  array->SetIndex(10);

  // Check timestep 1 for point 10
  if (!vtkMathUtilities::FuzzyCompare(array->GetValue(1), 0.725975f, 0.0001f))
  {
    std::cerr << "Expected 0.725975 but got " << array->GetValue(1) << "." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
};
