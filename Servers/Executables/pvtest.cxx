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
#include "vtkPVFileInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"

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
    vtkSMPropertyHelper(proxy, "Path").Set("/tmp");
    vtkSMPropertyHelper(proxy, "SpecialDirectories").Set(0);
    proxy->UpdateVTKObjects();

    vtkPVFileInformation* info = vtkPVFileInformation::New();
    proxy->GatherInformation(info);
    info->Print(cout);
    info->Delete();
    proxy->Delete();

    proxy = pxm->NewProxy("sources", "SphereSource");
    vtkSMPropertyHelper(proxy, "PhiResolution").Set(20);
    vtkSMPropertyHelper(proxy, "ThetaResolution").Set(20);
    proxy->UpdateVTKObjects();


    vtkSMProxy* shrink = pxm->NewProxy("filters", "ShrinkFilter");
    vtkSMPropertyHelper(shrink, "Input").Set(proxy);
    shrink->UpdateVTKObjects();
    vtkSMSourceProxy::SafeDownCast(shrink)->UpdatePipeline();

    proxy->Delete();
    shrink->Delete();
    cout << "Exiting..." << endl;
    session->Delete();
    }
  vtkInitializationHelper::Finalize();
  options->Delete();
  return ret_value;
}
