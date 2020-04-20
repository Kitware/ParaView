/*=========================================================================

   Program: ParaView
   Module:    pqDataInformationModel.cxx

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
#include "pqDataInformationModel.h"

// ParaView Server Manager includes.
#include "vtkPVDataInformation.h"
#include "vtkSMSourceProxy.h"

// Qt includes.
#include <QIcon>
#include <QList>
#include <QPointer>
#include <QtAlgorithms>
#include <QtDebug>

// ParaView includes.
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqView.h"

struct pqSourceInfo
{
  QPointer<pqOutputPort> OutputPort;
  int DataType;
  quint64 NumberOfCells;
  quint64 NumberOfPoints;
  double MemorySize;
  bool DataInformationValid;
  bool GeometryInformationValid;
  double GeometrySize;
  double Bounds[6];
  double TimeSpan[2];

  QString DataTypeName;

  vtkMTimeType MTime;
  pqSourceInfo() { this->Init(); }

  pqSourceInfo(pqOutputPort* port)
  {
    this->Init();
    this->OutputPort = port;
  }

  operator pqOutputPort*() const { return this->OutputPort; }

  void Init()
  {
    this->MTime = 0;
    this->DataType = 0;
    this->NumberOfCells = 0;
    this->NumberOfPoints = 0;
    this->MemorySize = 0;
    this->DataInformationValid = false;
    this->GeometrySize = 0;
    this->GeometryInformationValid = false;
  }

  QVariant getName() const
  {
    if (this->OutputPort)
    {
      pqPipelineSource* source = this->OutputPort->getSource();
      if (source->getNumberOfOutputPorts() > 1)
      {
        return QVariant(
          QString("%1 (%2)").arg(source->getSMName()).arg(this->OutputPort->getPortNumber()));
      }
      return QVariant(source->getSMName());
    }
    return QVariant("Unknown");
  }

  QVariant getNumberOfCells() const
  {
    if (this->DataInformationValid)
    {
      return QVariant(this->NumberOfCells);
    }
    return QVariant("Unavailable");
  }

  QVariant getNumberOfPoints() const
  {
    if (this->DataInformationValid)
    {
      return QVariant(this->NumberOfPoints);
    }
    return QVariant("Unavailable");
  }

  QVariant getMemorySize() const
  {
    if (this->DataInformationValid)
    {
      return QVariant(this->MemorySize);
    }
    return QVariant("Unavailable");
  }

  QVariant getGeometrySize() const
  {
    if (this->GeometryInformationValid)
    {
      return QVariant(this->GeometrySize);
    }

    return QVariant("Unavailable");
  }

  QVariant getBounds() const
  {
    if (this->DataInformationValid)
    {
      QString bounds("[ %1, %2 ] , [ %3, %4 ] , [ %5, %6 ]");
      for (int i = 0; i < 6; i++)
      {
        bounds = bounds.arg(this->Bounds[i], 0, 'g', 3);
      }
      return QVariant(bounds);
    }
    return QVariant("Unavailable");
  }
  QVariant getTimes() const
  {
    if (this->DataInformationValid)
    {
      if (this->TimeSpan[0] > this->TimeSpan[1])
      {
        QString times("[ALL]");
        return QVariant(times);
      }
      else
      {
        QString times("[ %1, %2]");
        times = times.arg(this->TimeSpan[0], 0, 'g', 3);
        times = times.arg(this->TimeSpan[1], 0, 'g', 3);
        return QVariant(times);
      }
    }
    return QVariant("Unavailable");
  }

  // Given a data type ID, returns the string.
  QString getDataTypeAsString() const
  {
    if (this->DataInformationValid)
    {
      return this->DataTypeName;
    }
    return "Unavailable";
  }

  // Given a datatype, returns the icon for that data type.
  QIcon getDataTypeAsIcon() const
  {
    if (!this->DataInformationValid)
    {
      return QIcon(":/pqWidgets/Icons/pqUnknownData16.png");
    }

    switch (this->DataType)
    {
      case VTK_POLY_DATA:
        return QIcon(":/pqWidgets/Icons/pqPolydata16.png");

      case VTK_HYPER_OCTREE:
        return QIcon(":/pqWidgets/Icons/pqOctreeData.svg");

      case VTK_UNSTRUCTURED_GRID:
        return QIcon(":/pqWidgets/Icons/pqUnstructuredGrid16.png");

      case VTK_STRUCTURED_GRID:
        return QIcon(":/pqWidgets/Icons/pqStructuredGrid16.png");

      case VTK_RECTILINEAR_GRID:
        return QIcon(":/pqWidgets/Icons/pqRectilinearGrid16.png");

      case VTK_IMAGE_DATA:
        /*
        {
        int *ext = dataInfo->GetExtent();
        if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
        {
        return "Image (Uniform Rectilinear)";
        }
        return "Volume (Uniform Rectilinear)";
        }
        */
        return QIcon(":/pqWidgets/Icons/pqStructuredGrid16.png");

      case VTK_MULTIGROUP_DATA_SET:
        return QIcon(":/pqWidgets/Icons/pqGroup.svg");

      case VTK_MULTIBLOCK_DATA_SET:
        return QIcon(":/pqWidgets/Icons/pqMultiBlockData16.png");

      case VTK_HIERARCHICAL_DATA_SET:
        return QIcon(":/pqWidgets/Icons/pqHierarchicalData16.png");

      case VTK_HIERARCHICAL_BOX_DATA_SET:
        return QIcon(":/pqWidgets/Icons/pqOctreeData.svg");

      default:
        return QIcon(":/pqWidgets/Icons/pqUnknownData16.png");
    }
  }
};

