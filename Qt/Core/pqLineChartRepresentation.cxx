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

#include "pqOutputPort.h"
#include "pqSMAdaptor.h"
#include "pqVTKLineChartSeries.h"

#include <QColor>
#include <QVector>
#include <QtDebug>

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkTable.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#define STATUS_ROW_LENGTH 11


//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
class pqLineChartDisplayItemList
{
public:
  pqLineChartDisplayItemList();
  ~pqLineChartDisplayItemList() {}

  void setXArray(vtkTable *data, bool useIndex,
      const QString &arrayName, int component);

  QVector<pqLineChartDisplayItem> Series;
  vtkSmartPointer<vtkDataArray> XArray;
  bool XChanged;
};


//-----------------------------------------------------------------------------
class pqLineChartRepresentation::pqInternals
{
public:
  pqInternals();
  ~pqInternals() {}

  void addLineItem(QList<QVariant> &values, const QString &arrayName,
      const QString &legendName, int enabled, int inLegend, double red,
      double green, double blue, int thickness, int lineStyle, int axesIndex,
      int component) const;
  void insertLineItem(QList<QVariant> &values, int index,
      const QString &arrayName, const QString &legendName, int enabled,
      int inLegend, double red, double green, double blue, int thickness,
      int lineStyle, int axesIndex, int component) const;

