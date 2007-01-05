/*=========================================================================

   Program:   ParaView
   Module:    pqPipelineModel.cxx

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

/// \file pqPipelineModel.cxx
/// \date 4/14/2006

#include "pqPipelineModel.h"

#include "pqConsumerDisplay.h"
#include "pqGenericViewModule.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerModelItem.h"

#include <QFont>
#include <QIcon>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QPointer>
#include <QString>
#include <QtDebug>

#include "vtkSMCompoundProxy.h"
#include "vtkSMProxy.h"

class pqPipelineModelSource;


/// \class pqPipelineModelItem
class pqPipelineModelItem : public QObject
{
public:
  enum VisibleState
    {
    NotAllowed,
    Visible,
    NotVisible
    };

public:
  pqPipelineModelItem(QObject *parent=0);
  virtual ~pqPipelineModelItem() {}

  virtual QString getName() const=0;
  virtual VisibleState getVisibleState(pqGenericViewModule *module) const=0;
  virtual bool isSelectable() const {return true;}
  virtual void setSelectable(bool selectable)=0;
  virtual bool isModified() const=0;
  virtual pqPipelineModelItem *getParent() const=0;
  virtual pqServerManagerModelItem *getObject() const=0;
  virtual int getChildCount() const=0;
  virtual int getChildIndex(pqPipelineModelItem *item) const=0;
  virtual pqPipelineModelItem *getChild(int row) const=0;
  virtual void removeChild(pqPipelineModelItem *item)=0;

  pqPipelineModel::ItemType getType() const {return this->Type;}

protected:
  void setType(pqPipelineModel::ItemType type) {this->Type = type;}

private:
  pqPipelineModel::ItemType Type;
};


/// \class pqPipelineModelServer
class pqPipelineModelServer : public pqPipelineModelItem
{
public:
  pqPipelineModelServer(pqServer *server=0, QObject *parent=0);
  virtual ~pqPipelineModelServer();

  virtual QString getName() const;
  virtual pqPipelineModelItem::VisibleState getVisibleState(
      pqGenericViewModule *module) const;
  virtual bool isSelectable() const {return this->Selectable;}
  virtual void setSelectable(bool selectable) {this->Selectable = selectable;}
  virtual bool isModified() const {return false;}
  virtual pqPipelineModelItem *getParent() const {return 0;}
  virtual pqServerManagerModelItem *getObject() const {return this->Server;}
  virtual int getChildCount() const {return this->Sources.size();}
  virtual int getChildIndex(pqPipelineModelItem *item) const;
  virtual pqPipelineModelItem *getChild(int row) const;
  virtual void removeChild(pqPipelineModelItem *item);

  pqServer *getServer() const {return this->Server;}
  void setServer(pqServer *server) {this->Server = server;}

  QList<pqPipelineModelSource *> &getSources() {return this->Sources;}

private:
  pqServer *Server;
  QList<pqPipelineModelSource *> Sources;
  bool Selectable;
};


/// \class pqPipelineModelObject
class pqPipelineModelObject : public pqPipelineModelItem
{
public:
  pqPipelineModelObject(pqPipelineModelServer *server=0, QObject *parent=0);
  virtual ~pqPipelineModelObject() {}

  pqPipelineModelServer *getServer() const {return this->Server;}
  void setServer(pqPipelineModelServer *server) {this->Server = server;}

private:
  pqPipelineModelServer *Server;
};


/// \class pqPipelineModelSource
class pqPipelineModelSource : public pqPipelineModelObject
{
public:
  pqPipelineModelSource(pqPipelineModelServer *server=0,
      pqPipelineSource *source=0, QObject *parent=0);
  virtual ~pqPipelineModelSource();

  virtual QString getName() const;
  virtual pqPipelineModelItem::VisibleState getVisibleState(
      pqGenericViewModule *module) const;
  virtual bool isSelectable() const {return this->Selectable;}
  virtual void setSelectable(bool selectable) {this->Selectable = selectable;}
  virtual bool isModified() const;
  virtual pqPipelineModelItem *getParent() const {return this->getServer();}
  virtual pqServerManagerModelItem *getObject() const {return this->Source;}
  virtual int getChildCount() const {return this->Outputs.size();}
  virtual int getChildIndex(pqPipelineModelItem *item) const;
  virtual pqPipelineModelItem *getChild(int row) const;
  virtual void removeChild(pqPipelineModelItem *item);

  pqPipelineSource *getSource() const {return this->Source;}
  void setSource(pqPipelineSource *source) {this->Source = source;}

  QList<pqPipelineModelObject *> &getOutputs() {return this->Outputs;}

private:
  pqPipelineSource *Source;
  QList<pqPipelineModelObject *> Outputs;
  bool Selectable;
};


/// \class pqPipelineModelFilter
class pqPipelineModelFilter : public pqPipelineModelSource
{
public:
  pqPipelineModelFilter(pqPipelineModelServer *server=0,
      pqPipelineSource *source=0,
      pqPipelineModel::ItemType type=pqPipelineModel::Filter,
      QObject *parent=0);
  virtual ~pqPipelineModelFilter() {}

  virtual pqPipelineModelItem *getParent() const;

  QList<pqPipelineModelSource *> &getInputs() {return this->Inputs;}

private:
  QList<pqPipelineModelSource *> Inputs;
};


/// \class pqPipelineModelLink
class pqPipelineModelLink : public pqPipelineModelObject
{
public:
  pqPipelineModelLink(pqPipelineModelServer *server=0, QObject *parent=0);
  virtual ~pqPipelineModelLink() {}

  virtual QString getName() const;
  virtual pqPipelineModelItem::VisibleState getVisibleState(
      pqGenericViewModule *module) const;
  virtual bool isSelectable() const;
  virtual void setSelectable(bool selectable);
  virtual bool isModified() const;
  virtual pqPipelineModelItem *getParent() const {return this->Source;}
  virtual pqServerManagerModelItem *getObject() const;
  virtual int getChildCount() const {return 0;}
  virtual int getChildIndex(pqPipelineModelItem *) const {return -1;}
  virtual pqPipelineModelItem *getChild(int) const {return 0;}
  virtual void removeChild(pqPipelineModelItem *) {}

  pqPipelineModelSource *getSource() const {return this->Source;}
  void setSource(pqPipelineModelSource *source) {this->Source = source;}

  pqPipelineModelFilter *getSink() const {return this->Sink;}
  void setSink(pqPipelineModelFilter *sink) {this->Sink = sink;}

private:
  pqPipelineModelSource *Source;
  pqPipelineModelFilter *Sink;
};


/// \class pqPipelineModelInternal
/// \brief
///   The pqPipelineModelInternal class hides implementation details
///   from the interface.
class pqPipelineModelInternal
{
public:
  enum PixmapIndex
    {
    Eyeball = pqPipelineModel::LastType + 1,
    EyeballGray,
    Total
    };

public:
  pqPipelineModelInternal();
  ~pqPipelineModelInternal() {}

  QList<pqPipelineModelServer *> Servers;
  QMap<pqServerManagerModelItem *, QPointer<pqPipelineModelItem> > ItemMap;
  pqGenericViewModule *RenderModule;
  pqServer *CleanupServer;
  QFont Modified;
};


//-----------------------------------------------------------------------------
pqPipelineModelItem::pqPipelineModelItem(QObject *parentObject)
  : QObject(parentObject)
{
  this->Type = pqPipelineModel::Invalid;
}


//-----------------------------------------------------------------------------
pqPipelineModelServer::pqPipelineModelServer(pqServer *server,
    QObject *parentObject)
  : pqPipelineModelItem(parentObject), Sources()
{
  this->Server = server;
  this->Selectable = true;

  this->setType(pqPipelineModel::Server);
}

pqPipelineModelServer::~pqPipelineModelServer()
{
  // Delete any source items still in the list.
  QList<pqPipelineModelSource *>::Iterator iter = this->Sources.begin();
  for( ; iter != this->Sources.end(); ++iter)
    {
    delete *iter;
    }

  this->Sources.clear();
}

QString pqPipelineModelServer::getName() const
{
  if(this->Server)
    {
    return this->Server->getResource().toString();
    }

  return QString();
}

pqPipelineModelItem::VisibleState pqPipelineModelServer::getVisibleState(
    pqGenericViewModule *) const
{
  return pqPipelineModelItem::NotAllowed;
}

int pqPipelineModelServer::getChildIndex(pqPipelineModelItem *item) const
{
  pqPipelineModelSource *source = dynamic_cast<pqPipelineModelSource *>(item);
  if(source)
    {
    return this->Sources.indexOf(source);
    }

  return -1;
}

pqPipelineModelItem *pqPipelineModelServer::getChild(int row) const
{
  return this->Sources[row];
}

void pqPipelineModelServer::removeChild(pqPipelineModelItem *item)
{
  pqPipelineModelSource *source = dynamic_cast<pqPipelineModelSource *>(item);
  if(source)
    {
    this->Sources.removeAll(source);
    }
}


//-----------------------------------------------------------------------------
pqPipelineModelObject::pqPipelineModelObject(pqPipelineModelServer *server,
    QObject *parentObject)
  : pqPipelineModelItem(parentObject)
{
  this->Server = server;
}


//-----------------------------------------------------------------------------
pqPipelineModelSource::pqPipelineModelSource(pqPipelineModelServer *server,
    pqPipelineSource *source, QObject *parentObject)
  : pqPipelineModelObject(server, parentObject), Outputs()
{
  this->Source = source;
  this->Selectable = true;

  this->setType(pqPipelineModel::Source);
}

pqPipelineModelSource::~pqPipelineModelSource()
{
  // Clean up the ouputs left in the list.
  QList<pqPipelineModelObject *>::Iterator iter = this->Outputs.begin();
  for( ; iter != this->Outputs.end(); ++iter)
    {
    delete *iter;
    }

  this->Outputs.clear();
}

QString pqPipelineModelSource::getName() const
{
  if(this->Source)
    {
    return this->Source->getProxyName();
    }

  return QString();
}

pqPipelineModelItem::VisibleState pqPipelineModelSource::getVisibleState(
    pqGenericViewModule *module) const
{
  pqPipelineModelItem::VisibleState state = pqPipelineModelItem::NotAllowed;
  if(module && module->getServer() == this->Source->getServer())
    {
    pqConsumerDisplay*display = this->Source->getDisplay(module);
    if(display && display->isVisible())
      {
      state = pqPipelineModelItem::Visible;
      }
    else if (module->canDisplaySource(this->Source))
      {
      state = pqPipelineModelItem::NotVisible;
      }
    else
      {
      state = pqPipelineModelItem::NotAllowed;
      }
    }

  return state;
}

bool pqPipelineModelSource::isModified() const
{
  if(this->Source)
    {
    return this->Source->isModified();
    }

  return false;
}

int pqPipelineModelSource::getChildIndex(pqPipelineModelItem *item) const
{
  pqPipelineModelObject *object =
      dynamic_cast<pqPipelineModelObject *>(item);
  if(object)
    {
    pqPipelineModelLink *link = 0;
    QList<pqPipelineModelObject *>::ConstIterator iter = this->Outputs.begin();
    for(int index = 0; iter != this->Outputs.end(); ++iter, ++index)
      {
      if(object == *iter)
        {
        return index;
        }

      // The output may be in a link object.
      link = dynamic_cast<pqPipelineModelLink *>(*iter);
      if(link && link->getSink() == object)
        {
        return index;
        }
      }
    }

  return -1;
}

pqPipelineModelItem *pqPipelineModelSource::getChild(int row) const
{
  return this->Outputs[row];
}

void pqPipelineModelSource::removeChild(pqPipelineModelItem *item)
{
  pqPipelineModelObject *object = dynamic_cast<pqPipelineModelObject *>(item);
  if(object)
    {
    this->Outputs.removeAll(object);
    }
}


//-----------------------------------------------------------------------------
pqPipelineModelFilter::pqPipelineModelFilter(pqPipelineModelServer *server,
    pqPipelineSource *source, pqPipelineModel::ItemType type,
    QObject *parentObject)
  : pqPipelineModelSource(server, source, parentObject), Inputs()
{
  if(type != pqPipelineModel::CustomFilter)
    {
    type = pqPipelineModel::Filter;
    }

  this->setType(type);
}

pqPipelineModelItem *pqPipelineModelFilter::getParent() const
{
  if(this->Inputs.size() == 1)
    {
    return this->Inputs[0];
    }
  else
    {
    return this->getServer();
    }
}


//-----------------------------------------------------------------------------
pqPipelineModelLink::pqPipelineModelLink(pqPipelineModelServer *server,
    QObject *parentObject)
  : pqPipelineModelObject(server, parentObject)
{
  this->Source = 0;
  this->Sink = 0;

  this->setType(pqPipelineModel::Link);
}

QString pqPipelineModelLink::getName() const
{
  if(this->Sink)
    {
    return this->Sink->getName();
    }

  return QString();
}

pqPipelineModelItem::VisibleState pqPipelineModelLink::getVisibleState(
    pqGenericViewModule *module) const
{
  if(this->Sink)
    {
    return this->Sink->getVisibleState(module);
    }

  return pqPipelineModelItem::NotAllowed;
}

bool pqPipelineModelLink::isSelectable() const
{
  if(this->Sink)
    {
    return this->Sink->isSelectable();
    }

  return false;
}

void pqPipelineModelLink::setSelectable(bool)
{
}

bool pqPipelineModelLink::isModified() const
{
  if(this->Sink)
    {
    return this->Sink->isModified();
    }

  return false;
}

pqServerManagerModelItem *pqPipelineModelLink::getObject() const
{
  if(this->Sink)
    {
    return this->Sink->getObject();
    }

  return 0;
}


//-----------------------------------------------------------------------------
pqPipelineModelInternal::pqPipelineModelInternal()
  : Servers(), ItemMap(), Modified()
{
  this->RenderModule = 0;
  this->CleanupServer = 0;

  this->Modified.setBold(true);
}


//-----------------------------------------------------------------------------
pqPipelineModel::pqPipelineModel(QObject *parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Internal = new pqPipelineModelInternal();
  this->PixmapList = 0;

  // Initialize the pixmap list.
  this->initializePixmaps();
}

pqPipelineModel::pqPipelineModel(const pqPipelineModel& vtkNotUsed(other),
    QObject *parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Internal = new pqPipelineModelInternal();
  this->PixmapList = 0;

  // Initialize the pixmap list.
  this->initializePixmaps();

  // TODO: Copy the other pipeline model.
}

pqPipelineModel::pqPipelineModel(const pqServerManagerModel &other,
    QObject *parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Internal = new pqPipelineModelInternal();
  this->PixmapList = 0;

  // Initialize the pixmap list.
  this->initializePixmaps();

  // Build a pipeline model from the current server manager model.
  QList<pqPipelineSource *> sources;
  QList<pqPipelineSource *>::Iterator source;
  QList<pqServer *> servers = other.getServers();
  QList<pqServer *>::Iterator server = servers.begin();
  for( ; server != servers.end(); ++server)
    {
    // Add the server to the model.
    this->addServer(*server);

    // Add the sources for the server.
    sources = other.getSources(*server);
    for(source = sources.begin(); source != sources.end(); ++source)
      {
      this->addSource(*source);
      }

    // Set up the pipeline connections.
    for(source = sources.begin(); source != sources.end(); ++source)
      {
      for(int i = 0; i < (*source)->getNumberOfConsumers(); ++i)
        {
        this->addConnection(*source, (*source)->getConsumer(i));
        }
      }
    }
}

pqPipelineModel::~pqPipelineModel()
{
  if(this->PixmapList)
    {
    delete [] this->PixmapList;
    }

  // Clean up the pipeline model items.
  this->Internal->ItemMap.clear();
  QList<pqPipelineModelServer *>::Iterator iter =
      this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    delete *iter;
    }

  this->Internal->Servers.clear();
  delete this->Internal;
}

int pqPipelineModel::rowCount(const QModelIndex &parentIndex) const
{
  if(parentIndex.isValid())
    {
    if(parentIndex.model() == this)
      {
      pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
          parentIndex.internalPointer());
      return item->getChildCount();
      }
    }
  else
    {
    return this->Internal->Servers.size();
    }

  return 0;
}

int pqPipelineModel::columnCount(const QModelIndex &) const
{
  return 2;
}

bool pqPipelineModel::hasChildren(const QModelIndex &parentIndex) const
{
  return this->rowCount(parentIndex) > 0;
}

QModelIndex pqPipelineModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  if(row < 0 || column < 0 || column > this->columnCount())
    {
    return QModelIndex();
    }

  if(parentIndex.isValid())
    {
    if(parentIndex.model() == this)
      {
      pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
          parentIndex.internalPointer());
      if(row < item->getChildCount())
        {
        return this->createIndex(row, column, item->getChild(row));
        }
      }
    }
  else if(row < this->Internal->Servers.size())
    {
    return this->createIndex(row, column, this->Internal->Servers[row]);
    }

  return QModelIndex();
}

QModelIndex pqPipelineModel::parent(const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem*>(
        idx.internalPointer());
    pqPipelineModelItem *parentItem = item->getParent();
    if(parentItem)
      {
      return this->makeIndex(parentItem);
      }
    }

  return QModelIndex();
}

QVariant pqPipelineModel::data(const QModelIndex &idx, int role) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem*>(
        idx.internalPointer());
    if(item)
      {
      switch(role)
        {
        case Qt::DisplayRole:
          {
          if(idx.column() == 1)
            {
            pqPipelineModelItem::VisibleState visible = item->getVisibleState(
                this->Internal->RenderModule);
            if(visible == pqPipelineModelItem::Visible)
              {
              return QVariant(QIcon(
                  this->PixmapList[pqPipelineModelInternal::Eyeball]));
              }
            else if(visible == pqPipelineModelItem::NotVisible)
              {
              return QVariant(QIcon(
                  this->PixmapList[pqPipelineModelInternal::EyeballGray]));
              }
            }
          }
        case Qt::ToolTipRole:
        case Qt::EditRole:
          {
          if(idx.column() == 0)
            {
            return QVariant(item->getName());
            }

          break;
          }
        case Qt::DecorationRole:
          {
          if(idx.column() == 0 && this->PixmapList &&
              item->getType() != pqPipelineModel::Invalid)
            {
            return QVariant(this->PixmapList[item->getType()]);
            }

          break;
          }
        case Qt::FontRole:
          {
          if(idx.column() == 0 && item->isModified())
            {
            return qVariantFromValue<QFont>(this->Internal->Modified);
            }

          break;
          }
        }
      }
    }

  return QVariant();
}

Qt::ItemFlags pqPipelineModel::flags(const QModelIndex &idx) const
{
  Qt::ItemFlags indexFlags = Qt::ItemIsEnabled;
  if(idx.column() == 0)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem*>(
        idx.internalPointer());
    if(item->isSelectable())
      {
      indexFlags |= Qt::ItemIsSelectable;
      }

    if(item->getType() != pqPipelineModel::Server && this->Editable)
      {
      indexFlags |= Qt::ItemIsEditable;
      }
    }

  return indexFlags;
}

bool pqPipelineModel::setData(const QModelIndex &idx, const QVariant &value,
    int)
{
  if(value.toString().isEmpty())
    {
    return false;
    }

  pqPipelineSource* source = qobject_cast<pqPipelineSource*>(
      this->getItemFor(idx));
  if(source)
    {
    source->rename(value.toString());
    }
  return true;
}

pqServerManagerModelItem *pqPipelineModel::getItemFor(
    const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item =reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());
    if(item)
      {
      return item->getObject();
      }
    }

  return 0;
}

QModelIndex pqPipelineModel::getIndexFor(pqServerManagerModelItem *item) const
{
  pqPipelineModelItem *modelItem = this->getModelItemFor(item);
  if(modelItem)
    {
    return this->makeIndex(modelItem);
    }

  return QModelIndex();
}

void pqPipelineModel::setSubtreeSelectable(pqServerManagerModelItem *item,
    bool selectable)
{
  pqPipelineModelItem *root = this->getModelItemFor(item);
  pqPipelineModelItem *modelItem = root;
  while(modelItem)
    {
    modelItem->setSelectable(selectable);
    modelItem = this->getNextModelItem(modelItem, root);
    }
}

void pqPipelineModel::setModifiedFont(const QFont &font)
{
  this->Internal->Modified = font;
}

void pqPipelineModel::addServer(pqServer *server)
{
  if(!server)
    {
    qDebug() << "Unable to add null server to the pipeline model.";
    return;
    }

  // Make sure the server does not already have an item in the model.
  if(this->getModelItemFor(server))
    {
    qWarning() << "Server already added to the pipeline model.";
    return;
    }

  // Create a new pipeline model item for the server.
  pqPipelineModelServer *item = new pqPipelineModelServer(server);
  if(item)
    {
    // Add the server to the map and to the model.
    this->Internal->ItemMap.insert(server, item);

    int row = this->Internal->Servers.size();
    this->beginInsertRows(QModelIndex(), row, row);
    this->Internal->Servers.append(item);
    this->endInsertRows();
    }
}

void pqPipelineModel::startRemovingServer(pqServer *server)
{
  // Check if another server is being cleaned up.
  if(this->Internal->CleanupServer)
    {
    qDebug() << "Already cleaning up another server.";
    return;
    }

  // Keep a pointer to the server around until all the sources are
  // removed to avoid debug warnings.
  this->Internal->CleanupServer = server;

  // Remove the server from the model.
  this->removeServer(server);
}

void pqPipelineModel::removeServer(pqServer *server)
{
  if(!server)
    {
    qDebug() << "Null server not found in the pipeline model.";
    return;
    }

  // Make sure the server has an item in the model.
  pqPipelineModelServer *serverItem =
      dynamic_cast<pqPipelineModelServer *>(this->getModelItemFor(server));
  if(!serverItem)
    {
    if(this->Internal->CleanupServer == server)
      {
      this->Internal->CleanupServer = 0;
      }
    else
      {
      qWarning() << "Server not found in the pipeline model.";
      }

    return;
    }

  // Remove the server from the model.
  int row = this->Internal->Servers.indexOf(serverItem);
  this->beginRemoveRows(QModelIndex(), row, row);
  this->Internal->Servers.removeAll(serverItem);
  this->endRemoveRows();

  // Deleting the item for the server will also delete all the child
  // items in the subtree. Since the map uses QPointers, all the
  // references in the map will be null including the server. All the
  // null references can then be cleaned up.
  delete serverItem;
  this->cleanPipelineMap();
}

void pqPipelineModel::addSource(pqPipelineSource *source)
{
  if(!source)
    {
    qDebug() << "Unable to add null source to the pipeline model.";
    return;
    }

  // Make sure the source item is not in the model.
  if(this->getModelItemFor(source))
    {
    qWarning() << "Source already added to the pipeline model.";
    return;
    }

  // Find the source's parent model item.
  pqPipelineModelServer *server = dynamic_cast<pqPipelineModelServer *>(
     this->getModelItemFor(source->getServer()));
  if(!server)
    {
    qWarning() << "Source server not found in the pipeline model.";
    return;
    }

  // Create the appropriate source object. Determine the type using
  // the source's xml group name.
  pqPipelineModelSource *item = 0;
  vtkSMProxy *proxy = source->getProxy();
  if(vtkSMCompoundProxy::SafeDownCast(proxy) != 0)
    {
    item = new pqPipelineModelFilter(server, source,
        pqPipelineModel::CustomFilter);
    }
  else if(strcmp(proxy->GetXMLGroup(), "filters") == 0)
    {
    item = new pqPipelineModelFilter(server, source);
    }
  else if(strcmp(proxy->GetXMLGroup(), "sources") == 0)
    {
    item = new pqPipelineModelSource(server, source);
    }
  else
    {
    // Now we determine type using some heuristics.
    if (proxy->GetProperty("Input"))
      {
      item = new pqPipelineModelFilter(server, source);
      }
    else
      {
      item = new pqPipelineModelSource(server, source);
      }
    }

  if(item)
    {
    // Add the source to the map.
    this->Internal->ItemMap.insert(source, item);

    QModelIndex parentIndex = this->makeIndex(server);
    int row = server->getChildCount();
    this->beginInsertRows(parentIndex, row, row);
    server->getSources().insert(row, item);
    this->endInsertRows();
    if(server->getChildCount() == 1)
      {
      emit this->firstChildAdded(parentIndex);
      }
    }
}

void pqPipelineModel::removeSource(pqPipelineSource *source)
{
  if(!source)
    {
    qDebug() << "Null source not found in the pipeline model.";
    return;
    }

  // Ignore source removal when cleaning up a server since it has
  // already been deleted.
  if(source->getServer() == this->Internal->CleanupServer)
    {
    return;
    }

  // Make sure the source has an item in the model.
  pqPipelineModelItem *item = this->getModelItemFor(source);
  pqPipelineModelSource *sourceItem =
      dynamic_cast<pqPipelineModelSource *>(item);
  if(!sourceItem)
    {
    qDebug() << "Source not found in the pipeline model.";
    return;
    }

  // The source should not have any outputs when it is deleted.
  int i = 0;
  pqPipelineModelObject *output = 0;
  pqPipelineModelFilter *filter = 0;
  pqPipelineModelLink *link = 0;
  if(sourceItem->getChildCount())
    {
    qWarning() << "Source deleted with outputs attached.";

    // Clean up the outputs to maintain model integrity.
    for(i = sourceItem->getChildCount() - 1; i >= 0; i--)
      {
      output = sourceItem->getOutputs().at(i);
      filter = dynamic_cast<pqPipelineModelFilter *>(output);
      if(!filter)
        {
        link = dynamic_cast<pqPipelineModelLink *>(output);
        if(link)
          {
          filter = link->getSink();
          if(!filter)
            {
            qDebug() << "Empty link found in source output.";
            continue;
            }
          }
        }

      // Calling remove connection will modify the output list. Since
      // the loop counts from the end, this should not be a problem.
      this->removeConnection(sourceItem, filter);
      }
    }

  // If the filter has more than one input, the link items pointing
  // to it need to be removed.
  int row = 0;
  QModelIndex parentIndex;
  pqPipelineModelItem *parentItem = 0;
  filter = dynamic_cast<pqPipelineModelFilter *>(sourceItem);
  if(filter && filter->getInputs().size() > 1)
    {
    pqPipelineModelSource *input = 0;
    for(i = filter->getInputs().size() - 1; i >= 0; i--)
      {
      input = filter->getInputs().at(i);
      row = input->getChildIndex(filter);
      link = dynamic_cast<pqPipelineModelLink *>(input->getChild(row));
      if(link)
        {
        parentIndex = this->makeIndex(input);
        this->beginRemoveRows(parentIndex, row, row);
        input->removeChild(link);
        this->endRemoveRows();
        delete link;
        }
      }
    }

  // Finally, remove the source from the model and map.
  parentItem = sourceItem->getParent();
  parentIndex = this->makeIndex(parentItem);
  row = parentItem->getChildIndex(sourceItem);
  this->beginRemoveRows(parentIndex, row, row);
  parentItem->removeChild(sourceItem);
  this->endRemoveRows();

  delete sourceItem;
  this->cleanPipelineMap();
}

void pqPipelineModel::addConnection(pqPipelineSource *source,
    pqPipelineSource *sink)
{
  pqPipelineModelSource *sourceItem =
      dynamic_cast<pqPipelineModelSource *>(this->getModelItemFor(source));
  if(!sourceItem)
    {
    qDebug() << "Connection source not found in the pipeline model.";
    return;
    }

  pqPipelineModelFilter *sinkItem =
      dynamic_cast<pqPipelineModelFilter *>(this->getModelItemFor(sink));
  if(!sinkItem)
    {
    qDebug() << "Connection sink not found in the pipeline model.";
    return;
    }

  this->addConnection(sourceItem, sinkItem);
}

void pqPipelineModel::removeConnection(pqPipelineSource *source,
    pqPipelineSource *sink)
{
  // Ignore disconnect when cleaning up a server since it has already
  // been removed.
  if(source->getServer() == this->Internal->CleanupServer)
    {
    return;
    }

  pqPipelineModelSource *sourceItem =
      dynamic_cast<pqPipelineModelSource *>(this->getModelItemFor(source));
  if(!sourceItem)
    {
    qDebug() << "Connection source not found in the pipeline model.";
    return;
    }

  pqPipelineModelFilter *sinkItem =
      dynamic_cast<pqPipelineModelFilter *>(this->getModelItemFor(sink));
  if(!sinkItem)
    {
    qDebug() << "Connection sink not found in the pipeline model.";
    return;
    }

  this->removeConnection(sourceItem, sinkItem);
}

void pqPipelineModel::updateItemName(pqServerManagerModelItem *item)
{
  pqPipelineModelItem *modelItem = this->getModelItemFor(item);
  if(modelItem)
    {
    // Update the name column.
    QModelIndex changed = this->makeIndex(modelItem, 0);
    emit this->dataChanged(changed, changed);

    // If the item is a fan in point, update the link items.
    this->updateInputLinks(dynamic_cast<pqPipelineModelFilter *>(modelItem));
    }
}

void pqPipelineModel::updateDisplays(pqPipelineSource *source,
    pqConsumerDisplay*)
{
  pqPipelineModelItem *item = this->getModelItemFor(source);
  if(item)
    {
    // Update the current window column.
    QModelIndex changed = this->makeIndex(item, 1);
    emit this->dataChanged(changed, changed);

    // TODO: Update the column with the display list.
    // If the item is a fan in point, update the link items.
    this->updateInputLinks(dynamic_cast<pqPipelineModelFilter *>(source), 1);
    }
}

void pqPipelineModel::setViewModule(pqGenericViewModule *module)
{
  if(module == this->Internal->RenderModule)
    {
    return;
    }

  // Update the current view column for the previous render module.
  // If the render modules are from different servers, the whole
  // column needs to be updated. Otherwise, use the previous and
  // current render module to look up the affected sources.
  QModelIndex changed;
  pqPipelineModelItem *item = 0;
  if(this->Internal->RenderModule && module &&
      this->Internal->RenderModule->getServer() != module->getServer())
    {
    this->Internal->RenderModule = module;
    if(this->Internal->Servers.size() > 0)
      {
      item = this->Internal->Servers.first();
      }

    while(item)
      {
      changed = this->makeIndex(item, 1);
      emit this->dataChanged(changed, changed);
      item = this->getNextModelItem(item);
      }
    }
  else
    {
    if(this->Internal->RenderModule)
      {
      this->updateDisplays(this->Internal->RenderModule);
      }

    this->Internal->RenderModule = module;
    if(this->Internal->RenderModule)
      {
      this->updateDisplays(this->Internal->RenderModule);
      }
    }
}

void pqPipelineModel::addConnection(pqPipelineModelSource *source,
    pqPipelineModelFilter *sink)
{
  pqPipelineModelServer *server = source->getServer();
  if(!server)
    {
    return;
    }

  int row = 0;
  QModelIndex parentIndex;
  if(sink->getInputs().size() == 0)
    {
    // The sink item needs to be moved from the server's source list
    // to the source's output list. Notify observers that the sink
    // will be temporarily removed.
    emit this->movingIndex(this->makeIndex(sink));
    parentIndex = this->makeIndex(server);
    row = server->getChildIndex(sink);
    this->beginRemoveRows(parentIndex, row, row);
    server->getSources().removeAll(sink);
    this->endRemoveRows();

    parentIndex = this->makeIndex(source);
    row = source->getChildCount();
    this->beginInsertRows(parentIndex, row, row);
    source->getOutputs().append(sink);
    sink->getInputs().append(source);
    this->endInsertRows();
    if(source->getChildCount() == 1)
      {
      emit this->firstChildAdded(parentIndex);
      }

    emit this->indexRestored(this->makeIndex(sink));
    }
  else
    {
    // The other cases both require a link object.
    pqPipelineModelLink *link = new pqPipelineModelLink(server);
    if(!link)
      {
      return;
      }

    link->setSource(source);
    link->setSink(sink);
    if(sink->getInputs().size() == 1)
      {
      // If the sink has one input, it needs to be moved to the server's
      // source list. An additional link item needs to be added in place
      // of the sink item.
      pqPipelineModelLink *otherLink = new pqPipelineModelLink(server);
      if(!otherLink)
        {
        delete link;
        return;
        }

      pqPipelineModelSource *otherSource = sink->getInputs().first();
      otherLink->setSource(otherSource);
      otherLink->setSink(sink);

      emit this->movingIndex(this->makeIndex(sink));
      parentIndex = this->makeIndex(otherSource);
      row = otherSource->getChildIndex(sink);
      this->beginRemoveRows(parentIndex, row, row);
      otherSource->getOutputs().removeAll(sink);
      this->endRemoveRows();

      int serverRow = server->getChildCount();
      this->beginInsertRows(this->makeIndex(server), serverRow, serverRow);
      sink->getInputs().append(source);
      server->getSources().append(sink);
      this->endInsertRows();

      this->beginInsertRows(parentIndex, row, row);
      otherSource->getOutputs().insert(row, otherLink);
      this->endInsertRows();
      if(otherSource->getChildCount() == 1)
        {
        emit this->firstChildAdded(parentIndex);
        }

      emit this->indexRestored(this->makeIndex(sink));
      }
    else
      {
      sink->getInputs().append(source);
      }

    // A link item needs to be added to the source's output list.
    parentIndex = this->makeIndex(source);
    row = source->getChildCount();
    this->beginInsertRows(parentIndex, row, row);
    source->getOutputs().append(link);
    this->endInsertRows();
    if(source->getChildCount() == 1)
      {
      emit this->firstChildAdded(parentIndex);
      }
    }
}

void pqPipelineModel::removeConnection(pqPipelineModelSource *source,
    pqPipelineModelFilter *sink)
{
  pqPipelineModelServer *server = source->getServer();
  if(!server)
    {
    return;
    }

  int row = 0;
  QModelIndex parentIndex;
  if(sink->getInputs().size() == 1)
    {
    // The sink needs to be moved to the server's source list when
    // the last input is removed.
    emit this->movingIndex(this->makeIndex(sink));
    parentIndex = this->makeIndex(source);
    row = source->getChildIndex(sink);
    this->beginRemoveRows(parentIndex, row, row);
    sink->getInputs().removeAll(source);
    source->getOutputs().removeAll(sink);
    this->endRemoveRows();

    parentIndex = this->makeIndex(server);
    row = server->getChildCount();
    this->beginInsertRows(parentIndex, row, row);
    server->getSources().append(sink);
    this->endInsertRows();
    emit this->indexRestored(this->makeIndex(sink));
    }
  else
    {
    row = source->getChildIndex(sink);
    pqPipelineModelLink *link = dynamic_cast<pqPipelineModelLink *>(
        source->getChild(row));
    if(sink->getInputs().size() == 2)
      {
      // The sink item needs to be moved from the server's source list
      // to the other source's output list. The link item needs to be
      // removed as well.
      row = sink->getInputs().indexOf(source) == 0 ? 1 : 0;
      pqPipelineModelSource *otherSource = sink->getInputs().at(row);
      row = otherSource->getChildIndex(sink);
      pqPipelineModelLink *otherLink = dynamic_cast<pqPipelineModelLink *>(
          otherSource->getChild(row));

      parentIndex = this->makeIndex(otherSource);
      this->beginRemoveRows(parentIndex, row, row);
      otherSource->getOutputs().removeAll(otherLink);
      this->endRemoveRows();
      delete otherLink;

      emit this->movingIndex(this->makeIndex(sink));
      QModelIndex serverIndex = this->makeIndex(server);
      int serverRow = server->getChildIndex(sink);
      this->beginRemoveRows(serverIndex, serverRow, serverRow);
      server->getSources().removeAll(sink);
      this->endRemoveRows();

      this->beginInsertRows(parentIndex, row, row);
      sink->getInputs().removeAll(source);
      otherSource->getOutputs().insert(row, sink);
      this->endInsertRows();
      if(otherSource->getChildCount() == 1)
        {
        emit this->firstChildAdded(parentIndex);
        }

      emit this->indexRestored(this->makeIndex(sink));
      }
    else
      {
      sink->getInputs().removeAll(source);
      }

    // The link item in the source's output needs to be removed.
    parentIndex = this->makeIndex(source);
    row = source->getChildIndex(link);
    this->beginRemoveRows(parentIndex, row, row);
    source->getOutputs().removeAll(link);
    this->endRemoveRows();
    delete link;
    }
}

void pqPipelineModel::updateDisplays(pqGenericViewModule *module)
{
  QModelIndex changed;
  pqPipelineModelItem *item = 0;
  pqConsumerDisplay* display = 0;
  int total = module->getDisplayCount();
  for(int i = 0; i < total; i++)
    {
    display = qobject_cast<pqConsumerDisplay*>(module->getDisplay(i));
    if(display)
      {
      item = this->getModelItemFor(display->getInput());
      if(item)
        {
        changed = this->makeIndex(item, 1);
        emit this->dataChanged(changed, changed);

        // If the item is a fan in point, update the link items.
        this->updateInputLinks(dynamic_cast<pqPipelineModelFilter *>(item), 1);
        }
      }
    }
}

void pqPipelineModel::updateInputLinks(pqPipelineModelFilter *sink, int column)
{
  if(sink && sink->getInputs().size() > 1)
    {
    QList<pqPipelineModelSource *>::Iterator source =
        sink->getInputs().begin();
    for( ; source != sink->getInputs().end(); ++source)
      {
      pqPipelineModelLink *link = dynamic_cast<pqPipelineModelLink *>(
          (*source)->getChild((*source)->getChildIndex(sink)));
      if(link)
        {
        QModelIndex changed = this->makeIndex(link, column);
        emit this->dataChanged(changed, changed);
        }
      }
    }
}

pqPipelineModelItem *pqPipelineModel::getModelItemFor(
    pqServerManagerModelItem *item) const
{
  QMap<pqServerManagerModelItem *, QPointer<pqPipelineModelItem> >::Iterator iter =
      this->Internal->ItemMap.find(item);
  if(iter != this->Internal->ItemMap.end())
    {
    return *iter;
    }

  return 0;
}

QModelIndex pqPipelineModel::makeIndex(pqPipelineModelItem *item,
    int column) const
{
  int row = 0;
  pqPipelineModelServer *server = dynamic_cast<pqPipelineModelServer *>(item);
  if(server)
    {
    row = this->Internal->Servers.indexOf(server);
    return this->createIndex(row, column, item);
    }
  else
    {
    row = item->getParent()->getChildIndex(item);
    return this->createIndex(row, column, item);
    }
}

void pqPipelineModel::cleanPipelineMap()
{
  // Clean out all the items in the map with null pointers.
  QMap<pqServerManagerModelItem *, QPointer<pqPipelineModelItem> >::Iterator iter =
      this->Internal->ItemMap.begin();
  while(iter != this->Internal->ItemMap.end())
    {
    if(iter->isNull())
      {
      iter = this->Internal->ItemMap.erase(iter);
      }
    else
      {
      ++iter;
      }
    }
}

pqPipelineModelItem *pqPipelineModel::getNextModelItem(
    pqPipelineModelItem *item, pqPipelineModelItem *root) const
{
  if(item->getChildCount() > 0)
    {
    return item->getChild(0);
    }

  // Search up the ancestors for an item with multiple children.
  // The next item will be the next child.
  int row = 0;
  int count = 0;
  pqPipelineModelItem *itemParent = 0;
  pqPipelineModelServer *server = 0;
  while(item != root)
    {
    itemParent = item->getParent();
    if(itemParent)
      {
      count = itemParent->getChildCount();
      if(count > 1)
        {
        row = itemParent->getChildIndex(item) + 1;
        if(row < count)
          {
          return itemParent->getChild(row);
          }
        }

      item = itemParent;
      }
    else
      {
      server = dynamic_cast<pqPipelineModelServer *>(item);
      if(!server)
        {
        break;
        }

      count = this->Internal->Servers.size();
      row = this->Internal->Servers.indexOf(server) + 1;
      if(row < 0 || row >= count)
        {
        break;
        }
      else
        {
        return this->Internal->Servers[row];
        }
      }
    }

  return 0;
}

void pqPipelineModel::initializePixmaps()
{
  if(this->PixmapList == 0)
    {
    this->PixmapList = new QPixmap[pqPipelineModelInternal::Total];
    this->PixmapList[pqPipelineModel::Server].load(
        ":/pqCore/Icons/pqServer16.png");
    this->PixmapList[pqPipelineModel::Source].load(
        ":/pqCore/Icons/pqSource16.png");
    this->PixmapList[pqPipelineModel::Filter].load(
        ":/pqCore/Icons/pqFilter16.png");
    this->PixmapList[pqPipelineModel::CustomFilter].load(
        ":/pqCore/Icons/pqBundle16.png");
    this->PixmapList[pqPipelineModel::Link].load(
        ":/pqCore/Icons/pqLinkBack16.png");
    this->PixmapList[pqPipelineModelInternal::Eyeball].load(
        ":/pqCore/Icons/pqEyeball16.png");
    this->PixmapList[pqPipelineModelInternal::EyeballGray].load(
        ":/pqCore/Icons/pqEyeballd16.png");
    }
}


