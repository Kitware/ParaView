/*=========================================================================

   Program: ParaView
   Module:    pqPlotSettingsModel.cxx

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
#include "pqPlotSettingsModel.h"

#include "vtkSMXYChartRepresentationProxy.h"
#include "pqDataRepresentation.h"
#include "vtkWeakPointer.h"
#include "vtkSMPropertyHelper.h"

#include <QPointer>

class pqPlotSettingsModel::pqImplementation
{
public:
  pqImplementation()
  {
  }

  vtkWeakPointer<vtkSMXYChartRepresentationProxy> RepresentationProxy;
  QPointer<pqDataRepresentation> Representation;
};

pqPlotSettingsModel::pqPlotSettingsModel(QObject* parentObject) :
  Superclass(parentObject), Implementation(new pqImplementation())
{
  // Set up the column headers.
  this->insertHeaderSections(Qt::Horizontal, 0, 1);
  this->setCheckable(0, Qt::Horizontal, true);
  this->setCheckState(0, Qt::Horizontal, Qt::Unchecked);
}

pqPlotSettingsModel::~pqPlotSettingsModel()
{
  delete this->Implementation;
}

void pqPlotSettingsModel::setRepresentation(pqDataRepresentation* rep)
{
  if (!rep || rep == this->Implementation->Representation)
    {
    return;
    }

  if (this->Implementation->Representation)
    {
    QObject::disconnect(this->Implementation->Representation, 0, this, 0);
    }

  this->Implementation->RepresentationProxy =
    vtkSMXYChartRepresentationProxy::SafeDownCast(rep->getProxy());
  this->Implementation->Representation = rep;
}

pqDataRepresentation* pqPlotSettingsModel::representation() const
{
  return this->Implementation->Representation;
}

int pqPlotSettingsModel::rowCount(const QModelIndex& /*parent*/) const
{
  return this->Implementation->RepresentationProxy->GetNumberOfSeries();
}

int pqPlotSettingsModel::columnCount(const QModelIndex& /*parent*/) const
{
  return 2;
}

QVariant pqPlotSettingsModel::data(const QModelIndex& idx, int role) const
{
  if(role == Qt::DisplayRole)
    {
    switch(idx.column())
      {
      case 0:
        return QString(this->getSeriesName(idx.row()));
      case 1:
        return this->getSeriesLabel(idx.row());
      }
    }
  else if (role == Qt::CheckStateRole)
    {
    if (idx.column() == 0)
      {
      return QVariant(this->getSeriesEnabled(idx.row()) ?
                      Qt::Checked : Qt::Unchecked);
      }
    }

  return QVariant();
}

QVariant pqPlotSettingsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
    switch(section)
      {
      case 0:
        return QString(tr("Variable"));
      case 1:
        return QString(tr("Legend Name"));
      }
    }

  return QVariant();
}

bool pqPlotSettingsModel::setData(const QModelIndex &idx, const QVariant &value,
                                  int role)
{
  bool result = false;
  if (idx.isValid() && idx.model() == this)
    {
    if (idx.column() == 1 && (role == Qt::DisplayRole || role == Qt::EditRole))
      {
      QString name = value.toString();
      if (!name.isEmpty())
        {
        this->setSeriesLabel(idx.row(), name);
        }
      }
    else if(idx.column() == 0 && role == Qt::CheckStateRole)
      {
      result = true;
      int checkstate = value.toInt();
      this->setSeriesEnabled(idx.row(), checkstate == Qt::Checked);
      }
    }
  return result;
}

//-----------------------------------------------------------------------------
QModelIndex pqPlotSettingsModel::index(int row, int column,
  const QModelIndex &parentIndex) const
{
  if(!parentIndex.isValid() && column >= 0 && column < 2 &&
    row >= 0 && row < this->rowCount(parentIndex))
    {
    return this->createIndex(row, column);
    }

  return QModelIndex();
}

