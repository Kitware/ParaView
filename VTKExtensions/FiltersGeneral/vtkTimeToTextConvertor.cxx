/*=========================================================================

  Program:   ParaView
  Module:    vtkTimeToTextConvertor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTimeToTextConvertor.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkTimeToTextConvertor);
//----------------------------------------------------------------------------
vtkTimeToTextConvertor::vtkTimeToTextConvertor()
{
  this->Format = 0;
  this->Shift = 0.0;
  this->Scale = 1.0;
  this->SetFormat("Time: %f");
}

//----------------------------------------------------------------------------
vtkTimeToTextConvertor::~vtkTimeToTextConvertor()
{
  this->SetFormat(0);
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

  char* buffer = new char[strlen(this->Format) + 1024];
  strcpy(buffer, "?");

  vtkInformation* inputInfo = input ? input->GetInformation() : 0;
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);

  if (inputInfo && inputInfo->Has(vtkDataObject::DATA_TIME_STEP()) && this->Format)
  {
    double time = inputInfo->Get(vtkDataObject::DATA_TIME_STEP());
    time = vtkTimeToTextConvertor_ForwardConvert(time, this->Shift, this->Scale);
    sprintf(buffer, this->Format, time);
  }
  else if (outputInfo && outputInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) &&
    this->Format)
  {
    double time = outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    time = vtkTimeToTextConvertor_ForwardConvert(time, this->Shift, this->Scale);
    sprintf(buffer, this->Format, time);
  }

  vtkStringArray* data = vtkStringArray::New();
  data->SetName("Text");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(buffer);
  output->AddColumn(data);
  data->Delete();

  delete[] buffer;
  return 1;
}

//----------------------------------------------------------------------------
void vtkTimeToTextConvertor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Format: " << (this->Format ? this->Format : "(none)") << endl;
}
