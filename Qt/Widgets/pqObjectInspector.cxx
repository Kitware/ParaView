
/// \file pqObjectInspector.cxx
/// \brief
///   The pqObjectInspector class is a model for an object's properties.
///
/// \date 11/7/2005

#include "pqObjectInspector.h"

#include "pqObjectInspectorItem.h"
#include "pqPipelineData.h"
#include "pqSMAdaptor.h"
#include "QVTKWidget.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyAdaptor.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>


class pqObjectInspectorInternal
{
public:
  pqObjectInspectorInternal()
    : Parameters(NULL), Information(NULL) {}

  pqObjectInspectorItem* Parameters;
  pqObjectInspectorItem* Information;

  int size() const
    {
    int count = 0;
    if(this->Parameters)
      count++;
    if(this->Information)
      count++;
    return count;
    }

  pqObjectInspectorItem* operator[](int i)
    {
    if(i == 0)
      return this->Parameters;
    if(i == 1)
      return this->Information;
    return NULL;
    }

  void clear()
    {
    if(this->Parameters)
      delete this->Parameters;
    this->Parameters = NULL;
    if(this->Information)
      delete this->Information;
    this->Information = NULL;
    }

};


pqObjectInspector::pqObjectInspector(QObject *parent)
  : QAbstractItemModel(parent)
{
  this->Commit = pqObjectInspector::Individually;
  this->Internal = new pqObjectInspectorInternal();
  this->Proxy = 0;
}

pqObjectInspector::~pqObjectInspector()
{
  if(this->Internal)
    {
    this->cleanData();
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
        rows = item->childCount();
      }
    else
      {
      rows = this->Internal->size();
      }
    }

  return rows;
}

int pqObjectInspector::columnCount(const QModelIndex &) const
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
      return item->childCount() > 0;
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
      if(item && row < item->childCount())
        {
        item = item->child(row);
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
    if(item && item->parent())
      {
      // Find the parent's row and column from the parent's parent.
      pqObjectInspectorItem *parent = item->parent();
      int row = this->itemIndex(parent);
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
      case Qt::ToolTipRole:
        {
        if(index.column() == 0)
          return QVariant(item->propertyName());
        else
          {
          QVariant domain = item->domain();
          if(item->value().type() == QVariant::Int &&
              domain.type() == QVariant::StringList)
            {
            // Return the enum name instead of the index.
            QStringList names = domain.toStringList();
            return QVariant(names[item->value().toInt()]);
            }
          else
            return QVariant(item->value().toString());
          }
        }
      case Qt::EditRole:
        {
        if(index.column() == 0)
          return QVariant(item->propertyName());
        else
          return item->value();
        }
      }
    }

  return QVariant();
}

bool pqObjectInspector::setData(const QModelIndex &index,
    const QVariant &value, int)
{
  if(index.isValid() && index.model() == this && index.column() == 1)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        index.internalPointer());
    if(item && item->childCount() == 0 && value.isValid())
      {
      // Save the value according to the item's type. The value
      // coming from the delegate might not preserve the correct
      // property type.
      QVariant localValue = value;
      if(localValue.convert(item->value().type()))
        {
        item->setValue(localValue);

        // If in immediate update mode, push the data to the server.
        // Otherwise, mark the item as modified for a later commit.
        if(this->Commit == pqObjectInspector::Individually)
          {
          if(this->Proxy)
            {
            this->commitChange(item);
            this->Proxy->UpdateVTKObjects();
            pqPipelineData *pipeline = pqPipelineData::instance();
            if(pipeline)
              {
              QVTKWidget *window = pipeline->getWindowFor(this->Proxy);
              if(window)
                window->update();
              }
            }
          }
        else
          item->setModified(true);

        return true;
        }
      }
    }

  return false;
}

