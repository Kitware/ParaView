
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
    CreateAndInsert
  };

public:
  pqPipelineListInternal();
  ~pqPipelineListInternal() {}

public:
  QHash<pqPipelineObject *, pqPipelineListItem *> Lookup;
  QHash<pqPipelineWindow *, pqPipelineListItem *> Windows;
  GroupedCommands CommandType;
  pqPipelineObject *MoveOrCreate;
  pqPipelineObject *Source;
  pqPipelineObject *Sink;
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
  : Lookup(), Windows()
{
  this->CommandType = pqPipelineListInternal::NoCommands;
  this->MoveOrCreate = 0;
  this->Source = 0;
  this->Sink = 0;
}


pqPipelineListModel::pqPipelineListModel(QObject *p)
  : QAbstractItemModel(p)
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
    //this->PixmapList[pqPipelineListModel::Bundle].load(
    //    ":/pqWidgets/pqBundle16.png");
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

int pqPipelineListModel::rowCount(const QModelIndex &p) const
{
  pqPipelineListItem *item = this->Root;
  if(p.isValid())
    {
    if(p.model() == this)
      item = reinterpret_cast<pqPipelineListItem *>(p.internalPointer());
    else
      item = 0;
    }

  if(item)
    return item->Internal.size();

  return 0;
}

int pqPipelineListModel::columnCount(const QModelIndex &/*p*/) const
{
  return 1;
}

bool pqPipelineListModel::hasChildren(const QModelIndex &p) const
{
  pqPipelineListItem *item = this->Root;
  if(p.isValid())
    {
    if(p.model() == this)
      item = reinterpret_cast<pqPipelineListItem *>(p.internalPointer());
    else
      item = 0;
    }

  if(item)
    return item->Internal.size() > 0;

  return false;
}

QModelIndex pqPipelineListModel::index(int row, int column,
    const QModelIndex &p) const
{
  pqPipelineListItem *item = this->Root;
  if(p.isValid())
    {
    if(p.model() == this)
      item = reinterpret_cast<pqPipelineListItem *>(p.internalPointer());
    else
      item = 0;
    }

  if(item && column == 0 && row >= 0 && row < item->Internal.size())
    return this->createIndex(row, column, item->Internal[row]);

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

Qt::ItemFlags pqPipelineListModel::flags(const QModelIndex &/*idx*/) const
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
      return item->Type;
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
        item->Type == pqPipelineListModel::Filter))
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
    QModelIndex p = this->createIndex(serverRow, 0, serverItem);
    this->beginInsertRows(p, rows, rows);
    serverItem->Internal.append(item);
    this->endInsertRows();

    if(serverItem->Internal.size() == 1)
      emit this->childAdded(p);
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
  QModelIndex p = this->createIndex(serverRow, 0, serverItem);
  int row = serverItem->Internal.indexOf(item);
  this->beginRemoveRows(p, row, row);
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
  if(!this->Internal || !source || !source->GetParent())
    return;

  // Get the source item from the lookup table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->Lookup.find(source);
  if(iter == this->Internal->Lookup.end())
    return;

  // Remove the item from the server and delete it. Remove it from
  // the hash table along with all of its children.
  pqPipelineListItem *item = *iter;
  pqPipelineListItem *windowItem = item->Parent;

  //int windowRow = windowItem->GetParent()->Internal.indexOf(windowItem);
  //QModelIndex parent = this->createIndex(windowRow, 0, windowItem);
  //int row = windowItem->Internal.indexOf(item);
  //this->beginRemoveRows(parent, row, row);
  windowItem->Internal.removeAll(item);
  this->Internal->Lookup.erase(iter);
  this->removeLookupItems(item);
  delete item;
  //this->endRemoveRows();
  this->reset(); //TEMP
}

