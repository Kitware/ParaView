/*=========================================================================

  Program:   ParaView
  Module:    TestCompleteArrays.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArrayCalculator.h"
#include "vtkCompleteArrays.h"
#include "vtkConeSource.h"
#include "vtkMultiProcessController.h"
#include "vtkPolyData.h"

// This will be called by all processes
void MyMain(vtkMultiProcessController* controller, void*)
{
  int myid, numProcs;

  // Obtain the id of the running process and the total
  // number of processes
  myid = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  vtkConeSource* cone = vtkConeSource::New();

  if (myid != 0)
  {
    // If I am not the root process
    // don't need to do anything
  }
  else
  {
    // If I am the root process

    vtkCompleteArrays* arrays = vtkCompleteArrays::New();
    arrays->SetController(controller);
    arrays->SetInput(cone->GetOutput());

    if (!arrays->GetOutput())
    {
      cerr << "Error: Output was not constructed" << endl;
    }

    arrays->Delete();
  }

  cone->Delete();
}

int main(int argc, char* argv[])
{
  vtkMultiProcessController* controller;

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  controller = vtkMultiProcessController::New();

  controller->Initialize(&argc, &argv);

  controller->SetSingleMethod(MyMain, NULL);

  // When using MPI, the number of processes is determined
  // by the external program which launches this application.
  // However, when using threads, we need to set it ourselves.
  if (controller->IsA("vtkThreadedController"))
  {
    // Set the number of processes to 2 for this example.
    controller->SetNumberOfProcesses(2);
  }
  controller->SingleMethodExecute();

  controller->Finalize();
  controller->Delete();

  return 0;
}
