/*=========================================================================

   Program: ParaView
   Module:    pqLineChartRepresentation.cxx

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

=========================================================================*/
#include "pqLineChartRepresentation.h"

#include "pqSMAdaptor.h"
#include "pqVTKLineChartSeries.h"

#include <QColor>
#include <QVector>
#include <QtDebug>

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#define STATUS_ROW_LENGTH 11


class pqLineChartDisplayItem
{
public:
  pqLineChartDisplayItem();
  pqLineChartDisplayItem(const pqLineChartDisplayItem &other);
  ~pqLineChartDisplayItem () {}

  pqLineChartDisplayItem &operator=(const pqLineChartDisplayItem &other);

  QString ArrayName;
  QString LegendName;
  QColor Color;
  Qt::PenStyle Style;
  int Thickness;
  int AxesIndex;
  int Component;
  bool Enabled;
  bool InLegend;
  bool ColorSet;
  bool StyleSet;
};


class pqLineChartDisplayItemList
{
public:
  pqLineChartDisplayItemList();
  ~pqLineChartDisplayItemList() {}

  void setXArray(vtkRectilinearGrid *data, bool usePoints, bool useIndex,
      const QString &arrayName, int component);

  QVector<pqLineChartDisplayItem> Series;
  vtkSmartPointer<vtkDataArray> XArray;
  bool XChanged;
};


class pqLineChartRepresentation::pqInternals
{
public:
  pqInternals();
  ~pqInternals() {}

  vtkSmartPointer<vtkDoubleArray> YIndexArray;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkTimeStamp LastUpdateTime;
  vtkTimeStamp ModifiedTime;
  pqLineChartDisplayItemList PointList;
  pqLineChartDisplayItemList CellList;
  pqLineChartDisplayItemList *List;
  int ChangeCount;
  bool InMultiChange;
};


//-----------------------------------------------------------------------------
pqLineChartDisplayItem::pqLineChartDisplayItem()
  : ArrayName(), LegendName(), Color(255, 255, 255, 0)
{
  this->Style = Qt::SolidLine;
  this->Thickness = 0;
  this->AxesIndex = 0;
  this->Component = -1;
  this->Enabled = false;
  this->InLegend = true;
  this->ColorSet = false;
  this->StyleSet = false;
}

pqLineChartDisplayItem::pqLineChartDisplayItem(
    const pqLineChartDisplayItem &other)
  : ArrayName(other.ArrayName), LegendName(other.LegendName),
    Color(other.Color)
{
  this->Style = other.Style;
  this->Thickness = other.Thickness;
  this->AxesIndex = other.AxesIndex;
  this->Component = other.Component;
  this->Enabled = other.Enabled;
  this->InLegend = other.InLegend;
  this->ColorSet = other.ColorSet;
  this->StyleSet = other.StyleSet;
}

pqLineChartDisplayItem &pqLineChartDisplayItem::operator=(
    const pqLineChartDisplayItem &other)
{
  this->ArrayName = other.ArrayName;
  this->LegendName = other.LegendName;
  this->Color = other.Color;
  this->Style = other.Style;
  this->Thickness = other.Thickness;
  this->AxesIndex = other.AxesIndex;
  this->Component = other.Component;
  this->Enabled = other.Enabled;
  this->InLegend = other.InLegend;
  this->ColorSet = other.ColorSet;
  this->StyleSet = other.StyleSet;
  return *this;
}


//-----------------------------------------------------------------------------
pqLineChartDisplayItemList::pqLineChartDisplayItemList()
  : Series(), XArray(0)
{
  this->XChanged = true;
}

