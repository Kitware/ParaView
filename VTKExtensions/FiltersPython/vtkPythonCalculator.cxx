/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonCalculator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPython.h" // has to be first!

#include "vtkPythonCalculator.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVStringFormatter.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <string>
#include <vtksys/SystemTools.hxx>

namespace
{
bool CheckAndFlushPythonErrors()
{
  if (PyErr_Occurred())
  {
    PyErr_Print();
    PyErr_Clear();
    return true;
  }
  return false;
}
}

vtkStandardNewMacro(vtkPythonCalculator);

//----------------------------------------------------------------------------
vtkPythonCalculator::vtkPythonCalculator()
{
  this->Expression = nullptr;
  this->ArrayName = nullptr;
  this->SetArrayName("result");
  this->SetExecuteMethod(vtkPythonCalculator::ExecuteScript, this);
  this->ArrayAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
}

//----------------------------------------------------------------------------
vtkPythonCalculator::~vtkPythonCalculator()
{
  this->SetExpression(nullptr);
  this->SetArrayName(nullptr);
}

//----------------------------------------------------------------------------
int vtkPythonCalculator::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  assert(input != nullptr);

  double dataTime = 0;
  bool dataTimeValid = false;
  std::vector<double> timeSteps;
  int timeIndex;

  // Extract time information
  if (vtkInformation* dataInformation = input->GetInformation())
  {
    if (dataInformation->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      dataTimeValid = true;
      dataTime = dataInformation->Get(vtkDataObject::DATA_TIME_STEP());
    }
  }

  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  if (inputInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    int numberOfTimeSteps = inputInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double* tempTimeSteps = inputInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    timeSteps.insert(timeSteps.begin(), tempTimeSteps, tempTimeSteps + numberOfTimeSteps);
  }

  timeIndex = 0;
  if (dataTimeValid && !timeSteps.empty())
  {
    for (int i = 0; i < timeSteps.size(); ++i)
    {
      if (timeSteps[i] == dataTime)
      {
        timeIndex = i;
      }
    }
  }

  // define calculator scope
  PV_STRING_FORMATTER_NAMED_SCOPE(
    "CALCULATOR", fmt::arg("timevalue", dataTime), fmt::arg("timeindex", timeIndex));

  char* cachedExpression = vtksys::SystemTools::DuplicateString(this->Expression);

  std::string formattableExpression = this->Expression ? this->Expression : std::string();
  delete[] this->Expression;
  this->Expression = vtksys::SystemTools::DuplicateString(
    vtkPVStringFormatter::Format(formattableExpression).c_str());

  // call superclass
  this->Superclass::RequestData(request, inputVector, outputVector);

  // restore cached expression
  delete[] this->Expression;
  this->Expression = vtksys::SystemTools::DuplicateString(cachedExpression);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonCalculator::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Output type is same as input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        vtkDataObject* newOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
        this->GetOutputPortInformation(0)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPythonCalculator::ExecuteScript(void* arg)
{
  vtkPythonCalculator* self = static_cast<vtkPythonCalculator*>(arg);
  if (self)
  {
    self->Exec(self->GetExpression());
  }
}

//----------------------------------------------------------------------------
void vtkPythonCalculator::Exec(const char* expression)
{
  // Do not execute if expression is nullptr or empty.
  if (!expression || expression[0] == '\0')
  {
    return;
  }

  // Replace tabs with two spaces
  std::string orgscript;
  size_t len = strlen(expression);
  for (size_t i = 0; i < len; i++)
  {
    if (expression[i] == '\t')
    {
      orgscript += "  ";
    }
    else
    {
      orgscript.push_back(expression[i]);
    }
  }

  // ensure Python is initialized (safe to call many times)
  vtkPythonInterpreter::Initialize();

  vtkPythonScopeGilEnsurer gilEnsurer;
  vtkSmartPyObject modCalculator(PyImport_ImportModule("paraview.detail.calculator"));
  CheckAndFlushPythonErrors();
  if (!modCalculator)
  {
    vtkErrorMacro("Failed to import `paraview.detail.calculator` module.");
    return;
  }

  vtkSmartPyObject self(vtkPythonUtil::GetObjectFromPointer(this));
  vtkSmartPyObject fname(PyString_FromString("execute"));
  vtkSmartPyObject pyexpression(PyString_FromString(orgscript.c_str()));

  // call `paraview.detail.calculator.execute(self)`
  vtkSmartPyObject retVal(PyObject_CallMethodObjArgs(
    modCalculator, fname.GetPointer(), self.GetPointer(), pyexpression.GetPointer(), nullptr));

  CheckAndFlushPythonErrors();

  // at some point we may want to check retval
  (void)retVal;
}

//----------------------------------------------------------------------------
int vtkPythonCalculator::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonCalculator::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPythonCalculator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
