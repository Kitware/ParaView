// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqXRInterfaceDockPanel.h"
#include "ui_pqXRInterfaceDockPanel.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqPVApplicationCore.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqXRInterfaceControls.h"
#include "vtkNew.h"
#include "vtkPVRenderView.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXRInterfaceCollaborationClient.h"
#include "vtkPVXRInterfaceHelper.h"
#include "vtkPVXRInterfaceWidgets.h"
#include "vtkRenderViewBase.h"
#include "vtkSMRenderViewProxy.h"

#include <sstream>

//-----------------------------------------------------------------------------
struct pqXRInterfaceDockPanel::pqInternals
{
  Ui::pqXRInterfaceDockPanel Ui;
  vtkNew<vtkPVXRInterfaceHelper> Helper;
  pqXRInterfaceControls* XRInterfaceControls = nullptr;
  bool XREnabled = false;
  bool Attached = false;
};

//-----------------------------------------------------------------------------
pqXRInterfaceDockPanel::pqXRInterfaceDockPanel(
  const QString& title, QWidget* parent, Qt::WindowFlags flag)
  : Superclass(title, parent, flag)
  , Internals(new pqXRInterfaceDockPanel::pqInternals())
{
  this->constructor();
}

//-----------------------------------------------------------------------------
pqXRInterfaceDockPanel::pqXRInterfaceDockPanel(QWidget* parent, Qt::WindowFlags flags)
  : Superclass(parent, flags)
  , Internals(new pqXRInterfaceDockPanel::pqInternals())
{
  this->constructor();
}

//-----------------------------------------------------------------------------
pqXRInterfaceDockPanel::~pqXRInterfaceDockPanel() = default;

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::constructor()
{
  this->Internals->XRInterfaceControls = new pqXRInterfaceControls(this->Internals->Helper, this);
  this->Internals->Helper->SetXRInterfaceControls(this->Internals->XRInterfaceControls);

  this->setWindowTitle("XRInterface");
  QWidget* t_widget = new QWidget(this);
  this->Internals->Ui.setupUi(t_widget);
  this->setWidget(t_widget);

  this->Internals->Ui.showXRViewButton->setEnabled(false);

  QObject::connect(this->Internals->Ui.sendToXRButton, &QPushButton::clicked,
    std::bind(&pqXRInterfaceDockPanel::sendToXRInterface, this));
  QObject::connect(this->Internals->Ui.attachToCurrentViewButton, &QPushButton::clicked,
    std::bind(&pqXRInterfaceDockPanel::attachToCurrentView, this));
  QObject::connect(this->Internals->Ui.showXRViewButton, &QPushButton::clicked,
    std::bind(&pqXRInterfaceDockPanel::showXRView, this));
  connect(this->Internals->Ui.exportLocationsAsSkyboxesButton, SIGNAL(clicked()), this,
    SLOT(exportLocationsAsSkyboxes()));
  connect(this->Internals->Ui.exportLocationsAsViewButton, SIGNAL(clicked()), this,
    SLOT(exportLocationsAsView()));

  QObject::connect(this->Internals->Ui.multisamples, &QCheckBox::stateChanged,
    [&](int state) { this->Internals->Helper->SetMultiSample(state == Qt::Checked); });
  QObject::connect(this->Internals->Ui.baseStationVisibility, &QCheckBox::stateChanged,
    [&](int state) { this->Internals->Helper->SetBaseStationVisibility(state == Qt::Checked); });

  connect(pqApplicationCore::instance(), SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)),
    this, SLOT(loadState(vtkPVXMLElement*, vtkSMProxyLocator*)));

  connect(pqApplicationCore::instance(), SIGNAL(stateSaved(vtkPVXMLElement*)), this,
    SLOT(saveState(vtkPVXMLElement*)));

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(preViewRemoved(pqView*)), this, SLOT(onViewRemoved(pqView*)));

  QObject::connect(
    QCoreApplication::instance(), SIGNAL(lastWindowClosed()), this, SLOT(prepareForQuit()));

  QObject::connect(pqPVApplicationCore::instance()->animationManager(),
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::beginPlay), this,
    &pqXRInterfaceDockPanel::beginPlay);
  QObject::connect(pqPVApplicationCore::instance()->animationManager(),
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::endPlay), this,
    &pqXRInterfaceDockPanel::endPlay);

  if (this->Internals->Helper->GetCollaborationClient()->SupportsCollaboration())
  {
    this->Internals->Ui.cConnectButton->setEnabled(false);
    connect(
      this->Internals->Ui.cConnectButton, SIGNAL(clicked()), this, SLOT(collaborationConnect()));
  }
  else
  {
    // hide widgets
    this->Internals->Ui.cConnectButton->hide();
    this->Internals->Ui.cSessionLabel->hide();
    this->Internals->Ui.cSessionValue->hide();
    this->Internals->Ui.cNameLabel->hide();
    this->Internals->Ui.cNameValue->hide();
    this->Internals->Ui.cPortLabel->hide();
    this->Internals->Ui.cPortValue->hide();
    this->Internals->Ui.cServerLabel->hide();
    this->Internals->Ui.cServerValue->hide();
    this->Internals->Ui.cHeader->hide();
    this->Internals->Ui.outputWindow->hide();
  }

  QObject::connect(this->Internals->Ui.chooseBackendCombo,
    QOverload<int>::of(&QComboBox::currentIndexChanged), this,
    &pqXRInterfaceDockPanel::xrBackendChanged);

