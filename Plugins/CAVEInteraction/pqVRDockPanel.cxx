// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqVRDockPanel.h"
#include "ui_pqVRDockPanel.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqLoadStateReaction.h"
#include "pqProxyWidget.h"
#include "pqRenderView.h"
#include "pqSaveStateReaction.h"
#include "pqServerManagerModel.h"
#include "pqVRAddConnectionDialog.h"
#include "pqVRAddStyleDialog.h"
#include "pqVRCollaborationWidget.h"
#include "pqVRConnectionManager.h"
#include "pqVRQueueHandler.h"
#include "pqView.h"

#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
#include "pqVRPNConnection.h"
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
#include "pqVRUIConnection.h"
#endif

#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMVRInteractorStyleProxy.h"
#include "vtkVRInteractorStyleFactory.h"
#include "vtkWeakPointer.h"

#include <QDebug>
#include <QListWidgetItem>
#include <QMap>
#include <QPointer>

#include <vtksys/FStream.hxx>

#include <cmath>

typedef std::map<std::string, std::string> StringMap;
typedef std::map<std::string, StringMap> StringMapMap;

class pqVRDockPanel::pqInternals : public Ui::VRDockPanel
{
public:
  QString createName(vtkSMVRInteractorStyleProxy*);

  bool IsRunning;

  pqVRCollaborationWidget* CollaborationWidget;
  vtkWeakPointer<vtkCamera> Camera;
  QMap<QString, vtkSMVRInteractorStyleProxy*> StyleNameMap;
  std::shared_ptr<StringMapMap> valuatorLookupTable;
};

//-----------------------------------------------------------------------------
void pqVRDockPanel::constructor()
{
  this->setWindowTitle("CAVE Interaction Manager");
  QWidget* container = new QWidget(this);
  this->Internals = new pqInternals();
  this->Internals->setupUi(container);
  this->setWidget(container);

  this->Internals->valuatorLookupTable = std::make_shared<StringMapMap>();

  this->Internals->IsRunning = false;
  this->updateStartStopButtonStates();

  this->Internals->stylePropertiesLabel->hide();

  this->Internals->propertyCombo->setCollapseVectors(true);

  vtkVRInteractorStyleFactory* styleFactory = vtkVRInteractorStyleFactory::GetInstance();
  styleFactory->AddObserver(
    vtkVRInteractorStyleFactory::INTERACTOR_STYLES_UPDATED, this, &pqVRDockPanel::initStyles);

  // Connections
  connect(this->Internals->addConnection, SIGNAL(clicked()), this, SLOT(addConnection()));

  connect(this->Internals->removeConnection, SIGNAL(clicked()), this, SLOT(removeConnection()));

  connect(this->Internals->editConnection, SIGNAL(clicked()), this, SLOT(editConnection()));

  connect(this->Internals->connectionsTable, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this,
    SLOT(editConnection(QListWidgetItem*)));

  connect(this->Internals->connectionsTable, SIGNAL(currentRowChanged(int)), this,
    SLOT(updateConnectionButtons(int)));

  connect(pqVRConnectionManager::instance(), SIGNAL(connectionsChanged()), this,
    SLOT(updateConnections()));

  connect(pqVRConnectionManager::instance(), SIGNAL(connectionsChanged()), this,
    SLOT(updateStartStopButtonStates()));

  // Styles
  connect(this->Internals->addStyle, SIGNAL(clicked()), this, SLOT(addStyle()));

  connect(this->Internals->removeStyle, SIGNAL(clicked()), this, SLOT(removeStyle()));

  connect(this->Internals->editStyle, SIGNAL(clicked()), this, SLOT(editStyle()));

  connect(this->Internals->stylesTable, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this,
    SLOT(editStyle(QListWidgetItem*)));

  connect(this->Internals->stylesTable, SIGNAL(currentRowChanged(int)), this,
    SLOT(updateStyleButtons(int)));

  connect(pqVRQueueHandler::instance(), SIGNAL(stylesChanged()), this, SLOT(updateStyles()));

  // Other
  connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(setActiveView(pqView*)));

  connect(this->Internals->proxyCombo, SIGNAL(currentProxyChanged(vtkSMProxy*)), this,
    SLOT(proxyChanged(vtkSMProxy*)));

  connect(this->Internals->saveState, SIGNAL(clicked()), this, SLOT(saveState()));

  connect(this->Internals->restoreState, SIGNAL(clicked()), this, SLOT(restoreState()));

  connect(this->Internals->startButton, SIGNAL(clicked()), this, SLOT(start()));

  connect(this->Internals->stopButton, SIGNAL(clicked()), this, SLOT(stop()));

  this->updateConnectionButtons(this->Internals->connectionsTable->currentRow());
  this->updateStyleButtons(this->Internals->stylesTable->currentRow());

  // Add the render view to the proxy combo
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();

  QList<pqRenderView*> rviews = ::pqFindItems<pqRenderView*>(smmodel);
  if (rviews.size() != 0)
    this->setActiveView(rviews.first());