Qt::ItemFlags pqObjectInspector::flags(const QModelIndex &index) const
{
  // Add editable to the flags for the item values.
  Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  if(index.isValid() && index.model() == this && index.column() == 1)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        index.internalPointer());
    if(item && item->childCount() == 0 &&
       ((item->parent() == this->Internal->Parameters) ||
       (item->parent() && item->parent()->parent() == this->Internal->Parameters)))
      {
      flags |= Qt::ItemIsEditable;
      }
    }
  return flags;
}

QVariant pqObjectInspector::domain(const QModelIndex &index) const
{
  if(index.isValid() && index.model() == this)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        index.internalPointer());
    if(item)
      return item->domain();
    }

  return QVariant();
}

void pqObjectInspector::setProxy(vtkSMSourceProxy *proxy)
{
  // Clean up the current property data.
  this->cleanData();


  // Save the new proxy. Get all the property data.
  pqSMAdaptor *adapter = pqSMAdaptor::instance();
  this->Proxy = proxy;
  if(this->Internal && adapter && this->Proxy)
    {
    // update pipline information on the server
    proxy->UpdatePipelineInformation();

    pqObjectInspectorItem *item = 0;
    pqObjectInspectorItem *child = 0;
    vtkSMProperty *prop = 0;
    vtkSMPropertyIterator *iter = this->Proxy->NewPropertyIterator();
      
    this->Internal->Parameters = new pqObjectInspectorItem();
    this->Internal->Parameters->setPropertyName(tr("Parameters"));

    for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      // Skip the property if it doesn't have a valid value. This
      // is the case with the data information.
      prop = iter->GetProperty();

      // skip information-only properties
      if(prop->GetInformationOnly())
        continue;

      QVariant value = adapter->getProperty(prop);
      if(!value.isValid())
        continue;

      // Add the valid property to the list. If the property value
      // is a list, create child items. Link the values to the vtk
      // properties.
      item = new pqObjectInspectorItem();
      if(item)
        {
        item->setParent(this->Internal->Parameters);
        this->Internal->Parameters->addChild(item);

        item->setPropertyName(iter->GetKey());
        if(value.type() == QVariant::List)
          {
          QList<QVariant> list = value.toList();
          QList<QVariant>::Iterator li = list.begin();
          for(int index = 0; li != list.end(); ++li, ++index)
            {
            QString num;
            QVariant subvalue;
            if(li->type() == QVariant::List)
              {
              QList<QVariant> sublist = li->toList();
              num = sublist[0].toString();
              subvalue = sublist[1];
              }
            else
              {
              num.setNum(index + 1);
              subvalue = *li;
              }

            child = new pqObjectInspectorItem();
            if(child)
              {
              child->setPropertyName(num);
              child->setValue(subvalue);
              child->setParent(item);
              item->addChild(child);
              adapter->linkPropertyTo(this->Proxy, prop, index,
                  child, "value");
              connect(child, SIGNAL(valueChanged(pqObjectInspectorItem *)),
                  this, SLOT(handleValueChange(pqObjectInspectorItem *)));
              }
            }
          }
        else
          {
          item->setValue(value);
          adapter->linkPropertyTo(this->Proxy, prop, 0, item, "value");
          connect(item, SIGNAL(valueChanged(pqObjectInspectorItem *)),
              this, SLOT(handleValueChange(pqObjectInspectorItem *)));
          }

        // Set up the property domain information and link it to the server.
        // TODO  fix pqSMAdaptor to recognize QObject deletions
        /*
        item->updateDomain(prop);
        adapter->connectDomain(prop, item,
            SLOT(updateDomain(vtkSMProperty*)));
            */
        }
      }
    iter->Delete();

    this->Internal->Information = new pqObjectInspectorItem();
    this->Internal->Information->setPropertyName(tr("Information"));

    vtkPVDataInformation* dataInfo = this->Proxy->GetDataInformation();

    item = new pqObjectInspectorItem();
    item->setParent(this->Internal->Information);
    this->Internal->Information->addChild(item);
    item->setPropertyName("Type");
    int dataType = dataInfo->GetDataSetType();
    switch(dataType)
      {
      case VTK_POLY_DATA:
        item->setValue("Polygonal");
        break;
      case VTK_UNSTRUCTURED_GRID:
        item->setValue("Unstructured Grid");
        break;
      case VTK_STRUCTURED_GRID:
        item->setValue("Structured Grid");
        break;
      case VTK_RECTILINEAR_GRID:
        item->setValue("Nonuniform Rectilinear");
        break;
      case VTK_IMAGE_DATA:
          {
          int* ext = dataInfo->GetExtent();
          if(ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
            {
            item->setValue("Image (Uniform Rectilinear)");
            }
          else
            {
            item->setValue("Volume (Uniform Rectilinear)");
            }
          }
        break;
      case VTK_MULTIGROUP_DATA_SET:
        item->setValue("Multi-group");
        break;
      case VTK_MULTIBLOCK_DATA_SET:
        item->setValue("Multi-block");
        break;
      case VTK_HIERARCHICAL_DATA_SET:
        item->setValue("Hierarchical AMR");
        break;
      case VTK_HIERARCHICAL_BOX_DATA_SET:
        item->setValue("Hierarchical Uniform AMR");
        break;
      default:
        item->setValue("Unknown");
      }

    item = new pqObjectInspectorItem();
    item->setParent(this->Internal->Information);
    this->Internal->Information->addChild(item);
    item->setPropertyName("Number of cells");
    item->setValue(static_cast<qint64>(dataInfo->GetNumberOfCells()));

    item = new pqObjectInspectorItem();
    item->setParent(this->Internal->Information);
    this->Internal->Information->addChild(item);
    item->setPropertyName("Number of points");
    item->setValue(static_cast<qint64>(dataInfo->GetNumberOfPoints()));
    
    item = new pqObjectInspectorItem();
    item->setParent(this->Internal->Information);
    this->Internal->Information->addChild(item);
    item->setPropertyName("Memory");
    QString mem;
    mem.sprintf("%g MBytes", dataInfo->GetMemorySize()/1000.0);
    item->setValue(mem);


    }

  // Notify the view that the model has changed.
  this->reset();
}

