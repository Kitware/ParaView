/*=========================================================================

  Program: ParaView
  Module:    $RCSfile$

  Copyright (c) 2013 Sandia Corporation, Kitware Inc.
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
  std::vector<std::array<double, 4> > XVMSPoints;
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
    std::array<double, 4>& xvms = this->Internals->XVMSPoints[idx.row()];
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
      std::array<double, 4> xvms;
      pwf->GetNodeValue(idx, xvms.data());
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
      std::array<double, 4> xvms;
      pwf->GetNodeValue(idx, xvms.data());
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
