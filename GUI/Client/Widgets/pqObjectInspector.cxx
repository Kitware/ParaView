
/// \file pqObjectInspector.cxx
/// \brief
///   The pqObjectInspector class is a model for an object's properties.
///
/// \date 11/7/2005

#include "pqObjectInspector.h"

#include "pqObjectInspectorItem.h"
#include "pqPipelineData.h"
#include "pqPipelineObject.h"
#include "pqSMAdaptor.h"
#include "QVTKWidget.h"
#include <vtkSMProperty.h>
#include <vtkSMPropertyAdaptor.h>
#include <vtkSMPropertyIterator.h>
#include <vtkPVDataInformation.h>
#include <vtkSMDisplayProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMCompoundProxy.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkPVArrayInformation.h>
#include <vtkPVGeometryInformation.h>
#include "pqParts.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
class pqObjectInspectorInternal { public:
  pqObjectInspectorInternal()
    : Display(NULL), Parameters(NULL), Information(NULL) {}

  pqObjectInspectorItem* Display;
  pqObjectInspectorItem* Parameters;
  pqObjectInspectorItem* Information;

  int size() const
    {
    int count = 0;
    if(this->Display)
      {
      count++;
      }
    if(this->Parameters)
      {
      count++;
      }
    if(this->Information)
      {
      count++;
      }
    return count;
    }

  pqObjectInspectorItem* operator[](int i)
    {
    if(this->Display)
      {
      if(i == 0)
        {
        return this->Display;
        }
      }
    else
      {
      i++;
      }
    if(this->Parameters)
      {
      if(i == 1)
        {
        return this->Parameters;
        }
      }
    else
      {
      i++;
      }
    if(this->Information)
      {
      if(i == 2)
        {
        return this->Information;
        }
      }
    else
      {
      i++;
      }
    return NULL;
    }

  void clear()
    {
    if(this->Display)
      {
      delete this->Display;
      }
    this->Display = NULL;
    if(this->Parameters)
      {
      delete this->Parameters;
      }
    this->Parameters = NULL;
    if(this->Information)
      {
      delete this->Information;
      }
    this->Information = NULL;
    }

};


pqObjectInspector::pqObjectInspector(QObject *p)
  : QAbstractItemModel(p)
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

int pqObjectInspector::rowCount(const QModelIndex &p) const
{
  int rows = 0;
  if(this->Internal)
    {
    if(p.isValid() && p.model() == this)
      {
      pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
          p.internalPointer());
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

bool pqObjectInspector::hasChildren(const QModelIndex &p) const
{
  if(p.isValid() && p.model() == this)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        p.internalPointer());
    if(item && p.column() == 0)
      return item->childCount() > 0;
    }
  else if(this->Internal)
    return this->Internal->size() > 0;

  return false;
}

QModelIndex pqObjectInspector::index(int row, int column,
    const QModelIndex &p) const
{
  if(this->Internal && row >= 0 && column >= 0 && column < 2)
    {
    pqObjectInspectorItem *item = 0;
    if(p.isValid() && p.model() == this)
      {
      item = reinterpret_cast<pqObjectInspectorItem *>(
          p.internalPointer());
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

QModelIndex pqObjectInspector::parent(const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        idx.internalPointer());
    if(item && item->parent())
      {
      // Find the parent's row and column from the parent's parent.
      pqObjectInspectorItem *p = item->parent();
      int row = this->itemIndex(p);
      if(row != -1)
        return this->createIndex(row, 0, p);
      }
    }

  return QModelIndex();
}

QVariant pqObjectInspector::data(const QModelIndex &idx, int role) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        idx.internalPointer());
    if(!item)
      {
      return QVariant();
      }

    switch(role)
      {
      case Qt::DisplayRole:
      case Qt::ToolTipRole:
        {
        if(idx.column() == 0)
          return QVariant(item->propertyName());
        else
          {
          QVariant dom = item->domain();
          if(item->value().type() == QVariant::Int &&
              dom.type() == QVariant::StringList)
            {
            // Return the enum name instead of the index.
            QStringList names = dom.toStringList();
            return QVariant(names[item->value().toInt()]);
            }
          else
            return QVariant(item->value().toString());
          }
        }
      case Qt::EditRole:
        {
        if(idx.column() == 0)
          return QVariant(item->propertyName());
        else
          return item->value();
        }
      }
    }

  return QVariant();
}

