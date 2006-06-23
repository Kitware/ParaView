/*=========================================================================

   Program: ParaView
   Module:    pqBundleDefinitionModel.cxx

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

========================================================================*/

/// \file pqBundleDefinitionModel.cxx
/// \date 6/19/2006

#include "pqBundleDefinitionModel.h"

#include "pqPipelineSource.h"
#include "pqServerManagerSelectionModel.h"

#include <QList>
#include <QMap>
#include <QPixmap>
#include <QString>

#include "vtkSMCompoundProxy.h"
#include "vtkSMProxy.h"


/// \class pqBundleDefinitionModelItem
class pqBundleDefinitionModelItem
{
public:
  pqBundleDefinitionModelItem(pqBundleDefinitionModelItem *parent=0);
  virtual ~pqBundleDefinitionModelItem();

  virtual QString GetName() const;
  virtual pqPipelineSource *GetPipelineSource() const {return 0;}

  pqBundleDefinitionModel::ItemType Type;
  pqBundleDefinitionModelItem *Parent;
  QList<pqBundleDefinitionModelItem *> Children;
};


/// \class pqBundleDefinitionModelSource
class pqBundleDefinitionModelSource : public pqBundleDefinitionModelItem
{
public:
  pqBundleDefinitionModelSource(pqBundleDefinitionModelItem *parent=0,
      pqPipelineSource *source=0);
  virtual ~pqBundleDefinitionModelSource() {}

  virtual QString GetName() const;
  virtual pqPipelineSource *GetPipelineSource() const;

  pqPipelineSource *Source;
};


/// \class pqBundleDefinitionModelLink
class pqBundleDefinitionModelLink : public pqBundleDefinitionModelItem
{
public:
  pqBundleDefinitionModelLink(pqBundleDefinitionModelItem *parent=0,
      pqBundleDefinitionModelSource *link=0);
  virtual ~pqBundleDefinitionModelLink() {}

  virtual QString GetName() const;
  virtual pqPipelineSource *GetPipelineSource() const;

  pqBundleDefinitionModelSource *Link;
};


//-----------------------------------------------------------------------------
pqBundleDefinitionModelItem::pqBundleDefinitionModelItem(
    pqBundleDefinitionModelItem *parent)
  : Children()
{
  this->Type = pqBundleDefinitionModel::Invalid;
  this->Parent = parent;
}

pqBundleDefinitionModelItem::~pqBundleDefinitionModelItem()
{
  QList<pqBundleDefinitionModelItem *>::Iterator iter = this->Children.begin();
  for( ; iter != this->Children.end(); ++iter)
    {
    delete *iter;
    }

  this->Children.clear();
}

QString pqBundleDefinitionModelItem::GetName() const
{
  return QString();
}


//-----------------------------------------------------------------------------
pqBundleDefinitionModelSource::pqBundleDefinitionModelSource(
    pqBundleDefinitionModelItem *parent, pqPipelineSource *source)
  : pqBundleDefinitionModelItem(parent)
{
  this->Source = source;

  // Set the type for the source.
  vtkSMProxy *proxy = source->getProxy();
  if(vtkSMCompoundProxy::SafeDownCast(proxy) != 0)
    {
    this->Type = pqBundleDefinitionModel::Bundle;
    }
  else if(strcmp(proxy->GetXMLGroup(), "filters") == 0)
    {
    this->Type = pqBundleDefinitionModel::Filter;
    }
  else if(strcmp(proxy->GetXMLGroup(), "sources") == 0)
    {
    this->Type = pqBundleDefinitionModel::Source;
    }
}

QString pqBundleDefinitionModelSource::GetName() const
{
  if(this->Source)
    {
    return this->Source->getProxyName();
    }

  return QString();
}

pqPipelineSource *pqBundleDefinitionModelSource::GetPipelineSource() const
{
  return this->Source;
}


//-----------------------------------------------------------------------------
pqBundleDefinitionModelLink::pqBundleDefinitionModelLink(
    pqBundleDefinitionModelItem *parent, pqBundleDefinitionModelSource *link)
  : pqBundleDefinitionModelItem(parent)
{
  this->Link = link;
  this->Type = pqBundleDefinitionModel::Link;
}

