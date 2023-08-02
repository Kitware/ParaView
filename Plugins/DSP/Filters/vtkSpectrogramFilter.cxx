// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSpectrogramFilter.h"

#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFFT.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTableFFT.h"

#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSpectrogramFilter);

//------------------------------------------------------------------------------
int vtkSpectrogramFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");

  return 1;
}

//------------------------------------------------------------------------------
int vtkSpectrogramFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  // Time resolution should be at least 3 samples to produce an image
  constexpr static int MIN_RESOLUTION = 3;
  if (this->TimeResolution < MIN_RESOLUTION)
  {
    vtkWarningMacro(<< "Time resolution should not be smaller than 3 samples. "
                    << "Setting time resolution to 3 samples.");
    this->TimeResolution = MIN_RESOLUTION;
  }

  // Arbitrary extent because at this point we cannot rely on the input because it may be empty.
  const int outExt[6] = { 0, VTK_INT_MAX, 0, VTK_INT_MAX, 0, 0 };
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outExt, 6);

  return 1;
}

//------------------------------------------------------------------------------
int vtkSpectrogramFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Retrieve input and output
  vtkTable* input = vtkTable::GetData(inputVector[0]);
  vtkImageData* output = vtkImageData::GetData(outputVector);

  if (!input || !output)
  {
    vtkErrorMacro("Missing input or output!");
    return 0;
  }

  if (input->GetNumberOfColumns() == 0)
  {
    return 1;
  }

  // Get array from which to produce a spectrogram
  vtkIdType nbInRows = input->GetNumberOfRows();
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);

  if (!inputArray)
  {
    vtkWarningMacro(<< "No input array specified. Using first column.");
    inputArray = vtkDataArray::SafeDownCast(input->GetColumn(0));
  }

  std::vector<vtkFFT::ScalarNumber> window(this->TimeResolution);
  switch (this->WindowType)
  {
    case vtkTableFFT::HANNING:
      vtkFFT::GenerateKernel1D(window.data(), window.size(), vtkFFT::HanningGenerator);
      break;
    case vtkTableFFT::BARTLETT:
      vtkFFT::GenerateKernel1D(window.data(), window.size(), vtkFFT::BartlettGenerator);
      break;
    case vtkTableFFT::SINE:
      vtkFFT::GenerateKernel1D(window.data(), window.size(), vtkFFT::SineGenerator);
      break;
    case vtkTableFFT::BLACKMAN:
      vtkFFT::GenerateKernel1D(window.data(), window.size(), vtkFFT::BlackmanGenerator);
      break;
    case vtkTableFFT::RECTANGULAR:
    default:
      vtkFFT::GenerateKernel1D(window.data(), window.size(), vtkFFT::RectangularGenerator);
  }
  vtkSmartPointer<vtkFFT::vtkScalarNumberArray> signal =
    vtkFFT::vtkScalarNumberArray::SafeDownCast(inputArray);
  if (!signal)
  {
    signal = vtkSmartPointer<vtkFFT::vtkScalarNumberArray>::New();
    signal->DeepCopy(inputArray);
  }
  const double sampleRate = this->ComputeSampleRate(input);
  const int noverlap = this->TimeResolution * (this->OverlapPercentage / 100.0);

  unsigned int shape[2];
  auto spectrogram = vtkFFT::Spectrogram(signal, window, sampleRate, noverlap, false, true,
    vtkFFT::Scaling::Density, vtkFFT::SpectralMode::PSD, shape, true);

  // Reshape output image (X is time, Y is frequency)
  const int dims[3] = { static_cast<int>(shape[1]), static_cast<int>(shape[0]), 1 };
  output->SetDimensions(dims);

  spectrogram->SetName(signal->GetName());
  output->GetPointData()->AddArray(spectrogram);

  // Create field data for time range
  vtkNew<vtkDoubleArray> timeRange;
  timeRange->SetName("Time Range");
  timeRange->SetNumberOfValues(2);
  const int nfft = static_cast<int>(this->TimeResolution * 0.5) + 1;
  timeRange->SetValue(0, (nfft - 1) / sampleRate);
  timeRange->SetValue(1, (nbInRows - nfft) / sampleRate);
  output->GetFieldData()->AddArray(timeRange);

  // Create field data for frequency range
  vtkNew<vtkDoubleArray> frequencyRange;
  frequencyRange->SetName("Frequency Range");
  frequencyRange->SetNumberOfValues(2);
  frequencyRange->SetValue(0, 0.0);
  frequencyRange->SetValue(1, ((nfft - 1) * sampleRate) / this->TimeResolution);
  output->GetFieldData()->AddArray(frequencyRange);

  return 1;
}

//-----------------------------------------------------------------------------
double vtkSpectrogramFilter::ComputeSampleRate(vtkTable* input)
{
  vtkDataArray* timeArray = nullptr;
  for (vtkIdType col = 0; col < input->GetNumberOfColumns(); col++)
  {
    vtkAbstractArray* column = input->GetColumn(col);
    const char* name = column->GetName();
    if (vtksys::SystemTools::Strucmp(name, "time") == 0)
    {
      timeArray = vtkDataArray::SafeDownCast(column);
      break;
    }
  }

  if (timeArray)
  {
    return 1.0 / (timeArray->GetTuple1(1) - timeArray->GetTuple1(0));
  }
  else
  {
    return this->DefaultSampleRate;
  }
}

//-----------------------------------------------------------------------------
void vtkSpectrogramFilter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  switch (this->WindowType)
  {
    case vtkTableFFT::HANNING:
      os << indent << "WindowType: Hanning" << std::endl;
      break;
    case vtkTableFFT::BARTLETT:
      os << indent << "WindowType: Bartlett" << std::endl;
      break;
    case vtkTableFFT::SINE:
      os << indent << "WindowType: Sine" << std::endl;
      break;
    case vtkTableFFT::BLACKMAN:
      os << indent << "WindowType: Blackman" << std::endl;
      break;
    case vtkTableFFT::RECTANGULAR:
      os << indent << "WindowType: Rectangular" << std::endl;
      break;
    default:
      os << indent << "WindowType: Unknown" << std::endl;
      break;
  }

  os << indent << "Time Resolution:" << this->TimeResolution << std::endl;
  os << indent << "Overlap Percentage:" << this->OverlapPercentage << std::endl;
  os << indent << "Default Sample Rate:" << this->DefaultSampleRate << std::endl;
}
