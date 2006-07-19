/*=========================================================================

   Program: ParaView
   Module:    pqProxy.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "vtkSmartPointer.h"
#include "vtkSMProxyManager.h"


#include <QMap>
#include <QList>
#include <QtDebug>


//-----------------------------------------------------------------------------
class pqProxyInternal
{
public:
  typedef QMap<QString, QList<vtkSmartPointer<vtkSMProxy> > > ProxyListsType;
  ProxyListsType ProxyLists;
};

//-----------------------------------------------------------------------------
pqProxy::pqProxy(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* _parent/*=NULL*/) 
: pqServerManagerModelItem(_parent),
  Server(server),
  ProxyName(name),
  SMName(name),
  SMGroup(group),
  Proxy(proxy)
{
  this->Internal = new pqProxyInternal;

}

pqProxy::~pqProxy()
{
  this->clearInternalProxies();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqProxy::addInternalProxy(const QString& key,
  vtkSMProxy* proxy)
{
  QString group_name = QString(this->getProxy()->GetXMLName()) + "_" + key;

  bool already_added = false;
  if (this->Internal->ProxyLists.contains(key))
    {
    already_added = this->Internal->ProxyLists[key].contains(proxy);
    }

  if (!already_added)
    {
    this->Internal->ProxyLists[key].push_back(proxy);
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    pxm->RegisterProxy(group_name.toStdString().c_str(),
      proxy->GetSelfIDAsString(), proxy);
    }
}

//-----------------------------------------------------------------------------
void pqProxy::removeInternalProxy(const QString& key,
  vtkSMProxy* proxy)
{
  if (!proxy)
    {
    qDebug() << "proxy argument to pqProxy::removeInternalProxy cannot be 0.";
    return;
    }

  QString group_name = QString(this->getProxy()->GetXMLName()) + "_" + key;
  if (this->Internal->ProxyLists.contains(key))
    {
    this->Internal->ProxyLists[key].removeAll(proxy);
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    pxm->RegisterProxy(group_name.toStdString().c_str(),
      proxy->GetSelfIDAsString(), proxy);
    }
}

//-----------------------------------------------------------------------------
QList<vtkSMProxy*> pqProxy::getInternalProxies(const QString& key) const
{
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
void pqProxy::clearInternalProxies()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (pxm)
    {
    pqProxyInternal::ProxyListsType::iterator iter
      = this->Internal->ProxyLists.begin();
    for (;iter != this->Internal->ProxyLists.end(); ++iter)
      {
      QString group_name = QString(this->getProxy()->GetXMLName())
        + "_" + iter.key();

      foreach(vtkSMProxy* proxy, iter.value())
        {
        pxm->UnRegisterProxy(group_name.toStdString().c_str(), 
          proxy->GetSelfIDAsString());
        }
      }
    }
  this->Internal->ProxyLists.clear();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------



