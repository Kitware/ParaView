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
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPythonInterpreter.h"
#include "vtkStdString.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

#include <assert.h>
#include <map>
#include <sstream>
#include <vector>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkPythonAnnotationFilter);
//----------------------------------------------------------------------------
vtkPythonAnnotationFilter::vtkPythonAnnotationFilter()
  : Expression(NULL)
  , ComputedAnnotationValue(NULL)
  , ArrayAssociation(vtkDataObject::FIELD)
  , DataTimeValid(false)
  , DataTime(0.0)
  , NumberOfTimeSteps(0)
  , TimeSteps(NULL)
  , TimeRangeValid(false)
  , CurrentInputDataObject(NULL)
{
  this->SetNumberOfInputPorts(1);
  this->TimeRange[0] = this->TimeRange[1] = 0.0;
}

//----------------------------------------------------------------------------
vtkPythonAnnotationFilter::~vtkPythonAnnotationFilter()
{
  this->SetExpression(0);
  this->SetComputedAnnotationValue(0);
}

//----------------------------------------------------------------------------
void vtkPythonAnnotationFilter::SetComputedAnnotationValue(const char* value)
{
  delete[] this->ComputedAnnotationValue;
  // SystemTools handles NULL strings.
  this->ComputedAnnotationValue = vtksys::SystemTools::DuplicateString(value);
  // don't call this->Modified. This method gets called in RequestData().
}

//----------------------------------------------------------------------------
int vtkPythonAnnotationFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  assert(input != NULL);

  // initialize variables.
  this->DataTimeValid = false;
  this->DataTime = 0.0;
  this->TimeSteps = NULL;
  this->NumberOfTimeSteps = 0;
  this->TimeRangeValid = false;
  this->TimeRange[0] = this->TimeRange[1] = 0.0;
  this->SetComputedAnnotationValue(NULL);
  this->CurrentInputDataObject = input;

  // Extract time information
  std::ostringstream timeInfo;
  if (vtkInformation* dataInformation = input->GetInformation())
  {
    if (dataInformation->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      this->DataTimeValid = true;
      this->DataTime = dataInformation->Get(vtkDataObject::DATA_TIME_STEP());
    }
  }

  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  if (inputInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inputInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->TimeSteps = inputInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }

  if (inputInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    this->TimeRangeValid = true;
    inputInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), this->TimeRange);
  }

  this->EvaluateExpression();

  // Make sure a valid ComputedAnnotationValue is available
  if (this->ComputedAnnotationValue == NULL)
  {
    this->SetComputedAnnotationValue("(error)");
  }

  // Update the output data
  vtkStringArray* data = vtkStringArray::New();
  data->SetName("Text");
  data->SetNumberOfComponents(1);
  data->InsertNextValue(this->ComputedAnnotationValue);

  vtkTable* output = vtkTable::GetData(outputVector);
  output->AddColumn(data);
  data->FastDelete();
  this->CurrentInputDataObject = NULL;

  if (vtkMultiProcessController::GetGlobalController() &&
    vtkMultiProcessController::GetGlobalController()->GetLocalProcessId() > 0)
  {
    // reset output on all ranks except the 0 root node.
    output->Initialize();
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonAnnotationFilter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
static std::string vtkGetReferenceAsString(void* ref)
{
  // Set self to point to this
  char addrofthis[1024];
  sprintf(addrofthis, "%p", ref);
  char* aplus = addrofthis;
  if ((addrofthis[0] == '0') && ((addrofthis[1] == 'x') || addrofthis[1] == 'X'))
  {
    aplus += 2; // skip over "0x"
  }
  return std::string(aplus);
}

//----------------------------------------------------------------------------
void vtkPythonAnnotationFilter::EvaluateExpression()
{
  std::ostringstream stream;
  stream << "def vtkPythonAnnotationFilter_EvaluateExpression():" << endl
         << "    from paraview import annotation as pv_ann" << endl
         << "    from paraview.vtk.vtkPVClientServerCoreCore import vtkPythonAnnotationFilter"
         << endl
         << "    me = vtkPythonAnnotationFilter('" << vtkGetReferenceAsString(this) << " ')" << endl
         << "    pv_ann.execute(me)" << endl
         << "    del me" << endl
         << "vtkPythonAnnotationFilter_EvaluateExpression()" << endl
         << "del vtkPythonAnnotationFilter_EvaluateExpression" << endl;

  // ensure Python is initialized.
  vtkPythonInterpreter::Initialize();
  vtkPythonInterpreter::RunSimpleString(stream.str().c_str());
}

//----------------------------------------------------------------------------
void vtkPythonAnnotationFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
