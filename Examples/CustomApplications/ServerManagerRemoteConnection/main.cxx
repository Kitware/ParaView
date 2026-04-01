#include <vtkCLIOptions.h>
#include <vtkDummyController.h>
#include <vtkInitializationHelper.h>
#include <vtkLogger.h>
#include <vtkMultiProcessController.h>
#include <vtkPVView.h>
#include <vtkProcessModule.h>
#include <vtkRemotingCoreConfiguration.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMProxyManager.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkSMSession.h>
#include <vtkSMSessionClient.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSmartPointer.h>

#include <vtk_cli11.h>

#include <string>

namespace
{

vtkSmartPointer<vtkSMSessionClient> Initialize(const std::string& serverURL)
{
  vtkInitializationHelper::SetApplicationName("SandboxClient");
  vtkInitializationHelper::Initialize("SandboxClient", vtkProcessModule::PROCESS_CLIENT);

  // Trigger connection request to give url
  vtkSmartPointer<vtkSMSessionClient> session = vtkSmartPointer<vtkSMSessionClient>::New();
  if (!session->Connect(serverURL.c_str()))
  {
    vtkLogF(ERROR, "Could not connect to server with URL: %s", serverURL.c_str());
    return nullptr;
  }

  // Setup the objects that needs the session handle
  vtkProcessModule::GetProcessModule()->RegisterSession(session);
  vtkSMProxyManager::GetProxyManager()->SetActiveSession(session);

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->InitializeSession(session);

  return session;
}

void Shutdown(vtkSmartPointer<vtkSMSessionClient> session)
{
  // Call disconnection event and unregister the session automatically.
  vtkSMSessionClient::Disconnect(session);

  vtkInitializationHelper::Finalize();
}

void MainLoop(vtkSmartPointer<vtkSMSessionClient> session,
  vtkSmartPointer<vtkSMParaViewPipelineControllerWithRendering> controller, bool inTesting)
{
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();

  // Create a render view
  vtkSmartPointer<vtkSMRenderViewProxy> renderView = vtkSmartPointer<vtkSMRenderViewProxy>::Take(
    vtkSMRenderViewProxy::SafeDownCast(pxm->NewProxy("views", "RenderView")));
  controller->InitializeProxy(renderView);
  controller->RegisterViewProxy(renderView);
  renderView->MakeRenderWindowInteractor();
  renderView->UpdateVTKObjects();

  // Create a source for the render view
  vtkSmartPointer<vtkSMSourceProxy> coneSource = vtkSmartPointer<vtkSMSourceProxy>::Take(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "ConeSource")));
  controller->RegisterPipelineProxy(coneSource);

  controller->Show(coneSource, 0, renderView);

  if (inTesting)
  {
    // If we run this as a test, we just render one frame to make sure it works
    renderView->StillRender();
  }
  else
  {
    // Start event loop
    renderView->GetInteractor()->Start();
  }
}

}

int main(int argc, char* argv[])
{
  vtkNew<vtkCLIOptions> cliOptions;
  bool inTesting = false;
  std::string serverURL;
  cliOptions->GetCLI11App()->add_option(
    "--url", serverURL, "The URL of the server, formatted as cs://host:port");
  cliOptions->GetCLI11App()->add_flag(
    "--test", inTesting, "Added by the server to indicate that it is a test run.");
  cliOptions->Parse(argc, argv);
  if (serverURL.empty())
  {
    vtkLogF(ERROR, "--url option needs to be specified.");
    return EXIT_FAILURE;
  }

  // Initializing connection
  vtkSmartPointer<vtkSMSessionClient> session = ::Initialize(serverURL);
  if (!session)
  {
    vtkLogF(ERROR, "Connection failed, aborting.");
    return EXIT_FAILURE;
  }

  // Setup dummy controller
  vtkNew<vtkDummyController> multiProcessController;
  vtkMultiProcessController::SetGlobalController(multiProcessController);

  // Starting event loop
  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
  ::MainLoop(session, controller, inTesting);

  // Clear all resources
  ::Shutdown(session);

  return EXIT_SUCCESS;
}
