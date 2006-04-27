/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineModel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqMultiView.h"
#include "pqPipelineData.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPipelineLink.h"
#include "pqPipelineModelItem.h"
#include "pqPipelineObject.h"
#include "pqPipelineServer.h"
#include "pqPipelineSource.h"
#include "pqServer.h"

#include <QApplication>
#include <QString>
#include <QStyle>
#include "QVTKWidget.h"

#include "vtkPVXMLElement.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"


class pqPipelineModelInternal : public QList<pqPipelineServer *> {};


pqPipelineModel::pqPipelineModel(QObject *p)
  : QAbstractItemModel(p)
{
  this->Internal = new pqPipelineModelInternal();
  this->IgnorePipeline = false;

  // Initialize the pixmap list.
  Q_INIT_RESOURCE(pqWidgets);
  this->PixmapList = new QPixmap[pqPipelineModel::Link + 1];
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
    }
}

pqPipelineModel::~pqPipelineModel()
{
  if(this->Internal)
    {
    // Clean up the server objects, which will clean up all the internal
    // pipeline objects.
    QList<pqPipelineServer *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      if(*iter)
        {
        delete *iter;
        }
      }

    delete this->Internal;
    }

  if(this->PixmapList)
    {
    delete [] this->PixmapList;
    }
}

int pqPipelineModel::rowCount(const QModelIndex &parentIndex) const
{
  if(parentIndex.isValid())
    {
    // Only handle indexes from this model.
    if(parentIndex.model() != this)
      {
      return 0;
      }

    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        parentIndex.internalPointer());

    // See if the item is a server or an object.
    pqPipelineServer *server = dynamic_cast<pqPipelineServer *>(item);
    pqPipelineSource *object = dynamic_cast<pqPipelineSource *>(item);
    if(server)
      {
      return server->GetSourceCount();
      }
    else if(object)
      {
      return object->GetOutputCount();
      }
    }
  else
    {
    // The index refers to the model root. The number of rows is
    // the number of servers.
    return this->getServerCount();
    }

  return 0;
}

int pqPipelineModel::columnCount(const QModelIndex &) const
{
  return 1;
}

bool pqPipelineModel::hasChildren(const QModelIndex &parentIndex) const
{
  return this->rowCount(parentIndex) > 0;
}

QModelIndex pqPipelineModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  // Make sure the row and column number is within range.
  int rows = this->rowCount(parentIndex);
  int columns = this->columnCount(parentIndex);
  if(row < 0 || row >= rows || column < 0 || column >= columns)
    {
    return QModelIndex();
    }

  pqPipelineServer *server = 0;
  if(parentIndex.isValid())
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        parentIndex.internalPointer());
    server = dynamic_cast<pqPipelineServer *>(item);
    pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
    if(server)
      {
      source = server->GetSource(row);
      return this->createIndex(row, column, source);
      }
    else if(source)
      {
      pqPipelineObject *object = source->GetOutput(row);
      return this->createIndex(row, column, object);
      }
    }
  else
    {
    // The parent refers to the model root. Get the specified server.
    server = this->getServer(row);
    return this->createIndex(row, column, server);
    }

  return QModelIndex();
}

QModelIndex pqPipelineModel::parent(const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());

    pqPipelineModelItem *parentItem = this->getItemParent(item);
    if(parentItem)
      {
      int row = this->getItemRow(parentItem);
      return this->createIndex(row, 0, parentItem);
      }
    }

  return QModelIndex();
}

QVariant pqPipelineModel::data(const QModelIndex &idx, int role) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());
    pqPipelineServer *server = dynamic_cast<pqPipelineServer *>(item);
    pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
    pqPipelineLink *link = dynamic_cast<pqPipelineLink *>(item);
    switch(role)
      {
      case Qt::DisplayRole:
      case Qt::ToolTipRole:
      case Qt::EditRole:
        {
        if(server)
          {
          if(idx.column() == 0)
            {
            return QVariant(server->GetServer()->getFriendlyName());
            }
          }
        else if(source)
          {
          if(idx.column() == 0)
            {
            return QVariant(source->GetProxyName());
            }
          }
        else if(link && link->GetLink())
          {
          if(idx.column() == 0)
            {
            return QVariant(link->GetLink()->GetProxyName());
            }
          }

        break;
        }
      case Qt::DecorationRole:
        {
        if(idx.column() == 0 && this->PixmapList)
          {
          //if(server)
          //  {
          //  return QVariant(QApplication::style()->standardIcon(
          //      QStyle::SP_ComputerIcon));
          //  }
          if(item && item->GetType() != pqPipelineModel::Invalid)
            {
            return QVariant(this->PixmapList[item->GetType()]);
            }
          }

        break;
        }
      }
    }

  return QVariant();
}

