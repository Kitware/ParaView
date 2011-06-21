/*=========================================================================

   Program: ParaView
   Module:    pqServerManagerModel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
#include "pqServerManagerModel.h"

#include "pqApplicationCore.h"
#include "pqHelperProxyStateLoader.h"
#include "pqInterfaceTracker.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxy.h"
#include "pqRepresentation.h"
#include "pqServer.h"
#include "pqServerManagerModelInterface.h"
#include "pqServerManagerObserver.h"
#include "pqView.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkStringList.h"

// Qt Includes.
#include <QPointer>
#include <QList>
#include <QMap>
#include <QtDebug>

//-----------------------------------------------------------------------------
class pqServerManagerModel::pqInternal
{
public:
  typedef QMap<vtkIdType, QPointer<pqServer> > ServerMap;
  ServerMap Servers;

  typedef QMap<vtkSMProxy*, QPointer<pqProxy> > ProxyMap;
  ProxyMap Proxies;

  typedef QMap<vtkSMOutputPort*, QPointer<pqOutputPort> > OutputPortMap;
  OutputPortMap OutputPorts;

  QList<QPointer<pqServerManagerModelItem> > ItemList;

  pqServerResource ActiveResource;
};

//-----------------------------------------------------------------------------
pqServerManagerModel::pqServerManagerModel(
    pqServerManagerObserver* observer,
    QObject* _parent /*=0*/) :
  QObject(_parent)
{
  this->Internal = new pqServerManagerModel::pqInternal();
  QObject::connect(observer, 
    SIGNAL(proxyRegistered(const QString&, const QString&, vtkSMProxy*)),
    this, SLOT(onProxyRegistered(const QString&, const QString&, vtkSMProxy*)));
  QObject::connect(observer, 
    SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)),
    this, SLOT(onProxyUnRegistered(const QString&, const QString&, vtkSMProxy*)));

  QObject::connect(observer, SIGNAL(connectionCreated(vtkIdType)),
    this, SLOT(onConnectionCreated(vtkIdType)));
  QObject::connect(observer, SIGNAL(connectionClosed(vtkIdType)),
    this, SLOT(onConnectionClosed(vtkIdType)));
  QObject::connect(observer,
    SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)),
    this, SLOT(onStateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)));
}

//-----------------------------------------------------------------------------
pqServerManagerModel::~pqServerManagerModel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::setActiveResource(const pqServerResource& resource)
{
  this->Internal->ActiveResource = resource;
}

//-----------------------------------------------------------------------------
pqServer* pqServerManagerModel::findServer(vtkSession* session) const
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return this->findServer(pm->GetSessionID(session));
}

//-----------------------------------------------------------------------------
pqServer* pqServerManagerModel::findServer(vtkSMSession* session) const
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  return this->findServer(pm->GetSessionID(session));
}

//-----------------------------------------------------------------------------
pqServer* pqServerManagerModel::findServer(vtkIdType cid) const
{
  pqInternal::ServerMap::iterator iter = this->Internal->Servers.find(cid);
  if (iter != this->Internal->Servers.end())
    {
    return iter.value();
    }

  return 0;
}

