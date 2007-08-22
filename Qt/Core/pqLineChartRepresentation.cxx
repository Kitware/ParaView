/*=========================================================================

   Program: ParaView
   Module:    pqLineChartRepresentation.cxx

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

=========================================================================*/
#include "pqLineChartRepresentation.h"

#include "pqSMAdaptor.h"

#include <QColor>
#include <QVector>
#include <QtDebug>

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"

#define STATUS_ROW_LENGTH 10


class pqLineChartDisplayItem
{
public:
  pqLineChartDisplayItem();
  pqLineChartDisplayItem(const pqLineChartDisplayItem &other);
  ~pqLineChartDisplayItem () {}

  QString ArrayName;
  QString LegendName;
  QColor Color;
  Qt::PenStyle Style;
  int Thickness;
  int AxesIndex;
  bool Enabled;
  bool InLegend;
  bool ColorSet;
  bool StyleSet;
};


class pqLineChartRepresentation::pqInternals
{
public:
  pqInternals();
  ~pqInternals() {}

  vtkSmartPointer<vtkDoubleArray> YIndexArray;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QVector<pqLineChartDisplayItem> PointSeries;
  QVector<pqLineChartDisplayItem> CellSeries;
  QVector<pqLineChartDisplayItem> *Series;
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
  this->Enabled = other.Enabled;
  this->InLegend = other.InLegend;
  this->ColorSet = other.ColorSet;
  this->StyleSet = other.StyleSet;
}


//-----------------------------------------------------------------------------
pqLineChartRepresentation::pqInternals::pqInternals()
  : PointSeries(), CellSeries()
{
  this->YIndexArray = vtkSmartPointer<vtkDoubleArray>::New();
  this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Series = &this->PointSeries;
  this->ChangeCount = 0;
  this->InMultiChange = false;
}


//-----------------------------------------------------------------------------
pqLineChartRepresentation::pqLineChartRepresentation(const QString& group, const QString& name,
  vtkSMProxy* display, pqServer* server, QObject* _parent)
: Superclass(group, name, display, server, _parent)
{
  this->Internals = new pqInternals();

  // Listen to some property change events.
  this->Internals->VTKConnect->Connect(display->GetProperty("AttributeType"),
      vtkCommand::ModifiedEvent, this, SLOT(changeSeriesList()));
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
  proxy->GetProperty("CellArrayInfo")->UpdateDependentDomains();
  proxy->GetProperty("PointArrayInfo")->UpdateDependentDomains();

  // Set default values for cell/point array status properties.
  this->setStatusDefaults(proxy->GetProperty("YPointArrayStatus"));
  this->setStatusDefaults(proxy->GetProperty("YCellArrayStatus"));
  proxy->UpdateVTKObjects();
}
//-----------------------------------------------------------------------------
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
    int arrayEnabled = this->isEnabledByDefault(arrayName);
    values.push_back(arrayName);
    values.push_back(arrayName);
    values.push_back(QVariant(arrayEnabled));
    values.push_back(QVariant((int)1));
    values.push_back(QVariant((double)-1.0));
    values.push_back(QVariant((double)-1.0));
    values.push_back(QVariant((double)-1.0));
    values.push_back(QVariant((int)1));
    values.push_back(QVariant((int)Qt::NoPen));
    values.push_back(QVariant((int)0));
    }

  pqSMAdaptor::setMultipleElementProperty(prop, values);
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
vtkDataArray* pqLineChartRepresentation::getXArray()
{
  vtkRectilinearGrid* data = this->getClientSideData();
  if (!data)
    {
    return 0;
    }

  vtkSMProxy* proxy = this->getProxy();

  bool use_y_index = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("UseYArrayIndex")).toBool();
  int attribute_type = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("AttributeType")).toInt();

  if (use_y_index)
    {
    vtkIdType numTuples = 
      (attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS)? 
      data->GetNumberOfPoints() : data->GetNumberOfCells();
    if (this->Internals->YIndexArray->GetNumberOfTuples() != numTuples)
      {
      this->Internals->YIndexArray->SetNumberOfComponents(1);
      this->Internals->YIndexArray->SetNumberOfTuples(numTuples);
      for (vtkIdType cc=0; cc < this->Internals->YIndexArray->GetNumberOfTuples(); cc++)
        {
        this->Internals->YIndexArray->SetTuple1(cc, cc);
        }
      // FIXME: Need to mark the array modified so that it does not
      // use the cached ranges for this array anymore. This is probably
      // a bug in vtkDataArrayTemplate, need to look into that.
      this->Internals->YIndexArray->Modified();
      }
    return this->Internals->YIndexArray;
    }

  QString xarrayname = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("XArrayName")).toString();
  vtkDataSetAttributes* attributes =
    (attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS)?
    static_cast<vtkDataSetAttributes*>(data->GetPointData()) :
    static_cast<vtkDataSetAttributes*>(data->GetCellData());
  return (attributes)?  attributes->GetArray(xarrayname.toAscii().data()) : 0;
}

