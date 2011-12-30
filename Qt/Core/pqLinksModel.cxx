/*=========================================================================

   Program:   ParaView
   Module:    pqLinksModel.cxx

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

=========================================================================*/

#include "pqLinksModel.h"

// Qt includes
#include <QPointer>

// vtk includes
#include <vtkEventQtSlotConnect.h>
#include <vtkSmartPointer.h>

// Server manager includes
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMProxyLink.h"
#include "vtkSMCameraLink.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxyListDomain.h"

// pqCore includes
#include "pqApplicationCore.h"
#include "pqProxy.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

class pqLinksModel::pqInternal : public vtkCommand
{
public:
  pqLinksModel* Model;
  QPointer<pqServer> Server;
  static pqInternal* New() { return new pqInternal; }
  static const char* columnHeaders[];

  void Execute(vtkObject*, unsigned long eid, void* callData)
    {
    vtkSMProxyManager::RegisteredProxyInformation *info =
      reinterpret_cast<vtkSMProxyManager::RegisteredProxyInformation *>(callData);

    if(!info ||
      info->Type != vtkSMProxyManager::RegisteredProxyInformation::LINK)
      {
      return;
      }

    QString linkName = info->ProxyName;

    if(eid == vtkCommand::RegisterEvent)
      {
      this->LinkObjects.append(
        new pqLinksModelObject(linkName, this->Model, this->Server));
      this->Model->reset();
      }
    else if(eid == vtkCommand::UnRegisterEvent)
      {
      QList<pqLinksModelObject*>::iterator iter;
      for(iter = this->LinkObjects.begin();
          iter != this->LinkObjects.end();
          ++iter)
        {
        if((*iter)->name() == linkName)
          {
          delete *iter;
          this->LinkObjects.erase(iter);
          this->Model->reset();
          break;
          }
        }
      }
    }

protected:
  pqInternal() {}
  ~pqInternal() {}
  QList<pqLinksModelObject*> LinkObjects;

private:
  pqInternal(const pqInternal&);
  pqInternal& operator=(const pqInternal&);
};

const char* pqLinksModel::pqInternal::columnHeaders[] =
{
  "Name",
  "Object 1",
  "Property",
  "Object 2",
  "Property"
};


/// construct a links model
pqLinksModel::pqLinksModel(QObject *p)
  : Superclass(p)
{
  this->Internal = pqInternal::New();
  this->Internal->Model = this;

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(serverAdded(pqServer*)),
    this, SLOT(onSessionCreated(pqServer*)));
  QObject::connect(smmodel, SIGNAL(serverRemoved(pqServer*)),
    this, SLOT(onSessionRemoved(pqServer*)));
}

/// destruct a links model
pqLinksModel::~pqLinksModel()
{
  this->Internal->Model = NULL;
  this->Internal->Delete();
}

void pqLinksModel::onSessionCreated(pqServer* server)
{
  this->Internal->Server = server;
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  pxm->AddObserver(vtkCommand::RegisterEvent, this->Internal);
  pxm->AddObserver(vtkCommand::UnRegisterEvent, this->Internal);
}

void pqLinksModel::onSessionRemoved(pqServer* server)
{
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  pxm->RemoveObserver(this->Internal);
}

static vtkSMProxy* getProxyFromLink(vtkSMProxyLink* link, int desiredDir)
{
  int numLinks = link->GetNumberOfLinkedProxies();
  for(int i=0; i<numLinks; i++)
    {
    vtkSMProxy* proxy = link->GetLinkedProxy(i);
    int dir = link->GetLinkedProxyDirection(i);
    if(dir == desiredDir)
      {
      return proxy;
      }
    }
  return NULL;
}

static vtkSMProxy* getProxyFromLink(vtkSMPropertyLink* link, int desiredDir)
{
  int numLinks = link->GetNumberOfLinkedProperties();
  for(int i=0; i<numLinks; i++)
    {
    vtkSMProxy* proxy = link->GetLinkedProxy(i);
    int dir = link->GetLinkedPropertyDirection(i);
    if(dir == desiredDir)
      {
      return proxy;
      }
    }
  return NULL;
}
 
// TODO: fix this so it isn't dependent on the order of links
QString pqLinksModel::getPropertyFromIndex(const QModelIndex& idx,
                                           int desiredDir) const
{
  QString name = this->getLinkName(idx);
  vtkSMLink* link = this->getLink(name);
  vtkSMPropertyLink* propertyLink = vtkSMPropertyLink::SafeDownCast(link);

  if(propertyLink)
    {
    int numLinks = propertyLink->GetNumberOfLinkedProperties();
    for(int i=0; i<numLinks; i++)
      {
      int dir = propertyLink->GetLinkedPropertyDirection(i);
      if(dir == desiredDir)
        {
        return propertyLink->GetLinkedPropertyName(i);
        }
      }
    }
  return QString();
}