QString pqBundleDefinitionModelLink::GetName() const
{
  if(this->Link)
    {
    return this->Link->GetName();
    }

  return QString();
}

pqPipelineSource *pqBundleDefinitionModelLink::GetPipelineSource() const
{
  if(this->Link)
    {
    return this->Link->GetPipelineSource();
    }

  return 0;
}


//-----------------------------------------------------------------------------
pqBundleDefinitionModel::pqBundleDefinitionModel(QObject *parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Root = new pqBundleDefinitionModelItem();

  // Initialize the pixmap list.
  this->PixmapList = new QPixmap[pqBundleDefinitionModel::LastType + 1];
  if(this->PixmapList)
    {
    this->PixmapList[pqBundleDefinitionModel::Source].load(
        ":/pqWidgets/pqSource16.png");
    this->PixmapList[pqBundleDefinitionModel::Filter].load(
        ":/pqWidgets/pqFilter16.png");
    this->PixmapList[pqBundleDefinitionModel::Bundle].load(
        ":/pqWidgets/pqBundle16.png");
    this->PixmapList[pqBundleDefinitionModel::Link].load(
        ":/pqWidgets/pqLinkBack16.png");
    }
}

pqBundleDefinitionModel::~pqBundleDefinitionModel()
{
  delete this->Root;
}

int pqBundleDefinitionModel::rowCount(const QModelIndex &parentIndex) const
{
  pqBundleDefinitionModelItem *item = this->getModelItemFor(parentIndex);
  if(item)
    {
    return item->Children.size();
    }

  return 0;
}

int pqBundleDefinitionModel::columnCount(const QModelIndex &) const
{
  return 1;
}

bool pqBundleDefinitionModel::hasChildren(const QModelIndex &parentIndex) const
{
  return this->rowCount(parentIndex) > 0;
}

QModelIndex pqBundleDefinitionModel::index(int row, int column,
    const QModelIndex &parentIndex) const
{
  pqBundleDefinitionModelItem *item = this->getModelItemFor(parentIndex);
  if(item && row >= 0 && row < item->Children.size() && column >= 0 &&
      column < this->columnCount(parentIndex))
    {
    return this->createIndex(row, column, item->Children[row]);
    }

  return QModelIndex();
}

QModelIndex pqBundleDefinitionModel::parent(const QModelIndex &idx) const
{
  pqBundleDefinitionModelItem *item = this->getModelItemFor(idx);
  if(item && item->Parent && item->Parent != this->Root)
    {
    int row = item->Parent->Parent->Children.indexOf(item->Parent);
    return this->createIndex(row, 0, item->Parent);
    }

  return QModelIndex();
}

QVariant pqBundleDefinitionModel::data(const QModelIndex &idx, int role) const
{
  pqBundleDefinitionModelItem *item = this->getModelItemFor(idx);
  if(item && item != this->Root)
    {
    switch(role)
      {
      case Qt::DisplayRole:
      case Qt::ToolTipRole:
      case Qt::EditRole:
        {
        if(idx.column() == 0)
          {
          return QVariant(item->GetName());
          }

        break;
        }
      case Qt::DecorationRole:
        {
        if(idx.column() == 0 && this->PixmapList &&
            item->Type != pqBundleDefinitionModel::Invalid)
          {
          return QVariant(this->PixmapList[item->Type]);
          }

        break;
        }
      }
    }

  return QVariant();
}

