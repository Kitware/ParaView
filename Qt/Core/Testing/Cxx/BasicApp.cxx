
// A Test of a very simple app based on pqCore
#include "BasicApp.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <QTimer>

#include "pqApplicationCore.h"
#include "pqCoreTestUtility.h"
#include "pqObjectBuilder.h"
#include "pqOptions.h"
#include "pqQVTKWidget.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include "QVTKRenderWindowAdapter.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <cassert>

MainWindow::MainWindow()
{
  // automatically make a server connection
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();
  pqServer* server = core->getObjectBuilder()->createServer(pqServerResource("builtin:"));

  vtkSMSessionProxyManager* pxm = server->proxyManager();

  vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

  // create a graphics window and put it in our main window
  vtkSmartPointer<vtkSMProxy> viewProxy;
  viewProxy.TakeReference(pxm->NewProxy("views", "RenderView"));
  assert(viewProxy);

  controller->InitializeProxy(viewProxy);
  controller->RegisterViewProxy(viewProxy);

  this->RenderView = smmodel->findItem<pqRenderView*>(viewProxy);
  assert(this->RenderView);
  this->setCentralWidget(this->RenderView->widget());

  // create source and elevation filter
  vtkSmartPointer<vtkSMProxy> source;
  source.TakeReference(pxm->NewProxy("sources", "SphereSource"));
  controller->InitializeProxy(source);
  controller->RegisterPipelineProxy(source);

  // updating source so that when elevation filter is created, the defaults
  // are setup correctly using the correct data bounds etc.
  vtkSMSourceProxy::SafeDownCast(source)->UpdatePipeline();

  vtkSmartPointer<vtkSMProxy> elevation;
  elevation.TakeReference(pxm->NewProxy("filters", "ElevationFilter"));
  controller->PreInitializeProxy(elevation);
  vtkSMPropertyHelper(elevation, "Input").Set(source);
  controller->PostInitializeProxy(elevation);
  controller->RegisterPipelineProxy(elevation);

  // Show the result.
  controller->Show(
    vtkSMSourceProxy::SafeDownCast(elevation), 0, vtkSMViewProxy::SafeDownCast(viewProxy));

  // zoom to sphere
  this->RenderView->resetDisplay();

  // make sure we update
  this->RenderView->render();
  QTimer::singleShot(100, this, SLOT(processTest()));
}

void MainWindow::processTest()
{
  if (pqOptions* const options = pqApplicationCore::instance()->getOptions())
  {
    // make sure the widget had enough time to become valid
    pqQVTKWidget* qwdg = qobject_cast<pqQVTKWidget*>(this->RenderView->widget());
    if (qwdg != nullptr && !qwdg->isValid())
    {
      QTimer::singleShot(100, this, SLOT(processTest()));
      return;
    }

    bool comparison_succeeded = true;
    if ((options->GetNumberOfTestScripts() > 0) && (options->GetTestBaseline(0) != nullptr))
    {
      comparison_succeeded = this->compareView(options->GetTestBaseline(0),
        options->GetTestImageThreshold(0), cout, options->GetTestDirectory());
    }
    if (options->GetExitAppWhenTestsDone())
    {
      QApplication::instance()->exit(comparison_succeeded ? 0 : 1);
    }
  }
}

bool MainWindow::compareView(
  const QString& referenceImage, double threshold, ostream& output, const QString& tempDirectory)
{
  pqRenderView* renModule = this->RenderView;
  if (!renModule)
  {
    output << "ERROR: Could not locate the render module." << endl;
    return false;
  }

  bool ret = pqCoreTestUtility::CompareView(
    renModule, referenceImage, threshold, tempDirectory, QSize(200, 150));
  renModule->render();
  return ret;
}

int main(int argc, char** argv)
{
  QSurfaceFormat::setDefaultFormat(
    QVTKRenderWindowAdapter::defaultFormat(/*supports_stereo=*/false));

  QApplication app(argc, argv);
  pqApplicationCore appCore(argc, argv);
  MainWindow window;
  window.resize(200, 150);
  window.show();
  return app.exec();
}
