/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqVRDockPanel.h"
#include "ui_pqVRDockPanel.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqLoadStateReaction.h"
#include "pqRenderView.h"
#include "pqSaveStateReaction.h"
#include "pqServerManagerModel.h"
#include "pqVRAddConnectionDialog.h"
#include "pqVRAddStyleDialog.h"
#include "pqVRConnectionManager.h"
#include "pqVRQueueHandler.h"
#include "pqView.h"

#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
#include "pqVRPNConnection.h"
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
#include "pqVRUIConnection.h"
#endif

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkVRInteractorStyle.h"
#include "vtkVRInteractorStyleFactory.h"
#include "vtkWeakPointer.h"

#include <QListWidgetItem>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QPointer>

#include <fstream>

class pqVRDockPanel::pqInternals : public Ui::VRDockPanel
{
public:
  QString createName(vtkVRInteractorStyle*);

  bool IsRunning;

  vtkWeakPointer<vtkCamera> Camera;
  QMap<QString, vtkVRInteractorStyle*> StyleNameMap;
};

//-----------------------------------------------------------------------------
void pqVRDockPanel::constructor()
{
  this->setWindowTitle("VR Panel");
  QWidget* container = new QWidget(this);
  this->Internals = new pqInternals();
  this->Internals->setupUi(container);
  this->setWidget(container);

  this->Internals->IsRunning = false;
  this->updateStartStopButtonStates();

  QFont font = this->Internals->debugLabel->font();
  font.setFamily("Courier");
  this->Internals->debugLabel->setFont(font);

  this->Internals->propertyCombo->setCollapseVectors(true);

  vtkVRInteractorStyleFactory* styleFactory = vtkVRInteractorStyleFactory::GetInstance();
  std::vector<std::string> styleDescs = styleFactory->GetInteractorStyleDescriptions();
  for (size_t i = 0; i < styleDescs.size(); ++i)
  {
    this->Internals->stylesCombo->addItem(QString::fromStdString(styleDescs[i]));
  }

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

  connect(this->Internals->stylesCombo, SIGNAL(currentIndexChanged(QString)), this,
    SLOT(styleComboChanged(QString)));

  connect(this->Internals->saveState, SIGNAL(clicked()), this, SLOT(saveState()));

  connect(this->Internals->restoreState, SIGNAL(clicked()), this, SLOT(restoreState()));

  connect(this->Internals->startButton, SIGNAL(clicked()), this, SLOT(start()));

  connect(this->Internals->stopButton, SIGNAL(clicked()), this, SLOT(stop()));

  this->styleComboChanged(this->Internals->stylesCombo->currentText());
  this->updateConnectionButtons(this->Internals->connectionsTable->currentRow());
  this->updateStyleButtons(this->Internals->stylesTable->currentRow());

  // Add the render view to the proxy combo
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();

  QList<pqRenderView*> rviews = ::pqFindItems<pqRenderView*>(smmodel);
  if (rviews.size() != 0)
    this->setActiveView(rviews.first());
}

