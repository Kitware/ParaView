/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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

/// \file pqPipelineListModel.cxx
/// \brief
///   The pqPipelineListModel class is used to represent the pipeline
///   in the form of a list.
///
/// \date 11/14/2005

#include "pqPipelineListModel.h"

#include "pqPipelineData.h"
#include "pqPipelineObject.h"
#include "pqPipelineServer.h"
#include "pqPipelineWindow.h"
#include "pqServer.h"

#include "QVTKWidget.h"
#include "vtkSMProxy.h"

#include <QHash>
#include <QList>
#include <QPixmap>
#include <QString>


class pqPipelineListItem
{
public:
  pqPipelineListItem();
  ~pqPipelineListItem();

  bool IsFirst(pqPipelineListItem *child) const;
  bool IsLast(pqPipelineListItem *child) const;

public:
  pqPipelineListModel::ItemType Type;
  QString Name;
  pqPipelineListItem *Parent;
  QList<pqPipelineListItem *> Internal;
  union {
    pqPipelineServer *Server;
    pqPipelineWindow *Window;
    pqPipelineObject *Object;
    pqPipelineListItem *Link;
  } Data;
};


class pqPipelineListInternal
{
public:
  enum GroupedCommands {
    NoCommands = -1,
    CreateAndAppend = 0,
    CreateAndInsert,
    DeleteAndConnect
  };

public:
  pqPipelineListInternal();
  ~pqPipelineListInternal() {}

  void ResetCommand();

public:
  QHash<pqPipelineObject *, pqPipelineListItem *> Lookup;
  QHash<pqPipelineWindow *, pqPipelineListItem *> Windows;
  GroupedCommands CommandType;
  pqPipelineObject *Subject;
  pqPipelineObject *Source;
  pqPipelineObject *Sink;
  QList<pqPipelineObject *> SourceList;
  QList<pqPipelineObject *> SinkList;
};


pqPipelineListItem::pqPipelineListItem()
  : Name(), Internal()
{
  this->Type = pqPipelineListModel::Invalid;
  this->Parent = 0;
  this->Data.Object = 0;
}

pqPipelineListItem::~pqPipelineListItem()
{
  // Clean up the list of items.
  QList<pqPipelineListItem *>::Iterator iter = this->Internal.begin();
  for( ; iter != this->Internal.end(); ++iter)
    {
    if(*iter)
      {
      delete *iter;
      *iter = 0;
      }
    }

  this->Internal.clear();
}

bool pqPipelineListItem::IsFirst(pqPipelineListItem *child) const
{
  if(this->Internal.size() > 0)
    return this->Internal[0] == child;
  return false;
}

bool pqPipelineListItem::IsLast(pqPipelineListItem *child) const
{
  if(this->Internal.size() > 0)
    return this->Internal[this->Internal.size() - 1] == child;
  return false;
}


pqPipelineListInternal::pqPipelineListInternal()
  : Lookup(), Windows(), SourceList(), SinkList()
{
  this->CommandType = pqPipelineListInternal::NoCommands;
  this->Subject = 0;
  this->Source = 0;
  this->Sink = 0;
}

void pqPipelineListInternal::ResetCommand()
{
  this->CommandType = pqPipelineListInternal::NoCommands;
  this->Subject = 0;
  this->Source = 0;
  this->Sink = 0;
  this->SourceList.clear();
  this->SinkList.clear();
}


pqPipelineListModel::pqPipelineListModel(QObject *parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Internal = new pqPipelineListInternal();
  this->Root = new pqPipelineListItem();

  // Initialize the list pixmaps.
  Q_INIT_RESOURCE(pqWidgets);
  this->PixmapList = new QPixmap[pqPipelineListModel::Merge + 1];
  if(this->PixmapList)
    {
    this->PixmapList[pqPipelineListModel::Server].load(
        ":/pqWidgets/pqServer16.png");
    this->PixmapList[pqPipelineListModel::Window].load(
        ":/pqWidgets/pqWindow16.png");
    this->PixmapList[pqPipelineListModel::Source].load(
        ":/pqWidgets/pqSource16.png");
    this->PixmapList[pqPipelineListModel::Filter].load(
        ":/pqWidgets/pqFilter16.png");
    this->PixmapList[pqPipelineListModel::Bundle].load(
        ":/pqWidgets/pqBundle16.png");
    this->PixmapList[pqPipelineListModel::LinkBack].load(
        ":/pqWidgets/pqLinkBack16.png");
    this->PixmapList[pqPipelineListModel::LinkOut].load(
        ":/pqWidgets/pqLinkOut16.png");
    this->PixmapList[pqPipelineListModel::LinkIn].load(
        ":/pqWidgets/pqLinkIn16.png");
    this->PixmapList[pqPipelineListModel::Split].load(
        ":/pqWidgets/pqSplit16.png");
    this->PixmapList[pqPipelineListModel::Merge].load(
        ":/pqWidgets/pqMerge16.png");
    }

  // Connect to the pipeline data object.
  pqPipelineData *pipeline = pqPipelineData::instance();
  if(pipeline)
    {
    connect(pipeline, SIGNAL(clearingPipeline()),
        this, SLOT(clearPipeline()));
    connect(pipeline, SIGNAL(serverAdded(pqPipelineServer *)),
        this, SLOT(addServer(pqPipelineServer *)));
    connect(pipeline, SIGNAL(removingServer(pqPipelineServer *)),
        this, SLOT(removeServer(pqPipelineServer *)));
    connect(pipeline, SIGNAL(windowAdded(pqPipelineWindow *)),
        this, SLOT(addWindow(pqPipelineWindow *)));
    connect(pipeline, SIGNAL(removingWindow(pqPipelineWindow *)),
        this, SLOT(removeWindow(pqPipelineWindow *)));
    connect(pipeline, SIGNAL(sourceCreated(pqPipelineObject *)),
        this, SLOT(addSource(pqPipelineObject *)));
    connect(pipeline, SIGNAL(filterCreated(pqPipelineObject *)),
        this, SLOT(addFilter(pqPipelineObject *)));
    connect(pipeline, SIGNAL(removingObject(pqPipelineObject *)),
        this, SLOT(removeObject(pqPipelineObject *)));
    connect(pipeline,
        SIGNAL(connectionCreated(pqPipelineObject *, pqPipelineObject *)),
        this, SLOT(addConnection(pqPipelineObject *, pqPipelineObject *)));
    connect(pipeline,
        SIGNAL(removingConnection(pqPipelineObject *, pqPipelineObject *)),
        this, SLOT(removeConnection(pqPipelineObject *, pqPipelineObject *)));
    }
}

