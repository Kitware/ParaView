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
#include "vtkSMSession.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMIntVectorProperty.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  vtkPVServerOptions* options = vtkPVServerOptions::New();
  bool success = true;
  vtkInitializationHelper::Initialize(argc, argv,
    vtkProcessModule2::PROCESS_BATCH, options);
  if (!success)
    {
    return -1;
    }

  int ret_value = 0;

  vtkProcessModule2* pm = vtkProcessModule2::GetProcessModule();
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
    vtkSMSession* session = vtkSMSession::New();
    cout << "Starting..." << endl;

    vtkSMProxyManager* pxm = session->GetProxyManager();
    vtkSMProxy* proxy = pxm->NewProxy("misc", "FileInformationHelper");
    vtkSMStringVectorProperty::SafeDownCast(
      proxy->GetProperty("Path"))->SetElement(0, "/tmp");
    vtkSMIntVectorProperty::SafeDownCast(
      proxy->GetProperty("SpecialDirectories"))->SetElement(0, 1);
    proxy->UpdateVTKObjects();
    proxy->Delete();
    cout << "Exiting..." << endl;
    session->Delete();
    }
  vtkInitializationHelper::Finalize();
  options->Delete();
  return ret_value;
}
