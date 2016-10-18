/*=========================================================================

Program:   ParaView
Module:    TestSubProxy.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests support for multiple-sessions in ServerManager.
#include "vtkInitializationHelper.h"
#include "vtkMultiProcessController.h"
#include "vtkNetworkAccessManager.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

int TestMultipleSessions(int argc, char* argv[])
{
  vtkNew<vtkPVOptions> options;
  vtkInitializationHelper::Initialize(
    argc, argv, vtkProcessModule::PROCESS_CLIENT, options.GetPointer());

  vtkIdType session1ID = vtkSMSession::ConnectToSelf();
  vtkIdType session2ID = vtkSMSession::ConnectToRemote("localhost", 11111);

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  pxm->SetActiveSession(session1ID);
  vtkSMProxy* sphere1 = pxm->NewProxy("sources", "SphereSource");
  sphere1->UpdateVTKObjects();

  vtkSMProxy* view1 = pxm->NewProxy("views", "RenderView");
  view1->UpdateVTKObjects();

  pxm->SetActiveSession(session2ID);
  vtkSMProxy* sphere2 = pxm->NewProxy("sources", "SphereSource");
  sphere2->UpdateVTKObjects();

  vtkSMProxy* view2 = pxm->NewProxy("views", "RenderView");
  vtkSMPropertyHelper(view2, "RemoteRenderThreshold").Set(0);
  view2->UpdateVTKObjects();

  vtkSMViewProxy::SafeDownCast(view1)->StillRender();
  vtkSMViewProxy::SafeDownCast(view2)->StillRender();
  int foo;
  cin >> foo;

  vtkSMViewProxy::SafeDownCast(view1)->StillRender();
  vtkSMViewProxy::SafeDownCast(view2)->StillRender();
  cin >> foo;

  sphere1->Delete();
  sphere2->Delete();
  view1->Delete();
  view2->Delete();

  vtkInitializationHelper::Finalize();
  return 0;
}