//-----------------------------------------------------------------------------
class pqDataInformationModelInternal
{
public:
  QPointer<pqView> View;
  QList<pqSourceInfo> Sources;
  vtkTimeStamp UpdateTime;

  bool contains(pqPipelineSource* src)
  {
    foreach (pqSourceInfo info, this->Sources)
    {
      if (info.OutputPort->getSource() == src)
      {
        return true;
      }
    }
    return false;
  }

  int indexOf(pqPipelineSource* src)
  {
    int index = 0;
    foreach (pqSourceInfo info, this->Sources)
    {
      if (info.OutputPort->getSource() == src)
      {
        return index;
      }
      ++index;
    }

    return -1;
  }

  int lastIndexOf(pqPipelineSource* src)
  {
    for (int cc = this->Sources.size() - 1; cc >= 0; --cc)
    {
      pqSourceInfo& info = this->Sources[cc];
      if (info.OutputPort->getSource() == src)
      {
        return cc;
      }
    }

    return -1;
  }
};

//-----------------------------------------------------------------------------
pqDataInformationModel::pqDataInformationModel(QObject* _parent /*=NULL*/)
  : QAbstractTableModel(_parent)
{
  this->Internal = new pqDataInformationModelInternal();
}

//-----------------------------------------------------------------------------
pqDataInformationModel::~pqDataInformationModel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
int pqDataInformationModel::rowCount(const QModelIndex& vtkNotUsed(parent) /*=QModelIndex()*/) const
{
  return (this->Internal->Sources.size());
}

//-----------------------------------------------------------------------------
int pqDataInformationModel::columnCount(
  const QModelIndex& vtkNotUsed(parent) /*= QModelIndex()*/) const
{
  return pqDataInformationModel::Max_Columns;
}

