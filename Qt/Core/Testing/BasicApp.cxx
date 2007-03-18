
// A Test of a very simple app based on pqCore

#include <QMainWindow>
#include <QApplication>
#include <QPointer>

#include <QVTKWidget.h>

#include "pqMain.h"
#include "pqProcessModuleGUIHelper.h"
#include "pqServer.h"
#include "pqRenderViewModule.h"
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
    this->RenderModule = qobject_cast<pqRenderViewModule*>(
      ob->createView(pqRenderViewModule::renderViewType(), server));
    this->setCentralWidget(this->RenderModule->getWidget());
    
    // create source and elevation filter
    pqPipelineSource* source;
    pqPipelineSource* elevation;

    source = ob->createSource("sources", "SphereSource", server);
    elevation = ob->createFilter("filters", "ElevationFilter", source);
    
    // put the elevation in the window
    ob->createDataDisplay(elevation, this->RenderModule);

    // zoom to sphere
    this->RenderModule->resetCamera();
    // make sure we update
    this->RenderModule->render();
  }
  
  QPointer<pqRenderViewModule> RenderModule;

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
    pqRenderViewModule* renModule = Win->RenderModule;

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