void pqLineChartDisplayItemList::setXArray(vtkRectilinearGrid *data,
    bool usePoints, bool useIndex, const QString &arrayName, int component)
{
  if(!data)
    {
    this->XArray = 0;
    return;
    }

  if(useIndex)
    {
    vtkIdType numTuples =
        usePoints ? data->GetNumberOfPoints() : data->GetNumberOfCells();
    this->XArray = vtkSmartPointer<vtkDoubleArray>::New();
    this->XArray->SetNumberOfComponents(1);
    this->XArray->SetNumberOfTuples(numTuples);
    for(vtkIdType i = 0; i < numTuples; i++)
      {
      this->XArray->SetTuple1(i, i);
      }
    }
  else
    {
    vtkDataSetAttributes *attributes = usePoints ?
        static_cast<vtkDataSetAttributes *>(data->GetPointData()) :
        static_cast<vtkDataSetAttributes *>(data->GetCellData());
    this->XArray =
        attributes ? attributes->GetArray(arrayName.toAscii().data()) : 0;

    // Handle the component value.
    if(this->XArray && this->XArray->GetNumberOfComponents() > 1)
      {
      this->XArray = pqVTKLineChartSeries::createArray(this->XArray, component);
      }
    }
}


//-----------------------------------------------------------------------------
pqLineChartRepresentation::pqInternals::pqInternals()
  : LastUpdateTime(), ModifiedTime(), PointList(), CellList()
{
  this->YIndexArray = vtkSmartPointer<vtkDoubleArray>::New();
  this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->List = &this->PointList;
  this->ChangeCount = 0;
  this->InMultiChange = false;
}


//-----------------------------------------------------------------------------
pqLineChartRepresentation::pqLineChartRepresentation(const QString& group,
    const QString& name, vtkSMProxy* display, pqServer* server,
    QObject* _parent)
  : Superclass(group, name, display, server, _parent)
{
  this->Internals = new pqInternals();

  // Listen to some property change events.
  this->Internals->VTKConnect->Connect(display->GetProperty("AttributeType"),
      vtkCommand::ModifiedEvent, this, SLOT(changeSeriesList()));
  this->Internals->VTKConnect->Connect(display,
      vtkCommand::PropertyModifiedEvent, this, SLOT(markAsModified()));
  this->Internals->VTKConnect->Connect(
      display->GetProperty("XPointArrayName"),
      vtkCommand::ModifiedEvent, this, SLOT(markPointModified()));
  this->Internals->VTKConnect->Connect(
      display->GetProperty("UseYPointArrayIndex"),
      vtkCommand::ModifiedEvent, this, SLOT(markPointModified()));
  this->Internals->VTKConnect->Connect(
      display->GetProperty("XPointArrayComponent"),
      vtkCommand::ModifiedEvent, this, SLOT(markPointModified()));
  this->Internals->VTKConnect->Connect(
      display->GetProperty("XCellArrayName"),
      vtkCommand::ModifiedEvent, this, SLOT(markCellModified()));
  this->Internals->VTKConnect->Connect(
      display->GetProperty("UseYCellArrayIndex"),
      vtkCommand::ModifiedEvent, this, SLOT(markCellModified()));
  this->Internals->VTKConnect->Connect(
      display->GetProperty("XCellArrayComponent"),
      vtkCommand::ModifiedEvent, this, SLOT(markCellModified()));
}

pqLineChartRepresentation::~pqLineChartRepresentation()
{
  delete this->Internals;
}

void pqLineChartRepresentation::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();

  if (!this->isVisible())
    {
    // we don't worry about invisible display defaults.
    return;
    }

  vtkSMProxy* proxy = this->getProxy();
  proxy->GetProperty("CellArrayInfo")->UpdateDependentDomains();
  proxy->GetProperty("PointArrayInfo")->UpdateDependentDomains();

  // Set the x-axis array name defaults.
  QString arrayName;
  vtkSMProperty *cellName = proxy->GetProperty("XCellArrayName");
  if(this->getXArrayDefault(cellName, arrayName))
    {
    pqSMAdaptor::setElementProperty(cellName, arrayName);
    pqSMAdaptor::setElementProperty(
        proxy->GetProperty("UseYCellArrayIndex"), QVariant((int)0));
    }

  vtkSMProperty *pointName = proxy->GetProperty("XPointArrayName");
  if(this->getXArrayDefault(pointName, arrayName))
    {
    pqSMAdaptor::setElementProperty(pointName, arrayName);
    pqSMAdaptor::setElementProperty(
        proxy->GetProperty("UseYPointArrayIndex"), QVariant((int)0));
    }

  // Set default values for cell/point array status properties.
  this->setStatusDefaults(proxy->GetProperty("YPointArrayStatus"));
  this->setStatusDefaults(proxy->GetProperty("YCellArrayStatus"));
  proxy->UpdateVTKObjects();
}