pqPipelineListModel::~pqPipelineListModel()
{
  if(this->Internal)
    delete this->Internal;
  if(this->Root)
    delete this->Root;
  if(this->PixmapList)
    delete [] this->PixmapList;
}

int pqPipelineListModel::rowCount(const QModelIndex &parentIndex) const
{
  pqPipelineListItem *item = this->Root;
  if(parentIndex.isValid())
    {
    if(parentIndex.model() == this)
      {
      item = reinterpret_cast<pqPipelineListItem *>(
          parentIndex.internalPointer());
      }
    else
      {
      item = 0;
      }
    }

  if(item)
    return item->Internal.size();

  return 0;
}

int pqPipelineListModel::columnCount(const QModelIndex &) const
{
  return 1;
}

bool pqPipelineListModel::hasChildren(const QModelIndex &parentIndex) const
{
  pqPipelineListItem *item = this->Root;
  if(parentIndex.isValid())
    {
    if(parentIndex.model() == this)
      {
      item = reinterpret_cast<pqPipelineListItem *>(
          parentIndex.internalPointer());
      }
    else
      {
      item = 0;
      }
    }

  if(item)
    {
    return item->Internal.size() > 0;
    }

  return false;
}

QModelIndex pqPipelineListModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  pqPipelineListItem *item = this->Root;
  if(parentIndex.isValid())
    {
    if(parentIndex.model() == this)
      {
      item = reinterpret_cast<pqPipelineListItem *>(
          parentIndex.internalPointer());
      }
    else
      {
      item = 0;
      }
    }

  if(item && column == 0 && row >= 0 && row < item->Internal.size())
    {
    return this->createIndex(row, column, item->Internal[row]);
    }

  return QModelIndex();
}

QModelIndex pqPipelineListModel::parent(const QModelIndex &idx) const
{
  if(this->Root && idx.isValid() && idx.model() == this)
    {
    pqPipelineListItem *item = reinterpret_cast<pqPipelineListItem *>(
        idx.internalPointer());
    if(item && item->Parent && item->Parent->Parent)
      {
      int row = item->Parent->Parent->Internal.indexOf(item->Parent);
      if(row >= 0)
        return this->createIndex(row, 0, item->Parent);
      }
    }

  return QModelIndex();
}

QVariant pqPipelineListModel::data(const QModelIndex &idx, int role) const
{
  if(this->Root && idx.isValid() && idx.model() == this)
    {
    pqPipelineListItem *item = reinterpret_cast<pqPipelineListItem *>(
        idx.internalPointer());
    if(item)
      {
      switch(role)
        {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::EditRole:
          {
          return QVariant(item->Name);
          }
        case Qt::DecorationRole:
          {
          if(this->PixmapList && item->Type != pqPipelineListModel::Invalid)
            return QVariant(this->PixmapList[item->Type]);
          }
        }
      }
    }

  return QVariant();
}

Qt::ItemFlags pqPipelineListModel::flags(const QModelIndex &) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

pqPipelineListModel::ItemType pqPipelineListModel::getTypeFor(
    const QModelIndex &idx) const
{
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    pqPipelineListItem *item = reinterpret_cast<pqPipelineListItem *>(
        idx.internalPointer());
    if(item)
      {
      return item->Type;
      }
    }

  return pqPipelineListModel::Invalid;
}

vtkSMProxy *pqPipelineListModel::getProxyFor(const QModelIndex &idx) const
{
  vtkSMProxy *proxy = 0;
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    pqPipelineListItem *item = reinterpret_cast<pqPipelineListItem *>(
        idx.internalPointer());
    if(item && (item->Type == pqPipelineListModel::Source || 
        item->Type == pqPipelineListModel::Filter ||
        item->Type == pqPipelineListModel::Bundle))
      {
      proxy = item->Data.Object->GetProxy();
      }
    }

  return proxy;
}

