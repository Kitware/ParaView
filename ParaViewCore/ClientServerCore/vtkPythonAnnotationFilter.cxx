/*=========================================================================

  Program:   ParaView
  Module:    vtkPythonAnnotationFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPythonAnnotationFilter.h"

#include "vtkDataObjectTypes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkPythonProgrammableFilter.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <assert.h>
#include <map>
#include <vector>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkPythonAnnotationFilter);
//----------------------------------------------------------------------------
vtkPythonAnnotationFilter::vtkPythonAnnotationFilter()
{
  this->SetNumberOfInputPorts(1);
  this->AnnotationValue = 0;
  this->PythonExpression = 0;
  this->TimeInformations = 0;
}

//----------------------------------------------------------------------------
vtkPythonAnnotationFilter::~vtkPythonAnnotationFilter()
{
  this->SetTimeInformations(0);
  this->SetPythonExpression(0);
  this->SetAnnotationValue(0);
}

//----------------------------------------------------------------------------
int vtkPythonAnnotationFilter::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
{
  // Meta-data
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkInformation* inputInfo = input ? input->GetInformation() : 0;
  vtkInformation* outInfo = this->GetExecutive()->GetOutputInformation(0);


  // Extract time informations
  vtksys_ios::ostringstream timeInfo;
  if(inputInfo)
    {
    timeInfo << ",";
    // Time
    if(inputInfo->Has(vtkDataObject::DATA_TIME_STEP()))
      {
      double time = inputInfo->Get(vtkDataObject::DATA_TIME_STEP());
      timeInfo << time << ", ";
      }
    else
      {
      timeInfo << "0" << ", ";
      }

    // TimeSteps
    if(outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
      {
      const double* timeSteps =
          outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      int len = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      timeInfo << "[";
      for(int i=0; i < len; i++)
        {
        timeInfo << timeSteps[i] << ", ";
        }
      timeInfo << "], ";
      }
    else
      {
      timeInfo << "[0, 1], ";
      }

    // TimeRange
    if(outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
      {
      double range[2];
      outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), range);
      timeInfo << "[" << range[0] << ", " << range[1] << "]";
      }
    else
      {
      timeInfo << "[0, 1]";
      }
    }
  this->SetTimeInformations(timeInfo.str().c_str());


  // Execute the python script to process and generate the annotation
  this->EvaluateExpression();

  // Make sure a valid AnnotationValue is available
  if(this->AnnotationValue == NULL || strlen(this->AnnotationValue) == 0)
    {
    this->SetAnnotationValue("Write a python expression like: 'Time index: %i' % t_index");
    }

  // Update the output data
  vtkStringArray* data = vtkStringArray::New();
  data->SetName("Text");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(this->AnnotationValue);

  vtkTable* output = vtkTable::GetData(outputVector);
  output->AddColumn(data);
  data->FastDelete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonAnnotationFilter::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void vtkPythonAnnotationFilter::EvaluateExpression()
{
  // Set self to point to this
  char addrofthis[1024];
  sprintf(addrofthis, "%p", this);
  char *aplus = addrofthis;
  if ((addrofthis[0] == '0') &&
      ((addrofthis[1] == 'x') || addrofthis[1] == 'X'))
    {
    aplus += 2; //skip over "0x"
    }

  vtksys_ios::ostringstream stream;
  stream << "import paraview" << endl
         << "paraview.fromFilter = True" << endl
         << "from paraview import annotation as pv_ann" << endl
         << "me = paraview.servermanager.vtkPythonAnnotationFilter('" << aplus << " ')" << endl
         << "pv_ann.ComputeAnnotation(me, me.GetInputDataObject(0, 0), me.GetPythonExpression()"
         << this->TimeInformations << ")" << endl
         << "del me" << endl;

  vtkPythonProgrammableFilter::GetGlobalPipelineInterpretor()->RunSimpleString(
    stream.str().c_str());
  vtkPythonProgrammableFilter::GetGlobalPipelineInterpretor()->FlushMessages();
}

//----------------------------------------------------------------------------
void vtkPythonAnnotationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
