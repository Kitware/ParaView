
#include "pqObjectInspector.h"

#include "pqObjectInspectorItem.h"
#include "pqSMAdaptor.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"

#include <QList>
#include <QString>
#include <QVariant>


class pqObjectInspectorInternal : public QList<pqObjectInspectorItem *> {};


pqObjectInspector::pqObjectInspector(QObject *parent)
  : QAbstractItemModel(parent)
{
  this->Internal = new pqObjectInspectorInternal();
  this->Adapter = 0;
  this->Proxy = 0;
}

pqObjectInspector::~pqObjectInspector()
{
  if(this->Internal)
    {
    this->cleanData(false);
    delete this->Internal;
    this->Internal = 0;
    }
}

int pqObjectInspector::rowCount(const QModelIndex &parent) const
{
  int rows = 0;
  if(this->Internal)
    {
    if(parent.isValid() && parent.model() == this)
      {
      pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
          parent.internalPointer());
      if(item)
        rows = item->getChildCount();
      }
    else
      rows = this->Internal->size();
    }

  return rows;
}

int pqObjectInspector::columnCount(const QModelIndex &parent) const
{
  if(this->Internal)
    return 2;
  return 0;
}

bool pqObjectInspector::hasChildren(const QModelIndex &parent) const
{
  if(parent.isValid() && parent.model() == this)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        parent.internalPointer());
    if(item && parent.column() == 0)
      return item->getChildCount() > 0;
    }
  else if(this->Internal)
    return this->Internal->size() > 0;

  return false;
}

QModelIndex pqObjectInspector::index(int row, int column,
    const QModelIndex &parent) const
{
  if(this->Internal && row >= 0 && column >= 0 && column < 2)
    {
    pqObjectInspectorItem *item = 0;
    if(parent.isValid() && parent.model() == this)
      {
      pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
          parent.internalPointer());
      if(item && row < item->getChildCount())
        {
        item = item->getChild(row);
        return this->createIndex(row, column, item);
        }
      }
    else if(row < this->Internal->size())
      {
      item = (*this->Internal)[row];
      return this->createIndex(row, column, item);
      }
    }

  return QModelIndex();
}

QModelIndex pqObjectInspector::parent(const QModelIndex &index) const
{
  if(index.isValid() && index.model() == this)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        index.internalPointer());
    if(item && item->getParent())
      {
      // Find the parent's row and column from the parent's parent.
      pqObjectInspectorItem *parent = item->getParent();
      int row = this->getItemIndex(parent);
      if(row != -1)
        return this->createIndex(row, 0, parent);
      }
    }

  return QModelIndex();
}

QVariant pqObjectInspector::data(const QModelIndex &index, int role) const
{
  if(index.isValid() && index.model() == this)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        index.internalPointer());
    switch(role)
      {
      case Qt::DisplayRole:
        {
        if(index.column() == 0)
          return QVariant(item->getPropertyName());
        else
          return QVariant(item->getValue().toString());
        }
      case Qt::EditRole:
        {
        if(index.column() == 0)
          return QVariant(item->getPropertyName());
        else
          return item->getValue();
        }
      }
    }

  return QVariant();
}

bool pqObjectInspector::setData(const QModelIndex &index,
    const QVariant &value, int role)
{
  return false;
}

