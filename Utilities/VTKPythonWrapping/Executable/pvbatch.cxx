/*=========================================================================

Program:   ParaView
Module:    pvbatch.cxx

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
#include "vtkPVMain.h"
#include "vtkPVProcessModulePythonHelper.h"
#include "vtkPVPythonOptions.h"
#include "vtkToolkits.h" // For VTK_USE_MPI

static void ParaViewInitializeInterpreter(vtkProcessModule* pm)
{
  // Initialize built-in wrapper modules.
  vtkInitializationHelper::InitializeInterpretor(pm);
}

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  vtkPVMain::SetUseMPI(1); // use MPI
  vtkPVMain::Initialize(&argc, &argv); 
  vtkPVMain* pvmain = vtkPVMain::New();
  vtkPVPythonOptions* options = vtkPVPythonOptions::New();
  options->SetProcessType(vtkPVOptions::PVBATCH);
  vtkPVProcessModulePythonHelper* helper = vtkPVProcessModulePythonHelper::New();
  helper->SetDisableConsole(true);
  int ret = pvmain->Initialize(
    options, helper, ParaViewInitializeInterpreter, argc, argv);
  if (!ret)
    {
    // Tell process module that we support Multiple connections.
    // This must be set before starting the event loop.
    vtkProcessModule::GetProcessModule()->SupportMultipleConnectionsOff();
    ret = helper->Run(options);
    }
  helper->Delete();
  pvmain->Delete();
  options->Delete();
  vtkPVMain::Finalize();
  return ret;
}

