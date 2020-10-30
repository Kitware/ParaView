/*=========================================================================

  Program:   ParaView
  Module:    vtkCPCxxHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPCxxHelper.h"

#include "vtkInitializationHelper.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h" // Required to get build options for paraview
#include "vtkPVOptions.h"
#include "vtkPVPluginTracker.h"
#include "vtkProcessModule.h"
#include "vtkSMObject.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"

// for PARAVIEW_INSTALL_DIR and PARAVIEW_BINARY_DIR variables
#include "vtkCPConfig.h"

#include "ParaView_paraview_plugins.h"

#include <assert.h>
#include <sstream>
#include <string>
#include <vtksys/SystemTools.hxx>

vtkWeakPointer<vtkCPCxxHelper> vtkCPCxxHelper::Instance;
bool vtkCPCxxHelper::ParaViewExternallyInitialized = false;

//----------------------------------------------------------------------------
vtkCPCxxHelper::vtkCPCxxHelper()
  : Options(nullptr)
{
}

//----------------------------------------------------------------------------
vtkCPCxxHelper::~vtkCPCxxHelper()
{
  if (!vtkCPCxxHelper::ParaViewExternallyInitialized)
  {
    vtkInitializationHelper::Finalize();
  }
}

//----------------------------------------------------------------------------
vtkCPCxxHelper* vtkCPCxxHelper::New()
{
  if (vtkCPCxxHelper::Instance.GetPointer() != nullptr)
  {
    vtkCPCxxHelper::Instance->Register(NULL);
    return vtkCPCxxHelper::Instance;
  }

  // For in situ, when running in distributed mode, ensure that we don't
  // generate any output on the stderr on satellite ranks. One can of course
  // generate log files, if needed for satellite, but it's best to disable
  // any terminal output for errors/warnings etc.
  if (auto controller = vtkMultiProcessController::GetGlobalController())
  {
    if (controller->GetLocalProcessId() > 0)
    {
      vtkLogger::SetStderrVerbosity(vtkLogger::VERBOSITY_OFF);
    }
  }

  // Try the factory first
  vtkCPCxxHelper* instance = (vtkCPCxxHelper*)vtkObjectFactory::CreateInstance("vtkCPCxxHelper");
  // if the factory did not provide one, then create it here
  if (!instance)
  {
    instance = new vtkCPCxxHelper;
    // for compliance with vtkDebugLeaks, initialize the object base
    instance->InitializeObjectBase();
  }

  vtkCPCxxHelper::Instance = instance;

  if (auto pm = vtkProcessModule::GetProcessModule())
  {
    vtkCPCxxHelper::ParaViewExternallyInitialized = true;
    vtkCPCxxHelper::Instance->Options = pm->GetOptions();
  }
  else
  {
    vtkCPCxxHelper::ParaViewExternallyInitialized = false;

// Since when coprocessing, we have no information about the executable, we
// make one up using the current working directory.
// std::string self_dir = vtksys::SystemTools::GetCurrentWorkingDirectory(/*collapse=*/true);
#if defined(_WIN32) && defined(CMAKE_INTDIR)
    std::string programname = PARAVIEW_BINARY_DIR "/bin/" CMAKE_INTDIR "/unknown_exe";
#else
    std::string programname = PARAVIEW_BINARY_DIR "/bin/unknown_exe";
#endif

    int argc = 1;
    char** argv = new char*[2];
    argv[0] = vtksys::SystemTools::DuplicateString(programname.c_str());
    argv[1] = nullptr;

    vtkCPCxxHelper::Instance->Options.TakeReference(vtkPVOptions::New());
    vtkCPCxxHelper::Instance->Options->SetSymmetricMPIMode(1);

    // Disable vtkSMSettings processing for Catalyst applications.
    vtkInitializationHelper::SetLoadSettingsFilesDuringInitialization(false);

    vtkInitializationHelper::Initialize(
      argc, argv, vtkProcessModule::PROCESS_BATCH, vtkCPCxxHelper::Instance->Options);

    delete[] argv[0];
    delete[] argv;
  }

  // Create session, if none exists.
  auto pm = vtkProcessModule::GetProcessModule();
  assert(pm != nullptr);

  // register static plugins
  ParaView_paraview_plugins_initialize();

  vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXMLs("paraview");

  if (pm->GetSession() == nullptr)
  {
    // Setup default session.
    vtkIdType connectionId = vtkSMSession::ConnectToSelf();
    assert(connectionId != 0);

    // initialize the session for a ParaView session.
    vtkNew<vtkSMParaViewPipelineController> controller;
    controller->InitializeSession(vtkSMSession::SafeDownCast(pm->GetSession(connectionId)));
  }

  return vtkCPCxxHelper::Instance;
}

//----------------------------------------------------------------------------
void vtkCPCxxHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
