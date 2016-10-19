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

//-----------------------------------------------------------------------------
pqColorTableModel::pqColorTableModel(pqColorOpacityEditorWidget* widget, QObject* parentObject)
  : Superclass(parentObject)
  , Widget(widget)
{
  this->NumberOfRowsCache = this->rowCount();
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

  if (this->Widget && this->Widget->proxy())
  {
    vtkDiscretizableColorTransferFunction* stc =
      vtkDiscretizableColorTransferFunction::SafeDownCast(
        this->Widget->proxy()->GetClientSideObject());
    if (stc)
    {
      double range[2];
      stc->GetRange(range);
      double newValue = value.toDouble();
      if (idx.column() == 0 && (newValue < range[0] || newValue > range[1]))
      {
        return false;
      }
      else if (idx.column() > 0 && (newValue < 0.0 || newValue > 1.0))
      {
        return false;
      }

      double xrgbms[6];
      stc->GetNodeValue(idx.row(), xrgbms);
      xrgbms[idx.column()] = newValue;
      stc->SetNodeValue(idx.row(), xrgbms);

      emit this->dataChanged(idx, idx);
      return true;
    }
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
    vtkDiscretizableColorTransferFunction* stc =
      vtkDiscretizableColorTransferFunction::SafeDownCast(
        this->Widget->proxy()->GetClientSideObject());
    if (stc)
    {
      size = stc->GetSize();
    }
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
    vtkDiscretizableColorTransferFunction* stc = NULL;
    if (this->Widget && this->Widget->proxy())
    {
      stc = vtkDiscretizableColorTransferFunction::SafeDownCast(
        this->Widget->proxy()->GetClientSideObject());
    }
    if (!stc)
    {
      return QVariant();
    }

    // Get the XRGB value
    double xrgbms[6];
    stc->GetNodeValue(idx.row(), xrgbms);

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
void pqColorTableModel::refresh()
{
  // Tell the model that the number of rows may have changed because it does not
  // get this information from the rowCount() method. There must be a  more elegant
  // way to do this.
  int currentNumberOfRows = this->rowCount();
  if (this->NumberOfRowsCache < currentNumberOfRows)
  {
    this->beginInsertRows(QModelIndex(), this->NumberOfRowsCache, currentNumberOfRows - 1);
    this->endInsertRows();
  }
  else if (this->NumberOfRowsCache > currentNumberOfRows)
  {
    this->beginRemoveRows(QModelIndex(), currentNumberOfRows, this->NumberOfRowsCache - 1);
    this->endRemoveRows();
  }
  this->NumberOfRowsCache = currentNumberOfRows;

  QModelIndex topLeft = this->createIndex(0, 0);
  QModelIndex bottomRight = this->createIndex(this->rowCount() - 1, this->columnCount() - 1);

  emit dataChanged(topLeft, bottomRight);
}