#if XRINTERFACE_HAS_OPENVR_SUPPORT
  // No need for tr()
  this->Internals->Ui.chooseBackendCombo->addItem(
    "OpenVR", QVariant(pqXRInterfaceDockPanel::XR_BACKEND_OPENVR));
#endif

#if XRINTERFACE_HAS_OPENXR_SUPPORT
  // No need for tr()
  this->Internals->Ui.chooseBackendCombo->addItem(
    "OpenXR", QVariant(pqXRInterfaceDockPanel::XR_BACKEND_OPENXR));

#if XRINTERFACE_HAS_OPENXRREMOTING_SUPPORT
  this->Internals->Ui.useOpenxrRemoting->setVisible(true);
  QObject::connect(this->Internals->Ui.useOpenxrRemoting, &QCheckBox::stateChanged, [&](int state) {
    this->Internals->Helper->SetUseOpenXRRemoting(state == Qt::Checked);
    this->Internals->Ui.remotingAddress->setVisible(state);
    this->Internals->Ui.remoteAddress->setVisible(state);
    this->Internals->Ui.runtimeVersionLabel->setText(
      this->Internals->Helper->GetOpenXRRuntimeVersionString().c_str());
  });
  QObject::connect(this->Internals->Ui.remoteAddress, &QLineEdit::textChanged,
    [&](QString text) { this->Internals->Helper->SetRemotingAddress(text.toStdString()); });
#else
  this->Internals->Ui.useOpenxrRemoting->setVisible(false);
#endif

#endif

  if (this->Internals->Ui.chooseBackendCombo->count() == 0)
  {
    this->Internals->Ui.chooseBackendCombo->addItem(
      tr("None"), QVariant(pqXRInterfaceDockPanel::XR_BACKEND_NONE));
  }
  if (this->Internals->Ui.chooseBackendCombo->count() == 1)
  {
    this->Internals->Ui.chooseBackendCombo->setEnabled(false);
  }
  this->Internals->Ui.chooseBackendCombo->setCurrentIndex(0);

