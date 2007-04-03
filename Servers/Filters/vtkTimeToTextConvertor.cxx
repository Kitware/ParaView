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
vtkCxxRevisionMacro(vtkTimeToTextConvertor, "1.1");
//----------------------------------------------------------------------------
vtkTimeToTextConvertor::vtkTimeToTextConvertor()
{
  this->Format = 0;
  this->SetFormat("Time: %f");
}

//----------------------------------------------------------------------------
vtkTimeToTextConvertor::~vtkTimeToTextConvertor()
{
  this->SetFormat(0);
}

//----------------------------------------------------------------------------
int vtkTimeToTextConvertor::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkTimeToTextConvertor::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkTable* output = vtkTable::GetData(outputVector);

  bool has_time=false;
  char *buffer = new char[strlen(this->Format)+1024];
  strcpy(buffer, this->Format);
  double time;

  vtkInformation* inputInfo= input->GetInformation();
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  if (inputInfo && inputInfo->Has(vtkDataObject::DATA_TIME_STEPS()))
    {
    time = inputInfo->Get(vtkDataObject::DATA_TIME_STEPS())[0];
    has_time = true;
    }
  else if(outputInfo && outputInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    time = outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0];
    has_time = true;
    }

  if (has_time)
    {
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
  os << indent << "Format: " 
    << (this->Format? this->Format : "(none)") << endl;
}


