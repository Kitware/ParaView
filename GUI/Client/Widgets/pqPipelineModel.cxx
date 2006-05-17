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
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include <QApplication>
#include <QString>
#include <QStyle>
#include <QtDebug>
#include "QVTKWidget.h"

#include "vtkPVXMLElement.h"

//-----------------------------------------------------------------------------
class pqPipelineModelDataItem : public QObject
{
public:
  pqPipelineModelDataItem* Parent;
  QList<pqPipelineModelDataItem*> Children;
  pqPipelineModelItem* Object;
  pqPipelineModel::ItemType Type;

  pqPipelineModelDataItem(QObject* p) :QObject(p)
    {
    this->Parent = NULL;
    this->Object = NULL;
    this->Type = pqPipelineModel::Invalid;
    }
  pqPipelineModel::ItemType getType()
    {
    return this->Type;
    }
  int getIndexInParent()
    {
    if (!this->Parent)
      {
      return 0;
      }
    return this->Parent->Children.indexOf(this);
    }
  void addChild(pqPipelineModelDataItem* child)
    {
    if (child->Parent)
      {
      qCritical() << "child has parent.";
      return;
      }
    child->Parent = this;
    this->Children.push_back(child);
    }
  void removeChild(pqPipelineModelDataItem* child)
    {
    if (child->Parent != this)
      {
      qCritical() << "Cannot remove a non-child.";
      return;
      }
    child->Parent = NULL;
    this->Children.removeAll(child);
    }
};

//-----------------------------------------------------------------------------
class pqPipelineModelInternal 
{
public:
  pqPipelineModelInternal(QObject* parent):
    Root(parent)
  {
  }

  pqPipelineModelDataItem Root;
};


//-----------------------------------------------------------------------------
pqPipelineModel::pqPipelineModel(QObject *p)
  : QAbstractItemModel(p)
{
  this->Internal = new pqPipelineModelInternal(this);
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
  delete this->Internal;
  if(this->PixmapList)
    {
    delete [] this->PixmapList;
    }
}
//-----------------------------------------------------------------------------
int pqPipelineModel::rowCount(const QModelIndex &parentIndex) const
{
  if (parentIndex.isValid() && parentIndex.model() == this)
    {
    pqPipelineModelDataItem *item = reinterpret_cast<pqPipelineModelDataItem*>(
      parentIndex.internalPointer());
    return item->Children.size();
    }
  return this->Internal->Root.Children.size();
}

//-----------------------------------------------------------------------------
int pqPipelineModel::columnCount(const QModelIndex &) const
{
  return 1;
}

//-----------------------------------------------------------------------------
bool pqPipelineModel::hasChildren(const QModelIndex &parentIndex) const
{
  return this->rowCount(parentIndex) > 0;
}

//-----------------------------------------------------------------------------
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

  pqPipelineModelDataItem* parentItem = 0; 
  if(parentIndex.isValid())
    {
    parentItem = reinterpret_cast<pqPipelineModelDataItem*>(
      parentIndex.internalPointer());
    }
  else
    {
    // The parent refers to the model root. 
    parentItem = &this->Internal->Root;
    }

  return this->createIndex(row, column, parentItem->Children[row]);
}

//-----------------------------------------------------------------------------
QModelIndex pqPipelineModel::parent(const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelDataItem *item = reinterpret_cast<pqPipelineModelDataItem*>(
        idx.internalPointer());
    
    pqPipelineModelDataItem* _parent = item->Parent;
    return this->getIndex(_parent);
    }

  return QModelIndex();
}

