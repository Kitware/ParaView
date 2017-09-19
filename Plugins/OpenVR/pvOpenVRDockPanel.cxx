#include "pvOpenVRDockPanel.h"
#include "ui_pvOpenVRDockPanel.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqPVApplicationCore.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"

#include "vtkSMRenderViewProxy.h"

#include "vtkNew.h"
#include "vtkPVOpenVRHelper.h"
#include "vtkPVRenderView.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderViewBase.h"

#include <sstream>

void pvOpenVRDockPanel::constructor()
{
  this->Helper = vtkPVOpenVRHelper::New();
  this->setWindowTitle("OpenVR");
  QWidget* t_widget = new QWidget(this);
  Ui::pvOpenVRDockPanel ui;
  ui.setupUi(t_widget);
  this->setWidget(t_widget);

  connect(ui.sendToOpenVRButton, SIGNAL(clicked()), this, SLOT(sendToOpenVR()));

  // connect(
  //   &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
  //   SLOT(setActiveView(pqView*)));

  connect(pqApplicationCore::instance(), SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)),
    this, SLOT(loadState(vtkPVXMLElement*, vtkSMProxyLocator*)));

  connect(pqApplicationCore::instance(), SIGNAL(stateSaved(vtkPVXMLElement*)), this,
    SLOT(saveState(vtkPVXMLElement*)));

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(preViewAdded(pqView*)), this, SLOT(onViewAdded(pqView*)));
  QObject::connect(smmodel, SIGNAL(preViewRemoved(pqView*)), this, SLOT(onViewRemoved(pqView*)));

  QObject::connect(
    QCoreApplication::instance(), SIGNAL(lastWindowClosed()), this, SLOT(prepareForQuit()));

  QObject::connect(pqPVApplicationCore::instance()->animationManager(), SIGNAL(beginPlay()), this,
    SLOT(beginPlay()));
  QObject::connect(
    pqPVApplicationCore::instance()->animationManager(), SIGNAL(endPlay()), this, SLOT(endPlay()));
}

void pvOpenVRDockPanel::sendToOpenVR()
{
  pqView* view = pqActiveObjects::instance().activeView();

  vtkSMViewProxy* smview = view->getViewProxy();

  this->Helper->SendToOpenVR(smview);
}

pvOpenVRDockPanel::~pvOpenVRDockPanel()
{
  this->Helper->Delete();
}

//-----------------------------------------------------------------------------
void pvOpenVRDockPanel::setActiveView(pqView* /*view*/)
{
  //  pqRenderView* rview = qobject_cast<pqRenderView*>(view);

  //  vtkSMRenderViewProxy* smview = rview->getRenderViewProxy();

  //  vtkPVRenderView* cview = vtkPVRenderView::SafeDownCast(smview->GetClientSideView());

  //  if (cview)
  //  {
  // vtkNew<vtkPVOpenVRHelper> helper;
  // helper->SendToOpenVR(cview, cview->GetRenderView()->GetRenderer());
  //  }
}

void pvOpenVRDockPanel::loadState(vtkPVXMLElement* root, vtkSMProxyLocator* /* locator */)
{
  vtkPVXMLElement* e = root->FindNestedElementByName("OpenVR");
  if (e)
  {
    std::string poses = e->GetAttributeOrEmpty("CameraPoses");
    this->Helper->SetNextXML(poses);
  }
}

void pvOpenVRDockPanel::saveState(vtkPVXMLElement* root)
{
  vtkNew<vtkPVXMLElement> e;
  e->SetName("OpenVR");

  std::string xmls = this->Helper->GetNextXML();
  e->AddAttribute("CameraPoses", xmls.c_str());
  root->AddNestedElement(e.Get(), 1);
}

void pvOpenVRDockPanel::prepareForQuit()
{
  this->Helper->Quit();
}

void pvOpenVRDockPanel::beginPlay()
{
  pqAnimationManager* am = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* as = am->getActiveScene();
  QObject::connect(as, SIGNAL(animationTime(double)), this, SLOT(updateSceneTime()));
}

void pvOpenVRDockPanel::endPlay()
{
  pqAnimationManager* am = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* as = am->getActiveScene();
  QObject::disconnect(as, 0, this, 0);
}

void pvOpenVRDockPanel::updateSceneTime()
{
  this->Helper->UpdateProps();
}

//-----------------------------------------------------------------------------
void pvOpenVRDockPanel::onViewAdded(pqView*)
{
  // nothing right now
}

//-----------------------------------------------------------------------------
void pvOpenVRDockPanel::onViewRemoved(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    vtkSMViewProxy* smview = view->getViewProxy();

    this->Helper->ViewRemoved(smview);
  }
}
