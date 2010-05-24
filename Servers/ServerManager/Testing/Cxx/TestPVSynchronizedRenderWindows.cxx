

#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkInitializationHelper.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkRenderView.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkPVRenderView.h"
#include "vtkRenderWindow.h"
#include "vtkSMPropertyHelper.h"

int main(int argc, char** argv)
{
  vtkPVOptions* options = vtkPVOptions::New();
  vtkInitializationHelper::Initialize(argc, argv, options);
  options->Delete();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkIdType connectionID = pm->ConnectToRemote("localhost", 11111);

  vtkSMProxy* proxy = vtkSMProxyManager::GetProxyManager()->NewProxy("views",
    "RenderView2");
  proxy->SetConnectionID(connectionID);
  vtkSMPropertyHelper(proxy,"Size").Set(0, 400);
  vtkSMPropertyHelper(proxy,"Size").Set(1, 400);
  proxy->UpdateVTKObjects();
  proxy->UpdateVTKObjects();

  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(proxy->GetClientSideObject());
  rv->GetRenderWindow()->Render();
//  rv->GetInteractor()->Start();

  vtkSMProxy* proxy2 = vtkSMProxyManager::GetProxyManager()->NewProxy("views",
    "RenderView2");
  proxy2->SetConnectionID(connectionID);
  vtkSMPropertyHelper(proxy2, "Size").Set(0, 400);
  vtkSMPropertyHelper(proxy2, "Size").Set(1, 400);
  vtkSMPropertyHelper(proxy2, "Position").Set(0, 400);
  proxy2->UpdateVTKObjects();

  vtkPVRenderView* rv2 = vtkPVRenderView::SafeDownCast(proxy2->GetClientSideObject());
  rv2->GetRenderWindow()->Render();
  rv2->GetInteractor()->Start();

  proxy->Delete();
  proxy2->Delete();

  vtkInitializationHelper::Finalize();
  return 0;
}
