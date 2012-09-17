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
#include "pqVRConnectionManager.h"
#include "pqVRQueueHandler.h"

#ifdef PARAVIEW_USE_VRPN
#include "pqVRPNConnection.h"
#endif
#ifdef PARAVIEW_USE_VRUI
#include "pqVRUIConnection.h"
#endif

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkVRGrabWorldStyle.h"
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

  // Connections
  connect(this->Internals->addConnection, SIGNAL(clicked()),
          this, SLOT(addConnection()));

  connect(this->Internals->removeConnection, SIGNAL(clicked()),
          this, SLOT(removeConnection()));

  connect(this->Internals->connectionsTable,
          SIGNAL(itemDoubleClicked(QListWidgetItem*)),
          this, SLOT(connectionDoubleClicked(QListWidgetItem*)));

  connect(pqVRConnectionManager::instance(), SIGNAL(connectionsChanged()),
          this, SLOT(updateConnections()));

  // Styles
  connect(this->Internals->addStyle, SIGNAL(clicked()),
          this, SLOT(addStyle()));

  connect(this->Internals->removeStyle, SIGNAL(clicked()),
          this, SLOT(removeStyle()));

  connect(this->Internals->stylesTable,
          SIGNAL(itemDoubleClicked(QListWidgetItem*)),
          this, SLOT(styleDoubleClicked(QListWidgetItem*)));

  connect(pqVRQueueHandler::instance(), SIGNAL(stylesChanged()),
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
  
  pqVRConnectionManager* mgr = pqVRConnectionManager::instance();
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
  pqVRConnectionManager* mgr = pqVRConnectionManager::instance();

  pqVRAddConnectionDialog dialog(this);
  bool set = false;
#ifdef PARAVIEW_USE_VRPN
  if (pqVRPNConnection *vrpnConn = mgr->GetVRPNConnection(connName))
    {
    set = true;
    dialog.setConnection(vrpnConn);
    }
#endif
#ifdef PARAVIEW_USE_VRUI
  if (pqVRUIConnection *vruiConn = mgr->GetVRUIConnection(connName))
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
    pqVRConnectionManager* mgr = pqVRConnectionManager::instance();
    dialog.updateConnection();
#ifdef PARAVIEW_USE_VRPN
    if (dialog.isVRPN())
      {
      pqVRPNConnection *conn = dialog.getVRPNConnection();
      mgr->add(conn);
      }
#endif
#ifdef PARAVIEW_USE_VRUI
    if (dialog.isVRUI())
      {
      pqVRUIConnection *conn = dialog.getVRUIConnection();
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
  pqVRConnectionManager* mgr = pqVRConnectionManager::instance();

  pqVRAddConnectionDialog dialog(this);
  bool done = false;
#ifdef PARAVIEW_USE_VRPN
  if (pqVRPNConnection *vrpnConn = mgr->GetVRPNConnection(name))
    {
    done = true;
    mgr->remove(vrpnConn);
    }
#endif
#ifdef PARAVIEW_USE_VRUI
  if (!done)
    {
    if (pqVRUIConnection *vruiConn = mgr->GetVRUIConnection(name))
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
  QByteArray property =
      this->Internals->propertyCombo->getCurrentPropertyName().toLocal8Bit();
  QString styleString = this->Internals->stylesCombo->currentText();

  vtkVRInteractorStyle *style = NULL;
  pqVRQueueHandler *handler = pqVRQueueHandler::instance();
  if (styleString == "Grab")
    {
    vtkVRGrabWorldStyle *grabStyle = vtkVRGrabWorldStyle::New();
    grabStyle->SetControlledProxy(proxy);
    grabStyle->SetControlledPropertyName(property.data());
    style = grabStyle;
    }
  else if (styleString == "Track")
    {
    vtkVRTrackStyle *trackStyle = vtkVRTrackStyle::New();
    trackStyle->SetControlledProxy(proxy);
    trackStyle->SetControlledPropertyName(property.data());
    style = trackStyle;
    }

  if (!style)
    {
    return;
    }

  pqVRAddStyleDialog dialog(this);
  QString name = this->Internals->createName(style);
  dialog.setInteractorStyle(style, name);
  if (!dialog.isConfigurable() || dialog.exec() == QDialog::Accepted)
    {
    dialog.updateInteractorStyle();
    handler->add(style);
    }

  // Clean up reference
  style->Delete();
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

  pqVRQueueHandler::instance()->remove(style);
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::updateStyles()
{
  this->Internals->StyleNameMap.clear();
  this->Internals->stylesTable->clear();

  foreach(vtkVRInteractorStyle *style, pqVRQueueHandler::instance()->styles())
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
  this->Internals->propertyCombo->setSourceWithoutProperties(pxy);
  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(pxy->NewPropertyIterator());
  // Show only 16 element properties (e.g. 4x4 matrices)
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMVectorProperty* smproperty =
      vtkSMVectorProperty::SafeDownCast(iter->GetProperty());
    if (!smproperty || !smproperty->GetAnimateable() ||
      smproperty->GetInformationOnly())
      {
      continue;
      }
    unsigned int num_elems = smproperty->GetNumberOfElements();
    if (num_elems != 16)
      {
      continue;
      }

    this->Internals->propertyCombo->addSMProperty(
          iter->GetProperty()->GetXMLLabel(), iter->GetKey(), 0);
    }
}

//-----------------------------------------------------------------------------
void pqVRDockPanel::setActiveView(pqView *view)
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);

  // Remove any RenderView.* entries in the combobox
  Qt::MatchFlags matchFlags = Qt::MatchStartsWith | Qt::MatchCaseSensitive;
  int ind = this->Internals->proxyCombo->findText("RenderView", matchFlags);
  while (ind != -1 )
    {
    QString label = this->Internals->proxyCombo->itemText(ind);
    this->Internals->proxyCombo->removeProxy(label);
    ind = this->Internals->proxyCombo->findText("RenderView", matchFlags);
    }

  if (rview)
    {
    this->Internals->proxyCombo->addProxy(0, rview->getSMName(),
                                          rview->getProxy());
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

  QString className = style->GetClassName();
  QString name = QString("%1").arg(className);
  if (vtkVRTrackStyle *trackStyle = vtkVRTrackStyle::SafeDownCast(style))
    {
    vtkSMProxy *smControlledProxy = trackStyle->GetControlledProxy();
    pqProxy *pqControlledProxy = model->findItem<pqProxy*>(smControlledProxy);
    name.append(QString(" on %1's %2")
                       .arg(pqControlledProxy->getSMName())
                       .arg(trackStyle->GetControlledPropertyName()));
    }
  return name;
}
