/*=========================================================================

  Program:   ParaView
  Module:    SimpleDriver.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Create a simple driver for the co-processing library by
// setting the desired classes inside of other classes.

#include "vtkCPLinearScalarFieldFunction.h"
#include "vtkCPNodalFieldBuilder.h"
#include "vtkCPTestDriver.h"
#include "vtkCPUniformGridBuilder.h"

int SimpleDriver(int, char* [])
{
  // Specify how the field varies over space and time.
  vtkCPLinearScalarFieldFunction* fieldFunction = vtkCPLinearScalarFieldFunction::New();
  fieldFunction->SetConstant(2.);
  fieldFunction->SetTimeMultiplier(.1);
  fieldFunction->SetYMultiplier(23.);

  // Specify how to construct the field over the grid.
  vtkCPNodalFieldBuilder* fieldBuilder = vtkCPNodalFieldBuilder::New();
  fieldBuilder->SetArrayName("Velocity");
  fieldBuilder->SetTensorFieldFunction(fieldFunction);
  fieldFunction->Delete();

  // Set the type of grid we are building.
  vtkCPUniformGridBuilder* gridBuilder = vtkCPUniformGridBuilder::New();
  int dimensions[3] = { 50, 50, 50 };
  gridBuilder->SetDimensions(dimensions);
  double spacing[3] = { .2, .2, .3 };
  gridBuilder->SetSpacing(spacing);
  double origin[3] = { 10, 20, 300 };
  gridBuilder->SetOrigin(origin);
  gridBuilder->SetFieldBuilder(fieldBuilder);
  fieldBuilder->Delete();

  vtkCPTestDriver* testDriver = vtkCPTestDriver::New();
  testDriver->SetNumberOfTimeSteps(100);
  testDriver->SetStartTime(.5);
  testDriver->SetEndTime(3.5);
  testDriver->SetGridBuilder(gridBuilder);
  gridBuilder->Delete();

  testDriver->Run();

  testDriver->Delete();

  return 0;
}
