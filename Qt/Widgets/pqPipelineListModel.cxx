
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

public:
  pqPipelineListModel::ItemType Type;
  QString Name;
  pqPipelineListItem *Parent;
  QList<pqPipelineListItem *> Internal;
  union {
    pqPipelineServer *Server;
    pqPipelineObject *Object;
  } Data;
};


class pqPipelineListInternal :
    public QHash<pqPipelineObject *, pqPipelineListItem *> {};


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


pqPipelineListModel::pqPipelineListModel(QObject *parent)
  : QAbstractItemModel(parent)
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
    connect(pipeline, SIGNAL(windowAdded(pqPipelineObject *)),
        this, SLOT(addWindow(pqPipelineObject *)));
    connect(pipeline, SIGNAL(removingWindow(pqPipelineObject *)),
        this, SLOT(removeWindow(pqPipelineObject *)));
    connect(pipeline, SIGNAL(sourceCreated(pqPipelineObject *)),
        this, SLOT(addSource(pqPipelineObject *)));
    connect(pipeline, SIGNAL(filterCreated(pqPipelineObject *)),
        this, SLOT(addFilter(pqPipelineObject *)));
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

int pqPipelineListModel::rowCount(const QModelIndex &parent) const
{
  pqPipelineListItem *item = this->Root;
  if(parent.isValid())
    {
    if(parent.model() == this)
      item = reinterpret_cast<pqPipelineListItem *>(parent.internalPointer());
    else
      item = 0;
    }

  if(item)
    return item->Internal.size();

  return 0;
}

int pqPipelineListModel::columnCount(const QModelIndex &parent) const
{
  return 1;
}

bool pqPipelineListModel::hasChildren(const QModelIndex &parent) const
{
  pqPipelineListItem *item = this->Root;
  if(parent.isValid())
    {
    if(parent.model() == this)
      item = reinterpret_cast<pqPipelineListItem *>(parent.internalPointer());
    else
      item = 0;
    }

  if(item)
    return item->Internal.size() > 0;

  return false;
}

QModelIndex pqPipelineListModel::index(int row, int column,
    const QModelIndex &parent) const
{
  pqPipelineListItem *item = this->Root;
  if(parent.isValid())
    {
    if(parent.model() == this)
      item = reinterpret_cast<pqPipelineListItem *>(parent.internalPointer());
    else
      item = 0;
    }

  if(item && column == 0 && row >= 0 && row < item->Internal.size())
    return this->createIndex(row, column, item->Internal[row]);

  return QModelIndex();
}

QModelIndex pqPipelineListModel::parent(const QModelIndex &index) const
{
  if(this->Root && index.isValid() && index.model() == this)
    {
    pqPipelineListItem *item = reinterpret_cast<pqPipelineListItem *>(
        index.internalPointer());
    if(item && item->Parent && item->Parent->Parent)
      {
      int row = item->Parent->Parent->Internal.indexOf(item->Parent);
      if(row >= 0)
        return this->createIndex(row, 0, item->Parent);
      }
    }

  return QModelIndex();
}

QVariant pqPipelineListModel::data(const QModelIndex &index, int role) const
{
  if(this->Root && index.isValid() && index.model() == this)
    {
    pqPipelineListItem *item = reinterpret_cast<pqPipelineListItem *>(
        index.internalPointer());
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

Qt::ItemFlags pqPipelineListModel::flags(const QModelIndex &index) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

pqPipelineListModel::ItemType pqPipelineListModel::getTypeFor(
    const QModelIndex &index) const
{
  if(this->Internal && index.isValid() && index.model() == this)
    {
    pqPipelineListItem *item = reinterpret_cast<pqPipelineListItem *>(
        index.internalPointer());
    if(item)
      return item->Type;
    }

  return pqPipelineListModel::Invalid;
}

vtkSMProxy *pqPipelineListModel::getProxyFor(const QModelIndex &index) const
{
  vtkSMProxy *proxy = 0;
  if(this->Internal && index.isValid() && index.model() == this)
    {
    pqPipelineListItem *item = reinterpret_cast<pqPipelineListItem *>(
        index.internalPointer());
    if(item && (item->Type == pqPipelineListModel::Source || 
        item->Type == pqPipelineListModel::Filter))
      {
      proxy = item->Data.Object->GetProxy();
      }
    }

  return proxy;
}

QWidget *pqPipelineListModel::getWidgetFor(const QModelIndex &index) const
{
  QWidget *widget = 0;
  if(this->Internal && index.isValid() && index.model() == this)
    {
    pqPipelineListItem *item = reinterpret_cast<pqPipelineListItem *>(
        index.internalPointer());
    if(item && item->Type == pqPipelineListModel::Window)
      widget = item->Data.Object->GetWidget();
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
        this->Internal->find(object);
    if(iter != this->Internal->end())
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
    pqPipelineObject *object = pipeline->getObjectFor(window);
    QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
        this->Internal->find(object);
    if(iter != this->Internal->end())
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
      //this->beginRemoveRows(QModelIndex(), i, i);
      delete *iter;
      this->Root->Internal.erase(iter);
      //this->endRemoveRows();
      this->reset(); //TEMP
      break;
      }
    }
}

void pqPipelineListModel::addWindow(pqPipelineObject *window)
{
  if(!this->Internal || !this->Root || !window || !window->GetServer())
    return;

  // Make sure the window item doesn't exist.
  if(this->Internal->find(window) != this->Internal->end())
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
    item->Name = "Window";
    item->Data.Object = window;
    item->Parent = serverItem;
    this->Internal->insert(window, item);

    // Add the window to the server item.
    int rows = serverItem->Internal.size();
    QModelIndex parent = this->createIndex(serverRow, 0, serverItem);
    this->beginInsertRows(parent, rows, rows);
    serverItem->Internal.append(item);
    this->endInsertRows();

    if(serverItem->Internal.size() == 1)
      emit this->childAdded(parent);
    }
}

