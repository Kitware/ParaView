/*=========================================================================

   Program: ParaView
   Module:    pqCustomFilterDefinitionModel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
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

/// \file pqCustomFilterDefinitionModel.cxx
/// \date 6/19/2006

#include "pqCustomFilterDefinitionModel.h"

#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxySelection.h"
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMOutputPort.h"

#include <QList>
#include <QMap>
#include <QPixmap>
#include <QSet>
#include <QString>

/// \class pqCustomFilterDefinitionModelItem
class pqCustomFilterDefinitionModelItem
{
public:
  pqCustomFilterDefinitionModelItem(pqCustomFilterDefinitionModelItem* parent = nullptr);
  virtual ~pqCustomFilterDefinitionModelItem();

  virtual QString GetName() const;
  virtual pqPipelineSource* GetPipelineSource() const { return nullptr; }

  pqCustomFilterDefinitionModel::ItemType Type;
  pqCustomFilterDefinitionModelItem* Parent;
  QList<pqCustomFilterDefinitionModelItem*> Children;
};

/// \class pqCustomFilterDefinitionModelSource
class pqCustomFilterDefinitionModelSource : public pqCustomFilterDefinitionModelItem
{
public:
  pqCustomFilterDefinitionModelSource(
    pqCustomFilterDefinitionModelItem* parent = nullptr, pqPipelineSource* source = nullptr);
  ~pqCustomFilterDefinitionModelSource() override = default;

  QString GetName() const override;
  pqPipelineSource* GetPipelineSource() const override;

  pqPipelineSource* Source;
};

/// \class pqCustomFilterDefinitionModelLink
class pqCustomFilterDefinitionModelLink : public pqCustomFilterDefinitionModelItem
{
public:
  pqCustomFilterDefinitionModelLink(pqCustomFilterDefinitionModelItem* parent = nullptr,
    pqCustomFilterDefinitionModelSource* link = nullptr);
  ~pqCustomFilterDefinitionModelLink() override = default;

  QString GetName() const override;
  pqPipelineSource* GetPipelineSource() const override;

  pqCustomFilterDefinitionModelSource* Link;
};

//-----------------------------------------------------------------------------
pqCustomFilterDefinitionModelItem::pqCustomFilterDefinitionModelItem(
  pqCustomFilterDefinitionModelItem* parent)
  : Children()
{
  this->Type = pqCustomFilterDefinitionModel::Invalid;
  this->Parent = parent;
}

pqCustomFilterDefinitionModelItem::~pqCustomFilterDefinitionModelItem()
{
  QList<pqCustomFilterDefinitionModelItem*>::Iterator iter;
  for (iter = this->Children.begin(); iter != this->Children.end(); ++iter)
  {
    delete *iter;
  }

  this->Children.clear();
}

QString pqCustomFilterDefinitionModelItem::GetName() const
{
  return QString();
}

//-----------------------------------------------------------------------------
pqCustomFilterDefinitionModelSource::pqCustomFilterDefinitionModelSource(
  pqCustomFilterDefinitionModelItem* parent, pqPipelineSource* source)
  : pqCustomFilterDefinitionModelItem(parent)
{
  this->Source = source;

  // Set the type for the source.
  vtkSMProxy* proxy = source->getProxy();
  if (proxy->IsA("vtkSMCompoundSourceProxy"))
  {
    this->Type = pqCustomFilterDefinitionModel::CustomFilter;
  }
  else if (strcmp(proxy->GetXMLGroup(), "filters") == 0)
  {
    this->Type = pqCustomFilterDefinitionModel::Filter;
  }
  else if (strcmp(proxy->GetXMLGroup(), "sources") == 0)
  {
    this->Type = pqCustomFilterDefinitionModel::Source;
  }
}

QString pqCustomFilterDefinitionModelSource::GetName() const
{
  if (this->Source)
  {
    return this->Source->getSMName();
  }

  return QString();
}

pqPipelineSource* pqCustomFilterDefinitionModelSource::GetPipelineSource() const
{
  return this->Source;
}

//-----------------------------------------------------------------------------
pqCustomFilterDefinitionModelLink::pqCustomFilterDefinitionModelLink(
  pqCustomFilterDefinitionModelItem* parent, pqCustomFilterDefinitionModelSource* link)
  : pqCustomFilterDefinitionModelItem(parent)
{
  this->Link = link;
  this->Type = pqCustomFilterDefinitionModel::Link;
}

QString pqCustomFilterDefinitionModelLink::GetName() const
{
  if (this->Link)
  {
    return this->Link->GetName();
  }

  return QString();
}

pqPipelineSource* pqCustomFilterDefinitionModelLink::GetPipelineSource() const
{
  if (this->Link)
  {
    return this->Link->GetPipelineSource();
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
pqCustomFilterDefinitionModel::pqCustomFilterDefinitionModel(QObject* parentObject)
  : QAbstractItemModel(parentObject)
{
  this->Root = new pqCustomFilterDefinitionModelItem();

  // Initialize the pixmap list.
  this->PixmapList = new QPixmap[pqCustomFilterDefinitionModel::LastType + 1];
  if (this->PixmapList)
  {
    this->PixmapList[pqCustomFilterDefinitionModel::Source].load(
      ":/pqWidgets/Icons/pqSource16.png");
    this->PixmapList[pqCustomFilterDefinitionModel::Filter].load(
      ":/pqWidgets/Icons/pqFilter16.png");
    this->PixmapList[pqCustomFilterDefinitionModel::CustomFilter].load(
      ":/pqWidgets/Icons/pqBundle16.png");
    this->PixmapList[pqCustomFilterDefinitionModel::Link].load(
      ":/pqWidgets/Icons/pqLinkBack16.png");
  }
}

pqCustomFilterDefinitionModel::~pqCustomFilterDefinitionModel()
{
  delete this->Root;
  delete[] this->PixmapList;
}

int pqCustomFilterDefinitionModel::rowCount(const QModelIndex& parentIndex) const
{
  pqCustomFilterDefinitionModelItem* item = this->getModelItemFor(parentIndex);
  if (item)
  {
    return item->Children.size();
  }

  return 0;
}

int pqCustomFilterDefinitionModel::columnCount(const QModelIndex&) const
{
  return 1;
}

bool pqCustomFilterDefinitionModel::hasChildren(const QModelIndex& parentIndex) const
{
  return this->rowCount(parentIndex) > 0;
}

QModelIndex pqCustomFilterDefinitionModel::index(
  int row, int column, const QModelIndex& parentIndex) const
{
  pqCustomFilterDefinitionModelItem* item = this->getModelItemFor(parentIndex);
  if (item && row >= 0 && row < item->Children.size() && column >= 0 &&
    column < this->columnCount(parentIndex))
  {
    return this->createIndex(row, column, item->Children[row]);
  }

  return QModelIndex();
}

QModelIndex pqCustomFilterDefinitionModel::parent(const QModelIndex& idx) const
{
  pqCustomFilterDefinitionModelItem* item = this->getModelItemFor(idx);
  if (item && item->Parent && item->Parent != this->Root)
  {
    int row = item->Parent->Parent->Children.indexOf(item->Parent);
    return this->createIndex(row, 0, item->Parent);
  }

  return QModelIndex();
}

QVariant pqCustomFilterDefinitionModel::data(const QModelIndex& idx, int role) const
{
  pqCustomFilterDefinitionModelItem* item = this->getModelItemFor(idx);
  if (item && item != this->Root)
  {
    switch (role)
    {
      case Qt::DisplayRole:
      case Qt::ToolTipRole:
      case Qt::EditRole:
      {
        if (idx.column() == 0)
        {
          return QVariant(item->GetName());
        }

        break;
      }
      case Qt::DecorationRole:
      {
        if (idx.column() == 0 && this->PixmapList &&
          item->Type != pqCustomFilterDefinitionModel::Invalid)
        {
          return QVariant(this->PixmapList[item->Type]);
        }

        break;
      }
    }
  }

  return QVariant();
}

Qt::ItemFlags pqCustomFilterDefinitionModel::flags(const QModelIndex&) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void pqCustomFilterDefinitionModel::setContents(const pqProxySelection& items)
{
  this->beginResetModel();
  delete this->Root;
  this->Root = new pqCustomFilterDefinitionModelItem();
  if (items.size() == 0)
  {
    this->endResetModel();
    return;
  }

  // locate pqPipelineSource instances for all the proxies in items.
  QSet<pqPipelineSource*> selectedSources;
  foreach (pqServerManagerModelItem* item, items)
  {
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = port ? port->getSource() : qobject_cast<pqPipelineSource*>(item);
    if (source)
    {
      selectedSources.insert(source);
    }
  }

  // Add all the items to the model.
  // TODO: Make sure the sources are from the same server.
  QMap<pqPipelineSource*, pqCustomFilterDefinitionModelSource*> itemMap;
  foreach (pqPipelineSource* source, selectedSources)
  {
    pqCustomFilterDefinitionModelSource* item =
      new pqCustomFilterDefinitionModelSource(this->Root, source);
    this->Root->Children.append(item);
    itemMap.insert(source, item);
  }

  // Connect the items based on inputs.
  int i = 0;
  pqCustomFilterDefinitionModelLink* link = nullptr;
  pqCustomFilterDefinitionModelSource* output = nullptr;
  pqCustomFilterDefinitionModelItem* otherItem = nullptr;
  QList<pqCustomFilterDefinitionModelItem*> fanInList;
  QMap<pqPipelineSource*, pqCustomFilterDefinitionModelSource*>::Iterator jter;
  foreach (pqPipelineSource* source, selectedSources)
  {
    jter = itemMap.find(source);
    if (jter != itemMap.end())
    {
      // Loop through the outputs of the source. If the output is in
      // the item map, move it to the correct place.
      pqCustomFilterDefinitionModelSource* item = *jter;
      QList<pqPipelineSource*> consumers = item->Source->getAllConsumers();
      for (i = 0; i < consumers.size(); i++)
      {
        jter = itemMap.find(consumers[i]);
        if (jter != itemMap.end())
        {
          output = *jter;
          if (output->Parent == this->Root)
          {
            if (fanInList.contains(output))
            {
              // Add a link item for the connection.
              link = new pqCustomFilterDefinitionModelLink(item, output);
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
            link = new pqCustomFilterDefinitionModelLink(item, output);
            item->Children.append(link);
            link = new pqCustomFilterDefinitionModelLink(otherItem, output);
            otherItem->Children.append(link);
          }
        }
      }
    }
  }

  this->endResetModel();
}

QModelIndex pqCustomFilterDefinitionModel::getNextIndex(const QModelIndex& idx) const
{
  pqCustomFilterDefinitionModelItem* item = this->getModelItemFor(idx);
  item = this->getNextItem(item);
  if (item && item->Parent)
  {
    int row = item->Parent->Children.indexOf(item);
    return this->createIndex(row, 0, item);
  }

  return QModelIndex();
}

pqPipelineSource* pqCustomFilterDefinitionModel::getSourceFor(const QModelIndex& idx) const
{
  pqCustomFilterDefinitionModelItem* item = this->getModelItemFor(idx);
  if (item)
  {
    return item->GetPipelineSource();
  }

  return nullptr;
}

pqCustomFilterDefinitionModelItem* pqCustomFilterDefinitionModel::getModelItemFor(
  const QModelIndex& idx) const
{
  if (!idx.isValid())
  {
    return this->Root;
  }

  if (idx.model() == this)
  {
    return reinterpret_cast<pqCustomFilterDefinitionModelItem*>(idx.internalPointer());
  }

  return nullptr;
}

pqCustomFilterDefinitionModelItem* pqCustomFilterDefinitionModel::getNextItem(
  pqCustomFilterDefinitionModelItem* item) const
{
  if (!item)
  {
    return nullptr;
  }

  if (item->Children.size() > 0)
  {
    return item->Children.first();
  }

  // Search up the ancestors for an item with multiple children.
  // The next item will be the next child.
  int row = 0;
  int count = 0;
  while (item->Parent)
  {
    count = item->Parent->Children.size();
    if (count > 1)
    {
      row = item->Parent->Children.indexOf(item) + 1;
      if (row < count)
      {
        return item->Parent->Children[row];
      }
    }

    item = item->Parent;
  }

  return nullptr;
}