Qt::ItemFlags pqPipelineModel::flags(const QModelIndex &) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

pqPipelineModel::ItemType pqPipelineModel::getTypeFor(
    const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());
    if(item)
      {
      return item->GetType();
      }
    }

  return pqPipelineModel::Invalid;
}

vtkSMProxy *pqPipelineModel::getProxyFor(const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());
    pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
    if(source)
      {
      return source->GetProxy();
      }

    pqPipelineLink *link = dynamic_cast<pqPipelineLink *>(item);
    if(link && link->GetLink())
      {
      return link->GetLink()->GetProxy();
      }
    }

  return 0;
}

pqPipelineObject *pqPipelineModel::getObjectFor(const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());
    return dynamic_cast<pqPipelineObject *>(item);
    }

  return 0;
}

pqPipelineSource *pqPipelineModel::getSourceFor(const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());
    pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
    if(!source)
      {
      pqPipelineLink *link = dynamic_cast<pqPipelineLink *>(item);
      if(link)
        {
        source = link->GetLink();
        }
      }

    return source;
    }

  return 0;
}

pqPipelineSource *pqPipelineModel::getSourceFor(vtkSMProxy *proxy) const
{
  if(this->Internal && proxy)
    {
    pqPipelineSource *source = 0;
    QList<pqPipelineServer *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      if(*iter)
        {
        source = (*iter)->GetObject(proxy);
        if(source)
          {
          return source;
          }
        }
      }
    }

  return 0;
}

pqPipelineServer *pqPipelineModel::getServerFor(const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelItem *item = reinterpret_cast<pqPipelineModelItem *>(
        idx.internalPointer());
    return dynamic_cast<pqPipelineServer *>(item);
    }

  return 0;
}

pqPipelineServer *pqPipelineModel::getServerFor(vtkSMProxy *proxy) const
{
  pqPipelineSource *source = this->getSourceFor(proxy);
  if(source)
    {
    return source->GetServer();
    }

  return 0;
}

pqPipelineServer *pqPipelineModel::getServerFor(pqServer *server) const
{
  if(this->Internal && server)
    {
    QList<pqPipelineServer *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      if((*iter)->GetServer() == server)
        {
        return *iter;
        }
      }
    }

  return 0;
}

QModelIndex pqPipelineModel::getIndexFor(vtkSMProxy *proxy) const
{
  if(this->Internal && proxy)
    {
    pqPipelineSource *source = 0;
    QList<pqPipelineServer *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      if(*iter)
        {
        source = (*iter)->GetObject(proxy);
        if(source)
          {
          return this->getIndexFor(source);
          }
        }
      }
    }

  return QModelIndex();
}

QModelIndex pqPipelineModel::getIndexFor(pqPipelineObject *object) const
{
  int row = this->getItemRow(object);
  if(row != -1)
    {
    return this->createIndex(row, 0, object);
    }

  return QModelIndex();
}

QModelIndex pqPipelineModel::getIndexFor(pqPipelineServer *server) const
{
  int row = this->getServerIndexFor(server);
  if(row != -1)
    {
    return this->createIndex(row, 0, server);
    }

  return QModelIndex();
}

int pqPipelineModel::getServerCount() const
{
  if(this->Internal)
    {
    return this->Internal->size();
    }

  return 0;
}

pqPipelineServer *pqPipelineModel::getServer(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->size())
    {
    return (*this->Internal)[index];
    }

  return 0;
}

