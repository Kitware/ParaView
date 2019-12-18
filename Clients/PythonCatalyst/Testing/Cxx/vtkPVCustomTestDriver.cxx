/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCustomTestDriver.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCustomTestDriver.h"

#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPLinearScalarFieldFunction.h"
#include "vtkCPNodalFieldBuilder.h"
#include "vtkCPProcessor.h"
#include "vtkCPPythonScriptPipeline.h"
#include "vtkCPPythonStringPipeline.h"
#include "vtkCPUniformGridBuilder.h"
#include "vtkCommunicator.h"
#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"

vtkStandardNewMacro(vtkPVCustomTestDriver);

//----------------------------------------------------------------------------
vtkPVCustomTestDriver::vtkPVCustomTestDriver()
{
  this->Processor = vtkCPProcessor::New();
  this->Processor->Initialize();

  // Specify how the field varies over space and time.
  vtkCPLinearScalarFieldFunction* fieldFunction = vtkCPLinearScalarFieldFunction::New();
  fieldFunction->SetConstant(2.);
  fieldFunction->SetTimeMultiplier(100);
  fieldFunction->SetXMultiplier(23.);
  fieldFunction->SetYMultiplier(15.);
  fieldFunction->SetZMultiplier(8.);

  // Specify how to construct the field over the grid.
  vtkCPNodalFieldBuilder* fieldBuilder = vtkCPNodalFieldBuilder::New();
  fieldBuilder->SetArrayName("Pressure");
  fieldBuilder->SetTensorFieldFunction(fieldFunction);
  fieldFunction->Delete();

  // Set the type of grid we are building.
  vtkCPUniformGridBuilder* gridBuilder = vtkCPUniformGridBuilder::New();
  int dimensions[3] = { 50, 50, 50 };
  gridBuilder->SetDimensions(dimensions);
  double spacing[3] = { .2, .2, .3 };
  gridBuilder->SetSpacing(spacing);
  double origin[3] = { 0, 20, 300 };
  gridBuilder->SetOrigin(origin);
  gridBuilder->SetFieldBuilder(fieldBuilder);
  fieldBuilder->Delete();

  this->SetGridBuilder(gridBuilder);
  gridBuilder->Delete();
}

//----------------------------------------------------------------------------
vtkPVCustomTestDriver::~vtkPVCustomTestDriver()
{
  if (this->Processor)
  {
    this->Processor->Delete();
    this->Processor = 0;
  }
}

//----------------------------------------------------------------------------
int vtkPVCustomTestDriver::Run()
{
  vtkCPBaseGridBuilder* gridBuilder = this->GetGridBuilder();
  if (gridBuilder == 0)
  {
    vtkErrorMacro("Need to set the grid builder.");
    return 1;
  }

  for (unsigned long i = 0; i < this->GetNumberOfTimeSteps(); i++)
  {
    // now call the coprocessing library
    vtkCPDataDescription* dataDescription = vtkCPDataDescription::New();
    double time = this->GetTime(i);
    dataDescription->SetTimeData(time, i);
    dataDescription->AddInput("input");

    if (this->Processor->RequestDataDescription(dataDescription))
    {
      unsigned int numberOfFields =
        dataDescription->GetInputDescriptionByName("input")->GetNumberOfFields();
      if (!numberOfFields)
      {
        cout << "No fields for coprocessing.\n";
      }
      int builtNewGrid = 0;
      vtkDataObject* grid = gridBuilder->GetGrid(i, this->GetTime(i), builtNewGrid);
      dataDescription->GetInputDescriptionByName("input")->SetGrid(grid);
      // we need to get the whole extent of any structured grids
      int extent[6];
      if (vtkImageData* image = vtkImageData::SafeDownCast(grid))
      {
        image->GetExtent(extent);
      }
      else if (vtkRectilinearGrid* rgrid = vtkRectilinearGrid::SafeDownCast(grid))
      {
        rgrid->GetExtent(extent);
      }
      else if (vtkStructuredGrid* sgrid = vtkStructuredGrid::SafeDownCast(grid))
      {
        sgrid->GetExtent(extent);
      }
      for (int j = 0; j < 3; j++)
      {
        extent[2 * j] = -extent[2 * j];
      }
      int wholeExtent[6];
      vtkMultiProcessController::GetGlobalController()->AllReduce(
        extent, wholeExtent, 6, vtkCommunicator::MAX_OP);
      for (int j = 0; j < 3; j++)
      {
        wholeExtent[2 * j] = -wholeExtent[2 * j];
      }
      dataDescription->GetInputDescriptionByName("input")->SetWholeExtent(wholeExtent);
      this->Processor->CoProcess(dataDescription);
    }
    dataDescription->Delete();
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPVCustomTestDriver::Initialize(const char* fileName)
{
  vtkCPPythonScriptPipeline* pipeline = vtkCPPythonScriptPipeline::New();

  int success = pipeline->Initialize(fileName);
  this->Processor->AddPipeline(pipeline);
  pipeline->Delete();

  // test adding a second pipeline and then deleting it
  vtkCPPythonScriptPipeline* tempPipeline = vtkCPPythonScriptPipeline::New();
  this->Processor->AddPipeline(tempPipeline);
  tempPipeline->Delete();
  if (this->Processor->GetNumberOfPipelines() != 2)
  {
    vtkErrorMacro("Wrong amount of pipelines.");
    success = 0;
  }
  else if (this->Processor->GetPipeline(0) != pipeline ||
    this->Processor->GetPipeline(1) != tempPipeline)
  {
    vtkErrorMacro("Bad ordering of the processor's pipeline.");
    success = 0;
  }
  this->Processor->RemovePipeline(tempPipeline);
  if (this->Processor->GetNumberOfPipelines() != 1)
  {
    vtkErrorMacro("Wrong amount of pipelines.");
    success = 0;
  }
  else if (this->Processor->GetPipeline(0) != pipeline)
  {
    vtkErrorMacro("Bad ordering of the processor's pipeline.");
    success = 0;
  }

  // add in a string pipeline to verify that it works
  vtkNew<vtkCPPythonStringPipeline> stringPipeline;
  this->Processor->AddPipeline(stringPipeline);
  // clang-format off
  std::string operations = R"(
from __future__ import print_function

def RequestDataDescription(datadescription):
  if datadescription.GetForceOutput() == True or datadescription.GetTimeStep() % 5 == 0:
    for i in range(datadescription.GetNumberOfInputDescriptions()):
      datadescription.GetInputDescription(i).AllFieldsOn()
      datadescription.GetInputDescription(i).GenerateMeshOn()
  return

def DoCoProcessing(datadescription):
  print('in DoCoProcessing')
)";
    // clang-format on
    stringPipeline->Initialize(operations.c_str());

  return success;
}

//----------------------------------------------------------------------------
int vtkPVCustomTestDriver::Finalize()
{
  this->Processor->Finalize();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVCustomTestDriver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Processor: " << this->Processor << endl;
}
