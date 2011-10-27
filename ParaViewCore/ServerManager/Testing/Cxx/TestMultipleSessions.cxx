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
#include "vtkProcessModule.h"
#include "vtkPVFileInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"
#include "vtkNew.h"

int main(int argc, char* argv[])
{
  vtkNew<vtkPVOptions> options;
  vtkInitializationHelper::Initialize(argc, argv,
    vtkProcessModule::PROCESS_CLIENT, options.GetPointer());
  
  vtkIdType session1ID = vtkSMSession::ConnectToSelf();
  vtkIdType session2ID = vtkSMSession::ConnectToSelf();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMSessionProxyManager* spxm1 = pxm->GetSessionProxyManager(
    vtkSMSession::SafeDownCast(pm->GetSession(session1ID)));
  vtkSMSessionProxyManager* spxm2 = pxm->GetSessionProxyManager(
    vtkSMSession::SafeDownCast(pm->GetSession(session2ID)));
  
  vtkSMProxy* sphere1 = spxm1->NewProxy("sources", "SphereSource");
  sphere1->UpdateVTKObjects();

  vtkSMProxy* sphere2 = spxm2->NewProxy("sources", "SphereSource");
  sphere2->UpdateVTKObjects();

  sphere1->Delete();
  sphere2->Delete();

  vtkInitializationHelper::Finalize();
  return 0;
}