  vtkSmartPointer<vtkDoubleArray> YIndexArray;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkTimeStamp LastUpdateTime;
  vtkTimeStamp ModifiedTime;
  pqLineChartDisplayItemList List;
  int ChangeCount;
  bool InMultiChange;
  unsigned int LastCompositeIndex;
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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void pqLineChartDisplayItemList::setXArray(vtkTable *data,
    bool useIndex, const QString &arrayName, int component)
{
  if(!data)
    {
    this->XArray = 0;
    return;
    }

  if(useIndex)
    {
    vtkIdType numTuples = data->GetNumberOfRows();
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
    vtkDataSetAttributes *attributes = data->GetRowData();
    this->XArray = attributes ?
      attributes->GetArray(arrayName.toAscii().data()) : 0;
    // Handle the component value.
    if(this->XArray && this->XArray->GetNumberOfComponents() > 1)
      {
      this->XArray = pqVTKLineChartSeries::createArray(this->XArray, component);
      }
    }
}


//-----------------------------------------------------------------------------
pqLineChartRepresentation::pqInternals::pqInternals()
  : LastUpdateTime(), ModifiedTime(), List()
{
  this->YIndexArray = vtkSmartPointer<vtkDoubleArray>::New();
  this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->ChangeCount = 0;
  this->InMultiChange = false;
  this->LastCompositeIndex = 0;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::pqInternals::addLineItem(
    QList<QVariant> &values, const QString &arrayName,
    const QString &legendName, int enabled, int inLegend, double red,
    double green, double blue, int thickness, int lineStyle, int axesIndex,
    int component) const
{
  values.push_back(QVariant(arrayName));
  values.push_back(QVariant(legendName));
  values.push_back(QVariant(enabled));
  values.push_back(QVariant(inLegend));
  values.push_back(QVariant(red));
  values.push_back(QVariant(green));
  values.push_back(QVariant(blue));
  values.push_back(QVariant(thickness));
  values.push_back(QVariant(lineStyle));
  values.push_back(QVariant(axesIndex));
  values.push_back(QVariant(component));
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::pqInternals::insertLineItem(
    QList<QVariant> &values, int index, const QString &arrayName,
    const QString &legendName, int enabled, int inLegend, double red,
    double green, double blue, int thickness, int lineStyle, int axesIndex,
    int component) const
{
  values.insert(index, QVariant(arrayName));
  values.insert(index + 1, QVariant(legendName));
  values.insert(index + 2, QVariant(enabled));
  values.insert(index + 3, QVariant(inLegend));
  values.insert(index + 4, QVariant(red));
  values.insert(index + 5, QVariant(green));
  values.insert(index + 6, QVariant(blue));
  values.insert(index + 7, QVariant(thickness));
  values.insert(index + 8, QVariant(lineStyle));
  values.insert(index + 9, QVariant(axesIndex));
  values.insert(index + 10, QVariant(component));
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
      display->GetProperty("XArrayName"),
      vtkCommand::ModifiedEvent, this, SLOT(markListModified()));
  this->Internals->VTKConnect->Connect(
      display->GetProperty("UseYArrayIndex"),
      vtkCommand::ModifiedEvent, this, SLOT(markListModified()));
  this->Internals->VTKConnect->Connect(
      display->GetProperty("XArrayComponent"),
      vtkCommand::ModifiedEvent, this, SLOT(markListModified()));
}

//-----------------------------------------------------------------------------
pqLineChartRepresentation::~pqLineChartRepresentation()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();

  if (!this->isVisible())
    {
    // we don't worry about invisible display defaults.
    return;
    }

  vtkSMProxy* proxy = this->getProxy();
  proxy->GetProperty("ArrayInfo")->UpdateDependentDomains();

  // Set the x-axis array name defaults.
  QString arrayName;
  vtkSMProperty *cellName = proxy->GetProperty("XArrayName");
  if(this->getXArrayDefault(cellName, arrayName))
    {
    pqSMAdaptor::setElementProperty(cellName, arrayName);
    pqSMAdaptor::setElementProperty(
        proxy->GetProperty("UseYArrayIndex"), QVariant((int)0));
    }
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
vtkTable* pqLineChartRepresentation::getClientSideData() const
{
  vtkSMClientDeliveryRepresentationProxy* proxy = 
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(this->getProxy());
  if (proxy)
    {
    return vtkTable::SafeDownCast(proxy->GetOutput());
    }
  return 0;
}

//-----------------------------------------------------------------------------
bool pqLineChartRepresentation::isDataModified() const
{
  if (this->getCompositeDataSetIndex() != this->Internals->LastCompositeIndex)
    {
    return true;
    }

  vtkTable *data = this->getClientSideData();
  return data && data->GetMTime() > this->Internals->LastUpdateTime;
}

//-----------------------------------------------------------------------------
vtkDataArray* pqLineChartRepresentation::getArray(
    const QString &arrayName) const
{
  vtkTable *data = this->getClientSideData();
  vtkDataSetAttributes *attributes = data? data->GetRowData() : 0;
  return attributes? attributes->GetArray(arrayName.toAscii().data()) : 0;
}

//-----------------------------------------------------------------------------
vtkDataArray* pqLineChartRepresentation::getXArray() const
{
  return this->Internals->List.XArray;
}

//-----------------------------------------------------------------------------
vtkDataArray* pqLineChartRepresentation::getYArray(int index) const
{
  vtkSMProperty *status = this->getProxy()->GetProperty("YArrayStatus");
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(status);

  int actual_index = index * STATUS_ROW_LENGTH;
  if(actual_index >= values.size())
    {
    qDebug() << "Invalid y-array index: " << index;
    return 0;
    }

  return this->getArray(values[actual_index].toString());
}

//-----------------------------------------------------------------------------
vtkDataArray* pqLineChartRepresentation::getMaskArray()
{
  vtkTable* data = this->getClientSideData();
  return data? data->GetRowData()->GetArray("vtkValidPointMask") : 0;
}

//-----------------------------------------------------------------------------
bool pqLineChartRepresentation::isUpdateNeeded() const
{
  return (this->Internals->LastUpdateTime <= this->Internals->ModifiedTime) ||
      this->isDataModified();
}

//-----------------------------------------------------------------------------
bool pqLineChartRepresentation::isArrayUpdateNeeded(int /*attributeType*/) const
{
  bool updateNeeded = this->Internals->List.XChanged;
  return updateNeeded || this->isDataModified();
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::getAttributeType() const
{
  return pqSMAdaptor::getElementProperty(
      this->getProxy()->GetProperty("AttributeType")).toInt();
}

//-----------------------------------------------------------------------------
unsigned int pqLineChartRepresentation::getCompositeDataSetIndex() const
{
  return pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("CompositeDataSetIndex")).toUInt();
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::getNumberOfSeries() const
{
  return this->Internals->List.Series.size();
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::getSeriesIndex(const QString &name,
    int component) const
{
  QVector<pqLineChartDisplayItem>::ConstIterator iter =
      this->Internals->List.Series.begin();
  for(int i = 0; iter != this->Internals->List.Series.end(); ++iter, ++i)
    {
    if(name == iter->ArrayName && component == iter->Component)
      {
      return i;
      }
    }

  return -1;
}

//-----------------------------------------------------------------------------
bool pqLineChartRepresentation::isSeriesEnabled(int series) const
{
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    return this->Internals->List.Series[series].Enabled;
    }

  return false;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesEnabled(int series, bool enabled)
{
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List.Series[series];
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
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    name = this->Internals->List.Series[series].ArrayName;
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesName(int series, const QString &name)
{
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List.Series[series];
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
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    return this->Internals->List.Series[series].InLegend;
    }

  return false;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesInLegend(int series, bool inLegend)
{
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List.Series[series];
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
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    label = this->Internals->List.Series[series].LegendName;
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesLabel(int series,
    const QString &label)
{
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List.Series[series];
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
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    color = this->Internals->List.Series[series].Color;
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesColor(int series, const QColor &color)
{
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List.Series[series];
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
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    return this->Internals->List.Series[series].ColorSet;
    }

  return false;
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::getSeriesThickness(int series) const
{
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    return this->Internals->List.Series[series].Thickness;
    }

  return 0;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesThickness(int series, int thickness)
{
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List.Series[series];
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
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    return this->Internals->List.Series[series].Style;
    }

  return Qt::SolidLine;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesStyle(int series, Qt::PenStyle style)
{
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List.Series[series];
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
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    return this->Internals->List.Series[series].StyleSet;
    }

  return false;
}

//-----------------------------------------------------------------------------
int pqLineChartRepresentation::getSeriesAxesIndex(int series) const
{
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    return this->Internals->List.Series[series].AxesIndex;
    }

  return 0;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::setSeriesAxesIndex(int series, int index)
{
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    pqLineChartDisplayItem *item = &this->Internals->List.Series[series];
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
  if(series >= 0 && series < this->Internals->List.Series.size())
    {
    return this->Internals->List.Series[series].Component;
    }

  return -1;
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::addComponentLabel(QString &name,
    int component, int numComponents) const
{
  if(numComponents > 1)
    {
    name.append(": ");
    if(component == -2)
      {
      name.append("Distance");
      }
    else if(component == -1)
      {
      name.append("Magnitude");
      }
    else if(numComponents == 3)
      {
      if(component == 0)
        {
        name.append("X");
        }
      else if(component == 1)
        {
        name.append("Y");
        }
      else if(component == 2)
        {
        name.append("Z");
        }
      }
    else
      {
      name.append(QString::number(component));
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
  proxy->GetProperty("ArrayInfo")->UpdateDependentDomains();

  bool sendSignal = false;
  pqLineChartDisplayItemList *series = &this->Internals->List;

  // Update the x-axis array.
  if (force || this->isArrayUpdateNeeded(0))
    {
    bool useIndex = pqSMAdaptor::getElementProperty(
        proxy->GetProperty("UseYArrayIndex")).toInt() != 0;
    QString xArray = pqSMAdaptor::getElementProperty(
        proxy->GetProperty("XArrayName")).toString();
    int component = pqSMAdaptor::getElementProperty(
        proxy->GetProperty("XArrayComponent")).toInt();
    series->setXArray(this->getClientSideData(), useIndex, xArray, component);
    }

  vtkSMStringVectorProperty *smProperty = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty("YArrayStatus"));
  vtkSMArraySelectionDomain *arrayDomain = vtkSMArraySelectionDomain::SafeDownCast(
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
      // Make sure the name is unique.
      if(!statusNames.contains(rowName))
        {
        statusNames.append(rowName);
        }

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
  for(int k = 0; jter != arrayNames.end(); ++jter)
    {
    // Get the number of components for the array.
    QString current = *jter;
    QString label = current;
    vtkDataArray *array = this->getArray(current);
    int numComponents = array ? array->GetNumberOfComponents() : 0;

    bool inStatus = statusNames.contains(current);
    if(inStatus)
      {
      current = status[k].toString();
      }
    else
      {
      statusChanged = true;
      this->addComponentLabel(label, -1, numComponents);
      int arrayEnabled = this->isEnabledByDefault(current);
      this->Internals->insertLineItem(status, k, current, label,
          arrayEnabled, 1, -1.0, -1.0, -1.0, 1, Qt::NoPen, 0, -1);
      }

    // If the array has multiple components, add an item for each.
    k += STATUS_ROW_LENGTH;
    if(numComponents > 1)
      {
      int comp = 0;
      if(inStatus && k < status.size())
        {
        // See how many components are already added. More entries
        // may be needed.
        QString nextName = status[k].toString();
        while(nextName == current)
          {
          k += STATUS_ROW_LENGTH;
          comp++;
          if(k >= status.size())
            {
            break;
            }

          nextName = status[k].toString();
          }
        }

      for( ; comp < numComponents; comp++, k += STATUS_ROW_LENGTH)
        {
        statusChanged = true;
        label = current;
        this->addComponentLabel(label, comp, numComponents);
        this->Internals->insertLineItem(status, k, current, label,
            0, 1, -1.0, -1.0, -1.0, 1, Qt::NoPen, 0, comp);
        }
      }
    }

  // Save any changes to the status array.
  if(statusChanged)
    {
    smProperty->SetNumberOfElements(status.size());
    pqSMAdaptor::setMultipleElementProperty(smProperty, status);
    proxy->UpdateVTKObjects();
    }

  if(statusChanged || series->Series.size() != arrayNames.size())
    {
    if (!sendSignal && series == &this->Internals->List)
      {
      sendSignal = true;
      }

    // Build the series list using the status list. Clear the current
    // items and make space for the new items.
    series->Series.clear();
    series->Series.resize(status.size() / STATUS_ROW_LENGTH);

    // Initialize the item for each of the status rows.
    QVector<pqLineChartDisplayItem>::Iterator kter =
        series->Series.begin();
    for(int ii = 0; kter != series->Series.end();
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

  if(sendSignal)
    {
    emit this->seriesListChanged();
    }
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::finishSeriesUpdate()
{
  this->Internals->LastUpdateTime.Modified();
  this->Internals->List.XChanged = false;
  this->Internals->LastCompositeIndex = this->getCompositeDataSetIndex();
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
  emit this->seriesListChanged();
}

//-----------------------------------------------------------------------------
void pqLineChartRepresentation::markListModified()
{
  this->Internals->List.XChanged = true;
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
    vtkSMStringVectorProperty::SafeDownCast(
      proxy->GetProperty("YArrayStatus"));

  QList<QVariant> status;
  QVector<pqLineChartDisplayItem>::Iterator iter =
      this->Internals->List.Series.begin();
  for( ; iter != this->Internals->List.Series.end(); ++iter)
    {
    double red = -1.0;
    double green = -1.0;
    double blue = -1.0;
    if(iter->ColorSet)
      {
      red = iter->Color.redF();
      green = iter->Color.greenF();
      blue = iter->Color.blueF();
      }

    int lineStyle = Qt::NoPen;
    if(iter->StyleSet)
      {
      lineStyle = iter->Style;
      }

    this->Internals->addLineItem(status, iter->ArrayName, iter->LegendName,
        iter->Enabled ? 1 : 0, iter->InLegend ? 1 : 0, red, green, blue,
        iter->Thickness, lineStyle, iter->AxesIndex, iter->Component);
    }

  smProperty->SetNumberOfElements(status.size());
  pqSMAdaptor::setMultipleElementProperty(smProperty, status);
  proxy->UpdateVTKObjects();
}


