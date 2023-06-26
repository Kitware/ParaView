// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDSPIterator.h"

#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkHDFReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMathUtilities.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPExtractDataArraysOverTime.h"
#include "vtkTable.h"
#include "vtkTableAlgorithm.h"
#include "vtkTemporalMultiplexing.h"
#include "vtkTestUtilities.h"

//-----------------------------------------------------------------------------
class vtkIteratorTestFilter : public vtkTableAlgorithm
{
public:
  static vtkIteratorTestFilter* New();
  vtkTypeMacro(vtkIteratorTestFilter, vtkTableAlgorithm);

protected:
  vtkIteratorTestFilter() = default;
  ~vtkIteratorTestFilter() override = default;

  int FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info) override
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    return 1;
  }

  int RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* vtkNotUsed(outputVector)) override
  {
    vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
    if (!input)
    {
      vtkErrorMacro("Unable to retrieve the input!");
      return 0;
    }

    auto dspIterator = vtkDSPIterator::GetInstance(input);
    if (!dspIterator)
    {
      vtkErrorMacro("Unable to generate iterator!");
      return 0;
    }

    for (dspIterator->GoToFirstItem(); !dspIterator->IsDoneWithTraversal();
         dspIterator->GoToNextItem())
    {
      vtkTable* table = dspIterator->GetCurrentTable();
      if (!table)
      {
        vtkErrorMacro("Unable to retrieve the table!");
        return 0;
      }

      vtkDataArray* array = vtkDataArray::SafeDownCast(table->GetColumn(0));
      if (!array)
      {
        vtkErrorMacro("Unable to retrieve data array!");
        return 0;
      }

      if (array->GetNumberOfTuples() == 0)
      {
        vtkErrorMacro("No values in array!");
        return 0;
      }

      if (array->GetNumberOfComponents() != 1)
      {
        vtkErrorMacro("Wrong number of components!");
        return 0;
      }
    }

    return 1;
  }

private:
  vtkIteratorTestFilter(const vtkIteratorTestFilter&) = delete;
  void operator=(const vtkIteratorTestFilter&) = delete;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkIteratorTestFilter);

//-----------------------------------------------------------------------------
int TestDSPIteratorIntegration(int argc, char* argv[])
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

  // Apply test filter
  vtkNew<vtkIteratorTestFilter> testFilter;
  testFilter->SetInputConnection(plotDataOverTime->GetOutputPort());
  testFilter->Update();

  // Create multiplexing filter
  vtkNew<vtkTemporalMultiplexing> multiplexer;
  multiplexer->SetInputConnection(reader->GetOutputPort());
  multiplexer->EnableAttributeArray("Modulator");

  // Apply test filter
  testFilter->SetInputConnection(multiplexer->GetOutputPort());
  testFilter->Update();

  return EXIT_SUCCESS;
};
