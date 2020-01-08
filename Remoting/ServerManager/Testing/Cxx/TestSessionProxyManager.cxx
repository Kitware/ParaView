/*=========================================================================

Program:   ParaView
Module:    TestSessionProxyManager.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCollection.h"
#include "vtkInitializationHelper.h"
#include "vtkNew.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

int TestSessionProxyManager(int argc, char* argv[])
{
  (void)argc;

  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  // Create a new session.
  vtkNew<vtkSMSession> session;
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  vtkSmartPointer<vtkSMProxy> sphereSource;
  sphereSource.TakeReference(pxm->NewProxy("sources", "SphereSource"));
  pxm->RegisterProxy("sources", "SphereSource", sphereSource);

  vtkNew<vtkCollection> sourceProxies;

  pxm->GetProxies("sources", sourceProxies.GetPointer());
  if (sourceProxies->GetNumberOfItems() != 1)
  {
    cerr << "Expected to get 1 source object, got " << sourceProxies->GetNumberOfItems() << "\n";
    vtkInitializationHelper::Finalize();
    return EXIT_FAILURE;
  }

  vtkInitializationHelper::Finalize();
  return EXIT_SUCCESS;
}