Qt::ItemFlags pqBundleDefinitionModel::flags(const QModelIndex &idx) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void pqBundleDefinitionModel::setContents(
    const pqServerManagerSelection *items)
{
  delete this->Root;
  this->Root = new pqBundleDefinitionModelItem();
  if(!items)
    {
    this->reset();
    return;
    }

  // Add all the items to the model.
  // TODO: Make sure the sources are from the same server.
  pqPipelineSource *source = 0;
  pqBundleDefinitionModelSource *item = 0;
  QMap<pqPipelineSource *, pqBundleDefinitionModelSource *> itemMap;
  pqServerManagerSelection::ConstIterator iter = items->begin();
  for( ; iter != items->end(); ++iter)
    {
    source = qobject_cast<pqPipelineSource *>(*iter);
    if(source)
      {
      item = new pqBundleDefinitionModelSource(this->Root, source);
      this->Root->Children.append(item);
      itemMap.insert(source, item);
      }
    }

  // Connect the items based on inputs.
  int i = 0;
  pqBundleDefinitionModelLink *link = 0;
  pqBundleDefinitionModelSource *output = 0;
  pqBundleDefinitionModelItem *otherItem = 0;
  QList<pqBundleDefinitionModelItem *> fanInList;
  QMap<pqPipelineSource *, pqBundleDefinitionModelSource *>::Iterator jter;
  for(iter = items->begin(); iter != items->end(); ++iter)
    {
    jter = itemMap.find(qobject_cast<pqPipelineSource *>(*iter));
    if(jter != itemMap.end())
      {
      // Loop through the outputs of the source. If the output is in
      // the item map, move it to the correct place.
      item = *jter;
      for(i = 0; i < item->Source->getNumberOfConsumers(); i++)
        {
        jter = itemMap.find(item->Source->getConsumer(i));
        if(jter != itemMap.end())
          {
          output = *jter;
          if(output->Parent == this->Root)
            {
            if(fanInList.contains(output))
              {
              // Add a link item for the connection.
              link = new pqBundleDefinitionModelLink(item, output);
              item->Children.append(link);
              }
            else
              {
              // Move the output to the item's list of children.
              this->Root->Children.removeAll(output);
              output->Parent = item;
              item->Children.append(output);
              }
            }
          else
            {
            // Move the output from the other item's list of children
            // to the root. Add the output to the fan-in list.
            otherItem = output->Parent;
            otherItem->Children.removeAll(output);
            output->Parent = this->Root;
            this->Root->Children.append(output);
            fanInList.append(output);
            link = new pqBundleDefinitionModelLink(item, output);
            item->Children.append(link);
            link = new pqBundleDefinitionModelLink(otherItem, output);
            otherItem->Children.append(link);
            }
          }
        }
      }
    }

  this->reset();
}

QModelIndex pqBundleDefinitionModel::getNextIndex(const QModelIndex &idx) const
{
  pqBundleDefinitionModelItem *item = this->getModelItemFor(idx);
  item = this->getNextItem(item);
  if(item && item->Parent)
    {
    int row = item->Parent->Children.indexOf(item);
    return this->createIndex(row, 0, item);
    }

  return QModelIndex();
}

pqPipelineSource *pqBundleDefinitionModel::getSourceFor(
    const QModelIndex &idx) const
{
  pqBundleDefinitionModelItem *item = this->getModelItemFor(idx);
  if(item)
    {
    return item->GetPipelineSource();
    }

  return 0;
}

pqBundleDefinitionModelItem *pqBundleDefinitionModel::getModelItemFor(
    const QModelIndex &idx) const
{
  if(!idx.isValid())
    {
    return this->Root;
    }

  if(idx.model() == this)
    {
    return reinterpret_cast<pqBundleDefinitionModelItem *>(
        idx.internalPointer());
    }

  return 0;
}

pqBundleDefinitionModelItem *pqBundleDefinitionModel::getNextItem(
    pqBundleDefinitionModelItem *item) const
{
  if(!item)
    {
    return 0;
    }

  if(item->Children.size() > 0)
    {
    return item->Children.first();
    }

  // Search up the ancestors for an item with multiple children.
  // The next item will be the next child.
  int row = 0;
  int count = 0;
  while(item->Parent)
    {
    count = item->Parent->Children.size();
    if(count > 1)
      {
      row = item->Parent->Children.indexOf(item) + 1;
      if(row < count)
        {
        return item->Parent->Children[row];
        }
      }

    item = item->Parent;
    }

  return 0;
}