//-----------------------------------------------------------------------------
QVariant pqPipelineModel::data(const QModelIndex &idx, int role) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqPipelineModelDataItem *item = reinterpret_cast<pqPipelineModelDataItem*>(
        idx.internalPointer());

    pqServer *server = dynamic_cast<pqServer*>(item->Object);
    pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item->Object);

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
            return QVariant(server->getFriendlyName());
            }
          }
        else if(source)
          {
          if(idx.column() == 0)
            {
            return QVariant(source->getProxyName());
            }
          }
        else
          {
          cout << "Cannot decide type." << endl;
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
          if(item && item->getType() != pqPipelineModel::Invalid)
            {
            // TODO: stuff to realize that this one is a link.
            return QVariant(this->PixmapList[item->getType()]);
            }
          }

        break;
        }
      }
    }

  return QVariant();
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqPipelineModel::flags(const QModelIndex &) const
{
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

//-----------------------------------------------------------------------------
pqPipelineModelItem* pqPipelineModel::getItem(const QModelIndex& idx) const
{
  if (idx.isValid() && idx.model() == this)
    {
    pqPipelineModelDataItem* item =reinterpret_cast<pqPipelineModelDataItem*>(
      idx.internalPointer());
    return item->Object;
    }
  return NULL;
}

//-----------------------------------------------------------------------------
QModelIndex pqPipelineModel::getIndexFor(pqPipelineModelItem *item) const
{
  pqPipelineModelDataItem* dataItem = this->getDataItem(item,
    &this->Internal->Root);
  return this->getIndex(dataItem);
}

//-----------------------------------------------------------------------------
pqPipelineModelDataItem* pqPipelineModel::getDataItem(pqPipelineModelItem* item,
  pqPipelineModelDataItem* _parent) const
{
  if (!_parent || !item)
    {
    return 0;
    }

  if (_parent->Object == item)
    {
    return _parent;
    }

  foreach(pqPipelineModelDataItem* child, _parent->Children)
    {
    pqPipelineModelDataItem* retVal = this->getDataItem(item, child);
    if (retVal)
      {
      return retVal;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
QModelIndex pqPipelineModel::getIndex(pqPipelineModelDataItem* dataItem) const
{
  if (dataItem && dataItem->Parent)
    {
    int rowNo = dataItem->getIndexInParent();
    if (rowNo != -1)
      {
      return this->createIndex(rowNo, 0, dataItem);
      }
    }

  // QModelIndex() implies the ROOT.
  return QModelIndex();
}


//-----------------------------------------------------------------------------
void pqPipelineModel::addChild(pqPipelineModelDataItem* _parent,
  pqPipelineModelDataItem* child)
{
  if (!_parent || !child)
    {
    qDebug() << "addChild cannot have null arguments.";
    return;
    }

  QModelIndex parentIndex = this->getIndex(_parent);
  int row = _parent->Children.size();

  this->beginInsertRows(parentIndex, row, row);
  _parent->addChild(child);
  this->endInsertRows();

  if(row == 0)
    {
    emit this->firstChildAdded(parentIndex);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineModel::removeChildFromParent(
  pqPipelineModelDataItem* child)
{
  if (!child)
    {
    qDebug() << "removeChild cannot have null arguments.";
    return;
    }

  pqPipelineModelDataItem* _parent = child->Parent;
  if (!_parent)
    {
    qDebug() << "cannot remove ROOT.";
    return;
    }

  QModelIndex parentIndex = this->getIndex(_parent);
  int row = child->getIndexInParent();

  this->beginRemoveRows(parentIndex, row, row);
  _parent->removeChild(child);
  this->endRemoveRows();
}



//-----------------------------------------------------------------------------
void pqPipelineModel::serverDataChanged()
{
  // TODO: we should determine which server data actually chnaged
  // and invalidate only that one. FOr now, just invalidate all.

  int max = this->Internal->Root.Children.size()-1;
  if (max >= 0)
    {
    QModelIndex minIndex = this->getIndex(this->Internal->Root.Children[0]);
    QModelIndex maxIndex = this->getIndex(this->Internal->Root.Children[max]);
    emit this->dataChanged(minIndex, maxIndex);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineModel::addServer(pqServer *server)
{
  if(!server)
    {
    return;
    }

  pqPipelineModelDataItem* item = new pqPipelineModelDataItem(this);
  item->Object = server;
  item->Type = pqPipelineModel::Server;

  QObject::connect(server, SIGNAL(dataModified()),
    this, SLOT(serverDataChanged()));

  this->addChild(&this->Internal->Root, item);
}

//-----------------------------------------------------------------------------
void pqPipelineModel::removeServer(pqServer *server)
{
  pqPipelineModelDataItem* item = this->getDataItem(server,
    &this->Internal->Root);

  if (!item)
    {
    qDebug() << "Requesting to remove a non-added server.";
    return;
    }

  this->removeChildFromParent(item);

  // TODO: cleanup server subtree.
  delete item;
}

//-----------------------------------------------------------------------------
void pqPipelineModel::addSource(pqPipelineSource* source)
{
  pqServerManagerModel* model = pqServerManagerModel::instance();
  pqServer* server = model->getServerForSource(source);
  pqPipelineModelDataItem* _parent = this->getDataItem(server,
    &this->Internal->Root);

  if (!_parent)
    {
    qDebug() << "Could not locate server on which the source is being added.";
    return;
    }
  
  pqPipelineModelDataItem* item = new pqPipelineModelDataItem(this);
  item->Object = source;
  item->Type = source->getType(); 

  // Add the 'source' to the server.
  this->addChild(_parent, item); 
}

//-----------------------------------------------------------------------------
void pqPipelineModel::removeSource(pqPipelineSource* source)
{
  pqPipelineModelDataItem* item = this->getDataItem(source,
    &this->Internal->Root);

  if (!item)
    {
    qDebug() << "Requesting to remove a non-added source.";
    return;
    }

  this->removeChildFromParent(item);

  // TODO: cleanup server subtree.
  delete item;
}

//-----------------------------------------------------------------------------
void pqPipelineModel::addConnection(pqPipelineSource *source,
    pqPipelineSource *sink)
{
  if(!source || !sink)
    {
    qDebug() << "Cannot connect a null source or sink.";
    return;
    }

  // Note: this slot is invoked after the connection has been set,
  
  // The only decision we need to make here is whether the sink
  // fanIn > 1. 
  // If fanIn == 1, take the sink form the server list
  // and put it under the source.
  // If fanIn > 1, -- we will deal with this later :). 

  pqPipelineFilter* filter = dynamic_cast<pqPipelineFilter*>(sink);
  if (!filter)
    {
    qDebug() << "Sink has to be a filter, alteast until we decide "
      << "if pqPipelineFilter separation is totally unnecessary.";
    return;
    }

  pqPipelineModelDataItem* srcItem = this->getDataItem(source,
    &this->Internal->Root);
  pqPipelineModelDataItem* sinkItem = this->getDataItem(sink,
    &this->Internal->Root);
  if (!srcItem || !sinkItem)
    {
    qDebug() << "COnnection involves a non-added source. Ignoring.";
    return;
    }

  if(filter->getInputCount() == 1)
    {
    // Remove the sink from where ever.
    this->removeChildFromParent(sinkItem);
  
    // Add to the children of the source.
    this->addChild(srcItem, sinkItem);
    }
  /*
  else if(filter->getInputCount() > 1)
    {
    // If the sink has one input, it needs to be moved to the server's
    // source list with an additional input. Add a link item to the
    // source in place of the sink.
    }
  else
    {
    // The sink item is already on the server's source list. A link
    // item needs to be added to the source for the new output.
    }*/
}

//-----------------------------------------------------------------------------
void pqPipelineModel::removeConnection(pqPipelineSource *source,
    pqPipelineSource *sink)
{
  if(!source || !sink)
    {
    qDebug() << "Cannot disconnect a null source or sink.";
    return;
    }

  pqPipelineModelDataItem* sinkItem = this->getDataItem(sink,
    &this->Internal->Root);
  if (!sinkItem)
    {
    qDebug() << "Connection involves a non-added source. Ignoring.";
    return;
    }

  // Note: this slot is invoked after the connection has been broken.

  // The decision to be made here is where the sink goes.
  // This depends on how may input connections the sink currently has.
  // * If fanIn == 0, put the sink in the server list.
  // * If fanIn == 1, put the sink under the source that it is connected to.
  // * If fanIn > 1, -- we will deal with this later :). 
  // The last 2 imply that the sink had a fanin of > 1 at somepoint,
  // we will deal with it later.

  pqPipelineFilter* filter = dynamic_cast<pqPipelineFilter*>(sink);
  if (!filter)
    {
    qDebug() << "Sink has to be a filter, alteast until we decide "
      << "if pqPipelineFilter separation is totally unnecessary.";
    return;
    }

  if(filter->getInputCount() == 0)
    {
    // Add to the children of the source.
    pqServer* server = sink->getServer();
    pqPipelineModelDataItem* serverItem = this->getDataItem(server,
      &this->Internal->Root);
    if (!serverItem)
      {
      qDebug() << "Failed to locate data item for server.";
      return;
      }

    this->removeChildFromParent(sinkItem);
    this->addChild(serverItem, sinkItem);
    }
  /*
  else if(filter->getInputCount() > 1)
  {
  // If the sink has one input, it needs to be moved to the server's
  // source list with an additional input. Add a link item to the
  // source in place of the sink.
  }
  else
  {
  // The sink item is already on the server's source list. A link
  // item needs to be added to the source for the new output.
  }*/

  /*
  // The server should be the same for both items.
  pqPipelineServer *server = sink->getServer();
  int serverRow = this->getServerIndexFor(server);
  if(serverRow == -1 || server != source->getServer())
  {
  return;
  }

  int row = 0;
  QModelIndex parentIndex;
  if(sink->getInputCount() == 1)
  {
  // The sink needs to be moved to the server's source list when
  // the last input is removed.
  parentIndex = this->createIndex(this->getItemRow(source), 0, source);
  row = source->getOutputIndexFor(sink);
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
  else if(sink->getInputCount() == 2)
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
   */
}

void pqPipelineModel::saveState(vtkPVXMLElement *vtkNotUsed(root), 
  pqMultiView *vtkNotUsed(multiView))
{
  /*
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
  */
}