QWidget *pqPipelineListModel::getWidgetFor(const QModelIndex &idx) const
{
  QWidget *widget = 0;
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    pqPipelineListItem *item = reinterpret_cast<pqPipelineListItem *>(
        idx.internalPointer());
    if(item && item->Type == pqPipelineListModel::Window)
      widget = item->Data.Window->GetWidget();
    }

  return widget;
}

pqPipelineObject *pqPipelineListModel::getObjectFor(
    const QModelIndex &idx) const
{
  pqPipelineObject *object = 0;
  if(this->Internal && idx.isValid() && idx.model() == this)
    {
    pqPipelineListItem *item = reinterpret_cast<pqPipelineListItem *>(
        idx.internalPointer());
    if(item && (item->Type == pqPipelineListModel::Source || 
        item->Type == pqPipelineListModel::Filter ||
        item->Type == pqPipelineListModel::Bundle))
      {
      object = item->Data.Object;
      }
    }

  return object;
}

QModelIndex pqPipelineListModel::getIndexFor(vtkSMProxy *proxy) const
{
  pqPipelineData *pipeline = pqPipelineData::instance();
  if(this->Internal && pipeline && proxy)
    {
    pqPipelineObject *object = pipeline->getObjectFor(proxy);
    QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
        this->Internal->Lookup.find(object);
    if(iter != this->Internal->Lookup.end())
      {
      pqPipelineListItem *item = *iter;
      int row = item->Parent->Internal.indexOf(item);
      return this->createIndex(row, 0, item);
      }
    }

  return QModelIndex();
}

QModelIndex pqPipelineListModel::getIndexFor(QVTKWidget *window) const
{
  pqPipelineData *pipeline = pqPipelineData::instance();
  if(this->Internal && pipeline && window)
  {
    pqPipelineWindow *object = pipeline->getObjectFor(window);
    QHash<pqPipelineWindow *, pqPipelineListItem *>::Iterator iter =
        this->Internal->Windows.find(object);
    if(iter != this->Internal->Windows.end())
    {
      pqPipelineListItem *item = *iter;
      int row = item->Parent->Internal.indexOf(item);
      return this->createIndex(row, 0, item);
    }
  }

  return QModelIndex();
}

void pqPipelineListModel::clearPipeline()
{
  // Clear out all the internal information.
  if(this->Internal)
  {
    this->Internal->Lookup.clear();
    this->Internal->Windows.clear();
    this->Internal->ResetCommand();
  }

  // Delete all the old pipeline objects.
  if(this->Root)
    {
    delete this->Root;
    }

  // Create a new root item.
  this->Root = new pqPipelineListItem();

  // Notify the view that everything has changed.
  this->reset();
}

void pqPipelineListModel::addServer(pqPipelineServer *server)
{
  if(!this->Root || !server || !server->GetServer())
    return;

  pqPipelineListItem *item = new pqPipelineListItem();
  if(item)
    {
    item->Type = pqPipelineListModel::Server;
    item->Name = server->GetServer()->getFriendlyName();
    item->Data.Server = server;
    item->Parent = this->Root;

    // Add the server to the root item.
    int rows = this->Root->Internal.size();
    this->beginInsertRows(QModelIndex(), rows, rows);
    this->Root->Internal.append(item);
    this->endInsertRows();
    }
}

void pqPipelineListModel::removeServer(pqPipelineServer *server)
{
  if(!this->Root || !server)
    return;

  // Find the server in the root object.
  QList<pqPipelineListItem *>::Iterator iter = this->Root->Internal.begin();
  for(int i = 0; iter != this->Root->Internal.end(); ++iter, ++i)
    {
    if(*iter && (*iter)->Data.Server == server)
      {
      pqPipelineListItem *item = *iter;
      this->beginRemoveRows(QModelIndex(), i, i);
      this->Root->Internal.erase(iter);
      this->endRemoveRows();
      delete item;
      break;
      }
    }
}

void pqPipelineListModel::addWindow(pqPipelineWindow *window)
{
  if(!this->Internal || !this->Root || !window || !window->GetServer())
    return;

  // Make sure the window item doesn't exist.
  if(this->Internal->Windows.find(window) != this->Internal->Windows.end())
    return;

  // Look up the server object in the root.
  int serverRow = 0;
  pqPipelineListItem *serverItem = 0;
  QList<pqPipelineListItem *>::Iterator iter = this->Root->Internal.begin();
  for( ; iter != this->Root->Internal.end(); ++iter, ++serverRow)
    {
    if(*iter && (*iter)->Data.Server == window->GetServer())
      {
      serverItem = *iter;
      break;
      }
    }

  if(!serverItem)
    return;

  // Add the window to the end of the list.
  pqPipelineListItem *item = new pqPipelineListItem();
  if(item)
    {
    item->Type = pqPipelineListModel::Window;
    item->Name = window->GetWidget()->windowTitle();
    item->Data.Window = window;
    item->Parent = serverItem;
    this->Internal->Windows.insert(window, item);

    // Add the window to the server item.
    int rows = serverItem->Internal.size();
    QModelIndex parentIndex = this->createIndex(serverRow, 0, serverItem);
    this->beginInsertRows(parentIndex, rows, rows);
    serverItem->Internal.append(item);
    this->endInsertRows();

    if(serverItem->Internal.size() == 1)
      {
      emit this->childAdded(parentIndex);
      }
    }
}

