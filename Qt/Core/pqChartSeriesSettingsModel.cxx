/*=========================================================================

   Program: ParaView
   Module:    pqParallelCoordinatesSettingsModel.cxx

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
#include "pqParallelCoordinatesSettingsModel.h"

#include "pqDataRepresentation.h"
#include "vtkChartRepresentation.h"
#include "vtkSMParallelCoordinatesRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkWeakPointer.h"

#include <QPointer>
#include <QPixmap>

class pqParallelCoordinatesSettingsModel::pqImplementation
{
public:
  pqImplementation()
  {
  }

  vtkWeakPointer<vtkSMParallelCoordinatesRepresentationProxy> RepresentationProxy;
  QPointer<pqDataRepresentation> Representation;

  vtkChartRepresentation* GetVTKRepresentation()
    {
    return this->RepresentationProxy?
      this->RepresentationProxy->GetRepresentation() : NULL;
    }
};

pqParallelCoordinatesSettingsModel::pqParallelCoordinatesSettingsModel(QObject* parentObject) :
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

pqParallelCoordinatesSettingsModel::~pqParallelCoordinatesSettingsModel()
{
  delete this->Implementation;
}

void pqParallelCoordinatesSettingsModel::setRepresentation(pqDataRepresentation* rep)
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
    vtkSMParallelCoordinatesRepresentationProxy::SafeDownCast(rep->getProxy());
  this->Implementation->Representation = rep;
}

pqDataRepresentation* pqParallelCoordinatesSettingsModel::representation() const
{
  return this->Implementation->Representation;
}

int pqParallelCoordinatesSettingsModel::rowCount(const QModelIndex& /*parent*/) const
{
  return this->Implementation->GetVTKRepresentation()?
      this->Implementation->GetVTKRepresentation()->GetNumberOfSeries() : 0;
}

int pqParallelCoordinatesSettingsModel::columnCount(const QModelIndex& /*parent*/) const
{
  return 2;
}

QVariant pqParallelCoordinatesSettingsModel::data(const QModelIndex& idx, int role) const
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

  return QVariant();
}

QVariant pqParallelCoordinatesSettingsModel::headerData(int section,
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

bool pqParallelCoordinatesSettingsModel::setData(const QModelIndex &idx, const QVariant &value,
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
QModelIndex pqParallelCoordinatesSettingsModel::index(int row, int column,
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
QModelIndex pqParallelCoordinatesSettingsModel::parent(const QModelIndex &) const
{
  return QModelIndex();
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqParallelCoordinatesSettingsModel::flags(const QModelIndex &idx) const
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
void pqParallelCoordinatesSettingsModel::reload()
{
  this->reset();
  this->updateCheckState(0, Qt::Horizontal);
}

//-----------------------------------------------------------------------------
const char* pqParallelCoordinatesSettingsModel::getSeriesName(int row) const
{
  return this->Implementation->GetVTKRepresentation()->GetSeriesName(row);
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesSettingsModel::setSeriesEnabled(int row, bool enabled)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesVisibility").SetStatus(
      this->getSeriesName(row), enabled ? 1 : 0);
    this->Implementation->RepresentationProxy->UpdateVTKObjects();

    QModelIndex idx = this->createIndex(row, 0);
    emit this->dataChanged(idx, idx);
    emit this->redrawChart();
    this->updateCheckState(0, Qt::Horizontal);
    }
}

//-----------------------------------------------------------------------------
bool pqParallelCoordinatesSettingsModel::getSeriesEnabled(int row) const
{
  return vtkSMPropertyHelper(
      this->Implementation->RepresentationProxy, "SeriesVisibility")
      .GetStatus(this->getSeriesName(row), 1) != 0;
}

//-----------------------------------------------------------------------------
void pqParallelCoordinatesSettingsModel::setSeriesLabel(int row, const QString& label)
{
  if (row >= 0 && row < this->rowCount(QModelIndex()))
    {
    vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
      "SeriesLabel").SetStatus(
      this->getSeriesName(row), label.toAscii().data());
    this->Implementation->RepresentationProxy->UpdateVTKObjects();
    emit this->redrawChart();
    }
}

//-----------------------------------------------------------------------------
QString pqParallelCoordinatesSettingsModel::getSeriesLabel(int row) const
{
  QString name = this->getSeriesName(row);
  return vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
    "SeriesLabel").GetStatus(name.toStdString().c_str(),
    name.toStdString().c_str()); // name by default.
}
