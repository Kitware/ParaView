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
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include "vtksys/RegularExpression.hxx"

#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqUndoStack.h"

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QtDebug>

//-----------------------------------------------------------------------------
class pqProxyInternal
{
public:
  pqProxyInternal() { this->Connection = vtkSmartPointer<vtkEventQtSlotConnect>::New(); }
  typedef QMap<QString, QList<vtkSmartPointer<vtkSMProxy> > > ProxyListsType;
  ProxyListsType ProxyLists;
  vtkSmartPointer<vtkSMProxy> Proxy;
  vtkSmartPointer<vtkEventQtSlotConnect> Connection;

  /// Returns true if the ProxyLists (the collection of helper proxies)
  /// contains the given proxy.
  bool containsHelperProxy(vtkSMProxy* aproxy, QString& key) const
  {
    for (ProxyListsType::const_iterator iter = this->ProxyLists.begin();
         iter != this->ProxyLists.end(); ++iter)
    {
      if (iter.value().contains(aproxy))
      {
        key = iter.key();
        return true;
      }
    }
    return false;
  }
};

//-----------------------------------------------------------------------------
pqProxy::pqProxy(const QString& group, const QString& name, vtkSMProxy* proxy, pqServer* server,
  QObject* _parent /*=nullptr*/)
  : pqServerManagerModelItem(_parent)
  , Server(server)
  , SMName(name)
  , SMGroup(group)
{
  this->Internal = new pqProxyInternal;
  this->Internal->Proxy = proxy;
  this->Modified = pqProxy::UNMODIFIED;
  this->UserModifiedSMName = false;
}

//-----------------------------------------------------------------------------
pqProxy::~pqProxy()
{
  // Attach listener for proxy registration to handle helper proxy
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerObserver* observer = core->getServerManagerObserver();
  QObject::disconnect(observer,
    SIGNAL(proxyRegistered(const QString&, const QString&, vtkSMProxy*)), this,
    SLOT(onProxyRegistered(const QString&, const QString&, vtkSMProxy*)));
  QObject::disconnect(observer,
    SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)), this,
    SLOT(onProxyUnRegistered(const QString&, const QString&, vtkSMProxy*)));

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

    QString groupname =
      QString("pq_helper_proxies.%1").arg(this->getProxy()->GetGlobalIDAsString());

    vtkSMSessionProxyManager* pxm = this->proxyManager();
    pxm->RegisterProxy(groupname.toLocal8Bit().data(), key.toLocal8Bit().data(), proxy);
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
    QString groupname =
      QString("pq_helper_proxies.%1").arg(this->getProxy()->GetGlobalIDAsString());
    vtkSMSessionProxyManager* pxm = this->proxyManager();
    const char* name = pxm->GetProxyName(groupname.toLocal8Bit().data(), proxy);
    if (name)
    {
      pxm->UnRegisterProxy(groupname.toLocal8Bit().data(), name, proxy);
    }
  }
}

//-----------------------------------------------------------------------------
void pqProxy::updateHelperProxies() const
{
  QString groupname = QString("pq_helper_proxies.%1").arg(this->getProxy()->GetGlobalIDAsString());
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();
  iter->SetModeToOneGroup();
  iter->SetSession(this->getProxy()->GetSession());
  for (iter->Begin(groupname.toLocal8Bit().data()); !iter->IsAtEnd(); iter->Next())
  {
    this->addInternalHelperProxy(QString(iter->GetKey()), iter->GetProxy());
  }
  iter->Delete();
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
    foreach (vtkSMProxy* proxy, this->Internal->ProxyLists[key])
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

  pqProxyInternal::ProxyListsType::iterator iter = this->Internal->ProxyLists.begin();
  for (; iter != this->Internal->ProxyLists.end(); ++iter)
  {
    foreach (vtkSMProxy* proxy, iter.value())
    {
      list.push_back(proxy);
    }
  }
  return list;
}

