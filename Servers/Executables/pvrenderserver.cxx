/*=========================================================================

Program:   ParaView
Module:    pvrenderserver.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkPVMain.h" // For VTK_USE_MPI
#include "vtkPVServerOptions.h"
#include "vtkToolkits.h" // For VTK_USE_MPI

// forward declare the initialize function
static void ParaViewInitializeInterpreter(vtkProcessModule* pm);

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  vtkPVMain::Initialize(&argc, &argv);
  // First create the correct options for this process
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  // set the type of process
  options->SetProcessType(vtkPVOptions::PVRENDER_SERVER);
  // Create a pvmain
  vtkPVMain* pvmain = vtkPVMain::New();
  // run the paraview main
  int ret = pvmain->Initialize(options, 0, ParaViewInitializeInterpreter, 
    argc, argv);
  if (!ret)
    {
    ret = pvmain->Run(options);
    }
  // clean up and return
  pvmain->Delete();
  options->Delete();
  vtkPVMain::Finalize();
  return ret;
}

//----------------------------------------------------------------------------
void ParaViewInitializeInterpreter(vtkProcessModule* pm)
{
  // Initialize built-in wrapper modules.
  vtkInitializationHelper::InitializeInterpretor(pm);

}
