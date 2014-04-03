/*=========================================================================

Program:   ParaView
Module:    TestSettings.cxx

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
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSmartPointer.h"

#include <vtksys/ios/sstream>
#include <assert.h>

int TestSettings(int argc, char* argv[])
{
  (void) argc;
  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  // Set up settings
  const char * settingString =
    "{\n"
    "  \"sources\" : {\n"
    "    \"SphereSource\" : {\n"
    "      \"Radius\" : 4.25,\n"
    "      \"Center\" : [1, 2, 3]\n"
    "    }\n"
    "  }\n"
    "}\n";
  cout << settingString;

  vtkSMSettings * settings = vtkSMSettings::GetInstance();
  settings->AddCollectionFromString(settingString, 1.0);

  const char * higherPrioritySettingsString =
    "{\n"
    "  \"sources\" : {\n"
    "    \"SphereSource\" : {\n"
    "      \"Radius\" : 1.0\n"
    "    }\n"
    "  }\n"
    "}\n";

  settings->AddCollectionFromString(higherPrioritySettingsString, 2.0);

  vtkNew<vtkSMParaViewPipelineController> controller;

  // Create a new session.
  vtkSMSession* session = vtkSMSession::New();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  if (!controller->InitializeSession(session))
    {
    cerr << "Failed to initialize ParaView session." << endl;
    return EXIT_FAILURE;
    }

  // Create sphere source
  vtkSmartPointer<vtkSMProxy> sphere;
  sphere.TakeReference(pxm->NewProxy("sources", "SphereSource"));
  controller->PreInitializeProxy(sphere);
  controller->PostInitializeProxy(sphere);

  // Check the sphere radius. It should be set from the settings.
  vtkSMDoubleVectorProperty* radiusProperty =
    vtkSMDoubleVectorProperty::SafeDownCast(sphere->GetProperty("Radius"));
  if (radiusProperty)
    {
    if (radiusProperty->GetElement(0) != 1.0)
      {
      cerr << "Failed at " << __LINE__ << endl;
      return EXIT_FAILURE;
      }

    // Check the sphere center. It should be set from the settings.
    vtkSMDoubleVectorProperty* centerProperty =
      vtkSMDoubleVectorProperty::SafeDownCast(sphere->GetProperty("Center"));
    if (centerProperty->GetElement(0) != 1.0 ||
        centerProperty->GetElement(1) != 2.0 ||
        centerProperty->GetElement(2) != 3.0)
      {
      cerr << "Failed at " << __LINE__ << endl;
      return EXIT_FAILURE;
      }
    }
  else
    {
    cerr << "Could not get Radius property\n";
    }

  session->Delete();
  vtkInitializationHelper::Finalize();
  return EXIT_SUCCESS;
}