//-----------------------------------------------------------------------------
pqProxy* pqProxy::findProxyWithHelper(vtkSMProxy* aproxy, QString& key)
{
  if (!aproxy)
  {
    return nullptr;
  }
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqServer* server = smmodel->findServer(aproxy->GetSession());
  QList<pqProxy*> proxies = smmodel->findItems<pqProxy*>(server);
  foreach (pqProxy* pqproxy, proxies)
  {
    if (pqproxy->Internal->containsHelperProxy(aproxy, key))
    {
      return pqproxy;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void pqProxy::rename(const QString& newname)
{
  if (newname != this->SMName)
  {
    SM_SCOPED_TRACE(RenameProxy).arg("proxy", this->getProxy());
    vtkSMSessionProxyManager* pxm = this->proxyManager();
    pxm->RegisterProxy(
      this->getSMGroup().toLocal8Bit().data(), newname.toLocal8Bit().data(), this->getProxy());
    pxm->UnRegisterProxy(this->getSMGroup().toLocal8Bit().data(),
      this->getSMName().toLocal8Bit().data(), this->getProxy());
    this->SMName = newname;
    this->UserModifiedSMName = true;
  }
}

//-----------------------------------------------------------------------------
void pqProxy::setSMName(const QString& name)
{
  if (!name.isEmpty() && this->SMName != name)
  {
    this->SMName = name;
    Q_EMIT this->nameChanged(this);
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
  if (modified != this->Modified)
  {
    this->Modified = modified;
    Q_EMIT this->modifiedStateChanged(this);
  }
}

//-----------------------------------------------------------------------------
vtkSMSessionProxyManager* pqProxy::proxyManager() const
{
  return this->Internal->Proxy ? this->Internal->Proxy->GetSessionProxyManager() : nullptr;
}
//-----------------------------------------------------------------------------
void pqProxy::initialize()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerObserver* observer = core->getServerManagerObserver();

  // Attach listener for proxy registration to handle helper proxy
  QObject::connect(observer, SIGNAL(proxyRegistered(const QString&, const QString&, vtkSMProxy*)),
    this, SLOT(onProxyRegistered(const QString&, const QString&, vtkSMProxy*)));
  QObject::connect(observer, SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)),
    this, SLOT(onProxyUnRegistered(const QString&, const QString&, vtkSMProxy*)));

  // Update helper proxy if any of them are already registered in ProxyManager
  this->updateHelperProxies();
}
//-----------------------------------------------------------------------------
void pqProxy::addInternalHelperProxy(const QString& key, vtkSMProxy* proxy) const
{
  if (!proxy || this->Internal->ProxyLists[key].contains(proxy))
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
  if (group == QString("pq_helper_proxies.%1").arg(this->getProxy()->GetGlobalIDAsString()))
  {
    this->addInternalHelperProxy(name, proxy);
  }
}
//-----------------------------------------------------------------------------
void pqProxy::onProxyUnRegistered(const QString& group, const QString& name, vtkSMProxy* proxy)
{
  if (group == QString("pq_helper_proxies.%1").arg(this->getProxy()->GetGlobalIDAsString()))
  {
    this->removeInternalHelperProxy(name, proxy);
  }
}

//-----------------------------------------------------------------------------
QString pqProxy::rstToHtml(const QString& rstStr)
{
  return QString::fromStdString(pqProxy::rstToHtml(rstStr.toUtf8().data()));
}

//-----------------------------------------------------------------------------
std::string pqProxy::rstToHtml(const char* rstStr)
{
  std::string htmlStr = rstStr;
  {
    // bold
    vtksys::RegularExpression re("[*][*]([^*]+)[*][*]");
    while (re.find(htmlStr))
    {
      const char* s = htmlStr.c_str();
      std::string bold(s + re.start(1), re.end(1) - re.start(1));
      htmlStr.replace(re.start(0), re.end(0) - re.start(0), std::string("<b>") + bold + "</b>");
    }
  }
  {
    // italic
    vtksys::RegularExpression re("[*]([^*]+)[*]");
    while (re.find(htmlStr))
    {
      const char* s = htmlStr.c_str();
      std::string it(s + re.start(1), re.end(1) - re.start(1));
      htmlStr.replace(re.start(0), re.end(0) - re.start(0), std::string("<i>") + it + "</i>");
    }
  }
  {
    // begin bullet list
    size_t start = 0;
    std::string src("\n\n- ");
    while ((start = htmlStr.find(src, start)) != std::string::npos)
    {
      htmlStr.replace(start, src.size(), "\n<ul><li>");
    }
  }
  {
    // li for bullet list
    size_t start = 0;
    std::string src("\n- ");
    while ((start = htmlStr.find(src, start)) != std::string::npos)
    {
      htmlStr.replace(start, src.size(), "\n<li>");
    }
  }
  {
    // end bullet list
    vtksys::RegularExpression re("<li>(.*)\n\n([^-])");
    while (re.find(htmlStr))
    {
      const char* s = htmlStr.c_str();
      std::string listItem(s + re.start(1), re.end(1) - re.start(1));
      std::string afterList(s + re.start(2), re.end(2) - re.start(2));
      htmlStr.replace(
        re.start(0), re.end(0) - re.start(0), std::string("<li>") + listItem + "</ul>" + afterList);
    }
  }
  {
    // paragraph
    size_t start = 0;
    std::string src("\n\n");
    while ((start = htmlStr.find(src, start)) != std::string::npos)
    {
      htmlStr.replace(start, src.size(), "\n<p>\n");
    }
  }
  return htmlStr;
}
