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
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "pqVRAddConnectionDialog.h"
#include "pqVRAddStyleDialog.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkVRConnectionManager.h"
#ifdef PARAVIEW_USE_VRPN
#include "vtkVRPNConnection.h"
#endif
#ifdef PARAVIEW_USE_VRUI
#include "vtkVRUIConnection.h"
#endif
#include "vtkVRGrabWorldStyle.h"
#include "vtkVRQueueHandler.h"
#include "vtkVRTrackStyle.h"
#include "vtkWeakPointer.h"

#include <QtGui/QListWidgetItem>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QPointer>

class pqVRDockPanel::pqInternals : public Ui::VRDockPanel
{
public:
  QString createName(vtkVRInteractorStyle *);

  vtkWeakPointer<vtkCamera> Camera;
  QMap<QString, QPointer<vtkVRInteractorStyle> > StyleNameMap;
};

//-----------------------------------------------------------------------------
void pqVRDockPanel::constructor()
{
  this->setWindowTitle("VR Panel");
  QWidget* container = new QWidget(this);
  this->Internals = new pqInternals();
  this->Internals->setupUi(container);
  this->setWidget(container);

  // Connections
  connect(this->Internals->addConnection, SIGNAL(clicked()),
          this, SLOT(addConnection()));

  connect(this->Internals->removeConnection, SIGNAL(clicked()),
          this, SLOT(removeConnection()));

  connect(this->Internals->connectionsTable,
          SIGNAL(itemDoubleClicked(QListWidgetItem*)),
          this, SLOT(connectionDoubleClicked(QListWidgetItem*)));

  connect(vtkVRConnectionManager::instance(), SIGNAL(connectionsChanged()),
          this, SLOT(updateConnections()));

  // Styles
  connect(this->Internals->addStyle, SIGNAL(clicked()),
          this, SLOT(addStyle()));

  connect(this->Internals->removeStyle, SIGNAL(clicked()),
          this, SLOT(removeStyle()));

  connect(this->Internals->stylesTable,
          SIGNAL(itemDoubleClicked(QListWidgetItem*)),
          this, SLOT(styleDoubleClicked(QListWidgetItem*)));

  connect(vtkVRQueueHandler::instance(), SIGNAL(stylesChanged()),
          this, SLOT(updateStyles()));

  // Other
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
    this, SLOT(setActiveView(pqView*)));

  connect(this->Internals->proxyCombo, SIGNAL(currentProxyChanged(vtkSMProxy*)),
          this, SLOT(proxyChanged(vtkSMProxy*)));

  // Add the render view to the proxy combo
  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();

  QList<pqRenderView*> rviews = ::pqFindItems<pqRenderView*>(smmodel);
  if (rviews.size() != 0)
    this->setActiveView(rviews.first());
}

