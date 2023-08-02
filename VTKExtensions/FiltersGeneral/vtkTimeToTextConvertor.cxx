// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTimeToTextConvertor.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVStringFormatter.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkTimeToTextConvertor);
//----------------------------------------------------------------------------
vtkTimeToTextConvertor::vtkTimeToTextConvertor()
{
  this->Format = nullptr;
  this->Shift = 0.0;
  this->Scale = 1.0;
  this->SetFormat("Time: {time:f}");
}

//----------------------------------------------------------------------------
vtkTimeToTextConvertor::~vtkTimeToTextConvertor()
{
  this->SetFormat(nullptr);
}

//----------------------------------------------------------------------------
int vtkTimeToTextConvertor::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkTimeToTextConvertor::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }
  double timeRange[2];
  timeRange[0] = VTK_DOUBLE_MIN;
  timeRange[1] = VTK_DOUBLE_MAX;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  return 1;
}
//----------------------------------------------------------------------------
inline double vtkTimeToTextConvertor_ForwardConvert(double T0, double shift, double scale)
{
  return T0 * scale + shift;
}
//----------------------------------------------------------------------------
int vtkTimeToTextConvertor::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkTable* output = vtkTable::GetData(outputVector);

  std::string result = "?";

  vtkInformation* inputInfo = input ? input->GetInformation() : nullptr;
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);

  bool timeArgumentPushed = false;
  if (inputInfo && inputInfo->Has(vtkDataObject::DATA_TIME_STEP()) && this->Format)
  {
    timeArgumentPushed = true;
    double time = inputInfo->Get(vtkDataObject::DATA_TIME_STEP());
    time = vtkTimeToTextConvertor_ForwardConvert(time, this->Shift, this->Scale);
    vtkPVStringFormatter::PushScope("TEXT", fmt::arg("time", time));
  }
  else if (outputInfo && outputInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) &&
    this->Format)
  {
    timeArgumentPushed = true;
    double time = outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    time = vtkTimeToTextConvertor_ForwardConvert(time, this->Shift, this->Scale);
    vtkPVStringFormatter::PushScope("TEXT", fmt::arg("time", time));
  }

  if (timeArgumentPushed)
  {
    // check for old format
    std::string formattedTitle = this->Format;
    std::string possibleOldFormatString = formattedTitle;
    vtksys::SystemTools::ReplaceString(formattedTitle, "%f", "{time:f}");
    if (possibleOldFormatString != formattedTitle)
    {
      vtkLogF(WARNING, "Legacy formatting pattern detected. Please replace '%s' with '%s'.",
        possibleOldFormatString.c_str(), formattedTitle.c_str());
    }

    result = vtkPVStringFormatter::Format(formattedTitle);

    vtkPVStringFormatter::PopScope();
  }

  vtkStringArray* data = vtkStringArray::New();
  data->SetName("Text");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(result.c_str());
  output->AddColumn(data);
  data->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkTimeToTextConvertor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Format: " << (this->Format ? this->Format : "(none)") << endl;
}