int pqPipelineModel::getServerIndexFor(pqPipelineServer *server) const
{
  if(this->Internal && server)
    {
    return this->Internal->indexOf(server);
    }

  return -1;
}

void pqPipelineModel::clearPipelines()
{
  if(this->Internal)
    {
    // Clean up the server objects, which will clean up all the internal
    // pipeline objects.
    this->IgnorePipeline = true;
    QList<pqPipelineServer *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      if(*iter)
        {
        (*iter)->ClearPipelines();
        delete *iter;
        *iter = 0;
        }
      }

    this->Internal->clear();
    this->IgnorePipeline = false;
    this->reset();
    }
}

void pqPipelineModel::addServer(pqServer *server)
{
  if(!this->Internal || !server)
    {
    return;
    }

  // Make sure the server doesn't already exist.
  QList<pqPipelineServer *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter)
    {
    if(*iter && (*iter)->GetServer() == server)
      {
      return;
      }
    }

  pqPipelineServer *object = new pqPipelineServer();
  if(object)
    {
    object->SetServer(server);

    // Notify the view that a new server is being added. Then, add
    // the server to the internal list.
    int row = this->Internal->size();
    this->beginInsertRows(QModelIndex(), row, row);
    this->Internal->append(object);
    this->endInsertRows();
    }
}

void pqPipelineModel::removeServer(pqServer *server)
{
  this->removeServer(this->getServerFor(server));
}

void pqPipelineModel::removeServer(pqPipelineServer *server)
{
  if(!this->Internal || !server || this->IgnorePipeline)
    {
    return;
    }

  // Notify the view that the server is being removed. Then,
  // clean up the server representation.
  int row = this->getServerIndexFor(server);
  if(row != -1)
    {
    this->beginRemoveRows(QModelIndex(), row, row);
    this->Internal->removeAll(server);
    this->endRemoveRows();

    // TODO: Disconnect all the proxies before cleaning up.
    server->ClearPipelines();

    // TODO: Remove all the render windows for the server.
    delete server;
    }
}

void pqPipelineModel::addWindow(QWidget *window, pqServer *server)
{
  if(!window || !server)
    {
    return;
    }

  // Find the server from the list of servers. Make sure the window
  // is not already in the list.
  pqPipelineServer *serverObject = this->getServerFor(server);
  if(serverObject && !serverObject->HasWindow(window))
    {
    serverObject->AddToWindowList(window);
    }
}

void pqPipelineModel::removeWindow(QWidget *window)
{
  if(!this->Internal || !window)
    {
    return;
    }

  // Search for the server that has the specified widget.
  QList<pqPipelineServer *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter)
    {
    if((*iter)->HasWindow(window))
      {
      (*iter)->RemoveFromWindowList(window);
      // TODO: Remove all displays referencing the window.
      // Notify the view of the changes.
      break;
      }
    }
}

void pqPipelineModel::addSource(vtkSMProxy *source, const QString &name,
    pqServer *server)
{
  if(!this->Internal || !source || !server)
    {
    return;
    }

  // Get the server object from the list.
  pqPipelineServer *serverObject = this->getServerFor(server);
  if(!serverObject)
    {
    return;
    }

  // Make sure the source doesn't already exist.
  pqPipelineSource *object = serverObject->GetObject(source);
  if(object)
    {
    return;
    }

  // Create a new pipeline source object for the source.
  object = new pqPipelineSource(source);
  if(object)
    {
    object->SetServer(serverObject);
    object->SetProxyName(name);

    // Add the source to the server.
    this->addItemAsSource(object, serverObject);
    }
}

void pqPipelineModel::addFilter(vtkSMProxy *filter, const QString &name,
    pqServer *server)
{
  if(!this->Internal || !filter || !server)
    {
    return;
    }

  // Get the server object from the list.
  pqPipelineServer *serverObject = this->getServerFor(server);
  if(!serverObject)
    {
    return;
    }

  // Make sure the filter doesn't already exist.
  pqPipelineSource *source = serverObject->GetObject(filter);
  if(source)
    {
    return;
    }

  // Create a new pipeline filter object for the filter.
  pqPipelineFilter *object = new pqPipelineFilter(filter);
  if(object)
    {
    object->SetServer(serverObject);
    object->SetProxyName(name);

    // Add the filter to the server.
    this->addItemAsSource(object, serverObject);
    }
}

