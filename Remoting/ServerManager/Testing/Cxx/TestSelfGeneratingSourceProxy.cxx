/*=========================================================================

Program:   ParaView
Module:    TestSelfGeneratingSourceProxy.cxx

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
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSelfGeneratingSourceProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

#include <assert.h>
#include <sstream>

const char* testdefinition =
  "<ServerManagerConfiguration>"
  "   <ProxyGroup name=\"sources\">"
  "     <SelfGeneratingSourceProxy name=\"ExampleSelfGeneratingSourceProxy\" "
  "class=\"vtkSphereSource\" />"
  "   </ProxyGroup>"
  "</ServerManagerConfiguration>";

const char* extension = " <Proxy> "
                        "     <DoubleVectorProperty animateable='1'"
                        "                           command='SetCenter'"
                        "                           default_values='0.0 0.0 0.0'"
                        "                           name='Center'"
                        "                           number_of_elements='3'"
                        "                           panel_visibility='default'>"
                        "       <DoubleRangeDomain name='range' />"
                        "     </DoubleVectorProperty>"
                        " </Proxy> ";

int TestSelfGeneratingSourceProxy(int argc, char* argv[])
{
  (void)argc;
  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  vtkNew<vtkSMParaViewPipelineController> controller;

  // Create a new session.
  vtkSMSession* session = vtkSMSession::New();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  if (!controller->InitializeSession(session))
  {
    return EXIT_FAILURE;
  }

  pxm->GetProxyDefinitionManager()->LoadConfigurationXMLFromString(testdefinition);

  vtkSmartPointer<vtkSMSelfGeneratingSourceProxy> sgProxy;
  sgProxy.TakeReference(vtkSMSelfGeneratingSourceProxy::SafeDownCast(
    pxm->NewProxy("sources", "ExampleSelfGeneratingSourceProxy")));
  controller->InitializeProxy(sgProxy);
  if (!sgProxy->ExtendDefinition(extension))
  {
    cerr << "Failed to extend proxy definition!" << endl;
    return EXIT_FAILURE;
  }
  sgProxy->UpdateVTKObjects();
  controller->RegisterPipelineProxy(sgProxy, "mysamplesource");

  double center[3] = { 10, 11, 12 };
  vtkSMPropertyHelper(sgProxy, "Center").Set(center, 3);
  sgProxy->UpdateVTKObjects();

  vtkWeakPointer<vtkSphereSource> sphere =
    vtkSphereSource::SafeDownCast(sgProxy->GetClientSideObject());
  if (sphere->GetCenter()[0] != center[0] || sphere->GetCenter()[1] != center[1] ||
    sphere->GetCenter()[2] != center[2])
  {
    cerr << "Failed to update VTK object correctly!" << endl;
    return EXIT_FAILURE;
  }

  // Test that saving and loading state works.
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    cerr << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string path = tempDir;
  path += "/TestSelfGeneratingSourceProxy.pvsm";
  delete[] tempDir;

  pxm->SaveXMLState(path.c_str());
  controller->UnRegisterProxy(sgProxy);
  sgProxy = nullptr;

  if (sphere != nullptr)
  {
    cerr << "Cleanup has failed!" << endl;
    return EXIT_FAILURE;
  }

  pxm->LoadXMLState(path.c_str());

  sgProxy =
    vtkSMSelfGeneratingSourceProxy::SafeDownCast(pxm->GetProxy("sources", "mysamplesource"));
  if (!sgProxy)
  {
    cerr << "Failed to load proxy from state file!" << endl;
    return EXIT_FAILURE;
  }

  sphere = vtkSphereSource::SafeDownCast(sgProxy->GetClientSideObject());
  if (sphere->GetCenter()[0] != center[0] || sphere->GetCenter()[1] != center[1] ||
    sphere->GetCenter()[2] != center[2])
  {
    cerr << "State didn't set VTK object ivars correctly. Load must have failed!" << endl;
    return EXIT_FAILURE;
  }

  // Now change center again to see it continues to work.
  center[0] = 13;
  center[1] = 14;
  center[2] = 15;
  vtkSMPropertyHelper(sgProxy, "Center").Set(center, 3);
  sgProxy->UpdateVTKObjects();
  if (sphere->GetCenter()[0] != center[0] || sphere->GetCenter()[1] != center[1] ||
    sphere->GetCenter()[2] != center[2])
  {
    cerr << "Failed to update VTK object correctly after loading state!" << endl;
    return EXIT_FAILURE;
  }
  sgProxy = nullptr;

  session->Delete();
  vtkInitializationHelper::Finalize();
  return EXIT_SUCCESS;
}