// TODO: fix this so it isn't dependent on the order of links
vtkSMProxy* pqLinksModel::getProxyFromIndex(const QModelIndex& idx, int dir) const
{
  QString name = this->getLinkName(idx);
  vtkSMLink* link = this->getLink(name);
  vtkSMPropertyLink* propertyLink = vtkSMPropertyLink::SafeDownCast(link);
  vtkSMProxyLink* proxyLink = vtkSMProxyLink::SafeDownCast(link);
    
  if(proxyLink)
    {
    return getProxyFromLink(proxyLink, dir);
    }
  else if(propertyLink)
    {
    return getProxyFromLink(propertyLink, dir);
    }

  return NULL;
}

vtkSMProxy* pqLinksModel::getProxy1(const QModelIndex& idx) const
{
  return this->getProxyFromIndex(idx, vtkSMLink::INPUT);
}

vtkSMProxy* pqLinksModel::getProxy2(const QModelIndex& idx) const
{
  return this->getProxyFromIndex(idx, vtkSMLink::OUTPUT);
}
  
QString pqLinksModel::getProperty1(const QModelIndex& idx) const
{
  return this->getPropertyFromIndex(idx, vtkSMLink::INPUT);
}

QString pqLinksModel::getProperty2(const QModelIndex& idx) const
{
  return this->getPropertyFromIndex(idx, vtkSMLink::OUTPUT);
}


// implementation to satisfy api
int pqLinksModel::rowCount(const QModelIndex&) const
{
  return this->Internal->Server?
    this->Internal->Server->proxyManager()->GetNumberOfLinks() : 0;
}

int pqLinksModel::columnCount(const QModelIndex&) const
{
  // name, master object, property, slave object, property
  return sizeof(pqInternal::columnHeaders) / sizeof(char*);
}

QVariant pqLinksModel::data(const QModelIndex &idx, int role) const
{
  if(role == Qt::DisplayRole)
    {
    QString name = this->getLinkName(idx);
    vtkSMLink* link = this->getLink(name);
    ItemType type = this->getLinkType(link);

    if(idx.column() == 0)
      {
      return name == QString::null ? "Unknown" : name;
      }
    else if(idx.column() == 1)
      {
      vtkSMProxy* pxy = this->getProxy1(idx);
      pqProxy* qpxy = this->representativeProxy(pxy);
      return qpxy ? qpxy->getSMName() : "Unknown";
      }
    else if(idx.column() == 2)
      {
      vtkSMProxy* pxy = this->getProxy1(idx);
      pqProxy* qpxy = this->representativeProxy(pxy);
      if(type == pqLinksModel::Proxy && qpxy->getProxy() == pxy)
        {
        return "All";
        }
      else if(type == pqLinksModel::Proxy && qpxy)
        {
        vtkSMProxyListDomain* d = this->proxyListDomain(qpxy->getProxy());
        if(d)
          {
          int numProxies = d->GetNumberOfProxies();
          for(int i=0; i<numProxies; i++)
            {
            if(pxy == d->GetProxy(i))
              {
              return d->GetProxyName(i);
              }
            }
          }
        }
      QString prop = this->getProperty1(idx);
      return prop.isEmpty() ? "Unknown" : prop; 
      }
    else if(idx.column() == 3)
      {
      vtkSMProxy* pxy = this->getProxy2(idx);
      pqProxy* qpxy = this->representativeProxy(pxy);
      return qpxy ? qpxy->getSMName() : "Unknown";
      }
    else if(idx.column() == 4)
      {
      vtkSMProxy* pxy = this->getProxy2(idx);
      pqProxy* qpxy = this->representativeProxy(pxy);
      if(type == pqLinksModel::Proxy && qpxy->getProxy() == pxy)
        {
        return "All";
        }
      else if(type == pqLinksModel::Proxy && qpxy)
        {
        vtkSMProxyListDomain* d = this->proxyListDomain(qpxy->getProxy());
        if(d)
          {
          int numProxies = d->GetNumberOfProxies();
          for(int i=0; i<numProxies; i++)
            {
            if(pxy == d->GetProxy(i))
              {
              return d->GetProxyName(i);
              }
            }
          }
        }
      QString prop = this->getProperty2(idx);
      return prop.isEmpty() ? "Unknown" : prop; 
      }
    }
  return QVariant();
}