void pqPipelineModel::addBundle(vtkSMProxy *bundle, const QString &name,
    pqServer *server)
{
  if(!this->Internal || !bundle || !server)
    {
    return;
    }

  // Get the server object from the list.
  pqPipelineServer *serverObject = this->getServerFor(server);
  if(!serverObject)
    {
    return;
    }

  // Make sure the bundle doesn't already exist.
  pqPipelineSource *source = serverObject->GetObject(bundle);
  if(source)
    {
    return;
    }

  // Create a new pipeline filter object for the bundle.
  pqPipelineFilter *object = new pqPipelineFilter(bundle,
      pqPipelineModel::Bundle);
  if(object)
    {
    object->SetServer(serverObject);
    object->SetProxyName(name);

    // Add the bundle to the server.
    this->addItemAsSource(object, serverObject);
    }
}

void pqPipelineModel::removeObject(vtkSMProxy *proxy)
{
  this->removeObject(this->getSourceFor(proxy));
}

void pqPipelineModel::removeObject(pqPipelineSource *source)
{
  if(!this->Internal || !source || this->IgnorePipeline)
    {
    return;
    }

  // Get the server from the source object. Make sure the server
  // is part of this model.
  pqPipelineServer *server = source->GetServer();
  if(!server || !this->Internal->contains(server))
    {
    return;
    }

  // If the object has links in its outputs, those connections
  // need to be handled before removing the object. If an output
  // goes to an object that has only one other input, that object
  // will need to be moved in the tree. Leave the link object in
  // the output list to reduce the changes to the view.
  int i = 0;
  pqPipelineLink *link = 0;
  for( ; i < source->GetOutputCount(); i++)
    {
    link = dynamic_cast<pqPipelineLink *>(source->GetOutput(i));
    if(link)
      {
      this->removeLink(link);
      }
    }

  // If the object has more than one input, the link items
  // referencing it need to be removed. If the object only has
  // one input, that connection will be broken when the object
  // is removed from the model.
  int row = 0;
  QModelIndex parentIndex;
  pqPipelineSource *input = 0;
  pqPipelineFilter *filter = dynamic_cast<pqPipelineFilter *>(source);
  if(filter && filter->GetInputCount() > 1)
    {
    for(i = 0; i < filter->GetInputCount(); i++)
      {
      input = filter->GetInput(i);
      row = input->GetOutputIndexFor(filter);
      link = dynamic_cast<pqPipelineLink *>(input->GetOutput(row));
      if(link)
        {
        parentIndex = this->createIndex(this->getItemRow(input), 0, input);
        this->beginRemoveRows(parentIndex, row, row);
        input->RemoveOutput(link);
        this->endRemoveRows();
        delete link;
        }
      }
    }

  // Remove the object from the model. The object could be a child
  // of a server or another object.
  pqPipelineModelItem *parentItem = this->getItemParent(source);
  parentIndex = this->createIndex(this->getItemRow(parentItem), 0, parentItem);
  row = this->getItemRow(source);
  this->beginRemoveRows(parentIndex, row, row);
  if(parentItem->GetType() != pqPipelineModel::Server)
    {
    input = dynamic_cast<pqPipelineSource *>(parentItem);
    input->RemoveOutput(source);
    }

  // This will also unregister the object and its displays.
  this->IgnorePipeline = true;
  server->RemoveObject(source);
  this->IgnorePipeline = false;
  this->endRemoveRows();

  // Remove the proxy connections in the background. Breaking these
  // connections shouldn't change the view. Put the orphaned output
  // chains back in the model hierarchy.
  QList<pqPipelineFilter *> orphans;
  for(i = 0; i < source->GetOutputCount(); i++)
    {
    filter = dynamic_cast<pqPipelineFilter *>(source->GetOutput(i));
    if(filter)
      {
      // Only the filters in the output list need to be added to
      // the server's source list.
      filter->RemoveInput(source);
      orphans.append(filter);

      // Remove the input in the proxy as well.
      this->IgnorePipeline = true;
      pqPipelineData::instance()->removeConnection(source->GetProxy(),
          filter->GetProxy());
      this->IgnorePipeline = false;
      }
    }

  if(orphans.size() > 0)
    {
    parentIndex = this->createIndex(this->getServerIndexFor(server), 0,
        server);
    int row = server->GetSourceCount();
    this->beginInsertRows(parentIndex, row, row + orphans.size() - 1);
    QList<pqPipelineFilter *>::Iterator iter = orphans.begin();
    for( ; iter != orphans.end(); ++iter)
      {
      server->AddToSourceList(*iter);
      }

    this->endInsertRows();
    }

  // Finally, Clean up the object.
  delete source;
}