#if CAVEINTERACTION_HAS_COLLABORATION
  this->Internals->CollaborationWidget = new pqVRCollaborationWidget(this);
  this->Internals->collaborationContainer->addWidget(this->Internals->CollaborationWidget);
#endif
}

//-----------------------------------------------------------------------------
pqVRDockPanel::~pqVRDockPanel()
{
  if (this->Internals->IsRunning)
  {
    this->stop();
  }

  if (this->Internals->CollaborationWidget)
  {
    delete this->Internals->CollaborationWidget;
  }

  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::initStyles()
{
  vtkVRInteractorStyleFactory* styleFactory = vtkVRInteractorStyleFactory::GetInstance();
  std::vector<std::string> styleDescs = styleFactory->GetInteractorStyleDescriptions();
  this->Internals->stylesCombo->clear();
  for (size_t i = 0; i < styleDescs.size(); ++i)
  {
    this->Internals->stylesCombo->addItem(QString::fromStdString(styleDescs[i]));
  }

  QObject::connect(this->Internals->stylesCombo, &QComboBox::currentTextChanged, this,
    &pqVRDockPanel::styleComboChanged, Qt::UniqueConnection);
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateConnections()
{
  this->Internals->connectionsTable->clear();

  pqVRConnectionManager* mgr = pqVRConnectionManager::instance();
  QList<QString> connectionNames = mgr->connectionNames();

  this->Internals->valuatorLookupTable->clear();

  Q_FOREACH (const QString& name, connectionNames)
  {
    this->Internals->connectionsTable->addItem(name);

    // Now update our valuator connection/event information
    bool found = false;
    std::map<std::string, std::string> valuatorMap;

#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
    if (pqVRPNConnection* vrpnConn = mgr->GetVRPNConnection(name))
    {
      found = true;
      valuatorMap = vrpnConn->valuatorMap();
    }
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
    if (!found)
    {
      if (pqVRUIConnection* vruiConn = mgr->GetVRUIConnection(name))
      {
        found = true;
        valuatorMap = vruiConn->valuatorMap();
      }
    }
#endif
    if (found)
    {
      auto& table = *(this->Internals->valuatorLookupTable);
      auto connMap = table[name.toStdString()];

      for (std::map<std::string, std::string>::const_iterator iter = valuatorMap.begin(),
                                                              itEnd = valuatorMap.end();
           iter != itEnd; ++iter)
      {
        connMap[iter->second] = iter->first;
      }

      table[name.toStdString()] = connMap;
    }
  }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::editConnection(QListWidgetItem* item)
{
  if (this->Internals->IsRunning)
  {
    return;
  }

  if (!item)
  {
    item = this->Internals->connectionsTable->currentItem();
  }

  if (!item)
  {
    return;
  }

  // Lookup connection
  QString connName = item->text();
  pqVRConnectionManager* mgr = pqVRConnectionManager::instance();
  (void)mgr; // Avoid unusued local variable warning if VRPN and VRUI not enabled

  pqVRAddConnectionDialog dialog(this);
  bool set = false;
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
  if (pqVRPNConnection* vrpnConn = mgr->GetVRPNConnection(connName))
  {
    set = true;
    dialog.setConnection(vrpnConn);
  }
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
  if (pqVRUIConnection* vruiConn = mgr->GetVRUIConnection(connName))
  {
    if (!set)
    {
      set = true;
      dialog.setConnection(vruiConn);
    }
  }
#endif

  if (!set)
  {
    // Connection not found!
    return;
  }

  if (dialog.exec() == QDialog::Accepted)
  {
    dialog.updateConnection();
    this->updateConnections();
  }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateConnectionButtons(int row)
{
  if (!this->Internals->IsRunning)
  {
    bool enabled = (row >= 0);
    this->Internals->editConnection->setEnabled(enabled);
    this->Internals->removeConnection->setEnabled(enabled);
  }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::addConnection()
{
  pqVRAddConnectionDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted)
  {
    pqVRConnectionManager* mgr = pqVRConnectionManager::instance();
    (void)mgr; // Avoid unusued local variable warning if VRPN and VRUI not enabled
    dialog.updateConnection();
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
    if (dialog.isVRPN())
    {
      pqVRPNConnection* conn = dialog.getVRPNConnection();
      mgr->add(conn);
    }
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
    if (dialog.isVRUI())
    {
      pqVRUIConnection* conn = dialog.getVRUIConnection();
      mgr->add(conn);
    }
#endif
  }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::removeConnection()
{
  QListWidgetItem* item = this->Internals->connectionsTable->currentItem();
  if (!item)
  {
    return;
  }
  QString name = item->text();
  pqVRConnectionManager* mgr = pqVRConnectionManager::instance();
  (void)mgr; // Avoid unusued local variable warning if VRPN and VRUI not enabled

  pqVRAddConnectionDialog dialog(this);
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
  if (pqVRPNConnection* vrpnConn = mgr->GetVRPNConnection(name))
  {
    mgr->remove(vrpnConn);
    return;
  }
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
  if (pqVRUIConnection* vruiConn = mgr->GetVRUIConnection(name))
  {
    mgr->remove(vruiConn);
  }
#endif
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::addStyle()
{
  vtkSMProxy* proxy = this->Internals->propertyCombo->getCurrentProxy();
  QByteArray property = this->Internals->propertyCombo->getCurrentPropertyName().toUtf8();
  QString styleString = this->Internals->stylesCombo->currentText();

  vtkVRInteractorStyleFactory* styleFactory = vtkVRInteractorStyleFactory::GetInstance();
  vtkSMVRInteractorStyleProxy* style =
    styleFactory->NewInteractorStyleFromDescription(styleString.toStdString());

  if (!style)
  {
    vtkWarningWithObjectMacro(nullptr, "Unable to add style " << styleString.toStdString());
    return;
  }

  style->SetControlledProxy(proxy);
  style->SetControlledPropertyName(property.data());

  style->AddObserver(vtkSMVRInteractorStyleProxy::INTERACTOR_STYLE_REQUEST_CONFIGURE, this,
    &pqVRDockPanel::configureStyle);

  pqVRAddStyleDialog dialog(this);
  QString name = this->Internals->createName(style);
  dialog.setInteractorStyle(style, name);
  if (!dialog.isConfigurable() || dialog.exec() == QDialog::Accepted)
  {
    dialog.updateInteractorStyle();
    pqVRQueueHandler* handler = pqVRQueueHandler::instance();
    handler->add(style);
  }

  // Clean up reference
  style->Delete();
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::configureStyle(vtkObject* caller, unsigned long, void*)
{
  vtkSMVRInteractorStyleProxy* styleProxy = vtkSMVRInteractorStyleProxy::SafeDownCast(caller);

  pqVRAddStyleDialog dialog(this);
  QString name = this->Internals->createName(styleProxy);
  dialog.setInteractorStyle(styleProxy, name);
  if (!dialog.isConfigurable() || dialog.exec() == QDialog::Accepted)
  {
    dialog.updateInteractorStyle();
  }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::removeStyle()
{
  QListWidgetItem* item = this->Internals->stylesTable->currentItem();
  if (!item)
  {
    return;
  }
  QString name = item->text();

  vtkSMVRInteractorStyleProxy* style = this->Internals->StyleNameMap.value(name, nullptr);
  if (!style)
  {
    return;
  }

  pqVRQueueHandler::instance()->remove(style);
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateStyles()
{
  this->Internals->StyleNameMap.clear();
  this->Internals->stylesTable->clear();

  Q_FOREACH (vtkSMVRInteractorStyleProxy* style, pqVRQueueHandler::instance()->styles())
  {
    style->SetValuatorLookupTable(this->Internals->valuatorLookupTable);

    if (style->GetIsInternal())
    {
      continue;
    }

    QString name = this->Internals->createName(style);
    this->Internals->StyleNameMap.insert(name, style);
    this->Internals->stylesTable->addItem(name);

    if (!style->HasObserver(vtkSMVRInteractorStyleProxy::INTERACTOR_STYLE_REQUEST_CONFIGURE))
    {
      style->AddObserver(vtkSMVRInteractorStyleProxy::INTERACTOR_STYLE_REQUEST_CONFIGURE, this,
        &pqVRDockPanel::configureStyle);
    }
  }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::editStyle(QListWidgetItem* item)
{
  if (!item)
  {
    item = this->Internals->stylesTable->currentItem();
  }

  if (!item)
  {
    return;
  }

  pqVRAddStyleDialog dialog(this);
  QString name = item->text();
  vtkSMVRInteractorStyleProxy* style = this->Internals->StyleNameMap.value(name, nullptr);
  if (!style)
  {
    return;
  }

  dialog.setInteractorStyle(style, name);
  if (!dialog.isConfigurable() || dialog.exec() == QDialog::Accepted)
  {
    dialog.updateInteractorStyle();
  }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateStyleButtons(int row)
{
  bool enabled = (row >= 0);
  this->Internals->editStyle->setEnabled(enabled);
  this->Internals->removeStyle->setEnabled(enabled);

  // Remove the existing proxy widget
  pqProxyWidget* proxyWidget = this->widget()->findChild<pqProxyWidget*>();
  if (proxyWidget)
  {
    proxyWidget->parentWidget()->layout()->removeWidget(proxyWidget);
    proxyWidget->deleteLater();
    this->Internals->stylePropertiesLabel->hide();
  }

  if (enabled)
  {
    QListWidgetItem* item = this->Internals->stylesTable->currentItem();
    QString name = item->text();
    vtkSMVRInteractorStyleProxy* style = this->Internals->StyleNameMap.value(name, nullptr);

    QString propertiesLabelText = "Properties (";
    propertiesLabelText.append(name);
    propertiesLabelText.append("):");
    this->Internals->stylePropertiesLabel->setText(propertiesLabelText);
    this->Internals->stylePropertiesLabel->show();

    if (!style)
    {
      vtkWarningWithObjectMacro(
        nullptr, "Unable to create proxy widget, no style with name " << name.toStdString());
      return;
    }

    proxyWidget = new pqProxyWidget(style, this);
    proxyWidget->setApplyChangesImmediately(true);
    QGridLayout* layout = qobject_cast<QGridLayout*>(this->widget()->layout());
    QVBoxLayout* propertiesLayout = layout->findChild<QVBoxLayout*>("stylePropertiesLayout");
    if (propertiesLayout)
    {
      propertiesLayout->addWidget(proxyWidget);
    }
  }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::proxyChanged(vtkSMProxy* pxy)
{
  this->Internals->propertyCombo->setSource(pxy);
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::styleComboChanged(const QString& name)
{
  vtkVRInteractorStyleFactory* styleFactory = vtkVRInteractorStyleFactory::GetInstance();
  vtkSMVRInteractorStyleProxy* style =
    styleFactory->NewInteractorStyleFromDescription(name.toStdString());
  int size = style ? style->GetControlledPropertySize() : -1;
  if (style)
  {
    style->Delete();
  }
  this->Internals->propertyCombo->setVectorSizeFilter(size);
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::setActiveView(pqView* view)
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);

  // Remove any RenderView entries in the combobox
  Qt::MatchFlags matchFlags = Qt::MatchStartsWith | Qt::MatchCaseSensitive;
  int ind = this->Internals->proxyCombo->findText("RenderView", matchFlags);
  while (ind != -1)
  {
    QString label = this->Internals->proxyCombo->itemText(ind);
    this->Internals->proxyCombo->removeProxy(label);
    ind = this->Internals->proxyCombo->findText("RenderView", matchFlags);
  }

  if (rview)
  {
    this->Internals->proxyCombo->addProxy(0, rview->getSMName(), rview->getProxy());
  }

#if CAVEINTERACTION_HAS_COLLABORATION
  if (view)
  {
    vtkSMRenderViewProxy* proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
    if (proxy)
    {
      proxy->SetEnableSynchronizableActors(true);
    }
  }
#endif
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::saveState()
{
  pqFileDialog fileDialog(nullptr, pqCoreUtilities::mainWidget(),
    tr("Save CAVE Interaction template"), QString(),
    QString("%1 (*.pvvr)").arg(tr("CAVE Interaction template files")), false);

  fileDialog.setFileMode(pqFileDialog::AnyFile);

  if (fileDialog.exec() != pqFileDialog::Accepted)
  {
    // User canceled
    return;
  }

  QString filename = fileDialog.getSelectedFiles().first();

  vtkNew<vtkPVXMLElement> root;
  root->SetName("CAVEInteractionState");

  if (pqVRConnectionManager* connMgr = pqVRConnectionManager::instance())
  {
    connMgr->saveConnectionsConfiguration(root.GetPointer());
  }
  if (pqVRQueueHandler* queueHandler = pqVRQueueHandler::instance())
  {
    queueHandler->saveStylesConfiguration(root.GetPointer());
  }

  if (this->Internals->CollaborationWidget)
  {
    this->Internals->CollaborationWidget->saveCollaborationState(root.GetPointer());
  }

  vtksys::ofstream os(filename.toUtf8().data(), ios::out);
  root->PrintXML(os, vtkIndent());
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::restoreState()
{
  pqFileDialog fileDialog(nullptr, pqCoreUtilities::mainWidget(),
    tr("Load CAVE Interaction template"), QString(),
    QString("%1 (*.pvvr);;%2 (*.pvsm)")
      .arg(tr("CAVE Interaction template files").arg(tr("ParaView state files"))),
    false);

  fileDialog.setFileMode(pqFileDialog::ExistingFile);

  if (fileDialog.exec() != pqFileDialog::Accepted)
  {
    // User canceled
    return;
  }

  QString filename = fileDialog.getSelectedFiles().first();

  vtkNew<vtkPVXMLParser> xmlParser;
  xmlParser->SetFileName(filename.toUtf8().data());
  xmlParser->Parse();

  vtkPVXMLElement* root = xmlParser->GetRootElement();

  pqVRConnectionManager* connMgr = pqVRConnectionManager::instance();
  vtkPVXMLElement* connRoot = root->FindNestedElementByName("VRConnectionManager");
  if (connMgr && connRoot)
  {
    connMgr->configureConnections(connRoot, nullptr);
  }

  pqVRQueueHandler* queueHandler = pqVRQueueHandler::instance();
  vtkPVXMLElement* stylesRoot = root->FindNestedElementByName("VRInteractorStyles");
  if (queueHandler && stylesRoot)
  {
    queueHandler->configureStyles(root, nullptr);
  }

  if (this->Internals->CollaborationWidget)
  {
    this->Internals->CollaborationWidget->restoreCollaborationState(root, nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::disableConnectionButtons()
{
  this->Internals->addConnection->setEnabled(false);
  this->Internals->editConnection->setEnabled(false);
  this->Internals->removeConnection->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::enableConnectionButtons()
{
  this->Internals->addConnection->setEnabled(true);
  this->Internals->editConnection->setEnabled(true);
  this->Internals->removeConnection->setEnabled(true);
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateStartStopButtonStates()
{
  pqVRConnectionManager* mgr = pqVRConnectionManager::instance();
  bool canStart = !this->Internals->IsRunning && mgr->numConnections() != 0;
  bool canStop = this->Internals->IsRunning;

  this->Internals->startButton->setEnabled(canStart);
  this->Internals->stopButton->setEnabled(canStop);
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::start()
{
  if (this->Internals->IsRunning)
  {
    qWarning() << "pqVRDockPanel: Cannot start listening for VR events --"
                  " already running!";
    return;
  }
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
  this->disableConnectionButtons();

  if (this->Internals->CollaborationWidget)
  {
    this->Internals->CollaborationWidget->initializeCollaboration(
      pqActiveObjects::instance().activeView());
  }

  pqVRConnectionManager::instance()->start();
  pqVRQueueHandler::instance()->start();
  this->Internals->IsRunning = true;
  this->updateStartStopButtonStates();
  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::stop()
{
  if (!this->Internals->IsRunning)
  {
    qWarning() << "pqVRDockPanel: Cannot stop listening for VR events --"
                  " not started!";
    return;
  }
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
  this->enableConnectionButtons();
  pqVRConnectionManager::instance()->stop();
  pqVRQueueHandler::instance()->stop();
  this->Internals->IsRunning = false;

  if (this->Internals->CollaborationWidget)
  {
    this->Internals->CollaborationWidget->stopCollaboration();
  }

  this->updateConnectionButtons(this->Internals->connectionsTable->currentRow());
  this->updateStartStopButtonStates();
  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
// createName() -- this method returns the string that will appear in the
//   "Interactions:" list in the Qt VR Panel for the individual given "*style".
QString pqVRDockPanel::pqInternals::createName(vtkSMVRInteractorStyleProxy* style)
{
  QString description;  // A one-line description of the interaction (style, object, property)
  QString className;    // The name of the style's VTK class (e.g. vtkVRTrackStyle)
  QString styleName;    // A human readable version of the style
  QString objectName;   // The object onto which the style interacts
  QString propertyName; // The property of the object which the style affects

  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* model = core->getServerManagerModel();
  vtkVRInteractorStyleFactory* styleFactory = vtkVRInteractorStyleFactory::GetInstance();

  className = style->GetClassName();
  styleName =
    QString::fromStdString(styleFactory->GetDescriptionFromClassName(className.toStdString()));

  vtkSMProxy* smControlledProxy = style->GetControlledProxy();
  pqProxy* pqControlledProxy = model->findItem<pqProxy*>(smControlledProxy);

  // WRS-TODO: I don't know why "<error>" occurs, there will always be a selected Proxy -- should be
  // investigated
  objectName =
    (pqControlledProxy ? pqControlledProxy->getSMName()
                       : (smControlledProxy ? smControlledProxy->GetXMLLabel() : "<error>"));
  propertyName =
    (strlen(style->GetControlledPropertyName()) ? style->GetControlledPropertyName() : "--");

  description = QString("%1 on %2's %3").arg(styleName).arg(objectName).arg(propertyName);

  return description;
}
