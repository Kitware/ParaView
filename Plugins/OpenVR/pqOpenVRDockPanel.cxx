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
#include "vtkPVOpenVRWidgets.h"
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

  this->Internals->showVRViewButton->setEnabled(false);

  // connect(this->Internals->sendToOpenVRButton, SIGNAL(clicked()), this, SLOT(sendToOpenVR()));
  QObject::connect(this->Internals->sendToOpenVRButton, &QPushButton::clicked,
    std::bind(&pqOpenVRDockPanel::sendToOpenVR, this));
  QObject::connect(this->Internals->attachToCurrentViewButton, &QPushButton::clicked,
    std::bind(&pqOpenVRDockPanel::attachToCurrentView, this));
  QObject::connect(this->Internals->showVRViewButton, &QPushButton::clicked,
    std::bind(&pqOpenVRDockPanel::showVRView, this));
  connect(this->Internals->exportLocationsAsSkyboxesButton, SIGNAL(clicked()), this,
    SLOT(exportLocationsAsSkyboxes()));
  connect(this->Internals->exportLocationsAsViewButton, SIGNAL(clicked()), this,
    SLOT(exportLocationsAsView()));

  connect(this->Internals->editableField, SIGNAL(textChanged(const QString&)), this,
    SLOT(editableFieldChanged(const QString&)));
  connect(this->Internals->fieldValues, SIGNAL(textChanged(const QString&)), this,
    SLOT(fieldValuesChanged(const QString&)));

  QObject::connect(this->Internals->multisamples, &QCheckBox::stateChanged,
    [&](int state) { this->Helper->SetMultiSample(state == Qt::Checked); });
  QObject::connect(this->Internals->baseStationVisibility, &QCheckBox::stateChanged,
    [&](int state) { this->Helper->SetBaseStationVisibility(state == Qt::Checked); });

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

// hide/show widgets based on Imago support
#ifdef OPENVR_HAS_IMAGO_SUPPORT
  QObject::connect(this->Internals->imagoLoginButton, &QPushButton::clicked, [&]() {
    std::string uid = this->Internals->imagoUserValue->text().toLocal8Bit().constData();
    std::string pw = this->Internals->imagoPasswordValue->text().toLocal8Bit().constData();
    if (this->Helper->GetWidgets()->LoginToImago(uid, pw))
    {
      // set the background of the login button to light green
      // to indicate success
      this->Internals->imagoLoginButton->setStyleSheet("border:2px solid #44ff44;");

      // set the combo box values to what the context has
      // try to save previous values if we can
      std::vector<std::string> vals;
      {
        this->Helper->GetWidgets()->GetImagoImageryTypes(vals);
        std::string oldValue =
          this->Internals->imagoImageryTypeCombo->currentText().toLatin1().data();
        this->Internals->imagoImageryTypeCombo->clear();
        QStringList list;
        list << QString("Any");
        for (auto s : vals)
        {
          list << QString(s.c_str());
        }
        this->Internals->imagoImageryTypeCombo->addItems(list);
        auto idx = this->Internals->imagoImageryTypeCombo->findText(QString(oldValue.c_str()));
        this->Internals->imagoImageryTypeCombo->setCurrentIndex(idx);
        this->Helper->GetWidgets()->SetImagoImageryType(oldValue);
      }

      {
        this->Helper->GetWidgets()->GetImagoImageTypes(vals);
        std::string oldValue =
          this->Internals->imagoImageTypeCombo->currentText().toLatin1().data();
        this->Internals->imagoImageTypeCombo->clear();
        QStringList list;
        list << QString("Any");
        for (auto s : vals)
        {
          list << QString(s.c_str());
        }
        this->Internals->imagoImageTypeCombo->addItems(list);
        auto idx = this->Internals->imagoImageTypeCombo->findText(QString(oldValue.c_str()));
        this->Internals->imagoImageTypeCombo->setCurrentIndex(idx);
        this->Helper->GetWidgets()->SetImagoImageType(oldValue);
      }

      {
        this->Helper->GetWidgets()->GetImagoDatasets(vals);
        std::string oldValue = this->Internals->imagoDatasetCombo->currentText().toLatin1().data();
        this->Internals->imagoDatasetCombo->clear();
        QStringList list;
        list << QString("Any");
        for (auto s : vals)
        {
          list << QString(s.c_str());
        }
        this->Internals->imagoDatasetCombo->addItems(list);
        auto idx = this->Internals->imagoDatasetCombo->findText(QString(oldValue.c_str()));
        this->Internals->imagoDatasetCombo->setCurrentIndex(idx);
        this->Helper->GetWidgets()->SetImagoDataset(oldValue);
      }

      {
        this->Helper->GetWidgets()->GetImagoWorkspaces(vals);
        std::string oldValue =
          this->Internals->imagoWorkspaceCombo->currentText().toLatin1().data();
        this->Internals->imagoWorkspaceCombo->clear();
        QStringList list;
        list << QString("Any");
        for (auto s : vals)
        {
          list << QString(s.c_str());
        }
        this->Internals->imagoWorkspaceCombo->addItems(list);
        auto idx = this->Internals->imagoWorkspaceCombo->findText(QString(oldValue.c_str()));
        this->Internals->imagoWorkspaceCombo->setCurrentIndex(idx);
        this->Helper->GetWidgets()->SetImagoWorkspace(oldValue);
      }
    }
    else
    {
      this->Internals->imagoLoginButton->setStyleSheet("border:2px solid #ff4444;");
    }
  });

  QObject::connect(this->Internals->imagoWorkspaceCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (text.length())
      {
        this->Helper->GetWidgets()->SetImagoWorkspace(text.toLocal8Bit().constData());
      }
    });
  QObject::connect(this->Internals->imagoDatasetCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (text.length())
      {
        this->Helper->GetWidgets()->SetImagoDataset(text.toLocal8Bit().constData());
      }
    });
  QObject::connect(this->Internals->imagoImageryTypeCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (text.length())
      {
        this->Helper->GetWidgets()->SetImagoImageryType(text.toLocal8Bit().constData());
      }
    });
  QObject::connect(this->Internals->imagoImageTypeCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (text.length())
      {
        this->Helper->GetWidgets()->SetImagoImageType(text.toLocal8Bit().constData());
      }
    });
