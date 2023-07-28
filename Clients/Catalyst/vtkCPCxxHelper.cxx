// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCPCxxHelper.h"

#include "vtkInitializationHelper.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVPluginTracker.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConfiguration.h"
#include "vtkSMObject.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"

// for PARAVIEW_INSTALL_DIR and PARAVIEW_BINARY_DIR variables
#include "vtkCPConfig.h"

#include "ParaView_paraview_plugins.h"

#include <cassert>
#include <sstream>
#include <string>
#include <vtksys/SystemTools.hxx>

vtkWeakPointer<vtkCPCxxHelper> vtkCPCxxHelper::Instance;
bool vtkCPCxxHelper::ParaViewExternallyInitialized = false;

//----------------------------------------------------------------------------
vtkCPCxxHelper::vtkCPCxxHelper() = default;

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
    vtkCPCxxHelper::Instance->Register(nullptr);
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

  auto pm = vtkProcessModule::GetProcessModule();
  if (pm)
  {
    vtkCPCxxHelper::ParaViewExternallyInitialized = true;
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

    // Disable vtkSMSettings processing for Catalyst applications.
    vtkInitializationHelper::SetLoadSettingsFilesDuringInitialization(false);

    vtkProcessModuleConfiguration::GetInstance()->SetSymmetricMPIMode(true);
    vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_BATCH);

    delete[] argv[0];
    delete[] argv;

    // Create session when none exists.
    pm = vtkProcessModule::GetProcessModule();
    assert(pm != nullptr);
  }

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
