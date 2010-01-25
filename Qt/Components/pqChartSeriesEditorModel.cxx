/*=========================================================================

   Program: ParaView
   Module:    pqChartSeriesEditorModel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqChartSeriesEditorModel.h"

#include "vtkQtChartRepresentation.h"
#include "vtkQtChartSeriesOptionsModel.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkSMChartRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"

#include <QPixmap>

#include "pqDataRepresentation.h"
//-----------------------------------------------------------------------------
pqChartSeriesEditorModel::pqChartSeriesEditorModel(QObject* parentObject)
: Superclass(parentObject)
{
  // Set up the column headers.
  this->insertHeaderSections(Qt::Horizontal, 0, 1);
  this->setCheckable(0, Qt::Horizontal, true);
  this->setCheckState(0, Qt::Horizontal, Qt::Unchecked);

  // Change the index check state when the header checkbox is clicked.
  this->connect(this, SIGNAL(headerDataChanged(Qt::Orientation, int, int)),
      this, SLOT(setIndexCheckState(Qt::Orientation, int, int)));
}

//-----------------------------------------------------------------------------
pqChartSeriesEditorModel::~pqChartSeriesEditorModel()
{

}

//-----------------------------------------------------------------------------
void pqChartSeriesEditorModel::setRepresentation(pqDataRepresentation *repr)
{
  if (!repr || repr == this->Representation)
    {
    return;
    }

  if (this->Representation)
    {
    QObject::disconnect(this->Representation, 0, this, 0);
    }

  vtkSMChartRepresentationProxy* chartRep =
    vtkSMChartRepresentationProxy::SafeDownCast(repr->getProxy());
  this->RepresentationProxy = chartRep;
  this->Representation = repr;

  vtkQtChartTableSeriesModel* dataModel =
    chartRep->GetVTKRepresentation()->GetSeriesModel();

  // Whenever the representation updates, we want to update the list of arrays
  // shown in the series browser.
  QObject::connect(dataModel, SIGNAL(modelReset()), this, SLOT(reload()));

  vtkQtChartSeriesOptionsModel *optionsModel = 
    chartRep->GetVTKRepresentation()->GetOptionsModel();

  QObject::connect(
    optionsModel, 
    SIGNAL(optionsChanged(vtkQtChartSeriesOptions*, int, const QVariant&, const QVariant&)),
    this, SLOT(optionsChanged(vtkQtChartSeriesOptions*)));
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqChartSeriesEditorModel::representation() const
{
  return this->Representation;
}

//-----------------------------------------------------------------------------
int pqChartSeriesEditorModel::rowCount(const QModelIndex &parentIndex) const
{
  if (!parentIndex.isValid() && this->RepresentationProxy)
    {
    return this->RepresentationProxy->GetNumberOfSeries();
    }
  return 0;
}

//-----------------------------------------------------------------------------
int pqChartSeriesEditorModel::columnCount(const QModelIndex &) const
{
  return 2;
}

//-----------------------------------------------------------------------------
bool pqChartSeriesEditorModel::hasChildren(const QModelIndex &parentIndex) const
{
  return (this->rowCount(parentIndex) > 0);
}

//-----------------------------------------------------------------------------
QModelIndex pqChartSeriesEditorModel::index(int row, int column,
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
QModelIndex pqChartSeriesEditorModel::parent(const QModelIndex &) const
{
  return QModelIndex();
}

//-----------------------------------------------------------------------------
QVariant pqChartSeriesEditorModel::data(const QModelIndex &idx, int role) const
{
  if (idx.isValid() && idx.model() == this)
    {
    if(role == Qt::DisplayRole || role == Qt::EditRole ||
        role == Qt::ToolTipRole)
      {
      if (idx.column() == 0)
        {
        QString arrayName = this->getSeriesName(idx.row());
        return QVariant(arrayName);
        }
      else if (idx.column() == 1)
        {
        QString legendName = this->getSeriesLabel(idx.row());
        return QVariant(legendName);
        }
      }
    else if (role == Qt::CheckStateRole)
      {
      if (idx.column() == 0)
        {
        return QVariant(this->getSeriesEnabled(idx.row())?
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
    }

  return QVariant();
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqChartSeriesEditorModel::flags(const QModelIndex &idx) const
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
bool pqChartSeriesEditorModel::setData(const QModelIndex &idx,
    const QVariant &value, int role)
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
QVariant pqChartSeriesEditorModel::headerData(int section,
    Qt::Orientation orient, int role) const
{
  if(orient == Qt::Horizontal && role == Qt::DisplayRole)
    {
    if(section == 0)
      {
      return QVariant(QString("Variable"));
      }
    else if(section == 1)
      {
      return QVariant(QString("Legend Name"));
      }
    }
  else
    {
    return this->Superclass::headerData(section, orient, role);
    }

  return QVariant();
}

//-----------------------------------------------------------------------------
void pqChartSeriesEditorModel::reload()
{
  this->reset();
  this->updateCheckState(0, Qt::Horizontal);
}

//-----------------------------------------------------------------------------
const char* pqChartSeriesEditorModel::getSeriesName(int row) const
{
  return this->RepresentationProxy->GetSeriesName(row);
}

//-----------------------------------------------------------------------------
void pqChartSeriesEditorModel::setSeriesEnabled(int row, bool enabled)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkSMPropertyHelper(this->RepresentationProxy,
      "SeriesVisibility").SetStatus(
      this->getSeriesName(row), enabled? 1 : 0);
    this->RepresentationProxy->UpdateVTKObjects();

    QModelIndex idx = this->createIndex(row, 0);
    emit this->dataChanged(idx, idx);
    this->updateCheckState(0, Qt::Horizontal);
    }
}

//-----------------------------------------------------------------------------
bool pqChartSeriesEditorModel::getSeriesEnabled(int row) const
{
  return vtkSMPropertyHelper(this->RepresentationProxy,
    "SeriesVisibility").GetStatus(
    this->getSeriesName(row), 1) != 0;
}

//-----------------------------------------------------------------------------
void pqChartSeriesEditorModel::setSeriesColor(int row, const QColor &color)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    double double_color[3];
    qreal qreal_color[3];
    color.getRgbF(qreal_color, qreal_color+1, qreal_color+2);
    double_color[0] = static_cast<double>(qreal_color[0]);
    double_color[1] = static_cast<double>(qreal_color[1]);
    double_color[2] = static_cast<double>(qreal_color[2]);
    vtkSMPropertyHelper(this->RepresentationProxy,
      "SeriesColor").SetStatus(
      this->getSeriesName(row), double_color, 3);
    this->RepresentationProxy->UpdateVTKObjects();

    QModelIndex idx = this->createIndex(row, 1);
    emit this->dataChanged(idx, idx);
    }
}

//-----------------------------------------------------------------------------
QColor pqChartSeriesEditorModel::getSeriesColor(int row) const
{
  double tmp[3] = {0.0, 1, 0};
  vtkSMPropertyHelper(this->RepresentationProxy,
    "SeriesColor").GetStatus(
    this->getSeriesName(row), tmp, 3);
  return QColor::fromRgbF(tmp[0], tmp[1], tmp[2]);
}


//-----------------------------------------------------------------------------
void pqChartSeriesEditorModel::setSeriesThickness(int row, int value)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkSMPropertyHelper(this->RepresentationProxy,
      "SeriesLineThickness").SetStatus(
      this->getSeriesName(row), value);
    this->RepresentationProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
int pqChartSeriesEditorModel::getSeriesThickness(int row) const
{
  return vtkSMPropertyHelper(this->RepresentationProxy,
    "SeriesLineThickness").GetStatus(
    this->getSeriesName(row), 1);
}

//-----------------------------------------------------------------------------
void pqChartSeriesEditorModel::setSeriesStyle(int row, int style)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkSMPropertyHelper(this->RepresentationProxy,
      "SeriesLineStyle").SetStatus(
      this->getSeriesName(row), style);
    this->RepresentationProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
int pqChartSeriesEditorModel::getSeriesStyle(int row) const
{
  return vtkSMPropertyHelper(this->RepresentationProxy,
    "SeriesLineStyle").GetStatus(
    this->getSeriesName(row), 1); // SOLID by default.
}

//-----------------------------------------------------------------------------
void pqChartSeriesEditorModel::setSeriesAxisCorner(int row, int axiscorner)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkSMPropertyHelper(this->RepresentationProxy,
      "SeriesAxisCorner").SetStatus(
      this->getSeriesName(row), axiscorner);
    this->RepresentationProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
int pqChartSeriesEditorModel::getSeriesAxisCorner(int row) const
{
  return vtkSMPropertyHelper(this->RepresentationProxy,
    "SeriesAxisCorner").GetStatus(
    this->getSeriesName(row), 0);
}

//-----------------------------------------------------------------------------
void pqChartSeriesEditorModel::setSeriesMarkerStyle(int row, int style)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkSMPropertyHelper(this->RepresentationProxy,
      "SeriesMarkerStyle").SetStatus(
      this->getSeriesName(row), style);
    this->RepresentationProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
int pqChartSeriesEditorModel::getSeriesMarkerStyle(int row) const
{
  return vtkSMPropertyHelper(this->RepresentationProxy,
    "SeriesMarkerStyle").GetStatus(
    this->getSeriesName(row), 0); // None by default.
}

//-----------------------------------------------------------------------------
void pqChartSeriesEditorModel::setSeriesLabel(int row, const QString& label)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkSMPropertyHelper(this->RepresentationProxy,
      "SeriesLabel").SetStatus(
      this->getSeriesName(row), label.toAscii().data());
    this->RepresentationProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
QString pqChartSeriesEditorModel::getSeriesLabel(int row) const
{
  QString name = this->getSeriesName(row);
  return vtkSMPropertyHelper(this->RepresentationProxy,
    "SeriesLabel").GetStatus(name.toStdString().c_str(),
    name.toStdString().c_str()); // name by default.
}

//-----------------------------------------------------------------------------
void pqChartSeriesEditorModel::optionsChanged(vtkQtChartSeriesOptions* options)
{
  vtkQtChartSeriesOptionsModel *optionsModel = 
    this->RepresentationProxy->GetVTKRepresentation()->GetOptionsModel();
  int row = optionsModel->getOptionsIndex(options);

  emit this->dataChanged(
    this->createIndex(row, 0), this->createIndex(row, 1));
}
