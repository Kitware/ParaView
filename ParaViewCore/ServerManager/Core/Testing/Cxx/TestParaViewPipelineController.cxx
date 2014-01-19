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
#include "vtkSMProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

#include <vtksys/ios/sstream>
#include <assert.h>

int main(int argc, char* argv[])
{
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

  if (controller->FindTimeKeeper(session) == NULL)
    {
    cerr << "Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
    }

  if (controller->FindAnimationScene(session) == NULL)
    {
    cerr << "Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
    }

  if (controller->GetTimeAnimationTrack(controller->GetAnimationScene(session)) == NULL)
    {
    cerr << "Failed at line " << __LINE__ << endl;
    return EXIT_FAILURE;
    }


  vtkSMProxy* exodusReader = pxm->NewProxy("sources", "ExodusIIReader");
  controller->PreInitializePipelineProxy(exodusReader);
  vtkSMPropertyHelper(exodusReader, "FileName").Set(
    "/home/utkarsh/Kitware/ParaView3/ParaViewData/Data/can.ex2");
  vtkSMPropertyHelper(exodusReader, "ApplyDisplacements").Set(0);
  exodusReader->UpdateVTKObjects();

  controller->PostInitializePipelineProxy(exodusReader);
  exodusReader->Delete();

  //pxm->SaveXMLState("/tmp/state.pvsm");

  session->Delete();
  vtkInitializationHelper::Finalize();
  return EXIT_SUCCESS;
}
