
// A Test of a very simple app based on pqCore

#include <QMainWindow>
#include <QApplication>
#include <QPointer>

#include <QVTKWidget.h>

#include "pqMain.h"
#include "pqProcessModuleGUIHelper.h"
#include "pqServer.h"
#include "pqRenderModule.h"
#include "pqApplicationCore.h"
#include "pqPipelineBuilder.h"
#include "pqUndoStack.h"
#include "pqTestUtility.h"


// our main window
class MainWindow : public QMainWindow
{
public:
  MainWindow()
  {
    // automatically make a server connection
    pqApplicationCore* core = pqApplicationCore::instance();
    pqPipelineBuilder* pb = core->getPipelineBuilder();
    pqServer* server = core->createServer(pqServerResource("builtin:"));
    
    // create a graphics window and put it in our main window
    this->RenderModule = pb->createWindow(server);
    this->setCentralWidget(this->RenderModule->getWidget());
    
    // hmm... (we get problems if we don't do this)
    core->getUndoStack()->setActiveServer(server);

    // create source and elevation filter
    pqPipelineSource* source;
    pqPipelineSource* elevation;
    source = pb->createSource("sources", "SphereSource", server, NULL);
    elevation = pb->createSource("filters", "ElevationFilter", server, NULL);
    // passing render module in for the creation of the
    // ElevationFilter only leads to problems since the pipeline isn't connected
    // yet

    // connect the sphere and elevation
    pb->addConnection(source, elevation);

    // put the elevation in the window
    pqPipelineDisplay* disp;
    disp = pb->createDisplayProxy(elevation, this->RenderModule);

    // zoom to sphere
    this->RenderModule->resetCamera();
    // make sure we update
    this->RenderModule->render();
  }
  
  QPointer<pqRenderModule> RenderModule;

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
    pqRenderModule* renModule = Win->RenderModule;

    if (!renModule)
      {
      output << "ERROR: Could not locate the render module." << endl;
      return false;
      }

    vtkRenderWindow* const render_window =
      renModule->getWidget()->GetRenderWindow();

    if(!render_window)
      {
      output << "ERROR: Could not locate the Render Window." << endl;
      return false;
      }

    bool ret = pqTestUtility::CompareImage(render_window, referenceImage, 
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


