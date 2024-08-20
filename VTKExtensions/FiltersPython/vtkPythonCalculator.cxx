// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPython.h" // has to be first!

#include "vtkPythonCalculator.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVStringFormatter.h"
#include "vtkPythonInterpreter.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPyObject.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <regex>
#include <string>

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
  this->SetArrayName("result");
  this->SetExecuteMethod(vtkPythonCalculator::ExecuteScript, this);
}

//----------------------------------------------------------------------------
vtkPythonCalculator::~vtkPythonCalculator()
{
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
    for (int i = 0; i < static_cast<int>(timeSteps.size()); ++i)
    {
      if (timeSteps[i] == dataTime)
      {
        timeIndex = i;
      }
    }
  }

  std::string formattableExpression =
    this->UseMultilineExpression ? this->MultilineExpression : this->Expression;

  // define calculator scope
  PV_STRING_FORMATTER_NAMED_SCOPE(
    "CALCULATOR", fmt::arg("timevalue", dataTime), fmt::arg("timeindex", timeIndex));

  if (this->UseMultilineExpression)
  {
    this->MultilineExpression = vtkPVStringFormatter::Format(formattableExpression);
  }
  else
  {
    this->Expression = vtkPVStringFormatter::Format(formattableExpression);
  }

  // call superclass
  this->Superclass::RequestData(request, inputVector, outputVector);

  // restore cached expression
  if (this->UseMultilineExpression)
  {
    this->MultilineExpression = formattableExpression;
  }
  else
  {
    this->Expression = formattableExpression;
  }

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
    self->Exec(
      self->UseMultilineExpression ? self->GetMultilineExpression() : self->GetExpression());
  }
}

//----------------------------------------------------------------------------
void vtkPythonCalculator::Exec(const std::string& expression)
{
  // Do not execute if expression is empty.
  if (expression.empty())
  {
    return;
  }

  // Replace tabs with two spaces
  std::string orgscript;
  for (size_t i = 0; i < expression.size(); i++)
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
  vtkSmartPyObject fname(PyUnicode_FromString("execute"));
  vtkSmartPyObject pyexpression(PyUnicode_FromString(orgscript.c_str()));

  // call `paraview.detail.calculator.execute(self)`
  // calculator.py references ArrayName, ArrayAssociation and ResultArrayType to create the output
  // array.
  vtkSmartPyObject retVal(
    PyObject_CallMethodObjArgs(modCalculator, fname.GetPointer(), self.GetPointer(),
      pyexpression.GetPointer(), (this->UseMultilineExpression ? Py_True : Py_False), nullptr));

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

  os << indent << "Expression: " << this->Expression << endl;
  os << indent << "MultilineExpression: " << this->MultilineExpression << endl;
  os << indent << "UseMultilineExpression: " << this->UseMultilineExpression << endl;
  os << indent << "ArrayName: " << this->ArrayName << endl;
}