void pqPipelineListModel::removeWindow(pqPipelineObject *window)
{
  if(!this->Internal || !this->Root || !window)
    return;

  // Look up the window in the hash table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->find(window);
  if(iter == this->Internal->end())
    return;

  // Remove the item from the server and delete it. Remove it from
  // the hash table along with all of its children.
  pqPipelineListItem *item = *iter;
  pqPipelineListItem *serverItem = item->Parent;

  //int serverRow = this->Root->Internal.indexOf(serverItem);
  //QModelIndex parent = this->createIndex(serverRow, 0, serverItem);
  //int row = serverItem->Internal.indexOf(item);
  //this->beginRemoveRows(parent, row, row);
  serverItem->Internal.removeAll(item);
  this->Internal->erase(iter);
  this->removeLookupItems(item);
  delete item;
  //this->endRemoveRows();
  this->reset(); //TEMP
}

void pqPipelineListModel::addSource(pqPipelineObject *source)
{
  if(!this->Internal || !this->Root || !source || !source->GetParent())
    return;

  // Make sure the source item doesn't exist.
  if(this->Internal->find(source) != this->Internal->end())
    return;

  // Get the window item from the lookup table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->find(source->GetParent());
  if(iter == this->Internal->end())
    return;

  pqPipelineListItem *windowItem = *iter;
  int windowRow = windowItem->Parent->Internal.indexOf(windowItem);

  // Add the source to the window.
  pqPipelineListItem *item = new pqPipelineListItem();
  if(item)
    {
    item->Type = pqPipelineListModel::Source;
    item->Name = source->GetProxy()->GetVTKClassName();
    item->Data.Object = source;
    item->Parent = windowItem;
    this->Internal->insert(source, item);

    // Add the source to the window item.
    int rows = windowItem->Internal.size();
    QModelIndex parent = this->createIndex(windowRow, 0, windowItem);
    this->beginInsertRows(parent, rows, rows);
    windowItem->Internal.append(item);
    this->endInsertRows();

    if(windowItem->Internal.size() == 1)
      emit this->childAdded(parent);
    }
}

void pqPipelineListModel::removeSource(pqPipelineObject *source)
{
  if(!this->Internal || !this->Root || !source || !source->GetParent())
    return;

  // Get the source item from the lookup table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->find(source);
  if(iter == this->Internal->end())
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
  this->Internal->erase(iter);
  this->removeLookupItems(item);
  delete item;
  //this->endRemoveRows();
  this->reset(); //TEMP
}

void pqPipelineListModel::addFilter(pqPipelineObject *filter)
{
  if(!this->Internal || !this->Root || !filter || !filter->GetParent())
    return;

  // Make sure the filter item doesn't exist.
  if(this->Internal->find(filter) != this->Internal->end())
    return;

  // Get the window item from the lookup table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->find(filter->GetParent());
  if(iter == this->Internal->end())
    return;

  pqPipelineListItem *windowItem = *iter;
  int windowRow = windowItem->Parent->Internal.indexOf(windowItem);

  // Add the filter to the window.
  pqPipelineListItem *item = new pqPipelineListItem();
  if(item)
    {
    item->Type = pqPipelineListModel::Filter;
    item->Name = filter->GetProxy()->GetVTKClassName();
    item->Data.Object = filter;
    item->Parent = windowItem;
    this->Internal->insert(filter, item);

    // Add the filter to the window item.
    int rows = windowItem->Internal.size();
    QModelIndex parent = this->createIndex(windowRow, 0, windowItem);
    this->beginInsertRows(parent, rows, rows);
    windowItem->Internal.append(item);
    this->endInsertRows();

    if(windowItem->Internal.size() == 1)
      emit this->childAdded(parent);
    }
}

void pqPipelineListModel::removeFilter(pqPipelineObject *filter)
{
  if(!this->Internal || !this->Root || !filter || !filter->GetParent())
    return;

  // Get the filter item from the lookup table.
  QHash<pqPipelineObject *, pqPipelineListItem *>::Iterator iter =
      this->Internal->find(filter);
  if(iter == this->Internal->end())
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
  this->Internal->erase(iter);
  this->removeLookupItems(item);
  delete item;
  //this->endRemoveRows();
  this->reset(); //TEMP
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
      this->Internal->remove((*iter)->Data.Object);
      }
    }
}


