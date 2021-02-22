/*=========================================================================

Program:   ParaView
Module:    TestParaViewPipelineController.cxx

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
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

#include <assert.h>
#include <sstream>

int TestParaViewPipelineController(int argc, char* argv[])
{
  (void)argc;
  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  vtkNew<vtkSMParaViewPipelineController> controller;

  // Create a new session.
  vtkSMSession* session = vtkSMSession::New();
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  if (!controller->InitializeSession(session))
  {
    cerr << "Failed to initialize ParaView session." << endl;
    return EXIT_FAILURE;
  }

  if (controller->FindTimeKeeper(session) == nullptr)
  {
    cerr << "Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  if (controller->FindAnimationScene(session) == nullptr)
  {
    cerr << "Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  if (controller->GetTimeAnimationTrack(controller->GetAnimationScene(session)) == nullptr)
  {
    cerr << "Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  {
    // Create reader.
    vtkSmartPointer<vtkSMProxy> exodusReader;
    exodusReader.TakeReference(pxm->NewProxy("sources", "ExodusIIReader"));

    controller->PreInitializeProxy(exodusReader);

    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/can.ex2");
    vtkSMPropertyHelper(exodusReader, "FileName").Set(fname);
    delete[] fname;

    vtkSMPropertyHelper(exodusReader, "ApplyDisplacements").Set(0);
    exodusReader->UpdateVTKObjects();

    controller->PostInitializeProxy(exodusReader);
    controller->RegisterPipelineProxy(exodusReader);

    // Create view
    vtkSmartPointer<vtkSMProxy> view;
    view.TakeReference(pxm->NewProxy("views", "RenderView"));
    controller->PreInitializeProxy(view);
    controller->PostInitializeProxy(view);
    controller->RegisterViewProxy(view);

    // Create display.
    vtkSmartPointer<vtkSMProxy> repr;
    repr.TakeReference(
      vtkSMViewProxy::SafeDownCast(view)->CreateDefaultRepresentation(exodusReader, 0));
    controller->PreInitializeProxy(repr);
    vtkSMPropertyHelper(repr, "Input").Set(exodusReader);
    controller->PostInitializeProxy(repr);
    controller->RegisterRepresentationProxy(repr);

    vtkSMPropertyHelper(view, "Representations").Add(repr);
    view->UpdateVTKObjects();
  }

  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    cerr << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string path = tempDir;
  path += "/state.pvsm";
  pxm->SaveXMLState(path.c_str());
  delete[] tempDir;
  session->Delete();
  vtkInitializationHelper::Finalize();
  return EXIT_SUCCESS;
}