#else
  this->Internals->imagoLine->hide();
  this->Internals->imagoGroupBox->hide();
#endif
}

void pqOpenVRDockPanel::editableFieldChanged(const QString& text)
{
  this->Helper->SetEditableField(text.toLatin1().data());
}

void pqOpenVRDockPanel::fieldValuesChanged(const QString& text)
{
  this->OpenVRControls->SetFieldValues(text.toLatin1().data());
}

void pqOpenVRDockPanel::sendToOpenVR()
{
  if (this->Internals->sendToOpenVRButton->text() == "Send to OpenVR")
  {
    pqView* view = pqActiveObjects::instance().activeView();

    if (view)
    {
      vtkSMViewProxy* smview = view->getViewProxy();
      if (this->Internals->cConnectButton->text() != "Connect")
      {
        this->Internals->cConnectButton->setText("Connect");
      }
      this->Internals->attachToCurrentViewButton->setEnabled(false);
      this->Internals->showVRViewButton->setEnabled(true);
      this->Internals->sendToOpenVRButton->setText("Quit VR");
      this->Internals->cConnectButton->setEnabled(true);
      this->Helper->SendToOpenVR(smview);
      this->Internals->cConnectButton->setEnabled(false);
      this->Internals->cConnectButton->setText("Connect");
      this->Internals->sendToOpenVRButton->setText("Send to OpenVR");
      this->Internals->attachToCurrentViewButton->setEnabled(true);
    }
  }
  else
  {
    this->Internals->showVRViewButton->setEnabled(false);
    this->Internals->cConnectButton->setEnabled(false);
    this->Helper->Quit();
  }
}

void pqOpenVRDockPanel::showVRView()
{
  this->Helper->ShowVRView();
}