QVariant pqLinksModel::headerData(int section, Qt::Orientation orient, 
                                  int role) const
{
  if(role == Qt::DisplayRole && 
     orient == Qt::Horizontal &&
     section >= 0 && 
     section < this->columnCount())
    {
    // column headers
    return QString(pqInternal::columnHeaders[section]);
    }
  else if(role == Qt::DisplayRole && orient == Qt::Vertical)
    {
    // row headers, just use numbers 1-n
    return QString("%1").arg(section+1);
    }
  
  return QVariant();
}

QString pqLinksModel::getLinkName(const QModelIndex& idx) const
{
  if (this->Internal->Server)
    {
    vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
    QString linkName = pxm->GetLinkName(idx.row());
    return linkName;
    }
  return QString();
}

QModelIndex pqLinksModel::findLink(vtkSMLink* link) const
{
  int numRows = this->rowCount();
  for(int i=0; i<numRows; i++)
    {
    QModelIndex idx = this->index(i, 0, QModelIndex());
    if(this->getLink(idx) == link)
      {
      return idx;
      }
    }
  return QModelIndex();
}

vtkSMLink* pqLinksModel::getLink(const QString& name) const
{
  if (this->Internal->Server)
    {
    vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
    vtkSMLink* link = pxm->GetRegisteredLink(name.toAscii().data());
    return link;
    }
  return NULL;
}

vtkSMLink* pqLinksModel::getLink(const QModelIndex& idx) const
{
  return this->getLink(this->getLinkName(idx));
}

pqLinksModel::ItemType pqLinksModel::getLinkType(vtkSMLink* link) const
{
  if(vtkSMPropertyLink::SafeDownCast(link))
    {
    return Property;
    }
  else if(vtkSMCameraLink::SafeDownCast(link))
    {
    return Camera;
    }
  else if(vtkSMProxyLink::SafeDownCast(link))
    {
    return Proxy;
    }
  return Unknown;
}

pqLinksModel::ItemType pqLinksModel::getLinkType(const QModelIndex& idx) const
{
  vtkSMLink* link = this->getLink(idx);
  return this->getLinkType(link);
}


void pqLinksModel::addProxyLink(const QString& name,
                                   vtkSMProxy* inputProxy,
                                   vtkSMProxy* outputProxy)
{
  vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
  vtkSMProxyLink* link = vtkSMProxyLink::New();
  // bi-directional link
  link->AddLinkedProxy(inputProxy, vtkSMLink::INPUT);
  link->AddLinkedProxy(outputProxy, vtkSMLink::OUTPUT);
  link->AddLinkedProxy(outputProxy, vtkSMLink::INPUT);
  link->AddLinkedProxy(inputProxy, vtkSMLink::OUTPUT);
  
  // any proxy property doesn't participate in the link
  // instead, these proxies are linkable themselves
  vtkSMPropertyIterator *iter = vtkSMPropertyIterator::New();
  iter->SetProxy(inputProxy);
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    if(vtkSMProxyProperty::SafeDownCast(iter->GetProperty()))
      {
      link->AddException(iter->GetKey());
      }
    }
  iter->Delete();

  pxm->RegisterLink(name.toAscii().data(), link);
  link->Delete();
}

void pqLinksModel::addCameraLink(const QString& name,
                                 vtkSMProxy* inputProxy,
                                 vtkSMProxy* outputProxy)
{
  vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
  vtkSMCameraLink* link = vtkSMCameraLink::New();
  // bi-directional link
  link->AddLinkedProxy(inputProxy, vtkSMLink::INPUT);
  link->AddLinkedProxy(outputProxy, vtkSMLink::OUTPUT);
  link->AddLinkedProxy(outputProxy, vtkSMLink::INPUT);
  link->AddLinkedProxy(inputProxy, vtkSMLink::OUTPUT);
  pxm->RegisterLink(name.toAscii().data(), link);
  link->Delete();
}

