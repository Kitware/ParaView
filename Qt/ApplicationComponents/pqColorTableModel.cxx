// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorTableModel.h"

#include "pqColorOpacityEditorWidget.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkSMProxy.h"

#include <algorithm>

//-----------------------------------------------------------------------------
class pqColorTableModel::pqInternals
{
public:
  std::vector<vtkVector<double, 6>> XRGBPoints;
};

//-----------------------------------------------------------------------------
pqColorTableModel::pqColorTableModel(pqColorOpacityEditorWidget* widget, QObject* parentObject)
  : Superclass(parentObject)
  , Widget(widget)
  , Internals(new pqColorTableModel::pqInternals())
{
  // Update the model when XRGB points change
  QObject::connect(widget, SIGNAL(xrgbPointsChanged()), this, SLOT(controlPointsChanged()));
  QObject::connect(widget, SIGNAL(changeFinished()), this, SLOT(controlPointsChanged()));

  QObject::connect(this, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
    SLOT(updatePoint(const QModelIndex&)));
}

//-----------------------------------------------------------------------------
pqColorTableModel::~pqColorTableModel()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqColorTableModel::flags(const QModelIndex& idx) const
{
  Qt::ItemFlags parentFlags = this->Superclass::flags(idx);
  if (idx.column() == 0 && (idx.row() == 0 || idx.row() == this->rowCount() - 1))
  {
    return parentFlags;
  }
  return parentFlags | Qt::ItemIsEditable;
}

//-----------------------------------------------------------------------------
bool pqColorTableModel::setData(const QModelIndex& idx, const QVariant& value, int role)
{
  Q_UNUSED(role);

  // Do not edit first and last control point scalar values
  if (idx.column() == 0 && (idx.row() == 0 || idx.row() == this->rowCount() - 1))
  {
    return false;
  }

  double newValue = value.toDouble();
  if (idx.column() == 0 && (newValue < this->Range[0] || newValue > this->Range[1]))
  {
    return false;
  }
  else if (idx.column() > 0 && (newValue < 0.0 || newValue > 1.0))
  {
    return false;
  }

  if (this->Widget && this->Widget->proxy())
  {
    this->Internals->XRGBPoints[idx.row()][idx.column()] = newValue;

    Q_EMIT dataChanged(idx, idx);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
int pqColorTableModel::rowCount(const QModelIndex& parentIndex) const
{
  Q_UNUSED(parentIndex);

  int size = 0;
  if (this->Widget && this->Widget->proxy())
  {
    size = static_cast<int>(this->Internals->XRGBPoints.size());
  }

  return size;
}

//-----------------------------------------------------------------------------
int pqColorTableModel::columnCount(const QModelIndex& parentIndex) const
{
  Q_UNUSED(parentIndex);
  return 4;
}

//-----------------------------------------------------------------------------
QVariant pqColorTableModel::data(const QModelIndex& idx, int role) const
{
  if (role == Qt::DisplayRole || role == Qt::EditRole)
  {
    vtkVector<double, 6>& xrgbms = this->Internals->XRGBPoints[idx.row()];
    if (idx.column() >= 0 && idx.column() < 4)
    {
      return QString::number(xrgbms[idx.column()]);
    }
  }
  else if (role == Qt::ToolTipRole || role == Qt::StatusTipRole)
  {
    switch (idx.column())
    {
      case 0:
        return "Data Value";
      case 1:
        return "Red Component";
      case 2:
        return "Green Component";
      case 3:
        return "Blue Component";
    }
  }
  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant pqColorTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
      case 0:
        return "Value";
      case 1:
        return "R";
      case 2:
        return "G";
      case 3:
        return "B";
    }
  }
  return this->Superclass::headerData(section, orientation, role);
}

//-----------------------------------------------------------------------------
std::vector<vtkVector<double, 6>> pqColorTableModel::points() const
{
  return this->Internals->XRGBPoints;
}

//-----------------------------------------------------------------------------
size_t pqColorTableModel::insertPoint(size_t loc)
{
  vtkDiscretizableColorTransferFunction* stc = vtkDiscretizableColorTransferFunction::SafeDownCast(
    this->Widget->proxy()->GetClientSideObject());
  if (!stc)
  {
    return 0;
  }

  size_t currentSize = stc->GetSize();
  loc = std::min(loc, currentSize);
  if (loc == 0 || loc >= currentSize)
  {
    // Cannot insert before the first point or after the last point.
    return currentSize;
  }

  const auto& pts = this->Internals->XRGBPoints;
  auto xrgbms = pts[loc - 1] + (pts[loc] - pts[loc - 1]) / vtkVector<double, 6>(2.0);
  double rgb[3] = {};
  stc->GetColor(xrgbms[0], rgb);
  xrgbms[1] = rgb[0];
  xrgbms[2] = rgb[1];
  xrgbms[3] = rgb[2];
  stc->AddRGBPoint(xrgbms[0], xrgbms[1], xrgbms[2], xrgbms[3], xrgbms[4], xrgbms[5]);

  Q_EMIT this->beginInsertRows(QModelIndex(), static_cast<int>(loc), static_cast<int>(loc));
  this->Internals->XRGBPoints.insert(std::next(pts.begin(), loc), xrgbms);
  Q_EMIT this->endInsertRows();

  return loc;
}

