/*=========================================================================

Program:   ParaView
Module:    TestRecreateVTKObjects.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInitializationHelper.h"
#include "vtkNew.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkWeakPointer.h"

int TestRecreateVTKObjects(int argc, char* argv[])
{
  (void)argc;

  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  // Create a new session.
  vtkNew<vtkSMSession> session;
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  vtkSmartPointer<vtkSMSourceProxy> sphereSource;
  sphereSource.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "SphereSource")));
  vtkSMPropertyHelper(sphereSource, "Radius").Set(10);
  sphereSource->UpdateVTKObjects();
  sphereSource->UpdatePipeline();

  vtkWeakPointer<vtkObject> oldObject =
    vtkObject::SafeDownCast(sphereSource->GetClientSideObject());
  sphereSource->RecreateVTKObjects();
  if (oldObject != NULL)
  {
    cerr << "ERROR: Old VTKObject not deleted!!!" << endl;
    return EXIT_FAILURE;
  }
  if (sphereSource->GetClientSideObject() == NULL)
  {
    cerr << "ERROR: New VTKObject not created!!!" << endl;
    return EXIT_FAILURE;
  }

  // Ensure that the new VTK object indeed has the same radius as before.
  if (vtkSphereSource::SafeDownCast(sphereSource->GetClientSideObject())->GetRadius() != 10)
  {
    cerr << "ERROR: Recreated VTK object doesn't have same state as original!!!" << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
