/*=========================================================================

   Program: ParaView
   Module:    pqChartSeriesSettingsModel.cxx

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
#include "pqChartSeriesSettingsModel.h"

#include "pqDataRepresentation.h"
#include "vtkChartRepresentation.h"
#include "vtkPVPlotMatrixRepresentation.h"
#include "vtkSMChartRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkWeakPointer.h"

#include <QMimeData>
#include <QPointer>
#include <QPixmap>

class pqChartSeriesSettingsModel::pqImplementation
{
public:
  pqImplementation()
  {
  }

  vtkWeakPointer<vtkSMChartRepresentationProxy> RepresentationProxy;
  QPointer<pqDataRepresentation> Representation;
  QModelIndex rootIndex;

  vtkChartRepresentation* GetVTKRepresentation()
    {
    return this->RepresentationProxy?
      this->RepresentationProxy->GetRepresentation() : NULL;
    }
};

pqChartSeriesSettingsModel::pqChartSeriesSettingsModel(QObject* parentObject) :
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

pqChartSeriesSettingsModel::~pqChartSeriesSettingsModel()
{
  delete this->Implementation;
}

void pqChartSeriesSettingsModel::setRepresentation(pqDataRepresentation* rep)
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
    vtkSMChartRepresentationProxy::SafeDownCast(rep->getProxy());
  this->Implementation->Representation = rep;
}

pqDataRepresentation* pqChartSeriesSettingsModel::representation() const
{
  return this->Implementation->Representation;
}

int pqChartSeriesSettingsModel::rowCount(const QModelIndex& /*parent*/) const
{
  return this->Implementation->GetVTKRepresentation()?
      this->Implementation->GetVTKRepresentation()->GetNumberOfSeries() : 0;
}

int pqChartSeriesSettingsModel::columnCount(const QModelIndex& /*parent*/) const
{
  return 2;
}

QVariant pqChartSeriesSettingsModel::data(const QModelIndex& idx, int role) const
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

QVariant pqChartSeriesSettingsModel::headerData(int section,
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

bool pqChartSeriesSettingsModel::setData(const QModelIndex &idx, const QVariant &value,
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
QModelIndex pqChartSeriesSettingsModel::index(int row, int column,
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
QModelIndex pqChartSeriesSettingsModel::parent(const QModelIndex &) const
{
  return this->rootIndex();
}

//-----------------------------------------------------------------------------
QModelIndex pqChartSeriesSettingsModel::rootIndex() const
{
  return this->Implementation->rootIndex;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqChartSeriesSettingsModel::flags(const QModelIndex &idx) const
{
  Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable
    | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
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
void pqChartSeriesSettingsModel::reload()
{
  this->reset();
  this->updateCheckState(0, Qt::Horizontal);
}

//-----------------------------------------------------------------------------
const char* pqChartSeriesSettingsModel::getSeriesName(int row) const
{
  return this->Implementation->GetVTKRepresentation()->GetSeriesName(row);
}

//-----------------------------------------------------------------------------
void pqChartSeriesSettingsModel::setSeriesEnabled(int row, bool enabled)
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
bool pqChartSeriesSettingsModel::getSeriesEnabled(int row) const
{
  return vtkSMPropertyHelper(
      this->Implementation->RepresentationProxy, "SeriesVisibility")
      .GetStatus(this->getSeriesName(row), 1) != 0;
}

//-----------------------------------------------------------------------------
void pqChartSeriesSettingsModel::setSeriesLabel(int row, const QString& label)
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
QString pqChartSeriesSettingsModel::getSeriesLabel(int row) const
{
  QString name = this->getSeriesName(row);
  return vtkSMPropertyHelper(this->Implementation->RepresentationProxy,
    "SeriesLabel").GetStatus(name.toStdString().c_str(),
    name.toStdString().c_str()); // name by default.
}

//-----------------------------------------------------------------------------
Qt::DropActions pqChartSeriesSettingsModel::supportedDropActions () const
{
  // returns what actions are supported when dropping
  return Qt::CopyAction | Qt::MoveAction;
}

//-----------------------------------------------------------------------------
bool pqChartSeriesSettingsModel::dropMimeData(const QMimeData *mData, Qt::DropAction action,
                                      int row, int column, const QModelIndex &onIndex)
{
  Q_UNUSED(row);
  Q_UNUSED(column);
  if (!mData || action != Qt::MoveAction)
      return false;

  QStringList types = mimeTypes();
  if (types.isEmpty())
      return false;
  QString format = types.at(0);
  if (!mData->hasFormat(format))
      return false;
  vtkPVPlotMatrixRepresentation* plotMatrixRep = vtkPVPlotMatrixRepresentation::SafeDownCast(
    this->Implementation->GetVTKRepresentation());
  if(!plotMatrixRep)
    {
    return false;
    }
  this->blockSignals(true);

  QByteArray encoded = mData->data(format);
  QDataStream stream(&encoded, QIODevice::ReadOnly);

  QList<int> rows;
  while (!stream.atEnd()) {
    int r, c;
    QMap<int, QVariant> v;
    stream >> r >> c >> v;
    if(!rows.contains(r))
      {
      rows.append(r);
      }
    }

  if(rows.count() ==0)
    {
    return false;
    }

  // if the drop is on an item, insert the dropping items
  // before the dropped-on item; else, we will just move
  // them to the end.
  int toRow = onIndex.isValid() ? onIndex.row() : rowCount();
  plotMatrixRep->MoveInputTableColumn(rows.value(0), toRow);
  this->blockSignals(false);
  this->emitDataChanged();
  return true;
}

//-----------------------------------------------------------------------------
void pqChartSeriesSettingsModel::emitDataChanged()
{
  emit this->dataChanged(
    this->createIndex(0, 0),
    this->createIndex(this->rowCount(QModelIndex())-1, 0));
  this->updateCheckState(0, Qt::Horizontal);
}