void pqOpenVRDockPanel::attachToCurrentView()
{
  if (this->Internals->attachToCurrentViewButton->text() == "Attach to Current View")
  {
    pqView* view = pqActiveObjects::instance().activeView();

    if (view)
    {
      vtkSMViewProxy* smview = view->getViewProxy();
      if (this->Internals->cConnectButton->text() != "Connect")
      {
        this->Internals->cConnectButton->setText("Connect");
      }
      this->Internals->cConnectButton->setEnabled(true);
      this->Internals->sendToOpenVRButton->setEnabled(false);
      this->Internals->attachToCurrentViewButton->setText("Detach from View");
      this->Helper->AttachToCurrentView(smview);
      this->Internals->cConnectButton->setEnabled(false);
      this->Internals->cConnectButton->setText("Connect");
      this->Internals->attachToCurrentViewButton->setText("Attach to Current View");
      this->Internals->sendToOpenVRButton->setEnabled(true);
    }
  }
  else
  {
    this->Internals->cConnectButton->setEnabled(false);
    this->Helper->Quit();
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
    if (e->GetScalarAttribute("BaseStationVisibility", &ms))
    {
      this->Helper->SetBaseStationVisibility(ms);
      this->Internals->baseStationVisibility->setCheckState(ms ? Qt::Checked : Qt::Unchecked);
    }
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

    std::string tmp = e->GetAttributeOrEmpty("EditableField");
    this->Internals->editableField->setText(QString(tmp.c_str()));
    tmp = e->GetAttributeOrEmpty("FieldValues");
    this->Internals->fieldValues->setText(QString(tmp.c_str()));

    tmp = e->GetAttributeOrEmpty("CollaborationServer");
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

    // imago values
    tmp = e->GetAttributeOrEmpty("ImagoUser");
    if (tmp.size())
    {
      this->Internals->imagoUserValue->setText(QString(tmp.c_str()));
    }
    tmp = e->GetAttributeOrEmpty("ImagoWorkspace");
    this->Internals->imagoWorkspaceCombo->clear();
    if (tmp.size())
    {
      QStringList list;
      list << QString(tmp.c_str());
      this->Internals->imagoWorkspaceCombo->addItems(list);
      auto idx = this->Internals->imagoWorkspaceCombo->findText(QString(tmp.c_str()));
      this->Internals->imagoWorkspaceCombo->setCurrentIndex(idx);
    }
    tmp = e->GetAttributeOrEmpty("ImagoDataset");
    this->Internals->imagoDatasetCombo->clear();
    if (tmp.size())
    {
      QStringList list;
      list << QString(tmp.c_str());
      this->Internals->imagoDatasetCombo->addItems(list);
      auto idx = this->Internals->imagoDatasetCombo->findText(QString(tmp.c_str()));
      this->Internals->imagoDatasetCombo->setCurrentIndex(idx);
    }
    tmp = e->GetAttributeOrEmpty("ImagoImageryType");
    this->Internals->imagoImageryTypeCombo->clear();
    if (tmp.size())
    {
      QStringList list;
      list << QString(tmp.c_str());
      this->Internals->imagoImageryTypeCombo->addItems(list);
      auto idx = this->Internals->imagoImageryTypeCombo->findText(QString(tmp.c_str()));
      this->Internals->imagoImageryTypeCombo->setCurrentIndex(idx);
    }
    tmp = e->GetAttributeOrEmpty("ImagoImageType");
    this->Internals->imagoImageTypeCombo->clear();
    if (tmp.size())
    {
      QStringList list;
      list << QString(tmp.c_str());
      this->Internals->imagoImageTypeCombo->addItems(list);
      auto idx = this->Internals->imagoImageTypeCombo->findText(QString(tmp.c_str()));
      this->Internals->imagoImageTypeCombo->setCurrentIndex(idx);
    }

    this->Helper->LoadState(e, locator);
  }
}

void pqOpenVRDockPanel::saveState(vtkPVXMLElement* root)
{
  vtkNew<vtkPVXMLElement> e;
  e->SetName("OpenVR");

  e->AddAttribute("EditableField", this->Internals->editableField->text().toLatin1().data());
  e->AddAttribute("FieldValues", this->Internals->fieldValues->text().toLatin1().data());

  e->AddAttribute("BaseStationVisibility", this->Helper->GetBaseStationVisibility() ? 1 : 0);
  e->AddAttribute("MultiSample", this->Helper->GetMultiSample() ? 1 : 0);

  e->AddAttribute("DefaultCropThickness", this->Helper->GetDefaultCropThickness());

  e->AddAttribute("CollaborationServer", this->Internals->cServerValue->text().toLatin1().data());
  e->AddAttribute("CollaborationSession", this->Internals->cSessionValue->text().toLatin1().data());
  e->AddAttribute("CollaborationPort", this->Internals->cPortValue->text().toLatin1().data());

  e->AddAttribute("ImagoUser", this->Internals->imagoUserValue->text().toLatin1().data());
  e->AddAttribute(
    "ImagoWorkspace", this->Internals->imagoWorkspaceCombo->currentText().toLatin1().data());
  e->AddAttribute(
    "ImagoDataset", this->Internals->imagoDatasetCombo->currentText().toLatin1().data());
  e->AddAttribute(
    "ImagoImageryType", this->Internals->imagoImageryTypeCombo->currentText().toLatin1().data());
  e->AddAttribute(
    "ImagoImageType", this->Internals->imagoImageTypeCombo->currentText().toLatin1().data());

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
