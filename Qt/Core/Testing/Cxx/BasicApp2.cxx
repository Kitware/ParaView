// This is a simple application for testing new view framework.
// This is for developement purposes only, must be removed once the new views
// are developed.

#include <QMainWindow>
#include <QApplication>
#include <QPointer>

#include <QVTKWidget.h>

#include "pqMain.h"
#include "pqProcessModuleGUIHelper.h"
#include "pqServer.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqCoreTestUtility.h"
#include "pqSMAdaptor.h"

#include "vtkSmartPointer.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkProcessModule.h"

#include "vtkSMStateVersionController.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"

class vtkRVProxy : public vtkPVRenderViewProxy
{
public:
  static vtkRVProxy* New()
    { return new vtkRVProxy(); }
   
  // Implemeting API from vtkPVRenderViewProxy.
  virtual void Render()
    {
    this->View->InteractiveRender();
    }
  virtual void EventuallyRender()
    {
    this->View->StillRender();
    }

  virtual vtkRenderWindow* GetRenderWindow()
    {
    return this->View->GetRenderWindow();
    }
  
  vtkSMRenderViewProxy* View;
protected:
  vtkRVProxy()
    {
    this->View = 0;
    }
};

// our main window
class MainWindow : public QMainWindow
{
public:
  MainWindow()
  {
    this->RVProxy.TakeReference(vtkRVProxy::New());
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkIdType cid = pm->ConnectToRemote("localhost", 11111);
    bool local = false;
    if (!cid)
      {
      cid = pm->ConnectToSelf();
      local = true;
      }

    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    if (local)
      {
      this->RenderView.TakeReference(vtkSMRenderViewProxy::SafeDownCast(
          pxm->NewProxy("views", "RenderView")));
      }
    else
      {
      this->RenderView.TakeReference(vtkSMRenderViewProxy::SafeDownCast(
          pxm->NewProxy("views", "IceTDesktopRenderView")));
      pqSMAdaptor::setElementProperty(this->RenderView->GetProperty("CompositeThreshold"), 0);
      }
      
    this->RenderView->SetConnectionID(cid);
    this->RenderView->UpdateVTKObjects();

    this->RVProxy->View = this->RenderView;

    QVTKWidget * view = new QVTKWidget();
    view->SetRenderWindow(this->RenderView->GetRenderWindow());

    this->RenderView->GetInteractor()->SetPVRenderView(this->RVProxy);
    this->setCentralWidget(view);
    this->createDefaultInteractors();

    this->RenderView->StillRender();
    this->RenderView->ResetCamera();

    // Create data pipeline.

    vtkSMProxy* proxy = pxm->NewProxy("sources", "SphereSource");
    proxy->SetConnectionID(cid);
    pqSMAdaptor::setElementProperty(proxy->GetProperty("ThetaResolution"),10);
    pqSMAdaptor::setElementProperty(proxy->GetProperty("PhiResolution"),10);
    proxy->UpdateVTKObjects();

    vtkSMProxy* repr = pxm->NewProxy("representations", "SurfaceRepresentation");
    repr->SetConnectionID(cid);
    pqSMAdaptor::setProxyProperty(repr->GetProperty("Input"), proxy);
    pqSMAdaptor::setElementProperty(repr->GetProperty("Opacity"), 0.4);
    repr->UpdateVTKObjects();

    pqSMAdaptor::addProxyProperty(this->RenderView->GetProperty("Representations"),
      repr);
    this->RenderView->UpdateVTKObjects();

    proxy->Delete();
    repr->Delete();
  }

  void createDefaultInteractors()
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkIdType cid = this->RenderView->GetConnectionID();

    vtkSMProxy* interactorStyle = 
      pxm->NewProxy("interactorstyles", "InteractorStyle");
    interactorStyle->SetConnectionID(cid);
    interactorStyle->SetServers(vtkProcessModule::CLIENT);
    interactorStyle->UpdateVTKObjects();

    // Set interactor style on the render module.
    pqSMAdaptor::setProxyProperty(
      this->RenderView->GetProperty("InteractorStyle"), interactorStyle);
    this->RenderView->UpdateVTKObjects();
    interactorStyle->Delete();

    vtkSMProperty *styleManips = 
      interactorStyle->GetProperty("CameraManipulators");

    // Create and register manipulators, then add to interactor style

    // LeftButton -- Rotate
    vtkSMProxy *manip = pxm->NewProxy("cameramanipulators", "TrackballRotate");
    manip->SetConnectionID(cid);
    manip->SetServers(vtkProcessModule::CLIENT);
    pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), 1);
    pqSMAdaptor::addProxyProperty(styleManips, manip);
    manip->UpdateVTKObjects();
    manip->Delete();

    // RightButton -- Zoom
    manip = pxm->NewProxy("cameramanipulators", "TrackballZoom");
    manip->SetConnectionID(cid);
    manip->SetServers(vtkProcessModule::CLIENT);
    pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), 3);
    pqSMAdaptor::addProxyProperty(styleManips, manip);
    manip->UpdateVTKObjects();
    manip->Delete();

    interactorStyle->UpdateVTKObjects();
    }
  
  vtkSmartPointer<vtkSMRenderViewProxy> RenderView;
  vtkSmartPointer<vtkRVProxy> RVProxy;
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

  QPointer<MainWindow> Win;
};


int main(int argc, char** argv)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName("/home/utkarsh/Kitware/ParaView3/ParaView3Bin/b.pvsm");
  parser->Parse();

  vtkSMStateVersionController* controller = vtkSMStateVersionController::New();
  controller->Process(parser->GetRootElement()->GetNestedElement(0));
  ofstream ofp("/tmp/out.xml");
  parser->GetRootElement()->PrintXML(ofp, vtkIndent());

  //QApplication app(argc, argv);
  //return pqMain::Run(app, GUIHelper::New());
}