//-----------------------------------------------------------------------------
pqServer* pqServerManagerModel::findServer(const pqServerResource& resource) const
{
  foreach(pqServer* server, this->Internal->Servers)
    {
    if (server && server->getResource() == resource)  
      {
      return server;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem* pqServerManagerModel::findItemHelper(
  const pqServerManagerModel* const model, const QMetaObject& vtkNotUsed(mo), 
  vtkSMProxy* proxy)
{
  pqInternal::ProxyMap::iterator iter = model->Internal->Proxies.find(proxy);
  if (iter != model->Internal->Proxies.end())
    {
    return iter.value();
    }

  vtkSMOutputPort* port = vtkSMOutputPort::SafeDownCast(proxy);
  if (port)
    {
    pqPipelineSource* src =
      model->findItem<pqPipelineSource*>(port->GetSourceProxy());
    if (src)
      {
      for (int cc=0; cc < src->getNumberOfOutputPorts(); cc++)
        {
        pqOutputPort* pqport = src->getOutputPort(cc);
        if (pqport && pqport->getOutputPortProxy() == port)
          {
          return pqport;
          }
        }
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem* pqServerManagerModel::findItemHelper(
  const pqServerManagerModel* const model,
  const QMetaObject& mo, vtkTypeUInt32 id)
{
  foreach (pqServerManagerModelItem* item, model->Internal->ItemList)
    {
    if (item && mo.cast(item))
      {
      pqProxy* proxy = qobject_cast<pqProxy*>(item);
      if (proxy && proxy->getProxy()->GetGlobalID() == id)
        {
        return proxy;
        }
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem* pqServerManagerModel::findItemHelper(
  const pqServerManagerModel* const model, const QMetaObject& mo, 
  const QString& name)
{
  foreach (pqServerManagerModelItem* item, model->Internal->ItemList)
    {
    if (item && mo.cast(item))
      {
      pqProxy* proxy = qobject_cast<pqProxy*>(item);
      if (proxy && proxy->getSMName() == name)
        {
        return proxy;
        }
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::findItemsHelper(const pqServerManagerModel *const model, 
  const QMetaObject &mo, QList<void *> *list, pqServer* server/*=0*/)
{
  if (!model || !list)
    {
    return;
    }

  foreach (pqServerManagerModelItem* item, model->Internal->ItemList)
    {
    if (item && mo.cast(item))
      {
      if (server)
        {
        pqProxy* pitem = qobject_cast<pqProxy*>(item);
        if (pitem && pitem->getServer() != server)
          {
          continue;
          }
        }
      list->push_back(item);
      }
    }
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onProxyRegistered(const QString& group,
  const QString& name, vtkSMProxy* proxy)
{
  if (group.endsWith("_prototypes") || proxy->GetSession() == NULL)
    {
    // Ignore prototype proxies.
    return;
    }
  if (name=="StreamingOptionsInstance" || name=="AdaptiveOptionsInstance")
    {
    // Ignore this particular proxy
    return;
    }
  if (!proxy)
    {
    qCritical() << "Null proxy cannot be registered.";
    return;
    }

  pqServer* server = this->findServer(proxy->GetSession());

  // Warn and return if the server can't be found. This indicates some logic error in
  // the application.
  if (!server)
    {
    qDebug() << "Failed to locate server for newly registered proxy ("
      << group << ", " << name << ")";
    return;
    }

  if (this->findItem<pqProxy*>(proxy))
    {
    // item already exists for this proxy, don't create a new one.
    return;
    }

  pqProxy* item = 0;

  QObjectList ifaces =
    pqApplicationCore::instance()->interfaceTracker()->interfaces();
  foreach(QObject* iface, ifaces)
    {
    pqServerManagerModelInterface* smi = 
      qobject_cast<pqServerManagerModelInterface*>(iface);
    if(smi)
      {
      item = smi->createPQProxy(group, name, proxy, server);
      if (item)
        {
        break;
        }
      }
    }

  if (!item)
    {
    return;
    }

  // Set the QObject parent, simplifying clean up of objects.
  item->setParent(this);

  emit this->preItemAdded(item);
  emit this->preProxyAdded(item);

  pqView* view = qobject_cast<pqView*>(item);
  pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
  pqRepresentation* repr = qobject_cast<pqRepresentation*>(item);

  if (view)
    {
    emit this->preViewAdded(view);
    }
  else if (source)
    {
    QObject::connect(source, 
      SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*, int)),
      this, SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*, int)));
    QObject::connect(source, 
      SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*, int)),
      this, SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*, int)));
    QObject::connect(source, 
      SIGNAL(preConnectionAdded(pqPipelineSource*, pqPipelineSource*, int)),
      this, SIGNAL(preConnectionAdded(pqPipelineSource*, pqPipelineSource*, int)));
    QObject::connect(source, 
      SIGNAL(preConnectionRemoved(pqPipelineSource*, pqPipelineSource*, int)),
      this, SIGNAL(preConnectionRemoved(pqPipelineSource*, pqPipelineSource*, int)));
    QObject::connect(source, SIGNAL(nameChanged(pqServerManagerModelItem*)), 
      this, SIGNAL(nameChanged(pqServerManagerModelItem*)));
    QObject::connect(
      source, SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
      this, SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)));
    QObject::connect(
      source, SIGNAL(dataUpdated(pqPipelineSource*)),
      this, SIGNAL(dataUpdated(pqPipelineSource*)));

    emit this->preSourceAdded(source);
    }
  else if (repr)
    {
    emit this->preRepresentationAdded(repr);
    }

  this->Internal->Proxies[proxy] = item;
  this->Internal->ItemList.push_back(item);

  emit this->itemAdded(item);
  emit this->proxyAdded(item);

  if (view)
    {
    emit this->viewAdded(view);
    }
  else if (source)
    {
    emit this->sourceAdded(source);
    }
  else if (repr)
    {
    emit this->representationAdded(repr);
    }

  // It is essential to let the world know of the addition of pqProxy
  // before we start emitting signals as we update the initial state 
  // of the pqProxy from its underlying proxy. Hence we emit this->proxyAdded()
  // before we do a pqSource->initialize();
  item->initialize();
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onProxyUnRegistered(const QString& group,
  const QString& name, vtkSMProxy* proxy)
{
  // Handle proxy renaming.
  pqProxy* item = this->findItem<pqProxy*>(proxy);
  if (!item || item->getSMName() != name || item->getSMGroup() != group)
    {
    // Proxy is unknown or being registered from a group/name for which we
    // didn't create any pq object.
    return;
    }

  // Verify if the proxy is registered under a different name in the same group.
  // If so, we are simply renaming the proxy.
  vtkSmartPointer<vtkStringList> names = vtkSmartPointer<vtkStringList>::New();
  vtkSMProxyManager* pxm = proxy->GetProxyManager();
  pxm->GetProxyNames(group.toAscii().data(), proxy, names);
  for (int cc=0; cc < names->GetLength(); cc++)
    {
    if (name == names->GetString(cc))
      {
      continue;
      }
    // Change the name of the pqsource.
    item->setSMName(names->GetString(cc));
    return;
    }

  pqView* view = qobject_cast<pqView*>(item);
  pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
  pqRepresentation* repr = qobject_cast<pqRepresentation*>(item);


  if (view)
    {
    emit this->preViewRemoved(view);
    }
  else if (source)
    {
    emit this->preSourceRemoved(source);
    }
  else if (repr)
    {
    emit this->preRepresentationRemoved(repr);
    }

  emit this->preProxyRemoved(item);
  emit this->preItemRemoved(item);

  QObject::disconnect(item, 0, this, 0);
  this->Internal->ItemList.removeAll(item);
  this->Internal->Proxies.remove(item->getProxy());

  if (view)
    {
    emit this->viewRemoved(view);
    }
  else if (source)
    {
    emit this->sourceRemoved(source);
    }
  else if (repr)
    {
    emit this->representationRemoved(repr);
    }

  emit this->proxyRemoved(item);
  emit this->itemRemoved(item);
  delete item;
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onConnectionCreated(vtkIdType id)
{
  // Avoid duplicate server creations.
  if (this->findServer(id))
    {
    return; // server already exists.
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pqServer* server = new pqServer(id, pm->GetOptions(), this);
  server->setResource(this->Internal->ActiveResource);
  this->Internal->ActiveResource = pqServerResource();

  emit this->preItemAdded(server);
  emit this->preServerAdded(server);

  this->Internal->Servers[id] = server;
  this->Internal->ItemList.push_back(server);

  // Lets the world know when the server name changes.
  this->connect(server, SIGNAL(nameChanged(pqServerManagerModelItem*)), 
    this, SIGNAL(nameChanged(pqServerManagerModelItem*)));
  
  server->initialize();

  emit this->itemAdded(server);
  emit this->serverAdded(server);
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onConnectionClosed(vtkIdType id)
{
  pqServer* server = this->findServer(id);
  if (!server)
    {
    qDebug() << "Unknown connection closed, simply ignoring it.";
    return;
    }

  emit this->preServerRemoved(server);
  emit this->preItemRemoved(server);

  this->Internal->Servers.remove(server->GetConnectionID());
  this->Internal->ItemList.removeAll(server);

  emit this->serverRemoved(server);
  emit this->itemRemoved(server);
  delete server;
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onStateLoaded(vtkPVXMLElement* root,
  vtkSMProxyLocator* locator)
{
  pqHelperProxyStateLoader loader;
  loader.loadState(root, locator);
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::beginRemoveServer(pqServer *server)
{
  emit this->aboutToRemoveServer(server);
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::endRemoveServer()
{
  emit this->finishedRemovingServer();
}


