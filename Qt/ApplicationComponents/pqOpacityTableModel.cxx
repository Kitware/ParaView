// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqOpacityTableModel.h"

#include "pqColorOpacityEditorWidget.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSMProxy.h"

#include <array>

//-----------------------------------------------------------------------------
class pqOpacityTableModel::pqInternals
{
public:
  std::vector<vtkVector4d> XVMSPoints;
};

//-----------------------------------------------------------------------------
pqOpacityTableModel::pqOpacityTableModel(pqColorOpacityEditorWidget* widget, QObject* parentObject)
  : Superclass(parentObject)
  , Widget(widget)
  , Internals(new pqOpacityTableModel::pqInternals())
{
  // Update the model when XVMS points change
  QObject::connect(widget, SIGNAL(xvmsPointsChanged()), this, SLOT(controlPointsChanged()));
  QObject::connect(widget, SIGNAL(changeFinished()), this, SLOT(controlPointsChanged()));

  QObject::connect(this, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
    SLOT(updatePoint(const QModelIndex&)));
}

//-----------------------------------------------------------------------------
pqOpacityTableModel::~pqOpacityTableModel()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqOpacityTableModel::flags(const QModelIndex& idx) const
{
  Qt::ItemFlags parentFlags = this->Superclass::flags(idx);
  if (idx.column() == 0 && (idx.row() == 0 || idx.row() == this->rowCount() - 1))
  {
    return parentFlags;
  }
  return parentFlags | Qt::ItemIsEditable;
}

//-----------------------------------------------------------------------------
bool pqOpacityTableModel::setData(const QModelIndex& idx, const QVariant& value, int role)
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
    this->Internals->XVMSPoints[idx.row()][idx.column()] = newValue;

    Q_EMIT dataChanged(idx, idx);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
int pqOpacityTableModel::rowCount(const QModelIndex& parentIndex) const
{
  Q_UNUSED(parentIndex);

  int size = 0;
  if (this->Widget && this->Widget->proxy())
  {
    size = static_cast<int>(this->Internals->XVMSPoints.size());
  }

  return size;
}

//-----------------------------------------------------------------------------
int pqOpacityTableModel::columnCount(const QModelIndex& parentIndex) const
{
  Q_UNUSED(parentIndex);

  return 2;
}

//-----------------------------------------------------------------------------
QVariant pqOpacityTableModel::data(const QModelIndex& idx, int role) const
{
  if (role == Qt::DisplayRole || role == Qt::EditRole)
  {
    vtkVector4d& xvms = this->Internals->XVMSPoints[idx.row()];
    if (idx.column() >= 0 && idx.column() < 4)
    {
      return QString::number(xvms[idx.column()]);
    }
  }
  else if (role == Qt::ToolTipRole || role == Qt::StatusTipRole)
  {
    switch (idx.column())
    {
      case 0:
        return "Data Value";
      case 1:
        return "Opacity";
    }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant pqOpacityTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
      case 0:
        return "Value";
      case 1:
        return "Opacity";
    }
  }
  return this->Superclass::headerData(section, orientation, role);
}

//-----------------------------------------------------------------------------
std::vector<vtkVector4d> pqOpacityTableModel::points() const
{
  return this->Internals->XVMSPoints;
}

//-----------------------------------------------------------------------------
size_t pqOpacityTableModel::insertPoint(size_t loc)
{
  vtkDiscretizableColorTransferFunction* stc = vtkDiscretizableColorTransferFunction::SafeDownCast(
    this->Widget->proxy()->GetClientSideObject());
  if (!stc)
  {
    return 0;
  }
  vtkPiecewiseFunction* pwf = nullptr;
  if (stc)
  {
    pwf = stc ? stc->GetScalarOpacityFunction() : nullptr;
  }
  if (!pwf)
  {
    return 0;
  }

  size_t currentSize = pwf->GetSize();
  loc = std::min(loc, currentSize);
  if (loc == 0 || loc >= currentSize)
  {
    // Cannot insert before the first point or after the last point.
    return currentSize;
  }

  const auto& pts = this->Internals->XVMSPoints;
  const auto xvms = pts[loc - 1] + (pts[loc] - pts[loc - 1]) / vtkVector4d(2.0);
  pwf->AddPoint(xvms[0], xvms[1], xvms[2], xvms[3]);

  Q_EMIT this->beginInsertRows(QModelIndex(), static_cast<int>(loc), static_cast<int>(loc));
  this->Internals->XVMSPoints.insert(std::next(pts.begin(), loc), xvms);
  Q_EMIT this->endInsertRows();

  return loc;
}