//-----------------------------------------------------------------------------
QVariant pqDataInformationModel::data(const QModelIndex& idx, int role /*= Qt::DisplayRole*/) const
{
  if (!idx.isValid() || idx.model() != this)
  {
    return QVariant();
  }

  if (idx.row() >= this->Internal->Sources.size())
  {
    qDebug() << "pqDataInformationModel::data called with invalid index: " << idx.row();
    return QVariant();
  }
  if (role == Qt::ToolTipRole)
  {
    return this->headerData(idx.column(), Qt::Horizontal, Qt::DisplayRole);
  }

  pqSourceInfo& info = this->Internal->Sources[idx.row()];

  switch (idx.column())
  {
    case pqDataInformationModel::Name:
      // Name column.
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return QVariant(info.getName());
      }
      break;

    case pqDataInformationModel::DataType:
      // Data column.
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return QVariant(info.getDataTypeAsString());

        case Qt::DecorationRole:
          return QVariant(info.getDataTypeAsIcon());
      }
      break;

    case pqDataInformationModel::CellCount:
      // Number of cells.
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return info.getNumberOfCells();

        case Qt::DecorationRole:
          return QVariant(QIcon(":/pqWidgets/Icons/pqCellData.svg"));
      }
      break;

    case pqDataInformationModel::PointCount:
      // Number of Points.
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return info.getNumberOfPoints();

        case Qt::DecorationRole:
          return QVariant(QIcon(":/pqWidgets/Icons/pqPointData.svg"));
      }
      break;

    case pqDataInformationModel::MemorySize:
      // Memory.
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return info.getMemorySize();
      }
      break;

    case pqDataInformationModel::GeometrySize:
      // Geometry size for active view.
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return info.getGeometrySize();
      }
      break;

    case pqDataInformationModel::Bounds:
      // Spatial Bounds.
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return info.getBounds();
      }
      break;

    case pqDataInformationModel::TimeSpan:
      // Temporal Bounds and steps
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::EditRole:
          return info.getTimes();
      }
      break;
  }
  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant pqDataInformationModel::headerData(
  int section, Qt::Orientation orientation, int role /*=Qt::DisplayRole*/) const
{
  if (orientation == Qt::Horizontal)
  {
    switch (role)
    {
      case Qt::DisplayRole:
        switch (section)
        {
          case pqDataInformationModel::Name:
            return QVariant("Name");

          case pqDataInformationModel::DataType:
            return QVariant("Data Type");

          case pqDataInformationModel::CellCount:
            return QVariant("No. of Cells");

          case pqDataInformationModel::PointCount:
            return QVariant("No. of Points");

          case pqDataInformationModel::MemorySize:
            return QVariant("Memory (MB)");

          case pqDataInformationModel::GeometrySize:
            return QVariant("Geometry Size (MB)");

          case pqDataInformationModel::Bounds:
            return QVariant("Spatial Bounds");

          case pqDataInformationModel::TimeSpan:
            return QVariant("Temporal Bounds");
        }
        break;
    }
  }
  return QVariant();
}

//-----------------------------------------------------------------------------
void pqDataInformationModel::dataUpdated(pqPipelineSource* changedSource)
{
  QList<pqSourceInfo>::iterator iter;
  int row_no = 0;
  for (iter = this->Internal->Sources.begin(); iter != this->Internal->Sources.end();
       ++iter, row_no++)
  {
    pqOutputPort* port = iter->OutputPort;
    pqPipelineSource* source = port->getSource();

    if (source != changedSource)
    {
      continue;
    }

    vtkPVDataInformation* dataInfo = port->getDataInformation();
    if (!iter->DataInformationValid || dataInfo->GetMTime() > iter->MTime)
    {
      iter->MTime = dataInfo->GetMTime();
      iter->DataType = dataInfo->GetDataSetType();
      iter->DataTypeName = dataInfo->GetPrettyDataTypeString();
      if (dataInfo->GetCompositeDataSetType() >= 0)
      {
        iter->DataType = dataInfo->GetCompositeDataSetType();
      }
      iter->NumberOfCells = dataInfo->GetNumberOfCells();
      iter->NumberOfPoints = dataInfo->GetNumberOfPoints();
      iter->MemorySize = dataInfo->GetMemorySize() / 1000.0;
      dataInfo->GetBounds(iter->Bounds);
      dataInfo->GetTimeSpan(iter->TimeSpan);
      iter->DataInformationValid = true;

      Q_EMIT this->dataChanged(
        this->index(row_no, Name), this->index(row_no, pqDataInformationModel::Max_Columns - 1));
    }
  }
}

