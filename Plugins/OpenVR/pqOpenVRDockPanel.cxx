#include "pqOpenVRDockPanel.h"
#include "ui_pqOpenVRDockPanel.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqOpenVRControls.h"
#include "pqPVApplicationCore.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"

#include "vtkSMRenderViewProxy.h"

#include "vtkNew.h"
#include "vtkPVOpenVRCollaborationClient.h"
#include "vtkPVOpenVRHelper.h"
#include "vtkPVRenderView.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderViewBase.h"

#include <sstream>

class pqOpenVRDockPanel::pqInternals : public Ui::pqOpenVRDockPanel
{
};

void pqOpenVRDockPanel::constructor()
{
  this->Helper = vtkPVOpenVRHelper::New();
  this->OpenVRControls = new pqOpenVRControls(this->Helper);
  this->Helper->SetOpenVRControls(this->OpenVRControls);

  this->setWindowTitle("OpenVR");
  QWidget* t_widget = new QWidget(this);
  this->Internals = new pqInternals();
  this->Internals->setupUi(t_widget);
  this->setWidget(t_widget);

  connect(this->Internals->sendToOpenVRButton, SIGNAL(clicked()), this, SLOT(sendToOpenVR()));
  connect(this->Internals->exportLocationsAsSkyboxesButton, SIGNAL(clicked()), this,
    SLOT(exportLocationsAsSkyboxes()));
  connect(this->Internals->exportLocationsAsViewButton, SIGNAL(clicked()), this,
    SLOT(exportLocationsAsView()));
  connect(
    this->Internals->multisamples, SIGNAL(stateChanged(int)), this, SLOT(multiSampleChanged(int)));
  connect(this->Internals->cropThickness, SIGNAL(textChanged(const QString&)), this,
    SLOT(defaultCropThicknessChanged(const QString&)));

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

  if (this->Helper->GetCollaborationClient()->SupportsCollaboration())
  {
    this->Internals->cConnectButton->setEnabled(false);
    connect(this->Internals->cConnectButton, SIGNAL(clicked()), this, SLOT(collaborationConnect()));
  }
  else
  {
    // hide widgets
    this->Internals->cConnectButton->hide();
    this->Internals->cSessionLabel->hide();
    this->Internals->cSessionValue->hide();
    this->Internals->cNameLabel->hide();
    this->Internals->cNameValue->hide();
    this->Internals->cPortLabel->hide();
    this->Internals->cPortValue->hide();
    this->Internals->cServerLabel->hide();
    this->Internals->cServerValue->hide();
    this->Internals->cHeader->hide();
  }
}

void pqOpenVRDockPanel::sendToOpenVR()
{
  pqView* view = pqActiveObjects::instance().activeView();

  vtkSMViewProxy* smview = view->getViewProxy();
  this->Internals->cConnectButton->setEnabled(true);
  if (this->Internals->cConnectButton->text() != "Connect")
  {
    this->Internals->cConnectButton->setText("Connect");
  }
  this->Helper->SendToOpenVR(smview);

  if (!this->Helper->InVR())
  {
    this->Internals->cConnectButton->setEnabled(false);
  }
}

void pqOpenVRDockPanel::collaborationConnect()
{
  if (this->Internals->cConnectButton->text() == "Connect")
  {
    vtkPVOpenVRCollaborationClient* cc = this->Helper->GetCollaborationClient();
    cc->SetLogCallback(std::bind(&pqOpenVRDockPanel::collaborationCallback, this,
                         std::placeholders::_1, std::placeholders::_2),
      nullptr);
    cc->SetCollabHost(this->Internals->cServerValue->text().toLatin1().data());
    cc->SetCollabSession(this->Internals->cSessionValue->text().toLatin1().data());
    cc->SetCollabName(this->Internals->cNameValue->text().toLatin1().data());
    cc->SetCollabPort(this->Internals->cPortValue->text().toInt());
    if (this->Helper->CollaborationConnect())
    {
      this->Internals->cConnectButton->setText("Disconnect");
    }
  }
  else
  {
    this->Internals->cConnectButton->setText("Connect");
    this->Helper->CollaborationDisconnect();
  }
}

