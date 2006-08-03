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

#include <vtksys/ios/sstream>

// Qt includes.
#include <QList>
#include <QMap>
#include <QPointer>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqNameCount.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqRenderModule.h"
#include "pqServer.h"
#include "pqServerResource.h"

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

  typedef QList<pqRenderModule*> ListOfRenderModules;
  ListOfRenderModules RenderModules;

  typedef QMap<vtkSMDataObjectDisplayProxy*, pqPipelineDisplay*>
    MapOfDisplayProxyToDisplay;
  MapOfDisplayProxyToDisplay Displays;

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
QList<pqServer*> pqServerManagerModel::getServers()
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
QList<pqPipelineSource*> pqServerManagerModel::getSources(pqServer* server)
{
  QList<pqPipelineSource*> list;

  if(server)
    {
    vtkIdType cid = server->GetConnectionID();
    foreach (pqPipelineSource* source, this->Internal->Sources)
      {
      if (source->getProxy()->GetConnectionID() == cid) 
        {
        list.push_back(source);
        }
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
    SIGNAL(displayAdded(pqPipelineSource*, pqPipelineDisplay*)),
    this, SIGNAL(sourceDisplayChanged(pqPipelineSource*, pqPipelineDisplay*)));
  QObject::connect(pqSource, 
    SIGNAL(displayRemoved(pqPipelineSource*, pqPipelineDisplay*)),
    this, SIGNAL(sourceDisplayChanged(pqPipelineSource*, pqPipelineDisplay*)));
  this->connect(pqSource, SIGNAL(nameChanged(pqServerManagerModelItem*)), 
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
  vtkSMDataObjectDisplayProxy* dProxy =
    vtkSMDataObjectDisplayProxy::SafeDownCast(proxy);

  if (!proxy || !dProxy)
    {
    qDebug() << "onAddDisplay cannot be called with a null proxy.";
    return;
    }

  if (this->Internal->Displays.contains(dProxy))
    {
    qDebug() << "Display already added. Are you registering the display?";
    return;
    }

  // Called when a new display is registered.
  // 1) determine server on which the display is created.
  pqServer* server = this->getServerForSource(dProxy);
  if (!server)
    {
    qDebug() << "Could not locate the server on which the new display "
      << "was added.";
    return;
    }

  // 2) create a new pqPipelineDisplay;
  pqPipelineDisplay* display = new pqPipelineDisplay(name,
    dProxy, server, this);

  this->Internal->Displays[dProxy] = display;

  // Listen for visibility changes.
  QObject::connect(display, SIGNAL(visibilityChanged(bool)),
      this, SLOT(updateDisplayVisibility(bool)));

  // emit this->displayAdded(display);
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onRemoveDisplay(vtkSMProxy* proxy)
{
  vtkSMDataObjectDisplayProxy* dProxy =
    vtkSMDataObjectDisplayProxy::SafeDownCast(proxy);

  pqPipelineDisplay* display = NULL;
  if (!proxy || !dProxy)
    {
    qDebug() << "onRemoveDisplay cannot be called with a null proxy.";
    return;
    }
  if (this->Internal->Displays.contains(dProxy))
    {
    display = this->Internal->Displays.take(dProxy);
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
    // don't create a new pqRenderModule, one already exists.
    return;
    }

  pqRenderModule* pqRM = new pqRenderModule(name, rm, server, this);

  emit this->preRenderModuleAdded(pqRM);
  this->Internal->RenderModules.push_back(pqRM);

  emit this->renderModuleAdded(pqRM);
}

//-----------------------------------------------------------------------------
void pqServerManagerModel::onRemoveRenderModule(vtkSMRenderModuleProxy* rm)
{
  pqRenderModule* toRemove = this->getRenderModule(rm);
  if (!toRemove)
    {
    // no need to raise an debug message, the render module being removed
    // is already not present.
    // qDebug() << "Failed to locate the pqRenderModule for the proxy";
    return;
    }
  emit this->preRenderModuleRemoved(toRemove);
  this->Internal->RenderModules.removeAll(toRemove);
  emit this->renderModuleRemoved(toRemove);

  delete toRemove;
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
pqRenderModule* pqServerManagerModel::getRenderModule(vtkSMRenderModuleProxy* rm)
{
  foreach(pqRenderModule* pqRM, this->Internal->RenderModules)
    {
    if (pqRM->getProxy() == rm)
      {
      return pqRM;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
pqRenderModule* pqServerManagerModel::getRenderModule(QVTKWidget* widget)
{
  foreach(pqRenderModule* pqRM, this->Internal->RenderModules)
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
pqPipelineSource* pqServerManagerModel::getPQSource(vtkSMProxy* proxy)
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
pqPipelineDisplay* pqServerManagerModel::getPQDisplay(vtkSMProxy* proxy)
{
  vtkSMDataObjectDisplayProxy* disp = 
    vtkSMDataObjectDisplayProxy::SafeDownCast(proxy);
  if (disp && this->Internal->Displays.contains(disp))
    {
    return this->Internal->Displays[disp];
    }
  return NULL;
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
  pqPipelineDisplay *display = qobject_cast<pqPipelineDisplay *>(this->sender());
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
pqRenderModule* pqServerManagerModel::getRenderModule(int idx)
{
  if (idx >= this->Internal->RenderModules.size())
    {
    return 0;
    }
  return this->Internal->RenderModules.value(idx);
}