void pqLineChartRepresentation::setStatusDefaults(vtkSMProperty* prop)
{
  QList<QVariant> values;

  vtkSMArraySelectionDomain* asd = vtkSMArraySelectionDomain::SafeDownCast(
    prop->GetDomain("array_list"));
  unsigned int total_size = asd->GetNumberOfStrings();

  // set point variables.
  for (unsigned int cc=0; cc < total_size; cc++)
    {
    QString arrayName = asd->GetString(cc);
    values.push_back(arrayName);
    values.push_back(arrayName);
    values.push_back(QVariant(this->isEnabledByDefault(arrayName)));
    values.push_back(QVariant((int)1));
    values.push_back(QVariant((double)-1.0));
    values.push_back(QVariant((double)-1.0));
    values.push_back(QVariant((double)-1.0));
    values.push_back(QVariant((int)1));
    values.push_back(QVariant((int)Qt::NoPen));
    values.push_back(QVariant((int)0));
    values.push_back(QVariant((int)-1));
    }

  pqSMAdaptor::setMultipleElementProperty(prop, values);
}

bool pqLineChartRepresentation::getXArrayDefault(vtkSMProperty* prop,
    QString &arrayName)
{
  arrayName = QString();
  vtkSMArrayListDomain *arrayDomain =
      vtkSMArrayListDomain::SafeDownCast(prop->GetDomain("array_list"));
  unsigned int total = arrayDomain->GetNumberOfStrings();
  for(unsigned int i = 0; i < total; i++)
    {
    QString current = arrayDomain->GetString(i);
    if(current == "Time")
      {
      arrayName = current;
      return true;
      }
    else if(current == "original_coordinates")
      {
      arrayName = current;
      }
    }

  return !arrayName.isEmpty();
}

vtkRectilinearGrid* pqLineChartRepresentation::getClientSideData() const
{
  vtkSMClientDeliveryRepresentationProxy* proxy = 
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(this->getProxy());
  if (proxy)
    {
    return vtkRectilinearGrid::SafeDownCast(proxy->GetOutput());
    }
  return 0;
}

bool pqLineChartRepresentation::isDataModified() const
{
  vtkRectilinearGrid *data = this->getClientSideData();
  return data && data->GetMTime() > this->Internals->LastUpdateTime;
}

vtkDataArray* pqLineChartRepresentation::getArray(
    const QString &arrayName) const
{
  return this->getArray(arrayName, this->getAttributeType());
}

vtkDataArray* pqLineChartRepresentation::getArray(const QString &arrayName,
    int attributeType) const
{
  vtkRectilinearGrid *data = this->getClientSideData();
  vtkDataSetAttributes *attributes = 0;
  if(data)
    {
    if(attributeType == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
      attributes = static_cast<vtkDataSetAttributes *>(data->GetPointData());
      }
    else
      {
      attributes = static_cast<vtkDataSetAttributes *>(data->GetCellData());
      }
    }

  return attributes ? attributes->GetArray(arrayName.toAscii().data()) : 0;
}

vtkDataArray* pqLineChartRepresentation::getXArray() const
{
  return this->Internals->List->XArray;
}

vtkDataArray* pqLineChartRepresentation::getYArray(int index) const
{
  int attribute_type = this->getAttributeType();
  vtkSMProperty *status = this->getProxy()->GetProperty(
      attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS ?
      "YPointArrayStatus" : "YCellArrayStatus");
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(status);

  int actual_index = index * STATUS_ROW_LENGTH;
  if(actual_index >= values.size())
    {
    qDebug() << "Invalid y-array index: " << index;
    return 0;
    }

  return this->getArray(values[actual_index].toString(), attribute_type);
}