void pqPipelineListModel::removeWindow(pqPipelineWindow *window)
{
  if(!this->Internal || !this->Root || !window)
    return;

  // Look up the window in the hash table.
  QHash<pqPipelineWindow *, pqPipelineListItem *>::Iterator iter =
      this->Internal->Windows.find(window);
  if(iter == this->Internal->Windows.end())
    return;

  // Remove the item from the server and delete it. Remove it from
  // the hash table along with all of its children.
  pqPipelineListItem *item = *iter;
  pqPipelineListItem *serverItem = item->Parent;

  int serverRow = this->Root->Internal.indexOf(serverItem);
  QModelIndex parentIndex = this->createIndex(serverRow, 0, serverItem);
  int row = serverItem->Internal.indexOf(item);
  this->beginRemoveRows(parentIndex, row, row);
  serverItem->Internal.removeAll(item);
  this->endRemoveRows();
  this->Internal->Windows.erase(iter);
  this->removeLookupItems(item);
  delete item;
}

void pqPipelineListModel::addSource(pqPipelineObject *source)
{
  this->addSubItem(source, pqPipelineListModel::Source);
}

void pqPipelineListModel::removeSource(pqPipelineObject *source)
{
  if(!this->Internal || !source)
    {
    return;
    }

  if(this->Internal->CommandType == pqPipelineListInternal::DeleteAndConnect)
    {
    if(source == this->Internal->Subject)
      {
      return;
      }
    }

  // Get the source item from the lookup table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->Lookup.find(source);
  if(iter == this->Internal->Lookup.end())
    return;

  // TODO: If the source is connected to multiple items, the
  // subsequent chains will be orphaned forming multiple pipelines.
  // Each splitter needs to be converted to a merge item.

  // Remove the item from the server and delete it. Remove it from
  // the hash table along with all of its children.
  pqPipelineListItem *item = *iter;
  QModelIndex parentIndex = this->createIndex(
      item->Parent->Parent->Internal.indexOf(item->Parent), 0, item->Parent);
  int row = item->Parent->Internal.indexOf(item);
  this->beginRemoveRows(parentIndex, row, row);
  item->Parent->Internal.removeAll(item);
  this->Internal->Lookup.erase(iter);
  this->removeLookupItems(item);
  this->endRemoveRows();
  delete item;
}

void pqPipelineListModel::addFilter(pqPipelineObject *filter)
{
  if(!this->Internal || !filter)
    return;

  if(this->Internal->CommandType == pqPipelineListInternal::CreateAndAppend ||
      this->Internal->CommandType == pqPipelineListInternal::CreateAndInsert)
  {
    if(filter == this->Internal->Subject)
      return;

    if(this->Internal->Subject == 0)
      this->Internal->Subject = filter;
    else
      this->addSubItem(filter, pqPipelineListModel::Filter);
  }
  else
    this->addSubItem(filter, pqPipelineListModel::Filter);
}

void pqPipelineListModel::removeFilter(pqPipelineObject *filter)
{
  if(!this->Internal || !filter)
    {
    return;
    }

  if(this->Internal->CommandType == pqPipelineListInternal::DeleteAndConnect)
    {
    if(filter == this->Internal->Subject)
      {
      return;
      }
    }

  // Get the filter item from the lookup table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->Lookup.find(filter);
  if(iter == this->Internal->Lookup.end())
    return;

  // TODO: Check the surrounding pipeline. Removing the filter may
  // require the pipeline list to be re-arranged.

  // Remove the item from the server and delete it. Remove it from
  // the hash table along with all of its children.
  pqPipelineListItem *item = *iter;
  QModelIndex parentIndex = this->createIndex(
      item->Parent->Parent->Internal.indexOf(item->Parent), 0, item->Parent);
  int row = item->Parent->Internal.indexOf(item);
  this->beginRemoveRows(parentIndex, row, row);
  item->Parent->Internal.removeAll(item);
  this->Internal->Lookup.erase(iter);
  this->removeLookupItems(item);
  this->endRemoveRows();
  delete item;
}

void pqPipelineListModel::removeObject(pqPipelineObject *object)
{
  if(!this->Internal || !object)
    {
    return;
    }

  if(this->Internal->CommandType == pqPipelineListInternal::DeleteAndConnect)
    {
    if(object == this->Internal->Subject)
      {
      return;
      }
    }

  // Get the object item from the lookup table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->Lookup.find(object);
  if(iter == this->Internal->Lookup.end())
    return;

  // TODO: If the source is connected to multiple items, the
  // subsequent chains will be orphaned forming multiple pipelines.
  // Each splitter needs to be converted to a merge item.

  // TODO: Check the surrounding pipeline. Removing the filter may
  // require the pipeline list to be re-arranged.

  // Remove the item from the server and delete it. Remove it from
  // the hash table along with all of its children.
  pqPipelineListItem *item = *iter;
  QModelIndex parentIndex = this->createIndex(
      item->Parent->Parent->Internal.indexOf(item->Parent), 0, item->Parent);
  int row = item->Parent->Internal.indexOf(item);
  this->beginRemoveRows(parentIndex, row, row);
  item->Parent->Internal.removeAll(item);
  this->Internal->Lookup.erase(iter);
  this->removeLookupItems(item);
  this->endRemoveRows();
  delete item;
}

