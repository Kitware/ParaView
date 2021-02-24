/*=========================================================================

Program:   ParaView
Module:    TestParaViewPipelineControllerWithRendering.cxx

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
#include "vtkRenderWindowInteractor.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"

#include <assert.h>
#include <sstream>

// This demonstrates how to put together a simple application with rendering
// capabilities using the vtkSMParaViewPipelineControllerWithRendering.

namespace
{
vtkSMRenderViewProxy* SetupView(vtkSMSession* session)
{
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSmartPointer<vtkSMRenderViewProxy> view;
  view.TakeReference(vtkSMRenderViewProxy::SafeDownCast(pxm->NewProxy("views", "RenderView")));

  // You can create as many controller instances as needed. Controllers have
  // no persistent state.
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  // Initialize the view.
  controller->InitializeProxy(view.Get());

  view->UpdateVTKObjects();

  // Registration is optional. For an application, you have  to decide if you
  // are going to register proxies with proxy manager or not. Generally,
  // registration helps with state save/restore and hence may make sense for
  // most applications.
  controller->RegisterViewProxy(view.Get());

  // Since in this example we are not using Qt, we setup a standard
  // vtkRenderWindowInteractor to enable interaction.
  view->MakeRenderWindowInteractor();

  return view.Get();
}

vtkSMSourceProxy* CreatePipelineProxy(
  vtkSMSession* session, const char* xmlgroup, const char* xmlname, vtkSMProxy* input = nullptr)
{
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSmartPointer<vtkSMSourceProxy> proxy;
  proxy.TakeReference(vtkSMSourceProxy::SafeDownCast(pxm->NewProxy(xmlgroup, xmlname)));
  if (!proxy)
  {
    vtkGenericWarningMacro("Failed to create: " << xmlgroup << ", " << xmlname << ". Aborting !!!");
    abort();
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(proxy.Get());
  if (input != nullptr)
  {
    vtkSMPropertyHelper(proxy, "Input").Set(input);
  }
  controller->PostInitializeProxy(proxy.Get());
  proxy->UpdateVTKObjects();

  controller->RegisterPipelineProxy(proxy);
  return proxy.Get();
}
}

int TestParaViewPipelineControllerWithRendering(int argc, char* argv[])
{
  vtkInitializationHelper::SetApplicationName("TestParaViewPipelineControllerWithRendering");
  vtkInitializationHelper::SetOrganizationName("Humanity");
  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  vtkNew<vtkSMSession> session;

  // Register the session with the process module.
  vtkProcessModule::GetProcessModule()->RegisterSession(session.Get());

  // Initializes a session and setups all basic proxies that are needed for a
  // ParaView-like application.
  controller->InitializeSession(session.Get());

  // Create a setup a view.
  vtkSMRenderViewProxy* view = SetupView(session.Get());

  // Setup a default visualization pipeline to show a sliced Wavelet.
  vtkSMSourceProxy* wavelet = CreatePipelineProxy(session.Get(), "sources", "RTAnalyticSource");

  // This ensure that when the slice is created, it has good data bounds and
  // data information to setup the slice position by default to be fairly
  // reasonable.
  wavelet->UpdatePipeline();

  vtkSMSourceProxy* slice = CreatePipelineProxy(session.Get(), "filters", "Cut", wavelet);
  vtkSMProxy* cutFunction = vtkSMPropertyHelper(slice, "CutFunction").GetAsProxy();
  double normal[] = { 0, 0, 1 };
  vtkSMPropertyHelper(cutFunction, "Normal").Set(normal, 3);
  cutFunction->UpdateVTKObjects();

  // Show the slice in the view.
  vtkSMProxy* sliceRepresentation = controller->Show(slice, 0, view);
  (void)sliceRepresentation;

  view->ResetCamera();
  view->StillRender();

  for (int cc = 1; cc < argc; cc++)
  {
    if (strcmp(argv[cc], "-I") == 0)
    {
      view->GetInteractor()->Start();
    }
  }

  // Cleanup.

  // Unregistering pipeline proxies will also release any representations
  // created for these proxies.
  controller->UnRegisterProxy(slice);
  controller->UnRegisterProxy(wavelet);
  controller->UnRegisterProxy(view);

  vtkProcessModule::GetProcessModule()->UnRegisterSession(session.Get());
  vtkInitializationHelper::Finalize();
  return 0;
}