void pqObjectInspector::setCommitType(pqObjectInspector::CommitType commit)
{
  this->Commit = commit;
}

void pqObjectInspector::commitChanges()
{
  if(!this->Proxy || !this->Internal)
    return;

  pqSMAdaptor *adapter = pqSMAdaptor::instance();
  if(!adapter)
    return;

  // Loop through all the properties and commit the modified values.
  int changeCount = 0;
  
  pqObjectInspectorItem *item = this->Internal->Parameters;
  pqObjectInspectorItem *child = 0;
  for(int i = 0; i < item->childCount(); i++)
    {
    child = item->child(i);
    if(child->isModified())
      {
      this->commitChange(child);
      changeCount++;
      }
    }
  
  // Update the server.
  if(changeCount > 0)
    {
    this->Proxy->UpdateVTKObjects();
    pqPipelineData *pipeline = pqPipelineData::instance();
    if(pipeline)
      {
      QVTKWidget *window = pipeline->getWindowFor(this->Proxy);
      if(window)
        window->update();
      }
    }
}

void pqObjectInspector::cleanData()
{
  if(this->Internal)
    {
    // Clean up the list of items.
    pqSMAdaptor *adapter = pqSMAdaptor::instance();
    if(this->Internal->Parameters)
      {
      for(int j=0; j<this->Internal->Parameters->childCount(); j++)
        {
        pqObjectInspectorItem *item = this->Internal->Parameters->child(j);
        if(item)
          {
          if(adapter && this->Proxy)
            {
            // If the item has a list, unlink each item from its property.
            // Otherwise, unlink the item from the vtk property.
            vtkSMProperty *property = this->Proxy->GetProperty(
                item->propertyName().toAscii().data());
            if(item->childCount() > 0)
              {
              for(int i = 0; i < item->childCount(); ++i)
                {
                pqObjectInspectorItem *child = item->child(i);
                adapter->unlinkPropertyFrom(this->Proxy, property, i, child,
                    "value");
                }
              }
            else
              {
              adapter->unlinkPropertyFrom(this->Proxy, property, 0, item,
                  "value");
              }
            }
          }
        }
      }
    this->Internal->clear();
    }
}

