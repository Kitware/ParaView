/*=========================================================================

  Program:   ParaView
  Module:    SimpleDriver2.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Create a simple driver for the co-processing library by
// deriving the desired classes.

#include "vtkCPLinearScalarFieldFunction.h"
#include "vtkCPNodalFieldBuilder.h"
#include "vtkCPTestDriver.h"
#include "vtkCPUniformGridBuilder.h"
#include "vtkObjectFactory.h"

class VTK_EXPORT vtkCPImplementedTestDriver : public vtkCPTestDriver
{
public:
  static vtkCPImplementedTestDriver* New();
  vtkTypeMacro(vtkCPImplementedTestDriver, vtkCPTestDriver);
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
  }

protected:
  vtkCPImplementedTestDriver()
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

    this->SetNumberOfTimeSteps(100);
    this->SetGridBuilder(gridBuilder);
    gridBuilder->Delete();
  }
  ~vtkCPImplementedTestDriver(){};

private:
  vtkCPImplementedTestDriver(const vtkCPImplementedTestDriver&) = delete;
  void operator=(const vtkCPImplementedTestDriver&) = delete;
};

vtkStandardNewMacro(vtkCPImplementedTestDriver);

int SimpleDriver2(int, char* [])
{
  vtkCPImplementedTestDriver* testDriver = vtkCPImplementedTestDriver::New();
  testDriver->SetNumberOfTimeSteps(100);
  testDriver->SetStartTime(.5);
  testDriver->SetEndTime(3.5);
  testDriver->Run();

  testDriver->Delete();

  return 0;
}
