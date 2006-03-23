/*=========================================================================

   Program:   ParaQ
   Module:    pqObjectInspector.cxx

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
          else if(item->value().value<pqSMProxy>())
            {
            pqSMProxy p = item->value().value<pqSMProxy>();
            pqPipelineObject* o = pqPipelineData::instance()->getObjectFor(p);
            if(o)
              {
              return pqPipelineData::instance()->getObjectFor(p)->GetProxyName();
              }
            else
              {
              return QString("No Name");
              }
            }
          else
            {
            return QVariant(item->value().toString());
            }
          }
        }
      case Qt::EditRole:
        {
        if(idx.column() == 0)
          return QVariant(item->propertyName());
        else if(item->value().value<pqSMProxy>())
          {
          pqSMProxy p = item->value().value<pqSMProxy>();
          QVariant v;
          v.setValue(p);
          return v;
          }
        else
          {
          return item->value();
          }
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
      if(localValue.type() == QVariant::UserType || localValue.convert(item->value().type()))
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
      QList<QList<QVariant> > extraCellArrays = adapter->getSelectionProperty(reader->GetProxy(), reader->GetProxy()->GetProperty("CellArrayStatus"));
      for(i=0; i<extraCellArrays.size(); i++)
        {
        QList<QVariant> cell = extraCellArrays[i];
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
      QList<QList<QVariant> > extraPointArrays = adapter->getSelectionProperty(reader->GetProxy(), reader->GetProxy()->GetProperty("PointArrayStatus"));
      for(i=0; i<extraPointArrays.size(); i++)
        {
        QList<QVariant> cell = extraPointArrays[i];
        if(cell[1] == false)
          {
          possibles.append(cell[0].toString() + " (point)");
          }
        }

      if(adapter->getElementProperty(display, display->GetProperty("ScalarVisibility")) == true)
        {
        QString fieldname = adapter->getElementProperty(display, display->GetProperty("ColorArray")).toString();
        QVariant fieldtype = adapter->getElementProperty(display, display->GetProperty("ScalarMode"));
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

      pqSMAdaptor::PropertyType pt = adapter->getPropertyType(prop);

      if(pt != pqSMAdaptor::UNKNOWN)
        {
        item = new pqObjectInspectorItem();
        item->setParent(this->Internal->Parameters);
        this->Internal->Parameters->addChild(item);
        item->setPropertyName(iter->GetKey());
        
        if(pt == pqSMAdaptor::PROXY)
          {
          pqSMProxy p = adapter->getProxyProperty(propertyProxy, prop);
          QVariant v;
          v.setValue(p);
          item->setValue(v);
          adapter->linkPropertyTo(propertyProxy, prop, 0, item, "value");
          connect(item, SIGNAL(valueChanged(pqObjectInspectorItem *)),
              this, SLOT(handleValueChange(pqObjectInspectorItem *)));
          }
        else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
          {
          QVariant value = adapter->getElementProperty(propertyProxy, prop);
          item->setValue(value);
          adapter->linkPropertyTo(propertyProxy, prop, 0, item, "value");
          connect(item, SIGNAL(valueChanged(pqObjectInspectorItem *)),
              this, SLOT(handleValueChange(pqObjectInspectorItem *)));
          }
        else if(pt == pqSMAdaptor::ENUMERATION)
          {
          QVariant value = adapter->getEnumerationProperty(propertyProxy, prop);
          item->setValue(value);
          adapter->linkPropertyTo(propertyProxy, prop, 0, item, "value");
          connect(item, SIGNAL(valueChanged(pqObjectInspectorItem *)),
              this, SLOT(handleValueChange(pqObjectInspectorItem *)));
          }
        else if(pt == pqSMAdaptor::SELECTION)
          {
          QList<QList<QVariant> > list = adapter->getSelectionProperty(propertyProxy, prop);
          QList<QList<QVariant> >::Iterator li = list.begin();
          for(int i = 0; li != list.end(); ++li, ++i)
            {
            QString num = (*li)[0].toString();
            QVariant subvalue = (*li)[1];
            child = new pqObjectInspectorItem();
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
        else if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
          {
          QList<QVariant> list = adapter->getMultipleElementProperty(propertyProxy, prop);
          QList<QVariant>::Iterator li = list.begin();
          for(int i = 0; li != list.end(); ++li, ++i)
            {
            QString num;
            num.setNum(i+1);
            QVariant subvalue = *li;
            child = new pqObjectInspectorItem();
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

        // Set up the property domain information and link it to the server.
        item->updateDomain(this->Proxy, prop);
        // TODO  fix pqSMAdaptor to recognize QObject deletions
        /*
        adapter->connectDomain(prop, item,
            SLOT(updateDomain(vtkSMProperty*)));
            */
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

      QVariant timeSteps = adapter->getMultipleElementProperty(dataInfoProxy, dataInfoProxy->GetProperty("TimestepValues"));
      if(timeSteps.isValid())
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
    pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(prop);
    if(pt == pqSMAdaptor::SELECTION)
      {
      QList<QList<QVariant> > selproperty;
      QList<QVariant> seldomain = adapter->getSelectionPropertyDomain(prop);
      QList<QVariant> selprop;
      selprop.push_back(seldomain[idx]);
      selprop.push_back(item->value());
      selproperty.push_back(selprop);
      adapter->setSelectionProperty(this->Proxy, prop, selproperty);
      }
    else if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
      {
      adapter->setMultipleElementProperty(this->Proxy, prop, idx, item->value());
      }
    else
      {
      assert("asdf" == NULL);
      }
    }
  else if(item->parent() && item->childCount() > 0)
    {
    // Set the value for each element of the property.
    pqObjectInspectorItem *child = 0;
    prop = this->Proxy->GetProperty(
        item->propertyName().toAscii().data());
    pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(prop);
    if(pt == pqSMAdaptor::SELECTION)
      {
      assert("asdf" == NULL);
      }
    else if(pt == pqSMAdaptor::MULTIPLE_ELEMENTS)
      {
      for(idx = 0; idx < item->childCount(); idx++)
        {
        child = item->child(idx);
        adapter->setMultipleElementProperty(this->Proxy, prop, idx, child->value());
        child->setModified(false);
        }
      }
    else
      {
      assert("asdf" == NULL);
      }
    }
  else
    {
    if(item->parent() == this->Internal->Display)
      {
      pqPipelineObject* pqObject = pqPipelineData::instance()->getObjectFor(this->Proxy);
      vtkSMDisplayProxy* display = pqObject->GetDisplayProxy();
      prop = display->GetProperty(item->propertyName().toAscii().data());
      }
    else
      {
      prop = this->Proxy->GetProperty(item->propertyName().toAscii().data());
      }
    pqSMAdaptor::PropertyType pt = pqSMAdaptor::getPropertyType(prop);
    if(pt == pqSMAdaptor::PROXY)
      {
      adapter->setProxyProperty(this->Proxy, prop, item->value().value<pqSMProxy>());
      }
    else if(pt == pqSMAdaptor::ENUMERATION)
      {
      adapter->setEnumerationProperty(this->Proxy, prop, item->value());
      }
    else if(pt == pqSMAdaptor::SINGLE_ELEMENT)
      {
      adapter->setElementProperty(this->Proxy, prop, item->value());
      }
    else if(pt == pqSMAdaptor::UNKNOWN)
      {
      }
    else
      {
      assert("asdf" == NULL);
      }
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


