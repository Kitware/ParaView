// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMeanPowerSpectralDensity.h"

#include "vtkAccousticUtilities.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDSPIterator.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <numeric>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMeanPowerSpectralDensity);

//------------------------------------------------------------------------------
int vtkMeanPowerSpectralDensity::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//------------------------------------------------------------------------------
int vtkMeanPowerSpectralDensity::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkTable* output = vtkTable::GetData(outputVector);

  if (!input || !output)
  {
    vtkErrorMacro("Missing valid input or output.");
    return 0;
  }
  if (this->FFTArrayName.empty())
  {
    vtkErrorMacro("Please specify an FFT array name.");
    return 0;
  }

  // Create DSP iterator according to input type
  auto iter = vtkDSPIterator::GetInstance(input);

  if (!iter)
  {
    vtkErrorMacro("Unable to generate iterator for the given input.");
    return 0;
  }

  iter->GoToFirstItem();

  // Check if input is empty
  if (iter->IsDoneWithTraversal())
  {
    return 1;
  }

  vtkIdType nbIterations = iter->GetNumberOfIterations();
  vtkTable* node = iter->GetCurrentTable();

  // Search for the FFT array
  vtkDataArray* fftArray =
    vtkDataArray::SafeDownCast(node->GetColumnByName(this->FFTArrayName.c_str()));

  if (!fftArray)
  {
    vtkErrorMacro("Could not find FFT array named " << this->FFTArrayName << ".");
    return 0;
  }

  // Retrieve optional ghost array
  vtkDataArray* ghostArray =
    vtkArrayDownCast<vtkDataArray>(node->GetColumnByName(vtkDataSetAttributes::GhostArrayName()));
  bool hasGhost = (ghostArray != nullptr);

  // Set number of values to (size - 1) because the first value is ignored
  // (DC frequency, not relevant for computing the PSD)
  vtkNew<vtkDoubleArray> res;
  res->SetNumberOfValues(fftArray->GetNumberOfTuples() - 1);
  auto resValueRange = vtk::DataArrayValueRange<1>(res);
  vtkSMPTools::Fill(resValueRange.begin(), resValueRange.end(), 0.0);

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

  // Copy frequency column if available
  vtkDataArray* freqArray = this->FrequencyArrayName.empty()
    ? nullptr
    : vtkDataArray::SafeDownCast(node->GetColumnByName(this->FrequencyArrayName.c_str()));
  if (freqArray)
  {
    vtkNew<vtkDoubleArray> freq;
    freq->SetName(this->FrequencyArrayName.c_str());
    freq->SetNumberOfValues(res->GetNumberOfValues());

    auto freqValueRange = vtk::DataArrayValueRange<1>(freqArray).GetSubRange(1);
    auto outFreqRange = vtk::DataArrayValueRange<1>(freq);

    assert(freqValueRange.size() == outFreqRange.size() && "FrequencyArray size is not coherent");

    using FreqType = decltype(freqValueRange)::ValueType;
    vtkSMPTools::Transform(freqValueRange.cbegin(), freqValueRange.cend(), outFreqRange.begin(),
      [](FreqType value) { return static_cast<double>(value); });

    output->AddColumn(freq);
  }

  // Compute sum of all FFTs over all microphones
  for (; !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    node = iter->GetCurrentTable();

    // Skip ghost points to avoid duplicates
    if (hasGhost)
    {
      ghostArray = vtkArrayDownCast<vtkDataArray>(
        node->GetColumnByName(vtkDataSetAttributes::GhostArrayName()));

      if (ghostArray && ghostArray->GetNumberOfTuples() > 0 && ghostArray->GetComponent(0, 0) != 0)
      {
        nbIterations--;
        continue;
      }
    }

    fftArray = vtkDataArray::SafeDownCast(node->GetColumnByName(this->FFTArrayName.c_str()));

    if (!fftArray)
    {
      vtkErrorMacro("Could not find FFT array named " << this->FFTArrayName << ".");
      return 0;
    }

    assert((fftArray->GetNumberOfTuples() - 1) == resValueRange.size() &&
      "fftArray size is not coherent");

    if (isComplex)
    {
      auto fftTupleRange = vtk::DataArrayTupleRange<2>(fftArray).GetSubRange(1);
      using TupleType = decltype(fftTupleRange)::ConstTupleReferenceType;
      vtkSMPTools::Transform(fftTupleRange.cbegin(), fftTupleRange.cend(), resValueRange.cbegin(),
        resValueRange.begin(),
        [](TupleType tuple, double value)
        {
          const double magnitude = std::hypot(*tuple.begin(), *(tuple.begin() + 1));
          return value + magnitude;
        });
    }
    else
    {
      auto fftValueRange = vtk::DataArrayValueRange<1>(fftArray).GetSubRange(1);
      using FFTType = decltype(fftValueRange)::ValueType;
      vtkSMPTools::Transform(fftValueRange.cbegin(), fftValueRange.cend(), resValueRange.cbegin(),
        resValueRange.begin(),
        // NOLINTNEXTLINE(readability-redundant-casting): `FFTType` might change.
        [](FFTType fft, double value) { return value + static_cast<double>(std::abs(fft)); });
    }
  }

  // Add array to output
  res->SetName("Mean PSD (dB)");
  output->AddColumn(res);
  vtkSmartPointer<vtkTable> localTable = output;

  // Retrieve controller for distributed context
  auto controller = vtkMultiProcessController::GetGlobalController();
  const vtkIdType nbProcesses = controller->GetNumberOfProcesses();

  // Gather all tables onto process 0
  std::vector<vtkSmartPointer<vtkDataObject>> allRanksTables(nbProcesses);
  std::vector<vtkIdType> allRanksNbIterations(nbProcesses, 0);
  controller->Gather(localTable, allRanksTables, 0);
  controller->Gather(&nbIterations, allRanksNbIterations.data(), 1, 0);

  // Reduce results on process 0
  if (controller->GetLocalProcessId() == 0)
  {
    // Compute total number of iterations for the mean PSD
    vtkIdType totalNbIterations = std::accumulate(
      allRanksNbIterations.begin(), allRanksNbIterations.end(), static_cast<vtkIdType>(0));

    // Start from first table
    vtkTable* reducedTable = vtkTable::SafeDownCast(allRanksTables[0]);
    vtkDoubleArray* firstPSDArray =
      vtkDoubleArray::SafeDownCast(reducedTable->GetColumnByName("Mean PSD (dB)"));
    auto firstRange = vtk::DataArrayValueRange<1>(firstPSDArray);

    // Compute sum of magnitudes across all processes
    for (vtkIdType idx = 1; idx < nbProcesses; idx++)
    {
      vtkTable* procTable = vtkTable::SafeDownCast(allRanksTables[idx]);
      vtkDoubleArray* psdArray =
        vtkDoubleArray::SafeDownCast(procTable->GetColumnByName("Mean PSD (dB)"));
      auto arrayRange = vtk::DataArrayValueRange<1>(psdArray);

      vtkSMPTools::Transform(arrayRange.cbegin(), arrayRange.cend(), firstRange.cbegin(),
        firstRange.begin(), [&](double x, double y) { return x + y; });
    }

    vtkSMPTools::Transform(firstRange.cbegin(), firstRange.cend(), firstRange.begin(),
      [&](double value)
      {
        return 10.0 *
          std::log10((value / totalNbIterations) /
            (vtkAccousticUtilities::REF_PRESSURE * vtkAccousticUtilities::REF_PRESSURE));
      });

    output->ShallowCopy(reducedTable);
  }
  else
  {
    output->Initialize();
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
