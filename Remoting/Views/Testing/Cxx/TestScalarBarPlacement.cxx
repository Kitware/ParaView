// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtk3DWidgetRepresentation.h"
#include "vtkDataObject.h"
#include "vtkInitializationHelper.h"
#include "vtkNew.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkScalarBarRepresentation.h"

#include <cassert>
#include <sstream>
#include <vector>

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
  vtkSMPropertyHelper(view, "ViewSize").Set(std::vector<int>({ 600, 600 }).data(), 2);
  view->UpdateVTKObjects();

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

extern int TestScalarBarPlacement(int argc, char* argv[])
{
  (void)argc;
  // setup a basic application without Qt for views/interaction
  vtkInitializationHelper::SetApplicationName("TestScalarBarPlacement");
  vtkInitializationHelper::SetOrganizationName("Wayne Foundation");
  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  vtkNew<vtkSMSession> session;
  vtkNew<vtkSMTransferFunctionManager> mgr;
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  vtkProcessModule::GetProcessModule()->RegisterSession(session.Get());
  controller->InitializeSession(session.Get());

  // Setup a view to render into.
  vtkSMRenderViewProxy* view = SetupView(session.Get());

  // Setup two sources that shall each display a scalar bar for their point data.
  vtkSMSourceProxy* wavelet = CreatePipelineProxy(session.Get(), "sources", "RTAnalyticSource");
  vtkSMSourceProxy* sphere = CreatePipelineProxy(session.Get(), "sources", "SphereSource");
  vtkSMPropertyHelper(sphere, "Center")
    .Set(std::vector<double>({ 20, 0, 0 }).data(), 3); // move sphere to x=10
  vtkSMPropertyHelper(sphere, "Radius").Set(5.0);      // Increase sphere radius

  // Update
  wavelet->UpdatePipeline();
  sphere->UpdateVTKObjects();
  sphere->UpdatePipeline();

  // Set representation to surface to visualize it and enable scalar coloring.
  auto waveletRep = vtkSMPVRepresentationProxy::SafeDownCast(controller->Show(wavelet, 0, view));
  waveletRep->SetRepresentationType("Surface");
  vtkSMColorMapEditorHelper::SetScalarColoring(waveletRep, "RTData", vtkDataObject::POINT);
  vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(waveletRep);

  // Set representation to surface to visualize it and enable scalar coloring.
  auto sphereRep = vtkSMPVRepresentationProxy::SafeDownCast(controller->Show(sphere, 0, view));
  sphereRep->SetRepresentationType("Surface");
  vtkSMColorMapEditorHelper::SetScalarColoring(sphereRep, "Normals", vtkDataObject::POINT);
  vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(sphereRep);

  // Get the scalar bar for wavelet and set its location and position.
  auto rtDataTf = mgr->GetColorTransferFunction("RTData", pxm);
  auto waveletSb = mgr->GetScalarBarRepresentation(rtDataTf, view);
  vtkSMPropertyHelper(waveletSb, "WindowLocation")
    .Set(static_cast<int>(vtkScalarBarRepresentation::AnyLocation));
  double positionWaveletSb[2] = { 0, 0 };
  vtkSMPropertyHelper(waveletSb, "Position").Set(positionWaveletSb, 2);
  vtkSMColorMapEditorHelper::SetScalarBarVisibility(
    waveletRep, view, true); // calls UpdateVTKObjects on waveletSb

  // verify that window location did not change.
  auto waveletSbWidgetRep =
    vtk3DWidgetRepresentation::SafeDownCast(waveletSb->GetClientSideObject());
  vtkScalarBarRepresentation* waveletSbClientSideRepObj = nullptr;
  if (waveletSbWidgetRep != nullptr)
  {
    waveletSbClientSideRepObj =
      vtkScalarBarRepresentation::SafeDownCast(waveletSbWidgetRep->GetRepresentation());
    if (waveletSbClientSideRepObj != nullptr)
    {
      const int& location = waveletSbClientSideRepObj->GetWindowLocation();
      if (location != static_cast<int>(vtkScalarBarRepresentation::AnyLocation))
      {
        cerr << "ERROR: Failed at line " << __LINE__ << endl;
        return EXIT_FAILURE;
      }
    }
  }

  // Get the scalar bar for sphere and set its location and position.
  auto normalTf = mgr->GetColorTransferFunction("Normals", pxm);
  auto sphereSb = mgr->GetScalarBarRepresentation(normalTf, view);
  vtkSMPropertyHelper(sphereSb, "WindowLocation")
    .Set(static_cast<int>(vtkScalarBarRepresentation::AnyLocation));
  double positionSphereSb[2] = { 0.5, 0.5 };
  vtkSMPropertyHelper(sphereSb, "Position").Set(positionSphereSb, 2);
  vtkSMColorMapEditorHelper::SetScalarBarVisibility(
    sphereRep, view, true); // calls UpdateVTKObjects on sphereSb

  // verify that window location did not alter.
  auto sphereSbWidgetRep = vtk3DWidgetRepresentation::SafeDownCast(sphereSb->GetClientSideObject());
  vtkScalarBarRepresentation* sphereSbClientSideRepObject = nullptr;
  if (sphereSbWidgetRep != nullptr)
  {
    sphereSbClientSideRepObject =
      vtkScalarBarRepresentation::SafeDownCast(sphereSbWidgetRep->GetRepresentation());
    if (sphereSbClientSideRepObject != nullptr)
    {
      const int& location = sphereSbClientSideRepObject->GetWindowLocation();
      if (location !=
        static_cast<int>(vtkScalarBarRepresentation::AnyLocation)) // should be AnyLocation
      {
        cerr << "ERROR: Failed at line " << __LINE__ << endl;
        return EXIT_FAILURE;
      }
    }
  }

  // Render
  view->ResetCamera();
  view->StillRender();

  // Optional interaction.
  // Dev note: You should be able to interactively move both the scalar bars with mouse!
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
  controller->UnRegisterProxy(sphere);
  controller->UnRegisterProxy(wavelet);
  controller->UnRegisterProxy(view);

  vtkProcessModule::GetProcessModule()->UnRegisterSession(session.Get());
  vtkInitializationHelper::Finalize();

  return EXIT_SUCCESS;
}
