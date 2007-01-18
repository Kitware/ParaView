/*=========================================================================

   Program: ParaView
   Module:    pqServerManagerModel.cxx

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
#include "pqServerManagerModel.h"


// ParaView Server Manager includes.
#include "vtkProcessModule.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkStringList.h"


// Qt includes.
#include <QList>
#include <QMap>
#include <QPointer>
#include <QtDebug>
#include <QRegExp>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqNameCount.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPlotViewModule.h"
#include "pqRenderViewModule.h"
#include "pqScalarBarDisplay.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerResource.h"
#include "pqTableViewModule.h"
#include "pqTextWidgetDisplay.h"

#include <QVTKWidget.h>

pqServerManagerModel* pqServerManagerModel::Instance = 0;
pqServerManagerModel* pqServerManagerModel::instance()
{
  return pqServerManagerModel::Instance;
}

//-----------------------------------------------------------------------------
class pqServerManagerModelInternal 
{
public:
  struct Key {
    vtkClientServerID ID;
    vtkIdType ConnectionID;
    Key()
      {
      this->ID.ID = 0;
      this->ConnectionID = 0;
      }
    Key(const vtkIdType &cid, const vtkClientServerID &vtkid)
      {
      this->ID = vtkid;
      this->ConnectionID = cid;
      }
    bool operator <(const Key &other) const
      {
      if (this->ConnectionID == other.ConnectionID)
        {
        return (this->ID < other.ID);
        }
      return (this->ConnectionID < other.ConnectionID);
      }
  };

  typedef QMap<Key, pqPipelineSource*> MapOfSources;
  MapOfSources Sources;

  typedef QList<QPointer<pqServer> > ListOfServers;
  ListOfServers  Servers;

  typedef QList<pqRenderViewModule*> ListOfRenderModules;
  ListOfRenderModules RenderModules;

  typedef QMap<vtkSMProxy*, pqConsumerDisplay*> MapOfDisplayProxyToDisplay;
  MapOfDisplayProxyToDisplay Displays;

  typedef QMap<vtkSMProxy*, pqProxy*> MapOfProxies;
  MapOfProxies Proxies;

  // The pqServerManagerModel has taken on the responsibility
  // of assigning initial "cute" names for source and filters.
  // We use the NameGenerator to sort-of-uniquify such names.
  pqNameCount NameGenerator;
};


//-----------------------------------------------------------------------------
pqServerManagerModel::pqServerManagerModel(QObject* _parent /*=NULL*/):
  QObject(_parent)
{
  this->Internal = new pqServerManagerModelInternal();
  if (!pqServerManagerModel::Instance)
    {
    pqServerManagerModel::Instance = this;
    }
}

//-----------------------------------------------------------------------------
pqServerManagerModel::~pqServerManagerModel()
{
  delete this->Internal;
  if (pqServerManagerModel::Instance == this)
    {
    pqServerManagerModel::Instance = 0;
    }
}