//-----------------------------------------------------------------------------
vtkDataArray* pqLineChartRepresentation::getMaskArray()
{
  vtkRectilinearGrid* data = this->getClientSideData();
  if (!data)
    {
    return 0;
    }

  return data->GetPointData()->GetArray("vtkValidPointMask");
}

//-----------------------------------------------------------------------------
bool pqLineChartRepresentation::isUpdateNeeded() const
{
  return (this->Internals->LastUpdateTime <= this->Internals->ModifiedTime) ||
      this->isDataModified();
}

//-----------------------------------------------------------------------------
bool pqLineChartRepresentation::isArrayUpdateNeeded(int attributeType) const
{
  bool updateNeeded = this->Internals->CellList.XChanged;
  if(attributeType == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
    updateNeeded = this->Internals->PointList.XChanged;
    }

  return updateNeeded || this->isDataModified();
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::getAttributeType() const
{
  return pqSMAdaptor::getElementProperty(
      this->getProxy()->GetProperty("AttributeType")).toInt();
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::getNumberOfSeries() const
{
  return this->Internals->List->Series.size();
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::getSeriesIndex(const QString &name) const
{
  QVector<pqLineChartDisplayItem>::ConstIterator iter =
      this->Internals->List->Series.begin();
  for(int i = 0; iter != this->Internals->List->Series.end(); ++iter, ++i)
    {
    if(name == iter->ArrayName)
      {
      return i;
      }
    }

  return -1;
}

//-----------------------------------------------------------------------------
bool pqLineChartRepresentation::isSeriesEnabled(int series) const
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    return this->Internals->List->Series[series].Enabled;
    }

  return false;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesEnabled(int series, bool enabled)
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List->Series[series];
    if(item->Enabled != enabled)
      {
      item->Enabled = enabled;
      this->Internals->ChangeCount++;
      if(!item->Enabled)
        {
        if(item->ColorSet)
          {
          item->ColorSet = false;
          item->Color = QColor(255, 255, 255, 0);
          emit this->colorChanged(series, item->Color);
          }

        if(item->StyleSet)
          {
          item->StyleSet = false;
          item->Style = Qt::SolidLine;
          emit this->styleChanged(series, item->Style);
          }
        }

      emit this->enabledStateChanged(series, item->Enabled);
      if(!this->Internals->InMultiChange)
        {
        this->saveSeriesChanges();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::getSeriesName(int series, QString &name) const
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    name = this->Internals->List->Series[series].ArrayName;
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesName(int series, const QString &name)
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List->Series[series];
    if(item->ArrayName != name)
      {
      item->ArrayName = name;
      this->Internals->ChangeCount++;
      if(!this->Internals->InMultiChange)
        {
        this->saveSeriesChanges();
        }
      }
    }
}

//-----------------------------------------------------------------------------
bool pqLineChartRepresentation::isSeriesInLegend(int series) const
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    return this->Internals->List->Series[series].InLegend;
    }

  return false;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesInLegend(int series, bool inLegend)
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List->Series[series];
    if(item->InLegend != inLegend)
      {
      item->InLegend = inLegend;
      this->Internals->ChangeCount++;
      emit this->legendStateChanged(series, item->InLegend);
      if(!this->Internals->InMultiChange)
        {
        this->saveSeriesChanges();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::getSeriesLabel(int series,
    QString &label) const
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    label = this->Internals->List->Series[series].LegendName;
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesLabel(int series,
    const QString &label)
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List->Series[series];
    if(item->LegendName != label)
      {
      item->LegendName = label;
      this->Internals->ChangeCount++;
      if(!this->Internals->InMultiChange)
        {
        this->saveSeriesChanges();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::getSeriesColor(int series, QColor &color) const
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    color = this->Internals->List->Series[series].Color;
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesColor(int series, const QColor &color)
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List->Series[series];
    if(!item->ColorSet || item->Color != color)
      {
      item->ColorSet = true;
      item->Color = color;
      this->Internals->ChangeCount++;
      emit this->colorChanged(series, color);
      if(!this->Internals->InMultiChange)
        {
        this->saveSeriesChanges();
        }
      }
    }
}

//-----------------------------------------------------------------------------
bool pqLineChartRepresentation::isSeriesColorSet(int series) const
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    return this->Internals->List->Series[series].ColorSet;
    }

  return false;
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::getSeriesThickness(int series) const
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    return this->Internals->List->Series[series].Thickness;
    }

  return 0;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesThickness(int series, int thickness)
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List->Series[series];
    if(item->Thickness != thickness)
      {
      item->Thickness = thickness;
      this->Internals->ChangeCount++;
      if(!this->Internals->InMultiChange)
        {
        this->saveSeriesChanges();
        }
      }
    }
}