void pqPipelineListModel::addConnection(pqPipelineObject *source,
    pqPipelineObject *sink)
{
  if(!this->Internal || !source || !sink || source == sink)
    return;

  // If running a grouped command, this new connection may be
  // handled later.
  if(this->Internal->CommandType == pqPipelineListInternal::CreateAndAppend)
    {
    if(this->Internal->Subject == sink)
      {
      this->Internal->Source = source;
      return;
      }
    }
  else if(this->Internal->CommandType ==
      pqPipelineListInternal::CreateAndInsert)
    {
    if(this->Internal->Subject == source)
      {
      this->Internal->Sink = sink;
      return;
      }
    else if(this->Internal->Subject == sink)
      {
      this->Internal->Source = source;
      return;
      }
    }
  else if(this->Internal->CommandType ==
      pqPipelineListInternal::DeleteAndConnect)
    {
    if(this->Internal->SinkList.indexOf(sink) != -1)
      {
      this->Internal->SourceList.append(source);
      return;
      }
    }

  // Get the source and sink items from the lookup table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->Lookup.find(source);
  if(iter == this->Internal->Lookup.end())
    return;

  pqPipelineListItem *sourceItem = *iter;
  iter = this->Internal->Lookup.find(sink);
  if(iter == this->Internal->Lookup.end())
    return;

  pqPipelineListItem *sinkItem = *iter;
  int i = 0;
  int row = 0;
  int sourceRow = 0;
  QModelIndex parentIndex;
  pqPipelineListItem *parentItem = 0;
  pqPipelineListItem *split1 = 0;
  pqPipelineListItem *split2 = 0;
  pqPipelineListItem *link = 0;
  if(source->GetParent() == sink->GetParent())
    {
    // Both items are in the same window. Check to see if they have
    // the same parent.
    if(sourceItem->Parent == sinkItem->Parent)
      {
      // TODO
      }
    else
      {
      // Check to see if the two items are connected. With different
      // parents in the same window, the only possibility is having
      // the source on a merge chain connected to the sink by a
      // link back item.
      parentItem = sourceItem->Parent;
      if(!sourceItem->Parent->IsLast(sourceItem))
        {
        sourceRow = parentItem->Internal.indexOf(sourceItem);
        link = parentItem->Internal[sourceRow + 1];
        if(link->Type == pqPipelineListModel::LinkBack &&
            link->Data.Link == sinkItem)
          {
          return;
          }

        // Since the source is not last in the list, a splitter needs
        // to be created. If the rest of the chain is composed of
        // splitters, only one needs to be made.
        bool needSecond = false;
        for(i = sourceRow + 1; i < parentItem->Internal.size(); i++)
          {
          if(parentItem->Internal[i]->Type != pqPipelineListModel::Split)
            {
            needSecond = true;
            break;
            }
          }

        split1 = new pqPipelineListItem();
        if(!split1)
          {
          return;
          }

        parentIndex = this->createIndex(
            parentItem->Parent->Internal.indexOf(parentItem), 0, parentItem);
        if(needSecond)
          {
          split2 = new pqPipelineListItem();
          if(!split2)
            {
            delete split1;
            return;
            }

          // Move all the subsequent items to the new splitter.
          split2->Type = pqPipelineListModel::Split;
          split2->Parent = parentItem;
          i = parentItem->Internal.size() - 1;
          this->beginRemoveRows(parentIndex, sourceRow + 1, i);
          for( ; i >= sourceRow + 1; i--)
            {
            pqPipelineListItem *item = parentItem->Internal.takeAt(i);
            item->Parent = split2;
            split2->Internal.prepend(item);
            }

          this->endRemoveRows();
          row = parentItem->Internal.size();
          this->beginInsertRows(parentIndex, row, row);
          parentItem->Internal.append(split2);
          this->endInsertRows();

          // TODO: Re-open all the sub-items.
          emit this->childAdded(this->createIndex(row, 0, split2));
          }

        split1->Type = pqPipelineListModel::Split;
        split1->Parent = parentItem;
        row = parentItem->Internal.size();
        this->beginInsertRows(parentIndex, row, row);
        parentItem->Internal.append(split1);
        this->endInsertRows();

        // Set the parent pointer for the new connection item(s).
        parentItem = split1;
        }

      // If the sink is at the beginning of its parent's list, the whole
      // list can be moved to the source's chain. Otherwise, a link back
      // item needs to be placed after the source.
      bool canMove = false;
      if(sinkItem->Parent->Type == pqPipelineListModel::Merge &&
          sinkItem->Parent->IsFirst(sinkItem))
        {
        // If the sink outputs to the source, the sink chain can't be moved.
        canMove = true;
        pqPipelineListItem *sourceParent = sourceItem->Parent;
        while(sourceParent)
          {
          if(sourceParent == sinkItem->Parent)
            {
            canMove = false;
            break;
            }

          sourceParent = sourceParent->Parent;
          }
        }

      if(canMove)
        {
        // If the source and sink have the same grandparent and the source
        // is on a merge chain, it may be possible to simplify farther. If
        // there are only two merge chains in the grandparent, the source
        // chain can be moved to the grandparent along with the sink.
        bool canSimplify =
            sourceItem->Parent->Type == pqPipelineListModel::Merge &&
            sourceItem->Parent->Parent == sinkItem->Parent->Parent &&
            sourceItem->Parent->Parent->Internal.size() == 2;

        // Remove the sink's parent from the model.
        pqPipelineListItem *sinkParent = sinkItem->Parent;
        parentIndex = this->createIndex(
            sinkParent->Parent->Parent->Internal.indexOf(sinkParent->Parent),
            0, sinkParent->Parent);
        row = sinkParent->Parent->Internal.indexOf(sinkParent);
        this->beginRemoveRows(parentIndex, row, row);
        sinkParent->Parent->Internal.removeAt(row);
        this->endRemoveRows();

        QList<pqPipelineListItem *>::Iterator jter;
        if(canSimplify)
          {
          // Remove the source's parent from the model.
          pqPipelineListItem *sourceParent = sourceItem->Parent;
          parentIndex = this->createIndex(
              sourceParent->Parent->Parent->Internal.indexOf(
              sourceParent->Parent), 0, sourceParent->Parent);
          row = sourceParent->Parent->Internal.indexOf(sourceParent);
          this->beginRemoveRows(parentIndex, row, row);
          sourceParent->Parent->Internal.removeAt(row);
          this->endRemoveRows();

          // Move the source chain to the grandparent.
          i = sourceParent->Parent->Internal.size();
          row = i + sourceParent->Internal.size() - 1;
          this->beginInsertRows(parentIndex, i, row);
          jter = sourceParent->Internal.begin();
          for( ; jter != sourceParent->Internal.end(); ++jter)
            {
            sourceParent->Parent->Internal.append(*jter);
            (*jter)->Parent = sourceParent->Parent;
            }

          this->endInsertRows();
          sourceParent->Internal.clear();
          delete sourceParent;

          // If the sink chain is not being added to a splitter, set up
          // the new parent.
          if(!split1)
            {
            parentItem = sourceItem->Parent;
            }

          // TODO: Re-open source chain items.
          }

        // Move the sink chain to the new location.
        parentIndex = this->createIndex(
            parentItem->Parent->Internal.indexOf(parentItem), 0, parentItem);
        i = parentItem->Internal.size();
        row = i + sinkParent->Internal.size() - 1;
        this->beginInsertRows(parentIndex, i, row);
        jter = sinkParent->Internal.begin();
        for( ; jter != sinkParent->Internal.end(); ++jter)
          {
          parentItem->Internal.append(*jter);
          (*jter)->Parent = parentItem;
          }

        this->endInsertRows();
        sinkParent->Internal.clear();
        delete sinkParent;

        // If the items are added to a new splitter, make sure the split
        // item is open.
        if(split1)
          {
          emit this->childAdded(parentIndex);
          }

        // TODO: Re-open sink chain items.
        }
      else
        {
        // Add a link back item for the new connection.
        link = new pqPipelineListItem();
        if(!link)
          {
          return;
          }

        link->Type = pqPipelineListModel::LinkBack;
        link->Parent = parentItem;
        link->Data.Link = sinkItem;
        row = parentItem->Internal.size();
        parentIndex = this->createIndex(
          parentItem->Parent->Internal.indexOf(parentItem), 0, parentItem);
        this->beginInsertRows(parentIndex, row, row);
        parentItem->Internal.append(link);
        this->endInsertRows();

        // If the link item is added to a new splitter, make sure the
        // split item is open.
        if(split1)
          {
          emit this->childAdded(parentIndex);
          }

        // TODO: Adding a link back item may create common pipeline
        // segments.
        }
      }
    }
  else
    {
    // TODO
    // Check to see if the source and sink are connected already.

    // Since the source and sink are in different windows, a link
    // object needs to be made.
    }
}