void pqPipelineModel::extractObject(pqPipelineFilter *filter)
{
  if(!this->Internal || !filter || this->IgnorePipeline)
    {
    return;
    }

  // Get the server from the source object. Make sure the server
  // is part of this model.
  pqPipelineServer *server = filter->GetServer();
  if(!server || !this->Internal->contains(server))
    {
    return;
    }

  // If the object has no inputs, this method is the same as the
  // removeObject method. If the object has more than one input and
  // more than one output, the reconnection is indeterminate. In
  // that case, use the removeObject method instead.
  if(filter->GetInputCount() == 0 || filter->GetOutputCount() == 0 ||
      (filter->GetInputCount() > 1 && filter->GetOutputCount() > 1))
    {
    this->removeObject(filter);
    return;
    }

  if(filter->GetOutputCount() > 1)
    {
    }
  else
    {
    }
}

void pqPipelineModel::removeBranch(pqPipelineSource *source)
{
  if(!this->Internal || !source || this->IgnorePipeline)
    {
    return;
    }

  // Get the server from the source object. Make sure the server
  // is part of this model.
  pqPipelineServer *server = source->GetServer();
  if(!server || !this->Internal->contains(server))
    {
    return;
    }

  // If there are link object in the branch, the connections need
  // to be broken before removing the branch from the tree. Leave
  // the empty link items in the tree to reduce view changes.

  // If the object has multiple inputs, the link items referencing
  // the object need to be removed.

  // Remove the proxy connections for the entire branch in the
  // background. Breaking these connections shouldn't change the view.

  // Clean up all the model items.
}

void pqPipelineModel::addConnection(vtkSMProxy *source, vtkSMProxy *sink)
{
  if(!this->Internal || !source || !sink)
    {
    return;
    }

  pqPipelineSource *sourceItem = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter)
    {
    if(*iter)
      {
      sourceItem = (*iter)->GetObject(source);
      if(sourceItem)
        {
        this->addConnection(sourceItem, dynamic_cast<pqPipelineFilter *>(
            (*iter)->GetObject(sink)));
        break;
        }
      }
    }
}

void pqPipelineModel::addConnection(pqPipelineSource *source,
    pqPipelineFilter *sink)
{
  if(!this->Internal || !source || !sink)
    {
    return;
    }

  // Make sure the items are not already connected.
  if(sink->HasInput(source))
    {
    return;
    }

  // The server should be the same for both items.
  pqPipelineServer *server = sink->GetServer();
  int serverRow = this->getServerIndexFor(server);
  if(serverRow == -1 || server != source->GetServer())
    {
    return;
    }

  int row = 0;
  QModelIndex parentIndex;
  if(sink->GetInputCount() == 0)
    {
    // The sink item needs to be moved from the server's source list
    // to the source's output list.
    parentIndex = this->createIndex(serverRow, 0, server);
    row = server->GetSourceIndexFor(sink);
    this->beginRemoveRows(parentIndex, row, row);
    server->RemoveFromSourceList(sink);
    this->endRemoveRows();

    parentIndex = this->createIndex(this->getItemRow(source), 0, source);
    row = source->GetOutputCount();
    this->beginInsertRows(parentIndex, row, row);
    source->AddOutput(sink);
    sink->AddInput(source);
    this->endInsertRows();

    if(row == 0)
      {
      emit this->firstChildAdded(parentIndex);
      }
    }
  else if(sink->GetInputCount() == 1)
    {
    // If the sink has one input, it needs to be moved to the server's
    // source list with an additional input. Add a link item to the
    // source in place of the sink.
    }
  else
    {
    // The sink item is already on the server's source list. A link
    // item needs to be added to the source for the new output.
    }
}