//-----------------------------------------------------------------------------
Qt::PenStyle pqLineChartRepresentation::getSeriesStyle(int series) const
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    return this->Internals->List->Series[series].Style;
    }

  return Qt::SolidLine;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesStyle(int series, Qt::PenStyle style)
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List->Series[series];
    if(!item->StyleSet || item->Style != style)
      {
      item->StyleSet = true;
      item->Style = style;
      this->Internals->ChangeCount++;
      emit this->styleChanged(series, style);
      if(!this->Internals->InMultiChange)
        {
        this->saveSeriesChanges();
        }
      }
    }
}

//-----------------------------------------------------------------------------
bool pqLineChartRepresentation::isSeriesStyleSet(int series) const
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    return this->Internals->List->Series[series].StyleSet;
    }

  return false;
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::getSeriesAxesIndex(int series) const
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    return this->Internals->List->Series[series].AxesIndex;
    }

  return 0;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesAxesIndex(int series, int index)
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List->Series[series];
    if(item->AxesIndex != index)
      {
      item->AxesIndex = index;
      this->Internals->ChangeCount++;
      if(!this->Internals->InMultiChange)
        {
        this->saveSeriesChanges();
        }
      }
    }
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::getSeriesComponent(int series) const
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    return this->Internals->List->Series[series].Component;
    }

  return 0;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesComponent(int series, int component)
{
  if(series >= 0 && series < this->Internals->List->Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List->Series[series];
    if(item->Component != component)
      {
      item->Component = component;
      this->Internals->ChangeCount++;
      if(!this->Internals->InMultiChange)
        {
        this->saveSeriesChanges();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::beginSeriesChanges()
{
  this->Internals->InMultiChange = true;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::endSeriesChanges()
{
  if(this->Internals->InMultiChange)
    {
    this->Internals->InMultiChange = false;
    this->saveSeriesChanges();
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::startSeriesUpdate(bool force)
{
  // Update the domain information.
  vtkSMProxy *proxy = this->getProxy();
  proxy->GetProperty("Input")->UpdateDependentDomains();
  proxy->UpdatePropertyInformation();
  proxy->GetProperty("CellArrayInfo")->UpdateDependentDomains();
  proxy->GetProperty("PointArrayInfo")->UpdateDependentDomains();

  bool sendSignal = false;
  const char *status_name[] = {"YPointArrayStatus", "YCellArrayStatus"};
  const char *xarray_name[] = {"XPointArrayName", "XCellArrayName"};
  const char *index_name[] = {"UseYPointArrayIndex", "UseYCellArrayIndex"};
  const char *comp_name[] = {"XPointArrayComponent", "XCellArrayComponent"};
  pqLineChartDisplayItemList *series[] =
      {&this->Internals->PointList, &this->Internals->CellList};
  vtkSMStringVectorProperty *smProperty = 0;
  vtkSMArraySelectionDomain *arrayDomain = 0;
  for(int i = 0; i < 2; i++)
    {
    // Update the x-axis array.
    if(force || this->isArrayUpdateNeeded(i == 0 ?
        vtkDataObject::FIELD_ASSOCIATION_POINTS :
        vtkDataObject::FIELD_ASSOCIATION_CELLS))
      {
      bool useIndex = pqSMAdaptor::getElementProperty(
          proxy->GetProperty(index_name[i])).toInt() != 0;
      QString xArray = pqSMAdaptor::getElementProperty(
          proxy->GetProperty(xarray_name[i])).toString();
      int component = pqSMAdaptor::getElementProperty(
          proxy->GetProperty(comp_name[i])).toInt();
      series[i]->setXArray(this->getClientSideData(), i == 0,
          useIndex, xArray, component);
      }

    smProperty = vtkSMStringVectorProperty::SafeDownCast(
        proxy->GetProperty(status_name[i]));
    arrayDomain = vtkSMArraySelectionDomain::SafeDownCast(
        smProperty->GetDomain("array_list"));

    // Put the array names in a string list for convenience.
    QStringList arrayNames;
    int total = arrayDomain->GetNumberOfStrings();
    for(int j = 0; j < total; j++)
      {
      arrayNames.append(arrayDomain->GetString(j));
      }
    arrayNames.removeAll("vtkValidPointMask");
    arrayNames.removeAll("Cell's Point Ids");

    // Get the current status array.
    QList<QVariant> status =
        pqSMAdaptor::getMultipleElementProperty(smProperty);

    // Remove the array names not in the domain.
    bool statusChanged = false;
    QStringList statusNames;
    QList<QVariant>::Iterator iter = status.begin();
    while(iter != status.end())
      {
      QString rowName = iter->toString();
      if(arrayNames.contains(rowName))
        {
        statusNames.append(rowName);
        iter += STATUS_ROW_LENGTH;
        }
      else
        {
        statusChanged = true;
        iter = status.erase(iter, iter + STATUS_ROW_LENGTH);
        }
      }

    // Add status rows for the new array names. Make sure the status
    // array order matches the domain array order.
    QStringList::Iterator jter = arrayNames.begin();
    for(int k = 0; jter != arrayNames.end(); ++jter, k += STATUS_ROW_LENGTH)
      {
      if(!statusNames.contains(*jter))
        {
        statusChanged = true;
        int arrayEnabled = this->isEnabledByDefault(*jter);
        status.insert(k, *jter);
        status.insert(k + 1, *jter);
        status.insert(k + 2, QVariant(arrayEnabled));
        status.insert(k + 3, QVariant(arrayEnabled));
        status.insert(k + 4, QVariant((double)-1.0));
        status.insert(k + 5, QVariant((double)-1.0));
        status.insert(k + 6, QVariant((double)-1.0));
        status.insert(k + 7, QVariant((int)1));
        status.insert(k + 8, QVariant((int)Qt::NoPen));
        status.insert(k + 9, QVariant((int)0));
        status.insert(k + 10, QVariant((int)-1));
        }
      }

    // Save any changes to the status array.
    if(statusChanged)
      {
      smProperty->SetNumberOfElements(status.size());
      pqSMAdaptor::setMultipleElementProperty(smProperty, status);
      proxy->UpdateVTKObjects();
      }

    if(statusChanged || series[i]->Series.size() != arrayNames.size())
      {
      if(!sendSignal && series[i] == this->Internals->List)
        {
        sendSignal = true;
        }

      // Build the series list using the status list. Clear the current
      // items and make space for the new items.
      series[i]->Series.clear();
      series[i]->Series.resize(status.size() / STATUS_ROW_LENGTH);

      // Initialize the item for each of the status rows.
      QVector<pqLineChartDisplayItem>::Iterator kter =
          series[i]->Series.begin();
      for(int ii = 0; kter != series[i]->Series.end();
          ++kter, ii += STATUS_ROW_LENGTH)
        {
        kter->ArrayName = status[ii].toString();
        kter->LegendName = status[ii + 1].toString();
        kter->Enabled = status[ii + 2].toInt() != 0;
        kter->InLegend = status[ii + 3].toInt() != 0;
        double red = status[ii + 4].toDouble();
        kter->ColorSet = red >= 0.0;
        if(kter->ColorSet)
          {
          kter->Color.setRgbF(
              red, status[ii + 5].toDouble(), status[ii + 6].toDouble());
          }

        kter->Thickness = status[ii + 7].toInt();
        int style = status[ii + 8].toInt();
        kter->StyleSet = style > Qt::NoPen && style < Qt::CustomDashLine;
        if(kter->StyleSet)
          {
          kter->Style = (Qt::PenStyle)style;
          }

        kter->AxesIndex = status[ii + 9].toInt();
        kter->Component = status[ii + 10].toInt();
        }
      }
    }

  if(sendSignal)
    {
    emit this->seriesListChanged();
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::finishSeriesUpdate()
{
  this->Internals->LastUpdateTime.Modified();
  this->Internals->PointList.XChanged = false;
  this->Internals->CellList.XChanged = false;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setAttributeType(int attr)
{
  pqSMAdaptor::setElementProperty(
      this->getProxy()->GetProperty("AttributeType"), QVariant(attr));
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::markAsModified()
{
  this->Internals->ModifiedTime.Modified();
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::changeSeriesList()
{
  int attribute_type = pqSMAdaptor::getElementProperty(
      this->getProxy()->GetProperty("AttributeType")).toInt();
  pqLineChartDisplayItemList *newArray =
      attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS ?
      &this->Internals->PointList : &this->Internals->CellList;
  if(this->Internals->List != newArray)
    {
    this->Internals->List = newArray;
    emit this->seriesListChanged();
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::markPointModified()
{
  this->Internals->PointList.XChanged = true;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::markCellModified()
{
  this->Internals->CellList.XChanged = true;
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::isEnabledByDefault(const QString &arrayName) const
{
  if(arrayName == "ObjectId" ||
      arrayName.compare("Time", Qt::CaseInsensitive) == 0 ||
      arrayName.compare("TimeData", Qt::CaseInsensitive) == 0 ||
      arrayName == "GlobalElementId" || arrayName == "GlobalNodeId" ||
      arrayName == "GlobalEdgeId" || arrayName == "GlobalFaceId" || 
      arrayName == "PedigreeElementId" || arrayName == "PedigreeNodeId" ||
      arrayName == "PedigreeEdgeId" || arrayName == "PedigreeFaceId" ||
      arrayName == "vtkEAOTValidity" || arrayName == "Cell's Point Ids" ||
      arrayName == "original_coordinates" || arrayName == "arc_length")
    {
    return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::saveSeriesChanges()
{
  if(this->Internals->ChangeCount == 0)
    {
    return;
    }

  this->Internals->ChangeCount = 0;
  vtkSMProxy *proxy = this->getProxy();
  vtkSMStringVectorProperty *smProperty =
      vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty(
      this->Internals->List == &this->Internals->PointList ?
      "YPointArrayStatus" : "YCellArrayStatus"));

  QList<QVariant> status;
  QVector<pqLineChartDisplayItem>::Iterator iter =
      this->Internals->List->Series.begin();
  for( ; iter != this->Internals->List->Series.end(); ++iter)
    {
    status.push_back(QVariant(iter->ArrayName));
    status.push_back(QVariant(iter->LegendName));
    status.push_back(QVariant(iter->Enabled ? 1 : 0));
    status.push_back(QVariant(iter->InLegend ? 1 : 0));
    if(iter->ColorSet)
      {
      status.push_back(QVariant(iter->Color.redF()));
      status.push_back(QVariant(iter->Color.greenF()));
      status.push_back(QVariant(iter->Color.blueF()));
      }
    else
      {
      status.push_back(QVariant((double)-1.0));
      status.push_back(QVariant((double)-1.0));
      status.push_back(QVariant((double)-1.0));
      }

    status.push_back(QVariant(iter->Thickness));
    if(iter->StyleSet)
      {
      status.push_back(QVariant(iter->Style));
      }
    else
      {
      status.push_back(QVariant((int)Qt::NoPen));
      }

    status.push_back(QVariant(iter->AxesIndex));
    status.push_back(QVariant(iter->Component));
    }

  smProperty->SetNumberOfElements(status.size());
  pqSMAdaptor::setMultipleElementProperty(smProperty, status);
  proxy->UpdateVTKObjects();
}


