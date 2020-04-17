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
#include "pqColorTableModel.h"

#include "pqColorOpacityEditorWidget.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkSMProxy.h"

#include <array>

//-----------------------------------------------------------------------------
class pqColorTableModel::pqInternals
{
public:
  std::vector<std::array<double, 6> > XRGBPoints;
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
    std::array<double, 6>& xrgbms = this->Internals->XRGBPoints[idx.row()];
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
      std::array<double, 6> xrgbms;
      stc->GetNodeValue(idx, xrgbms.data());
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
      std::array<double, 6> xrgbms;
      stc->GetNodeValue(idx, xrgbms.data());
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
