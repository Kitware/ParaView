/*=========================================================================

  Program:   ParaView
  Module:    ServerManagerStateLoader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkPVMain.h"
#include "vtkTestingOptions.h"
#include "vtkTestingProcessModuleGUIHelper.h"
#include "vtkTestingProcessModuleGUIHelper.h"
#include "vtkToolkits.h" // For VTK_USE_MPI

static void ParaViewInitializeInterpreter(vtkProcessModule* pm)
{
  // Initialize built-in wrapper modules.
  vtkInitializationHelper::InitializeInterpretor(pm);
}

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  vtkPVMain::SetUseMPI(0); // don't use MPI at all.
  vtkPVMain::Initialize(&argc, &argv); 
  vtkPVMain* pvmain = vtkPVMain::New();
  vtkTestingOptions* options = vtkTestingOptions::New();
  options->SetProcessType(vtkPVOptions::PVBATCH);
  vtkTestingProcessModuleGUIHelper* helper = vtkTestingProcessModuleGUIHelper::New();
  int ret = pvmain->Initialize(options, helper, ParaViewInitializeInterpreter, argc, argv);
  if (!ret)
    {
    ret = helper->Run(options);
    }
  helper->Delete();
  pvmain->Delete();
  options->Delete();
  vtkPVMain::Finalize();
  return ret;
}