int pqObjectInspector::itemIndex(pqObjectInspectorItem *item) const
{
  if(item && this->Internal)
    {
    if(item->parent())
      return item->parent()->childIndex(item);
    else
      {
      for(int row = 0; row < this->Internal->size(); row++)
        {
        if((*this->Internal)[row] == item)
          return row;
        }
      }
    }

  return -1;
}

void pqObjectInspector::commitChange(pqObjectInspectorItem *item)
{
  if(!item)
    return;

  pqSMAdaptor *adapter = pqSMAdaptor::instance();
  if(!adapter)
    return;

  int index = 0;
  vtkSMProperty *property = 0;
  item->setModified(false);
  if(item->parent() && item->parent()->parent())
    {
    // Get the parent name.
    index = this->itemIndex(item);
    property = this->Proxy->GetProperty(
        item->parent()->propertyName().toAscii().data());
    adapter->setProperty(property, index, item->value());
    }
  else if(item->parent() && item->childCount() > 0)
    {
    // Set the value for each element of the property.
    pqObjectInspectorItem *child = 0;
    property = this->Proxy->GetProperty(
        item->propertyName().toAscii().data());
    for(index = 0; index < item->childCount(); index++)
      {
      child = item->child(index);
      adapter->setProperty(property, index, child->value());
      child->setModified(false);
      }
    }
  else
    {
    property = this->Proxy->GetProperty(
        item->propertyName().toAscii().data());
    adapter->setProperty(property, 0, item->value());
    }
}

void pqObjectInspector::handleNameChange(pqObjectInspectorItem *item)
{
  int row = this->itemIndex(item);
  if(row != -1)
    {
    QModelIndex index = this->createIndex(row, 0, item);
    emit dataChanged(index, index);
    }
}

void pqObjectInspector::handleValueChange(pqObjectInspectorItem *item)
{
  int row = this->itemIndex(item);
  if(row != -1)
    {
    QModelIndex index = this->createIndex(row, 1, item);
    emit dataChanged(index, index);
    }

  // update information
  // can this get called too many times to slow things down unecessarily???
  vtkPVDataInformation* dataInfo = this->Proxy->GetDataInformation();
  int i;

  int numCells = dataInfo->GetNumberOfCells();
  for(i=0; i<this->Internal->Information->childCount(); i++)
    {
    if(this->Internal->Information->child(i)->propertyName() == "Number of cells")
      item = this->Internal->Information->child(i);
    }
  if(numCells != item->value())
    {
    item->setValue(numCells);
    QModelIndex index = this->createIndex(this->itemIndex(item), 1, item);
    emit dataChanged(index, index);
    }
  
  int numPoints = dataInfo->GetNumberOfPoints();
  for(i=0; i<this->Internal->Information->childCount(); i++)
    {
    if(this->Internal->Information->child(i)->propertyName() == "Number of points")
      item = this->Internal->Information->child(i);
    }
  if(numPoints != item->value())
    {
    item->setValue(numPoints);
    QModelIndex index = this->createIndex(this->itemIndex(item), 1, item);
    emit dataChanged(index, index);
    }
  
  double memory = dataInfo->GetMemorySize() / 1000.0;
  for(i=0; i<this->Internal->Information->childCount(); i++)
    {
    if(this->Internal->Information->child(i)->propertyName() == "Memory")
      item = this->Internal->Information->child(i);
    }
  QString mem;
  mem.sprintf("%g MBytes", memory);
  if(mem != item->value())
    {
    item->setValue(mem);
    QModelIndex index = this->createIndex(this->itemIndex(item), 1, item);
    emit dataChanged(index, index);
    }
}