Qt::ItemFlags pqObjectInspector::flags(const QModelIndex &index) const
{
  // TODO: Add editable to the flags for the item values.
  return (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

void pqObjectInspector::setProxy(pqSMAdaptor *adapter, vtkSMProxy *proxy)
{
  // Clean up the current property data.
  this->cleanData();

  // Save the new proxy. Get all the property data.
  this->Adapter = adapter;
  this->Proxy = proxy;
  if(this->Internal && this->Adapter && this->Proxy)
    {
    pqObjectInspectorItem *item = 0;
    pqObjectInspectorItem *child = 0;
    vtkSMProperty *prop = 0;
    vtkSMPropertyIterator *iter = this->Proxy->NewPropertyIterator();

    // Put the items on a temporary list. The view needs to know
    // the number of rows that will be added.
    QList<pqObjectInspectorItem *> tempList;
    for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      // Skip the property if it doesn't have a valid value. This
      // is the case with the data information.
      prop = iter->GetProperty();
      QVariant value = this->Adapter->getProperty(prop);
      if(!value.isValid())
        continue;

      // Add the valid property to the list. If the property value
      // is a list, create child items. Link the values to the vtk
      // properties.
      item = new pqObjectInspectorItem();
      if(item)
        {
        tempList.append(item);
        item->setPropertyName(iter->GetKey());
        if(value.type() == QVariant::List)
          {
          QList<QVariant> list = value.toList();
          QList<QVariant>::Iterator li = list.begin();
          for(int index = 0; li != list.end(); ++li, ++index)
            {
            QString num;
            num.setNum(index + 1);
            child = new pqObjectInspectorItem();
            if(child)
              {
              child->setPropertyName(num);
              child->setValue(*li);
              child->setParent(item);
              item->addChild(child);
              this->Adapter->linkPropertyTo(this->Proxy, prop, index,
                  child, "value");
              connect(child, SIGNAL(valueChanged(pqObjectInspectorItem *)),
                  this, SLOT(handleValueChange(pqObjectInspectorItem *)));
              }
            }
          }
        else
          {
          item->setValue(value);
          this->Adapter->linkPropertyTo(this->Proxy, prop, 0, item, "value");
          connect(item, SIGNAL(valueChanged(pqObjectInspectorItem *)),
              this, SLOT(handleValueChange(pqObjectInspectorItem *)));
          }
        }
      }

    // Notify the view that new rows will be inserted. Then, copy
    // the new rows to the model's list.
    if(tempList.size() > 0)
      {
      this->beginInsertRows(QModelIndex(), 0, tempList.size() - 1);
      QList<pqObjectInspectorItem *>::Iterator li = tempList.begin();
      for( ; li != tempList.end(); ++li)
        this->Internal->append(*li);
      this->endInsertRows();
      }
    }
}

void pqObjectInspector::cleanData(bool notify)
{
  if(this->Internal)
    {
    // Make sure the view is notified of the removed items.
    bool notifyView = this->Internal->size() > 0;
    if(notify && notifyView)
      this->beginRemoveRows(QModelIndex(), 0, this->Internal->size() - 1);

    // Clean up the list of items.
    pqObjectInspectorInternal::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      pqObjectInspectorItem *item = *iter;
      if(item)
        {
        if(this->Adapter && this->Proxy)
          {
          // If the item has a list, unlink each item from its property.
          // Otherwise, unlink the item from the vtk property.
          QString name = item->getPropertyName();
          if(item->getChildCount() > 0)
            {
            for(int i = 0; i < item->getChildCount(); ++i)
              {
              pqObjectInspectorItem *child = item->getChild(i);
              this->Adapter->unlinkPropertyFrom(this->Proxy,
                  this->Proxy->GetProperty(name.toAscii().data()), i,
                  child, "value");
              }
            }
          else
            {
            this->Adapter->unlinkPropertyFrom(this->Proxy,
                this->Proxy->GetProperty(name.toAscii().data()), 0,
                item, "value");
            }
          }

        delete *iter;
        *iter = 0;
        }
      }

    this->Internal->clear();
    if(notify && notifyView)
      this->endRemoveRows();
    }
}

int pqObjectInspector::getItemIndex(pqObjectInspectorItem *item) const
{
  if(item && this->Internal)
    {
    if(item->getParent())
      return item->getParent()->getChildIndex(item);
    else
      {
      pqObjectInspectorInternal::Iterator iter = this->Internal->begin();
      for(int row = 0; iter != this->Internal->end(); ++iter, ++row)
        {
        if(*iter == item)
          return row;
        }
      }
    }

  return -1;
}

void pqObjectInspector::handleNameChange(pqObjectInspectorItem *item)
{
  int row = this->getItemIndex(item);
  if(row != -1)
    {
    QModelIndex index = this->createIndex(row, 0, item);
    emit dataChanged(index, index);
    }
}

void pqObjectInspector::handleValueChange(pqObjectInspectorItem *item)
{
  int row = this->getItemIndex(item);
  if(row != -1)
    {
    QModelIndex index = this->createIndex(row, 1, item);
    emit dataChanged(index, index);
    }
}


