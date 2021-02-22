#include "vtkInitializationHelper.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkProcessModule.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManagerUtilities.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>

namespace
{

template <class T>
std::set<T> operator-(std::set<T> reference, std::set<T> items_to_remove)
{
  std::set<T> result;
  std::set_difference(reference.begin(), reference.end(), items_to_remove.begin(),
    items_to_remove.end(), std::inserter(result, result.end()));
  return result;
}

template <class T>
std::set<T> symmetric_difference(std::set<T> reference, std::set<T> items_to_remove)
{
  std::set<T> result;
  std::set_symmetric_difference(reference.begin(), reference.end(), items_to_remove.begin(),
    items_to_remove.end(), std::inserter(result, result.end()));
  return result;
}

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

std::set<vtkSMProxy*> GetAllProxies(vtkSMSession* session)
{
  std::set<vtkSMProxy*> result;
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(session->GetSessionProxyManager());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    if (auto proxy = iter->GetProxy())
    {
      result.insert(proxy);
    }
  }
  return result;
}

std::set<vtkSMProxy*> CreatePipeline(
  vtkSMSession* session, const std::map<std::string, std::string>& annotations)
{
  const std::set<vtkSMProxy*> startSet = GetAllProxies(session);

  // Create a setup a view.
  auto view = SetupView(session);

  // Setup a default visualization pipeline to show a sliced Wavelet.
  auto wavelet = CreatePipelineProxy(session, "sources", "RTAnalyticSource");

  // This ensure that when the slice is created, it has good data bounds and
  // data information to setup the slice position by default to be fairly
  // reasonable.
  wavelet->UpdatePipeline();

  auto slice = CreatePipelineProxy(session, "filters", "Cut", wavelet);
  auto cutFunction = vtkSMPropertyHelper(slice, "CutFunction").GetAsProxy();
  double normal[] = { 0, 0, 1 };
  vtkSMPropertyHelper(cutFunction, "Normal").Set(normal, 3);
  cutFunction->UpdateVTKObjects();

  // Show the slice in the view.
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  auto sliceRepresentation = controller->Show(slice, 0, view);

  view->ResetCamera();
  view->StillRender();

  const std::set<vtkSMProxy*> endSet = GetAllProxies(session);

  // add pair to pipeline proxies.
  for (auto& pair : annotations)
  {
    wavelet->SetAnnotation(pair.first.c_str(), pair.second.c_str());
    slice->SetAnnotation(pair.first.c_str(), pair.second.c_str());
    view->SetAnnotation(pair.first.c_str(), pair.second.c_str());
  }

  auto result = (endSet - startSet);

  // since the transfer function proxies are reused, we add them to the result
  // set explicitly.
  if (auto lut = vtkSMPropertyHelper(sliceRepresentation, "LookupTable").GetAsProxy())
  {
    result.insert(lut);
    result.insert(vtkSMPropertyHelper(lut, "ScalarOpacityFunction").GetAsProxy());
  }

  // add material library explicitly, if present.
  vtkNew<vtkSMParaViewPipelineController> contr;
  if (auto materialLib = contr->FindMaterialLibrary(session))
  {
    result.insert(materialLib);
  }

  return result;
}
}

int TestProxyManagerUtilities(int, char* argv[])
{
  vtkInitializationHelper::SetApplicationName("TestParaViewPipelineControllerWithRendering");
  vtkInitializationHelper::SetOrganizationName("Humanity");
  vtkInitializationHelper::Initialize(argv[0], vtkProcessModule::PROCESS_CLIENT);

  auto session = vtkSmartPointer<vtkSMSession>::New();

  // Register the session with the process module.
  vtkProcessModule::GetProcessModule()->RegisterSession(session.Get());

  // Initializes a session and setups all basic proxies that are needed for a
  // ParaView-like application.
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  controller->InitializeSession(session.Get());

  auto pxm = session->GetSessionProxyManager();

  auto proxies1 = CreatePipeline(session, { { "pipeline", "one" }, { "foo", "bar" } });
  auto proxies2 = CreatePipeline(session, { { "pipeline", "two" }, { "foo", "bar" } });

  vtkNew<vtkSMProxyManagerUtilities> utils;
  utils->SetProxyManager(pxm);
  const auto set1 =
    utils->GetProxiesWithAllAnnotations({ { "pipeline", "one" }, { "foo", "bar" } });
  if (set1.size() != 3)
  {
    vtkLogF(ERROR, "Invalid annotation set #1");
    return EXIT_FAILURE;
  }

  const auto set2 =
    utils->GetProxiesWithAnyAnnotations({ { "pipeline", "one" }, { "foo", "bar" } });
  if (set2.size() != 6)
  {
    vtkLogF(ERROR, "Invalid annotation set #2");
    return EXIT_FAILURE;
  }

  auto group1 = utils->CollectHelpersAndRelatedProxies(
    utils->GetProxiesWithAllAnnotations({ { "pipeline", "one" } }));
  if (group1 != proxies1)
  {
    vtkLogF(ERROR, "Failed to get all related proxies #1");
    for (const auto& item : symmetric_difference(proxies1, group1))
    {
      vtkLogF(ERROR, "%s, %s", item->GetXMLGroup(), item->GetXMLName());
    }
    return EXIT_FAILURE;
  }

  auto group2 = utils->CollectHelpersAndRelatedProxies(
    utils->GetProxiesWithAllAnnotations({ { "pipeline", "two" } }));
  if (group2 != proxies2)
  {
    vtkLogF(ERROR, "Failed to get all related proxies #2.");
    for (const auto& item : symmetric_difference(proxies2, group2))
    {
      vtkLogF(ERROR, "%s, %s", item->GetXMLGroup(), item->GetXMLName());
    }
    return EXIT_FAILURE;
  }

  // Unregistering pipeline proxies will also release any representations
  // created for these proxies.
  pxm->UnRegisterProxies();

  vtkProcessModule::GetProcessModule()->UnRegisterSession(session.Get());
  session = nullptr;

  vtkInitializationHelper::Finalize();
  return EXIT_SUCCESS;
}