//-----------------------------------------------------------------------------
bool pqOpacityTableModel::setPoints(const std::vector<vtkVector4d>& pts)
{
  vtkDiscretizableColorTransferFunction* stc = vtkDiscretizableColorTransferFunction::SafeDownCast(
    this->Widget->proxy()->GetClientSideObject());
  if (!stc)
  {
    return false;
  }
  vtkPiecewiseFunction* pwf = nullptr;
  if (stc)
  {
    pwf = stc ? stc->GetScalarOpacityFunction() : nullptr;
  }
  if (!pwf)
  {
    return false;
  }
  auto& existingPts = this->Internals->XVMSPoints;
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
        pwf->RemovePoint(existingPts[i][0]);
      }
      existingPts.clear();
      existingPts.reserve(pwf->GetSize());
      for (int idx = 0; idx < pwf->GetSize(); ++idx)
      {
        vtkVector4d xvms;
        pwf->GetNodeValue(idx, xvms.GetData());
        existingPts.push_back(xvms);
      }
      Q_EMIT this->endRemoveRows();
      return true;
    }
  }
  else if (pts.size() >= 2)
  {
    this->beginResetModel();
    pwf->RemoveAllPoints();
    existingPts.clear();
    existingPts.reserve(pts.size());
    for (const auto& xvms : pts)
    {
      pwf->AddPoint(xvms[0], xvms[1], xvms[2], xvms[3]);
      existingPts.push_back(xvms);
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
void pqOpacityTableModel::controlPointsChanged()
{
  vtkDiscretizableColorTransferFunction* stc = vtkDiscretizableColorTransferFunction::SafeDownCast(
    this->Widget->proxy()->GetClientSideObject());
  vtkPiecewiseFunction* pwf = nullptr;
  int newSize = 0;
  if (stc)
  {
    pwf = stc ? stc->GetScalarOpacityFunction() : nullptr;
    if (pwf)
    {
      newSize = pwf->GetSize();
      stc->GetRange(this->Range);
    }
  }

  int previousSize = static_cast<int>(this->Internals->XVMSPoints.size());
  if (newSize > previousSize)
  {
    Q_EMIT this->beginInsertRows(QModelIndex(), previousSize, newSize - 1);
    for (int idx = previousSize; idx < newSize; ++idx)
    {
      vtkVector4d xvms;
      pwf->GetNodeValue(idx, xvms.GetData());
      this->Internals->XVMSPoints.push_back(xvms);
    }
    Q_EMIT this->endInsertRows();
  }
  else if (newSize < previousSize)
  {
    Q_EMIT this->beginRemoveRows(QModelIndex(), newSize, previousSize - 1);
    size_t numToRemove = previousSize - newSize;
    for (size_t idx = 0; idx < numToRemove; ++idx)
    {
      this->Internals->XVMSPoints.pop_back();
    }
    Q_EMIT this->endRemoveRows();
  }
  else // newSize == previousSize
  {
    this->beginResetModel();
    for (int idx = 0; idx < newSize; ++idx)
    {
      vtkVector4d xvms;
      pwf->GetNodeValue(idx, xvms.GetData());
      this->Internals->XVMSPoints[idx] = xvms;
    }
    this->endResetModel();
  }
}

//-----------------------------------------------------------------------------
void pqOpacityTableModel::updatePoint(const QModelIndex& idx)
{
  vtkDiscretizableColorTransferFunction* stc = vtkDiscretizableColorTransferFunction::SafeDownCast(
    this->Widget->proxy()->GetClientSideObject());
  vtkPiecewiseFunction* pwf = nullptr;
  if (stc)
  {
    pwf = stc->GetScalarOpacityFunction();
    if (pwf)
    {
      double xvms[4];
      pwf->GetNodeValue(idx.row(), xvms);
      QVariant data = this->data(idx);
      xvms[idx.column()] = data.toDouble();
      pwf->SetNodeValue(idx.row(), xvms);
    }
  }
}
