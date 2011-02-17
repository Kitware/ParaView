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
#include "vtkSMSession.h"
#include "vtkTestingOptions.h"
#include "vtkTestingProcessModuleGUIHelper.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  vtkTestingOptions* options = vtkTestingOptions::New();

  vtkInitializationHelper::Initialize(argc, argv,
    vtkProcessModule::PROCESS_BATCH, options);

  vtkSMSession* session = vtkSMSession::New();
  vtkProcessModule::GetProcessModule()->RegisterSession(session);
  session->Delete();

  vtkTestingProcessModuleGUIHelper* helper = vtkTestingProcessModuleGUIHelper::New();
  int ret = helper->Run();

  helper->Delete();
  vtkInitializationHelper::Finalize();
  options->Delete();
  return ret;
}

