/*=========================================================================

Program:   ParaView
Module:    paraview.h

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Description:
// This header file provide the common processing loop of ParaView processes

#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkProcessModule.h"
#include "vtkSMSessionServer.h"

namespace ParaView {

  //---------------------------------------------------------------------------
  vtkSMSessionServer* CreateServerSession()
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkMultiProcessController* controller = pm->GetGlobalController();
    if (controller->GetLocalProcessId() > 0)
      {
      // satellites never wait for client connections. They simply have 1 session
      // instance and then they simply listen for the root node to issue requests
      // for actions.
      vtkSMSession* session = vtkSMSession::New();
      controller->ProcessRMIs();
      session->Delete();
      }
    else
      {
      return vtkSMSessionServer::New();
      }
    return NULL;
    }

  //---------------------------------------------------------------------------
  int RunAndConnect()
    {
    // Init ParaView session (This will lock in case of satelite processes)
    vtkSMSessionServer* session = ParaView::CreateServerSession();

    // If the Session is available it means that we are on the ROOT process
    if( session )
      {
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      vtkNetworkAccessManager* nam = pm->GetNetworkAccessManager();
      cout << "Waiting for client" << endl;
      if (session->Connect())
        {
        // TODO: detect when an error happens in processing events.
        while (nam->ProcessEvents(0) != -1)
          {
          // more
          }
        }
      cout << "Exiting..." << endl;
      session->Delete();
      }
    return EXIT_SUCCESS;
    }

  //---------------------------------------------------------------------------
}
