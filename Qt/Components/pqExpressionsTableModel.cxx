/*=========================================================================

   Program: ParaView
   Module:    pqExpressionsTableModel.cxx

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

#include "pqExpressionsTableModel.h"

#include <QDebug>

#include <set>

pqExpressionsTableModel::pqExpressionsTableModel(QObject* parent)
  : Superclass(parent)
{
}

//----------------------------------------------------------------------------
int pqExpressionsTableModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
  {
    return 0;
  }

  return this->Expressions.size();
}

//----------------------------------------------------------------------------
int pqExpressionsTableModel::columnCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return ColumnKey::NumberOfColumns;
}

//----------------------------------------------------------------------------
QVariant pqExpressionsTableModel::data(const QModelIndex& index, int role) const
{
  if (index.isValid() && index.row() < this->rowCount(QModelIndex()) &&
    (role == Qt::DisplayRole || role == Qt::EditRole))
  {
    switch (index.column())
    {
      case ColumnKey::ExpressionColumn:
      {
        return QVariant(this->Expressions[index.row()].Value);
      }
      case ColumnKey::NameColumn:
      {
        return QVariant(this->Expressions[index.row()].Name);
      }
      case ColumnKey::GroupColumn:
      {
        return QVariant(this->Expressions[index.row()].Group);
      }
      default:
        break;
    }
  }
  return QVariant();
}

//----------------------------------------------------------------------------
QVariant pqExpressionsTableModel::headerData(
  int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
      case ColumnKey::ExpressionColumn:
      {
        return "Content";
      }
      case ColumnKey::NameColumn:
      {
        return "Name";
      }
      case ColumnKey::GroupColumn:
      {
        return "Type";
      }
      default:
        break;
    }
  }

  return QVariant();
}

//----------------------------------------------------------------------------
Qt::ItemFlags pqExpressionsTableModel::flags(const QModelIndex& index) const
{
  Qt::ItemFlags flags = Superclass::flags(index);
  return flags |= Qt::ItemIsEditable;
}

//----------------------------------------------------------------------------
bool pqExpressionsTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (index.isValid() && role == Qt::EditRole)
  {
    switch (index.column())
    {
      case ColumnKey::ExpressionColumn:
      {
        return this->setExpression(index.row(), value.toString());
      }
      case ColumnKey::NameColumn:
      {
        return this->setExpressionName(index.row(), value.toString());
      }
      case ColumnKey::GroupColumn:
      {
        return this->setExpressionGroup(index.row(), value.toString());
      }
      default:
        break;
    }
  }

  qWarning() << "Cannot set data, item not found";
  return false;
}

//----------------------------------------------------------------------------
bool pqExpressionsTableModel::insertRows(int position, int rows, const QModelIndex& parent)
{
  Q_UNUSED(parent);
  this->beginInsertRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row)
  {
    this->Expressions.insert(position, pqExpressionsManager::pqExpression());
  }

  this->endInsertRows();
  return true;
}

//----------------------------------------------------------------------------
bool pqExpressionsTableModel::removeRows(int position, int rows, const QModelIndex& parent)
{
  Q_UNUSED(parent);
  this->beginRemoveRows(QModelIndex(), position, position + rows - 1);

  this->Expressions.erase(
    this->Expressions.begin() + position, this->Expressions.begin() + position + rows);

  this->endRemoveRows();
  return true;
}

//----------------------------------------------------------------------------
QModelIndex pqExpressionsTableModel::addNewExpression()
{
  int row = this->rowCount(QModelIndex());
  this->insertRow(row);
  return this->index(row, ColumnKey::ExpressionColumn);
}

//----------------------------------------------------------------------------
QString pqExpressionsTableModel::getExpressionAsString(const QModelIndex& idx)
{
  return this->Expressions[idx.row()].Value;
}

//----------------------------------------------------------------------------
QModelIndex pqExpressionsTableModel::expressionIndex(
  const pqExpressionsManager::pqExpression& expression)
{
  for (int row = 0; row < this->rowCount(QModelIndex()); row++)
  {
    if (this->Expressions[row] == expression)
    {
      return this->index(row, ColumnKey::ExpressionColumn);
    }
  }

  return QModelIndex();
}

//----------------------------------------------------------------------------
QString pqExpressionsTableModel::getGroup(const QModelIndex& idx)
{
  return this->Expressions[idx.row()].Group;
}

//----------------------------------------------------------------------------
QList<pqExpressionsManager::pqExpression> pqExpressionsTableModel::getExpressions()
{
  return this->Expressions;
}

//----------------------------------------------------------------------------
void pqExpressionsTableModel::setExpressions(
  const QList<pqExpressionsManager::pqExpression>& expressions)
{
  this->beginResetModel();
  this->Expressions.clear();
  this->endResetModel();

  QList<pqExpressionsManager::pqExpression> cleanExpressions;
  for (const pqExpressionsManager::pqExpression& expr : expressions)
  {
    if (!cleanExpressions.contains(expr))
    {
      cleanExpressions.push_back(expr);
    }
  }

  this->insertRows(0, cleanExpressions.size(), QModelIndex());
  this->Expressions = cleanExpressions;

  Q_EMIT this->dataChanged(
    this->index(0, 0), this->index(this->Expressions.size() - 1, ColumnKey::NumberOfColumns - 1));
}

//----------------------------------------------------------------------------
void pqExpressionsTableModel::addExpressions(
  const QList<pqExpressionsManager::pqExpression>& expressions)
{
  auto tmp = this->Expressions + expressions;
  this->setExpressions(tmp);
}

//----------------------------------------------------------------------------
void pqExpressionsTableModel::removeAllExpressions()
{
  this->beginResetModel();
  this->Expressions.clear();
  this->endResetModel();
}

//----------------------------------------------------------------------------
void pqExpressionsTableModel::removeExpressions(const QModelIndexList& indices)
{
  if (indices.empty())
  {
    return;
  }

  // use a set to get an ordered list of rows, so we can remove them from the last to the first
  std::set<int> rowsToRemove;
  for (const QModelIndex& idx : indices)
  {
    rowsToRemove.insert(idx.row());
  }

  for (auto riter = rowsToRemove.rbegin(); riter != rowsToRemove.rend(); ++riter)
  {
    this->beginRemoveRows(QModelIndex(), *riter, *riter);
    this->removeExpression(*riter);
    this->endRemoveRows();
  }
}

//----------------------------------------------------------------------------
bool pqExpressionsTableModel::setExpression(int idx, const QString& expression)
{
  if (this->Expressions.size() > idx)
  {
    this->Expressions[idx].Value = expression;
    // invalidate the whole row to ensure sort / filter is updated
    Q_EMIT this->dataChanged(this->index(idx, 0), this->index(idx, ColumnKey::NumberOfColumns - 1));
    return true;
  }

  qWarning() << "expression not found";
  return false;
}

//----------------------------------------------------------------------------
bool pqExpressionsTableModel::removeExpression(int idx)
{
  if (this->Expressions.size() > idx)
  {
    this->Expressions.removeAt(idx);
    return true;
  }

  qWarning() << "expression not found";
  return false;
}

//----------------------------------------------------------------------------
bool pqExpressionsTableModel::setExpressionName(int idx, const QString& name)
{
  if (this->Expressions.size() > idx)
  {
    this->Expressions[idx].Name = name;
    // invalidate the whole row to ensure sort / filter is updated
    Q_EMIT this->dataChanged(this->index(idx, 0), this->index(idx, ColumnKey::NumberOfColumns - 1));
    return true;
  }

  qWarning() << "expression not found";
  return false;
}

//----------------------------------------------------------------------------
bool pqExpressionsTableModel::setExpressionGroup(int idx, const QString& group)
{
  if (this->Expressions.size() > idx)
  {
    this->Expressions[idx].Group = group;
    // invalidate the whole row to ensure sort / filter is updated
    Q_EMIT this->dataChanged(this->index(idx, 0), this->index(idx, ColumnKey::NumberOfColumns - 1));
    return true;
  }

  qWarning() << "expression not found";
  return false;
}

//----------------------------------------------------------------------------
QSet<QString> pqExpressionsTableModel::getGroups()
{
  QSet<QString> groups;
  for (const pqExpressionsManager::pqExpression& expression : this->Expressions)
  {
    groups.insert(expression.Group);
  }

  return groups;
}

//----------------------------------------------------------------------------
QList<pqExpressionsManager::pqExpression> pqExpressionsTableModel::getExpressions(
  const QString& group)
{
  QList<pqExpressionsManager::pqExpression> expressions;
  for (const pqExpressionsManager::pqExpression& expression : this->Expressions)
  {
    if (expression.Group == group)
    {
      expressions << expression;
    }
  }

  return expressions;
}
