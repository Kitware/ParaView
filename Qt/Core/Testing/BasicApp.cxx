
// A Test of a very simple app based on pqCore

#include <QMainWindow>
#include <QApplication>
#include <QPointer>

#include <QVTKWidget.h>

#include "pqMain.h"
#include "pqProcessModuleGUIHelper.h"
#include "pqServer.h"
#include "pqRenderView.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqCoreTestUtility.h"


// our main window
class MainWindow : public QMainWindow
{
public:
  MainWindow()
  {
    // automatically make a server connection
    pqApplicationCore* core = pqApplicationCore::instance();
    pqObjectBuilder* ob = core->getObjectBuilder();
    pqServer* server = core->createServer(pqServerResource("builtin:"));
    
    // create a graphics window and put it in our main window
    this->RenderView = qobject_cast<pqRenderView*>(
      ob->createView(pqRenderView::renderViewType(), server));
    this->setCentralWidget(this->RenderView->getWidget());
    
    // create source and elevation filter
    pqPipelineSource* source;
    pqPipelineSource* elevation;

    source = ob->createSource("sources", "SphereSource", server);
    elevation = ob->createFilter("filters", "ElevationFilter", source);
    
    // put the elevation in the window
    ob->createDataRepresentation(elevation, this->RenderView);

    // zoom to sphere
    this->RenderView->resetCamera();
    // make sure we update
    this->RenderView->render();
  }
  
  QPointer<pqRenderView> RenderView;

};


// our gui helper makes our MainWindow
class GUIHelper : public pqProcessModuleGUIHelper
{
public:
  static GUIHelper* New()
  {
    return new GUIHelper;
  }
  QWidget* CreateMainWindow()
  {
    Win = new MainWindow;
    return Win;
  }
  bool compareView(const QString& referenceImage, double threshold,
                   ostream& output, const QString& tempDirectory)
  {
    pqRenderView* renModule = Win->RenderView;

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

  QPointer<MainWindow> Win;
};


int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  return pqMain::Run(app, GUIHelper::New());
}