// hide/show widgets based on Imago support
#if XRINTERFACE_HAS_IMAGO_SUPPORT
  QObject::connect(this->Internals->Ui.imagoLoginButton, &QPushButton::clicked, [&]() {
    std::string uid = this->Internals->Ui.imagoUserValue->text().toUtf8().toStdString();
    std::string pw = this->Internals->Ui.imagoPasswordValue->text().toUtf8().toStdString();
    if (this->Internals->Helper->GetWidgets()->LoginToImago(uid, pw))
    {
      // set the background of the login button to light green
      // to indicate success
      this->Internals->Ui.imagoLoginButton->setStyleSheet("border:2px solid #44ff44;");

      // set the combo box values to what the context has
      // try to save previous values if we can
      std::vector<std::string> vals;
      {
        this->Internals->Helper->GetWidgets()->GetImagoImageryTypes(vals);
        std::string oldValue =
          this->Internals->Ui.imagoImageryTypeCombo->currentText().toUtf8().data();
        this->Internals->Ui.imagoImageryTypeCombo->clear();
        QStringList list;
        list << QString("Any");
        for (auto s : vals)
        {
          list << QString(s.c_str());
        }
        this->Internals->Ui.imagoImageryTypeCombo->addItems(list);
        auto idx = this->Internals->Ui.imagoImageryTypeCombo->findText(QString(oldValue.c_str()));
        this->Internals->Ui.imagoImageryTypeCombo->setCurrentIndex(idx);
        this->Internals->Helper->GetWidgets()->SetImagoImageryType(oldValue);
      }

      {
        this->Internals->Helper->GetWidgets()->GetImagoImageTypes(vals);
        std::string oldValue =
          this->Internals->Ui.imagoImageTypeCombo->currentText().toUtf8().data();
        this->Internals->Ui.imagoImageTypeCombo->clear();
        QStringList list;
        list << QString("Any");
        for (auto s : vals)
        {
          list << QString(s.c_str());
        }
        this->Internals->Ui.imagoImageTypeCombo->addItems(list);
        auto idx = this->Internals->Ui.imagoImageTypeCombo->findText(QString(oldValue.c_str()));
        this->Internals->Ui.imagoImageTypeCombo->setCurrentIndex(idx);
        this->Internals->Helper->GetWidgets()->SetImagoImageType(oldValue);
      }

      {
        this->Internals->Helper->GetWidgets()->GetImagoDatasets(vals);
        std::string oldValue = this->Internals->Ui.imagoDatasetCombo->currentText().toUtf8().data();
        this->Internals->Ui.imagoDatasetCombo->clear();
        QStringList list;
        list << QString("Any");
        for (auto s : vals)
        {
          list << QString(s.c_str());
        }
        this->Internals->Ui.imagoDatasetCombo->addItems(list);
        auto idx = this->Internals->Ui.imagoDatasetCombo->findText(QString(oldValue.c_str()));
        this->Internals->Ui.imagoDatasetCombo->setCurrentIndex(idx);
        this->Internals->Helper->GetWidgets()->SetImagoDataset(oldValue);
      }

      {
        this->Internals->Helper->GetWidgets()->GetImagoWorkspaces(vals);
        std::string oldValue =
          this->Internals->Ui.imagoWorkspaceCombo->currentText().toUtf8().data();
        this->Internals->Ui.imagoWorkspaceCombo->clear();
        QStringList list;
        list << QString("Any");
        for (auto s : vals)
        {
          list << QString(s.c_str());
        }
        this->Internals->Ui.imagoWorkspaceCombo->addItems(list);
        auto idx = this->Internals->Ui.imagoWorkspaceCombo->findText(QString(oldValue.c_str()));
        this->Internals->Ui.imagoWorkspaceCombo->setCurrentIndex(idx);
        this->Internals->Helper->GetWidgets()->SetImagoWorkspace(oldValue);
      }
    }
    else
    {
      this->Internals->Ui.imagoLoginButton->setStyleSheet("border:2px solid #ff4444;");
    }
  });

  QObject::connect(this->Internals->Ui.imagoWorkspaceCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (text.length())
      {
        this->Internals->Helper->GetWidgets()->SetImagoWorkspace(text.toUtf8().toStdString());
      }
    });
  QObject::connect(this->Internals->Ui.imagoDatasetCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (text.length())
      {
        this->Internals->Helper->GetWidgets()->SetImagoDataset(text.toUtf8().toStdString());
      }
    });
  QObject::connect(this->Internals->Ui.imagoImageryTypeCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (text.length())
      {
        this->Internals->Helper->GetWidgets()->SetImagoImageryType(text.toUtf8().toStdString());
      }
    });
  QObject::connect(this->Internals->Ui.imagoImageTypeCombo,
    QOverload<const QString&>::of(&QComboBox::activated), [=](QString const& text) {
      if (text.length())
      {
        this->Internals->Helper->GetWidgets()->SetImagoImageType(text.toUtf8().toStdString());
      }
    });
