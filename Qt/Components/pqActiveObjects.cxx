/*=========================================================================

   Program: ParaView
   Module:    pqActiveObjects.cxx

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
#include "pqActiveObjects.h"

#include "pqApplicationCore.h"
#include "pqMultiViewWidget.h"
#include "pqServerManagerModel.h"
#include "pqTabbedMultiViewWidget.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSession.h"
#include "vtkSMViewLayoutProxy.h"

#include <algorithm>

//-----------------------------------------------------------------------------
pqActiveObjects& pqActiveObjects::instance()
{
  static pqActiveObjects activeObject;
  return activeObject;
}

//-----------------------------------------------------------------------------
pqActiveObjects::pqActiveObjects()
  : CachedServer(NULL)
  , CachedSource(NULL)
  , CachedPort(NULL)
  , CachedView(NULL)
  , CachedRepresentation(NULL)
  , VTKConnector(vtkEventQtSlotConnect::New())
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(serverAdded(pqServer*)), this, SLOT(serverAdded(pqServer*)));
  QObject::connect(smmodel, SIGNAL(serverRemoved(pqServer*)), this, SLOT(serverRemoved(pqServer*)));
  QObject::connect(smmodel, SIGNAL(preItemRemoved(pqServerManagerModelItem*)), this,
    SLOT(proxyRemoved(pqServerManagerModelItem*)));

  QObject::connect(this, SIGNAL(viewChanged(pqView*)), this, SLOT(updateRepresentation()));
  QObject::connect(this, SIGNAL(portChanged(pqOutputPort*)), this, SLOT(updateRepresentation()));

  this->VTKConnector->Connect(vtkSMProxyManager::GetProxyManager(),
    vtkSMProxyManager::ActiveSessionChanged, this, SLOT(onActiveServerChanged()));

  QList<pqServer*> servers = smmodel->findItems<pqServer*>();
  if (servers.size() == 1)
  {
    this->setActiveServer(servers[0]);
  }
}

//-----------------------------------------------------------------------------
pqActiveObjects::~pqActiveObjects()
{
  this->VTKConnector->Delete();
  this->VTKConnector = NULL;
}

//-----------------------------------------------------------------------------
void pqActiveObjects::resetActives()
{
  this->ActiveSource = NULL;
  this->ActivePort = NULL;
  this->ActiveView = NULL;
  this->ActiveRepresentation = NULL;
  this->Selection.clear();
}

//-----------------------------------------------------------------------------
void pqActiveObjects::triggerSignals()
{
  if (this->signalsBlocked())
  {
    // don't update cached variables when signals are blocked.
    return;
  }

  if (this->ActiveServer.data() != this->CachedServer)
  {
    this->CachedServer = this->ActiveServer.data();
    emit this->serverChanged(this->ActiveServer);
  }

  if (this->ActivePort.data() != this->CachedPort)
  {
    this->CachedPort = this->ActivePort.data();
    emit this->portChanged(this->ActivePort);
  }

  if (this->ActiveSource.data() != this->CachedSource)
  {
    this->CachedSource = this->ActiveSource.data();
    emit this->sourceChanged(this->ActiveSource);
  }

  if (this->ActiveRepresentation.data() != this->CachedRepresentation)
  {
    this->CachedRepresentation = this->ActiveRepresentation.data();
    emit this->representationChanged(this->ActiveRepresentation);
    emit this->representationChanged(
      static_cast<pqRepresentation*>(this->ActiveRepresentation.data()));
  }

  if (this->ActiveView.data() != this->CachedView)
  {
    this->CachedView = this->ActiveView.data();
    emit this->viewChanged(this->ActiveView);
  }

  if (this->CachedSelection != this->Selection)
  {
    this->CachedSelection = this->Selection;
    emit this->selectionChanged(this->Selection);
  }
}

//-----------------------------------------------------------------------------
void pqActiveObjects::serverAdded(pqServer* server)
{
  if (this->activeServer() == NULL && server)
  {
    this->setActiveServer(server);
  }
}

//-----------------------------------------------------------------------------
void pqActiveObjects::serverRemoved(pqServer* server)
{
  if (this->activeServer() == server)
  {
    this->setActiveServer(NULL);
  }
}

//-----------------------------------------------------------------------------
void pqActiveObjects::proxyRemoved(pqServerManagerModelItem* proxy)
{
  bool prev = this->blockSignals(true);

  if (this->ActiveSource == proxy)
  {
    this->setActiveSource(NULL);
  }
  else if (this->ActivePort == proxy)
  {
    this->setActivePort(NULL);
  }
  else if (this->ActiveView == proxy)
  {
    this->setActiveView(NULL);
  }

  this->blockSignals(prev);
  this->triggerSignals();
}

//-----------------------------------------------------------------------------
void pqActiveObjects::viewSelectionChanged()
{
  pqServer* server = this->activeServer();
  if (!this->ActiveServer)
  {
    this->resetActives();
    this->triggerSignals();
    return;
  }

  if (server->activeViewSelectionModel() == NULL)
  {
    // This mean that the servermanager is currently updating itself and no
    // selection manager is set yet...
    return;
  }

  vtkSMProxy* selectedProxy = NULL;
  vtkSMProxySelectionModel* viewSelection = server->activeViewSelectionModel();
  if (viewSelection->GetNumberOfSelectedProxies() == 1)
  {
    selectedProxy = viewSelection->GetSelectedProxy(0);
  }
  else if (viewSelection->GetNumberOfSelectedProxies() > 1)
  {
    selectedProxy = viewSelection->GetCurrentProxy();
    if (selectedProxy && !viewSelection->IsSelected(selectedProxy))
    {
      selectedProxy = NULL;
    }
  }

  pqView* view =
    pqApplicationCore::instance()->getServerManagerModel()->findItem<pqView*>(selectedProxy);

  if (this->ActiveView)
  {
    QObject::disconnect(this->ActiveView, 0, this, 0);
  }
  if (view)
  {
    QObject::connect(view, SIGNAL(representationAdded(pqRepresentation*)), this,
      SLOT(updateRepresentation()), Qt::UniqueConnection);
    QObject::connect(view, SIGNAL(representationRemoved(pqRepresentation*)), this,
      SLOT(updateRepresentation()), Qt::UniqueConnection);
  }

  this->ActiveView = view;

  // if view changed, then the active representation may have changed as well.
  this->updateRepresentation();
  // updateRepresentation calls triggerSignals().
}

//-----------------------------------------------------------------------------
void pqActiveObjects::sourceSelectionChanged()
{
  pqServer* server = this->activeServer();
  if (!this->ActiveServer)
  {
    this->resetActives();
    this->triggerSignals();
    return;
  }

  if (server->activeSourcesSelectionModel() == NULL)
  {
    // This mean that the servermanager is currently updating itself and no
    // selection manager is set yet...
    return;
  }

  if (this->ActiveSource)
  {
    QObject::disconnect(
      this->ActiveSource, SIGNAL(dataUpdated(pqPipelineSource*)), this, SIGNAL(dataUpdated()));
  }

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  vtkSMProxySelectionModel* selModel = server->activeSourcesSelectionModel();

  // setup the "ActiveSource" and "ActivePort" which depends on the
  // "CurrentProxy".
  vtkSMProxy* currentProxy = selModel->GetCurrentProxy();

  pqServerManagerModelItem* item = smmodel->findItem<pqServerManagerModelItem*>(currentProxy);
  pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
  pqPipelineSource* source = opPort ? opPort->getSource() : qobject_cast<pqPipelineSource*>(item);
  if (source && !opPort && source->getNumberOfOutputPorts() > 0)
  {
    opPort = source->getOutputPort(0);
  }

  if (this->ActivePort)
  {
    QObject::disconnect(this->ActivePort, 0, this, 0);
  }
  if (opPort)
  {
    QObject::connect(opPort, SIGNAL(representationAdded(pqOutputPort*, pqDataRepresentation*)),
      this, SLOT(updateRepresentation()));
  }
  this->ActiveSource = source;
  this->ActivePort = opPort;

  if (this->ActiveSource)
  {
    QObject::connect(
      this->ActiveSource, SIGNAL(dataUpdated(pqPipelineSource*)), this, SIGNAL(dataUpdated()));
  }

  // Update the Selection.
  pqProxySelectionUtilities::copy(selModel, this->Selection);

  this->updateRepresentation();
  // updateRepresentation calls triggerSignals().
}

//-----------------------------------------------------------------------------
void pqActiveObjects::onActiveServerChanged()
{
  vtkSMSession* activeSession = vtkSMProxyManager::GetProxyManager()->GetActiveSession();
  if (activeSession == NULL)
  {
    return;
  }

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqServer* newActiveServer = smmodel->findServer(activeSession);

  if (newActiveServer)
  {
    this->setActiveServer(newActiveServer);
  }
}
//-----------------------------------------------------------------------------

void pqActiveObjects::setActiveServer(pqServer* server)
{
  // Make sure the active server has the proper listener setup
  // in case of collaboration the object initialisation can cause a server to
  // become active before any selection model get setup
  if (this->ActiveServer == server && this->VTKConnector->GetNumberOfConnections() > 1)
  {
    return;
  }

  bool prev = this->blockSignals(true);

  this->VTKConnector->Disconnect();

  // Need to connect it back to know if python is changing the active one
  this->VTKConnector->Connect(vtkSMProxyManager::GetProxyManager(),
    vtkSMProxyManager::ActiveSessionChanged, this, SLOT(onActiveServerChanged()));

  this->ActiveServer = server;

  vtkSMProxyManager::GetProxyManager()->SetActiveSession(server ? server->session() : NULL);
  if (server && server->activeSourcesSelectionModel() && server->activeViewSelectionModel())
  {
    this->VTKConnector->Connect(server->activeSourcesSelectionModel(),
      vtkCommand::CurrentChangedEvent, this, SLOT(sourceSelectionChanged()));
    this->VTKConnector->Connect(server->activeSourcesSelectionModel(),
      vtkCommand::SelectionChangedEvent, this, SLOT(sourceSelectionChanged()));

    this->VTKConnector->Connect(server->activeViewSelectionModel(), vtkCommand::CurrentChangedEvent,
      this, SLOT(viewSelectionChanged()));
    this->VTKConnector->Connect(server->activeViewSelectionModel(),
      vtkCommand::SelectionChangedEvent, this, SLOT(viewSelectionChanged()));
  }

  this->sourceSelectionChanged();
  this->viewSelectionChanged();

  this->blockSignals(prev);
  this->triggerSignals();
}

//-----------------------------------------------------------------------------
void pqActiveObjects::setActiveView(pqView* view)
{
  bool prev = this->blockSignals(true);

  // ensure that the appropriate server is active.
  if (view)
  {
    this->setActiveServer(view->getServer());
  }

  pqServer* server = this->activeServer();
  if (server && server->activeViewSelectionModel())
  {
    server->activeViewSelectionModel()->SetCurrentProxy(
      view ? view->getProxy() : NULL, vtkSMProxySelectionModel::CLEAR_AND_SELECT);

    // this triggers a call to viewSelectionChanged(); but in collaboration mode
    // the ProxySelectionModel has been changed underneath therefore this
    // current proxy call do not trigger anything as it is already set with
    // the given value so we force our current class to update itself.
    this->viewSelectionChanged();
  }
  else
  {
    // if there's no-active server, it implies that setActiveView() must be
    // called with NULL. In that case, we cannot really clear the selection
    // since we have no clue what's the active server. So nothing to do here.
  }

  this->blockSignals(prev);
  this->triggerSignals();
}

//-----------------------------------------------------------------------------
void pqActiveObjects::setActiveSource(pqPipelineSource* source)
{
  bool prev = this->blockSignals(true);

  // ensure that the appropriate server is active.
  if (source)
  {
    this->setActiveServer(source->getServer());
  }

  pqServer* server = this->activeServer();
  if (server && server->activeSourcesSelectionModel())
  {
    server->activeSourcesSelectionModel()->SetCurrentProxy(
      source ? source->getProxy() : NULL, vtkSMProxySelectionModel::CLEAR_AND_SELECT);
    // this triggers a call to sourceSelectionChanged();
  }
  else
  {
    // if there's no-active server, it implies that setActiveSource() must be
    // called with NULL. In that case, we cannot really clear the selection
    // since we have no clue what's the active server. So nothing to do here.
  }

  this->blockSignals(prev);
  this->triggerSignals();
}

//-----------------------------------------------------------------------------
void pqActiveObjects::setActivePort(pqOutputPort* port)
{
  bool prev = this->blockSignals(true);

  // ensure that the appropriate server is active.
  if (port)
  {
    this->setActiveServer(port->getServer());
  }

  pqServer* server = this->activeServer();
  if (server)
  {
    server->activeSourcesSelectionModel()->SetCurrentProxy(
      port ? port->getOutputPortProxy() : NULL, vtkSMProxySelectionModel::CLEAR_AND_SELECT);
    // this triggers a call to selectionChanged();
  }
  else
  {
    // if there's no-active server, it implies that setActivePort() must be
    // called with NULL. In that case, we cannot really clear the selection
    // since we have no clue what's the active server. So nothing to do here.
  }

  this->blockSignals(prev);
  this->triggerSignals();
}

//-----------------------------------------------------------------------------
void pqActiveObjects::setSelection(
  const pqProxySelection& selection_, pqServerManagerModelItem* current)
{
  pqProxy* current_proxy = qobject_cast<pqProxy*>(current);
  pqOutputPort* current_port = qobject_cast<pqOutputPort*>(current);
  pqServer* server =
    current_proxy ? current_proxy->getServer() : (current_port ? current_port->getServer() : NULL);

  // ascertain that all items in the selection have the same server.
  foreach (pqServerManagerModelItem* item, selection_)
  {
    pqProxy* proxy = qobject_cast<pqProxy*>(item);
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    pqServer* cur_server =
      proxy ? proxy->getServer() : (port ? port->getServer() : qobject_cast<pqServer*>(item));
    if (cur_server != NULL && cur_server != server)
    {
      if (server == NULL)
      {
        server = cur_server;
      }
      else
      {
        qCritical("Selections with heterogeneous servers are not supported.");
        return;
      }
    }
  }

  bool prev = this->blockSignals(true);

  // ensure that the appropriate server is active.
  if (server)
  {
    this->setActiveServer(server);

    pqProxySelectionUtilities::copy(selection_, server->activeSourcesSelectionModel());
    // this triggers a call to selectionChanged() if selection actually changed.

    vtkSMProxy* proxy = current_proxy ? current_proxy->getProxy()
                                      : (current_port ? current_port->getOutputPortProxy() : NULL);
    server->activeSourcesSelectionModel()->SetCurrentProxy(proxy, vtkSMProxySelectionModel::SELECT);
  }
  else
  {
    // if there's no-active server, it implies that setSelection() must be
    // called with NULL. In that case, we cannot really clear the selection
    // since we have no clue what's the active server. So nothing to do here.
  }
  this->blockSignals(prev);
  this->triggerSignals();
}

//-----------------------------------------------------------------------------
void pqActiveObjects::updateRepresentation()
{
  pqOutputPort* port = this->activePort();
  if (port)
  {
    this->ActiveRepresentation = port->getRepresentation(this->activeView());
  }
  else
  {
    this->ActiveRepresentation = NULL;
  }
  this->triggerSignals();
}

//-----------------------------------------------------------------------------
vtkSMSessionProxyManager* pqActiveObjects::proxyManager() const
{
  return this->activeServer() ? this->activeServer()->proxyManager() : NULL;
}

//-----------------------------------------------------------------------------
vtkSMViewLayoutProxy* pqActiveObjects::activeLayout() const
{
  if (pqView* view = this->activeView())
  {
    if (auto l = vtkSMViewLayoutProxy::FindLayout(view->getViewProxy()))
    {
      return l;
    }
  }

  auto server = this->activeServer();
  if (!server)
  {
    // if not active server, then don't even attempt to deduce the active layout
    // using the pqTabbedMultiViewWidget since that doesn't matter, there's no
    // active server.
    return nullptr;
  }

  // if no active view is present, let's attempt to look for
  // pqTabbedMultiViewWidget and use its current tab if it is on the active
  // server.
  auto core = pqApplicationCore::instance();
  auto tmvwidget = qobject_cast<pqTabbedMultiViewWidget*>(core->manager("MULTIVIEW_WIDGET"));
  auto layoutProxy = tmvwidget ? tmvwidget->layoutProxy() : nullptr;
  return (layoutProxy && server->session() == layoutProxy->GetSession()) ? layoutProxy : nullptr;
}

//-----------------------------------------------------------------------------
int pqActiveObjects::activeLayoutLocation() const
{
  if (auto layout = this->activeLayout())
  {
    auto core = pqApplicationCore::instance();
    if (auto tmvwidget = qobject_cast<pqTabbedMultiViewWidget*>(core->manager("MULTIVIEW_WIDGET")))
    {
      if (auto mvwidget = tmvwidget->findTab(layout))
      {
        return std::max(mvwidget->activeFrameLocation(), 0);
      }
    }
  }
  return 0;
}
