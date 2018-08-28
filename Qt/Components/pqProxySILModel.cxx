/*=========================================================================

   Program: ParaView
   Module:    pqProxySILModel.cxx

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
#include "pqProxySILModel.h"

// Server Manager Includes.

// Qt Includes.
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QStyleOptionButton>

// ParaView Includes.
#include "pqSILModel.h"

#define PQ_INVALID_INDEX -1947

//-----------------------------------------------------------------------------
pqProxySILModel::pqProxySILModel(const QString& hierarchyName, QObject* _parent)
  : Superclass(_parent)
{
  this->HierarchyName = hierarchyName;
  this->noCheckBoxes = false;

  QStyle::State styleOptions[3] = { QStyle::State_On | QStyle::State_Enabled,
    QStyle::State_NoChange | QStyle::State_Enabled, QStyle::State_Off | QStyle::State_Enabled };

  QStyleOptionButton option;
  QRect r = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option);
  option.rect = QRect(QPoint(0, 0), r.size());
  for (int i = 0; i < 3; i++)
  {
    this->CheckboxPixmaps[i] = QPixmap(r.size());
    this->CheckboxPixmaps[i].fill(QColor(0, 0, 0, 0));
    QPainter painter(&this->CheckboxPixmaps[i]);
    option.state = styleOptions[i];
    QApplication::style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &painter);
  }
}

//-----------------------------------------------------------------------------
pqProxySILModel::~pqProxySILModel()
{
}

//-----------------------------------------------------------------------------
QModelIndex pqProxySILModel::mapFromSource(const QModelIndex& sourceIndex) const
{
  pqSILModel* silModel = qobject_cast<pqSILModel*>(this->sourceModel());
  if (!sourceIndex.isValid() || sourceIndex == silModel->hierarchyIndex(this->HierarchyName))
  {
    return QModelIndex();
  }

  return this->createIndex(sourceIndex.row(), sourceIndex.column(),
    static_cast<quint32>(static_cast<vtkIdType>(sourceIndex.internalId())));
}

//-----------------------------------------------------------------------------
QModelIndex pqProxySILModel::mapToSource(const QModelIndex& proxyIndex) const
{
  pqSILModel* silModel = qobject_cast<pqSILModel*>(this->sourceModel());
  if (!silModel)
  {
    return this->createIndex(PQ_INVALID_INDEX, PQ_INVALID_INDEX);
  }
  else if (proxyIndex.isValid())
  {
    if (proxyIndex.column() < silModel->columnCount())
    {
      return silModel->makeIndex(static_cast<vtkIdType>(proxyIndex.internalId()));
    }
    return QModelIndex();
  }

  return silModel->hierarchyIndex(this->HierarchyName);
}

//-----------------------------------------------------------------------------
void pqProxySILModel::setSourceModel(QAbstractItemModel* srcModel)
{
  if (this->sourceModel() == srcModel)
  {
    return;
  }
  if (this->sourceModel())
  {
    QObject::disconnect(this->sourceModel(), 0, this, 0);
  }

  this->Superclass::setSourceModel(srcModel);

  if (srcModel)
  {
    QObject::connect(srcModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this,
      SLOT(sourceDataChanged(const QModelIndex&, const QModelIndex&)));
    QObject::connect(srcModel, SIGNAL(modelReset()), this, SIGNAL(modelReset()));
    QObject::connect(srcModel, SIGNAL(modelAboutToBeReset()), this, SIGNAL(modelAboutToBeReset()));
    QObject::connect(srcModel, SIGNAL(checkStatusChanged()), this, SLOT(onCheckStatusChanged()));
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqProxySILModel::values() const
{
  pqSILModel* silModel = qobject_cast<pqSILModel*>(this->sourceModel());
  return silModel->status(this->HierarchyName);
}

//-----------------------------------------------------------------------------
void pqProxySILModel::setValues(const QList<QVariant>& arg)
{
  pqSILModel* silModel = qobject_cast<pqSILModel*>(this->sourceModel());
  silModel->setStatus(this->HierarchyName, arg);
}

//-----------------------------------------------------------------------------
void pqProxySILModel::onCheckStatusChanged()
{
  emit valuesChanged();
}

//-----------------------------------------------------------------------------
QVariant pqProxySILModel::headerData(
  int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
  Q_UNUSED(section);

  // we want align all text to the left-vcenter.
  if (role == Qt::TextAlignmentRole && orientation == Qt::Horizontal)
  {
    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
  }

  if (role == Qt::DisplayRole)
  {
    return this->HeaderTitle.isEmpty() == false ? this->HeaderTitle : this->HierarchyName;
  }
  else if (this->noCheckBoxes == false && role == Qt::CheckStateRole)
  {
    QModelIndex srcIndex = this->mapToSource(QModelIndex());
    Qt::ItemFlags iflags = this->sourceModel()->flags(srcIndex);
    if ((iflags & Qt::ItemIsUserCheckable) || (iflags & Qt::ItemIsTristate))
    {
      return this->sourceModel()->data(srcIndex, Qt::CheckStateRole);
    }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
bool pqProxySILModel::setHeaderData(
  int section, Qt::Orientation orientation, const QVariant& value, int role)
{
  if (role == Qt::CheckStateRole && orientation == Qt::Horizontal)
  {
    Q_UNUSED(section);
    const QModelIndex srcIndex = this->mapToSource(QModelIndex());
    auto srcModel = this->sourceModel();
    return srcModel->setData(srcIndex, value, Qt::CheckStateRole);
  }
  return false;
}

//-----------------------------------------------------------------------------
QVariant pqProxySILModel::data(const QModelIndex& proxyIndex, int role) const
{
  if (this->noCheckBoxes && (role == Qt::DecorationRole || role == Qt::CheckStateRole))
  {
    return QVariant();
  }
  return QAbstractProxyModel::data(proxyIndex, role);
}

//-----------------------------------------------------------------------------
void pqProxySILModel::setNoCheckBoxes(bool val)
{
  this->noCheckBoxes = val;
}
//-----------------------------------------------------------------------------
void pqProxySILModel::setHeaderTitle(QString& title)
{
  this->HeaderTitle = title;
}
//-----------------------------------------------------------------------------
Qt::ItemFlags pqProxySILModel::flags(const QModelIndex& idx) const
{
  Qt::ItemFlags iflags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if (idx.isValid() && (idx.column() < this->sourceModel()->columnCount()))
  {
    QModelIndex srcIndex = this->mapToSource(idx);
    iflags = this->sourceModel()->flags(srcIndex);
    if (this->noCheckBoxes)
    {
      Qt::ItemFlags mask = Qt::ItemIsUserCheckable | Qt::ItemIsTristate;
      mask = ~mask;
      iflags = iflags & mask;
    }
  }
  return iflags;
}