//-----------------------------------------------------------------------------
QModelIndex pqPlotSettingsModel::parent(const QModelIndex &) const
{
  return QModelIndex();
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqPlotSettingsModel::flags(const QModelIndex &idx) const
{
  Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if(idx.isValid() && idx.model() == this)
    {
    if(idx.column() == 0)
      {
      result |= Qt::ItemIsUserCheckable;
      }
    else if(idx.column() == 1)
      {
      result |= Qt::ItemIsEditable;
      }
    }

  return result;
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::reload()
{
  this->reset();
  this->updateCheckState(0, Qt::Horizontal);
}

//-----------------------------------------------------------------------------
const char* pqPlotSettingsModel::getSeriesName(int row) const
{
  return this->Implementation->RepresentationProxy->GetSeriesName(row);
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesEnabled(int row, bool enabled)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesVisibility").SetStatus(
      this->getSeriesName(row), enabled ? 1 : 0);
    this->Implementation->RepresentationProxy->UpdateVTKObjects();

    QModelIndex idx = this->createIndex(row, 0);
    emit this->dataChanged(idx, idx);
    this->updateCheckState(0, Qt::Horizontal);
    }
}

//-----------------------------------------------------------------------------
bool pqPlotSettingsModel::getSeriesEnabled(int row) const
{
  return vtkSMPropertyHelper(
      this->Implementation->RepresentationProxy, "SeriesVisibility")
      .GetStatus(this->getSeriesName(row), 1) != 0;
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesLabel(int row, const QString& label)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesLabel").SetStatus(
      this->getSeriesName(row), label.toAscii().data());
    this->Implementation->RepresentationProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
QString pqPlotSettingsModel::getSeriesLabel(int row) const
{
  QString name = this->getSeriesName(row);
  return vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
    "SeriesLabel").GetStatus(name.toStdString().c_str(),
    name.toStdString().c_str()); // name by default.
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesColor(int row, const QColor &color)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    double double_color[3];
    qreal qreal_color[3];
    color.getRgbF(qreal_color, qreal_color+1, qreal_color+2);
    double_color[0] = static_cast<double>(qreal_color[0]);
    double_color[1] = static_cast<double>(qreal_color[1]);
    double_color[2] = static_cast<double>(qreal_color[2]);
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesColor").SetStatus(
      this->getSeriesName(row), double_color, 3);
    this->Implementation->RepresentationProxy->UpdateVTKObjects();

    QModelIndex idx = this->createIndex(row, 1);
    emit this->dataChanged(idx, idx);
    }
}

//-----------------------------------------------------------------------------
QColor pqPlotSettingsModel::getSeriesColor(int row) const
{
  double tmp[3] = {0.0, 1.0, 0.0};
  vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
    "SeriesColor").GetStatus(
    this->getSeriesName(row), tmp, 3);
  cout << "GetSeriesColor: " << tmp[0] << ", " << tmp[1] << ", " << tmp[2] << endl;
  return QColor::fromRgbF(tmp[0], tmp[1], tmp[2]);
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesThickness(int row, int value)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesLineThickness").SetStatus(
      this->getSeriesName(row), value);
    this->Implementation->RepresentationProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
int pqPlotSettingsModel::getSeriesThickness(int row) const
{
  return vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
    "SeriesLineThickness").GetStatus(this->getSeriesName(row), 1);
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesStyle(int, int)
{

}

//-----------------------------------------------------------------------------
int pqPlotSettingsModel::getSeriesStyle(int) const
{
  return 0;
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesAxisCorner(int, int)
{

}

//-----------------------------------------------------------------------------
int pqPlotSettingsModel::getSeriesAxisCorner(int) const
{
  return 0;
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesMarkerStyle(int, int)
{

}

//-----------------------------------------------------------------------------
int pqPlotSettingsModel::getSeriesMarkerStyle(int) const
{
  return 0;
}