#else
  this->Internals->Ui.imagoLine->hide();
  this->Internals->Ui.imagoGroupBox->hide();
#endif
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::sendToXRInterface()
{
  if (!this->Internals->XREnabled)
  {
    pqView* view = pqActiveObjects::instance().activeView();

    if (view)
    {
      // XXX setText should be avoid (here and below) with a cleaner
      // UI design: https://gitlab.kitware.com/vtk/vtk/-/issues/18302
      vtkSMViewProxy* smview = view->getViewProxy();
      this->Internals->Ui.cConnectButton->setText(tr("Connect"));
      this->Internals->Ui.attachToCurrentViewButton->setEnabled(false);
      this->Internals->Ui.showXRViewButton->setEnabled(true);
      this->Internals->Ui.sendToXRButton->setText(tr("Quit XR"));
      this->Internals->XREnabled = true;
      this->Internals->Ui.cConnectButton->setEnabled(true);
      this->Internals->Helper->SendToXR(smview);
      this->Internals->Ui.cConnectButton->setEnabled(false);
      this->Internals->Ui.cConnectButton->setText(tr("Connect"));
      this->Internals->Ui.sendToXRButton->setText(tr("Send to XR"));
      this->Internals->XREnabled = false;
      this->Internals->Ui.attachToCurrentViewButton->setEnabled(true);
    }
  }
  else
  {
    this->Internals->Ui.showXRViewButton->setEnabled(false);
    this->Internals->Ui.cConnectButton->setEnabled(false);
    this->Internals->Helper->Quit();
  }
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::showXRView()
{
  this->Internals->Helper->ShowXRView();
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::attachToCurrentView()
{
  if (!this->Internals->Attached)
  {
    pqView* view = pqActiveObjects::instance().activeView();

    if (view)
    {
      vtkSMViewProxy* smview = view->getViewProxy();
      this->Internals->Ui.cConnectButton->setText(tr("Connect"));
      this->Internals->Ui.cConnectButton->setEnabled(true);
      this->Internals->Ui.sendToXRButton->setEnabled(false);
      this->Internals->Ui.attachToCurrentViewButton->setText(tr("Detach from View"));
      this->Internals->Attached = true;
      this->Internals->Helper->AttachToCurrentView(smview);
      this->Internals->Ui.cConnectButton->setEnabled(false);
      this->Internals->Ui.cConnectButton->setText(tr("Connect"));
      this->Internals->Ui.attachToCurrentViewButton->setText(tr("Attach to Current View"));
      this->Internals->Attached = false;
      this->Internals->Ui.sendToXRButton->setEnabled(true);
    }
  }
  else
  {
    this->Internals->Ui.cConnectButton->setEnabled(false);
    this->Internals->Helper->Quit();
  }
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::collaborationConnect()
{
  if (this->Internals->Ui.cConnectButton->text() == "Connect")
  {
    vtkPVXRInterfaceCollaborationClient* cc = this->Internals->Helper->GetCollaborationClient();
    cc->SetLogCallback(std::bind(&pqXRInterfaceDockPanel::collaborationCallback, this,
      std::placeholders::_1, std::placeholders::_2));
    cc->SetCollabHost(this->Internals->Ui.cServerValue->text().toUtf8().data());
    cc->SetCollabSession(this->Internals->Ui.cSessionValue->text().toUtf8().data());
    cc->SetCollabName(this->Internals->Ui.cNameValue->text().toUtf8().data());
    cc->SetCollabPort(this->Internals->Ui.cPortValue->text().toInt());
    if (this->Internals->Helper->CollaborationConnect())
    {
      this->Internals->Ui.cConnectButton->setText(tr("Disconnect"));
    }
  }
  else
  {
    this->Internals->Ui.cConnectButton->setText(tr("Connect"));
    this->Internals->Helper->CollaborationDisconnect();
  }
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::collaborationCallback(
  std::string const& msg, vtkLogger::Verbosity /*verbosity*/)
{
  // send message if any to text window
  if (!msg.length())
  {
    return;
  }

  this->Internals->Ui.outputWindow->appendPlainText(msg.c_str());
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::exportLocationsAsSkyboxes()
{
  pqView* view = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* smview = view->getViewProxy();

  this->Internals->Helper->ExportLocationsAsSkyboxes(smview);
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::exportLocationsAsView()
{
  pqView* view = pqActiveObjects::instance().activeView();
  vtkSMViewProxy* smview = view->getViewProxy();

  this->Internals->Helper->ExportLocationsAsView(smview);
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::defaultCropThicknessChanged(const QString& text)
{
  bool ok;
  double d = text.toDouble(&ok);
  if (ok)
  {
    this->Internals->Helper->SetDefaultCropThickness(d);
  }
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::loadState(vtkPVXMLElement* root, vtkSMProxyLocator* locator)
{
  vtkPVXMLElement* e = root->FindNestedElementByName("XRInterface");
  if (!e)
  {
    // For backwards compatibility
    // XXX should be handled through actual deprecation mechanism
    // https://gitlab.kitware.com/vtk/vtk/-/issues/18302
    e = root->FindNestedElementByName("OpenVR");
  }
  if (e)
  {
    int ms = 0;
    if (e->GetScalarAttribute("BaseStationVisibility", &ms))
    {
      this->Internals->Helper->SetBaseStationVisibility(ms);
      this->Internals->Ui.baseStationVisibility->setCheckState(ms ? Qt::Checked : Qt::Unchecked);
    }
    if (e->GetScalarAttribute("MultiSample", &ms))
    {
      this->Internals->Helper->SetMultiSample(ms);
      this->Internals->Ui.multisamples->setCheckState(ms ? Qt::Checked : Qt::Unchecked);
    }

    std::string tmp = e->GetAttributeOrEmpty("CollaborationServer");
    if (!tmp.empty())
    {
      this->Internals->Ui.cServerValue->setText(QString(tmp.c_str()));
    }
    tmp = e->GetAttributeOrEmpty("CollaborationSession");
    if (!tmp.empty())
    {
      this->Internals->Ui.cSessionValue->setText(QString(tmp.c_str()));
    }
    tmp = e->GetAttributeOrEmpty("CollaborationPort");
    if (!tmp.empty())
    {
      this->Internals->Ui.cPortValue->setText(QString(tmp.c_str()));
    }

    // imago values
    tmp = e->GetAttributeOrEmpty("ImagoUser");
    if (!tmp.empty())
    {
      this->Internals->Ui.imagoUserValue->setText(QString(tmp.c_str()));
    }
    tmp = e->GetAttributeOrEmpty("ImagoWorkspace");
    this->Internals->Ui.imagoWorkspaceCombo->clear();
    if (!tmp.empty())
    {
      QStringList list;
      list << QString(tmp.c_str());
      this->Internals->Ui.imagoWorkspaceCombo->addItems(list);
      auto idx = this->Internals->Ui.imagoWorkspaceCombo->findText(QString(tmp.c_str()));
      this->Internals->Ui.imagoWorkspaceCombo->setCurrentIndex(idx);
    }
    tmp = e->GetAttributeOrEmpty("ImagoDataset");
    this->Internals->Ui.imagoDatasetCombo->clear();
    if (!tmp.empty())
    {
      QStringList list;
      list << QString(tmp.c_str());
      this->Internals->Ui.imagoDatasetCombo->addItems(list);
      auto idx = this->Internals->Ui.imagoDatasetCombo->findText(QString(tmp.c_str()));
      this->Internals->Ui.imagoDatasetCombo->setCurrentIndex(idx);
    }
    tmp = e->GetAttributeOrEmpty("ImagoImageryType");
    this->Internals->Ui.imagoImageryTypeCombo->clear();
    if (!tmp.empty())
    {
      QStringList list;
      list << QString(tmp.c_str());
      this->Internals->Ui.imagoImageryTypeCombo->addItems(list);
      auto idx = this->Internals->Ui.imagoImageryTypeCombo->findText(QString(tmp.c_str()));
      this->Internals->Ui.imagoImageryTypeCombo->setCurrentIndex(idx);
    }
    tmp = e->GetAttributeOrEmpty("ImagoImageType");
    this->Internals->Ui.imagoImageTypeCombo->clear();
    if (!tmp.empty())
    {
      QStringList list;
      list << QString(tmp.c_str());
      this->Internals->Ui.imagoImageTypeCombo->addItems(list);
      auto idx = this->Internals->Ui.imagoImageTypeCombo->findText(QString(tmp.c_str()));
      this->Internals->Ui.imagoImageTypeCombo->setCurrentIndex(idx);
    }

    this->Internals->Helper->LoadState(e, locator);
  }
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::saveState(vtkPVXMLElement* root)
{
  vtkNew<vtkPVXMLElement> e;
  e->SetName("XRInterface");

  e->AddAttribute(
    "BaseStationVisibility", this->Internals->Helper->GetBaseStationVisibility() ? 1 : 0);
  e->AddAttribute("MultiSample", this->Internals->Helper->GetMultiSample() ? 1 : 0);

  e->AddAttribute("DefaultCropThickness", this->Internals->Helper->GetDefaultCropThickness());

  e->AddAttribute("CollaborationServer", this->Internals->Ui.cServerValue->text().toUtf8().data());
  e->AddAttribute(
    "CollaborationSession", this->Internals->Ui.cSessionValue->text().toUtf8().data());
  e->AddAttribute("CollaborationPort", this->Internals->Ui.cPortValue->text().toUtf8().data());

  e->AddAttribute("ImagoUser", this->Internals->Ui.imagoUserValue->text().toUtf8().data());
  e->AddAttribute(
    "ImagoWorkspace", this->Internals->Ui.imagoWorkspaceCombo->currentText().toUtf8().data());
  e->AddAttribute(
    "ImagoDataset", this->Internals->Ui.imagoDatasetCombo->currentText().toUtf8().data());
  e->AddAttribute(
    "ImagoImageryType", this->Internals->Ui.imagoImageryTypeCombo->currentText().toUtf8().data());
  e->AddAttribute(
    "ImagoImageType", this->Internals->Ui.imagoImageTypeCombo->currentText().toUtf8().data());

  this->Internals->Helper->SaveState(e);

  root->AddNestedElement(e.Get(), 1);
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::prepareForQuit()
{
  this->Internals->Helper->Quit();
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::beginPlay()
{
  pqAnimationManager* am = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* as = am->getActiveScene();
  QObject::connect(as, SIGNAL(animationTime(double)), this, SLOT(updateSceneTime()));
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::endPlay()
{
  pqAnimationManager* am = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* as = am->getActiveScene();
  QObject::disconnect(as, nullptr, this, nullptr);
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::updateSceneTime()
{
  this->Internals->Helper->UpdateProps();
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::onViewRemoved(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    vtkSMViewProxy* smview = view->getViewProxy();

    this->Internals->Helper->ViewRemoved(smview);
  }
}

//-----------------------------------------------------------------------------
void pqXRInterfaceDockPanel::xrBackendChanged(int index)
{
  switch (this->Internals->Ui.chooseBackendCombo->itemData(index).toInt())
  {
    case pqXRInterfaceDockPanel::XR_BACKEND_OPENVR:
      this->Internals->Ui.runtimeVersionLabel->hide();
      this->Internals->Helper->SetUseOpenXR(false);
      break;
    case pqXRInterfaceDockPanel::XR_BACKEND_OPENXR:
      this->Internals->Helper->SetUseOpenXR(true);
      this->Internals->Ui.runtimeVersionLabel->setText(
        this->Internals->Helper->GetOpenXRRuntimeVersionString().c_str());
      this->Internals->Ui.runtimeVersionLabel->show();
      break;
    case pqXRInterfaceDockPanel::XR_BACKEND_NONE:
    default:
      this->Internals->Ui.runtimeVersionLabel->hide();
      break;
  }
}