//-----------------------------------------------------------------------------
QList<pqServer*> pqServerManagerModel::getServers() const
{
  QList<pqServer*> list;
  foreach (pqServer* server, this->Internal->Servers)
    {
    if (server)
      {
      list.push_back(server);
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
QList<pqPipelineDisplay*> pqServerManagerModel::getPipelineDisplays(
  pqServer* server)
{
  QList<pqPipelineDisplay*> list;

  foreach(pqConsumerDisplay* cd, this->Internal->Displays)
    {
    pqPipelineDisplay* display = qobject_cast<pqPipelineDisplay*>(cd);
    if (display && (!server || display->getServer() == server))
      {
      list.push_back(display);
      }
    }

  return list;
}

//-----------------------------------------------------------------------------
QList<pqConsumerDisplay*> pqServerManagerModel::getDisplays(pqServer* server)
{
  QList<pqConsumerDisplay*> list;

  foreach(pqConsumerDisplay* display, this->Internal->Displays)
    {
    if (display && (!server || display->getServer() == server))
      {
      list.push_back(display);
      }
    }

  return list;
}

//-----------------------------------------------------------------------------
QList<pqPipelineSource*> pqServerManagerModel::getSources(pqServer* server) const
{
  QList<pqPipelineSource*> list;

  vtkIdType cid = server->GetConnectionID();
  foreach (pqPipelineSource* source, this->Internal->Sources)
    {
    if (!server || source->getProxy()->GetConnectionID() == cid) 
      {
      list.push_back(source);
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
QList<pqRenderViewModule*> pqServerManagerModel::getRenderModules(pqServer* server)
{
  if (!server)
    {
    return this->Internal->RenderModules;
    }
  QList<pqRenderViewModule*> list;
  foreach(pqRenderViewModule* rm, this->Internal->RenderModules)
    {
    if (rm && (!server || rm->getServer() == server))
      {
      list.push_back(rm);
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
QList<pqGenericViewModule*> pqServerManagerModel::getViewModules(pqServer* server)
{
  QList<pqGenericViewModule*> list;
  QList<pqRenderViewModule*> rmlist = this->getRenderModules(server);
  foreach(pqRenderViewModule* rm, rmlist)
    {
    list.push_back(rm);
    }

  foreach(pqProxy* proxy, this->Internal->Proxies)
    {
    pqGenericViewModule* view = qobject_cast<pqGenericViewModule*>(proxy);
    if (view && (!server || view->getServer() == server))
      {
      list.push_back(view);
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onAddSource(QString name, vtkSMProxy* source)
{
  if (!source)
    {
    qDebug() << "onAddSource cannot be called with null source.";
    return;
    }

  if (this->getPQSource(source))
    {
    // proxy is being registered under "sources" group more than once,
    // the gui is only interested in the first name.
    return;
    }

  // Called when a new Source is registered on the ProxyManager.
  // 1) Determine the server on which this source is created.
  pqServer* server = this->getServerForSource(source);
  if (!server)
    {
    qDebug() << "Could not locate the server on which the new source was added.";
    return;
    }
  
  // 2) Create a new pqPipelineSource for the proxy.

  // We rely on Qt memory management to delete  sources when pqServerManagerModel
  // is destroyed.
  pqPipelineSource * pqSource = NULL;
  if (source->GetProperty("Input"))
    {
    pqSource = new pqPipelineFilter(name, source, server, this);
    }
  else
    {
    pqSource = new pqPipelineSource(name, source, server, this);
    }

  // Mark the new object as modified since the user has to hit the
  // accept button after creating the proxy.
  pqSource->setModified(true);
  
  //// Set a nice label for the source, since registration names
  //// in ParaView are nothing but IDs which are merely numbers.
  //vtksys_ios::ostringstream name_stream;
  //name_stream << source->GetXMLName() << 
  //  this->Internal->NameGenerator.GetCountAndIncrement(source->GetXMLName());
  ////pqSource->setProxyName(name_stream.str().c_str());


  QObject::connect(pqSource, 
    SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*)),
    this, SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*)));
  QObject::connect(pqSource, 
    SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*)),
    this, SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*)));
  QObject::connect(pqSource, 
    SIGNAL(preConnectionAdded(pqPipelineSource*, pqPipelineSource*)),
    this, SIGNAL(preConnectionAdded(pqPipelineSource*, pqPipelineSource*)));
  QObject::connect(pqSource, 
    SIGNAL(preConnectionRemoved(pqPipelineSource*, pqPipelineSource*)),
    this, SIGNAL(preConnectionRemoved(pqPipelineSource*, pqPipelineSource*)));
  QObject::connect(pqSource, 
    SIGNAL(displayAdded(pqPipelineSource*, pqConsumerDisplay*)),
    this, SIGNAL(sourceDisplayChanged(pqPipelineSource*, pqConsumerDisplay*)));
  QObject::connect(pqSource, 
    SIGNAL(displayRemoved(pqPipelineSource*, pqConsumerDisplay*)),
    this, SIGNAL(sourceDisplayChanged(pqPipelineSource*, pqConsumerDisplay*)));
  this->connect(pqSource, SIGNAL(nameChanged(pqServerManagerModelItem*)), 
    this, SIGNAL(nameChanged(pqServerManagerModelItem*)));
  this->connect(
    pqSource, SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
    this, SIGNAL(nameChanged(pqServerManagerModelItem*)));

  emit this->preSourceAdded(pqSource);
  this->Internal->Sources.insert(
    pqServerManagerModelInternal::Key(server->GetConnectionID(), 
      source->GetSelfID()), pqSource);
  emit this->sourceAdded(pqSource);

  // It is essential to let the world know of the addition of pqSource
  // before we start emitting signals as we update the initial state 
  // of the pqSource from its underlying proxy. Hence we emit this->sourceAdded()
  // before we do a pqSource->initialize();
  pqSource->initialize();
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onRemoveSource(QString name, vtkSMProxy* proxy)
{
  pqPipelineSource* source= this->getPQSource(proxy);
  if (!source)
    {
    return;
    }

  if (source->getProxyName() != name)
    {
    // The proxy is being unregistered from a name that is not visible to the GUI.
    // The GUI can only view the first name with which the proxy is registered
    // under the sources group. Hence, nothing to do here.
    // Note: this is not an error, hence we don't raise any.
    return;
    }

  // Now check if this proxy is registered under some other name with the
  // proxy manager under the sources group. If so, this is akin to a simply
  // name change, the pqSource is simply taking on a new identity. 
  vtkSmartPointer<vtkStringList> names = vtkSmartPointer<vtkStringList>::New();
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->GetProxyNames("sources", proxy, names);
  for (int cc=0; cc < names->GetLength(); cc++)
    {
    if (name == names->GetString(cc))
      {
      continue;
      }
    // Change the name of the pqsource.
    source->setProxyName(names->GetString(cc));
    return;
    }

  // If not, then alone, is the pqSource really being removed.
  emit this->preSourceRemoved(source);
  this->Internal->Sources.remove(pqServerManagerModelInternal::Key(
      proxy->GetConnectionID(), proxy->GetSelfID()));

  // disconnect everything.
  QObject::disconnect(source, 0, this, 0);
  emit this->sourceRemoved(source);
  delete source;
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onAddDisplay(QString name, 
  vtkSMProxy* proxy)
{
  if (!proxy)
    {
    qDebug() << "onAddDisplay cannot be called with a null proxy.";
    return;
    }

  if (this->Internal->Displays.contains(proxy))
    {
    qDebug() << "Display already added. Are you registering the display?";
    return;
    }

  // Called when a new display is registered.
  // 1) determine server on which the display is created.
  pqServer* server = this->getServerForSource(proxy);
  if (!server)
    {
    qDebug() << "Could not locate the server on which the new display "
      << "was added.";
    return;
    }

  pqConsumerDisplay* display = 0;

  vtkSMDataObjectDisplayProxy* dProxy =
    vtkSMDataObjectDisplayProxy::SafeDownCast(proxy);
  if (dProxy)
    {
    // 2) create a new pqConsumerDisplay;
    display = new pqPipelineDisplay(name, dProxy, server, this);
    }
  else if (proxy->GetXMLName() == QString("TextWidgetDisplay"))
    {
    display = new pqTextWidgetDisplay("displays", name, proxy, server, this);
    }
  else
    {
    display = new pqConsumerDisplay("displays", name, proxy, server, this);
    }

  this->Internal->Displays[proxy] = display;

  // Listen for visibility changes.
  QObject::connect(display, SIGNAL(visibilityChanged(bool)),
      this, SLOT(updateDisplayVisibility(bool)));

  // emit this->displayAdded(display);
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onRemoveDisplay(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    qDebug() << "onRemoveDisplay cannot be called with a null proxy.";
    return;
    }
  pqConsumerDisplay* display = NULL;
  if (this->Internal->Displays.contains(proxy))
    {
    display = this->Internal->Displays.take(proxy);
    }
  QObject::disconnect(display, 0, this, 0);
  // emit this->displayRemoved(display);

  delete display;
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onAddServer(vtkIdType id)
{
  if (this->getServer(id))
    {
    return;// Server already exists.
    }
  // TODO: how to assign friendly name for connections
  // origniation internal to the GUI?
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pqServer* server = new pqServer(id, pm->GetOptions(), this);
  this->onAddServer(server);
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onAddServer(pqServer* server)
{
  if (!server)
    {
    qCritical() << "onAddServer cannot add null server.";
    return;
    }
  if (this->Internal->Servers.contains(server))
    {
    qCritical() << "onAddServer called with an already existing server.";
    return;
    }

  this->Internal->Servers.push_back(server);
  this->connect(server, SIGNAL(nameChanged()), this, SLOT(updateServerName()));

  emit this->serverAdded(server);
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onRemoveServer(vtkIdType cid)
{
  this->onRemoveServer(this->getServer(cid));
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onRemoveServer(pqServer* server)
{
  if (!server)
    {
    qCritical() << "onRemoveServer cannot remove null server.";
    return;
    }
  int index = this->Internal->Servers.indexOf(server);
  if (index == -1)
    {
    qCritical() << "onRemoveServer called with non-added server.";
    return;
    }
  this->Internal->Servers.removeAt(index);
  // TODO: remove sources/views on this server as well.

  emit this->serverRemoved(server);

  delete server;
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onAddRenderModule(QString name, 
  vtkSMRenderModuleProxy* rm)
{
  if (!rm)
    {
    qDebug() << "onAddRenderModule cannot be called with null render module.";
    return;
    }


  pqServer* server =this->getServerForSource(rm);
  if (!server)
    {
    qDebug() << "Failed to locate server for render module.";
    return;
    }

  if (this->getRenderModule(rm))
    {
    // don't create a new pqRenderViewModule, one already exists.
    return;
    }

  pqRenderViewModule* pqRM = new pqRenderViewModule(name, rm, server, this);

  emit this->preRenderModuleAdded(pqRM);
  this->Internal->RenderModules.push_back(pqRM);

  emit this->renderModuleAdded(pqRM);
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onRemoveRenderModule(vtkSMRenderModuleProxy* rm)
{
  pqRenderViewModule* toRemove = this->getRenderModule(rm);
  if (!toRemove)
    {
    // no need to raise an debug message, the render module being removed
    // is already not present.
    // qDebug() << "Failed to locate the pqRenderViewModule for the proxy";
    return;
    }
  emit this->preRenderModuleRemoved(toRemove);
  this->Internal->RenderModules.removeAll(toRemove);
  emit this->renderModuleRemoved(toRemove);

  delete toRemove;
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onProxyRegistered(QString group, QString name, 
  vtkSMProxy* proxy)
{
  if (!proxy || group == "" || name == "" || 
    group.indexOf(QRegExp(".*_prototypes$")) != -1 )
    {
    return;
    }

  if (this->Internal->Proxies.contains(proxy))
    {
    return;
    }

  pqServer * server = this->getServerForSource(proxy);
  if (!server)
    {
    qDebug() << "Could not locate the server on which the new "
      " proxy was added.";
    return;
    }

  pqProxy *pq_proxy = NULL;
  if (group == "lookup_tables")
    {
    // Lookup Table registered.
    pq_proxy = new pqScalarsToColors(group, name, proxy, server, this);
    }
  else if (group == "scalar_bars")
    {
    pq_proxy = new pqScalarBarDisplay(group, name, proxy, server, this);
    }
  else if (group == "plot_modules")
    {
    int type;
    QString xmlType = proxy->GetXMLName();
    if (xmlType == "HistogramViewModule")
      {
      type = pqPlotViewModule::BAR_CHART;
      }
    else if (xmlType == "XYPlotViewModule")
      {
      type = pqPlotViewModule::XY_PLOT;
      }
    else
      {
      qDebug() << "Don't know what kind of plot is a " << xmlType;
      return;
      }
    vtkSMAbstractViewModuleProxy* ren = 
      vtkSMAbstractViewModuleProxy::SafeDownCast(proxy);
    if (ren)
      {
      pq_proxy = new pqPlotViewModule(type, group, name, ren, server, this);
      }
    }
  else if(group == "views" && proxy->GetXMLName() == QString("TableView"))
    {
    pq_proxy = new pqTableViewModule(group, name, 
      vtkSMAbstractViewModuleProxy::SafeDownCast(proxy), server, this);
    }
  else if (group == "animation")
    {
    // Animation subsystem.
    if (proxy->IsA("vtkSMAnimationSceneProxy"))
      {
      pq_proxy = new pqAnimationScene(group, name, proxy, server, this);
      }
    else if (proxy->IsA("vtkSMAnimationCueProxy"))
      {
      pq_proxy = new pqAnimationCue(group, name, proxy, server, this);
      }
    }

  if (pq_proxy)
    {
    emit this->preProxyAdded(pq_proxy);
    this->Internal->Proxies.insert(proxy, pq_proxy);
    emit this->proxyAdded(pq_proxy);
    }
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onProxyUnRegistered(QString group,
  QString name, vtkSMProxy* proxy)
{
  if (!this->Internal->Proxies.contains(proxy))
    {
    return;
    }

  pqProxy* pq_proxy = this->Internal->Proxies[proxy];
  if (pq_proxy->getSMName() == name && pq_proxy->getSMGroup() == group)
    {
    emit this->preProxyRemoved(pq_proxy);
    this->Internal->Proxies.remove(proxy);
    emit this->proxyRemoved(pq_proxy);

    delete pq_proxy;
    }
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

//-----------------------------------------------------------------------------
pqRenderViewModule* pqServerManagerModel::getRenderModule(vtkSMRenderModuleProxy* rm)
{
  foreach(pqRenderViewModule* pqRM, this->Internal->RenderModules)
    {
    if (pqRM->getProxy() == rm)
      {
      return pqRM;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
pqRenderViewModule* pqServerManagerModel::getRenderModule(QVTKWidget* widget)
{
  foreach(pqRenderViewModule* pqRM, this->Internal->RenderModules)
    {
    if (pqRM->getWidget() == widget)
      {
      return pqRM;
      }
    }
  return NULL;
}
//-----------------------------------------------------------------------------
pqServer* pqServerManagerModel::getServerForSource(pqPipelineSource* src)
{
  return this->getServerForSource(src->getProxy());
}

//-----------------------------------------------------------------------------
pqServer* pqServerManagerModel::getServerForSource(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    qCritical() << "proxy cannot be null.";
    return 0;
    }

  vtkIdType cid = proxy->GetConnectionID();
  return this->getServer(cid);
}

//-----------------------------------------------------------------------------
pqServer* pqServerManagerModel::getServer(vtkIdType cid)
{
  foreach(pqServer* server, this->Internal->Servers)
    {
    if (server && server->GetConnectionID() == cid)  
      {
      return server;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
pqServer* pqServerManagerModel::getServer(const pqServerResource& resource) const
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
pqServer* pqServerManagerModel::getServerByIndex(unsigned int idx) const
{
  if((int)idx < this->Internal->Servers.size())
    {
    return this->Internal->Servers[idx];
    }

  return NULL;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqServerManagerModel::getPQSource(vtkSMProxy* proxy) const
{
  pqServerManagerModelInternal::Key key(proxy->GetConnectionID(),
    proxy->GetSelfID());

  pqServerManagerModelInternal::MapOfSources::iterator it =
    this->Internal->Sources.find(key);

  if (it != this->Internal->Sources.end())
    {
    return it.value();
    }
  return NULL;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqServerManagerModel::getPQSource(const QString &name) const
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* proxy = pxm->GetProxy("sources", name.toAscii().data());
  if(proxy)
    {
    return this->getPQSource(proxy);
    }

  return 0;
}

//-----------------------------------------------------------------------------
pqConsumerDisplay* pqServerManagerModel::getPQDisplay(vtkSMProxy* disp)
{
  if (disp && this->Internal->Displays.contains(disp))
    {
    return this->Internal->Displays[disp];
    }
  return NULL;
}

//-----------------------------------------------------------------------------
pqProxy* pqServerManagerModel::getPQProxy(vtkSMProxy* proxy)
{
  pqProxy* pqproxy = 0;
  if (proxy)
    {
    if (this->Internal->Proxies.contains(proxy))
      {
      pqproxy = this->Internal->Proxies[proxy];
      }
    if (!pqproxy)
      {
      pqproxy = this->getPQDisplay(proxy);
      }
    if (!pqproxy)
      {
      pqproxy = this->getPQSource(proxy);
      }
    if (!pqproxy)
      {
      pqproxy = this->getRenderModule(
        vtkSMRenderModuleProxy::SafeDownCast(proxy));
      }
    }
  return pqproxy;
}

//-----------------------------------------------------------------------------
unsigned int pqServerManagerModel::getNumberOfServers()
{
  return static_cast<unsigned int>(this->Internal->Servers.size());
}

//-----------------------------------------------------------------------------
unsigned int pqServerManagerModel::getNumberOfSources()
{
  return static_cast<unsigned int>(this->Internal->Sources.size());
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqServerManagerModel::getPQSource(int idx)
{
  pqServerManagerModelInternal::MapOfSources::iterator iter;
  int i;
  for(i=0, iter = this->Internal->Sources.begin();
      iter != this->Internal->Sources.end();
      ++iter, ++i)
    {
    if(i == idx)
      {
      return iter.value();
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::updateServerName()
{
  // Get the server object from the sender.
  pqServer *server = qobject_cast<pqServer *>(this->sender());
  if(server)
    {
    emit this->nameChanged(server);
    }
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::updateDisplayVisibility(bool)
{
  // Get the display object from the sender.
  pqConsumerDisplay *display = qobject_cast<pqConsumerDisplay *>(this->sender());
  if(display)
    {
    pqPipelineSource *source = display->getInput();
    if(source)
      {
      emit this->sourceDisplayChanged(source, display);
      }
    }
}


//-----------------------------------------------------------------------------
int pqServerManagerModel::getNumberOfRenderModules()
{
  return this->Internal->RenderModules.size();
}

//-----------------------------------------------------------------------------
pqRenderViewModule* pqServerManagerModel::getRenderModule(int idx)
{
  if (idx >= this->Internal->RenderModules.size())
    {
    return 0;
    }
  return this->Internal->RenderModules.value(idx);
}

