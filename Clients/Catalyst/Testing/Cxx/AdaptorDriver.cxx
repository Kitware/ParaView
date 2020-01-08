/*=========================================================================

  Program:   ParaView
  Module:    AdaptorDriver.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Give an example of creating an unstructured grid with point data
// and cell data to demonstrate how other people can pass their
// simulation data to the coprocessors.

#include "vtkCPTestDriver.h"
#include "vtkCustomUnstructuredGridBuilder.h"

int AdaptorDriver(int, char* [])
{
  // Set the type of grid we are building.
  vtkCustomUnstructuredGridBuilder* gridBuilder = vtkCustomUnstructuredGridBuilder::New();

  vtkCPTestDriver* testDriver = vtkCPTestDriver::New();
  testDriver->SetNumberOfTimeSteps(100);
  testDriver->SetStartTime(0);
  testDriver->SetEndTime(.5);
  testDriver->SetGridBuilder(gridBuilder);
  gridBuilder->Delete();

  testDriver->Run();

  testDriver->Delete();

  return 0;
}