void pqPipelineListModel::removeConnection(pqPipelineObject *source,
    pqPipelineObject *sink)
{
  if(!this->Internal || !source || !sink)
    {
    return;
    }

  if(this->Internal->CommandType == pqPipelineListInternal::DeleteAndConnect)
    {
    if(this->Internal->Subject == 0 || this->Internal->Subject == source)
      {
      this->Internal->Subject = source;
      this->Internal->SinkList.append(sink);
      return;
      }
    }

  // TODO
}

void pqPipelineListModel::beginCreateAndAppend()
{
  if(this->Internal)
    {
    this->Internal->ResetCommand();
    this->Internal->CommandType = pqPipelineListInternal::CreateAndAppend;
    }
}

void pqPipelineListModel::finishCreateAndAppend()
{
  if(!this->Internal || this->Internal->CommandType !=
      pqPipelineListInternal::CreateAndAppend)
    {
    return;
    }

  pqPipelineObject *filter = this->Internal->Subject;
  pqPipelineObject *source = this->Internal->Source;
  this->Internal->ResetCommand();
  if(!filter || !source)
    return;

  // Get the source item from the lookup table. It must exist to
  // process the command.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->Lookup.find(source);
  if(iter == this->Internal->Lookup.end())
    return;
  pqPipelineListItem *sourceItem = *iter;

  // The filter item should not exist.
  iter = this->Internal->Lookup.find(filter);
  if(iter != this->Internal->Lookup.end())
    return;

  pqPipelineListItem *newItem = new pqPipelineListItem();
  if(!newItem)
    return;

  if(filter->GetType() == pqPipelineObject::Bundle)
    newItem->Type = pqPipelineListModel::Bundle;
  else
    newItem->Type = pqPipelineListModel::Filter;
  newItem->Data.Object = filter;
  newItem->Name = filter->GetProxyName();

  // If the source is not at the end of the list, the new filter needs
  // to be added to a split item. The items after the source may also
  // need to be moved to a new split item in that case.
  QModelIndex parentIndex;
  pqPipelineListItem *parentItem = sourceItem->Parent;
  if(parentItem->IsLast(sourceItem))
    {
    // Add the filter to the parent after the source.
    this->Internal->Lookup.insert(filter, newItem);
    newItem->Parent = parentItem;
    parentIndex = this->createIndex(
        parentItem->Parent->Internal.indexOf(parentItem), 0, parentItem);
    int row = parentItem->Internal.size();
    this->beginInsertRows(parentIndex, row, row);
    parentItem->Internal.append(newItem);
    this->endInsertRows();
    }
  else
    {
    delete newItem; // TEMP
    }
}