void pqPipelineListModel::addFilter(pqPipelineObject *filter)
{
  if(!this->Internal || !filter)
    return;

  if(this->Internal->CommandType == pqPipelineListInternal::CreateAndAppend ||
      this->Internal->CommandType == pqPipelineListInternal::CreateAndInsert)
  {
    if(filter == this->Internal->MoveOrCreate)
      return;

    if(this->Internal->MoveOrCreate == 0)
      this->Internal->MoveOrCreate = filter;
    else
      this->addSubItem(filter, pqPipelineListModel::Filter);
  }
  else
    this->addSubItem(filter, pqPipelineListModel::Filter);
}

void pqPipelineListModel::removeFilter(pqPipelineObject *filter)
{
  if(!this->Internal || !filter || !filter->GetParent())
    return;

  // Get the filter item from the lookup table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->Lookup.find(filter);
  if(iter == this->Internal->Lookup.end())
    return;

  // Remove the item from the server and delete it. Remove it from
  // the hash table along with all of its children.
  pqPipelineListItem *item = *iter;
  pqPipelineListItem *windowItem = item->Parent;

  //int windowRow = windowItem->GetParent()->Internal.indexOf(windowItem);
  //QModelIndex parent = this->createIndex(windowRow, 0, windowItem);
  //int row = windowItem->Internal.indexOf(item);
  //this->beginRemoveRows(parent, row, row);
  windowItem->Internal.removeAll(item);
  this->Internal->Lookup.erase(iter);
  this->removeLookupItems(item);
  delete item;
  //this->endRemoveRows();
  this->reset(); //TEMP
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
    if(this->Internal->MoveOrCreate == sink)
      {
      this->Internal->Source = source;
      return;
      }
    }
  else if(this->Internal->CommandType ==
      pqPipelineListInternal::CreateAndInsert)
    {
    if(this->Internal->MoveOrCreate == source)
      {
      this->Internal->Sink = sink;
      return;
      }
    else if(this->Internal->MoveOrCreate == sink)
      {
      this->Internal->Source = source;
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
  int sourceRow = 0;
  QModelIndex parentIndex;
  pqPipelineListItem *p = 0;
  pqPipelineListItem *split1 = 0;
  pqPipelineListItem *split2 = 0;
  if(source->GetParent() == sink->GetParent())
    {
    // Check to see if the source and sink are connected already.
    if(!sourceItem->Parent->IsLast(sourceItem))
      {
      sourceRow = sourceItem->Parent->Internal.indexOf(sourceItem);
      pqPipelineListItem *next = sourceItem->Parent->Internal[++sourceRow];
      if(next == sinkItem)
        return;
      if(next->Type == pqPipelineListModel::LinkBack &&
          next->Data.Link == sinkItem)
        {
        return;
        }

      // The source may have several outputs.
      while(next->Type == pqPipelineListModel::Split)
        {
        if(next->Internal.size() > 0 && (next->Internal[0] == sinkItem ||
            (next->Internal[0]->Type == pqPipelineListModel::LinkBack &&
            next->Internal[0]->Data.Link == sinkItem)))
          {
          return;
          }

        if(sourceItem->Parent->IsLast(next))
          break;
        next = sourceItem->Parent->Internal[++sourceRow];
        }
      }

    // Step through the different same window connection cases.
    if(sourceItem->Parent == sinkItem->Parent)
      {
      // If both items are in the same chain, creating a new
      // connection will result in a pipeline branch and merge.
      // If the sink is higher in the chain, this connection will
      // form a loop.
      pqPipelineListItem *link = new pqPipelineListItem();
      if(!link)
        return;

      // Set up the link for the new connection.
      link->Type = pqPipelineListModel::LinkBack;
      link->Data.Link = sinkItem;

      // Get the parent index ready.
      p = sourceItem->Parent;
      parentIndex = this->createIndex(
          p->Parent->Internal.indexOf(p), 0, p);
      if(p->IsLast(sourceItem))
        {
        // If the source is at the end, this is a loop back. Since it
        // is at the end, a branch is not needed.
        link->Parent = p;
        int row = p->Internal.size();
        this->beginInsertRows(parentIndex, row, row);
        p->Internal.append(link);
        this->endInsertRows();
        }
      else
        {
        split1 = new pqPipelineListItem();
        if(!split1)
          {
          delete link;
          return;
          }

        split1->Type = pqPipelineListModel::Split;
        split1->Parent = p;
        split2 = new pqPipelineListItem();
        if(!split2)
          {
          delete link;
          delete split1;
          return;
          }

        split2->Type = pqPipelineListModel::Split;
        split2->Parent = p;
        link->Parent = split1;
        split1->Internal.append(link);

        // When the sink is after the source, a second link object is
        // needed.
        sourceRow = p->Internal.indexOf(sourceItem);
        int sinkRow = p->Internal.indexOf(sinkItem);
        pqPipelineListItem *link2 = 0;
        if(sinkRow > sourceRow)
          {
          link2 = new pqPipelineListItem();
          if(!link2)
            {
            delete split1;
            delete split2;
            return;
            }

          link2->Type = pqPipelineListModel::LinkBack;
          link2->Data.Link = sinkItem;
          link2->Parent = split2;
          }

        // desermine which rows will be removed.
        int startRow = sourceRow + 1;
        int endRow = sinkRow - 1;
        if(sinkRow < sourceRow)
          endRow = p->Internal.size() - 1;

        // Remove the items from the parent. Put the items on the second
        // split item.
        this->beginRemoveRows(parentIndex, startRow, endRow);
        for(i = endRow; i >= startRow; i--)
          split2->Internal.prepend(p->Internal.takeAt(i));
        this->endRemoveRows();

        // Set the parent pointers for the new chain.
        QList<pqPipelineListItem *>::Iterator jter = split2->Internal.begin();
        for( ; jter != split2->Internal.end(); ++jter)
          (*jter)->Parent = split2;

        // Insert the extra link item if needed.
        if(sinkRow > sourceRow)
          split2->Internal.append(link2);

        // Insert the new split items after the source.
        this->beginInsertRows(parentIndex, startRow, startRow + 1);
        p->Internal.append(split1);
        p->Internal.append(split2);
        this->endInsertRows();

        // Make sure the new split items are expanded.
        emit this->childAdded(this->createIndex(startRow, 0, split1));
        emit this->childAdded(this->createIndex(startRow + 1, 0, split2));
        }
      }
    else
      {
      // Source and sink have different parents.
      }
    }
  else
    {
    // Check to see if the source and sink are connected already.

    // Since the source and sink are in different windows, a link
    // object needs to be made.
    }
}

void pqPipelineListModel::removeConnection(pqPipelineObject* /*source*/,
                                           pqPipelineObject* /*sink*/)
{
}

void pqPipelineListModel::beginCreateAndAppend()
{
  if(this->Internal)
    {
    this->Internal->CommandType = pqPipelineListInternal::CreateAndAppend;
    this->Internal->MoveOrCreate = 0;
    this->Internal->Source = 0;
    }
}

void pqPipelineListModel::finishCreateAndAppend()
{
  if(!this->Internal || this->Internal->CommandType !=
      pqPipelineListInternal::CreateAndAppend)
    {
    return;
    }

  this->Internal->CommandType = pqPipelineListInternal::NoCommands;
  pqPipelineObject *filter = this->Internal->MoveOrCreate;
  pqPipelineObject *source = this->Internal->Source;
  this->Internal->MoveOrCreate = 0;
  this->Internal->Source = 0;
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
  pqPipelineListItem *p = sourceItem->Parent;
  if(p->IsLast(sourceItem))
    {
    // Add the filter to the parent after the source.
    this->Internal->Lookup.insert(filter, newItem);
    newItem->Parent = p;
    parentIndex = this->createIndex(
        p->Parent->Internal.indexOf(p), 0, p);
    int row = p->Internal.size();
    this->beginInsertRows(parentIndex, row, row);
    p->Internal.append(newItem);
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
    this->Internal->CommandType = pqPipelineListInternal::CreateAndInsert;
    this->Internal->MoveOrCreate = 0;
    this->Internal->Source = 0;
    this->Internal->Sink = 0;
    }
}

void pqPipelineListModel::finishCreateAndInsert()
{
  if(!this->Internal || this->Internal->CommandType !=
      pqPipelineListInternal::CreateAndInsert)
    {
    return;
    }

  this->Internal->CommandType = pqPipelineListInternal::NoCommands;
  pqPipelineObject *filter = this->Internal->MoveOrCreate;
  pqPipelineObject *source = this->Internal->Source;
  pqPipelineObject *sink = this->Internal->Sink;
  this->Internal->MoveOrCreate = 0;
  this->Internal->Source = 0;
  this->Internal->Sink = 0;
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
  pqPipelineListItem *p = sourceItem->Parent;
  int i = p->Internal.indexOf(sourceItem) + 1;
  if(i == p->Internal.size())
    {
    delete newItem;
    return;
    }

  QModelIndex parentIndex;
  pqPipelineListItem *next = p->Internal[i];
  if(next == sinkItem || (next->Type == pqPipelineListModel::LinkBack &&
      next->Data.Link == sinkItem))
    {
    // Insert the filter after the source item.
    this->Internal->Lookup.insert(filter, newItem);
    parentIndex = this->createIndex(
        p->Parent->Internal.indexOf(p), 0, p);
    this->beginInsertRows(parentIndex, i, i);
    newItem->Parent = p;
    p->Internal.insert(i, newItem);
    this->endInsertRows();
    }
  else
    {
    for( ; i < p->Internal.size(); i++)
      {
      next = p->Internal[i];
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
  pqPipelineListItem *p = *iter;
  if(p->Internal.size() > 0)
    {
    int mergeRow = 0;
    int windowRow = p->Parent->Internal.indexOf(p);
    QModelIndex windowIndex = this->createIndex(windowRow, 0, p);

    // If not all the window items are merge items, the current
    // children of the window need to be added to a new merge
    // item.
    bool needsMerge = false;
    QList<pqPipelineListItem *>::Iterator jter = p->Internal.begin();
    for( ; jter != p->Internal.end(); ++jter)
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
      merge->Parent = p;

      // Remove the window's current items.
      this->beginRemoveRows(windowIndex, 0, p->Internal.size() - 1);
      this->endRemoveRows();

      // Add the items to the new merge item.
      merge->Internal = p->Internal;
      p->Internal.clear();
      jter = merge->Internal.begin();
      for( ; jter != merge->Internal.end(); ++jter)
        {
        if(*jter)
          (*jter)->Parent = merge;
        }

      // Add the merge item to the window.
      this->beginInsertRows(windowIndex, 0, 0);
      p->Internal.append(merge);
      this->endInsertRows();

      emit this->childAdded(this->createIndex(0, 0, merge));
      // TODO: Sub-items of the new merge may need to be expanded.
      }

    // Add a merge item for the new source, filter, or bundle.
    pqPipelineListItem *windowItem = p;
    p = new pqPipelineListItem();
    if(p)
      {
      p->Type = pqPipelineListModel::Merge;
      p->Parent = windowItem;

      // Add the merge item to the window.
      mergeRow = windowItem->Internal.size();
      this->beginInsertRows(windowIndex, mergeRow, mergeRow);
      windowItem->Internal.append(p);
      this->endInsertRows();
      }
    }

  if(!p)
    return;

  pqPipelineListItem *item = new pqPipelineListItem();
  if(item)
    {
    item->Type = itemType;
    item->Name = object->GetProxyName();
    item->Data.Object = object;
    item->Parent = p;
    this->Internal->Lookup.insert(object, item);

    // Add the source, filter, or bundle to the parent item.
    int parentRow = p->Parent->Internal.indexOf(p);
    int rows = p->Internal.size();
    QModelIndex parentIndex = this->createIndex(parentRow, 0, p);
    this->beginInsertRows(parentIndex, rows, rows);
    p->Internal.append(item);
    this->endInsertRows();

    if(p->Internal.size() == 1)
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


