/*=========================================================================

  Program:   ParaView
  Module:    vtkCPTestDriver.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPTestDriver - Class for creating a co-processor test driver.
// .SECTION Description
// Class for creating a co-processor test driver.  It is intended
// as a framework for creating custom inputs replicating a simulation for 
// the co-processing library.

#ifndef __vtkCPTestDriver_h
#define __vtkCPTestDriver_h

#include "vtkObject.h"

class vtkCPBaseGridBuilder;

class VTK_EXPORT vtkCPTestDriver : public vtkObject
{
public:
  static vtkCPTestDriver * New();
  vtkTypeMacro(vtkCPTestDriver, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Run the test driver.  Returns 0 if there were no errors.
  virtual int Run();

  // Description:
  // Set/get NumberOfTimeSteps.
  vtkSetMacro(NumberOfTimeSteps, unsigned long);
  vtkGetMacro(NumberOfTimeSteps, unsigned long);

  // Description:
  // Given a TimeStep, return the simulation time corresponding to 
  // that time step.  This implementation has constant time
  // steps between StartTime and EndTime.
  virtual double GetTime(unsigned long timeStep);

  // Description:
  // Set/get GridBuilder.  
  void SetGridBuilder(vtkCPBaseGridBuilder* gridBuilder);
  vtkCPBaseGridBuilder* GetGridBuilder();

  // Description:
  // Set/get the start and end times of the simulation.
  vtkSetMacro(StartTime, double);
  vtkGetMacro(StartTime, double);
  vtkSetMacro(EndTime, double);
  vtkGetMacro(EndTime, double);

protected:
  vtkCPTestDriver();
  ~vtkCPTestDriver();

private:
  vtkCPTestDriver(const vtkCPTestDriver&); // Not implemented
  void operator=(const vtkCPTestDriver&); // Not implemented

  // Description:
  // The grid builder for creating the input grids to the coprocessing library.
  vtkCPBaseGridBuilder* GridBuilder;

  // Description:
  // The total number of time steps the test driver will compute.
  // The time steps are numbered 0 through NumberOfTimeSteps-1.
  unsigned long NumberOfTimeSteps;

  // Description:
  // The start and end times of the simulation.
  double StartTime;
  double EndTime;
};

#endif
