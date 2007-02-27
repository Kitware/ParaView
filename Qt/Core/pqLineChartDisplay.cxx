/*=========================================================================

   Program: ParaView
   Module:    pqLineChartDisplay.cxx

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
#include "pqLineChartDisplay.h"

#include "vtkDoubleArray.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSmartPointer.h"
#include "vtkSMGenericViewDisplayProxy.h"
#include "vtkSMProperty.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <QColor>
#include <QtDebug>

#include "pqSMAdaptor.h"

class pqLineChartDisplay::pqInternals
{
public:
  vtkSmartPointer<vtkDoubleArray> YIndexArray;
};
//-----------------------------------------------------------------------------
pqLineChartDisplay::pqLineChartDisplay(const QString& group, const QString& name,
  vtkSMProxy* display, pqServer* server, QObject* _parent)
: pqConsumerDisplay(group, name, display, server, _parent)
{
  this->Internals = new pqInternals();
  this->Internals->YIndexArray = vtkSmartPointer<vtkDoubleArray>::New();
}

//-----------------------------------------------------------------------------
pqLineChartDisplay::~pqLineChartDisplay()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqLineChartDisplay::setDefaults()
{
  this->Superclass::setDefaults();

  vtkSMProxy* proxy = this->getProxy();
  proxy->GetProperty("CellArrayInfo")->UpdateDependentDomains();
  proxy->GetProperty("PointArrayInfo")->UpdateDependentDomains();

  // Set default values for cell/point array status properties.
  this->setStatusDefaults(proxy->GetProperty("YPointArrayStatus"));
  this->setStatusDefaults(proxy->GetProperty("YCellArrayStatus"));
  proxy->UpdateVTKObjects();
}
//-----------------------------------------------------------------------------
void pqLineChartDisplay::setStatusDefaults(vtkSMProperty* prop)
{
  QList<QVariant> values;

  vtkSMArraySelectionDomain* asd = vtkSMArraySelectionDomain::SafeDownCast(
    prop->GetDomain("array_list"));


  unsigned int total_size = asd->GetNumberOfStrings();
  double hue_step = (total_size > 1)?  (1.0 / total_size) : 1.0;

  // set point variables.
  for (unsigned int cc=0; cc < total_size; cc++)
    {
    QColor qcolor;
    qcolor.setHsvF(cc*hue_step, 1.0, 1.0);
    values.push_back(QVariant(qcolor.redF()));
    values.push_back(QVariant(qcolor.greenF()));
    values.push_back(QVariant(qcolor.blueF()));
    values.push_back(QVariant(1));
    values.push_back(asd->GetString(cc));
    }

  pqSMAdaptor::setMultipleElementProperty(prop, values);
}

//-----------------------------------------------------------------------------
vtkRectilinearGrid* pqLineChartDisplay::getClientSideData() const
{
  vtkSMGenericViewDisplayProxy* proxy = 
    vtkSMGenericViewDisplayProxy::SafeDownCast(this->getProxy());
  if (proxy)
    {
    return vtkRectilinearGrid::SafeDownCast(proxy->GetOutput());
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkDataArray* pqLineChartDisplay::getXArray()
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
int pqLineChartDisplay::getNumberOfYArrays() const
{
  vtkSMProxy* proxy = this->getProxy();
  int attribute_type = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("AttributeType")).toInt();

  vtkSMProperty* prop = proxy->GetProperty(
    (attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS)?
    "YPointArrayStatus" : "YCellArrayStatus");
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(prop);
  return (values.size()/5);
}

//-----------------------------------------------------------------------------
bool pqLineChartDisplay::getYArrayEnabled(int index) const
{
  vtkSMProxy* proxy = this->getProxy();
  int attribute_type = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("AttributeType")).toInt();

  vtkSMProperty* prop = proxy->GetProperty(
    (attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS)?
    "YPointArrayStatus" : "YCellArrayStatus");

  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(prop);
  int actual_index = (index*5 + 3);
  if (actual_index >= values.size())
    {
    qDebug() << "Invalid index: " << index;
    return 0;
    }
  return values[actual_index].toBool();
}

//-----------------------------------------------------------------------------
QColor pqLineChartDisplay::getYColor(int index) const
{
  vtkSMProxy* proxy = this->getProxy();
  int attribute_type = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("AttributeType")).toInt();

  vtkSMProperty* prop = proxy->GetProperty(
    (attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS)?
    "YPointArrayStatus" : "YCellArrayStatus");

  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(prop);
  if ((index*5 + 4) >= values.size())
    {
    qDebug() << "Invalid index: " << index;
    return QColor();
    }
  QColor color;
  color.setRedF(values[5*index+0].toDouble());
  color.setGreenF(values[5*index+1].toDouble());
  color.setBlueF(values[5*index+2].toDouble());
  return color; 
}

//-----------------------------------------------------------------------------
vtkDataArray* pqLineChartDisplay::getYArray(int index)
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

  int actual_index = (index*5 + 4);
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

//-----------------------------------------------------------------------------
bool pqLineChartDisplay::getYArrayEnabled(const QString& arrayname) const
{
  vtkSMProxy* proxy = this->getProxy();
  int attribute_type = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("AttributeType")).toInt();

  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    proxy->GetProperty(
      (attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS)?
      "YPointArrayStatus" : "YCellArrayStatus"));

  for (int cc=0; cc+4 < values.size(); cc++)
    {
    if (values[cc+4].toString() == arrayname)
      {
      return values[cc+3].toBool();
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
QColor pqLineChartDisplay::getYColor(const QString& arrayname) const
{
  vtkSMProxy* proxy = this->getProxy();
  int attribute_type = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("AttributeType")).toInt();

  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    proxy->GetProperty(
      (attribute_type == vtkDataObject::FIELD_ASSOCIATION_POINTS)?
      "YPointArrayStatus" : "YCellArrayStatus"));

  for (int cc=0; cc+4 < values.size(); cc++)
    {
    if (values[cc+4].toString() == arrayname)
      {
      QColor color;
      color.setRedF(values[cc+0].toDouble());
      color.setGreenF(values[cc+1].toDouble());
      color.setBlueF(values[cc+2].toDouble());
      return color;
      }
    }

  return QColor(100,100,100);
}