void pqPipelineModel::removeConnection(vtkSMProxy *source, vtkSMProxy *sink)
{
  if(!this->Internal || !source || !sink || this->IgnorePipeline)
    {
    return;
    }

  pqPipelineSource *sourceItem = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter)
    {
    if(*iter)
      {
      sourceItem = (*iter)->GetObject(source);
      if(sourceItem)
        {
        this->removeConnection(sourceItem, dynamic_cast<pqPipelineFilter *>(
            (*iter)->GetObject(sink)));
        break;
        }
      }
    }
}

void pqPipelineModel::removeConnection(pqPipelineSource *source,
    pqPipelineFilter *sink)
{
  if(!this->Internal || !source || !sink || this->IgnorePipeline)
    {
    return;
    }

  // Make sure the items are connected.
  if(!sink->HasInput(source))
    {
    return;
    }

  // The server should be the same for both items.
  pqPipelineServer *server = sink->GetServer();
  int serverRow = this->getServerIndexFor(server);
  if(serverRow == -1 || server != source->GetServer())
    {
    return;
    }

  int row = 0;
  QModelIndex parentIndex;
  if(sink->GetInputCount() == 1)
    {
    // The sink needs to be moved to the server's source list when
    // the last input is removed.
    parentIndex = this->createIndex(this->getItemRow(source), 0, source);
    row = source->GetOutputIndexFor(sink);
    this->beginRemoveRows(parentIndex, row, row);
    sink->RemoveInput(source);
    source->RemoveOutput(sink);
    this->endRemoveRows();

    parentIndex = this->createIndex(serverRow, 0, server);
    row = server->GetSourceCount();
    this->beginInsertRows(parentIndex, row, row);
    server->AddToSourceList(sink);
    this->endInsertRows();
    }
  else if(sink->GetInputCount() == 2)
    {
    // The sink item needs to be moved from the server's source list
    // to the other source's output list. The link items need to be
    // removed as well.
    }
  else
    {
    // Removing the input will not change the sink item's location.
    // The link item in the source's output needs to be removed.
    }
}

void pqPipelineModel::addDisplay(vtkSMDisplayProxy *display,
    const QString &name, vtkSMProxy *proxy, vtkSMRenderModuleProxy *module)
{
  if(!this->Internal || !proxy || !display)
    {
    return;
    }

  // Find the pipeline object associated with the proxy.
  // TODO: How to handle compound proxies?
  pqPipelineSource *source = this->getSourceFor(proxy);
  if(!source)
    {
    return;
    }

  // Make sure the source does not already have the display.
  pqPipelineDisplay *displayList = source->GetDisplay();
  if(!displayList || displayList->GetDisplayIndexFor(display) != -1)
    {
    return;
    }

  // Get the widget from the view map. Add the display/window pair
  // to the list.
  QWidget *window = pqPipelineData::instance()->getRenderWindow(module);
  displayList->AddDisplay(display, name, window);
}

void pqPipelineModel::removeDisplay(vtkSMDisplayProxy *display,
    vtkSMProxy *proxy)
{
  if(!this->Internal || !display || !proxy || this->IgnorePipeline)
    {
    return;
    }

  // Find the pipeline object associated with the proxy.
  // TODO: How to handle compound proxies?
  pqPipelineSource *source = this->getSourceFor(proxy);
  if(!source)
    {
    return;
    }

  // Remove the display from the display list.
  pqPipelineDisplay *displayList = source->GetDisplay();
  if(displayList)
    {
    displayList->RemoveDisplay(display);
    }
}

void pqPipelineModel::saveState(vtkPVXMLElement *root, pqMultiView *multiView)
{
  if(!root || !this->Internal)
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
  QList<pqPipelineServer *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter)
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

  // Save the server manager state. This should save the proxy
  // information for every server connection.
  element = vtkPVXMLElement::New();
  element->SetName("ServerManagerState");
  vtkSMObject::GetProxyManager()->SaveState(element);
  pipeline->AddNestedElement(element);
  element->Delete();

  // Add the pipeline element to the xml.
  root->AddNestedElement(pipeline);
  pipeline->Delete();
}

