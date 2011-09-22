// A Test of a very simple app based on pqCore
#include "FilteredPipelineBrowserApp.h"

#include <QTimer>
#include <QApplication>
#include <QVBoxLayout>
#include <QDebug>
#include <QStringList>

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
#include "pqPipelineAnnotationFilterModel.h"
#include "pqServer.h"
#include "pqStandardViewModules.h"
#include "vtkProcessModule.h"

MainPipelineWindow::MainPipelineWindow()
{
  // Init ParaView
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* ob = core->getObjectBuilder();
  pqInterfaceTracker* pgm = pqApplicationCore::instance()->interfaceTracker();
  pgm->addInterface(new pqStandardViewModules(pgm));

  // Set Filter/Annotation list
  this->FilterNames.append("No filtering");
  this->FilterNames.append("Filter 1");
  this->FilterNames.append("Filter 2");
  this->FilterNames.append("Filter 3");
  this->FilterNames.append("Filter 4");

  // Set Filter selector
  this->FilterSelector = new QComboBox();
  this->FilterSelector->addItems(this->FilterNames);
  QObject::connect( this->FilterSelector, SIGNAL(currentIndexChanged(int)),
                    this, SLOT(updateSelectedFilter(int)),
                    Qt::QueuedConnection);

  // Set Pipeline Widget
  this->PipelineWidget = new pqPipelineBrowserWidget(NULL);

  // Create server only after a pipeline browser get created...
  pqServer* server = ob->createServer(pqServerResource("builtin:"));

  // Init and layout the UI
  QWidget *container = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(this->FilterSelector);
  layout->addWidget(this->PipelineWidget);
  container->setLayout(layout);
  this->setCentralWidget(container);

  // Create a complex pipeline with different annotations
  for(int i=1; i<this->FilterNames.size(); i++)
    {
    createPipelineWithAnnotation(server, this->FilterNames.at(i));
    }

  QTimer::singleShot(100, this, SLOT(processTest()));
}

//-----------------------------------------------------------------------------
void MainPipelineWindow::updateSelectedFilter(int filterIndex)
{
  if(filterIndex == 0)
    {
    this->PipelineWidget->disableAnnotationFilter();
    }
  else
    {
    this->PipelineWidget->enableAnnotationFilter(
        this->FilterNames.at(filterIndex));
    }
}

//-----------------------------------------------------------------------------
void MainPipelineWindow::processTest()
{
  if (pqOptions* const options = pqApplicationCore::instance()->getOptions())
    {
    bool test_succeeded = true;

    // ---- Do the testing here ----

    // ---- Do the testing here ----

    if (options->GetExitAppWhenTestsDone())
      {
      QApplication::instance()->exit(test_succeeded ? 0 : 1);
      }
    }
}
//-----------------------------------------------------------------------------
void MainPipelineWindow::createPipelineWithAnnotation(pqServer* server, const QString& annotationKey)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* ob = core->getObjectBuilder();

  // create source and elevation filter
  pqPipelineSource* source;
  pqPipelineSource* elevation;

  source = ob->createSource("sources", "SphereSource", server);
  // updating source so that when elevation filter is created, the defaults
  // are setup correctly using the correct data bounds etc.
  vtkSMSourceProxy::SafeDownCast(source->getProxy())->UpdatePipeline();

  elevation = ob->createFilter("filters", "ElevationFilter", source);

  // Add annotation
  source->getProxy()->SetAnnotation(annotationKey.toAscii().data(),"X");
  elevation->getProxy()->SetAnnotation(annotationKey.toAscii().data(),"X");

  // Add tooltip annotation
  source->getProxy()->SetAnnotation("tooltip", annotationKey.toAscii().data());
  elevation->getProxy()->SetAnnotation("tooltip", annotationKey.toAscii().data());

  // Rename source ???
}
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  pqApplicationCore appCore(argc, argv);
  MainPipelineWindow window;
  window.resize(200, 150);
  window.show();
  return app.exec();
}
