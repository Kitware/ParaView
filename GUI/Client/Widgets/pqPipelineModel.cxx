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

#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqRenderModule.h"
#include "pqServer.h"
#include "pqServerManagerModelItem.h"

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
  pqPipelineModelItem(QObject *parent=0);
  virtual ~pqPipelineModelItem() {}

  virtual QString getName() const=0;
  virtual bool isVisible(pqRenderModule *module) const=0;
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
  virtual bool isVisible(pqRenderModule *) const {return false;}
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
  virtual bool isVisible(pqRenderModule *module) const;
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
  virtual bool isVisible(pqRenderModule *module) const;
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
    Total
    };

public:
  pqPipelineModelInternal();
  ~pqPipelineModelInternal() {}

  QList<pqPipelineModelServer *> Servers;
  QMap<pqServerManagerModelItem *, QPointer<pqPipelineModelItem> > ItemMap;
  pqRenderModule *CurrentView;
  pqServer *CleanupServer;
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
    return this->Server->getFriendlyName();
    }

  return QString();
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

bool pqPipelineModelSource::isVisible(pqRenderModule *module) const
{
  pqPipelineDisplay *display = this->Source->getDisplay(module);
  if(display)
    {
    return display->isVisible();
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
  if(type != pqPipelineModel::Bundle)
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

bool pqPipelineModelLink::isVisible(pqRenderModule *module) const
{
  if(this->Sink)
    {
    return this->Sink->isVisible(module);
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
  : Servers(), ItemMap()
{
  this->CurrentView = 0;
  this->CleanupServer = 0;
}


//-----------------------------------------------------------------------------
pqPipelineModel::pqPipelineModel(QObject *p)
  : QAbstractItemModel(p)
{
  this->Internal = new pqPipelineModelInternal();

  // Initialize the pixmap list.
  Q_INIT_RESOURCE(pqWidgets);
  this->PixmapList = new QPixmap[pqPipelineModelInternal::Total];
  if(this->PixmapList)
    {
    this->PixmapList[pqPipelineModel::Server].load(
        ":/pqWidgets/pqServer16.png");
    this->PixmapList[pqPipelineModel::Source].load(
        ":/pqWidgets/pqSource16.png");
    this->PixmapList[pqPipelineModel::Filter].load(
        ":/pqWidgets/pqFilter16.png");
    this->PixmapList[pqPipelineModel::Bundle].load(
        ":/pqWidgets/pqBundle16.png");
    this->PixmapList[pqPipelineModel::Link].load(
        ":/pqWidgets/pqLinkBack16.png");
    this->PixmapList[pqPipelineModelInternal::Eyeball].load(
        ":/pqWidgets/pqEyeball16.png");
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
          if(idx.column() == 1 && item->isVisible(this->Internal->CurrentView))
            {
            return QVariant(QIcon(
                this->PixmapList[pqPipelineModelInternal::Eyeball]));
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
    indexFlags |= Qt::ItemIsSelectable;
    }

  return indexFlags;
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
    item = new pqPipelineModelFilter(server, source, pqPipelineModel::Bundle);
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
    qDebug() << "Unknown source proxy type added to the pipeline model.";
    return;
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
  // TODO: If the item is a fan in point, update the link items.
  // The name is in column 0 so the getIndexFor method will work.
  QModelIndex changed = this->getIndexFor(item);
  if(changed.isValid())
    {
    emit this->dataChanged(changed, changed);
    }
}

void pqPipelineModel::updateDisplays(pqPipelineSource *source,
    pqPipelineDisplay *)
{
  pqPipelineModelItem *item = this->getModelItemFor(source);
  if(item)
    {
    // Update the current window column.
    QModelIndex changed = this->makeIndex(item, 1);
    emit this->dataChanged(changed, changed);

    // TODO: Update the column with the display list.
    // TODO: If the item is a fan in point, update the link items.
    }
}

void pqPipelineModel::updateCurrentWindow(pqRenderModule *module)
{
  if(module == this->Internal->CurrentView)
    {
    return;
    }

  // Update the current view column for the previous render module.
  // If the render modules are from different servers, the whole
  // column needs to be updated. Otherwise, use the previous and
  // current render module to look up the affected sources.
  QModelIndex changed;
  pqPipelineModelItem *item = 0;
  if(this->Internal->CurrentView && module &&
      this->Internal->CurrentView->getServer() != module->getServer())
    {
    this->Internal->CurrentView = module;
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
    if(this->Internal->CurrentView)
      {
      this->updateDisplays(this->Internal->CurrentView);
      }

    this->Internal->CurrentView = module;
    if(this->Internal->CurrentView)
      {
      this->updateDisplays(this->Internal->CurrentView);
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

      QModelIndex serverIndex = this->makeIndex(server);
      int serverRow = server->getChildCount();
      this->beginInsertRows(serverIndex, serverRow, serverRow);
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
      otherSource->getOutputs().insert(row, sink);
      this->endInsertRows();
      if(otherSource->getChildCount() == 1)
        {
        emit this->firstChildAdded(parentIndex);
        }

      emit this->indexRestored(this->makeIndex(sink));
      }

    // The link item in the source's output needs to be removed.
    parentIndex = this->makeIndex(source);
    row = source->getChildIndex(link);
    this->beginRemoveRows(parentIndex, row, row);
    source->getOutputs().removeAll(link);
    sink->getInputs().removeAll(source);
    this->endRemoveRows();
    delete link;
    }
}

void pqPipelineModel::updateDisplays(pqRenderModule *module)
{
  QModelIndex changed;
  pqPipelineModelItem *item = 0;
  pqPipelineDisplay *display = 0;
  int total = module->getDisplayCount();
  for(int i = 0; i < total; i++)
    {
    display = module->getDisplay(i);
    if(display)
      {
      item = this->getModelItemFor(display->getInput());
      if(item)
        {
        changed = this->makeIndex(item, 1);
        emit this->dataChanged(changed, changed);
        // TODO: If the item is a fan in point, update the link items.
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
    pqPipelineModelItem *item) const
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
  while(true)
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

/*
void pqPipelineModel::saveState(vtkPVXMLElement *vtkNotUsed(root), 
  pqMultiView *vtkNotUsed(multiView))
{
  if(!root)
    {
    return;
    }

  // Create an element to hold the pipeline state.
  vtkPVXMLElement *pipeline = vtkPVXMLElement::New();
  pipeline->SetName("Pipeline");

  // Save the state for each of the servers.
  QString address;
  pqServer *server = 0;
  vtkPVXMLElement *element = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->ServerList.begin();
  for( ; iter != this->Internal->ServerList.end(); ++iter)
    {
    element = vtkPVXMLElement::New();
    element->SetName("Server");

    // Save the connection information.
    server = (*iter)->GetServer();
    address = server->getAddress();
    element->AddAttribute("address", address.toAscii().data());
    if(address != server->getFriendlyName())
      {
      element->AddAttribute("name", server->getFriendlyName().toAscii().data());
      }

    (*iter)->SaveState(element, multiView);
    pipeline->AddNestedElement(element);
    element->Delete();
    }

  // Add the pipeline element to the xml.
  root->AddNestedElement(pipeline);
  pipeline->Delete();
}
*/

