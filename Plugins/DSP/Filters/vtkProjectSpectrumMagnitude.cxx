/*=========================================================================

  Plugin:   DigitalSignalProcessing
  Module:   vtkProjectSpectrumMagnitude.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkProjectSpectrumMagnitude.h"

#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <array>
#include <cmath>
#include <vector>
#include <vtkDataArrayRange.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkProjectSpectrumMagnitude);

// ----------------------------------------------------------------------------
vtkProjectSpectrumMagnitude::vtkProjectSpectrumMagnitude()
{
  this->SetNumberOfInputPorts(2);

  // Add observer for selection update
  this->ColumnSelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkProjectSpectrumMagnitude::Modified);
}

//------------------------------------------------------------------------------
void vtkProjectSpectrumMagnitude::SetSourceData(vtkDataSet* source)
{
  this->SetInputData(1, source);
}

//------------------------------------------------------------------------------
void vtkProjectSpectrumMagnitude::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
int vtkProjectSpectrumMagnitude::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    return 1;
  }

  return 0;
}

//------------------------------------------------------------------------------
int vtkProjectSpectrumMagnitude::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[1]);

  if (!input)
  {
    vtkErrorMacro("Missing input!");
    return 0;
  }

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));

  if (!output || !output->IsA(input->GetClassName()))
  {
    vtkDataSet* newOutput = input->NewInstance();
    info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    newOutput->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkProjectSpectrumMagnitude::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Output is not temporal
  auto outInfo = outputVector->GetInformationObject(0);
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return 1;
}

//------------------------------------------------------------------------------
int vtkProjectSpectrumMagnitude::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMultiBlockDataSet* tables = vtkMultiBlockDataSet::GetData(inputVector[0]);
  if (!tables)
  {
    vtkErrorMacro("Missing valid input");
    return 0;
  }

  vtkDataSet* input = vtkDataSet::GetData(inputVector[1]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  const vtkIdType nbBlocks = tables->GetNumberOfBlocks();
  if (nbBlocks == 0)
  {
    return 1;
  }

  if (input->GetNumberOfPoints() != nbBlocks)
  {
    vtkErrorMacro("Number of mesh points does not match number of table blocks!");
    return 0;
  }

  // Retrieve table information in order to
  // find row indices for the requested frequency range
  vtkTable* firstTable = vtkTable::SafeDownCast(tables->GetBlock(0));
  if (firstTable == nullptr)
  {
    vtkErrorMacro("Invalid block type");
    return 0;
  }
  vtkDataArray* freqArray = this->GetInputArrayToProcess(0, firstTable);
  if (!freqArray || freqArray->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro("Could not find a valid frequency array.");
    return 0;
  }

  // Retrieve the frequency range we should process.
  // We assume frequency bins are the same for all blocks and frequency is sorted
  const std::array<vtkIdType, 2> freqRange = [&] {
    const vtkIdType nvalues = freqArray->GetNumberOfValues();
    vtkIdType idx = 0;
    while (idx < nvalues && freqArray->GetTuple1(idx) < this->LowerFrequency)
    {
      idx++;
    }
    const vtkIdType begin = idx;

    idx = nvalues - 1;
    while (idx >= 0 && freqArray->GetTuple1(idx) > this->UpperFrequency)
    {
      idx--;
    }
    const vtkIdType end = idx + 1;
    return std::array<vtkIdType, 2>{ begin, end };
  }();

  // Configure the output
  output->CopyStructure(input);
  output->CopyAttributes(input);

  // Loop over selected arrays
  for (vtkIdType arrIdx = 0; arrIdx < firstTable->GetNumberOfColumns(); arrIdx++)
  {
    // Check that array was selected
    const std::string colName = firstTable->GetColumnName(arrIdx);
    if (!this->ColumnSelection->ArrayIsEnabled(colName.c_str()))
    {
      continue;
    }

    // Prepare output array
    vtkDataArray* modelArray =
      vtkDataArray::SafeDownCast(firstTable->GetColumnByName(colName.c_str()));
    if (!modelArray)
    {
      vtkWarningMacro("Could not find array named " << colName << ".");
      continue;
    }
    auto outArray = vtk::TakeSmartPointer(modelArray->NewInstance());
    outArray->SetNumberOfComponents(modelArray->GetNumberOfComponents());
    outArray->SetNumberOfTuples(nbBlocks);
    outArray->SetName((colName).c_str());
    outArray->Fill(0.0);
    auto outRange = vtk::DataArrayTupleRange(outArray);
    auto outIterator = outRange.begin();
    const double componentFactor = 1.0 / firstTable->GetNumberOfRows();

    // Fill output array
    auto iter = vtk::TakeSmartPointer(tables->NewIterator());
    iter->SkipEmptyNodesOn();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), outIterator++)
    {
      vtkTable* table = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
      if (table == nullptr)
      {
        vtkErrorMacro("Invalid block type");
        return 0;
      }
      vtkDataArray* inArray = vtkDataArray::SafeDownCast(table->GetColumnByName(colName.c_str()));
      if (!inArray)
      {
        vtkErrorMacro("Could not find array named " << colName << ".");
        return 0;
      }

      // Mean all values inside frequency range
      const auto inRange = vtk::DataArrayTupleRange(inArray, freqRange[0], freqRange[1]);
      for (const auto inTuple : inRange)
      {
        auto outComponent = outIterator->begin();
        for (const auto inComponent : inTuple)
        {
          *outComponent += inComponent * componentFactor;
          outComponent++;
        }
      }
    }

    output->GetPointData()->AddArray(outArray);
  }

  return 1;
}

//--------------------------------------- --------------------------------------
void vtkProjectSpectrumMagnitude::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LowerFrequency: " << this->LowerFrequency << std::endl;
  os << indent << "UpperFrequency: " << this->UpperFrequency << std::endl;
  os << indent << "ColumnSelection:\n";
  this->ColumnSelection->PrintSelf(os, indent.GetNextIndent());
}
