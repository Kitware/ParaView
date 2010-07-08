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
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"

#include <QMap>
#include <QList>
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
  this->clearHelperProxies();
  delete this->Internal;
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
    QString groupname = QString("pq_helper_proxies.%1").arg(
      this->getProxy()->GetSelfIDAsString());

    this->Internal->ProxyLists[key].push_back(proxy);
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
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

  if (this->Internal->ProxyLists.contains(key))
    {
    this->Internal->ProxyLists[key].removeAll(proxy);


    QString groupname = QString("pq_helper_proxies.%1").arg(
      this->getProxy()->GetSelfIDAsString());
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
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
    this->getProxy()->GetSelfIDAsString());
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();
  iter->SetModeToOneGroup();
  for (iter->Begin(groupname.toAscii().data()); !iter->IsAtEnd(); iter->Next())
    {
    const char* key = iter->GetKey();
    vtkSMProxy* proxy = iter->GetProxy();
    if (proxy != NULL && !this->Internal->ProxyLists[key].contains(proxy))
      {
      this->Internal->ProxyLists[key].push_back(proxy);
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void pqProxy::clearHelperProxies()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (pxm)
    {
    QString groupname = QString("pq_helper_proxies.%1").arg(
      this->getProxy()->GetSelfIDAsString());

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
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
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



