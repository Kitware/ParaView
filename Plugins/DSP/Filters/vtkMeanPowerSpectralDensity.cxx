/*=========================================================================

  Plugin:   DigitalSignalProcessing
  Module:   vtkMeanPowerSpectralDensity.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMeanPowerSpectralDensity.h"

#include "vtkAccousticUtilities.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMeanPowerSpectralDensity);

//------------------------------------------------------------------------------
int vtkMeanPowerSpectralDensity::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkMeanPowerSpectralDensity::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMultiBlockDataSet* input = vtkMultiBlockDataSet::GetData(inputVector[0]);
  if (!input)
  {
    vtkErrorMacro("Missing valid input");
    return 0;
  }
  if (this->FFTArrayName.empty())
  {
    vtkErrorMacro("Please specify an FFT array name.");
    return 0;
  }

  // Search for the FFT array
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(input->NewIterator());
  iter->SkipEmptyNodesOn();
  iter->GoToFirstItem();
  vtkTable* node = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
  vtkDataArray* fftArray =
    vtkDataArray::SafeDownCast(node->GetColumnByName(this->FFTArrayName.c_str()));

  if (!fftArray)
  {
    vtkErrorMacro("Could not find FFT array named " << this->FFTArrayName << ".");
    return 0;
  }

  // Set number of values to (size - 1) because the first value is ignored
  // (DC frequency, not relevant for computing the PSD)
  vtkNew<vtkDoubleArray> res;
  res->SetNumberOfValues(fftArray->GetNumberOfTuples() - 1);
  auto resValueRange = vtk::DataArrayValueRange(res);
  vtkSMPTools::Fill(resValueRange.begin(), resValueRange.end(), 0.0);

  int N = 0;
  bool isComplex = false;

  if (fftArray->GetNumberOfComponents() == 2)
  {
    isComplex = true;
  }
  else if (fftArray->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro("The selected FFT array should only have 1 or 2 components.");
    return 0;
  }

  // Compute sum of all FFTs over all microphones
  for (; !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    node = vtkTable::SafeDownCast(iter->GetCurrentDataObject());
    fftArray = vtkDataArray::SafeDownCast(node->GetColumnByName(this->FFTArrayName.c_str()));

    if (!fftArray)
    {
      vtkErrorMacro("Could not find FFT array named " << this->FFTArrayName << " in block "
                                                      << iter->GetCurrentFlatIndex());
      return 0;
    }

    assert((fftArray->GetNumberOfTuples() - 1) == resValueRange.size() &&
      "fftArray size is not coherent");

    if (isComplex)
    {
      auto fftTupleRange = vtk::DataArrayTupleRange<2>(fftArray).GetSubRange(1);
      using TupleType = decltype(fftTupleRange)::ConstTupleReferenceType;
      vtkSMPTools::Transform(fftTupleRange.cbegin(), fftTupleRange.cend(), resValueRange.cbegin(),
        resValueRange.begin(), [](TupleType tuple, double value) {
          const double magnitude = std::hypot(*tuple.begin(), *(tuple.begin() + 1));
          return value + magnitude;
        });
    }
    else
    {
      auto fftValueRange = vtk::DataArrayValueRange(fftArray).GetSubRange(1);
      using FFTType = decltype(fftValueRange)::ValueType;
      vtkSMPTools::Transform(fftValueRange.cbegin(), fftValueRange.cend(), resValueRange.cbegin(),
        resValueRange.begin(),
        [](FFTType fft, double value) { return value + static_cast<double>(std::abs(fft)); });
    }

    ++N;
  }

  // Compute mean PSD
  res->SetName("Mean PSD (dB)");

  vtkSMPTools::Transform(
    resValueRange.cbegin(), resValueRange.cend(), resValueRange.begin(), [N](double value) {
      return 10.0 *
        std::log10((value / N) /
          (vtkAccousticUtilities::REF_PRESSURE * vtkAccousticUtilities::REF_PRESSURE));
    });

  // Construct output
  vtkTable* output = vtkTable::GetData(outputVector);
  output->AddColumn(res);

  // Add frequency column if available
  vtkDataArray* freqArray = this->FrequencyArrayName.empty()
    ? nullptr
    : vtkDataArray::SafeDownCast(node->GetColumnByName(this->FrequencyArrayName.c_str()));
  if (freqArray)
  {
    vtkNew<vtkDoubleArray> freq;
    freq->SetName(this->FrequencyArrayName.c_str());
    freq->SetNumberOfValues(res->GetNumberOfValues());

    auto freqValueRange = vtk::DataArrayValueRange(freqArray).GetSubRange(1);
    auto outFreqRange = vtk::DataArrayValueRange(freq);

    assert(freqValueRange.size() == outFreqRange.size() && "FrequencyArray size is not coherent");

    using FreqType = decltype(freqValueRange)::ValueType;
    vtkSMPTools::Transform(freqValueRange.cbegin(), freqValueRange.cend(), outFreqRange.begin(),
      [](FreqType value) { return static_cast<double>(value); });

    output->AddColumn(freq);
  }

  return 1;
}

//--------------------------------------- --------------------------------------
void vtkMeanPowerSpectralDensity::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FFT Array Name:" << this->FFTArrayName << std::endl;
  os << indent << "Frequency Array Name:" << this->FrequencyArrayName << std::endl;
}
