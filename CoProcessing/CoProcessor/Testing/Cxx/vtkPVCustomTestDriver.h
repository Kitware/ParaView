/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCustomTestDriver.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCustomTestDriver - A custom test driver code that uses ParaView and python.
// .SECTION Description
// A custom test driver that creates a vtkUniformGrid with a single
// scalar point field named "Pressure".  It runs a python script
// using ParaView.

#ifndef __vtkPVCustomTestDriver_h
#define __vtkPVCustomTestDriver_h

#include "vtkCPTestDriver.h"

class vtkCPProcessor;

class VTK_EXPORT vtkPVCustomTestDriver : public vtkCPTestDriver
{
public:
  static vtkPVCustomTestDriver * New();
  vtkTypeMacro(vtkPVCustomTestDriver, vtkCPTestDriver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Run the test driver with the coprocessor.  
  // Returns 0 if there were no errors.
  virtual int Run();

  // Description:
  // Initialize the driver with the coprocessor.  fileName is the
  // name of the python script.  Returns 0 on failure.
  virtual int Initialize(const char* fileName);

  // Description:
  // Finalize the driver with the coprocessor.
  virtual int Finalize();

protected:
  vtkPVCustomTestDriver();
  ~vtkPVCustomTestDriver();

private:
  vtkPVCustomTestDriver(const vtkPVCustomTestDriver&); // Not implemented
  void operator=(const vtkPVCustomTestDriver&); // Not implemented

  // Description:
  // The coprocessor to be called by this custom test.
  vtkCPProcessor* Processor;
};

#endif
