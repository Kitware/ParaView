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

#ifdef PARAVIEW_ENABLE_PYTHON
extern "C" {
void vtkPVInitializePythonModules();
}
#endif

#include "vtkInitializationHelper.h"
#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkPVServerOptions.h"
#include "vtkPVSessionServer.h"
#include "vtkProcessModule.h"

#ifndef BUILD_SHARED_LIBS
#include "paraview_plugins_static.h"
#endif

static bool RealMain(int argc, char* argv[], vtkProcessModule::ProcessTypes type)
{
  // Marking this static avoids the false leak messages from vtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any non-static objects
  // created before MPI_Init().
  static vtkSmartPointer<vtkPVServerOptions> options = vtkSmartPointer<vtkPVServerOptions>::New();

  // Init current process type
  vtkInitializationHelper::Initialize(argc, argv, type, options);
  if (options->GetTellVersion() || options->GetHelpSelected() || options->GetPrintMonitors())
  {
    vtkInitializationHelper::Finalize();
    return 1;
  }

#ifdef PARAVIEW_ENABLE_PYTHON
  // register callback to initialize modules statically. The callback is
  // empty when BUILD_SHARED_LIBS is ON.
  vtkPVInitializePythonModules();
#endif

#ifndef BUILD_SHARED_LIBS
  // load static plugins
  paraview_plugins_static_init();
#endif

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkMultiProcessController* controller = pm->GetGlobalController();

  vtkPVSessionServer* session = vtkPVSessionServer::New();
  session->SetMultipleConnection(options->GetMultiClientMode() != 0);
  session->SetDisableFurtherConnections(options->GetDisableFurtherConnections() != 0);

  int process_id = controller->GetLocalProcessId();
  if (process_id == 0)
  {
    // Report status:
    if (options->GetReverseConnection())
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
  return success;
}