void pqPipelineListModel::beginCreateAndInsert()
{
  if(this->Internal)
    {
    this->Internal->ResetCommand();
    this->Internal->CommandType = pqPipelineListInternal::CreateAndInsert;
    }
}

void pqPipelineListModel::finishCreateAndInsert()
{
  if(!this->Internal || this->Internal->CommandType !=
      pqPipelineListInternal::CreateAndInsert)
    {
    return;
    }

  pqPipelineObject *filter = this->Internal->Subject;
  pqPipelineObject *source = this->Internal->Source;
  pqPipelineObject *sink = this->Internal->Sink;
  this->Internal->ResetCommand();
  if(!filter || !source || !sink)
    return;

  // Get the source and sink items from the lookup table. They both
  // must exist to process the command.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->Lookup.find(source);
  if(iter == this->Internal->Lookup.end())
    return;

  pqPipelineListItem *sourceItem = *iter;
  iter = this->Internal->Lookup.find(sink);
  if(iter == this->Internal->Lookup.end())
    return;
  pqPipelineListItem *sinkItem = *iter;

  // The filter item should not exist.
  iter = this->Internal->Lookup.find(filter);
  if(iter != this->Internal->Lookup.end())
    return;

  pqPipelineListItem *newItem = new pqPipelineListItem();
  if(!newItem)
    return;

  if(filter->GetType() == pqPipelineObject::Bundle)
    newItem->Type = pqPipelineListModel::Bundle;
  else
    newItem->Type = pqPipelineListModel::Filter;
  newItem->Data.Object = filter;
  newItem->Name = filter->GetProxyName();

  // Make sure the sink is connected to the source. If the source is
  // connected to multiple objects, the filter should be inserted in
  // the sink chain.
  pqPipelineListItem *parentItem = sourceItem->Parent;
  int i = parentItem->Internal.indexOf(sourceItem) + 1;
  if(i == parentItem->Internal.size())
    {
    delete newItem;
    return;
    }

  QModelIndex parentIndex;
  pqPipelineListItem *next = parentItem->Internal[i];
  if(next == sinkItem || (next->Type == pqPipelineListModel::LinkBack &&
      next->Data.Link == sinkItem))
    {
    // Insert the filter after the source item.
    this->Internal->Lookup.insert(filter, newItem);
    parentIndex = this->createIndex(
        parentItem->Parent->Internal.indexOf(parentItem), 0, parentItem);
    this->beginInsertRows(parentIndex, i, i);
    newItem->Parent = parentItem;
    parentItem->Internal.insert(i, newItem);
    this->endInsertRows();
    }
  else
    {
    for( ; i < parentItem->Internal.size(); i++)
      {
      next = parentItem->Internal[i];
      if(next->Type != pqPipelineListModel::Split)
        break;
      if(next->Internal.size() > 0 && next->Internal[0] == sinkItem)
        {
        // Insert the item before the sink in the chain.
        this->Internal->Lookup.insert(filter, newItem);
        parentIndex = this->createIndex(i, 0, next);
        this->beginInsertRows(parentIndex, 0, 0);
        newItem->Parent = next;
        next->Internal.insert(0, newItem);
        this->endInsertRows();
        return;
        }
      }

    delete newItem;
    }
}

void pqPipelineListModel::beginDeleteAndConnect()
{
  if(this->Internal)
    {
    this->Internal->ResetCommand();
    this->Internal->CommandType = pqPipelineListInternal::DeleteAndConnect;
    }
}

