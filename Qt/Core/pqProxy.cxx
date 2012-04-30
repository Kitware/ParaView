/*=========================================================================

   Program: ParaView
   Module:    pqProxy.cxx

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
#include "pqProxy.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include "pqApplicationCore.h"
#include "pqHelperProxyRegisterUndoElement.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqUndoStack.h"

#include <QMap>
#include <QList>
#include <QString>
#include <QStringList>
#include <QtDebug>

//-----------------------------------------------------------------------------
class pqProxyInternal
{
public:
  pqProxyInternal()
    {
    this->Connection = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
  typedef QMap<QString, QList<vtkSmartPointer<vtkSMProxy> > > ProxyListsType;
  ProxyListsType ProxyLists;
  vtkSmartPointer<vtkSMProxy> Proxy;
  vtkSmartPointer<vtkEventQtSlotConnect> Connection;
};

//-----------------------------------------------------------------------------
pqProxy::pqProxy(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* _parent/*=NULL*/) 
: pqServerManagerModelItem(_parent),
  Server(server),
  SMName(name),
  SMGroup(group)
{
  this->Internal = new pqProxyInternal;
  this->Internal->Proxy = proxy;
  this->Modified = pqProxy::UNMODIFIED;
}

//-----------------------------------------------------------------------------
pqProxy::~pqProxy()
{
  // Attach listener for proxy registration to handle helper proxy
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerObserver* observer = core->getServerManagerObserver();
  QObject::disconnect(observer,
                      SIGNAL(proxyRegistered(const QString&, const QString&, vtkSMProxy*)),
                      this,
                      SLOT(onProxyRegistered(const QString&, const QString&, vtkSMProxy*)));
  QObject::disconnect(observer,
                      SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)),
                      this,
                      SLOT(onProxyUnRegistered(const QString&, const QString&, vtkSMProxy*)));

  this->clearHelperProxies();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqServer* pqProxy::getServer() const
{
  return this->Server;
}

//-----------------------------------------------------------------------------
void pqProxy::addHelperProxy(const QString& key, vtkSMProxy* proxy)
{
  bool already_added = false;
  if (this->Internal->ProxyLists.contains(key))
    {
    already_added = this->Internal->ProxyLists[key].contains(proxy);
    }

  if (!already_added)
    {
    // We call that method so sub-class can update domain or do what ever...
    this->addInternalHelperProxy(key, proxy);

    QString groupname = QString("pq_helper_proxies.%1").arg(
      this->getProxy()->GetGlobalIDAsString());

    vtkSMSessionProxyManager* pxm = this->proxyManager();
    pxm->RegisterProxy(groupname.toAscii().data(), 
      key.toAscii().data(), proxy);
    }
}

//-----------------------------------------------------------------------------
void pqProxy::removeHelperProxy(const QString& key, vtkSMProxy* proxy)
{
  if (!proxy)
    {
    qDebug() << "proxy argument to pqProxy::removeHelperProxy cannot be 0.";
    return;
    }

  // We call that method so sub-class can update domain or do what ever...
  this->removeInternalHelperProxy(key, proxy);

  if (this->Internal->ProxyLists.contains(key))
    {
    QString groupname = QString("pq_helper_proxies.%1").arg(
      this->getProxy()->GetGlobalIDAsString());
    vtkSMSessionProxyManager* pxm = this->proxyManager();
    const char* name = pxm->GetProxyName(groupname.toAscii().data(), proxy);
    if (name)
      {
      pxm->UnRegisterProxy(groupname.toAscii().data(), name, proxy);
      }
    }
}