//-----------------------------------------------------------------------------
void pqDataInformationModel::addSource(pqPipelineSource* source)
{
  if (this->Internal->contains(source))
  {
    return;
  }

  int numOutputPorts = source->getNumberOfOutputPorts();
  this->beginInsertRows(QModelIndex(), this->Internal->Sources.size(),
    this->Internal->Sources.size() + numOutputPorts - 1);

  for (int cc = 0; cc < numOutputPorts; cc++)
  {
    this->Internal->Sources.push_back(source->getOutputPort(cc));
  }
  this->endInsertRows();

  QObject::connect(
    source, SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(dataUpdated(pqPipelineSource*)));
}

//-----------------------------------------------------------------------------
void pqDataInformationModel::removeSource(pqPipelineSource* source)
{
  int idx = this->Internal->indexOf(source);

  if (idx != -1)
  {
    int lastIdx = this->Internal->lastIndexOf(source);

    this->beginRemoveRows(QModelIndex(), idx, lastIdx);
    for (int cc = lastIdx; cc >= idx; --cc)
    {
      this->Internal->Sources.removeAt(cc);
    }
    this->endRemoveRows();
  }

  QObject::disconnect(source, 0, this, 0);
}

//-----------------------------------------------------------------------------
QModelIndex pqDataInformationModel::getIndexFor(pqOutputPort* item) const
{
  if (!this->Internal->Sources.contains(item))
  {
    return QModelIndex();
  }
  return this->index(this->Internal->Sources.indexOf(item), 0);
}

//-----------------------------------------------------------------------------
pqOutputPort* pqDataInformationModel::getItemFor(const QModelIndex& idx) const
{
  if (!idx.isValid() && idx.model() != this)
  {
    return NULL;
  }
  if (idx.row() >= this->Internal->Sources.size())
  {
    qDebug() << "Index: " << idx.row() << " beyond range.";
    return NULL;
  }
  return this->Internal->Sources[idx.row()].OutputPort;
}

//-----------------------------------------------------------------------------
void pqDataInformationModel::setActiveView(pqView* view)
{
  if (this->Internal->View == view)
  {
    return;
  }

  if (this->Internal->View)
  {
    QObject::disconnect(this->Internal->View, 0, this, 0);
  }

  this->Internal->View = view;

  if (view)
  {
    QObject::connect(view, SIGNAL(endRender()), this, SLOT(refreshGeometrySizes()));
  }

  this->refreshGeometrySizes();
}

//-----------------------------------------------------------------------------
void pqDataInformationModel::refreshGeometrySizes()
{
  // Must be called only after endRender() when we are assured that all
  // representations are up-to-date.
  QList<pqSourceInfo>::iterator iter;
  for (iter = this->Internal->Sources.begin(); iter != this->Internal->Sources.end(); ++iter)
  {
    pqSourceInfo& sourceInfo = (*iter);
    sourceInfo.GeometryInformationValid = false;
    pqOutputPort* port = sourceInfo.OutputPort;
    if (this->Internal->View)
    {
      pqDataRepresentation* repr = port->getRepresentation(this->Internal->View);
      if (!repr || !repr->isVisible())
      {
        continue;
      }
      sourceInfo.GeometryInformationValid = true;
      sourceInfo.GeometrySize = repr->getFullResMemorySize() / 1000.0;
    }
  }

  Q_EMIT this->dataChanged(this->index(0, pqDataInformationModel::GeometrySize),
    this->index(this->rowCount() - 1, pqDataInformationModel::GeometrySize));
}
//-----------------------------------------------------------------------------
Qt::ItemFlags pqDataInformationModel::flags(const QModelIndex& idx) const
{
  return QAbstractTableModel::flags(idx) | Qt::ItemIsEditable;
}
//-----------------------------------------------------------------------------
bool pqDataInformationModel::setData(const QModelIndex&, const QVariant&, int)
{
  // Do nothing, we are not supposed to change our data...
  return true;
}
