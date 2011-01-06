
// A Test of a very simple app based on pqCore
#include "BasicApp.h"

#include <QTimer>
#include <QApplication>

#include "QVTKWidget.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMSourceProxy.h"

#include "pqApplicationCore.h"
#include "pqCoreTestUtility.h"
#include "pqInterfaceTracker.h"
#include "pqObjectBuilder.h"
#include "pqOptions.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqStandardViewModules.h"
#include "vtkProcessModule.h"

MainWindow::MainWindow()
{
  // automatically make a server connection
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* ob = core->getObjectBuilder();
  pqServer* server = ob->createServer(pqServerResource("builtin:"));

  // Register ParaView interfaces.
  pqInterfaceTracker* pgm = pqApplicationCore::instance()->interfaceTracker();

  // * adds support for standard paraview views.
  pgm->addInterface(new pqStandardViewModules(pgm));

  // create a graphics window and put it in our main window
  this->RenderView = qobject_cast<pqRenderView*>(
    ob->createView(pqRenderView::renderViewType(), server));
  this->setCentralWidget(this->RenderView->getWidget());

  // create source and elevation filter
  pqPipelineSource* source;
  pqPipelineSource* elevation;

  source = ob->createSource("sources", "SphereSource", server);
  // updating source so that when elevation filter is created, the defaults
  // are setup correctly using the correct data bounds etc.
  vtkSMSourceProxy::SafeDownCast(source->getProxy())->UpdatePipeline();

  elevation = ob->createFilter("filters", "ElevationFilter", source);

  // put the elevation in the window
  ob->createDataRepresentation(elevation->getOutputPort(0), this->RenderView);

  // zoom to sphere
  this->RenderView->resetCamera();
  // make sure we update
  this->RenderView->render();
  QTimer::singleShot(100, this, SLOT(processTest()));
}

void MainWindow::processTest()
{
  if (pqOptions* const options = pqApplicationCore::instance()->getOptions())
    {
    bool comparison_succeeded = true;
    if ( (options->GetNumberOfTestScripts() > 0) &&
      (options->GetTestBaseline(0) != NULL))
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

bool MainWindow::compareView(const QString& referenceImage, double threshold,
  ostream& output, const QString& tempDirectory)
{
  pqRenderView* renModule = this->RenderView;

  if (!renModule)
    {
    output << "ERROR: Could not locate the render module." << endl;
    return false;
    }

  QVTKWidget* const widget = qobject_cast<QVTKWidget*>(renModule->getWidget());
  if(!widget)
    {
    output << "ERROR: Not a QVTKWidget." << endl;
    return false;
    }

  vtkRenderWindow* const render_window =
    widget->GetRenderWindow();

  if(!render_window)
    {
    output << "ERROR: Could not locate the Render Window." << endl;
    return false;
    }

  bool ret = pqCoreTestUtility::CompareImage(render_window, referenceImage, 
    threshold, output, tempDirectory);
  renModule->render();
  return ret;
}
  
int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  pqApplicationCore appCore(argc, argv);
  MainWindow window;
  window.resize(200, 150);
  window.show();
  return app.exec();
}


