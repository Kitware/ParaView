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

#include "pqDataRepresentation.h"
#include "pqUndoStack.h"
#include "vtkChartRepresentation.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMChartRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkWeakPointer.h"

#include <QPointer>
#include <QPixmap>

class pqPlotSettingsModel::pqImplementation
{
public:
  pqImplementation()
    {
    this->Connection = vtkEventQtSlotConnect::New();
    }

  ~pqImplementation()
    {
    this->Connection->Delete();
    this->Connection = NULL;
    }

  vtkWeakPointer<vtkSMChartRepresentationProxy> RepresentationProxy;
  QPointer<pqDataRepresentation> Representation;
  vtkEventQtSlotConnect* Connection;

  vtkChartRepresentation* GetVTKRepresentation()
    {
    return this->RepresentationProxy?
      this->RepresentationProxy->GetRepresentation() : NULL;
    }
};

pqPlotSettingsModel::pqPlotSettingsModel(QObject* parentObject) :
  Superclass(parentObject), Implementation(new pqImplementation())
{
  // Set up the column headers.
  this->insertHeaderSections(Qt::Horizontal, 0, 1);
  this->setCheckable(0, Qt::Horizontal, true);
  this->setCheckState(0, Qt::Horizontal, Qt::Unchecked);

  // Change the index check state when the header checkbox is clicked.
  this->connect(this, SIGNAL(headerDataChanged(Qt::Orientation, int, int)),
      this, SLOT(setIndexCheckState(Qt::Orientation, int, int)));
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

  this->Implementation->Connection->Disconnect();
  if (this->Implementation->Representation)
    {
    QObject::disconnect(this->Implementation->Representation, 0, this, 0);
    }

  this->Implementation->RepresentationProxy =
    vtkSMChartRepresentationProxy::SafeDownCast(rep->getProxy());
  this->Implementation->Representation = rep;
  if (this->Implementation->RepresentationProxy)
    {
    this->Implementation->Connection->Connect(
      this->Implementation->RepresentationProxy,
      vtkCommand::PropertyModifiedEvent,
      this, SLOT(emitDataChanged()));
    }
}

pqDataRepresentation* pqPlotSettingsModel::representation() const
{
  return this->Implementation->Representation;
}

int pqPlotSettingsModel::rowCount(const QModelIndex& /*parent*/) const
{
  return this->Implementation->GetVTKRepresentation()?
      this->Implementation->GetVTKRepresentation()->GetNumberOfSeries() : 0;
}

int pqPlotSettingsModel::columnCount(const QModelIndex& /*parent*/) const
{
  return 2;
}

QVariant pqPlotSettingsModel::data(const QModelIndex& idx, int role) const
{
  if(role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole)
    {
    if (idx.column() == 0)
      {
      return QString(this->getSeriesName(idx.row()));
      }
    else if (idx.column() == 1)
      {
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
  else if (role == Qt::DecorationRole)
    {
    if (idx.column() == 1)
      {
      QPixmap pixmap(16, 16);
      pixmap.fill(this->getSeriesColor(idx.row()));
      return QVariant(pixmap);
      }
    }

  return QVariant();
}

QVariant pqPlotSettingsModel::headerData(int section,
                                         Qt::Orientation orientation,
                                         int role) const
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
  else
    {
    return this->Superclass::headerData(section, orientation, role);
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
void pqPlotSettingsModel::emitDataChanged()
{
  emit this->dataChanged(
    this->createIndex(0, 0),
    this->createIndex(this->rowCount(QModelIndex())-1, 0));
  this->updateCheckState(0, Qt::Horizontal);
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
  return this->Implementation->GetVTKRepresentation()->GetSeriesName(row);
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesEnabled(int row, bool enabled)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    BEGIN_UNDO_SET("Change Series Visibility");
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesVisibility").SetStatus(
      this->getSeriesName(row), enabled ? 1 : 0);
    this->Implementation->RepresentationProxy->UpdateVTKObjects();

    this->setSeriesColor(row, this->getSeriesColor(row));
    QModelIndex idx = this->createIndex(row, 0);
    emit this->dataChanged(idx, idx);
    emit this->redrawChart();
    emit this->rescaleChart();
    this->updateCheckState(0, Qt::Horizontal);
    END_UNDO_SET();
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
    BEGIN_UNDO_SET("Change Series Label");
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesLabel").SetStatus(
      this->getSeriesName(row), label.toAscii().data());
    this->Implementation->RepresentationProxy->UpdateVTKObjects();
    emit this->redrawChart();
    END_UNDO_SET();
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
    BEGIN_UNDO_SET("Change Series Color");
    double double_color[3];
    qreal qreal_color[3];
    color.getRgbF(qreal_color, qreal_color+1, qreal_color+2);
    double_color[0] = static_cast<double>(qreal_color[0]);
    double_color[1] = static_cast<double>(qreal_color[1]);
    double_color[2] = static_cast<double>(qreal_color[2]);
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesColor").SetStatus(this->getSeriesName(row), double_color, 3);
    this->Implementation->RepresentationProxy->UpdateVTKObjects();

    QModelIndex idx = this->createIndex(row, 1);
    emit this->dataChanged(idx, idx);
    emit this->redrawChart();
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
QColor pqPlotSettingsModel::getSeriesColor(int row) const
{
  double tmp[3] = {0.0, 1.0, 0.0};
  vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
    "SeriesColor").GetStatus(this->getSeriesName(row), tmp, 3);
  return QColor::fromRgbF(tmp[0], tmp[1], tmp[2]);
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesThickness(int row, int value)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    BEGIN_UNDO_SET("Change Series Line Thickness");
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesLineThickness").SetStatus(
      this->getSeriesName(row), value);
    this->Implementation->RepresentationProxy->UpdateVTKObjects();
    emit this->redrawChart();
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
int pqPlotSettingsModel::getSeriesThickness(int row) const
{
  return vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
    "SeriesLineThickness").GetStatus(this->getSeriesName(row), 1);
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesStyle(int row, int value)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    BEGIN_UNDO_SET("Change Series Line Style");
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesLineStyle").SetStatus(
      this->getSeriesName(row), value);
    this->Implementation->RepresentationProxy->UpdateVTKObjects();
    emit this->redrawChart();
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
int pqPlotSettingsModel::getSeriesStyle(int row) const
{
  return vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
    "SeriesLineStyle").GetStatus(this->getSeriesName(row), 1);
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesAxisCorner(int row, int value)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    BEGIN_UNDO_SET("Change Series Axes Placement");
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesPlotCorner").SetStatus(
      this->getSeriesName(row), value);
    this->Implementation->RepresentationProxy->UpdateVTKObjects();
    emit this->redrawChart();
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
int pqPlotSettingsModel::getSeriesAxisCorner(int row) const
{
  return vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
    "SeriesPlotCorner").GetStatus(this->getSeriesName(row), 1);
}

//-----------------------------------------------------------------------------
void pqPlotSettingsModel::setSeriesMarkerStyle(int row, int value)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    BEGIN_UNDO_SET("Change Series Marker Style");
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesMarkerStyle").SetStatus(
      this->getSeriesName(row), value);
    this->Implementation->RepresentationProxy->UpdateVTKObjects();
    emit this->redrawChart();
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
int pqPlotSettingsModel::getSeriesMarkerStyle(int row) const
{
  return vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
    "SeriesMarkerStyle").GetStatus(this->getSeriesName(row), 1);
}