//-----------------------------------------------------------------------------
vtkDataArray* pqLineChartRepresentation::getYArray(int index)
{
  vtkRectilinearGrid* data = this->getClientSideData();
  if (!data)
    {
    return 0;
    }

  vtkSMProxy* proxy = this->getProxy();
  int attribute_type = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("AttributeType")).toInt();

  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    proxy->GetProperty(
      (attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS)?
      "YPointArrayStatus" : "YCellArrayStatus"));

  int actual_index = (index * STATUS_ROW_LENGTH);
  if (actual_index >= values.size())
    {
    qDebug() << "Invalid index: " << index;
    return 0;
    }

  QString yarrayname = values[actual_index].toString();

  vtkDataSetAttributes* attributes =
    (attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS)?
    static_cast<vtkDataSetAttributes*>(data->GetPointData()) :
    static_cast<vtkDataSetAttributes*>(data->GetCellData());
  return (attributes)? attributes->GetArray(
    yarrayname.toAscii().data()) : 0;
}

int pqLineChartRepresentation::getAttributeType() const
{
  return pqSMAdaptor::getElementProperty(
      this->getProxy()->GetProperty("AttributeType")).toInt();
}

int pqLineChartRepresentation::getNumberOfSeries() const
{
  return this->Internals->Series->size();
}

int pqLineChartRepresentation::getSeriesIndex(const QString &name) const
{
  QVector<pqLineChartDisplayItem>::ConstIterator iter =
      this->Internals->Series->begin();
  for(int i = 0; iter != this->Internals->Series->end(); ++iter, ++i)
    {
    if(name == iter->ArrayName)
      {
      return i;
      }
    }

  return -1;
}

bool pqLineChartRepresentation::isSeriesEnabled(int series) const
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    return this->Internals->Series->at(series).Enabled;
    }

  return false;
}

void pqLineChartRepresentation::setSeriesEnabled(int series, bool enabled)
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    pqLineChartDisplayItem *item = &(*this->Internals->Series)[series];
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

void pqLineChartRepresentation::getSeriesName(int series, QString &name) const
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    name = this->Internals->Series->at(series).ArrayName;
    }
}

void pqLineChartRepresentation::setSeriesName(int series, const QString &name)
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    pqLineChartDisplayItem *item = &(*this->Internals->Series)[series];
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

bool pqLineChartRepresentation::isSeriesInLegend(int series) const
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    return this->Internals->Series->at(series).InLegend;
    }

  return false;
}

void pqLineChartRepresentation::setSeriesInLegend(int series, bool inLegend)
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    pqLineChartDisplayItem *item = &(*this->Internals->Series)[series];
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

void pqLineChartRepresentation::getSeriesLabel(int series,
    QString &label) const
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    label = this->Internals->Series->at(series).LegendName;
    }
}

void pqLineChartRepresentation::setSeriesLabel(int series,
    const QString &label)
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    pqLineChartDisplayItem *item = &(*this->Internals->Series)[series];
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

void pqLineChartRepresentation::getSeriesColor(int series, QColor &color) const
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    color = this->Internals->Series->at(series).Color;
    }
}

void pqLineChartRepresentation::setSeriesColor(int series, const QColor &color)
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    pqLineChartDisplayItem *item = &(*this->Internals->Series)[series];
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

bool pqLineChartRepresentation::isSeriesColorSet(int series) const
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    return this->Internals->Series->at(series).ColorSet;
    }

  return false;
}

int pqLineChartRepresentation::getSeriesThickness(int series) const
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    return this->Internals->Series->at(series).Thickness;
    }

  return 0;
}

void pqLineChartRepresentation::setSeriesThickness(int series, int thickness)
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    pqLineChartDisplayItem *item = &(*this->Internals->Series)[series];
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

Qt::PenStyle pqLineChartRepresentation::getSeriesStyle(int series) const
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    return this->Internals->Series->at(series).Style;
    }

  return Qt::SolidLine;
}