//-----------------------------------------------------------------------------
pqVRDockPanel::~pqVRDockPanel()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateConnections()
{
  this->Internals->connectionsTable->clear();
  
  vtkVRConnectionManager* mgr = vtkVRConnectionManager::instance();
  QList<QString> connectionNames = mgr->connectionNames();
  foreach (const QString& name, connectionNames)
    {
    this->Internals->connectionsTable->addItem(name);
  }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::connectionDoubleClicked(QListWidgetItem *item)
{
  if (!item)
    {
    return;
    }
  // Lookup connection
  QString connName = item->text();
  vtkVRConnectionManager* mgr = vtkVRConnectionManager::instance();

  pqVRAddConnectionDialog dialog(this);
  bool set = false;
#ifdef PARAVIEW_USE_VRPN
  if (vtkVRPNConnection *vrpnConn = mgr->GetVRPNConnection(connName))
    {
    set = true;
    dialog.setConnection(vrpnConn);
    }
#endif
#ifdef PARAVIEW_USE_VRUI
  if (vtkVRUIConnection *vruiConn = mgr->GetVRUIConnection(connName))
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
void pqVRDockPanel::addConnection()
{
  pqVRAddConnectionDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted)
    {
    vtkVRConnectionManager* mgr = vtkVRConnectionManager::instance();
    dialog.updateConnection();
#ifdef PARAVIEW_USE_VRPN
    if (dialog.isVRPN())
      {
      vtkVRPNConnection *conn = dialog.getVRPNConnection();
      mgr->add(conn);
      }
#endif
#ifdef PARAVIEW_USE_VRUI
    if (dialog.isVRUI())
      {
      vtkVRUIConnection *conn = dialog.getVRUIConnection();
      mgr->add(conn);
      }
#endif
  }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::removeConnection()
{
  QListWidgetItem *item = this->Internals->connectionsTable->currentItem();
  if (!item)
    {
    return;
    }
  QString name = item->text();
  vtkVRConnectionManager* mgr = vtkVRConnectionManager::instance();

  pqVRAddConnectionDialog dialog(this);
  bool done = false;
#ifdef PARAVIEW_USE_VRPN
  if (vtkVRPNConnection *vrpnConn = mgr->GetVRPNConnection(name))
    {
    done = true;
    mgr->remove(vrpnConn);
    }
#endif
#ifdef PARAVIEW_USE_VRUI
  if (!done)
    {
    if (vtkVRUIConnection *vruiConn = mgr->GetVRUIConnection(name))
      {
      done = true;
      mgr->remove(vruiConn);
      }
    }
#endif
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::addStyle()
{
  vtkSMProxy *proxy = this->Internals->proxyCombo->getCurrentProxy();
  QString property = this->Internals->propertyCombo->getCurrentPropertyName();
  QString styleString = this->Internals->stylesCombo->currentText();

  vtkVRInteractorStyle *style = NULL;
  vtkVRQueueHandler *handler = vtkVRQueueHandler::instance();
  if (styleString == "Grab")
    {
    vtkVRGrabWorldStyle *grabStyle = new vtkVRGrabWorldStyle(handler);
    grabStyle->setControlledProxy(proxy);
    grabStyle->setControlledPropertyName(property);
    style = grabStyle;
    }
  else if (styleString == "Track")
    {
    vtkVRTrackStyle *trackStyle = new vtkVRTrackStyle(handler);
    trackStyle->setControlledProxy(proxy);
    trackStyle->setControlledPropertyName(property);
    style = trackStyle;
    }

  pqVRAddStyleDialog dialog(this);
  QString name = this->Internals->createName(style);
  dialog.setInteractorStyle(style, name);
  if (!dialog.isConfigurable() || dialog.exec() == QDialog::Accepted)
    {
    dialog.updateInteractorStyle();
    handler->add(style);
    }
  else
    {
    style->deleteLater();
    }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::removeStyle()
{
  QListWidgetItem *item = this->Internals->stylesTable->currentItem();
  if (!item)
    {
    return;
    }
  QString name = item->text();

  vtkVRInteractorStyle *style = this->Internals->StyleNameMap.value(name, NULL);
  if (!style)
    {
    return;
    }

  vtkVRQueueHandler::instance()->remove(style);
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateStyles()
{
  this->Internals->StyleNameMap.clear();
  this->Internals->stylesTable->clear();

  foreach(vtkVRInteractorStyle *style, vtkVRQueueHandler::instance()->styles())
    {
    QString name = this->Internals->createName(style);
    this->Internals->StyleNameMap.insert(name, style);
    this->Internals->stylesTable->addItem(name);
    }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::styleDoubleClicked(QListWidgetItem *item)
{
  pqVRAddStyleDialog dialog(this);
  QString name = item->text();
  vtkVRInteractorStyle *style = this->Internals->StyleNameMap.value(name, NULL);
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
void pqVRDockPanel::proxyChanged(vtkSMProxy *pxy)
{
  if(vtkSMRenderViewProxy::SafeDownCast(pxy))
    {
    this->Internals->propertyCombo->setSourceWithoutProperties(pxy);
    this->Internals->propertyCombo->addSMProperty("Eye Transform",
                                                  "EyeTransformMatrix", 0);
    this->Internals->propertyCombo->addSMProperty("Model Transform",
                                                  "ModelTransformMatrix", 0);
    }
  else
    {
    this->Internals->propertyCombo->setSource(pxy);
    }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::setActiveView(pqView *view)
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);
  this->Internals->proxyCombo->removeProxy("RenderView");
  if (rview && this->Internals->proxyCombo->findText("RenderView") == -1)
    {
    this->Internals->proxyCombo->addProxy(0, "RenderView", rview->getProxy());
    }

  this->Internals->Camera = NULL;
  if (rview)
    {
    if (vtkSMRenderViewProxy *renPxy = rview->getRenderViewProxy())
      {
      if (this->Internals->Camera = renPxy->GetActiveCamera())
        {
        pqCoreUtilities::connect(this->Internals->Camera,
                                 vtkCommand::ModifiedEvent,
                                 this, SLOT(updateDebugLabel()));
        }
      }
    }
  this->updateDebugLabel();
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateDebugLabel()
{
  if (this->Internals->Camera)
    {
    double *pos = this->Internals->Camera->GetPosition();
    QString debugString = QString("Camera position: %1 %2 %3")
        .arg(pos[0]).arg(pos[1]).arg(pos[2]);
    this->Internals->debugLabel->setText(debugString);
    this->Internals->debugLabel->show();
    }
  else
    {
    this->Internals->debugLabel->hide();
    }
}

//-----------------------------------------------------------------------------
QString pqVRDockPanel::pqInternals::createName(vtkVRInteractorStyle *style)
{
  pqApplicationCore *core = pqApplicationCore::instance();
  pqServerManagerModel *model = core->getServerManagerModel();

  QString className = style->metaObject()->className();
  QString name = QString("%1").arg(className);
  if (vtkVRTrackStyle *trackStyle = qobject_cast<vtkVRTrackStyle*>(style))
    {
    vtkSMProxy *smControlledProxy = trackStyle->controlledProxy();
    pqProxy *pqControlledProxy = model->findItem<pqProxy*>(smControlledProxy);
    name.append(QString(" on %1's %2")
                       .arg(pqControlledProxy->getSMName())
                       .arg(trackStyle->controlledPropertyName()));
    }
  return name;
}