//-----------------------------------------------------------------------------
pqVRDockPanel::~pqVRDockPanel()
{
  if (this->Internals->IsRunning)
  {
    this->stop();
  }
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateConnections()
{
  this->Internals->connectionsTable->clear();

  pqVRConnectionManager* mgr = pqVRConnectionManager::instance();
  QList<QString> connectionNames = mgr->connectionNames();
  foreach (const QString& name, connectionNames)
  {
    this->Internals->connectionsTable->addItem(name);
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

  pqVRAddConnectionDialog dialog(this);
  bool set = false;
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
  if (pqVRPNConnection* vrpnConn = mgr->GetVRPNConnection(connName))
  {
    set = true;
    dialog.setConnection(vrpnConn);
  }
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
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
    dialog.updateConnection();
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
    if (dialog.isVRPN())
    {
      pqVRPNConnection* conn = dialog.getVRPNConnection();
      mgr->add(conn);
    }
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
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

  pqVRAddConnectionDialog dialog(this);
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
  if (pqVRPNConnection* vrpnConn = mgr->GetVRPNConnection(name))
  {
    mgr->remove(vrpnConn);
    return;
  }
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
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
  QByteArray property = this->Internals->propertyCombo->getCurrentPropertyName().toLocal8Bit();
  QString styleString = this->Internals->stylesCombo->currentText();

  vtkVRInteractorStyleFactory* styleFactory = vtkVRInteractorStyleFactory::GetInstance();
  vtkVRInteractorStyle* style =
    styleFactory->NewInteractorStyleFromDescription(styleString.toStdString());

  if (!style)
  {
    return;
  }

  style->SetControlledProxy(proxy);
  style->SetControlledPropertyName(property.data());

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
void pqVRDockPanel::removeStyle()
{
  QListWidgetItem* item = this->Internals->stylesTable->currentItem();
  if (!item)
  {
    return;
  }
  QString name = item->text();

  vtkVRInteractorStyle* style = this->Internals->StyleNameMap.value(name, NULL);
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

  foreach (vtkVRInteractorStyle* style, pqVRQueueHandler::instance()->styles())
  {
    QString name = this->Internals->createName(style);
    this->Internals->StyleNameMap.insert(name, style);
    this->Internals->stylesTable->addItem(name);
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
  vtkVRInteractorStyle* style = this->Internals->StyleNameMap.value(name, NULL);
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
  vtkVRInteractorStyle* style = styleFactory->NewInteractorStyleFromDescription(name.toStdString());
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

  // Remove any RenderView.* entries in the combobox
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

  this->Internals->Camera = NULL;
  if (rview)
  {
    if (vtkSMRenderViewProxy* renPxy = rview->getRenderViewProxy())
    {
      if (this->Internals->Camera = renPxy->GetActiveCamera())
      {
        pqCoreUtilities::connect(
          this->Internals->Camera, vtkCommand::ModifiedEvent, this, SLOT(updateDebugLabel()));
      }
    }
  }
  this->updateDebugLabel();
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::saveState()
{
  pqFileDialog fileDialog(NULL, pqCoreUtilities::mainWidget(), "Save VR plugin template", QString(),
    "VR plugin template files (*.pvvr)");

  fileDialog.setFileMode(pqFileDialog::AnyFile);

  if (fileDialog.exec() != pqFileDialog::Accepted)
  {
    // User canceled
    return;
  }

  QString filename = fileDialog.getSelectedFiles().first();

  vtkNew<vtkPVXMLElement> root;
  root->SetName("VRPluginState");

  if (pqVRConnectionManager* connMgr = pqVRConnectionManager::instance())
  {
    connMgr->saveConnectionsConfiguration(root.GetPointer());
  }
  if (pqVRQueueHandler* queueHandler = pqVRQueueHandler::instance())
  {
    queueHandler->saveStylesConfiguration(root.GetPointer());
  }

  // Avoid temporary QByteArrays in QString --> const char * conversion:
  QByteArray filename_ba = filename.toLocal8Bit();
  ofstream os(filename_ba.constData(), ios::out);
  root->PrintXML(os, vtkIndent());
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::restoreState()
{
  pqFileDialog fileDialog(NULL, pqCoreUtilities::mainWidget(), "Load VR plugin template", QString(),
    "VR plugin template files (*.pvvr);;"
    "ParaView state files (*.pvsm)");

  fileDialog.setFileMode(pqFileDialog::ExistingFile);

  if (fileDialog.exec() != pqFileDialog::Accepted)
  {
    // User canceled
    return;
  }

  QString filename = fileDialog.getSelectedFiles().first();

  vtkNew<vtkPVXMLParser> xmlParser;
  xmlParser->SetFileName(qPrintable(filename));
  xmlParser->Parse();

  vtkPVXMLElement* root = xmlParser->GetRootElement();

  pqVRConnectionManager* connMgr = pqVRConnectionManager::instance();
  vtkPVXMLElement* connRoot = root->FindNestedElementByName("VRConnectionManager");
  if (connMgr && connRoot)
  {
    connMgr->configureConnections(connRoot, NULL);
  }

  pqVRQueueHandler* queueHandler = pqVRQueueHandler::instance();
  vtkPVXMLElement* stylesRoot = root->FindNestedElementByName("VRInteractorStyles");
  if (queueHandler && stylesRoot)
  {
    queueHandler->configureStyles(root, NULL);
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
  this->updateConnectionButtons(this->Internals->connectionsTable->currentRow());
  this->updateStartStopButtonStates();
  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateDebugLabel()
{
  if (this->Internals->Camera)
  {
    double* pos = this->Internals->Camera->GetPosition();
    QString debugString =
      QString("Camera position: %1 %2 %3\n").arg(pos[0]).arg(pos[1]).arg(pos[2]);
    vtkMatrix4x4* mv = this->Internals->Camera->GetModelViewTransformMatrix();
    debugString += "ModelView Matrix:\n";
    for (int i = 0; i < 4; ++i)
    {
      double e0 = mv->GetElement(i, 0);
      double e1 = mv->GetElement(i, 1);
      double e2 = mv->GetElement(i, 2);
      double e3 = mv->GetElement(i, 3);
      debugString += QString("%1 %2 %3 %4\n")
                       .arg(fabs(e0) < 1e-5 ? 0.0 : e0, 8, 'g', 3)
                       .arg(fabs(e1) < 1e-5 ? 0.0 : e1, 8, 'g', 3)
                       .arg(fabs(e2) < 1e-5 ? 0.0 : e2, 8, 'g', 3)
                       .arg(fabs(e3) < 1e-5 ? 0.0 : e3, 8, 'g', 3);
    }
    // Pop off trailing newline
    debugString.remove(QRegExp("\n$"));
    this->Internals->debugLabel->setText(debugString);
    this->Internals->debugLabel->show();
  }
  else
  {
    this->Internals->debugLabel->hide();
  }
}

//-----------------------------------------------------------------------------
QString pqVRDockPanel::pqInternals::createName(vtkVRInteractorStyle* style)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* model = core->getServerManagerModel();
  vtkVRInteractorStyleFactory* styleFactory = vtkVRInteractorStyleFactory::GetInstance();

  QString className = style->GetClassName();
  QString desc =
    QString::fromStdString(styleFactory->GetDescriptionFromClassName(className.toStdString()));
  vtkSMProxy* smControlledProxy = style->GetControlledProxy();
  pqProxy* pqControlledProxy = model->findItem<pqProxy*>(smControlledProxy);
  QString name =
    QString("%1 on %2's %3")
      .arg(desc)
      .arg(pqControlledProxy ? pqControlledProxy->getSMName() : smControlledProxy->GetXMLLabel())
      .arg(style->GetControlledPropertyName());

  return name;
}