//-----------------------------------------------------------------------------
bool pqColorTableModel::setPoints(const std::vector<vtkVector<double, 6>>& pts)
{
  vtkDiscretizableColorTransferFunction* stc = vtkDiscretizableColorTransferFunction::SafeDownCast(
    this->Widget->proxy()->GetClientSideObject());
  if (!stc)
  {
    return false;
  }
  auto& existingPts = this->Internals->XRGBPoints;
  if (pts.empty())
  {
    if (existingPts.empty())
    {
      return false;
    }
    else if (existingPts.size() == 2)
    {
      // If we have only two points, we cannot remove any.
      return false;
    }
    else
    {
      // Remove all points except the first and last.
      Q_EMIT this->beginRemoveRows(QModelIndex(), 1, static_cast<int>(existingPts.size()) - 2);
      for (size_t i = 1; i < existingPts.size() - 1; ++i)
      {
        stc->RemovePoint(existingPts[i][0]);
      }
      existingPts.clear();
      existingPts.reserve(stc->GetSize());
      for (int idx = 0; idx < stc->GetSize(); ++idx)
      {
        vtkVector<double, 6> xrgbms;
        stc->GetNodeValue(idx, xrgbms.GetData());
        existingPts.push_back(xrgbms);
      }
      Q_EMIT this->endRemoveRows();
      return true;
    }
  }
  else if (pts.size() >= 2)
  {
    this->beginResetModel();
    stc->RemoveAllPoints();
    existingPts.clear();
    existingPts.reserve(pts.size());
    for (const auto& xrgbms : pts)
    {
      stc->AddRGBPoint(xrgbms[0], xrgbms[1], xrgbms[2], xrgbms[3], xrgbms[4], xrgbms[5]);
      existingPts.push_back(xrgbms);
    }
    this->endResetModel();
  }
  else
  {
    return false; // we need at least two points.
  }

  return true;
}

//-----------------------------------------------------------------------------
void pqColorTableModel::controlPointsChanged()
{
  vtkDiscretizableColorTransferFunction* stc = vtkDiscretizableColorTransferFunction::SafeDownCast(
    this->Widget->proxy()->GetClientSideObject());
  int newSize = 0;
  if (stc)
  {
    newSize = stc->GetSize();
    stc->GetRange(this->Range);
  }

  int previousSize = static_cast<int>(this->Internals->XRGBPoints.size());
  if (newSize > previousSize)
  {
    Q_EMIT this->beginInsertRows(QModelIndex(), previousSize, newSize - 1);
    for (int idx = previousSize; idx < newSize; ++idx)
    {
      vtkVector<double, 6> xrgbms;
      stc->GetNodeValue(idx, xrgbms.GetData());
      this->Internals->XRGBPoints.push_back(xrgbms);
    }
    Q_EMIT this->endInsertRows();
  }
  else if (newSize < previousSize)
  {
    Q_EMIT this->beginRemoveRows(QModelIndex(), newSize, previousSize - 1);
    size_t numToRemove = previousSize - newSize;
    for (size_t idx = 0; idx < numToRemove; ++idx)
    {
      this->Internals->XRGBPoints.pop_back();
    }
    Q_EMIT this->endRemoveRows();
  }
  else // newSize == previousSize
  {
    this->beginResetModel();
    for (int idx = 0; idx < newSize; ++idx)
    {
      vtkVector<double, 6> xrgbms;
      stc->GetNodeValue(idx, xrgbms.GetData());
      this->Internals->XRGBPoints[idx] = xrgbms;
    }
    this->endResetModel();
  }
}

//-----------------------------------------------------------------------------
void pqColorTableModel::updatePoint(const QModelIndex& idx)
{
  vtkDiscretizableColorTransferFunction* stc = vtkDiscretizableColorTransferFunction::SafeDownCast(
    this->Widget->proxy()->GetClientSideObject());
  if (stc)
  {
    double xrgbms[6];
    stc->GetNodeValue(idx.row(), xrgbms);
    QVariant data = this->data(idx);
    xrgbms[idx.column()] = data.toDouble();
    stc->SetNodeValue(idx.row(), xrgbms);
  }
}