bool pqObjectInspector::setData(const QModelIndex &idx,
    const QVariant &value, int)
{
  if(idx.isValid() && idx.model() == this && idx.column() == 1)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        idx.internalPointer());
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

Qt::ItemFlags pqObjectInspector::flags(const QModelIndex &idx) const
{
  // Add editable to the flags for the item values.
  Qt::ItemFlags itemFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  if(idx.isValid() && idx.model() == this && idx.column() == 1)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        idx.internalPointer());
    // TODO: consider putting editable flag on item?
    if(item && item->childCount() == 0 &&
       (
       (item->parent() == this->Internal->Parameters) ||
       (item->parent() && item->parent()->parent() == this->Internal->Parameters) ||
       (item->parent() == this->Internal->Display) ||
       (item->parent() && item->parent()->parent() == this->Internal->Display)
       ))
      {
      itemFlags |= Qt::ItemIsEditable;
      }
    }
  return itemFlags;
}

QVariant pqObjectInspector::domain(const QModelIndex &idx) const
{
  if(idx.isValid() && idx.model() == this)
    {
    pqObjectInspectorItem *item = reinterpret_cast<pqObjectInspectorItem *>(
        idx.internalPointer());
    if(item)
      return item->domain();
    }

  return QVariant();
}

void pqObjectInspector::setProxy(vtkSMProxy *sourceProxy)
{
  // Clean up the current property data.
  this->cleanData();


  // Save the new proxy. Get all the property data.
  pqSMAdaptor *adapter = pqSMAdaptor::instance();
  this->Proxy = sourceProxy;
  if(this->Internal && adapter && this->Proxy)
    {
      // update pipline information on the server
      {
      // scope so rest of code only uses vtkSMProxy interface
      vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(sourceProxy);
      if(sp)
        {
        sp->UpdatePipelineInformation();
        }
      else
        {
        vtkSMCompoundProxy* cp = vtkSMCompoundProxy::SafeDownCast(sourceProxy);
        if(cp)
          {
          for(unsigned int i=0; i<cp->GetNumberOfProxies(); i++)
            {
            vtkSMSourceProxy* src = vtkSMSourceProxy::SafeDownCast(cp->GetProxy(i));
            if(src)
              {
              src->UpdatePipelineInformation();
              }
            }
          }
        }
      }
    pqObjectInspectorItem *item = 0;
    pqObjectInspectorItem *child = 0;

    // ************* display properties ***************

    pqPipelineObject* pqObject = pqPipelineData::instance()->getObjectFor(sourceProxy);
    vtkSMDisplayProxy* display = pqObject->GetDisplayProxy();
    if(display)
      {
      this->Internal->Display = new pqObjectInspectorItem();
      this->Internal->Display->setPropertyName(tr("Display"));

      // visibility
      item = new pqObjectInspectorItem();
      item->setParent(this->Internal->Display);
      this->Internal->Display->addChild(item);
      item->setPropertyName("Visibility");
      item->setValue(static_cast<bool>(display->GetVisibilityCM()));
      QList<QVariant> possibles;
      possibles.append(true);
      possibles.append(false);
      item->setDomain(possibles);
      this->connect(item, SIGNAL(valueChanged(pqObjectInspectorItem*)), SLOT(updateDisplayProperties(pqObjectInspectorItem*)));

      possibles.clear();
      // color by
      item = new pqObjectInspectorItem();
      item->setParent(this->Internal->Display);
      this->Internal->Display->addChild(item);
      item->setPropertyName("Color by");

      // get the beginning of the pipeline
      pqPipelineObject* reader = pqObject;
      while(reader->GetInput(0))
        reader = reader->GetInput(0);
      
      possibles.append("Default");
      vtkPVDataInformation* geomInfo = display->GetGeometryInformation();
      vtkPVDataSetAttributesInformation* cellinfo = geomInfo->GetCellDataInformation();
      int i;
      for(i=0; i<cellinfo->GetNumberOfArrays(); i++)
        {
        vtkPVArrayInformation* info = cellinfo->GetArrayInformation(i);
        QString name = info->GetName();
        name += " (cell)";
        possibles.append(name);
        }
      // also include unloaded arrays if any
      QList<QVariant> extraCellArrays = adapter->getProperty(reader->GetProxy(), reader->GetProxy()->GetProperty("CellArrayStatus")).toList();
      for(i=0; i<extraCellArrays.size(); i++)
        {
        QList<QVariant> cell = extraCellArrays[i].toList();
        if(cell[1] == false)
          {
          possibles.append(cell[0].toString() + " (cell)");
          }
        }
      
      vtkPVDataSetAttributesInformation* pointinfo = geomInfo->GetPointDataInformation();
      for(i=0; i<pointinfo->GetNumberOfArrays(); i++)
        {
        vtkPVArrayInformation* info = pointinfo->GetArrayInformation(i);
        QString name = info->GetName();
        name += " (point)";
        possibles.append(name);
        }
      // also include unloaded arrays if any
      QList<QVariant> extraPointArrays = adapter->getProperty(reader->GetProxy(), reader->GetProxy()->GetProperty("PointArrayStatus")).toList();
      for(i=0; i<extraPointArrays.size(); i++)
        {
        QList<QVariant> cell = extraPointArrays[i].toList();
        if(cell[1] == false)
          {
          possibles.append(cell[0].toString() + " (point)");
          }
        }

      if(adapter->getProperty(display, display->GetProperty("ScalarVisibility")) == true)
        {
        QString fieldname = adapter->getProperty(display, display->GetProperty("ColorArray")).toString();
        QVariant fieldtype = adapter->getProperty(display, display->GetProperty("ScalarMode"));
        if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
          {
          item->setValue(QString(fieldname) + " (cell)");
          }
        else
          {
          item->setValue(QString(fieldname) + " (point)");
          }
        }
      else
        {
        item->setValue(possibles[0]);
        }
      item->setDomain(possibles);
      this->connect(item, SIGNAL(valueChanged(pqObjectInspectorItem*)), SLOT(updateDisplayProperties(pqObjectInspectorItem*)));
      }

    
    // ************* object properties *****************

    vtkSMProperty *prop = 0;
    vtkSMProxy* propertyProxy = this->Proxy;
    vtkSMCompoundProxy* compoundProxy = vtkSMCompoundProxy::SafeDownCast(this->Proxy);
    if(compoundProxy)
      {
      propertyProxy = compoundProxy->GetMainProxy();
      }

    vtkSMPropertyIterator *iter = propertyProxy->NewPropertyIterator();
      
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

      QVariant value = adapter->getProperty(propertyProxy, prop);
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
          for(int i = 0; li != list.end(); ++li, ++i)
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
              num.setNum(i + 1);
              subvalue = *li;
              }

            child = new pqObjectInspectorItem();
            if(child)
              {
              child->setPropertyName(num);
              child->setValue(subvalue);
              child->setParent(item);
              item->addChild(child);
              adapter->linkPropertyTo(propertyProxy, prop, i,
                  child, "value");
              connect(child, SIGNAL(valueChanged(pqObjectInspectorItem *)),
                  this, SLOT(handleValueChange(pqObjectInspectorItem *)));
              }
            }
          }
        else
          {
          item->setValue(value);
          adapter->linkPropertyTo(propertyProxy, prop, 0, item, "value");
          connect(item, SIGNAL(valueChanged(pqObjectInspectorItem *)),
              this, SLOT(handleValueChange(pqObjectInspectorItem *)));
          }

        // Set up the property domain information and link it to the server.
        item->updateDomain(prop);
        // TODO  fix pqSMAdaptor to recognize QObject deletions
        /*
        adapter->connectDomain(prop, item,
            SLOT(updateDomain(vtkSMProperty*)));
            */
        }
      }
    iter->Delete();

    // ************* information ***************

    vtkSMSourceProxy* dataInfoProxy = vtkSMSourceProxy::SafeDownCast(this->Proxy);
    if(dataInfoProxy)
      {
      this->Internal->Information = new pqObjectInspectorItem();
      this->Internal->Information->setPropertyName(tr("Information"));

      vtkPVDataInformation* dataInfo = dataInfoProxy->GetDataInformation();

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

      QVariant timeSteps = adapter->getProperty(dataInfoProxy, dataInfoProxy->GetProperty("TimestepValues"));
      if(timeSteps.type() == QVariant::List)
        {
        item = new pqObjectInspectorItem();
        item->setParent(this->Internal->Information);
        this->Internal->Information->addChild(item);
        item->setPropertyName("Number of timesteps");
        item->setValue(timeSteps.toList().size());
        }
      }
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
            vtkSMProperty *prop = this->Proxy->GetProperty(
                item->propertyName().toAscii().data());
            if(item->childCount() > 0)
              {
              for(int i = 0; i < item->childCount(); ++i)
                {
                pqObjectInspectorItem *child = item->child(i);
                adapter->unlinkPropertyFrom(this->Proxy, prop, i, child,
                    "value");
                }
              }
            else
              {
              adapter->unlinkPropertyFrom(this->Proxy, prop, 0, item,
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

  int idx = 0;
  vtkSMProperty *prop = 0;
  item->setModified(false);
  if(item->parent() && item->parent()->parent())
    {
    // Get the parent name.
    idx = this->itemIndex(item);
    prop = this->Proxy->GetProperty(
        item->parent()->propertyName().toAscii().data());
    adapter->setProperty(this->Proxy, prop, idx, item->value());
    }
  else if(item->parent() && item->childCount() > 0)
    {
    // Set the value for each element of the property.
    pqObjectInspectorItem *child = 0;
    prop = this->Proxy->GetProperty(
        item->propertyName().toAscii().data());
    for(idx = 0; idx < item->childCount(); idx++)
      {
      child = item->child(idx);
      adapter->setProperty(this->Proxy, prop, idx, child->value());
      child->setModified(false);
      }
    }
  else
    {
    prop = this->Proxy->GetProperty(
        item->propertyName().toAscii().data());
    adapter->setProperty(this->Proxy, prop, 0, item->value());
    }
}

void pqObjectInspector::handleNameChange(pqObjectInspectorItem *item)
{
  int row = this->itemIndex(item);
  if(row != -1)
    {
    QModelIndex idx = this->createIndex(row, 0, item);
    emit dataChanged(idx, idx);
    }
}

void pqObjectInspector::handleValueChange(pqObjectInspectorItem *item)
{
  int row = this->itemIndex(item);
  if(row != -1)
    {
    QModelIndex idx = this->createIndex(row, 1, item);
    emit dataChanged(idx, idx);
    }

  vtkSMSourceProxy* dataInfoProxy = vtkSMSourceProxy::SafeDownCast(this->Proxy);
  if(dataInfoProxy)
    { 
    // update information
    // can this get called too many times to slow things down unecessarily???
    vtkPVDataInformation* dataInfo = dataInfoProxy->GetDataInformation();
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
      QModelIndex idx = this->createIndex(this->itemIndex(item), 1, item);
      emit dataChanged(idx, idx);
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
      QModelIndex idx = this->createIndex(this->itemIndex(item), 1, item);
      emit dataChanged(idx, idx);
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
      QModelIndex idx = this->createIndex(this->itemIndex(item), 1, item);
      emit dataChanged(idx, idx);
      }
    }
}


void pqObjectInspector::updateDisplayProperties(pqObjectInspectorItem* item)
{
  // TODO: this code should probably be somewhere else
  vtkSMDisplayProxy* display = pqPipelineData::instance()->getObjectFor(this->Proxy)->GetDisplayProxy();

  if(item->propertyName().toAscii() == "Visibility")
    {
    display->SetVisibilityCM(item->value().toInt());
    display->UpdateVTKObjects();
    }
  if(item->propertyName().toAscii() == "Color by")
    {
    QString value = item->value().toString();
    if(value == "Default")
      {
      pqPart::Color(display);
      }
    else
      {
      bool cell = value.right(strlen("(cell)")) == "(cell)";
      if(cell)
        {
        value.chop(strlen(" (cell)"));
        pqPart::Color(display, value.toAscii().data(), vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
        }
      else
        {
        value.chop(strlen(" (point)"));
        pqPart::Color(display, value.toAscii().data(), vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
        }
      }
    }
}