int pqPipelineModel::getItemRow(pqPipelineModelItem *item) const
{
  if(!this->Internal || !item)
    {
    return -1;
    }

  // If the parent is null, the item should be a server.
  pqPipelineModelItem *parentItem = this->getItemParent(item);
  if(parentItem)
    {
    pqPipelineServer *server = dynamic_cast<pqPipelineServer *>(parentItem);
    pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(parentItem);
    if(source)
      {
      return source->GetOutputIndexFor(dynamic_cast<pqPipelineObject *>(item));
      }
    else if(server)
      {
      return server->GetSourceIndexFor(dynamic_cast<pqPipelineSource *>(item));
      }
    }
  else if(item->GetType() == pqPipelineModel::Server)
    {
    return this->getServerIndexFor(dynamic_cast<pqPipelineServer *>(item));
    }

  return -1;
}

pqPipelineModelItem *pqPipelineModel::getItemParent(
    pqPipelineModelItem *item) const
{
  if(!item)
    {
    return 0;
    }

  // If the item is a server, the parent is the root. If the item
  // is a pipeline object, the parent could be a server or another
  // pipeline object.
  pqPipelineServer *server = 0;
  pqPipelineLink *link = dynamic_cast<pqPipelineLink *>(item);
  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
  pqPipelineFilter *filter = dynamic_cast<pqPipelineFilter *>(item);
  if(link)
    {
    return link->GetSource();
    }
  else if(filter)
    {
    server = filter->GetServer();
    if(server->HasSource(filter))
      {
      // The server is the parent when a filter has no input or more
      // than one input.
      return server;
      }
    else
      {
      // If the filter is not in the server source list, it should
      // have only one input.
      return filter->GetInput(0);
      }
    }
  else if(source)
    {
    // A source has no inputs. Its parent is the server.
    return source->GetServer();
    }

  return 0;
}

void pqPipelineModel::addItemAsSource(pqPipelineSource *source,
    pqPipelineServer *server)
{
  // Add the 'source' to the server.
  int row = server->GetSourceCount();
  int serverRow = this->getServerIndexFor(server);
  QModelIndex parentIndex = this->createIndex(serverRow, 0, server);
  this->beginInsertRows(parentIndex, row, row);
  server->AddSource(source);
  this->endInsertRows();

  if(row == 0)
    {
    emit this->firstChildAdded(parentIndex);
    }
}

void pqPipelineModel::removeLink(pqPipelineLink *link)
{
  // Remove the input from the filter's list. Make sure the proxy's
  // input is removed as well.
  pqPipelineFilter *filter = link->GetLink();
  filter->RemoveInput(link->GetSource());
  this->IgnorePipeline = true;
  pqPipelineData::instance()->removeConnection(filter->GetProxy(),
      link->GetSource()->GetProxy());
  this->IgnorePipeline = false;
  link->SetLink(0);

  // If the filter only has one input remaining, it needs to be
  // moved to the input's chain in the model.
  if(filter->GetInputCount() == 1)
    {
    pqPipelineServer *server = filter->GetServer();
    int row = server->GetSourceIndexFor(filter);
    QModelIndex parentIndex = this->createIndex(
        this->getServerIndexFor(server), 0, server);
    this->beginRemoveRows(parentIndex, row, row);
    server->RemoveFromSourceList(filter);
    this->endRemoveRows();

    // Replace the link in the source output with the filter.
    pqPipelineSource *source = filter->GetInput(0);
    row = source->GetOutputIndexFor(filter);
    link = dynamic_cast<pqPipelineLink *>(source->GetOutput(row));
    parentIndex = this->createIndex(this->getItemRow(source), 0, source);
    this->beginRemoveRows(parentIndex, row, row);
    source->RemoveOutput(link);
    this->endRemoveRows();
    delete link;

    this->beginInsertRows(parentIndex, row, row);
    source->InsertOutput(row, filter);
    this->endInsertRows();
    }
}


