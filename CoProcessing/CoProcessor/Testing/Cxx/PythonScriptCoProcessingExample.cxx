/*=========================================================================

  Program:   ParaView
  Module:    PythonScriptCoProcessingExample.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Build the grid inside of vtkCustomTestDriver.  This also calls the 
// python coprocessor.

#include "vtkPVCustomTestDriver.h"

#include "CPSystemInformation.h"
#ifdef COPROCESSOR_USE_MPI
#define MPICH_SKIP_MPICXX
#include "mpi.h"
#endif
#include <iostream>

int main(int argc, char* argv[])
{
  if(argc < 2)
    {
    cerr << "Wrong number of arguments.  Command is: <exe> <python script>\n";
    return 1;
    }
#ifdef COPROCESSOR_USE_MPI
  cout << "Initializing MPI.\n";
  MPI_Init(&argc,&argv);
#endif
  int errors = 0;
  vtkPVCustomTestDriver* testDriver = vtkPVCustomTestDriver::New();
  if(testDriver->Initialize(argv[1]))
    {
    testDriver->SetNumberOfTimeSteps(1);
    testDriver->SetStartTime(0);
    testDriver->SetEndTime(.5);
    
    if(testDriver->Run())
      {
      errors++;
      }
    testDriver->Finalize();
    }
  else
    {
    errors++;
    }
  testDriver->Delete();

#ifdef COPROCESSOR_USE_MPI
  MPI_Finalize();
#endif

  cout << "Finished run with " << errors << " errors.\n";

  return errors;
}