//-----------------------------------------------------------------------------
void pqProxy::updateHelperProxies() const
{
  QString groupname = QString("pq_helper_proxies.%1").arg(
    this->getProxy()->GetGlobalIDAsString());
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();
  iter->SetModeToOneGroup();
  iter->SetSession(this->getProxy()->GetSession());
  for (iter->Begin(groupname.toAscii().data()); !iter->IsAtEnd(); iter->Next())
    {
    this->addInternalHelperProxy(QString(iter->GetKey()), iter->GetProxy());
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void pqProxy::clearHelperProxies()
{
  if ( this->getServer() && this->getServer()->session() &&
       !this->getServer()->session()->IsMultiClients())
    {
    // This is sort-of-a-hack to ensure that when this operation (delete)
    // is undo, all the helper proxies are discovered correctly. This needs to
    // happen only when all helper proxies are still around.
    pqHelperProxyRegisterUndoElement* elem =
        pqHelperProxyRegisterUndoElement::New();
    elem->SetOperationTypeToUndo(); // Undo deletion
    elem->RegisterHelperProxies(this);
    ADD_UNDO_ELEM(elem);
    elem->Delete();
    }

  vtkSMSessionProxyManager* pxm = this->proxyManager();
  if (pxm)
    {
    QString groupname = QString("pq_helper_proxies.%1").arg(
      this->getProxy()->GetGlobalIDAsString());

    pqProxyInternal::ProxyListsType::iterator iter
      = this->Internal->ProxyLists.begin();
    for (;iter != this->Internal->ProxyLists.end(); ++iter)
      {
      foreach(vtkSMProxy* proxy, iter.value())
        {
        const char* name = pxm->GetProxyName(
          groupname.toAscii().data(), proxy);
        if (name)
          {
          pxm->UnRegisterProxy(groupname.toAscii().data(), name, proxy);
          }
        }
      }
    }

  this->Internal->ProxyLists.clear();
}


//-----------------------------------------------------------------------------
QList<QString> pqProxy::getHelperKeys() const
{
  this->updateHelperProxies();
  return this->Internal->ProxyLists.keys();
}

//-----------------------------------------------------------------------------
QList<vtkSMProxy*> pqProxy::getHelperProxies(const QString& key) const
{
  this->updateHelperProxies();

  QList<vtkSMProxy*> list;

  if (this->Internal->ProxyLists.contains(key))
    {
    foreach( vtkSMProxy* proxy, this->Internal->ProxyLists[key])
      {
      list.push_back(proxy);
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
QList<vtkSMProxy*> pqProxy::getHelperProxies() const
{
  this->updateHelperProxies();

  QList<vtkSMProxy*> list;

  pqProxyInternal::ProxyListsType::iterator iter
      = this->Internal->ProxyLists.begin();
    for (;iter != this->Internal->ProxyLists.end(); ++iter)
      {
      foreach( vtkSMProxy* proxy, iter.value())
        {
        list.push_back(proxy);
        }
      }
  return list;
}

//-----------------------------------------------------------------------------
void pqProxy::rename(const QString& newname)
{
  if(newname != this->SMName)
    {
    vtkSMSessionProxyManager* pxm = this->proxyManager();
    pxm->RegisterProxy(this->getSMGroup().toAscii().data(),
      newname.toAscii().data(), this->getProxy());
    pxm->UnRegisterProxy(this->getSMGroup().toAscii().data(),
      this->getSMName().toAscii().data(), this->getProxy());
    this->SMName = newname;
    }
}
  
//-----------------------------------------------------------------------------
void pqProxy::setSMName(const QString& name)
{
  if (!name.isEmpty() && this->SMName != name)
    {
    this->SMName = name; 
    emit this->nameChanged(this);
    }
}

//-----------------------------------------------------------------------------
const QString& pqProxy::getSMName()
{ 
  return this->SMName;
}

//-----------------------------------------------------------------------------
const QString& pqProxy::getSMGroup()
{
  return this->SMGroup;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqProxy::getProxy() const
{
  return this->Internal->Proxy;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pqProxy::getHints() const
{
  return this->Internal->Proxy->GetHints();
}

//-----------------------------------------------------------------------------
void pqProxy::setModifiedState(ModifiedState modified)
{
  if(modified != this->Modified)
    {
    this->Modified = modified;
    emit this->modifiedStateChanged(this);
    }
}

//-----------------------------------------------------------------------------
void pqProxy::setDefaultPropertyValues()
{
  vtkSMProxy* proxy = this->getProxy();

  // If this is a compound proxy, its property values will be set from XML
  // This seems like a hack. Need some graceful solution.
  if(proxy->IsA("vtkSMCompoundSourceProxy"))
    {
    return;
    }

  // since some domains rely on information properties,
  // it is essential that we update the property information 
  // before resetting values.
  proxy->UpdatePropertyInformation();

  vtkSMPropertyIterator* iter = proxy->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* smproperty = iter->GetProperty();

    if (!smproperty->GetInformationOnly())
      {
      vtkPVXMLElement* propHints = iter->GetProperty()->GetHints();
      if (propHints && propHints->FindNestedElementByName("NoDefault"))
        {
        // Don't reset properties that request overriding of default mechanism.
        continue;
        }
      iter->GetProperty()->ResetToDefault();
      iter->GetProperty()->UpdateDependentDomains();
      }
    }

  // Since domains may depend on defaul values of other properties to be set,
  // we iterate over the properties once more. We need a better mechanism for this.
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* smproperty = iter->GetProperty();

    if (!smproperty->GetInformationOnly())
      {
      vtkPVXMLElement* propHints = iter->GetProperty()->GetHints();
      if (propHints && propHints->FindNestedElementByName("NoDefault"))
        {
        // Don't reset properties that request overriding of default mechanism.
        continue;
        }
      iter->GetProperty()->ResetToDefault();
      iter->GetProperty()->UpdateDependentDomains();
      }
    }

  iter->Delete();
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
vtkSMSessionProxyManager* pqProxy::proxyManager() const
{
  return this->Internal->Proxy ?
         this->Internal->Proxy->GetSessionProxyManager() : NULL;
}
//-----------------------------------------------------------------------------
void pqProxy::initialize()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerObserver* observer = core->getServerManagerObserver();

  // Attach listener for proxy registration to handle helper proxy
  QObject::connect(observer,
                   SIGNAL(proxyRegistered(const QString&, const QString&, vtkSMProxy*)),
                   this,
                   SLOT(onProxyRegistered(const QString&, const QString&, vtkSMProxy*)));
  QObject::connect(observer,
                   SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)),
                   this,
                   SLOT(onProxyUnRegistered(const QString&, const QString&, vtkSMProxy*)));

  // Update helper proxy if any of them are already registered in ProxyManager
  this->updateHelperProxies();
}
//-----------------------------------------------------------------------------
void pqProxy::addInternalHelperProxy(const QString& key, vtkSMProxy* proxy) const
{
  if(!proxy || this->Internal->ProxyLists[key].contains(proxy))
    {
    return; // No proxy to add
    }

  this->Internal->ProxyLists[key].push_back(proxy);
}
//-----------------------------------------------------------------------------
void pqProxy::removeInternalHelperProxy(const QString& key, vtkSMProxy* proxy) const
{
  if (this->Internal->ProxyLists.contains(key))
    {
    this->Internal->ProxyLists[key].removeAll(proxy);
    }
}
//-----------------------------------------------------------------------------
void pqProxy::onProxyRegistered(const QString& group, const QString& name, vtkSMProxy* proxy)
{
  if(group == QString("pq_helper_proxies.%1").arg(this->getProxy()->GetGlobalIDAsString()))
    {
    this->addInternalHelperProxy(name, proxy);
    }
}
//-----------------------------------------------------------------------------
void pqProxy::onProxyUnRegistered(const QString& group, const QString& name, vtkSMProxy* proxy)
{
  if(group == QString("pq_helper_proxies.%1").arg(this->getProxy()->GetGlobalIDAsString()))
    {
    this->removeInternalHelperProxy(name, proxy);
    }
}
