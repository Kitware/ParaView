/*=========================================================================

  Program:   ParaView
  Module:    vtkCPTestDriver.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPTestDriver.h"

#include "vtkCPBaseGridBuilder.h"
#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkCPTestDriver);
vtkCxxSetObjectMacro(vtkCPTestDriver, GridBuilder, vtkCPBaseGridBuilder);

//----------------------------------------------------------------------------
vtkCPTestDriver::vtkCPTestDriver()
{
  // put in reasonable values for the time stepping
  this->NumberOfTimeSteps = 10;
  this->GridBuilder = 0;
  this->StartTime = 0;
  this->EndTime = 1;
}

//----------------------------------------------------------------------------
vtkCPTestDriver::~vtkCPTestDriver()
{
  this->SetGridBuilder(0);
}
//----------------------------------------------------------------------------
int vtkCPTestDriver::Run()
{
  if (this->GridBuilder == 0)
  {
    vtkErrorMacro("Need to set the grid builder.");
    return 1;
  }

  // create the coprocessor
  vtkSmartPointer<vtkCPProcessor> processor = vtkSmartPointer<vtkCPProcessor>::New();
  // here we should add in a pipeline for the processor to use

  for (unsigned long i = 0; i < this->NumberOfTimeSteps; i++)
  {
    vtkSmartPointer<vtkCPDataDescription> dataDescription =
      vtkSmartPointer<vtkCPDataDescription>::New();
    dataDescription->SetTimeData(this->GetTime(i), i);
    dataDescription->AddInput("input");
    if (processor->RequestDataDescription(dataDescription))
    {
      int builtNewGrid = 0;
      dataDescription->GetInputDescriptionByName("input")->SetGrid(
        this->GridBuilder->GetGrid(i, this->GetTime(i), builtNewGrid));
      // now call the coprocessing library
      processor->CoProcess(dataDescription);
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
double vtkCPTestDriver::GetTime(unsigned long timeStep)
{
  if (this->EndTime == this->StartTime)
  {
    vtkWarningMacro("EndTime equals StartTime.");
  }
  double deltaTimeStep = (this->EndTime - this->StartTime) / (1. * this->NumberOfTimeSteps);
  return this->StartTime + deltaTimeStep * timeStep;
}

//----------------------------------------------------------------------------
vtkCPBaseGridBuilder* vtkCPTestDriver::GetGridBuilder()
{
  return this->GridBuilder;
}

//----------------------------------------------------------------------------
void vtkCPTestDriver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
  os << indent << "GridBuilder: " << this->GridBuilder << endl;
  os << indent << "StartTime: " << this->StartTime << endl;
  os << indent << "EndTime: " << this->EndTime << endl;
}
