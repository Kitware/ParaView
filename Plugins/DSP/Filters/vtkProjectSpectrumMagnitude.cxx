// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkProjectSpectrumMagnitude.h"

#include "vtkCommand.h"
#include "vtkDSPIterator.h"
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
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
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
  auto dspIterator = vtkDSPIterator::GetInstance(vtkDataObject::GetData(inputVector[0]));
  if (!dspIterator)
  {
    vtkErrorMacro("Unable to generate iterator!");
    return 0;
  }

  dspIterator->GoToFirstItem();
  if (dspIterator->IsDoneWithTraversal())
  {
    // Silent return with empty input
    return 1;
  }

  vtkDataSet* input = vtkDataSet::GetData(inputVector[1]);
  if (input->GetNumberOfPoints() == 0)
  {
    // Silent return with empty input
    return 1;
  }

  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  // Retrieve table information in order to
  // find row indices for the requested frequency range
  vtkTable* firstTable = dspIterator->GetCurrentTable();
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
  const double lowerFrequency =
    this->FreqFromOctave ? this->ComputedLowerFrequency : this->LowerFrequency;
  const double upperFrequency =
    this->FreqFromOctave ? this->ComputedUpperFrequency : this->UpperFrequency;

  const std::array<vtkIdType, 2> freqRangeIndices = [&] {
    const vtkIdType nvalues = freqArray->GetNumberOfValues();
    vtkIdType idx = 0;
    while (idx < nvalues && freqArray->GetTuple1(idx) < lowerFrequency)
    {
      idx++;
    }
    const vtkIdType begin = idx;

    idx = nvalues - 1;
    while (idx >= 0 && freqArray->GetTuple1(idx) > upperFrequency)
    {
      idx--;
    }
    const vtkIdType end = idx + 1;
    return std::array<vtkIdType, 2>{ begin, end };
  }();

  // Copy the structure and arrays of the input
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
    outArray->SetNumberOfTuples(input->GetNumberOfPoints());
    outArray->SetName((colName).c_str());
    outArray->Fill(0.0);
    auto outRange = vtk::DataArrayTupleRange(outArray);
    auto outIterator = outRange.begin();
    const double componentFactor = 1.0 / firstTable->GetNumberOfRows();

    // Fill output array
    for (dspIterator->GoToFirstItem();
         !dspIterator->IsDoneWithTraversal() && outIterator != outRange.end();
         dspIterator->GoToNextItem(), outIterator++)
    {
      vtkTable* table = dspIterator->GetCurrentTable();
      if (!table)
      {
        continue;
      }
      vtkDataArray* inArray = vtkDataArray::SafeDownCast(table->GetColumnByName(colName.c_str()));
      if (!inArray)
      {
        vtkErrorMacro("Could not find array named " << colName << ".");
        return 0;
      }

      // Mean all values inside frequency range
      const auto inRange =
        vtk::DataArrayTupleRange(inArray, freqRangeIndices[0], freqRangeIndices[1]);
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

    if (!dspIterator->IsDoneWithTraversal() || outIterator != outRange.end())
    {
      vtkWarningMacro("Iteration over the two inputs has not completed because of a dimensional "
                      "issue, result may be incorrect");
    }

    output->GetPointData()->AddArray(outArray);
  }

  return 1;
}

//--------------------------------------- --------------------------------------
void vtkProjectSpectrumMagnitude::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LowerFrequency: "
     << (this->FreqFromOctave ? this->ComputedLowerFrequency : this->LowerFrequency) << std::endl;
  os << indent << "UpperFrequency: "
     << (this->FreqFromOctave ? this->ComputedUpperFrequency : this->UpperFrequency) << std::endl;
  os << indent << "ColumnSelection:\n";
  this->ColumnSelection->PrintSelf(os, indent.GetNextIndent());
  os << indent << "FreqFromOctave: " << (this->FreqFromOctave ? "On" : "Off") << std::endl;
  if (this->FreqFromOctave)
  {
    os << indent << "BaseTwoOctave: " << (this->BaseTwoOctave ? "On (base-2)" : "Off (base-10)")
       << std::endl;
    os << indent << "Octave: " << this->Octave << std::endl;
    os << indent << "OctaveSubdivision: " << this->OctaveSubdivision << std::endl;
  }
}

//--------------------------------------- --------------------------------------
void vtkProjectSpectrumMagnitude::ComputeFreqFromOctave()
{
  const std::array<double, 2> freqRange =
    vtkFFT::GetOctaveFrequencyRange(static_cast<vtkFFT::Octave>(this->Octave),
      static_cast<vtkFFT::OctaveSubdivision>(this->OctaveSubdivision), this->BaseTwoOctave);

  this->ComputedLowerFrequency = freqRange[0];
  this->ComputedUpperFrequency = freqRange[1];
}

//--------------------------------------- --------------------------------------
void vtkProjectSpectrumMagnitude::SetFreqFromOctave(bool freqFromOctave)
{
  if (this->FreqFromOctave != freqFromOctave)
  {
    this->FreqFromOctave = freqFromOctave;
    this->Modified();

    if (this->FreqFromOctave)
    {
      this->ComputeFreqFromOctave();
    }
  }
}

//--------------------------------------- --------------------------------------
void vtkProjectSpectrumMagnitude::SetBaseTwoOctave(bool baseTwoOctave)
{
  if (this->BaseTwoOctave != baseTwoOctave)
  {
    this->BaseTwoOctave = baseTwoOctave;
    this->Modified();

    if (this->FreqFromOctave)
    {
      this->ComputeFreqFromOctave();
    }
  }
}

//--------------------------------------- --------------------------------------
void vtkProjectSpectrumMagnitude::SetOctave(int octave)
{
  octave = octave < vtkFFT::Octave::Hz_31_5
    ? vtkFFT::Octave::Hz_31_5
    : (octave > vtkFFT::Octave::kHz_16 ? vtkFFT::Octave::kHz_16 : octave);
  if (this->Octave != octave)
  {
    this->Octave = octave;
    this->Modified();

    if (this->FreqFromOctave)
    {
      this->ComputeFreqFromOctave();
    }
  }
}

//--------------------------------------- --------------------------------------
void vtkProjectSpectrumMagnitude::SetOctaveSubdivision(int octaveSubdivision)
{
  octaveSubdivision = octaveSubdivision < vtkFFT::OctaveSubdivision::Full
    ? vtkFFT::OctaveSubdivision::Full
    : (octaveSubdivision > vtkFFT::OctaveSubdivision::ThirdThird
          ? vtkFFT::OctaveSubdivision::ThirdThird
          : octaveSubdivision);
  if (this->OctaveSubdivision != octaveSubdivision)
  {
    this->OctaveSubdivision = octaveSubdivision;
    this->Modified();

    if (this->FreqFromOctave)
    {
      this->ComputeFreqFromOctave();
    }
  }
}
