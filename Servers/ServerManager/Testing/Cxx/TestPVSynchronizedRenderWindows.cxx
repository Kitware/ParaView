

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

#include <QApplication>
#include <QMainWindow>
#include "QVTKWidget.h"
#include <QHBoxLayout>

//#define SECOND_WINDOW
//#define REMOTE_CONNECTION

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  QMainWindow mainWindow;
  mainWindow.resize(400, 400);
  mainWindow.show();
  QApplication::processEvents();

  vtkPVOptions* options = vtkPVOptions::New();
  vtkInitializationHelper::Initialize(argc, argv, options);
  options->Delete();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
#ifdef REMOTE_CONNECTION
  vtkIdType connectionID = pm->ConnectToRemote("localhost", 11111);
#else
  vtkIdType connectionID = pm->ConnectToSelf();
#endif

  vtkSMProxy* proxy = vtkSMProxyManager::GetProxyManager()->NewProxy("views",
    "RenderView2");
  proxy->SetConnectionID(connectionID);
  vtkSMPropertyHelper(proxy,"Size").Set(0, 400);
  vtkSMPropertyHelper(proxy,"Size").Set(1, 400);
  proxy->UpdateVTKObjects();
  proxy->UpdateVTKObjects();

  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(proxy->GetClientSideObject());
  QVTKWidget* qwidget = new QVTKWidget(&mainWindow);
  qwidget->SetRenderWindow(rv->GetRenderWindow());

  QWidget *centralWidget = new QWidget(&mainWindow);
  QHBoxLayout* hbox = new QHBoxLayout(centralWidget);
  mainWindow.setCentralWidget(centralWidget);
  hbox->addWidget(qwidget);

  proxy->InvokeCommand("ResetCamera");
#ifdef SECOND_WINDOW

  vtkSMProxy* proxy2 = vtkSMProxyManager::GetProxyManager()->NewProxy("views",
    "RenderView2");
  proxy2->SetConnectionID(connectionID);
  vtkSMPropertyHelper(proxy2, "Size").Set(0, 400);
  vtkSMPropertyHelper(proxy2, "Size").Set(1, 400);
  vtkSMPropertyHelper(proxy2, "Position").Set(0, 400);
  proxy2->UpdateVTKObjects();

  vtkPVRenderView* rv2 = vtkPVRenderView::SafeDownCast(proxy2->GetClientSideObject());
  qwidget = new QVTKWidget(&mainWindow);
  qwidget->SetRenderWindow(rv2->GetRenderWindow());
  hbox->addWidget(qwidget);
  proxy2->InvokeCommand("ResetCamera");

#endif
  mainWindow.show();

  int ret = app.exec();
#ifdef SECOND_WINDOW
  proxy2->Delete();
#endif
  proxy->Delete();

  vtkInitializationHelper::Finalize();
  return ret;
}