void pqOpenVRDockPanel::collaborationCallback(std::string const& msg, void*)
{
  // send message if any to text window
  if (!msg.length())
  {
    return;
  }

  this->Internals->outputWindow->appendPlainText(msg.c_str());
}

void pqOpenVRDockPanel::exportLocationsAsSkyboxes()
{
  pqView* view = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* smview = view->getViewProxy();

  this->Helper->ExportLocationsAsSkyboxes(smview);
}

void pqOpenVRDockPanel::exportLocationsAsView()
{
  pqView* view = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* smview = view->getViewProxy();

  this->Helper->ExportLocationsAsView(smview);
}

void pqOpenVRDockPanel::multiSampleChanged(int state)
{
  this->Helper->SetMultiSample(state == Qt::Checked);
}

void pqOpenVRDockPanel::defaultCropThicknessChanged(const QString& text)
{
  bool ok;
  double d = text.toDouble(&ok);
  if (ok)
  {
    this->Helper->SetDefaultCropThickness(d);
  }
}

pqOpenVRDockPanel::~pqOpenVRDockPanel()
{
  delete this->Internals;
  this->Helper->Delete();
}

//-----------------------------------------------------------------------------
void pqOpenVRDockPanel::setActiveView(pqView* /*view*/)
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

void pqOpenVRDockPanel::loadState(vtkPVXMLElement* root, vtkSMProxyLocator* locator)
{
  vtkPVXMLElement* e = root->FindNestedElementByName("OpenVR");
  if (e)
  {
    int ms = 0;
    if (e->GetScalarAttribute("MultiSample", &ms))
    {
      this->Helper->SetMultiSample(ms);
      this->Internals->multisamples->setCheckState(ms ? Qt::Checked : Qt::Unchecked);
    }
    double dcropThickness = 0;
    if (e->GetScalarAttribute("DefaultCropThickness", &dcropThickness))
    {
      this->Internals->cropThickness->setText(QString::number(dcropThickness));
    }

    std::string tmp = e->GetAttributeOrEmpty("CollaborationServer");
    if (tmp.size())
    {
      this->Internals->cServerValue->setText(QString(tmp.c_str()));
    }
    tmp = e->GetAttributeOrEmpty("CollaborationSession");
    if (tmp.size())
    {
      this->Internals->cSessionValue->setText(QString(tmp.c_str()));
    }
    tmp = e->GetAttributeOrEmpty("CollaborationPort");
    if (tmp.size())
    {
      this->Internals->cPortValue->setText(QString(tmp.c_str()));
    }
    this->Helper->LoadState(e, locator);
  }
}

void pqOpenVRDockPanel::saveState(vtkPVXMLElement* root)
{
  vtkNew<vtkPVXMLElement> e;
  e->SetName("OpenVR");

  e->AddAttribute("MultiSample", this->Helper->GetMultiSample() ? 1 : 0);

  e->AddAttribute("DefaultCropThickness", this->Helper->GetDefaultCropThickness());

  e->AddAttribute("CollaborationServer", this->Internals->cServerValue->text().toLatin1().data());
  e->AddAttribute("CollaborationSession", this->Internals->cSessionValue->text().toLatin1().data());
  e->AddAttribute("CollaborationPort", this->Internals->cPortValue->text().toLatin1().data());

  this->Helper->SaveState(e);

  root->AddNestedElement(e.Get(), 1);
}

void pqOpenVRDockPanel::prepareForQuit()
{
  this->Helper->Quit();
}

void pqOpenVRDockPanel::beginPlay()
{
  pqAnimationManager* am = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* as = am->getActiveScene();
  QObject::connect(as, SIGNAL(animationTime(double)), this, SLOT(updateSceneTime()));
}

void pqOpenVRDockPanel::endPlay()
{
  pqAnimationManager* am = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* as = am->getActiveScene();
  QObject::disconnect(as, 0, this, 0);
}

void pqOpenVRDockPanel::updateSceneTime()
{
  this->Helper->UpdateProps();
}

//-----------------------------------------------------------------------------
void pqOpenVRDockPanel::onViewAdded(pqView*)
{
  // nothing right now
}

//-----------------------------------------------------------------------------
void pqOpenVRDockPanel::onViewRemoved(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    vtkSMViewProxy* smview = view->getViewProxy();

    this->Helper->ViewRemoved(smview);
  }
}
