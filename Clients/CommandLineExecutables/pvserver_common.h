/*=========================================================================

Program:   ParaView
Module:    pvserver.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVConfig.h"

#include "vtkCLIOptions.h"
#include "vtkInitializationHelper.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVSessionServer.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConfiguration.h"
#include "vtkRemotingCoreConfiguration.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"

#ifdef PARAVIEW_USE_PYTHON
extern "C"
{
  void vtkPVInitializePythonModules();
}
#endif

#include "ParaView_paraview_plugins.h"

static int RealMain(int argc, char* argv[], vtkProcessModule::ProcessTypes type)
{
  auto cliApp = vtk::TakeSmartPointer(vtkCLIOptions::New());
  cliApp->SetAllowExtras(false);
  cliApp->SetStopOnUnrecognizedArgument(false);
  switch (type)
  {
    case vtkProcessModule::PROCESS_DATA_SERVER:
      cliApp->SetDescription(
        "pvdataserver: the ParaView data-server\n"
        "=============================\n"
        "This is the ParaView data-server executable. Together with the render-server "
        "(pvrenderserver), "
        "this can be used for client-server use-cases. "
        "This process handles all the rendering requests. \n\n"
        "Typically, one connects a ParaView client (either a graphical client, or a Python-based "
        "client) to this process to drive the data analysis and visualization pipelines.");
      break;

    case vtkProcessModule::PROCESS_RENDER_SERVER:
      cliApp->SetDescription(
        "pvrenderserver: the ParaView render-server\n"
        "=============================\n"
        "This is the ParaView render-server executable. Together with the data-server "
        "(pvdataserver), "
        "this can be used for client-server use-cases. "
        "This process handles all the data-processing requests. \n\n"
        "Typically, one connects a ParaView client (either a graphical client, or a Python-based "
        "client) to this process to drive the data analysis and visualization pipelines.");
      break;

    case vtkProcessModule::PROCESS_SERVER:
      cliApp->SetDescription(
        "pvserver: the ParaView server\n"
        "=============================\n"
        "This is the ParaView server executable. This is intended for client-server use-cases "
        "which require the client and server to be on different processes, potentially on "
        "different systems.\n\n"
        "Typically, one connects a ParaView client (either a graphical client, or a Python-based "
        "client) to this process to drive the data analysis and visualization pipelines.");
      break;
    default:
      vtkLogF(ERROR, "process type not supported!");
      abort();
  }

  // Init current process type
  auto status = vtkInitializationHelper::Initialize(argc, argv, type, cliApp);
  cliApp = nullptr;
  if (!status)
  {
    return vtkInitializationHelper::GetExitCode();
  }

  auto config = vtkRemotingCoreConfiguration::GetInstance();

#ifdef PARAVIEW_USE_PYTHON
  // register callback to initialize modules statically. The callback is
  // empty when BUILD_SHARED_LIBS is ON.
  vtkPVInitializePythonModules();
#endif

  // register static plugins
  ParaView_paraview_plugins_initialize();

  vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXMLs("paraview");

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkMultiProcessController* controller = pm->GetGlobalController();

  vtkPVSessionServer* session = vtkPVSessionServer::New();
  session->SetMultipleConnection(config->GetMultiClientMode());
  session->SetDisableFurtherConnections(config->GetDisableFurtherConnections());

  int process_id = controller->GetLocalProcessId();
  if (process_id == 0)
  {
    // Report status:
    if (config->GetReverseConnection())
    {
      cout << "Connecting to client (reverse connection requested)..." << endl;
    }
    else
    {
      cout << "Waiting for client..." << endl;
    }
  }
  bool success = false;
  if (session->Connect())
  {
    success = true;
    pm->RegisterSession(session);
    if (controller->GetLocalProcessId() == 0)
    {
      while (pm->GetNetworkAccessManager()->ProcessEvents(0) != -1)
      {
      }
    }
    else
    {
      controller->ProcessRMIs();
    }
    pm->UnRegisterSession(session);
  }

  cout << "Exiting..." << endl;
  session->Delete();
  // Exit application
  vtkInitializationHelper::Finalize();
  return success ? vtkInitializationHelper::GetExitCode() : EXIT_FAILURE;
}