void pqLinksModel::addPropertyLink(const QString& name,
                                      vtkSMProxy* inputProxy,
                                      const QString& inputProp,
                                      vtkSMProxy* outputProxy,
                                      const QString& outputProp)
{
  vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
  vtkSMPropertyLink* link = vtkSMPropertyLink::New();
  
  // bi-directional link
  link->AddLinkedProperty(inputProxy,
                          inputProp.toAscii().data(),
                          vtkSMLink::INPUT);
  link->AddLinkedProperty(outputProxy,
                          outputProp.toAscii().data(),
                          vtkSMLink::OUTPUT);
  link->AddLinkedProperty(outputProxy,
                          outputProp.toAscii().data(),
                          vtkSMLink::INPUT);
  link->AddLinkedProperty(inputProxy,
                          inputProp.toAscii().data(),
                          vtkSMLink::OUTPUT);
  pxm->RegisterLink(name.toAscii().data(), link);
  link->Delete();
  
}

void pqLinksModel::removeLink(const QModelIndex& idx)
{
  if(!idx.isValid())
    {
    return;
    }
  
  // we want an index for the first column
  QModelIndex removeIdx = this->index(idx.row(), 0, idx.parent());
  // get the name from the first column
  QString name = this->data(removeIdx, Qt::DisplayRole).toString();
  
  this->removeLink(name);
}
  
void pqLinksModel::removeLink(const QString& name)
{
  if(name != QString::null)
    {
    vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
    pxm->UnRegisterLink(name.toAscii().data());
    }
}

pqProxy* pqLinksModel::representativeProxy(vtkSMProxy* pxy)
{
  // assume internal proxies don't have pqProxy counterparts
  pqServerManagerModel* smModel =
    pqApplicationCore::instance()->getServerManagerModel();
  pqProxy* rep = smModel->findItem<pqProxy*>(pxy);
  
  if(!rep)
    {
    // get the owner of this internal proxy
    int numConsumers = pxy->GetNumberOfConsumers();
    for(int i=0; rep == NULL && i<numConsumers; i++)
      {
      vtkSMProxy* consumer = pxy->GetConsumerProxy(i);
      rep = smModel->findItem<pqProxy*>(consumer);
      }
    }
  return rep;
}
  
vtkSMProxyListDomain* pqLinksModel::proxyListDomain(
  vtkSMProxy* pxy)
{
  vtkSMProxyListDomain* pxyDomain = NULL;

  if(pxy == NULL)
    {
    return NULL;
    }
  
  vtkSMPropertyIterator* iter = vtkSMPropertyIterator::New();
  iter->SetProxy(pxy);
  for(iter->Begin(); 
      pxyDomain == NULL && !iter->IsAtEnd(); 
      iter->Next())
    {
    vtkSMProxyProperty* pxyProperty =
      vtkSMProxyProperty::SafeDownCast(iter->GetProperty());
    if(pxyProperty)
      {
      pxyDomain = vtkSMProxyListDomain::SafeDownCast(
        pxyProperty->GetDomain("proxy_list"));
      }
    }
  iter->Delete();
  return pxyDomain;
}


class pqLinksModelObject::pqInternal
{
public:
  QPointer<pqServer> Server;
  // a list of proxies involved in the link
  QList<pqProxy*> OutputProxies;
  QList<pqProxy*> InputProxies;
  vtkSmartPointer<vtkEventQtSlotConnect> Connection;
  // name of link
  QString Name;
  vtkSmartPointer<vtkSMLink> Link;
  bool Setting;
};


pqLinksModelObject::pqLinksModelObject(QString linkName, pqLinksModel* p,
  pqServer* server)
  : QObject(p)
{
  this->Internal = new pqInternal;
  this->Internal->Connection = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Internal->Server = server;
  this->Internal->Name = linkName;
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  this->Internal->Link = pxm->GetRegisteredLink(linkName.toAscii().data());
  this->Internal->Setting = false;
  this->Internal->Connection->Connect(this->Internal->Link, 
                            vtkCommand::ModifiedEvent,
                            this, SLOT(refresh()));
  this->refresh();
}

pqLinksModelObject::~pqLinksModelObject()
{
  if(vtkSMCameraLink::SafeDownCast(this->Internal->Link))
    {
    foreach(pqProxy* p, this->Internal->InputProxies)
      {
      // For render module links, we have to ensure that we remove
      // the links between their interaction undo stacks as well.
      pqRenderView* ren = qobject_cast<pqRenderView*>(p);
      if (ren)
        {
        this->unlinkUndoStacks(ren);
        }
      }
    }

  delete this->Internal;
}
  
QString pqLinksModelObject::name() const
{
  return this->Internal->Name;
}

vtkSMLink* pqLinksModelObject::link() const
{
  return this->Internal->Link;
}

