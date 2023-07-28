// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLiveInsituVisualizationManager.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkWeakPointer.h"

#include <QPointer>

#include <cassert>

//-----------------------------------------------------------------------------
class pqLiveInsituVisualizationManager::pqInternals
{
public:
  QPointer<pqServer> InsituSession;
  QPointer<pqServer> DisplaySession;
  vtkWeakPointer<vtkSMLiveInsituLinkProxy> LiveInsituLinkProxy;
  QList<QPointer<pqPipelineSource>> ExtractSourceProxies;
};

//-----------------------------------------------------------------------------
pqLiveInsituVisualizationManager::pqLiveInsituVisualizationManager(
  int connection_port, pqServer* server)
  : Superclass(server)
  , Internals(new pqInternals())
{
  assert(server != nullptr);

  this->Internals->DisplaySession = server;

  pqApplicationCore* core = pqApplicationCore::instance();

  // we need to unregister extracts when the extracts proxy is deleted.
  QObject::connect(core->getServerManagerModel(), SIGNAL(preSourceRemoved(pqPipelineSource*)), this,
    SLOT(sourceRemoved(pqPipelineSource*)));

  vtkSMProxy* proxy =
    core->getObjectBuilder()->createProxy("coprocessing", "LiveInsituLink", server, "coprocessing");

  vtkSMLiveInsituLinkProxy* adaptor = vtkSMLiveInsituLinkProxy::SafeDownCast(proxy);
  assert(adaptor != nullptr);

  vtkSMPropertyHelper(adaptor, "InsituPort").Set(connection_port);
  vtkSMPropertyHelper(adaptor, "ProcessType").Set("Visualization");
  adaptor->UpdateVTKObjects();

  // create a new "server session" that acts as the dummy session representing
  // the insitu viz pipeline.
  pqServer* catalyst = core->getObjectBuilder()->createServer(pqServerResource("catalyst:"));
  catalyst->setProperty(
    "LiveInsituVisualizationManager", QVariant::fromValue(static_cast<QObject*>(this)));
  catalyst->setProperty("DisplaySession", QVariant::fromValue<QObject*>(server));
  adaptor->SetInsituProxyManager(catalyst->proxyManager());
  catalyst->setMonitorServerNotifications(true);
  adaptor->InvokeCommand("Initialize");

  pqCoreUtilities::connect(adaptor, vtkCommand::UpdateEvent, this, SLOT(timestepsUpdated()));
  pqCoreUtilities::connect(
    adaptor, vtkCommand::ConnectionClosedEvent, this, SIGNAL(insituDisconnected()));
  pqCoreUtilities::connect(
    adaptor, vtkCommand::ConnectionCreatedEvent, this, SIGNAL(insituConnected()));

  this->Internals->InsituSession = catalyst;
  this->Internals->LiveInsituLinkProxy = adaptor;
}

//-----------------------------------------------------------------------------
pqServer* pqLiveInsituVisualizationManager::insituSession() const
{
  return this->Internals->InsituSession;
}

//-----------------------------------------------------------------------------
pqServer* pqLiveInsituVisualizationManager::displaySession() const
{
  return this->Internals->DisplaySession;
}

//-----------------------------------------------------------------------------
pqServer* pqLiveInsituVisualizationManager::displaySession(pqServer* server)
{
  pqServer* displayServer = server
    ? qobject_cast<pqServer*>(server->property("DisplaySession").value<QObject*>())
    : nullptr;
  return displayServer ? displayServer : server;
}

//-----------------------------------------------------------------------------
pqLiveInsituVisualizationManager::~pqLiveInsituVisualizationManager()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core && this->Internals->InsituSession && core->getObjectBuilder())
  {
    core->getObjectBuilder()->removeServer(this->Internals->InsituSession);
  }

  // Remove the "LiveInsituLink" proxy.
  if (this->Internals->DisplaySession && this->Internals->LiveInsituLinkProxy)
  {
    vtkSMSessionProxyManager* pxm = this->Internals->DisplaySession->proxyManager();
    pxm->UnRegisterProxy("coprocessing",
      pxm->GetProxyName("coprocessing", this->Internals->LiveInsituLinkProxy),
      this->Internals->LiveInsituLinkProxy);
    if (this->Internals->LiveInsituLinkProxy != nullptr)
    {
      qWarning("LiveInsituLinkProxy must have been unregistered and deleted by now."
               " Since it wasn't, it would imply a leak.");
    }
    this->Internals->LiveInsituLinkProxy = nullptr;
  }

  delete this->Internals;
}

