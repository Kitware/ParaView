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
#include "vtkInitializationHelper.h"
#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkProcessModule2.h"
#include "vtkPVServerOptions.h"
#include "vtkSMSessionServer.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  bool success = true;
  vtkInitializationHelper::Initialize(argc, argv,
    vtkProcessModule2::PROCESS_SERVER, options);
  if (!success)
    {
    return -1;
    }

  int ret_value = 0;

  vtkProcessModule2* pm = vtkProcessModule2::GetProcessModule();
  vtkMultiProcessController* controller = pm->GetGlobalController();
  if (controller->GetLocalProcessId() > 0)
    {
    // satellite.
    vtkSMSession* session = vtkSMSession::New();
    controller->ProcessRMIs();
    session->Delete();
    }
  else
    {
    vtkNetworkAccessManager* nam = pm->GetNetworkAccessManager();
    vtkSMSessionServer* session = vtkSMSessionServer::New();
    cout << "Waiting for client" << endl;
    cout << session->Connect("cs://localhost:11111") << endl;
    // TODO: detect when a error happens in processing events.
    while (nam->ProcessEvents(0) != -1)
      {
      // more
      }
    cout << "Exiting..." << endl;
    session->Delete();
    }
  return ret_value;
}