void pqLineChartRepresentation::setSeriesStyle(int series, Qt::PenStyle style)
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    pqLineChartDisplayItem *item = &(*this->Internals->Series)[series];
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

bool pqLineChartRepresentation::isSeriesStyleSet(int series) const
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    return this->Internals->Series->at(series).StyleSet;
    }

  return false;
}

int pqLineChartRepresentation::getSeriesAxesIndex(int series) const
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    return this->Internals->Series->at(series).AxesIndex;
    }

  return 0;
}

void pqLineChartRepresentation::setSeriesAxesIndex(int series, int index)
{
  if(series >= 0 && series < this->Internals->Series->size())
    {
    pqLineChartDisplayItem *item = &(*this->Internals->Series)[series];
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

void pqLineChartRepresentation::beginSeriesChanges()
{
  this->Internals->InMultiChange = true;
}

void pqLineChartRepresentation::endSeriesChanges()
{
  if(this->Internals->InMultiChange)
    {
    this->Internals->InMultiChange = false;
    this->saveSeriesChanges();
    }
}

void pqLineChartRepresentation::updateSeries()
{
  // Update the domain information.
  vtkSMProxy *proxy = this->getProxy();
  proxy->GetProperty("Input")->UpdateDependentDomains();
  proxy->UpdatePropertyInformation();
  proxy->GetProperty("CellArrayInfo")->UpdateDependentDomains();
  proxy->GetProperty("PointArrayInfo")->UpdateDependentDomains();

  bool sendSignal = false;
  const char *status_name[] = {"YPointArrayStatus", "YCellArrayStatus"};
  QVector<pqLineChartDisplayItem> *series[] =
      {&this->Internals->PointSeries, &this->Internals->CellSeries};
  vtkSMStringVectorProperty *smProperty = 0;
  vtkSMArraySelectionDomain *arrayDomain = 0;
  for(int i = 0; i < 2; i++)
    {
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
        }
      }

    // Save any changes to the status array.
    if(statusChanged)
      {
      smProperty->SetNumberOfElements(status.size());
      pqSMAdaptor::setMultipleElementProperty(smProperty, status);
      proxy->UpdateVTKObjects();
      }

    if(statusChanged || series[i]->size() != arrayNames.size())
      {
      if(!sendSignal && series[i] == this->Internals->Series)
        {
        sendSignal = true;
        }

      // Build the series list using the status list. Clear the current
      // items and make space for the new items.
      series[i]->clear();
      series[i]->resize(status.size() / STATUS_ROW_LENGTH);

      // Initialize the item for each of the status rows.
      QVector<pqLineChartDisplayItem>::Iterator kter = series[i]->begin();
      for(int ii = 0; kter != series[i]->end(); ++kter, ii += STATUS_ROW_LENGTH)
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
        }
      }
    }

  if(sendSignal)
    {
    emit this->seriesListChanged();
    }
}

void pqLineChartRepresentation::setAttributeType(int attr)
{
  pqSMAdaptor::setElementProperty(
      this->getProxy()->GetProperty("AttributeType"), QVariant(attr));
}

void pqLineChartRepresentation::changeSeriesList()
{
  int attribute_type = pqSMAdaptor::getElementProperty(
      this->getProxy()->GetProperty("AttributeType")).toInt();
  QVector<pqLineChartDisplayItem> *newArray =
      attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS ?
      &this->Internals->PointSeries : &this->Internals->CellSeries;
  if(this->Internals->Series != newArray)
    {
    this->Internals->Series = newArray;
    emit this->seriesListChanged();
    }
}

int pqLineChartRepresentation::isEnabledByDefault(const QString &arrayName) const
{
  if(arrayName == "BlockId" || arrayName == "Time" ||
      arrayName == "GlobalElementId" || arrayName == "GlobalNodeId" ||
      arrayName == "PedigreeElementId" || arrayName == "PedigreeNodeId" ||
      arrayName == "vtkEAOTValidity" || arrayName == "Cell's Point Ids")
    {
    return 0;
    }

  return 1;
}

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
      this->Internals->Series == &this->Internals->PointSeries ?
      "YPointArrayStatus" : "YCellArrayStatus"));

  QList<QVariant> status;
  QVector<pqLineChartDisplayItem>::Iterator iter =
      this->Internals->Series->begin();
  for( ; iter != this->Internals->Series->end(); ++iter)
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
    }

  smProperty->SetNumberOfElements(status.size());
  pqSMAdaptor::setMultipleElementProperty(smProperty, status);
  proxy->UpdateVTKObjects();
}