//-----------------------------------------------------------------------------
vtkSMLiveInsituLinkProxy* pqLiveInsituVisualizationManager::getProxy() const
{
  return this->Internals->LiveInsituLinkProxy;
}

//-----------------------------------------------------------------------------
bool pqLiveInsituVisualizationManager::hasExtracts(pqOutputPort* port) const
{
  if (port == nullptr || port->getServer() != this->Internals->InsituSession ||
    this->Internals->LiveInsituLinkProxy == nullptr)
  {
    return false;
  }

  if (this->Internals->LiveInsituLinkProxy->HasExtract(
        port->getSource()->getSMGroup().toUtf8().data(),
        port->getSource()->getSMName().toUtf8().data(), port->getPortNumber()))
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqLiveInsituVisualizationManager::addExtract(pqOutputPort* port)
{
  if (this->hasExtracts(port))
  {
    // don't add another extract for the same object.
    return false;
  }

  pqPipelineSource* source = port->getSource();

  vtkSMProxy* proxy =
    this->Internals->LiveInsituLinkProxy->CreateExtract(source->getSMGroup().toUtf8().data(),
      source->getSMName().toUtf8().data(), port->getPortNumber());

  QString name;
  if (source->getNumberOfOutputPorts() > 1)
  {
    name = QString("Extract: %1 (%2)").arg(source->getSMName()).arg(port->getPortNumber());
  }
  else
  {
    name = QString("Extract: %1").arg(source->getSMName());
  }

  this->Internals->DisplaySession->proxyManager()->RegisterProxy(
    "sources", name.toUtf8().data(), proxy);

  pqPipelineSource* pqproxy =
    pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(proxy);
  assert(pqproxy);
  pqproxy->setModifiedState(pqProxy::UNINITIALIZED);
  pqproxy->setProperty("CATALYST_EXTRACT", true);

  this->Internals->ExtractSourceProxies.push_back(pqproxy);

  // To ensure that the Pipeline browser updates the icon for the port, we fire
  // a bogus event.
  pqProxy::ModifiedState curState = source->modifiedState();
  source->setModifiedState(curState == pqProxy::MODIFIED ? pqProxy::UNMODIFIED : pqProxy::MODIFIED);
  source->setModifiedState(curState);

  // pqActiveObjects::instance().setActiveServer(pqproxy->getServer());
  // this->setRepresentationVisibility(
  //  pqproxy->getOutputPort(0),
  //  pqActiveObjects::instance().activeView(), true);
  return true;
}

//-----------------------------------------------------------------------------
void pqLiveInsituVisualizationManager::sourceRemoved(pqPipelineSource* source)
{
  if (source->getServer() != this->Internals->DisplaySession ||
    !source->property("CATALYST_EXTRACT").toBool() ||
    this->Internals->LiveInsituLinkProxy == nullptr)
  {
    return;
  }

  // remove extract.
  this->Internals->LiveInsituLinkProxy->RemoveExtract(source->getProxy());
  this->Internals->ExtractSourceProxies.removeAll(source);
}

//-----------------------------------------------------------------------------
void pqLiveInsituVisualizationManager::timestepsUpdated()
{
  Q_FOREACH (pqPipelineSource* source, this->Internals->ExtractSourceProxies)
  {
    if (source)
    {
      source->getProxy()->UpdateVTKObjects();
      source->updatePipeline();
      source->setModifiedState(pqProxy::UNMODIFIED);
    }
  }
  this->Internals->ExtractSourceProxies.clear();
  Q_EMIT nextTimestepAvailable();
}