void pqPipelineListModel::finishDeleteAndConnect()
{
  if(!this->Internal || this->Internal->CommandType !=
      pqPipelineListInternal::DeleteAndConnect)
    {
    return;
    }

  pqPipelineObject *object = this->Internal->Subject;
  QList<pqPipelineObject *> sourceList = this->Internal->SourceList;
  QList<pqPipelineObject *> sinkList = this->Internal->SinkList;
  this->Internal->ResetCommand();
  if(!object || sinkList.size() < 1)
    {
    return;
    }

  // Get the list item from the lookup table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->Lookup.find(object);
  if(iter == this->Internal->Lookup.end())
    {
    return;
    }

  // Since the connections were re-established around the removed
  // item, it can simply be removed.
  pqPipelineListItem *item = *iter;
  QModelIndex parentIndex = this->createIndex(
      item->Parent->Parent->Internal.indexOf(item->Parent), 0, item->Parent);
  int row = item->Parent->Internal.indexOf(item);
  this->beginRemoveRows(parentIndex, row, row);
  item->Parent->Internal.removeAll(item);
  this->Internal->Lookup.erase(iter);
  this->removeLookupItems(item);
  this->endRemoveRows();
  delete item;

  // If the source list has more than one item, the link back items
  // in the tree need to be updated.
  if(sourceList.size() > 1)
    {
    iter = this->Internal->Lookup.find(sinkList[0]);
    if(iter == this->Internal->Lookup.end())
      {
      return;
      }

    pqPipelineListItem *sourceItem = 0;
    pqPipelineListItem *sinkItem = *iter;
    QList<pqPipelineObject *>::Iterator jter = sourceList.begin();
    for( ; jter != sourceList.end(); ++jter)
      {
      iter = this->Internal->Lookup.find(*jter);
      if(iter == this->Internal->Lookup.end())
        {
        continue;
        }

      // Find the link item in the source's parent. Change the link
      // to point to the new sink item.
      sourceItem = *iter;
      row = sourceItem->Parent->Internal.indexOf(sourceItem) + 1;
      if(row < sourceItem->Parent->Internal.size())
        {
        item = sourceItem->Parent->Internal[row];
        if(item->Type == pqPipelineListModel::LinkBack)
          {
          item->Data.Link = sinkItem;
          }
        }
      }
    }
}

void pqPipelineListModel::addSubItem(pqPipelineObject *object,
    ItemType itemType)
{
  if(!this->Internal || !object || !object->GetParent())
    return;

  // Make sure the item doesn't exist.
  if(this->Internal->Lookup.find(object) != this->Internal->Lookup.end())
    return;

  // Get the window item from the lookup table.
  QHash<pqPipelineWindow *, pqPipelineListItem *>::Iterator iter =
      this->Internal->Windows.find(object->GetParent());
  if(iter == this->Internal->Windows.end())
    return;

  // If there are items in the window's list, the new item needs
  // a merge item to contain it.
  pqPipelineListItem *parentItem = *iter;
  if(parentItem->Internal.size() > 0)
    {
    int mergeRow = 0;
    int windowRow = parentItem->Parent->Internal.indexOf(parentItem);
    QModelIndex windowIndex = this->createIndex(windowRow, 0, parentItem);

    // If not all the window items are merge items, the current
    // children of the window need to be added to a new merge
    // item.
    bool needsMerge = false;
    QList<pqPipelineListItem *>::Iterator jter = parentItem->Internal.begin();
    for( ; jter != parentItem->Internal.end(); ++jter)
      {
      if(!(*jter) || (*jter)->Type != pqPipelineListModel::Merge)
        {
        needsMerge = true;
        break;
        }
      }

    if(needsMerge)
      {
      pqPipelineListItem *merge = new pqPipelineListItem();
      if(!merge)
        return;

      merge->Type = pqPipelineListModel::Merge;
      merge->Parent = parentItem;

      // Remove the window's current items.
      this->beginRemoveRows(windowIndex, 0, parentItem->Internal.size() - 1);
      this->endRemoveRows();

      // Add the items to the new merge item.
      merge->Internal = parentItem->Internal;
      parentItem->Internal.clear();
      jter = merge->Internal.begin();
      for( ; jter != merge->Internal.end(); ++jter)
        {
        if(*jter)
          (*jter)->Parent = merge;
        }

      // Add the merge item to the window.
      this->beginInsertRows(windowIndex, 0, 0);
      parentItem->Internal.append(merge);
      this->endInsertRows();

      emit this->childAdded(this->createIndex(0, 0, merge));
      // TODO: Sub-items of the new merge may need to be expanded.
      }

    // Add a merge item for the new source, filter, or bundle.
    pqPipelineListItem *windowItem = parentItem;
    parentItem = new pqPipelineListItem();
    if(parentItem)
      {
      parentItem->Type = pqPipelineListModel::Merge;
      parentItem->Parent = windowItem;

      // Add the merge item to the window.
      mergeRow = windowItem->Internal.size();
      this->beginInsertRows(windowIndex, mergeRow, mergeRow);
      windowItem->Internal.append(parentItem);
      this->endInsertRows();
      }
    }

  if(!parentItem)
    return;

  pqPipelineListItem *item = new pqPipelineListItem();
  if(item)
    {
    item->Type = itemType;
    item->Name = object->GetProxyName();
    item->Data.Object = object;
    item->Parent = parentItem;
    this->Internal->Lookup.insert(object, item);

    // Add the source, filter, or bundle to the parent item.
    int parentRow = parentItem->Parent->Internal.indexOf(parentItem);
    int rows = parentItem->Internal.size();
    QModelIndex parentIndex = this->createIndex(parentRow, 0, parentItem);
    this->beginInsertRows(parentIndex, rows, rows);
    parentItem->Internal.append(item);
    this->endInsertRows();

    if(parentItem->Internal.size() == 1)
      emit this->childAdded(parentIndex);
    }
}

void pqPipelineListModel::removeLookupItems(pqPipelineListItem *item)
{
  if(!this->Internal || !item)
    return;

  QList<pqPipelineListItem *>::Iterator iter = item->Internal.begin();
  for( ; iter != item->Internal.end(); ++iter)
    {
    if(*iter)
      {
      this->removeLookupItems(*iter);
      this->Internal->Lookup.remove((*iter)->Data.Object);
      }
    }
}