void pqLinksModelObject::proxyModified(pqServerManagerModelItem* item)
{
  if(this->Internal->Setting)
    {
    return;
    }

  this->Internal->Setting = true;
  pqProxy* source = qobject_cast<pqProxy*>(item);
  if(source && source->modifiedState() == pqProxy::MODIFIED)
    {
    foreach(pqProxy* p, this->Internal->OutputProxies)
      {
      if(p != source && !p->modifiedState() != pqProxy::MODIFIED)
        {
        p->setModifiedState(pqProxy::MODIFIED);
        }
      }
    }
  this->Internal->Setting = false;
}

void pqLinksModelObject::remove()
{
  vtkSMSessionProxyManager* pxm = this->Internal->Server->proxyManager();
  pxm->UnRegisterLink(this->name().toAscii().data());
}


void pqLinksModelObject::unlinkUndoStacks(pqRenderView* ren)
{
  foreach (pqProxy* output, this->Internal->OutputProxies)
    {
    // assume all are render modules because some might be deleted already
    pqRenderView* other = static_cast<pqRenderView*>(output);
    if (other && other != ren)
      {
      ren->unlinkUndoStack(other);
      }
    }
}


void pqLinksModelObject::linkUndoStacks()
{
  foreach (pqProxy* proxy, this->Internal->InputProxies)
    {
    pqRenderView* src = qobject_cast<pqRenderView*>(proxy);
    if (src)
      {
      for (int cc=0; cc < this->Internal->OutputProxies.size(); cc++)
        {
        pqRenderView* dest = qobject_cast<pqRenderView*>(
          this->Internal->OutputProxies[cc]);
        if (dest && src != dest)
          {
          src->linkUndoStack(dest);
          }
        }
      }
    }
}

void pqLinksModelObject::refresh()
{
  foreach(pqProxy* p, this->Internal->InputProxies)
    {
    QObject::disconnect(p,
      SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
      this, SLOT(proxyModified(pqServerManagerModelItem*)));

    // For render module links, we have to ensure that we remove
    // the links between their interaction undo stacks as well.
    pqRenderView* ren = qobject_cast<pqRenderView*>(p);
    if (ren)
      {
      this->unlinkUndoStacks(ren);
      }
    }

  this->Internal->InputProxies.clear();
  this->Internal->OutputProxies.clear();

  vtkSMProxyLink* proxyLink = vtkSMProxyLink::SafeDownCast(this->link());
  vtkSMPropertyLink* propertyLink = vtkSMPropertyLink::SafeDownCast(this->link());

  QList<vtkSMProxy*> tmpInputs, tmpOutputs;

  if(proxyLink)
    {
    int numLinks = proxyLink->GetNumberOfLinkedProxies();
    for(int i=0; i<numLinks; i++)
      {
      vtkSMProxy* pxy = proxyLink->GetLinkedProxy(i);
      int dir = proxyLink->GetLinkedProxyDirection(i);
      if(dir == vtkSMLink::INPUT)
        {
        tmpInputs.append(pxy);
        }
      else if(dir == vtkSMLink::OUTPUT)
        {
        tmpOutputs.append(pxy);
        }
      }
    }
  else if(propertyLink)
    {
    int numLinks = propertyLink->GetNumberOfLinkedProperties();
    for(int i=0; i<numLinks; i++)
      {
      vtkSMProxy* pxy = propertyLink->GetLinkedProxy(i);
      int dir = propertyLink->GetLinkedPropertyDirection(i);
      if(dir == vtkSMLink::INPUT)
        {
        tmpInputs.append(pxy);
        }
      else if(dir == vtkSMLink::OUTPUT)
        {
        tmpOutputs.append(pxy);
        }
      }
    }
  else
    {
    qWarning("Unhandled proxy type\n");
    }

  foreach(vtkSMProxy* p, tmpInputs)
    {
    pqProxy* pxy = pqLinksModel::representativeProxy(p);
    if(pxy)
      {
      this->Internal->InputProxies.append(pxy);
      QObject::connect(pxy,
        SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
        this, SLOT(proxyModified(pqServerManagerModelItem*)));
      QObject::connect(pxy, SIGNAL(destroyed(QObject*)),
        this, SLOT(remove()));
      }
    }
  
  foreach(vtkSMProxy* p, tmpOutputs)
    {
    pqProxy* pxy = pqLinksModel::representativeProxy(p);
    if(pxy)
      {
      this->Internal->OutputProxies.append(pxy);
      QObject::connect(pxy, SIGNAL(destroyed(QObject*)),
        this, SLOT(remove()));
      }
    }

  if (vtkSMCameraLink::SafeDownCast(this->link()))
    {
    this->linkUndoStacks();
    }
}


